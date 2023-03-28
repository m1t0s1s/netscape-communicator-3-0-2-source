#include "ugen.h"
#include "unicpriv.h"
#include "npc.h"
#ifndef XP_MAC
#	include "nspr/prcpucfg.h"	/* For IS_LITTLE_ENDIAN */
#else
#	include "prcpucfg.h"
#endif
/*
	GenTableData.c

*/
/*=========================================================================================
		Generator Table
=========================================================================================*/
#define PACK(h,l)		(int16)(( (h) << 8) | (l))

#if	defined(IS_LITTLE_ENDIAN)
#define ShiftCell(sub,len,min,max,minh,minl,maxh,maxl)	\
		PACK(len,sub), PACK(max,min), PACK(minl,minh), PACK(maxl,maxh)
#else
#define ShiftCell(sub,len,min,max,minh,minl,maxh,maxl)	\
		PACK(sub,len), PACK(min, max), PACK(minh,minl), PACK(maxh,maxl)
#endif
/*-----------------------------------------------------------------------------------
		ShiftTable for single byte encoding 
-----------------------------------------------------------------------------------*/
static int16 sbShiftT[] = 	{ 
	0, u1ByteCharset, 
	ShiftCell(0,0,0,0,0,0,0,0) 
};
/*-----------------------------------------------------------------------------------
		ShiftTable for two byte encoding 
-----------------------------------------------------------------------------------*/
static int16 tbShiftT[] = 	{ 
	0, u2BytesCharset, 
	ShiftCell(0,0,0,0,0,0,0,0) 
};
/*-----------------------------------------------------------------------------------
		ShiftTable for two byte encoding 
-----------------------------------------------------------------------------------*/
static int16 tbGRShiftT[] = 	{ 
	0, u2BytesGRCharset, 
	ShiftCell(0,0,0,0,0,0,0,0) 
};
/*-----------------------------------------------------------------------------------
		ShiftTable for shift jis encoding 
-----------------------------------------------------------------------------------*/
static int16 sjisShiftT[] = { 
	4, uMultibytesCharset, 	
	ShiftCell(u1ByteChar, 	1, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x7F),
	ShiftCell(u1ByteChar, 	1, 0xA1, 0xDF, 0x00, 0xA1, 0x00, 0xDF),
	ShiftCell(u2BytesChar,	2, 0x81, 0x9F, 0x81, 0x40, 0x9F, 0xFC),
	ShiftCell(u2BytesChar, 	2, 0xE0, 0xFC, 0xE0, 0x40, 0xFC, 0xFC)
};
/*-----------------------------------------------------------------------------------
		ShiftTable for JIS0212 in EUCJP encoding 
-----------------------------------------------------------------------------------*/
static int16 x0212ShiftT[] = 	{ 
	0, u2BytesGRPrefix8FCharset, 
	ShiftCell(0,0,0,0,0,0,0,0) 
};
/*-----------------------------------------------------------------------------------
		ShiftTable for CNS11643-2 in EUC_TW encoding 
-----------------------------------------------------------------------------------*/
static int16 cns2ShiftT[] = 	{ 
	0, u2BytesGRPrefix8EA2Charset, 
	ShiftCell(0,0,0,0,0,0,0,0) 
};
/*-----------------------------------------------------------------------------------
		ShiftTable for big5 encoding 
-----------------------------------------------------------------------------------*/
static int16 big5ShiftT[] = { 
	2, uMultibytesCharset, 	
	ShiftCell(u1ByteChar, 	1, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x7F),
	ShiftCell(u2BytesChar, 	2, 0xA1, 0xFE, 0xA1, 0x40, 0xFE, 0xFE)
};
/*-----------------------------------------------------------------------------------
		ShiftTable for jis0201 for euc_jp encoding 
-----------------------------------------------------------------------------------*/
static int16 x0201ShiftT[] = { 
	2, uMultibytesCharset, 	
	ShiftCell(u1ByteChar, 		1, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x7F),
	ShiftCell(u1BytePrefix8EChar, 2, 0x8E, 0x8E, 0x00, 0xA1, 0x00, 0xDF)
};
/*-----------------------------------------------------------------------------------
		Array of ShiftTable Pointer
-----------------------------------------------------------------------------------*/
/* This table is used for npc and unicode to unknow encoding conversion */
/* for those font csid, it do not shift GR/GL */
static	int16*	npcShiftTable[MAXCSIDINNPC] =
{
	sbShiftT,	sbShiftT,	sbShiftT,	0,				sjisShiftT,	0,			sbShiftT,	big5ShiftT,	
	tbGRShiftT,	0,			sbShiftT,	sbShiftT,		tbGRShiftT,	0,			sbShiftT,	sbShiftT,
	sbShiftT,	sbShiftT,	sbShiftT,	sbShiftT,		sbShiftT,	sbShiftT,	sbShiftT,	sbShiftT,
	tbShiftT,	tbShiftT,	tbShiftT,	sbShiftT,		tbShiftT,	sbShiftT,	tbShiftT,	tbShiftT,
	0,			0,			0,			0,				0,			big5ShiftT,	0,			sbShiftT,
	sbShiftT,	sbShiftT,	sbShiftT,	sbShiftT,		sbShiftT,	sbShiftT,	sbShiftT,			0,
	0,			0,			0,			0,				0,			0,			0,			0,
	0,			0,			0,			0,				0,			0,			sbShiftT,	0
};

