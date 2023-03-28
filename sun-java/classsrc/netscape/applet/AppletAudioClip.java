/*
 * @(#)AppletAudioClip.java	1.9 95/11/13
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

package netscape.applet;


import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.net.URLConnection;
import java.applet.AudioClip;
import java.lang.SecurityManager;
import sun.audio.*;

/**
 * Applet audio clip;
 *
 * @version 	1.9, 13 Nov 1995
 * @author Arthur van Hoff
 */
public class AppletAudioClip implements AudioClip {
    URL url;
    AudioData data;
    InputStream stream;
    
    /**
     * This should really fork a seperate thread to
     * load the data.
     */
    public AppletAudioClip(URL url) {
	InputStream in = null;
	try {
	    try {
		URLConnection uc = url.openConnection();
		uc.setAllowUserInteraction(true);
		in = uc.getInputStream();
		data = new AudioStream(in).getData();
	    } finally {
		if (in != null) {
		    in.close();
		}
	    }
	} catch (IOException e) {
	    data = null;
	}
    }
    
    public synchronized void play() {
	SecurityManager.setScopePermission();
	stop();
	if (data != null) {
	    stream = new AudioDataStream(data);
	    AudioPlayer.player.start(stream);
	}
    }

    public synchronized void loop() {
	SecurityManager.setScopePermission();
	stop();
	if (data != null) {
	    stream = new ContinuousAudioDataStream(data);
	    AudioPlayer.player.start(stream);
	}
    }

    public synchronized void stop() {
	SecurityManager.setScopePermission();
	if (stream != null) {
	    AudioPlayer.player.stop(stream);
	    try {
		stream.close();
	    } catch (IOException e) {
	    }
	}
    }

    public String toString() {
	return getClass().toString() + "[" + url + "]";
    }
}
