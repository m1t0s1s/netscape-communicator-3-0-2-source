/* -*- Mode: C; tab-width: 4; -*- */
/*******************************************************************************
 * Java Runtime Interface -- JDK Implementation
 * Copyright (c) 1996 Netscape Communications Corporation. All rights reserved.
 ******************************************************************************/

/*
** Define this first so that the first file that pulls in jri_md.h
** defines the GUIDs for JRI:
*/
#define INITGUID

#include "oobj.h"
#include "interpreter.h"
#include "javaThreads.h"
#include "exceptions.h"
#include "opcodes.h"
#include "java_lang_Throwable.h"
#include "java_lang_Thread.h"
#include "java_lang_ThreadGroup.h"
#include "java_lang_ClassLoader.h"
#include "jio.h"
#include "utf.h"
#if defined(NETSCAPE) && defined(XP_UNIX)
#include "prmon.h"
#endif

/*
** Don't include jriext.h first because we need to make sure the proper
** symbols are defined to get the right long long definitions. Since
** jri.h (and consequently jri.h and jri_md.h) are stand-alone headers,
** this doesn't happen automatically.
*/
#include "jriext.h"

#include <sys/stat.h>
#include <fcntl.h>

extern HClass* java_lang_Object_getClass(HObject *p); /* from object.c */

/*******************************************************************************
 * RefTable
 ******************************************************************************/

#define GLOBAL_TABLE_INITIAL_SIZE	256
#define GLOBAL_TABLE_INCREMENT_SIZE	256

typedef JHandle** RefCell;

static jbool JRI_CALLBACK
RefTable_init(RefTable* table)
{
    RefCell elements;
    elements = (RefCell)sysMalloc(GLOBAL_TABLE_INITIAL_SIZE
                                  * sizeof(JHandle*));
    if (elements == NULL)
        return JRIFalse;
	
    table->element = elements;
    table->size = GLOBAL_TABLE_INITIAL_SIZE;
    table->top = 0;
	
    return JRITrue;
}

/*
** JHandle* RefTable_deref(RefTable* table, jref ref)
**
** This dereferencing operator doesn't check for NULL.
*/
#define RefTable_deref(table, ref)		(((table)->element[((jsize)ref)-1]))

#define RefTable_refIsValid(table, ref)	\
	sysAssert(/*(uint32_t)(ref)-1 >= 0 &&*/ (uint32_t)(ref)-1 < (table)->top)
		
#ifdef CHECK_ON_PUSH
/* This version checks on every push, potentially growing the table */

/*
** jref RefTable_push(RefTable* table, JHandle handle) 
**
** Table should be a variable.
*/
#define RefTable_push(table, handle)     \
	((((table)->top >= (table)->size)    \
	  ? RefTable_grow(table) : JRITrue) \
	 ? RefTable_push1(table, handle)     \
     : NULL)                             \

/*
** jref RefTable_push1(RefTable* table, JHandle* handle) 
**
** This pushes without checking whether to grow the table. Table should
** be a variable.
*/
#define RefTable_push1(table, handle)           \
	(sysAssert((table)->top < (table)->size),   \
	 (table)->element[(table)->top] = (handle), \
	 (jref)(++(table)->top))                    \
    
static jbool JRI_CALLBACK
RefTable_grow(RefTable* table) 
{
	if (table->top >= table->size) {
		RefCell elements = (RefCell)
			sysRealloc(table->element, 
					   (table->size + GLOBAL_TABLE_INCREMENT_SIZE)
					   * sizeof(JHandle*));
		if (elements == NULL) {
			return JRIFalse;
		}
		table->element = elements;
		table->size += GLOBAL_TABLE_INCREMENT_SIZE;
	}
	return JRITrue;
}

#else /* !CHECK_ON_PUSH */

/*
** jref RefTable_push(RefTable* table, JHandle* handle) 
**
** This pushes without checking whether to grow the table. Table should
** be a variable.
*/
#define RefTable_push(table, handle)            \
	(sysAssert((table)->top < (table)->size),   \
	 (table)->element[(table)->top] = (handle), \
	 (jref)(++(table)->top))                    \
	
/*
** jbool RefTable_ensure(RefTable* table, jsize refCount)
**
** Table and count should be variables.
*/
#define RefTable_ensure(table, refCount)		  \
	(((table)->top + refCount >= (table)->size)	  \
	 ? RefTable_grow(table, refCount) : JRITrue) \

#ifndef MAX
#define MAX(x, y)	(((x) > (y)) ? (x) : (y))
#endif

static jbool JRI_CALLBACK
RefTable_grow(RefTable* table, jsize refCount) 
{
    if (table->top + refCount >= table->size) {
		RefCell elements = (RefCell)
			sysRealloc(table->element, 
					   (size_t)(table->size + 
						MAX(GLOBAL_TABLE_INCREMENT_SIZE, refCount))
					   * sizeof(JHandle*));
		if (elements == NULL) {
			return JRIFalse;
		}
		table->element = elements;
		table->size += GLOBAL_TABLE_INCREMENT_SIZE;
    }
    return JRITrue;
}

#endif  /* !CHECK_ON_PUSH */

/*
** JHandle* RefTable_pop(RefTable* table)
*/
#define RefTable_pop(table)		((table)->element[--(table)->top])
	
/*
** JHandle* RefTable_top(RefTable* table)
*/
#define RefTable_top(table)		((table)->element[(table)->top - 1])
	
/*
** jrefFrame RefTable_beginFrame(RefTable* table)
*/
#define RefTable_beginFrame(table)	((table)->top)
	
/*
** void RefTable_endFrame(RefTable* table, int32 frame)
*/
#define RefTable_endFrame(table, frame)	((table)->top = (frame))

#define EMPTY		((JHandle*)-1)	/* must not be == NULL */

static jref JRI_CALLBACK
RefTable_insert(RefTable* table, JHandle* handle)
{
    RefCell elements = table->element;
    uint32_t top = table->top;
    int32_t i;
    /* First, just push (if there's room). The table's allocated anyway. */
    if (top < table->size) {
#if CHECK_ON_PUSH
		return RefTable_push1(table, handle);
#else /* !CHECK_ON_PUSH */
		return RefTable_push(table, handle);
#endif /* !CHECK_ON_PUSH */
    }
    /*
    ** Next, look for an already deleted (empty) cell. Assume youngest
    ** die young.
    */
    for (i = (int32_t) top - 1; i >= 0; i--) {
		if (elements[i] == EMPTY) {
			elements[i] = handle;
			return (jref)i;
		}
    }
    /* Finally, if the table's completely full, let it grow. */
#if CHECK_ON_PUSH
    return RefTable_push(table, handle);
#else /* !CHECK_ON_PUSH */
    RefTable_ensure(table, 1);
    return RefTable_push(table, handle);
#endif /* !CHECK_ON_PUSH */
}

/*
** void RefTable_set(RefTable* table, RefCell ref, JHandle* value)
*/
#define RefTable_set(table, ref, value)	\
	(RefTable_refIsValid(table, ref), RefTable_deref(table, ref) = (value))

/*
** void RefTable_drop(RefTable* table, RefCell ref)
*/
#define RefTable_drop(table, ref)		RefTable_set(table, ref, EMPTY)

#if 0
static int
RefTable_isInTable(RefTable* table, RefCell ref)
{
    RefCell elements = table->element;
    RefCell index = (RefCell)ref;
    return elements <= index && index < (elements + table->top);
}
#endif

/*
** bool_t RefTable_isLocation(RefCell ref)
*/
#define RefTable_isLocation(ref)	((ref) != NULL)

/*
** JHandle* RefTable_get(RefTable* table, RefCell ref)
*/
#define RefTable_get(table, ref)									 \
	(RefTable_isLocation(ref)										 \
	 ? (RefTable_refIsValid(table, ref), RefTable_deref(table, ref)) \
	 : (JHandle*)(ref))												 \

#ifdef DEBUG
void
RefTable_describe(RefTable* refTable) 
{
	int i;
	fprintf(stderr, "RefTable 0x%p:\n", refTable);
	for (i = 0; i < refTable->top; i++) {
	    extern ClassClass* classJavaLangClass;
	    JHandle* h = refTable->element[i];
	    HClass* hc;
	    if (h == NULL || h == (JHandle*)-1) 
		continue;
	    hc = java_lang_Object_getClass(h);
	    if (unhand(hc) == classJavaLangClass) {
		fprintf(stderr,
			"\t%d. %s @ 0x%p (%s)\n", i, unhand(hc)->name, h,
			unhand((HClass*)h)->name);
	    }
	    else {
		fprintf(stderr,
			"\t%d. %s @ 0x%p\n", i, unhand(hc)->name, h);
	    }
	}
}
#endif /* DEBUG */

/*******************************************************************************
 * Manipulating Interfaces
 ******************************************************************************/

#define NaEnv2EE(env)		\
	((ExecEnv*)((char*)(env) - offsetof(ExecEnv, nativeInterface)))
#define RtEnv2EE(env)		\
	((ExecEnv*)((char*)(env) - offsetof(ExecEnv, runtimeInterface)))
#define RfEnv2EE(env)		\
	((ExecEnv*)((char*)(env) - offsetof(ExecEnv, reflectionInterface)))
#define DbEnv2EE(env)		\
	((ExecEnv*)((char*)(env) - offsetof(ExecEnv, debuggerInterface)))
#define CpEnv2EE(env)		\
	((ExecEnv*)((char*)(env) - offsetof(ExecEnv, compilerInterface)))
#define ExEnv2EE(env)		\
	((ExecEnv*)((char*)(env) - offsetof(ExecEnv, exprInterface)))

#define EE2NaEnv(ee)		(&(ee)->nativeInterface)
#define EE2RtEnv(ee)		(&(ee)->runtimeInterface)
#define EE2RfEnv(ee)		(&(ee)->reflectionInterface)
#define EE2DbEnv(ee)		(&(ee)->debuggerInterface)
#define EE2CpEnv(ee)		(&(ee)->compilerInterface)
#define EE2ExEnv(ee)		(&(ee)->exprInterface)

/*******************************************************************************
 * Local References
 ******************************************************************************/

#if 0 /* Last minute decision -- no non-conservative gc. */

#define JRIEnv_ref(env, handle)                  \
	RefTable_push(&Env2EE(env)->refStack, handle) \

#define JRIEnv_deref(env, ref)                             \
	((JHandle*)RefTable_get(&Env2EE(env)->refStack, ref)) \

#else

/* Local references in the jdk are just JHandles */

#define JRIEnv_ref(env, handle)        ((jref)handle)
#define JRIEnv_deref(env, ref)         ((JHandle*)ref)

#endif

/*******************************************************************************
 * General Stuff
 ******************************************************************************/

/* This implementation only allows one: */
extern const JRIRuntimeInterface jri_RuntimeInterface;
JRIRuntimeInstance jriRuntimeInstance = &jri_RuntimeInterface;
#define jriRuntime		&jriRuntimeInstance

extern ClassClass* classJavaLangClass;
extern ClassClass* classJavaLangString;

static ExecEnv _defaultEE;
static jbool jdkInitialized = (jbool)JRIFalse;
static ClassClass* classNullPointerException;
static ClassClass* classThrowable;

static ClassClass* JRI_CALLBACK
jri_Ref2Class(ExecEnv* ee, jref clazz)
{
    JHandle* handle = JRIEnv_deref(ee, clazz);
    if (handle == NULL) {
		SignalError(ee, JAVAPKG "NullPointerException", NULL);
		return NULL;
    }
	if (!is_instance_of(handle, classJavaLangClass, ee)) {
		SignalError(ee, JAVAPKG "IllegalAccessError",
					"native method specified something that wasn't a class");
		return NULL;
    }
    return unhand((HClass*)handle);
}

static struct fieldblock* JRI_CALLBACK
jri_FindFieldBlock(ExecEnv* ee, ClassClass* cb, hash_t fieldID, jsize *index)
{
	struct fieldblock** fields;
	struct fieldblock* fb = NULL;
    int i, nFields;

 	if (makeslottable(cb) == NULL)
		return NULL;
    nFields = cbSlotTableSize(cb);
	fields = cbSlotTable(cb);
    for (i = nFields - 1; i >= 0; i--) {
		fb = fields[i];
    	if (fb->ID == fieldID)
			break;
	}
	if (i == -1) {
		SignalError(ee, JAVAPKG "NoSuchFieldError",
					"native method specified an invalid field");
		return NULL;
    }
    else {
		if (index != NULL)
			*index = (jsize)i;
		return fb;
    }
}

#ifdef XP_MAC
#ifndef DEBUG
#pragma global_optimizer on 
#pragma optimization_level 2 
#endif
#endif /* XP_MAC */

