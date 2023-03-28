/*
 * @(#)io.c	1.42 95/11/15
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

#include "oobj.h"
#include "interpreter.h"
#include "javaString.h"
#include "fd_md.h"

#include "java_io_FileDescriptor.h"
#include "java_io_FileInputStream.h"
#include "java_io_FileOutputStream.h"
#include "java_io_RandomAccessFile.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define IO_EXCEPTION "java/io/IOException"


/**************************************************************
 * File Descriptor
 */

struct Hjava_io_FileDescriptor *
java_io_FileDescriptor_initSystemFD(struct Hjava_io_FileDescriptor *this, 
struct Hjava_io_FileDescriptor *fdObj, long fd){

  Classjava_io_FileDescriptor *fdptr = unhand(fdObj);
  if (fdptr == 0) {
    SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
    return 0;
  }

  sysInitFD(fdptr, (int) fd);
  return fdObj;
}

/* To start with we will have this routine in shared code; if needed
it can be moved to system specific io_md.c */
long java_io_FileDescriptor_valid(struct Hjava_io_FileDescriptor *fdObj) {
  Classjava_io_FileDescriptor *fdptr = unhand(fdObj);
  if (fdptr == 0) {
    SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
    return 0;
  }
  return (fdptr->fd)>0;
}

/**************************************************************
 * Input stream
 */

void
java_io_FileInputStream_open(Hjava_io_FileInputStream *this, HString *name)
{
    Classjava_io_FileInputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    char buf[MAXPATHLEN];
    int fd, flags;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    if (name == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    javaString2CString(name, buf, sizeof(buf));

    flags = O_RDONLY;
    if ((fd = sysOpenFD(fdptr, buf, flags, 0)) == -1) {
	SignalError(0, IO_EXCEPTION, buf);
	return;
    }
}

long
java_io_FileInputStream_read(Hjava_io_FileInputStream *this)
{
    Classjava_io_FileInputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);

    unsigned char buf[1];
    int	    cnt;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return 0;
    }

    if ((cnt = sysReadFD(fdptr, (char *) buf, 1)) != 1) {
	if (cnt == 0) {
	    return -1;	    /* EOF */
	}
	if (errno != EINTR)
	    SignalError(0, IO_EXCEPTION, "read error");
    }
    return buf[0];
}

long
java_io_FileInputStream_readBytes(Hjava_io_FileInputStream *this, HArrayOfByte *data, long off, long len)
{
    Classjava_io_FileInputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    char *dataptr;
    uint32_t datalen, n;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return 0;
    }

    if (data == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    dataptr = unhand(data)->body;
    datalen = obj_length(data);

    if ((off < 0) || ((uint32_t)off > datalen)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return -1;
    }
    if ((uint32_t)off + len > datalen) {
	len = datalen - off;
    }
    if (len <= 0) {
	return 0;
    }

    n = sysReadFD(fdptr, dataptr + off, len);
    if (n == -1) {
	SignalError(0, IO_EXCEPTION, "read error");
    }

    /* AVH: this is bogus but it stops the pointer from being gc'd */
    if (dataptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
    }
    return (n == 0) ? -1 : n;
}

int64_t
java_io_FileInputStream_skip(Hjava_io_FileInputStream *this, int64_t n) 
{
    Classjava_io_FileInputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    off_t cur = 0, end = 0;

     if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return ll_zero_const;
    }

    if ((cur = sysLseekFD(fdptr, 0L, SEEK_CUR)) == -1) {
	SignalError(0, IO_EXCEPTION, 0);
    } else if ((end = sysLseekFD(fdptr, ll2int(n), SEEK_CUR)) == -1) {
	SignalError(0, IO_EXCEPTION, 0);
    }
    return int2ll(end - cur);
}

long
java_io_FileInputStream_available(Hjava_io_FileInputStream *this)
{
    Classjava_io_FileInputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    long bytes;

     if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return 0;
    }

    if (sysAvailableFD(fdptr, &bytes)) {
	return bytes;
    }
    SignalError(0, IO_EXCEPTION, 0);
    return 0;
}

void
java_io_FileInputStream_close(Hjava_io_FileInputStream *this) 
{
    Classjava_io_FileInputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    sysCloseFD(fdptr);
}

/**************************************************************
 * Output stream
 */

