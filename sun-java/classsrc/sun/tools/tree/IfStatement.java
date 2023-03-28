/*
 * @(#)IfStatement.java	1.13 95/10/08 Arthur van Hoff
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
class IfStatement extends Statement {
    Expression cond;
    Statement ifTrue;
    Statement ifFalse;

    /**
     * Constructor
     */
    public IfStatement(int where, Expression cond, Statement ifTrue, Statement ifFalse) {
	super(IF, where);
	this.cond = cond;
	this.ifTrue = ifTrue;
	this.ifFalse = ifFalse;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	CheckContext newctx = new CheckContext(ctx, this);
	ConditionVars cvars = 
	      cond.checkCondition(env, newctx, reach(env, vset), exp);
	cond = convert(env, newctx, Type.tBoolean, cond);
	// if either the true clause or the false clause is unreachable,
	// do a reasonable check on the child anyway.
	long vsTrue  = cvars.vsTrue == -1 ? vset : cvars.vsTrue;
	long vsFalse = cvars.vsFalse == -1 ? vset : cvars.vsFalse;
	vsTrue = ifTrue.check(env, newctx, vsTrue, exp);
	if (ifFalse != null) 
	    vsFalse = ifFalse.check(env, newctx, vsFalse, exp);
	vset = vsTrue & vsFalse & newctx.vsBreak;
	return ctx.removeAdditionalVars(vset);
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	ctx = new Context(ctx, this);
	cond = cond.inlineValue(env, ctx);
	if (cond.equals(true)) {
	    ifTrue = ifTrue.inline(env, ctx);
	    return eliminate(env, ifTrue);
	}
	if (cond.equals(false)) {
	    return eliminate(env, (ifFalse != null) ? ifFalse.inline(env, ctx) : null);
	}
	if (ifTrue != null) {
	    ifTrue = ifTrue.inline(env, ctx);
	}
	if (ifFalse != null) {
	    ifFalse = ifFalse.inline(env, ctx);
	}
	if ((ifTrue == null) && (ifFalse == null)) {
	    return eliminate(env, new ExpressionStatement(where, cond).inline(env, ctx));
	}
	if (ifTrue == null) {
	    cond = new NotExpression(cond.where, cond).inlineValue(env, ctx);
	    return eliminate(env, new IfStatement(where, cond, ifFalse, null));
	}
	return this;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	IfStatement s = (IfStatement)clone();
	s.cond = cond.copyInline(ctx);
	if (ifTrue != null) {
	    s.ifTrue = ifTrue.copyInline(ctx, valNeeded);
	}
	if (ifFalse != null) {
	    s.ifFalse = ifFalse.copyInline(ctx, valNeeded);
	}
	return s;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	int cost = 1 + cond.costInline(thresh);
	if (ifTrue != null) {
	    cost += ifTrue.costInline(thresh);
	}
	if (ifFalse != null) {
	    cost += ifFalse.costInline(thresh);
	}
	return cost;
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	CodeContext newctx = new CodeContext(ctx, this);
	
	Label l1 = new Label();
	cond.codeBranch(env, newctx, asm, l1, false);
	ifTrue.code(env, newctx, asm);
	if (ifFalse != null) {
	    Label l2 = new Label();
	    asm.add(where, opc_goto, l2);
	    asm.add(l1);
	    ifFalse.code(env, newctx, asm);
	    asm.add(l2);
	} else {
	    asm.add(l1);
	}

	asm.add(newctx.breakLabel);
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("if ");
	cond.print(out);
	out.print(" ");
	ifTrue.print(out, indent);
	if (ifFalse != null) {
	    out.print(" else ");
	    ifFalse.print(out, indent);
	}
    }
}
