#include "base64.h"
extern int SEC_ERROR_OUTPUT_LEN;

/* Copyright (C) RSA Data Security, Inc. created 1990.  This is an
   unpublished work protected as such under copyright law.  This work
   contains proprietary, confidential, and trade secret information of
   RSA Data Security, Inc.  Use, disclosure or reproduction without the
   express written authorization of RSA Data Security, Inc. is
   prohibited.
 */

/* Padding character */
#define PAD 0x3d

/* Binary to RFC1113 ascii values */
static unsigned char table[65] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* RFC1113 Ascii to binary reverse table */
static unsigned char *revtable;

struct ATOBContextStr {
    unsigned saved;
    unsigned char buf[4];
};

/* Build table that reverse maps ascii to binary values */
static SECStatus MakeRevTable(void)
{
    unsigned char *rt;
    int i;

    rt = (unsigned char*) PORT_ZAlloc(256);
    if (!rt) {
	return SECFailure;
    }

    revtable = rt;
    for (i = 0; i < 64; i++) {
	rt[table[i]] = i;
    }
    return SECSuccess;
}

ATOBContext *ATOB_NewContext(void)
{
    ATOBContext *cx;

    cx = (ATOBContext*) PORT_ZAlloc(sizeof(ATOBContext));
    return cx;
}

void ATOB_DestroyContext(ATOBContext *cx)
{
    PORT_Assert(cx != NULL);
    PORT_Free(cx);
}

SECStatus ATOB_Begin(ATOBContext *cx)
{
    SECStatus rv;

    if (!revtable) {
	rv = MakeRevTable();
	if (rv) return rv;
    }
    cx->saved = 0;
    return SECSuccess;
}

/*
** Copy from input stream into a buffer, skipping over white space
*/
static int CopyChars(unsigned char *dest, unsigned destLen,
		     const unsigned char **srcp, unsigned *lenp)
{
    unsigned char ch;
    const unsigned char *src = *srcp;
    unsigned len = *lenp;
    int count = 0;
    while (len && destLen) {
	ch = *src++;
	--len;
	if ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n')) {
	    continue;
	}
	*dest++ = ch;
	--destLen;
	count++;
    }
    *lenp = len;
    *srcp = src;
    return count;
}

/*
** Convert an input block to an output block. The input block may have
** PAD characters on the end. Deal with it.
*/
static int ConvertBlock(unsigned char *out, unsigned char *in)
{
    int count;
    unsigned char r0, r1, r2, r3;
    unsigned char *rt = revtable;

    count = 0;
    r0 = rt[in[0]]; /* first 6 bits */
    r1 = rt[in[1]];
    r2 = rt[in[2]];
    r3 = rt[in[3]]; /* last 6 bits */

    if ((in[0] == PAD) || (in[1] == PAD)) {
	/* These are technically error conditions. Just return no data */
	return 0;
    } else if (in[2] == PAD) {
	/* 8 bits is represented with 2 characters and 2 PADs */
	out[0] = (r0 << 2) | ((r1 >> 4) & 0x3);
	return 1;
    } else if (in[3] == PAD) {
	/* 16 bits is represented with 3 characters and 1 PAD */
	out[0] = (r0 << 2) | ((r1 >> 4) & 0x3);
	out[1] = ((r1 & 0xf) << 4) | ((r2 >> 2) & 0xf);
	return 2;
    } else {
	/* 24 bits is represented with 4 characters */
	out[0] = (r0 << 2) | ((r1 >> 4) & 0x3);
	out[1] = ((r1 & 0xf) << 4) | ((r2 >> 2) & 0xf);
	out[2] = ((r2 & 0x3) << 6) | r3;
	return 3;
    }
}

SECStatus ATOB_Update(ATOBContext *cx, unsigned char *output,
		     unsigned int *outputLen, unsigned maxOutputLen,
		     const unsigned char *input, unsigned inputLen)
{
    int amt, saved;
    unsigned char *origOutput;
    SECStatus rv;

    if (!revtable) {
	rv = MakeRevTable();
	if (rv != SECSuccess)
	    return rv;
    }

    /*
    ** Emit complete chunks until input is exhausted. Always use up data
    ** in the saved buffer first.
    */
    origOutput = output;
    saved = cx->saved;
    while (inputLen) {
	/* Get 4 real characters */
	amt = CopyChars(cx->buf + saved, 4 - saved, &input, &inputLen);
	saved += amt;
	if (saved != 4) {
	    break;
	}
	if (maxOutputLen < 3) {
	    /* Buffer is too small */
	    PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	    return SECFailure;
	}
	amt = ConvertBlock(output, cx->buf);
	saved = 0;
	output += amt;
    }
    cx->saved = saved;

    /* Lingering data has been saved for later */
    *outputLen = output - origOutput;
    return SECSuccess;
}

