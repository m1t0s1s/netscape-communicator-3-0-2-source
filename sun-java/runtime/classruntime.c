/*
 * @(#)classruntime.c	1.59 96/04/17  
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

/*-
 *      Runtime routines to support classes.
 */

#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>

#include "oobj.h"
#include "tree.h"
#include "interpreter.h"
#include "bool.h"
#include "timeval.h"
#include "javaThreads.h"
#include "monitor.h"
#include "exceptions.h"
#include "javaString.h"
#include "path.h"

#include "sys_api.h"

#include "java_lang_Throwable.h"
#include "java_lang_Thread.h"

#include "jri.h"

#if defined(XP_PC) && !defined(_WIN32)
#if !defined(stderr)
extern FILE *stderr;
#endif
#if !defined(stdout)
extern FILE *stdout;
#endif
#endif

#ifdef DEBUG
void
VerifyClasses()
{
    static int32_t lastn;
    int32_t         i;
    sysAssert(nbinclasses >= lastn);
    lastn = nbinclasses;
    sysAssert(nbinclasses <= sizebinclasses);
    sysAssert(binclasses != 0 || nbinclasses == 0);
    for (i = 0; i < nbinclasses; i++) {
	ClassClass *clb = binclasses[i];
        if (!clb) continue;
	sysAssert(((int32_t) clb & 3) == 0);
	sysAssert(clb->fields_count < 1000);
	sysAssert(clb->methods_count < 1000);
	sysAssert(cbInstanceSize(clb) && cbInstanceSize(clb) < 4000);
	sysAssert(classname(clb)[0]);
    }
}
#endif

/* These are all flavors of method invocation depending on the type
   of method (defined by its access flags) being invoked.  There
   are 4 possibilities: java method, synchronized java method,
   native (C) method, synchronized native method.  Each methodblock
   has a pointer to the appropriate one for its access, and each
   one of these procedures optimizes its particular case.

   They don't all use all they parameters, which is unfortunate. */


bool_t
invokeJavaMethod(JHandle *o, struct methodblock *mb, int args_size, ExecEnv *ee)
{
    int         nlocals = mb->nlocals; /* maximum number of locals */
    JavaFrame   *old_frame = ee->current_frame; /* previous frame */
    stack_item *optop = old_frame->optop; /* Points to just before args */
    JavaStack   *javastack = old_frame->javastack; /* Current stack pointer */
    JavaFrame   *frame = (JavaFrame *)(optop + nlocals); /* tentative  */

#ifdef TRACING
    if (trace) {
	char where_am_i[100];
	pc2string(mb->code, mb, where_am_i, where_am_i + sizeof where_am_i);
	fprintf(stdout, "Entering %s\n", where_am_i);
	fflush(stdout);
    }
#endif /* TRACING */
    if (frame->ostack + mb->maxstack > javastack->end_data) {
	stack_item *copied_optop;
	if (javastack->stack_so_far >= JavaStackSize) {
	    SignalError(ee, JAVAPKG "StackOverflowError", 0);
	    return FALSE;
	}
	javastack = javastack->next 
	        ? javastack->next : CreateNewJavaStack(ee, javastack);
	if (javastack == 0) {
	    SignalError(ee, JAVAPKG "OutOfMemoryError", 0);
	    return FALSE;
	}
	frame = (JavaFrame *)(javastack->data + nlocals);
	for (copied_optop = javastack->data; --args_size >= 0 ; ) 
	    copied_optop[args_size] = optop[args_size];
	optop = copied_optop;
    }
    frame->vars = optop;
    frame->javastack = javastack;
    frame->prev = old_frame;
    frame->optop = frame->ostack;
    frame->current_method = mb; 
    frame->constant_pool = cbConstantPool(fieldclass(&mb->fb));
    frame->returnpc = mb->code;
    frame->monitor = 0;
    frame->annotation = 0;
    ee->current_frame = frame;
    if (java_monitor)
	  frame->mon_starttime = now();
    return TRUE;
}


