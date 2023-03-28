/*
 * @(#)LocalField.java	1.11 95/11/29 Arthur van Hoff
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

package sun.tools.tree;

import sun.tools.java.*;
import sun.tools.tree.*;

/**
 * A local Field
 */

public
class LocalField extends FieldDefinition {
    /**
     * The number of the variable
     */
    int number;

    /**
     * Some statistics
     */
    int readcount;
    int writecount;

    /**
     * The previous local variable, this list is used to build a nested
     * context of local variables.
     */
    LocalField prev;

    /**
     * Constructor
     */
    public LocalField(int where, ClassDefinition clazz, int modifiers, Type type, 
		      Identifier name) {
	super(where, clazz, modifiers, type, name, new ClassDeclaration[0], null);
    }

    /**
     * Special checks
     */
    public boolean isLocal() {
	return true;
    }

    /**
     * Check if used
     */
    public boolean isUsed() {
	return (readcount != 0) || (writecount != 0);
    }

    /**
     * Return value
     */
    public Node getValue(Environment env) {
	return (Expression)getValue();
    }
}
