/*
 * @(#)gifdecoder.c	1.2 95/10/11 Jim Graham
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

#include <stdio.h>
#ifdef XP_MAC
#include "j_awt_image_IndexColorModel.h"
#include "jdk_java_io_InputStream.h"
#else
#include "java_awt_image_IndexColorModel.h"
#include "java_io_InputStream.h"
#endif
#include "sun_awt_image_GifImageDecoder.h"

#if defined(XP_PC) && !defined(_WIN32)
#if !defined(stderr)
extern FILE *stderr;
#endif
#if !defined(stdout)
extern FILE *stdout;
#endif
#endif

#define OUTCODELENGTH 1025

long
sun_awt_image_GifImageDecoder_parseImage(Hsun_awt_image_GifImageDecoder *this,
					 long width, long height,
					 long interlace, long initCodeSize,
					 HArrayOfByte *blockh,
					 HArrayOfByte *raslineh)
{
    /* Patrick Naughton:
     * Note that I ignore the possible existence of a local color map.
     * I'm told there aren't many files around that use them, and the
     * spec says it's defined for future use.  This could lead to an
     * error reading some files.
     *
     * Start reading the image data. First we get the intial code size
     * and compute decompressor constant values, based on this code
     * size.
     *
     * The GIF spec has it that the code size is the code size used to
     * compute the above values is the code size given in the file,
     * but the code size used in compression/decompression is the code
     * size given in the file plus one. (thus the ++).
     *
     * Arthur van Hoff:
     * The following narly code reads LZW compressed data blocks and
     * dumps it into the image data. The input stream is broken up into
     * blocks of 1-255 characters, each preceded by a length byte.
     * 3-12 bit codes are read from these blocks. The codes correspond to
     * entry is the hashtable (the prefix, suffix stuff), and the appropriate
     * pixels are written to the image.
     */
#ifdef TRIMMED
    static int verbose = 0;
