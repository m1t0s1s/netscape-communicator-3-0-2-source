#include "prtypes.h"
#include "prlong.h"
#include "prmem.h"
#include "prprf.h"
#include "stdio.h"

#ifndef XP_MAC
#include "memory.h"
#endif

#ifdef XP_UNIX
#include "string.h"
#endif

#include "car_md.h"
#include "oobj.h"

#ifdef XP_MAC
#include "io_md.h"
#include <string.h>
#endif

/*
** @(#)car_md.c
**
** CAR file reader code
**
** Author: Kipp E.B. Hickman
*/

#define SWAPB(a,b) ((t) = (a), (a) = (b), (b) = (t))

static PRBool ReadLong(CARFile *f, int64 *ip)
{
    union {
	unsigned char c[8];
	int64 ll;
    } u;

    if (fread(u.c, 1, 8, f->file) != 8) {
	f->ioerror = 1;
	return PR_FALSE;
    }
#ifdef IS_LITTLE_ENDIAN
    {
	unsigned char t;
	SWAPB(u.c[0], u.c[7]);
	SWAPB(u.c[1], u.c[6]);
	SWAPB(u.c[2], u.c[5]);
	SWAPB(u.c[3], u.c[4]);
    }
#endif
    *ip = u.ll;
    return PR_TRUE;
}

static PRBool ReadInt(CARFile *f, int32 *ip)
{
    union {
	unsigned char c[4];
	int32 i;
    } u;

    if (fread(u.c, 1, 4, f->file) != 4) {
	f->ioerror = 1;
	return PR_FALSE;
    }
#ifdef IS_LITTLE_ENDIAN
    {
	unsigned char t;
	SWAPB(u.c[0], u.c[3]);
	SWAPB(u.c[1], u.c[2]);
    }
#endif
    *ip = u.i;
    return PR_TRUE;
}

/*
** Hash a utf string. The identical hash function is folded into the
** ReadUTF code below.
*/
static uint32 HashUTF(uint8 *s)
{
    uint32 hash;

    hash = 0;
    for (;;) {
	int c = *s++;
	hash = (hash >> 28) ^ (hash << 4) ^ c;
	if (c == 0)
	    break;
    }
    return hash;
}

/*
** Read in a null terminated utf encoded name. Don't de-encoded it
** because we will be comparing utf names against utf names. While we are
** doing the i/o, compute a hash of the name for faster searching.
*/
static PRBool ReadUTF(CARFile *f, uint8 **ip, uint32 *hp)
{
    uint8 b[32];
    uint8 *bp, *rv;
    uint8 bix, blen;
    uint32 hash;

    bix = 0;
    blen = sizeof(b);
    bp = b;
    hash = 0;
    for (;;) {
	int c = getc(f->file);
	if (c == EOF) {
	    f->ioerror = 1;
	    if (bp != b) {
		free(bp);
	    }
	    return PR_FALSE;
	}
	if (bix == blen) {
	    /* Grow bp */
	    blen *= 2;
	    if (bp == b) {
		bp = (uint8*) malloc(blen);
		if (!bp) {
		    return PR_FALSE;
		}
		memcpy(bp, b, sizeof(b));
	    } else {
		bp = (uint8*) realloc(bp, blen);
		if (!bp) {
		    return PR_FALSE;
		}
	    }
	}
	bp[bix++] = c;
	hash = (hash >> 28) ^ (hash << 4) ^ c;
	if (c == 0)
	    break;
    }

    rv = (uint8*) strdup((char*) bp);
    if (bp != b) {
	free(bp);
    }
    if (!rv) {
	return PR_FALSE;
    }
    *ip = rv;
    *hp = hash;
    return PR_TRUE;
}

