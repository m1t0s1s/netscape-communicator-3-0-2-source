/*
 * @(#)BinaryAssignExpression.java	1.11 95/09/14 Arthur van Hoff
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
import java.util.Hashtable;

public
class BinaryAssignExpression extends BinaryExpression {
    /**
     * Constructor
     */
    BinaryAssignExpression(int op, int where, Expression left, Expression right) {
	super(op, where, left.type, left, right);
    }

    /**
     * Order the expression based on precedence
     */
    public Expression order() {
	if (precedence() >= left.precedence()) {
	    UnaryExpression e = (UnaryExpression)left;
	    left = e.right;
	    e.right = order();
	    return e;
	}
	return this;
    }

    /**
     * Check void expression
     */
    public long check(Environment env, Context ctx, long vset, Hashtable exp) {
	return checkValue(env, ctx, vset, exp);
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	return inlineValue(env, ctx);
    }
    public Expression inlineValue(Environment env, Context ctx) {
	left = left.inlineLHS(env, ctx);
	right = right.inlineValue(env, ctx);
	return this;
    }
}
