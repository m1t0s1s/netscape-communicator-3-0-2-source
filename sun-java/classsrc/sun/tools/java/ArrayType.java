/*
 * @(#)ArrayType.java	1.8 95/09/14 Arthur van Hoff
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
 * This class represents an Java array type.
 * It overrides the relevant methods in class Type.
 *
 * @author 	Arthur van Hoff
 * @version 	1.8, 14 Sep 1995
 */
final
class ArrayType extends Type {
    /**
     * The type of the element.
     */
    Type elemType;

    /**
     * Construct an array type. Use Type.tArray to create
     * a new array type.
     */
    ArrayType(String typeSig, Type elemType) {
	super(TC_ARRAY, typeSig);
	this.elemType = elemType;
    }
    
    public Type getElementType() {
	return elemType;
    }

    public int getArrayDimension() {
	return elemType.getArrayDimension() + 1;
    }
    
    public String typeString(String id, boolean abbrev, boolean ret) {
	return getElementType().typeString(id, abbrev, ret) + "[]";
    }
}
