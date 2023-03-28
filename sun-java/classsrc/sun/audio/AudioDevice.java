/*
 * @(#)AudioDevice.java	1.4 96/03/12 Arthur van Hoff, Thomas Ball
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

import java.util.Vector;
import java.util.Enumeration;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.lang.SecurityManager;

/**
 * This class provides an interface to a Sun audio device.
 *
 * This class emulates systems with multiple audio channels, mixing
 * multiple streams for the workstation's single-channel device.
 *
 * @see AudioData
 * @see AudioDataStream
 * @see AudioStream
 * @see AudioStreamSequence
 * @see ContinuousAudioDataStream
 * @author Arthur van Hoff, Thomas Ball
 * @version 	1.2, 23 Aug 1995
 */
public class AudioDevice {

    private Vector streams;
    private byte ulaw[];
    private int linear[];
    private int dev;

    /*
     * ulaw stuff
     */
    private static final int MSCLICK = 50;
    private static final int MSMARGIN = MSCLICK / 3;
    private static final int BYTES_PER_SAMPLE = 1;
    private static final int SAMPLE_RATE = 8000;

    /* define the add-in bias for 16 bit samples */
    private final static int ULAW_BIAS = 0x84;
    private final static int ULAW_CLIP = 32635;

    private final static int ULAW_TAB[] = {
	-32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
	-23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
	-15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
	-11900, -11388, -10876, -10364,  -9852,  -9340,  -8828,  -8316,
	-7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
	-5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
	-3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
	-2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
	-1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
	-1372,  -1308,  -1244,  -1180,  -1116,  -1052,   -988,   -924,
	-876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
	-620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
	-372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
	-244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
	-120,   -112,   -104,    -96,    -88,    -80,    -72,    -64,
	-56,    -48,    -40,    -32,    -24,    -16,     -8,      0,
	32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
	23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
	15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
	11900,  11388,  10876,  10364,   9852,   9340,   8828,   8316,
	7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
	5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
	3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
	2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
	1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
	1372,   1308,   1244,   1180,   1116,   1052,    988,    924,
	876,    844,    812,    780,    748,    716,    684,    652,
	620,    588,    556,    524,    492,    460,    428,    396,
	372,    356,    340,    324,    308,    292,    276,    260,
	244,    228,    212,    196,    180,    164,    148,    132,
	120,    112,    104,     96,     88,     80,     72,     64,
        56,     48,     40,     32,     24,     16,      8,      0
    };
    private final static int ULAW_LUT[] = {
	0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
	4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
    };

    private native int audioOpen();
    private native void audioClose();
    private synchronized native void audioWrite(byte buf[], int len);

    /**
     * The default audio player. This audio player is initialized
     * automatically.
     */
    public static final AudioDevice device = new AudioDevice();

    /**
     * Create an AudioDevice instance.
     */
    private AudioDevice() {
        try {
	    SecurityManager.setScopePermission();
	    System.loadLibrary("mm3230");
	    SecurityManager.resetScopePermission();
        } catch (UnsatisfiedLinkError e) {
            System.out.println("could not find/load the mmedia library");
        }

	streams = new Vector();
	int bufferSize = ((SAMPLE_RATE * MSCLICK) / 1000) * BYTES_PER_SAMPLE;
	ulaw = new byte[bufferSize];
	linear = new int[bufferSize];
    }

    /**
     *  Open an audio channel.
     */
    public synchronized void openChannel(InputStream in) {
	streams.insertElementAt(in, 0);
	notify();
    }

    /**
     *  Close an audio channel.
     */
    public synchronized void closeChannel(InputStream in) {
	if (streams.removeElement(in)) {
	    try {
		in.close();
	    } catch (IOException e) {
	    }
	}
    }

    /**
     * Open the device (done automatically)
     */
    public synchronized void open() {
	int	ntries = 1;
	int	maxtries = 5;
	while (dev == 0) {
	    dev = audioOpen();
	    if (dev < 0) {
		System.out.println("no audio device");
		return;
	    }
	    if (dev == 0) {
		System.out.println("audio device busy (attempt " + ntries +
				   " out of " + maxtries + ")");
		if ((streams.size() == 0) || (++ntries > maxtries)) {
		    // failed to open the device
		    // close all open streams, wait a while and return
		    closeStreams();
		    return;
		}

		// use wait instead of sleep because this unlocks the
		// current object during the wait.
		try {
		    wait(3000);
		} catch (InterruptedException e) {
		    closeStreams();
		    return;
		}
	    }
	}
    }

    /**
     * Close the device (done automatically)
     */
    public synchronized void close() {
	if (dev != 0) {
	    audioClose();
	    dev = 0;
	}
	closeStreams();
    }

