/*
 * @(#)OrExpression.java	1.14 95/11/12 Arthur van Hoff
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
class OrExpression extends BinaryLogicalExpression {
    /**
     * constructor
     */
    public OrExpression(int where, Expression left, Expression right) {
	super(OR, where, left, right);
    }

    /*
     * Check an "or" expression.
     * 
     * cvars is modified so that 
     *    cvar.vsTrue indicates variables with a known value if 
     *        either the left and right hand side isn true
     *    cvars.vsFalse indicates variables with a known value if
     *        both the left or right hand side are false
     */
    public void checkCondition(Environment env, Context ctx, long vset, 
			       Hashtable exp, ConditionVars cvars) {
	// Find out when the left side is true/false
	left.checkCondition(env, ctx, vset, exp, cvars);
	left = convert(env, ctx, Type.tBoolean, left);
	long vsTrue = cvars.vsTrue;
	long vsFalse = cvars.vsFalse;

	// Only look at the right side if the left side is false
	right.checkCondition(env, ctx, vsFalse, exp, cvars);
	right = convert(env, ctx, Type.tBoolean, right);

	// cvars.vsFalse actually reports that both returned false
	// cvars.vsTrue must be set back to either left side or the right
	//     side returning false;
	cvars.vsTrue &= vsTrue;
    }

    /**
     * Evaluate
     */
    Expression eval(boolean a, boolean b) {
	return new BooleanExpression(where, a || b);
    }

    /**
     * Simplify
     */
    Expression simplify() {
	if (left.equals(true) || right.equals(false)) {
	    return left;
	}
	if (left.equals(false) || right.equals(true)) {
	    return right;
	}
	return this;
    }

    /**
     * Code
     */
    void codeBranch(Environment env, Context ctx, Assembler asm, Label lbl, boolean whenTrue) {
	if (whenTrue) {
	    left.codeBranch(env, ctx, asm, lbl, true);
	    right.codeBranch(env, ctx, asm, lbl, true);
	} else {
	    Label lbl2 = new Label();
	    left.codeBranch(env, ctx, asm, lbl2, true);
	    right.codeBranch(env, ctx, asm, lbl, false);
	    asm.add(lbl2);
	}
    }
}