static struct methodblock* JRI_CALLBACK
jri_GetMethodBlock(ExecEnv* ee, ClassClass* cb, jint methodID,
				   bool_t isStaticCall, bool_t isConstructor)
{
    hash_t id = (hash_t)methodID;
    int32_t i, nMethods;
    struct methodblock** methods;
    struct methodblock* mb = NULL;

	/* First search in the virtual method table: */
	if (!CCIs(cb, Resolved)) {
		char* detail;
		char* exception = ResolveClass(cb, &detail);
		if (exception != NULL) {
			SignalError(ee, exception, detail);
			return NULL;
		}
	}
    nMethods = cbMethodTableSize(cb);
    methods = cbMethodTable(cb)->methods;
    for (i = 0; i < nMethods; i++) {
		mb = methods[i];
#ifdef DEBUG
		if (getenv("METHOD_LOOKUP") != 0 && mb) {
			fprintf(stderr, "%d: %s.%s%s (%d)", id, classname(cb),
					mb->fb.name, mb->fb.signature, mb->fb.ID);
			if (mb != NULL && mb->fb.ID == id)
				fprintf(stderr, " => found");
			fprintf(stderr, "\n");
		}
#endif
		if (mb != NULL && mb->fb.ID == id)
			return mb;
	}
    if (i == nMethods) {
		/* If not found yet, search in this class' methods: */
		nMethods = cb->methods_count;
		mb = cb->methods;
		for (i = 0; i < nMethods; i++, mb++) {
#ifdef DEBUG
			if (getenv("METHOD_LOOKUP") != 0 && mb) {
				fprintf(stderr, "%di: %s.%s%s (%d)", id, classname(cb),
						mb->fb.name, mb->fb.signature, mb->fb.ID);
				if (mb != NULL && mb->fb.ID == id)
					fprintf(stderr, " => found");
				fprintf(stderr, "\n");
			}
#endif
			if (mb != NULL && mb->fb.ID == id)
				return mb;
		}
		/* not found anywhere */
		SignalError(ee, JAVAPKG "NoSuchMethodError",
					"native method specified an invalid method");
		return NULL;
    }
    return mb;
}

#ifdef XP_MAC
#ifndef DEBUG
#pragma global_optimizer reset 
#endif
#endif /* XP_MAC */

#if defined(NETSCAPE) && defined(XP_UNIX)

#define WITH_LOCKED_GUI_THREAD								\
	{														\
		int _xWasLocked_ = PR_InMonitor(_pr_rusty_lock);	\
		if (_xWasLocked_) PR_XUnlock();						\

#define END_WITH_LOCKED_GUI_THREAD				\
		if (_xWasLocked_) PR_XLock();			\
	}											\

#else 

#define WITH_LOCKED_GUI_THREAD 
#define END_WITH_LOCKED_GUI_THREAD

#endif

typedef union JRI_PushArgsType {
	va_list		v;
	JRIValue*	a;
} JRI_PushArgsType;

typedef char*
(*JRI_PushArguments_t)(ExecEnv* ee, char* method_signature, char* method_name,
					   JavaFrame *current_frame, JRI_PushArgsType args);

static char* JRI_CALLBACK
jri_PushArgumentsVararg(ExecEnv* ee, char* method_name, char* method_signature,
					   JavaFrame *current_frame, JRI_PushArgsType a)
{
    bool_t shortFloats = FALSE;
    char *p;
    Java8 tdub;
	va_list args = a.v;

    for (p = method_signature + 1; *p != SIGNATURE_ENDFUNC; p++) {
		switch (*p) {
		  case SIGNATURE_BOOLEAN:
		  case SIGNATURE_SHORT:
		  case SIGNATURE_BYTE:
		  case SIGNATURE_CHAR:
		  case SIGNATURE_INT:
			(current_frame->optop++)->i = va_arg(args, long);
			break;
		  case SIGNATURE_FLOAT:
			if (shortFloats) 
				(current_frame->optop++)->i = va_arg(args, long);
			else 
				(current_frame->optop++)->f = (float)va_arg(args, double);
			break;
		  case SIGNATURE_CLASS:
		  {
			  jref ref = va_arg(args, jref);	/* don't put va_arg in the macro */
			  (current_frame->optop++)->h = JRIEnv_deref(EE2NaEnv(ee), ref);
		  }
		  while (*p != SIGNATURE_ENDCLASS) p++;
		  break;
		  case SIGNATURE_ARRAY:
			while ((*p == SIGNATURE_ARRAY)) p++;
			if (*p == SIGNATURE_CLASS) 
	        { while (*p != SIGNATURE_ENDCLASS) p++; } 
			{
				jref ref = va_arg(args, jref);	/* don't put va_arg in the macro */
				(current_frame->optop++)->p = (void*)JRIEnv_deref(EE2NaEnv(ee), ref);
			}
			break;
		  case SIGNATURE_LONG:
			SET_INT64(tdub, current_frame->optop, va_arg(args, int64_t));
			current_frame->optop += 2;
			break;
		  case SIGNATURE_DOUBLE:
			SET_DOUBLE(tdub, current_frame->optop, va_arg(args, double));
			current_frame->optop += 2;
			break;
		  default:
			fprintf(stderr, "Invalid method signature '%s'\n", method_name);
			return NULL;
		}
    }
	return p;
}

static char* JRI_CALLBACK
jri_PushArgumentsArray(ExecEnv* ee, char* method_name, char* method_signature,
					   JavaFrame *current_frame, JRI_PushArgsType a)
{
    bool_t shortFloats = FALSE;
    char *p;
    JRI_JDK_Java8 tdub;
	JRIValue* args = a.a;

    for (p = method_signature + 1; *p != SIGNATURE_ENDFUNC; p++) {
		switch (*p) {
		  case SIGNATURE_BOOLEAN:
		  case SIGNATURE_SHORT:
		  case SIGNATURE_BYTE:
		  case SIGNATURE_CHAR:
		  case SIGNATURE_INT:
			(current_frame->optop++)->i = (*args++).i;
			break;
		  case SIGNATURE_FLOAT:
			if (shortFloats) 
				(current_frame->optop++)->i = (*args++).i;
			else 
				(current_frame->optop++)->f = (float)(*args++).d;
			break;
		  case SIGNATURE_CLASS:
		  {
			  jref ref = (*args++).r;	/* don't put this expr in the macro */
			  (current_frame->optop++)->h = JRIEnv_deref(EE2NaEnv(ee), ref);
		  }
		  while (*p != SIGNATURE_ENDCLASS) p++;
		  break;
		  case SIGNATURE_ARRAY:
			while ((*p == SIGNATURE_ARRAY)) p++;
			if (*p == SIGNATURE_CLASS) 
	        { while (*p != SIGNATURE_ENDCLASS) p++; } 
			{
				jref ref = (*args++).r;	/* don't put this expr in the macro */
				(current_frame->optop++)->p = (void*)JRIEnv_deref(EE2NaEnv(ee), ref);
			}
			break;
		  case SIGNATURE_LONG:
			JRI_SET_INT64(tdub, current_frame->optop, (*args++).l);
			current_frame->optop += 2;
			break;
		  case SIGNATURE_DOUBLE:
			JRI_SET_DOUBLE(tdub, current_frame->optop, (*args++).d);
			current_frame->optop += 2;
			break;
		  default:
			fprintf(stderr, "Invalid method signature '%s'\n", method_name);
			return NULL;
		}
    }
	return p;
}

extern bool_t ExecuteJava(unsigned char*, ExecEnv* ee);

static JRIValue JRI_CALLBACK
jri_Invoke(ExecEnv* ee, jref self, JRIMethodID methodID,
		   JRI_PushArguments_t pushArguments, JRI_PushArgsType args,
		   bool_t isStaticCall, bool_t isConstructor)
{
    JRIValue result;
    void* obj;
    ClassClass* cb;
    struct methodblock* mb;
    char* method_name;
    char* method_signature;
    JavaFrame *current_frame, *previous_frame;
    JavaStack *current_stack;
    char *p;

    /* zero result */
    result.l = jlong_ZERO;
    
    if (isStaticCall) {
		cb = jri_Ref2Class(ee, self);
		if (cb == NULL) return result;
		obj = cb;
    }
    else {
		obj = JRIEnv_deref(ee, self);
		if (obj == NULL) {
			SignalError(ee, JAVAPKG "NullPointerException", NULL);
			return result;
		}
		cb = obj_classblock((JHandle*)obj);
    }
    /*
    ** We have to look up the method block or we may get security
    ** violations when we invoke the method. The JRI can access any
    ** method.
    */
	sysAssert(methodID != JRIUninitialized);
    mb = jri_GetMethodBlock(ee, cb, methodID, isStaticCall, isConstructor);
    if (mb == NULL) {
		/* exception already raised */
		return result;
    }
    method_name = mb->fb.name;
    method_signature = mb->fb.signature;

    /*
    ** The following is stolen from do_execute_java_method_vararg, and
    ** tweaked to grab jrefs off the stack instead of JHandle*s
    */
    previous_frame = ee->current_frame;
    if (previous_frame == 0) {	
		/* bottommost frame on this Exec Env. */
		current_stack = ee->initial_stack;
		current_frame = (JavaFrame *)(current_stack->data); /* no vars */
    } else {
		int args_size = mb ? mb->args_size 
			/* We don't need the +1 if a static call, but it's
			 * okay to make args_size too big.  */
			: (Signature2ArgsSize(method_signature) + 1);
		current_stack = previous_frame->javastack; /* assume same stack */
		if (previous_frame->current_method) {
			int size = previous_frame->current_method->maxstack;
			current_frame = (JavaFrame *)(&previous_frame->ostack[size]);
		} else {
			/* The only frames that don't have a mb are pseudo frames like 
			 * this one and they don't really touch their stack. */
			current_frame = (JavaFrame *)(previous_frame->optop + 3);
		}
		if (current_frame->ostack + args_size > current_stack->end_data) {
			/* Ooops.  The current stack isn't big enough.  */
			if (current_stack->next != 0) {
				current_stack = current_stack->next;
			} else {
				current_stack = CreateNewJavaStack(ee, current_stack);
				if (current_stack == 0) {
					SignalError(ee, JAVAPKG "OutOfMemoryError", 0);
					return result;
				}
			}	
			current_frame = (JavaFrame *)(current_stack->data); /* no vars */
		} 
    }
    ee->current_frame = current_frame;

    current_frame->prev = previous_frame;
    current_frame->javastack = current_stack;
    current_frame->optop = current_frame->ostack;
    current_frame->vars = 0;	/* better not reference any! */
    current_frame->monitor = 0;	/* not monitoring anything */
    current_frame->annotation = 0;
    current_frame->current_method = 0; /* not part of a method. */
    
    /* Push the target object, if not a static call */
    if (!isStaticCall) 
        (current_frame->optop++)->p = obj;
	
	p = pushArguments(ee, method_name, method_signature, current_frame, args);
	if (p == NULL)
		return result;	/* argument error */

    {
		ClassClass *cb = (isStaticCall || mb != 0) ? obj 
			: obj_array_classblock((JHandle *)obj);
		unsigned char pc[6];
		cp_item_type  constant_pool[10];
		unsigned char cpt[10];
		bool_t isInitializer = (method_name[0] == '<'  &&  /* fast test */
								!isStaticCall && 
								!strcmp(method_name, "<init>"));
	
		constant_pool[CONSTANT_POOL_TYPE_TABLE_INDEX].p = cpt;
		cpt[0] = CONSTANT_POOL_ENTRY_RESOLVED;

		if (isStaticCall || mb) {
			pc[0] = isStaticCall ? (mb ? opc_invokestatic_quick 
									: opc_invokestatic) 
				: opc_invokenonvirtual_quick;
			pc[1] = 0; pc[2] = 1;	/* constant pool entry #1 */
			pc[3] = opc_return;
		} else {
			int nargs = (current_frame->optop - current_frame->ostack);
			pc[0] = isInitializer ? opc_invokenonvirtual 
				: opc_invokeinterface;
			pc[1] = 0; pc[2] = 1;	/* constant pool entry #1 */
			pc[3] = nargs;
			pc[4] = 0;			/* padding needed for guess */
			pc[5] = opc_return;
		}

		if (mb) {
			constant_pool[1].p = mb;
			cpt[1] = CONSTANT_POOL_ENTRY_RESOLVED | CONSTANT_Methodref;
		} else {
			constant_pool[1].i = (2 << 16) + 3; /* Class, NameAndType */
			cpt[1] = (isStaticCall || isInitializer)
		        ? CONSTANT_Methodref : CONSTANT_InterfaceMethodref; 
			constant_pool[2].p = cb; /* class */
			cpt[2] = CONSTANT_POOL_ENTRY_RESOLVED | CONSTANT_Class;
			constant_pool[3].i = (4 << 16) + 5;	/* Name, Type */
			cpt[3] = CONSTANT_NameAndType;
			constant_pool[4].cp = method_name;
			cpt[4] = CONSTANT_POOL_ENTRY_RESOLVED | CONSTANT_Utf8;
			constant_pool[5].cp = method_signature;
			cpt[5] = CONSTANT_POOL_ENTRY_RESOLVED | CONSTANT_Utf8;	    
		}

		current_frame->constant_pool = constant_pool;

		WITH_LOCKED_GUI_THREAD {

		/* Run the byte codes outside the lock and catch any exceptions. */
		ee->exceptionKind = EXCKIND_NONE;
		if (ExecuteJava(pc, ee)) {
			switch (p[1]) {
			  case SIGNATURE_VOID:
				break;
			  case SIGNATURE_BOOLEAN:
				result.z = (jbool)current_frame->optop[-1].i;
				break;
			  case SIGNATURE_BYTE:
				result.b = (jbyte)current_frame->optop[-1].i;
				break;
			  case SIGNATURE_CHAR:
				result.c = (jchar)current_frame->optop[-1].i;
				break;
			  case SIGNATURE_SHORT:
				result.s = (jshort)current_frame->optop[-1].i;
				break;
			  case SIGNATURE_LONG: {
				JRI_JDK_Java8 tmp;
				result.l = JRI_GET_INT64(tmp, &current_frame->optop[-2].i);
				break;
			  }
			  case SIGNATURE_DOUBLE: {
				JRI_JDK_Java8 tmp;
				result.d = JRI_GET_DOUBLE(tmp, &current_frame->optop[-2].i);
				break;
			  }
			  default:	/* int, object and array */
				result.i = (jint)current_frame->optop[-1].i;
				break;
			}
		}
		/* Our caller can look at ee->exceptionKind and ee->exception. */
		ee->current_frame = previous_frame;

		} END_WITH_LOCKED_GUI_THREAD;
    }
    return result;
}

