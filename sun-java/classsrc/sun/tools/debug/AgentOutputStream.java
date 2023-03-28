/*
 * @(#)AgentOutputStream.java	1.5 95/09/14
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

package sun.tools.debug;

import java.io.*;

class AgentOutputStream extends FilterOutputStream implements AgentConstants {

    AgentOutputStream(OutputStream out) {
	super(out);
    }

    synchronized public void write(int b) throws IOException {
	out.write(CMD_WRITE_BYTES);
	out.write(1);
	out.write(b);
    }

    public void write(byte b[]) throws IOException {
	write(b, 0, b.length);
    }

    synchronized public void write(byte b[], int off, int len) throws IOException {
	out.write(CMD_WRITE_BYTES);
	out.write(len);
	out.write(b, off, len);
    }
}
