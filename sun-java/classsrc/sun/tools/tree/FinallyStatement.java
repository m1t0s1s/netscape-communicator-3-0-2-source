/*
 * @(#)FinallyStatement.java	1.15 95/10/11 Arthur van Hoff
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
import sun.tools.asm.TryData;
import sun.tools.asm.CatchData;
import java.io.PrintStream;
import java.util.Hashtable;

public
class FinallyStatement extends Statement {
    Statement body;
    Statement finalbody;
    boolean finallyFinishes; // does finalBody never return?

    /**
     * Constructor
     */
    public FinallyStatement(int where, Statement body, Statement finalbody) {
	super(FINALLY, where);
	this.body = body;
	this.finalbody = finalbody;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = reach(env, vset);
        CheckContext newctx1 = new CheckContext(ctx, this);
	long vset1 = body.check(env, newctx1, vset, exp) & newctx1.vsBreak;

        CheckContext newctx2 = new CheckContext(ctx, this);
	long vset2 = finalbody.check(env, newctx2, vset, exp) & newctx2.vsBreak;
	finallyFinishes = ((vset2 & DEAD_END) == 0);
	if (!finallyFinishes) 
	    exp.clear();
	return ctx.removeAdditionalVars(vset1 | vset2);
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	if (body != null) {
	    body = body.inline(env, ctx);
	}
	if (finalbody != null) {
	    finalbody = finalbody.inline(env, ctx);
	}
	if (body == null) {
	    return eliminate(env, finalbody);
	}
	if (finalbody == null) {
	    return eliminate(env, body);
	}
	return this;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	throw new CompilerError("copyInline");
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
        ctx = new Context(ctx, null);
	Integer num1 = null, num2 = null;
	Label endLabel = null;

	if (finallyFinishes) { 
	    LocalField f1, f2;
	    ClassDefinition thisClass = ctx.field.getClassDefinition();
	    f1 = new LocalField(where, thisClass, 0, Type.tObject, null);
	    f2 = new LocalField(where, thisClass, 0, Type.tInt, null);
	    num1 = new Integer(ctx.declare(env, f1));
	    num2 = new Integer(ctx.declare(env, f2));
	    endLabel = new Label();
	}

	TryData td = new TryData();
	td.add(null);
	
	// Main body
	CodeContext bodyctx = new CodeContext(ctx, this);
	asm.add(where, opc_try, td); // start of protected code
	body.code(env, bodyctx, asm);
	asm.add(bodyctx.breakLabel);
	asm.add(td.getEndLabel());   // end of protected code

	// Cleanup afer body
	if (finallyFinishes) {
	    asm.add(where, opc_jsr, bodyctx.contLabel);
	    asm.add(where, opc_goto, endLabel);
	} else {
	    // just goto the cleanup code.  It will never return.
	    asm.add(where, opc_goto, bodyctx.contLabel);
	} 
	
	// Catch code
	CatchData cd = td.getCatch(0);
	asm.add(cd.getLabel());
	if (finallyFinishes) {
	    asm.add(where, opc_astore, num1);
	    asm.add(where, opc_jsr, bodyctx.contLabel);
	    asm.add(where, opc_aload, num1);
	    asm.add(where, opc_athrow);
	} else { 
	    // pop exception off stack.  Fall through to finally code
	    asm.add(where, opc_pop);
	}

	// Final body
	CodeContext finalctx = new CodeContext(ctx, this);
	asm.add(bodyctx.contLabel);
	if (finallyFinishes) {
	    asm.add(where, opc_astore, num2); // save the return address
	}
	finalbody.code(env, ctx, asm);        // execute the code
	asm.add(finalctx.breakLabel); 
	if (finallyFinishes) {              // return from jsr
	    asm.add(where, opc_ret, num2);
	    asm.add(endLabel);
	}
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("try ");
	if (body != null) {
	    body.print(out, indent);
	} else {
	    out.print("<empty>");
	}
	out.print(" finally ");
	if (finalbody != null) {
	    finalbody.print(out, indent);
	} else {
	    out.print("<empty>");
	}
    }
}
