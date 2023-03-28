/* -*- Mode: C; tab-width: 4 -*- */
/*	fe_ccc.c	*/
/* Test harness code to be replaced by FE specific code	*/

/* #define USE_NPC 1  */

#include "intlpriv.h"

#ifndef STANDALONE_TEST
#include <stdio.h>
#include "xp.h"
#endif /* ! STANDALONE_TEST */

#ifdef XP_MAC
#include "resgui.h"
#endif

#ifdef USE_NPC
#include "npc.h"
#endif

#ifdef XP_UNIX
#ifdef DEBUG
#if 0
#define UNICODE_INIT_TEST	/* Turn this off unless testing unicode*/
#endif
#endif
#endif


/* for XP_GetString() */
#include "xpgetstr.h"
extern int XP_DOCINFO_1;
extern int XP_DOCINFO_2;
extern int XP_DOCINFO_3;
extern int XP_DOCINFO_4;
extern int MK_OUT_OF_MEMORY;


static unsigned char *
mz_b52b5(CCCDataObject *obj, const unsigned char *kscbuf, int32 kscbufsz);

static unsigned char *
mz_cns2cns(CCCDataObject *obj, const unsigned char *cnsbuf, int32 cnsbufsz);

static unsigned char *
mz_euc2euc(CCCDataObject *obj, const unsigned char *eucbuf, int32 eucbufsz);

static unsigned char *
mz_gb2gb(CCCDataObject *obj, const unsigned char *kscbuf, int32 kscbufsz);

static unsigned char *
mz_ksc2ksc(CCCDataObject *obj, const unsigned char *kscbuf, int32 kscbufsz);

static unsigned char *
mz_sjis2sjis(CCCDataObject *obj, const unsigned char *sjisbuf, int32 sjisbufsz);


static int haveBig5 = 0;


csinfo_t csinfo_tbl[] =
{
/* b = bytes; c = columns                                               */
/* charset ID    b c  range 1       b c  range 2       b c  range 3     */
{CS_SJIS,      {{2,2,{0x81,0x9f}}, {2,2,{0xe0,0xfc}}, {0,0,{0x00,0x00}}}},
{CS_EUCJP,     {{2,2,{0xa1,0xfe}}, {2,1,{0x8e,0x8e}}, {3,2,{0x8f,0x8f}}}},
{CS_KSC_8BIT,  {{2,2,{0xa1,0xfe}}, {0,0,{0x00,0x00}}, {0,0,{0x00,0x00}}}},
{CS_GB_8BIT,   {{2,2,{0xa1,0xfe}}, {0,0,{0x00,0x00}}, {0,0,{0x00,0x00}}}},
{CS_CNS_8BIT,  {{2,2,{0xa1,0xfe}}, {4,2,{0x8e,0x8e}}, {0,0,{0x00,0x00}}}},
{CS_BIG5,      {{2,2,{0xa1,0xfe}}, {0,0,{0x00,0x00}}, {0,0,{0x00,0x00}}}},
{CS_CNS11643_1,{{2,2,{0x21,0x7e}}, {0,0,{0x00,0x00}}, {0,0,{0x00,0x00}}}},
{CS_CNS11643_2,{{2,2,{0x21,0x7e}}, {0,0,{0x00,0x00}}, {0,0,{0x00,0x00}}}},
{CS_JISX0208,  {{2,2,{0x21,0x7e}}, {0,0,{0x00,0x00}}, {0,0,{0x00,0x00}}}},
{CS_KSC5601,   {{2,2,{0x21,0x7e}}, {0,0,{0x00,0x00}}, {0,0,{0x00,0x00}}}},
{CS_JISX0212,  {{2,2,{0x21,0x7e}}, {0,0,{0x00,0x00}}, {0,0,{0x00,0x00}}}},
{CS_GB2312,    {{2,2,{0x21,0x7e}}, {0,0,{0x00,0x00}}, {0,0,{0x00,0x00}}}},
{CS_UTF8,	   {{2,2,{0xC0, 0xDF}},{3,2,{0xE0,0xEF}},{0,0,{0x00,0x00}}}},
{0,            {{0,0,{0x00,0x00}}, {0,0,{0x00,0x00}}, {0,0,{0x00,0x00}}}}
};

			/* Charset names and aliases from RFC 1700. Names are case
			 * insenstive. Currently searches table linearly, so keep commonly used names at the beginning.
			 */
csname2id_t csname2id_tbl[] = {
											/* default if not specified */
			{"RESERVED", CS_DEFAULT},			/* or unknown charset		*/

			{"us-ascii", CS_ASCII},
			{"iso-8859-1", CS_LATIN1},
			{"iso-2022-jp", CS_JIS},
			{"iso-2022-jp-2", CS_JIS},		/* treat same as iso-2022-jp*/
			{"Shift_JIS", CS_SJIS},
			{"x-euc-jp", CS_EUCJP},
			{"jis_x0208-1983", CS_JISX0208},
			{"x-jisx0208-11", CS_JISX0208_11},
			{"jis_x0201", CS_JISX0201},
			{"jis_x0212-1990", CS_JISX0212},
			{"x-mac-roman", CS_MAC_ROMAN}, 
			{"iso-8859-2", CS_LATIN2},
			{"iso-8859-3", CS_8859_3},
			{"iso-8859-4", CS_8859_4},
			{"iso-8859-5", CS_8859_5},
			{"iso-8859-6", CS_8859_6},
			{"iso-8859-7", CS_8859_7},
			{"iso-8859-8", CS_8859_8},
			{"iso-8859-9", CS_8859_9},
			{"x-mac-ce", CS_MAC_CE},
			{"euc-kr", CS_KSC_8BIT},
			{"ks_c_5601-1987", CS_KSC5601},
			{"x-ksc5601-11", CS_KSC5601_11},
			{"gb2312", CS_GB_8BIT},
			{"gb_2312-80", CS_GB2312},
			{"x-gb2312-11", CS_GB2312_11},
			{"x-euc-tw", CS_CNS_8BIT},
			{"x-cns11643-1", CS_CNS11643_1},
			{"x-cns11643-2", CS_CNS11643_2},
			{"x-cns11643-1110", CS_CNS11643_1110},
			{"iso-2022-kr", CS_2022_KR},
			{"big5", CS_BIG5},
			{"x-x-big5", CS_X_BIG5},
			{"x-tis620", CS_TIS620},
			{"adobe-symbol-encoding", CS_SYMBOL},
			{"x-dingbats", CS_DINGBATS},
			{"x-dectech", CS_DECTECH},
			{"koi8-r", CS_KOI8_R},
/*			{"iso-8859-5-windows-latin-5", CS_CP_1251},	*/
			{"x-mac-cyrillic", CS_MAC_CYRILLIC}, 
			{"x-mac-greek", CS_MAC_GREEK}, 
			{"x-mac-turkish", CS_MAC_TURKISH}, 
			{"windows-1250", CS_CP_1250},
			{"windows-1251", CS_CP_1251}, /* cyrillic */

#ifdef NOTDEF /* for auto Japanese codeset detection. when we have time. */
			{"x-japan", CS_JAPAN},
#endif /* NOTDEF */
											/* aliases for us-ascii: */
			{"ansi_x3.4-1968", CS_ASCII},
			{"iso-ir-6", CS_ASCII},
			{"ansi_x3.4-1986", CS_ASCII},
			{"iso_646.irv:1991", CS_ASCII},
			{"ascii", CS_ASCII},
			{"iso646-us", CS_ASCII},
			{"us", CS_ASCII},
			{"ibm367", CS_ASCII},
			{"cp367", CS_ASCII},
			{"csASCII", CS_ASCII},
											/* aliases for iso_8859-1:	*/
			{"latin1", CS_LATIN1},
			{"iso_8859-1", CS_LATIN1},
			{"iso_8859-1:1987", CS_LATIN1},
			{"iso-ir-100", CS_LATIN1},
			{"l1", CS_LATIN1},
			{"ibm819", CS_LATIN1},
			{"cp819", CS_LATIN1},
			
											/* aliases for ISO_8859-2:	*/
			{"latin2", CS_LATIN2},			
			{"iso_8859-2", CS_LATIN2},
			{"iso_8859-2:1987", CS_LATIN2},
			{"iso-ir-101", CS_LATIN2},
			{"l2", CS_LATIN2},											
											/* aliases for KS_C_5601-1987:	*/
			{"ks_c_5601-1987", CS_KSC5601},
			{"iso-ir-149", CS_KSC5601},
			{"ks_c_5601-1989", CS_KSC5601},
			{"ks_c_5601", CS_KSC5601},
			{"korean", CS_KSC5601},
			{"csKSC56011987", CS_KSC5601},
											/* aliases for iso-2022-kr:	*/
			{"csISO2022KR", CS_2022_KR},
											/* aliases for euc-kr:	*/
			{"csEUCKR", CS_KSC_8BIT},
											/* aliases for iso-2022-jp:	*/
			{"csISO2022JP", CS_JIS},
											/* aliases for iso-2022-jp-2:	*/
			{"csISO2022JP2", CS_JIS},
											/* aliases for GB_2312-80:	*/
			{"iso-ir-58", CS_GB2312},
			{"chinese", CS_GB2312},
			{"csISO58GB231280", CS_GB2312},
											/* aliases for gb2312:	*/
			{"csGB2312", CS_GB_8BIT},
											/* aliases for big5:	*/
			{"csBig5", CS_BIG5},
											/* aliases for iso-8859-7:	*/
			{"iso-ir-126", CS_8859_7},
			{"iso_8859-7", CS_8859_7},
			{"iso_8859-7:1987", CS_8859_7},
			{"elot_928", CS_8859_7},
			{"ecma-118", CS_8859_7},
			{"greek", CS_8859_7},
			{"greek8", CS_8859_7},
			{"csISOLatinGreek", CS_8859_7},
											/* aliases for iso-8859-5:	*/
			{"iso-ir-144", CS_8859_5},
			{"iso_8859-5", CS_8859_5},
			{"iso_8859-5:1988", CS_8859_5},
			{"cyrillic", CS_8859_5},
			{"csISOLatinCyrillic", CS_8859_5},
											/* aliases for jis_x0212-1990:	*/
			{"x0212", CS_JISX0212},
			{"iso-ir-159", CS_JISX0212},
			{"csISO159JISX02121990", CS_JISX0212},
											/* aliases for jis_x0201:	*/
			{"x0201", CS_JISX0201},
			{"csHalfWidthKatakana", CS_JISX0201},
											/* aliases for koi8-r:	*/
			{"csKOI8R", CS_KOI8_R},
											/* aliases for Shift_JIS:	*/
			{"x-sjis", CS_SJIS},
			{"ms_Kanji", CS_SJIS},
			{"csShiftJIS", CS_SJIS},
											/* aliases for x-euc-jp:	*/
			{"Extended_UNIX_Code_Packed_Format_for_Japanese", CS_EUCJP},
			{"csEUCPkdFmtJapanese", CS_EUCJP},
			{"euc-jp", CS_EUCJP},

											/* aliases for adobe-symbol-encoding:	*/
			{"csHPPSMath", CS_SYMBOL},
											/* aliases for iso-8859-5-windows-latin-5:	*/
			{"csWindows31Latin5", CS_CP_1251},

			{"UNICODE-1-1-UTF-8", CS_UTF8},

			{"x-user-defined", CS_USER_DEFINED_ENCODING},											
			{"x-user-defined", CS_USRDEF2},											

											/* windows additions */
			{"ISO-8859-1-Windows-3.0-Latin-1", CS_LATIN1},
			{"ISO-8859-1-Windows-3.1-Latin-1", CS_LATIN1},
			{"ISO-8859-2-Windows-Latin-2", CS_LATIN2},
			{"iso-8859-5-windows-latin-5", CS_CP_1251},
			{"Windows-31J", CS_SJIS},
			{"x-cp1250", CS_CP_1250},
			{"x-cp1251", CS_CP_1251},
			{"windows-1253", CS_8859_7},  /* greek */
			{"windows-1254", CS_8859_9},  /* turkish */

			{"CN-GB", CS_GB_8BIT},  /* Simplified Chinese */
			{"CN-GB-ISOIR165", CS_GB_8BIT},  /* Simplified Chinese */
			{"CN-Big5", CS_BIG5},  /* Traditional Chinese */

			{"", 	CS_UNKNOWN}
};
		/* Table that maps the FROM char, codeset to all other relevant info:
		 *	- TO character codeset
		 *	- Fonts (fixe & proportional) for TO character codeset
		 *	- Type of conversion (func for Win/Mac, value for X)
		 *	- Argument for conversion routine.  Routine-defined.
		 *
		 * Not all of these may be available.  Depends upon available fonts,
		 * scripts, codepages, etc.  Need to query system to build valid table.
		 *
		 * What info do I need to make the font change API on the 3 platforms?
		 *  Is just a 32bit font ID sufficient?
		 *
		 * Some X Windows can render Japanese in either EUC or SJIS, how do we
		 * choose?
		 */
#ifndef XP_UNIX
cscvt_t		cscvt_tbl[] = {
		/* The ***first*** match of a "FROM" encoding (1st col.) will be
		 * used as the URL->native encoding.  Be careful of the
		 * ordering.
		 * Additional entries for the same "FROM" encoding, specifies
		 * how to convert going out (e.g., sending mail, news or forms).
		 */
#ifdef XP_MAC
		{CS_LATIN1,		CS_MAC_ROMAN,	0, (CCCFunc)One2OneCCC,	xlat_LATIN1_TO_MAC_ROMAN},
		{CS_ASCII,		CS_MAC_ROMAN,	0, (CCCFunc)One2OneCCC,	xlat_LATIN1_TO_MAC_ROMAN},
		{CS_MAC_ROMAN,	CS_MAC_ROMAN,	0, (CCCFunc)0,			0},
		{CS_MAC_ROMAN,	CS_LATIN1,		0, (CCCFunc)One2OneCCC,	xlat_MAC_ROMAN_TO_LATIN1},
		{CS_MAC_ROMAN,	CS_ASCII,		0, (CCCFunc)One2OneCCC,	xlat_MAC_ROMAN_TO_LATIN1},

		{CS_LATIN2,		CS_MAC_CE,		0, (CCCFunc)One2OneCCC, xlat_LATIN2_TO_MAC_CE},
		{CS_MAC_CE,		CS_MAC_CE,		0, (CCCFunc)0,			0},
		{CS_MAC_CE,		CS_LATIN2,		0, (CCCFunc)One2OneCCC, xlat_MAC_CE_TO_LATIN2},
		{CS_MAC_CE,		CS_ASCII,		0, (CCCFunc)One2OneCCC, xlat_MAC_CE_TO_LATIN2},

		{CS_CP_1250,	CS_MAC_CE,		0, (CCCFunc)One2OneCCC, xlat_CP_1250_TO_MAC_CE},
		{CS_MAC_CE,		CS_CP_1250,		0, (CCCFunc)One2OneCCC, xlat_MAC_CE_TO_CP_1250},

		{CS_8859_5,		CS_MAC_CYRILLIC,0, (CCCFunc)One2OneCCC, xlat_8859_5_TO_MAC_CYRILLIC},
		{CS_MAC_CYRILLIC,CS_MAC_CYRILLIC,		0, (CCCFunc)0,			0},
		{CS_MAC_CYRILLIC,CS_8859_5,		0, (CCCFunc)One2OneCCC, xlat_MAC_CYRILLIC_TO_8859_5},
		{CS_MAC_CYRILLIC,CS_ASCII,		0, (CCCFunc)One2OneCCC, xlat_MAC_CYRILLIC_TO_8859_5},

		{CS_CP_1251,	CS_MAC_CYRILLIC,0, (CCCFunc)One2OneCCC, xlat_CP_1251_TO_MAC_CYRILLIC},
		{CS_MAC_CYRILLIC,CS_CP_1251,	0, (CCCFunc)One2OneCCC, xlat_MAC_CYRILLIC_TO_CP_1251},

		{CS_KOI8_R,		CS_KOI8_R,		0, (CCCFunc)0, 			0},
		{CS_KOI8_R,		CS_MAC_CYRILLIC,0, (CCCFunc)One2OneCCC, xlat_KOI8_R_TO_MAC_CYRILLIC},
		{CS_MAC_CYRILLIC,CS_KOI8_R,		0, (CCCFunc)One2OneCCC, xlat_MAC_CYRILLIC_TO_KOI8_R},

		{CS_8859_7,		CS_MAC_GREEK,	0, (CCCFunc)One2OneCCC, xlat_8859_7_TO_MAC_GREEK},
		{CS_MAC_GREEK,	CS_MAC_GREEK,	0, (CCCFunc)0,			0},
		{CS_MAC_GREEK,	CS_8859_7,		0, (CCCFunc)One2OneCCC, xlat_MAC_GREEK_TO_8859_7},
		{CS_MAC_GREEK,	CS_ASCII,		0, (CCCFunc)One2OneCCC, xlat_MAC_GREEK_TO_8859_7},


		{CS_8859_9,		CS_MAC_TURKISH,	0, (CCCFunc)One2OneCCC, xlat_8859_9_TO_MAC_TURKISH},
		{CS_MAC_TURKISH,CS_MAC_TURKISH,	0, (CCCFunc)0,			0},
		{CS_MAC_TURKISH,CS_8859_9,		0, (CCCFunc)One2OneCCC, xlat_MAC_TURKISH_TO_8859_9},
		{CS_MAC_TURKISH,CS_ASCII,		0, (CCCFunc)One2OneCCC, xlat_MAC_TURKISH_TO_8859_9},



/*	ftang: Add CS_USER_DEFINED_ENCODING. Do we need this in here ? */

		{CS_USER_DEFINED_ENCODING,	CS_USER_DEFINED_ENCODING,	0, (CCCFunc)0,			0},	

#endif /* XP_MAC */

#ifdef XP_MAC
		/* How about GB ??? */
		{CS_GB_8BIT,	CS_GB_8BIT,		0, (CCCFunc)mz_gb2gb,		0},
#endif /* XP_MAC */

#ifdef XP_WIN
		{CS_LATIN1,		CS_LATIN1,		0,	(CCCFunc)0,			0},
		{CS_LATIN1,		CS_ASCII,		0, 	(CCCFunc)0,			0},
		{CS_ASCII,		CS_LATIN1,		0, 	(CCCFunc)0,			0},
		{CS_ASCII,		CS_ASCII,		0, 	(CCCFunc)0,			0},

		/* Latin2 <-> Win-Latin2 */
		{CS_CP_1250,	CS_CP_1250,		0, (CCCFunc)0,			0},
		{CS_CP_1250,	CS_LATIN2,		0, (CCCFunc)One2OneCCC,	0},
		{CS_LATIN2,		CS_CP_1250,		0, (CCCFunc)One2OneCCC,	0},
		{CS_LATIN2,		CS_LATIN2,		0,	(CCCFunc)0,			0},
		{CS_LATIN2,		CS_ASCII,		0,	(CCCFunc)0,			0},
		/* ISO, KOI, Windows Cyrillic */
		{CS_CP_1251,	CS_CP_1251,		0, (CCCFunc)0,			0},
		{CS_8859_5,		CS_CP_1251,		0, (CCCFunc)One2OneCCC, 0},
		{CS_CP_1251,	CS_8859_5,		0, (CCCFunc)One2OneCCC, 0},
		{CS_CP_1251,	CS_CP_1251,		0, (CCCFunc)0,			0},
		{CS_KOI8_R,		CS_KOI8_R,		0, (CCCFunc)0,			0},
		{CS_CP_1251,	CS_KOI8_R,		0, (CCCFunc)One2OneCCC, 0},
		{CS_KOI8_R,		CS_CP_1251,		0, (CCCFunc)One2OneCCC, 0},
		/* ISO,Windows Greek */
		{CS_8859_7,		CS_8859_7,		0, (CCCFunc)0,			0},
		/* ISO, Windows Turkish */
		{CS_8859_9,		CS_8859_9,		0, (CCCFunc)0,			0},

		{CS_UTF8,		CS_UTF8,		0, (CCCFunc)0,			0},
		
		{CS_USER_DEFINED_ENCODING,	CS_USER_DEFINED_ENCODING,	0, (CCCFunc)0,			0},	
#endif /* XP_WIN */
		{CS_SJIS,		CS_SJIS,		1, (CCCFunc)mz_sjis2sjis,	0},
		{CS_SJIS,		CS_JIS,			1, (CCCFunc)mz_sjis2jis,	0},
		{CS_JIS,		CS_SJIS,		1, (CCCFunc)jis2other,	0},
		{CS_EUCJP,		CS_SJIS,		1, (CCCFunc)mz_euc2sjis,	0},
		{CS_JIS,		CS_EUCJP,		1, (CCCFunc)jis2other,	1},
		{CS_EUCJP,		CS_JIS,			1, (CCCFunc)mz_euc2jis,	0},
		{CS_SJIS,		CS_EUCJP,		1, (CCCFunc)mz_sjis2euc,	0},
		{CS_KSC_8BIT,	CS_KSC_8BIT,	0, (CCCFunc)mz_ksc2ksc,		0},
		{CS_BIG5,		CS_BIG5,		0, (CCCFunc)mz_b52b5,	0},
		{CS_GB_8BIT,	CS_GB_8BIT,		0, (CCCFunc)mz_gb2gb,		0},

		{CS_2022_KR,	CS_KSC_8BIT,	0, (CCCFunc)mz_iso2euckr,0},
		{CS_KSC_8BIT,	CS_2022_KR,		0, (CCCFunc)mz_euckr2iso,0},
		{CS_BIG5,		CS_CNS_8BIT,	0, (CCCFunc)mz_b52cns,0},
		{CS_CNS_8BIT,	CS_BIG5,		0, (CCCFunc)mz_cns2b5,0},

		/* auto-detect Japanese conversions						*/
		{CS_SJIS_AUTO,		CS_SJIS,		1, (CCCFunc)autoJCCC,	0},
		{CS_JIS_AUTO,		CS_SJIS,		1, (CCCFunc)autoJCCC,	0},
		{CS_EUCJP_AUTO,		CS_SJIS,		1, (CCCFunc)autoJCCC,	0},
		{0,					0,				1, (CCCFunc)0,			0}
};