/*******************************************************************************
 * MCOM
 ******************************************************************************/

JRI_DEFINE_GUID(JRINativePkgID, 0xb79cd9f0, 0x8c2f, 0x11cf);
JRI_DEFINE_GUID(JRIRuntimePkgID, 0xb8342180, 0x8c2f, 0x11cf);
JRI_DEFINE_GUID(JRIReflectionPkgID, 0xb89c0a40, 0x8c2f, 0x11cf);
JRI_DEFINE_GUID(JRIDebuggerPkgID, 0xb8fac7e0, 0x8c2f, 0x11cf);
JRI_DEFINE_GUID(JRICompilerPkgID, 0xb9567720, 0x8c2f, 0x11cf);
JRI_DEFINE_GUID(JRIExprPkgID, 0xb9af1800, 0x8c2f, 0x11cf);

static struct java_lang_Object* JRI_CALLBACK
jri_GetInterface(ExecEnv* self, jint interfaceID)
{
	GUID* id = (GUID*)interfaceID;
	if (memcmp(id, &JRINativePkgID, sizeof(GUID)) == 0)
		return (struct java_lang_Object*)&self->nativeInterface;
	else if (memcmp(id, &JRIRuntimePkgID, sizeof(GUID)) == 0)
		return (struct java_lang_Object*)&self->runtimeInterface;
	else if (memcmp(id, &JRIReflectionPkgID, sizeof(GUID)) == 0)
		return (struct java_lang_Object*)&self->reflectionInterface;
	else if (memcmp(id, &JRIDebuggerPkgID, sizeof(GUID)) == 0)
		return (struct java_lang_Object*)&self->debuggerInterface;
	else if (memcmp(id, &JRICompilerPkgID, sizeof(GUID)) == 0)
		return (struct java_lang_Object*)&self->compilerInterface;
	else if (memcmp(id, &JRIExprPkgID, sizeof(GUID)) == 0)
		return (struct java_lang_Object*)&self->exprInterface;
	else
		return NULL;
}

static long JRI_CALLBACK
jri_AddRef(void* self)
{
	return 0;	/* ExecEnvs are not reference counted (yet) */
}

static long JRI_CALLBACK
jri_Release(void* self)
{
	return 0;	/* ExecEnvs are not reference counted (yet) */
}

/*******************************************************************************
 * Environments
 ******************************************************************************/

typedef void	JavaObject; 

static JavaObject* JRI_CALLBACK
jri_NativeGetInterface(JRIEnv* self, jint id)
{
	return jri_GetInterface(NaEnv2EE(self), id);
}

/* !!! This is a non-jump-table entry point: */
JRI_PUBLIC_API(const JRIEnvInterface**)
JRI_GetCurrentEnv(void)
{
	if (!jdkInitialized)
		return NULL;
	return EE2NaEnv(EE());
}

/* Defining Classes */

static struct java_lang_Class* JRI_CALLBACK
jri_DefineClass(JRIEnv* env, jint op, struct java_lang_ClassLoader* classLoader,
				jbyte* buf, jsize bufLen)
{
	ExecEnv* ee = NaEnv2EE(env);
    ClassClass* cb;
    char* ename;
    char* detail;

    java_lang_ClassLoader_checkInitialized((struct Hjava_lang_ClassLoader*)
										   classLoader);
    if (exceptionOccurred(ee))
		return NULL;

    cb = AllocClass();
    if (cb == NULL) {
		SignalError(ee, JAVAPKG "OutOfMemoryError", NULL);
		return NULL;
    }

    cb->loader = (HObject*)classLoader;
	lock_classes();
    if (!createInternalClass((unsigned char*)buf,
							 (unsigned char*)buf + bufLen,
							 cb)) {
		if (!exceptionOccurred(ee))
			SignalError(ee, JAVAPKG "ClassFormatError", NULL);
		unlock_classes();
        return NULL;
    }
    AddBinClass(cb);
    /*
    ** Clear the CCF_IsResolving bit for the class since it is fully 
    ** initialized now...
    */
    sysAssert(CCIs(cb, Resolving));
    CCClear(cb, Resolving);
	unlock_classes();

    /*
    ** InitializeClass will lock the class and initialize it if
    ** necessary. Note that we need to hold the CCF_Locked across the
    ** call to prevent the class unloader from unloading this currently
    ** unreferenced class!
    */
    if ((ename = InitializeClass(cb, &detail)) != 0) {
		SignalError(ee, ename, detail);
		return 0;
    }
    return JRIEnv_ref(ee, (HObject*)cbHandle(cb));
}

static struct java_lang_Class* JRI_CALLBACK
jri_FindClass(JRIEnv* env, jint op, const char* name)
{
	ExecEnv* ee = NaEnv2EE(env);
    ClassClass* cb;
	
    cb = FindClass(ee, (char*)name, TRUE);
    if (cb != NULL)
    	return JRIEnv_ref(ee, (HObject*)cbHandle(cb));
    else
    	return NULL;
}

/* Working with Exceptions */

static void JRI_CALLBACK
jri_Throw(JRIEnv* env, jint op, struct java_lang_Throwable* obj)
{
	ExecEnv* ee = NaEnv2EE(env);
    HObject* ret;

    ret = JRIEnv_deref(ee, obj);
    if (!is_instance_of(ret, classThrowable, ee)) {
		SignalError(ee, JAVAPKG "IllegalAccessError", 
					"object is not an instance of java.lang.Throwable");
		return;
    }
    fillInStackTrace((struct Hjava_lang_Throwable*)ret, ee);
    exceptionThrow(ee, ret);
}

static void JRI_CALLBACK
jri_ThrowNew(JRIEnv* env, jint op, struct java_lang_Class* clazz, const char* message)
{
	ExecEnv* ee = NaEnv2EE(env);
    ClassClass* cb;
    
    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return;
    if (!is_subclass_of(cb, classThrowable, ee)) {
		SignalError(ee, JAVAPKG "IllegalAccessError", 
					"class is not a subclass of java.lang.Throwable");
		return;
    }
    SignalError1(ee, cb, (char*)message);
}

static struct java_lang_Throwable* JRI_CALLBACK
jri_ExceptionOccurred(JRIEnv* env, jint op)
{
	ExecEnv* ee = NaEnv2EE(env);
    if (exceptionOccurred(ee))
		return (struct java_lang_Throwable*)
			JRIEnv_ref(ee, ee->exception.exc);
    else
		return NULL;
}

#if 0
static void
jri_ExceptionDescribe(JRIEnv* env, jint op, JavaWriterProc writer,
					   void* dest, jsize destLen)
{
	ExecEnv* ee = NaEnv2EE(env);
    exception_t exc;
    
    ee = Env2EE(env);
    exc = ee->exception.exc;
    if (!is_instance_of(exc, classJavaLangThreadDeath, ee)
		&& is_instance_of(exc, classJavaLangThrowable, ee)) {
		(void)printStackTrace((struct Hjava_lang_Throwable*)exc,
							  (StackPrinter)writer, dest, destLen);
    }
}
#else
/*
** Describe to stderr -- yuck!
*/
static void JRI_CALLBACK
jri_ExceptionDescribe(JRIEnv* env, jint op)
{
	ExecEnv* ee = NaEnv2EE(env);
	WITH_LOCKED_GUI_THREAD {
		exceptionDescribe(ee);
	} END_WITH_LOCKED_GUI_THREAD;
}
#endif

static void JRI_CALLBACK
jri_ExceptionClear(JRIEnv* env, jint op)
{
	ExecEnv* ee = NaEnv2EE(env);
    exceptionClear(ee);
}

/*******************************************************************************
 * References
 ******************************************************************************/

/*
** jglobal is equivalent to RefCell (JHandle**), whereas
** jref is equivalent to JHandle* 
*/

static jglobal JRI_CALLBACK
jri_NewGlobalRef(JRIEnv* env, jint op, JavaObject* ref)
{
	ExecEnv* ee = NaEnv2EE(env);
    jint result;
	RefTable* globalRefs;

	globalRefs = &ee->globalRefs;
	
	if (globalRefs->element == NULL) {
		if (!RefTable_init(globalRefs)) {
			SignalError(ee, JAVAPKG "OutOfMemory", NULL);
			return 0;
		}
	}

    result = (jint)RefTable_insert(globalRefs, JRIEnv_deref(ee, ref));
    if (result == 0) {
		SignalError(ee, JAVAPKG "OutOfMemory", NULL);
	}
    return (jglobal)result;
}

static void JRI_CALLBACK
jri_DisposeGlobalRef(JRIEnv* env, jint op, jglobal ref)
{
	ExecEnv* ee = NaEnv2EE(env);
	if (ref == 0) {
		SignalError(ee, JAVAPKG "NullPointerException", NULL);
		return;
	}
	RefTable_drop(&ee->globalRefs, (RefCell)ref);
}

static JavaObject* JRI_CALLBACK
jri_GetGlobalRef(JRIEnv* env, jint op, jglobal globalRef)
{
	ExecEnv* ee = NaEnv2EE(env);
    jref result;
    result = JRIEnv_ref(ee, RefTable_get(&ee->globalRefs,
										 (RefCell)globalRef));
    return result;
}

static void JRI_CALLBACK
jri_SetGlobalRef(JRIEnv* env, jint op, jglobal globalRef, JavaObject* value)
{
	ExecEnv* ee = NaEnv2EE(env);
	if (globalRef == NULL) {
		SignalError(ee, JAVAPKG "NullPointerException", NULL);
		return;
	}
	RefTable_set(&ee->globalRefs, (RefCell)globalRef,
				 JRIEnv_deref(ee, value));
}

static jbool JRI_CALLBACK
jri_IsSameObject(JRIEnv* env, jint op, JavaObject* r1, JavaObject* r2)
{
    return JRIEnv_deref(NaEnv2EE(env), r1) == JRIEnv_deref(NaEnv2EE(env), r2);
}

/*******************************************************************************
 * Identifiers
 ******************************************************************************/

static jint
jri_GetMethodID(JRIEnv* env, jint op, struct java_lang_Class* clazz, 
				const char* name, const char* sig)
{
	ExecEnv* ee = NaEnv2EE(env);
    ClassClass* cb;
    hash_t id;
    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) 
		return -1;

    /* Passing in cb has the effect of never failing a call because
     * the ClassLoaders of the caller and cb are different.
     * Users of the JRI are responsible for making sure this is typesafe. */
	id = NameAndTypeToHash((char*)name, (char*)sig, cb);
	return (jint)id;
}

/*******************************************************************************
 * Object Operations
 ******************************************************************************/

