/* -*- Mode: C; tab-width: 4 -*- */
/*	cns2b5.c	*/

/*	Copyright (c) CCL/ITRI	1990,1991	*/
/*	  All Rights Reserved			*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF CCL/ITRI	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#include "intlpriv.h"

#include "cnstab.h"

#ifdef USE_B5TOCNS_USER_TABLE
#include <fcntl.h>
#include <stropts.h>


#define		HEAD		4

static int	fd;
#endif /* USE_B5TOCNS_USER_TABLE */


extern int MK_OUT_OF_MEMORY;


/* forward declarations */
static int	userword(unsigned char *tmp);
static int	cnsnum_b5num(int *num);
static int	b5num_b5code(int num, unsigned char *code);

static int	wordcnt;

/* net_cns2b5(obj, cnsbuf, cnsbufsz)
 * Args:
 *	cnsbuf:		Ptr to a buf of CNS chars
 *	cnsbufsz:	Size in bytes of cnsbuf
 *	obj->uncvtbuf:	If entire buffer was converted, uncvtbuf[0] will be nul,
 *		else this points to CNS chars that were NOT converted
 *		and mz_cns2b5() with additional CNS chars appended.
 * Return:
 *	Returns NULL on failure, otherwise it returns a pointer to a buffer of
 *	converted characters.  Caller must XP_FREE() this memory.
 *
 * Description:
 *
 *	Allocate destination buffer.
 *
 *  All bytes < SS2 (0x8E) are either ASCII or invalid.  Treat them
 *	as ASCII and just copy them unchanged.
 *
 *	If the next byte < 0xA0, this is not valid CNS, so treat it as
 *	ASCII and just copy it unchanged.
 *
 *	If the next 2 bytes are both >= 0xA0, then this is a valid 2-byte
 *	CNS, so convert them to Big5.  Otherwise, copy the 1st byte and
 *	continue.
 */

int
net_cns2b5(CCCDataObject		*obj,
			const unsigned char	*cnsbuf,	/* CNS buffer for conversion	*/
			int32				cnsbufsz)	/* CNS buffer size in bytes		*/
{
	unsigned char	*tobuf = NULL;

	tobuf = mz_cns2b5(obj, cnsbuf, cnsbufsz);

	if (tobuf) {
		/* CCC_PUTBLOCK macro does a return()	*/
		CCC_PUTBLOCK(obj, tobuf, strlen((char *)tobuf));
	} else {
		return(obj->retval);
	}
}

