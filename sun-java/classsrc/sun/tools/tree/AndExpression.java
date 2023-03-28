/*
 * @(#)AndExpression.java	1.13 95/11/12 Arthur van Hoff
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
class AndExpression extends BinaryLogicalExpression {
    /**
     * constructor
     */
    public AndExpression(int where, Expression left, Expression right) {
	super(AND, where, left, right);
    }

    /*
     * Check an "and" expression.
     * 
     * cvars is modified so that 
     *    cvar.vsTrue indicates variables with a known value if 
     *        both the left and right hand side are true
     *    cvars.vsFalse indicates variables with a known value 
     *        either the left or right hand side is false
     */
    public void checkCondition(Environment env, Context ctx, long vset, 
			       Hashtable exp, ConditionVars cvars) {
	// Find out when the left side is true/false
	left.checkCondition(env, ctx, vset, exp, cvars);
	left = convert(env, ctx, Type.tBoolean, left);
	long vsTrue = cvars.vsTrue;
	long vsFalse = cvars.vsFalse;

	// Only look at the right side if the left side is true
	right.checkCondition(env, ctx, vsTrue, exp, cvars);
	right = convert(env, ctx, Type.tBoolean, right);

	// cvars.vsTrue already reports when both returned true
	// cvars.vsFalse must be set to either the left or right side 
	//    returning false
	cvars.vsFalse &= vsFalse;
    }

    /**
     * Evaluate
     */
    Expression eval(boolean a, boolean b) {
	return new BooleanExpression(where, a && b);
    }

    /**
     * Simplify
     */
    Expression simplify() {
	if (left.equals(true) || right.equals(false)) {
	    return right;
	}
	if (left.equals(false) || right.equals(true)) {
	    return left;
	}
	return this;
    }

    /**
     * Code
     */
    void codeBranch(Environment env, Context ctx, Assembler asm, Label lbl, boolean whenTrue) {
	if (whenTrue) {
	    Label lbl2 = new Label();
	    left.codeBranch(env, ctx, asm, lbl2, false);
	    right.codeBranch(env, ctx, asm, lbl, true);
	    asm.add(lbl2);
	} else {
	    left.codeBranch(env, ctx, asm, lbl, false);
	    right.codeBranch(env, ctx, asm, lbl, false);
	}
    }
}
