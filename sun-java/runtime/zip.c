/*
 * @(#)zip.c	1.5 95/10/22 David Connelly
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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
 * Routines for reading "zip" file format. Currently, there is no support
 * for compressed or encrypted zip file members.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "sys_api.h"
#include "io_md.h"
#include "bool.h"
#include "zip.h"

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/*
 * Header signatures
 */
#define SIGSIZ 4
#define LOCSIG "PK\003\004"
#define CENSIG "PK\001\002"
#define ENDSIG "PK\005\006"

/*
 * Header sizes including signatures
 */
#define LOCHDRSIZ 30
#define CENHDRSIZ 46
#define ENDHDRSIZ 22

/*
 * Header field access macros
 */
#define CH(b, n) ((unsigned short)(((unsigned char *)(b))[n]))
#define SH(b, n) ((unsigned long)(CH(b, n) | (CH(b, n+1) << 8)))
#define LG(b, n) (SH(b, n) | (SH(b, n+2) << 16))

/*
 * Macros for getting local file header (LOC) fields
 */
#define LOCFLG(b) SH(b, 6)	    /* encrypt flags */
#define LOCHOW(b) SH(b, 8)	    /* compression method */
#define LOCCRC(b) LG(b, 14)	    /* uncompressed file crc-32 value */
#define LOCSIZ(b) LG(b, 18)	    /* compressed size */
#define LOCLEN(b) LG(b, 22)	    /* uncompressed size */
#define LOCNAM(b) SH(b, 26)	    /* filename size */
#define LOCEXT(b) SH(b, 28)	    /* extra field size */

/*
 * Macros for getting central directory header (CEN) fields
 */
#define CENTIM(b) LG(b, 12)	    /* file modification time (DOS format) */
#define CENSIZ(b) LG(b, 20)	    /* compressed size */
#define CENLEN(b) LG(b, 24)	    /* uncompressed size */
#define CENNAM(b) SH(b, 28)	    /* length of filename */
#define CENEXT(b) SH(b, 30)	    /* length of extra field */
#define CENCOM(b) SH(b, 32)	    /* file comment length */
#define CENOFF(b) LG(b, 42)	    /* offset of local header */

/*
 * Macros for getting end of central directory header (END) fields
 */
#define ENDSUB(b) SH(b, 8)	    /* number of entries on this disk */
#define ENDTOT(b) SH(b, 10)	    /* total number of entries */
#define ENDSIZ(b) LG(b, 12)	    /* central directory size */
#define ENDOFF(b) LG(b, 16)	    /* central directory offset */
#define ENDCOM(b) SH(b, 20)	    /* size of zip file comment */

static void
errmsg(const char *msg)
{
#ifdef DEBUG
    sysWrite(2, msg, strlen(msg));
#endif
}

static void
ziperr(zip_t *zip, const char *msg)
{
#ifdef DEBUG
    errmsg("Zip Error: ");
    errmsg(zip->fn);
    errmsg(": ");
    errmsg(msg);
    errmsg("\n");
#endif
}

/*
 * Read exactly 'len' bytes of data into 'buf'. Return TRUE if all bytes
 * could be read, otherwise return FALSE.
 */
static bool_t
readFully(int fd, void *buf, int32_t len)
{
    char *bp = buf;

    while (len > 0) {
	int32_t rqst;
	int32_t n;
	
	rqst = len;
#if defined(XP_PC) && !defined(_WIN32)
    if (rqst > 32767)
    	rqst = 32767;
#endif
	n = sysRead(fd, bp, rqst);
	if (n <= 0) {
	    return FALSE;
	}
	bp += n;
	len -= n;
    }
    return TRUE;
}

#define INBUFSIZ 64

/*
 * Find end of central directory (END) header in zip file and set the file
 * pointer to start of header. Return FALSE if the END header could not be
 * found, otherwise return TRUE.
 */
static bool_t
findEnd(zip_t *zip)
{
    char buf[INBUFSIZ+SIGSIZ], *bp;
    int32_t len, off, mark;

    /* Need to search backwards from end of file */
    if ((len = lseek(zip->fd, 0, SEEK_END)) == -1)
	return FALSE;
    
    /*
     * Set limit on how far back we need to search. The END header must be
     * located within the last 64K bytes of the file.
     */
    mark = max(0, len - 0xffff);
    /*
     * Search backwards INBUFSIZ bytes at a time from end of file stopping
     * when the END header signature has been found. Since the signature
     * may straddle a buffer boundary, we need to stash the first SIGSIZ-1
     * bytes of the previous record at the tail of the current record so
     * that the search can overlap.
     */
    memset(buf, 0, SIGSIZ);
    for (off = len; off > mark; ) {
	long n = min(off - mark, INBUFSIZ);
	memcpy(buf + n, buf, SIGSIZ);
	if (lseek(zip->fd, off -= n, SEEK_SET) == -1) {
	    return FALSE;
	}
	if (!readFully(zip->fd, buf, n)) {
	    ziperr(zip, "Fatal read error while searching for END record");
	    return FALSE;
	}
	for (bp = buf + n - 1; bp >= buf; --bp) {
	    if (strncmp(bp, ENDSIG, SIGSIZ) == 0) {
		char endbuf[ENDHDRSIZ];
		long endoff = off + (bp - buf);
		if (len - endoff < ENDHDRSIZ) {
		    continue;
		}
		if (lseek(zip->fd, endoff, SEEK_SET) == -1) {
		}
		if (!readFully(zip->fd, endbuf, ENDHDRSIZ)) {
		    ziperr(zip, "Read error while searching for END record");
		    return FALSE;
		}
		if (endoff + ENDHDRSIZ + ENDCOM(endbuf) != len) {
		    continue;
		}
		if (lseek(zip->fd, endoff, SEEK_SET) == -1) {
		    return FALSE;
		}
		zip->endoff = endoff;
		return TRUE;
	    }
	}
    }
    return FALSE;
}