/* This table is used for unicode to single encoding conversion */
/* for those font csid, it always shift GR/GL */
static	int16*	strShiftTable[MAXCSIDINNPC] =
{
	sbShiftT,	sbShiftT,	sbShiftT,	0,				sjisShiftT,	0,			sbShiftT,	big5ShiftT,	
	tbGRShiftT,	0,			sbShiftT,	sbShiftT,		tbGRShiftT,	0,			sbShiftT,	sbShiftT,
	sbShiftT,	sbShiftT,	sbShiftT,	sbShiftT,		sbShiftT,	sbShiftT,	sbShiftT,	sbShiftT,
	tbGRShiftT,	cns2ShiftT,	tbGRShiftT,	x0201ShiftT,	tbGRShiftT,	sbShiftT,	x0212ShiftT,tbGRShiftT,
	0,			0,			0,			0,				0,			big5ShiftT,	0,			sbShiftT,
	sbShiftT,	sbShiftT,	sbShiftT,	sbShiftT,		sbShiftT,	sbShiftT,	sbShiftT,			0,
	0,			0,			0,			0,				0,			0,			0,			0,
	0,			0,			0,			0,				0,			0,			0,			0
};

static UnicodeTableSet unicodetableset[] =
{
#ifdef XP_MAC
	{	CS_MAC_ROMAN, {
		{CS_MAC_ROMAN,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_MAC_CE,	{		
		{CS_MAC_CE,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_SJIS,	{		
		{CS_SJIS,0x00,0xFF},		
		{CS_DEFAULT,0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_BIG5,	{		
		{CS_BIG5,0x81,0xFC},		
		{CS_MAC_ROMAN,0x00,0x7E},		
		{CS_DEFAULT,0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_GB_8BIT,	{		
		{CS_GB_8BIT,0xA1,0xFE},		
		{CS_MAC_ROMAN,0x00,0x7E},		
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_KSC_8BIT,	{		
		{CS_KSC_8BIT,0xA1,0xFE},		
		{CS_MAC_ROMAN,0x00,0x7E},		
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_MAC_CYRILLIC, {
		{CS_MAC_CYRILLIC,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_MAC_GREEK, {
		{CS_MAC_GREEK,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_MAC_TURKISH, {
		{CS_MAC_TURKISH,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_DEFAULT,	{		
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00}
	}}
#endif
#ifdef XP_WIN
	{	CS_LATIN1, {
		{CS_LATIN1,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_CP_1250,	{		
		{CS_CP_1250,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_LATIN2,	{		/* this should be replaced by CS_CP_1250 */
		{CS_LATIN2,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_SJIS,	{		
		{CS_SJIS,0x00,0xFF},		
		{CS_DEFAULT,0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_BIG5,	{		
		{CS_BIG5,0x81,0xFC},		
		{CS_LATIN1,0x00,0x7E},		
		{CS_DEFAULT,0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_GB_8BIT,	{		
		{CS_GB_8BIT,0xA1,0xFE},		
		{CS_LATIN1,0x00,0x7E},		
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_KSC_8BIT,	{		
		{CS_KSC_8BIT,0xA1,0xFE},		
		{CS_LATIN1,0x00,0x7E},		
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_CP_1251, {
		{CS_CP_1251,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_8859_7, {
		{CS_8859_7,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_8859_9, {
		{CS_8859_9,		0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_DEFAULT,	{		
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00}
	}}
#endif
#ifdef XP_UNIX
	{	CS_LATIN1, {
		{CS_LATIN1,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_LATIN2,	{		
		{CS_LATIN2,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_SJIS,	{		
		{CS_SJIS,0x00,0xFF},		
		{CS_DEFAULT,0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_EUCJP,	{		
		{CS_JISX0208,0xA1,0xFE},		
		{CS_JISX0201,0x20,0x7E},		
		{CS_JISX0201,0x8E,0x8E},		
		{CS_JISX0212,0x8F,0x8F}
	}},
	{	CS_BIG5,	{		
		{CS_X_BIG5,0x81,0xFC},		
		{CS_LATIN1,0x00,0x7E},		
		{CS_DEFAULT,0xFF,0x00},		
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_CNS_8BIT,	{		
		{CS_CNS11643_1,0xA1,0xFE},		
		{CS_CNS11643_2,0x8E,0x8E},		
		{CS_LATIN1,0x00,0x7E},		
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_GB_8BIT,	{		
		{CS_GB2312,0xA1,0xFE},		
		{CS_LATIN1,0x00,0x7E},		
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_KSC_8BIT,	{		
		{CS_KSC5601,0xA1,0xFE},		
		{CS_LATIN1,0x00,0x7E},		
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00}
	}},
	{	CS_8859_5, {
		{CS_8859_5,		0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_8859_7, {
		{CS_8859_7,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_8859_9, {
		{CS_8859_9,	0x00,0xFF},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00},		
		{CS_DEFAULT,	0xFF,0x00} 
	}},
	{	CS_DEFAULT,	{		
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00},
		{CS_DEFAULT,0xFF,0x00}
	}}
#endif

};

UnicodeTableSet* GetUnicodeTableSet(uint16 csid)
{
	int i;
	for(i=0;unicodetableset[i].maincsid != CS_DEFAULT;i++)
	{
		if(unicodetableset[i].maincsid == csid)
			return &(unicodetableset[i]);	 
	}
	XP_ASSERT(TRUE);
	return NULL;
}

/*-----------------------------------------------------------------------------------
		Public Function
-----------------------------------------------------------------------------------*/
uShiftTable* GetShiftTableFromCsid(uint16 csid)
{
	return (uShiftTable*)(strShiftTable[csid & 0x3F]);
}
uShiftTable* InfoToShiftTable(unsigned char info)
{
	return (uShiftTable*)(npcShiftTable[info & (MAXCSIDINNPC - 1)]);
}
