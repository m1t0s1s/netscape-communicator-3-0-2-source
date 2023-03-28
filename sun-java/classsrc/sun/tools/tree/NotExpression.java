/*
 * @(#)NotExpression.java	1.14 95/11/12 Arthur van Hoff
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
import java.util.Hashtable;

public
class NotExpression extends UnaryExpression {
    /**
     * Constructor
     */
    public NotExpression(int where, Expression right) {
	super(NOT, where, Type.tBoolean, right);
    }
 
    /**
     * Select the type of the expression
     */
    void selectType(Environment env, Context ctx, int tm) {
	right = convert(env, ctx, Type.tBoolean, right);
    }

    /*
     * Check a "not" expression.
     * 
     * cvars is modified so that 
     *    cvar.vsTrue indicates variables with a known value if 
     *         the expression is true.
     *    cvars.vsFalse indicates variables with a known value if
     *         the expression is false
     *
     * For "not" expressions, we look at the inside expression, and then
     * swap true and false.
     */

    public void checkCondition(Environment env, Context ctx, int vset, 
			       Hashtable exp, ConditionVars cvars) {
	right.checkCondition(env, ctx, vset, exp, cvars);
	right = convert(env, ctx, Type.tBoolean, right);
	// swap true and false
	long temp = cvars.vsFalse;
	cvars.vsFalse = cvars.vsTrue;
	cvars.vsTrue = temp;
    }

    /**
     * Evaluate
     */
    Expression eval(boolean a) {
	return new BooleanExpression(where, !a);
    }

    /**
     * Simplify
     */
    Expression simplify() {
	// Check if the expression can be optimized
	switch (right.op) {
	  case NOT:
	    return ((NotExpression)right).right;

	  case EQ:
	  case NE:
	  case LT:
	  case LE:
	  case GT:
	  case GE:
	    break;

	  default:
	    return this;
	}

	// Can't negate real comparisons
	BinaryExpression bin = (BinaryExpression)right;
	if (bin.left.type.inMask(TM_REAL)) {
	    return this;
	}
	
	// Negate comparison
	switch (right.op) {
	  case EQ:
	    return new NotEqualExpression(where, bin.left, bin.right);
	  case NE:
	    return new EqualExpression(where, bin.left, bin.right);
	  case LT:
	    return new GreaterOrEqualExpression(where, bin.left, bin.right);
	  case LE:
	    return new GreaterExpression(where, bin.left, bin.right);
	  case GT:
	    return new LessOrEqualExpression(where, bin.left, bin.right);
	  case GE:
	    return new LessExpression(where, bin.left, bin.right);
	}
	return this;
    }

    /**
     * Code
     */
    void codeBranch(Environment env, Context ctx, Assembler asm, Label lbl, boolean whenTrue) {
	right.codeBranch(env, ctx, asm, lbl, !whenTrue);
    }
}