    /**
     * Play one mixed click of data
     */
    private synchronized void mix() {
	int len = ulaw.length;
	byte ubuf[] = ulaw;

	switch (streams.size()) {
	  case 0: {
	    // fill the buffer with silence
	    for (int n = len ; n-- > 0 ;) {
		ubuf[n] = 127;
	    }
	    break;
	  }

	  case 1: {
	    // read from the input stream
	    InputStream in = (InputStream)streams.elementAt(0);
	    int n = 0;
	    try {
		n = in.read(ubuf, 0, len);
	    } catch (IOException e) {
		n = -1;
	    }

	    // Close the stream if needed
	    if (n <= 0) {
		streams.removeElementAt(0);
		n = 0;
		try {
		    in.close();
		} catch (IOException e) {
		}
	    } 

	    // fill the rest of the buffer with silence
	    for (; n < len ; n++) {
		ubuf[n] = 127;
	    }
	    break;
	  }

	  default: {
	    int tab[] = ULAW_TAB;
	    int lbuf[] = linear;
	    int i = streams.size() - 1;

	    // fill linear buffer with the first stream
	    InputStream in = (InputStream)streams.elementAt(i);
	    int n = 0;
	    try {
		n = in.read(ubuf, 0, len);
	    } catch (IOException e) {
		n = -1;
	    }

	    // Close the stream if needed
	    if (n <= 0) {
		streams.removeElementAt(i);
		n = 0;
		try {
		    in.close();
		} catch (IOException e) {
		}
		
	    }

	    // copy the data into the linear buffer
	    for (int j = 0 ; j < n ; j++) {
		lbuf[j] = tab[ubuf[j] & 0xFF];
	    }

	    // zero the rest of the buffer.
	    for (; n < len ; n++) {
		lbuf[n] = 0;
	    }

	    // mix the rest of the streams into the linear buffer
	    while (i-- > 0) {
		in = (InputStream)streams.elementAt(i);
		try {
		    n = in.read(ubuf, 0, len);
		} catch (IOException e) {
		    n = -1;
		}
		if (n <= 0) {
		    streams.removeElementAt(i);
		    n = 0;
		    try {
			in.close();
		    } catch (IOException e) {
		    }
		}
		while (n-- > 0) {
		    lbuf[n] += tab[ubuf[n] & 0xFF];
		}
	    }

	    // convert the linear buffer back to ulaw
	    int lut[] = ULAW_LUT;
	    for (n = len ; n-- > 0 ; ) {
		int sample = lbuf[n];

		/* Get the sample into sign-magnitude. */
		if (sample >= 0) {
		    if (sample > ULAW_CLIP) {
			sample = ULAW_CLIP;	/* clip the magnitude */
		    }
		   
		    /* Convert from 16 bit linear to ulaw. */
		    sample += ULAW_BIAS;
		    int exponent = lut[sample >> 7];
		    int mantissa = (sample >> (exponent + 3)) & 0x0F;
		    sample = ((exponent << 4) | mantissa) ^ 0xFF;
		} else {
		    sample = -sample;
		    if (sample > ULAW_CLIP) {
			sample = ULAW_CLIP;	/* clip the magnitude */
		    }
		    
		    /* Convert from 16 bit linear to ulaw. */
		    sample += ULAW_BIAS;
		    int exponent = lut[sample >> 7];
		    int mantissa = (sample >> (exponent + 3)) & 0x0F;
		    sample = ((exponent << 4) | mantissa) ^ 0x7F;
		}
		ubuf[n] = (byte)sample;
	    }
	  }
	}
    }

    /**
     * Wait for data
     */
    private synchronized boolean waitForData() throws InterruptedException {
	if (streams.size() == 0) {
	    close();
	    wait();
	    open();
	    return true;
	}
	return false;
    }

    /**
     * Play open audio stream(s)
     */
    public void play() {
	try { 
	    long tm = System.currentTimeMillis() - MSMARGIN;

	    while (dev > 0) {
		// wait for data
		if (waitForData()) {
		    tm = System.currentTimeMillis() - MSMARGIN;
		}

		// mix the next bit
		mix();

		// write the next buffer
		audioWrite(ulaw, ulaw.length);
		
		// wait for the time out
		tm += MSCLICK;
		long delay = tm - System.currentTimeMillis();
		if (delay > 0) {
		    Thread.currentThread().sleep(delay);
		} else {
		    // We've lost it, reset the time..
		    //System.out.println("delay2=" + delay);
		    tm = System.currentTimeMillis() - MSMARGIN;
		}
	    }
	} catch (InterruptedException e) {
	    // the thread got interrupted, exit
	}
    }
    
    /**
     * Close streams
     */
    public synchronized void closeStreams() {
	// close the streams be garbage collected
	for (Enumeration e = streams.elements() ; e.hasMoreElements() ; ) {
	    try {
		((InputStream)e.nextElement()).close();
	    } catch (IOException ee) {
	    }
	}
	streams = new Vector();
    }

    /**
     * Number of channels currently open.
     */
    public int openChannels() {
	return streams.size();
    }
}