SECStatus ATOB_End(ATOBContext *cx, unsigned char *output,
		  unsigned int *outputLen, unsigned maxOutputLen)
{
    int amt;

    if (cx->saved) {
	PORT_Memset(cx->buf + cx->saved, PAD, 4 - cx->saved);
	if (maxOutputLen < 3) {
	    PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	    return SECFailure;
	}
	amt = ConvertBlock(output, cx->buf);
	*outputLen = amt;
    } else {
	*outputLen = 0;
    }
    return SECSuccess;
}

/************************************************************************/

struct BTOAContextStr {
    unsigned chunks;
    unsigned saved;
    unsigned char buf[3];
};

BTOAContext *BTOA_NewContext(void)
{
    BTOAContext *cx;

    cx = (BTOAContext*) PORT_ZAlloc(sizeof(BTOAContext));
    return cx;
}

void BTOA_DestroyContext(BTOAContext *cx)
{
    PORT_Assert(cx != NULL);
    PORT_Free(cx);
}

SECStatus BTOA_Begin(BTOAContext *cx)
{
    cx->chunks = 0;
    cx->saved = 0;
    return SECSuccess;
}

SECStatus BTOA_Update(BTOAContext *cx, unsigned char *output,
		     unsigned int *outputLen, unsigned maxOutputLen,
		     const unsigned char *input, unsigned inputLen)
{
    int chunks, lines, amount;
    unsigned ch;
    unsigned char *origOutput, *tp;

    /* Make sure remainder buffer is full */
    if (inputLen < 3 - cx->saved) {
	PORT_Memcpy(cx->buf + cx->saved, input, inputLen);
	cx->saved += inputLen;
	*outputLen = 0;
	return SECSuccess;
    }

    /* Compute number of complete chunks & lines to output */
    chunks = (inputLen + cx->saved) / 3;
    lines = (chunks + cx->chunks) / 16;

    /*
    ** Make sure output buffer is large enough. We have to consume all of
    ** the input. We will output one newline per line of output and 64
    ** characters per line.
    */
    if ((unsigned)(lines + 4*chunks) > maxOutputLen) {
	PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	return SECFailure;
    }

    /* Complete saved chunk */
    amount = 3 - cx->saved;
    PORT_Memcpy(cx->buf + cx->saved, input, amount);
    input += amount;
    inputLen -= amount;
    origOutput = output;

    /* Emit saved chunk */
    tp = table;
    output[0] = tp[(cx->buf[0] >> 2) & 0x3f];
    output[1] = tp[((cx->buf[0] << 4) & 0x30) | ((cx->buf[1] >> 4) & 0x0f)];
    output[2] = tp[((cx->buf[1] << 2) & 0x3c) | ((cx->buf[2] >> 6) & 0x03)];
    output[3] = tp[cx->buf[2] & 0x3f];
    ch = cx->chunks + 1;
    if (ch == 16) {
	output[4] = '\n';
	ch = 0;
	output += 5;
    } else {
	output += 4;
    }

    /* Now emit complete chunks until input is exhausted */
    while (inputLen >= 3) {
	output[0] = tp[(input[0] >> 2) & 0x3f];
	output[1] = tp[((input[0] << 4) & 0x30) | ((input[1] >> 4) & 0x0f)];
	output[2] = tp[((input[1] << 2) & 0x3c) | ((input[2] >> 6) & 0x03)];
	output[3] = tp[input[2] & 0x3f];
	if (++ch == 16) {
	    output[4] = '\n';
	    ch = 0;
	    output += 5;
	} else {
	    output += 4;
	}
	input += 3;
	inputLen -= 3;
    }

    /* Save any lingering data for later */
    cx->chunks = ch;
    PORT_Memcpy(cx->buf, input, inputLen);
    cx->saved = inputLen;
    *outputLen = output - origOutput;
    return SECSuccess;
}

