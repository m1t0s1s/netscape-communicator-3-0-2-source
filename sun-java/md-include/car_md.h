#ifndef sun_java_car_md_h___
#define sun_java_car_md_h___

#include <stdio.h>
#include "prtypes.h"

#define CAR_MAGIC	0xBEBACAFF
#define CAR_VERSION	0x00000002

typedef struct CARFileStr CARFile;
typedef struct TOCEntryStr TOCEntry;

struct CARFileStr {
    FILE *file;
    char *name;
    int ioerror;
    int32 entries;
    TOCEntry *toc;
    int32 numBuckets;
    TOCEntry **buckets;
};

struct TOCEntryStr {
    int64 lastModified;
    int64 offset;
    int64 length;
    uint8 *name;
    uint32 nameHash;
    TOCEntry *next;		/* hash bucket linkage */
};

/*
** Open a car file up. This reads in the table of contents and prepares
** the CARFile for lookup's. Returns NULL if there is some sort of
** problem, which includes file not found, out of memory and a damaged
** car file.
*/
CARFile *CAR_Open(const char *carname);

/*
** Destroy a car. f is free'd as well as anything it refers to.
*/
extern void CAR_Destroy(CARFile *f);

/*
** Look for "name" in the car file. Returns zero for failure, otherwise
** returns a pointer to a pointer to the TOCEntry that contains name.
*/
extern TOCEntry **CAR_Lookup(CARFile *f, uint8 *name);

#endif /* sun_java_car_md_h___ */
