/* -*- Mode: C; tab-width: 4 -*- */
/*	euc2sjis.c	*/

#include "intlpriv.h"


extern int MK_OUT_OF_MEMORY;


/* net_euc2sjis(obj, eucbuf, eucbufsz)
 * Args:
 *	eucbuf:		Ptr to a buf of EUC chars
 *	eucbufsz:	Size in bytes of eucbuf
 *	obj->uncvtbuf:	If entire buffer was converted, uncvtbuf[0] will be nul,
 *		else this points to EUC chars that were NOT converted
 *		and mz_euc2sjis() with additional EUC chars appended.
 * Return:
 *	Returns NULL on failure, otherwise it returns a pointer to a buffer of
 *	converted characters.  Caller must XP_FREE() this memory.
 *
 * Description:
 *
 *	Allocate destination buffer.
 *
 *  All bytes < SS2 (0x8E) are either ASCII/Roman or invalid.  Treat them
 *	as ASCII/Roman and just copy them unchanged.
 *
 *	If SS2, then the next byte should be a Half-width Katakana, just copy
 *	it unchanged after disposing of the SS2 byte.
 *
 *	If SS3, then the next 2 bytes should be JIS x212.  If the bytes
 *	are not in the valide range for JIS x212, then copy the SS3 and
 *	continue.  Leave the 2 byte to be processed from the beginning.
 *
 *	If the next byte < 0xA0, this is not valid EUC, so treat it as
 *	ASCII/Roman and just copy it unchanged.
 *
 *	If the next 2 bytes are both >= 0xA0, then this is a valid 2-byte
 *	x208, so convert them to SJIS.  Otherwise, copy the 1st byte and
 *	continue.
 */

int
net_euc2sjis(CCCDataObject		*obj,
			const unsigned char	*eucbuf,	/* EUC buffer for conversion	*/
			int32				eucbufsz)	/* EUC buffer size in bytes		*/
{
	unsigned char	*tobuf = NULL;

	tobuf = mz_euc2sjis(obj, eucbuf, eucbufsz);

	if (tobuf) {
		/* CCC_PUTBLOCK macro does a return()	*/
		CCC_PUTBLOCK(obj, tobuf, strlen((char *)tobuf));
	} else {
		return(obj->retval);
	}
}