#else /* XP_UNIX */

cscvt_t		cscvt_tbl[] = {

#ifndef USE_NPC

		/* "FROM" encoding (1st col.) must be unique		*/

		{CS_LATIN1,		CS_LATIN1,		0, NULL,			0},
		{CS_LATIN1,		CS_ASCII,		0, NULL,			0},
		{CS_LATIN2,		CS_LATIN2,		0, NULL,			0},
		{CS_LATIN2,		CS_ASCII,		0, NULL,			0},
		{CS_ASCII,		CS_LATIN1,		0, NULL,			0},
		{CS_EUCJP,		CS_EUCJP,		1, mz_euc2euc,		0},
		{CS_JIS,		CS_EUCJP,		1, jis2other,		1},
		{CS_SJIS,		CS_EUCJP,		1, mz_sjis2euc,		0},
		{CS_EUCJP,		CS_SJIS,		1, mz_euc2sjis,		0},
		{CS_JIS,		CS_SJIS,		1, jis2other,		0},
		{CS_SJIS,		CS_SJIS,		1, mz_sjis2sjis,	0},
		{CS_EUCJP,		CS_JIS,			1, mz_euc2jis,		0},
		{CS_SJIS,		CS_JIS,			1, mz_sjis2jis,		0},
		{CS_KSC_8BIT,	CS_KSC_8BIT,	0, mz_ksc2ksc,		0},
		{CS_GB_8BIT,	CS_GB_8BIT,		0, mz_gb2gb,		0},
		{CS_CNS_8BIT,	CS_CNS_8BIT,	0, mz_cns2cns,		0},
		{CS_2022_KR,	CS_KSC_8BIT,	0, mz_iso2euckr,	0},
		{CS_KSC_8BIT,	CS_2022_KR,		0, mz_euckr2iso,	0},
		{CS_BIG5,		CS_CNS_8BIT,	0, mz_b52cns,		0},
		{CS_CNS_8BIT,	CS_BIG5,		0, mz_cns2b5,		0},
		{CS_BIG5,		CS_BIG5,		0, mz_b52b5,		0},
		{CS_KOI8_R,		CS_KOI8_R,		0, NULL,			0},	
		{CS_8859_5,		CS_8859_5,		0, NULL,			0},	
		{CS_8859_7,		CS_8859_7,		0, NULL,			0},	
		{CS_8859_9,		CS_8859_9,		0, NULL,			0},	
		{CS_USRDEF2,	CS_USRDEF2,		0, NULL,			0},	

		/* auto-detect Japanese conversions					*/

		{CS_JIS_AUTO,	CS_EUCJP,		1, autoJCCC,		1},
		{CS_SJIS_AUTO,	CS_EUCJP,		1, autoJCCC,		0},
		{CS_EUCJP_AUTO,	CS_EUCJP,		1, autoJCCC,		0},
		{CS_EUCJP_AUTO,	CS_SJIS,		1, autoJCCC,		0},
		{CS_JIS_AUTO,	CS_SJIS,		1, autoJCCC,		0},
		{CS_SJIS_AUTO,	CS_SJIS,		1, autoJCCC,		0},

#else /* USE_NPC */

		{CS_LATIN1,		CS_LATIN1,		0, intl_ISO88591ToNPC,	0},

		/*
		{CS_JIS,		CS_LATIN1,		0, intl_JISToNPC,		0},
		{CS_SJIS,		CS_LATIN1,		0, intl_SJISToNPC,		0},
		{CS_EUCJP,		CS_LATIN1,		0, intl_EUCJPToNPC,		0},
		*/

		/*
		{CS_2022_KR,	CS_LATIN1,		0, intl_2022KRToNPC,	0},
		{CS_ASCII,		CS_LATIN1,		0, intl_ASCIIToNPC,		0},
		{CS_BIG5,		CS_LATIN1,		0, intl_BIG5ToNPC,		0},
		{CS_GB_8BIT,	CS_LATIN1,		0, intl_EUCCNToNPC,		0},
		{CS_KSC_8BIT,	CS_LATIN1,		0, intl_EUCKRToNPC,		0},
		{CS_CNS_8BIT,	CS_LATIN1,		0, intl_EUCTWToNPC,		0},
		{CS_LATIN2,		CS_LATIN1,		0, intl_ISO88592ToNPC,	0},
		*/

#endif /* USE_NPC */

		{0,				0,				0, NULL,			0}

};

#endif /* XP_UNIX */

	/* Maps a window encoding to the MIME encoding used for posting
	 * news and internet email.  String is for the MIME charset string.
	 */
csmime_t	csmime_tbl[] = {
			{CS_ASCII,		CS_ASCII,	"us-ascii"		},
			
			{CS_LATIN1,		CS_LATIN1,	"iso-8859-1"	},
			
			{CS_JIS,		CS_JIS,		"iso-2022-jp"	},
			{CS_SJIS,		CS_JIS,		"iso-2022-jp"	},
			{CS_EUCJP,		CS_JIS,		"iso-2022-jp"	},
			
			{CS_KSC_8BIT,	CS_KSC_8BIT,"euc-kr"		},
			
			{CS_GB_8BIT,	CS_GB_8BIT,	"gb2312"		},
			
			{CS_BIG5,		CS_BIG5,	"big5"		},
			{CS_CNS_8BIT,	CS_BIG5,	"big5"		},
			
			{CS_MAC_ROMAN,	CS_LATIN1,	"iso-8859-1"	},
			
			{CS_LATIN2,		CS_LATIN2, 	"iso-8859-2"	},
			{CS_MAC_CE,		CS_LATIN2,	"iso-8859-2"	},
			{CS_CP_1250,	CS_LATIN2, 	"iso-8859-2"	},
			
			{CS_8859_5,			CS_KOI8_R,	"koi8-r"	},
			{CS_KOI8_R,			CS_KOI8_R,	"koi8-r"	},
			{CS_MAC_CYRILLIC,	CS_KOI8_R,	"koi8-r"	},
			{CS_CP_1251,		CS_KOI8_R,	"koi8-r"	},
			
			{CS_8859_7,		CS_8859_7,	"iso-8859-7"	},
			{CS_MAC_GREEK,	CS_8859_7,	"iso-8859-7"	},

			{CS_8859_9,		CS_8859_9, 	"iso-8859-9"	},
			{CS_MAC_TURKISH,CS_8859_9,	"iso-8859-9"	},
			
			{0,				0,			""				}
};

#ifdef XP_UNIX
cslocale_t	cslocale_tbl[] = {
	{CS_LATIN1,	"C",			"*8859-1*, *jisx208*, *jisx201*" },
	{CS_EUCJP,	"jp_JP.EUC",	"*8859-1*, *jisx208*, *jisx201*" },
	{CS_SJIS,	"jp_JP.SJIS",	"*8859-1*, *jisx208*, *jisx201*" },
	{CS_LATIN2, "C",			"*8859-2*, *jisx208*, *jisx201*" },
	{CS_KSC_8BIT,	"ko_KR.euc",	"*8859-1*, *ksc5601*" },
	{CS_GB_8BIT,	"zh_CN.ugb",	"*8859-1*, *gb2312*" },
	{CS_CNS_8BIT,	"zh_TW.ucns",	"*8859-1*, *cns11643*" },
	{CS_BIG5,		"zh_TW.ucns",	"*8859-1*, *cns11643*" },
	{0,			0,				0}
};
#endif /* XP_UNIX */


static int16 *availableFontCharSets = NULL;


	/* returns Handle to 256 x 1Byte table for converting 1Byte char. encodings	*/

char ** 
INTL_GetSingleByteTable(int16 from_csid, int16 to_csid, int32 resourceid)
{
#if defined(XP_MAC) || defined(XP_WIN) 
	return FE_GetSingleByteTable(from_csid, to_csid,resourceid);
#else
	return(NULL);
#endif
}

	
char *INTL_LockTable(char **cvthdl)
{
#ifdef XP_WIN
	return FE_LockTable(cvthdl);
#else
	return *cvthdl;
#endif
}

        /* INTL_FreeSingleByteTable(char **cvthdl)
         * mac-specific, at present. Sets the conversion table resource
         * in memory as purgeable when it is no longer in use. To be
         * called by cvchcode.c and pa_amp.c
         */

void INTL_FreeSingleByteTable(char **cvthdl) {
#ifdef XP_MAC
	FE_FreeSingleByteTable(cvthdl);
#endif
}

#ifdef XP_MAC
#pragma cplusplus on
#include "uprefd.h"
#endif
#ifdef XP_UNIX
char* FE_GetAcceptLanguage();
#endif

/* INTL_GetAcceptLanguage()						*/
/* return the AcceptLanguage from FE Preference */
/* this should be a C style NULL terminated string */
char* INTL_GetAcceptLanguage()
{
#ifdef XP_MAC
	return (CPrefs::GetString( CPrefs::AcceptLanguage ));
#else
	return FE_GetAcceptLanguage();
#endif
}
#ifdef XP_MAC
#pragma cplusplus reset
#endif
PRIVATE 
XP_Bool onlyAsciiStr(const char *s)
{
	for(; *s; s++)
		if(*s & 0x80)
			return FALSE;
	return TRUE;
}

/*
 * this routine is needed to make sure parser and layout see whole
 * characters, not partial characters
 */
static unsigned char *
mz_euc2euc(CCCDataObject *obj, const unsigned char *eucbuf, int32 eucbufsz)
{
	unsigned char	b;
	int32			left_over;
	int32			len;
	unsigned char	*p;
	unsigned char	*q;
	unsigned char	*ret;
	int32			total;

	if ((!obj) || (!eucbuf) || (eucbufsz < 0))
	{
		return NULL;
	}

	total = strlen((char *)obj->uncvtbuf) + eucbufsz;
	ret = (unsigned char *) XP_ALLOC(total + 1);
	if (!ret)
	{
		obj->retval = MK_OUT_OF_MEMORY;
		return NULL;
	}

	p = ret;
	q = obj->uncvtbuf;
	while (*q)
	{
		*p++ = *q++;
	}

	q = (unsigned char *) eucbuf;
	while (eucbufsz)
	{
		*p++ = *q++;
		eucbufsz--;
	}

	p = ret;
	len = total;
	left_over = 0;
	while (len)
	{
		b = *p++;
		len--;
		if (b < SS2)
		{
			/* single byte */
		}
		else if (b == SS2)
		{
			if (len == 0)
			{
				left_over = 1;
				break;
			}
			p++;
			len--;
		}
		else if (b == SS3)
		{
			if (len == 0)
			{
				left_over = 1;
				break;
			}
			if (len == 1)
			{
				left_over = 2;
				break;
			}
			p += 2;
			len -= 2;
		}
		else if (b < 0xa1)
		{
			/* single byte */
		}
		else if (b < 0xff)
		{
			if (len == 0)
			{
				left_over = 1;
				break;
			}
			p++;
			len--;
		}
		else /* (b == 0xff) */
		{
			/* single byte */
		}
	}

	obj->len = total - left_over;

	q = --p;
	p = obj->uncvtbuf;
	while (left_over)
	{
		*p++ = *q++;
		left_over--;
	}
	*p = 0;

	ret[obj->len] = 0;

	return ret;
}

/*
 * this routine is needed to make sure parser and layout see whole
 * characters, not partial characters
 */
static unsigned char *
mz_sjis2sjis(CCCDataObject *obj, const unsigned char *sjisbuf, int32 sjisbufsz)
{
	unsigned char	b;
	int32			left_over;
	int32			len;
	unsigned char	*p;
	unsigned char	*q;
	unsigned char	*ret;
	int32			total;

	if ((!obj) || (!sjisbuf) || (sjisbufsz < 0))
	{
		return NULL;
	}

	total = strlen((char *)obj->uncvtbuf) + sjisbufsz;
	ret = (unsigned char *) XP_ALLOC(total + 1);
	if (!ret)
	{
		obj->retval = MK_OUT_OF_MEMORY;
		return NULL;
	}

	p = ret;
	q = obj->uncvtbuf;
	while (*q)
	{
		*p++ = *q++;
	}

	q = (unsigned char *) sjisbuf;
	while (sjisbufsz)
	{
		*p++ = *q++;
		sjisbufsz--;
	}

	p = ret;
	len = total;
	left_over = 0;
	while (len)
	{
		b = *p++;
		len--;
		if (b < 0x80)
		{
			/* single byte */
		}
		else if (b < 0xa0)
		{
			if (len == 0)
			{
				left_over = 1;
				break;
			}
			p++;
			len--;
		}
		else if (b < 0xe0)
		{
			/* single byte */
		}
		else /* (b <= 0xff) */
		{
			if (len == 0)
			{
				left_over = 1;
				break;
			}
			p++;
			len--;
		}
	}

	obj->len = total - left_over;

	q = --p;
	p = obj->uncvtbuf;
	while (left_over)
	{
		*p++ = *q++;
		left_over--;
	}
	*p = 0;

	ret[obj->len] = 0;

	return ret;
}

/*
 * this routine is needed to make sure parser and layout see whole
 * characters, not partial characters
 */
static unsigned char *
mz_ksc2ksc(CCCDataObject *obj, const unsigned char *kscbuf, int32 kscbufsz)
{
	unsigned char	b;
	int32			left_over;
	int32			len;
	unsigned char	*p;
	unsigned char	*q;
	unsigned char	*ret;
	int32			total;

	if ((!obj) || (!kscbuf) || (kscbufsz < 0))
	{
		return NULL;
	}

	total = strlen((char*)obj->uncvtbuf) + kscbufsz;
	ret = (unsigned char *) XP_ALLOC(total + 1);
	if (!ret)
	{
		obj->retval = MK_OUT_OF_MEMORY;
		return NULL;
	}

	p = ret;
	q = obj->uncvtbuf;
	while (*q)
	{
		*p++ = *q++;
	}

	q = (unsigned char *) kscbuf;
	while (kscbufsz)
	{
		*p++ = *q++;
		kscbufsz--;
	}

	p = ret;
	len = total;
	left_over = 0;
	while (len)
	{
		b = *p++;
		len--;
		if (b < 0xa1)
		{
			/* single byte */
		}
		else if (b < 0xff)
		{
			if (len == 0)
			{
				left_over = 1;
				break;
			}
			p++;
			len--;
		}
		else /* (b == 0xff) */
		{
			/* single byte */
		}
	}

	obj->len = total - left_over;

	q = --p;
	p = obj->uncvtbuf;
	while (left_over)
	{
		*p++ = *q++;
		left_over--;
	}
	*p = 0;

	ret[obj->len] = 0;

	return ret;
}

/*
 * this routine is needed to make sure parser and layout see whole
 * characters, not partial characters
 */
