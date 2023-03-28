/*
 * @(#)SwitchStatement.java	1.16 95/11/17 Arthur van Hoff
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
import sun.tools.asm.SwitchData;
import java.io.PrintStream;
import java.util.Hashtable;

public
class SwitchStatement extends Statement {
    Expression expr;
    Statement args[];

    /**
     * Constructor
     */
    public SwitchStatement(int where, Expression expr, Statement args[]) {
	super(SWITCH, where);
	this.expr = expr;
	this.args = args;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	CheckContext newctx = new CheckContext(ctx, this);
	vset = expr.checkValue(env, newctx, reach(env, vset), exp);
	Type switchType = expr.type;

	expr = convert(env, newctx, Type.tInt, expr);

	Hashtable tab = new Hashtable();
	boolean hasDefault = false;
	long vs = vset;

	for (int i = 0 ; i < args.length ; i++) {
	    Statement s = args[i];
	    
	    if (s.op == CASE) {
		vs = s.check(env, newctx, vset, exp);

		Expression lbl = ((CaseStatement)s).expr;
		if (lbl != null) {
		    if (lbl instanceof IntegerExpression) {
			Integer Ivalue = 
			    (Integer)(((IntegerExpression)lbl).getValue());
			int ivalue = Ivalue.intValue();
			if (tab.get(lbl) != null) {
			    env.error(s.where, "duplicate.label", Ivalue);
			} else {
			    tab.put(lbl, s);
			    boolean overflow;
			    switch (switchType.getTypeCode()) { 
			        case TC_BYTE:
				    overflow = (ivalue != (byte)ivalue); break;
			        case TC_SHORT : 
				    overflow = (ivalue != (short)ivalue); break;
			        case TC_CHAR:
				    overflow = (ivalue != (char)ivalue); break;
			        default: 
				    overflow = false;
			    }
			    if (overflow) { 
				env.error(s.where, "switch.overflow", 
					  Ivalue, switchType);
			    }
			}
		    } else {
			env.error(s.where, "const.expr.required");
		    }
		} else {
		    if (hasDefault) {
			env.error(s.where, "duplicate.default");
		    }
		    hasDefault = true;
		}
	    } else {
		vs = s.check(env, newctx, vs, exp);
	    }
	}
	if ((vs & DEAD_END) == 0) {
	    newctx.vsBreak &= vs;
	}
	if (hasDefault) 
	    vset = newctx.vsBreak;
	return ctx.removeAdditionalVars(vset);
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	ctx = new Context(ctx, this);
	expr = expr.inlineValue(env, ctx);
	for (int i = 0 ; i < args.length ; i++) {
	    if (args[i] != null) {
		args[i] = args[i].inline(env, ctx);
	    }
	}
	return this;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	SwitchStatement s = (SwitchStatement)clone();
	expr = expr.copyInline(ctx);
	s.args = new Statement[args.length];
	for (int i = 0 ; i < args.length ; i++) {
	    if (args[i] != null) {
		s.args[i] = args[i].copyInline(ctx, valNeeded);
	    }
	}
	return s;
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	int cost = expr.costInline(thresh);
	for (int i = 0 ; (i < args.length) && (cost < thresh) ; i++) {
	    if (args[i] != null) {
		cost += args[i].costInline(thresh);
	    }
	}
	return cost;
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	CodeContext newctx = new CodeContext(ctx, this);

	expr.codeValue(env, newctx, asm);

	SwitchData sw = new SwitchData();
	boolean hasDefault = false;

	for (int i = 0 ; i < args.length ; i++) {
	    Statement s = args[i];
	    if ((s != null) && (s.op == CASE)) {
		Expression e = ((CaseStatement)s).expr;
		if (e != null) {
		    sw.add(((IntegerExpression)e).value, new Label());
		}
	    } 
	}

	asm.add(where, opc_tableswitch, sw);

	for (int i = 0 ; i < args.length ; i++) {
	    Statement s = args[i];
	    if (s != null) {
		if (s.op == CASE) {
		    Expression e = ((CaseStatement)s).expr;
		    if (e != null) {
			asm.add(sw.get(((IntegerExpression)e).value));
		    } else {
			asm.add(sw.getDefaultLabel());
			hasDefault = true;
		    }
		} else {
		    s.code(env, newctx, asm);
		}
	    }
	}

	if (!hasDefault) {
	    asm.add(sw.getDefaultLabel());
	}
	asm.add(newctx.breakLabel);
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("switch (");
	expr.print(out);
	out.print(") {\n");
	for (int i = 0 ; i < args.length ; i++) {
	    if (args[i] != null) {
		printIndent(out, indent + 1);
		args[i].print(out, indent + 1);
		out.print("\n");
	    }
	}
	printIndent(out, indent);
	out.print("}");
    }
}