/*
 * Convert DOS time to UNIX time
 */
static time_t
unixtime(int dostime)
{
    struct tm tm;

    tm.tm_sec = (dostime << 1) & 0x3e;
    tm.tm_min = (dostime >> 5) & 0x3f;
    tm.tm_hour = (dostime >> 11) & 0x1f;
    tm.tm_mday = (dostime >> 16) & 0x1f;
    tm.tm_mon = ((dostime >> 21) & 0xf) - 1;
    tm.tm_year = ((dostime >> 25) & 0x7f) + 1980;
    return mktime(&tm);
}

static int
direlcmp(const void *d1, const void *d2)
{

    return strcmp(((direl_t *)d1)->fn, ((direl_t *)d2)->fn);
}

/*
 * Initialize zip file reader, read in central directory and construct the
 * lookup table for locating zip file members.
 */
static bool_t
initReader(zip_t *zip)
{
    char endbuf[ENDHDRSIZ];
    char *cenbuf, *cp;
    uint32_t i, cenlen;

    /* Seek to END header */
    if (!findEnd(zip)) {
	ziperr(zip, "Unable to locate end-of-central-directory record");
	return FALSE;
    }
    /* Read END header */
    if (!readFully(zip->fd, endbuf, ENDHDRSIZ)) {
	ziperr(zip, "Fatal error while reading END header");
	return FALSE;
    }
    /* Verify END signature */
    if (strncmp(endbuf, ENDSIG, SIGSIZ) != 0) {
	ziperr(zip, "Invalid END signature");
	return FALSE;
    }
    /* Get offset of central directory */
    zip->cenoff = ENDOFF(endbuf);
    /* Get length of central directory */
    cenlen = ENDSIZ(endbuf);
    /* Check offset and length */
    if (zip->cenoff + cenlen != zip->endoff) {
	ziperr(zip, "Invalid end-of-central-directory header");
	return FALSE;
    }
    /* Get total number of central directory entries */
    zip->nel = ENDTOT(endbuf);
    /* Check entry count */
    if (zip->nel * CENHDRSIZ > cenlen) {
	ziperr(zip, "Invalid end-of-central-directory header");
	return FALSE;
    }
    /* Verify that zip file contains only one drive */
    if (ENDSUB(endbuf) != zip->nel) {
	ziperr(zip, "Cannot contain more than one drive");
	return FALSE;
    }
    /* Seek to first CEN header */
    if (lseek(zip->fd, zip->cenoff, SEEK_SET) == -1) {
	return FALSE;
    }
    /* Allocate file lookup table */
    if ((zip->dir = sysMalloc((size_t)(zip->nel * sizeof(direl_t)))) == 0) {
	ziperr(zip, "Out of memory allocating lookup table");
	return FALSE;
    }
    /* Allocate temporary buffer for contents of central directory */
    if ((cenbuf = sysMalloc((size_t)cenlen)) == 0) {
	ziperr(zip, "Out of memory allocating central directory buffer");
	return FALSE;
    }
    /* Read central directory */
    if (!readFully(zip->fd, cenbuf, cenlen)) {
	ziperr(zip, "Fatal error while reading central directory");
	sysFree(cenbuf);
	return FALSE;
    }
    /* Scan each header in central directory */
    for (i = 0, cp = cenbuf; i < zip->nel; i++) {
	direl_t *dp = &zip->dir[i];
	uint32_t n;
	/* Verify signature of CEN header */
	if (strncmp(cp, CENSIG, SIGSIZ) != 0) {
	    ziperr(zip, "Invalid central directory header signature");
	    sysFree(cenbuf);
	    return FALSE;
	}
	/* Get file name */
	n = CENNAM(cp);
	if ((dp->fn = sysMalloc((size_t)n + 1)) == 0) {
	    ziperr(zip, "Out of memory reading CEN header file name");
	    sysFree(cenbuf);
	    return FALSE;
	}
	strncpy(dp->fn, cp + CENHDRSIZ, (size_t) n);
	dp->fn[n] = '\0';
	/* Get offset of LOC header */
	dp->off = CENOFF(cp);
	/* Get uncompressed file size */
	dp->len = CENLEN(cp);
	/* Check offset and size */
	if (dp->off + CENSIZ(cp) > zip->cenoff) {
	    ziperr(zip, "Invalid CEN header");
	    return FALSE;
	}
	/* Get file modification time */
	dp->mod = (int) CENTIM(cp);
	/* Skip to next CEN header */
	cp += CENHDRSIZ + CENNAM(cp) + CENEXT(cp) + CENCOM(cp);
	if (cp > cenbuf + cenlen) {
	    ziperr(zip, "Invalid CEN header");
	    return FALSE;
	}
    }
    /* Free temporary buffer */
    sysFree(cenbuf);
    /* Sort directory elements by name */
    qsort(zip->dir, (size_t) zip->nel, sizeof(direl_t), direlcmp);
    return TRUE;
}