bool_t
invokeSynchronizedJavaMethod(JHandle *o, struct methodblock *mb, int args_size,
			    ExecEnv *ee)
{
    int         nlocals = mb->nlocals; /* maximum number of locals */
    JavaFrame   *old_frame = ee->current_frame; /* previous frame */
    stack_item *optop = old_frame->optop; /* Points to just before args */
    JavaStack   *javastack = old_frame->javastack; /* Current stack pointer */
    JavaFrame   *frame = (JavaFrame *)(optop + nlocals); /* tentative  */

#ifdef TRACING
    if (trace) {
	char where_am_i[100];
	pc2string(mb->code, mb, where_am_i, where_am_i + sizeof where_am_i);
	fprintf(stdout, "Entering %s\n", where_am_i);
	fflush(stdout);
    }
#endif /* TRACING */
    if (frame->ostack + mb->maxstack > javastack->end_data) {
	stack_item *copied_optop;
	if (javastack->stack_so_far >= JavaStackSize) {
	    SignalError(ee, JAVAPKG "StackOverflowError", 0);
	    return FALSE;
	}
	javastack = javastack->next 
	             ? javastack->next : CreateNewJavaStack(ee, javastack);
	if (javastack == 0) {
	    SignalError(ee, JAVAPKG "OutOfMemoryError", 0);
	    return FALSE;
	}
	frame = (JavaFrame *)(javastack->data + nlocals);
	for (copied_optop = javastack->data; --args_size >= 0 ; ) 
	    copied_optop[args_size] = optop[args_size];
	optop = copied_optop;
    }
    frame->javastack = javastack;
    frame->prev = old_frame;
    frame->vars = optop;
    frame->optop = frame->ostack;
    frame->current_method = mb; 
    frame->constant_pool = cbConstantPool(fieldclass(&mb->fb));
    frame->returnpc = mb->code;
    frame->monitor = o;
    frame->annotation = 0;
    ee->current_frame = frame;
    monitorEnter(obj_monitor(o));
    if (java_monitor)
	  frame->mon_starttime = now();
    return TRUE;
}

bool_t
invokeNativeMethod(JHandle *o, struct methodblock *mb, int args_size, 
		   ExecEnv *ee)
{
    JavaFrame *old_frame = ee->current_frame;
    stack_item *optop = old_frame->optop;
    if (java_monitor) {
	long start_time, end_time;
	start_time = now();
	optop = (*(stack_item * (*) (stack_item*, ExecEnv*))
		 mb->code) (optop, ee);
	end_time = now();
	java_mon(old_frame->current_method, mb, end_time - start_time);
	/* java_mon(fieldclass(&mb->fb), mb, ee, 0, end_time - start_time); */
    } else {
	optop = (*(stack_item * (*) (stack_item*, ExecEnv*))
		 mb->code) (optop, ee);
    }
    old_frame->optop = optop;
    TRACE_METHOD(ee, mb, args_size, TRACE_METHOD_NATIVE_RETURN);
    return !(exceptionOccurred(ee));
}

bool_t
invokeSynchronizedNativeMethod(JHandle *o, struct methodblock *mb, int args_size,
			       ExecEnv *ee)
{
    JavaFrame *old_frame = ee->current_frame;
    stack_item *optop = old_frame->optop;
    monitorEnter(obj_monitor(o));
    if (java_monitor) {
	long start_time, end_time;
	start_time = now();
	optop = (*(stack_item * (*) (stack_item*, ExecEnv*))
		 mb->code) (optop, ee);
	end_time = now();
	java_mon(old_frame->current_method, mb, end_time - start_time);
	/* java_mon(fieldclass(&mb->fb), mb, ee, 0, end_time - start_time); */
    } else {
	optop = (*(stack_item * (*) (stack_item*, ExecEnv*))
		 mb->code) (optop, ee);
    }
    old_frame->optop = optop;
    monitorExit(obj_monitor(o));
    TRACE_METHOD(ee, mb, args_size, TRACE_METHOD_NATIVE_RETURN);
    return !(exceptionOccurred(ee));
}

bool_t
invokeLazyNativeMethod(JHandle *o, struct methodblock *mb, int args_size, 
		       ExecEnv *ee)
{     
    if ((mb->code != 0) || dynoLink(mb)) {
	mb->invoker = (mb->fb.access & ACC_SYNCHRONIZED) 
	             ?  invokeSynchronizedNativeMethod : invokeNativeMethod;

	return (mb->invoker)(o, mb, args_size, ee);
    } else {
	char buf[1024];
	jio_snprintf(buf, sizeof(buf), "native method %s.%s not found: %s", 
		     classname((&mb->fb)->clazz), fieldname(&mb->fb),
		     PR_GetErrorString());
	SignalError(ee, JAVAPKG "UnsatisfiedLinkError", buf);
	return FALSE;
    }
}

