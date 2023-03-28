/*
 * @(#)Statement.java	1.20 95/11/15 Arthur van Hoff
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
class Statement extends Node {
    public static final long DEAD_END = (long)1 << 63;
    Identifier labels[] = null;

    /**
     * Constructor
     */
    Statement(int op, int where) {
	super(op, where);
    }

    /**
     * Set the label of a statement
     */
    public void setLabel(Environment env, Expression e) {
	if (e.op == IDENT) {
	    if (labels == null) { 
		labels = new Identifier[1];
	    } else { 
		// this should almost never happen.  Multiple labels on
		// the same statement.  But handle it gracefully.
		Identifier newLabels[] = new Identifier[labels.length + 1];
		System.arraycopy(labels, 0, newLabels, 1, labels.length);
		labels = newLabels;
	    }
	    labels[0] = ((IdentifierExpression)e).id;
	} else {
	    env.error(e.where, "invalid.label");
	}
    }

    /**
     * Check a statement
     */
    public long checkMethod(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = check(env, ctx, vset, exp);

	// Check for return
	if (!ctx.field.getType().getReturnType().isType(TC_VOID)) {
	    if ((vset & DEAD_END) == 0) {
		env.error(ctx.field.getWhere(), "return.required.at.end", ctx.field);
	    }
	}
	return vset;
    }
    long checkDeclaration(Environment env, Context ctx, long vset, int mod, Type t, Hashtable exp) {
	throw new CompilerError("checkDeclaration");
    }
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	throw new CompilerError("check");
    }

    long reach(Environment env, long vset) {
	if ((vset & DEAD_END) != 0) {
	    if (env.warnings()) {
		env.error(where, "stat.not.reached");
	    }
	    vset &= ~DEAD_END;
	}
	return vset;
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	return this;
    }

    /**
     * Eliminate this statement, which is only possible if it has no label.
     */
    public Statement eliminate(Environment env, Statement s) {
	if ((s != null) && (labels != null)) {
	    Statement args[] = {s};
	    s = new CompoundStatement(where, args);
	    s.labels = labels;
	}
	return s;
    }
     

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	throw new CompilerError("code");
    }

    /** 
     * Generate the code to call all finally's for a break, continue, or
     * return statement.  We must call "jsr" on all the cleanup code between
     * the current context "ctx", and the destination context "stopctx".  
     * If 'save' isn't null, there is also a value on the top of the stack
     */
    void codeFinally(Environment env, Context ctx, Assembler asm, 
		        Context stopctx, Type save) {
	Integer num = null;
	boolean haveCleanup = false; // there is a finally or synchronize;
	boolean haveNonLocalFinally = false; // some finally doesn't return;

	for (Context c = ctx; (c != null) && (c != stopctx); c = c.prev) {
	    if (c.node == null) 
		continue;
	    if (c.node.op == SYNCHRONIZED) { 
		haveCleanup = true;
	    } else if (c.node.op == FINALLY) { 
		haveCleanup = true;	    
		FinallyStatement st = ((FinallyStatement)(c.node));
		if (!st.finallyFinishes) {
		    haveNonLocalFinally = true;
		    // after hitting a non-local finally, no need generating
		    // further code, because it won't get executed.
		    break;
		}
	    }
	}
	if (!haveCleanup) {
	    // there is no cleanup that needs to be done.  Just quit.
	    return;
	} 
	if (save != null) {
	    // This statement has a return value on the stack.
	    ClassDefinition def = ctx.field.getClassDefinition();
	    if (!haveNonLocalFinally) { 
		// Save the return value in a register
		LocalField lf = new LocalField(where, def, 0, save, null);
		num = new Integer(ctx.declare(env, lf));
		asm.add(where, opc_istore + save.getTypeCodeOffset(), num);
	    } else { 
		// Pop the return value.
		switch(def.getType().getTypeCode()) { 
		    case TC_VOID:                  
			break;
		    case TC_DOUBLE: case TC_LONG:   
			asm.add(where, opc_pop2); break;
		    default:
			asm.add(where, opc_pop); break;
		}
	    }
	}
	// Call each of the cleanup functions, as necessary.
	for (Context c = ctx ; (c != null)  && (c != stopctx) ; c = c.prev) {
	    if (c.node == null) 
		continue;
	    if (c.node.op == SYNCHRONIZED) { 
		asm.add(where, opc_jsr, ((CodeContext)c).contLabel);
	    } else if (c.node.op == FINALLY) { 
		FinallyStatement st = ((FinallyStatement)(c.node));
		Label label = ((CodeContext)c).contLabel;
		if (st.finallyFinishes) { 
		    asm.add(where, opc_jsr, label);
		} else {
		    // the code never returns, so we're done.
		    asm.add(where, opc_goto, label);
		    break;
		}
	    }
	}
	// Move the return value from the register back to the stack.
	if (num != null) {
	    asm.add(where, opc_iload + save.getTypeCodeOffset(), num);
	}
    }

    /* 
     * Return true if the statement has the given label 
     */
    public boolean hasLabel (Identifier lbl) { 
	Identifier labels[] = this.labels;
	if (labels != null) { 
	    for (int i = labels.length; --i >= 0; ) {
		if (labels[i].equals(lbl)) {
		    return true;
		}
	    }
	}
	return false;
    }

    /**
     * Check if the first thing is a constructor invocation
     */
    public boolean firstConstructor() {
	return false;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	return (Statement)clone();
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return thresh;
    }

    /**
     * Print
     */
    void printIndent(PrintStream out, int indent) {
	for (int i = 0 ; i < indent ; i++) {
	    out.print("    ");
	}
    }
    public void print(PrintStream out, int indent) {
	if (labels != null) {
	    for (int i = labels.length; --i >= 0; ) 
		out.print(labels[i] + ": ");
	}
    }
    public void print(PrintStream out) {
	print(out, 0);
    }
}
