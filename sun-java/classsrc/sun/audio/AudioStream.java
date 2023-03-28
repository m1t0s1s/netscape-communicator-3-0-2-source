/*
/*
 * @(#)AudioStream.java	1.10 95/08/22 Arthur van Hoff
 *
 * Copyright (c) 1994, 1995 Sun Microsystems, Inc. All Rights Reserved.
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
 * Convert an InputStream to an AudioStream. 
 *
 * @version 	1.10, 08/22/95
 */
public
class AudioStream extends FilterInputStream {

    NativeAudioStream audioIn;
	
    public AudioStream(InputStream in) throws IOException {
	super(in);
	try {
	    audioIn = (new NativeAudioStream(in));
	} catch (InvalidAudioFormatException e) {
	    // Not a native audio stream -- use a translator (if available).
	    // If not, let the exception bubble up.
	    audioIn = (new AudioTranslatorStream(in));
	}
	this.in = audioIn;
    }

    /**
     * A blocking read.
     */
    public int read(byte buf[], int pos, int len) throws IOException {
	int count = 0;
	while (count < len) {
	    int n = super.read(buf, pos + count, len - count);
	    if (n < 0) {
		return count;
	    }
	    count += n;
	    Thread.currentThread().yield();
	}
	return count;
    }

    /**
     * Get the data.
     */
    public AudioData getData() throws IOException {
	byte buffer[] = new byte[audioIn.getLength()];
	int gotbytes = read(buffer, 0, audioIn.getLength());
	close();
	if (gotbytes != audioIn.getLength()) {
	    throw new IOException("audio data read error");
	}
	return new AudioData(buffer);
    }
    
    public int getLength() {
	return audioIn.getLength();
    }
}