bool_t
invokeAbstractMethod(JHandle *o, struct methodblock *mb, int args_size, 
		     ExecEnv *ee)
{     
    SignalError(ee, JAVAPKG "AbstractMethodError", 
		fieldname(&mb->fb));
    return FALSE;
}

bool_t
dynoLink(struct methodblock *mb)
{
    char sym[300];
    void* lib;
    ClassClass *cb = fieldclass(&mb->fb);
    if (cbLoader(cb)) {
	return 0;
    }
    mangleMethodName(mb, sym, sizeof(sym), "_stub");
    mb->code = (unsigned char *)sysDynamicLink(sym, &lib);
    mb->lib = lib;
    return (mb->code != 0);
}

/*  I put this into a separate function so that other parts of the
    interpreter can use it without knowing how an object is actually
    created. - csw
*/
HObject *
newobject(ClassClass *cb, unsigned char *pc, struct execenv *ee)
{
    HObject *ret;
    if ((ret = ObjAlloc(cb, 0)) == 0) {
#if 0
	char buf[128];
#endif
	if (ee != NULL) 
	    ee->current_frame->lastpc = pc;
#if 0
	/*
	** removed -- because we recurse trying to call newobject to
	** allocate the exception:
	*/
	SignalError(ee, JAVAPKG "OutOfMemoryError", classname2string(classname(cb), buf, sizeof(buf)));
#endif
	exceptionThrow((ee ? ee : EE()), exceptionInternalObject(IEXC_OutOfMemory));
	return 0;
    }
    return ret;
}

char*
RunStaticInitializers(ClassClass *cb)
{
    struct methodblock *mb;
    int size;
    char *s;
    struct methodblock *StaticInitializer = 0;
    char* ret = NULL;

    mb = cbMethods(cb);
    for (size = 0; size < (int) cb->methods_count; size++, mb++) {
	sysAssert(mb->fb.u.offset < cbMethodTableSize(cb));
	s = fieldname(&mb->fb);
	if (s[0] == '<' && strcmp(s, "<clinit>") == 0 
	        && strcmp(fieldsig(&mb->fb), "()V") == 0)
	    StaticInitializer = mb;
    }
    if (StaticInitializer && !InhibitExecute) {
	struct execenv *ee = EE();
#ifndef TRIMMED
	if (verbose)
	    fprintf(stderr, "[Running static initializer for %s]\n", 
		    classname(cb));
#endif
	do_execute_java_method(ee, cb, 0, 0, StaticInitializer, TRUE);
	if (ee && exceptionOccurred(ee)) {
	    /*
	    ** XXX This sucks how Sun has done this. There's about 10
	    ** levels of routines that just return exception name strings
	    ** rather than call SignalError. Now way down here where we
	    ** actually call java and could get back a real exception
	    ** object, we have to yank out its name and return it, only
	    ** to have it constructed again later. They used to just drop
	    ** this error on the floor, so this hack is better than
	    ** nothing.
	    */
	    JHandle* exc = ee->exception.exc;
	    ClassClass* cl = obj_classblock(exc);
	    ret = classname(cl); 
	}
    }
    return ret;
}

void
InitializeInvoker(ClassClass *cb)
{
    struct methodblock *mb;
    int size;

    mb = cbMethods(cb);
    for (size = 0; size < (int) cb->methods_count; size++, mb++) {
	unsigned int access = mb->fb.access;
	sysAssert(mb->fb.u.offset < cbMethodTableSize(cb));
	if (access & ACC_MACHINE_COMPILED) { 
	    mb->invoker = invokeCompiledMethod;
	} else if ((access & (ACC_ABSTRACT | ACC_NATIVE)) == 0) {
	    /* Normal java method */
	    mb->invoker = (access & ACC_SYNCHRONIZED) 
		? invokeSynchronizedJavaMethod
		: invokeJavaMethod;
	} else if (access & ACC_ABSTRACT) {
	    cbAccess(cb) |= ACC_ABSTRACT;
	    mb->invoker = invokeAbstractMethod;
	} else { 
	    mb->invoker = invokeLazyNativeMethod;
	}
    }
}

