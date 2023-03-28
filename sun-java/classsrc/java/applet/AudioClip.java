/*
 * @(#)AudioClip.java	1.4 95/07/29 Arthur van Hoff
 *
 * Copyright (c) 1994-1995 Sun Microsystems, Inc. All Rights Reserved.
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

package java.applet;

/**
 * AudioClip interface. A very high level abstraction
 * of audio.
 *
 * @version 	1.4, 29 Jul 1995
 * @author 	Arthur van Hoff
 */
public interface AudioClip {
    /**
     * Start playing the clip. Each time this is called
     * the clip is restarted from the beginning.
     */
    void play();

    /**
     * Start playing the clip in a loop.
     */
    void loop();

    /**
     * Stop playing the clip.
     */
    void stop();
}
