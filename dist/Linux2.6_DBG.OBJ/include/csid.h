/* -*- Mode: C; tab-width: 4 -*- */
/*	csid.h	*/

#ifndef _CSID_H_
#define _CSID_H_

/* Codeset type */
#define SINGLEBYTE 0x0000 /* 0000 0000 0000 0000 */
#define MULTIBYTE  0x0100 /* 0000 0001 0000 0000 */
#define STATEFUL   0x0200 /* 0000 0010 0000 0000 */
#define WIDECHAR   0x0300 /* 0000 0011 0000 0000 */

/* line-break on spaces */
#define CS_SPACE   0x0400 /* 0000 0100 0000 0000 */

/* Auto Detect Mode */
#define CS_AUTO    0x0800 /* 0000 1000 0000 0000 */


/* Code Set IDs */
/* CS_DEFAULT: used if no charset param in header */
/* CS_UNKNOWN: used for unrecognized charset */

                    /* type                  id   */
#define CS_DEFAULT    (SINGLEBYTE         |   0)
#define CS_ASCII      (SINGLEBYTE         |   1)
#define CS_LATIN1     (SINGLEBYTE         |   2)
#define CS_JIS        (STATEFUL           |   3)
#define CS_SJIS       (MULTIBYTE          |   4)
#define CS_EUCJP      (MULTIBYTE          |   5)

#define CS_JIS_AUTO   (CS_AUTO|STATEFUL   |   3)
#define CS_SJIS_AUTO  (CS_AUTO|MULTIBYTE  |   4)
#define CS_EUCJP_AUTO (CS_AUTO|MULTIBYTE  |   5)

#define CS_MAC_ROMAN  (SINGLEBYTE         |   6)
#define CS_BIG5       (MULTIBYTE          |   7)
#define CS_GB_8BIT    (MULTIBYTE          |   8)
#define CS_CNS_8BIT   (MULTIBYTE          |   9)
#define CS_LATIN2     (SINGLEBYTE         |  10)
#define CS_MAC_CE     (SINGLEBYTE         |  11)
#define CS_KSC_8BIT   (MULTIBYTE|CS_SPACE |  12)
#define CS_2022_KR    (STATEFUL           |  13)
#define CS_8859_3     (SINGLEBYTE         |  14)
#define CS_8859_4     (SINGLEBYTE         |  15)
#define CS_8859_5     (SINGLEBYTE         |  16)	/* ISO Cyrillic */
#define CS_8859_6     (SINGLEBYTE         |  17)	/* ISO Arabic */
#define CS_8859_7     (SINGLEBYTE         |  18)	/* ISO Greek */
#define CS_8859_8     (SINGLEBYTE         |  19)	/* ISO Hebrew */
#define CS_8859_9     (SINGLEBYTE         |  20)
#define CS_SYMBOL     (SINGLEBYTE         |  21)
#define CS_DINGBATS   (SINGLEBYTE         |  22)
#define CS_DECTECH    (SINGLEBYTE         |  23)
#define CS_CNS11643_1 (MULTIBYTE          |  24)
#define CS_CNS11643_2 (MULTIBYTE          |  25)
#define CS_JISX0208   (MULTIBYTE          |  26)
#define CS_JISX0201   (SINGLEBYTE         |  27)
#define CS_KSC5601    (MULTIBYTE          |  28)
#define CS_TIS620     (SINGLEBYTE         |  29)
#define CS_JISX0212   (MULTIBYTE          |  30)
#define CS_GB2312     (MULTIBYTE          |  31)
#define CS_UCS2       (WIDECHAR           |  32)
#define CS_UCS4       (WIDECHAR           |  33)
#define CS_UTF8       (MULTIBYTE          |  34)
#define CS_UTF7       (STATEFUL           |  35)
#define CS_NPC        (MULTIBYTE          |  36)
#define CS_X_BIG5     (MULTIBYTE          |  37)
#define CS_USRDEF2    (SINGLEBYTE         |  38)

#define CS_KOI8_R     (SINGLEBYTE         |  39)
#define CS_MAC_CYRILLIC     (SINGLEBYTE   |  40)
#define CS_CP_1251    (SINGLEBYTE         |  41)	/* CS_CP_1251 is window Cyrillic */
#define CS_MAC_GREEK  (SINGLEBYTE         |  42)
/* CS_CP_1253 should be delete we should use CS_8859_7 instead */
#define CS_CP_1253    (SINGLEBYTE         |  43)	/* CS_CP_1253 is window Greek */
#define CS_CP_1250    (SINGLEBYTE         |  44)	/* CS_CP_1250 is window Centrl Europe */
/* CS_CP_1254 should be delete we should use CS_8859_9 instead */
#define CS_CP_1254    (SINGLEBYTE         |  45)	/* CS_CP_1254 is window Turkish */
#define CS_MAC_TURKISH (SINGLEBYTE        |  46)	
#define CS_GB2312_11  (MULTIBYTE          |  47)
#define CS_JISX0208_11 (MULTIBYTE         |  48)
#define CS_KSC5601_11 (MULTIBYTE          |  49)
#define CS_CNS11643_1110 (MULTIBYTE       |  50)

#define INTL_CHAR_SET_MAX                    51    /* must be highest + 1 */


#define CS_USER_DEFINED_ENCODING    (SINGLEBYTE          |  254)
#define CS_UNKNOWN    (SINGLEBYTE         | 255)

#endif /* _CSID_H_ */
