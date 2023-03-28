/*
 * @(#)CaseStatement.java	1.11 95/09/14 Arthur van Hoff
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
import java.util.Hashtable;

public
class CaseStatement extends Statement {
    Expression expr;

    /**
     * Constructor
     */
    public CaseStatement(int where, Expression expr) {
	super(CASE, where);
	this.expr = expr;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	if (expr != null) {
	    expr.checkValue(env, ctx, vset, exp);
	    expr = convert(env, ctx, Type.tInt, expr);
	    expr = expr.inlineValue(env, ctx);
	}
	return vset & ~DEAD_END;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return 6;
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	if (expr == null) {
	    out.print("default");
	} else {
	    out.print("case ");
	    expr.print(out);
	}
	out.print(":");
    }
}
