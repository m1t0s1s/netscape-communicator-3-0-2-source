/*
 * @(#)MultiplyExpression.java	1.9 95/09/14 Arthur van Hoff
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
class MultiplyExpression extends BinaryArithmeticExpression {
    /**
     * constructor
     */
    public MultiplyExpression(int where, Expression left, Expression right) {
	super(MUL, where, left, right);
    }

    /**
     * Evaluate
     */
    Expression eval(int a, int b) {
	return new IntExpression(where, a * b);
    }
    Expression eval(long a, long b) {
	return new LongExpression(where, a * b);
    }
    Expression eval(float a, float b) {
	return new FloatExpression(where, a * b);
    }
    Expression eval(double a, double b) {
	return new DoubleExpression(where, a * b);
    }

    /**
     * Simplify
     */
    Expression simplify() {
	if (left.equals(1)) {
	    return right;
	}
	if (right.equals(1)) {
	    return left;
	}
	return this;
    }

    /**
     * Code
     */
    void codeOperation(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_imul + type.getTypeCodeOffset());
    }
}
