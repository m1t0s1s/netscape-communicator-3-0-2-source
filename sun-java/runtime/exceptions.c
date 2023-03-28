/*
 * @(#)exceptions.c	1.26 95/11/16 Peter King, Bruce Martin
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
 * The Java runtime exception handling mechanism.
 */


/*
 * Header files.
 */
#ifdef __cplusplus
#error
#endif

#include "exceptions.h"
#include "javaThreads.h"

#ifdef XP_MAC
#include "j_lang_NoClassDefFoundError.h"
#else
#include "java_lang_NoClassDefFoundError.h"
#endif
#include "java_lang_OutOfMemoryError.h"
#ifdef XP_MAC
#include "jdk_java_lang_Object.h"
#else
#include "java_lang_Object.h"
#endif

#if defined(XP_PC) && !defined(_WIN32)
#if !defined(stderr)
extern FILE *stderr;
#endif
#if !defined(stdout)
extern FILE *stdout;
#endif
#endif

/*
 * Static data.
 */

/* Internal exception objects */
static Classjava_lang_NoClassDefFoundError noClassDefFound;
static Classjava_lang_OutOfMemoryError outOfMemory;

/* The classes the internal exception objects belong to */
static char *internalExceptionClasses[IEXC_END] = {
    "",						/* IEXC_NoException */
    JAVAPKG "NoClassDefFoundError",
    JAVAPKG "OutOfMemoryError"
};

/* An array of handles to the internal exception objects */
static JHandle internalExceptions[IEXC_END] = {
    { NULL, NULL },
    { (ClassObject *) &noClassDefFound, NULL },
    { (ClassObject *) &outOfMemory, NULL }
};

/*
 * Internal routines.
 */

/*
 * exceptionInitInternalObjects() -- Initialize the exception subsystem's
 *	internal objects.
 */
void
exceptionInit(void) {
    internal_exception_t	ie;
    ClassClass			*cb;
    ClassObject			*object;
    JHandle				*handle;

    /* Initialize the internal objects */
    for (ie = (internal_exception_t)IEXC_NONE + 1; ie < (internal_exception_t)IEXC_END; ie++) {

	/* Look up class */
	if ((cb = FindClassFromClass(NULL, internalExceptionClasses[ie], TRUE, NULL)) != 0) {
	    handle = &internalExceptions[ie];
	    handle->methods = cbMethodTable(cb);
            cb->flags |= CCF_Sticky;
	    
	    /* Initialize the object */
	    object = unhand(handle);
	    memset((char *) object, 0, cbInstanceSize(cb));
	}
    }
}

/*
 * exceptionInternalObject() -- Return an internal, preallocated
 *	exception object.  These are shared by all threads, so they
 *	should only be used in a last ditch effort.
 */
exception_t	    
exceptionInternalObject(internal_exception_t exc)
{
    if (exc <= IEXC_NONE || exc >= IEXC_END ||
	internalExceptions[exc].methods == NULL) {
	return NULL;
    }
    return &internalExceptions[exc];
}

/*
 * exceptionDescribe() -- Print out a description of a given exception
 *	object.
 */
void
exceptionDescribe(ExecEnv *ee)
{
    exception_t exc = ee->exception.exc;

    if (is_instance_of(exc, classJavaLangThreadDeath, ee)) {
	/* Don't print anything if we are being killed. */
    } else {
	if (is_instance_of(exc, classJavaLangThrowable, ee)) {
	    /* Object is an exception. */
	    execute_java_dynamic_method(ee, exc, "printStackTrace", "()V");
	}
#ifdef DEBUG
	else {
	    char buf[128];
	    ClassClass *class = unhand(java_lang_Object_getClass(exc));
	    prints("Exception ");
	    if (threadSelf()) {
		prints("in thread ");
		prints(Object2CString((JHandle *)getThreadName()));
	    }
	    printf(".  Uncaught exception of type %s\n", 
		   classname2string(classname(class), buf, sizeof(buf)));
	    fflush(stdout);
	}
#endif
    }
}
