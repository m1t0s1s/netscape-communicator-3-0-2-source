/*
 * @(#)interpreter.c	1.174 96/03/30
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

/*
 * Interpret an Java object file
 */

#ifdef XP_MAC
#include <ConditionalMacros.h>

#ifndef DEBUG
#pragma global_optimizer on 
#pragma optimization_level 1 
#endif

#endif

#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include "bool.h"
#include "jriext.h"

#ifndef XP_MAC
#include <memory.h>
#endif

#include "opcodes.h"
#include "oobj.h"
#include "interpreter.h"
#include "signature.h"
#include "code.h"
#include "tree.h"
#include "javaThreads.h"
#include "monitor.h"
#include "exceptions.h"
#include "javaString.h"
#include "path.h"
#include "byteorder.h"
#include "jmath.h"

#include "sys_api.h"

#include "java_lang_Thread.h"
#include "java_lang_Throwable.h"

#if defined(XP_PC) && !defined(_WIN32)
#if !defined(stderr)
extern FILE *stderr;
#endif
#if !defined(stdout)
extern FILE *stdout;
#endif
#endif

#ifdef BREAKPTS
#include "sun_tools_debug_BreakpointQueue.h"
#endif

#ifdef TRACING
int trace;
int tracem;
#  define TRACE(a) (trace ? printf("%6lX %8lX", (int32_t)threadSelf(), (int32_t)pc), \
	printf a, fflush(stdout), 0 : 0)
#  define TRACEIF(o, a) (opcode == o ? TRACE(a) : 0)

#else
#  define TRACE(a)
#  define TRACEIF(a, b)
#endif

#ifdef STATISTICS
int instruction_count[256];
#endif

#define UCAT(base,offset) (((unsigned char *)base)[offset])
#define ULCAST(x) ((unsigned long)(x))
#define USCAST(x) ((unsigned short)(x))
#define SCAT(base,offset) (((signed char *)base)[offset])
#define SLCAST(x) ((long)(x))
#define SSCAST(x) ((short)(x))

ExecEnv *DefaultExecEnv;

/* Some JIT compilers don't want to use the versions of
 * opc_invokevirtual_quick and opc_getfield_quick that lose information
 */
bool_t UseLosslessQuickOpcodes=FALSE;	

/*
 * Java stack management
 */

/*
 * Lock around code freeing deleted Java stack segments
 */
static sys_mon_t *_ostack_lock;
#define OSTACK_LOCK_INIT()  monitorRegister(_ostack_lock, "Java stack lock")
#define OSTACK_LOCK()	    sysMonitorEnter(_ostack_lock)
#define OSTACK_UNLOCK()	    sysMonitorExit(_ostack_lock)

void FreeJavaStackMemory(void);

#ifdef WIN32
static void HACK_NOP() {
    /* This is a hack to work around an optimizer bug in MSVC++ */
}
#else
#define HACK_NOP()
#endif

JavaStack *CreateNewJavaStack(ExecEnv *ee, JavaStack *previous_stack)
{
    JavaStack *stack;

    FreeJavaStackMemory();	/* Free any stack segments for dead threads */

    stack = sysMalloc(sizeof (*stack));
    if (stack == 0) {
	return 0;
    }
    stack->execenv = ee;
    stack->prev = previous_stack;
    stack->next = 0;
    if (previous_stack) {
	sysAssert(previous_stack->next == 0);
        previous_stack->next = stack;
	stack->stack_so_far = previous_stack->stack_so_far 
	          + JAVASTACK_CHUNK_SIZE * sizeof(stack->data[0]);
    } else {
	stack->stack_so_far = JAVASTACK_CHUNK_SIZE * sizeof(stack->data[0]);
    }
    stack->end_data = &stack->data[JAVASTACK_CHUNK_SIZE];
    return stack;
}

extern const JRIEnvInterface jri_NativeInterface;
extern const JRIRuntimeInterface jri_RuntimeInterface;
extern const JRIReflectionInterface jri_ReflectionInterface;
extern const JRIDebuggerInterface jri_DebuggerInterface;
extern const JRICompilerInterface jri_CompilerInterface;
extern const JRIExprInterface jri_ExprInterface;

/*
 * Note that CreatenewJavaStack may return 0 if we run out of malloc
 * space.  Users of InitializeExecEnv should test for this case by checking
 * ee->initial_stack, and throw an OutOfMemoryError if that's possible or
 * otherwise call out_of_memory (for instance in the middle of startup).
 */
void InitializeExecEnv(ExecEnv *ee, JHandle *thread) {
    if (DefaultExecEnv == 0 && thread == 0) {
	DefaultExecEnv = ee;
    }
    ee->current_frame = 0;
    ee->thread = thread;
    ee->initial_stack = CreateNewJavaStack(ee, 0); /* 0 if out of memory */
    if (thread) { /* we don't have thread at initialization */
        unhand((Hjava_lang_Thread *)thread)->eetop = (long)ee;
    }
    exceptionClear(ee);

    /* Stuff for the JRI: */
    ee->nativeInterface		= &jri_NativeInterface;
    ee->runtimeInterface	= &jri_RuntimeInterface;
    ee->reflectionInterface	= &jri_ReflectionInterface;
    ee->debuggerInterface	= &jri_DebuggerInterface;
    ee->compilerInterface	= &jri_CompilerInterface;
    ee->exprInterface		= &jri_ExprInterface;
    ee->globalRefs.element	= NULL;
    ee->globalRefs.size		= 0;
    ee->globalRefs.top		= 0;

    ee->mochaContext		= NULL;

#ifdef JAVA_PREEMPT_USING_COUNT
    ee->opcodeCount             = 0;
#endif

#ifdef DEBUG
    ee->debugFlags              = 0;
#endif
}

#ifdef DEBUG
static int java_stack_chunks_freed = 0;
#endif

/*
 * The list on which we put freed thread stacks pending reclamation.
 */
typedef struct free_javastack_t {
    struct free_javastack_t *next;
} free_javastack_t;

static free_javastack_t *javastackFreeList = (free_javastack_t *) 0;

void DeleteExecEnv(ExecEnv *ee, JHandle *thread) {
    JavaStack *this_stack;

    if (thread) { 
        unhand((Hjava_lang_Thread *)thread)->eetop = (long)0;
    }

    OSTACK_LOCK();    /* Could do per chunk, but higher locking cost */
    for (this_stack = ee->initial_stack; this_stack != 0; ) {
        JavaStack *next_stack = this_stack->next;
        ((free_javastack_t *) this_stack)->next = javastackFreeList;
        javastackFreeList = (free_javastack_t *) this_stack;
#ifdef DEBUG
        java_stack_chunks_freed++;
#endif
        this_stack = next_stack;
    }

    /* JRI */
    {
	RefTable* refs = &ee->globalRefs;
	if (refs->element) {
	    sysFree(refs->element);
	    refs->element = NULL;
	}
	refs->size = 0;
	refs->top = 0;
    }

    OSTACK_UNLOCK();

    ee->initial_stack = 0;

    /* Free the EE structure which was allocated in ThreadRT0(...) */
    sysFree(ee);
}

/*
 * Free Java stack segments for dead threads.  We do this whenever the
 * idle thread runs, and whenever we grow an Java stack.
 */
void FreeJavaStackMemory() {
    free_javastack_t* mem;
    free_javastack_t* next;

    /* There's a race in this test, but losing only delays freeing */
    if (javastackFreeList != NULL) {
	OSTACK_LOCK();
	mem = javastackFreeList;
	javastackFreeList = 0;
	OSTACK_UNLOCK();
	while (mem) {
	    next = mem->next;
	    sysFree(mem);
	    mem = next;
	}
    }
}

bool_t ExecuteJava(unsigned char  *, ExecEnv *ee);


/* Call an Java method from C.  The java method may have any number of arguments
 * and may return a single 4 byte value which may be an integer, a pointer to
 * an object handle, whatever.
 */
JRI_PUBLIC_API(long)
execute_java_dynamic_method(ExecEnv *ee, HObject *obj, 
			   char *method_name, char *signature, ...)
{
    va_list args;
    long result;
    va_start(args, signature);
        result = do_execute_java_method_vararg(ee, obj, method_name, signature, 
					      0, FALSE, args, 0, FALSE);
    va_end(args);
    return result;
}


/* Call a static Java method from C.  The java method may have any number of
 * arguments and may return a single 4-byte value.`
 */
long
execute_java_static_method(ExecEnv *ee, ClassClass *cb, 
			  char *method_name, char *signature, ...)
{
    va_list args;
    long result;
    va_start(args, signature);
        result = do_execute_java_method_vararg(ee, cb, method_name, signature,
					      0, TRUE, args, 0, FALSE);
    va_end(args);
    return result;
}

/* Create a new Java object from C.  For example:
 *
 *       obj = execute_java_constructor(0, "java/lang/Float", 0, "(F)", f);
 *
 * EE should be either 
 *     .) the current execution environment
 *     .) 0, in which case the current execution environment is calculated
 *     .) PRIVILEGED_EE, meaning explicitly allow this call to access 
 *        constructors for which the current EE doesn't have privileges.
 *        [i.e. you can access private constructors, etc.]
 *
 * Either classname or cb gives the class of the object to be constructed.  
 * Pass the name of the class in classname, or a pointer to the class object
 * in cb.  Whichever one you give, the other should be NULL.
 *
 * signature is the signature of the arguments to the constructor.  It's 
 * first and last character should be "(" and ")" respectively.  Following
 * are the arguments to the function.
 */

HObject *execute_java_constructor_vararg(struct execenv *ee,
				  char *classname,
				  ClassClass *cb,
				  char *signature, va_list args)
{
#ifdef TRACING
    int pc = 0;			/* for TRACE */
#endif

    HObject *obj;
    char real_signature[256];
    hash_t initID;
    int len;
    ClassClass *current_class;
    struct methodblock *mb;
    bool_t security = TRUE;

    
    if (ee == PRIVILEGED_EE) {
	ee = EE();
	security = FALSE;
    } else if (ee == 0) {
        ee = EE();
    }

    if (ee  &&  ee->current_frame  &&  ee->current_frame->current_method)
	current_class = fieldclass(&ee->current_frame->current_method->fb);
    else
	current_class = 0;

    /* Find the class.  Make sure we can allocate an object of that class */
    if (cb == NULL) {
	cb = FindClass(0, classname, TRUE);
	if (cb == NULL) {
	    if (!ee || !exceptionOccurred(ee)) {
		SignalError(0, JAVAPKG "NoClassDefFoundError", classname);
            }
	    return 0;
	}
    } else { 
	classname = classname(cb);
    }
    if (cbAccess(cb) & (ACC_INTERFACE | ACC_ABSTRACT)) { 
	SignalError(0, JAVAPKG "InstantiationException", classname(cb));
	return 0;
    }
    if (security && !VerifyClassAccess(current_class, cb, FALSE)) {
	SignalError(0, JAVAPKG "IllegalAccessException", classname(cb));
	return 0;
    }
    
    /* Find the constructor */
    if (jio_snprintf(real_signature, sizeof(real_signature), "%sV", signature) == -1) {
	SignalError(0, JAVAPKG "InternalError", "signature overflow");
	return 0;
    }

    /* Only call 'universal' methods (all argument and result types local). */
    initID = NameAndTypeToHash("<init>", real_signature, 0);
    for (len = cb->methods_count, mb = cb->methods; --len >= 0; mb++) 
	if (mb->fb.ID == initID) 
	    break;
    if (len < 0) { 
	SignalError(0, JAVAPKG "NoSuchMethodError", 0);
	return 0;
    }
    if (security && !VerifyFieldAccess(current_class, fieldclass(&mb->fb), 
					mb->fb.access, FALSE)) {
	SignalError(0, JAVAPKG "IllegalAccessException", 0);
	return 0;
    } 

    /* Allocate the object and call the constructor */
    if ((obj = newobject(cb, 0, ee)) == 0) { 
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    }
    TRACE(("execute_java_constructor new %s => %s\n", 
	   classname(cb), Object2CString(obj)));
    do_execute_java_method_vararg(ee, obj, 0, 0, mb, FALSE, args, 0, FALSE);
    return obj;
}

JRI_PUBLIC_API(HObject *)
execute_java_constructor(struct execenv *ee,
				  char *classname,
				  ClassClass *cb,
				  char *signature, ...)
{
    HObject *obj;
    va_list args;
    va_start(args, signature);
        obj = execute_java_constructor_vararg(ee, classname, cb, signature, args);
    va_end(args);
    return obj;
}

JRI_PUBLIC_API(long)
do_execute_java_method(ExecEnv *ee, void *obj, 
		      char *method_name, char *signature, 
		      struct methodblock *mb,
		      bool_t isStaticCall, ...) {
    va_list args;
    long result;
    va_start(args, isStaticCall);
        result = do_execute_java_method_vararg(ee, obj, 
					  method_name, signature, 
					  mb, isStaticCall, args, 0, FALSE);
    va_end(args);
    return result;
}


long
do_execute_java_method_vararg(ExecEnv *ee, void *obj, 
			      char *method_name, char *method_signature, 
			      struct methodblock *mb,
			      bool_t isStaticCall, va_list args, 
			      long *otherBits, bool_t shortFloats) {

#ifdef TRACING
    int pc = 0;			/* for TRACE */
#endif

    JavaFrame *current_frame, *previous_frame;
    JavaStack *current_stack;
    int32_t retval = 0;
    char *p;
    Java8 tdub;

    if (ee == 0)
        ee = EE();

    if (otherBits) {
	*otherBits = 0;
    }

    if (mb) {
        method_name = fieldname(&mb->fb);
	method_signature = fieldsig(&mb->fb);
    }
    
    TRACE(("\texecute_java_method_vararg method = %s, signature = %s\n\n", method_name, method_signature));

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
		    SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		    return 0;
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
		  (current_frame->optop++)->f = (float) va_arg(args, double);
	      break;
	  case SIGNATURE_CLASS:
	    (current_frame->optop++)->h = va_arg(args, HObject *);
	    while (*p != SIGNATURE_ENDCLASS) p++;
	    break;
	  case SIGNATURE_ARRAY:
	    while ((*p == SIGNATURE_ARRAY)) p++;
	    if (*p == SIGNATURE_CLASS) 
	        { while (*p != SIGNATURE_ENDCLASS) p++; } 
	    (current_frame->optop++)->p = va_arg(args, void *);
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
	    return 0;
	}
    }
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
	    constant_pool[1].i = (ULCAST(2) << 16) + 3; /* Class, NameAndType */
	    cpt[1] = (isStaticCall || isInitializer)
		        ? CONSTANT_Methodref : CONSTANT_InterfaceMethodref; 
	    constant_pool[2].p = cb; /* class */
	    cpt[2] = CONSTANT_POOL_ENTRY_RESOLVED | CONSTANT_Class;
	    constant_pool[3].i = (ULCAST(4) << 16) + 5;	/* Name, Type */
	    cpt[3] = CONSTANT_NameAndType;
	    constant_pool[4].cp = method_name;
	    cpt[4] = CONSTANT_POOL_ENTRY_RESOLVED | CONSTANT_Utf8;
	    constant_pool[5].cp = method_signature;
	    cpt[5] = CONSTANT_POOL_ENTRY_RESOLVED | CONSTANT_Utf8;	    
	}

	current_frame->constant_pool = constant_pool;

	/* Run the byte codes outside the lock and catch any exceptions. */
	ee->exceptionKind = EXCKIND_NONE;
	if (ExecuteJava(pc, ee) && (p[1] != SIGNATURE_VOID)) {
	    retval = current_frame->optop[-1].i;
	    if ((p[1] == SIGNATURE_DOUBLE || p[1] == SIGNATURE_LONG) 
		  && (otherBits != 0))
		*otherBits = current_frame->optop[-2].i;
	}
	/* Our caller can look at ee->exceptionKind and ee->exception. */
	ee->current_frame = previous_frame;
	return retval;
    }
}

