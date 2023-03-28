/*
 * @(#)MethodType.java	1.9 95/09/14 Arthur van Hoff
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

package sun.tools.java;

/**
 * This class represents an Java method type.
 * It overrides the relevant methods in class Type.
 *
 * @author 	Arthur van Hoff
 * @version 	1.9, 14 Sep 1995
 */
final
class MethodType extends Type {
    /**
     * The return type.
     */
    Type returnType;

    /**
     * The argument types.
     */
    Type argTypes[];

    /**
     * Construct a method type. Use Type.tMethod to create
     * a new method type.
     * @see Type.tMethod
     */
    MethodType(String typeSig, Type returnType, Type argTypes[]) {
	super(TC_METHOD, typeSig);
	this.returnType = returnType;
	this.argTypes = argTypes;
    }

    public Type getReturnType() {
	return returnType;
    }

    public Type getArgumentTypes()[] {
	return argTypes;
    }

    public boolean equalArguments(Type t) {
	if (t.typeCode != TC_METHOD) {
	    return false;
	}
	MethodType m = (MethodType)t;
	if (argTypes.length != m.argTypes.length) {
	    return false;
	}
	for (int i = argTypes.length - 1 ; i >= 0 ; i--) {
	    if (argTypes[i] != m.argTypes[i]) {
		return false;
	    }
	}
	return true;
    }

    public int stackSize() {
	int n = 0;
	for (int i = 0 ; i < argTypes.length ; i++) {
	    n += argTypes[i].stackSize();
	}
	return n;
    }
    
    public String typeString(String id, boolean abbrev, boolean ret) {
	StringBuffer buf = new StringBuffer();
	buf.append(id);
	buf.append('(');
	for (int i = 0 ; i < argTypes.length ; i++) {
	    if (i > 0) {
		buf.append(", ");
	    }
	    buf.append(argTypes[i].typeString("", abbrev, ret));
	}
	buf.append(')');
	
	return ret ? getReturnType().typeString(buf.toString(), abbrev, ret) : buf.toString();
    }
}
