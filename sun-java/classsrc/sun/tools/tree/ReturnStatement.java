/*
 * @(#)ReturnStatement.java	1.15 95/11/12 Arthur van Hoff
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
class ReturnStatement extends Statement {
    Expression expr;

    /**
     * Constructor
     */
    public ReturnStatement(int where, Expression expr) {
	super(RETURN, where);
	this.expr = expr;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = reach(env, vset);
	if (expr != null) {
	    expr.checkValue(env, ctx, vset, exp);
	}

	// Make sure the return isn't inside a static initializer
	if (ctx.field.getName() == idClassInit) {
	    env.error(where, "return.inside.static.initializer");
	    return -1;
	}
	// Check return type
	if (ctx.field.getType().getReturnType().isType(TC_VOID)) {
	    if (expr != null) {
		if (ctx.field.isConstructor()) {
		    env.error(where, "return.with.value.constr", ctx.field);
		} else {
		    env.error(where, "return.with.value", ctx.field);
		}
		expr = null;
	    }
	} else {
	    if (expr == null) {
		env.error(where, "return.without.value", ctx.field);
	    } else {
		expr = convert(env, ctx, ctx.field.getType().getReturnType(), expr);
	    }
	}
	return -1;
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
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return 1 + ((expr != null) ? expr.costInline(thresh) : 0);
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	Expression e = (expr != null) ? expr.copyInline(ctx) : null;
	
	if ((!valNeeded) && (e != null)) {
	    Statement body[] = {
		new ExpressionStatement(where, e),
		new InlineReturnStatement(where, null)
	    };
	    return new CompoundStatement(where, body);
	}
	return new InlineReturnStatement(where, e);
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	if (expr == null) {
	    codeFinally(env, ctx, asm, null, null);
	    asm.add(where, opc_return);
	    return;
	}
	expr.codeValue(env, ctx, asm);
	codeFinally(env, ctx, asm, null, expr.type);
	asm.add(where, opc_ireturn + expr.type.getTypeCodeOffset());
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("return");
	if (expr != null) {
	    out.print(" ");
	    expr.print(out);
	}
	out.print(";");
    }
}
