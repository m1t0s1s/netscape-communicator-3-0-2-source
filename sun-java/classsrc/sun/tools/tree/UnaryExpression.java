/*
 * @(#)UnaryExpression.java	1.15 95/09/14 Arthur van Hoff
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
import java.io.PrintStream;
import java.util.Hashtable;

public
class UnaryExpression extends Expression {
    Expression right;

    /**
     * Constructor
     */
    UnaryExpression(int op, int where, Type type, Expression right) {
	super(op, where, type);
	this.right = right;
    }

    /**
     * Order the expression based on precedence
     */
    public Expression order() {
	if (precedence() > right.precedence()) {
	    UnaryExpression e = (UnaryExpression)right;
	    right = e.right;
	    e.right = order();
	    return e;
	}
	return this;
    }

    /**
     * Select the type of the expression
     */
    void selectType(Environment env, Context ctx, int tm) {
	throw new CompilerError("selectType: " + opNames[op]);
    }

    /**
     * Check a unary expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = right.checkValue(env, ctx, vset, exp);

	int tm = right.type.getTypeMask();
	selectType(env, ctx, tm);
	if (((tm & TM_ERROR) == 0) && type.isType(TC_ERROR)) {
	    env.error(where, "invalid.arg", opNames[op]);
	}
	return vset;
    }

    /**
     * Evaluate
     */
    Expression eval(int a) {
	return this;
    }
    Expression eval(long a) {
	return this;
    }
    Expression eval(float a) {
	return this;
    }
    Expression eval(double a) {
	return this;
    }
    Expression eval(boolean a) {
	return this;
    }
    Expression eval(String a) {
	return this;
    }
    Expression eval() {
	switch (right.op) {
	  case BYTEVAL:
	  case CHARVAL:
	  case SHORTVAL:
	  case INTVAL:
	    return eval(((IntegerExpression)right).value);
	  case LONGVAL:
	    return eval(((LongExpression)right).value);
	  case FLOATVAL:
	    return eval(((FloatExpression)right).value);
	  case DOUBLEVAL:
	    return eval(((DoubleExpression)right).value);
	  case BOOLEANVAL:
	    return eval(((BooleanExpression)right).value);
	  case STRINGVAL:
	    return eval(((StringExpression)right).value);
	}
	return this;
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	return right.inline(env, ctx);
    }
    public Expression inlineValue(Environment env, Context ctx) {
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
	UnaryExpression e = (UnaryExpression)clone();
	if (right != null) {
	    e.right = right.copyInline(ctx);
	}
	return e;
    }

    /**
     * The cost of inlining this expression
     */
    public int costInline(int thresh) {
	return 1 + right.costInline(thresh);
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(" + opNames[op] + " ");
	right.print(out);
	out.print(")");
    }
}
