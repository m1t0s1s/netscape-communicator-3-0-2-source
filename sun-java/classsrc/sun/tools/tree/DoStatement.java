/*
 * @(#)DoStatement.java	1.13 95/11/12 Arthur van Hoff
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
class DoStatement extends Statement {
    Statement body;
    Expression cond;

    /**
     * Constructor
     */
    public DoStatement(int where, Statement body, Expression cond) {
	super(DO, where);
	this.body = body;
	this.cond = cond;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
        CheckContext newctx = new CheckContext(ctx, this);
	vset = body.check(env, newctx, reach(env, vset), exp);
	// get to the test either by falling through the body, or through
	// a "continue" statement.
	ConditionVars cvars = 
	    cond.checkCondition(env, newctx, vset & newctx.vsContinue, exp);
	cond = convert(env, newctx, Type.tBoolean, cond);
	// exit the loop through the test returning false, or a "break"
	vset = newctx.vsBreak & cvars.vsFalse;
	return ctx.removeAdditionalVars(vset);
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	ctx = new Context(ctx, this);
	if (body != null) {
	    body = body.inline(env, ctx);
	}
	cond = cond.inlineValue(env, ctx);
	return this;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	DoStatement s = (DoStatement)clone();
	s.cond = cond.copyInline(ctx);
	if (body != null) {
	    s.body = body.copyInline(ctx, valNeeded);
	}
	return s;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return 1 + cond.costInline(thresh) + ((body != null) ? body.costInline(thresh) : 0);
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	Label l1 = new Label();
	asm.add(l1);

	CodeContext newctx = new CodeContext(ctx, this);

	if (body != null) {
	    body.code(env, newctx, asm);
	}
	asm.add(newctx.contLabel);
	cond.codeBranch(env, newctx, asm, l1, true);
	asm.add(newctx.breakLabel);
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("do ");
	body.print(out, indent);
	out.print(" while ");
	cond.print(out);
	out.print(";");
    }
}