char       *
Object2CString(JHandle *oh)
{                               /* ARB to C string */
    ClassObject *o;
    ClassClass *cb;
    /* Using a static buffer is not MT-safe! */
    static char buf[100];
    int maxlen = sizeof(buf) - 10;

    if (oh == 0)
	return "NULL";
    o = unhand(oh);
    if (-1000 < (long) o && (long) o < 1000)
	/* Silently truncate on unlikely overflow */
	(void) jio_snprintf(buf, sizeof(buf), "BOGUS-PTR[%ld]", (int32_t)o);
    else {
	switch (obj_flags(oh)) {
	case T_NORMAL_OBJECT:
	    {
		char cbuf[128];
		cb = obj_classblock(oh);
		/* Quietly truncate on buffer overflow */
		(void) jio_snprintf(buf, sizeof(buf), "%s@%lX%c%lX",
			 classname2string(classname(cb), cbuf, sizeof(cbuf)),
			 (int32_t)oh, DIR_SEPARATOR, (int32_t)o);
	    }
	    break;
	case T_CHAR: {
	    HArrayOfChar *str = (HArrayOfChar *) oh;
	    int32_t len = (int32_t) obj_length(str);
	    unicode *s = unhand(str)->body;
	    char *p = buf;
	    
	    len = (len > maxlen ? maxlen : len);

	    *p++ = '"';
	    while (--len >= 0)
		*p++ = (char) *s++;
	    *p++ = '"';
	    *p++ = '\0';
	    break;
	    }
	case T_CLASS:
#define BODYOF(h)   unhand(h)->body
	    {
		char cbuf[128];

		cb = ((ClassClass **) BODYOF((HArrayOfObject*)oh))[obj_length(oh)];
		/* Quietly truncate on buffer overflow */
		(void) jio_snprintf(buf, sizeof(buf), "%s[%d]",
			cb ? classname2string(classname(cb), cbuf, sizeof(cbuf)) : "*MissingClassName*",
			obj_length(oh));
	    }
	    break;
	default:
	    /* Quietly truncate on buffer overflow */
	    (void) jio_snprintf(buf, sizeof(buf), "%s[%ld]", 
		    arrayinfo[obj_flags(oh)].name, obj_length(oh));
	}
    }
    return buf;
}

#ifdef BREAKPTS
static Hsun_tools_debug_BreakpointQueue *the_bkptQ = NULL;
static ClassClass *bkptHandler_class = NULL;

static int single_stepping = 0;		/* debugger sets this */

void set_single_stepping(bool_t b) {
    if (b) {
        single_stepping++;
    } else if (single_stepping > 0) {
        single_stepping--;
    }
}

static void
notify_debugger(HThread *hp, unsigned char *pc, HObject *exc, 
		unsigned char *catch_pc)
{
    /* transfer control to the debugger thread by signalling the monitor. */
    monitorEnter(obj_monitor(the_bkptQ));

    /* communicate the information about this breakpoint */
    unhand(the_bkptQ)->thread = hp;
    unhand(the_bkptQ)->pc = (long)pc;
    unhand(the_bkptQ)->opcode = -1;
    unhand(the_bkptQ)->exception = (Hjava_lang_Throwable *)exc;
    unhand(the_bkptQ)->catch_pc = (long)catch_pc;
    unhand(the_bkptQ)->updated = FALSE;

    /* this will yield to the debugger thread since it is running at */
    /* MaximumPriority */
    monitorNotifyAll(obj_monitor(the_bkptQ));
    monitorExit(obj_monitor(the_bkptQ));
    /* Win32 fails to block until the debugger thread is through, so
     * wait for it to explicitly signal it's done. */
    while (unhand(the_bkptQ)->updated == FALSE) {
        monitorEnter(obj_monitor(bkptHandler_class));
        monitorWait(obj_monitor(bkptHandler_class), 100);
        monitorExit(obj_monitor(bkptHandler_class));
    }
}

static unsigned char 
get_breakpoint_opcode(unsigned char *pc, ExecEnv *ee)
{
    HThread *hp = threadSelf();

    int real_opcode;
    ClassClass *agent_class;
    long fSystemThread;

    /* If this is the first time we're called, perform initialization. */
    if (the_bkptQ == 0) {
	Hsun_tools_debug_BreakpointQueue **bkptQ;
	if (bkptHandler_class == NULL) {
	    /* Find the class */
	    bkptHandler_class = FindClass(ee, INTRP_BRKPT_STRING, TRUE);
	    if (bkptHandler_class == NULL) {
		return opc_breakpoint;
	    }
	}

	/* Find the static variable that contains the queue */
	bkptQ =(Hsun_tools_debug_BreakpointQueue **)
	    getclassvariable(bkptHandler_class, "the_bkptQ");
	if (bkptQ == 0 || *bkptQ == 0) {
#ifdef DEBUG
	    printf("?breakpoint queue is null?\n");
#endif
	    SignalError(ee, JAVAPKG "InternalError", 
			"debugger not initialized");
	    return opc_breakpoint;
	} else {
	    the_bkptQ = *bkptQ;
	}
    }

    /* ignore non-debuggable threads */
    agent_class = FindClass(ee, "sun/tools/debug/Agent", TRUE);
    if (agent_class == NULL) {
        return opc_breakpoint;
    }
    fSystemThread = execute_java_static_method(EE(), agent_class, "systemThread", 
                                               "(Ljava/lang/Thread;)Z", hp);
    if (fSystemThread != 0) {
        return execute_java_static_method(
            EE(), (ClassClass *)bkptHandler_class, 
            "getOpcode", "(I)I", pc);
    }

    notify_debugger(hp, pc, NULL, NULL);

    real_opcode = unhand(the_bkptQ)->opcode;
    if (real_opcode == -1) {
        real_opcode = execute_java_static_method(
            EE(), (ClassClass *)bkptHandler_class, 
            "getOpcode", "(I)I", pc);
    }
    unhand(the_bkptQ)->opcode = -1;
    return real_opcode;
}

void
notify_debugger_of_exception(unsigned char *pc, ExecEnv *ee, HObject *object)
{
    int real_opcode;
    unsigned char *catch_pc;
    struct javaframe *fr;
    static unsigned char *ProcedureFindThrowTag();

    if (debugagent == 0 || get_the_bkptQ() == NULL) {
        return;
    }

    /* walk up the stack to see if this exception is caught anywhere. */
    catch_pc = NULL;
    fr = ee->current_frame;
    while (fr != NULL) {
        if (fr->current_method != NULL) {	/* skip C frames */
            catch_pc = ProcedureFindThrowTag(fr, object, fr->lastpc);
            if (catch_pc != NULL) {
                break;
            }
        }
        fr = fr->prev;
    }
    real_opcode = unhand(the_bkptQ)->opcode;
    notify_debugger(threadSelf(), pc, object, catch_pc);
    unhand(the_bkptQ)->opcode = real_opcode;
}

static void
set_breakpoint_opcode(unsigned char *pc, unsigned char opcode)
{
    HThread *hp = threadSelf();

    /* transfer control to the debugger thread by signalling the monitor. */
    monitorEnter(obj_monitor(the_bkptQ));

    /* communicate the information about this breakpoint */
    unhand(the_bkptQ)->thread = hp;
    unhand(the_bkptQ)->pc = (int)pc;
    unhand(the_bkptQ)->opcode = opcode;

    /* Yield to the debugger thread, running at Maximum priority. */
    monitorNotifyAll(obj_monitor(the_bkptQ));
    monitorExit(obj_monitor(the_bkptQ));
}
#endif

bool_t is_subclass_of(ClassClass *, ClassClass *, ExecEnv *);


JRI_PUBLIC_API(bool_t)
is_instance_of(JHandle * h, ClassClass *dcb, ExecEnv *ee)
{
    if (h == NULL) {		/* null can be cast to anything */
	return TRUE;
    } else if (obj_flags(h) == T_NORMAL_OBJECT) {
	/* h is an object.  Get its class and to the work */
	return is_subclass_of(obj_classblock(h), dcb, ee);
    } else {
	/* h is an array.  dcb must either be classJavaLangObject, or 
	   interfaceJavaLangCloneable, or an array class */
	return dcb->name[0] == SIGNATURE_ARRAY
	           ? array_is_instance_of_array_type(h, dcb, ee)
		   : (dcb == classJavaLangObject 
		          || dcb == interfaceJavaLangCloneable);
    }
}

bool_t
is_subclass_of(ClassClass *cb, ClassClass *dcb, ExecEnv *ee)
{
    if (dcb->access & ACC_INTERFACE) {
	for (; ; cb = unhand(cbSuperclass(cb))) {
	    if (ImplementsInterface(cb, dcb, ee)) 
		return TRUE;
	    if (cbSuperclass(cb) == 0)
		return FALSE;
	}
    } else {
	for (; ; cb = unhand(cbSuperclass(cb))) {
	    if (cb == dcb)
		return TRUE;
	    if (cbSuperclass(cb) == 0)
		return FALSE;
	}
    }
}

/* Return TRUE if cb implements the interface icb */

bool_t 
ImplementsInterface(ClassClass *cb, ClassClass *icb, ExecEnv *ee)
{
    /* Let's avoid resolving the class unless we really have to.  So first,
     * First, do a string comparison of icb against all the interfaces thant
     * cb implements.
     */
    char *icb_name = icb->name;
    cp_item_type *constant_pool = cb->constantpool;
    int i;
    for (i = 0; i < (int)(cb->implements_count); i++) {
	int interface_index = cb->implements[i];
	char *interface_name = 
	        GetClassConstantClassName(constant_pool, interface_index);
	/* First, check to see if the names match, so we know whether we
	 * need to bother trying to load in the interface definition to check
	 * for class loaders, etc */
	if (strcmp(icb_name, interface_name) == 0) {
	    if (cbLoader(icb) == cbLoader(cb)) {
		return TRUE;
	    } else {
		ClassClass *sub_cb;
		if (!ResolveClassConstantFromClass(cb, interface_index, ee, 
						   1 << CONSTANT_Class))
		    return FALSE;
		sub_cb = (ClassClass *)(constant_pool[interface_index].p);
		if (sub_cb == icb) 
		    return TRUE;
                /* printf("  bogus match skipped\n");*//* should we break?? return false? */
	    }
	}
    }
    /* See if any of the interfaces that we implement possibly implement icb */
    for (i = 0; i < (int)(cb->implements_count); i++) {
	int interface_index = cb->implements[i];
	ClassClass *sub_cb;
	if (!ResolveClassConstantFromClass(cb, interface_index, ee, 
					   1 << CONSTANT_Class))
	    return FALSE;
	sub_cb = (ClassClass *)(constant_pool[interface_index].p);
	if (ImplementsInterface(sub_cb, icb, ee)) { 
	    return TRUE;
	}
    }
    return FALSE;
}

bool_t array_is_instance_of_array_type(JHandle * h, ClassClass *cb, ExecEnv *ee)
{
    int32_t class_depth = cb->constantpool[CONSTANT_POOL_ARRAY_DEPTH_INDEX].i;
    int32_t class_type = cb->constantpool[CONSTANT_POOL_ARRAY_TYPE_INDEX].i;
    int32_t item_depth, item_type;
    ClassClass *item_class, *class_class;

    ResolveClassConstantFromClass(cb, CONSTANT_POOL_ARRAY_CLASS_INDEX,
				  ee, 1 << CONSTANT_Class);
    if (exceptionOccurred(ee))
	return FALSE;
    class_class = (class_type == T_CLASS) 
	             ? cb->constantpool[CONSTANT_POOL_ARRAY_CLASS_INDEX].p : 0;

    if (obj_flags(h) == T_CLASS) {
	/* We have an array of classes.  Find out the types of the elements. */
	ArrayOfObject *array = (ArrayOfObject *)unhand(h);
	/* uint32_t length = obj_length(h); Removed because it was crashing MSVC's cl 1.52 */
	ClassClass *array_cb = (ClassClass *) array->body[obj_length(h)];
	if (array_cb->name[0] == SIGNATURE_ARRAY) {
	    /* an Array of arrays */
	    item_depth = 
		  array_cb->constantpool[CONSTANT_POOL_ARRAY_DEPTH_INDEX].i + 1;
	    item_type = 
		  array_cb->constantpool[CONSTANT_POOL_ARRAY_TYPE_INDEX].i;
	    ResolveClassConstantFromClass(array_cb, 
					  CONSTANT_POOL_ARRAY_CLASS_INDEX,
					  ee, 1 << CONSTANT_Class);
	    if (exceptionOccurred(ee))
		return FALSE;
	    item_class = 
		  array_cb->constantpool[CONSTANT_POOL_ARRAY_CLASS_INDEX].p;
	} else {
	    /* An array of clases */
	    item_depth = 1;
	    item_type = T_CLASS;
	    item_class = array_cb;
	}
    } else {
	item_depth = 1;
	item_type = obj_flags(h);
	item_class = 0;		/* immaterial */
    }
    if (item_depth > class_depth) { 
	return (class_class == classJavaLangObject) || 
	       (class_class == interfaceJavaLangCloneable);
    } else if (item_depth == class_depth) { 
	return    (class_type == item_type) 
	       && (   (class_type != T_CLASS) 
		   || (class_class == item_class)
		   || is_subclass_of(item_class, class_class, ee));
    } else { 
	return FALSE;
    }
    return FALSE;
}

/*
 * Lock around rewriting method code for quick_ instruction optimization
 * We abandon locking efforts if the opcode has already been re-written.
 */
static sys_mon_t *_code_lock;
#define CODE_LOCK_INIT()    monitorRegister(_code_lock, "Code rewrite lock")
#define CODE_UNLOCK()	    sysMonitorExit(_code_lock)
#define CODE_LOCK()	    { unsigned char real_opcode;                \
                              sysMonitorEnter(_code_lock);              \
                              real_opcode = *pc;                        \
                              REMOVE_BREAKPOINT_CONFUSION(real_opcode); \
                              if (opcode != real_opcode) { /* Someone else translated it */ \
                                CODE_UNLOCK();                          \
                                opcode = real_opcode;                   \
                                goto again;                             \
                              }                                         \
                            }

#ifdef BREAKPTS    
#define WITH_OPCODE_CHANGE { \
	bool_t debugging = (*pc == opc_breakpoint); 
#define END_WITH_OPCODE_CHANGE \
        if (debugging && (*pc != opc_breakpoint)) { \
        	opcode = *pc; \
                *pc = opc_breakpoint;  \
		set_breakpoint_opcode(pc, opcode); \
                goto again; } }
#define REMOVE_BREAKPOINT_CONFUSION(possible_opcode)        \
        if (opc_breakpoint == possible_opcode)              \
          possible_opcode = get_breakpoint_opcode(pc, ee);  \
        else                                                \
          /* a semicolon comes here */

#else
#define WITH_OPCODE_CHANGE
#define END_WITH_OPCODE_CHANGE
#define REMOVE_BREAKPOINT_CONFUSION(possible_opcode)
#endif

/* An exception has been thrown.  Find the pc at which this exception
 * is handled.  Alternatively, rethrow the error if no appropriate pc
 * is found.
 */

/* WARNING: Similar code in jit/win32/jinterf.c must be modified if the 
   code that follows is repaired/changed.  See GetJavaExceptionHandler(). */

