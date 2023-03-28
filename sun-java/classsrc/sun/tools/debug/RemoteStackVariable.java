/*
 * @(#)RemoteStackVariable.java	1.5 96/01/18
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
 * A RemoteStackVariable represents a method argument or local variable.
 * It is similar to a RemoteField, but is much more transient in nature.
 *
 * @see RemoteField
 */
public class RemoteStackVariable extends LocalVariable {
    RemoteValue value;

    RemoteStackVariable(int slot, String name, String signature, 
                        boolean argument, RemoteValue value) {
	this.slot = slot;
	this.name = name;
	this.signature = signature;
        this.methodArgument = argument;
	this.value = value;
    }

    /** Return the name of a stack variable or argument. */
    public String getName() {
	return name;
    }

    /** Return the value of a stack variable or argument. */
    public RemoteValue getValue() {
	return value;
    }

    /** Return whether variable is in scope. */
    public boolean inScope() {
        return (slot != -1);
    }

    /** Return whether variable is a method argument. */
    public boolean methodArgument() {
        return methodArgument;
    }
}
