/*
 * @(#)DeclarationStatement.java	1.18 95/09/14 Arthur van Hoff
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
import sun.tools.asm.Assembler;
import java.util.Hashtable;

public
class DeclarationStatement extends Statement {
    int mod;
    Expression type;
    Statement args[];

    /**
     * Constructor
     */
    public DeclarationStatement(int where, int mod, Expression type, Statement args[]) {
	super(DECLARATION, where);
	this.mod = mod;
	this.type = type;
	this.args = args;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	try {
	    vset = reach(env, vset);
	    Type t = type.toType(env, ctx);

	    env.resolve(where, ctx.field.getClassDefinition(), t);

	    for (int i = 0 ; i < args.length ; i++) {
		vset = args[i].checkDeclaration(env, ctx, vset, mod, t, exp);
	    }
	} catch (ClassNotFound e) {
	    env.error(where, "class.not.found", e.name, opNames[op]);
	}
	return vset;
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	int n = 0;
	for (int i = 0 ; i < args.length ; i++) {
	    if ((args[i] = args[i].inline(env, ctx)) != null) {
		n++;
	    }
	}
	return (n == 0) ? null : this;
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	for (int i = 0 ; i < args.length ; i++) {
	    if (args[i] != null) {
		args[i].code(env, ctx, asm);
	    }
	}
    }

    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	out.print("declare ");
	super.print(out, indent);
	type.print(out);
	out.print(" ");
	for (int i = 0 ; i < args.length ; i++) {
	    if (i > 0) {
		out.print(", ");
	    }
	    if (args[i] != null)  {
		args[i].print(out);
	    } else {
		out.print("<empty>");
	    }
	}
	out.print(";");
    }
}