static unsigned char *
ProcedureFindThrowTag(ExecEnv *ee, JavaFrame *frame, 
		      JHandle *object, unsigned char *pc)
{
    if (frame->current_method == 0) {
	return 0;
    } else {
	/* Search current class for catch frame, based on pc value,
	   matching specified type. */
	struct methodblock *mb = frame->current_method;
	ClassClass *methodClass = fieldclass(&mb->fb);
	ClassClass *objectClass = obj_array_classblock(object);
	struct CatchFrame *cf = mb->exception_table;
	struct CatchFrame *cfEnd = cf + mb->exception_table_length;
	codepos_t pcTarget = pc - mb->code; /* offset from start of code */
	cp_item_type *constant_pool = frame->constant_pool;
	for (; cf < cfEnd; cf++) { 
	    if (cf->start_pc <= pcTarget && pcTarget < cf->end_pc) { 
		/* Is the class name a superclass of object's class. */
		if (cf->catchType == 0) {
		    /* Special hack put in for finally.  Always succeed. */
		    goto found;
		} else {
		    /* The usual case */
		    int catchType = cf->catchType;
		    char *catchName = 
			GetClassConstantClassName(constant_pool, catchType);
		    ClassClass *catchClass = NULL;
		    ClassClass *cb;
		    for (cb = objectClass;; cb = unhand(cbSuperclass(cb))) {
			if (strcmp(classname(cb), catchName) == 0) {
			    if (cbLoader(cb) == cbLoader(methodClass)) 
				goto found;
			    if (catchClass == NULL) { 
				/* I haven't yet turned found the actual
				 * class corresponding to catchType, but it's
				 * got the right name.  Find the actual class */
				if (!ResolveClassConstantFromClass(
				       methodClass, catchType, ee, 
				       1 << CONSTANT_Class)) 
				    return FALSE;
				catchClass = 
				    (ClassClass *)(constant_pool[catchType].p);
			    }
			    if (cb == catchClass) 
				goto found;
			}
			if (cbSuperclass(cb) == 0) {
			    /* Not found.  Try next catch frame. */
			    break;
			}
		    }
		}
	    }
	}
	/* not found in this execution environment */
	return 0;
      found:
	return mb->code + cf->handler_pc;
    }
}


/* Called by opc_multianewarray.  Create a filled in multidimensional array
 *    dimensions:  The number of dimensions to fill in
 *    array_cb:    The type of the array
 *    sizes:       An array of the dimension sizes
 *
 * Return 0 if we've run out of space.  Otherwise, return the allocated array.
 */

static HObject *MultiArrayAlloc2(int, ClassClass **, stack_item *, int32_t);

HObject *
MultiArrayAlloc(int dimensions, ClassClass *array_cb, stack_item *sizes)
{
    /* Get the base type and depth of the array class we're constructing */
    cp_item_type *constant_pool = cbConstantPool(array_cb);
    int32_t depth = constant_pool[CONSTANT_POOL_ARRAY_DEPTH_INDEX].i;
    int32_t base_type = constant_pool[CONSTANT_POOL_ARRAY_TYPE_INDEX].i;
    ClassClass *sub_array_cb[256];
    char *class_name = classname(array_cb);
    struct execenv *ee = EE();
    int i;

    sysAssert(class_name[0] == SIGNATURE_ARRAY);
    sysAssert(dimensions <= depth);

    /* Fill in sub_array_cb with the array types of each of the lower.
     * dimensions.  Put a 0 in the slot if we've reached a non-class base type
     */
    for (i = 1; i <= dimensions; i++) {
	if (i < depth) {
	    sub_array_cb[i - 1] = 
		FindClassFromClass(ee, class_name + i, FALSE, array_cb);
	} else if (i == depth && base_type == T_CLASS) {
	    ResolveClassConstantFromClass(array_cb, 
					  CONSTANT_POOL_ARRAY_CLASS_INDEX,
					  ee, 1 << CONSTANT_Class);
	    if (exceptionOccurred(ee))
		return NULL;
	    sub_array_cb[i - 1] = 
		constant_pool[CONSTANT_POOL_ARRAY_CLASS_INDEX].p;
	} else {
	    sub_array_cb[i - 1] = 0;
	}
    }
    /* Call a recursive function that does all the work */
    return MultiArrayAlloc2(dimensions, sub_array_cb, sizes, base_type);
}

/* This function does all the actual work for MultiArrayAlloc(), after that
 * function has done all the checking and initial work. 
 */
static HObject *
MultiArrayAlloc2(int dimensions, ClassClass **types, stack_item *sizes, 
		 int32_t base_type)
{
    int32_t size = sizes[0].i;	/* size of this dimension */
    HObject *ret;
    int32_t i;

    if (types[0] != 0) {
	ret = ArrayAlloc(T_CLASS, size); /* generate the array */
	if (ret == 0) 
	    return 0;
	/* fill in the type  */
	unhand(((HArrayOfObject *)ret))->body[size] = (HObject*)types[0];
	if (dimensions > 1) {
	    /* fill in the subarrays */
	    for (i = 0; i < size; i++) {
		HObject *sub = MultiArrayAlloc2(dimensions - 1, types + 1, 
						sizes + 1, base_type);
		if (sub == 0)	/* out of memory? */
		    return 0;
		unhand(((HArrayOfObject *)ret))->body[i] = sub;
	    } 
	} else {
	    /* this is the last dimension.  The GC zero'd the array */
	}
    } else {
	sysAssert(dimensions == 1);
	ret = ArrayAlloc(base_type, size);
	if (ret == 0)		/* out of memory? */
	    return 0;
    }
    return ret;
}



/*
    This routine is used to get the current time in milliseconds for
    the profiling routines.
*/
long now()
{
    return(sysGetMilliTicks());
}


/* 
 * Should  a call to the specified methodblock be turned into
 * an invokespecial_quick instead of a invokenonvirtual_quick?
 *
 * The four conditions that have to be satisfied:
 *    The method isn't private
 *    The method isn't an <init> method
 *    The ACC_SUPER flag is set in the current class
 *    The method's class is a superclass (and not equal) to the current class.
 * hack: ...but ACC_SUPER is never set :-/... and it conflicts with ACC_SYNCHRONIZED :-/ 1.0.2 bug? :-/ 
 * hack: See section 13.4.5 of the Java Lang Spec for discussion of this flag,
 *       and how it solves a bug, and remains backwards compatible with "older"
 *       compiled code
 */ 
bool_t
isSpecialSuperCall(ClassClass *current_class, struct methodblock *mb) {
    if (((cbAccess(current_class) & ACC_SUPER) != 0)
	  && (!IsPrivate(mb->fb.access))
	  && (strcmp(mb->fb.name, "<init>") != 0)
	  && (current_class != classJavaLangObject)) {
	ClassClass *methodClass = fieldclass(&mb->fb);
	ClassClass *cb = unhand(cbSuperclass(current_class));
	for (; ; cb = unhand(cbSuperclass(cb))) {
	    if (cb == methodClass) 
		return TRUE;
	    if (cbSuperclass(cb) == 0)
		return FALSE;
	}
    }
    return FALSE;
}

/*  __ExecuteJavaStart__ Search for __ExecuteJavaEnd__ find the end of the function */
/* Execute bytecodes at "pc" in the specified execution environment.
   Returns TRUE if completed OK, FALSE if finished because of some
   uncaught exception. */

