/*
 * @(#)classresolver_stubs.c	1.10 96/01/10  
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
 *    This file has a number of stub routines that sre compiled into
 *    programs that do not need to load and initialize classes
 *    but do not have the full functionality of the interpreter.
 *    This would be things like JAVAP, JAVAH, and possibly JAVAC.
 */

#include <oobj.h>
#include "interpreter.h"

int verify_in_progress;

/*  Perform runtime-specific initialization on the class.  In the
    interpreter this routine would Resolve the instance prototype,
    load class constants, and run the static initializers, if any.
    For other uses we just stub the routine out.
*/
char *
RuntimeInitClass(struct execenv *ee, ClassClass * cb)
{
    return (char *) 0;
}

/*
    Make an java array of chars from a C array of chars.
    This is a no-op in anything but the interpreter because 
    we do not have a heap out of which to allocate.
*/
HArrayOfChar *
MakeString(char *str, long len)
{
    return (HArrayOfChar *) 0;
}


void
SignalError(struct execenv * ee, char *ename, char *DetailMessage)
{
    fprintf(stderr, 
	    "Signalled error \"%s\" with detail \"%s\"\n",
	    ename, DetailMessage);
}


/* 
   Provide a static environment struct, since we won't be runing
   multi-threaded in javah, javap, and javac.
*/

ExecEnv *
EE() {
    static struct execenv lee;
    return &lee;
}

char *CompiledCodeAttribute = 0;

int sysMonitorSizeof()
{
    return 1;
}



/* Verify that current_class can a field of new_class, where that field's 
 * access bits are "access".  We assume that we've already verified that
 * class can access field_class.
 *
 * If the classloader_only flag is set, we automatically allow any accesses
 * in which current_class doesn't have a classloader.
 *
 * This is a no-op in anything but the interpreter. 
 */


bool_t 
VerifyFieldAccess(ClassClass *current_class, ClassClass *field_class, 
		  int access, bool_t classloader_only)
{
  return TRUE;
}

