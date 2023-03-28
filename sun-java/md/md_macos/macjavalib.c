/*
 * @(#)socket.c	1.8 95/08/23 Jonathan Payne
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

#include <Types.h>

#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include "socket_md.h"
#include <stdio.h>
#include <stat.h>
#include "prmacos.h"
#include "iomgr.h"
#include <fcntl.h>
#include <unistd.h>

#include "macjavalib.h"

#include "java_net_Socket.h"
#include "java_net_SocketImpl.h"
#include "java_net_PlainSocketImpl.h"
#include "java_net_ServerSocket.h"
#include "java_net_SocketInputStream.h"
#include "java_net_SocketOutputStream.h"
#include "java_net_InetAddress.h"

#define JAVANETPKG 	"java/net/"

#define BODYOF(h)   unhand(h)->body

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN	1024
#endif

#ifdef sgi
#include <signal.h>
#endif

int OpenCode(char *fn, char *sfn, char *dir, struct stat * st);

int
OpenCode(char *fn, char *sfn, char *dir, struct stat * st)
{
    long fd = -1;
    char srcname[300];
    char sccsname[300];
    long len;
    long mtime;
    struct stat srcst, sccsst;
    char *src, *dst;
    
#if defined(WIN32)
    if (st != 0 && fn != 0 && (fd = open(fn, O_RDONLY|O_BINARY, 0644)) >= 0
#elif defined(XP_MAC)
    if (st != 0 && fn != 0 && (fd = PR_OpenFile(fn, O_RDONLY, 0644)) >= 0
#else
    if (st != 0 && fn != 0 && (fd = open(fn, O_RDONLY, 0644)) >= 0
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
	for (src = fn; *dst++ = *src++;);
	if (dst - srcname < 3 + JAVAOBJEXTLEN
		|| strcmp(dst - JAVAOBJEXTLEN - 2,
			  "." JAVAOBJEXT) != 0)
	    return fd < 0 ? -2 : fd;
	dst -= JAVAOBJEXTLEN + 2;
	strcpy(dst, "." JAVASRCEXT);
    } else {
	for (src = sfn; *dst++ = *src++;);
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
	    return -1;
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
	    return -1;
	}
	if (fn) {
#if defined(WIN32)
	    fd = open(fn, O_BINARY, 0644);
#elif defined(XP_MAC)
	    fd = PR_OpenFile(fn, O_BINARY, 0644);
#else
	    fd = open(fn, 0, 0644);
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


void gcvt(float a, int size, char *buf)
{
	DebugStr("\pgcvt is not implemented");
	buf[0] = '\0';
}