bool_t ExecuteJava(unsigned char *initial_pc, ExecEnv *ee) 
{
/* This is ugly.  Some machines (and compilers) produce better code from 
 * positive offsets than from negative offsets.  So we cache ee->optop in
 * a local variable, but have it point OPTOP_OFFSET words further down the 
 * stack.  That way, stack references will be a nonnegative index off offset
 */

/* avh: using a static buffer to print longs is unsafe */
static char longstringbuf[30];

#define OPTOP_OFFSET 4

#define S_INFO(offset)    (optop[offset + OPTOP_OFFSET])
#define S_INT(offset)     (S_INFO(offset).i)
#define S_FLOAT(offset)   (S_INFO(offset).f)
#define S_HANDLE(offset)  (S_INFO(offset).h)
#define S_OBJECT(offset)  (S_INFO(offset).o)
#define S_POINTER(offset) (S_INFO(offset).p)
#define S_OSTRING(offset) (Object2CString(S_HANDLE(offset)))
#define S_LONGSTRING(offset) \
        (int642CString(S_LONG(offset), longstringbuf, sizeof(longstringbuf)))

#ifdef OSF1
#define S_DOUBLE(offset)  (S_INFO(offset).d)
#define S_DOUBLE2(offset) (S_INFO(offset).d)
#define S_LONG(offset)    (S_INFO(offset).l)
#define S_LONG2(offset)   (S_INFO(offset).l)
#define SET_S_DOUBLE(offset, value) S_INFO(offset).d = (value)
#define SET_S_LONG(offset, value)   S_INFO(offset).l = (value)
#else
#define S_DOUBLE(offset)  GET_DOUBLE(tdub,&(S_INFO(offset)))
#define S_DOUBLE2(offset)  GET_DOUBLE(tdub2,&(S_INFO(offset)))
#define S_LONG(offset)    GET_INT64(tdub,&(S_INFO(offset)))
#define S_LONG2(offset)    GET_INT64(tdub2,&(S_INFO(offset)))
#define SET_S_DOUBLE(offset, value) SET_DOUBLE(tdub,&(S_INFO(offset)), (value))
#define SET_S_LONG(offset, value)   SET_INT64(tdub,&(S_INFO(offset)), (value))
#endif

#define REAL_OPTOP()      (&S_INFO(0))

#ifdef DEBUG
#define DECACHE_OPTOP()   frame->optop = REAL_OPTOP(); ee->debugFlags &= ~EE_DEBUG_OPTOP_CACHED;
#define CACHE_OPTOP()     optop = frame->optop - OPTOP_OFFSET; ee->debugFlags |= EE_DEBUG_OPTOP_CACHED;
#else
#define DECACHE_OPTOP()   (frame->optop = REAL_OPTOP())
#define CACHE_OPTOP()     (optop = frame->optop - OPTOP_OFFSET)
#endif

#define SIZE_AND_STACK(size, stack) pc += size; optop += stack; continue
#define SIZE_AND_STACK_BUT(size, stack) pc += size; optop += stack;
#define JUMP_AND_STACK(offset, stack) \
          if (1) { pc += offset; optop += stack; \
                   if (offset > 0) continue; \
	           goto check_for_exception; } else
/*
#define pc2signedshort(pc)  ((((signed char *)(pc))[1] << 8) | pc[2])
#define pc2signedlong(pc)  ((((signed char *)(pc))[1] << 24) | (pc[2]  << 16) \
			      | (pc[3] << 8) | (pc[4]))
*/
#define pc2signedshort(pc)  ((int16_t)((SSCAST(UCAT(pc,1)) << 8) | UCAT(pc,2)))
#define pc2signedlong(pc)\
	((int32_t)(\
		  (SLCAST(UCAT(pc,1)) << 24)\
		| (SLCAST(UCAT(pc,2)) << 16)\
		| (SLCAST(UCAT(pc,3)) <<  8)\
		|  SLCAST(UCAT(pc,4))       \
	))

/* I can't use the do { } while(0) trick because the compiler complains about
 * the test not bein reachable.  So I'm forced to use an uglier trick.
 */
#undef  ERROR			/* Avoid conflict on PC */
#ifdef BREAKPTS
#define ERROR(name, data) \
        if (1) { frame->lastpc = pc;  \
		 DECACHE_OPTOP(); \
	         SignalError(ee, JAVAPKG name, data); \
                 notify_debugger_of_exception(pc, ee, ee->exception.exc); \
                 goto handle_exception; } else 
#else
#define ERROR(name, data) \
        if (1) { frame->lastpc = pc;  \
		 DECACHE_OPTOP(); \
	         SignalError(ee, JAVAPKG name, data); \
                 goto handle_exception; } else 
#endif

    register unsigned char  *pc = initial_pc;
    register JavaFrame       *frame = ee->current_frame;
    JavaFrame                *initial_frame = frame;
    register stack_item     *optop;
    register stack_item     *vars = frame->vars;

    unsigned char            opcode;
    cp_item_type            *constant_pool = frame->constant_pool;
 
    int	    args_size;

	/*	Declarations for commonly used temporary variables.   Note that
		some implementations will need to clear these explicitly in order
		to ensure better garbage collection. */ 

   struct fieldblock       *fb;
   struct methodblock      *mb;
#if defined(XP_MAC) || (defined(XP_PC) && !defined(_WIN32))
   struct methodblock      **mbt;
#endif
   stack_item              *location;
   ClassClass              *cb, *array_cb;
   JHandle                 *o, *h;
   OBJECT                  *d;
   ClassClass              *current_class;
   ClassClass              *superClass;
   HObject                 *retO;
   JHandle                 *retH;
   HArrayOfChar            *arr;
   char                    *signature;
   HArrayOfObject          *retArray;
  
   char                    isig;
   unsigned int            offset;
   int                     delta;

    Java8 tdub, tdub2;

    /*
     * Never enter ExecuteJava(...) if the previous ee->optop has not been
     * DECACHED().  If this happens the GC will not properly scan the 
     * previous java frame for roots...
     */
    sysAssert( !(ee->debugFlags & EE_DEBUG_OPTOP_CACHED) );

    /*
     * It is necessary for proper java stack scavenging during GC that the 
     * EE passed into ExecuteJava(...) is the SAME as the thread's EE...
     *
     * Otherwise, any java stack frames will NOT be scanned for GC roots
     */
    sysAssert( (ExecEnv *) THREAD(threadSelf())->eetop == ee );
    CACHE_OPTOP();

    if (!threadCheckStack()) {
	/*
	 * Got a stack overflow: have to be careful to not do anything
	 * that writes to the stack.  "StackOverflowError" is a 
	 * special kind of exception, because we can't call SignalError().
	 */
	frame->lastpc = pc;
	ee->exceptionKind = EXCKIND_STKOVRFLW;
#ifdef DEBUG
        ee->debugFlags &= ~EE_DEBUG_OPTOP_CACHED;
#endif
	return FALSE;
    }

#ifdef TRACING
    {
	extern PRLogModule MethodLog;
	if (PR_LOG_TEST(Method, out)) {
	    /* If netscape trace log messages are turned on, 
	       we'd better turn tracing on too: */
	    tracem = 1;
	}
    }
    if (trace) {
	char where_am_i[100];

	pc2string(pc, frame->current_method, where_am_i, where_am_i + sizeof where_am_i);
	fprintf(stdout, "Entering %s\n", where_am_i);
	fflush(stdout);
    }
#endif /* TRACING */

    while (1) {
        sysAssert(REAL_OPTOP() >= frame->ostack);
	sysAssert(!frame->current_method || 
	       REAL_OPTOP() <= frame->ostack + frame->current_method->maxstack);

#ifdef JAVA_PREEMPT_USING_COUNT
        if (ee->opcodeCount++ >= JAVA_YIELD_COUNT) {
            if (!exceptionOccurred(ee)) {
                ee->opcodeCount   = 0;
                ee->exceptionKind = EXCKIND_YIELD;
                goto check_for_exception;
            } else {
                sysAssert(0);
            }
        }
#endif

#ifdef DBINFO 
	frame->lastpc = pc;
#ifdef BREAKPTS
	if (single_stepping > 0 && 
            unhand((HThread *)(ee->thread))->single_step &&
            frame->current_method != NULL) {
	    DECACHE_OPTOP();
	    notify_debugger(threadSelf(), pc, NULL, NULL);
	}
#endif
#endif

	opcode = *pc;
again:
#ifdef STATISTICS
	instruction_count[opcode]++;
#endif
	switch (opcode) { 

	      /* NOPs.  Don't do anything. */
	case opc_nop:
	    TRACE(("\tnop\n"));
	    SIZE_AND_STACK(1, 0);

	case opc_aload:
	case opc_iload:
        case opc_fload:
	    S_INFO(0) = vars[pc[1]];
	    TRACEIF(opc_iload, ("\tiload r%d => %ld\n", pc[1], S_INT(0)));
	    TRACEIF(opc_aload, ("\taload r%d => %s\n", pc[1], S_OSTRING(0)));
	    TRACEIF(opc_fload, ("\tfload r%d => %g\n", pc[1], S_FLOAT(0)));
	    SIZE_AND_STACK(2, 1);
	      
	case opc_lload:
	case opc_dload: {
	    int reg = pc[1];
	    S_INFO(0) = vars[reg];
	    S_INFO(1) = vars[reg + 1];
	    TRACEIF(opc_lload, ("\tlload r%d => %s\n", reg, S_LONGSTRING(0)));
	    TRACEIF(opc_dload, ("\tdload r%d => %g\n", reg, S_DOUBLE(0)));
	    SIZE_AND_STACK(2, 2);
	}

#define OPC_DO_LOAD_n(num)                                              \
	case opc_iload_##num:                                           \
	case opc_aload_##num:                                           \
	case opc_fload_##num:                                           \
	    S_INFO(0) = vars[num];                                      \
	    TRACEIF(opc_iload_##num,                                    \
		    ("\t%s => %ld\n", opnames[opcode], S_INT(0)));       \
	    TRACEIF(opc_aload_##num,                                    \
		    ("\t%s => %s\n", opnames[opcode], S_OSTRING(0)));   \
	    TRACEIF(opc_fload_##num,                                    \
		    ("\t%s => %g\n", opnames[opcode], S_FLOAT(0)));     \
	    SIZE_AND_STACK(1, 1);					\
        case opc_lload_##num:                                         	\
	case opc_dload_##num:	                                        \
	    S_INFO(0) = vars[num];                                      \
	    S_INFO(1) = vars[num + 1];                                  \
	    TRACEIF(opc_lload_##num,                                    \
		    ("\t%s => %s\n", opnames[opcode], S_LONGSTRING(0))); \
	    TRACEIF(opc_dload_##num,                                    \
		    ("\t%s => %g\n", opnames[opcode], S_DOUBLE(0)));    \
	    SIZE_AND_STACK(1, 2);


	OPC_DO_LOAD_n(0)
	OPC_DO_LOAD_n(1)
	OPC_DO_LOAD_n(2)
	OPC_DO_LOAD_n(3)

	      /* store a local variable onto the stack. */
	case opc_istore:
	case opc_astore:
	case opc_fstore:
	    vars[pc[1]] = S_INFO(-1);
	    TRACEIF(opc_istore,("\tistore %ld => r%d\n", S_INT(-1), pc[1]));
	    TRACEIF(opc_astore,("\tastore %s => r%d\n", S_OSTRING(-1),pc[1]));
	    TRACEIF(opc_fstore,("\tfstore %g => r%d\n", S_FLOAT(-1), pc[1]));
	    SIZE_AND_STACK(2, -1);

	case opc_lstore:
	case opc_dstore: {
	    int reg = pc[1];
	    vars[reg] = S_INFO(-2);
	    vars[reg + 1] = S_INFO(-1);
	    TRACEIF(opc_lstore, ("\tlstore %s => r%d\n", S_LONGSTRING(-2),reg));
	    TRACEIF(opc_dstore, ("\tdstore %g => r%d\n", S_DOUBLE(-2), reg));
	    SIZE_AND_STACK(2, -2);
	}

#define OPC_DO_STORE_n(num)                                             \
	case opc_istore_##num:				        	\
	case opc_astore_##num: 						\
	case opc_fstore_##num: 						\
	    vars[num] = S_INFO(-1);  					\
	    TRACEIF(opc_istore_##num, ("\t%s %ld => r%d\n", 		\
		       opnames[opcode], S_INT(-1), num));  		\
	    TRACEIF(opc_astore_##num, ("\t%s %d => r%d\n", 		\
			      opnames[opcode], S_OSTRING(-1), num)); 	\
	    TRACEIF(opc_fstore_##num, ("\t%s %g => r%d\n", 		\
			      opnames[opcode], S_FLOAT(-1), num)); 	\
	    SIZE_AND_STACK(1, -1);                                      \
	case opc_lstore_##num: 						\
	case opc_dstore_##num: 						\
	    vars[num] = S_INFO(-2); 					\
	    vars[num + 1] = S_INFO(-1); 				\
	    TRACEIF(opc_lstore_##num, ("\t%s %s => r%d\n", 		\
			     opnames[opcode], S_LONGSTRING(-2), num)); 	\
	    TRACEIF(opc_dstore_##num, ("\t%s %g => r%d\n", 		\
			     opnames[opcode], S_DOUBLE(-2), num)); 	\
	    SIZE_AND_STACK(1, -2);

	OPC_DO_STORE_n(0)
	OPC_DO_STORE_n(1)
	OPC_DO_STORE_n(2)
	OPC_DO_STORE_n(3)

	    
	    /* Return a value from a function */
	case opc_areturn:
	case opc_ireturn:
	case opc_freturn:
	    frame->prev->optop[0] = S_INFO(-1);
	    frame->prev->optop++;
	    TRACEIF(opc_ireturn, ("\tireturn %ld\n", S_INT(-1)));
	    TRACEIF(opc_areturn, ("\tareturn %s\n", S_OSTRING(-1)));
	    TRACEIF(opc_freturn, ("\tfreturn %g\n", S_FLOAT(-1)));
	    goto finish_return;
	      
	case opc_lreturn:
	case opc_dreturn:
	    frame->prev->optop[0] = S_INFO(-2);
	    frame->prev->optop[1] = S_INFO(-1);
	    frame->prev->optop += 2;
	    TRACEIF(opc_lreturn, ("\tlreturn %s\n", S_LONGSTRING(-2)));
	    TRACEIF(opc_dreturn, ("\tdreturn %g\n", S_DOUBLE(-2)));
	    goto finish_return;

	    /* Return from a function with no value */
	case opc_return:
	    TRACE(("\treturn\n"));
	    goto finish_return;

	case opc_i2f:		/* Convert top of stack int to float */
	    S_FLOAT(-1) = (float) S_INT(-1);
	    TRACE(("\ti2f => %g\n", S_FLOAT(-1)));
	    SIZE_AND_STACK(1, 0);

	case opc_i2l:		/* Convert top of stack int to long */
	    SET_S_LONG(-1, int2ll(S_INT(-1)));
	    TRACE(("\ti2l => %s\n", S_LONGSTRING(-1)));
	    SIZE_AND_STACK(1, 1);

	case opc_i2d:
	    SET_S_DOUBLE(-1, S_INT(-1));
	    TRACE(("\ti2d => %g\n", S_DOUBLE(-1)));
	    SIZE_AND_STACK(1, 1);
	      
	case opc_f2i:		/* Convert top of stack float to int */
	    S_INT(-1) = (int32_t) S_FLOAT(-1);
	    TRACE(("\tf2i => %ld\n", S_INT(-1)));
	    SIZE_AND_STACK(1, 0);

	case opc_f2l:		/* convert top of stack float to long */
	    SET_S_LONG(-1, float2ll(S_FLOAT(-1)));
	    TRACE(("\tf2l => %s\n", S_LONGSTRING(-1)));
	    SIZE_AND_STACK(1, 1);
	      
	case opc_f2d:		/* convert top of stack float to double */
	    SET_S_DOUBLE(-1, S_FLOAT(-1));
	    TRACE(("\tf2d => %g\n", S_DOUBLE(-1)));
	    SIZE_AND_STACK(1, 1);

	case opc_l2i:		/* convert top of stack long to int */
	    S_INT(-2) = ll2int(S_LONG(-2));
	    TRACE(("\tl2i => %ld\n", S_INT(-2)));
	    SIZE_AND_STACK(1, -1);
	      
	case opc_l2f:		/* convert top of stack long to float */
	    S_FLOAT(-2) = ll2float(S_LONG(-2));
	    HACK_NOP();
	    TRACE(("\tl2f => %g\n", S_FLOAT(-2)));
	    SIZE_AND_STACK(1, -1);

	case opc_l2d:		/* convert top of stack long to double */
	    SET_S_DOUBLE(-2, ll2double(S_LONG(-2)));
	    HACK_NOP();
	    TRACE(("\tl2d => %g\n", S_DOUBLE(-2)));
	    SIZE_AND_STACK(1, 0);

	case opc_d2i:
	    S_INT(-2) = (int32_t) S_DOUBLE(-2);
	    TRACE(("\td2i => %ld\n", S_INT(-2)));
	    SIZE_AND_STACK(1, -1);
	      
	case opc_d2f:
	    S_FLOAT(-2) = (float) S_DOUBLE(-2);
	    HACK_NOP();
	    TRACE(("\td2f => %g\n", S_FLOAT(-2)));
	    SIZE_AND_STACK(1, -1);

	case opc_d2l:
	    SET_S_LONG(-2, double2ll(S_DOUBLE(-2)));
	    TRACE(("\td2l => %s\n", S_LONGSTRING(-2)));
	    SIZE_AND_STACK(1, 0);

	case opc_int2byte:
	    S_INT(-1) = (signed char) S_INT(-1);
	    TRACE(("\tint2byte => %ld\n", S_INT(-1)));
	    SIZE_AND_STACK(1, 0);
	      
	case opc_int2char:
	    S_INT(-1) &= 0xFFFF;
	    TRACE(("\tint2char => %ld\n", S_INT(-1)));
	    SIZE_AND_STACK(1, 0);

	case opc_int2short:
	    S_INT(-1) = (signed short) S_INT(-1);
	    TRACE(("\tint2short => %ld\n", S_INT(-1)));
	    SIZE_AND_STACK(1, 0);

        case opc_fcmpl:
        case opc_fcmpg: {
	    float l = S_FLOAT(-2);
	    float r = S_FLOAT(-1);
	    int value =   (l < r) ? -1 : (l > r) ? 1 : (l == r ) ?  0 
	                /* Beyond here, must be NaN. */
	                : (opcode == opc_fcmpl) ? -1 
			: (opcode == opc_fcmpg) ? 1 
			: 0;
	    S_INT(-2) = value;
	    TRACE(("\t%s => %ld\n", opnames[opcode], S_INT(-2))); 
	    SIZE_AND_STACK(1, -1);
	}

	case opc_lcmp: {
	    int64_t l = S_LONG(-4);
	    int64_t r = S_LONG(-2);
	    S_INT(-4) = (ll_lt(l, r)) ? -1 : (ll_gt(l, r)) ? 1 : 0;
	    TRACE(("\tlcmp => %ld\n", S_INT(-4))); 
	    SIZE_AND_STACK(1, -3);
	}

	case opc_dcmpl: 
	case opc_dcmpg: {
	    double l = S_DOUBLE(-4);
	    double r = S_DOUBLE(-2);
	    int value =   (l < r) ? -1 : (l > r) ? 1 : (l == r ) ?  0 
	               /* Beyond here, must be NaN. */
			: (opcode == opc_dcmpl) ? -1 
			: (opcode == opc_dcmpg) ? 1 
			: 0;
	    S_INT(-4) = value;
	    TRACE(("\t%s => %ld\n", opnames[opcode], S_INT(-4))); 
	    SIZE_AND_STACK(1, -3);
	}
	      
	    /* Push a 1-byte signed integer value onto the stack. */
	case opc_bipush:	
	    S_INT(0) = (signed char)(pc[1]);
	    TRACE(("\tbipush %ld\n", S_INT(0)));
	    SIZE_AND_STACK(2, 1);

	    /* Push a 2-byte signed integer constant onto the stack. */
	case opc_sipush:	
	    S_INT(0) = pc2signedshort(pc);
	    TRACE(("\tsipush %ld\n", S_INT(0)));
	    SIZE_AND_STACK(3, 1);

	case opc_ldc:
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE
	        ResolveClassConstant(constant_pool, pc[1], ee, 
				     (1 << CONSTANT_Integer) | 
				     (1 << CONSTANT_Float) |
				     (1 << CONSTANT_String));
	        if (!exceptionOccurred(ee))
		    pc[0] = opc_ldc_quick;
	    END_WITH_OPCODE_CHANGE
	    goto check_for_exception;

	case opc_ldc_quick:
	    S_POINTER(0) = constant_pool[pc[1]].p;
	    TRACE(("\tldc1 #%d => 0x%lx\n", pc[1], S_INT(0)));
	    SIZE_AND_STACK(2, 1);

	case opc_ldc_w:
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE
	        ResolveClassConstant(constant_pool, GET_INDEX(pc + 1), ee, 
				     (1 << CONSTANT_Integer) | 
				     (1 << CONSTANT_Float) |
				     (1 << CONSTANT_String));
	        if (!exceptionOccurred(ee))
		    pc[0] = opc_ldc_w_quick;
	    END_WITH_OPCODE_CHANGE
	    goto check_for_exception;

	case opc_ldc_w_quick:
	    S_POINTER(0) = constant_pool[GET_INDEX(pc + 1)].p;
	    TRACE(("\tldc2 #%ld => 0x%lx\n", GET_INDEX(pc + 1), S_INT(0)));
	    SIZE_AND_STACK(3, 1);

	case opc_ldc2_w:	/* load a two word constant */
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE
	        ResolveClassConstant(constant_pool, GET_INDEX(pc + 1), ee,
				 (1 << CONSTANT_Double) | (1 << CONSTANT_Long));
	        if (!exceptionOccurred(ee))
		    pc[0] = opc_ldc2_w_quick;
	    END_WITH_OPCODE_CHANGE
	    goto check_for_exception;

	case opc_ldc2_w_quick:
	    SET_S_LONG(0, GET_INT64(tdub2, &constant_pool[GET_INDEX(pc + 1)]));
	    TRACE(("\tldc2 #%d => long=%s double=%g\n", GET_INDEX(pc + 1), 
		   S_LONGSTRING(0), S_DOUBLE2(0)));
	    SIZE_AND_STACK(3, 2);

#define CONSTANT_OP(opcode, this_type, value) 				\
	case opcode: 							\
	    S_INFO(0).this_type = value; 				\
	    TRACE(("\t%s\n", opnames[opcode])); 			\
	    SIZE_AND_STACK(1, 1);
	  
	    /* Push miscellaneous constants onto the stack. */
	CONSTANT_OP(opc_aconst_null, p,        0)
	CONSTANT_OP(opc_iconst_m1,   i,       -1)
	CONSTANT_OP(opc_iconst_0,    i,        0)
	CONSTANT_OP(opc_iconst_1,    i,        1)
	CONSTANT_OP(opc_iconst_2,    i,        2)
	CONSTANT_OP(opc_iconst_3,    i,        3)
	CONSTANT_OP(opc_iconst_4,    i,        4)
	CONSTANT_OP(opc_iconst_5,    i,        5)
	CONSTANT_OP(opc_fconst_0,    f,      (float)0.0)
	CONSTANT_OP(opc_fconst_1,    f,      (float)1.0)
	CONSTANT_OP(opc_fconst_2,    f,      (float)2.0)

