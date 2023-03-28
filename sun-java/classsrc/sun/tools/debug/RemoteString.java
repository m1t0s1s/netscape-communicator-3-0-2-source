/*
 * @(#)RemoteString.java	1.5 95/11/16
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
 * The RemoteString class allows access to a string in a remote Java
 * interpreter.  
 *
 * @see RemoteDebugger
 * @author Thomas Ball
 */
public class RemoteString extends RemoteObject {

    RemoteString(RemoteAgent agent, int id) {
	super(agent, TP_STRING, id, agent.classString);
    }

    /** Print this RemoteValue's type ("String"). */
    public String typeName() {
	return new String("String");
    }
    
    /** Return the string value, or "null" */
    public String description() {
	return toString();
    }

    /** Return the string value, or "null" */
    public String toString() {
	String retStr = agent.objectToString(id);
	return (retStr.length() == 0) ? new String("null") : retStr;
    }
}
