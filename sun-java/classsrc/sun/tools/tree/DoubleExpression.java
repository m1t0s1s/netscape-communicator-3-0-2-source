/*
 * @(#)DoubleExpression.java	1.11 95/11/08 Arthur van Hoff
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
import java.io.PrintStream;

public
class DoubleExpression extends ConstantExpression {
    double value;
    
    /**
     * Constructor
     */
    public DoubleExpression(int where, double value) {
	super(DOUBLEVAL, where, Type.tDouble);
	this.value = value;
    }

    /**
     * Get the value
     */
    public Object getValue() {
	return new Double(value);
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
        // don't allow -0.0
        return (Double.doubleToLongBits(value) == 0);
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_ldc2_w, new Double(value));
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print(value + "D");
    }
}