/*
 * Open zip_t file and initialize file lookup table.
 */
zip_t *
zip_open(const char *fn)
{
    int fd = sysOpen(fn, O_RDONLY, 0);
    zip_t *zip;

    if (fd == -1) {
	return 0;
    }
    if ((zip = sysMalloc(sizeof(zip_t))) == 0) {
	errmsg("Out of memory");
	return 0;
    }
    if ((zip->fn = strdup(fn)) == 0) {
	errmsg("Out of memory");
	return 0;
    }
    zip->fd = fd;
    if (!initReader(zip)) {
	sysFree(zip->fn);
	sysFree(zip);
	sysClose(fd);
	return 0;
    }
    return zip;
}

/*
 * Close zip file and free lookup table
 */
void
zip_close(zip_t *zip)
{
    uint32_t i;

    sysFree(zip->fn);
    close(zip->fd);
    for (i = 0; i < zip->nel; i++) {
	sysFree(zip->dir[i].fn);
    }
    sysFree(zip->dir);
    sysFree(zip);
}

static direl_t *
lookup(zip_t *zip, const char *fn)
{
    direl_t key;
    key.fn = (char *)fn;
    return bsearch(&key, zip->dir, (size_t) zip->nel, sizeof(direl_t), direlcmp);
}

/*
 * Return file status for zip_t file member
 */
bool_t
zip_stat(zip_t *zip, const char *fn, struct stat *sbuf)
{
    direl_t *dp = lookup(zip, fn);

    if (dp == 0) {
	return FALSE;
    }
    memset(sbuf, 0, sizeof(struct stat));
    sbuf->st_mode = 444;
    sbuf->st_size = dp->len;
    sbuf->st_mtime = unixtime(dp->mod);
    sbuf->st_atime = sbuf->st_mtime;
    sbuf->st_ctime = sbuf->st_mtime;
    return TRUE;
}

/*
 * Read contents of zip file member into buffer 'buf'. If the size of the
 * data should exceed 'len' then nothing is read and FALSE is returned.
 */
bool_t
zip_get(zip_t *zip, const char *fn, void *buf, int32_t len)
{
    char locbuf[LOCHDRSIZ];
    long off;
    direl_t *dp = lookup(zip, fn);

    /* Look up file member */
    if (dp == 0) {
	return FALSE;
    }
    /* Check buffer length */
    if (len != (int32_t) dp->len) {
	return FALSE;
    }
    /* Seek to beginning of LOC header */
    if (lseek(zip->fd, dp->off, SEEK_SET) == -1) {
	return FALSE;
    }
    /* Read LOC header */
    if (!readFully(zip->fd, locbuf, LOCHDRSIZ)) {
	ziperr(zip, "Fatal error while reading LOC header");
	return FALSE;
    }
    /* Verify signature */
    if (strncmp(locbuf, LOCSIG, SIGSIZ) != 0) {
	ziperr(zip, "Invalid LOC header signature");
	return FALSE;
    }
    /* Make sure file is not compressed */
    if (LOCHOW(locbuf) != 0) {
	ziperr(zip, "Member is compressed");
	return FALSE;
    }
    /* Make sure file is not encrypted */
    if ((LOCFLG(locbuf) & 1) == 1) {
	ziperr(zip, "Member is encrypted");
	return FALSE;
    }
    /* Get offset of file data */
    off = dp->off + LOCHDRSIZ + LOCNAM(locbuf) + LOCEXT(locbuf);
    if (off + dp->len > zip->cenoff) {
	ziperr(zip, "Invalid LOC header");
	return FALSE;
    }
    /* Seek to file data */
    if (lseek(zip->fd, off, SEEK_SET) == -1) {
	return FALSE;
    }
    /* Read file data */
    if (!readFully(zip->fd, buf, dp->len)) {
	ziperr(zip, "Fatal error while reading LOC data");
	return FALSE;
    }
    return TRUE;
}
