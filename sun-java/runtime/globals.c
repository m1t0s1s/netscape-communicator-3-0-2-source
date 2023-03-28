/*
 * @(#)globals.c	1.14 95/11/15 John Seamons
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
 *      Some development environments don't support common storage
 *      (i.e. data in multiply included .h files not marked extern).
 *      Here is where we declare everything in .h files marked extern.
 */


#include "oobj.h"
#include "tree.h"
#include "interpreter.h"

int   SkipSourceChecks = 0;
char *progname;
ClassClass **binclasses = NULL;
long nbinclasses, sizebinclasses;
int verbose = 0;
int verbosegc;
int verifyclasses;
int noasyncgc;
int debugagent;

int ImportAcceptable;
int InhibitExecute;
unsigned char **mainstktop;

#include "decode.h"
int     PrintCode;		/* used by javap */
int     PrintAsC;		/* used by javap */
int     PrintPrivate;		/* used by javap */
int	PrintLocalTable;	/* used by javap */
int	PrintLineTable;		/* used by javap */
int	PrintConstantPool;	/* used by javap */

/* Win32 can't export variables to shared-libraries, just functions.
 * These access routines are therefore needed by the debugger.
 */
ClassClass** get_binclasses(void)  { return binclasses; }
ClassClass*  get_classClass(void)  { return classJavaLangClass; }
ClassClass*  get_classObject(void) { return classJavaLangObject; }
long         get_nbinclasses(void) { return nbinclasses; }


#if defined(XP_PC) && !defined(WIN32) && !defined(_WIN32)
	/* Win16 can't reference variables from shared libraries (as above), no
	 * sense in making these functions, since they're supposed to be read only
	 */
	#ifndef HAVE_LONG_LONG
		#ifdef IS_LITTLE_ENDIAN
		int64 LL_MAXINT = { 0xffffffff, 0x7fffffff };
		int64 LL_MININT = { 0x00000000, 0x80000000 };
		#else
		int64 LL_MAXINT = { 0x7fffffff, 0xffffffff };
		int64 LL_MININT = { 0x80000000, 0x00000000 };
		#endif
	#endif

	#if defined(_WINDLL)
		FILE *stderr;
		FILE *stdout;
	#endif

#endif
