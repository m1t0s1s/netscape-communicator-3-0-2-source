/*
 * @(#)NaryExpression.java	1.11 95/09/14 Arthur van Hoff
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
import java.io.PrintStream;

public
class NaryExpression extends UnaryExpression {
    Expression args[];

    /**
     * Constructor
     */
    NaryExpression(int op, int where, Type type, Expression right, Expression args[]) {
	super(op, where, type, right);
	this.args = args;
    }

    /**
     * Create a copy of the expression for method inlining
     */
    public Expression copyInline(Context ctx) {
	NaryExpression e = (NaryExpression)clone();
	if (right != null) {
	    e.right = right.copyInline(ctx);
	}
	e.args = new Expression[args.length];
	for (int i = 0 ; i < args.length ; i++) {
	    if (args[i] != null) {
		e.args[i] = args[i].copyInline(ctx);
	    }
	}
	return e;
    }

    /**
     * The cost of inlining this expression
     */
    public int costInline(int thresh) {
	int cost = 3 + ((right != null) ? right.costInline(thresh) : 0);
	for (int i = 0 ; (i < args.length) && (cost < thresh) ; i++) {
	    if (args[i] != null) {
		cost += args[i].costInline(thresh);
	    }
	}
	return cost;
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(" + opNames[op] + "#" + hashCode());
	if (right != null) {
	    out.print(" ");
	    right.print(out);
	}
	for (int i = 0 ; i < args.length ; i++) {
	    out.print(" ");
	    if (args[i] != null) {
		args[i].print(out);
	    } else {
		out.print("<null>");
	    }
	}
	out.print(")");
    }
}
