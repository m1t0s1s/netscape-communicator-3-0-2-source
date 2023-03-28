/*
 * @(#)CompoundStatement.java	1.15 95/11/15 Arthur van Hoff
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
class CompoundStatement extends Statement {
    Statement args[];

    /**
     * Constructor
     */
    public CompoundStatement(int where, Statement args[]) {
	super(STAT, where);
	this.args = args;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	if (args.length > 0) {
	    vset = reach(env, vset);
	    CheckContext newctx = new CheckContext(ctx, this);
	    for (int i = 0 ; i < args.length ; i++) {
		vset = args[i].check(env, newctx, vset, exp);
	    }
	    vset &= newctx.vsBreak;
	}
	return ctx.removeAdditionalVars(vset);
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	ctx = new Context(ctx, this);
	boolean expand = false;
	int count = 0;
	for (int i = 0 ; i < args.length ; i++) {
	    Statement s = args[i];
	    if (s != null) {
		if ((s = s.inline(env, ctx)) != null) {
		    if ((s.op == STAT) && (s.labels == null)) {
			count += ((CompoundStatement)s).args.length;
		    } else {
			count++;
		    }
		    expand = true;
		}
		args[i] = s;
	    }
	}
	switch (count) {
	  case 0:
	    return null;
	    
	  case 1:
	    for (int i = args.length ; i-- > 0 ;) {
		if (args[i] != null) {
		    return eliminate(env, args[i]);
		}
	    }
	    break;
	}
	if (expand || (count != args.length)) {
	    Statement newArgs[] = new Statement[count];
	    for (int i = args.length ; i-- > 0 ;) {
		Statement s = args[i];
		if (s != null) {
		    if ((s.op == STAT) && (s.labels == null)) {
			Statement a[] = ((CompoundStatement)s).args;
			for (int j = a.length ; j-- > 0 ; ) {
			    newArgs[--count] = a[j];
			}
		    } else {
			newArgs[--count] = s;
		    }
		}
	    }
	    args = newArgs;
	}
	return this;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	CompoundStatement s = (CompoundStatement)clone();
	s.args = new Statement[args.length];
	for (int i = 0 ; i < args.length ; i++) {
	    s.args[i] = args[i].copyInline(ctx, valNeeded);
	}
	return s;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	int cost = 0;
	for (int i = 0 ; (i < args.length) && (cost < thresh) ; i++) {
	    cost += args[i].costInline(thresh);
	}
	return cost;
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	CodeContext newctx = new CodeContext(ctx, this);
	for (int i = 0 ; i < args.length ; i++) {
	    args[i].code(env, newctx, asm);
	}
	asm.add(newctx.breakLabel);
    }

    /**
     * Check if the first thing is a constructor invocation
     */
    public boolean firstConstructor() {
	return (args.length > 0) && args[0].firstConstructor();
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("{\n");
	for (int i = 0 ; i < args.length ; i++) {
	    printIndent(out, indent+1);
	    if (args[i] != null) {
		args[i].print(out, indent + 1);
	    } else {
		out.print("<empty>");
	    }
	    out.print("\n");
	}
	printIndent(out, indent);
	out.print("}");
    }
}
