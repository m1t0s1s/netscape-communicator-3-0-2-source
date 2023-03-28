/*
 * @(#)CommaExpression.java	1.11 95/09/14 Arthur van Hoff
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
class CommaExpression extends BinaryExpression {
    /**
     * constructor
     */
    public CommaExpression(int where, Expression left, Expression right) {
	super(COMMA, where, (right != null) ? right.type : Type.tVoid, left, right);
    }

    /**
     * Check void expression
     */
    public long check(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = left.check(env, ctx, vset, exp);
	vset = right.check(env, ctx, vset, exp);
	return vset;
    }

    /**
     * Select the type
     */
    void selectType(Environment env, Context ctx, int tm) {
	type = right.type;
    }

    /**
     * Simplify
     */
    Expression simplify() {
	if (left == null) {
	    return right;
	}
	if (right == null) {
	    return left;
	}
	return this;
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	if (left != null) {
	    left = left.inline(env, ctx);
	}
	if (right != null) {
	    right = right.inline(env, ctx);
	}
	return simplify();
    }
    public Expression inlineValue(Environment env, Context ctx) {
	if (left != null) {
	    left = left.inline(env, ctx);
	}
	if (right != null) {
	    right = right.inlineValue(env, ctx);
	}
	return simplify();
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	if (left != null) {
	    left.code(env, ctx, asm);
	}
	right.codeValue(env, ctx, asm);
    }
    public void code(Environment env, Context ctx, Assembler asm) {
	if (left != null) {
	    left.code(env, ctx, asm);
	}
	if (right != null) {
	    right.code(env, ctx, asm);
	}
    }
}