SECStatus BTOA_End(BTOAContext *cx, unsigned char *output,
		  unsigned int *outputLen, unsigned maxOutputLen)
{
    if (!cx->saved) {
	if (maxOutputLen < 1) {
	    PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	    return SECFailure;
	}
	*outputLen = 1;
	output[0] = '\n';
    } else {
	unsigned char *in = cx->buf;

	if (maxOutputLen < 5) {
	    PORT_SetError(SEC_ERROR_OUTPUT_LEN);
	    return SECFailure;
	}

	/* Zero unused bytes */
	PORT_Memset(in + cx->saved, 0, 3 - cx->saved);

	output[0] = table[(in[0] >> 2) & 0x3f];
	output[1] = table[((in[0] << 4) & 0x30) | ((in[1] >> 4) & 0x0f)];
	if (cx->saved == 1) {
	    output[2] = PAD;
	    output[3] = PAD;
	} else if (cx->saved == 2) {
	    output[2] = table[((in[1] << 2) & 0x3c) | ((in[2] >> 6) & 0x03)];
	    output[3] = PAD;
	}
	output[4] = '\n';
	*outputLen = 5;
    }
    return SECSuccess;
}

int BTOA_GetLength(int binaryLength)
{
    int chunks, lines;

    if (binaryLength < 1)
	return 1;

    /* Compute number of 4 character pieces */
    chunks = (binaryLength + 2) / 3;

    /* We output at most 16 chunks per line */
    lines = (chunks + 15) / 16;

    /*
    ** This is the max number of bytes of data we could ever need. We add
    ** 1 for the newline between lines. We add the final one for the case
    ** where we have a binaryLength exactly mod 64 (the coder adds an extra
    ** new line at the end)
    */
    return lines * (64 + 1) + 1;
}

char *
BTOA_DataToAscii(const unsigned char *anything, unsigned int ilen)
{
    SECStatus rv;
    BTOAContext *btoa;
    char *retptr;
    unsigned char *tmpptr;
    unsigned int part1, part2, olen;

    btoa = BTOA_NewContext();
    if (btoa == NULL)
	return NULL;

    /*
     * retptr stays NULL until all chance of errors (premature exit)
     * are past
     */
    retptr = NULL;

    olen = BTOA_GetLength(ilen);
    tmpptr = (unsigned char *) PORT_Alloc(olen + 1);	/* 1 for null term */
    if (tmpptr == NULL)
	goto loser;

    rv = BTOA_Begin(btoa);
    if (rv != SECSuccess)
	goto loser;
    rv = BTOA_Update(btoa, tmpptr, &part1, olen, anything, ilen);
    if (rv != SECSuccess)
	goto loser;
    rv = BTOA_End(btoa, tmpptr + part1, &part2, olen - part1);
    if (rv != SECSuccess)
	goto loser;

    retptr = (char *) tmpptr;
    tmpptr = NULL;

    /* Put a null at the end, and strip any trailing newlines */
    retptr[part1 + part2] = '\0';
    while (retptr[part1 + part2 - 1] == '\n') {
	part2--;
	retptr[part1 + part2] = '\0';
    }

loser:
    if (tmpptr)
	PORT_Free(tmpptr);
    BTOA_DestroyContext(btoa);
    return retptr;
}

unsigned char *
ATOB_AsciiToData(const char *anything, unsigned int *olenp)
{
    SECStatus rv;
    ATOBContext *atob;
    unsigned char *retptr;
    unsigned char *tmpptr;
    unsigned int part1, part2, len;

    *olenp = 0;

    atob = ATOB_NewContext();
    if (atob == NULL)
	return NULL;

    /*
     * retptr stays NULL until all chance of errors (premature exit)
     * are past
     */
    retptr = NULL;

    len = PORT_Strlen(anything);
    tmpptr = (unsigned char *) PORT_Alloc(len);
    if (tmpptr == NULL)
	goto loser;

    rv = ATOB_Begin(atob);
    if (rv != SECSuccess)
	goto loser;
    rv = ATOB_Update(atob, tmpptr, &part1, len,
		     (const unsigned char *) anything, len);
    if (rv != SECSuccess)
	goto loser;
    rv = ATOB_End(atob, tmpptr + part1, &part2, len - part1);
    if (rv != SECSuccess)
	goto loser;

    retptr = tmpptr;
    tmpptr = NULL;

    *olenp = part1 + part2;

loser:
    if (tmpptr)
	PORT_Free(tmpptr);
    ATOB_DestroyContext(atob);
    return retptr;
}
