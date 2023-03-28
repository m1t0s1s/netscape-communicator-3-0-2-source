/*
 * @(#)ThrowStatement.java	1.17 95/10/05 Arthur van Hoff
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
import java.io.PrintStream;
import java.util.Hashtable;

public
class ThrowStatement extends Statement {
    Expression expr;

    /**
     * Constructor
     */
    public ThrowStatement(int where, Expression expr) {
	super(THROW, where);
	this.expr = expr;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	try {
	    vset = reach(env, vset);
	    expr.checkValue(env, ctx, vset, exp);
	    if (expr.type.isType(TC_CLASS)) { 
	        ClassDeclaration c = env.getClassDeclaration(expr.type);
		if (exp.get(c) == null) {
		    exp.put(c, this);
		}
		ClassDefinition def = c.getClassDefinition(env);
		ClassDeclaration throwable = 
		    env.getClassDeclaration(idJavaLangThrowable);
		if (!def.subClassOf(env, throwable)) {
		    env.error(where, "throw.not.throwable", def);
		}
		expr = convert(env, ctx, Type.tObject, expr);
	    } else if (!expr.type.isType(TC_ERROR)) {
	        env.error(expr.where, "throw.not.throwable", expr.type);
	    }
	} catch (ClassNotFound e) {
	    env.error(where, "class.not.found", e.name, opNames[op]);
	}
	return -1;
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	expr = expr.inlineValue(env, ctx);
	return this;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	ThrowStatement s = (ThrowStatement)clone();
	s.expr = expr.copyInline(ctx);
	return s;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return 1 + expr.costInline(thresh);
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	expr.codeValue(env, ctx, asm);
	asm.add(where, opc_athrow);
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("throw ");
	expr.print(out);
	out.print(":");
    }
}
