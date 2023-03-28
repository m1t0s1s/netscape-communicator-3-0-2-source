/*
/*
 * @(#)InvalidAudioFormatException.java	1.1 95/08/22 Thomas Ball
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
import java.io.IOException;

/**
 * Signals an invalid audio stream for the stream handler.
 */
class InvalidAudioFormatException extends IOException {
    /**
     * Constructor.
     */
    public InvalidAudioFormatException() {
	super();
    }

    /**
     * Constructor with a detail message.
     */
    public InvalidAudioFormatException(String s) {
	super(s);
    }
}
