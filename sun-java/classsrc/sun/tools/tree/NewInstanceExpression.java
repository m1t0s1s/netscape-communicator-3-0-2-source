/*
 * @(#)NewInstanceExpression.java	1.25 95/09/14 Arthur van Hoff
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
import java.util.Vector;
import java.util.Hashtable;

public
class NewInstanceExpression extends NaryExpression {
    FieldDefinition field;
    
    /**
     * Constructor
     */
    public NewInstanceExpression(int where, Expression right, Expression args[]) {
	super(NEWINSTANCE, where, Type.tError, right, args);
    }
    public NewInstanceExpression(int where, FieldDefinition field, Expression args[]) {
	super(NEWINSTANCE, where, field.getType(), null, args);
	this.field = field;
    }

    int precedence() {
	return 100;
    }

    /**
     * Check expression type
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	// What type?
	Type t = right.toType(env, ctx);
	right = new TypeExpression(right.where, t);
	boolean hasErrors = t.isType(TC_ERROR);

	if (!t.isType(TC_CLASS)) {
	    if (!hasErrors) {
		env.error(where, "invalid.arg.type", t, opNames[op]);
		hasErrors = true;
	    }
	}

	// Compose a list of argument types
	Type argTypes[] = new Type[args.length];

	for (int i = 0 ; i < args.length ; i++) {
	    vset = args[i].checkValue(env, ctx, vset, exp);
	    argTypes[i] = args[i].type;
	    hasErrors = hasErrors || argTypes[i].isType(TC_ERROR);
	}

	// Check if there are any type errors in the arguments
	if (hasErrors) {
	    type = Type.tError;
	    return vset;
	}

	ClassDeclaration c = env.getClassDeclaration(t);

	try {
	    ClassDefinition def = c.getClassDefinition(env);

	    // Check if it is an interface
	    if (def.isInterface()) {
		env.error(where, "new.intf", c);
		return vset;
	    }

	    // Check for abstract class
	    if (def.isAbstract(env)) {
		env.error(where, "new.abstract", c);
		return vset;
	    }

	    // Check for class access
	    if (!ctx.field.getClassDefinition().canAccess(env, def.getClassDeclaration())) {
		env.error(where, "cant.access.class", def);
		return vset;
	    }
	    
	    // Get the method field, given the argument types
	    ClassDefinition sourceClass = ctx.field.getClassDefinition();
	    field = def.matchMethod(env, sourceClass, idInit, argTypes);
	    if (field == null) {
		String sig = c.getName().getName().toString();
		sig = Type.tMethod(Type.tError, argTypes).typeString(sig, false, false);
		env.error(where, "unmatched.constr", sig, c);
		return vset;
	    }

	} catch (ClassNotFound ee) {
	    env.error(where, "class.not.found", ee.name, opNames[op]);
	    return vset;
	    
	} catch (AmbiguousField ee) {
	    env.error(where, "ambig.constr", ee.field1, ee.field2);
	    return vset;
	}

	// Cast arguments
	argTypes = field.getType().getArgumentTypes();
	for (int i = 0 ; i < args.length ; i++) {
	    args[i] = convert(env, ctx, argTypes[i], args[i]);
	}

	// Throw the declared exceptions.
	ClassDeclaration exceptions[] = field.getExceptions(env);
	for (int i = 0 ; i < exceptions.length ; i++) {
	    if (exp.get(exceptions[i]) == null) {
		exp.put(exceptions[i], this);
	    }
	}

	type = t;
	return vset;
    }

    /**
     * Check void expression
     */
    public long check(Environment env, Context ctx, long vset, Hashtable exp) {
	return checkValue(env, ctx, vset, exp);
    }

    /**
     * Inline
     */
    final int MAXINLINECOST = 30;

    Expression inlineNewInstance(Environment env, Context ctx, Statement s) {
	if (env.dump()) {
	    System.out.println("INLINE NEW INSTANCE " + field + " in " + ctx.field);
	}
	Vector v = field.getArguments();
	Statement body[] = new Statement[v.size() + 1];

	for (int i = 0 ; i < args.length ; i++) {
	    body[i] = new VarDeclarationStatement(where, (LocalField)v.elementAt(i + 1), args[i]);
	}
	//System.out.print("BEFORE:"); s.print(System.out); System.out.println();
	body[body.length - 1] = (s != null) ? s.copyInline(ctx, false) : null;
	//System.out.print("COPY:"); body[body.length - 1].print(System.out); System.out.println();
	//System.out.print("AFTER:"); s.print(System.out); System.out.println();


	return new InlineNewInstanceExpression(where, type, field, new CompoundStatement(where, body)).inline(env, ctx);
    }

    public Expression inline(Environment env, Context ctx) {
	return inlineValue(env, ctx);
    }
    public Expression inlineValue(Environment env, Context ctx) {
	//right = right.inlineValue(env, ctx);

	try {
	    for (int i = 0 ; i < args.length ; i++) {
		args[i] = args[i].inlineValue(env, ctx);
	    }
	    if (env.optimize() && false && field.isInlineable(env, false) &&
		(!ctx.field.isInitializer()) && ctx.field.isMethod() &&
		(ctx.getInlineFieldContext(field) == null)) {
		Statement s = (Statement)field.getValue(env);
		if ((s == null) || (s.costInline(MAXINLINECOST) < MAXINLINECOST))  {
		    return inlineNewInstance(env, ctx, s);
		}
	    }
	} catch (ClassNotFound e) {
	    throw new CompilerError(e);
	}
	return this;
    }

    /**
     * Code
     */
    public void code(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_new, field.getClassDeclaration());
	for (int i = 0 ; i < args.length ; i++) {
	    args[i].codeValue(env, ctx, asm);
	}
	asm.add(where, opc_invokenonvirtual, field);
    }
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_new, field.getClassDeclaration());
	asm.add(where, opc_dup);
	for (int i = 0 ; i < args.length ; i++) {
	    args[i].codeValue(env, ctx, asm);
	}
	asm.add(where, opc_invokenonvirtual, field);
    }
}
