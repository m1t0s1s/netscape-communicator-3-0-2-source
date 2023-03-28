/*
 * @(#)javap.c	1.20 95/11/16
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
 *	print an java object file
 */

#include "code.h"
#include "decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <interpreter.h>

extern int decode_verbose;
static void usage(char *progname);

#include <path_md.h>

#define RELEASE "1.0"

int
main(argc, argv)
    int argc;
    register char **argv;
{
    int         processedfile = 0;
    char       *progname;
	const char* cp;

    if ((progname = strrchr(argv[0], LOCAL_DIR_SEPARATOR)) != 0) {
	progname++;
    } else {
	progname = argv[0];
    }
	cp = getenv("CLASSPATH");
    if (cp) cp = strdup(cp);
    setCLASSPATH(cp);

    while (--argc > 0)
	if ((++argv)[0][0] == '-')
	    switch (argv[0][1]) {
	    case 'v':
		if (strcmp(argv[0], "-verify") == 0) {
		    verifyclasses = VERIFY_ALL;
#ifdef DEBUG
		} else if (strcmp(argv[0], "-verify-verbose") == 0) {
		    extern int verify_verbose;
		    verify_verbose++;
#endif
                } else if (strcmp(argv[0], "-version") == 0) {
                    fprintf(stderr, "%s version \"%s\"\n", progname, RELEASE);
                    processedfile++;  /* suppress usage warning. */
		} else {
		    decode_verbose = 1;
		}
		break;
	    case 'c':
		if (strcmp(argv[0], "-classpath") == 0) {
		    if (argc > 1) {
			setCLASSPATH(strdup(argv[1]));
			argc--; argv++;
		    } else {
			fprintf(stderr,
			     "-classpath requires class path specification\n");
			usage(progname);
			return 1;
		    }
		} else if (strcmp(argv[0], "-c") == 0) {
		    PrintCode = 1;
		} else {
		    usage(progname);
		    return 1;
		}
		break;
	    case 'P':
		PrintConstantPool = 1;
		break;
	    case 'p':
		PrintPrivate = 1;
		break;
	    case 'h':
		PrintCode = 0;
		PrintAsC = 1;
		PrintPrivate = 1;
		break;
	    case 'l':
		PrintLineTable = 1;
		PrintLocalTable = 1;
		break;
	    default:
		fprintf(stderr, "%s: Illegal option %s\n\n", progname, argv[0]);
		usage(progname);
		return 1;
	} else if (strchr(argv[0], '/')) {
	    fprintf(stderr, "Invalid class name: %s\n", *argv);
	    usage(progname);
	    return 1;
	} else {
	    char *classname = argv[0];
	    char *classcopy = strdup(classname);
	    char *p;
	    extern retcode;

	    /* Convert all periods in the classname to slashes */
	    for (p = classcopy; ((p = strchr(p, '.')) != 0); *p++ = '/');

	    retcode = 0;
	    DecodeFile(classcopy);
	    processedfile++;
	    free(classcopy);
	    if (retcode != 0) 
	        fprintf(stderr, "Can't find class %s\n", classname);
	}
    if (processedfile == 0) {
	usage(progname);
	return 1;
    }
    return 0;
}

static void usage(char *progname) {
    fprintf(stderr, "Usage: %s [-v] [-c] [-p] [-h] [-verify] [-verify-verbose] files...\n", progname);
    fprintf(stderr, "\n");
    fprintf(stderr, "where options include:\n");
    fprintf(stderr, "   -c         Disassemble the code\n");
    fprintf(stderr, "   -classpath <directories separated by colons>\n");
    fprintf(stderr, "              List directories in which to look for classes\n");

    fprintf(stderr, "   -h         Create info that can be put into a C header file.\n");
    fprintf(stderr, "   -l         Print local variable tables.\n");
    fprintf(stderr, "   -p         Include private fields and methods\n");
    fprintf(stderr, "   -v         Print verbosely\n");
    fprintf(stderr, "   -verify    Run the verifier\n");
#ifdef DEBUG
    fprintf(stderr, "   -verify-verbose\n");
#endif
    fprintf(stderr, "              Verify, printing out debugging info\n");
    fprintf(stderr, "   -version   Print the javap version string\n");
    fprintf(stderr, "\n");
}
    

int
OpenCode(char *fn, char *sfn, char *dir, struct stat * st)
{
    long codefd;
    if (fn == 0 || (codefd = sysOpen(fn, 0, 0644)) < 0
	    || fstat(codefd, st) < 0)
	return -2;
    return codefd;
}

ClassClass *AllocClass(void)
{
    return (ClassClass*) calloc(sizeof(ClassClass), 1);
}

void *AllocContext(size_t size)
{
    return calloc(size, 1);
}

void *gcInfo;
