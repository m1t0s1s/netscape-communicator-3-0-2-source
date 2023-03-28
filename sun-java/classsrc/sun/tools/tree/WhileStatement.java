/*
 * @(#)WhileStatement.java	1.15 95/11/12 Arthur van Hoff
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
class WhileStatement extends Statement {
    Expression cond;
    Statement body;

    /**
     * Constructor
     */
    public WhileStatement(int where, Expression cond, Statement body) {
	super(WHILE, where);
	this.cond = cond;
	this.body = body;
    }

    /**
     * Check a while statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	CheckContext newctx = new CheckContext(ctx, this);
	// check the condition.  Determine which variables have values if 
	// it returns true or false.
	ConditionVars cvars = 
	      cond.checkCondition(env, newctx, reach(env, vset), exp);
	cond = convert(env, newctx, Type.tBoolean, cond);
	// check the body, given that the condition returned true.
	body.check(env, newctx, cvars.vsTrue, exp);
	// Exit the while loop by testing false or getting a break statement
	vset = newctx.vsBreak & cvars.vsFalse;
	return ctx.removeAdditionalVars(vset);
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	ctx = new Context(ctx, this);
	cond = cond.inlineValue(env, ctx);
	if (body != null) {
	    body = body.inline(env, ctx);
	}
	return this;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return 1 + cond.costInline(thresh) + ((body != null) ? body.costInline(thresh) : 0);
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	WhileStatement s = (WhileStatement)clone();
	s.cond = cond.copyInline(ctx);
	if (body != null) {
	    s.body = body.copyInline(ctx, valNeeded);
	}
	return s;
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	CodeContext newctx = new CodeContext(ctx, this);

	asm.add(where, opc_goto, newctx.contLabel);

	Label l1 = new Label();
	asm.add(l1);

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
	out.print("while ");
	cond.print(out);
	if (body != null) {
	    out.print(" ");
	    body.print(out, indent);
	} else {
	    out.print(";");
	}
    }
}
