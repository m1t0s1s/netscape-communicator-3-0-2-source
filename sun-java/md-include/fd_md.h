/*
 * @(#)fd_md.h	1.4 95/11/11  Pavani Diwanji
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

#ifndef	FD_MD_H_
#define	FD_MD_H_

#ifndef XP_PC
#include <sys/param.h>
#include <dirent.h>
#include <unistd.h>
#endif
#include "java_io_FileDescriptor.h"

void sysInitFD(Classjava_io_FileDescriptor *, int);

int sysOpenFD(Classjava_io_FileDescriptor *, const char *, int, int);
int sysCloseFD(Classjava_io_FileDescriptor *);
void sysSocketFD(Classjava_io_FileDescriptor *, long);
int sysReadFD(Classjava_io_FileDescriptor *, char *, int32_t);
int sysWriteFD(Classjava_io_FileDescriptor *, char *, int32_t);
int sysRecvFD(Classjava_io_FileDescriptor *, char *, int, int32_t);
int sysSendFD(Classjava_io_FileDescriptor *, char *, int, int32_t);
int sysAvailableFD(Classjava_io_FileDescriptor *, long *);
long sysLseekFD(Classjava_io_FileDescriptor *, long, long);
int sysBindFD(Classjava_io_FileDescriptor *, void*, int);

#endif	/* !FD_MD_H_ */
