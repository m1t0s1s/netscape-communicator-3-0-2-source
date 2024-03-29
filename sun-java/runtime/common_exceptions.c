/*
 * @(#)common_exceptions.c	1.10 95/08/11 Peter King
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
 * Common exceptions thrown from within the interpreter (and
 * associated libraries).
 */

/*
 * Header files.
 */
#include <common_exceptions.h>
#include <oobj.h>
#include <interpreter.h>
#include <javaThreads.h>

#include "java_lang_Thread.h"

/*
 * Macros.
 */

#define	JAVA_LANG_EXCEPTION_ROUTINE(name)			\
void								\
name(void)							\
{								\
    SignalError(0, JAVAPKG #name, 0);				\
}

/*
 * External routines.
 */
JAVA_LANG_EXCEPTION_ROUTINE(OutOfMemoryError)


