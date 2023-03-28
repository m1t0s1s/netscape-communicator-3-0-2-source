/*
 * @(#)LessOrEqualExpression.java	1.11 95/09/14 Arthur van Hoff
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
import sun.tools.asm.Label;

public
class LessOrEqualExpression extends BinaryCompareExpression {
    /**
     * constructor
     */
    public LessOrEqualExpression(int where, Expression left, Expression right) {
	super(LE, where, left, right);
    }

    /**
     * Evaluate
     */
    Expression eval(int a, int b) {
	return new BooleanExpression(where, a <= b);
    }
    Expression eval(long a, long b) {
	return new BooleanExpression(where, a <= b);
    }
    Expression eval(float a, float b) {
	return new BooleanExpression(where, a <= b);
    }
    Expression eval(double a, double b) {
	return new BooleanExpression(where, a <= b);
    }

    /**
     * Simplify
     */
    Expression simplify() {
	if (left.isConstant() && !right.isConstant()) {
	    return new GreaterOrEqualExpression(where, right, left);
	}
	return this;
    }

    /**
     * Code
     */
    void codeBranch(Environment env, Context ctx, Assembler asm, Label lbl, boolean whenTrue) {
	left.codeValue(env, ctx, asm);
	switch (left.type.getTypeCode()) {
	  case TC_INT:
	    if (!right.equals(0)) {
		right.codeValue(env, ctx, asm);
		asm.add(where, whenTrue ? opc_if_icmple : opc_if_icmpgt, lbl);
		return;
	    }
	    break;
	  case TC_LONG:
	    right.codeValue(env, ctx, asm);
	    asm.add(where, opc_lcmp);
	    break;
	  case TC_FLOAT:
	    right.codeValue(env, ctx, asm);
	    asm.add(where, opc_fcmpg);
	    break;
	  case TC_DOUBLE:
	    right.codeValue(env, ctx, asm);
	    asm.add(where, opc_dcmpg);
	    break;
	  default:
	    throw new CompilerError("Unexpected Type");
	}
	asm.add(where, whenTrue ? opc_ifle : opc_ifgt, lbl);
    }
}