static JavaObject* JRI_CALLBACK
jri_Construct(JRIEnv* env, struct java_lang_Class* clazz, jint methodID,
			  JRI_PushArguments_t pushArguments, JRI_PushArgsType args)
{
	ExecEnv* ee = NaEnv2EE(env);
    HObject* obj;
    ClassClass* cb;
    JavaObject* result;

    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return NULL;
    
    /* Find the class.  Make sure we can allocate an object of that class */
    if (cbAccess(cb) & (ACC_INTERFACE | ACC_ABSTRACT)) {
		SignalError(ee, JAVAPKG "InstantiationException",
					classname(cb));
        return NULL;
    }

    /* Allocate the object and call the constructor */
	obj = newobject(cb, 0, ee);
    if (obj == NULL) { 
		SignalError(ee, JAVAPKG "OutOfMemory", NULL);
        return NULL;
    }
    result = (JavaObject*)JRIEnv_ref(ee, obj);
    jri_Invoke(ee, result, methodID, pushArguments, args, FALSE, TRUE);
    return result;
}

static JavaObject* JRI_CALLBACK
jri_NewObjectA(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint methodID, JRIValue* args)
{
	JRI_PushArgsType a;
	a.a = args;
	return jri_Construct(env, clazz, methodID,
						 (JRI_PushArguments_t)jri_PushArgumentsArray, a);
}

static JavaObject* JRI_CALLBACK
jri_NewObjectV(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint methodID, va_list args)
{
	JRI_PushArgsType a;
	a.v = args;
	return jri_Construct(env, clazz, methodID,
						 (JRI_PushArguments_t)jri_PushArgumentsVararg, a);
}

static JavaObject* JRI_CALLBACK
jri_NewObject(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint methodID, ...)
{
    jref result;
    va_list args;
	JRI_PushArgsType a;
    va_start(args, methodID);
	a.v = args;
	result = jri_Construct(env, clazz, methodID,
						   (JRI_PushArguments_t)jri_PushArgumentsVararg, a);
    va_end(args);
    return result;
}

static struct java_lang_Class* JRI_CALLBACK
jri_GetObjectClass(JRIEnv* env, jint op, JavaObject* obj)
{
	JHandle* o = JRIEnv_deref(NaEnv2EE(env), obj);
	HClass* hc = java_lang_Object_getClass(o);
	return JRIEnv_ref(NaEnv2EE(env), (HObject*)hc);
}

static jbool JRI_CALLBACK
jri_IsInstanceOf(JRIEnv* env, jint op, JavaObject* obj,
				 struct java_lang_Class* clazz)
{
	ExecEnv* ee = NaEnv2EE(env);
    ClassClass* cb;

    cb = jri_Ref2Class(ee, clazz);
    return cb != NULL
        && is_instance_of(JRIEnv_deref(ee, obj), cb, ee);
}

#if 0
static void
jri_PrintObject(JRIEnv* env, jint op, JavaObject* obj,
				JavaWriterProc writer, void* dest, jsize destLen)
{
	ExecEnv* ee = NaEnv2EE(env);
    char* str;
    int len;

    str = Object2CString(JRIEnv_deref(ee, obj));	/* static buffer */
    len = strlen(str);
    (void)writer(env, dest, destLen, str, len);
}
#endif

/******************************************************************************/
/* Accessing Fields by Index */

static jint
jri_GetFieldID(JRIEnv* env, jint op, struct java_lang_Class* clazz, 
			   const char* name, const char* sig)
{
	ExecEnv* ee = NaEnv2EE(env);
	hash_t id;
	jsize index;
	ClassClass* cb;
	struct fieldblock* fb;
	cb = jri_Ref2Class(ee, clazz);
	if (cb == NULL) 
		return -1;

    /* Passing in cb has the effect of never failing a call because
     * the ClassLoaders of the caller and cb are different.
     * Users of the JRI are responsible for making sure this is typesafe. */
	id = NameAndTypeToHash((char*)name, (char*)sig, cb);
	fb = jri_FindFieldBlock(ee, cb, id, &index);
	if (fb == NULL)
		return -1;
	if (fb->access & ACC_STATIC) {
		SignalError(ee, JAVAPKG "NoSuchFieldError",
					"static field accessed with non-static accessor");
		return -1;
	}
	return (jint)fb->u.offset;
}

#define obj_Getoffsetaddr(o, off) ((OBJECT*)((char *)unhand(o)+(off)))
#define obj_Getoffset(o, off)	  (*obj_Getoffsetaddr(o, off))
#define obj_Setoffset(o, off, v)  (*obj_Getoffsetaddr(o, off) = (v))

static JavaObject* JRI_CALLBACK
jri_GetField_ref(JRIEnv* env, jint op, JavaObject* obj, jint fieldID)
{
    JHandle* o;
    JHandle* value;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
	value = (JHandle*)obj_Getoffset(o, fieldID);
    return (JavaObject*)JRIEnv_ref(NaEnv2EE(env), value);
}

static jbool JRI_CALLBACK
jri_GetField_boolean(JRIEnv* env, jint op, JavaObject* obj, jint fieldID)
{
    JHandle* o;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    return (jbool)obj_Getoffset(o, fieldID);
}

static jbyte JRI_CALLBACK
jri_GetField_byte(JRIEnv* env, jint op, JavaObject* obj, jint fieldID)
{
    JHandle* o;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    return (jbyte)obj_Getoffset(o, fieldID);
}

static jchar JRI_CALLBACK
jri_GetField_char(JRIEnv* env, jint op, JavaObject* obj, jint fieldID)
{
    JHandle* o;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    return (jchar)obj_Getoffset(o, fieldID);
}

static jshort JRI_CALLBACK
jri_GetField_short(JRIEnv* env, jint op, JavaObject* obj, jint fieldID)
{
    JHandle* o;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    return (jshort)obj_Getoffset(o, fieldID);
}

static jint JRI_CALLBACK
jri_GetField_int(JRIEnv* env, jint op, JavaObject* obj, jint fieldID)
{
    JHandle* o;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    return (jint)obj_Getoffset(o, fieldID);
}

static jlong JRI_CALLBACK
jri_GetField_long(JRIEnv* env, jint op, JavaObject* obj, jint fieldID)
{
    JHandle* o;
	OBJECT* slot;
    JRI_JDK_Java8 tmp;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    slot = obj_Getoffsetaddr(o, fieldID);
    return JRI_GET_INT64(tmp, slot);
}

static jfloat JRI_CALLBACK
jri_GetField_float(JRIEnv* env, jint op, JavaObject* obj, jint fieldID)
{
    JHandle* o;
	OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    slot = obj_Getoffsetaddr(o, fieldID);
    return *(jfloat*)slot;
}

static jdouble JRI_CALLBACK
jri_GetField_double(JRIEnv* env, jint op, JavaObject* obj, jint fieldID)
{
    JHandle* o;
	OBJECT* slot;
    JRI_JDK_Java8 tmp;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    slot = obj_Getoffsetaddr(o, fieldID);
    return JRI_GET_DOUBLE(tmp, slot);
}

/******************************************************************************/

static void JRI_CALLBACK
jri_SetField_ref(JRIEnv* env, jint op, JavaObject* obj, jint fieldID, 
				 JavaObject* value)
{
    JHandle* o = JRIEnv_deref(NaEnv2EE(env), obj);
    JHandle* v = JRIEnv_deref(NaEnv2EE(env), value);
    obj_Setoffset(o, fieldID, (OBJECT)v);
}

static void JRI_CALLBACK
jri_SetField_boolean(JRIEnv* env, jint op, JavaObject* obj, jint fieldID,
					  jbool value)
{
    JHandle* o;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    obj_Setoffset(o, fieldID, value);
}

static void JRI_CALLBACK
jri_SetField_byte(JRIEnv* env, jint op, JavaObject* obj, jint fieldID,
				   jbyte value)
{
    JHandle* o;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    obj_Setoffset(o, fieldID, value);
}

static void JRI_CALLBACK
jri_SetField_char(JRIEnv* env, jint op, JavaObject* obj, jint fieldID,
				   jchar value)
{
    JHandle* o;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    obj_Setoffset(o, fieldID, value);
}

static void JRI_CALLBACK
jri_SetField_short(JRIEnv* env, jint op, JavaObject* obj, jint fieldID,
					jshort value)
{
    JHandle* o;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    obj_Setoffset(o, fieldID, value);
}

static void JRI_CALLBACK
jri_SetField_int(JRIEnv* env, jint op, JavaObject* obj, jint fieldID,
				  jint value)
{
    JHandle* o;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    obj_Setoffset(o, fieldID, value);
}

static void JRI_CALLBACK
jri_SetField_long(JRIEnv* env, jint op, JavaObject* obj, jint fieldID,
				   jlong value)
{
    JHandle* o;
	OBJECT* slot;
    JRI_JDK_Java8 tmp;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    slot = obj_Getoffsetaddr(o, fieldID);
    JRI_SET_INT64(tmp, slot, value);
}

static void JRI_CALLBACK
jri_SetField_float(JRIEnv* env, jint op, JavaObject* obj, jint fieldID,
					jfloat value)
{
    JHandle* o;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    obj_Setoffset(o, fieldID, (OBJECT)value);
}

static void JRI_CALLBACK
jri_SetField_double(JRIEnv* env, jint op, JavaObject* obj, jint fieldID,
					 jdouble value)
{
    JHandle* o;
	OBJECT* slot;
    Java8 tmp;
	sysAssert(fieldID != JRIUninitialized);
	o = JRIEnv_deref(NaEnv2EE(env), obj);
    slot = obj_Getoffsetaddr(o, fieldID);
    SET_DOUBLE(tmp, slot, value);
}

/******************************************************************************/

/* Calling Methods */

#define DEFINE_CALLMETHOD(ResultType, suffix, unionType)				 \
static ResultType JRI_CALLBACK														 \
jri_CallMethod##suffix(JRIEnv* env, jint op, JavaObject* obj,		 \
						 jint methodID, ...)							 \
{																		 \
	ExecEnv* ee = NaEnv2EE(env);										 \
	ResultType result;													 \
	va_list args;														 \
	JRI_PushArgsType a;													 \
																		 \
	va_start(args, methodID);											 \
	a.v = args;															 \
	result = jri_Invoke(ee, obj, methodID,								 \
						(JRI_PushArguments_t)jri_PushArgumentsVararg, a, \
						FALSE, FALSE).unionType;						 \
	va_end(args);														 \
	return result;														 \
}																		 \
																		 \
static ResultType JRI_CALLBACK														 \
jri_CallMethod##suffix##V(JRIEnv* env, jint op, JavaObject* obj,	 \
						  jint methodID, va_list args)					 \
{																		 \
	ExecEnv* ee = NaEnv2EE(env);										 \
	ResultType result;													 \
	JRI_PushArgsType a;													 \
	a.v = args;															 \
																		 \
	result = jri_Invoke(ee, obj, methodID,								 \
						(JRI_PushArguments_t)jri_PushArgumentsVararg, a, \
						FALSE, FALSE).unionType;						 \
	return result;														 \
}																		 \
																		 \
static ResultType JRI_CALLBACK														 \
jri_CallMethod##suffix##A(JRIEnv* env, jint op, JavaObject* obj,	 \
						  jint methodID, JRIValue* args)				 \
{																		 \
	ExecEnv* ee = NaEnv2EE(env);										 \
	ResultType result;													 \
	JRI_PushArgsType a;													 \
	a.a = args;															 \
	result = jri_Invoke(ee, obj, methodID,								 \
						(JRI_PushArguments_t)jri_PushArgumentsArray, a,	 \
						FALSE, FALSE).unionType;						 \
	return result;														 \
}																		 \

DEFINE_CALLMETHOD(JavaObject*,	_ref, r)
DEFINE_CALLMETHOD(jbool,					_boolean, z)
DEFINE_CALLMETHOD(jbyte,					_byte, b)
DEFINE_CALLMETHOD(jchar,					_char, c)
DEFINE_CALLMETHOD(jshort,					_short, s)
DEFINE_CALLMETHOD(jint,						_int, i)
DEFINE_CALLMETHOD(jlong,					_long, l)
DEFINE_CALLMETHOD(jfloat,					_float, f)
DEFINE_CALLMETHOD(jdouble,					_double, d)

#define DEFINE_CALLSTATICMETHOD(ResultType, suffix, unionType)				\
static ResultType JRI_CALLBACK															\
jri_CallStaticMethod##suffix(JRIEnv* env, jint op, struct java_lang_Class* clazz,	\
							   jint methodID, ...)							\
{																			\
	ExecEnv* ee = NaEnv2EE(env);											\
	ResultType result;														\
	va_list args;															\
	JRI_PushArgsType a;														\
	va_start(args, methodID);												\
	a.v = args;																\
	result = jri_Invoke(ee, clazz, methodID,								\
						(JRI_PushArguments_t)jri_PushArgumentsVararg, a,	\
						TRUE, FALSE).unionType;								\
	va_end(args);															\
	return result;															\
}																			\
																			\
