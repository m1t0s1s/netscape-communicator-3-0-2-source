/*
 * @(#)RemoteValue.java	1.8 95/09/14
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
 * The RemoteValue class allows access to a copy of a value in the
 * remote Java interpreter.  This value may be a primitive type, such
 * as a boolean or float, or an object, class, array, etc.
 *
 * Remote values are not created by the local debugger, but are returned
 * by the remote debugging agent when queried for the values of instance
 * or static variables of known objects, or from local (stack) variables.
 *
 * @see RemoteDebugger
 * @see RemoteArray
 * @see RemoteBoolean
 * @see RemoteByte
 * @see RemoteChar
 * @see RemoteClass
 * @see RemoteDouble
 * @see RemoteFloat
 * @see RemoteInt
 * @see RemoteLong
 * @see RemoteObject
 * @see RemoteShort
 * @see RemoteString
 * @see RemoteThread
 * @see RemoteThreadGroup
 * @author Thomas Ball
 */
public abstract class RemoteValue implements AgentConstants {
    private int type;

    RemoteValue(int type) {
	this.type = type;
    }

    /** Returns the RemoteValue's type. */
    public final int getType() {
	return type;
    }

    /**
     * Returns whether the RemoteValue is an Object (as opposed to
     * a primitive type, such as int).
     */
    public final boolean isObject() {
	return (type == TP_OBJECT || type == TP_CLASS ||
		type == TP_THREAD || type == TP_THREADGROUP ||
		type == TP_STRING);
    }

    /** Returns the RemoteValue's type as a string. */
    public abstract String typeName() throws Exception;

    /** Return a description of the RemoteValue. */
    public String description() {
	return toString();
    }

    /** Convert an int to a hexadecimal string. */
    public static String toHex(int n) {
	char s1[] = new char[8];
	char s2[] = new char[10];

	/* Store digits in reverse order. */
	int i = 0;
	do {
	    int d = n & 0xf;
	    s1[i++] = (char)((d < 10) ? ('0' + d) : ('a' + d - 10));
	} while ((n >>>= 4) > 0);

	/* Now reverse the array. */
	s2[0] = '0';
	s2[1] = 'x';
	int j = 2;
	while (--i >= 0) {
	    s2[j++] = s1[i];
	}
	return new String(s2, 0, j);
    }

    /** Convert hexadecimal strings to ints. */
    public static int fromHex(String hexStr) {
	String str = hexStr.startsWith("0x") ?
	    hexStr.substring(2).toLowerCase() : hexStr.toLowerCase();
	if (hexStr.length() == 0) {
	    throw new NumberFormatException();
	}
	
	int ret = 0;
	for (int i = 0; i < str.length(); i++) {
	    int c = str.charAt(i);
	    if (c >= '0' && c <= '9') {
		ret = (ret * 16) + (c - '0');
	    } else if (c >= 'a' && c <= 'f') {
		ret = (ret * 16) + (c - 'a' + 10);
	    } else {
		throw new NumberFormatException();
	    }
	}
	return ret;
    }
}
