/*
 * @(#)InlineMethodExpression.java	1.6 95/09/14 Arthur van Hoff
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
import sun.tools.asm.Label;
import sun.tools.asm.Assembler;
import java.io.PrintStream;

public
class InlineMethodExpression extends Expression {
    FieldDefinition field;
    Statement body;

    /**
     * Constructor
     */
    InlineMethodExpression(int where, Type type, FieldDefinition field, Statement body) {
	super(INLINEMETHOD, where, type);
	this.field = field;
	this.body = body;
    }
    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	body = body.inline(env, new Context(ctx, this));
	return ((body == null) || (body.op == INLINERETURN)) ? null : this;
    }
    public Expression inlineValue(Environment env, Context ctx) {
	body = body.inline(env, new Context(ctx, this));
	return (body.op == INLINERETURN) ? ((InlineReturnStatement)body).expr : this;
    }

    /**
     * Create a copy of the expression for method inlining
     */
    public Expression copyInline(Context ctx) {
	InlineMethodExpression e = (InlineMethodExpression)clone();
	if (body != null) {
	    e.body = body.copyInline(ctx, true);
	}
	return e;
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	codeValue(env, ctx, asm);
	// don't pop the result because there isn't any
    }
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	CodeContext newctx = new CodeContext(ctx, this);
	body.code(env, newctx, asm);
	asm.add(newctx.breakLabel);
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(" + opNames[op] + "\n");
	body.print(out, 1);
	out.print(")");
    }
}
