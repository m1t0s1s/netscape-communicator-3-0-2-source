/*
 * @(#)AudioPlayer.java	1.29 96/02/03 Arthur van Hoff
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
import java.io.FileOutputStream;
import java.lang.SecurityManager;

/**
 * This class provides an interface to play audio streams.
 *
 * To play an audio stream use:
 * <pre>
 *	AudioPlayer.player.start(audiostream);
 * </pre>
 * To stop playing an audio stream use:
 * <pre>
 *	AudioPlayer.player.stop(audiostream);
 * </pre>
 * To play an audio stream from a URL use:
 * <pre>
 *	AudioStream audiostream = new AudioStream(url.openStream());
 *	AudioPlayer.player.start(audiostream);
 * </pre>
 * To play a continuous sound you first have to
 * create an AudioData instance and use it to construct a
 * ContinuousAudioDataStream.
 * For example:
 * <pre>
 *	AudoData data = new AudioStream(url.openStream()).getData();
 *	ContinuousAudioDataStream audiostream = new ContinuousAudioDataStream(data);
 *	AudioPlayer.player.stop(audiostream);
 * </pre>
 *
 * @see AudioData
 * @see AudioDataStream
 * @see AudioDevice
 * @see AudioStream
 * @author Arthur van Hoff, Thomas Ball
 * @version 	1.29, 02/03/96
 */
public
class AudioPlayer extends Thread {

    private AudioDevice devAudio;
    
    /**
     * The default audio player. This audio player is initialized
     * automatically.
     */
    public static final AudioPlayer player = CreateAudioPlayer();

    private static AudioPlayer CreateAudioPlayer() {
       SecurityManager.setScopePermission();
       return new AudioPlayer();
    }

    private static ThreadGroup getAudioThreadGroup() {
	ThreadGroup g = currentThread().getThreadGroup();
	while ((g.getParent() != null) && (g.getParent().getParent() != null)) {
	    g = g.getParent();
	}
	return g;
    }

    /**
     * Construct an AudioPlayer.
     */
    private AudioPlayer() {
	super(getAudioThreadGroup(), "Audio Player");
	devAudio = AudioDevice.device;
    SecurityManager.setScopePermission();
	setPriority(MAX_PRIORITY);
	setDaemon(true);
	SecurityManager.resetScopePermission();
	start();
    }

    /**
     * Start playing a stream. The stream will continue to play
     * until the stream runs out of data, or it is stopped.
     * @see AudioPlayer#stop
     */
    public synchronized void start(InputStream in) {
	devAudio.openChannel(in);
	notify();
    }

    /**
     * Stop playing a stream. The stream will stop playing,
     * nothing happens if the stream wasn't playing in the
     * first place.
     * @see AudioPlayer#start
     */
    public synchronized void stop(InputStream in) {
	devAudio.closeChannel(in);
    }

    /**
     * Main mixing loop. This is called automatically when the AudioPlayer
     * is created.
     */
    public void run() {
	devAudio.open();
	devAudio.play();
	devAudio.close();
	System.out.println("audio player exit");
    }
}