unsigned char *
mz_cns2b5(	CCCDataObject		*obj,
			const unsigned char	*cnsbuf,	/* CNS buffer for conversion	*/
			int32				cnsbufsz)	/* CNS buffer size in bytes		*/
{
 	char unsigned			*tobuf = NULL;
 	int32					tobufsz;
 	register unsigned char	*tobufp, *cnsp;		/* current byte in bufs	*/
 	register unsigned char	*tobufep, *cnsep;	/* end of buffers		*/
 	int32					uncvtlen;

	unsigned char	code[2], prec, c, buf[2];
	int		state, num;

#ifdef USE_B5TOCNS_USER_TABLE
	if ((fd = open("/home/chinese/usrword/b5cns.tbl", O_RDONLY)) < 0)
		wordcnt = 0;
	else
		read(fd, &wordcnt, 4);
#else
	wordcnt = 0;
#endif /* USE_B5TOCNS_USER_TABLE */

	buf[0] = buf[1] = 0x00;

 										/* Allocate a dest buffer:		*/
		/* Usually CNS will be the same length as Big5.  Big5 will be shorter
		 * if CNS codeset 2 is used, but in the worst case, the converted
		 * Big5 will be the same size as the orignal CNS + 1 for nul byte.
		 */
	uncvtlen = strlen((char *)obj->uncvtbuf);
	tobufsz = cnsbufsz + uncvtlen + 1;
	if ((tobuf = (unsigned char *)XP_ALLOC(tobufsz)) == (unsigned char *)NULL) {
		obj->retval = MK_OUT_OF_MEMORY;
		return(NULL);
	}
										/* Initialize pointers, etc.	*/
 	cnsp = (unsigned char *)cnsbuf;
 	cnsep = cnsp + cnsbufsz - 1;
 	
#define uncvtp	tobufp	/* use tobufp as temp */ 		 		
							/* If prev. unconverted chars, append unconverted
							 * chars w/new chars and try to process.
							 */
 	if (obj->uncvtbuf[0] != '\0') {
 		uncvtp = obj->uncvtbuf + uncvtlen;
 		while (uncvtp < (obj->uncvtbuf + sizeof(obj->uncvtbuf)) &&
														cnsp <= cnsep)
 			*uncvtp++ = *cnsp++;
 		*uncvtp = '\0';							/* nul terminate	*/
 		cnsp = obj->uncvtbuf;					/* process unconverted first */
 		cnsep = uncvtp - 1;
 	}
#undef uncvtp

 	tobufp = tobuf;
 	tobufep = tobufp + tobufsz - 2;		/* save space for terminating null */

WHILELOOP:
	state = 0;
									/* While CNS data && space in dest. buf. */
 	while ((tobufp <= tobufep) && (cnsp <= cnsep)) {
		c = *cnsp++;
next:
		switch(state) {
		case 0:
			if (c >= 0xa1 && c <= 0xa5)
				state = 1;
			else if (c >= 0xc4 && c <= 0xfd)
				state = 2;
			else if (c == 0x8e)
				state = 3;
			else {
				*tobufp++ = c;
				break;
			} /* if */
			prec = c;
			break;
		case 1:
			if ((prec == 0xa1 || prec == 0xa2) && (c >= 0xa1 && c <= 0xfe))
				num = (prec - 0xa1) * 94 + c - 0xa1;
			else if (prec == 0xa3 && c >= 0xa1 && c <= 0xce)
				num = 188 + c - 0xa1;
			else if (prec == 0xa4 && c >= 0xa1 && c <= 0xfe)
				num = 234 + c - 0xa1;
			else if (prec == 0xa5 && c >= 0xa1 && c <= 0xf0)
				num = 328 + c - 0xa1;
			else {
				*tobufp++ = prec;
				state = 0;
				goto next;
			} /* if */
			goto ret;
		case 2:
			if (prec >= 0xc4 && prec <= 0xfc && c >= 0xa1 && c <= 0xfe)
				num = 408 + (prec - 0xc4) * 94 + c - 0xa1;
			else if (prec == 0xfd && c >= 0xa1 && c <= 0xcb)
				num = 408 + 5358 + c - 0xa1;
			else {
				*tobufp++ = prec;
				state = 0;
				goto next;
			} /* if */
			goto ret;
		case 3:
			if (c == 0xa2)
				state = 4;
			else if (c == 0xac) {
				if (wordcnt == 0 || userword(buf) == 0) {
					*tobufp++ = 0x8e;
					*tobufp++ = 0xac;
				}
				state = 0;
				break;
			}
			else {
				*tobufp++ = 0x8e;
				state = 0;
				goto next;
			} /* if */
			break;
		case 4:
			if (c >= 0xa1 && c <= 0xf2) {
				prec = c;
				state = 5;
			}
			else {
				*tobufp++ = 0x8e;
				*tobufp++ = 0xa2;
				state = 0;
				goto next;
			} /* if */
			break;
		case 5:
			if (prec == 0xf2 && c >= 0xa1 && c <= 0xc4)
				num = 5809 + 7614 + c - 0xa1;
			else if (prec >= 0xa1 && prec <= 0xf1 && c >= 0xa1 && c <= 0xfe)
				num = 5809 + (prec - 0xa1) * 0x5e + c - 0xa1;
			else {
				*tobufp++ = 0x8e;
				*tobufp++ = 0xa2;
				*tobufp++ = prec;
				state = 0;
				goto next;
			} /* if */
ret:
			if (cnsnum_b5num(&num)) {
				b5num_b5code(num, code);
				*tobufp++ = code[0];
				*tobufp++ = code[1];
			}
			else {
				switch (state) {
				case 1:
				case 2:
					*tobufp++ = prec;
					*tobufp++ = c;
					break;
				case 5:
					*tobufp++ = 0x8e;
					*tobufp++ = 0xa2;
					*tobufp++ = prec;
					*tobufp++ = c;
					break;
				} /* switch */
			} /* if */
			state = 0;
			continue;
		} /* case */
		if (buf[0] != 0x00) {
			c = buf[0];
			buf[0] = buf[1];
			buf[1] = 0x00;
			goto next;
		}
	} /* while */
	if (state) {
		switch (state) {
		case 1:
		case 2:
		case 3:
			cnsp--;
			break;
		case 4:
			cnsp -= 2;
			break;
		case 5:
			cnsp -= 3;
			break;
		} /* switch */
	} /* if */
	
 	if (obj->uncvtbuf[0] != '\0') {
										/* Just processed unconverted chars:
 										 * cnsp pts to 1st unprocessed char in
 										 * cnsbuf.  Some may have been processed
 										 * while processing unconverted chars,
 										 * so set up ptrs not to process them
 										 * twice.
 										 */
										/* If nothing was converted, this can
										 * only happen if there was not
										 * enough CNS data.  Stop and get
										 * more data.
										 */
		if (cnsp == obj->uncvtbuf) {	/* Nothing converted */
			*tobufp = '\0';
			return(NULL);
		}
 		cnsp = (unsigned char *)cnsbuf + (cnsp - obj->uncvtbuf - uncvtlen);
 		cnsep = (unsigned char *)cnsbuf + cnsbufsz - 1;	/* save space for nul */
 		obj->uncvtbuf[0] = '\0';		 /* No more uncoverted chars.	*/
 		goto WHILELOOP;					/* Process new data				*/
 	}

	*tobufp =  '\0';						/* null terminate dest. data */
	obj->len =  tobufp - tobuf;			/* length not counting null	*/

 	if (cnsp <= cnsep) {				/* uncoverted CNS?		*/
		tobufp = obj->uncvtbuf;			/* reuse the tobufp as a TEMP */
 		while (cnsp <= cnsep)
 			*tobufp++ = *cnsp++;
 		*tobufp = '\0';					/* null terminate		*/
 	}

#ifdef USE_B5TOCNS_USER_TABLE
	close(fd);
#endif /* USE_B5TOCNS_USER_TABLE */

	return(tobuf);
}

