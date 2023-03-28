/* -*- Mode: C; tab-width: 4 -*- */
/*	sjis2jis.c	*/

#include "intlpriv.h"


extern int MK_OUT_OF_MEMORY;


									/* SJIS to JIS Algorithm.		*/
#define TwoByteSJIS2JIS(sjisp, jisp, offset) {					\
 	*jisp = (*sjisp++ - offset) << 1; /* assign 1st byte */		\
 	if (*sjisp < 0x9F) {			/* check 2nd SJIS byte */	\
 		*jisp++ -= 1;				/* adjust 1st JIS byte */	\
 		if (*sjisp > 0x7F)										\
 			*jisp++ = *sjisp++ - 0x20;							\
 		else													\
 			*jisp++ = *sjisp++ - 0x1F;							\
 	} else {													\
 		jisp++;													\
 		*jisp++ = *sjisp++ - 0x7E;								\
 	}															\
}
				
/* net_sjis2jis(obj, sjisbuf, sjisbufsz)
 * Args:
 *	sjisbuf:	Ptr to a buf of SJIS chars
 *	sjisbufsz:	Size in bytes of sjisbuf
 *	obj->jismode:	Ptr to encoding mode, use as arg for next call to
 *		mz_sjis2jis() for rest of current SJIS data.  First call should
 *		initialize mode to ASCII (0).					
 *	obj->uncvtbuf:	If entire buffer was converted, uncvtbuf[0] will be null,
 *		else this points to SJIS chars that were NOT converted
 *		and mz_sjis2jis() with additional SJIS chars appended.
 * Return:
 *	Returns NULL on failure, otherwise it returns a pointer to a buffer of
 *	converted SJIS characters.  Caller must XP_FREE() this memory.
 *
 * Description:
 * 	Allocate destination JIS buffer.
 *
 * 	If the SJIS to JIS conversion changes JIS encoding, output proper ESC
 * 	sequence.
 *
 * 	If byte in ASCII range, just copy it to JIS buffer.
 * 	If Half-width SJIS katakana (1 byte), convert to Half-width JIS katakana.
 * 	If 2-byte SJIS, convert to 2-byte JIS.
 * 	Otherwise assume user-defined SJIS, just copy 2 bytes.
 *
 *	If either SJIS buffer does not contain complete SJIS char or JIS buffer
 *	is full, then return unconverted SJIS to caller.  Caller should
 *	append more data and recall mz_sjis2jis.
 */

int
net_sjis2jis(	CCCDataObject		*obj,
				const unsigned char	*sjisbuf,	/* SJIS buf for conversion	*/
				int32				sjisbufsz)	/* SJIS buf size in bytes	*/
{
	unsigned char	*tobuf = NULL;

	tobuf = mz_sjis2jis(obj, sjisbuf, sjisbufsz);

	if (tobuf) {
		/* CCC_PUTBLOCK macro does a return()	*/
		CCC_PUTBLOCK(obj, tobuf, strlen((char *)tobuf));
	} else {
		return(obj->retval);
	}
}