#if GENERATING68K
	default: 
		switch (opcode) {
#endif

	case opc_dconst_0:
	    SET_S_DOUBLE(0, 0.0);
	    TRACE(("\t%s\n", opnames[opcode]));
	    SIZE_AND_STACK(1, 2);

	case opc_dconst_1:
	    SET_S_DOUBLE(0, 1.0);
	    TRACE(("\t%s\n", opnames[opcode]));
	    SIZE_AND_STACK(1, 2);

	case opc_lconst_0:
	    SET_S_LONG(0, ll_zero_const);
	    TRACE(("\t%s\n", opnames[opcode]));
	    SIZE_AND_STACK(1, 2);

	case opc_lconst_1:
   	    SET_S_LONG(0, ll_one_const);
	    TRACE(("\t%s\n", opnames[opcode]));
	    SIZE_AND_STACK(1, 2);

#define BINARY_INT_OP(name, op, test)                               \
	case opc_i##name:                                          	\
	    if (test && S_INT(-1) == 0)                              	\
               ERROR("ArithmeticException", "/ by zero");               \
	    S_INT(-2) = S_INT(-2) op S_INT(-1);                       	\
	    TRACE(("\t%s => %ld\n", opnames[opcode], S_INT(-2)));      	\
	    SIZE_AND_STACK(1, -1);                                    	\
	case opc_l##name:                                             	\
	    if (test && ll_eqz(S_LONG(-2)))                             \
               ERROR("ArithmeticException", "/ by zero");               \
	    SET_S_LONG(-4, ll_##name(S_LONG(-4), S_LONG2(-2)));          \
	    TRACE(("\t%s => %s\n", opnames[opcode], S_LONGSTRING(-4))); \
	    SIZE_AND_STACK(1, -2);

	      /* Perform various binary integer operations */
	BINARY_INT_OP(add, +, 0) 
	BINARY_INT_OP(sub, -, 0)
	BINARY_INT_OP(mul, *, 0)
	BINARY_INT_OP(and, &, 0)
	BINARY_INT_OP(or,  |, 0)
	BINARY_INT_OP(xor, ^, 0)
	BINARY_INT_OP(div, /, 1)
	BINARY_INT_OP(rem, %, 1)

#define BINARY_SHIFT_OP(name, op)                               	\
	case opc_i##name:                                          	\
	    S_INT(-2) = ((int32_t)(S_INT(-2))) op (S_INT(-1) & 0x1F);       \
	    TRACE(("\t%s => %ld\n", opnames[opcode], S_INT(-2)));      	\
	    SIZE_AND_STACK(1, -1);                                    	\
	case opc_l##name:                                             	\
	    SET_S_LONG(-3, ll_##name(S_LONG(-3), (S_INT(-1) & 0x3F)));	\
	    TRACE(("\t%s => %s\n", opnames[opcode], S_LONGSTRING(-3))); \
	    SIZE_AND_STACK(1, -1);                                      \

	BINARY_SHIFT_OP(shl, <<)
	BINARY_SHIFT_OP(shr, >>)

	case opc_iushr:
	    S_INT(-2) = ((uint32_t)(S_INT(-2))) >> (S_INT(-1) & 0x1F);
	    TRACE(("\t%s => %ld\n", opnames[opcode], S_INT(-2)));
	    SIZE_AND_STACK(1, -1);
	case opc_lushr:
	    SET_S_LONG(-3, ll_ushr(S_LONG(-3), (S_INT(-1) & 0x3F)));
	    TRACE(("\t%s => %s\n", opnames[opcode], S_LONGSTRING(-3)));
	    SIZE_AND_STACK(1, -1); 

	      /* Unary negation */
	case opc_ineg:
	    S_INT(-1) = -S_INT(-1);
	    TRACE(("\tineg => %ld\n", S_INT(-1)));
	    SIZE_AND_STACK(1, 0);

	case opc_lneg:
            SET_S_LONG(-2, ll_neg(S_LONG(-2)));
	    TRACE(("\tlneg => %ld\n", S_LONG(-2)));
	    SIZE_AND_STACK(1, 0);
	    
	    /* Add a small constant to a local variable */
	case opc_iinc:
	    vars[pc[1]].i += ((signed char *) pc)[2];
	    TRACE(("\tiinc r%d+%d => %ld\n", 
		   pc[1], ((signed char *) pc)[2], vars[pc[1]].i));
	    SIZE_AND_STACK(3, 0);


#define BINARY_FLOAT_OP(name, op) 					\
	case opc_f##name: 						\
	    S_FLOAT(-2) = S_FLOAT(-2) op S_FLOAT(-1);                 	\
	    HACK_NOP();						        \
	    TRACE(("\t%s => %g\n", opnames[opcode], S_FLOAT(-2))); 	\
	    SIZE_AND_STACK(1, -1); 					\
	case opc_d##name: 						\
	    SET_S_DOUBLE(-4, S_DOUBLE(-4) op S_DOUBLE2(-2));   	        \
	    TRACE(("\t%s => %g\n", opnames[opcode], S_DOUBLE(-4)));     \
	    SIZE_AND_STACK(1, -2);

	BINARY_FLOAT_OP(add, +)
	BINARY_FLOAT_OP(sub, -)
	BINARY_FLOAT_OP(mul, *)
	BINARY_FLOAT_OP(div, /)

	case opc_frem: 
	    S_FLOAT(-2) = (float) DREM(S_FLOAT(-2), S_FLOAT(-1));
	    TRACE(("\t%s => %g\n", opnames[opcode], S_FLOAT(-2))); 
	    SIZE_AND_STACK(1, -1);
	      
	case opc_drem: 
	    SET_S_DOUBLE(-4, DREM(S_DOUBLE(-4), S_DOUBLE2(-2)));
	    TRACE(("\t%s => %g\n", opnames[opcode], S_DOUBLE(-4)));
	    SIZE_AND_STACK(1, -2);

	case opc_fneg:
	    S_FLOAT(-1) = -S_FLOAT(-1);
	    TRACE(("\tfneg => %g\n", S_FLOAT(-1)));
	    SIZE_AND_STACK(1, 0);

	case opc_dneg:
	    SET_S_DOUBLE(-2, -S_DOUBLE(-2));
	    HACK_NOP();
	    TRACE(("\tdneg => %g\n", S_DOUBLE(-2)));
	    SIZE_AND_STACK(1, 0);

	    /* Unconditional goto.  Maybe save return address in register. */
	case opc_jsr:
	    S_INFO(0).addr = pc + 3;
	    optop++;
	    /* FALL THROUGH */

	case opc_goto: {
	    int skip = pc2signedshort(pc);
	    TRACE(("\t%s %8X [.+(%d)]\n", opnames[opcode], pc + skip, skip));
	    JUMP_AND_STACK(skip, 0);
	}

	case opc_jsr_w:
	    S_INFO(0).addr = pc + 5;
	    optop++;
	    /* FALL THROUGH */

	case opc_goto_w: {
	    int32_t skip = pc2signedlong(pc);
	    TRACE(("\t%s %8X\n", opnames[opcode], pc + skip));
	    JUMP_AND_STACK(skip, 0);
	}

	    /* Return from subroutine.  June to address in register. */
	case opc_ret:
	    TRACE(("\tret %d (%8X)\n", pc[1], vars[pc[1]].addr));
	    pc = vars[pc[1]].addr;
	    SIZE_AND_STACK_BUT(0, 0);
	    goto check_for_exception;
	      

#define COMPARISON_OP(name, comparison)                                   \
	case opc_if_icmp##name: {					  \
            int skip = (S_INT(-2) comparison S_INT(-1))                   \
	           ? pc2signedshort(pc) : 3;                              \
            TRACE(("\t%s goto %8lX (%staken)\n",                           \
		   opnames[opcode],                                       \
		   (uint32_t)(pc + pc2signedshort(pc)),                               \
		   (S_INT(-2) comparison S_INT(-1)) ? "" : "not "));	  \
            JUMP_AND_STACK(skip, -2); 					  \
        }								  \
	case opc_if##name: {					     	  \
            int skip = (S_INT(-1) comparison 0) ? pc2signedshort(pc) : 3; \
            TRACE(("\t%s goto %8lX (%staken) [.+(%d)]\n",                           \
		   opnames[opcode],                                       \
		   (uint32_t)(pc + pc2signedshort(pc)),                               \
		   (S_INT(-1) comparison 0) ? "" : "not ", skip));	          \
            JUMP_AND_STACK(skip, -1); 					  \
        }                                                                 \
            
#define COMPARISON_OP2(name, nullname, comparison)                        \
        COMPARISON_OP(name, comparison)                                   \
	case opc_if_acmp##name: {					  \
            int skip = (S_POINTER(-2) comparison S_POINTER(-1))           \
	           ? pc2signedshort(pc) : 3;                              \
            TRACE(("\t%s goto %8lX (%staken)\n",                           \
		   opnames[opcode],                                       \
		   (uint32_t)(pc + pc2signedshort(pc)),                               \
		   (S_POINTER(-2) comparison S_POINTER(-1)) ? "" : "not ")); \
            JUMP_AND_STACK(skip, -2); 					  \
        }								  \
	case opc_if##nullname: {				     	  \
            int skip = (S_POINTER(-1) comparison NULL) ? pc2signedshort(pc) : 3; \
            TRACE(("\t%s goto %8lX (%staken)\n",                           \
		   opnames[opcode],                                       \
		   (uint32_t) (pc + pc2signedshort(pc)),                               \
		   (S_INT(-1) comparison 0) ? "" : "not "));	          \
            JUMP_AND_STACK(skip, -1); 					  \
        }                                                                 \


	COMPARISON_OP2(eq, null, ==)	/* also generate acmp_eq and acmp_ne */
	COMPARISON_OP2(ne, nonnull, !=)
	COMPARISON_OP(lt, <)
	COMPARISON_OP(gt, >)
	COMPARISON_OP(le, <=)
	COMPARISON_OP(ge, >=)


	    /* Discard the top item on the stack */
	case opc_pop:
	    TRACE(("\tpop\n"));
	    SIZE_AND_STACK(1, -1);

	    /* Discard the top item on the stack */
	case opc_pop2:
	    TRACE(("\tpop2\n"));
	    SIZE_AND_STACK(1, -2);

	    /* Duplicate the top item on the stack */
	case opc_dup:
	    S_INFO(0) = S_INFO(-1);
	    TRACE(("\tdup\n"));
	    SIZE_AND_STACK(1, 1);

	case opc_dup2:		/* Duplicate the top two items on the stack */
	    S_INFO(0) = S_INFO(-2);
	    S_INFO(1) = S_INFO(-1);
	    TRACE(("\tdup2\n"));
	    SIZE_AND_STACK(1, 2);

	case opc_dup_x1:	/* insert S_INFO(-1) on stack past 1 */
	    S_INFO(0) = S_INFO(-1);
	    S_INFO(-1) = S_INFO(-2);
	    S_INFO(-2) = S_INFO(0);
	    TRACE(("\tdup_x1\n"));
	    SIZE_AND_STACK(1, 1);
	      
	case opc_dup_x2:	/* insert S_INFO(-1) on stack past 2*/
	    S_INFO(0) = S_INFO(-1);
	    S_INFO(-1) = S_INFO(-2);
	    S_INFO(-2) = S_INFO(-3);
	    S_INFO(-3) = S_INFO(0);
	    TRACE(("\tdup_x2\n"));
	    SIZE_AND_STACK(1, 1);

	case opc_dup2_x1:	/* insert S_INFO(-1,-2) on stack past 1 */
	    S_INFO(1) = S_INFO(-1);
	    S_INFO(0) = S_INFO(-2);
	    S_INFO(-1) = S_INFO(-3);
	    S_INFO(-2) = S_INFO(1);
	    S_INFO(-3) = S_INFO(0);
	    TRACE(("\tdup2_x1\n"));
	    SIZE_AND_STACK(1, 2);
	      
	case opc_dup2_x2:	/* insert S_INFO(-1,2) on stack past 2 */
	    S_INFO(1) = S_INFO(-1);
	    S_INFO(0) = S_INFO(-2);
	    S_INFO(-1) = S_INFO(-3);
	    S_INFO(-2) = S_INFO(-4);
	    S_INFO(-3) = S_INFO(1);
	    S_INFO(-4) = S_INFO(0);
	    TRACE(("\tdup2_x2\n"));
	    SIZE_AND_STACK(1, 2);
	      
	case opc_swap: {        /* swap top two elements on the stack */
	    stack_item j;
	    j = S_INFO(-1);
	    S_INFO(-1) = S_INFO(-2);
	    S_INFO(-2) = j;
	    SIZE_AND_STACK(1, 0);
	}

	    /* Find the length of an array */
	case opc_arraylength: {
	    arr = (HArrayOfChar *)S_HANDLE(-1);
	    if (arr == 0) 
		ERROR("NullPointerException", "trying to get array length");
	    S_INT(-1) = obj_length(arr);
	    TRACE(("\tarraylength %s => %ld\n", 
		   Object2CString((JHandle*)arr), S_INT(-1)));
	    SIZE_AND_STACK(1, 0);
	}

	    /* Goto one of a sequence of targets. */
	case opc_tableswitch: {
            unsigned char *method_base = ee->current_frame->current_method->code;
	    int32_t *lpc = (int32_t *) REL_ALIGN(method_base, pc-method_base+1); /* UCALIGN(pc + 1); */
	    int32_t key = S_INT(-1);
	    int32_t low = ntohl(lpc[1]);
	    int32_t high = ntohl(lpc[2]);
	    int32_t skip;
	    key -= low;
	    skip = ((key > high - low) || (key < 0))
		        ? ntohl(lpc[0]) 
		        : ntohl(lpc[key + 3]);
	    TRACE(("\ttableswitch %ld [%ld-%ld] .+(%ld)\n", key+low, low, high, skip));
	    JUMP_AND_STACK(skip, -1);
	}
	      
	    /* Goto one of a sequence of targets */
	case opc_lookupswitch: {
            unsigned char *method_base = ee->current_frame->current_method->code;
	    int32_t *lpc = (int32_t *) REL_ALIGN(method_base, pc-method_base+1); /* UCALIGN(pc + 1); */
	    int32_t key = S_INT(-1);
	    int32_t skip = ntohl(lpc[0]); /* default skip amount */
	    int32_t npairs = ntohl(lpc[1]);
	    TRACE(("\tlookupswitch %ld\n", key));
	    while (--npairs >= 0) {
		lpc += 2;
		if (key == (int32_t) ntohl(lpc[0])) {
		    skip = ntohl(lpc[1]);
		    break;
		}
	    }
	    JUMP_AND_STACK(skip, -1);
	}


	    /* Throw to a given value */
	case opc_athrow: {
		o = S_HANDLE(-1);
	    TRACE(("\tathrow => %s\n", Object2CString(o)));
	    if (o == 0) {
	        ERROR("NullPointerException", "trying to throw");
	    } else {
		exceptionThrow(ee, o);
#ifdef BREAKPTS
                notify_debugger_of_exception(pc, ee, ee->exception.exc);
#endif
		goto handle_exception;
	    }

	}
	    
	case opc_getfield:
	case opc_putfield: 
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE {
		cp_index_type field_index;
                CODE_LOCK();
		field_index = GET_INDEX(pc+1);
                CODE_UNLOCK();

		ResolveClassConstant(constant_pool, field_index, ee, 
				     1 << CONSTANT_Fieldref);
		fb = (struct fieldblock *)constant_pool[field_index].p;

		if (!exceptionOccurred(ee)) {
		    signature = fieldsig(fb);
		    TRACE(("\t%s %s.%s\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb)));
		    if (fb->access & ACC_STATIC) {
			char buf[256];
			int len;
			classname2string(classname(fieldclass(fb)), 
					 buf, sizeof(buf));
			len = strlen(buf);
			/* If buffer overflow, quietly truncate */
			(void) jio_snprintf(buf + len, sizeof(buf)-len,
				": field %s did not used to be static",
				fb->name);
			ERROR("IncompatibleClassChangeError", buf);
                    } else if ((fb->access & ACC_FINAL) 
                               && (opcode == opc_putfield)
                               && ((ee->current_frame->current_method == NULL) 
                                   || 
                                   (fieldclass(fb) != 
                                    fieldclass(&ee->current_frame->current_method->fb)))) {
                        char buf[256];
                        int len;
                        classname2string(classname(fieldclass(fb)), buf, sizeof(buf));
                        len = strlen(buf);
                        /* If buffer overflow, quietly truncate */
                        (void) jio_snprintf(buf + len, sizeof(buf)-len, 
                                            ": field %s is final", fb->name);
                        ERROR("IllegalAccessError", buf);
		    } else {
			offset = fb->u.offset / sizeof (OBJECT);
			if (offset <= 0xFF && !UseLosslessQuickOpcodes) { 
			    delta = (signature[0] == SIGNATURE_LONG 
					   || signature[0] == SIGNATURE_DOUBLE)
				      ? (opc_getfield2_quick - opc_getfield)
				      : (opc_getfield_quick - opc_getfield);
			    CODE_LOCK();
			      pc[1] = offset;
			      pc[0] = opcode + delta;
			    CODE_UNLOCK();
			} else { 
			    delta = (opc_getfield_quick_w - opc_getfield);
			    pc[0] = opcode + delta;
			}
		    }
		}
	    } END_WITH_OPCODE_CHANGE
	    goto check_for_exception;

	case opc_getstatic:
	case opc_putstatic:
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE {
		cp_index_type field_index = GET_INDEX(pc+1);

		ResolveClassConstant(constant_pool, field_index, ee, 
				     1 << CONSTANT_Fieldref);
		fb = (struct fieldblock *)(constant_pool[field_index].p);
		if (!exceptionOccurred(ee)) {
		    TRACE(("\t%s %s.%s\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb)));
		    if ((fb->access & ACC_STATIC) == 0) {
			char buf[256];
			int len;
			classname2string(classname(fieldclass(fb)), 
					 buf, sizeof(buf));
			len = strlen(buf);
			/* If buffer overflow, quietly truncate */
			(void) jio_snprintf(buf + len, sizeof(buf)-len,
				": field %s used to be static",
				fb->name);
			ERROR("IncompatibleClassChangeError", buf);
                    } else if ((fb->access & ACC_FINAL) 
                               && (opcode == opc_putstatic)
                               && ((ee->current_frame->current_method == NULL) 
                                   || 
                                   (fieldclass(fb) != 
                                    fieldclass(&ee->current_frame->current_method->fb)))) {
                        char buf[256];
                        int len;
                        classname2string(classname(fieldclass(fb)), buf, sizeof(buf));
                        len = strlen(buf);
                        /* If buffer overflow, quietly truncate */
                        (void) jio_snprintf(buf + len, sizeof(buf)-len, 
                                            ": field %s is final", fb->name);
                        ERROR("IllegalAccessError", buf);
		    } else {
			signature = fieldsig(fb);
			delta = (signature[0] == SIGNATURE_LONG 
				       || signature[0] == SIGNATURE_DOUBLE)
			           ? (opc_getstatic2_quick - opc_getstatic)
			           : (opc_getstatic_quick - opc_getstatic);
			pc[0] = opcode + delta;
		    }
		}
	    } END_WITH_OPCODE_CHANGE
	    goto check_for_exception;

	case opc_invokevirtual:
	case opc_invokenonvirtual:
	case opc_invokestatic:
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE {
		cp_index_type method_index;
                CODE_LOCK();
		method_index = GET_INDEX(pc+1);
                CODE_UNLOCK();
		
		ResolveClassConstant(constant_pool, method_index, ee, 
				     1 << CONSTANT_Methodref);

		if (!exceptionOccurred(ee)) {
		    mb = constant_pool[method_index].p;
		    TRACE(("\t%s %s.%s%s\n", opnames[opcode], 
			   classname(fieldclass(&mb->fb)), 
			   fieldname(&mb->fb), fieldsig(&mb->fb)));
		    if (opcode == opc_invokestatic) {
			if ((mb->fb.access & ACC_STATIC) == 0) {
			    char buf[256];
			    int len;
			    classname2string(classname(fieldclass(&mb->fb)), 
					     buf, sizeof(buf));
			    len = strlen(buf);
			    /* If buffer overflow, quietly truncate */
			    (void) jio_snprintf(buf + len, sizeof(buf)-len,
					": method %s%s used to be static",
					mb->fb.name, mb->fb.signature);
			    ERROR("IncompatibleClassChangeError", buf);
			} else { 
			    pc[0] = opc_invokestatic_quick;
			}
		    } else {
			if (mb->fb.access & ACC_STATIC) {
			    char buf[256];
			    int len;
			    classname2string(classname(fieldclass(&mb->fb)), 
					     buf, sizeof(buf));
			    len = strlen(buf);
			    /* If buffer overflow, quietly truncate */
			    (void) jio_snprintf(buf + len, sizeof(buf)-len,
					": method %s%s did not used to be static",
					mb->fb.name, mb->fb.signature);
			    ERROR("IncompatibleClassChangeError", buf);
			} 
		    }
		    if (opcode == opc_invokevirtual) {
			offset = mb->fb.u.offset; 
			if (offset <= 0xFF && !UseLosslessQuickOpcodes) {
			  /* almost always */
			  CODE_LOCK();
			      pc[1] = (unsigned char) mb->fb.u.offset; 
			      pc[2] = (unsigned char) mb->args_size; 
			      pc[0] = fieldclass(&mb->fb) == classJavaLangObject 
				  ? opc_invokevirtualobject_quick
				      : opc_invokevirtual_quick;
			   CODE_UNLOCK();
			} else { 
			    pc[0] = opc_invokevirtual_quick_w;
			}
		    } else if (opcode == opc_invokenonvirtual) {
			current_class = frame->current_method ? 
			      fieldclass(&frame->current_method->fb) : NULL;
			if ((current_class != NULL) && 
			        isSpecialSuperCall(current_class, mb)) {
			    fb_offset_t offset = mb->fb.u.offset; 
			    struct methodblock *new_mb;
			    superClass = 
				unhand(cbSuperclass(current_class));
			    new_mb = 
				cbMethodTable(superClass)->methods[offset];
			    if (mb != new_mb) { 
				CODE_LOCK();
				pc[1] = offset >> 8;
				pc[2] = offset & 0xFF;
				pc[0] = opc_invokesuper_quick;
				CODE_UNLOCK();
			    } else
				pc[0] = opc_invokenonvirtual_quick;
			} else
			    pc[0] = opc_invokenonvirtual_quick;
		    }
		}
	    } END_WITH_OPCODE_CHANGE
	    goto check_for_exception;
	  
	case opc_invokeinterface:
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE {
		cp_index_type InterfaceMethodref_index;
                CODE_LOCK();
		InterfaceMethodref_index = GET_INDEX(pc + 1);
                CODE_UNLOCK();
		ResolveClassConstant(constant_pool, InterfaceMethodref_index,
				     ee, 1 << CONSTANT_InterfaceMethodref);
		if (!exceptionOccurred(ee)) {
		    uint32_t InterfaceMethodref = 
		         constant_pool[InterfaceMethodref_index].i;
		    cp_index_type NameAndType_index = InterfaceMethodref & 0xFFFF;
		    hash_t ID = constant_pool[NameAndType_index].i;
		    TRACE(("\t%s %s\n", opnames[opcode], 
			   IDToNameAndTypeString(ID)));
		    CODE_LOCK();
		      pc[1] = (unsigned char) (NameAndType_index >> 8);
		      pc[2] = (unsigned char) (NameAndType_index & 0xFFL);
		      pc[4] = 0;	/* no guess, yet */
		      pc[0] = opc_invokeinterface_quick;
		    CODE_UNLOCK();
		}
	    } END_WITH_OPCODE_CHANGE
	    goto check_for_exception;

	case opc_putfield_quick: {
	    o = S_HANDLE(-2);
	    if (o == 0) {
		char buf[64];
		jio_snprintf(buf, sizeof(buf), "trying to set field at offset %ld",
			     (long)S_POINTER(-1));
	        ERROR("NullPointerException", buf);
	    }
	    TRACE(("\t%s %s.(#%ld) <= %lX\n", opnames[opcode], 
		   Object2CString(o), pc[1], S_POINTER(-1)));
	    obj_setslot(o, pc[1], (long)S_POINTER(-1));
	    SIZE_AND_STACK(3, -2);
	}

	case opc_getfield_quick: {
	    o = S_HANDLE(-1);
	    if (o == 0) {
		char buf[64];
		jio_snprintf(buf, sizeof(buf), "trying to get field at offset %ld",
			     (long)S_POINTER(-1));
	        ERROR("NullPointerException", buf);
	    }
	    S_POINTER(-1) = (void *)obj_getslot(o, pc[1]);
	    TRACE(("\t%s %s.(#%d) <= %lX\n", opnames[opcode], 
		   Object2CString(o), pc[1], S_POINTER(-1)));
	    SIZE_AND_STACK(3, 0);
	}
	    
	case opc_putfield2_quick: {
	    o = S_HANDLE(-3);
	    if (o == 0) {
		char buf[64];
		jio_snprintf(buf, sizeof(buf), "trying to set field at offset %ld (long/double)",
			     (long)S_POINTER(-2));
	        ERROR("NullPointerException", buf);
	    }
	    TRACE(("\t%s %s.(#%d) <= long=%s double=%g\n", opnames[opcode], 
		   Object2CString(o), pc[1], S_LONGSTRING(-2), S_DOUBLE2(-2)));
	    obj_setslot(o, pc[1], (long)S_POINTER(-2));
#ifndef OSF1
	    obj_setslot(o, pc[1] + 1, (long)S_POINTER(-1));
#endif
	    SIZE_AND_STACK(3, -3);
	}

	case opc_getfield2_quick: {
	    o = S_HANDLE(-1);
	    if (o == 0) {
		char buf[64];
		jio_snprintf(buf, sizeof(buf), "trying to get field at offset %ld (long/double)",
			     pc[1]);
	        ERROR("NullPointerException", buf);
	    }
	    S_POINTER(-1) = (void *)obj_getslot(o, pc[1]);
#ifdef OSF1
	    S_POINTER(0) = 0;
#else
	    S_POINTER(0) = (void *)obj_getslot(o, pc[1] + 1);
#endif
	    TRACE(("\t%s %s.(#%d) <= long=%s double=%g\n", opnames[opcode], 
		   Object2CString(o), pc[1], S_LONGSTRING(-1), S_DOUBLE2(-1)));
	    SIZE_AND_STACK(3, 1);
	}


	case opc_putstatic_quick: {   
	    fb = (struct fieldblock *)(constant_pool[GET_INDEX(pc+1)].p);
	    d = (OBJECT *)normal_static_address(fb);
	    *d = S_OBJECT(-1);
#ifdef TRACING
	    if (trace) {
		char isig = fieldsig(fb)[0];
		if (isig == SIGNATURE_FLOAT) {
		    TRACE(("\t%s %s.%s <= %g\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb), 
			   S_FLOAT(-1)));
		} else if (isig == SIGNATURE_CLASS || isig == SIGNATURE_ARRAY) {
		    TRACE(("\t%s %s.%s <= %s\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb), 
			   S_OSTRING(-1)));
		} else {
		    TRACE(("\t%s %s.%s <= %ld\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb), 
			   S_INT(-1)));
		}
	    }
#endif /* TRACING */
	    SIZE_AND_STACK(3, -1);
	}

	case opc_getstatic_quick: {
	    fb = (struct fieldblock *)(constant_pool[GET_INDEX(pc+1)].p);
	    d = (OBJECT *) normal_static_address(fb);
	    S_OBJECT(0) = *d;
#ifdef TRACING
	    if (trace) {
		char isig = fieldsig(fb)[0];
		if (isig == SIGNATURE_FLOAT) {
		    TRACE(("\t%s %s.%s <= %g\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb), 
			   S_FLOAT(0)));
		} else if (isig == SIGNATURE_CLASS || isig == SIGNATURE_ARRAY) {
		    TRACE(("\t%s %s.%s <= %s\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb), 
			   S_OSTRING(0)));
		} else {
		    TRACE(("\t%s %s.%s <= %ld\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb), 
			   S_INT(0)));
		} 
	    }
#endif /* TRACING */
	    SIZE_AND_STACK(3, 1);
	}

	case opc_putstatic2_quick: {   
	    fb = (struct fieldblock *)(constant_pool[GET_INDEX(pc+1)].p);
	    location = (stack_item *)twoword_static_address(fb);
	    location[0] = S_INFO(-2);
#ifndef OSF1
	    location[1] = S_INFO(-1);
#endif
#ifdef TRACING
	    if (trace) {
		signature = fieldsig(fb);
		if (signature[0] == SIGNATURE_LONG) {
		    TRACE(("\t%s %s.%s <= %s\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb), 
			   S_LONGSTRING(-2)));
		} else {
		    TRACE(("\t%s %s.%s <= %g\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb), 
			   S_DOUBLE(-2)));
		}
	    }
#endif /* TRACING */
	    SIZE_AND_STACK(3, -2);
	}

	case opc_putfield_quick_w: { 
	    fb = (struct fieldblock *)(constant_pool[GET_INDEX(pc+1)].p);
	    isig = fieldsig(fb)[0];
	    offset = fb->u.offset / sizeof (OBJECT);
	    if (isig == SIGNATURE_LONG || isig == SIGNATURE_DOUBLE) {
		o = S_HANDLE(-3);
		if (o == 0) {
		    char buf[64];
		    jio_snprintf(buf, sizeof(buf), "trying to set field %s",
				 fb->name);
		    ERROR("NullPointerException", buf);
		}
#ifdef TRACING
		if (trace) {
		    if (isig == SIGNATURE_LONG) { 
			TRACE(("\t%s %s.%s <= %s\n", opnames[opcode], 
			       Object2CString(o), fieldname(fb), 
			       S_LONGSTRING(-2)));
		    } else {
			TRACE(("\t%s %s.%s <= %g\n", opnames[opcode], 
			       Object2CString(o), fieldname(fb), 
			       S_DOUBLE(-2)));
		    }
		}
#endif /* TRACING */
		obj_setslot(o, offset, (long)S_POINTER(-2));
#ifndef OSF1
		obj_setslot(o, offset + 1, (long)S_POINTER(-1));
#endif
		SIZE_AND_STACK(3, -3);
	    } else {
		o = S_HANDLE(-2);
		if (o == 0) {
		    char buf[64];
		    jio_snprintf(buf, sizeof(buf), "trying to set field %s",
				 fb->name);
		    ERROR("NullPointerException", buf);
		}
#ifdef TRACING
		if (trace) {
		    if (isig == SIGNATURE_FLOAT) {
			TRACE(("\t%s %s.%s <= %g\n", opnames[opcode], 
			       Object2CString(o), fieldname(fb), 
			       S_FLOAT(-1)));
		    } else if (isig == SIGNATURE_CLASS || 
			       isig == SIGNATURE_ARRAY) {
			TRACE(("\t%s %s.%s <= %s\n", opnames[opcode], 
			       Object2CString(o), fieldname(fb), 
			       S_OSTRING(-1)));
		    } else {
			TRACE(("\t%s %s.%s <= %ld\n", opnames[opcode], 
			       Object2CString(o), fieldname(fb), 
			       S_INT(-1)));
		    } 
		}
#endif /* TRACING */
		obj_setslot(o, offset, (long)S_POINTER(-1));
		SIZE_AND_STACK(3, -2);
	    }
	}

	case opc_getstatic2_quick: {
	    fb = (struct fieldblock *)(constant_pool[GET_INDEX(pc+1)].p);
	    location = (stack_item *)twoword_static_address(fb);
	    S_INFO(0) = location[0];
#ifdef OSF1
	    S_LONG(1) = 0;
#else
	    S_INFO(1) = location[1];
#endif
#ifdef TRACING
	    if (trace) {
		signature = fieldsig(fb);
		if (signature[0] == SIGNATURE_LONG) {
		    TRACE(("\t%s %s.%s <= %s\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb), 
			   S_LONGSTRING(0)));
		} else {
		    TRACE(("\t%s %s.%s <= %g\n", opnames[opcode], 
			   classname(fieldclass(fb)), fieldname(fb), 
			   S_DOUBLE(0)));
		}
	    }
#endif /* TRACING */
	    SIZE_AND_STACK(3, 2);
	}

	case opc_getfield_quick_w: { 
	    fb = (struct fieldblock *)(constant_pool[GET_INDEX(pc+1)].p);
	    isig = fieldsig(fb)[0];
	    offset = fb->u.offset / sizeof (OBJECT);
	    o = S_HANDLE(-1);
	    if (o == 0) {
		char buf[64];
		jio_snprintf(buf, sizeof(buf), "trying to get field %s",
			     fb->name);
		ERROR("NullPointerException", buf);
	    }
	    if (isig == SIGNATURE_LONG || isig == SIGNATURE_DOUBLE) {
		S_POINTER(-1) = (void *)obj_getslot(o, offset);
#ifdef OSF1
		S_POINTER(0) = 0;
#else
		S_POINTER(0) = (void *)obj_getslot(o, offset + 1);
#endif
#ifdef TRACING
		if (trace) {
		    if (isig == SIGNATURE_LONG) { 
			TRACE(("\t%s %s.%s <= %s\n", opnames[opcode], 
			       Object2CString(o), fieldname(fb), 
			       S_LONGSTRING(-1)));
		    } else {
			TRACE(("\t%s %s.%s <= %g\n", opnames[opcode], 
			       Object2CString(o), fieldname(fb), 
			       S_DOUBLE(-1)));
		    }
		}
#endif /* TRACING */
		SIZE_AND_STACK(3, 1);
	    } else {
		S_POINTER(-1) = (void *)obj_getslot(o, offset);
#ifdef TRACING
		if (trace) {
		    if (isig == SIGNATURE_FLOAT) {
			TRACE(("\t%s %s.%s <= %g\n", opnames[opcode], 
			       Object2CString(o), fieldname(fb), 
			       S_FLOAT(-1)));
		    } else if (isig == SIGNATURE_CLASS || 
			       isig == SIGNATURE_ARRAY) {
			TRACE(("\t%s %s.%s <= %s\n", opnames[opcode], 
			       Object2CString(o), fieldname(fb), 
			       S_OSTRING(-1)));
		    } else {
			TRACE(("\t%s %s.%s <= %ld\n", opnames[opcode], 
			       Object2CString(o), fieldname(fb), 
			       S_INT(-1)));
		    } 
		}
#endif /* TRACING */
		SIZE_AND_STACK(3, 0);
	    }
	}

	case opc_invokenonvirtual_quick:
	    mb = constant_pool[GET_INDEX(pc + 1)].p;
	    args_size = mb->args_size;
	    optop -= args_size;
	    frame->returnpc = pc + 3;
	    o = S_HANDLE(0);
	    if (o == 0) {
		char buf[64];
		jio_snprintf(buf, sizeof(buf), "trying to call %s%s (nonvirtual method)",
			     mb->fb.name, mb->fb.signature);
		ERROR("NullPointerException", buf);
	    }
	    goto callmethod;

	case opc_invokesuper_quick: { 
	    int index = GET_INDEX(pc + 1);
	    current_class = fieldclass(&frame->current_method->fb);
	    superClass = unhand(cbSuperclass(current_class));
	    mb = cbMethodTable(superClass)->methods[index];
	    args_size = mb->args_size;
	    optop -= args_size;
	    frame->returnpc = pc + 3;
	    o = S_HANDLE(0);
	    if (o == 0) {
		char buf[64];
		jio_snprintf(buf, sizeof(buf), "trying to call %s%s (super)",
			     mb->fb.name, mb->fb.signature);
		ERROR("NullPointerException", buf);
	    }
	    goto callmethod;
	}

	case opc_invokevirtual_quick_w:
	    mb = constant_pool[GET_INDEX(pc + 1)].p;
	    args_size = mb->args_size;
	    optop -= args_size;
	    frame->returnpc = pc + 3;
	    o = S_HANDLE(0);
	    if (o == 0) {
		char buf[64];
		jio_snprintf(buf, sizeof(buf), "trying to call %s%s (virtual method)",
			     mb->fb.name, mb->fb.signature);
		ERROR("NullPointerException", buf);
	    }
	    mb = mt_slot(obj_array_methodtable(o), mb->fb.u.offset);
	    goto callmethod;

	case opc_invokestatic_quick:
	    mb = constant_pool[GET_INDEX(pc + 1)].p;
	    args_size = mb->args_size;
	    optop -= args_size;
	    frame->returnpc = pc + 3;
	    /* o should be the handle to the method's class */
	    o = (JHandle *)(cbHandle(fieldclass(&mb->fb)));
	    goto callmethod;

	case opc_invokeinterface_quick: {
	    cp_index_type NameAndType_index = GET_INDEX(pc + 1);
	    uint32_t ID = constant_pool[NameAndType_index].u;
	    int32_t mslot = pc[4];	/* guess the slot of the method name */
	    int32_t nslots;
	    struct methodblock **mbt;  /* XXX These declarations should be deleted */
	    ClassClass *cb;            /* XXX These declarations should be deleted */
	    
	    args_size = pc[3];
	    optop -= args_size;
	    frame->returnpc = pc + 5;
	    o = S_HANDLE(0);
	    if (o == 0) {
		char buf[64];
		jio_snprintf(buf, sizeof(buf), "trying to call %s%s (interface method)",
			     mb->fb.name, mb->fb.signature);
		ERROR("NullPointerException", buf);
	    }
	    /* Currently, arrays don't implement any interfaces that have
	     * methods in them.  But this is cheap, and we may change our 
	     * minds some time in the future. */
	    if (obj_flags(o) == T_NORMAL_OBJECT) { 
		struct methodtable *mtable = obj_methodtable(o);
		mbt = mtable->methods;
		cb = mtable->classdescriptor;
	    } else { 
		cb = classJavaLangObject;
		mbt = cbMethodTable(cb)->methods;
	    }
	    nslots = cbMethodTableSize(cb);
	    if (mslot >= nslots || (mb = mbt[mslot]) == 0 || mb->fb.ID != ID) {
		/* The guess didn't pay off, search */
		int32_t n = cbMethodTableSize(cb);
		int32_t slot = 0;
		    
		for (; --n > 0;) {
		    mb = mbt[++slot]; /* XXX what about slot 0??? */
		    if (mb->fb.ID == ID) {
			break;
		    }
		}
		if (n <= 0) {
		    char buf[256];
		    int32_t len;
		    classname2string(classname(cb), buf, sizeof(buf));
		    len = strlen(buf);
		    /* If buffer overflow, quietly truncate */
		    (void) jio_snprintf(buf + len, (size_t) (sizeof(buf)-len),
			    ": instance method %s not found",
			    IDToNameAndTypeString(ID));
		    ERROR("IncompatibleClassChangeError", buf);
		}
		/* After searching, set the guess for the next time around */
                pc[4] = (unsigned char) slot;
	    } /* end of if(mslot...) block */
                /* Don't allow calls to static methods.
                   Require that the called method be public.
                   The following lists those restrictions.
                */
            if (((mb->fb.access & ACC_STATIC) != 0) || 
                ( ((mb->fb.access & ACC_PUBLIC) != ACC_PUBLIC) 
                 && (frame->current_method != NULL))) { 
                /* The call is illegal... so throw an exception */
                char buf[256];
                int32_t len;
                char *msg;
                classname2string(classname(cb), buf, sizeof(buf));
                len = strlen(buf);
                if (mb->fb.access & ACC_STATIC) {
                    msg = ": method %s%s used to be dynamic, but is now static";
                } else {
                    msg = ": method %s%s has access restrictions: can't call via interface";
                }
                /* If buffer overflow, quietly truncate */
                (void) jio_snprintf(buf + len, (size_t) (sizeof(buf)-len),
                                    msg,
                                    mb->fb.name, mb->fb.signature);
                if (mb->fb.access & ACC_STATIC) {
                    ERROR("IncompatibleClassChangeError", buf);
                } else { 
                    ERROR("IllegalAccessError", buf);
                }
            }
            if (exceptionOccurred(ee)) {
                frame->lastpc = pc;  
                goto handle_exception;
            }
	    goto callmethod;
	}

	case opc_invokevirtualobject_quick:
	    frame->returnpc = pc + 3;
	    args_size = pc[2];
	    optop -= args_size;
	    o = S_HANDLE(0);
	    if (o == 0) {
		char buf[64];
		struct methodblock* mb = frame->current_method;
		if (mb)
		    jio_snprintf(buf, sizeof(buf), "trying to call %s%s (virtual method)", 
				 mb->fb.name, mb->fb.signature);
		else 
		    jio_snprintf(buf, sizeof(buf), "trying to call native (virtual method)");
		ERROR("NullPointerException", buf);
	    }
	    mb = mt_slot(obj_array_methodtable(o), pc[1]);
	    goto callmethod;

	case opc_invokevirtual_quick:
	    frame->returnpc = pc + 3;
	    args_size = pc[2];
	    optop -= args_size;
	    o = S_HANDLE(0);
	    if (o == 0)  {
		char buf[64];
		struct methodblock* mb = frame->current_method;
		if (mb)
		    jio_snprintf(buf, sizeof(buf), "trying to call %s%s (virtual method)", 
				 mb->fb.name, mb->fb.signature);
		else 
		    jio_snprintf(buf, sizeof(buf), "trying to call native (virtual method)");
		ERROR("NullPointerException", buf);
	    }
	    mb = mt_slot(obj_methodtable(o), pc[1]);
		
	callmethod:
	    sysAssert(o != 0);
	    TRACE(("\t%s %s.%s%s (%d)\n", opnames[opcode], 
		   classname(fieldclass(&mb->fb)), fieldname(&mb->fb), 
		   fieldsig(&mb->fb), args_size));
	    TRACE_METHOD(ee, mb, args_size, TRACE_METHOD_ENTER);

#ifndef DBINFO		/* don't do it twice if DBINFO is defined */
	    frame->lastpc = pc;
#endif

	    DECACHE_OPTOP();
	    if (!mb->invoker(o, mb, args_size, ee)) {
		/* FALSE indicates an exception might have occurred */
		goto check_for_exception;
	    } else {
		frame = ee->current_frame;
		vars = frame->vars;
		pc = frame->returnpc;
		constant_pool = frame->constant_pool;
		CACHE_OPTOP();
		SIZE_AND_STACK(0, 0);
	    }

	case opc_instanceof:
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE
	        ResolveClassConstant(constant_pool, GET_INDEX(pc + 1), ee, 
				     1 << CONSTANT_Class);
	        if (!exceptionOccurred(ee))
		    pc[0] = opc_instanceof_quick;
	    END_WITH_OPCODE_CHANGE
	    goto check_for_exception;

	case opc_instanceof_quick: {
	    DECACHE_OPTOP();
	    cb = constant_pool[GET_INDEX(pc + 1)].p;
	    h = S_HANDLE(-1);
	    S_INT(-1) = ((h != NULL) && is_instance_of(h, cb, ee));
	    TRACE(("\tinstanceof %s => %ld\n", classname(cb), S_INT(-1)));
	    SIZE_AND_STACK_BUT(3, 0); /* may error on implementations */
	    goto check_for_exception;
	}
	
	case opc_checkcast:
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE
	        ResolveClassConstant(constant_pool, GET_INDEX(pc + 1), ee, 
				     1 << CONSTANT_Class);
	        if (!exceptionOccurred(ee))
	            pc[0] = opc_checkcast_quick;
	    END_WITH_OPCODE_CHANGE
	    goto check_for_exception;

	case opc_checkcast_quick: {
	    DECACHE_OPTOP();
	    h = S_HANDLE(-1);
	    cb = constant_pool[GET_INDEX(pc + 1)].p;
	    if (!is_instance_of(h, cb, ee)) {
	        /* Checkcast fails  */
		char buf[255];
		ERROR("ClassCastException", 
		      classname2string(classname(obj_array_classblock(h)), 
				       buf, sizeof(buf)));
	    }
	    TRACE(("\tcheckcast %s\n", classname(cb)));
	    SIZE_AND_STACK(3, 0);
	}

	case opc_new:
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE
	        if (ResolveClassConstant(constant_pool, GET_INDEX(pc + 1), ee, 
 					 1 << CONSTANT_Class)) {
		    current_class = frame->current_method ? 
			        fieldclass(&frame->current_method->fb) : NULL;
		    cb = constant_pool[GET_INDEX(pc + 1)].p;
		    if (cbAccess(cb) & (ACC_INTERFACE | ACC_ABSTRACT))
			ERROR("InstantiationError", cb->name);
		    if (!VerifyClassAccess(current_class, cb, FALSE))
			ERROR("IllegalAccessError", cb->name);
		}
	        if (!exceptionOccurred(ee))
	            pc[0] = opc_new_quick;
	    END_WITH_OPCODE_CHANGE
	    goto check_for_exception;

	case opc_new_quick: {
	    cb = constant_pool[GET_INDEX(pc + 1)].p;
	    S_HANDLE(0) = newobject(cb, pc, ee);
	    TRACE(("\tnew %s => %s\n", classname(cb), S_OSTRING(0)));
	    SIZE_AND_STACK_BUT(3, 1);
	    goto check_for_exception;
	}

	case opc_anewarray:
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE
	        ResolveClassConstant(constant_pool, GET_INDEX(pc + 1), ee, 
				    1 << CONSTANT_Class);
	        if (!exceptionOccurred(ee))
		    pc[0] = opc_anewarray_quick;
	    END_WITH_OPCODE_CHANGE
	    goto check_for_exception;

	case opc_anewarray_quick: {
	    int32_t size = S_INT(-1);

            if (size < 0) 
		ERROR("NegativeArraySizeException", 0);
	    retArray = (HArrayOfObject *)ArrayAlloc(T_CLASS, size);
	    if (retArray == 0) 
	        ERROR("OutOfMemoryError", 0);
	    else {
		array_cb = constant_pool[GET_INDEX(pc + 1)].p;
		unhand(retArray)->body[size] = (HObject*)array_cb;
		TRACE(("\tanewarray_quick %s[%ld] => %s\n", 
		       array_cb->name, size, Object2CString((JHandle*)retArray)));
		S_HANDLE(-1) = (JHandle *)retArray;
		SIZE_AND_STACK(3, 0);
	    }
	}

	case opc_multianewarray:
	    DECACHE_OPTOP();
	    WITH_OPCODE_CHANGE
	       ResolveClassConstant(constant_pool, GET_INDEX(pc + 1), ee, 
				    1 << CONSTANT_Class);
	       if (!exceptionOccurred(ee))
		   pc[0] = opc_multianewarray_quick;
	    END_WITH_OPCODE_CHANGE
	    goto check_for_exception;

	case opc_multianewarray_quick: {
	    int dimensions = pc[3];
	    int i;
	    DECACHE_OPTOP();
	    array_cb = constant_pool[GET_INDEX(pc + 1)].p;

	    ResolveClassConstantFromClass(array_cb, 
					  CONSTANT_POOL_ARRAY_CLASS_INDEX,
					  ee, 1 << CONSTANT_Class);
	    optop -= dimensions;
	    for (i = 0; i < dimensions; i++) 
		if (S_INT(i) < 0)
		    ERROR("NegativeArraySizeException", 0);
	    retO = MultiArrayAlloc(dimensions, array_cb, &S_INFO(0));
	    if (retO == 0)
		ERROR("OutOfMemoryError", 0);
	    S_HANDLE(0) = retO;
	    TRACE(("\tmultianewarray_quick %s, dimensions %d => %s\n", 
		       array_cb->name, dimensions, 
		   Object2CString((JHandle*)retO)));
	    SIZE_AND_STACK(4, 1);
	}

	case opc_newarray: {
	    int32_t size = S_INT(-1);
	    int32_t type = pc[1];
	    /*int32_t array_size = sizearray(pc[1], size);*/
	    if (size < 0) 
	        ERROR("NegativeArraySizeException", 0);
	    retH = ArrayAlloc(type, size);
	    if (retH == 0) 
	        ERROR("OutOfMemoryError", 0);
	    TRACE(("\tnewarray %ld %ld => %s\n", 
		   size, sizearray(type, size), Object2CString(retH)));
	    S_HANDLE(-1) = retH;
	    SIZE_AND_STACK(2, 0);
	}

#define ARRAY_INTRO(type, indexOffset, arrayOffset)             \
	    int32_t index = S_INT(indexOffset);                     \
	    H##type *arrh = (H##type *)(S_HANDLE(arrayOffset)); \
	    type *arr;                                          \
	    if (arrh == 0)                                      \
	        ERROR("NullPointerException", "trying to access array element");               \
	    arr = (type *) unhand(arrh);                                 \
	    if (index < 0 || index >= (int32_t) obj_length(arrh))  {      \
                /* A 32-bit number is at most 10 digits */      \
		char buf[30];                                   \
		/* On (unlikely) overflow, quietly truncate */  \
		(void) jio_snprintf(buf, sizeof(buf), "%ld", index); \
	        ERROR("ArrayIndexOutOfBoundsException", buf);   \
	    }

#define ARRAY_LOAD_INTRO(type)   ARRAY_INTRO(type, -1, -2)

	case opc_iaload: {
	    ARRAY_LOAD_INTRO(ArrayOfInt);
	    S_INT(-2) = arr->body[index];
	    TRACE(("\tiaload ?[%ld] => %ld\n", index, S_INT(-2)));
	    SIZE_AND_STACK(1, -1);
	}
	    
	case opc_laload: {
	    ARRAY_LOAD_INTRO(ArrayOfLong);
	    SET_S_LONG(-2, arr->body[index]);
	    TRACE(("\tlaload ?[%ld] => %s\n", index, S_LONGSTRING(-2)));
	    SIZE_AND_STACK(1, 0);
	}

	case opc_faload: {
	    ARRAY_LOAD_INTRO(ArrayOfFloat);
	    S_FLOAT(-2) = arr->body[index];
	    TRACE(("\tfaload ?[%ld] => %g\n", index, S_FLOAT(-2)));
	    SIZE_AND_STACK(1, -1);
	}

	case opc_daload: {
	    ARRAY_LOAD_INTRO(ArrayOfDouble);
	    SET_S_DOUBLE(-2, arr->body[index]);
	    TRACE(("\tdaload ?[%ld] => %g\n", index, S_DOUBLE(-2)));
	    SIZE_AND_STACK(1, 0);
	}

	case opc_aaload: {
	    ARRAY_LOAD_INTRO(ArrayOfObject);
	    S_HANDLE(-2) = arr->body[index];
	    TRACE(("\taaload ?[%ld] => %s\n", index, S_OSTRING(-2)));
	    SIZE_AND_STACK(1, -1);
	}

	case opc_baload: {
	    ARRAY_LOAD_INTRO(ArrayOfByte);
	    S_INT(-2) = arr->body[index];
	    TRACE(("\tbaload ?[%ld] => %ld\n", index, S_INT(-2)));
	    SIZE_AND_STACK(1, -1);
	}
	    
	case opc_caload: {
	    ARRAY_LOAD_INTRO(ArrayOfChar);
	    S_INT(-2) = arr->body[index];
	    TRACE(("\tcaload ?[%ld] => %ld\n", index, S_INT(-2)));
	    SIZE_AND_STACK(1, -1);
	}

	case opc_saload: {
	    ARRAY_LOAD_INTRO(ArrayOfShort);
	    S_INT(-2) = arr->body[index];
	    TRACE(("\tsaload ?[%ld] => %ld\n", index, S_INT(-2)));
	    SIZE_AND_STACK(1, -1);
	}

	case opc_iastore: {
	    ARRAY_INTRO(ArrayOfInt, -2, -3);
	    arr->body[index] = S_INT(-1);
	    TRACE(("\tiastore ?[%ld] <= %ld\n", index, S_INT(-1)));
	    SIZE_AND_STACK(1, -3);
	    arr = 0;
	}

	case opc_lastore: {
	    ARRAY_INTRO(ArrayOfLong, -3, -4);
	    arr->body[index] = S_LONG(-2);
	    TRACE(("\tlastore ?[%ld] <= %s\n", index, S_LONGSTRING(-2)));
	    SIZE_AND_STACK(1, -4);
	    arr = 0;
	}

	case opc_fastore: {
	    ARRAY_INTRO(ArrayOfFloat, -2, -3);
	    arr->body[index] = S_FLOAT(-1);
	    TRACE(("\tfastore ?[%ld] <= %g\n", index, S_FLOAT(-1)));
	    SIZE_AND_STACK(1, -3);
	    arr = 0;
	}

	case opc_dastore: {
	    ARRAY_INTRO(ArrayOfDouble, -3, -4);
	    arr->body[index] = S_DOUBLE(-2);
	    TRACE(("\tdastore ?[%ld] <= %g\n", index, S_DOUBLE(-2)));
	    SIZE_AND_STACK(1, -4);
	    arr = 0;
	}

	case opc_bastore:{
	    ARRAY_INTRO(ArrayOfByte, -2, -3);
	    arr->body[index] = (char) S_INT(-1);
	    TRACE(("\tbastore ?[%ld] <= %ld\n", index, S_INT(-1)));
	    SIZE_AND_STACK(1, -3);
	    arr = 0;
	}

	case opc_castore:{
	    ARRAY_INTRO(ArrayOfChar, -2, -3);
	    arr->body[index] = (unicode) S_INT(-1);
	    TRACE(("\tcastore ?[%ld] <= %ld\n", index, (unicode) S_INT(-1)));
	    SIZE_AND_STACK(1, -3);
	    arr = 0;
	}

	case opc_sastore: {
	    ARRAY_INTRO(ArrayOfShort, -2, -3);
	    arr->body[index] = (int16_t) S_INT(-1);
	    TRACE(("\tsastore ?[%ld] <= %d\n", index, (short) S_INT(-1)));
	    SIZE_AND_STACK(1, -3);
	    arr = 0;
	}

	case opc_aastore: {
	    JHandle *value = S_HANDLE(-1);
	    ARRAY_INTRO(ArrayOfObject, -2, -3);

	    array_cb = (ClassClass *)(arr->body[obj_length(arrh)]);
	    if (!is_instance_of(value, array_cb, ee)) {
		/* The value isn't null and doesn't match the type. */
		ERROR("ArrayStoreException", 0);
	    }
	    arr->body[index] = value;
	    TRACE(("\taastore ?[%ld] <= %d\n", index, Object2CString(value)));
	    SIZE_AND_STACK(1, -3);
	    value = 0;
	}

	case opc_monitorenter: {
	    o = S_HANDLE(-1);
	    TRACE(("%s(%s)\n", opnames[opcode], Object2CString(o)));
	    if (o == 0) {
	        ERROR("NullPointerException", "trying to enter monitor");
	    } else {
#if defined(XP_MAC) || (defined(XP_PC) && !defined(_WIN32))
		/* Be sure to fix md-include/sysmacros_md.h if you add any new
                   long-lived-locals.
                   The REALLY should be a single macro list:
                   #define CLEAR_ALL_LOCALS ...   */
		fb = 0;
		mb = 0;
		mbt = 0;
		location = 0;
		cb = 0;
		array_cb = 0;
                /* o = 0;  /* XXX: This variable needs to be cleared as well */
		h = 0;
		d = 0;
		current_class = 0;
                superClass = 0;
		retO = 0;
		retH = 0;
		arr = 0;
		signature = 0;
		retArray = 0;
#endif
		monitorEnter(obj_monitor(o));
	    }
	    SIZE_AND_STACK(1, -1);
	}

	case opc_monitorexit: {
	    o = S_HANDLE(-1);
	    TRACE(("%s(%s)\n", opnames[opcode], Object2CString(o)));
	    if (o == 0) {
	        ERROR("NullPointerException", "trying to exit monitor");
	    } else {
		monitorExit(obj_monitor(o));
	    }
	    SIZE_AND_STACK(1, -1);
	}

	case opc_breakpoint:
	    TRACE(("\tbreakpoint\n"));
#ifdef BREAKPTS
	    DECACHE_OPTOP();
	    opcode = get_breakpoint_opcode(pc, ee);
	    goto again;
#else
	    SIZE_AND_STACK(1, 0);	/* treat as opc_nop */
#endif

	case opc_wide: {
	    int reg = GET_INDEX(pc + 2);
	    int offset;

	    switch(pc[1]) { 
	        case opc_aload:
	        case opc_iload:
	        case opc_fload: 
		    S_INFO(0) = vars[reg];
		    TRACEIF(opc_iload, 
			      ("\tiload r%d => %ld\n", reg, S_INT(0)));
		    TRACEIF(opc_aload, 
			    ("\taload r%d => %s\n", reg, S_OSTRING(0)));
		    TRACEIF(opc_fload, 
			    ("\tfload r%d => %g\n", reg, S_FLOAT(0)));
		    SIZE_AND_STACK(4, 1);
	        case opc_lload:
	        case opc_dload: 
		    S_INFO(0) = vars[reg];
		    S_INFO(1) = vars[reg + 1];
		    TRACEIF(opc_lload, 
			    ("\tlload r%d => %s\n", reg, S_LONGSTRING(0)));
		    TRACEIF(opc_dload, 
			    ("\tdload r%d => %g\n", reg, S_DOUBLE(0)));
		    SIZE_AND_STACK(4, 2);
	        case opc_istore:
	        case opc_astore:
	        case opc_fstore:
		    vars[reg] = S_INFO(-1);
		    TRACEIF(opc_istore,
			    ("\tistore %ld => r%d\n", S_INT(-1), reg));
		    TRACEIF(opc_astore,
			    ("\tastore %s => r%d\n", S_OSTRING(-1), reg));
		    TRACEIF(opc_fstore,
			    ("\tfstore %g => r%d\n", S_FLOAT(-1), reg));
		    SIZE_AND_STACK(4, -1);
	        case opc_lstore:
	        case opc_dstore: 
		    vars[reg] = S_INFO(-2);
		    vars[reg + 1] = S_INFO(-1);
		    TRACEIF(opc_lstore, 
			    ("\tlstore %s => r%d\n", S_LONGSTRING(-2), reg));
		    TRACEIF(opc_dstore, 
			    ("\tdstore %g => r%d\n", S_DOUBLE(-2), reg));
		    SIZE_AND_STACK(4, -2);
	        case opc_iinc:
		    offset = SSCAST(SCAT(pc,4)) << 8 | UCAT(pc,5); /* (((signed char *)pc)[4] << 8) + pc[5]; */
		    vars[reg].i += offset;
		    TRACE(("\tiinc r%d+%d => %ld\n", reg, offset, vars[reg].i));
		    SIZE_AND_STACK(6, 0);
	        case opc_ret: 
		    TRACE(("\tret %d (%8X)\n", reg, vars[reg].addr));
		    pc = vars[reg].addr;
		    SIZE_AND_STACK_BUT(0, 0);
		    goto check_for_exception;
	        default:
		    ERROR("InternalError", "undefined opcode");
	    }
	}

        case opc_invokeignored_quick: /* 1.1 optimization.  Not implemented */
	default:
	    ERROR("InternalError", "undefined opcode");
	    
	} /* end of switch */

      check_for_exception:
	sysCheckException(ee);
	/* Fall through */

      handle_exception: {
	  /* We've got an exception. */
	  JHandle *object;
	  unsigned char *new_pc;
	  DECACHE_OPTOP();
	  
	  object = ee->exception.exc;
          new_pc = ProcedureFindThrowTag(ee, frame, object, pc);
	  if (new_pc != 0) {
	      /* Found the exception handler. */
	      exceptionClear(ee);
	      vars = frame->vars;
	      constant_pool = frame->constant_pool;
	      frame->optop = frame->ostack; /* Reset to empty */
	      CACHE_OPTOP();
	      /* Push exception object onto the stack. */
	      S_HANDLE(0) = object;
	      optop++;
	      pc = new_pc;
#ifdef TRACING
	      if (trace) {
		  char where_am_i[100];
		  pc2string(pc, frame->current_method, 
			    where_am_i, where_am_i + sizeof where_am_i);
		  fprintf(stdout, "Catch at %s\n", where_am_i);
		  fflush(stdout);
	      }
#endif /* TRACING */
	      continue;		/* continue with the outer loop */
	  }
      }
	/* Fall through */
      finish_return:
	/* Exit from a monitor, if necessary. */
	if (frame->monitor)
	    monitorExit(obj_monitor(frame->monitor));
	if (java_monitor) {
	    /* Indicate how much time we've used. */
	    mb = frame->current_method;
	    if (mb != 0) {
	    	int32_t time = now() - frame->mon_starttime;
		java_mon(frame->prev->current_method, mb, time);
            }
        }
#ifdef TRACING
	if (trace) {
	    char where_am_i[100];
	    pc2string(pc, frame->current_method, 
		      where_am_i, where_am_i + sizeof where_am_i);
	    fprintf(stdout, "Leaving %s\n", where_am_i);
	    fflush(stdout);
	}
#endif /* TRACING */
	if (frame == initial_frame) {
#ifdef DEBUG
            ee->debugFlags &= ~EE_DEBUG_OPTOP_CACHED;
#endif
	    return !exceptionOccurred(ee);
	} else {
	    TRACE_METHOD(ee, frame->current_method, args_size, TRACE_METHOD_RETURN);
	    ee->current_frame = frame = frame->prev;
	    CACHE_OPTOP();
	    if (exceptionOccurred(ee)) {
		/* Go back, and look for an exception handler. */
		pc = frame->lastpc;
	        goto handle_exception;
	    }
	    pc = frame->returnpc;
	    vars = frame->vars;
	    constant_pool = frame->constant_pool;
/*	    CACHE_OPTOP(); */
	    continue;
	}
#if GENERATING68K
	}
#endif

    } /* end of infinite loop */
}
/*  __ExecuteJavaEnd__ */
/* Search backwards for __ExecuteJavaStart__ to find the end of the function */