static ResultType JRI_CALLBACK															\
jri_CallStaticMethod##suffix##V(JRIEnv* env, jint op, struct java_lang_Class* clazz,	\
								jint methodID, va_list args)				\
{																			\
	ExecEnv* ee = NaEnv2EE(env);											\
	ResultType result;														\
	JRI_PushArgsType a;														\
	a.v = args;																\
	result = jri_Invoke(ee, clazz, methodID,								\
						(JRI_PushArguments_t)jri_PushArgumentsVararg, a,	\
						TRUE, FALSE).unionType;								\
	return result;															\
}																			\
																			\
static ResultType JRI_CALLBACK															\
jri_CallStaticMethod##suffix##A(JRIEnv* env, jint op, struct java_lang_Class* clazz,	\
								jint methodID, JRIValue* args)				\
{																			\
	ExecEnv* ee = NaEnv2EE(env);											\
	ResultType result;														\
	JRI_PushArgsType a;														\
	a.a = args;																\
	result = jri_Invoke(ee, clazz, methodID,								\
						(JRI_PushArguments_t)jri_PushArgumentsArray, a,		\
						TRUE, FALSE).unionType;								\
	return result;															\
}																			\

DEFINE_CALLSTATICMETHOD(JavaObject*,	_ref, r)
DEFINE_CALLSTATICMETHOD(jbool,						_boolean, z)
DEFINE_CALLSTATICMETHOD(jbyte,						_byte, b)
DEFINE_CALLSTATICMETHOD(jchar,						_char, c)
DEFINE_CALLSTATICMETHOD(jshort,						_short, s)
DEFINE_CALLSTATICMETHOD(jint,						_int, i)
DEFINE_CALLSTATICMETHOD(jlong,						_long, l)
DEFINE_CALLSTATICMETHOD(jfloat,						_float, f)
DEFINE_CALLSTATICMETHOD(jdouble,					_double, d)

/*******************************************************************************
 * Class Operations
 ******************************************************************************/

static jbool JRI_CALLBACK
jri_IsSubclassOf(JRIEnv* env, jint op, struct java_lang_Class* clazz, 
				 struct java_lang_Class* super)
{
	ExecEnv* ee = NaEnv2EE(env);
    ClassClass* cb;
    ClassClass* scb;

    cb = jri_Ref2Class(ee, clazz);
    scb = jri_Ref2Class(ee, super);
    return cb != NULL
        && scb != NULL
        && is_subclass_of(cb, scb, ee);
}

/* Accessing Static Fields */

static jint JRI_CALLBACK
jri_GetStaticFieldID(JRIEnv* env, jint op, struct java_lang_Class* clazz, 
					 const char* name, const char* sig)
{
	ExecEnv* ee = NaEnv2EE(env);
	hash_t id;
	jsize index;
	ClassClass* cb;
	struct fieldblock* fb;
	cb = jri_Ref2Class(ee, clazz);
	if (cb == NULL) 
		return -1;

    /* Passing in cb has the effect of never failing a call because
     * the ClassLoaders of the caller and cb are different.
     * Users of the JRI are responsible for making sure this is typesafe. */
	id = NameAndTypeToHash((char*)name, (char*)sig, cb);
	fb = jri_FindFieldBlock(ee, cb, id, &index);
	if (fb == NULL)
		return -1;
	if (!(fb->access & ACC_STATIC)) {
		SignalError(ee, JAVAPKG "NoSuchFieldError",
					"non-static field accessed with static accessor");
		return -1;
	}
	return (jint)((sig[0] == SIGNATURE_LONG || sig[0] == SIGNATURE_DOUBLE)
				  ? (void*)twoword_static_address(fb)
				  : (void*)normal_static_address(fb));
}

static JavaObject* JRI_CALLBACK
jri_GetStaticField_ref(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID)
{
    OBJECT* slot;
	JHandle* value;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
	value = (JHandle*)*slot;
    return (JavaObject*)JRIEnv_ref(ee, value);
}

static jbool JRI_CALLBACK
jri_GetStaticField_boolean(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID)
{
    OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    return (jbool)*slot;
}

static jbyte JRI_CALLBACK
jri_GetStaticField_byte(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID)
{
    OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    return (jbyte)*slot;
}

static jchar JRI_CALLBACK
jri_GetStaticField_char(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID)
{
    OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    return (jchar)*slot;
}

static jshort JRI_CALLBACK
jri_GetStaticField_short(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID)
{
    OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    return (jshort)*slot;
}

static jint JRI_CALLBACK
jri_GetStaticField_int(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID)
{
    OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    return *slot;
}

static jlong JRI_CALLBACK
jri_GetStaticField_long(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID)
{
    OBJECT* slot;
    JRI_JDK_Java8 tmp;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    return JRI_GET_INT64(tmp, slot);
}

static jfloat JRI_CALLBACK
jri_GetStaticField_float(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID)
{
    OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    return *(float*)slot;
}

static jdouble JRI_CALLBACK
jri_GetStaticField_double(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID)
{
    OBJECT* slot;
    JRI_JDK_Java8 tmp;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    return JRI_GET_DOUBLE(tmp, slot);
}

/******************************************************************************/

static void JRI_CALLBACK
jri_SetStaticField_ref(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID,
					   JavaObject* value)
{
    OBJECT* slot;
	JHandle* v;
	sysAssert(fieldID != JRIUninitialized);
    v = JRIEnv_deref(ee, value);
	slot = (OBJECT*)fieldID;
    *slot = (OBJECT)v;
}

static void JRI_CALLBACK
jri_SetStaticField_boolean(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID,
							jbool value)
{
    OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    *slot = (OBJECT)value;
}

static void JRI_CALLBACK
jri_SetStaticField_byte(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID,
						 jbyte value)
{
    OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    *slot = (OBJECT)value;
}

static void JRI_CALLBACK
jri_SetStaticField_char(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID,
						 jchar value)
{
    OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    *slot = (OBJECT)value;
}

static void JRI_CALLBACK
jri_SetStaticField_short(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID,
						  jshort value)
{
    OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    *slot = (OBJECT)value;
}

static void JRI_CALLBACK
jri_SetStaticField_int(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID,
						jint value)
{
    OBJECT* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    *slot = (OBJECT)value;
}

static void JRI_CALLBACK
jri_SetStaticField_long(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID,
						 jlong value)
{
    OBJECT* slot;
    JRI_JDK_Java8 tmp;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    JRI_SET_INT64(tmp, slot, value);
}

static void JRI_CALLBACK
jri_SetStaticField_float(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID,
						  jfloat value)
{
    float* slot;
	sysAssert(fieldID != JRIUninitialized);
	slot = (float*)fieldID;
    *slot = value;
}

static void JRI_CALLBACK
jri_SetStaticField_double(JRIEnv* env, jint op, struct java_lang_Class* clazz, jint fieldID,
						   jdouble value)
{
    OBJECT* slot;
    JRI_JDK_Java8 tmp;
	sysAssert(fieldID != JRIUninitialized);
	slot = (OBJECT*)fieldID;
    JRI_SET_DOUBLE(tmp, slot, value);
}

/*******************************************************************************
 * String Operations
 ******************************************************************************/

/* Unicode Interface */

static struct java_lang_String* JRI_CALLBACK
jri_NewString(JRIEnv* env, jint op, const jchar* unicodeChars, jint len)
{
	ExecEnv* ee = NaEnv2EE(env);
    HArrayOfChar* val = (HArrayOfChar *) ArrayAlloc(T_CHAR, len);
    unicode* p;
    struct Hjava_lang_String* result;
    
    if (val == NULL) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return NULL;
    }
    p = unhand(val)->body;
    if (unicodeChars) memcpy(p, unicodeChars, (size_t) (len << 1L));
    result = (struct Hjava_lang_String*)
		execute_java_constructor(ee, 0, classJavaLangString, "([C)", val);
    return (struct java_lang_String*)JRIEnv_ref(ee, (JHandle*)result);
}

static Hjava_lang_String* JRI_CALLBACK
jri_GetString(JRIEnv* env, struct java_lang_String* string)
{
	ExecEnv* ee = NaEnv2EE(env);
    struct Hjava_lang_String* s =
		(struct Hjava_lang_String*)JRIEnv_deref(ee, string);
	if (s == NULL) {
		SignalError(ee, JAVAPKG "NullPointerException", NULL);
	}
	else if (obj_classblock(s) != classJavaLangString) {
		SignalError(ee, JAVAPKG "IllegalAccessError", "object is not a String");
	}
	return s;
}

static jint JRI_CALLBACK
jri_GetStringLength(JRIEnv* env, jint op, struct java_lang_String* string)
{
    struct Hjava_lang_String* s = jri_GetString(env, string);
	if (s == NULL) return 0;
    return javaStringLength(s);
}

static const jchar* JRI_CALLBACK
jri_GetStringChars(JRIEnv* env, jint op, struct java_lang_String* string)
{
	ExecEnv* ee = NaEnv2EE(env);
    Classjava_lang_String* str;
    HArrayOfChar* hac;
    unicode* body;
    struct Hjava_lang_String* s = jri_GetString(env, string);
	if (s == NULL) return NULL;
	str = unhand(s);
	hac = (HArrayOfChar*)str->value;
    if (hac == NULL) {	/* XXX can this really happen? */
		SignalError(ee, JAVAPKG "NullPointerException", NULL);
		return NULL;
	}
    body = unhand(hac)->body;
    return (const jchar*)body;
}

/* UTF Interface */

static struct java_lang_String* JRI_CALLBACK
jri_NewStringUTF(JRIEnv* env, jint op, const jbyte* bytes, jint length)
{
    struct Hjava_lang_String* s =
		makeJavaString((char*)bytes, (size_t)length);
    return JRIEnv_ref(NaEnv2EE(env), (JHandle*)s);
}

static jint JRI_CALLBACK
jri_GetStringUTFLength(JRIEnv* env, jint op, struct java_lang_String* string)
{
	ExecEnv* ee = NaEnv2EE(env);
    Classjava_lang_String* str;
    HArrayOfChar* hac;
    unicode* body;
    struct Hjava_lang_String* s = jri_GetString(env, string);
	if (s == NULL) return 0;
	str = unhand(s);
	hac = (HArrayOfChar*)str->value;
    if (hac == NULL) {	/* XXX can this really happen? */
		SignalError(ee, JAVAPKG "NullPointerException", NULL);
		return 0;
	}
    body = unhand(hac)->body;
    return unicode2utfstrlen(body, str->count);
}

static const jbyte* JRI_CALLBACK
jri_GetStringUTFChars(JRIEnv* env, jint op, struct java_lang_String* string)
{
    struct Hjava_lang_String* s = jri_GetString(env, string);
	if (s == NULL) return NULL;
    return (const jbyte*)makeCString(s);
}

/*******************************************************************************
 * Byte Array Operations
 ******************************************************************************/

static JavaObject* JRI_CALLBACK
jri_NewScalarArray(JRIEnv* env, jint op, jint length, const char* elementSig,
				   const jbyte* initialElements)
{
	ExecEnv* ee = NaEnv2EE(env);
    HArrayOfByte* array;
    char* body;
	int elementType;
    
	switch (elementSig[0]) {
	  case SIGNATURE_BYTE:		elementType = T_BYTE; break;
	  case SIGNATURE_CHAR:		elementType = T_CHAR; break;
	  case SIGNATURE_SHORT:		elementType = T_SHORT; break;
	  case SIGNATURE_INT:		elementType = T_INT; break;
	  case SIGNATURE_LONG:		elementType = T_LONG; break;
	  case SIGNATURE_FLOAT:		elementType = T_FLOAT; break;
	  case SIGNATURE_DOUBLE:	elementType = T_DOUBLE; break;
	  default:
		SignalError(ee, JAVAPKG "InstantiationError",
					"invalid scalar array element signature");
		return NULL;
	}
	array = (HArrayOfByte*)ArrayAlloc(elementType, length);
    if (array == NULL) {
		SignalError(ee, JAVAPKG "OutOfMemory", NULL);
		return NULL;
    }

    /* Give the array a "type" */
    body = unhand(array)->body;
    /* Initialize the array */
    if (initialElements != NULL) {
		memcpy(body, initialElements, (size_t)length);
    }
    return (JavaObject*)JRIEnv_ref(ee, (JHandle*)array);
}

