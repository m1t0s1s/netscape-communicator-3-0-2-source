
#ifndef __UGEN__
#define __UGEN__

#include "xp_core.h"


/* =================================================
					uShiftTable
================================================= */
/*=================================================================================

=================================================================================*/

enum {
	u1ByteCharset = 0,
	u2BytesCharset,
	uMultibytesCharset,
	u2BytesGRCharset,
	u2BytesGRPrefix8FCharset,
	u2BytesGRPrefix8EA2Charset,
	uNumOfCharsetType
};
/*=================================================================================

=================================================================================*/

enum {
	u1ByteChar			= 0,
	u2BytesChar,
	u2BytesGRChar,
	u1BytePrefix8EChar,		/* Used by JIS0201 GR in EUC_JP */
	uNumOfCharType
};
/*=================================================================================

=================================================================================*/
typedef struct  {
	unsigned char MinHB;
	unsigned char MinLB;
	unsigned char MaxHB;
	unsigned char MaxLB;
} uShiftOut;
/*=================================================================================

=================================================================================*/
typedef struct  {
	unsigned char Min;
	unsigned char Max;
} uShiftIn;
/*=================================================================================

=================================================================================*/
typedef struct  {
	unsigned char   classID;
	unsigned char   reserveLen;
	uShiftIn	shiftin;
	uShiftOut	shiftout;
} uShiftCell;
/*=================================================================================

=================================================================================*/
typedef struct  {
	int16		numOfItem;
	int16		classID;
	uShiftCell	shiftcell[1];
} uShiftTable;
/*=================================================================================

=================================================================================*/

#define MAXINTERCSID 4
typedef struct StrRangeMap StrRangeMap;
struct StrRangeMap {
	uint16 intercsid;
	unsigned char min;
	unsigned char max;	
};
typedef struct UnicodeTableSet UnicodeTableSet;
struct UnicodeTableSet {
 uint16 maincsid;
 StrRangeMap range[MAXINTERCSID];
};
#endif