/*
** Read the next TOCEntry from the car file
*/
static PRBool ReadTOCEntry(CARFile *f, TOCEntry *te)
{
    if (!ReadLong(f, &te->lastModified) ||
	!ReadLong(f, &te->offset) ||
	!ReadLong(f, &te->length) ||
	!ReadUTF(f, &te->name, &te->nameHash)) {
	return PR_FALSE;
    }
    return PR_TRUE;
}

/*
** Read the entire TOC
*/
static PRBool ReadTOC(CARFile *f)
{
    int32 i, n;
    TOCEntry *te;
    TOCEntry **tep;
    uint32 bucket;

    n = f->entries;
    te = f->toc;
    for (i = 0; i < n; i++, te++) {
	if (!ReadTOCEntry(f, te))
	    return PR_FALSE;

	/* Install entry in the hash buckets */
	bucket = te->nameHash & (f->numBuckets - 1);
	tep = &f->buckets[bucket];
	te->next = *tep;
	*tep = te;
    }
    return PR_TRUE;
}

/*
** Read in a CAR file. Actually all we read is the header and the TOC
*/
static PRBool ReadCar(CARFile *f)
{
    int32 magic;
    int32 version;
    int64 tocsize;
    int32 numBuckets;

    if (!ReadInt(f, &magic) || (magic != CAR_MAGIC) ||
	!ReadInt(f, &version) || (version != CAR_VERSION) ||
	!ReadInt(f, &f->entries) || !ReadLong(f, &tocsize)) {
	return PR_FALSE;
    }
    f->toc = (TOCEntry*) calloc(sizeof(TOCEntry), (size_t)f->entries);
    if (!f->toc) {
	return PR_FALSE;
    }
    numBuckets = PR_CeilingLog2(f->entries >> 1);
    if (!numBuckets) numBuckets = 1;
    f->numBuckets = numBuckets;
    f->buckets = (TOCEntry**) calloc(sizeof(TOCEntry*), (size_t)numBuckets);
    if (!f->buckets) {
	return PR_FALSE;
    }
    if (!ReadTOC(f)) {
	return PR_FALSE;
    }
    return PR_TRUE;
}

/*
** Shut de door
*/
static void CloseCar(CARFile *f)
{
    if (f) {
	if (f->file) {
	    fclose(f->file);
	    f->file = 0;
	}
    }
}

void CAR_Destroy(CARFile *f)
{
    if (f) {
	CloseCar(f);
	if (f->name) {
	    free(f->name);
	    f->name = 0;
	}
	if (f->buckets) {
	    free(f->buckets);
	    f->buckets = 0;
	}
	if (f->toc) {
	    TOCEntry *tp = f->toc;
	    TOCEntry *etp = tp + f->entries;
	    for (; tp < etp; tp++) {
		if (tp->name) {
		    free(tp->name);
		    tp->name = 0;
		}
	    }
	    free(f->toc);
	    f->toc = 0;
	}
	free(f);
    }
}

/*
** Hop on in
*/
CARFile *CAR_Open(const char *carname)
{
    FILE *fp;
    CARFile *f;

    fp = fopen(carname, "rb");
    if (!fp) return 0;
    f = (CARFile*) calloc(1, sizeof(CARFile));
    if (!f) {
	fclose(fp);
	return 0;
    }
    f->name = strdup(carname);
    f->file = fp;
    if (!ReadCar(f)) {
	CAR_Destroy(f);
	f = 0;
    }
    return f;
}

/*
** Look in the car for a given class name (look under the seats if we have
** to :-)
*/
TOCEntry **CAR_Lookup(CARFile *f, uint8 *name)
{
    uint32 hash = HashUTF(name);
    uint32 bucket = hash & (f->numBuckets - 1);
    TOCEntry **tep = &f->buckets[bucket];
    TOCEntry *te;

    while ((te = *tep) != 0) {
	if (te->nameHash == hash) {
	    if (strcmp((char*) te->name, (char*) name) == 0) {
		return tep;
	    }
	}
	tep = &te->next;
    }
    return 0;
}
