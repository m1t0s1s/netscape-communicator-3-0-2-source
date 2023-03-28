/*
 * @(#)ExprExpression.java	1.13 95/11/12 Arthur van Hoff
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
import java.util.Hashtable;

/** Parenthesised expressions. */


public
class ExprExpression extends UnaryExpression {
    /**
     * Constructor
     */
    public ExprExpression(int where, Expression right) {
	super(EXPR, where, right.type, right);
    }
 
    /**
     * Check a condition.  We must pass it on to our unparenthesised form.
     */
    public void checkCondition(Environment env, Context ctx, long vset, 
			       Hashtable exp, ConditionVars cvars) {
	right.checkCondition(env, ctx, vset, exp, cvars);
	type = right.type;
    }

    /**
     * Select the type of the expression
     */
    void selectType(Environment env, Context ctx, int tm) {
	type = right.type;
    }

    /**
     * Simplify
     */
    Expression simplify() {
	return right;
    }
}