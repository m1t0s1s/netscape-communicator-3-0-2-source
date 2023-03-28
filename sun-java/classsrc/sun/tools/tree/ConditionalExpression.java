/*
 * @(#)ConditionalExpression.java	1.18 95/10/12 Arthur van Hoff
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
import java.io.PrintStream;
import java.util.Hashtable;

public
class ConditionalExpression extends BinaryExpression {
    Expression cond;

    /**
     * Constructor
     */
    public ConditionalExpression(int where, Expression cond, Expression left, Expression right) {
	super(COND, where, Type.tError, left, right);
	this.cond = cond;
    }

    /**
     * Order the expression based on precedence
     */
    public Expression order() {
	if (precedence() > cond.precedence()) {
	    UnaryExpression e = (UnaryExpression)cond;
	    cond = e.right;
	    e.right = order();
	    return e;
	}
	return this;
    }

    /**
     * Check the expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	try {
	    ConditionVars cvars = cond.checkCondition(env, ctx, vset, exp);
	    vset = left.checkValue(env, ctx, cvars.vsTrue, exp) & right.checkValue(env, ctx, cvars.vsFalse, exp);
	    cond = convert(env, ctx, Type.tBoolean, cond);

	    int tm = left.type.getTypeMask() | right.type.getTypeMask();
	    if ((tm & TM_ERROR) != 0) {
		type = Type.tError;
		return vset;
	    }
	    if (left.type.equals(right.type)) {
		type = left.type;
	    } else if ((tm & TM_DOUBLE) != 0) {
		type = Type.tDouble;
	    } else if ((tm & TM_FLOAT) != 0) {
		type = Type.tFloat;
	    } else if ((tm & TM_LONG) != 0) {
		type = Type.tLong;
	    } else if ((tm & TM_REFERENCE) != 0) {
		type = env.implicitCast(right.type, left.type) ? left.type : right.type;
	    } else if (((tm & TM_CHAR) != 0) && left.fitsType(env, Type.tChar) && right.fitsType(env, Type.tChar)) {
		type = Type.tChar;
	    } else if (((tm & TM_SHORT) != 0) && left.fitsType(env, Type.tShort) && right.fitsType(env, Type.tShort)) {
		type = Type.tShort;
	    } else if (((tm & TM_BYTE) != 0) && left.fitsType(env, Type.tByte) && right.fitsType(env, Type.tByte)) {
		type = Type.tByte;
	    } else {
		type = Type.tInt;
	    }
	    
	    left = convert(env, ctx, type, left);
	    right = convert(env, ctx, type, right);
	    return vset;

	} catch (ClassNotFound e) {
	    throw new CompilerError(e);
	}
    }
    public long check(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = cond.checkValue(env, ctx, vset, exp);
	cond = convert(env, ctx, Type.tBoolean, cond);
	return left.check(env, ctx, vset, exp) & right.check(env, ctx, vset, exp);
    }

    /**
     * Simplify
     */
    Expression simplify() {
	if (cond.equals(true)) {
	    return left;
	}
	if (cond.equals(false)) {
	    return right;
	}
	return this;
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	left = left.inline(env, ctx);
	right = right.inline(env, ctx);
	if ((left == null) && (right == null)) {
	    return cond.inline(env, ctx);
	}
	if (left == null) {
	    left = right;
	    right = null;
	    cond = new NotExpression(where, cond);
	}
	cond = cond.inlineValue(env, ctx);
	return simplify();
    }

    public Expression inlineValue(Environment env, Context ctx) {
	cond = cond.inlineValue(env, ctx);
	left = left.inlineValue(env, ctx);
	right = right.inlineValue(env, ctx);
	return simplify();
    }

    /**
     * The cost of inlining this expression
     */
    public int costInline(int thresh) {
	return 1 + cond.costInline(thresh) + left.costInline(thresh) + right.costInline(thresh);
    }

    /**
     * Create a copy of the expression for method inlining
     */
    public Expression copyInline(Context ctx) {
	ConditionalExpression e = (ConditionalExpression)clone();
	e.cond = cond.copyInline(ctx);
	e.left = left.copyInline(ctx);
	e.right = right.copyInline(ctx);
	return e;
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	Label l1 = new Label();
	Label l2 = new Label();

	cond.codeBranch(env, ctx, asm, l1, false);
	left.codeValue(env, ctx, asm);
	asm.add(where, opc_goto, l2);
	asm.add(l1);
	right.codeValue(env, ctx, asm);
	asm.add(l2);
    }
    public void code(Environment env, Context ctx, Assembler asm) {
	Label l1 = new Label();
	cond.codeBranch(env, ctx, asm, l1, false);
	left.code(env, ctx, asm);
	if (right != null) {
	    Label l2 = new Label();
	    asm.add(where, opc_goto, l2);
	    asm.add(l1);
	    right.code(env, ctx, asm);
	    asm.add(l2);
	} else {
	    asm.add(l1);
	}
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(" + opNames[op] + " ");
	cond.print(out);
	out.print(" ");
	left.print(out);
	out.print(" ");
	if (right != null) {
	    right.print(out);
	} else {
	    out.print("<null>");
	}
	out.print(")");
    }
}