JRI_PUBLIC_API(void)
SignalError(struct execenv * ee, char *ename, char *DetailMessage)
{
    ClassClass          *cb;

    if (ee == 0)
        ee = EE();

#ifdef DEBUG
    if (exceptionOccurred(ee)) {
	/* Masking a pending error */
	ee = ee;
    }
#endif

    /*
     * Find the exception class (or the default).  If we can't find it
     * throw a preallocated object
     */
    if ((cb = FindClassFromClass(ee, ename, TRUE, NULL)) == 0 &&
	(cb = FindClassFromClass(ee, JAVAPKG "UnknownError", TRUE, NULL)) == 0) {
#ifndef TRIMMED
	char where[100];
	if (ee->current_frame) {
	    pc2string(ee->current_frame->lastpc, 
		      ee->current_frame->current_method, 
		      where, where + sizeof where);
	    fprintf(stderr, "Class missing for error: %s at %s\n", ename, where);
	} else {
	    fprintf(stderr, "Class missing for error: %s\n", ename);
	}
#endif
	exceptionThrow(ee, exceptionInternalObject(IEXC_NoClassDefinitionFound));
	return;
    }

    SignalError1(ee, cb, DetailMessage);
}

void
SignalError1(struct execenv * ee, ClassClass* cb, char *DetailMessage)
{
    Hjava_lang_Throwable *ret;
    Classjava_lang_Throwable *new_obj;
    JavaFrame            *frame;

    sysAssert(ee);

    /*
     * Instantiate the object.  If we can't then throw a preallocated
     * object.
     */
    if ((ret = (Hjava_lang_Throwable *)newobject(cb, 0, ee)) == NULL) { 
	exceptionThrow(ee, exceptionInternalObject(IEXC_OutOfMemory));
	return;
    }
    new_obj = unhand(ret);

    /* Backtrace through the interpreter frames. */
    frame = ee ? ee->current_frame : 0;
#if 0	/* XXX What is this? */
    if (frame == 0) {
#ifndef TRIMMED
	fprintf(stderr, "Exception: %s %s (can't backtrace because of a missing "
		"context)\n", cb->name, (DetailMessage ? DetailMessage : ""));
#endif
	exceptionThrow(ee, (HObject *)ret);
	return;	
    }
#endif
#ifdef TRACING
    if (trace) {
	printf("%6X %8X\tERROR %s\n", threadSelf(), 
	       (frame ? frame->lastpc : (unsigned char*)-1), cb->name);
	fflush(stdout);
    }
#endif /* TRACING */

    if (DetailMessage)
	new_obj->detailMessage = makeJavaString(DetailMessage,
						strlen(DetailMessage));
#ifdef DEBUG
    if (threadSelf() == NULL)
	/* can only happen on startup */
	fprintf(stderr, "Exception: %s %s (can't backtrace because of a missing "
		"context)\n", cb->name, (DetailMessage ? DetailMessage : ""));
    else
#endif
	fillInStackTrace(ret, ee);

    /* This must be the last thing, since makeJavaString() may clear the
     * signal Error slot. 
     */
    exceptionThrow(ee, (HObject *)ret);
}


char       *
addhex(long n, char *buf, char *limit)
{
    int         i;
    for (i = 32; (i -= 4) >= 0 && buf < limit;)
	*buf++ = "0123456789ABCDEF"[(n >> i) & 0xF];
    return buf;
}

char       *
adddec(long n, char *buf, char *limit)
{
    if (n < 0) {
	n = -n;
	if (buf < limit)
	    *buf++ = '-';
    }
    if (n >= 10)
	buf = adddec(n / 10, buf, limit);
    if (buf < limit)
	*buf++ = (n % 10) + '0';
    return buf;
}


int32_t
pc2lineno(struct methodblock *mb, unsigned int pc_offset)
{
    uint32_t length = mb->line_number_table_length;

    if (length > 0) {
	struct lineno *ln = &mb->line_number_table[length];
	int32_t i;
	for (i = mb->line_number_table_length; --i >= 0; )
	    if (pc_offset >= (--ln)->pc)
	      return (int32_t) ln->line_number;
    }
    return -1;
}

static struct methodblock *pc2method(unsigned char *pc) {
    struct methodblock *mb;
    ClassClass *cb;
    int32_t i, j;

