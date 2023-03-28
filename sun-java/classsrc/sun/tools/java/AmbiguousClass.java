/*
 * @(#)AmbiguousClass.java	1.8 95/09/14 Arthur van Hoff
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
 * This exception is thrown when an unqualified class name
 * is used that can be resolved in more than one way.
 */

public
class AmbiguousClass extends ClassNotFound {
    /**
     * The class that was not found
     */
    public Identifier name1;
    public Identifier name2;

    /**
     * Constructor
     */
    public AmbiguousClass(Identifier name1, Identifier name2) {
	super(name1.getName());
	this.name1 = name1;
	this.name2 = name2;
    }
}
