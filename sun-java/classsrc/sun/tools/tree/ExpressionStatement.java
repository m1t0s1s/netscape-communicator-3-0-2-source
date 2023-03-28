/*
 * @(#)ExpressionStatement.java	1.13 95/09/14 Arthur van Hoff
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
import java.io.PrintStream;
import java.util.Hashtable;

public
class ExpressionStatement extends Statement {
    Expression expr;

    /**
     * Constructor
     */
    public ExpressionStatement(int where, Expression expr) {
	super(EXPRESSION, where);
	this.expr = expr;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	return expr.check(env, ctx, reach(env, vset), exp);
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	if (expr != null) {
	    expr = expr.inline(env, ctx);
	    return (expr == null) ? null : this;
	}
	return null;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	ExpressionStatement s = (ExpressionStatement)clone();
	s.expr = expr.copyInline(ctx);
	return s;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return expr.costInline(thresh);
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	expr.code(env, ctx, asm);
    }

    /**
     * Check if the first thing is a constructor invocation
     */
    public boolean firstConstructor() {
	return expr.firstConstructor();
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	if (expr != null) {
	    expr.print(out);
	} else {
	    out.print("<empty>");
	}
	out.print(";");
    }
}