unsigned char *
mz_euc2sjis(	CCCDataObject		*obj,
			const unsigned char	*eucbuf,	/* EUC buffer for conversion	*/
			int32				eucbufsz)	/* EUC buffer size in bytes		*/
{
 	char unsigned			*tobuf = NULL;
 	int32					tobufsz;
 	register unsigned char	*tobufp, *eucp;		/* current byte in bufs	*/
 	register unsigned char	*tobufep, *eucep;	/* end of buffers		*/
 	int32					uncvtlen;

 										/* Allocate a dest buffer:		*/
		/* Usually EUC will be the same length as SJIS.  SJIS will be shorter
		 * if Half-width Kana are used, but in the worst case, the converted
		 * SJIS will be the same size as the orignal EUC + 1 for nul byte.
		 */
	uncvtlen = strlen((char *)obj->uncvtbuf);
	tobufsz = eucbufsz + uncvtlen + 1;
	if ((tobuf = (unsigned char *)XP_ALLOC(tobufsz)) == (unsigned char *)NULL) {
		obj->retval = MK_OUT_OF_MEMORY;
		return(NULL);
	}
										/* Initialize pointers, etc.	*/
 	eucp = (unsigned char *)eucbuf;
 	eucep = eucp + eucbufsz - 1;
 	
#define uncvtp	tobufp	/* use tobufp as temp */ 		 		
							/* If prev. unconverted chars, append unconverted
							 * chars w/new chars and try to process.
							 */
 	if (obj->uncvtbuf[0] != '\0') {
 		uncvtp = obj->uncvtbuf + uncvtlen;
 		while (uncvtp < (obj->uncvtbuf + sizeof(obj->uncvtbuf)) &&
														eucp <= eucep)
 			*uncvtp++ = *eucp++;
 		*uncvtp = '\0';							/* nul terminate	*/
 		eucp = obj->uncvtbuf;					/* process unconverted first */
 		eucep = uncvtp - 1;
 	}
#undef uncvtp

 	tobufp = tobuf;
 	tobufep = tobufp + tobufsz - 2;		/* save space for terminating null */

WHILELOOP:
									/* While EUC data && space in dest. buf. */
 	while ((tobufp <= tobufep) && (eucp <= eucep)) {
		if (*eucp < SS2) {				/* ASCII/JIS-Roman/invalid EUC	*/
 			*tobufp++ = *eucp++;
		} else if (*eucp == SS2) {		/* Half-width katakana		*/
 			if (eucp+1 > eucep)			/* No 2nd byte in EUC buffer?	*/
 				break;
			eucp++;						/* Dispose of SS2			*/
 			*tobufp++ = *eucp++;

		} else if (*eucp == SS3) {		/* JIS X 0212-1990				*/
 			if (eucp+2 > eucep)			/* No 2nd & 3rd bytes in EUC buffer? */
 				break;
			if (*eucp < 0xA0 || *(eucp+1) < 0xA0) {	/* Invalid EUC212	*/
				*tobufp++ = *eucp++;			/* process SS3 only		*/
 			} else {
 				*tobufp++ = *eucp++;		/* SS3						*/
 				*tobufp++ = *eucp++;		/* 1st 212 byte unconverted	*/
 				*tobufp++ = *eucp++;		/* 2nd 212 byte unconverted	*/
			}
		} else if (*eucp < 0xA0) {		/* Invalid EUC: treat as Roman	*/
 			*tobufp++ = *eucp++;
		} else {						/* JIS X 0208					*/
 			if (eucp+1 > eucep)			/* No 2nd byte in EUC buffer?	*/
 				break;
			if (*(eucp+1) < 0xA0) {		/* 1st byte OK, check if 2nd is valid */
				*tobufp++ = *eucp++;		/* process 1 byte as Roman	*/

			} else {				/* Convert EUC-208 to SJIS: 	Same as	*/
									/* jis2other.c's JIS208-to-SJIS algorithm */
									/* except JIS 8th bit is set. */
				unsigned char b;
				b = ((*eucp) & 0x7F);		/* Convert 1st EUC byte to JIS	*/
				if (b < 0x5F)				/* Convert it to SJIS byte	*/
					*tobufp++ = ((b + 1) >> 1) + 0x70;
				else
					*tobufp++ = ((b + 1) >> 1) + 0xB0;
											/* Convert 2nd SJIS byte	*/
				if ((*eucp++) & 1) {		/* if 1st JIS byte is odd	*/
					b = ((*eucp) & 0x7F);	/* convert 2nd EUC byte to JIS	*/
					if (b > 0x5F)
						*tobufp = b + 0x20;
					else
						*tobufp = b + 0x1F;
				} else {
					*tobufp = (*eucp & 0x7F) + 0x7E;
				}
				tobufp++;
				eucp++;
			}
		}
	}
	
 	if (obj->uncvtbuf[0] != '\0') {
										/* Just processed unconverted chars:
 										 * eucp pts to 1st unprocessed char in
 										 * eucbuf.  Some may have been processed
 										 * while processing unconverted chars,
 										 * so set up ptrs not to process them
 										 * twice.
 										 */
										/* If nothing was converted, this can
										 * only happen if there was not
										 * enough EUC data.  Stop and get
										 * more data.
										 */
		if (eucp == obj->uncvtbuf) {	/* Nothing converted */
			*tobufp = '\0';
			return(NULL);
		}
 		eucp = (unsigned char *)eucbuf + (eucp - obj->uncvtbuf - uncvtlen);
 		eucep = (unsigned char *)eucbuf + eucbufsz - 1;	/* save space for nul */
 		obj->uncvtbuf[0] = '\0';		 /* No more uncoverted chars.	*/
 		goto WHILELOOP;					/* Process new data				*/
 	}

	*tobufp =  '\0';						/* null terminate dest. data */
	obj->len =  tobufp - tobuf;			/* length not counting null	*/

 	if (eucp <= eucep) {				/* uncoverted EUC?		*/
		tobufp = obj->uncvtbuf;			/* reuse the tobufp as a TEMP */
 		while (eucp <= eucep)
 			*tobufp++ = *eucp++;
 		*tobufp = '\0';					/* null terminate		*/
 	}
	return(tobuf);
}
				
#ifdef STANDALONE_TEST
main()
{
 	unsigned char	eucbuf[0001];	/* EUC buffer for conversion */
 	int32			rdcnt;			/* #bytes from read		*/
    CCCDataObject	object;
 	
 	object.uncvtbuf[0] = '\0';					/* Init. no unconverted chars */
 	
 	while(rdcnt = read(0, eucbuf, sizeof(eucbuf))) {
		if (net_euc2sjis(&object, eucbuf, rdcnt)) {
		 	fprintf(stderr, "euc2sjis failed\n");
		 	exit(-1);
		}
	}
}
#endif /* STANDALONE_TEST */
