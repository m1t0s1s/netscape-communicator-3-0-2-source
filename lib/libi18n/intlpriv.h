/* -*- Mode: C; tab-width: 4 -*- */
/*	intlpriv.h	*/

#ifndef _INTLPRIV_H_
#define _INTLPRIV_H_

#ifdef STANDALONE_TEST

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/* void *malloc (size_t size); */
void free(void *ptr);
#define XP_ALLOC(size)		malloc(size)
#define XP_FREE(ptr)		free(ptr)
#define int32			int
#define int16			short
typedef int 
(*MKStreamWriteFunc)		(void *data_object, const char *str, int32 len);
#define NET_StreamClass		char *
#define URL_Struct		char
#define MWContext		char
#define URL_Struct		char
#define MK_OUT_OF_MEMORY	1
int debug_on = 0;
#define debug			if (debug_on) fprintf

#else /* STANDALONE_TEST */

#include "libi18n.h"

struct CCCDataObject {
    NET_StreamClass    *current_stream;
    NET_StreamClass    *next_stream;
    CCCFunc            cvtfunc;
    int32              jismode;
    int32              cvtflag;                /* cvt func dependent flag   */
    unsigned char      uncvtbuf[MAX_UNCVT];
    int16              from_csid;
    int16              to_csid;
    int                retval;                 /* error value for return    */
    int32              len;                    /* byte len of converted buf */
};

#endif /* STANDALONE_TEST */

					/* values for ASCII chars	*/
#define ESC		0x1B		/* ESC character		*/ 
#define NL		0x0A		/* newline			*/ 
#undef CR
#define CR		0x0D		/* carriage return		*/
#define DOLLAR		0x24		/* carriage return		*/

					/* values for EUC shift chars	*/
#define SS2		0x8E		/* Single Shift 2		*/
#define SS3		0x8F		/* Single Shift 3		*/
 
		 			/* JIS encoding mode flags	*/
#define JIS_Roman	0
#define JIS_208_83	1
#define JIS_HalfKana	2
#define JIS_212_90	3

/* I am using low nibble for the ESC flag and the next high nibble for Shift */
#define KSC_5601_87	0x04

/* Default state is SHIFT_OUT when we begin.
 * So SHIFT_IN should have value 0x0- */
#define SHIFT_IN	0x00
#define SHIFT_OUT	0x10

/* The actual values to be output for SHIFTING */
#define SO		0x0e
#define SI		0x0f

/* Some masks for computation */
#define ESC_MASK	0x0F
#define SHIFT_MASK	0xF0
 

/*
 * Shift JIS Encoding
 *				1st Byte Range	2nd Byte Range
 * ASCII/JIS-Roman		0x21-0x7F	n/a
 * 2-Byte Char(low range)	0x81-0x9F	0x40-0x7E, 0x80-0xFC
 * Half-width space(non-std)	0xA0		n/a
 * Half-width katakana		0xA1-0xDF	n/a
 * 2-Byte Char(high range)	0xE0-0xEF	0x40-0x7E, 0x80-0xFC
 * User Defined(non-std)	0xF0-0xFC	0x40-0x7E, 0x80-0xFC
 *
 * JIS Encoding
 *				1st Byte Range	2nd Byte Range
 * ASCII/JIS-Roman		0x21-0x7E	n/a
 * Half-width katakana(non-std)	0x21-0x5F	n/a
 * 2-Byte Char			0x21-7E		0x21-7E
 *
 * Japanese EUC Encoding
 *				1st Byte Range	2nd Byte Range	3rd Byte Range
 * ASCII/JIS-Roman		0x21-0x7E	n/a		n/a
 * JIS X 0208-1990		0xA0-0xFF	0xA0-0xFF	n/a
 * Half-width katakana		SS2		0xA0-0xFF	n/a
 * JIS X 0212-1990		SS3		0xA0-0xFF	0xA0-0xFF
 *
 *
 *	List of ISO2022-INT Escape Sequences:
 *
 *		SUPPORTED:
 *		ASCII			ESC ( B		G0
 *		JIS X 0201-Roman	ESC ( J		G0
 *		Half-width Katakana	ESC ( I
 *		JIS X 0208-1978		ESC $ @		G0
 *		JIS X 0208-1983		ESC $ B		G0
 *		JIS X 0212-1990		ESC $ ( D	G0	(to EUC only)
 *		ISO8859-1		ESC - A		G1
 *
 *		UNSUPPORTED:
 *		GB 2312-80		ESC $ A		G0
 *		KS C 5601-1987		ESC $ ) C	G1
 *		CNS 11643-1986-1	ESC $ ( G	G0
 *		CNS 11643-1986-2	ESC $ ( H	G0
 *		ISO8859-7(Greek)	ESC - F		G1
 *
 * Added right parens: ))))) to balance editors' showmatch...
 */

 
	/* JIS-Roman mode enabled by 3 char ESC sequence: <ESC> ( J	*/