    for (i = nbinclasses; --i >= 0; ) {
	cb = binclasses[i];
        if (!cb) continue;
	for (mb = cbMethods(cb), j = cb->methods_count; --j >= 0; mb++) {
	    if (!(mb->fb.access & ACC_NATIVE)) {
                /*
                ** In WIN16 pointer arithmetic deals with only the low 16-bits of a pointer.
                ** So, case everything to long...
                */
		long code_start;
		if (mb->fb.access & ACC_MACHINE_COMPILED) {
		    if (PCinCompiledCode(pc, mb)) 
			return mb;
		} else { 
		    code_start = (long)mb->code;
		if ((long)pc >= code_start && (long)pc < code_start + mb->code_length) 
		    return mb;
	    }
	}
    }
    }
    return 0;
}

char *
pc2string(unsigned char *pc, struct methodblock *mb, char *buf, char *limit)
{
    if (buf >= limit) {
	return buf;
    }
    --limit;	/* Save room for terminating '\0' */
    if (mb == 0) {
        mb = pc2method(pc);
    }
    if (mb != 0) {
	char cbuf[128];
	ClassClass *cb = fieldclass(&mb->fb);
	buf = addstr(classname2string(classname(cb),
		     cbuf, sizeof(cbuf)), buf, limit, 0);
	buf = addstr(".", buf, limit, 0);
	buf = addstr(fieldname(&mb->fb), buf, limit, '(');
	if (cb != 0) {
	    if ((mb->fb.access & ACC_MACHINE_COMPILED) == 0) {
	    char        *fn;
	    int32_t      lineno;
	    if (cb->source_name)
	        fn = strrchr(classsrcname(cb), '/');
	    else
	        fn = 0;
	    buf = addstr("(", buf, limit, 0);
	    buf = addstr(fn ? fn + 1 : classsrcname(cb), buf, limit, 0);
	    lineno = pc2lineno(mb, pc - mb->code);
	    if (lineno >= 0) {
		buf = addstr(":", buf, limit, 0);
		buf = adddec(lineno, buf, limit);
	    }
	    buf = addstr(")", buf, limit, 0);
	    } else {
		buf = addstr("(Compiled Code)", buf, limit, 0);
	    }
	}
	*buf = 0;
    } else {
	buf[0] = '\0';
    }
    return buf;
}

/*
   Make an Java unicode string out of a series of bytes that are
   already encoded as unicode.
*/
HArrayOfChar *
MakeUniString(unicode  *str, long len)
{
    ArrayOfChar *new_str;
    JHandle *ret;
    unicode *tmp;

    sysAssert (str != 0);
    for (tmp = str; *tmp; tmp++)
	;
    if (len > tmp-str)
	len = (int) (tmp-str);
    ret = ArrayAlloc(T_CHAR, len);
    if (ret == 0)
	return 0;
    new_str = (ArrayOfChar *) unhand(ret);
    memmove((char *) new_str->body, (char *) str, (size_t)(len * sizeof(unicode)));
    return (HArrayOfChar *)ret;
}


/*
    Make an Java unicode string from a non-unicode C-style string.
*/
HArrayOfChar *
MakeString(char *str, long len)
{
    ArrayOfChar *new_str;
    HArrayOfChar  *ret;

    sysAssert(str != 0);
    ret = (HArrayOfChar *) ArrayAlloc(T_CHAR, len);
    if (ret == 0)
	return 0;
    new_str = unhand(ret);
    str2unicode(str, new_str->body, len);
    return ret;
}

/*
    Make an Java array of bytes
*/
HArrayOfByte *
MakeByteString(char *str, long len)
{
    ArrayOfByte *new_str;
    JHandle *ret;

    ret = ArrayAlloc(T_BYTE, len);
    if (ret == 0)
	return 0;
    new_str = (ArrayOfByte *) unhand(ret);
    if (str) {
	memmove((char *) new_str->body, (char *)str, (size_t) len);
    }
    return (HArrayOfByte *)ret;
}


JRI_PUBLIC_API(ExecEnv *)
EE()
{
    ExecEnv *ee;
    TID t = threadSelf();

    ee = (t) ? (ExecEnv *)THREAD(threadSelf())->eetop : DefaultExecEnv;

    sysAssert(ee != 0);
    return ee;
}
