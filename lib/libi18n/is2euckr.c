/* -*- Mode: C; tab-width: 4 -*- */
/*	is2euckr.c	*/

#include "intlpriv.h"


extern int MK_OUT_OF_MEMORY;


/* net_iso2euckr(obj, isobuf, isobufsz, uncvtbuf)
 * Args:
 *	isobuf:		Ptr to a buf of iso-2022-kr chars
 *	isobufsz:	Size in bytes of isobuf
 *	obj->jismode:	Ptr to encoding mode, use as arg for next call to
 *		mz_iso2euckr() for rest of current 2022-kr data.  First call should
 *		initialize mode to ASCII (0).					
 *	obj->uncvtbuf:	If entire buffer was converted, uncvtbuf[0] will be nul,
 *		else this points to iso-2022-kr chars that were NOT converted
 *		and mz_iso2euckr() with additional iso-2022-kr chars appended.
 * Return:
 *	Returns NULL on failure, otherwise it returns a pointer to a buffer of
 *	converted EUC-KR characters.  Caller must XP_FREE() this memory.
 *
 * Description:
 *
 *	Allocate destination buffer (for EUC-KR).
 *
 *	Set mode state based upon ESC sequence and SO/SI.
 *
 *  If mode is KSC 5601, set 8th bits of next 2 bytes.
 *
 *	If any other mode, then assume ASCII and strip the 8th bit.
 *
 *	If either 2022-kr buffer does not contain complete char or EUC-KR buffer
 *	is full, then return unconverted 2022-kr to caller.  Caller should
 *	append more data and recall mz_iso2euckr.
 */

int
net_iso2euckr(	CCCDataObject		*obj,
				const unsigned char	*isobuf,	/* 2022-kr buf for conv */
				int32				isobufsz)	/* 2022-kr buf size in bytes */
{
	unsigned char	*tobuf = NULL;

	tobuf = mz_iso2euckr(obj, isobuf, isobufsz);

	if (tobuf) {
		/* CCC_PUTBLOCK macro does a return()	*/
		CCC_PUTBLOCK(obj, tobuf, strlen((char *)tobuf));
	} else {
		return(obj->retval);
	}
}