/*
 * Note: Call InitializeInterpreter() BEFORE InitializeClassThread() to
 * ensure that the Java stack lock is initialized before the idle thread
 * has a chance to run!
 */
void
InitializeInterpreter()
{
    _code_lock=(sys_mon_t *) sysMalloc(sysMonitorSizeof());
    memset((char*) _code_lock, 0, sysMonitorSizeof());
    CODE_LOCK_INIT();
    _ostack_lock=(sys_mon_t *) sysMalloc(sysMonitorSizeof());
    memset((char*) _ostack_lock, 0, sysMonitorSizeof());
    OSTACK_LOCK_INIT();

#ifdef WIN32
	/* if you dont get a JIT we really should set it back */
	UseLosslessQuickOpcodes = TRUE;
#endif

#ifdef STATISTICS
    memset(instruction_count, 0, sizeof(instruction_count));
#endif
}

#ifdef DEBUG
void PrintObject(JHandle *oh) {
    if (oh == 0)
        printf("NULL\n");
    else {
	ClassObject *o = unhand(oh);
	if (-1000 < (long) o && (long) o < 1000)
	    printf("BOGUS-PTR[%ld]\n", (int32_t)o);
	else if (obj_flags(oh) != T_NORMAL_OBJECT) 
	    printf("%s\n", Object2CString(oh));
	else {
	    ExecEnv *current_ee = threadSelf() 
	                            ? (ExecEnv *)(unhand(threadSelf())->eetop)
				    : 0;
	    ExecEnv lee;
	    ClassClass *system_class;
	    ExecEnv *ee = current_ee ? current_ee 
	                         : (InitializeExecEnv(&lee, 0), &lee);
	    if (ee->initial_stack == 0) {
		fprintf(stderr, "Out of memory\n");
	    }
	    system_class = FindClassFromClass(&lee, "java/lang/System", TRUE, NULL);
	    if (system_class == 0) {
		fprintf(stderr, "Unable to find System class\n");
	    } else {
		JHandle *out = *(JHandle **)getclassvariable(system_class, "out");
#ifdef TRACING
		int oldtrace = trace;   /* we don't want to see this */
		trace = 0;
#endif /* TRACING */
		execute_java_dynamic_method(ee, out, 
					   "println", "(Ljava/lang/Object;)V",
					   oh);
#ifdef TRACING
		trace = oldtrace;
#endif /* TRACING */
	    }
	    if (current_ee == 0)
	        DeleteExecEnv(&lee, 0);
	}
    }
}    
#endif


