/*
 * @(#)RemoteInt.java	1.5 95/09/14
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
 * The RemoteInt class extends RemoteValue for ints.
 *
 * @see RemoteValue
 * @see RemoteDebugger
 * @author Thomas Ball
 */
public class RemoteInt extends RemoteValue {
    private int value;

    public RemoteInt(int i) {
	super(TC_INT);
	value = i;
    }
    
    /** Return the int's value. */
    public int get() {
	return value;
    }

    /** Print this RemoteValue's type ("int"). */
    public String typeName() {
	return new String("int");
    }
    
    /** Return the int's value as a string. */
    public String toString() {
	return new Integer(value).toString();
    }
}