unsigned char *
mz_sjis2jis(	CCCDataObject		*obj,
			const unsigned char	*sjisbuf,	/* SJIS buf for conversion	*/
			int32				sjisbufsz)	/* SJIS buf size in bytes	*/
{
 	unsigned char			*tobuf = NULL;
 	int32					tobufsz;
 	register unsigned char	*sjisp, *tobufp;	/* current byte in bufs	*/
 	register unsigned char	*sjisep, *toep;		/* end of buffers		*/
 	int32					uncvtlen;
 	
 										/* Allocate a JIS buffer:			*/
		/* JIS is longer than SJIS because of ESC seq.  In the worst case
		 * ( alternating Half-width Kana and Roman chars ), converted
		 * JIS will be 4X the size of the original SJIS + 1 for nul byte.
		 * Worst case: single half-width kana:
		 *	ESC ( I KANA ESC ( J
		 */
	uncvtlen = strlen((char *)obj->uncvtbuf);
	tobufsz = ((sjisbufsz + uncvtlen) << 2) + 8;
	if ((tobuf = (unsigned char *)XP_ALLOC(tobufsz)) == (unsigned char *)NULL) {
		obj->retval = MK_OUT_OF_MEMORY;
		return(NULL);
	}
										/* Initialize pointers, etc.	*/
 	sjisp = (unsigned char *)sjisbuf;
 	sjisep = sjisp + sjisbufsz - 1;

#define uncvtp	tobufp	/* use tobufp as temp */ 	
							/* If prev. unconverted chars, append unconverted
							 * chars w/new chars and try to process.
							 */
 	if (obj->uncvtbuf[0] != '\0') {
 		uncvtp = obj->uncvtbuf + uncvtlen;
 		while (uncvtp < (obj->uncvtbuf + sizeof(obj->uncvtbuf)) &&
													sjisp <= sjisep)
 			*uncvtp++ = *sjisp++;
 		*uncvtp = '\0';						/* nul terminate	*/
 		sjisp = obj->uncvtbuf;				/* process unconverted first */
 		sjisep = uncvtp - 1;
 	}
#undef uncvtp
 	
 	tobufp = tobuf;
 	toep = tobufp + tobufsz - 2;		/* save space for terminating null */
 	
WHILELOOP: 	
									/* While SJIS data && space in JIS buf. */
 	while ((sjisp <= sjisep) && (tobufp <= toep)) {
		if (*sjisp < 0x80) {
 										/* ASCII/JIS-Roman 				*/
 			if (obj->jismode != JIS_Roman) {
 				InsRoman_ESC(tobufp, obj);
 			}
 			*tobufp++ = *sjisp++;

 		} else if (*sjisp < 0xA0) {
 										/* 1st byte of 2-byte low SJIS. */
 			if (sjisp+1 > sjisep)		/* No 2nd byte in SJIS buffer?	*/
 				break;

 			if (obj->jismode != JIS_208_83) {
 				Ins208_83_ESC(tobufp, obj);
 			}

 			TwoByteSJIS2JIS(sjisp, tobufp, 0x70);

 		} else if (*sjisp==0xA0) {
										/* SJIS half-width space.	*/
										/* Just treat like Roman??	*/
 			if (obj->jismode != JIS_Roman) {
 				InsRoman_ESC(tobufp, obj);
 			}
 			*tobufp++ = *sjisp++;

 		} else if (*sjisp < 0xE0) {
										/* SJIS half-width katakana		*/
 			if (obj->jismode != JIS_HalfKana) {
 				InsHalfKana_ESC(tobufp, obj);
 			}
 			*tobufp++ = *sjisp & 0x7F;
			sjisp++;
 		} else if (*sjisp < 0xF0) {
										/* 1st byte of 2-byte high SJIS */
 			if (sjisp+1 > sjisep)		/* No 2nd byte in SJIS buffer? */
 				break;

 			if (obj->jismode != JIS_208_83) {
 				Ins208_83_ESC(tobufp, obj);
 			}

 			TwoByteSJIS2JIS(sjisp, tobufp, 0xB0);
 		} else {
										/* User Defined SJIS: copy bytes */
 			if (sjisp+1 > sjisep)		/* No 2nd byte in SJIS buf?	*/
 				break;

 			if (obj->jismode != JIS_208_83) {
 				Ins208_83_ESC(tobufp, obj);
 			}

 			*tobufp++ = *sjisp++;			/* Just copy 2 bytes.	*/
 			*tobufp++ = *sjisp++;
 		}
 	}
 	
 	if (obj->uncvtbuf[0] != '\0') {
 										/* tobufp pts to 1st unprocessed char in
 										 * tobuf.  Some may have been processed
 										 * while processing unconverted chars,
 										 * so set up ptrs not to process them
 										 * twice.
 										 */
 		sjisp = (unsigned char *)sjisbuf + (sjisp - obj->uncvtbuf - uncvtlen);
											/* save space for term. null */
 		sjisep = (unsigned char *)sjisbuf + sjisbufsz - 1;
 		obj->uncvtbuf[0] = '\0';		/* No more uncoverted chars.	*/
 		goto WHILELOOP;					/* Process new data				*/
 	}

 	if (obj->jismode != JIS_Roman) {
 		obj->jismode = JIS_Roman;
 		InsRoman_ESC(tobufp, obj);
	}

	*tobufp = '\0';						/* null terminate JIS data */
	obj->len =  tobufp - tobuf;			/* length not counting null	*/

 	if (sjisp <= sjisep) {				/* uncoverted SJIS?		*/
		tobufp = obj->uncvtbuf;			/* reuse the tobufp as a TEMP */
 		while (sjisp <= sjisep)
 			*tobufp++ = *sjisp++;
 		*tobufp = '\0';					/* null terminate		*/
 	}
	return(tobuf);
}
 
#ifdef STANDALONE_TEST
main()
{
 	unsigned char	sjisbuf[0001];	/* SJIS bufr for conversion */
 	int32			rdcnt;			/* #bytes from read		*/
	CCCDataObject	object;
 	
 	object.jismode = JIS_Roman;					/* initially JIS Roman */
 	object.uncvtbuf[0] = '\0';					/* Init. no unconverted chars */
 	
 	while(rdcnt = read(0, sjisbuf, sizeof(sjisbuf))) {
		if (net_sjis2jis(&object, sjisbuf, rdcnt)) {
		 	fprintf(stderr, "sjis2jis failed\n");
		 	exit(-1);
		}
	}
}
#endif /* STANDALONE_TEST */