#ifdef STATISTICS
void
DumpStatistics ()
{

    int i;
    for (i = 0; i < 256; i++) {
	if (instruction_count[i]) {
	    printf("%30.-30s %6d\n", opnames[i], instruction_count[i]);
	}
    }
}
#endif

#ifdef XP_MAC
#ifndef DEBUG
#pragma global_optimizer reset 
#endif
#endif

#ifdef TRACING

#if 0 /* Java 1.0.2 code that has moved to md/trace_md.c  This
       Code should be merged in one direction or the other. */

void
trace_method(struct execenv* ee, struct methodblock* mb,
	     int args_size, int type)
{
    HArrayOfChar* threadName = threadSelf() ? getThreadName() : NULL;
    JavaFrame *frame;
    int depth;
    int i;
    for (frame = ee->current_frame, depth = 0;
	   frame != NULL;
	   frame = frame->prev)
	if (frame->current_method != 0) depth++;
    if (type == TRACE_METHOD_RETURN)
	depth--;
    printf("# ");
    if (threadName) {
	int threadNameLen = obj_length(threadName);
	unicode *threadNameStr = unhand(threadName)->body;
	for (i = 0; i < threadNameLen; i++)
	    putchar((char)(threadNameStr[i]));
    }
    printf(" [%2d] ", depth);
    for (i = 0; i < depth; i++)
	printf("| ");
    printf("%c %s.%s%s ",
	   ((type == TRACE_METHOD_ENTER) ? '>' : '<'),
	   classname(fieldclass(&mb->fb)),
	   fieldname(&mb->fb),
	   fieldsig(&mb->fb));
    if (type == TRACE_METHOD_ENTER)
	printf("(%d) entered\n", args_size);
    else if (exceptionOccurred(ee))
	printf("throwing %s\n",
		ee->exception.exc->methods->classdescriptor->name);
    else
	printf("returning\n");
    fflush(stdout);
}

#endif /* 0 Java 1.0.2 code */

#endif
