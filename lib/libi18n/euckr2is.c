/* -*- Mode: C; tab-width: 4 -*- */
/*	euc-kt to iso2022-kr	*/

#include "intlpriv.h"


extern int MK_OUT_OF_MEMORY;


/* net_euckr2iso(obj, eucbuf, eucbufsz)
 * Args:
 *	eucbuf:		Ptr to a buf of EUC chars
 *	eucbufsz:	Size in bytes of eucbuf
 *	obj->uncvtbuf:	If entire buffer was converted, uncvtbuf[0] will be nul,
 *		else this points to EUC chars that were NOT converted
 *		and mz_euckr2iso() with additional EUC chars appended.
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
net_euckr2iso(CCCDataObject		*obj,
			const unsigned char	*eucbuf,	/* EUC buffer for conversion	*/
			int32				eucbufsz)	/* EUC buffer size in bytes		*/
{
	unsigned char	*tobuf = NULL;

	tobuf = mz_euckr2iso(obj, eucbuf, eucbufsz);

	if (tobuf) {
		/* CCC_PUTBLOCK macro does a return()	*/
		CCC_PUTBLOCK(obj, tobuf, strlen((char *)tobuf));
	} else {
		return(obj->retval);
	}
}

unsigned char *
mz_euckr2iso(	CCCDataObject		*obj,
			const unsigned char	*eucbuf,	/* EUC buffer for conversion	*/
			int32				eucbufsz)	/* EUC buffer size in bytes		*/
{
 	char unsigned			*tobuf = NULL;
 	int32					tobufsz;
 	register unsigned char	*tobufp, *eucp;		/* current byte in bufs	*/
 	register unsigned char	*tobufep, *eucep;	/* end of buffers		*/
 	int32					uncvtlen;

 	/* Allocate a dest buffer:
	 * ISO2022 would be at worst 3 times that of EUC
	 */
	uncvtlen = strlen((char *)obj->uncvtbuf);
	tobufsz = (eucbufsz + uncvtlen) * 3;
	if ((tobuf = (unsigned char *)XP_ALLOC(tobufsz)) == (unsigned char *)NULL) {
		obj->retval = MK_OUT_OF_MEMORY;
		return(NULL);
	}

	/* Initialize pointers, etc. */
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
		if (*eucp & 0x80) {
			if (eucp+1 > eucep)			/* No 2nd byte in EUC buffer?	*/
 				break;
			if (!IsIns5601_87_ESC(obj)) {
				Ins5601_87_ESC(tobufp, obj);
			}
			if (IsIns5601_87_SI(obj)) {
				Ins5601_87_SO(tobufp, obj);
			}
			*tobufp++ = *eucp++ & 0x7f;
			*tobufp++ = *eucp++ & 0x7f;
		}
		else {
			if (IsIns5601_87_SO(obj)) {
				Ins5601_87_SI(tobufp, obj);
			}
			*tobufp++ = *eucp++;
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
		if (net_euckr2iso(&object, eucbuf, rdcnt)) {
		 	fprintf(stderr, "euckr2iso failed\n");
		 	exit(-1);
		}
	}
}
#endif /* STANDALONE_TEST */