static unsigned char *
mz_gb2gb(CCCDataObject *obj, const unsigned char *gbbuf, int32 gbbufsz)
{
	unsigned char *start, *p, *q;
	unsigned char *output;
	int i, j, len;

	q = output = XP_ALLOC(strlen((char*)obj->uncvtbuf) + gbbufsz + 1);
	if (q == NULL)
		return NULL;

	start = NULL;

	for (j = 0; j < 2; j++)
	{
		if (j == 0 && (len = strlen((char *)obj->uncvtbuf)))
		{
			p = (unsigned char *) obj->uncvtbuf;
		}
		else
		{
			p = (unsigned char *) gbbuf ;
			len = gbbufsz;
			j = 100;  /* quit this loop next time */
		}
			
		for (i = 0; i < len;)
		{
			if (start)
			{
				if (*p == '~' && *(p+1) == '}')   /* switch back to ASCII mode */
				{
					for (; start < p; start++)
						*q++ = *start | 0x80;
					p += 2;
					i += 2;
					start = NULL;
				}
				else if (*p == 0x0D && *(p+1) == 0x0A)  /* Unix or Mac return */
				{
					for (; start < p; start++)
						*q++ = *start | 0x80;
					i += 2;
					*q++ = *p++;   /* 0x0D  */
					*q++ = *p++;   /* 0x0A  */
					start = NULL;   /* reset start if we see normal line return */
				}
				else if (*p == 0x0A)  /* Unix or Mac return */
				{
					for (; start < p; start++)
						*q++ = *start | 0x80;
					i ++;
					*q++ = *p++;   /* LF  */
					start = NULL;   /* reset start if we see normal line return */
				}
				else if (*p == 0x0D)  /* Unix or Mac return */
				{
					for (; start < p; start++)
						*q++ = *start | 0x80;
					i ++;
					*q++ = *p++;   /* LF  */
					start = NULL;   /* reset start if we see normal line return */
				}
				else
				{
					i ++ ;
					p ++ ;
				}
			}
			else
			{
				if (*p == '~' && *(p+1) == '{')    /* switch to GB mode */
				{
					start = p + 2;
					p += 2;
					i += 2;
				}
				else if (*p == '~' && *(p+1) == 0x0D && *(p+2) == 0x0A)  /* line-continuation marker */
				{
					i += 3;
					p += 3;
				}
				else if (*p == '~' && *(p+1) == 0x0A)  /* line-continuation marker */
				{
					i += 2;
					p += 2;
				}
				else if (*p == '~' && *(p+1) == 0x0D)  /* line-continuation marker */
				{
					i += 2;
					p += 2;
				}
				else if (*p == '~' && *(p+1) == '~')   /* ~~ means ~ */
				{
					*q++ = '~';
					p += 2;
					i += 2;
				}
				else
				{
					i ++;
					*q++ = *p++;
				}
			}
		}
	}
	*q = '\0';
	obj->len = q - output;
	if (start)
	{
#if 0
		if ((p - start) < sizeof(obj->uncvtbuf))
		{
			q = obj->uncvtbuf;
			for (; start < p; start++)
				*q++ = *start;
			*q = '\0';
		}
		else  /* if leftover is too big, we give up */
		{
			obj->len += p - start;
			for (; start < p; start++)
				*q++ = *start;
			*q = '\0';
		}
#else 
		/* Consider sizeof uncvtbuf is only 8 byte long, it's not enough 
		   for HZ anyway. Let's convert leftover to GB first and deal with
		   unfinished buffer in the coming block.
		*/
		obj->len += p - start;
		for (; start < p; start++)
			*q++ = *start | 0x80;
		*q = '\0';

		q = obj->uncvtbuf;
		XP_STRCPY((char *)q, "~{");
#endif
	}

    return output;
}


static unsigned char *
mz_b52b5(CCCDataObject *obj, const unsigned char *kscbuf, int32 kscbufsz)
{
    return(mz_ksc2ksc(obj, kscbuf, kscbufsz));
}

/*
 * this routine is needed to make sure parser and layout see whole
 * characters, not partial characters
 * XXX - obj->uncvtbuf needs to have atleast 4 bytes space.
 */
static unsigned char *
mz_cns2cns(CCCDataObject *obj, const unsigned char *cnsbuf, int32 cnsbufsz)
{
	unsigned char	b;
	int32			left_over;
	int32			len;
	unsigned char	*p;
	unsigned char	*q;
	unsigned char	*ret;
	int32			total;

	if ((!obj) || (!cnsbuf) || (cnsbufsz < 0))
	{
		return NULL;
	}

	total = strlen((char*)obj->uncvtbuf) + cnsbufsz;
	ret = (unsigned char *) XP_ALLOC(total + 1);
	if (!ret)
	{
		obj->retval = MK_OUT_OF_MEMORY;
		return NULL;
	}

	p = ret;
	q = obj->uncvtbuf;
	while (*q)
	{
		*p++ = *q++;
	}

	q = (unsigned char *) cnsbuf;
	while (cnsbufsz)
	{
		*p++ = *q++;
		cnsbufsz--;
	}

	p = ret;
	len = total;
	left_over = 0;
	while (len)
	{
		b = *p++;
		len--;
		if (b == 0x8E) {
			/* 4 bytes */
			if (len < 3) {
				left_over = len+1;
				break;
			}
			p+=3;
			len-=3;
		}
		else if (b < 0xa1)
		{
			/* single byte */
		}
		else if (b < 0xff)
		{
			/* 2 bytes */
			if (len == 0)
			{
				left_over = 1;
				break;
			}
			p++;
			len--;
		}
		else /* (b == 0xff) */
		{
			/* single byte */
		}
	}

	obj->len = total - left_over;

	q = --p;
	p = obj->uncvtbuf;
	while (left_over)
	{
		*p++ = *q++;
		left_over--;
	}
	*p = 0;

	ret[obj->len] = 0;

	return ret;
}


CCCDataObject *
INTL_CreateCharCodeConverter()
{
	return XP_NEW_ZAP(CCCDataObject);
}


void
INTL_DestroyCharCodeConverter(CCCDataObject *obj)
{
	XP_FREE(obj);
}


unsigned char *
INTL_CallCharCodeConverter(CCCDataObject *obj, const unsigned char *buf,
	int32 bufsz)
{
	return (obj->cvtfunc)(obj, buf, bufsz);
}


	/* INTL_GetCharCodeConverter:
	 * RETURN: 1 if converter found, else 0
	 * Also, sets:
	 *			obj->cvtfunc:	function handle for chararcter
	 *					code set streams converter
	 *			obj->cvtflag:	(Optional) flag to converter
	 *					function
	 *			obj->from_csid:	Code set converting from
	 *			obj->to_csid:	Code set converting to
	 * If the arg to_csid==0, then use the the conversion  for the
	 * first conversion entry that matches the from_csid.
	 */
int
INTL_GetCharCodeConverter(	register int16	from_csid,
				register int16	to_csid,
				CCCDataObject	*obj)
{
	register cscvt_t		*cscvtp;

	if (from_csid == CS_DEFAULT)
	{
		if (obj->current_stream == NULL)  /* Get Global if no current window */
			obj->from_csid = INTL_DefaultDocCharSetID(0);
		else
			obj->from_csid = INTL_DefaultDocCharSetID(obj->current_stream->window_id);
	}
	else
		obj->from_csid = from_csid;

		/* Look-up conversion method given FROM and TO char. code sets	*/
	cscvtp = cscvt_tbl;
	while (cscvtp->from_csid) {
		if (cscvtp->from_csid == from_csid) {
			if (to_csid == 0) {				/* unknown TO codeset	*/
/*
 * disgusting hack...
 */
#ifdef XP_UNIX
				if ((cscvtp->to_csid == CS_CNS_8BIT) && haveBig5) {
					cscvtp++;
					continue;
				}
#endif
				to_csid = cscvtp->to_csid;
				break;
			} else if (cscvtp->to_csid == to_csid)
				break;
		}
		cscvtp++;
	}
	obj->to_csid = to_csid;
	obj->cvtflag = cscvtp->cvtflag;
	obj->cvtfunc = cscvtp->cvtmethod;

	return (obj->cvtfunc) ? 1 : 0;
}

/*
 INTL_DefaultMailCharSetID,
   Based on DefaultDocCSID, it determines which CSID to use for mail and news.
   Right now, if it's Japanese code, we return JIS otherwise LATIN1.
*/
int16 INTL_DefaultMailCharSetID(int16 csid)
{
	int16 iRet ;
	if (csid == 0)
		csid = INTL_DefaultDocCharSetID(0);
	csid = csid & ~CS_AUTO;
	switch (csid) {
	  case CS_JIS:
	  case CS_SJIS:
	  case CS_EUCJP:
		iRet = CS_JIS;
		break ;
	  case CS_KSC_8BIT:
		iRet = CS_KSC_8BIT;
		break;
	  case CS_GB_8BIT:
		iRet = CS_GB_8BIT;
		break;
	  case CS_BIG5:
	  case CS_CNS_8BIT:
	    iRet = CS_BIG5;
	    break;
	  case CS_LATIN2:
	  case CS_CP_1250:
	  case CS_MAC_CE:
	    iRet = CS_LATIN2;
	    break;  	  	
	  case CS_8859_5:
	  case CS_MAC_CYRILLIC:
	  case CS_CP_1251:
	  case CS_KOI8_R:
	    iRet = CS_KOI8_R;
	    break;  	  	
	  case CS_MAC_GREEK:
	  case CS_8859_7:
	    iRet = CS_8859_7;
	    break;  	  	
	  case CS_MAC_TURKISH:
	  case CS_8859_9:
	    iRet = CS_8859_9;
	    break;  	  	
	  default:
	    iRet = CS_LATIN1;
		break ;
	}

	return iRet ;
}

/*
 INTL_DefaultWinCharSetID,
   Based on DefaultDocCSID, it determines which Win CSID to use for Display
*/
int16 INTL_DefaultWinCharSetID(MWContext *context)
{
	cscvt_t		*cscvtp;
	int16       from_csid = 0,  to_csid = 0;

	if (context && context->win_csid)
		return context->win_csid;
	from_csid = INTL_DefaultDocCharSetID(context) & ~CS_AUTO;	/* remove auto bit  */

	/* Look-up conversion method given FROM and TO char. code sets	*/
	cscvtp = cscvt_tbl;
	while (cscvtp->from_csid)
	{
		if (cscvtp->from_csid == from_csid)
		{
/*
 * disgusting hack...
 */
#ifdef XP_UNIX
			if ((cscvtp->to_csid == CS_CNS_8BIT) && haveBig5) {
				cscvtp++;
				continue;
			}
#endif
			to_csid = cscvtp->to_csid;
			break ;
		}
		cscvtp++;
	}
#ifndef XP_MAC
	return to_csid == 0 ? CS_LATIN1: to_csid ;
#else /* XP_UNIX */
	return to_csid == 0 ? CS_MAC_ROMAN: to_csid ;
#endif /* XP_UNIX */
}

/*
 INTL_DocToWinCharSetID,
   Based on DefaultDocCSID, it determines which Win CSID to use for Display
*/
int16 INTL_DocToWinCharSetID(int16 csid)
{
	cscvt_t		*cscvtp;
	int16       from_csid = 0,  to_csid = 0;

	from_csid = csid & ~CS_AUTO;	/* remove auto bit  */

	/* Look-up conversion method given FROM and TO char. code sets	*/
	cscvtp = cscvt_tbl;
	while (cscvtp->from_csid)
	{
		if (cscvtp->from_csid == from_csid)
		{
/*
 * disgusting hack...
 */
#ifdef XP_UNIX
			if ((cscvtp->to_csid == CS_CNS_8BIT) && haveBig5) {
				cscvtp++;
				continue;
			}
#endif
			to_csid = cscvtp->to_csid;
			break ;
		}
		cscvtp++;
	}
#ifndef XP_MAC
	return to_csid == 0 ? CS_LATIN1: to_csid ;
#else /* XP_UNIX */
	return to_csid == 0 ? CS_MAC_ROMAN: to_csid ;
#endif /* XP_UNIX */
}



int16
INTL_CharSetNameToID(char	*charset)
{
	csname2id_t	*csn2idp;
	int16			csid;

			/* Parse the URL charset string for the charset ID.
			 * If no MIME Content-Type charset pararm., default.
			 * HTML specifies ASCII, but let user override cuz
			 * of prior conventions (i.e. Japan).
			 */
	csn2idp = csname2id_tbl;
	csid = csn2idp->cs_id;		/* 1st entry is default codeset ID	*/

	if (charset != NULL) {		/* Linear search for charset string */
		while (*(csn2idp->cs_name) != '\0') {
 			if (strcasecomp(charset, (char *)csn2idp->cs_name) == 0) {
 				return(csn2idp->cs_id);
 			}
 			csn2idp++;
		}
 	}
	return(csn2idp->cs_id);		/* last entry is CS_UNKNOWN	*/
}
unsigned char *INTL_CsidToCharsetNamePt(int16 csid)
{
	csname2id_t	*csn2idp;

	if (csid & CS_AUTO)
		return (unsigned char *)"AUTO";
	csn2idp = &csname2id_tbl[1];	/* First one is reserved, skip it. */
	csid &= 0xff;

	/* Linear search for charset string */
	while (*(csn2idp->cs_name) != '\0') {
		if ((csn2idp->cs_id & 0xff) == csid)
			return csn2idp->cs_name;
 		csn2idp++;
	}
	return (unsigned char *)"";
}

void 
INTL_CharSetIDToName(int16 csid, char  *charset)
{
	if (charset) {	
		strcpy(charset,(char *)INTL_CsidToCharsetNamePt(csid));
	}
}

/***********************************************************
 Input:  int (int16) charsetid		Char Set ID
         char *pstr				   Buffer which always point to Multibyte char first byte
                      			   or normal single byte char
 Output: return next char position
***********************************************************/
char * INTL_NextChar(int charsetid, char *pstr)
{
#ifdef NOT
	/* Need to remove strlen later and pass in the length of the string */
	int len = mblen(pstr, strlen(pstr));
	return pstr+(len <= 0 ? 1 : len) ;
#else
	csinfo_t	*pFontTable ;
	unsigned char ch ;
	int i;

	if ((INTL_CharSetType(charsetid) == SINGLEBYTE) || (*pstr == 0)) /* If no csid, assume it's not multibyte */
		return pstr + 1;

	ch = *pstr ;
	pFontTable = &csinfo_tbl[0] ;
	for (;pFontTable->csid; pFontTable++)
	{
		if (pFontTable->csid == charsetid)
		{
			for (i=0; i<MAX_FIRSTBYTE_RANGE && pFontTable->enc[i].bytes > 0; i++)
			{
				if ((ch >= pFontTable->enc[i].range[0]) && (ch <= pFontTable->enc[i].range[1]))
				{	
					int j = 0;
					for (j=0; pstr[j] && j < pFontTable->enc[i].bytes; j++)
						;
					if (j < pFontTable->enc[i].bytes)
						return pstr+1;
					else
						return pstr+j;
				}
			}
			return pstr + 1;
		}
	}
	return pstr + 1;
#endif
}

/********************************************************
 Input:  MWContext context		Window Context
         unsigned char ch		Buffer which always point to Multibyte char
								first byte or normal single byte char
 Output: 1, if ch is under ShiftJIS type MultiByte first byte range
         2, if ch is under EUC type MultiByte first byte range
		 0, if it's not MultiByte firstbyte
*********************************************************/

int
INTL_IsLeadByte(int charsetid, unsigned char ch)
{
	csinfo_t	*pFontTable ;
	int i;

	if ((INTL_CharSetType(charsetid) == SINGLEBYTE) || (ch == 0)) /* If no csid, assume it's not multibyte */
		return 0;

	pFontTable = &csinfo_tbl[0] ;
	for (;pFontTable->csid; pFontTable++)
	{
		if (pFontTable->csid == charsetid)
		{
			for (i=0; i<MAX_FIRSTBYTE_RANGE && pFontTable->enc[i].bytes > 0; i++)
				if ((ch >= pFontTable->enc[i].range[0]) &&
					(ch <= pFontTable->enc[i].range[1]))
					return pFontTable->enc[i].bytes-1;
			return 0 ;
		}
	}
	return 0;
}

int 
INTL_CharLen(int charsetid, unsigned char *pstr)
{
	int i,l;
	if ((!pstr) || (!*pstr)) return 0;
	l = 1 + INTL_IsLeadByte(charsetid, *pstr);
	for(i=1, pstr++ ; (i<l) && (*pstr); i++, pstr++)
		;
	return i;
}

int
INTL_ColumnWidth(int charsetid, unsigned char *str)
{
	unsigned char	b;
	csinfo_t	*entry;
	int		i;

	if ((!str) || (!*str))
	{
		return 0;
	}

	if (INTL_CharSetType(charsetid) == SINGLEBYTE)
	{
		return 1;
	}

	for (entry = csinfo_tbl; entry->csid; entry++)
	{
		if (entry->csid == charsetid)
		{
			b = *str;
			for (i = 0; (i < MAX_FIRSTBYTE_RANGE) &&
				entry->enc[i].bytes; i++)
			{
				if ((b >= entry->enc[i].range[0]) &&
					(b <= entry->enc[i].range[1]))
				{
					return entry->enc[i].columns;
				}
			}
		}
	}

	return 1;
}

/********************************************************
 Input:  int (int16) charsetid	Char Set ID
         char *pstr				Buffer which always point to Multibyte char
								first byte or normal single byte char
		 int  pos				byte position
 Output: 0,   if pos is not on kanji char
 		 1,   if pos is on kanji 1st byte
	     2,   if pos is on kanji 2nd byte
		 3,   if pos is on kanji 3rd byte
 Note:   Current this one only works for ShiftJis type multibyte not for JIS or EUC
*********************************************************/
int
INTL_NthByteOfChar(int charsetid, char *pstr, int pos)
{
	int	i;
	int	prev;

	pos--;

	if
	(
		(INTL_CharSetType(charsetid) == SINGLEBYTE) ||
		(!pstr)  ||
		(!*pstr) ||
		(pos < 0)
	)
	{
		return 0;
	}

	i = 0;
	prev = 0;
	while (pstr[i] && (i <= pos))
	{
		prev = i;
		i += INTL_CharLen(charsetid, (unsigned char *) &pstr[i]);
	}
	if (i <= pos)
	{
		return 0;
	}
	if (INTL_CharLen(charsetid, (unsigned char *) &pstr[prev]) < 2)
	{
		return 0;
	}

	return pos - prev + 1;
}

int
INTL_IsHalfWidth(MWContext *context, unsigned char *pstr)
{
	int	c;

	c = *pstr;

	switch (context->win_csid)
	{
	case CS_SJIS:
		if ((0xa1 <= c) && (c <= 0xdf))
		{
			return 1;
		}
		break;
	case CS_EUCJP:
		if (c == 0x8e)
		{
			return 1;
		}
		break;
	default:
		break;
	}

	return 0;
}

int16
GetMimeCSID(int16	win_csid, char	**mime_charset)
{
	csmime_t	*csmimep = csmime_tbl;

	while(csmimep->win_csid != 0) {
		if (win_csid == csmimep->win_csid) {
			*mime_charset = csmimep->mime_charset;
			return(csmimep->mime_csid);
		}
		csmimep++;
	}
	*mime_charset = "iso-8859-1";
	return(win_csid);			/* causes no conversion */
}

char *
INTL_CharSetDocInfo(MWContext *context)
{
	register int16			doc_csid = context->doc_csid;
	register csname2id_t	*csn2idp;
	char					*s = NULL;
	int						detected = 0;

	if (doc_csid == CS_DEFAULT) {
		doc_csid = INTL_DefaultDocCharSetID(context) & ~CS_AUTO;	/* Get CSID from prefs	*/
	} else if (doc_csid & CS_AUTO) {
		doc_csid &= ~CS_AUTO;				/* mask off bit for name lookup */
		detected = 1;
	} else {
		StrAllocCopy(s, context->mime_charset);	/* string from MIME header */

		if (doc_csid == CS_UNKNOWN)
				StrAllocCat(s, XP_GetString(XP_DOCINFO_1));
		return(s);
	}
			/* Look up name for default & autodetected CSIDs		*/
#ifdef XP_WIN
	csn2idp = &csname2id_tbl[1] ;	/* skip first default one	*/
	for (; *(csn2idp->cs_name) != '\0'; csn2idp++)
#else
	for (csn2idp = csname2id_tbl; *(csn2idp->cs_name) != '\0'; csn2idp++)
#endif
	{
 		if (doc_csid == csn2idp->cs_id) {
			StrAllocCopy(s, (char *)csn2idp->cs_name);
			if (detected)
				StrAllocCat(s, XP_GetString(XP_DOCINFO_2));
			else
				StrAllocCat(s, XP_GetString(XP_DOCINFO_3));
			return(s);
 		}
	}
	StrAllocCopy(s, context->mime_charset);	/* string from MIME header */
	StrAllocCat(s, XP_GetString(XP_DOCINFO_4));
	return (s);
}

