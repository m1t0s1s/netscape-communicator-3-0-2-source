/*
 * @(#)ArrayAccessExpression.java	1.15 95/10/03 Arthur van Hoff
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
class ArrayAccessExpression extends UnaryExpression {
    Expression index;
    
    /**
     * constructor
     */
    public ArrayAccessExpression(int where, Expression right, Expression index) {
	super(ARRAYACCESS, where, Type.tError, right);
	this.index = index;
    }

    /**
     * Check expression type
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = right.checkValue(env, ctx, vset, exp);
	if (index == null) {
	    env.error(where, "array.index.required");
	    return vset;
	}
	vset = index.checkValue(env, ctx, vset, exp);
	index = convert(env, ctx, Type.tInt, index);

	if (!right.type.isType(TC_ARRAY)) {
	    if (!right.type.isType(TC_ERROR)) {
		env.error(where, "not.array", right.type);
	    }
	    return vset;
	}

	type = right.type.getElementType();
	return vset;
    }

    /* 
     * Check the array if it appears on the LHS of an assignment
     */
    public long checkLHS(Environment env, Context ctx, 
			 long vset, Hashtable exp) {
	return checkValue(env, ctx, vset, exp);
    }

    /* 
     * Check the array if it appears on the LHS of an op= expression
     */
    public long checkAssignOp(Environment env, Context ctx, 
			      long vset, Hashtable exp, Expression outside) {
	return checkValue(env, ctx, vset, exp);
    }

    /**
     * Convert to a type
     */
    Type toType(Environment env, Context ctx) {
	return toType(env, right.toType(env, ctx));
    }
    Type toType(Environment env, Type t) {
	if (index != null) {
	    env.error(index.where, "array.dim.in.type");
	}
	return Type.tArray(t);
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	return new CommaExpression(where, right, index).inline(env, ctx);
    }
    public Expression inlineValue(Environment env, Context ctx) {
	right = right.inlineValue(env, ctx);
	index = index.inlineValue(env, ctx);
	return this;
    }
    public Expression inlineLHS(Environment env, Context ctx) {
	return inlineValue(env, ctx);
    }

    /**
     * Create a copy of the expression for method inlining
     */
    public Expression copyInline(Context ctx) {
	ArrayAccessExpression e = (ArrayAccessExpression)clone();
	e.right = right.copyInline(ctx);
	e.index = index.copyInline(ctx);
	return e;
    }

    /**
     * The cost of inlining this expression
     */
    public int costInline(int thresh) {
	return 1 + right.costInline(thresh) + index.costInline(thresh);
    }

    /**
     * Code
     */
    int codeLValue(Environment env, Context ctx, Assembler asm) {
	right.codeValue(env, ctx, asm);
	index.codeValue(env, ctx, asm);
	return 2;
    }
    void codeLoad(Environment env, Context ctx, Assembler asm) {
	switch (type.getTypeCode()) {
	  case TC_BOOLEAN:
	  case TC_BYTE:
	    asm.add(where, opc_baload);
	    break;
	  case TC_CHAR:
	    asm.add(where, opc_caload);
	    break;
	  case TC_SHORT:
	    asm.add(where, opc_saload);
	    break;
	  default:
	    asm.add(where, opc_iaload + type.getTypeCodeOffset());
	}
    }
    void codeStore(Environment env, Context ctx, Assembler asm) {
	switch (type.getTypeCode()) {
	  case TC_BOOLEAN:
	  case TC_BYTE:
	    asm.add(where, opc_bastore);
	    break;
	  case TC_CHAR:
	    asm.add(where, opc_castore);
	    break;
	  case TC_SHORT:
	    asm.add(where, opc_sastore);
	    break;
	  default:
	    asm.add(where, opc_iastore + type.getTypeCodeOffset());
	}
    }
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	codeLValue(env, ctx, asm);
	codeLoad(env, ctx, asm);
    }
    

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(" + opNames[op] + " ");
	right.print(out);
	out.print(" ");
	if (index != null) {
	    index.print(out);
	} else {
	out.print("<empty>");
	}
	out.print(")");
    }
}