#endif

    int32_t clearCode = (1L << initCodeSize);
    int32_t eofCode = clearCode + 1;
    int32_t bitMask;
    int32_t curCode;
    int32_t outCount;

    /* Variables used to form reading data */
    int32_t blockEnd = 0;
    int32_t remain = 0;
    int32_t byteoff = 0;
    int32_t accumbits = 0;
    int32_t accumdata = 0;

    /* Variables used to decompress the data */
    int32_t codeSize = initCodeSize + 1;
    int32_t maxCode = 1L << codeSize;
    int32_t codeMask = maxCode - 1;
    int32_t freeCode = clearCode + 2;
    int32_t code = 0;
    int32_t oldCode = 0;
    unsigned char prevChar = 0;

    /* Temproray storage for decompression */
    short prefix[4096];
    unsigned char suffix[4096];
    unsigned char outCode[OUTCODELENGTH];
    unsigned char *rasline;
    unsigned char *block;
    int32_t blockLength;

    /* Variables used for writing pixels */
    int32_t x = width;
    int32_t y = 0;
    int32_t off = 0;
    int32_t pass = 0;
    int32_t len;

    ExecEnv *ee = EE();
    Hjava_io_InputStream *input;

    if (blockh == 0 || raslineh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    input = unhand(this)->input;
    rasline = (unsigned char *) unhand(raslineh)->body;
    block = (unsigned char *) unhand(blockh)->body;
    bitMask = unhand(unhand(this)->model)->map_size - 1;

#ifndef TRIMMED
    if (verbose) {
	printf("Decompressing...");
	fflush(stdout);
    }
#endif /* TRIMMED */

    /* Read codes until the eofCode is encountered */
    for (;;) {
	if (accumbits < codeSize) {
	    /* fill the buffer if needed */
	    remain -= 2;
	    while (remain < 0 && !blockEnd) {
		/* move remaining bytes to the beginning of the buffer */
		block[0] = block[byteoff];
		byteoff = 0;

		/* read the next block length */
		blockLength = execute_java_dynamic_method(ee, (void *) input,
							  "read", "()I");
		if (exceptionOccurred(ee)) {
		    return 0;
		}
		if (blockLength < 0) {
		    /*	throw new IOException(); */
		    if (off > 0) {
			execute_java_dynamic_method(ee, (void *) this,
						    "sendPixels",
						    "(II[B)I",
						    y, width, raslineh);
			if (exceptionOccurred(ee)) {
			    return 0;
			}
		    }
		    /* quietly accept truncated GIF images */
		    return 1;
		}
		if (blockLength == 0) {
		    blockEnd = 1;
		}

		/* fill the block */
		if (!execute_java_dynamic_method(ee, (void *) this,
						 "readBytes", "([BII)Z",
						 blockh, remain + 2,
						 blockLength)
		    || exceptionOccurred(ee))
		{
		    /*	throw new IOException(); */
		    /* quietly accept truncated GIF images */
		    return 1;
		}
		remain += blockLength;
	    }

	    /* 2 bytes at a time saves checking for accumbits < codeSize.
	     * We know we'll get enough and also that we can't overflow
	     * since codeSize <= 12.
	     */
	    accumdata += (block[byteoff++] & 0xffL) << accumbits;
	    accumbits += 8;
	    accumdata += (block[byteoff++] & 0xffL) << accumbits;
	    accumbits += 8;
	}

	/* Compute the code */
	code = accumdata & codeMask;
	accumdata >>= codeSize;
	accumbits -= codeSize;

	/*
	 * Interpret the code
	 */
	if (code == clearCode) {
	    /* Clear code sets everything back to its initial value, then
	     * reads the immediately subsequent code as uncompressed data.
	     */
#ifndef TRIMMED
	    if (verbose) {
		printf(".");
		fflush(stdout);
	    }
#endif /* TRIMMED */

	    /* Note that freeCode is one less than it is supposed to be,
	     * this is because it will be incremented next time round the loop
	     */
	    freeCode = clearCode + 1;
	    codeSize = initCodeSize + 1;
	    maxCode = 1L << codeSize;
	    codeMask = maxCode - 1;

	    /* Continue if we've NOT reached the end, some Gif images
	     * contain bogus codes after the last clear code.
	     */
	    if (y < height) {
		continue;
	    }

	    /* pretend we've reached the end of the data */
	    code = eofCode;
	}

	if (code == eofCode) {
	    /* make sure we read the whole block of pixels. */
	    if (!blockEnd) {
		execute_java_dynamic_method(ee, (void *) input, "read", "()I");
	    }
	    return 1;
	} 

	/* It must be data: save code in CurCode */
	curCode = code;
	outCount = OUTCODELENGTH;

	/* If greater or equal to freeCode, not in the hash table
	 * yet; repeat the last character decoded
	 */
	if (curCode >= freeCode) {
	    curCode = oldCode;
	    outCode[--outCount] = prevChar;
	}

	/* Unless this code is raw data, pursue the chain pointed
	 * to by curCode through the hash table to its end; each
	 * code in the chain puts its associated output code on
	 * the output queue.
	 */
	 while (curCode > bitMask) {
	     outCode[--outCount] = suffix[curCode];
	     curCode = prefix[curCode];
	 }

	/* The last code in the chain is treated as raw data. */
	prevChar = (unsigned char)curCode;
	outCode[--outCount] = prevChar;

	/* Now we put the data out to the Output routine. It's
	 * been stacked LIFO, so deal with it that way...
	 */
	len = OUTCODELENGTH - outCount;
	while (--len >= 0) {
	    rasline[off++] = outCode[outCount++];

	    /* Update the X-coordinate, and if it overflows, update the
	     * Y-coordinate
	     */
	    if (--x == 0) {
		/* If a non-interlaced picture, just increment y to the next
		 * scan line.  If it's interlaced, deal with the interlace as
		 * described in the GIF spec.  Put the decoded scan line out
		 * to the screen if we haven't gone past the bottom of it
		 */
		int32_t count = execute_java_dynamic_method(ee, (void *) this,
							"sendPixels",
							"(II[B)I",
							y, width, raslineh);
		if (count <= 0 || exceptionOccurred(ee)) {
		    /* Nobody is listening any more. */
#ifndef TRIMMED
		    if (verbose) {
			printf("Orphan gif decoder quitting\n");
		    }
#endif /* TRIMMED */
		    return 0;
		}
		x = width;
		off = 0;
		if (interlace) {
		    switch (pass) {
		    case 0:
			y += 8;
			if (y >= height) {
			    pass++;
			    y = 4;
			}
			break;
		    case 1:
			y += 8;
			if (y >= height) {
			    pass++;
			    y = 2;
			}
			break;
		    case 2:
			y += 4;
			if (y >= height) {
			    pass++;
			    y = 1;
			}
			break;
		    case 3:
			y += 2;
			break;
		    }
		} else {
		    y++;
		}

		/* Some files overrun the end */
		if (y >= height) {
		    return 1;
		}
	    }
	}

	/* Build the hash table on-the-fly. No table is stored in the file. */
	prefix[freeCode] = (short)oldCode;
	suffix[freeCode] = prevChar;
	oldCode = code;

	/* Point to the next slot in the table.  If we exceed the
	 * maxCode, increment the code size unless
	 * it's already 12.  If it is, do nothing: the next code
	 * decompressed better be CLEAR
	 */
	if (++freeCode >= maxCode) {
	    if (codeSize < 12) {
		codeSize++;
		maxCode <<= 1L;
		codeMask = maxCode - 1;
	    }
	}
    }
}
