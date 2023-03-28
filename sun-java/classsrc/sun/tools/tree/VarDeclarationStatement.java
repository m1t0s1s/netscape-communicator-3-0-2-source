/*
 * @(#)VarDeclarationStatement.java	1.16 95/09/14 Arthur van Hoff
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
import java.io.PrintStream;
import java.util.Hashtable;

public
class VarDeclarationStatement extends Statement {
    LocalField field;
    Expression expr;

    /**
     * Constructor
     */
    public VarDeclarationStatement(int where, Expression expr) {
	super(VARDECLARATION, where);
	this.expr = expr;
    }
    public VarDeclarationStatement(int where, LocalField field, Expression expr) {
	super(VARDECLARATION, where);
	this.field = field;
	this.expr = expr;
    }

    /**
     * Check statement
     */
    long checkDeclaration(Environment env, Context ctx, long vset, int mod, Type t, Hashtable exp) {
	if (field != null) {
	    ctx.declare(env, field);
	    vset |= (long)1 << field.number;
	    return (expr != null) ? expr.checkValue(env, ctx, vset, exp) : vset;
	}
	
	Expression e = expr;

	if (e.op == ASSIGN) {
	    expr = ((AssignExpression)e).right;
	    e = ((AssignExpression)e).left;
	} else {
	    expr = null;
	}

	boolean declError = t.isType(TC_ERROR);
	while (e.op == ARRAYACCESS) {
	    ArrayAccessExpression array = (ArrayAccessExpression)e;
	    if (array.index != null) {
		env.error(array.index.where, "array.dim.in.type");
		declError = true;
	    }
	    e = array.right;
	    t = Type.tArray(t);
	}
	if (e.op == IDENT) {
	    Identifier id = ((IdentifierExpression)e).id;
	    if (ctx.getLocalField(id) != null) {
		env.error(where, "local.redefined", id);
	    }

	    field = new LocalField(e.where, ctx.field.getClassDefinition(), mod, t, id);
	    ctx.declare(env, field);

	    if (expr != null) {
		vset = expr.checkInitializer(env, ctx, vset, t, exp);
		expr = convert(env, ctx, t, expr);
		vset |= (long)1 << field.number;
	    } else if (declError) {
		vset |= (long)1 << field.number;
	    }
	    return vset;
	}
	env.error(e.where, "invalid.decl");
	return vset;
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	if (!field.isUsed()) {
	    return new ExpressionStatement(where, expr).inline(env, ctx);
	}

	ctx.declare(env, field);

	if (expr != null) {
	    expr = expr.inlineValue(env, ctx);
	    if (env.optimize() && (field.writecount == 0)) {
		if (expr.op == IDENT) {
		    IdentifierExpression e = (IdentifierExpression)expr;
		    if (e.field.isLocal() && ((ctx = ctx.getInlineContext()) != null) &&
			(((LocalField)e.field).number < ctx.varNumber)) {
			//System.out.println("FINAL IDENT = " + field + " in " + ctx.field);
			field.setValue(expr);
			field.addModifiers(M_FINAL);
			expr = null;
			return null;
		    }
		}
		if (expr.isConstant() || (expr.op == THIS) || (expr.op == SUPER)) {
		    //System.out.println("FINAL = " + field + " in " + ctx.field);
		    field.setValue(expr);
		    field.addModifiers(M_FINAL);
		    expr = null;
		    return null;
		}
	    }
	}
	return this;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	VarDeclarationStatement s = (VarDeclarationStatement)clone();
	if (expr != null) {
	    s.expr = expr.copyInline(ctx);
	}
	return s;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return (expr != null) ? expr.costInline(thresh) : 0;
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	if (expr != null) {
	    expr.codeValue(env, ctx, asm);
	    ctx.declare(env, field);
	    asm.add(where, opc_istore + field.getType().getTypeCodeOffset(),
		    new LocalVariable(field, field.number));
	} else {
	    ctx.declare(env, field);
	}
    }
    
    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	out.print("local ");
	if (field != null) {
	    out.print(field + "#" + field.hashCode());
	    if (expr != null) {
		out.print(" = ");
		expr.print(out);
	    }
	} else {
	    expr.print(out);
	    out.print(";");
	}
    }
}
