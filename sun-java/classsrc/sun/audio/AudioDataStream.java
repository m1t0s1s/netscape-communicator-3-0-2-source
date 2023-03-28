/*
 * @(#)AudioDataStream.java	1.7 95/08/12 Arthur van Hoff
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

import java.io.ByteArrayInputStream;

/**
 * An input stream to play AudioData.
 *
 * @see AudioPlayer
 * @see AudioData
 * @author Arthur van Hoff
 * @version 	1.7, 08/12/95
 */
public
class AudioDataStream extends ByteArrayInputStream {
    /**
     * Constructor
     */
    public AudioDataStream(AudioData data) {
	super(data.buffer);
    }
}