#ifdef XP_WIN

/*
 This routine will change the default URL charset to
 newCharset, BTW newCharset is passed from UI.
*/
void
FE_ChangeURLCharset(const char *charset)
{
	csname2id_t	*csn2idp;
	char			*cp;

	if (charset == NULL)
		return;

	csn2idp = csname2id_tbl;

	cp = (char *)charset;
	if (cp)
		while (*cp != '\0') {
			*cp = tolower(*cp);
			cp++;
    	}

	while (*(csn2idp->cs_name) != '\0') {
 		if (strcmp(charset, (char *)csn2idp->cs_name) == 0) {
				INTL_ChangeDefaultCharSetID(csn2idp->cs_id);
 				return;
 			}
 			csn2idp++;
		}
}

void
INTL_ChangeDefaultCharSetID(int16 csid)
{
	csname2id_tbl[0].cs_id = csid;
}

#endif /* XP_WIN */

XP_Bool
INTL_CanAutoSelect(int16 csid)
{
	register cscvt_t		*cscvtp;

	cscvtp = cscvt_tbl;
	while (cscvtp->from_csid) {
		if (cscvtp->from_csid == csid) {
			if (cscvtp->autoselect)
				return TRUE;
			else
				return FALSE;
		}
		cscvtp++;
	}
	return FALSE;
}



PRIVATE int DecodeBase64 (const char *in, char *out)
{
  /* reads 4, writes 3. */
  int j;
  unsigned long num = 0;

  for (j = 0; j < 4; j++)
	{
	  unsigned char c;
	  if (in[j] >= 'A' && in[j] <= 'Z')		 c = in[j] - 'A';
	  else if (in[j] >= 'a' && in[j] <= 'z') c = in[j] - ('a' - 26);
	  else if (in[j] >= '0' && in[j] <= '9') c = in[j] - ('0' - 52);
	  else if (in[j] == '+')				 c = 62;
	  else if (in[j] == '/')				 c = 63;
	  else if (in[j] == '=')				 c = 0;
	  else
	  {
		/*	abort ();	*/
		strcpy(out, in);	/* I hate abort */
		return 0;
	  }
	  num = (num << 6) | c;
	}

  *out++ = (unsigned char) (num >> 16);
  *out++ = (unsigned char) ((num >> 8) & 0xFF);
  *out++ = (unsigned char) (num & 0xFF);
  return 1;
}

PRIVATE char *DecodeQP(char *in)
{
	int i = 0, length;
	char token[3];
	char *out, *dest = 0;

	out = dest = (char *)XP_ALLOC(strlen(in)+1);
	if (dest == NULL)
		return NULL;
	memset(out, 0, strlen(in)+1);
	length = strlen(in);
  	while (length > 0 || i != 0)
	{
	  	while (i < 3 && length > 0)
		{
		  	token [i++] = *in;
		  	in++;
		  	length--;
		}

	  	if (i < 3)
		{
		  /* Didn't get enough for a complete token.
			 If it might be a token, unread it.
			 Otherwise, just dump it.
			 */
			strncpy (out, token, i);
		  	break;
		}
	  	i = 0;

	  	if (token [0] == '=')
		{
		  	unsigned char c = 0;
		  	if (token[1] >= '0' && token[1] <= '9')
				c = token[1] - '0';
		  	else if (token[1] >= 'A' && token[1] <= 'F')
				c = token[1] - ('A' - 10);
		  	else if (token[1] >= 'a' && token[1] <= 'f')
				c = token[1] - ('a' - 10);
		  	else if (token[1] == CR || token[1] == LF)
			{
			  	/* =\n means ignore the newline. */
			  	if (token[1] == CR && token[2] == LF)
					;		/* swallow all three chars */
			  	else
				{
				  	in--;	/* put the third char back */
				  	length++;
				}
			  	continue;
			}
		  	else
			{
			  	/* = followed by something other than hex or newline -
				 pass it through unaltered, I guess.  (But, if
				 this bogus token happened to occur over a buffer
				 boundary, we can't do this, since we don't have
				 space for it.  Oh well.  Screw it.)  */
			  	if (in > out) *out++ = token[0];
			  	if (in > out) *out++ = token[1];
			  	if (in > out) *out++ = token[2];
			  	continue;
			}

		  	/* Second hex digit */
		  	c = (c << 4);
		  	if (token[2] >= '0' && token[2] <= '9')
				c += token[2] - '0';
		  	else if (token[2] >= 'A' && token[2] <= 'F')
				c += token[2] - ('A' - 10);
		  	else if (token[2] >= 'a' && token[2] <= 'f')
				c += token[2] - ('a' - 10);
		  	else
			{
			  	/* We got =xy where "x" was hex and "y" was not, so
				 treat that as a literal "=", x, and y.  (But, if
				 this bogus token happened to occur over a buffer
				 boundary, we can't do this, since we don't have
				 space for it.  Oh well.  Screw it.) */
			  	if (in > out) *out++ = token[0];
			  	if (in > out) *out++ = token[1];
			  	if (in > out) *out++ = token[2];
			  	continue;
			}

		  	*out++ = (char) c;
		}
	  	else
		{
		  	*out++ = token [0];

		  	token[0] = token[1];
		  	token[1] = token[2];
		  	i = 2;
		}
	}
	/* take care of special underscore case */
	for (out = dest; *out; out++)
		if (*out == '_') 	*out = ' ';
	return dest;
	
}

PRIVATE char *DecodeBase64Buffer(char *subject)
{
	char *output = 0;
	char *pSrc, *pDest ;
	int i ;

	StrAllocCopy(output, subject); /* Assume converted text are always 
									  less than source text */

	pSrc = subject;
	pDest = output ;
	for (i = strlen(subject); i > 3; i -= 4)
	{
		if (DecodeBase64(pSrc, pDest) == 0)
		{
			pSrc += 4;
			pDest += 4;
		}
		else
		{
			pSrc += 4;
			pDest += 3;
		}
	}
	
	*pDest = '\0';
	return output;
}

/* 
 ConvSubject
  This functions converts mail charset to Window charset for subject
	Syntax:   =?MimeCharset?[B|Q]?text?=
		MimeCharset = ISO-2022-JP
		              ISO-8859-1
					  ISO-8859-?
					  ....
		?B? :  Base64 Encoding          (used for multibyte encoding)
		?Q? :  Quote Printable Encoding (used for single byte encoding)

  	eg. for Japanese mail, it looks like
    	 =?ISO-2022-JP?B?   ........   ?=
*/
PUBLIC
char *IntlDecodeMimePartIIStr(const char *header, int wincsid, XP_Bool plaindecode)
{
    CCCDataObject	*obj;
	char *buffer = NULL, *convbuf = NULL;
	char *subject = NULL;
	char *p, *q, *text;
	char *begin, *end, *retbuff;
	int16	 csid = 0;
	int  len;
	int  ret = 0;
	int16 mailcsid = INTL_DefaultMailCharSetID(wincsid);

	if (header == 0 || *header == '\0')
		return NULL;
	if (wincsid == 0) /* Use global if undefined */
		wincsid = INTL_DefaultWinCharSetID(0);

	if (plaindecode == FALSE && wincsid == CS_GB_8BIT)
	{  /* for subject list pane, if it's GB, we only do HZ conversion */
		if (XP_STRSTR(header, "~{") == NULL)  
			return NULL;

    	obj = XP_NEW_ZAP(CCCDataObject);
		if (obj == NULL)
			return NULL;
		INTL_GetCharCodeConverter((int16)wincsid, (int16)wincsid, obj);
		convbuf = NULL;
		if (obj->cvtfunc)
			convbuf = (char*)obj->cvtfunc(obj, (unsigned char *)header, (int32)XP_STRLEN((char*)header));
		XP_FREE(obj);
		return convbuf;
	}
	if((! onlyAsciiStr(header) ) ||								/* it has 8 bit data, or */
	   ((mailcsid & STATEFUL) && (XP_STRCHR(header, '\033'))))	/* it is statefule encoding with esc */
	{	/* then we assume it is not mime part 2  encoding, we convert from the internet encoding to wincsid */
		if (plaindecode == FALSE)
		{      
			char* tmpbuf = NULL;
			/* Copy buf to tmpbuf, this guarantee the convresion won't overwrite the origional buffer and  */
			/* It will always return something it any conversion occcur */
			StrAllocCopy(tmpbuf, header);
			
			if(tmpbuf == NULL)
				return NULL;
			
    		obj = XP_NEW_ZAP(CCCDataObject);
			if (obj == NULL)
				return NULL;
			INTL_GetCharCodeConverter(mailcsid, (int16)wincsid, obj);
			convbuf = NULL;
			if (obj->cvtfunc)
				convbuf = (char*)obj->cvtfunc(obj, (unsigned char*)tmpbuf, (int32)XP_STRLEN((char*)tmpbuf));
			XP_FREE(obj);
			
			if(convbuf != tmpbuf)
				XP_FREE(tmpbuf);
			return convbuf;
		}
		return NULL;
		/* IMPORTANT NOTE: */
		/* Return NULL in this interface only mean ther are no conversion */
		/* It does not mean the conversion is store in the origional buffer */
		/* and the length is not change. This is differ from other conversion routine */
	}

    obj = XP_NEW_ZAP(CCCDataObject);
	if (obj == NULL)
		return NULL;


	StrAllocCopy(buffer, header);  /* temporary buffer */
	StrAllocCopy(subject, header);

	if (buffer == NULL || subject == NULL)
		return NULL;
	retbuff = subject;
	begin = buffer;

	while (*begin != '\0')
	{
		/*		GetCharset();		*/
		p = strstr(begin, "=?");
		if (p == NULL)
			break;
		subject += p - begin; /* skip strings don't need conversion */
		p += 2;
		q = strchr(p, '?');  /* Get charset info */
		if (q == NULL)
			break;
		*q++ = '\0';
		csid = INTL_CharSetNameToID(p);
		if (csid == CS_UNKNOWN)
		{
			/*
			 * @@@ may want to use context's default doc_csid in the future
			 */
			break;
		}

		if (*(q+1) == '?' &&
		    (*q == 'Q' || *q == 'q'))
		{			/* Quote printable  */
			p = strstr(q+2, "?=");
			if (p == NULL)
				break;
			*p = '\0';
			text = DecodeQP(q+2);
		}
 		else if (*(q+1) == '?' &&
		    (*q == 'B' || *q == 'b'))
		{			/* Base 64  */
			p = strstr(q+2, "?=");
			if (p == NULL)
				break;
			*p = '\0';
			text = DecodeBase64Buffer(q+2);
		}
		else
			break;
		if (text == NULL)
			break ;

		end = p + 2;
		if (plaindecode)
		{
           	/* move converted data to original buffer */
			ret = 1;
	    	len = strlen(text);
			XP_MEMCPY(subject, (char *)text, len);
			XP_MEMMOVE(subject+len, end, strlen(end)+1); /* move '\0' also */
			subject += len;
			XP_FREE(text);
		}
		else	/*		Convert to window encoding	*/
		{
			INTL_GetCharCodeConverter(csid, (int16)wincsid, obj);

			convbuf = NULL;
			if (obj->cvtfunc)
				convbuf = (char*)obj->cvtfunc(obj, (unsigned char *)text, strlen((char*)text));
			else if (csid == wincsid)
			{    /* pass through if no conversion needed */
				StrAllocCopy(convbuf, (const char *)text);
			}

			if (convbuf)
			{
            	/* move converted data to original buffer */
				ret = 1;
		    	len = strlen(convbuf);
				XP_MEMCPY(subject, (char *)convbuf, len);
				XP_MEMMOVE(subject+len, end, strlen(end)+1); /* move '\0' also */
				if (convbuf != text)
				{
					XP_FREE(convbuf);
				}
				subject += len;
				if(convbuf != text)
					XP_FREE(text);
			}
			else  /* shouldn't come here */
			{
				XP_FREE(text);
				break;
			}
		}
		begin = end;
	}
	XP_FREE(obj);
	if (buffer)
		XP_FREE(buffer);

	if (ret)
		return retbuff;
	else
	{
		XP_FREE(retbuff);
		return NULL;  /* null means no conversion */
	}
}



static char basis_64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
PRIVATE int EncodeBase64 (const char *in, char *out)
{
	unsigned char c1, c2, c3;
	c1 = in[0];
	c2 = in[1];
	c3 = in[2];

    *out++ = basis_64[c1>>2];
    *out++ = basis_64[((c1 & 0x3)<< 4) | ((c2 & 0xF0) >> 4)];

    *out++ = basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)];
    *out++ = basis_64[c3 & 0x3F];
	return 1;
}



PRIVATE char *EncodeBase64Buffer(char *subject)
{
	char *output = 0;
	char *pSrc, *pDest ;
	int i ;

	output = (char *)XP_ALLOC(strlen(subject) * 4 / 3 + 4);
	if (output == NULL)
		return NULL;

	pSrc = subject;
	pDest = output ;
	for (i = strlen(subject); i >= 3; i -= 3)
	{
		if (EncodeBase64(pSrc, pDest) == 0) /* error */
		{
			pSrc += 3;
			pDest += 3;
		}
		else
		{
			pSrc += 3;
			pDest += 4;
		}
	}
	/* in case (i % 3 ) == 1 or 2 */
	if(i > 0)
	{
		char in[3];
		int j;
		in[0] = in[1] = in[2] ='\0';
		for(j=0;j<i;j++)
			in[j] = *pSrc++;
		EncodeBase64(in, pDest);
		for(j=i+1;j<4;j++)
			pDest[j] = '=';
		pDest += 4;
	}
	*pDest = '\0';
	return output;
}



PRIVATE char *EncodeQPBuffer(char *subject)
{
	char *output = 0;
	unsigned char *p, *pDest ;
	int i, n, len ;

	if (subject == NULL || *subject == '\0')
		return NULL;
	len = strlen(subject);
	output = XP_ALLOC(len * 3 + 1);
	if (output == NULL)
		return NULL;

	p = (unsigned char*)subject;
	pDest = (unsigned char*)output ;

	for (i = 0; i < len; i++)
	{
		/* XP_IS_ALPHA(*p) || XP_IS_DIGIT(*p)) */
		if ((*p < 0x80) &&
			(((*p >= 'a') && (*p <= 'z')) ||
			 ((*p >= 'A') && (*p <= 'Z')) ||
			 ((*p >= '0') && (*p <= '9')))
		   )
			*pDest = *p;
		else
		{
			*pDest++ = '=';
			n = (*p & 0xF0) >> 4; /* high byte */
			if (n < 10)
				*pDest = '0' + n;
			else
				*pDest = 'A' + n - 10;
			pDest ++ ;

			n = *p & 0x0F;			/* low byte */
			if (n < 10)
				*pDest = '0' + n;
			else
				*pDest = 'A' + n - 10;
		}

		p ++;
		pDest ++;
	}

	*pDest = '\0';
	return output;
}

#define MAXLINELEN 72
#define IS_MAIL_SEPARATOR(p) ((*(p) == ',' || *(p) == ' ' || *(p) == '\"' || *(p) == ':' || \
  *(p) == '(' || *(p) == ')' || *(p) == '\\' || (unsigned char)*p < 0x20))

char *encode_next8bitword(int wincsid, char *src)
{
	char *p;
	XP_Bool non_ascii = FALSE;
	if (src == NULL)
		return NULL;
	p = src;
	while (*p == ' ')
		p ++ ;
	while ( *p )
	{
		if ((unsigned char) *p > 0x7F)
			non_ascii = TRUE;
		if ( IS_MAIL_SEPARATOR(p) )
		{
			break;
		}
		p = INTL_NextChar(wincsid, p);
	}

	if (non_ascii)
		return p;
	else
		return NULL;
}


