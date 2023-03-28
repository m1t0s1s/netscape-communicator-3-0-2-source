/*
 * @(#)TryStatement.java	1.27 95/11/22 Arthur van Hoff
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
import sun.tools.asm.TryData;
import sun.tools.asm.CatchData;
import java.io.PrintStream;
import java.util.Enumeration;
import java.util.Hashtable;

public
class TryStatement extends Statement {
    Statement body;
    Statement args[];

    /**
     * Constructor
     */
    public TryStatement(int where, Statement body, Statement args[]) {
	super(TRY, where);
	this.body = body;
	this.args = args;
    }

    /**
     * Check statement
     */
    long check(Environment env, Context ctx, long vset, Hashtable exp) {
	try {
	    vset = reach(env, vset);
	    Hashtable newexp = new Hashtable();
	    CheckContext newctx =  new CheckContext(ctx, this);
	    long vs = body.check(env, newctx, vset, newexp);
	    
	    for (int i = 0 ; i < args.length ; i++) {
		vs &= args[i].check(env, newctx, vset, exp);;
	    }

	    // Check that catch statements are actually reached
	    for (int i = 1 ; i < args.length ; i++) {
		CatchStatement cs = (CatchStatement)args[i];
		if (cs.field == null) {
		    continue;
		}
		Type type = cs.field.getType();
		ClassDefinition def = env.getClassDefinition(type);

		for (int j = 0 ; j < i ; j++) {
		    CatchStatement cs2 = (CatchStatement)args[j];
		    if (cs2.field == null) {
			continue;
		    }
		    Type t = cs2.field.getType();
		    ClassDeclaration c = env.getClassDeclaration(t);
		    if (def.subClassOf(env, c)) {
			env.error(args[i].where, "catch.not.reached");
			break;
		    }
		}
	    }

	    ClassDeclaration ignore1 = env.getClassDeclaration(idJavaLangError);
	    ClassDeclaration ignore2 = env.getClassDeclaration(idJavaLangRuntimeException);

	    // Make sure the exception is actually throw in that part of the code
	    for (int i = 0 ; i < args.length ; i++) {
		CatchStatement cs = (CatchStatement)args[i];
		if (cs.field == null) {
		    continue;
		}
		Type type = cs.field.getType();
		if (!type.isType(TC_CLASS)) {
		    // CatchStatement.checkValue() will have already printed
		    // an error message
		    continue;
		}

		ClassDefinition def = env.getClassDefinition(type);

		// Anyone can throw these!
		if (def.subClassOf(env, ignore1) || def.superClassOf(env, ignore1) || 
		    def.subClassOf(env, ignore2) || def.superClassOf(env, ignore2)) {
		    continue;
		}

		// Make sure the exception is actually throw in that part of the code
		boolean ok = false;
		for (Enumeration e = newexp.keys() ; e.hasMoreElements() ; ) {
		    ClassDeclaration c = (ClassDeclaration)e.nextElement();
		    if (def.superClassOf(env, c) || def.subClassOf(env, c)) {
			ok = true;
			break;
		    }
		}
		if (!ok) {
		    env.error(cs.where, "catch.not.thrown", def.getName());
		}
	    }

	    // Only carry over exceptions that are not caught
	    for (Enumeration e = newexp.keys() ; e.hasMoreElements() ; ) {
		ClassDeclaration c = (ClassDeclaration)e.nextElement();
		ClassDefinition def = c.getClassDefinition(env);
		boolean add = true;
		for (int i = 0 ; i < args.length ; i++) {
		    CatchStatement cs = (CatchStatement)args[i];
		    if (cs.field == null) {
			continue;
		    }
		    Type type = cs.field.getType();
		    if (type.isType(TC_ERROR))
			continue;
		    if (def.subClassOf(env, env.getClassDeclaration(type))) {
			add = false;
			break;
		    }
		}
		if (add) {
		    exp.put(c, newexp.get(c));
		}
	    }
	    return ctx.removeAdditionalVars(vs & newctx.vsBreak);
	} catch (ClassNotFound e) {
	    env.error(where, "class.not.found", e.name, opNames[op]);
	    return vset;
	}
    }

    /**
     * Inline
     */
    public Statement inline(Environment env, Context ctx) {
	if (body != null) {
	    body = body.inline(env, new Context(ctx, this));
	}
	if (body == null) {
	    return null;
	}
	for (int i = 0 ; i < args.length ; i++) {
	    if (args[i] != null) {
		args[i] = args[i].inline(env, new Context(ctx, this));
	    }
	}
	return (args.length == 0) ? eliminate(env, body) : this;
    }

    /**
     * Create a copy of the statement for method inlining
     */
    public Statement copyInline(Context ctx, boolean valNeeded) {
	throw new CompilerError("copyInline");
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	CodeContext newctx = new CodeContext(ctx, this);
	
	TryData td = new TryData();
	for (int i = 0 ; i < args.length ; i++) {
	    Type t = ((CatchStatement)args[i]).field.getType();
	    if (t.isType(TC_CLASS)) {
		td.add(env.getClassDeclaration(t));
	    } else {
		td.add(t);
	    }
	}
	asm.add(where, opc_try, td);
	if (body != null) {
	    body.code(env, newctx, asm);
	}

	asm.add(td.getEndLabel());
	asm.add(where, opc_goto, newctx.breakLabel);
	
	for (int i = 0 ; i < args.length ; i++) {
	    CatchData cd = td.getCatch(i);
	    asm.add(cd.getLabel());
	    args[i].code(env, newctx, asm);
	    asm.add(where, opc_goto, newctx.breakLabel);
	}
	
	asm.add(newctx.breakLabel);
    }
    
    /**
     * Print
     */
    public void print(PrintStream out, int indent) {
	super.print(out, indent);
	out.print("try ");
	if (body != null) {
	    body.print(out, indent);
	} else {
	    out.print("<empty>");
	}
	for (int i = 0 ; i < args.length ; i++) {
	    out.print(" ");
	    args[i].print(out, indent);
	}
    }
}