#define InsRoman_ESC(cp, obj) {				\
 	obj->jismode = JIS_Roman;			\
 	*cp++ = ESC;					\
 	*cp++ = '(';					\
 	*cp++ = 'J';					\
}
	/* JIS x208-1983 mode enabled by 3 char ESC sequence: <ESC> $ B	*/
#define Ins208_83_ESC(cp, obj) {			\
	obj->jismode = JIS_208_83;			\
 	*cp++ = ESC;					\
 	*cp++ = '$';					\
 	*cp++ = 'B';					\
}
	/* JIS Half-width katakana mode enabled by 3 char ESC seq.: ESC ( I */
#define InsHalfKana_ESC(cp, obj) {			\
 	obj->jismode = JIS_HalfKana;			\
 	*cp++ = ESC;					\
 	*cp++ = '(';					\
 	*cp++ = 'I';					\
}
	/* JIS x212-1990 mode enabled by 4 char ESC sequence: <ESC> $ ( D */
#define Ins212_90_ESC(cp, obj) {			\
	obj->jismode = JIS_212_90;			\
 	*cp++ = ESC;					\
 	*cp++ = '$';					\
 	*cp++ = '(';					\
 	*cp++ = 'D';					\
}

	/* KSC 5601-1987 mode enabled by 4 char ESC sequence: <ESC> $ ) D */
#define Ins5601_87_ESC(cp, obj) {	\
	obj->jismode = (obj->jismode & ~ESC_MASK) | KSC_5601_87;	\
 	*cp++ = ESC;							\
 	*cp++ = '$';							\
 	*cp++ = ')';							\
 	*cp++ = 'C';							\
}
#define Ins5601_87_SI(cp, obj)	{					\
	obj->jismode = ((obj->jismode & ~SHIFT_MASK) | SHIFT_IN);	\
	*cp++ = SI;							\
}
#define Ins5601_87_SO(cp, obj)	{					\
	obj->jismode = ((obj->jismode & ~SHIFT_MASK) | SHIFT_OUT);	\
	*cp++ = SO;							\
}
#define IsIns5601_87_ESC(obj)	((obj->jismode & ESC_MASK) == KSC_5601_87)
#define IsIns5601_87_SI(obj)	((obj->jismode & SHIFT_MASK) == SHIFT_IN)
#define IsIns5601_87_SO(obj)	((obj->jismode & SHIFT_MASK) == SHIFT_OUT)

	/* Added right parens: )))))) to balance editors' showmatch...	*/

	/* Maximum Length of Escape Sequence and Character Bytes per Encoding */
#define MAX_SJIS	2
#define MAX_EUC		3
#define	MAX_JIS		6

#define MAX_FIRSTBYTE_RANGE  3
typedef struct {
    int16	csid;			/* character code set ID */
    struct {
        unsigned char bytes;       /* number of bytes for range */
        unsigned char columns;     /* number of columns for range */
        unsigned char range[2];    /* Multibyte first byte range */
    } enc[MAX_FIRSTBYTE_RANGE];
} csinfo_t;

#define MAX_CSNAME	64
typedef struct _csname2id_t {
	unsigned char	cs_name[MAX_CSNAME];
	int16		cs_id;
	unsigned char	fill[3];
} csname2id_t;

typedef struct {
	int16		from_csid;	/* "from" codeset ID		*/
	int16		to_csid;	/* "to" codeset ID		*/
	int16		autoselect;	/* autoselectable		*/
	CCCFunc		cvtmethod;	/* char. codeset conv method	*/
	int32		cvtflag;	/* conv func dependent flag	*/
} cscvt_t;

/*
	"The character set names may be up to 40 characters taken from the
	printable characters of US-ASCII.  However, no distinction is made
	between use of upper and lower case letters." [RFC 1700]
*/
typedef struct {
	int16	win_csid;
	int16	mime_csid;
	char	mime_charset[48];
} csmime_t;

#ifdef XP_UNIX
typedef struct {
	int16	win_csid;
	char	*locale;
	char	*fontlist;
} cslocale_t;
#endif /* XP_UNIX */

