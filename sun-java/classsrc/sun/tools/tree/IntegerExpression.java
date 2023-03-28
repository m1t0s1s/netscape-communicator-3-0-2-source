/*
 * @(#)IntegerExpression.java	1.13 95/11/08 Arthur van Hoff
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
import sun.tools.asm.Assembler;

public
class IntegerExpression extends ConstantExpression {
    int value;
    
    /**
     * Constructor
     */
    IntegerExpression(int op, int where, Type type, int value) {
	super(op, where, type);
	this.value = value;
    }

    /**
     * See if this number fits in the given type.
     */
    public boolean fitsType(Environment env, Type t) {
	switch (t.getTypeCode()) {
	  case TC_BYTE:
	    return value == (byte)value;
	  case TC_SHORT:
	    return value == (short)value;
	  case TC_CHAR:
	    return value == (char)value;
	}
	return super.fitsType(env, t);
    }

    /**
     * Get the value
     */
    public Object getValue() {
	return new Integer(value);
    }

    /**
     * Check if the expression is equal to a value
     */
    public boolean equals(int i) {
	return value == i;
    }

    /**
     * Check if the expression is equal to its default static value
     */
    public boolean equalsDefault() {
        return value == 0;
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_ldc, new Integer(value));
    }
}