static HArrayOfByte* JRI_CALLBACK
jri_GetScalarArray(JRIEnv* env, JavaObject* array)
{
	ExecEnv* ee = NaEnv2EE(env);
    HArrayOfByte* arr = (HArrayOfByte*)JRIEnv_deref(NaEnv2EE(env), array);
	int flags;
	if (arr == NULL) {
		SignalError(ee, JAVAPKG "NullPointerException", NULL);
	}
	else if ((flags = obj_flags(arr), flags == T_NORMAL_OBJECT)
			 || !T_ISNUMERIC(flags)) {
		SignalError(ee, JAVAPKG "IllegalAccessError", "object is not a scalar array");
	}
	return arr;
}

static jint JRI_CALLBACK
jri_GetScalarArrayLength(JRIEnv* env, jint op, JavaObject* array)
{
    HArrayOfByte* arr = jri_GetScalarArray(env, array);
#if defined(XP_PC) && !defined(_WIN32)
    /*
    ** XXX:
    **     This unsed temporary variable is necessary to avoid an intermal compiler error
    **     with MSVC 1.52c
    */
    unsigned long tmp = 0;
#endif
    if (arr == NULL) return 0;
    return obj_length(arr);
}

static jbyte* JRI_CALLBACK
jri_GetScalarArrayElements(JRIEnv* env, jint op, JavaObject* array)
{
    char* body;
    HArrayOfByte* arr = jri_GetScalarArray(env, array);
    if (arr == NULL) return NULL;
    body = unhand(arr)->body;
    return (char*)body;
}

/*******************************************************************************
 * Object Array Operations
 ******************************************************************************/

static JavaObject* JRI_CALLBACK
jri_NewObjectArray(JRIEnv* env, jint op, jint length, struct java_lang_Class* elementClass,
				   JavaObject* initialElement)
{
	ExecEnv* ee = NaEnv2EE(env);
    HArrayOfObject* array;
    ClassClass* cb;
    JHandle** body;

    cb = jri_Ref2Class(ee, elementClass);
    if (cb == NULL) return NULL;

    array = (HArrayOfObject*)ArrayAlloc(T_CLASS, length);
    if (array == NULL) {
		SignalError(ee, JAVAPKG "OutOfMemory", NULL);
		return NULL;
    }

    /* Give the array a "type" */
    body = unhand((HArrayOfObject*)array)->body;
    body[length] = (JHandle*)cb;
    /* Initialize the array */
    if (initialElement != NULL) {
		JHandle* elt = JRIEnv_deref(ee, initialElement);
		jsize i;
		if (!is_instance_of(elt, cb, ee)) {
			SignalError(ee, JAVAPKG "ArrayStoreException", NULL);
			return NULL;
		}
		for (i = 0; i < length; i++) {
			body[i] = elt;
		}
    }
    return (JavaObject*)JRIEnv_ref(ee, (JHandle*)array);
}

static HArrayOfObject* JRI_CALLBACK
jri_GetObjectArray(JRIEnv* env, JavaObject* array)
{
	ExecEnv* ee = NaEnv2EE(env);
    HArrayOfObject* arr = (HArrayOfObject*)JRIEnv_deref(ee, array);
	int flags;
	if (arr == NULL) {
		SignalError(ee, JAVAPKG "NullPointerException", NULL);
	}
	else if ((flags = obj_flags(arr), flags == T_NORMAL_OBJECT)
			 || flags != T_CLASS) {
		SignalError(ee, JAVAPKG "IllegalAccessError", "object is not an Object array");
	}
	return arr;
}

static jint JRI_CALLBACK
jri_GetObjectArrayLength(JRIEnv* env, jint op, JavaObject* array)
{
    HArrayOfObject* arr = jri_GetObjectArray(env, array);
#if defined(XP_PC) && !defined(_WIN32)
    /*
    ** XXX:
    **     This unsed temporary variable is necessary to avoid an intermal compiler error
    **     with MSVC 1.52c
    */
    unsigned long tmp = 0;
#endif
    if (arr == NULL) return 0;
    return obj_length(arr);
}

static JavaObject* JRI_CALLBACK
jri_GetObjectArrayElement(JRIEnv* env, jint op, JavaObject* array, jint index)
{
	ExecEnv* ee = NaEnv2EE(env);
    HArrayOfObject* arr = jri_GetObjectArray(env, array);
    if (arr == NULL) return NULL;
    if (index >= obj_length(arr)) {
		SignalError(ee, JAVAPKG "ArrayIndexOutOfBoundsException", NULL);
		return NULL;
    }
    return (JavaObject*)JRIEnv_ref(ee, unhand(arr)->body[index]);
}

static void JRI_CALLBACK
jri_SetObjectArrayElement(JRIEnv* env, jint op, JavaObject* array, jint index,
						  JavaObject* value)
{
	ExecEnv* ee = NaEnv2EE(env);
	HObject** body;
	JHandle* val;
	ClassClass* elementClass;
	jsize len;
    HArrayOfObject* arr = jri_GetObjectArray(env, array);
    if (arr == NULL) return;
	len = obj_length(arr);
    if (index >= len) {
		SignalError(ee, JAVAPKG "ArrayIndexOutOfBoundsException", NULL);
		return;
    }

	body = unhand(arr)->body;
	elementClass = (ClassClass*)body[len];
	val = JRIEnv_deref(ee, value);
	if (!is_instance_of(val, elementClass, ee)) {
		SignalError(ee, JAVAPKG "ArrayStoreException", NULL);
		return;
	}
    body[index] = val;
}

/*******************************************************************************
 * Native Bootstrap
 ******************************************************************************/

static void JRI_CALLBACK
jri_RegisterNatives(JRIEnv* env, jint op, struct java_lang_Class* clazz,
					char** nameAndSigArray, void** nativeProcArray)
{
	ExecEnv* ee = NaEnv2EE(env);
	ClassClass* cb;

    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return;

	/* Register Natives */
    for (; *nameAndSigArray != NULL; nameAndSigArray++, nativeProcArray++) {
		struct methodblock* mb;
		jint methodID;
		char* nameAndSig = *nameAndSigArray;
#if 0
		void* nativeProc = *nativeProcArray;
#endif
		char name[256];
		char* n = name;
		char* sig;
		while ((*n++ = *nameAndSig++) != SIGNATURE_FUNC);
		*--n = '\0';
		sig = --nameAndSig;

        /* Passing in cb allows registering of non-universal methods.
         * Users of the JRI are responsible for making sure this is
         * typesafe. */
        methodID = NameAndTypeToHash(name, sig, cb);
		mb = jri_GetMethodBlock(ee, cb, methodID, FALSE, FALSE);
		if (mb == NULL) return;
		if ((mb->fb.access & ACC_NATIVE) != ACC_NATIVE) {
			SignalError(ee, JAVAPKG "NoSuchMethodError", 
						"tried to register undeclared native method");
			return;
		}
        /*
        ** Right now we don't do much with the native procs that were
        ** supplied to us. We still use the jdk dynamic linker strategy
        ** to find the stub routines that invoke the natives.  In the
        ** future, the idea is to emulate the calling conventions on all
        ** the architectures we care about and call these procs
        ** directly. Ambitious yes, but it'll be fast.
        */
		/*mb->code = nativeProc;*/
    }
}

static void JRI_CALLBACK
jri_UnregisterNatives(JRIEnv* env, jint op, struct java_lang_Class* clazz)
{
#if 0 /* Since we don't really register methods now, don't do this either: */
	int nslots;
	struct methodblock* mb;
	ClassClass* cb;

    cb = jri_Ref2Class(NaEnv2EE(env), clazz);
    if (cb == NULL) return;
	for (nslots = 0, mb = cbMethods(cb); nslots < cb->methods_count; nslots++, mb++) {
		mb->code = NULL;
	}
#endif
}

/*******************************************************************************
 * Optional Interfaces
 ******************************************************************************/

/*******************************************************************************
 * Optional Embedding
 ******************************************************************************/

static JavaObject* JRI_CALLBACK
jri_RuntimeGetInterface(void* self, jint id)
{
	return jri_GetInterface(RtEnv2EE(self), id);
}

/* Runtime */

/* !!! This is a non-jump-table entry point: */
JRI_PUBLIC_API(JRIRuntimeInstance*)
JRI_NewRuntime(JRIRuntimeInitargs* initargs)
{
    HThread* self;
    char* errmsg;
    ClassClass * volatile tmpClass;
	
    /* The JDK JRI can only support one runtime instance because its
       state is a bunch of global variables: */
    if (jdkInitialized) return NULL;

    intrInit();

    monitorRegistryInit();
    monitorCacheInit();
	
    /*
    ** We need an ExecEnv to be able to load and run clinit methods in
    ** the classes that the runtime loads during initialization...
    */
    InitializeExecEnv(&_defaultEE, 0);  /* Call before InitializeMem() */
	
    InitializeAlloc(initargs->maxHeapSize, initargs->initialHeapSize);
	
    /* Call before InitializeClassThread() */
    InitializeInterpreter();
	
    self = InitializeClassThread(&_defaultEE, &errmsg);
    if (self == NULL) return NULL;
	
    /* Finish initialization of the main thread */
    InitializeMainThread();
	
    /*
    ** This code is necessary for preemptive threading environments.  Since the 
    ** classes are being stored into global variables, they DO NOT show up on the
    ** C-Stack and are vulnerable to GC until they are made sticky...
    **
    ** Use the volatile stack temporary tmpClass for GC safety...
    */
    tmpClass = FindClass(&_defaultEE, JAVAPKG "NullPointerException", TRUE);
    if (tmpClass == NULL) return NULL;
    tmpClass->flags |= CCF_Sticky;
    classNullPointerException = tmpClass;

    tmpClass = FindClass(&_defaultEE, JAVAPKG "Throwable", TRUE);
    if (tmpClass == NULL) return NULL;
    tmpClass->flags |= CCF_Sticky;
    classThrowable = tmpClass;

    verifyclasses = initargs->verifyMode;
	
    jdkInitialized = JRITrue;
    return jriRuntime;
}

static void JRI_CALLBACK
jri_DisposeRuntime(JRIRuntimeInstance* runtime)
{
    sysAssert(runtime == jriRuntime && jdkInitialized);
    /* JDK does no shutdown stuff (yet). */
    jdkInitialized = (jbool)JRIFalse;
}

static void JRI_CALLBACK
jri_SetIOMode(JRIRuntimeInstance* runtime, JRIIOModeFlags mode)
{
    sysAssert(runtime == jriRuntime && jdkInitialized);
    set_java_io_mode((int)mode);    
}

static void JRI_CALLBACK
jri_SetFSMode(JRIRuntimeInstance* runtime, JRIFSModeFlags mode)
{
    sysAssert(runtime == jriRuntime && jdkInitialized);
    set_java_fs_mode((JavaFSMode)mode);
}

static void JRI_CALLBACK
jri_SetRTMode(JRIRuntimeInstance* runtime, JRIRTModeFlags mode)
{
    sysAssert(runtime == jriRuntime && jdkInitialized);
    set_java_rt_mode((JavaRTMode)mode);
}

/* Environments */

/* from threadruntime.c: */
extern ClassClass* Thread_classblock;
extern HThreadGroup *maingroup;
static int envCount = 0;

static JRIEnv* JRI_CALLBACK
jri_NewEnv(JRIRuntimeInstance* runtime, void* systhread)
{
	ExecEnv* ee;
    struct Classjava_lang_Thread* tid;
    char name[64];
    HThread* jthread;

    if (!(runtime == jriRuntime && jdkInitialized)) return NULL;
    
    jthread = sysThreadGetBackPtr(systhread);
    if (jthread != NULL) 
		ee = (ExecEnv*)unhand(jthread)->eetop;
    else {
		ee = (ExecEnv*)sysMalloc(sizeof(struct execenv));
		if (ee == NULL) return NULL;
	
		/* create the java thread */
		jthread = (HThread*) ObjAlloc(Thread_classblock, 0);
		if (jthread == NULL) {
			sysFree(ee);
			return NULL;
		}
		tid = THREAD(jthread);
	
		/* Need to set unhand(jthread)->eetop before threadBootstrap() */
		unhand(jthread)->eetop = (long)ee;
		unhand(jthread)->group = maingroup;
	
		/* make jthread and systhread point to each other */
		unhand(jthread)->PrivateInfo = (long)systhread;
		sysThreadSetBackPtr(systhread, (void*)jthread);
	
		unhand(jthread)->priority = NormalPriority;
		threadSetPriority(jthread, NormalPriority);
	
		/* Hack alert -- this NULL is supposed to be the stack top, but I know
		   this routine is a no-op (now). */
		threadInit(jthread, NULL);
	
		/* initialize the ExecEnv before invoking the init method */
		InitializeExecEnv(ee, (JHandle*)jthread);

		jio_snprintf(name, sizeof(name), "JRIEnv-%d\0", ++envCount);
	
		execute_java_dynamic_method(ee, (HObject*)jthread, "init", 
									"(Ljava/lang/ThreadGroup;Ljava/lang/Runnable;Ljava/lang/String;)V", 
									maingroup, /* ThreadGroup */
									NULL,      /* runnable */
									makeJavaString(name, strlen(name)));
		if (exceptionOccurred(ee)) {
#ifdef DEBUG
			exceptionDescribe(ee);
#endif
			DeleteExecEnv(ee, (JHandle*)jthread);
			sysFree(ee);
			return NULL;
		}
    }
    return EE2NaEnv(ee);
}

