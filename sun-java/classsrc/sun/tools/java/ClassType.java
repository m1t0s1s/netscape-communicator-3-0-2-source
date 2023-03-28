/*
 * @(#)ClassType.java	1.8 95/09/14 Arthur van Hoff
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
 * This class represents an Java class type.
 * It overrides the relevant methods in class Type.
 *
 * @author 	Arthur van Hoff
 * @version 	1.8, 14 Sep 1995
 */
final
class ClassType extends Type {
    /**
     * The fully qualified class name.
     */
    Identifier className;

    /**
     * Construct a class type. Use Type.tClass to create
     * a new class type.
     */
    ClassType(String typeSig, Identifier className) {
	super(TC_CLASS, typeSig);
	this.className = className;
    }

    public Identifier getClassName() {
	return className;
    }
    
    public String typeString(String id, boolean abbrev, boolean ret) {
	String s = (abbrev ? getClassName().getName() : getClassName()).toString();
	return (id.length() > 0) ? s + " " + id : s;
    }
}
