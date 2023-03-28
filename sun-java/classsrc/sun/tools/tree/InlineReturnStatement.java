/*
 * @(#)InlineReturnStatement.java	1.6 95/09/14 Arthur van Hoff
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

public
class InlineReturnStatement extends Statement {
    Expression expr;

    /**
     * Constructor
     */
    public InlineReturnStatement(int where, Expression expr) {
	super(INLINERETURN, where);
	this.expr = expr;
    }

    /**
     * Get the destination context of a break
     */
    Context getDestination(Context ctx) {
	for (; ctx != null ; ctx = ctx.prev) {
	    if ((ctx.node != null) && ((ctx.node.op == INLINEMETHOD) || (ctx.node.op == INLINENEWINSTANCE))) {
		return ctx;
	    }
	}
	return null;
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	if (expr != null) {
	    expr = expr.inlineValue(env, ctx);
	}
	return this;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	InlineReturnStatement s = (InlineReturnStatement)clone();
	if (expr != null) {
	    expr = expr.copyInline(ctx);
	}
	return s;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return 1 + ((expr != null) ? expr.costInline(thresh) : 0);
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	if (expr != null) {
	    expr.codeValue(env, ctx, asm);
	}
	CodeContext destctx = (CodeContext)getDestination(ctx);
	asm.add(where, opc_goto, destctx.breakLabel);
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("inline-return");
	if (expr != null) {
	    out.print(" ");
	    expr.print(out);
	}
	out.print(";");
    }
}
