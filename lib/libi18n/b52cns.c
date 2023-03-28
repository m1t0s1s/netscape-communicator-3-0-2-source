/* -*- Mode: C; tab-width: 4 -*- */
/*	b52cns.c	*/

#include "intlpriv.h"
#include "b5tab.h"


extern int MK_OUT_OF_MEMORY;


/* forward declarations */
static int b5num_cnsnum(int *num);
static int cnsnum_cnscode(int num, unsigned char *code);


/* net_b52cns(obj, b5buf, b5bufsz)
 * Args:
 *	b5buf:		Ptr to a buf of EUC chars
 *	b5bufsz:	Size in bytes of b5buf
 *	obj->uncvtbuf:	If entire buffer was converted, uncvtbuf[0] will be nul,
 *		else this points to EUC chars that were NOT converted
 *		and mz_b52cns() with additional EUC chars appended.
 * Return:
 *	Returns NULL on failure, otherwise it returns a pointer to a buffer of
 *	converted characters.  Caller must XP_FREE() this memory.
 *
 */

int
net_b52cns(CCCDataObject			*obj,
			const unsigned char	*b5buf,	/* EUC buffer for conversion	*/
			int32				b5bufsz)	/* EUC buffer size in bytes		*/
{
	unsigned char	*tobuf = NULL;

	tobuf = mz_b52cns(obj, b5buf, b5bufsz);

	if (tobuf) {
		/* CCC_PUTBLOCK macro does a return()	*/
		CCC_PUTBLOCK(obj, tobuf, strlen((char *)tobuf));
	} else {
		return(obj->retval);
	}
}

#ifdef USE_B5TOCNS_USER_TABLE
#else /* USE_B5TOCNS_USER_TABLE */
#define userword(prec, c)	0
#endif /* USE_B5TOCNS_USER_TABLE */
unsigned char *
mz_b52cns(	CCCDataObject		*obj,
			const unsigned char	*b5buf,	/* EUC buffer for conversion	*/
			int32				b5bufsz)	/* EUC buffer size in bytes		*/
{
 	unsigned char			*tobuf = NULL;
 	int32					tobufsz;
 	register unsigned char	*tobufp, *b5p;		/* current byte in bufs	*/
 	register unsigned char	*tobufep, *b5ep;	/* end of buffers		*/
 	int32					uncvtlen;

	int del = 0;			/* For now dont delete ^M's */
	int wordcnt = 0;		/* No user b5tocns table */
	unsigned char	code[4], prec, c;
	int		state, num, n, cnt;
 										/* Allocate a dest buffer:		*/
		/* CNS takes as much as BIG 5  + 1 for a possible NULL */
	uncvtlen = strlen((const char *)obj->uncvtbuf);
	tobufsz = ((b5bufsz + uncvtlen) * 3) + 1;
	if ((tobuf = (unsigned char *)XP_ALLOC(tobufsz)) == (unsigned char *)NULL) {
		obj->retval = MK_OUT_OF_MEMORY;
		return(NULL);
	}
										/* Initialize pointers, etc.	*/
 	b5p = (unsigned char *)b5buf;
 	b5ep = b5p + b5bufsz - 1;

#define uncvtp	tobufp	/* use tobufp as temp */ 		 		
							/* If prev. unconverted chars, append unconverted
							 * chars w/new chars and try to process.
							 */
 	if (obj->uncvtbuf[0] != '\0') {
 		uncvtp = (unsigned char *)obj->uncvtbuf + uncvtlen;
 		while (uncvtp < ((unsigned char *)obj->uncvtbuf + sizeof(obj->uncvtbuf)) &&
														b5p <= b5ep)
 			*uncvtp++ = *b5p++;
 		*uncvtp = '\0';						/* nul terminate	*/
 		b5p = (unsigned char *)obj->uncvtbuf; /* process unconverted first */
 		b5ep = uncvtp - 1;
 	}
#undef uncvtp
 	
 	tobufp = tobuf;
 	tobufep = tobufp + tobufsz - 2;		/* save space for terminating null */


	code[0] = 0x8e;
	code[1] = 0xa2;
	state = 0;
 	
WHILELOOP: 	
									/* While EUC data && space in dest. buf. */
 	while ((tobufp <= tobufep) && (b5p <= b5ep)) {
		c = *b5p++;
next:
		switch(state) {
		case 0:
			num = -1;
			if (c >= 0xa1 && c <= 0xa3)
				num = 0;
			else if (c >= 0xa4 && c <= 0xc6)
				num = 408;
			else if (c >= 0xc9 && c <= 0xf9)
				num = 5809;
			else if (c >= 0x80 && wordcnt > 0)
				num = -1;
			else {
				if (c != 0x0d || del == 0)
 					*tobufp++ = c;
				break;
			}
			state = 1;
			prec = c;
			break;
		case 1:
			if (num < 0) {
				if (userword(prec, c) == 0)
					goto next1;
				state = 0;
				continue;
			}
			else if (prec == 0xa3)
				if (c >= 0x40 && c <= 0x7e)
					num = 314 + c - 0x40;
				else if (c >= 0xa1 && c <= 0xbf)
					num = 377 + c - 0xa1;
				else
					goto next1;
			else if (prec == 0xc6)
				if (c >= 0x40 && c <= 0x7e)
					num += 5338 + c - 0x40;
				else
					goto next1;
			else if (prec == 0xf9)
				if (c >= 0x40 && c <= 0x7e)
					num += 7536 + c - 0x40;
				else if (c >= 0xa1 && c <= 0xd5)
					num += 7599 + c - 0xa1;
				else {
					if (wordcnt == 0 || userword(prec, c) == 0)
						goto next1;
					state = 0;
					continue;
				}
			else {
				if (num == 0)
					n = (prec - 0xa1)*0x9d;
				else if (num == 408)
					n = (prec - 0xa4)*0x9d;
				else
					n = (prec - 0xc9)*0x9d;
				if (c >= 0x40 && c <= 0x7e)
					num += n + c - 0x40;
				else if (c >= 0xa1 && c <= 0xfe)
					num += n + 0x3f + c - 0xa1;
				else
					goto next1;
			} /* if */
			state = 0;
			if (b5num_cnsnum(&num)) {
				cnt = cnsnum_cnscode(num, &code[2]);
				if (cnt == 1) {
 					*tobufp++ = code[2];
 					*tobufp++ = code[3];
				}
				else if (cnt == 2) {
 					*tobufp++ = code[0];
 					*tobufp++ = code[1];
 					*tobufp++ = code[2];
 					*tobufp++ = code[3];
				}
			}
			else {
				code[0] = code[1] = 0x20;
 				*tobufp++ = code[0];
 				*tobufp++ = code[1];
			}
			continue;
next1:
			if (prec != 0x0d || del == 0)
 				*tobufp++ = prec;
			state = 0;
			goto next;
		} /* switch */
	} /* while */
	if (state)
		/* This last character was unconverted. Retain it in the
		   unconverted buffer. (prec) is unconverted character */
		b5p--;

 	if (obj->uncvtbuf[0] != '\0') {
										/* Just processed unconverted chars:
										 * b5p pts to 1st unprocessed char in
 										 * b5buf.  Some may have been processed
 										 * while processing unconverted chars,
 										 * so set up ptrs not to process them
 										 * twice.
 										 */
										/* If nothing was converted, this can
										 * only happen if there was not
										 * enough EUC data.  Stop and get
										 * more data.
										 */
		if (b5p == (unsigned char *)obj->uncvtbuf) {	/* Nothing converted */
			*tobufp = '\0';
			return(NULL);
		}
 		b5p = (unsigned char *)b5buf +
 							(b5p - (unsigned char *)obj->uncvtbuf - uncvtlen);
 		b5ep = (unsigned char *)b5buf + b5bufsz - 1;	/* save space for nul */
 		obj->uncvtbuf[0] = '\0';		/* No more uncoverted chars.	*/
 		goto WHILELOOP;					/* Process new data				*/
 	}
 	

	*tobufp = '\0';						/* null terminate dest. data */
	obj->len =  tobufp - tobuf;			/* length not counting null	*/

 	if (b5p <= b5ep) {				/* uncoverted EUC?			*/
		tobufp = (unsigned char *)obj->uncvtbuf;/* reuse the tobufp as a TEMP */
 		while (b5p <= b5ep)
 			*tobufp++ = *b5p++;
 		*tobufp = '\0';					/* null terminate			*/
 	}
	return(tobuf);
}


