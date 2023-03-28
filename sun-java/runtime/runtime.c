/*
 * @(#)runtime.c	1.31 95/11/29  
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
 *      Link foreign methods.  This first half of this file contains the
 *	machine independent dynamic linking routines.
 *	See "BUILD_PLATFORM"/java/lang/linker_md.c to see
 *	the implementation of this shared dynamic linking
 *	interface.
 *
 *	NOTE - source in this file is POSIX.1 compliant, host
 *	       specific code lives in the platform specific
 *	       code tree.
 */

#include "oobj.h"
#include "interpreter.h"
#include "tree.h"
#include "javaString.h"

#include "sys_api.h"
#include "path.h"
#include "jio.h"

#include "java_lang_Runtime.h"

static char *security_exception = JAVAPKG "SecurityException";

static JavaRTMode java_rt_mode = JRT_NONE;

void set_java_rt_mode(JavaRTMode new_mode)
{
    java_rt_mode = new_mode;
}

void
java_lang_Runtime_exitInternal(Hjava_lang_Runtime *this, long code)
{
    if (java_rt_mode == JRT_NONE) {
	SignalError(0, security_exception, "exit is not allowed");
	return;
    }
    sysExit((int)code);
}


/* 
 * Initialize the linker. The library search path is returned.
 */

HString *
java_lang_Runtime_initializeLinkerInternal(struct Hjava_lang_Runtime *Hlink) 
{
    char *ldpathbuf;

    ldpathbuf = sysInitializeLinker();
    if (ldpathbuf) {
		return makeJavaString(ldpathbuf, strlen(ldpathbuf));
	} else {
		return 0;
	}
}

/*
 * Load a shared library;  return true if loaded successfully.
 */

long /* bool_t */
java_lang_Runtime_loadFileInternal(struct Hjava_lang_Runtime *Hlinker, 
				   HString *Hfilename) {
    char fname[128];
    if (Hfilename == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "filename");
	return 0;
    }
    javaString2CString(Hfilename, fname, sizeof fname);
    return sysAddDLSegment(fname);
}


/*
 * build a complete ld library string name with path, filename, and ext's
 */

HString *
java_lang_Runtime_buildLibName(struct Hjava_lang_Runtime *Hlinker,
			       HString *Hpath,
			       HString *Hfilename)
{
    char fname[128];
    char pname[1024];
    char holder[1280];
    int  holderlen = sizeof(holder);

    if ((Hpath == 0) || (Hfilename == 0)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    javaString2CString(Hpath, pname, sizeof(pname));    
    javaString2CString(Hfilename, fname, sizeof(fname));
    sysBuildLibName(holder, holderlen, pname, fname);
    return(makeJavaString(holder, strlen(holder)));
}


int64_t
java_lang_Runtime_freeMemory(Hjava_lang_Runtime *this)
{
    return ll_add(FreeObjectMemory(), FreeHandleMemory());
}

int64_t
java_lang_Runtime_totalMemory(Hjava_lang_Runtime *this)
{
    return ll_add(TotalObjectMemory(), TotalHandleMemory());
}

void
java_lang_Runtime_gc(Hjava_lang_Runtime *this) {
    extern void gc(int, unsigned int);
    gc(0, 0);
}

void
java_lang_Runtime_runFinalization(Hjava_lang_Runtime *this) {
    extern void runFinalization(void);
    runFinalization();
}

void
java_lang_Runtime_traceInstructions(Hjava_lang_Runtime *this, long on) {
    extern void TraceInstructions(long);
    if (java_rt_mode == JRT_NONE) {
	SignalError(0, security_exception, "instruction tracing is not allowed");
	return;
    }
    (void) TraceInstructions(on);
}

void
java_lang_Runtime_traceMethodCalls(Hjava_lang_Runtime *this, long on) {
    extern void TraceMethodCalls(long);
    if (java_rt_mode == JRT_NONE) {
	SignalError(0, security_exception, "method tracing is not allowed");
	return;
    }
    (void) TraceMethodCalls(on);
}

