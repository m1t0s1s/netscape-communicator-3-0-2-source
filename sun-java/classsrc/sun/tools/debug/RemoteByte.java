/*
 * @(#)RemoteByte.java	1.4 95/09/14
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

/**
 * The RemoteByte class extends RemoteValue for bytes.
 *
 * @see RemoteValue
 * @see RemoteDebugger
 * @author Thomas Ball
 */
public class RemoteByte extends RemoteValue {
    private byte value;

    RemoteByte(byte b) {
	super(TC_BYTE);
	value = b;
    }
    
    /** Return the byte's value. */
    public byte get() {
	return value;
    }

    /** Print this RemoteValue's type ("byte"). */
    public String typeName() {
	return new String("byte");
    }
    
    /** Return the byte's value as a string. */
    public String toString() {
	return toHex(value);
    }
}
