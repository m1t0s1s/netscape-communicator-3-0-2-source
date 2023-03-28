/*
 * @(#)NativeAudioStream.java	1.2 95/09/02 Arthur van Hoff, Thomas Ball
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.audio;

import java.io.InputStream;
import java.io.DataInputStream;
import java.io.FilterInputStream;
import java.io.IOException;

/**
 * A Sun-specific AudioStream that supports the .au file format. 
 *
 * @version 	1.1, 22 Aug 1995
 */
public
class NativeAudioStream extends FilterInputStream {
    private final int SUN_MAGIC  = 0x2e736e64;
    private final int DEC_MAGIC  = 0x2e736400;
    private final int MINHDRSIZE = 24;
    private final int TYPE_ULAW  = 1;

    private int length = 0;

    /**
     * Read header, only sun 8 bit, ulaw encoded, single channel,
     * 8000hz is supported
     */
    public NativeAudioStream(InputStream in) throws IOException {
	super(in);
	DataInputStream data = new DataInputStream(in);
	int magic = data.readInt();
	if (magic != SUN_MAGIC && magic != DEC_MAGIC) {
	    System.out.println("NativeAudioStream: invalid file type.");
	    throw new InvalidAudioFormatException();
	}
	int hdr_size = data.readInt(); // header size
	if (hdr_size < MINHDRSIZE) {
	    System.out.println("NativeAudioStream: wrong header size of " +
			       hdr_size + ".");
	    throw new InvalidAudioFormatException();
	}
	length = data.readInt();
	
	int encoding = data.readInt();
	if (encoding != TYPE_ULAW) {
	    System.out.println("NativeAudioStream: invalid audio encoding.");
	    throw new InvalidAudioFormatException();
	}

	int sample_rate = data.readInt();
	if ((sample_rate / 1000) != 8) {	// allow some slop
	    System.out.println("NativeAudioStream: invalid sample rate of " +
			       sample_rate + ".");
	    throw new InvalidAudioFormatException();
	}

	int channels = data.readInt();
	if (channels != 1) {
	    System.out.println("NativeAudioStream: wrong number of channels. "
			       + "(wanted 1, actual " + channels + ")");
	    throw new InvalidAudioFormatException();
	}
	
	in.skip(hdr_size - MINHDRSIZE);
    }

    public int getLength() {
	return length;
    }
}
