/*
 * @(#)ContinuousAudioDataStream.java	1.7 95/08/12 Arthur van Hoff
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

package sun.audio;

/**
 * Create a continuous audio stream. This wraps a stream
 * around an AudioData object, the stream is restarted
 * at the beginning everytime the end is reached, thus
 * creating continuous sound.<p>
 * For example:
 * <pre>
 *	AudioData data = AudioData.getAudioData(url);
 *	ContinuousAudioDataStream audiostream = new ContinuousAudioDataStream(data);
 *	AudioPlayer.player.start(audiostream);
 * </pre>
 *
 * @see AudioPlayer
 * @see AudioData
 * @author Arthur van Hoff
 * @version 	1.7, 08/12/95
 */
public
class ContinuousAudioDataStream extends AudioDataStream {
    /**
     * Create a continuous stream of audio.
     */
    public ContinuousAudioDataStream(AudioData data) {
	super(data);
    }

    /**
     * When reaching the EOF, rewind to the beginning.
     */
    public int read() {
	int c = super.read();
	if (c == -1) {
	    reset();
	    c = super.read();
	}
	return c;
    }

    /**
     * When reaching the EOF, rewind to the beginning.
     */
    public int read(byte buf[], int pos, int len) {
	int count = 0;
	while (count < len) {
	    int n = super.read(buf, pos + count, len - count);
	    if (n >= 0) {
		count += n;
	    } else {
		reset();
	    }
	}
	return count;
    }
}