/*
 * num : input b5 sequence number.
 *	 output cns sequence number.
 */
static int	b5num_cnsnum(int	*num)
{
	int	i;

/*	no corresponding sequence number in cns	*/
	if ((*num == 5819) || (*num == 9103))
		return(0);
/*	b5 to cns	*/
	for (i = 0 ; i < B5MAX ; ++i)
		if (*num <= b5tab[i].boundary) {
			*num += b5tab[i].diff;
			return(1);
		}
	return(0);
}

/*
 * num : input cns sequence number.
 * code : output cns code(4 bytes).
 */
static int	cnsnum_cnscode(int		num, unsigned char	*code)
{
	int	rem;

	if ((num < 0) || (num > 13458))
		return(0);
	if (num >= 5809)
		goto second;
	else if (num >= 408)
		goto first;
	if (num < 234) {
		code[0] = 0xa1 + num/94;
		code[1] = 0xa1 + num%94;
	}
	else {
		num -= 234;
		code[0] = 0xa4 + num/94;
		code[1] = 0xa1 + num%94;
	}
	return(1);
first:
	num -= 408;
	code[0] = 0xc4 + num/0x5e;
	rem = num % 0x5e;
	code[1] = 0xa1 + rem;
	return(1);
second:
	num -= 5809;
	code[0] = 0xa1 + num/0x5e;
	rem = num % 0x5e;
	code[1] = 0xa1 + rem;
	return(2);
}

/*
 * This could be used to implement user preferences for convertion.
 * For now we dont
 */

#ifdef USE_B5TOCNS_USER_TABLE
#define		HEAD		4

static int	userword(unsigned charprec, unsigned charc)
{
	int	i;
	unsigned char	code[6];
    static int fd = 0;

    if (fd == -2)
		if ((fd = open("/home/chinese/usrword/b5cns.tbl", O_RDONLY)) < 0) {
			wordcnt = 0;
			fd = -1;
		}
		else
			read(fd, &wordcnt, HEAD);

	if (fd < 0) return;

	lseek(fd, HEAD, 0);
	for (i = 0 ; i < wordcnt ; ++i) {
		if (read(fd, code, 6) != 6)
			break;
		if (code[4] == prec && code[5] == c) {
			sendbuf(code, 4);
			return(1);
		}
	}
	return(0);
}
#endif /* USE_B5TOCNS_USER_TABLE */


#ifdef STANDALONE_TEST
int
main(int argc, char *argv[])
{
 	unsigned char	b5buf[0001];	/* EUC buffer for conversion */
 	int32			rdcnt;			/* #bytes from read		*/
    CCCDataObject	object;
 	
 	object.uncvtbuf[0] = '\0';					/* Init. no unconverted chars */
 	
 	while(rdcnt = read(0, b5buf, sizeof(b5buf))) {
		 if (net_b52cns(&object, b5buf, rdcnt)) {
		 	fprintf(stderr, "b52cns failed\n");
		 	exit(-1);
		 }
	}
}
#endif /* STANDALONE_TEST */
