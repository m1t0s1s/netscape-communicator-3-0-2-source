/*
 * @(#)ThisExpression.java	1.15 95/09/14 Arthur van Hoff
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
class ThisExpression extends Expression {
    LocalField field;
    
    /**
     * Constructor
     */
    public ThisExpression(int where) {
	super(THIS, where, Type.tObject);
    }
    public ThisExpression(int where, Context ctx) {
	super(THIS, where, Type.tObject);
	if (ctx.field.isMethod()) {
	    field = ctx.getLocalField(idThis);
	    field.readcount++;
	}
    }

    /**
     * Check expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	if (ctx.field.isStatic()) {
	    env.error(where, "undef.var", idThis);
	    type = Type.tError;
	    return vset;
	}
	if ((vset & 1) == 0) {
	    env.error(where, "access.inst.before.super", idThis);
	    type = Type.tError;
	    return vset;
	}
	type = ctx.field.getClassDeclaration().getType();
	if (ctx.field.isMethod()) {
	    field = ctx.getLocalField(idThis);
	    field.readcount++;
	}
	return vset;
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	return null;
    }
    public Expression inlineValue(Environment env, Context ctx) {
	if (field != null) {
	    Expression e = (Expression)field.getValue(env);
	    //System.out.println("INLINE = "+ e + ", THIS");
	    if (e != null) {
		return e;
	    }
	}
	return this;
    }

    /**
     * Create a copy of the expression for method inlining
     */
    public Expression copyInline(Context ctx) {
	ThisExpression e = (ThisExpression)clone();
	if (field == null) {
	    // The expression is copied into the context of a method
	    e.field = ctx.getLocalField(idThis);
	    e.field.readcount++;
	}
	return e;
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_aload, new Integer(field.number));
    }
    
    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("this#" + ((field != null) ? field.hashCode() : 0));
    }
}
