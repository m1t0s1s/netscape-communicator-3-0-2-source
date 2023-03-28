/*
 * @(#)AmbiguousField.java	1.7 95/09/14 Arthur van Hoff
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

import java.util.Enumeration;

/**
 * This exception is thrown when a field reference is
 * ambiguous.
 */
public
class AmbiguousField extends Exception {
    /**
     * The field that was not found
     */
    public FieldDefinition field1;
    public FieldDefinition field2;

    /**
     * Constructor
     */
    public AmbiguousField(FieldDefinition field1, FieldDefinition field2) {
	super(field1.getName() + " + " + field2.getName());
	this.field1 = field1;
	this.field2 = field2;
    }
}
