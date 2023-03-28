/*
 * @(#)ContinueStatement.java	1.13 95/09/14 Arthur van Hoff
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
class ContinueStatement extends Statement {
    Identifier lbl;

    /**
     * Constructor
     */
    public ContinueStatement(int where, Identifier lbl) {
	super(CONTINUE, where);
	this.lbl = lbl;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	reach(env, vset);
	CheckContext destctx = (CheckContext)new Context(ctx, this).getContinueContext(lbl);
	if (destctx != null) {
	    switch (destctx.node.op) {
	      case FOR:
	      case DO:
	      case WHILE:
		destctx.vsContinue &= vset;
		break;
	      default:
		env.error(where, "invalid.continue");
	    }
	} else {
	    if (lbl != null) {
		env.error(where, "label.not.found", lbl);
	    } else {
		env.error(where, "invalid.continue");
	    }
	}
	return -1;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return 1;
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	CodeContext destctx = (CodeContext)ctx.getContinueContext(lbl);
	codeFinally(env, ctx, asm, destctx, null);
	asm.add(where, opc_goto, destctx.contLabel);
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("continue");
	if (lbl != null) {
	    out.print(" " + lbl);
	}
	out.print(";");
    }
}
