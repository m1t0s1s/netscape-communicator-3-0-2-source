/*
 * @(#)HttpPostBufferStream.java	1.1 95/11/07 Herb Jellinek
 *
 * Copyright (c) 1994-1995 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL or COMMERCIAL purposes and
 * without fee is hereby granted.
 * Please refer to the file http://java.sun.com/copy_trademarks.html
 * for further important copyright and trademark information and to
 * http://java.sun.com/licensing.html for further important licensing
 * information for the Java (tm) Technology.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 *
 * THIS SOFTWARE IS NOT DESIGNED OR INTENDED FOR USE OR RESALE AS ON-LINE
 * CONTROL EQUIPMENT IN HAZARDOUS ENVIRONMENTS REQUIRING FAIL-SAFE
 * PERFORMANCE, SUCH AS IN THE OPERATION OF NUCLEAR FACILITIES, AIRCRAFT
 * NAVIGATION OR COMMUNICATION SYSTEMS, AIR TRAFFIC CONTROL, DIRECT LIFE
 * SUPPORT MACHINES, OR WEAPONS SYSTEMS, IN WHICH THE FAILURE OF THE
 * SOFTWARE COULD LEAD DIRECTLY TO DEATH, PERSONAL INJURY, OR SEVERE
 * PHYSICAL OR ENVIRONMENTAL DAMAGE ("HIGH RISK ACTIVITIES").  SUN
 * SPECIFICALLY DISCLAIMS ANY EXPRESS OR IMPLIED WARRANTY OF FITNESS FOR
 * HIGH RISK ACTIVITIES.
 */

package sun.net.www.protocol.http;

import java.io.*;

/**
 * HttpPostBufferStream: A subclass of ByteArrayOutputStream that disgorges its
 * contents plus a Content-length: spec to a target stream when closed.
 *
 * @version 1.1, 07 Nov 1995
 * @author Herb Jellinek
 */

public class HttpPostBufferStream extends ByteArrayOutputStream {

    OutputStream target;

    static final String contentLengthMsg = "Content-length: ";
    static final String EOL = "\r\n";

    /*
     * Create one.
     * @param target the OutputStream to point at.
     */
    public HttpPostBufferStream(OutputStream target) {
	super();
	this.target = target;
    }

    /*
     * Close the stream and spit out the contents of the buffer.
     * Do <em>not</em> close the target stream.
     */
    public void close() throws IOException {
	// first write the header...
	int msgBufLen = contentLengthMsg.length();
	byte msgBuf[] = new byte[msgBufLen];
	contentLengthMsg.getBytes(0, msgBufLen, msgBuf, 0);
	target.write(msgBuf);

	// ...including the length.
	String sizeStr = size()+EOL+EOL;
	msgBufLen = sizeStr.length();
	msgBuf = new byte[msgBufLen];
	sizeStr.getBytes(0, msgBufLen, msgBuf, 0);
	target.write(msgBuf);

	// then write the buffer itself
	target.write(buf);

	// and a final EOL
	msgBuf = new byte[EOL.length()];
	EOL.getBytes(0, EOL.length(), msgBuf, 0);
	target.write(msgBuf);
    }
}
