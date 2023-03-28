/*
 * @(#)BreakpointQueue.java	1.9 96/02/13
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

class BreakpointQueue extends Object {
    int			pc;
    Thread		thread;
    int			opcode;
    BreakpointQueue	nextQ;
    Throwable		exception;
    int			catch_pc;
    boolean		updated;

    synchronized boolean nextEvent() {
        try {	
	    wait();
	} catch (InterruptedException ex) {
	    // just drop through
	}
	return true;
    }

    synchronized void reset() {
	pc = -1;
    }
}
