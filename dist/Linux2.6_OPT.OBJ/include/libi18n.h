/* -*- Mode: C; tab-width: 4 -*- */

/* libi18n.h */


#ifndef INTL_LIBI18N_H
#define INTL_LIBI18N_H


#include "xp.h"
#include "csid.h"


#define MAX_UNCVT 8    /* At least: longest ESC Seq + Char Bytes - 1 */


#define INTL_CharSetType(charsetid) (charsetid & 0x700)


typedef struct INTL_FontEncodingList INTL_FontEncodingList;

struct INTL_FontEncodingList
{
	INTL_FontEncodingList	*next;
	int16			charset;
	unsigned char		*text;
};


typedef struct CCCDataObject CCCDataObject;

typedef unsigned char *
(*CCCFunc)(CCCDataObject *data_object, const unsigned char *str, int32 len);


#ifdef STANDALONE_TEST


int16
INTL_CharSetNameToID();

int
INTL_GetCharCodeConverter();

char *
INTL_GetSingleByteTable();


#else /* STANDALONE_TEST */


XP_BEGIN_PROTOS


unsigned char *
INTL_CallCharCodeConverter(CCCDataObject *obj, const unsigned char *str,
	int32 len);

XP_Bool
INTL_CanAutoSelect(int16 csid);

#ifdef XP_WIN
void
INTL_ChangeDefaultCharSetID(int16 csid);
#endif

int
INTL_CharLen(int charsetid, unsigned char *pstr);

char *
INTL_CharSetDocInfo(MWContext *);

void
INTL_CharSetIDToName(int16 csid, char *charset);

int16
INTL_CharSetNameToID(char *charset);

int
INTL_ColumnWidth(int charsetid, unsigned char *pstr);

NET_StreamClass *
INTL_ConvCharCode(FO_Present_Types format_out, void *data_obj,
                  URL_Struct *URL_s, MWContext *window_id);

unsigned char *
INTL_ConvMailToWinCharCode(MWContext *context, unsigned char *bit7buff, uint32 block_size);

unsigned char *
INTL_ConvWinToMailCharCode(MWContext *context, unsigned char *bit8buff, uint32 block_size);

CCCDataObject *
INTL_CreateDocToMailConverter(MWContext *context, XP_Bool isHTML, unsigned char *buffer, uint32 buffer_size);

CCCDataObject *
INTL_CreateCharCodeConverter(void);

int16
INTL_DefaultDocCharSetID(MWContext *context);

int16
INTL_DefaultMailCharSetID(int16 csid);

int16
INTL_DefaultTextAttributeCharSetID(MWContext *context);

int16
INTL_DefaultWinCharSetID(MWContext *context);

void
INTL_DestroyCharCodeConverter(CCCDataObject *obj);

int16
INTL_DocToWinCharSetID(int16 csid);

void
INTL_EntityReference(unsigned char val, unsigned char *out, uint16 outbuflen,
	uint16 *outlen);

void
INTL_FreeFontEncodingList(INTL_FontEncodingList *);

void
INTL_FreeSingleByteTable(char **cvthdl);

int
INTL_GetCharCodeConverter(int16 from_csid, int16 to_csid, CCCDataObject *obj);

char **
INTL_GetSingleByteTable(int16 fromcsid, int16 tocsid,int32 func_ctx);

char *
INTL_LockTable(char **cvthdl);

INTL_FontEncodingList *
INTL_InternalToFontEncoding(int16 charset, unsigned char *text);

int
INTL_IsHalfWidth(MWContext *context, unsigned char *pstr);

int
INTL_IsLeadByte(int charsetid, unsigned char ch);

int
INTL_IsKana(int charsetid, unsigned char *pstr);

char *
INTL_NextChar(int charsetid, char *pstr);

int
INTL_NthByteOfChar(int charsetid, char *pstr, int pos);

void
INTL_NumericCharacterReference(unsigned char *in, uint32 inlen, uint32 *inread,
	unsigned char *out, uint16 outbuflen, uint16 *outlen, Bool force);

/*
 * to be called by front end
 * arg is null-terminated array of available font charsets
 * front end is responsible for freeing this array before exiting
 */
void
INTL_ReportFontCharSets(int16 *charsets);

char *
INTL_ResourceCharSet(void);

/*
 * To be called when backend catches charset info on <meta ... charset=...> tag.
 * This will force netlib to go get fresh data again either through cache or
 * network.
 */
enum
{
	METACHARSET_NONE = 0,
	METACHARSET_HASCHARSET,
	METACHARSET_REQUESTRELAYOUT,
	METACHARSET_FORCERELAYOUT,
	METACHARSET_RELAYOUTDONE
};
void 
INTL_Relayout(MWContext *context);

char* INTL_GetAcceptLanguage(void);	/* return the AcceptLanguage from FE Preference */ 

char * 
IntlDecodeMimePartIIStr(const char *subject, int wincsid, XP_Bool plaindecode);

char *IntlEncodeMimePartIIStr(char *subject, int win_csid, XP_Bool bUseMime);


int INTL_NextCharIdxInText(int16 csid,unsigned char *text, int pos);
int INTL_PrevCharIdxInText(int16 csid,unsigned  char *text, int pos);
XP_Bool INTL_MatchOneChar(int16 csid, unsigned char *text1,unsigned char *text2,int *charlen);
XP_Bool INTL_MatchOneCaseChar(int16 csid, unsigned char *text1,unsigned char *text2,int *charlen);


