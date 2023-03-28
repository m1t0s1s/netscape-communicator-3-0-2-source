/*
 * @(#)BinaryExpression.java	1.15 95/09/14 Arthur van Hoff
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
import sun.tools.asm.Label;
import sun.tools.asm.Assembler;
import java.io.PrintStream;
import java.util.Hashtable;

public
class BinaryExpression extends UnaryExpression {
    Expression left;

    /**
     * Constructor
     */
    BinaryExpression(int op, int where, Type type, Expression left, Expression right) {
	super(op, where, type, right);
	this.left = left;
    }

    /**
     * Order the expression based on precedence
     */
    public Expression order() {
	if (precedence() > left.precedence()) {
	    UnaryExpression e = (UnaryExpression)left;
	    left = e.right;
	    e.right = order();
	    return e;
	}
	return this;
    }

    /**
     * Check a binary expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = left.checkValue(env, ctx, vset, exp);
	vset = right.checkValue(env, ctx, vset, exp);

	int tm = left.type.getTypeMask() | right.type.getTypeMask();
	selectType(env, ctx, tm);

	if (((tm & TM_ERROR) == 0) && type.isType(TC_ERROR)) {
	    env.error(where, "invalid.args", opNames[op]);
	}
	return vset;
    }

    /**
     * Evaluate
     */
    Expression eval(int a, int b) {
	return this;
    }
    Expression eval(long a, long b) {
	return this;
    }
    Expression eval(float a, float b) {
	return this;
    }
    Expression eval(double a, double b) {
	return this;
    }
    Expression eval(boolean a, boolean b) {
	return this;
    }
    Expression eval(String a, String b) {
	return this;
    }
    Expression eval() {
	if (left.op == right.op) {
	    switch (left.op) {
	      case BYTEVAL:
	      case CHARVAL:
	      case SHORTVAL:
	      case INTVAL:
		return eval(((IntegerExpression)left).value, ((IntegerExpression)right).value);
	      case LONGVAL:
		return eval(((LongExpression)left).value, ((LongExpression)right).value);
	      case FLOATVAL:
		return eval(((FloatExpression)left).value, ((FloatExpression)right).value);
	      case DOUBLEVAL:
		return eval(((DoubleExpression)left).value, ((DoubleExpression)right).value);
	      case BOOLEANVAL:
		return eval(((BooleanExpression)left).value, ((BooleanExpression)right).value);
	      case STRINGVAL:
		return eval(((StringExpression)left).value, ((StringExpression)right).value);
	    }
	}
	return this;
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	left = left.inline(env, ctx);
	right = right.inline(env, ctx);
	return (left == null) ? right : new CommaExpression(where, left, right);
    }
    public Expression inlineValue(Environment env, Context ctx) {
	left = left.inlineValue(env, ctx);
	right = right.inlineValue(env, ctx);
	try {
	    return eval().simplify();
	} catch (ArithmeticException e) {
	    env.error(where, "arithmetic.exception");
	    return this;
	}
    }

    /**
     * Create a copy of the expression for method inlining
     */
    public Expression copyInline(Context ctx) {
	BinaryExpression e = (BinaryExpression)clone();
	if (left != null) {
	    e.left = left.copyInline(ctx);
	}
	if (right != null) {
	    e.right = right.copyInline(ctx);
	}
	return e;
    }

    /**
     * The cost of inlining this expression
     */
    public int costInline(int thresh) {
	return 1 + ((left != null) ? left.costInline(thresh) : 0) +
	    	   ((right != null) ? right.costInline(thresh) : 0);
    }

    /**
     * Code
     */
    void codeOperation(Environment env, Context ctx, Assembler asm) {
	throw new CompilerError("codeOperation: " + opNames[op]);
    }
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	if (type.isType(TC_BOOLEAN)) {
	    Label l1 = new Label();
	    Label l2 = new Label();

	    codeBranch(env, ctx, asm, l1, true);
	    asm.add(where, opc_ldc, new Integer(0));
	    asm.add(where, opc_goto, l2);
	    asm.add(l1);
	    asm.add(where, opc_ldc, new Integer(1));
	    asm.add(l2);
	} else {
	    left.codeValue(env, ctx, asm);
	    right.codeValue(env, ctx, asm);
	    codeOperation(env, ctx, asm);
	}
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(" + opNames[op] + " ");
	if (left != null) {
	    left.print(out);
	} else {
	    out.print("<null>");
	}
	out.print(" ");
	if (right != null) {
	    right.print(out);
	} else {
	    out.print("<null>");
	}
	out.print(")");
    }
}