char * encode_mail_address(int wincsid, const char *src, CCCDataObject *obj)
{
	char *begin, *end;
	char *retbuf = NULL, *srcbuf = NULL;
	char sep;
	char *name;
	int  retbufsize;
	int line_len = 0;
	int srclen;

	if (src == NULL || *src == '\0')
		return NULL;
	/* make a copy, so don't need touch original buffer */
	StrAllocCopy(srcbuf, src);
	if (srcbuf == NULL)
		return NULL;
	begin = srcbuf;
	GetMimeCSID ((int16)wincsid, &name);

	/* allocate enough buffer for conversion, this way it can avoid
	   do another memory allocation which is expensive
	 */
	   
	retbufsize = XP_STRLEN(srcbuf) * 3 + MAX_CSNAME + 8;
	retbuf =  XP_ALLOC(retbufsize);
	if (retbuf == NULL)  /* Give up if not enough memory */
	{
		XP_FREE(srcbuf);
		return NULL;
	}

	*retbuf = '\0';

	srclen = XP_STRLEN(srcbuf);
	while (begin < (srcbuf + srclen))
	{	/* get block of data between commas */
		char *p, *q;
		char *buf1, *buf2;
		int len, newsize, convlen, retbuflen;
		XP_Bool non_ascii;

		retbuflen = XP_STRLEN(retbuf);
		end = NULL;
		/* scan for separator, conversion happens on 8bit
		   word between separators
		 */
		if (IS_MAIL_SEPARATOR(begin))
		{   /*  skip white spaces and separator */
			q = begin;
			while (	IS_MAIL_SEPARATOR(q) )
				q ++ ;
			sep = *(q - 1);
			*(q - 1) = '\0';
			end = q - 1;
		}
		else
		{
			sep = '\0';
			/* scan for next separator */
			non_ascii = FALSE;
			for (q = begin; *q;)
			{
				if ((unsigned char) *q > 0x7F)
					non_ascii = TRUE;
				if ( IS_MAIL_SEPARATOR(q) )
				{
					if ((*q == ' ') && (non_ascii == TRUE))
					{
						while ((p = encode_next8bitword(wincsid, q)))
						{
							if (p == NULL)
								break;
							q = p;
							if (*p != ' ')
								break;
						}
					}
					sep = *q;
					*q = '\0';
					end = q;
					break;
				}
				q = INTL_NextChar(wincsid, q);
			}
		}

		len = XP_STRLEN(begin);
		if (onlyAsciiStr(begin) == 0)
		{
			if (obj && obj->cvtfunc)
			{
				buf1 = (char *) obj->cvtfunc(obj, (unsigned char *)begin, len);
				if (!buf1)
				{
					XP_FREE(srcbuf);
					XP_FREE(retbuf);
					return NULL;
				}
			}
			else
			{
				buf1 = XP_ALLOC(len + 1);
				if (!buf1)
				{
					XP_FREE(srcbuf);
					XP_FREE(retbuf);
					return NULL;
				}
				XP_MEMCPY(buf1, begin, len);
				*(buf1 + len) = '\0';
			}

			if (wincsid & MULTIBYTE)
			{
				/* converts to Base64 Encoding */
				buf2 = (char *)EncodeBase64Buffer(buf1);
			}
			else
			{
				/* Converts to Quote Printable Encoding */
				buf2 = (char *)EncodeQPBuffer(buf1);
			}


			if (buf1 && (buf1 != begin))
				XP_FREE(buf1);

			if (buf2 == NULL) /* QUIT if memory allocation failed */
			{
				XP_FREE(srcbuf);
				XP_FREE(retbuf);
				return NULL;
			}

			/* realloc memory for retbuff if necessary, 
			   7: =?...?B?..?=, 3: CR LF TAB */
			convlen = XP_STRLEN(buf2) + XP_STRLEN(name) + 7;
			newsize = convlen + retbuflen + 3 + 2;  /* 2:SEP '\0', 3:CRLFTAB */

			if (newsize > retbufsize)
			{
				char *tempbuf;
				tempbuf = XP_REALLOC(retbuf, newsize);
				if (tempbuf == NULL)  /* QUIT, if not enough memory left */
				{
					XP_FREE(buf2);
					XP_FREE(srcbuf);
					XP_FREE(retbuf);
					return NULL;
				}
				retbuf = tempbuf;
				retbufsize = newsize;
			}
			/* buf1 points to end of current retbuf */
			buf1 = retbuf + retbuflen; 

			if ((line_len > 10) && 
			    ((line_len + convlen) > MAXLINELEN))
			{
		  		*buf1++ = CR;
		  		*buf1++ = LF;
		  		*buf1++ = '\t';
				line_len = 0;
			}
			*buf1 = '\0';

			/* Add encoding tag for base62 and QP */
			XP_STRCAT(buf1, "=?");
			XP_STRCAT(buf1, name );
			if(wincsid & MULTIBYTE)
				XP_STRCAT(buf1, "?B?");
			else
				XP_STRCAT(buf1, "?Q?");
			XP_STRCAT(buf1, buf2);
			XP_STRCAT(buf1, "?=");

			line_len += convlen + 1;  /* 1: SEP */
			XP_FREE(buf2);	/* free base64 buffer */
		}
		else  /* if no 8bit data in the block */
		{
			newsize = retbuflen + len + 2 + 3; /* 2: ',''\0', 3: CRLFTAB */
			if (newsize > retbufsize)
			{
				char *tempbuf;
				tempbuf = XP_REALLOC(retbuf, newsize);
				if (tempbuf == NULL)
				{
					XP_FREE(srcbuf);
					XP_FREE(retbuf);
					return NULL;
				}
				retbuf = tempbuf;
				retbufsize = newsize;
			}
			buf1 = retbuf + retbuflen;

			if ((line_len > 10) && 
			    ((line_len + len) > MAXLINELEN))
			{
		  		*buf1++ = CR;
		  		*buf1++ = LF;
		  		*buf1++ = '\t';
				line_len = 0;
			}
			/* copy buffer from begin to buf1 stripping CRLFTAB */
			for (p = begin; *p; p++)
			{
				if (*p == CR || *p == LF || *p == TAB)
					len --;
				else
					*buf1++ = *p;
			}
			*buf1 = '\0';
			line_len += len + 1;  /* 1: SEP */
		}

		buf1 = buf1 + XP_STRLEN(buf1);
		if (sep == CR || sep == LF || sep == TAB) /* strip CR,LF,TAB */
			*buf1 = '\0';
		else
		{
			*buf1 = sep;
			*(buf1+1) = '\0';
		}

		if (end == NULL)
			break;
		begin = end + 1;
	}
	if (srcbuf)
		XP_FREE(srcbuf);
	return retbuf;
}


/*
	Latin1, latin2:
	   Source --> Quote Printable --> Encoding Info
	Japanese:
	   EUC,JIS,SJIS --> JIS --> Base64 --> Encoding Info
	Others:
	   No conversion  
    flag:   0:   8bit on
	        1:   mime_use_quoted_printable_p
	return:  NULL  if no conversion occured

*/
PUBLIC
char *IntlEncodeMimePartIIStr(char *subject, int wincsid, XP_Bool bUseMime)
{
	int iSrcLen;
	unsigned char *buf  = NULL;	/* Initial to NULL */
	int16 mail_csid;
   	CCCDataObject	*obj = NULL;
	char  *name;

	if (subject == NULL || *subject == '\0')
		return NULL;

	iSrcLen = XP_STRLEN(subject);
	if (wincsid == 0)
		wincsid = INTL_DefaultWinCharSetID(0) ;

	mail_csid = GetMimeCSID ((int16)wincsid, &name);
	
	/* check to see if subject are all ascii or not */
	if(onlyAsciiStr(subject))
		return NULL;
		
	if (mail_csid != wincsid)
	{
   		obj = XP_NEW_ZAP(CCCDataObject);
		if (obj == NULL)
			return 0;
		/* setup converter from wincsid --> mail_csid */
		INTL_GetCharCodeConverter((int16)wincsid, mail_csid, obj) ;
	}
	/* Erik said in the case of STATEFUL mail encoding, we should FORCE it to use */
	/* MIME Part2 to get ride of ESC in To: and CC: field, which may introduce more trouble */
	if((bUseMime) || (mail_csid & STATEFUL))/* call encode_mail_address */
	{
		buf = (unsigned char *)encode_mail_address(wincsid, subject, obj);
		if(buf == (unsigned char*)subject)	/* no encoding, set return value to NULL */
			buf =  NULL;
	}
	else
	{	/* 8bit, just do conversion if necessary */
		/* In this case, since the conversion routine may reuse the origional buffer */
		/* We better allocate one first- We don't want to reuse the origional buffer */
		
		if ((mail_csid != wincsid) && (obj->cvtfunc))
		{
			char* newbuf = NULL;
			/* Copy buf to newbuf */
			StrAllocCopy(newbuf, subject);
			if(newbuf != NULL)
			{
				buf = (unsigned char *)obj->cvtfunc(obj, (unsigned char*)newbuf, iSrcLen);
				if(buf != (unsigned char*)newbuf)
					XP_FREE(newbuf);
			}
		}
	}
	if (obj)   
		XP_FREE(obj);
	return (char*)buf;
	
	/* IMPORTANT NOTE: */
	/* Return NULL in this interface only mean ther are no conversion */
	/* It does not mean the conversion is store in the origional buffer */
	/* and the length is not change. This is differ from other conversion routine */
}


int16
INTL_DefaultTextAttributeCharSetID(MWContext *context)
{
	if (context && (context->win_csid))
	{
		return context->win_csid;
	}

	return INTL_DefaultWinCharSetID(context);
}


void
INTL_NumericCharacterReference(unsigned char *in, uint32 inlen, uint32 *inread,
	unsigned char *out, uint16 outbuflen, uint16 *outlen, Bool force)
{
	unsigned char	b;
#ifdef XP_MAC
	char		**cvthdl;
#endif  /* 'cuz it made Jamie unhappy */
	int		i;
	uint32		ncr;
	unsigned char	val;

	XP_ASSERT(outbuflen >= 10);

	for (i = 0; i < inlen; i++)
	{
		b = in[i];
		if ((b < '0') || (b > '9'))
		{
			break;
		}
	}
	if (i == 0)
	{
		*inread = 0;
		out[0] = 0;
		*outlen = 0;
		return;
	}
	if ((i >= inlen) && (!force))
	{
		*inread = (inlen + 1);
		out[0] = 0;
		*outlen = 0;
		return;
	}
	inlen = i;

	ncr = 0;
	for (i = 0; i < inlen; i++)
	{
		b = in[i];
		if ((b < '0') || ('9' < b))
		{
			break;
		}
		if (ncr < 256)
		{
			ncr = ((10 * ncr) + (b - '0'));
		}
	}
	*inread = i;

	if (ncr > 255)
	{
		val = (unsigned char) '?';
	}
	else
	{
		val = (unsigned char) ncr;
	}


#ifdef XP_MAC

#define xlat_LATIN1_TO_MAC_ROMAN	 128

	cvthdl = INTL_GetSingleByteTable(CS_LATIN1, CS_MAC_ROMAN,
		xlat_LATIN1_TO_MAC_ROMAN);
	if (cvthdl)
	{
		val = ((unsigned char) ((*cvthdl)[val]));
		INTL_FreeSingleByteTable(cvthdl);
	}

#endif /*XP_MAC*/


#ifdef USE_NPC

	out[0] = CSIDToNPC(CS_LATIN1);
	out[1] = val;
	*outlen = 2;

#else /* USE_NPC */

	out[0] = val;
	*outlen = 1;

#endif /* USE_NPC */

}


void
INTL_EntityReference(unsigned char val, unsigned char *out, uint16 outbuflen,
	uint16 *outlen)
{
	XP_ASSERT(outbuflen >= 10);

#ifdef USE_NPC

	out[0] = CSIDToNPC(CS_LATIN1);
	out[1] = val;
	*outlen = 2;

#else /* USE_NPC */

	out[0] = val;
	*outlen = 1;

#endif /* USE_NPC */

}


#ifndef USE_NPC

void
INTL_FreeFontEncodingList(INTL_FontEncodingList *list)
{
	if (list)
	{
		XP_FREE(list);
	}
}


INTL_FontEncodingList *
INTL_InternalToFontEncoding(int16 charset, unsigned char *text)
{
	INTL_FontEncodingList	*ret;

	ret = XP_NEW_ZAP(INTL_FontEncodingList);
	if (!ret)
	{
		return NULL;
	}

	ret->next = NULL;
	ret->charset = charset;
	ret->text = text;

	return ret;
}


#else /* USE_NPC */


void
INTL_FreeFontEncodingList(INTL_FontEncodingList *list)
{
	INTL_FontEncodingList	*next;

	while (list)
	{
		next = list->next;
		if (list->text)
		{
			XP_FREE(list->text);
		}
		XP_FREE(list);
		list = next;
	}
}


INTL_FontEncodingList *
INTL_InternalToFontEncoding(int16 charset, unsigned char *text)
{
	INTL_FontEncodingList	*current;
	unsigned char		*in;
	uint16			inlen;
	uint16			inread;
	unsigned char		*out;
	uint16			outbuflen;
	int16			outcsid;
	uint16			outlen;
	INTL_FontEncodingList	**prev;
	INTL_FontEncodingList	*ret;

	if (!text)
	{
		return NULL;
	}

	in = text;
	while (*in++)
	{
		;
	}
	inlen = in - text;

	ret = NULL;
	prev = &ret;

	in = text;
	while (*in)
	{
		current = XP_NEW_ZAP(INTL_FontEncodingList);
		if (!current)
		{
			INTL_FreeFontEncodingList(ret);
			return NULL;
		}

		/*
		 * @@@ allocating too much? -- erik
		 */
		outbuflen = inlen + 1;
		out = XP_ALLOC(outbuflen);
		if (!out)
		{
			XP_FREE(current);
			INTL_FreeFontEncodingList(ret);
			return NULL;
		}

		if (INTL_ConvertNPC(in, inlen, &inread, out, outbuflen,
			&outlen, &outcsid))
		{
			in += inread;
			inlen -= inread;

			out[outlen] = 0;

			current->next = NULL;
			current->charset = outcsid;
			current->text = out;

			*prev = current;
			prev = &current->next;
		}
		else
		{
			break;
		}
	}

	return ret;
}

#endif /* USE_NPC */


void
INTL_ReportFontCharSets(int16 *charsets)
{
	uint16	len;

	if (!charsets)
	{
		return;
	}

	if (availableFontCharSets)
	{
		free(availableFontCharSets);
	}

	availableFontCharSets = charsets;

	while (*charsets)
	{
		if (*charsets == CS_X_BIG5)
		{
			haveBig5 = 1;
		}
		charsets++;
	}
	len = (charsets - availableFontCharSets);

#ifdef XP_UNIX
	INTL_SetUnicodeCSIDList(len, availableFontCharSets);

#ifdef UNICODE_INIT_TEST
	{
		extern void npctest(void);

		npctest();
	}
#endif /* UNICODE_INIT_TEST */

#endif

}
/*	
	INTL_NextCharIdxInText 
		Input: 	csid - window csid	
				text - point to a text buffer
				pos  - origional index position
		output: index of the position of next character
		Called by lo_next_character in layfind.c
*/
int INTL_NextCharIdxInText(int16 csid, unsigned char *text, int pos)
{
	return pos + INTL_CharLen(csid ,text+pos);
}
/*	
	INTL_PrevCharIdxInText 
		Input: 	csid - window csid	
				text - point to a text buffer
				pos  - origional index position
		output: index of the position of previous character
		Called by lo_next_character in layfind.c
*/
int INTL_PrevCharIdxInText(int16 csid, unsigned char *text, int pos)
{
	int rev, ff , thislen;
	if((INTL_CharSetType(csid) == SINGLEBYTE) ) {
		return pos - 1;
	}
	else 
	{
		/*	First, backward to character in ASCII range */
		for(rev=pos - 1; rev > 0 ; rev--)
		{
			if(((text[rev] & 0x80 ) == 0) &&
			   ((rev + INTL_CharLen(csid ,text+rev)) < pos))
				break;
		}
			
		/*	Then forward till we cross the position. */
		for(ff = rev ; ff < pos ; ff += thislen)
		{
			thislen = INTL_CharLen(csid ,text+ff);
			if((ff + thislen) >= pos)
				break;
		}
		return ff;
	}
}

/*	
	INTL_NextCharIdx
		Input: 	csid - window csid	
				text - point to a text buffer
				pos  - 0 based position
		output: 0 based next char position
		Note: this one works for any position no matter it's legal or not
*/

int INTL_NextCharIdx(int16 csid, unsigned char *str, int pos)
{
	int n;
	unsigned char *p;

	if((INTL_CharSetType(csid) == SINGLEBYTE) || (pos < 0))
		return pos + 1;

	n = INTL_NthByteOfChar(csid, (char *) str, pos+1);
	if (n == 0)
		return pos + 1;

	p = str + pos - n + 1;
	return pos + INTL_CharLen(csid, p) - n + 1;
}
/*	
	INTL_PrevCharIdx
		Input: 	csid - window csid	
				text - point to a text buffer
				pos  - 0 based position
		output: 0 based prev char position
		Note: this one works for any position no matter it's legal or not
*/
int INTL_PrevCharIdx(int16 csid, unsigned char *str, int pos)
{
	int n;
	if((INTL_CharSetType(csid) == SINGLEBYTE) || (pos <= 0))
		return pos - 1;
#ifdef DEBUG
	n = INTL_NthByteOfChar(csid, (char *) str, pos+1);
	if (n > 1)
	{
		XP_TRACE(("Wrong position passed to INTL_PrevCharIdx"));
		pos -= (n - 1); 
	}
#endif

	pos --;
	if ((n = INTL_NthByteOfChar(csid, (char *) str, pos+1)) > 1)
		return pos - n + 1;
	else
		return pos;
}

