/*
 * @(#)CatchStatement.java	1.22 95/11/22 Arthur van Hoff
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
import sun.tools.asm.LocalVariable;
import sun.tools.asm.Label;
import java.io.PrintStream;
import java.util.Hashtable;

public
class CatchStatement extends Statement {
    Expression texpr;
    Identifier id;
    Statement body;
    LocalField field;

    /**
     * Constructor
     */
    public CatchStatement(int where, Expression texpr, Identifier id, Statement body) {
	super(CATCH, where);
	this.texpr = texpr;
	this.id = id;
	this.body = body;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = reach(env, vset);
	ctx = new Context(ctx, this);
	Type type = texpr.toType(env, ctx);

	try {
	    env.resolve(where, ctx.field.getClassDefinition(), type);

	    if (ctx.getLocalField(id) != null) {
		env.error(where, "local.redefined", id);
	    }

	    if (type.isType(TC_ERROR)) { 
		// error message printed out elsewhere
	    } else if (!type.isType(TC_CLASS)) {
		env.error(where, "catch.not.throwable", type);
	    } else { 
		ClassDefinition def = env.getClassDefinition(type);
		if (!def.subClassOf(env, 
			       env.getClassDeclaration(idJavaLangThrowable))) {
		    env.error(where, "catch.not.throwable", def);
		}
	    }

	    field = new LocalField(where, ctx.field.getClassDefinition(), 0, type, id);
	    ctx.declare(env, field);
	    vset |= (long)1 << field.number;

	    return body.check(env, ctx, vset, exp);
	} catch (ClassNotFound e) {
	    env.error(where, "class.not.found", e.name, opNames[op]);
	    return vset;
	}
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	ctx = new Context(ctx, this);
	if (field.isUsed()) {
	    ctx.declare(env, field);
	}
	if (body != null) {
	    body = body.inline(env, ctx);
	}
	return this;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	CatchStatement s = (CatchStatement)clone();
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
	if (field.isUsed()) {
	    newctx.declare(env, field);
	    asm.add(where, opc_astore, new LocalVariable(field, field.number));
	} else {
	    asm.add(where, opc_pop);
	}
	if (body != null) {
	    body.code(env, newctx, asm);
	}
	//asm.add(newctx.breakLabel);
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("catch (");
	texpr.print(out);
	out.print(" " + id + ") ");
	if (body != null) {
	    body.print(out, indent);
	} else {
	    out.print("<empty>");
	}
    }
}