static void JRI_CALLBACK
jri_DisposeEnv(JRIEnv* env)
{
	ExecEnv* ee = NaEnv2EE(env);
    DeleteExecEnv(ee, ee->thread);
    sysFree(ee);
}

static JRIRuntimeInstance* JRI_CALLBACK
jri_GetRuntime(JRIEnv* env)
{
    return jriRuntime;	/* only one */
}

static void* JRI_CALLBACK
jri_GetThread(JRIEnv* env)
{
	ExecEnv* ee = NaEnv2EE(env);
    HThread* thread = (HThread*)ee->thread;
    return (void*)unhand(thread)->PrivateInfo;
}

static void
jri_SetClassLoader(JRIEnv* env, jref classLoader)
{
}

/*******************************************************************************
 * Optional Reflection
 ******************************************************************************/

static JavaObject* JRI_CALLBACK
jri_ReflectionGetInterface(void* self, jint id)
{
	return jri_GetInterface(RfEnv2EE(self), id);
}

static jsize JRI_CALLBACK
jri_GetClassCount(JRIReflectionEnv* env)
{
    return get_nbinclasses();
}

static jref JRI_CALLBACK
jri_GetClass(JRIReflectionEnv* env, jsize index)
{
	ExecEnv* ee = RfEnv2EE(env);
    jsize nclasses;
    ClassClass** cbs;
	jref result = NULL;

	lock_classes();
    nclasses = get_nbinclasses();
    if (index >= nclasses) {
		SignalError(ee, JAVAPKG "ArrayIndexOutOfBoundsException", 
					"no class found at that index");
		goto done;
    }
    cbs = get_binclasses();
    result = JRIEnv_ref(ee, (JHandle*)cbHandle(cbs[index]));
  done:
	unlock_classes();
	return result;
}

static const char* JRI_CALLBACK
jri_GetClassName(JRIReflectionEnv* env, struct java_lang_Class* clazz)
{
	ExecEnv* ee = RfEnv2EE(env);
    ClassClass* cb;
    char buf[128];

    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return NULL;
    classname2string(classname(cb), buf, sizeof(buf));
    return strdup(buf);
}

extern bool_t VerifyClass(ClassClass *cb);

static jbool JRI_CALLBACK
jri_VerifyClass(JRIReflectionEnv* env, struct java_lang_Class* clazz)
{
	ExecEnv* ee = RfEnv2EE(env);
    ClassClass* cb;

    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return JRIFalse;
    return VerifyClass(cb) ? JRITrue : JRIFalse;
}

static jref JRI_CALLBACK
jri_GetClassSuperclass(JRIReflectionEnv* env, struct java_lang_Class* clazz)
{
	ExecEnv* ee = RfEnv2EE(env);
    ClassClass* cb;

    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return NULL;
    return JRIEnv_ref(ee, (JHandle*)cbSuperclass(cb));
}

static jsize JRI_CALLBACK
jri_GetClassInterfaceCount(JRIReflectionEnv* env, struct java_lang_Class* clazz)
{
	ExecEnv* ee = RfEnv2EE(env);
    ClassClass* cb;

    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return 0;
    return cb->implements_count;
}

static jref JRI_CALLBACK
jri_GetClassInterface(JRIReflectionEnv* env, struct java_lang_Class* clazz, jsize index)
{
	ExecEnv* ee = RfEnv2EE(env);
    ClassClass* cb;
    short intf;
    struct Hjava_lang_Class* ch;

    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return NULL;
    if (index >= cb->implements_count) {
		SignalError(ee, JAVAPKG "ArrayIndexOutOfBoundsException", 
					"native method tried to access interface at illegal index");
		return NULL;
    }
    intf = cbImplements(cb)[index];
    ResolveClassConstant(cb->constantpool, intf, ee,
						 1 << CONSTANT_Class);
    ch = cbHandle((ClassClass *)(cb->constantpool[intf].p));
    return JRIEnv_ref(ee, (JHandle*)ch);
}

static jsize JRI_CALLBACK
jri_GetClassFieldCount(JRIReflectionEnv* env, struct java_lang_Class* clazz)
{
	ExecEnv* ee = RfEnv2EE(env);
    ClassClass* cb;

    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return 0;
 	if (makeslottable(cb) == NULL)
		return 0;
    return cbSlotTableSize(cb);
}

static void JRI_CALLBACK
jri_GetClassFieldInfo(JRIReflectionEnv* env, struct java_lang_Class* clazz, jsize fieldIndex,
					  char* *fieldName, char* *fieldSig, 
					  JRIAccessFlags *fieldAccess, jref *fieldClass)
{
	ExecEnv* ee = RfEnv2EE(env);
    ClassClass* cb;
    jsize nslots;
    struct fieldblock* fb;

    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return;
    nslots = cbSlotTableSize(cb);
    if (fieldIndex >= nslots) {
		SignalError(ee, JAVAPKG "ArrayIndexOutOfBoundsException", 
					"native method tried to access field at illegal index");
		return;
    }
    fb = cbSlotTable(cb)[fieldIndex];
    if (fb == NULL) {	/* XXX can this really happen? */
		SignalError(ee, JAVAPKG "NoSuchFieldError", NULL);
		return;
    }
    if (fieldName != NULL)
		*fieldName = strdup(fieldname(fb));
    if (fieldSig != NULL)
		*fieldSig = strdup(fieldsig(fb));
#if 0
    if (fieldID != NULL)
		*fieldID = (JavaIdent)fb->ID;
#endif
    if (fieldAccess != NULL)
		*fieldAccess = (JRIAccessFlags)fb->access;
    if (fieldClass != NULL) {
		ClassClass* cb = fieldclass(fb);
		*fieldClass = JRIEnv_ref(ee, (JHandle*)cbHandle(cb));
    }
}

static jsize JRI_CALLBACK
jri_GetClassMethodCount(JRIReflectionEnv* env, struct java_lang_Class* clazz)
{
	ExecEnv* ee = RfEnv2EE(env);
    ClassClass* cb;

    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return 0;
    return cb->methods_count;
}

static void JRI_CALLBACK
jri_GetClassMethodInfo(JRIReflectionEnv* env, struct java_lang_Class* clazz, jsize methodIndex,
					   char* *methodName, char* *methodSig, 
					   JRIAccessFlags *methodAccess,
					   jref *methodClass, void* *methodNativeProc)
{
	ExecEnv* ee = RfEnv2EE(env);
    ClassClass* cb;
    jsize nmeths;
    struct methodblock* mb;
    struct fieldblock* fb;

    cb = jri_Ref2Class(ee, clazz);
    if (cb == NULL) return;
    nmeths = cb->methods_count;
    if (methodIndex >= nmeths) {
		SignalError(ee, JAVAPKG "ArrayIndexOutOfBoundsException", 
					"native method tried to access method at illegal index");
		return;
    }
    mb = &cbMethods(cb)[methodIndex];
    if (mb == NULL) {	/* XXX can this really happen? */
		SignalError(ee, JAVAPKG "NoSuchMethodError", NULL);
		return;
    }
    fb = &mb->fb;
    if (methodName != NULL)
		*methodName = strdup(fieldname(fb));
    if (methodSig != NULL)
		*methodSig = strdup(fieldsig(fb));
#if 0
    if (methodID != NULL)
		*methodID = strdup(fieldsig(fb));
#endif
    if (methodAccess != NULL)
		*methodAccess = (JRIAccessFlags)fb->access;
    if (methodClass != NULL)
		*methodClass = JRIEnv_ref(ee, (JHandle*)cbHandle(fieldclass(fb)));
    if (methodNativeProc != NULL)
		*methodNativeProc = mb->code;
}

/*******************************************************************************
 * Optional Debugger
 ******************************************************************************/

static JavaObject* JRI_CALLBACK
jri_DebuggerGetInterface(void* self, jint id)
{
	return jri_GetInterface(DbEnv2EE(self), id);
}

/* Manipulating Stacks */

static jsize JRI_CALLBACK
jri_GetFrameCount(JRIDebuggerEnv* env)
{
	ExecEnv* ee = DbEnv2EE(env);
    jsize count = 0;
    JavaFrame* jframe;

    jframe = ee->current_frame;
    while (jframe) {
		jframe = jframe->prev;
        if (jframe->current_method != 0)
			count++;
    }
    return count;
}

static JavaFrame* JRI_CALLBACK
jri_FindFrame(ExecEnv* ee, jsize frameIndex)
{
    JavaFrame* jframe;
    int fr = (int)frameIndex;

    jframe = ee->current_frame;
    while (jframe) {
        /* skip C stack frames */
        if (jframe->current_method == 0) {
            jframe = jframe->prev;
        }
        if (--fr >= 0 && jframe) {
            jframe = jframe->prev;
        } else {
            break;
        }
    }
    if (jframe == NULL) {
		SignalError(ee, JAVAPKG "ArrayIndexOutOfBoundsException", 
					"native method tried to access frame at illegal index");
	}
    return jframe;
}

static jbool JRI_CALLBACK
jri_GetFrameInfo(JRIDebuggerEnv* env, jsize frameIndex,
				  jref *methodClass, jsize *methodIndex,
				  jsize *pc, jsize *varsCount)
{
	ExecEnv* ee = DbEnv2EE(env);
    JavaFrame* frame;
    struct methodblock* mb;

    frame = jri_FindFrame(ee, frameIndex);
    if (frame == NULL) return JRIFalse;
    mb = frame->current_method;
    if (mb != NULL) {
		/* a normal method */
		ClassClass *cb = fieldclass(&mb->fb);
		if (methodClass != NULL) {
			*methodClass = JRIEnv_ref(ee, (JHandle*)cbHandle(cb));
		}
		if (methodIndex != NULL) {
			/* XXX the method index should get cached on mb to avoid
			   this loop */
			int count = cb->methods_count;
			struct methodblock* mbs = cbMethods(cb);
			int i;
			for (i = 0; i < count; i++) {
				if (mb->fb.ID == mbs->fb.ID)
					break;
			}
			sysAssert(i < count);
			*methodIndex = i;
		}
		if (pc != NULL)
			*pc = frame->lastpc - mb->code;
		if (varsCount != NULL)
			*varsCount = mb->localvar_table_length;
		return JRITrue;
    }
    else {
		/* a native method */
		return JRIFalse;
    }
}

static void JRI_CALLBACK
jri_GetSourceInfo(JRIDebuggerEnv* env, jsize frameIndex,
				   const char* *filename, jsize *lineNumber)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static struct localvar* JRI_CALLBACK
jri_FindLocalVar(ExecEnv* ee, JavaFrame* frame, jsize varIndex,
			 struct methodblock* *mbptr)
{
    struct methodblock* mb;

    mb = frame->current_method;
    if (mb == NULL) {
		SignalError(ee, JAVAPKG "IllegalAccessError",
					"native method tried to access a variable of a native method");
		return NULL;
    }
    if (mbptr != NULL)
		*mbptr = mb;
    if (varIndex >= mb->localvar_table_length) {
		SignalError(ee, JAVAPKG "ArrayIndexOutOfBoundsException", 
					"native method tried to access variable at illegal index");
		return NULL;
    }
    return &mb->localvar_table[varIndex];
}