#define INTL_SingleByteToLower(lower, ch)	((ch & 0x80) ? (lower[(ch & 0x7f)]) : (lower_lookup_ascii[ch]))
/*
static unsigned char lower_lookup_no_change[256]={
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
*/
/*	
	lower_lookup_ascii  map 
		0x41-0x5a to 0x61-0x7a
*/
static unsigned char lower_lookup_ascii[128]={
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
	0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
};
#ifndef XP_MAC
/*	
	lower_lookup_latin1 map 
	(2) 0xc0-0xd6 to 0xe0-0xf6
	(3) 0xd8-0xde to 0xf8-0xfe
*/
static unsigned char lower_lookup_latin1[128]={
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xd7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
/*	
	lower_lookup_latin2  map 
	(2) 0xa1 to 0xb1
	(3) 0xa3 to 0xb3
	(4) 0xa5-0xa6 to 0xb5-0xb6
	(5) 0xa9-0xac to 0xb9-0xbc
	(6) 0xae-0xaf to 0xbe-0xbf
	(7) 0xc0-0xd6 to 0xe0-0xf6
	(8) 0xd8-0xde to 0xf8-0xfe
*/
static unsigned char lower_lookup_latin2[128]={
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xb1, 0xa2, 0xb3, 0xa4, 0xb5, 0xb6, 0xa7, 0xa8, 0xb9, 0xba, 0xbb, 0xbc, 0xad, 0xbe, 0xbf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xd7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
/*	
	lower_lookup_macicelandic map 
		0x80 to 0x8a
		0x81-0x83 to 0x8c-0x8e
		0x84 to 0x96
		0x85 to 0x9a
		0x86 to 0x9f
		0xa0 to 0xe0
		0xae-0xaf to 0xbe-0xbf
		0xcb to 0x88
		0xcc to 0x8b
		0xcd to 0x9b
		0xce to 0xcf
		0xd9 to 0xd8
		0xdc to 0xdd
		0xde to 0xdf
		0xe5 to 0x89
		0xe6 to 0x90
		0xe7 to 0x87
		0xe8 to 0x91
		0xe9 to 0x8f
		0xea to 0x92
		0xeb to 0x94
		0xec to 0x95
		0xed to 0x93
		0xee to 0x97
		0xef to 0x99
		0xf1 to 0x98
		0xf2 to 0x9c
		0xf3 to 0x9e
		0xf4 to 0x9d
*/
#else /* ifndef XP_MAC */
static unsigned char lower_lookup_macicelandic[128]={
	0x8a, 0x8c, 0x8d, 0x8e, 0x96, 0x9a, 0x9f, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xe0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xbe, 0xbf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0x88, 0x8b, 0x9b, 0xcf, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd8, 0xda, 0xdb, 0xdd, 0xdd, 0xdf, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0x89, 0x90, 0x87, 0x91, 0x8f, 0x92, 0x94, 0x95, 0x93, 0x97, 0x99,
	0xf0, 0x98, 0x9c, 0x9e, 0x9d, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
/*	
	lower_lookup_macce  map 
			0x80 to 0x8a
			0x81 to 0x82
			0x83 to 0x8e
			0x84 to 0x88
			0x85 to 0x9a
			0x86 to 0x9f
			0x89 to 0x8b
			0x8c to 0x8d
			0x8f to 0x90
			0x91 to 0x93
			0x94 to 0x95
			0x96 to 0x98
			0x9d to 0x9e
			0xa2 to 0xab
			0xaf to 0xb0
			0xb1 to 0xb4
			0xb5 to 0xfa
			0xb9 to 0xba
			0xbb to 0xbc
			0xbd to 0xbe
			0xbf to 0xc0
			0xc1 to 0xc4
			0xc5 to 0xcb
			0xcc to 0xce
			0xcd to 0x9b
			0xcf to 0xd8
			0xd9 to 0xda
			0xdb to 0xde
			0xdf to 0xe0
			0xe1 to 0xe4
			0xe5 to 0xe6
			0xe7 to 0x87
			0xe8 to 0xe9
			0xea to 0x92
			0xeb to 0xec
			0xed to 0xf0
			0xee to 0x97
			0xef to 0x99
			0xf1 to 0xf3
			0xf2 to 0x9c
			0xf4 to 0xf5
			0xf6 to 0xf7
			0xf8 to 0xf9
			0xfb to 0xfd
			0xfc to 0xb8
			0xfe to 0xae
*/
static unsigned char lower_lookup_macce[128]={
	0x8a, 0x82, 0x82, 0x8e, 0x88, 0x9a, 0x9f, 0x87, 0x88, 0x8b, 0x8a, 0x8b, 0x8d, 0x8d, 0x8e, 0x90,
	0x90, 0x93, 0x92, 0x93, 0x95, 0x95, 0x98, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xab, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xae, 0xae, 0xb0,
	0xb0, 0xb4, 0xb2, 0xb3, 0xb4, 0xfa, 0xb6, 0xb7, 0xb8, 0xba, 0xba, 0xbc, 0xbc, 0xbe, 0xbe, 0xc0,
	0xc0, 0xc4, 0xc2, 0xc3, 0xc4, 0xcb, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xce, 0x9b, 0xce, 0xd8,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xda, 0xda, 0xde, 0xdc, 0xdd, 0xde, 0xe0,
	0xe0, 0xe4, 0xe2, 0xe3, 0xe4, 0xe6, 0xe6, 0x87, 0xe9, 0xe9, 0x92, 0xec, 0xec, 0xf0, 0x97, 0x99,
	0xf0, 0xf3, 0x9c, 0xf3, 0xf5, 0xf5, 0xf7, 0xf7, 0xf9, 0xf9, 0xfa, 0xfd, 0xb8, 0xfd, 0xae, 0xff
};
#endif /* ifndef XP_MAC */

#ifndef XP_UNIX
/*	
	lower_lookup_jis0201  map 
		0xa7-0xab to 0xb1-0xb5
		0xac-0xae to 0xd4-0xd6
		0xaf to 0xc2
*/
static unsigned char lower_lookup_jis0201[128]={
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xd4, 0xd5, 0xd6, 0xc2,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#endif

#ifdef XP_MAC
/*
	lower_lookup_mac_cyrillic
		0x80-0x9e to 0xe0-0xfe		A - YU
		0x9F	  to 0xDF			YA
		0xA7	  to 0xb4			BYELORUSSIAN-UKRAINIAN I
		0xAb	  to 0xAC			DJE
		0xae	  to 0xaf			gje
		0xb7	  to 0xc0			je
		0xb8	  to 0xb9			ie
		0xba	  to 0xbb			yi
		0xbc	  to 0xbc			lje
		0xbd	  to 0xbd			nje
		0xc1	  to 0xcf			dze
		0xcb	  to 0xcc			tshe
		0xcd	  to 0xce			kje
		0xd8	  to 0xd9			short u
		0xda	  to 0xdb			dzhe
		0xdd	  to 0xde			dzhe
*/
static unsigned char lower_lookup_mac_cyrillic[128]={
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xdf,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xb4, 0xa8, 0xa9, 0xaa, 0xac, 0xac, 0xad, 0xaf, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xc0, 0xb9, 0xb9, 0xbb, 0xbb, 0xbd, 0xbd, 0xbf, 0xbf,
	0xc0, 0xcf, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcc, 0xcc, 0xce, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd9, 0xd9, 0xdb, 0xdb, 0xdc, 0xde, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#endif
#ifdef XP_UNIX
/*	
	lower_lookup_8859_5  map 
		0xa1-0xac to 0xf1-0xfc
		0xae-0xaf to 0xfe-0xff
		0xb0-0xcf to 0xd0-0xef
*/
static unsigned char lower_lookup_8859_5[128]={
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xad, 0xfe, 0xff,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#endif
#ifdef XP_WIN
/*	
	lower_lookup_cp1251  map 
		0x80		 0x90
		0x81		 0x83
		0x8a		 0x9a
		0x8c-0x8f	 0x9c-0x9f
		0xa1		 0xa2
		0xa3		 0xbc
		0xa5		 0xb4
		0xa8		 0xb8
		0xaa		 0xba
		0xaf		 0xbf
		0xb2		 0xb3
		0xbd		 0xbe
*/
static unsigned char lower_lookup_cp1251[128]={
	0x90, 0x83, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x9a, 0x8b, 0x9c, 0x9d, 0x9e, 0x9f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa2, 0xa2, 0xbc, 0xa4, 0xb4, 0xa6, 0xa7, 0xb8, 0xa9, 0xba, 0xab, 0xac, 0xad, 0xae, 0xbf,
	0xb0, 0xb1, 0xb3, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbe, 0xbe, 0xbf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#endif
#ifdef XP_MAC
/*	
	lower_lookup_mac_greek  map 
		0x80		to	0x8a
		0x8e		to 	0x8e
		0x85		to 	0x9a
		0x86		to 	0x9f
		0xa1		to 	0xe7
		0xa2		to 	0xe4
		0xa3		to 	0xf5
		0xa4		to 	0xec
		0xa5		to 	0xea
		0xa6		to 	0xf0
		0xaa		to 	0xf3
		0xab		to 	0xfb
		0xb0		to 	0xe1		
		0xb5		to 	0xe2
		0xb6		to 	0xe5
		0xb7		to 	0xfa
		0xb8		to 	0xe8
		0xb9		to 	0xe9
		0xba		to 	0xeb
		0xbb		to 	0xed
		0xbc		to 	0xe6
		0xbd		to 	0xfc
		0xbe		to 	0xe3
		0xbf		to 	0xf6		
		0xc1		to	0xee
		0xc3		to	0xef
		0xc4		to	0xf2
		0xc6		to	0xf4		
		0xcb		to	0xf9
		0xcc		to	0xf8
		0xcd		to	0xc0
		0xce		to	0xdb

		0xd7-0xd9	to	0xdc-0xde
		0xda		to	0xe0
		0xdf		to	0xf1
		
		
*/
static unsigned char lower_lookup_mac_greek[128]={
	0x8a, 0x81, 0x82, 0x8e, 0x84, 0x9a, 0x9f, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xe7, 0xe4, 0xf5, 0xec, 0xea, 0xf0, 0xa7, 0xa8, 0xa9, 0xf3, 0xfb, 0xac, 0xad, 0xae, 0xaf,
	0xe1, 0xb1, 0xb2, 0xb3, 0xb4, 0xe2, 0xe5, 0xfa, 0xe8, 0xe9, 0xeb, 0xed, 0xe6, 0xfc, 0xe3, 0xf6,
	0xc0, 0xee, 0xc2, 0xef, 0xf2, 0xc5, 0xf4, 0xc7, 0xc8, 0xc9, 0xca, 0xf9, 0xf8, 0xc0, 0xdb, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xdc, 0xdd, 0xde, 0xe0, 0xdb, 0xdc, 0xdd, 0xde, 0xf1,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#endif
#ifdef XP_UNIX
/*	
	lower_lookup_8859_7  map 
		0xb6	  to 0xdc
		0xb8-0xba to 0xdd-0xdf
		0xbc	  to 0xfc
		0xbe-0xbf to 0xfd-0xfe
		0xc1-0xdb to 0xe1-0xfb
*/
static unsigned char lower_lookup_8859_7[128]={
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xdc, 0xb7, 0xdd, 0xde, 0xdf, 0xbb, 0xfc, 0xbd, 0xfd, 0xfe,
	0xc0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#endif
#ifdef XP_WIN
/*	
	The only differences between these two lower_lookup_8859_7
	( The differences in these two table. I am not saying the difference between these two codeset)
		is 0xb6 and 0xa2
	lower_lookup_8859_7  map 
		0xa2	  to 0xdc
		0xb8-0xba to 0xdd-0xdf
		0xbc	  to 0xfc
		0xbe-0xbf to 0xfd-0xfe
		0xc1-0xdb to 0xe1-0xfb
*/
static unsigned char lower_lookup_8859_7[128]={
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xdc, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xdd, 0xde, 0xdf, 0xbb, 0xfc, 0xbd, 0xfd, 0xfe,
	0xc0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xdc, 0xdd, 0xde, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#endif

#ifdef XP_WIN
/*
	lower_lookup_cp1250  map 
	
*/
static unsigned char lower_lookup_cp1250[128]={
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x9a, 0x8b, 0x9c, 0x9d, 0x9e, 0x9f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xb3, 0xa4, 0xb9, 0xa6, 0xa7, 0xa8, 0xa9, 0xba, 0xab, 0xac, 0xad, 0xae, 0xbf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbe, 0xbd, 0xbe, 0xbf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xd7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#endif
#ifndef XP_MAC
/*
	lower_lookup_8859_9  map 
	
	PROBLEM!!!!! Need more information !!!!
	Currently I map 0xdd to 0xfd for tolower
	0xDD is LATIN CAPITAL LETTER I WITH DOT ABOVE
	0xFD is LATIN SMALL LETTER DOTLESS I
	
	Should I do that ?
	
*/
static unsigned char lower_lookup_8859_9[128]={
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x9a, 0x8b, 0x9c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0xff,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xd7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
	0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#else
/*
	lower_lookup_mac_turkish  map 
	PROBLEM!!!!! Need more information !!!!
	Currently I map 0xdc to 0xdd for tolower
	0xDC is LATIN CAPITAL LETTER I WITH DOT ABOVE
	0xDD is LATIN SMALL LETTER DOTLESS I
	
	Should I do that ?
*/
static unsigned char lower_lookup_mac_turkish[128]={
	0x8a, 0x8c, 0x8d, 0x8e, 0x96, 0x9a, 0x9f, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xbe, 0xbf,
	0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0x88, 0xcb, 0x9b, 0xcf, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd8, 0xdb, 0xdb, 0xdd, 0xdd, 0xdf, 0xdf,
	0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0x89, 0x90, 0x87, 0x91, 0x8f, 0x92, 0x94, 0x95, 0x93, 0x97, 0x99,
	0xf0, 0x98, 0x9c, 0x9e, 0x9d, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
};
#endif
/*
	lower_lookup_koi8_r  map 
*/
static unsigned char lower_lookup_koi8_r[128]={
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
	0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
	0xb0, 0xb1, 0xb1, 0xa3, 0xa4, 0xb5, 0xa6, 0xa7, 0xb8, 0xb9, 0xba, 0xbb, 0xbd, 0xbd, 0xbe, 0xbf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
	0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
	0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf
};

unsigned char *INTL_GetSingleByteToLowerMap(int16 csid)
{
	switch(csid)
	{
#ifdef XP_MAC
		case CS_MAC_ROMAN:
			return lower_lookup_macicelandic;
		case CS_MAC_CE:
			return lower_lookup_macce;
		case CS_MAC_CYRILLIC:
			return lower_lookup_mac_cyrillic;
		case CS_MAC_GREEK:
			return lower_lookup_mac_greek;
		case CS_MAC_TURKISH:	
			return lower_lookup_mac_turkish;
		case CS_SJIS:
			return lower_lookup_jis0201;
#endif
#ifdef XP_WIN
		case CS_LATIN1:
			return lower_lookup_latin1;
		case CS_CP_1250:	
			return lower_lookup_cp1250;
		case CS_CP_1251:
			return lower_lookup_cp1251;
		case CS_8859_7:
			return lower_lookup_8859_7;
		case CS_8859_9:	
			return lower_lookup_8859_9;
		case CS_SJIS:
			return lower_lookup_jis0201;
#endif
#ifdef XP_UNIX
		case CS_LATIN1:
			return lower_lookup_latin1;
		case CS_LATIN2:
			return lower_lookup_latin2;
		case CS_8859_5:
			return lower_lookup_8859_5;
		case CS_8859_7:
			return lower_lookup_8859_7;
		case CS_8859_9:	
			return lower_lookup_8859_9;
#endif	
		case CS_KOI8_R:
			return lower_lookup_koi8_r;
		/* currently, return the ascii table. We should supply table for this csid later */
		
		case CS_8859_3:	
		case CS_8859_4:		
		case CS_8859_6:
		case CS_8859_8:
		case CS_SYMBOL:
		case CS_DINGBATS:
		case CS_TIS620:
		case CS_JISX0201:
		
		/* use the ascii table */
		case CS_DEFAULT:
		case CS_ASCII:
		case CS_BIG5:		
		case CS_CNS_8BIT:		
		case CS_GB_8BIT:	
		case CS_KSC_8BIT:	
		default:
			return lower_lookup_ascii;
	}
}
	/*	ShiftJIS	*/	
typedef struct {
	unsigned char src_b1;
	unsigned char src_b2_start;
	unsigned char src_b2_end;
	unsigned char dest_b1;
	unsigned char dest_b2_start;
} DoubleByteToLowerMap;

static DoubleByteToLowerMap lower_lookup_none[]=
{	
	{ 0x00, 0x00, 0x00, 0x00, 0x00 }	/* Terminator */
};

#ifndef XP_UNIX
static DoubleByteToLowerMap lower_lookup_sjis[]=
{	
	/*	Full-width Latin */
	{ 0x82, 0x60, 0x7a, 0x82, 0x81 },	
	
	/*	Map Full-width Hiragana to  Full-width Katakana */
	{ 0x82, 0x9f, 0x9f, 0x83, 0x41 },	/*	A	*/
	{ 0x82, 0xa0, 0xa0, 0x83, 0x41 },

	{ 0x82, 0xa1, 0xa1, 0x83, 0x43 },	/*	I	*/
	{ 0x82, 0xa2, 0xa2, 0x83, 0x43 },

	{ 0x82, 0xa3, 0xa3, 0x83, 0x45 },	/*	U	*/
	{ 0x82, 0xa4, 0xa4, 0x83, 0x45 },

	{ 0x82, 0xa5, 0xa5, 0x83, 0x47 },	/*	E	*/
	{ 0x82, 0xa6, 0xa6, 0x83, 0x47 },

	{ 0x82, 0xa7, 0xa7, 0x83, 0x49 },	/*	O	*/
	{ 0x82, 0xa8, 0xa8, 0x83, 0x49 },
	
	{ 0x82, 0xa9, 0xc0, 0x83, 0x4a },	/* KA - DI */

	{ 0x82, 0xc1, 0xc1, 0x83, 0x63 },	/* TU */
	{ 0x82, 0xc2, 0xc2, 0x83, 0x63 },

	{ 0x82, 0xc3, 0xdd, 0x83, 0x64 },	/* DU - MI */
	{ 0x82, 0xde, 0xe0, 0x83, 0x80 },	/* MU - MO */

	{ 0x82, 0xe1, 0xe1, 0x83, 0x84 },	/* YA	*/
	{ 0x82, 0xe2, 0xe2, 0x83, 0x84 },

	{ 0x82, 0xe3, 0xe3, 0x83, 0x86 },	/* YU	*/
	{ 0x82, 0xe4, 0xe4, 0x83, 0x86 },

	{ 0x82, 0xe5, 0xe5, 0x83, 0x88 },	/* YO	*/
	{ 0x82, 0xe6, 0xe6, 0x83, 0x88 },

	{ 0x82, 0xe7, 0xeb, 0x83, 0x89 },	/* RA - RO */

	{ 0x82, 0xec, 0xec, 0x83, 0x8f },	/* WA	*/
	{ 0x82, 0xed, 0xed, 0x83, 0x8f },

	{ 0x82, 0xee, 0xf1, 0x83, 0x90 },	/* WI - N */

	/*	Map SMALL Full-width Katakana to Full-width Katakana */
	{ 0x83, 0x40, 0x40, 0x83, 0x41 },	/*	A	*/
	{ 0x83, 0x42, 0x42, 0x83, 0x43 },	/*	I	*/
	{ 0x83, 0x43, 0x44, 0x83, 0x45 },	/*	U	*/
	{ 0x83, 0x46, 0x46, 0x83, 0x47 },	/*	E	*/
	{ 0x83, 0x48, 0x47, 0x83, 0x49 },	/*	O	*/
	{ 0x83, 0x62, 0x62, 0x83, 0x63 },	/* TU	*/
	{ 0x83, 0x83, 0x83, 0x83, 0x84 },	/* YA	*/
	{ 0x83, 0x85, 0x85, 0x83, 0x86 },	/* YU	*/
	{ 0x83, 0x87, 0x87, 0x83, 0x88 },	/* YO	*/
	{ 0x83, 0x8e, 0x8e, 0x83, 0x8f },	/* WA	*/

	/*	Full-width Greek */
	{ 0x83, 0x9f, 0xb6, 0x83, 0xbf },	
	
	/*	Full-width Cyrillic*/
	{ 0x84, 0x40, 0x4e, 0x84, 0x70 },	/* Part 1 */
	{ 0x84, 0x4f, 0x61, 0x84, 0x80 },	/* Part 2 */
	
	{ 0x00, 0x00, 0x00, 0x00, 0x00 }	/* Terminator */
};
#endif
/* Don't #ifdef XP_UNIX for lower_lookup_eucjp. It is also used by GB2312 */
static DoubleByteToLowerMap lower_lookup_eucjp[]=
{	
	/*	Half-width Katakana */
	{ SS2,  0xa6, 0xa6, 0xa5, 0xf2 },	/*	WO */
	
	{ SS2,  0xa7, 0xa7, 0xa5, 0xa2 },	/*	A */
	{ SS2,  0xa8, 0xa8, 0xa5, 0xa4 },	/*	I */
	{ SS2,  0xa9, 0xa9, 0xa5, 0xa6 },	/*	U */
	{ SS2,  0xaa, 0xaa, 0xa5, 0xa8 },	/*	E */
	{ SS2,  0xab, 0xab, 0xa5, 0xaa },	/*	O */
	
	{ SS2,  0xac, 0xac, 0xa5, 0xe4 },	/*	YA */
	{ SS2,  0xad, 0xad, 0xa5, 0xe6 },	/*	YU */
	{ SS2,  0xae, 0xae, 0xa5, 0xe8 },	/*	YO */
	{ SS2,  0xaf, 0xaf, 0xa5, 0xc4 },	/*	TU */
	
	{ SS2,  0xb1, 0xb1, 0xa5, 0xa2 },	/*	A */
	{ SS2,  0xb2, 0xb2, 0xa5, 0xa4 },	/*	I */
	{ SS2,  0xb3, 0xb3, 0xa5, 0xa6 },	/*	U */
	{ SS2,  0xb4, 0xb4, 0xa5, 0xa8 },	/*	E */
	{ SS2,  0xb5, 0xb5, 0xa5, 0xaa },	/*	O */
	
	{ SS2,  0xb6, 0xb6, 0xa5, 0xab },	/*	KA */
	{ SS2,  0xb7, 0xb7, 0xa5, 0xad },	/*	KI */
	{ SS2,  0xb8, 0xb8, 0xa5, 0xaf },	/*	KU */
	{ SS2,  0xb9, 0xb9, 0xa5, 0xb1 },	/*	KE */
	{ SS2,  0xba, 0xba, 0xa5, 0xb3 },	/*	KO */
	
	{ SS2,  0xbb, 0xbb, 0xa5, 0xb5 },	/*	SA */
	{ SS2,  0xbc, 0xbc, 0xa5, 0xb7 },	/*	SI */
	{ SS2,  0xbd, 0xbd, 0xa5, 0xb9 },	/*	SU */
	{ SS2,  0xbe, 0xbe, 0xa5, 0xbb },	/*	SE */
	{ SS2,  0xbf, 0xbf, 0xa5, 0xbd },	/*	SO */
	
	{ SS2,  0xc0, 0xc0, 0xa5, 0xbf },	/*	TA */
	{ SS2,  0xc1, 0xc1, 0xa5, 0xc1 },	/*	TI */
	{ SS2,  0xc2, 0xc2, 0xa5, 0xc4 },	/*	TU */
	{ SS2,  0xc3, 0xc3, 0xa5, 0xc6 },	/*	TE */
	{ SS2,  0xc4, 0xc4, 0xa5, 0xc8 },	/*	TO */
	
	{ SS2,  0xc5, 0xc9, 0xa5, 0xca },	/*	NA - NO */
	
	{ SS2,  0xca, 0xca, 0xa5, 0xcf },	/*	HA */
	{ SS2,  0xcb, 0xcb, 0xa5, 0xd2 },	/*	HI */
	{ SS2,  0xcc, 0xcc, 0xa5, 0xd5 },	/*	HU */
	{ SS2,  0xcd, 0xcd, 0xa5, 0xd8 },	/*	HE */
	{ SS2,  0xce, 0xce, 0xa5, 0xdb },	/*	HO */
	
	{ SS2,  0xcf, 0xd3, 0xa5, 0xde },	/*	MA - MO */
	
	{ SS2,  0xd4, 0xd4, 0xa5, 0xe4 },	/*	YA */
	{ SS2,  0xd5, 0xd5, 0xa5, 0xe6 },	/*	YU */
	{ SS2,  0xd6, 0xd6, 0xa5, 0xe8 },	/*	YO */

	{ SS2,  0xd7, 0xdb, 0xa5, 0xe9 },	/*	RA - RO */

	{ SS2,  0xdc, 0xdc, 0xa5, 0xef },	/*	WA */

	{ SS2,  0xdd, 0xdd, 0xa5, 0xf3 },	/*	N */
	
	/*	Full-width Latin */
	{ 0xa3, 0xc1, 0xda, 0xa3, 0xe1 },	
	
	/*	Map Full-width Hiragana to  Full-width Katakana */
	{ 0xa4, 0xa1, 0xa1, 0xa5, 0xa2 },	/*	A	*/
	{ 0xa4, 0xa2, 0xa2, 0xa5, 0xa2 },

	{ 0xa4, 0xa3, 0xa3, 0xa5, 0xa4 },	/*	I	*/
	{ 0xa4, 0xa4, 0xa4, 0xa5, 0xa4 },

	{ 0xa4, 0xa5, 0xa5, 0xa5, 0xa6 },	/*	U	*/
	{ 0xa4, 0xa6, 0xa6, 0xa5, 0xa6 },

	{ 0xa4, 0xa7, 0xa7, 0xa5, 0xa8 },	/*	E	*/
	{ 0xa4, 0xa8, 0xa8, 0xa5, 0xa8 },

	{ 0xa4, 0xa9, 0xa9, 0xa5, 0xaa },	/*	O	*/
	{ 0xa4, 0xaa, 0xaa, 0xa5, 0xaa },	
	
	{ 0xa4, 0xab, 0xc2, 0xa5, 0xab},	/* KA - DI */

	{ 0xa4, 0xc3, 0xc3, 0xa5, 0xc4 },	/* TU */
	{ 0xa4, 0xc4, 0xc4, 0xa5, 0xc4 },

	{ 0xa4, 0xc5, 0xe2, 0xa5, 0xc5 },	/* DU - MO */

	{ 0xa4, 0xe3, 0xe3, 0xa5, 0xe4 },	/* YA	*/
	{ 0xa4, 0xe4, 0xe4, 0xa5, 0xe4 },

	{ 0xa4, 0xe5, 0xe5, 0xa5, 0xe6 },	/* YU	*/
	{ 0xa4, 0xe6, 0xe6, 0xa5, 0xe6 },

	{ 0xa4, 0xe7, 0xe7, 0xa5, 0xe8 },	/* YO	*/
	{ 0xa4, 0xe8, 0xe8, 0xa5, 0xe8 },

	{ 0xa4, 0xe9, 0xed, 0xa5, 0xe9},	/* RA - RO */

	{ 0xa4, 0xee, 0xee, 0xa5, 0xef },	/* WA	*/
	{ 0xa4, 0xef, 0xef, 0xa5, 0xef },

	{ 0xa4, 0xf0, 0xf3, 0xa5, 0xf0 },	/* WI - N */
	
	/*	Map SMALL Full-width Katakana to Full-width Katakana */
	{ 0xa5, 0xa1, 0xa1, 0xa5, 0xa2 },	/*	A	*/
	{ 0xa5, 0xa3, 0xa3, 0xa5, 0xa4 },	/*	I	*/
	{ 0xa5, 0xa5, 0xa5, 0xa5, 0xa6 },	/*	U	*/
	{ 0xa5, 0xa7, 0xa7, 0xa5, 0xa8 },	/*	E	*/
	{ 0xa5, 0xa9, 0xa9, 0xa5, 0xaa },	/*	O	*/
	{ 0xa5, 0xc3, 0xc3, 0xa5, 0xc4 },	/* TU	*/
	{ 0xa5, 0xe3, 0xe3, 0xa5, 0xe4 },	/* YA	*/
	{ 0xa5, 0xe5, 0xe5, 0xa5, 0xe6 },	/* YU	*/
	{ 0xa5, 0xe7, 0xe7, 0xa5, 0xe8 },	/* YO	*/
	{ 0xa5, 0xee, 0xee, 0xa5, 0xef },	/* WA	*/
	
	/*	Full-width Greek */
	{ 0xa6, 0xa1, 0xb8, 0xa6, 0xc1 },	
	/*	Full-width Cyrillic*/
	{ 0xa7, 0xa1, 0xc1, 0xa7, 0xd1 },	

	{ 0x00, 0x00, 0x00, 0x00, 0x00 }	/* Terminator */
};	
static DoubleByteToLowerMap lower_lookup_big5[]=
{	
	/*	Full-width Latin */
	{ 0xa2, 0xcf, 0xe4, 0xa2, 0xe9 },	/* Part 1 A-V */
	{ 0xa2, 0xe5, 0xe8, 0xa3, 0x40 },	/* Part 2 W-Z */
	
	/*	Full-width Greek */
	{ 0xa3, 0x44, 0x5b, 0xa3, 0x5c },

	{ 0x00, 0x00, 0x00, 0x00, 0x00 }	/* Terminator */
};	
static DoubleByteToLowerMap lower_lookup_cns11643_1[]=
{	
	/*	Roman Number  */
	{ 0xa4, 0xab, 0xb4, 0xa6, 0xb5 },	
	
	/*	Full-width Latin */
	{ 0xa4, 0xc1, 0xda, 0xa4, 0xdb },	
	
	/*	Full-width Greek */
	{ 0xa4, 0xf5, 0xfe, 0xa5, 0xaf },	/* Part 1 Alpha - kappa */
	{ 0xa5, 0xa1, 0xae, 0xa5, 0xb9 },	/* Part 2 Lamda - Omega */

	{ 0x00, 0x00, 0x00, 0x00, 0x00 }	/* Terminator */
};	
static DoubleByteToLowerMap lower_lookup_ksc5601[]=
{	
	/*	Full-width Latin */
	{ 0xa3, 0xc1, 0xda, 0xa3, 0xe1 },	

	/*	Full-width Roman Number */
	{ 0xa5, 0xb0, 0xb9, 0xa5, 0xa1 },	

	/*	Full-width Greek */
	{ 0xa5, 0xc1, 0xd8, 0xa5, 0xe1 },	

	/*	Map Full-width Hiragana to  Full-width Katakana */
	{ 0xaa, 0xa1, 0xa1, 0xab, 0xa2 },	/*	A	*/
	{ 0xaa, 0xa2, 0xa2, 0xab, 0xa2 },

	{ 0xaa, 0xa3, 0xa3, 0xab, 0xa4 },	/*	I	*/
	{ 0xaa, 0xa4, 0xa4, 0xab, 0xa4 },

	{ 0xaa, 0xa5, 0xa5, 0xab, 0xa6 },	/*	U	*/
	{ 0xaa, 0xa6, 0xa6, 0xab, 0xa6 },

	{ 0xaa, 0xa7, 0xa7, 0xab, 0xa8 },	/*	E	*/
	{ 0xaa, 0xa8, 0xa8, 0xab, 0xa8 },

	{ 0xaa, 0xa9, 0xa9, 0xab, 0xaa },	/*	O	*/
	{ 0xaa, 0xaa, 0xaa, 0xab, 0xaa },	
	
	{ 0xaa, 0xab, 0xc2, 0xab, 0xab},	/* KA - DI */

	{ 0xaa, 0xc3, 0xc3, 0xab, 0xc4 },	/* TU */
	{ 0xaa, 0xc4, 0xc4, 0xab, 0xc4 },

	{ 0xaa, 0xc5, 0xe2, 0xab, 0xc5 },	/* DU - MO */

	{ 0xaa, 0xe3, 0xe3, 0xab, 0xe4 },	/* YA	*/
	{ 0xaa, 0xe4, 0xe4, 0xab, 0xe4 },

	{ 0xaa, 0xe5, 0xe5, 0xab, 0xe6 },	/* YU	*/
	{ 0xaa, 0xe6, 0xe6, 0xab, 0xe6 },

	{ 0xaa, 0xe7, 0xe7, 0xab, 0xe8 },	/* YO	*/
	{ 0xaa, 0xe8, 0xe8, 0xab, 0xe8 },

	{ 0xaa, 0xe9, 0xed, 0xab, 0xe9},	/* RA - RO */

	{ 0xaa, 0xee, 0xee, 0xab, 0xef },	/* WA	*/
	{ 0xaa, 0xef, 0xef, 0xab, 0xef },

	{ 0xaa, 0xf0, 0xf3, 0xab, 0xf0 },	/* WI - N */
	
	/*	Map SMALL Full-width Katakana to Full-width Katakana */
	{ 0xab, 0xa1, 0xa1, 0xab, 0xa2 },	/*	A	*/
	{ 0xab, 0xa3, 0xa3, 0xab, 0xa4 },	/*	I	*/
	{ 0xab, 0xa5, 0xa5, 0xab, 0xa6 },	/*	U	*/
	{ 0xab, 0xa7, 0xa7, 0xab, 0xa8 },	/*	E	*/
	{ 0xab, 0xa9, 0xa9, 0xab, 0xaa },	/*	O	*/
	{ 0xab, 0xc3, 0xc3, 0xab, 0xc4 },	/* TU	*/
	{ 0xab, 0xe3, 0xe3, 0xab, 0xe4 },	/* YA	*/
	{ 0xab, 0xe5, 0xe5, 0xab, 0xe6 },	/* YU	*/
	{ 0xab, 0xe7, 0xe7, 0xab, 0xe8 },	/* YO	*/
	{ 0xab, 0xee, 0xee, 0xab, 0xef },	/* WA	*/

	/*	Full-width Cyrillic*/
	{ 0xac, 0xa1, 0xc1, 0xac, 0xd1 },	
	
	{ 0x00, 0x00, 0x00, 0x00, 0x00 }	/* Terminator */
};

DoubleByteToLowerMap *INTL_GetDoubleByteToLowerMap(int16 csid)
{
	switch(csid)
	{
#ifndef XP_UNIX
		case CS_SJIS:
			return lower_lookup_sjis;
#else
		case CS_EUCJP:
			return lower_lookup_eucjp;
#endif
		case CS_BIG5:		
			return lower_lookup_big5;
		case CS_CNS_8BIT:		
			return lower_lookup_cns11643_1;
		case CS_GB_8BIT:	
			return lower_lookup_eucjp;	/*	The to_lower mapping for GB 2312 and JIS0208 are exactly the same */
										/*  We just use the same table here.								  */
		case CS_KSC_8BIT:	
			return lower_lookup_ksc5601;
		default:
			return lower_lookup_none;
	}
}
void INTL_DoubleByteToLower(DoubleByteToLowerMap *db_tolowermap, unsigned char* lowertext, unsigned char* text)
{
	DoubleByteToLowerMap *p;
	for(p = db_tolowermap; !((p->src_b1 == 0) && (p->src_b2_start == 0)); p++)
	{
		if( (p->src_b1       == text[0]) &&
			(p->src_b2_start <= text[1] ) &&
			(p->src_b2_end   >= text[1]) )
		{
			lowertext[0] = p->dest_b1;
			lowertext[1] = text[1] - p->src_b2_start + p->dest_b2_start;
			return;
		}
		else 
		{	/*	The map have to be sorted order to implement a fast search */
			if(p->src_b1 > text[0])
				break;
			else {
				if((p->src_b1 == text[0]) && (p->src_b2_start > text[1]))
					break;
			}
		}
	}
	lowertext[0] = text[0];
	lowertext[1] = text[1];
	return;
}

XP_Bool INTL_MatchOneChar(int16 csid, unsigned char *text1,unsigned char *text2,int *charlen)
{
	if((INTL_CharSetType(csid) == SINGLEBYTE) ) {
		unsigned char *sb_tolowermap;
		*charlen = 1;
		sb_tolowermap = INTL_GetSingleByteToLowerMap(csid);
		return( INTL_SingleByteToLower(sb_tolowermap,text1[0]) == INTL_SingleByteToLower(sb_tolowermap, text2[0]));
	}
	else
	{
		int l1, l2;
		l1 = INTL_CharLen(csid ,text1);
		l2 = INTL_CharLen(csid ,text2);
		if(l1 != l2)
			return FALSE;
		if(l1 == 1)
		{
			unsigned char *sb_tolowermap;
			*charlen = 1;
			sb_tolowermap = INTL_GetSingleByteToLowerMap(csid);
			return( INTL_SingleByteToLower(sb_tolowermap,text1[0]) == INTL_SingleByteToLower(sb_tolowermap, text2[0]));
		}
		else
		{
			if(l1 == 2)
			{
				DoubleByteToLowerMap *db_tolowermap;
				unsigned char lowertext1[2], lowertext2[2];
				*charlen = 2;
				db_tolowermap = INTL_GetDoubleByteToLowerMap(csid);
				INTL_DoubleByteToLower(db_tolowermap, lowertext1, text1);
				INTL_DoubleByteToLower(db_tolowermap, lowertext2, text2);
				return( ( lowertext1[0] ==  lowertext2[0] ) &&
						( lowertext1[1] ==  lowertext2[1] ) );
			}
			else
			{
				/* for character which is neither one byte nor two byte, we cannot ignore case for them */
				int i;
				*charlen = l1;
				for(i=0;i<l1;i++)
				{
					if(text1[i] != text2[i])
						return FALSE;
				}
				return TRUE;
			}
		}
	}
}
XP_Bool INTL_MatchOneCaseChar(int16 csid, unsigned char *text1,unsigned char *text2,int *charlen)
{
	if((INTL_CharSetType(csid) == SINGLEBYTE) ) {
		*charlen = 1;
		return( text1[0]== text2[0]);
	}
	else
	{
		int i,len;
		*charlen = len  = INTL_CharLen(csid, (unsigned char *) text1);
		for(i=0 ; i < len; i++)
		{
			if(text1[i] != text2[i]) 
				return FALSE;
		}
		return TRUE;
	}
}

PUBLIC
char *INTL_Strstr(int16 csid, const char *s1,const char *s2)
{
	int len;
	char *p1, *pp1, *p2;
	if((s2==NULL) || (*s2 == '\0'))
		return (char *)s1;
	if((s1==NULL) || (*s1 == '\0'))
		return NULL;
	
	for(p1=(char*)s1; *p1  ;p1 = INTL_NextChar(csid ,p1))
	{
		for(p2=(char*)s2, pp1=p1 ;  
			((*pp1) && (*p2) && INTL_MatchOneCaseChar(csid, (unsigned char*)pp1, (unsigned char*)p2, &len)); 
			pp1 += len, p2 += len)
				;	/* do nothing in the loop */
		if(*p2 == '\0')
			return p1;
	}
	return NULL;
}

PUBLIC
char *INTL_Strcasestr(int16 csid, const char *s1, const char *s2)
{
	int len;
	char *p1, *pp1, *p2;
	if((s2==NULL) || (*s2 == '\0'))
		return (char *)s1;
	if((s1==NULL) || (*s1 == '\0'))
		return NULL;

	for(p1=(char*)s1; *p1  ;p1 = INTL_NextChar(csid , p1))
	{
		for(p2=(char*)s2, pp1=p1 ;  
			((*pp1) && (*p2) && INTL_MatchOneChar(csid, (unsigned char*)pp1, (unsigned char*)p2, &len)); 
			pp1 += len, p2 += len)
				;	/* do nothing in the loop */
		if(*p2 == '\0')
			return p1;
	}
	return NULL;
}


PUBLIC
XP_Bool INTL_FindMimePartIIStr(int16 csid, XP_Bool searchcasesensitive, const char *mimepart2str,const char *s2)
{
	XP_Bool onlyAscii;
	char *ret = NULL;
	char *s1 = (char*)mimepart2str;
	char *conv;
	if((s2 == NULL) || (*s2 == '\0'))	/* if search for NULL string, return TRUE */
		return TRUE;
	if((s1 == NULL) || (*s1 == '\0'))	/* if string is NULL, return FALSE */
		return FALSE;
	
	conv= IntlDecodeMimePartIIStr(mimepart2str, csid, FALSE);
	if(conv)
		s1 = conv;
	onlyAscii = onlyAsciiStr(s1) && onlyAsciiStr(s2);
	if(onlyAscii)	/* for performance reason, let's call the ANSI C routine for ascii only case */
	{
		if(searchcasesensitive)
			ret= strstr( s1, s2);
		else
			ret= strcasestr(s1, s2);
	}
	else
	{
		if(searchcasesensitive)
			ret= INTL_Strstr(csid, s1, s2);
		else
			ret= INTL_Strcasestr(csid, s1, s2);
	}
	if(conv != mimepart2str)
		XP_FREE(conv);
	return (ret != NULL);		/* return TRUE if it find something */
}

PUBLIC
int32  INTL_TextByteCountToCharLen(int16 csid, unsigned char* text, uint32 byteCount)
{
	/* quickly return if it is zero */
	if(byteCount == 0 )
		return 0;
	if(INTL_CharSetType(csid) == SINGLEBYTE)
	{
		/* for single byte csid, byteCount equal to charLen */
		return byteCount;
	}
	else
	{
		csinfo_t	*info ;
		info = &csinfo_tbl[0] ;
		for (;info->csid; info++)			/* looking for an entry for this csid */
		{
			if (info->csid == csid)	/* find the entry for this csid */
			{
				uint32 curByte, curChar;
				int thislen;
				for(curByte=curChar=0; curByte < byteCount ;curChar++,curByte += thislen)
				{
					int i;
					unsigned char ch = text[curByte];
					/* preset thislen to 1 and looking for the entry for this char */
					for (i=0, thislen = 1; i<MAX_FIRSTBYTE_RANGE && info->enc[i].bytes > 0; i++)
					{
						if ((ch >= info->enc[i].range[0]) && (ch <= info->enc[i].range[1]))
							thislen = info->enc[i].bytes;
					}
				}
				return curChar;		
			}
		}
	}
	/* it should not come to here */
	XP_ASSERT(byteCount);
	return byteCount;		
}



PUBLIC
int32  INTL_TextCharLenToByteCount(int16 csid, unsigned char* text, uint32 charLen)
{
	/* quickly return if it is zero */
	if(charLen == 0 )
		return 0;
	if(INTL_CharSetType(csid) == SINGLEBYTE)
	{
		/* for single byte csid, byteCount equal to charLen */
		return charLen;
	}
	else
	{
		csinfo_t	*info ;
		info = &csinfo_tbl[0] ;
		for (;info->csid; info++)			/* looking for an entry for this csid */
		{
			if (info->csid == csid)	/* find the entry for this csid */
			{
				uint32 curByte, curChar;
				int thislen;
				for(curByte=curChar=0; curChar < charLen ;curChar++,curByte += thislen)
				{
					int i;
					unsigned char ch = text[curByte];
					/* preset thislen to 1 and looking for the entry for this char */
					for (i=0, thislen = 1; i<MAX_FIRSTBYTE_RANGE && info->enc[i].bytes > 0; i++)
					{
						if ((ch >= info->enc[i].range[0]) && (ch <= info->enc[i].range[1]))
							thislen = info->enc[i].bytes;
					}
				}
				return curByte;		
			}
		}
	}
	/* it should not come to here */
	XP_ASSERT(charLen);
	return charLen;
}

const char *INTL_NonBreakingSpace(MWContext *pContext)
{
    int csid = pContext->win_csid;

    switch (csid)
    {
    case CS_LATIN1:  
    case CS_LATIN2:
        return "\240";      /* 0xA0 */
    case CS_MAC_ROMAN:
    case CS_MAC_CE:
        return "\312";      /* 0xCA */
    case CS_SJIS:
        return "\201\100";  /* 0x8140 */
    case CS_BIG5:
        return "\241\100";   /* 0xA140 */
    case CS_KSC_8BIT:       /* korean */
        return "\241\241";   /* 0xA1A1 */
    case CS_EUCJP:
        return "\241\241";   /* 0xA1A1 */
    case CS_GB_8BIT:        /* chinese */
        return "\241\241";   /* 0xA1A1 */
    default:
        return "\240";
    }
}

/*
    INTL_CharClass is used for multibyte to divide character to different type
*/
int
INTL_CharClass(int charset, unsigned char *pstr)
{
	int	c1, c2;

	c1 = *pstr;

	switch (charset)
	{
	case CS_SJIS:
        if (c1 < 0x80)
            return  SEVEN_BIT_CHAR;
        if ((c1 >= 0xA0) && (c1 < 0xE0))
            return HALFWIDTH_PRONOUNCE_CHAR;

        c2 = *(pstr + 1);
        if (c1 == 0x82 && (c2 >= 0x60 && c2 <= 0x9A))
        {       /*   0x8260 -- 0x929A  */
            return FULLWIDTH_ASCII_CHAR;

        }
        if ((c1 == 0x82 && (c2 >= 0x9F && c2 <= 0xF1)) || 
            (c1 == 0x83 && (c2 >= 0x40 && c2 <= 0x96)) ||
            (c1 == 0x81 && (c2 == 0x5B || c2 == 0x5C || c2 == 0x5D)))
        {        /* 829F -- 82F1 
                    8340 -- 8396 */
            return FULLWIDTH_PRONOUNCE_CHAR;

        }
        if (c1 >= 0x88  &&  c1 <= 0xFC)
        {
            return KANJI_CHAR;

        }
        return UNCLASSIFIED_CHAR;
    case CS_KSC_8BIT:
        if (c1 < 0x80)
            return  SEVEN_BIT_CHAR;

        c2 = *(pstr + 1);
        if (c1 == 0xC1 && ((c2 >= 0xC1 && c2 <= 0xDA) ||
                           (c2 >= 0xE1 && c2 <= 0xFA)))
        {       /*   0xA3C1 -- 0xA3DA, 0xA3E1 -- A3FA  */
            return FULLWIDTH_ASCII_CHAR;

        }
        if ((c1 == 0xA4 && (c2 >= 0xA1 && c2 <= 0xFE)) || 
            (c1 >= 0xB0 && c1 < 0xCA))
        {        /* A4A1 -- A4FE, B0A0 -- C8FF */
            return FULLWIDTH_PRONOUNCE_CHAR;

        }
        if (c1 >= 0xCA  &&  c1 <= 0xFD)
        {       /* CAA0 -- FDFF */
            return KANJI_CHAR;

        }
        return UNCLASSIFIED_CHAR;
    case CS_GB_8BIT:
        if (c1 < 0x80)
            return  SEVEN_BIT_CHAR;

        c2 = *(pstr + 1);
        if (c1 == 0xA3 && ((c2 >= 0xC1 && c2 <= 0xDA) ||
                           (c2 >= 0xE1 && c2 <= 0xFA)))
        {       /*   0xA3C1 -- 0xA3DA, 0xA3E1 -- A3FA  */
            return FULLWIDTH_ASCII_CHAR;

        }
        if (c1 >= 0xB0 && c1 <= 0xF7) 
        {        /* B0A1 -- F7FE */
            return KANJI_CHAR;

        }
        return UNCLASSIFIED_CHAR;
    case CS_BIG5:
        if (c1 < 0x80)
            return  SEVEN_BIT_CHAR;

        c2 = *(pstr + 1);
        if ((c1 == 0xA2 && (c2 >= 0xCF && c2 <= 0xFF)) || 
            (c1 == 0xA3 && c2 <= 0x43))
        {       /*   0xA2CF -- 0xA343  */
            return FULLWIDTH_ASCII_CHAR;

        }
        if (((c1 == 0xA3 && ((c2 >= 0x74 && c2 <= 0x7E))) || 
			 (c2 >= 0xA1 && c2 <= 0xBF)))
        {        /* A374 -- A37E, A3A1 -- A3BF */
            return FULLWIDTH_PRONOUNCE_CHAR;

        }
        if ((c1 == 0xA4 && c2 != 0x40) || c1 > 0xA4)
        {       /* over A441 */
            return KANJI_CHAR;

        }
        return UNCLASSIFIED_CHAR;
#if 0
	case CS_EUCJP:
		if (c == 0x8e)
		{
			return 1;
		}
		break;
    case CS_GB_8BIT:
#endif
	default:
		break;
	}

	return UNCLASSIFIED_CHAR;
}

/*
  Japanese Kinsoku table:
  Characters are not allowed in the beginning of line 
*/
const char *ProhibitBegin_SJIS[] = 
{
"\x21",  "\x25",  "\x29",  "\x2C",  "\x2E",  "\x3A",  "\x3B",  "\x3F",  "\x5D", "\x7D", 
"\xA1",   "\xA3",   "\xA4",   "\xA5",   "\xDE",   "\xDF",   "\x81\x41", "\x81\x42", "\x81\x43", "\x81\x44", 	
"\x81\x45", "\x81\x46", "\x81\x47", "\x81\x48", "\x81\x49", "\x81\x4A", "\x81\x4B", "\x81\x52",	"\x81\x53", "\x81\x54", 
"\x81\x55", "\x81\x58", "\x81\x66", "\x81\x68", "\x81\x6A", "\x81\x6C", "\x81\x6E", "\x81\x70", "\x81\x72", "\x81\x74", 	
"\x81\x76", "\x81\x78", "\x81\x7A", "\x81\x8B", "\x81\x8C", "\x81\x8D", "\x81\x8E", "\x81\x91", "\x81\x93", "\x81\xF1", 
"\0"
};
const char *ProhibitBegin_EUCJP[] = 
{
"\x21",	 "\x25",	  "\x29",   "\x2C",   "\x2E",   "\x3A",   "\x3B",   "\x3F",   "\x5D",   "\x7D",            
"\x8E\xA1", "\x8E\xA3", "\x8E\xA4", "\x8E\xA5", "\x8E\xDE", "\x8E\xDF", "\xA1\xA2", "\xA1\xA3", "\xA1\xA4", "\xA1\xA5", 
"\xA1\xA6", "\xA1\xA7", "\xA1\xA8", "\xA1\xA9", "\xA1\xAA", "\xA1\xAB", "\xA1\xAC", "\xA1\xB3", "\xA1\xB4", "\xA1\xB5",
"\xA1\xB6", "\xA1\xB9", "\xA1\xC7", "\xA1\xC9",	"\xA1\xCB", "\xA1\xCD", "\xA1\xCF", "\xA1\xD1", "\xA1\xD3", "\xA1\xD5",
"\xA1\xD7", "\xA1\xD9", "\xA1\xDB", "\xA1\xEB", "\xA1\xEC", "\xA1\xED", "\xA1\xEE", "\xA1\xF1", "\xA1\xF3", "\xA2\xF3",
"\0"
};

/*
 * The datas are from Nadine Kano's book - Developing International Software -
 *   Page 239 - 245 
 */
const char *ProhibitBegin_BIG5[] = 
{
"\x21",     "\x29",     "\x2C", 	"\x2E",     "\x3A",     "\x3B",     "\x3F",     "\x5D",     "\x7D",     "\xA1\x41", 
"\xA1\x43", "\xA1\x44", "\xA1\x45", "\xA1\x46", "\xA1\x47", "\xA1\x48", "\xA1\x49", "\xA1\x4A", "\xA1\x4B", "\xA1\x4C", 
"\xA1\x4D", "\xA1\x4E", "\xA1\x50", "\xA1\x51", "\xA1\x52", "\xA1\x53", "\xA1\x54", "\xA1\x55", "\xA1\x56", "\xA1\x57", 
"\xA1\x58", "\xA1\x59", "\xA1\x5A", "\xA1\x5B", "\xA1\x5C", "\xA1\x5E", "\xA1\x60", "\xA1\x62", "\xA1\x64", "\xA1\x66", 
"\xA1\x68", "\xA1\x6A", "\xA1\x6C", "\xA1\x6E", "\xA1\x70", "\xA1\x72", "\xA1\x74", "\xA1\x76", "\xA1\x78", "\xA1\x7A", 
"\xA1\x7C", "\xA1\x7E", "\xA1\xA2", "\xA1\xA4", "\xA1\xA6", "\xA1\xA8", "\xA1\xAA", "\xA1\xAC", "\0"
};

const char *ProhibitBegin_GB[] = 
{
"\x21",     "\x29",     "\x2C",     "\x2E",     "\x3A",     "\x3B",     "\x3F",     "\x5D",     "\x7D",     "\xA1\xA2", 
"\xA1\xA3", "\xA1\xA4", "\xA1\xA5", "\xA1\xA6", "\xA1\xA7", "\xA1\xA8", "\xA1\xA9", "\xA1\xAA", "\xA1\xAB", "\xA1\xAC", 
"\xA1\xAD", "\xA1\xAF", "\xA1\xB1", "\xA1\xB3", "\xA1\xB5", "\xA1\xB7", "\xA1\xB9", "\xA1\xBB", "\xA1\xBD", "\xA1\xBF", 
"\xA1\xC3", "\xA3\xA1", "\xA3\xA2", "\xA3\xA7", "\xA3\xA9", "\xA3\xAC", "\xA3\xAE", "\xA3\xBA", "\xA3\xBB", "\xA3\xBF", 
"\xA3\xDD", "\xA3\xE0", "\xA3\xFC", "\xA3\xFD", "\0"
};

const char *ProhibitBegin_KSC[] = 
{
"\x21",     "\x25",     "\x29",     "\x2C",     "\x2E",     "\x3A",     "\x3B",     "\x3F",     "\x5D",     "\x7D", 
"\xA1\xA2", "\xA1\xAF", "\xA1\xB1", "\xA1\xB3", "\xA1\xB5", "\xA1\xB7", "\xA1\xB9", "\xA1\xBB", "\xA1\xBD", "\xA1\xC6", 
"\xA1\xC7", "\xA1\xC8", "\xA1\xC9", "\xA1\xCB", "\xA3\xA1", "\xA3\xA5", "\xA3\xA9", "\xA3\xAC", "\xA3\xAE", "\xA3\xBA", 
"\xA3\xBB", "\xA3\xBF", "\xA3\xDC", "\xA3\xDD", "\xA3\xFD", "\0"
};




/*
  Japanese Kinsoku table:
  Characters are not allowed in the end of line 
*/

const char *ProhibitEnd_SJIS[] =
{
"\x24",     "\x28",     "\x5B",     "\x5C",     "\x7B",     "\xA2",     "\x81\x65", "\x81\x67", "\x81\x69", "\x81\x6B",
"\x81\x6D", "\x81\x6F", "\x81\x71", "\x81\x73", "\x81\x75", "\x81\x77", "\x81\x79", "\x81\x8F", "\x81\x90", "\x81\x92",
"\0"
};
const char *ProhibitEnd_EUCJP[] =
{
"\x24",     "\x28",     "\x5B",     "\x5C",     "\x7B",     "\x8E\xA2", "\xA1\xC6", "\xA1\xC8", "\xA1\xCA", "\xA1\xCC",
"\xA1\xCE", "\xA1\xD0", "\xA1\xD2", "\xA1\xD4", "\xA1\xD6", "\xA1\xD8", "\xA1\xDA", "\xA1\xEF", "\xA1\xF0", "\xA1\xF2",
"\0"
};

const char *ProhibitEnd_BIG5[] = 
{
"\x28",    "\x5B",    "\x7B",    "\xA1\x5D", "\xA1\x5F", "\xA1\x61", "\xA1\x63", "\xA1\x65", "\xA1\x67", "\xA1\x69", 
"\xA1\x6D","\xA1\x6F","\xA1\x71","\xA1\x73", "\xA1\x75", "\xA1\x77", "\xA1\x79", "\xA1\x7B", "\xA1\x7D", 
"\xA1\xA1","\xA1\xA3","\xA1\xA5","\xA1\xA7", "\xA1\xA9", "\xA1\xAB", "\0"
};

const char *ProhibitEnd_GB[] = 
{
"\x28",    "\x5B",    "\x7B",    "\xA1\xAE", "\xA1\xB0", "\xA1\xB2", "\xA1\xB4", "\xA1\xB6", "\xA1\xB8", "\xA1\xBA", 
"\xA1\xBC","\xA1\xBE","\xA3\xA8","\xA3\xAE", "\xA3\xDB", "\xA3\xFB", "\0"
};

const char *ProhibitEnd_KSC[] = 
{
"\x28",    "\x5B",    "\x5C",    "\x7B",    "\xA1\xAE", "\xA1\xB0", "\xA1\xB2", "\xA1\xB4", "\xA1\xB6", "\xA1\xB8", 
"\xA1\xBA","\xA1\xBC","\xA3\xA4","\xA3\xA8","\xA3\xDB", "\xA3\xDC", "\xA3\xFB", "\0"
};


int INTL_KinsokuClass(int16 win_csid, unsigned char *pstr)
{
    int i;
	switch (win_csid)
	{
	  case CS_SJIS:
        for (i = 0; ProhibitBegin_SJIS[i][0]; i++)
            if (XP_STRNCMP((char *)pstr, ProhibitBegin_SJIS[i], XP_STRLEN(ProhibitBegin_SJIS[i])) == 0)
                return  PROHIBIT_BEGIN_OF_LINE;
        for (i = 0; ProhibitEnd_SJIS[i][0]; i++)
            if (XP_STRNCMP((char *)pstr, ProhibitEnd_SJIS[i], XP_STRLEN(ProhibitEnd_SJIS[i])) == 0)
                return PROHIBIT_END_OF_LINE;
		break;
	case CS_EUCJP:
        for (i = 0; i < ProhibitBegin_EUCJP[i][0]; i++)
            if (XP_STRNCMP((char *)pstr, ProhibitBegin_EUCJP[i], XP_STRLEN(ProhibitBegin_EUCJP[i])) == 0)
                return  PROHIBIT_BEGIN_OF_LINE;
        for (i = 0; i < ProhibitEnd_EUCJP[i][0]; i++)
            if (XP_STRNCMP((char *)pstr, ProhibitEnd_EUCJP[i], XP_STRLEN(ProhibitEnd_EUCJP[i])) == 0)
                return PROHIBIT_END_OF_LINE;
		break;
	case CS_GB_8BIT:
        for (i = 0; ProhibitBegin_GB[i][0]; i++)
            if (XP_STRNCMP((char *)pstr, ProhibitBegin_GB[i], XP_STRLEN(ProhibitBegin_GB[i])) == 0)
                return  PROHIBIT_BEGIN_OF_LINE;
        for (i = 0; ProhibitEnd_GB[i][0]; i++)
            if (XP_STRNCMP((char *)pstr, ProhibitEnd_GB[i], XP_STRLEN(ProhibitEnd_GB[i])) == 0)
                return PROHIBIT_END_OF_LINE;
		break;
	case CS_BIG5:
        for (i = 0; ProhibitBegin_BIG5[i][0]; i++)
            if (XP_STRNCMP((char *)pstr, ProhibitBegin_BIG5[i], XP_STRLEN(ProhibitBegin_BIG5[i])) == 0)
                return  PROHIBIT_BEGIN_OF_LINE;
        for (i = 0; ProhibitEnd_BIG5[i][0]; i++)
            if (XP_STRNCMP((char *)pstr, ProhibitEnd_BIG5[i], XP_STRLEN(ProhibitEnd_BIG5[i])) == 0)
                return PROHIBIT_END_OF_LINE;
		break;
	case CS_KSC_8BIT:
        for (i = 0; ProhibitBegin_KSC[i][0]; i++)
            if (XP_STRNCMP((char *)pstr, ProhibitBegin_KSC[i], XP_STRLEN(ProhibitBegin_KSC[i])) == 0)
                return  PROHIBIT_BEGIN_OF_LINE;
        for (i = 0; ProhibitEnd_KSC[i][0]; i++)
            if (XP_STRNCMP((char *)pstr, ProhibitEnd_KSC[i], XP_STRLEN(ProhibitEnd_KSC[i])) == 0)
                return PROHIBIT_END_OF_LINE;
		break;
	}

    return  PROHIBIT_NOWHERE;
}


#ifdef XP_UNIX
/* We need this linkerdummy to keep INTL_CompoundStrFromUnicode */
void* linkerdummy = (void*)INTL_CompoundStrFromUnicode;
#endif
