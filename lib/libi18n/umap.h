#ifndef __UMAP__
#define __UMAP__

#define NOMAPPING 0xfffd

/* =================================================
					MapCellArray
================================================= */

enum {
	uFormat0Tag = 0,
	uFormat1Tag,
	uFormat2Tag,
	uNumFormatTag
};

typedef struct {
		uint16 srcBegin;		/* 2 byte	*/
		uint16 srcEnd;			/* 2 byte	*/
		uint16 destBegin;		/* 2 byte	*/
} uFormat0;

typedef struct {
		uint16 srcBegin;		/* 2 byte	*/
		uint16 srcEnd;			/* 2 byte	*/
		uint16	mappingOffset;	/* 2 byte	*/
} uFormat1;

typedef struct {
		uint16 srcBegin;		/* 2 byte	*/
		uint16 srcEnd;			/* 2 byte	-waste	*/
		uint16 destBegin;		/* 2 byte	*/
} uFormat2;

typedef struct  {
	union {
		uFormat0	format0;
		uFormat1	format1;
		uFormat2	format2;
	} fmt;
} uMapCell;

/* =================================================
					uTable 
================================================= */
typedef struct  {
	uint16 		itemOfList;
	uint16		offsetToFormatArray;
	uint16		offsetToMapCellArray;
	uint16		offsetToMappingTable;
	uint16		data[1];
} uTable;

#endif
