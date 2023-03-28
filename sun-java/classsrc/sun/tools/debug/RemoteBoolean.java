/*
 * @(#)RemoteBoolean.java	1.5 95/09/14
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
 * The RemoteBoolean class extends RemoteValue for booleans.
 *
 * @see RemoteValue
 * @see RemoteDebugger
 * @author Thomas Ball
 */
public class RemoteBoolean extends RemoteValue {
    private boolean value;

    RemoteBoolean(boolean b) {
	super(TC_BOOLEAN);
	value = b;
    }

    /** Return the boolean's value. */
    public boolean get() {
	return value;
    }

    /** Print this RemoteValue's type ("boolean"). */
    public String typeName() {
	return new String("boolean");
    }

    /** Return the boolean's value as a string. */
    public String toString() {
	return (new Boolean(value).toString());
    }
}
