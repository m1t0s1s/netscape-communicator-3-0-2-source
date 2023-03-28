/*
 * @(#)ForStatement.java	1.14 95/10/08 Arthur van Hoff
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
class ForStatement extends Statement {
    Statement init;
    Expression cond;
    Expression inc;
    Statement body;

    /**
     * Constructor
     */
    public ForStatement(int where, Statement init, Expression cond, Expression inc, Statement body) {
	super(FOR, where);
	this.init = init;
	this.cond = cond;
	this.inc = inc;
	this.body = body;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = reach(env, vset);
	CheckContext newctx = new CheckContext(ctx, this);
	if (init != null) {
	    vset = init.check(env, newctx, vset, exp);
	}
	if (cond != null) {
	    ConditionVars cvars = cond.checkCondition(env, newctx, reach(env, vset), exp);
	    cond = convert(env, newctx, Type.tBoolean, cond);
	    vset = body.check(env, newctx, cvars.vsTrue, exp);
	    if (inc != null) {
		inc.check(env, newctx, vset & newctx.vsContinue, exp);
	    }
	    // exit by testing false or executing a break;
	    vset = newctx.vsBreak & cvars.vsFalse;
	} else {
	    // a missing test is equivalent to "true"
	    vset = body.check(env, newctx, vset, exp);
	    if (inc != null) {
		inc.check(env, newctx, vset & newctx.vsContinue, exp);
	    }
	    // exit by executing a break;
	    vset =  newctx.vsBreak;
	}
	return ctx.removeAdditionalVars(vset);
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	ctx = new Context(ctx, this);
	if (init != null) {
	    Statement body[] = {init, this};
	    init = null;
	    return new CompoundStatement(where, body).inline(env, ctx);
	}
	if (cond != null) {
	    cond = cond.inlineValue(env, ctx);
	}
	if (body != null) {
	    body = body.inline(env, ctx);
	}
	if (inc != null) {
	    inc = inc.inline(env, ctx);
	}
	return this;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	ForStatement s = (ForStatement)clone();
	if (init != null) {
	    s.init = init.copyInline(ctx, valNeeded);
	}
	if (cond != null) {
	    s.cond = cond.copyInline(ctx);
	}
	if (body != null) {
	    s.body = body.copyInline(ctx, valNeeded);
	}
	if (inc != null) {
	    s.inc = inc.copyInline(ctx);
	}
	return s;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	int cost = 2;
	if (init != null) {
	    cost += init.costInline(thresh);
	}
	if (cond != null) {
	    cost += cond.costInline(thresh);
	}
	if (body != null) {
	    cost += body.costInline(thresh);
	}
	if (inc != null) {
	    cost += inc.costInline(thresh);
	}
	return cost;
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	CodeContext newctx = new CodeContext(ctx, this);
	if (init != null) {
	    init.code(env, newctx, asm);
	}

	Label l1 = new Label();
	Label l2 = new Label();

	asm.add(where, opc_goto, l2);

	asm.add(l1);
	if (body != null) {
	    body.code(env, newctx, asm);
	}

	asm.add(newctx.contLabel);
	if (inc != null) {
	    inc.code(env, newctx, asm);
	}

	asm.add(l2);
	if (cond != null) {
	    cond.codeBranch(env, newctx, asm, l1, true);
	} else {
	    asm.add(where, opc_goto, l1);
	}
	asm.add(newctx.breakLabel);
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("for (");
	if (init != null) {
	    init.print(out, indent);
	    out.print(" ");
	} else {
	    out.print("; ");
	}
	if (cond != null) {
	    cond.print(out);
	    out.print(" ");
	}
	out.print("; ");
	if (inc != null) {
	    inc.print(out);
	}
	out.print(") ");
	if (body != null) {
	    body.print(out, indent);
	} else {
	    out.print(";");
	}
    }
}
