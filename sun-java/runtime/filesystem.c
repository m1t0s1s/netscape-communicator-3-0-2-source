/*
 * @(#)filesystem.c	1.36 95/11/15 Jonathan Payne
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

/* This is a system-independent level of file system operations. */

#include "io_md.h"
#include "oobj.h"
#include "typecodes.h"
#include "interpreter.h"
#include "javaString.h"
#include <sys/types.h>
#include <sys/stat.h>

#include "java_io_File.h"
#include "sys_api.h"
#include "jio.h"

static char *security_exception = JAVAPKG "SecurityException";
static JavaFSMode java_fs_mode = JFS_UNRESTRICTED;

void set_java_fs_mode(JavaFSMode new_mode)
{
    java_fs_mode = new_mode;
}

long
java_io_File_exists0(Hjava_io_File *this) 
{
    Classjava_io_File *thisptr = unhand(this);
    char path[MAXPATHLEN];

    if (thisptr->path == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    if (java_fs_mode != JFS_UNRESTRICTED) {
	SignalError(0, security_exception, 0);
	return 0;
    }

    javaString2CString(thisptr->path, path, sizeof (path));
    return sysAccess(path, F_OK) ? 0 : 1;
}

long
java_io_File_canRead0(Hjava_io_File *this) 
{
    Classjava_io_File *thisptr = unhand(this);
    char path[MAXPATHLEN];

    if (thisptr->path == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    if (java_fs_mode != JFS_UNRESTRICTED) {
	SignalError(0, security_exception, 0);
	return 0;
    }

    javaString2CString(thisptr->path, path, sizeof (path));
    return sysAccess(path, R_OK) ? 0 : 1;
}

long
java_io_File_canWrite0(Hjava_io_File *this) 
{
    Classjava_io_File *thisptr = unhand(this);
    char path[MAXPATHLEN];

    if (thisptr->path == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    if (java_fs_mode != JFS_UNRESTRICTED) {
	SignalError(0, security_exception, 0);
	return 0;
    }

    javaString2CString(thisptr->path, path, sizeof (path));
    return sysAccess(path, W_OK) ? 0 : 1;
}

long
java_io_File_isFile0(Hjava_io_File *this) 
{
    Classjava_io_File *thisptr = unhand(this);
    char path[MAXPATHLEN];
    struct stat buf;

    if (thisptr->path == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    if (java_fs_mode != JFS_UNRESTRICTED) {
	SignalError(0, security_exception, 0);
	return 0;
    }

    javaString2CString(thisptr->path, path, sizeof (path));
    return ((sysStat(path, &buf) != -1) && S_ISREG(buf.st_mode)) ? 1 : 0;
}

long
java_io_File_isDirectory0(Hjava_io_File *this) 
{
    Classjava_io_File *thisptr = unhand(this);
    char path[MAXPATHLEN];
    struct stat buf;

    if (thisptr->path == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    if (java_fs_mode != JFS_UNRESTRICTED) {
	SignalError(0, security_exception, 0);
	return 0;
    }

    javaString2CString(thisptr->path, path, sizeof (path));
    return ((sysStat(path, &buf) != -1) && S_ISDIR(buf.st_mode)) ? 1 : 0;
}

long
java_io_File_isAbsolute(Hjava_io_File *this)
{
    Classjava_io_File *thisptr = unhand(this);
    char path[MAXPATHLEN];

    if (thisptr->path == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    javaString2CString(thisptr->path, path, sizeof (path));

    return sysIsAbsolute(path);
}

int64_t
java_io_File_lastModified0(Hjava_io_File *this) 
{
    Classjava_io_File *thisptr = unhand(this);
    char path[MAXPATHLEN];
    struct stat buf;
    int64_t mtime;

    if (thisptr->path == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return ll_zero_const;
    }
    if (java_fs_mode != JFS_UNRESTRICTED) {
	SignalError(0, security_exception, 0);
	return ll_zero_const;
    }

    javaString2CString(thisptr->path, path, sizeof (path));
    if (sysStat(path, &buf) != -1) {
	mtime = ll_mul(int2ll(buf.st_mtime), int2ll(1000));
    } else {
	mtime = ll_zero_const;
    }  
    return mtime;
}

int64_t
java_io_File_length0(Hjava_io_File *this) 
{
    Classjava_io_File *thisptr = unhand(this);
    char path[MAXPATHLEN];
    struct stat buf;

    if (thisptr->path == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return ll_zero_const;
    }
    if (java_fs_mode != JFS_UNRESTRICTED) {
	SignalError(0, security_exception, 0);
	return ll_zero_const;
    }

    javaString2CString(thisptr->path, path, sizeof (path));
    return (sysStat(path, &buf) != -1)  ? int2ll(buf.st_size) : ll_zero_const;
}


long
java_io_File_mkdir0(Hjava_io_File *this) 
{
    Classjava_io_File *thisptr = unhand(this);
    char path[MAXPATHLEN];

    if (thisptr->path == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    if (java_fs_mode != JFS_UNRESTRICTED) {
	SignalError(0, security_exception, 0);
	return 0;
    }

    javaString2CString(thisptr->path, path, sizeof (path));

    return sysMkdir(path, 0771) != -1;
}

long
java_io_File_renameTo0(Hjava_io_File *this, Hjava_io_File *dest) 
{
    Classjava_io_File *thisptr = unhand(this);
    Classjava_io_File *destptr;
    char path[MAXPATHLEN];
    char dpath[MAXPATHLEN];

    if ((thisptr->path == 0) || (dest == 0)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    if (java_fs_mode != JFS_UNRESTRICTED) {
	SignalError(0, security_exception, 0);
	return 0;
    }

    javaString2CString(thisptr->path, path, sizeof (path));
    destptr = unhand(dest);
    javaString2CString(destptr->path, dpath, sizeof (dpath));

    return rename(path, dpath) != -1;
}

long
java_io_File_delete0(Hjava_io_File *this) 
{
    Classjava_io_File *thisptr = unhand(this);
    char path[MAXPATHLEN];
  
    if (thisptr->path == 0) {
  	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    if (java_fs_mode != JFS_UNRESTRICTED) {
	SignalError(0, security_exception, 0);
	return 0;
    }

    javaString2CString(thisptr->path, path, sizeof (path));

    return sysUnlink(path) != -1;
}

HArrayOfString *
java_io_File_list0(Hjava_io_File *this) 
{
    Classjava_io_File *thisptr = unhand(this);
    char path[MAXPATHLEN];
    PRDir *dir;
    struct PRDirEntryStr *ptr;
    int len, maxlen;
    HArrayOfString *arr;
    HArrayOfString *narr;
  
    if (thisptr->path == 0) {
  	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    if (java_fs_mode != JFS_UNRESTRICTED) {
	SignalError(0, security_exception, 0);
	return 0;
    }

    /* open the directory */
    javaString2CString(thisptr->path, path, sizeof (path));

    if ((dir = sysOpenDir(path)) == NULL) {
	return NULL;
    }

    /* allocate a list of string handles */
    len = 0;
    maxlen = 4;
    arr = (HArrayOfString *)ArrayAlloc(T_CLASS, maxlen);
    if (arr == NULL) {
  	sysCloseDir(dir);
  	SignalError(0, JAVAPKG "OutOfMemoryError", path);
  	return NULL;
    }

    /* scan the directory */
    while ((ptr = sysReadDir(dir)) != NULL) {
  	if (strcmp(ptr->d_name, ".") && strcmp(ptr->d_name, "..")) {
  	    if (len == maxlen) {
 		narr = (HArrayOfString *) ArrayAlloc(T_CLASS, maxlen <<= 1);
 		if (narr == NULL) {
  		    sysCloseDir(dir);
  		    SignalError(0, JAVAPKG "OutOfMemoryError", path);
 		    arr = NULL;		/* Hide arr from GC */
  		    return NULL;
  		}
 		memcpy(unhand(narr), unhand(arr), len * sizeof(HString *));
 		arr = narr;
 		narr = 0;		/* Hide narr from GC */
  	    }
  
  	    /* add it to the list */
 	    unhand(arr)->body[len++] =
 		makeJavaString(ptr->d_name, strlen(ptr->d_name));
  	}
    }
  
    /* allocate the array */
    narr = (HArrayOfString *)ArrayAlloc(T_CLASS, len);
    if (narr == NULL) {
  	sysCloseDir(dir);
  	SignalError(0, JAVAPKG "OutOfMemoryError", path);
 	arr = 0;			/* Hide arr from GC */
  	return NULL;
    }
    unhand((HArrayOfObject *)narr)->body[len] = (HObject *)classJavaLangString;
    memcpy(unhand(narr), unhand(arr), len * sizeof(HString *));
  
      /* free the list */
    sysCloseDir(dir);
    arr = 0;
    return narr;
}
