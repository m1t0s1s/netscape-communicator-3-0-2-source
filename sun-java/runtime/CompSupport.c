/*
 * @(#)CompSupport.c	1.20 95/08/30  
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
 *	Support for running compiled classes
 */

#include <stdarg.h>
#include "oobj.h"
#include "interpreter.h"
#include "javaThreads.h"
#include "typecodes.h"
#include "tree.h"

#include "java_lang_Thread.h"

extern void    InvokeCompiledMethodVoid(struct methodblock *, void*, stack_item*, int);
extern int     InvokeCompiledMethodInt(struct methodblock *, void*, stack_item*, int);
extern int64_t InvokeCompiledMethodLong(struct methodblock *, void*, stack_item*, int);
extern float   InvokeCompiledMethodFloat(struct methodblock *, void*, stack_item*, int);
extern double  InvokeCompiledMethodDouble(struct methodblock *, void*, stack_item*, int);

extern void    ErrorUnwind();

HObject *
CompSupport_new(ClassClass * cl)
{
    return newobject(cl, 0, 0);
}

HObject *
CompSupport_newarray(long T, long size) 
{
    HObject *ret;
    if (size < 0) {
	SignalError(0, JAVAPKG "NegativeArraySizeException", 0);
	ErrorUnwind();
	return 0;
    }
    ret = ArrayAlloc(T, size);
    if (ret == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	ErrorUnwind();
    }
    return ret;
}

HArrayOfObject *
CompSupport_anewarray(ClassClass *cb, long size) 
{
    HArrayOfObject *ret;
    if (size < 0) {
	SignalError(0, JAVAPKG "NegativeArraySizeException", 0);
	ErrorUnwind();
	return 0;
    }
    ret = (HArrayOfObject *)ArrayAlloc(T_CLASS, size);
    if (ret == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	ErrorUnwind();
    } else { 
	unhand(ret)->body[size] = (HObject *)cb;
    }
    return ret;
}

HArrayOfObject *
CompSupport_multianewarray(ClassClass *cb, int dimensions, ...) 
{
    HArrayOfObject *result;
    stack_item sizes[256];	/* maximum possible size */
    int i;
    va_list args;
    va_start(args, dimensions);
        for (i = 0; i < dimensions; i++) {
	    if ((sizes[i].i = va_arg(args, long)) < 0) {
		SignalError(0, JAVAPKG "NegativeArraySizeException", 0);
		ErrorUnwind();
	    }
	}
    va_end(args);
    result = (HArrayOfObject *)MultiArrayAlloc(dimensions, cb, sizes);
    if (result == 0) 
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
    if (exceptionOccurred(EE()))
	ErrorUnwind();
    return result;
}


HObject *
CompSupport_checkcast(ClassClass * cb, HObject *h)
{
    if (!is_instance_of(h, cb, EE())) {
	char buf[128];
	classname2string(classname(obj_array_classblock(h)), 
			 buf, sizeof(buf));
	SignalError(0, JAVAPKG "ClassCastException", buf);
	ErrorUnwind();
    }
    return h;
}

int
CompSupport_instanceof(ClassClass *cb, HObject *h)
{
    int result = is_instance_of(h, cb, EE());
    if (exceptionOccurred(EE()))
	ErrorUnwind();
    return result;
}


void
CompSupport_athrow(HObject * obj)
{
    exceptionThrow(EE(), obj);
    ErrorUnwind();
}


void
CompSupport_monitorenter(HObject * obj)
{
    monitorEnter(obj_monitor(obj));
    if (exceptionOccurred(EE()))
	ErrorUnwind();
}

void
CompSupport_monitorexit(HObject * obj)
{
    monitorExit(obj_monitor(obj));
    if (exceptionOccurred(EE()))
	ErrorUnwind();
}



/*
 * temporary hack: doesn't work for float parameters or double/long return
 * values
 */

bool_t
invokeCompiledMethod(JHandle * o, struct methodblock * mb, 
		     int args_size, struct execenv *ee) {
    /*  these are implemented in assembler */

    JavaFrame *frame = ee->current_frame;
    stack_item *optop = frame->optop;
    void *code = mb->CompiledCode;
#if defined(HAVE_ALIGNED_DOUBLES)
    Java8 t;
#endif

    switch((mb->CompiledCodeFlags >> 24) & 0xFF) {
        default:
	    optop->i = InvokeCompiledMethodInt(mb, code, optop, args_size);
	    ee->current_frame->optop = optop + 1;
	    break;
        case SIGNATURE_LONG:
	    SET_INT64(t, optop, 
		      InvokeCompiledMethodLong(mb, code, optop, args_size));
	    ee->current_frame->optop = optop + 2;
	    break;
	case SIGNATURE_FLOAT:
	    optop->f = InvokeCompiledMethodFloat(mb, code, optop, args_size);
	    ee->current_frame->optop = optop + 1;
	    break;
	case SIGNATURE_DOUBLE:
	    SET_DOUBLE(t, optop, 
		       InvokeCompiledMethodDouble(mb, code, optop, args_size));
	    ee->current_frame->optop = optop + 2;
	    break;
	case SIGNATURE_VOID:
	    InvokeCompiledMethodVoid(mb, code, optop, args_size);
	    break;
    }
    return !exceptionOccurred(ee);
}

/* Given a handle and a method ID, find the method for that object that 
 * corresponds to the given ID.
 */
 
void *
MethodID2CompiledMethod(JHandle *o, long ID, struct methodblock **mb_ptr) {
    ClassClass *cb = o->methods->classdescriptor;
    uint16_t nslots = cbMethodTableSize(cb);
    struct methodblock **mbt = o->methods->methods;
    while (--nslots >= 0) {
	struct methodblock *mb = mbt[nslots];
	if (mb->fb.ID == (uint32_t) ID) {
	    *mb_ptr = mb;
	    return mb->CompiledCode;
	}
    }
    return 0;
}


/* Called when we're unwinding a frame on the stack. */
void *
CompiledCodeCleanup(struct methodblock *mb, HObject *o) {
    if (mb->fb.access & ACC_SYNCHRONIZED) {
	if (mb->fb.access & ACC_STATIC)
	    monitorExit(obj_monitor(mb));
	else 
	    monitorExit(obj_monitor(o));
    }
    /* Eventually, this will return the pc of the error handler, if the
     * appropropriate catch frame is found. 
     */
    return 0;
}