unsigned char *
mz_iso2euckr(	CCCDataObject		*obj,
			const unsigned char	*isobuf,	/* 2022-kr buffer for conversion */
			int32				isobufsz)	/* 2022-kr buffer size in bytes	*/
{
 	unsigned char			*tobuf = NULL;
 	int32					tobufsz;
 	unsigned char	*tobufp, *isop;		/* current byte in bufs	*/
 	unsigned char	*tobufep, *isoep;	/* end of buffers		*/
 	int32					uncvtlen;

#define euckrbufsz	tobufsz
#define euckrbuf		tobuf
#define euckrp		tobufp
#define euckrep		tobufep
 										/* Allocate a dest buffer:		*/
		/* 2022-kr is usually longer than EUC-KR because of ESC seq.
		 *
		 * In the worst case (all ASCII), converted EUC-KR will be the same
		 * length as the original 2022-kr + 1 for nul byte
		 */
	uncvtlen = strlen((char *)obj->uncvtbuf);
	tobufsz = isobufsz + uncvtlen + 1;

	if (!tobufsz) {
		return NULL;
	}

	if ((tobuf = (unsigned char *)XP_ALLOC(tobufsz)) == (unsigned char *)NULL) {
		obj->retval = MK_OUT_OF_MEMORY;
		return(NULL);
	}
										/* Initialize pointers, etc.	*/
 	isop = (unsigned char *)isobuf;
 	isoep = isop + isobufsz - 1;

#define uncvtp	tobufp	/* use tobufp as temp */ 		 		
							/* If prev. unconverted chars, append unconverted
							 * chars w/new chars and try to process.
							 */
 	if (obj->uncvtbuf[0] != '\0') {
 		uncvtp = obj->uncvtbuf + uncvtlen;
 		while (uncvtp < (obj->uncvtbuf + sizeof(obj->uncvtbuf)) &&
														isop <= isoep)
 			*uncvtp++ = *isop++;
 		*uncvtp = '\0';						/* nul terminate	*/
 		isop = obj->uncvtbuf;				/* process unconverted first */
 		isoep = uncvtp - 1;
 	}
#undef uncvtp
 	
 	tobufp = tobuf;
 	tobufep = tobufp + tobufsz - 2;		/* save space for terminating null */
 	
WHILELOOP: 	
							/* While 2022-kr data && space in EUC-KR buf. */
 	while ((tobufp <= tobufep) && (isop <= isoep)) {
		if (*isop == ESC) {
 			if ((isoep - isop) < 3)	/* Incomplete ESC seq in 2022-kr buf? */
 				break;
			switch (isop[1]) {
				case DOLLAR:
					switch (isop[2]) {
						case ')':
							switch (isop[3]) {
								case 'C':			/* KS C 5601-1987	*/
									obj->jismode = KSC_5601_87;
									isop += 4;	/* remove rest of ESC seq.	*/
									break;
								default:		/* pass thru invalid ESC seq. */
									*tobufp++ = *isop++;
									*tobufp++ = *isop++;
									*tobufp++ = *isop++;
									*tobufp++ = *isop++;
									break;
							}
							break;
						default:				/* pass thru invalid ESC seq. */
							*tobufp++ = *isop++;
							*tobufp++ = *isop++;
							*tobufp++ = *isop++;
					}
					break;
				default:						/* pass thru invalid ESC seq. */
					*tobufp++ = *isop++;
					*tobufp++ = *isop++;
			}
		} else if (*isop == SO) {
			obj->jismode |= SHIFT_OUT;
			isop++;
		} else if (*isop == SI) {
			obj->jismode &= (~SHIFT_OUT);
			isop++;
		} else if (obj->jismode == (KSC_5601_87 | SHIFT_OUT)) {
 			if ((isop+1) > isoep)		/* Incomplete 2Byte char in JIS buf? */
 				break;

			*euckrp++ = *isop++ | 0x80;
			*euckrp++ = *isop++ | 0x80;
		} else {
											/* Unknown type: no conversion	*/
			*tobufp++ = *isop++ & 0x7f;
		}
	}
	
 	if (obj->uncvtbuf[0] != '\0') {
										/* Just processed unconverted chars:
										 * isop pts to 1st unprocessed char in
 										 * isobuf.  Some may have been processed
 										 * while processing unconverted chars,
 										 * so set up ptrs not to process them
 										 * twice.
 										 */
										/* If nothing was converted, this can
										 * only happen if there was not
										 * enough 2022-kr data.  Stop and get
										 * more data.
										 */
		if (isop == obj->uncvtbuf) {	/* Nothing converted */
			*tobufp = '\0';
			return(NULL);
		}
 		isoep = (unsigned char *)isobuf + isobufsz - 1 ;
 		isop = (unsigned char *)isobuf + (isop - obj->uncvtbuf - uncvtlen);
 		obj->uncvtbuf[0] = '\0';			/* No more uncoverted chars.	*/
 		goto WHILELOOP;					/* Process new data				*/
 	}

	*tobufp = '\0';						/* null terminate dest. data */
	obj->len =  tobufp - tobuf;			/* length not counting null	*/

 	if (isop <= isoep) {				/* unconverted 2022-kr?		*/
		tobufp = obj->uncvtbuf;				/* reuse the tobufp as a TEMP */
 		while (isop <= isoep)
 			*tobufp++ = *isop++;
 		*tobufp = '\0';					/* null terminate		*/
 	}
	return(tobuf);
}

#ifdef STANDALONE_TEST
int
main(int argc, char *argv[])
{
 	unsigned char	isobuf[0001];	/* iso-2022-kr buffer for conversion */
 	int32			rdcnt;			/* #bytes from read		*/
    CCCDataObject	object;
 	
 	object.jismode = 0;							/* initially ASCII */
 	object.uncvtbuf[0] = '\0';					/* Init. no unconverted chars */
 	
 	while(rdcnt = read(0, isobuf, sizeof(isobuf))) {
		if (net_iso2euckr(&object, isobuf, rdcnt)) {
		 	fprintf(stderr, "mz_iso2euckr failed\n");
		 	exit(-1);
		}
	}
}
#endif /* STANDALONE_TEST */
