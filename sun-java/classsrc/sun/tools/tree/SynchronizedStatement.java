/*
 * @(#)SynchronizedStatement.java	1.15 95/10/08 Arthur van Hoff
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
class SynchronizedStatement extends Statement {
    Expression expr;
    Statement body;

    /**
     * Constructor
     */
    public SynchronizedStatement(int where, Expression expr, Statement body) {
	super(SYNCHRONIZED, where);
	this.expr = expr;
	this.body = body;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	CheckContext newctx = new CheckContext(ctx, this);
	vset = reach(env, vset);
	vset = expr.checkValue(env, newctx, vset, exp);
	expr = convert(env, newctx, Type.tClass(idJavaLangObject), expr);
	vset = body.check(env, newctx, vset, exp);
	return ctx.removeAdditionalVars(vset & newctx.vsBreak);
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	if (body != null) {
	    body = body.inline(env, ctx);
	}
	if (body == null) {
	    return new ExpressionStatement(where, expr).inline(env, ctx);
	}
	expr = expr.inlineValue(env, ctx);
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
	expr.codeValue(env, ctx, asm);
	
	ctx = new Context(ctx, null);
	LocalField f1 = new LocalField(where, ctx.field.getClassDefinition(), 0, Type.tObject, null);
	LocalField f2 = new LocalField(where, ctx.field.getClassDefinition(), 0, Type.tInt, null);
	Integer num1 = new Integer(ctx.declare(env, f1));
	Integer num2 = new Integer(ctx.declare(env, f2));

	Label endLabel = new Label();

	TryData td = new TryData();
	td.add(null);

	// lock the object
	asm.add(where, opc_astore, num1);
	asm.add(where, opc_aload, num1);
	asm.add(where, opc_monitorenter);

	// Main body
	CodeContext bodyctx = new CodeContext(ctx, this);
	asm.add(where, opc_try, td);
	body.code(env, bodyctx, asm);
	asm.add(bodyctx.breakLabel);
	asm.add(td.getEndLabel());

	// Cleanup afer body
	asm.add(where, opc_aload, num1);
	asm.add(where, opc_monitorexit);
	asm.add(where, opc_goto, endLabel);

	// Catch code
	CatchData cd = td.getCatch(0);
	asm.add(cd.getLabel());
	asm.add(where, opc_aload, num1);
	asm.add(where, opc_monitorexit);
	asm.add(where, opc_athrow);

	// Final body
	asm.add(bodyctx.contLabel);
	asm.add(where, opc_astore, num2);
	asm.add(where, opc_aload, num1);
	asm.add(where, opc_monitorexit);
	asm.add(where, opc_ret, num2);

	asm.add(endLabel);
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("synchronized ");
	expr.print(out);
	out.print(" ");
	body.print(out, indent);
    }
}
