/*
 * @(#)RemoteChar.java	1.4 95/09/14
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
 * The RemoteChar class extends RemoteValue for chars.
 *
 * @see RemoteValue
 * @see RemoteDebugger
 * @author Thomas Ball
 */
public class RemoteChar extends RemoteValue {
    private char value;

    RemoteChar(char c) {
	super(TC_CHAR);
	value = c;
    }
    
    /** Return the char's value. */
    public char get() {
	return value;
    }

    /** Print this RemoteValue's type ("char"). */
    public String typeName() {
	return new String("char");
    }
    
    /** Return the char's value as a string. */
    public String toString() {
	return (new Character(value).toString());
    }
}
