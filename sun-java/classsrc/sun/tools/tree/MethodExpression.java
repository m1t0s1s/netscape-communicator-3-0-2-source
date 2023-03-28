/*
 * @(#)MethodExpression.java	1.37 95/11/29 Arthur van Hoff
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
import java.io.PrintStream;
import java.util.Hashtable;

public
class MethodExpression extends NaryExpression {
    Identifier id;
    ClassDefinition clazz;
    FieldDefinition field;
    
    /**
     * constructor
     */
    public MethodExpression(int where, Expression right, Identifier id, Expression args[]) {
	super(METHOD, where, Type.tError, right, args);
	this.id = id;
    }
    public MethodExpression(int where, Expression right, FieldDefinition field, Expression args[]) {
	super(METHOD, where, field.getType().getReturnType(), right, args);
	this.id = field.getName();
	this.field = field;
	this.clazz = field.getClassDefinition();
    }

    /**
     * Check expression type
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	ClassDeclaration c = null;
	boolean staticRef = false;
	Type argTypes[] = new Type[args.length];

	try {
	    if (right == null) {
		staticRef = ctx.field.isStatic();
		c = ctx.field.getClassDeclaration();
	    } else {
		if (id.equals(idInit)) {
		    if (!ctx.field.isConstructor()) {
			env.error(where, "invalid.constr.invoke");
			return vset | 1;
		    }
		    if ((vset & 1) != 0) {
			env.error(where, "constr.invoke.not.first");
			return vset;
		    }
		    right.checkValue(env, ctx, vset | 1, exp); 
		} else {
		    IdentifierExpression errorName[] = new IdentifierExpression[1];
		    Expression mungeRight = 
			FieldExpression.mungeFieldRight(right, env, ctx, errorName);
		    if (mungeRight == null) {
			env.error(errorName[0].where, 
				  "undef.var", errorName[0].id);
			return vset;
		    } else { 
			right = mungeRight;
			if (mungeRight instanceof TypeExpression) { 
			    staticRef = true;
			} else {
			    vset = right.checkValue(env, ctx, vset, exp);
			}
		    }
		}
		if (right.type.isType(TC_CLASS)) {
		    c = env.getClassDeclaration(right.type);
		} else if (right.type.isType(TC_ARRAY)) {
		    c = env.getClassDeclaration(Type.tObject);
		} else {
		    if (!right.type.isType(TC_ERROR)) {
			env.error(where, "invalid.method.invoke", right.type);
		    }
		    return vset;
		}
	    }
	    
	    // Compose a list of argument types
	    boolean hasErrors = false;
	    
	    for (int i = 0 ; i < args.length ; i++) {
		vset = args[i].checkValue(env, ctx, vset, exp);
		argTypes[i] = args[i].type;
		hasErrors = hasErrors || argTypes[i].isType(TC_ERROR);
	    }

	    // "this" is defined after the constructor invocation
	    if (id.equals(idInit)) {
		vset |= 1;
	    }

	    // Check if there are any type errors in the arguments
	    if (hasErrors) {
		return vset;
	    }

	    // Get the method field, given the argument types
	    clazz = c.getClassDefinition(env); 
	    ClassDefinition sourceClass = ctx.field.getClassDefinition(); 
	    field = clazz.matchMethod(env, sourceClass, id, argTypes);
	    if (field == null) {
		if (id.equals(idInit)) {
		    String sig = clazz.getName().getName().toString();
		    sig = Type.tMethod(Type.tError, argTypes).typeString(sig, false, false);
		    env.error(where, "unmatched.constr", sig, c);
		    return vset;
		}

		String sig = id.toString();
		sig = Type.tMethod(Type.tError, argTypes).typeString(sig, false, false);
		if (clazz.findAnyMethod(env, id) == null) {
		    env.error(where, "undef.meth", sig, c);
		} else {
		    env.error(where, "unmatched.meth", sig, c);
		}
		return vset;
	    }

	    // Make sure that static references are allowed
	    if (staticRef && !field.isStatic()) { 
		env.error(where, "no.static.meth.access", 
			  field, field.getClassDeclaration());
		return vset;
	    }

	    if (field.isProtected() 
		  && !(right == null) 
		  && !(right instanceof SuperExpression)
		  && !sourceClass.protectedAccess(env, field, right.type)) {
		env.error(where, "invalid.protected.method.use", 
			  field.getName(), field.getClassDeclaration(), 
			  right.type);
		return vset;
	    }

	    // Make sure that we are not invoking an abstract method
	    if (field.isAbstract() && (right != null) && (right.op == SUPER)) {
		env.error(where, "invoke.abstract", field, field.getClassDeclaration());
		return vset;
	    }

	    // Check for recursive constructor
	    if (field.isConstructor() && ctx.field.equals(field)) {
		env.error(where, "recursive.constr", field);
	    }

	    ctx.field.getClassDefinition().addDependency(field.getClassDeclaration());
	} catch (ClassNotFound ee) {
	    env.error(where, "class.not.found", ee.name, ctx.field);
	    return vset;
	    
	} catch (AmbiguousField ee) {
	    env.error(where, "ambig.field", id, ee.field1, ee.field2);
	    return vset;
	}

	// Make sure it is qualified
	if ((right == null) && !field.isStatic()) {
	    right = new ThisExpression(where, ctx);
	    if ((vset & 1) == 0) {
		env.error(where, "access.inst.before.super", idThis);
	    }
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

	type = field.getType().getReturnType();
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

    Expression inlineMethod(Environment env, Context ctx, Statement s, boolean valNeeded) {
	if (env.dump()) {
	    System.out.println("INLINE METHOD " + field + " in " + ctx.field);
	}
	Vector v = field.getArguments();
	Statement body[] = new Statement[v.size() + 2];

	int n = 0;
	if (field.isStatic()) {
	    body[0] = new ExpressionStatement(where, right);
	} else {
	    if ((right != null) && (right.op == SUPER)) {
		right = new ThisExpression(right.where, ctx);
	    }
	    body[0] = new VarDeclarationStatement(where, (LocalField)v.elementAt(n++), right);
	}
	for (int i = 0 ; i < args.length ; i++) {
	    body[i + 1] = new VarDeclarationStatement(where, (LocalField)v.elementAt(n++), args[i]);
	}
	//System.out.print("BEFORE:"); s.print(System.out); System.out.println();
	body[body.length - 1] = (s != null) ? s.copyInline(ctx, valNeeded) : null;
	//System.out.print("COPY:"); body[body.length - 1].print(System.out); System.out.println();


	Expression e = new InlineMethodExpression(where, type, field, new CompoundStatement(where, body));
	return valNeeded ? e.inlineValue(env, ctx) : e.inline(env, ctx);
    }
    
    public Expression inline(Environment env, Context ctx) {
	try {
	    if (right != null) {
		right = field.isStatic() ? right.inline(env, ctx) : right.inlineValue(env, ctx);
	    }
	    for (int i = 0 ; i < args.length ; i++) {
		args[i] = args[i].inlineValue(env, ctx);
	    }

	    Expression e = this;
	    if (env.optimize() && field.isInlineable(env, clazz.isFinal()) &&
		((id == null) || !id.equals(idInit)) && 
		(!ctx.field.isInitializer()) && ctx.field.isMethod() &&
		(ctx.getInlineFieldContext(field) == null)) {
		Statement s = (Statement)field.getValue(env);
		if ((s == null) || (s.costInline(MAXINLINECOST) < MAXINLINECOST))  {
		    e = inlineMethod(env, ctx, s, false);
		}
	    }
	    if (ctx.field.isConstructor() && field.isConstructor() && (right != null) && (right.op == SUPER) && (id != null)) {
		id = null;
		// insert instance initializers
		for (FieldDefinition f = ctx.field.getClassDefinition().getFirstField() ; f != null ; f = f.getNextField()) {
		    if (f.isVariable() && !f.isStatic()) {
			Expression val = (Expression)f.getValue(env);
			if ((val != null) && !val.equals(0)) {
			    FieldExpression fe = new FieldExpression(f.getWhere(),
					new ThisExpression(f.getWhere(), ctx), f.getName());
			    fe.field = f;
			    val = new AssignExpression(f.getWhere(), fe, val.copyInline(ctx));
			    val = val.inline(env, ctx);
			    if (val != null) {
				e = new CommaExpression(f.getWhere(), e, val);
			    }
			}
		    }
		}
	    }
	    return e;

	} catch (ClassNotFound e) {
	    throw new CompilerError(e);
	}
    }
    public Expression inlineValue(Environment env, Context ctx) {
	try {
	    if (right != null) {
		right = field.isStatic() ? right.inline(env, ctx) : right.inlineValue(env, ctx);
	    }
	    for (int i = 0 ; i < args.length ; i++) {
		args[i] = args[i].inlineValue(env, ctx);
	    }
	    if (env.optimize() && field.isInlineable(env, clazz.isFinal()) &&
		(!ctx.field.isInitializer()) && ctx.field.isMethod() &&
		(ctx.getInlineFieldContext(field) == null)) {
		Statement s = (Statement)field.getValue(env);
		if ((s == null) || (s.costInline(MAXINLINECOST) < MAXINLINECOST))  {
		    return inlineMethod(env, ctx, s, true);
		}
	    }
	    return this;
	} catch (ClassNotFound e) {
	    throw new CompilerError(e);
	}
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	if (field.isStatic()) {
	    if (right != null) {
		right.code(env, ctx, asm);
	    }
	} else if (right == null) {
	    asm.add(where, opc_aload, new Integer(0));
	} else {
	    right.codeValue(env, ctx, asm);
	}

	for (int i = 0 ; i < args.length ; i++) {
	    args[i].codeValue(env, ctx, asm);
	}

	if (field.isStatic()) {
	    asm.add(where, opc_invokestatic, field);
	} else if (field.isConstructor() || field.isPrivate() || ((right != null) && (right.op == SUPER))) {
	    asm.add(where, opc_invokenonvirtual, field);
	} else if (field.getClassDefinition().isInterface()) {
	    asm.add(where, opc_invokeinterface, field);
	} else {
	    asm.add(where, opc_invokevirtual, field);
	}
    }

    /**
     * Check if the first thing is a constructor invocation
     */
    public boolean firstConstructor() {
	return id.equals(idInit);
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(" + opNames[op]);
	if (right != null) {
	    out.print(" ");
	    right.print(out);
	} 
	out.print(" " + ((id == null) ? idInit : id)); 
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
