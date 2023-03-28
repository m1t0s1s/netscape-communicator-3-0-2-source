/*
 * @(#)CastExpression.java	1.13 95/09/14 Arthur van Hoff
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
class CastExpression extends BinaryExpression {
    /**
     * constructor
     */
    public CastExpression(int where, Expression left, Expression right) {
	super(CAST, where, left.type, left, right);
    }

    /**
     * Check the expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	type = left.toType(env, ctx);
	vset = right.checkValue(env, ctx, vset, exp);

	if (type.isType(TC_ERROR) || right.type.isType(TC_ERROR)) {
	    // An error was already reported
	    return vset;
	}

	if (type.equals(right.type)) {
	    // The types are already the same
	    return vset;
	}

	try {
	    if (env.explicitCast(right.type, type)) {
		right = new ConvertExpression(where, type, right);
		return vset;
	    }
	} catch (ClassNotFound e) {
	    env.error(where, "class.not.found", e.name, opNames[op]);
	}

	// The cast is not allowed
	env.error(where, "invalid.cast", right.type, type);
	return vset;
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	return right.inline(env, ctx);
    }
    public Expression inlineValue(Environment env, Context ctx) {
	return right.inlineValue(env, ctx);
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(" + opNames[op] + " ");
	if (type.isType(TC_ERROR)) {
	    left.print(out);
	} else {
	    out.print(type);
	}
	out.print(" ");
	right.print(out);
	out.print(")");
    }
}
