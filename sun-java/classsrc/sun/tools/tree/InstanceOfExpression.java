/*
 * @(#)InstanceOfExpression.java	1.14 95/11/16 Arthur van Hoff
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
import java.io.PrintStream;
import java.util.Hashtable;

public
class InstanceOfExpression extends BinaryExpression {
    /**
     * constructor
     */
    public InstanceOfExpression(int where, Expression left, Expression right) {
	super(INSTANCEOF, where, Type.tBoolean, left, right);
    }

    /**
     * Check the expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = left.checkValue(env, ctx, vset, exp);
	right = new TypeExpression(right.where, right.toType(env, ctx));

	if (right.type.isType(TC_ERROR) || left.type.isType(TC_ERROR)) {
	    // An error was already reported
	    return vset;
	}

	if (!right.type.inMask(TM_CLASS|TM_ARRAY)) {
	    env.error(right.where, "invalid.arg.type", right.type, opNames[op]);
	    return vset;
	} 
	try {
	    if (!env.explicitCast(left.type, right.type)) {
		env.error(where, "invalid.instanceof", left.type, right.type);
	    }
	} catch (ClassNotFound e) { 
	    env.error(where, "class.not.found", e.name, opNames[op]);
	}
	return vset;
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	return left.inline(env, ctx);
    }
    public Expression inlineValue(Environment env, Context ctx) {
	left = left.inlineValue(env, ctx);
	return this;
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	left.codeValue(env, ctx, asm);
	if (right.type.isType(TC_CLASS)) {
	    asm.add(where, opc_instanceof, env.getClassDeclaration(right.type));
	} else {
	    asm.add(where, opc_instanceof, right.type);
	}
    }
    void codeBranch(Environment env, Context ctx, Assembler asm, Label lbl, boolean whenTrue) {
	codeValue(env, ctx, asm);
	asm.add(where, whenTrue ? opc_ifne : opc_ifeq, lbl);
    }
    public void code(Environment env, Context ctx, Assembler asm) {
	left.code(env, ctx, asm);
    }
    
    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(" + opNames[op] + " ");
	left.print(out);
	out.print(" ");
	if (right.op == TYPE) {
	    out.print(right.type.toString());
	} else {
	    right.print(out);
	}
	out.print(")");
    }
}
