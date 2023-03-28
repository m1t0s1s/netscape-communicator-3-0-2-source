/*
 * @(#)AssignOpExpression.java	1.18 95/10/03 Arthur van Hoff
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
class AssignOpExpression extends BinaryAssignExpression {
    final int NOINC = Integer.MAX_VALUE;
    
    /**
     * Constructor
     */
    public AssignOpExpression(int op, int where, Expression left, Expression right) {
	super(op, where, left, right);
    }

    /**
     * Get the increment, return NOINC if an increment is not possible
     */
    int getIncrement() {
	if ((left.op == IDENT) && type.isType(TC_INT) && (right.op == INTVAL))
	    if ((op == ASGADD) || (op == ASGSUB))
		if (((IdentifierExpression)left).field.isLocal()) { 
		    int val = ((IntExpression)right).value;
		    if (op == ASGSUB) 
			val = -val;
		    if (val == (short)val)
			return val;
		}
	return NOINC;
    }


    /**
     * Check an assignment expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = left.checkAssignOp(env, ctx, vset, exp, this);
	vset = right.checkValue(env, ctx, vset, exp);
	selectType(env, ctx, 
		   left.type.getTypeMask() | right.type.getTypeMask());
	if (!type.isType(TC_ERROR)) {
	    convert(env, ctx, type, left);
	}
	return vset;
    }

    /**
     * Inline
     */
    public Expression inlineValue(Environment env, Context ctx) {
	left = left.inlineValue(env, ctx);
	right = right.inlineValue(env, ctx);
	return this;
    }

    /**
     * Create a copy of the expression for method inlining
     */
    public Expression copyInline(Context ctx) {
	AssignOpExpression e = (AssignOpExpression)clone();
	e.left = left.copyInline(ctx);
	e.right = right.copyInline(ctx);
	return e;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return (getIncrement() != NOINC) ? 2 : 3 + super.costInline(thresh);
    }

    /**
     * Code
     */
    void code(Environment env, Context ctx, Assembler asm, boolean valNeeded) {
	int val = getIncrement();
	if (val != NOINC) {
	    int v = ((LocalField)((IdentifierExpression)left).field).number;
	    int[] operands = { v, val };
	    asm.add(where, opc_iinc, operands);
	    if (valNeeded) {
		left.codeValue(env, ctx, asm);
	    }
	    return;
	}

	int depth = left.codeLValue(env, ctx, asm);
	codeDup(env, ctx, asm, depth, 0);
	left.codeLoad(env, ctx, asm);
	codeConversion(env, ctx, asm, left.type, type);
	right.codeValue(env, ctx, asm);
	codeOperation(env, ctx, asm);
	codeConversion(env, ctx, asm, type, left.type);
	if (valNeeded) {
	    codeDup(env, ctx, asm, type.stackSize(), depth);
	}
	left.codeStore(env, ctx, asm);
    }
    
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	code(env, ctx, asm, true);
    }
    public void code(Environment env, Context ctx, Assembler asm) {
	code(env, ctx, asm, false);
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(" + opNames[op] + " ");
	left.print(out);
	out.print(" ");
	right.print(out);
	out.print(")");
    }
}
