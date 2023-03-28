/*
 * @(#)javai.c	1.75 95/08/11
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
 *      interpret an java object file
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "javaThreads.h"
#include "errno.h"
#include "tree.h"
#include "oobj.h"
#include "interpreter.h"
#include "exceptions.h"
#include "javaString.h"
#include "log.h"
#include "installpath.h"
#include "bool.h"

#ifndef NATIVE
#include "iomgr.h"
#include <fcntl.h>
#ifndef XP_PC
#include <sys/wait.h>
#include <unistd.h>
#include <sys/signal.h>
#endif /* ! WIN32 */
#include <signal.h>
#else /* NATIVE  */
#include "monitor.h"
#endif /* NATIVE */

#if defined(XP_PC) && !defined(_WIN32)
#include "jrtpcos.h"
#endif

void
Execute(char **command, char *alternate);

/* OpenCode for the interpreter.

   Tries to open a code file (fn).  That is, a .class file.  This
   returns -1 if the specified file can be opened but cannot be
   stat'd.  REMIND: When can that happen?  It returns -2 if the
   file cannot be opened.  Otherwise, it returns a valid file
   descriptor. If SkipSourceChecks is true, then we return at that
   point.

   Otherwise, we try to recompile the source.  If fn == 0, then we
   use sfn as the source file name to use.  sfn is specified by
   LoadFile by using the source hint associated with the import
   statement, or by the source file name as extracted from the
   previously recompiled version of this class.  That only happens
   if LoadFile already successfully loaded the class, and then
   noticed that the source is more recent than the .class file it
   opened.

   If sfn is 0 then a source file name is constructed by stripping
   the ".class" off of fn and replacing it with .java.  This often
   doesn't work, because .class files can be put in difference
   directories from their source files, but it's a good guess
   sometimes.  Then javac is forked to recompile the class, and
   the resulting .class file is opened.

   In summary, OpenCode for the interpreter returns a file
   descriptor for a compiler .class file.  If it can't open the
   .class file, it forks the compiler to compile some source so
   that the .class file will be there.  If after all that there is
   still no .class file, it gives up and returns -2.

   If st == 0, then doesn't even bother trying to open the file.
   Instead, it just does the source file thing. */

#ifndef XP_PC
int
OpenCode(char *fn, char *sfn, char *dir, struct stat * st)
{
    long fd = -1;
    char srcname[300];
    char sccsname[300];
    long mtime;
    struct stat srcst, sccsst;
    char *src, *dst;

#ifndef XP_PC
    if (st != 0 && fn != 0 && (fd = open(fn, O_RDONLY, 0644)) >= 0
#else
    if (st != 0 && fn != 0 && (fd = open(fn, O_RDONLY|O_BINARY, 0644)) >= 0
#endif
	    && fstat(fd, st) < 0) {
	close(fd);
	fd = -1;
    }
    if (SkipSourceChecks)
	return fd < 0 ? -2 : fd;
    dst = srcname;
    *dst++ = '-';
    *dst++ = 'G';
    if (sfn == 0) {
	sysAssert(fn);
	for ((src = fn); (*dst++ = *src++););
	if (dst - srcname < 3 + JAVAOBJEXTLEN
		|| strcmp(dst - JAVAOBJEXTLEN - 2,
			  "." JAVAOBJEXT) != 0)
	    return fd < 0 ? -2 : fd;
	dst -= JAVAOBJEXTLEN + 2;
	strcpy(dst, "." JAVASRCEXT);
    } else {
	for ((src = sfn); (*dst++ = *src++););
    }
    while (dst > srcname + 2 && dst[-1] != '/')
	dst--;
    strncpy(sccsname, srcname + 2, dst - srcname);
    strncpy(sccsname + (dst - srcname - 2), "SCCS/s.", 7);
    strcpy(sccsname + (dst - srcname) + 5, dst);
    mtime = ((fd < 0 && fn) || st == 0) ? 0 : st->st_mtime;
    if (stat(srcname + 2, &srcst) < 0)
	srcst.st_mtime = 0;
    if (stat(sccsname, &sccsst) < 0)
	sccsst.st_mtime = 0;
    if (srcst.st_mtime > mtime
	    || sccsst.st_mtime > mtime) {
	char *com[30];
	char **comp = com;
	if (fd >= 0) {
	    close(fd);
	    fd = -1;
	}
	if (sccsst.st_mtime > srcst.st_mtime) {
	    com[0] = "/usr/sccs/get";
	    com[1] = sccsname;
	    com[2] = srcname;
	    com[3] = 0;
	    Execute(com, 0);
	}
#ifdef DEBUG
	*comp++ = "javac_g";
#else
	*comp++ = "javac";
#endif
	if (verbose)
	    *comp++ = "-verbose";
	if (dir) {
	    *comp++ = "-d";
	    *comp++ = dir;
	}
	*comp++ = srcname+2;
	*comp = 0;
	{
	    char    path[256];
	    char    *alt = 0;
	    char    *ep;


	    if ((ep = getenv("JAVA_HOME")) == 0)
		alt = 0;
	    else {
		sprintf(path, "%s/bin/", ep);
#ifdef DEBUG
		strcat(path, "javac_g");
#else
		strcat(path, "javac");
#endif
		alt = path;
	    }
	    Execute(com, alt);
	}
	if (fn) {
#ifndef XP_PC
	    fd = open(fn, 0, 0644);
#else
	    fd = open(fn, O_BINARY, 0644);
#endif
	    if (fd >= 0 && st)
		if (fstat(fd, st) < 0) {
		    close(fd);
		    fd = -1;
		}
	}
    }
    return fd < 0 ? -2 : fd;
}

void
ClassDefHook(ClassClass *cb)
{
}
#endif    /* ! XP_PC */

extern JAVA_PUBLIC_API(int) start_java(int argc, char **argv);

#if !defined(XP_PC) || defined(_WIN32)

int
main(int argc, char **argv)
{
    return start_java(argc, argv);
}

#else

extern int
JRTPC_StartJava(int argc, char *(argv[]));

FILE *output;

static void
WriteStandardOut(const char *string)
{
	fwrite(string, sizeof(char), strlen(string), output);
	fflush(output);
}

int PASCAL 
WinMain(HINSTANCE hinstCurrent, HINSTANCE hinstPrevious,
    LPSTR lpszCmdLine, int nCmdShow)
{
	extern int __argc;
	extern char ** __argv;
	int	   ac;
	char **av;
	int	   javaExitCode;
	HINSTANCE lib;

	ac = __argc;
	av = __argv;

	if (ac >2) {
		if (strcmp(av[1],"-o") == 0) {
			output = fopen(av[2],"w");
			if (output) {
				JRTPC_SetClientOutputCallback( WriteStandardOut );
				ac -= 2;
				av += 2;
			}
		}
	}

	lib = JRTPC_Init(NULL);
	javaExitCode = JRTPC_StartJava(ac, av);
        if (output != NULL)
           fclose(output);
        if (lib != (HINSTANCE) NULL)
		FreeLibrary(lib);
	return javaExitCode;
}

#endif
