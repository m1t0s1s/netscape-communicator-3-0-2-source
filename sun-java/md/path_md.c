/*
 * @(#)path_md.c	1.18 95/10/18
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
 * Machine dependent path name and file name manipulation code
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef XP_MAC
#include "prmacos.h"
#endif

#include "sys_api.h"
#include "io_md.h"
#include "path.h"	/* for cpe_t */
#include "zip.h"	/* for zip_t */
#include "jri.h"

#ifndef DEBUG_jwz	/* You're all a bunch of weenies. */
# undef ZIP_NAME	/* Remove this ifdef to Do the Right Thing. */
#endif

#ifndef ZIP_NAME
  /* This fallback value is only used if -DZIP_NAME was not passed in as
     a cc option by the Makefile. */
# define ZIP_NAME   "java_301"
#endif

char *path_md_zip_name = ZIP_NAME;


#ifdef XP_PC
#define strcasecmp(a,b)     strcmpi(a,b)


char *
sysNativePath(char *path)
{
    char *pp;

    for (pp = path; *pp != '\0'; pp++) {
        if (*pp == '/') {
            *pp = '\\';
        }
    }
    return path;
}

#endif /* XP_PC */

static const char* clpath = NULL;

JRI_PUBLIC_API(const char*)
CLASSPATH(void)
{
	return clpath;
}

JRI_PUBLIC_API(void)
setCLASSPATH(const char* cp)
{
	if (clpath) free((char*)clpath);
	clpath = cp;
}

cpe_t **
sysGetClassPath(void)
{
	static cpe_t **classpath;
	
	if (classpath == 0) {			/* once and only once */
		char *cps, *strPtr;
		int numClassPathEntries = 1;
		cpe_t **cpp;
		
		/* 	Grab the environment variable containting the list of class paths
			and make a copy of it for local munging. */
		if ((cps = (char*)CLASSPATH()) == 0) {
			cps = ".:classes";
		}
		if ((cps = strdup(cps)) == 0) {
			return 0;				/* Out of memeory */
		}
		
		/* Count the number of paths in the class path list */
		for (strPtr = cps; *strPtr != '\0'; strPtr++) {
			if (*strPtr == PATH_SEPARATOR) {
				numClassPathEntries++;
			}
		}
		
		/* Allocate a class path element structure for each path in the list */
		cpp = classpath = (cpe_t **) sysMalloc((numClassPathEntries + 1) * sizeof(cpe_t *));
		if (cpp == 0) {
			return 0;				/* Out of memeory */
		}
		
		while (cps && *cps) {		/* For each path in the list */
			struct stat statBuf;
			char *path = cps;
			if ((cps = strchr(cps, PATH_SEPARATOR)) != 0) {
				*cps++ = '\0';
			}
			if (*path == '\0') {
				path = ".";
			}
			sysNativePath(path);
			if (sysStat(path, &statBuf) == 0 && S_ISREG(statBuf.st_mode)) {		/* it's a file */
				int len = strlen(path);
                                int zip_len = strlen(path_md_zip_name);
				if (((len > 4) && (strcasecmp(path+len-4, ".zip") == 0)) || /* Is it a zip file? */
                                    ((len >= zip_len) && 
                                     (strcasecmp(path+len-zip_len, path_md_zip_name) == 0))) { /* java_301 file ? */
					zip_t *zip = zip_open(path);
					if (zip != 0) {												/* successfully opened it */
						cpe_t *cpe = sysMalloc(sizeof(cpe_t));
						if (cpe == 0) {
							return 0;				/* Out of memeory */
						}
						cpe->type = CPE_ZIP;
						cpe->u.zip = zip;
						*cpp++ = cpe;
					}
				}
			} else {										/* not a file, assume directory */
				cpe_t *cpe = sysMalloc(sizeof(cpe_t));
				if (cpe == 0) {
					return 0;				/* Out of memeory */
				}
				cpe->type = CPE_DIR;
				cpe->u.dir = path;
				*cpp++ = cpe;
			}
		}
		*cpp = 0;
	}
	return classpath;
}