static void JRI_CALLBACK
jri_GetVarInfo(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex,
			   char* *name, char* *signature, jbool *isArgument,
			   jsize *startScope, jsize *endScope)
{
	ExecEnv* ee = DbEnv2EE(env);
    JavaFrame* frame;
    struct methodblock* mb;
    struct localvar* lv;

    frame = jri_FindFrame(ee, frameIndex);
    if (frame == NULL) return;
    lv = jri_FindLocalVar(ee, frame, varIndex, &mb);
    if (lv == NULL) return;
#if 0
    if (id != NULL) {
		/*
		** The jdk doesn't really have identifiers for stack variables,
		** but we'll fabricate one for consistency with the way object
		** field info is returned.
		**
        ** DJH: NameAndTypeToHash now takes the class in which names are
        ** to be resolved as a parameter. What should we use in this case?
        ** A debugging environment should probably have a ClassLoader
        ** associated with it.
        ** Doesn't matter for 3.01 because this is #if'd out.
        */
		*id = (JavaIdent)
			NameAndTypeToHash(frame->constant_pool[lv->nameoff].cp,
							  frame->constant_pool[lv->sigoff].cp,
                              /* what class should we put here? */);
    }
#endif
    if (name != NULL)
		*name = strdup(frame->constant_pool[lv->nameoff].cp);
    if (signature != NULL)
		*signature = strdup(frame->constant_pool[lv->sigoff].cp);
    if (isArgument != NULL)
		*isArgument = (lv->slot < mb->args_size);
    if (startScope != NULL)
		*startScope = lv->pc0;
    if (endScope != NULL)
		*endScope = lv->pc0 + lv->length;
}

static long JRI_CALLBACK
jri_GetVarAddr(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex)
{
	ExecEnv* ee = DbEnv2EE(env);
    JavaFrame* frame;
    struct localvar* lv;
    jsize currentPC;
    long slot = -1;

    frame = jri_FindFrame(ee, frameIndex);
    if (frame == NULL) return -1;
    lv = jri_FindLocalVar(ee, frame, varIndex, NULL);
    if (lv == NULL) return -1;
    currentPC = (long)frame->lastpc - (long)frame->current_method->code;
    if (currentPC >= lv->pc0 && currentPC <= (lv->pc0 + lv->length)) {
		slot = lv->slot;
    }
    return slot;
}

static jref JRI_CALLBACK
jri_GetVar(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex)
{
	long slot = jri_GetVarAddr(env, frameIndex, varIndex);
    return JRIEnv_ref(ee, *(JHandle**)slot);
}

static jbool JRI_CALLBACK
jri_GetVar_boolean(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
	return (jbool)JRIFalse;
}

static jbyte JRI_CALLBACK
jri_GetVar_byte(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
	return 0;
}

static jchar JRI_CALLBACK
jri_GetVar_char(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
	return 0;
}

static jshort JRI_CALLBACK
jri_GetVar_short(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
	return 0;
}

static jint JRI_CALLBACK
jri_GetVar_int(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
	return 0;
}

static jlong JRI_CALLBACK
jri_GetVar_long(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
	return jlong_ZERO;
}

static jfloat JRI_CALLBACK
jri_GetVar_float(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
	return 0.0f;
}

static jdouble JRI_CALLBACK
jri_GetVar_double(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
	return 0.0;
}

/******************************************************************************/

static void JRI_CALLBACK
jri_SetVar(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex,
			jref value)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_SetVar_boolean(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex,
					  jbool value)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_SetVar_byte(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex,
				   jbyte value)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_SetVar_char(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex,
				   jchar value)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_SetVar_short(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex,
					jshort value)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_SetVar_int(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex,
				  jint value)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_SetVar_long(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex,
				   jlong value)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_SetVar_float(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex,
					jfloat value)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_SetVar_double(JRIDebuggerEnv* env, jsize frameIndex, jsize varIndex,
					 jdouble value)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

/******************************************************************************/

/* Controlling Execution */

static void JRI_CALLBACK
jri_StepOver(JRIDebuggerEnv* env)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_StepIn(JRIDebuggerEnv* env)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_StepOut(JRIDebuggerEnv* env)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_Continue(JRIDebuggerEnv* env)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

static void JRI_CALLBACK
jri_Return(JRIDebuggerEnv* env, jsize frameIndex, JRIValue value)
{
	ExecEnv* ee = DbEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

/*******************************************************************************
 * Optional Compiling
 ******************************************************************************/

static JavaObject* JRI_CALLBACK
jri_CompilerGetInterface(void* self, jint id)
{
	ExecEnv* ee = CpEnv2EE(self);
	return jri_GetInterface(ee, id);
}

static void JRI_CALLBACK
jri_CompileClass(JRICompilerEnv* env, const char* classSrc, jsize classSrcLen,
				 jbyte* *resultingClassData, jsize *classDataLen)
{
	ExecEnv* ee = CpEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
}

/*******************************************************************************
 * Optional Expression Evaluation
 ******************************************************************************/

static JavaObject* JRI_CALLBACK
jri_ExprGetInterface(void* self, jint id)
{
	return jri_GetInterface(ExEnv2EE(self), id);
}

static jref JRI_CALLBACK
jri_CompileExpr(JRIExprEnv* env, const char* exprSrc, jsize exprSrcLen)
{
	ExecEnv* ee = ExEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
    return NULL;
}

static jref JRI_CALLBACK
jri_EvalExpr(JRIExprEnv* env, jref expr)
{
	ExecEnv* ee = ExEnv2EE(env);
	SignalError(ee, JAVAPKG "InternalError", "not implemented");
    return NULL;
}

/*******************************************************************************
 * JDK Specific Things
 ******************************************************************************/

#ifndef HAVE_LONG_LONG
#ifdef IS_LITTLE_ENDIAN
jlong jlong_MAXINT = { 0xffffffff, 0x7fffffff };
jlong jlong_MININT = { 0x00000000, 0x80000000 };
#else
jlong jlong_MAXINT = { 0x7fffffff, 0xffffffff };
jlong jlong_MININT = { 0x80000000, 0x00000000 };
#endif
jlong jlong_ZERO   = { 0x00000000, 0x00000000 };

JRI_PUBLIC_API(void)
jlong_udivmod(julong *qp, julong *rp, julong a, julong b)
{
    sysAssert(0);	/* XXX */
}

#endif /* !HAVE_LONG_LONG */

/******************************************************************************/

const JRIEnvInterface jri_NativeInterface = {
	NULL,
	(void*)jri_NativeGetInterface,
	NULL,
	NULL,
	jri_FindClass,
	jri_Throw,
	jri_ThrowNew,
	jri_ExceptionOccurred,
	jri_ExceptionDescribe,
	jri_ExceptionClear,
	jri_NewGlobalRef,
	jri_DisposeGlobalRef,
	jri_GetGlobalRef,
	jri_SetGlobalRef,
	jri_IsSameObject,
	jri_NewObject,
	jri_NewObjectV,
	jri_NewObjectA,
	jri_GetObjectClass,
	jri_IsInstanceOf,
	jri_GetMethodID,
	jri_CallMethod_ref,
	jri_CallMethod_refV,
	jri_CallMethod_refA,
	jri_CallMethod_boolean,
	jri_CallMethod_booleanV,
	jri_CallMethod_booleanA,
	jri_CallMethod_byte,
	jri_CallMethod_byteV,
	jri_CallMethod_byteA,
	jri_CallMethod_char,
	jri_CallMethod_charV,
	jri_CallMethod_charA,
	jri_CallMethod_short,
	jri_CallMethod_shortV,
	jri_CallMethod_shortA,
	jri_CallMethod_int,
	jri_CallMethod_intV,
	jri_CallMethod_intA,
	jri_CallMethod_long,
	jri_CallMethod_longV,
	jri_CallMethod_longA,
	jri_CallMethod_float,
	jri_CallMethod_floatV,
	jri_CallMethod_floatA,
	jri_CallMethod_double,
	jri_CallMethod_doubleV,
	jri_CallMethod_doubleA,
	jri_GetFieldID,
	jri_GetField_ref,
	jri_GetField_boolean,
	jri_GetField_byte,
	jri_GetField_char,
	jri_GetField_short,
	jri_GetField_int,
	jri_GetField_long,
	jri_GetField_float,
	jri_GetField_double,
	jri_SetField_ref,
	jri_SetField_boolean,
	jri_SetField_byte,
	jri_SetField_char,
	jri_SetField_short,
	jri_SetField_int,
	jri_SetField_long,
	jri_SetField_float,
	jri_SetField_double,
	jri_IsSubclassOf,
	jri_GetMethodID,
	jri_CallStaticMethod_ref,
	jri_CallStaticMethod_refV,
	jri_CallStaticMethod_refA,
	jri_CallStaticMethod_boolean,
	jri_CallStaticMethod_booleanV,
	jri_CallStaticMethod_booleanA,
	jri_CallStaticMethod_byte,
	jri_CallStaticMethod_byteV,
	jri_CallStaticMethod_byteA,
	jri_CallStaticMethod_char,
	jri_CallStaticMethod_charV,
	jri_CallStaticMethod_charA,
	jri_CallStaticMethod_short,
	jri_CallStaticMethod_shortV,
	jri_CallStaticMethod_shortA,
	jri_CallStaticMethod_int,
	jri_CallStaticMethod_intV,
	jri_CallStaticMethod_intA,
	jri_CallStaticMethod_long,
	jri_CallStaticMethod_longV,
	jri_CallStaticMethod_longA,
	jri_CallStaticMethod_float,
	jri_CallStaticMethod_floatV,
	jri_CallStaticMethod_floatA,
	jri_CallStaticMethod_double,
	jri_CallStaticMethod_doubleV,
	jri_CallStaticMethod_doubleA,
	jri_GetStaticFieldID,
	jri_GetStaticField_ref,
	jri_GetStaticField_boolean,
	jri_GetStaticField_byte,
	jri_GetStaticField_char,
	jri_GetStaticField_short,
	jri_GetStaticField_int,
	jri_GetStaticField_long,
	jri_GetStaticField_float,
	jri_GetStaticField_double,
	jri_SetStaticField_ref,
	jri_SetStaticField_boolean,
	jri_SetStaticField_byte,
	jri_SetStaticField_char,
	jri_SetStaticField_short,
	jri_SetStaticField_int,
	jri_SetStaticField_long,
	jri_SetStaticField_float,
	jri_SetStaticField_double,
	jri_NewString,
	jri_GetStringLength,
	jri_GetStringChars,
	jri_NewStringUTF,
	jri_GetStringUTFLength,
	jri_GetStringUTFChars,
	jri_NewScalarArray,
	jri_GetScalarArrayLength,
	jri_GetScalarArrayElements,
	jri_NewObjectArray,
	jri_GetObjectArrayLength,
	jri_GetObjectArrayElement,
	jri_SetObjectArrayElement,
	jri_RegisterNatives,
	jri_UnregisterNatives,
	jri_DefineClass
};

const JRIRuntimeInterface jri_RuntimeInterface = {
	(void*)jri_RuntimeGetInterface,
	(void*)jri_AddRef,
	(void*)jri_Release,
	NULL,
	jri_DisposeRuntime,
	jri_SetIOMode,
	jri_SetFSMode,
	jri_SetRTMode,
	jri_NewEnv,
	jri_DisposeEnv,
	jri_GetRuntime,
	jri_GetThread,
	jri_SetClassLoader
};

const JRIReflectionInterface jri_ReflectionInterface = {
	(void*)jri_ReflectionGetInterface,
	(void*)jri_AddRef,
	(void*)jri_Release,
	NULL,
	jri_GetClassCount,
	jri_GetClass,
	jri_GetClassName,
	jri_VerifyClass,
	jri_GetClassSuperclass,
	jri_GetClassInterfaceCount,
	jri_GetClassInterface,
	jri_GetClassFieldCount,
	jri_GetClassFieldInfo,
	jri_GetClassMethodCount,
	jri_GetClassMethodInfo
};

const JRIDebuggerInterface jri_DebuggerInterface = {
	(void*)jri_DebuggerGetInterface,
	(void*)jri_AddRef,
	(void*)jri_Release,
	NULL,
	jri_GetFrameCount,
	jri_GetFrameInfo,
	jri_GetVarInfo,
	jri_GetSourceInfo,
	jri_GetVar,
	jri_GetVar_boolean,
	jri_GetVar_byte,
	jri_GetVar_char,
	jri_GetVar_short,
	jri_GetVar_int,
	jri_GetVar_long,
	jri_GetVar_float,
	jri_GetVar_double,
	jri_SetVar,
	jri_SetVar_boolean,
	jri_SetVar_byte,
	jri_SetVar_char,
	jri_SetVar_short,
	jri_SetVar_int,
	jri_SetVar_long,
	jri_SetVar_float,
	jri_SetVar_double,
	jri_StepOver,
	jri_StepIn,
	jri_StepOut,
	jri_Continue,
	jri_Return
};

const JRICompilerInterface jri_CompilerInterface = {
	(void*)jri_CompilerGetInterface,
	(void*)jri_AddRef,
	(void*)jri_Release,
	NULL,
	jri_CompileClass
};

const JRIExprInterface jri_ExprInterface = {
	(void*)jri_ExprGetInterface,
	(void*)jri_AddRef,
	(void*)jri_Release,
	NULL,
	jri_CompileExpr,
	jri_EvalExpr
};

/******************************************************************************/