void
java_io_FileOutputStream_open(Hjava_io_FileOutputStream *this, HString *name)
{
    Classjava_io_FileOutputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    char buf[MAXPATHLEN];
    int fd, flags;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    if (name == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    javaString2CString(name, buf, sizeof(buf));

    flags = O_WRONLY|O_CREAT|O_TRUNC;
    if ((fd = sysOpenFD(fdptr, buf, flags, 0664)) == -1) {
	SignalError(0, IO_EXCEPTION, buf);
	return;
    }
}

void
java_io_FileOutputStream_write(Hjava_io_FileOutputStream *this, long c) 
{
    Classjava_io_FileOutputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    char buf[1];

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    buf[0] = (char) c;
    if (sysWriteFD(fdptr, buf, 1) != 1) {
	SignalError(0, IO_EXCEPTION, "write error");
    }
}

void
java_io_FileOutputStream_writeBytes(Hjava_io_FileOutputStream *this, HArrayOfByte *data, long off, long len) 
{
    Classjava_io_FileOutputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    char *dataptr;
    uint32_t datalen, n;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    if (data == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    datalen = obj_length(data);
    dataptr = unhand(data)->body;
    if ((off < 0) || (len < 0) || ((uint32_t)off + len > datalen)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return;
    }
    while (len > 0) {
	n = sysWriteFD(fdptr, dataptr + off, len);
	if (n == -1) {
	    SignalError(0, IO_EXCEPTION, "write error");
	    break;
	}
        off += n;
	len -= n;
    }
}

void
java_io_FileOutputStream_close(Hjava_io_FileOutputStream *this) 
{
    Classjava_io_FileOutputStream *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    sysCloseFD(fdptr);
}

/**************************************************************
 * RandomAccessFile
 */

void
java_io_RandomAccessFile_open(Hjava_io_RandomAccessFile *this, HString *name,
			     long writeable)
{
    Classjava_io_RandomAccessFile *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    char buf[MAXPATHLEN];
    int fd;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    if (name == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    javaString2CString(name, buf, sizeof(buf));

    if (writeable) {
	if ((fd = sysOpenFD(fdptr, buf, O_RDWR|O_CREAT, 0664)) == -1) {
	    SignalError(0, IO_EXCEPTION, buf);
	    return;
	}
    }
    else {
	if ((fd = sysOpenFD(fdptr, buf, O_RDONLY, 0)) == -1) {
	    SignalError(0, IO_EXCEPTION, buf);
	    return;
	}
    }
}


long
java_io_RandomAccessFile_read(Hjava_io_RandomAccessFile *this)
{
    Classjava_io_RandomAccessFile *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    unsigned char buf[1];
    int	    cnt;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return 0;
    }

    if ((cnt = sysReadFD(fdptr, (char *) buf, 1)) != 1) {
	if (cnt == 0) {
	    return -1;	    /* EOF */
	}
	SignalError(0, IO_EXCEPTION, "read error");
    }
    return buf[0];
}

long
java_io_RandomAccessFile_readBytes(Hjava_io_RandomAccessFile *this, HArrayOfByte *data, long off, long len)
{
    Classjava_io_RandomAccessFile *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    char *dataptr;
    uint32_t datalen, n;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return 0;
    }

    if (data == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    datalen = obj_length(data);
    dataptr = unhand(data)->body;

    if ((off < 0) || ((uint32_t)off > datalen)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return -1;
    }
    if ((uint32_t)off + len > datalen) {
	len = datalen - off;
    }
    if (len <= 0) {
	return 0;
    }

    n = sysReadFD(fdptr, dataptr + off, len);
    if (n == -1) {
	SignalError(0, IO_EXCEPTION, "read error");
    }
    /* AVH: this is bogus but it stops the pointer from being gc'd */
    if (dataptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
    }
    return n;
}


void
java_io_RandomAccessFile_write(Hjava_io_RandomAccessFile *this, long c) 
{
    Classjava_io_RandomAccessFile *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    char buf[1];

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    buf[0] = (char) c;
    if (sysWriteFD(fdptr, buf, 1) != 1) {
	SignalError(0, IO_EXCEPTION, "write error");
    }
}

void
java_io_RandomAccessFile_writeBytes(Hjava_io_RandomAccessFile *this, HArrayOfByte *data, long off, long len) 
{
    Classjava_io_RandomAccessFile *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);
    char *dataptr;
    uint32_t datalen, n;

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    if (data == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    datalen = obj_length(data);
    dataptr = unhand(data)->body;

    if ((off < 0) || (len < 0) || ((uint32_t)off + len > datalen)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return;
    }

    while (len > 0) {
	n = sysWriteFD(fdptr, dataptr + off, len);
	if (n == -1) {
	    SignalError(0, IO_EXCEPTION, "write error");
	    break;
	}
        off += n;
	len -= n;
    }
}


int64_t
java_io_RandomAccessFile_getFilePointer(Hjava_io_RandomAccessFile *this)
{
    Classjava_io_RandomAccessFile *fileptr = unhand(this);
    Classjava_io_FileDescriptor *thisptr = unhand(fileptr->fd);
    off_t cur = 0;

    if ((cur = sysLseekFD(thisptr, 0L, SEEK_CUR)) == -1) {
	SignalError(0, IO_EXCEPTION, 0);
    }
    return int2ll(cur);
}


void
java_io_RandomAccessFile_seek(Hjava_io_RandomAccessFile *this, int64_t pos) 
{
    Classjava_io_RandomAccessFile *fileptr = unhand(this);
    Classjava_io_FileDescriptor *thisptr = unhand(fileptr->fd);

    if (sysLseekFD(thisptr, ll2int(pos), SEEK_SET) == -1) {
	SignalError(0, IO_EXCEPTION, 0);
    }
}

int64_t
java_io_RandomAccessFile_length(Hjava_io_RandomAccessFile *this)
{
    Classjava_io_RandomAccessFile *fileptr = unhand(this);
    Classjava_io_FileDescriptor *thisptr = unhand(fileptr->fd);
    
    off_t cur = 0, end = 0;

    if ((cur = sysLseekFD(thisptr, 0L, SEEK_CUR)) == -1) {
	SignalError(0, IO_EXCEPTION, 0);
    } else if ((end = sysLseekFD(thisptr, 0L, SEEK_END)) == -1) {
	SignalError(0, IO_EXCEPTION, 0);
    } else if (sysLseekFD(thisptr, cur, SEEK_SET) == -1) {
	SignalError(0, IO_EXCEPTION, 0);
    } 
    return int2ll(end);
}

void
java_io_RandomAccessFile_close(Hjava_io_RandomAccessFile *this) 
{
    Classjava_io_RandomAccessFile *thisptr = unhand(this);
    Classjava_io_FileDescriptor *fdptr = unhand(thisptr->fd);

    if (fdptr == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "null FileDescriptor");
	return;
    }

    sysCloseFD(fdptr);
}