#ifdef STANDALONE_TEST
int		net_1to1CCC();
int		net_sjis2jis();
int		net_sjis2euc();
int		net_euc2jis();
int		net_euc2sjis();
int		net_jis2other();
int		net_euckr2iso();
int		net_iso2euckr();
int		net_b52cns();
int		net_cns2b5();

unsigned char	*One2OneCCC();
unsigned char	*mz_sjis2jis();
unsigned char	*mz_sjis2euc();
unsigned char	*mz_euc2jis();
unsigned char	*mz_euc2sjis();
unsigned char	*jis2other();
unsigned char	*mz_euckr2iso();
unsigned char	*mz_iso2euckr();
unsigned char	*mz_b52cns();
unsigned char	*mz_cns2b5();

#else /* STANDALONE_TEST */
XP_BEGIN_PROTOS

int	      net_1to1CCC(CCCDataObject *, const unsigned char *s, int32 l);
unsigned char *One2OneCCC(CCCDataObject *, const unsigned char *s, int32 l);
int	      net_sjis2jis(CCCDataObject *,const unsigned char *s,int32 l);
unsigned char *mz_sjis2jis(CCCDataObject *, const unsigned char *s, int32 l);
int	      net_sjis2euc(CCCDataObject *,const unsigned char *s,int32 l);
unsigned char *mz_sjis2euc(CCCDataObject *, const unsigned char *s, int32 l);
int	      net_euc2jis(CCCDataObject *,const unsigned char *s, int32 l);
unsigned char *mz_euc2jis(CCCDataObject *, const unsigned char *s, int32 l);
int	      net_euc2sjis(CCCDataObject *,const unsigned char *s,int32 l);
unsigned char *mz_euc2sjis(CCCDataObject *, const unsigned char *s, int32 l);
unsigned char *mz_euckr2iso(CCCDataObject *, const unsigned char *s, int32 l);
unsigned char *mz_iso2euckr(CCCDataObject *, const unsigned char *s, int32 l);
unsigned char *mz_b52cns(CCCDataObject *, const unsigned char *s, int32 l);
unsigned char *mz_cns2b5(CCCDataObject *, const unsigned char *s, int32 l);
int	      net_jis2other(CCCDataObject *, const unsigned char *s, int32 l);
unsigned char *jis2other(CCCDataObject *, const unsigned char *s, int32 l);
unsigned char *autoJCCC (CCCDataObject *, const unsigned char *s, int32 l);
int16	      detect_JCSID (MWContext *context,const unsigned char *, int32);
void	      FE_fontchange(MWContext *window_id, int16  csid);
void          FE_ChangeURLCharset(const char *newCharset);
int16	      GetMimeCSID(int16, char	**);
char **		FE_GetSingleByteTable(int16 from_csid, int16 to_csid, int32 resourceid);
char *		FE_LockTable(char **cvthdl);
void 		FE_FreeSingleByteTable(char **cvthdl);

unsigned char *ConvHeader(unsigned char *subject);

#ifdef XP_UNIX
int16	      FE_WinCSID(MWContext *);
#else /* XP_UNIX */
#define	      FE_WinCSID(window_id) 0
#endif /* XP_UNIX */

int16 *intl_GetFontCharSets(void);

XP_Bool INTL_ConvertNPC(unsigned char *in, uint16 inlen, uint16 *inread,
    unsigned char *out, uint16 outbuflen, uint16 *outlen, int16 *outcsid);

unsigned char *intl_2022KRToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len);
unsigned char *intl_ASCIIToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len);
unsigned char *intl_BIG5ToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len);
unsigned char *intl_EUCCNToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len);
unsigned char *intl_EUCJPToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len);
unsigned char *intl_EUCKRToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len);
unsigned char *intl_EUCTWToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len);
unsigned char *intl_ISO88591ToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len);
unsigned char *intl_ISO88592ToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len);
unsigned char *intl_JISToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len);
unsigned char *intl_SJISToNPC(CCCDataObject *obj, const unsigned char *buf, int32 len);

XP_END_PROTOS
#endif /* STANDALONE_TEST */


#ifdef STANDALONE_TEST
#define CCC_PUTBLOCK(obj, s, l)		\
  {					\
	write(1, s, l);			\
	XP_FREE(s);			\
	return(0);			\
  }
#else /* STANDALONE_TEST */
#define CCC_PUTBLOCK(obj, s, l)						\
  {									\
	int rv;								\
	rv = (*(obj)->next_stream->put_block)				\
		((obj)->next_stream->data_object, (const char *) s, l);	\
	XP_FREE(s);							\
	return(rv);							\
  }
#endif /* STANDALONE_TEST */

#endif /* _INTLPRIV_H_ */
