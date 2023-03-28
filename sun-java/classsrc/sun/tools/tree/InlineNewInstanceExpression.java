/*
 * @(#)InlineNewInstanceExpression.java	1.8 95/09/14 Arthur van Hoff
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
import java.util.Vector;

public
class InlineNewInstanceExpression extends Expression {
    FieldDefinition field;
    Statement body;

    /**
     * Constructor
     */
    InlineNewInstanceExpression(int where, Type type, FieldDefinition field, Statement body) {
	super(INLINENEWINSTANCE, where, type);
	this.field = field;
	this.body = body;
    }
    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	return inlineValue(env, ctx);
    } 
    public Expression inlineValue(Environment env, Context ctx) {
	if (body != null) {
	    LocalField v = (LocalField)field.getArguments().elementAt(0);
	    Context newctx = new Context(ctx, this);
	    newctx.declare(env, v);
	    body = body.inline(env, newctx);
	}
	if ((body != null) && (body.op == INLINERETURN)) {
	    body = null;
	}
	return this;
    }

    /**
     * Create a copy of the expression for method inlining
     */
    public Expression copyInline(Context ctx) {
	InlineNewInstanceExpression e = (InlineNewInstanceExpression)clone();
	e.body = body.copyInline(ctx, true);
	return e;
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_new, field.getClassDeclaration());
	if (body != null) {
	    LocalField v = (LocalField)field.getArguments().elementAt(0);
	    CodeContext newctx = new CodeContext(ctx, this);
	    newctx.declare(env, v);
	    asm.add(where, opc_astore, new Integer(v.number));
	    body.code(env, newctx, asm);
	    asm.add(newctx.breakLabel);
	}
    }
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_new, field.getClassDeclaration());
	if (body != null) {
	    LocalField v = (LocalField)field.getArguments().elementAt(0);
	    CodeContext newctx = new CodeContext(ctx, this);
	    newctx.declare(env, v);
	    asm.add(where, opc_astore, new Integer(v.number));
	    body.code(env, newctx, asm);
	    asm.add(newctx.breakLabel);
	    asm.add(where, opc_aload, new Integer(v.number));
	}
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	LocalField v = (LocalField)field.getArguments().elementAt(0);
	out.println("(" + opNames[op] + "#" + v.hashCode() + "=" + field.hashCode());
	if (body != null) {
	    body.print(out, 1);
	} else {
	    out.print("<empty>");
	}
	out.print(")");
    }
}
