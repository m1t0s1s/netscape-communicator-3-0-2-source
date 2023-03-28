/*
 * @(#)ProgressReport.java	1.5 95/08/22 Chris Warth
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

package sun.net.www;

public 
class ProgressReport
{
    public int recvd = 0;
    public int expected = 0;
    public int	connections = 0;

    public ProgressReport() {}

    public ProgressReport(int rcv, int exp) {
	set(rcv, exp, 0);
    }

    public ProgressReport set(int rcv, int exp, int con) {
	recvd = rcv;
	expected = exp;
	connections = con;

	return this;
    }
}