/*
 * num : input cns sequence number.
 *	 output b5 sequence number.
 */
static int	cnsnum_b5num(int	*num)
{
	int	i;

/*	cns to b5	*/
	for (i = 0 ; i < CNSMAX ; ++i)
		if (*num <= cnstab[i].boundary) {
			*num += cnstab[i].diff;
			return(1);
		}
	return(0);
}

/*
 * num : input b5 sequence number.
 * code : output b5 code(2 bytes).
 */
static int	b5num_b5code(int		num, unsigned char	*code)
{
	int	rem;

	if ((num < 0) || (num > 13460))
		return(0);
	if (num >= 5809)
		goto second;
	else if (num >= 408)
		goto first;
	code[0] = 0xa1 + num/0x9d;
	rem = num % 0x9d;
	if (rem < 0x3f)
		code[1] = 0x40 + rem;
	else
		code[1] = 0xa1 + (rem-0x3f);
	return(1);
first:
	num -= 408;
	code[0] = 0xa4 + num/0x9d;
	rem = num % 0x9d;
	if (rem < 0x3f)
		code[1] = 0x40 + rem;
	else
		code[1] = 0xa1 + (rem-0x3f);
	return(1);
second:
	num -= 5809;
	code[0] = 0xc9 + num/0x9d;
	rem = num % 0x9d;
	if (rem < 0x3f)
		code[1] = 0x40 + rem;
	else
		code[1] = 0xa1 + (rem-0x3f);
	return(2);
}

#ifdef USE_B5TOCNS_USER_TABLE
static int	userword(unsigned char	*tmp)
{
	unsigned char	code[6], buf[2];
	int		i;

	tmp[0] = 0x00;
	tmp[1] = 0x00;
	if (getcharacter(&buf[0]) < 0)
		return (0);
	if (getcharacter(&buf[1]) < 0) {
		tmp[0] = buf[0];
		return(0);
	}
	lseek(fd, HEAD, 0);
	for (i = 0 ; i < wordcnt ; ++i) {
		if (read(fd, code, 6) != 6)
			break;
		if (code[2] == buf[0] && code[3] == buf[1]) {
			sendbuf(&code[4], 2);
			return(1);
		}
	}
	tmp[0] = buf[0];
	tmp[1] = buf[1];
	return(0);
}
#else
static int	userword(unsigned char	*tmp)
{
	return 0;
}
#endif /* USE_B5TOCNS_USER_TABLE */
				
#ifdef STANDALONE_TEST
int
main(int argc, char *argv[])
{
 	unsigned char	cnsbuf[0001];	/* CNS buffer for conversion */
 	int32			rdcnt;			/* #bytes from read		*/
    CCCDataObject	object;
 	
 	object.uncvtbuf[0] = '\0';					/* Init. no unconverted chars */
 	
 	while(rdcnt = read(0, cnsbuf, sizeof(cnsbuf))) {
		if (net_cns2b5(&object, cnsbuf, rdcnt)) {
		 	fprintf(stderr, "cns2b5 failed\n");
		 	exit(-1);
		}
	}
}
#endif /* STANDALONE_TEST */