/*      INTL_Unicode    */
typedef uint16 INTL_Unicode ;

typedef uint16 INTL_Encoding_ID;

/* 	INTL_CompoundStr				*/
typedef struct INTL_CompoundStr INTL_CompoundStr;

/*	The data structure should be hide from the caller */
struct INTL_CompoundStr {
	INTL_Encoding_ID			encoding;
	unsigned char				*text;
	INTL_CompoundStr				*next;
};

typedef  INTL_CompoundStr* INTL_CompoundStrIterator;

/*	Public Function		*/
/*  Creationg Function  */
INTL_CompoundStr* 		INTL_CompoundStrFromStr(INTL_Encoding_ID inencoding, unsigned char* intext);
INTL_CompoundStr* 		INTL_CompoundStrFromUnicode(INTL_Unicode* inunicode, uint32 inlen);
void 					INTL_CompoundStrDestroy(INTL_CompoundStr* This);

/*  String Function		*/
void					INTL_CompoundStrCat(INTL_CompoundStr* s1, INTL_CompoundStr* s2);
INTL_CompoundStr* 		INTL_CompoundStrClone(INTL_CompoundStr* s1);

/* 	Iterator 			*/
INTL_CompoundStrIterator	INTL_CompoundStrFirstStr(INTL_CompoundStr* This, INTL_Encoding_ID *outencoding, unsigned char** outtext);
INTL_CompoundStrIterator	INTL_CompoundStrNextStr(INTL_CompoundStrIterator iterator, INTL_Encoding_ID *outencoding, unsigned char** outtext);

typedef void* INTL_UnicodeToStrIterator ;

INTL_UnicodeToStrIterator    INTL_UnicodeToStrIteratorCreate(
    INTL_Unicode*        	 ustr,
    uint32			ustrlen,
    INTL_Encoding_ID     	*encoding,
    unsigned char*         	 dest, 
    uint32            		 destbuflen
);

int     INTL_UnicodeToStrIterate(
    INTL_UnicodeToStrIterator     iterator,
    INTL_Encoding_ID         *encoding,
    unsigned char*            dest, 
    uint32                    destbuflen
);
void    INTL_UnicodeToStrIteratorDestroy(
    INTL_UnicodeToStrIterator     iterator
);

uint32    INTL_UnicodeToStrLen(
    INTL_Encoding_ID encoding,
    INTL_Unicode*    ustr,
    uint32			 ustrlen
);
void    INTL_UnicodeToStr(
    INTL_Encoding_ID encoding,
    INTL_Unicode*    ustr, 
    uint32			 ustrlen,
    unsigned char*   dest, 
    uint32           destbuflen
);

INTL_Encoding_ID    INTL_UnicodeToEncodingStr(
    INTL_Unicode*    ustr,
    uint32 			 ustrlen,
    unsigned char*   dest,
    uint32           destbuflen
);

uint32 INTL_UnicodeLen(INTL_Unicode* ustr);

uint32    INTL_StrToUnicodeLen(
    INTL_Encoding_ID encoding,
    unsigned char*    src 
);
uint32    INTL_StrToUnicode(
    INTL_Encoding_ID encoding,
    unsigned char*   src, 
    INTL_Unicode*    ustr, 
    uint32           ubuflen
);
uint32    INTL_TextToUnicodeLen(
    INTL_Encoding_ID encoding,
    unsigned char*    src,
    uint32            srclen
);
uint32    INTL_TextToUnicode(
    INTL_Encoding_ID encoding,
    unsigned char*   src, 
    uint32           srclen,
    INTL_Unicode*    ustr, 
    uint32           ubuflen
);


unsigned char *INTL_CsidToCharsetNamePt(int16 csid);

char *INTL_Strstr(int16 csid, const char *s1,const char *s2);
char *INTL_Strcasestr(int16 csid, const char *s1, const char *s2);
XP_Bool INTL_FindMimePartIIStr(int16 csid, XP_Bool searchcasesensitive, const char *mimepart2str,const char *s2);

void INTL_SetUnicodeCSIDList(uint16 numOfItems, int16 *csidlist);	/* Moved from intlpriv.h */

int16*  INTL_GetUnicodeCSIDList(int16 * outnum);
unsigned char **INTL_GetUnicodeCharsetList(int16 * outnum);

int32  INTL_TextByteCountToCharLen(int16 csid, unsigned char* text, uint32 byteCount);
int32  INTL_TextCharLenToByteCount(int16 csid, unsigned char* text, uint32 charLen);


int INTL_NextCharIdx(int16 csid,unsigned char *str, int pos);
int INTL_PrevCharIdx(int16 csid,unsigned  char *str, int pos);

const char *INTL_NonBreakingSpace(MWContext *pContext);

enum {
    SEVEN_BIT_CHAR,
    HALFWIDTH_PRONOUNCE_CHAR,
    FULLWIDTH_ASCII_CHAR,
    FULLWIDTH_PRONOUNCE_CHAR,
    KANJI_CHAR,
    UNCLASSIFIED_CHAR
};
int
INTL_CharClass(int charset, unsigned char *pstr);

enum {
    PROHIBIT_NOWHERE,
    PROHIBIT_BEGIN_OF_LINE,
    PROHIBIT_END_OF_LINE
};
int INTL_KinsokuClass(int16 win_csid, unsigned char *pstr);

XP_END_PROTOS


#endif /* STANDALONE_TEST */


#endif /* INTL_LIBI18N_H */
