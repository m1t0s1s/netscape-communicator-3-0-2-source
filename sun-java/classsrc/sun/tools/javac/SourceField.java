/*
 * @(#)SourceField.java	1.38 95/11/21 Arthur van Hoff
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

package sun.tools.javac;

import sun.tools.java.*;
import sun.tools.tree.*;
import sun.tools.asm.*;
import java.util.Vector;
import java.util.Enumeration;
import java.util.Hashtable;
import java.io.PrintStream;

/**
 * A Source Field
 */

public final
class SourceField extends FieldDefinition implements Constants {
    /**
     * The argument names (if it is a method)
     */
    Vector args;

    // set to the FieldDefinition in the interface if we have this field because
    // it has been forced on us 
    FieldDefinition abstractSource;

    /**
     * The status of the field
     */
    int status;

    static final int PARSED	= 0;
    static final int CHECKING  	= 1;
    static final int CHECKED	= 2;
    static final int INLINING	= 3;
    static final int INLINED	= 4;
    static final int ERROR	= 5;

    public Vector getArguments() {
	return args;
    }

    /**
     * Constructor
     */
    public SourceField(int where, ClassDefinition clazz, String doc, int modifiers, Type type,
		       Identifier name, Vector argNames, ClassDeclaration exp[], Node value) {
	super(where, clazz, modifiers, type, name, exp, value);
	this.documentation = doc;

	// Create a list of arguments
	if (isMethod()) {
	    args = new Vector();

	    if (isConstructor() || !isStatic()) {
		args.addElement(new LocalField(where, clazz, 0, clazz.getType(), idThis));
	    }

	    if (argNames != null) {
		Enumeration e = argNames.elements();
		Type argTypes[] = getType().getArgumentTypes();
		for (int i = 0 ; i < argTypes.length ; i++) {
		    args.addElement(new LocalField(where, clazz, 0, argTypes[i],
						   (Identifier)e.nextElement()));
		}
	    }
	}
    }

    /**
     * Constructor.
     * Used only to generate an abstract copy of a method that a class
     * inherits from an interface 
     */
    public SourceField(FieldDefinition f, ClassDefinition c, Environment env) { 
	this(f.getWhere(), c, f.getDocumentation(), 
	     f.getModifiers() | M_ABSTRACT, f.getType(), f.getName(), null, 
	     f.getExceptions(env), null);
	this.args = f.getArguments();
	this.abstractSource = f;
    }

    /**
     * Get the class declaration in which the field is actually defined
     */
    public final ClassDeclaration getDefiningClassDeclaration() {
	if (abstractSource == null)
	    return super.getDefiningClassDeclaration();
	else
	    return abstractSource.getDefiningClassDeclaration();
    }

    /**
     * Resolve a class name
     */
    public Identifier resolve(Environment env, Identifier nm) throws ClassNotFound {
	return ((SourceClass)getClassDefinition()).imports.resolve(env, nm);
    }

    /**
     * Check this field
     */
    public void check(Environment env) throws ClassNotFound {
	if (status == PARSED) {
	    if (env.dump()) {
		System.out.println("[check field " + getClassDeclaration().getName() + "." + getName() + "]");
		if (getValue() != null) {
		    getValue().print(System.out);
		    System.out.println();
		}
	    }
	    env = new Environment(env, this);
	    env.resolve(where, getClassDefinition(), getType());

	    // make sure that all the classes that we claim to throw really
	    // are subclasses of Throwable, and are classes that we can reach
	    if (isMethod()) {
		ClassDeclaration throwable = 
		    env.getClassDeclaration(idJavaLangThrowable);
		ClassDeclaration exp[] = getExceptions(env);
		for (int i = 0 ; i < exp.length ; i++) {
		    ClassDefinition def;
		    try {
			def = exp[i].getClassDefinition(env);
		    } catch (ClassNotFound e) {
			env.error(where, "class.not.found", e.name, "throws");
			break;
		    }
		    if (!getClassDefinition().
			  canAccess(env, def.getClassDeclaration())) {
			env.error(where, "cant.access.class", def);
		    } else if (!def.subClassOf(env, throwable)) {
			env.error(where, "throws.not.throwable", def);
		    }
		}
	    }
	    
	    status = CHECKING;

	    if (isMethod() && args != null) {
		int length = args.size();
	    outer_loop:
		for (int i = 0; i < length; i++) { 
		    LocalField lf = (LocalField)(args.elementAt(i));
		    Identifier name_i = lf.getName();
		    for (int j = i + 1; j < length; j++) {
			Identifier name_j = 
			      ((LocalField)(args.elementAt(j))).getName();
			if (name_i.equals(name_j)) {
			    env.error(getWhere(), "duplicate.argument", 
				      name_i);
			    break outer_loop;
			}
		    }
		}
	    }
	    if (getValue() != null) {
		Context ctx = new Context(this);
		if (isMethod()) {
		    Statement s = (Statement)getValue();
		    long vset = 0;
		    // initialize vset, indication that each of the arguments
		    // to the function has a value
		    
		    for (Enumeration e = args.elements(); e.hasMoreElements();){
			LocalField f = (LocalField)e.nextElement();
			vset |= (long)1 << ctx.declare(env, f);
		    }

		    if (isConstructor()) {
			// undefine "this" in some constructors, until after the
			// super constructor has been called.
			vset &= ~1;
			// If the first thing in the definition isn't a call
			// to either super() or this(), then insert one.
			if ((!s.firstConstructor()) 
			    && (getClassDefinition().getSuperClass() != null)) {
			    Statement body[] = {
				new ExpressionStatement(where,
				    new MethodExpression(where, 
						    new SuperExpression(where),
							 idInit, new Expression[0])), s
			    };
			    s = new CompoundStatement(where, body);
			    setValue(s);
			}
		    }

		    //System.out.println("VSET = " + vset);
		    Hashtable thrown = new Hashtable();
		    s.checkMethod(env, ctx, vset, thrown);
		    ClassDeclaration exp[] = 
			     getExceptions(env);
		    ClassDeclaration ignore1 = 
			     env.getClassDeclaration(idJavaLangError);
		    ClassDeclaration ignore2 = 
			    env.getClassDeclaration(idJavaLangRuntimeException);

		    for (Enumeration e = thrown.keys(); e.hasMoreElements();) {	
			ClassDeclaration c = (ClassDeclaration)e.nextElement();
			ClassDefinition def = c.getClassDefinition(env);
			if (def.subClassOf(env, ignore1) 
			         || def.subClassOf(env, ignore2)) {
			    continue;
			}

			boolean ok = false;
			if (!isInitializer()) {
			    for (int i = 0 ; i < exp.length ; i++) {
				if (def.subClassOf(env, exp[i])) {
				    ok = true;
				}
			    }
			}
			if (!ok) {
			    Node n = (Node)thrown.get(c);
			    env.error(n.getWhere(), isInitializer() ? "initializer.exception" : "uncaught.exception", c.getName());
			}
		    }
		} else {
		    Hashtable thrown = new Hashtable();
		    Expression val = (Expression)getValue();
		    val.checkInitializer(env, ctx, isStatic() ? 0 : 1, 
					 getType(), thrown);
		    setValue(val.convert(env, ctx, getType(), val));
		    ClassDeclaration except = 
			 env.getClassDeclaration(idJavaLangThrowable);
		    ClassDeclaration ignore = 
			env.getClassDeclaration(idJavaLangRuntimeException);

		    for (Enumeration e = thrown.keys(); e.hasMoreElements(); ) {
			ClassDeclaration c = (ClassDeclaration)e.nextElement();
			ClassDefinition def = c.getClassDefinition(env);

			if (!def.subClassOf(env, ignore) 
			    && def.subClassOf(env, except)) { 
   			    Node n = (Node)thrown.get(c);
			    env.error(n.getWhere(), 
				      "initializer.exception", c.getName());
			}
		    }
		}
		if (env.dump()) {
		    getValue().print(System.out);
		    System.out.println();
		}
	    }
	    status = getClassDefinition().getError() ? ERROR : CHECKED;
	}
    }

    /**
     * Inline the field
     */
    void inline(Environment env) throws ClassNotFound {
	switch (status) {
	  case PARSED:
	    check(env);
	    inline(env);
	    break;

	  case CHECKED:
	    if (env.dump()) {
		System.out.println("[inline field " + getClassDeclaration().getName() + "." + getName() + "]");
	    }
	    status = INLINING;
	    env = new Environment(env, this);

	    if (isMethod()) {
		if ((!isNative()) && (!isAbstract())) {
		    Context ctx = new Context(this);
		    for (Enumeration e = args.elements() ; e.hasMoreElements() ; ) {
			ctx.declare(env, (LocalField)e.nextElement());
		    }
		    setValue(((Statement)getValue()).inline(env, ctx));
		}
	    } else {
		if (getValue() != null)  {
		    Context ctx = new Context(this);
		    setValue(((Expression)getValue()).inlineValue(env, ctx));
		}
	    }
	    if (env.dump()) {
		System.out.println("[inlined field " + getClassDeclaration().getName() + "." + getName() + "]");
		if (getValue() != null) {
		    getValue().print(System.out);
		    System.out.println();
		} else {
		    System.out.println("<empty>");
		}
	    }
	    status = INLINED;
	    break;
	}
    }

    /**
     * Get the value of the field (or null if the value can't be determined)
     */
    public Node getValue(Environment env) throws ClassNotFound {
	if (getValue() == null) {
	    return null;
	}
	switch (status) {
	  case PARSED:
	    check(env);
	    return getValue(env);

	  case CHECKED:
	    inline(env);
	    return getValue(env);

	  case INLINED:
	    return getValue();
	}
	return null;
    }

    public boolean isInlineable(Environment env, boolean fromFinal) throws ClassNotFound {
	if (super.isInlineable(env, fromFinal)) {
	    getValue(env);
	    return (status == INLINED) && !getClassDefinition().getError();
	}
	return false;
    }
	

    /**
     * Get the initial value of the field
     */
    public Object getInitialValue() {
	if (isMethod() || (getValue() == null) || (!isFinal()) || (status != INLINED)) {
	    return null;
	}
	return ((Expression)getValue()).getValue();
    }

    /**
     * Generate code
     */
    public void code(Environment env, Assembler asm) throws ClassNotFound {
	switch (status) {
	  case PARSED:
	    check(env);
	    code(env, asm);
	    return;

	  case CHECKED:
	    inline(env);
	    code(env, asm);
	    return;

	  case INLINED:
	    // Actually generate code
	    if (env.dump()) {
		System.out.println("[code field " + getClassDeclaration().getName() + "." + getName() + "]");
	    }
	    if (isMethod() && (!isNative()) && (!isAbstract())) {
		env = new Environment(env, this);
		Context ctx = new Context(this);
		Statement s = (Statement)getValue();

		for (Enumeration e = args.elements() ; e.hasMoreElements() ; ) {
		    LocalField f = (LocalField)e.nextElement();
		    ctx.declare(env, f);
		    //ctx.declare(env, (LocalField)e.nextElement());
		}

		/*
		if (isConstructor() && ((s == null) || !s.firstConstructor())) {
		    ClassDeclaration c = getClassDefinition().getSuperClass();
		    if (c != null) {
			FieldDefinition field = c.getClassDefinition(env).matchMethod(env, getClassDefinition(), idInit);
			asm.add(getWhere(), opc_aload, new Integer(0));
			asm.add(getWhere(), opc_invokenonvirtual, field);
			asm.add(getWhere(), opc_pop);
		    }

		    // Ouput initialization code
		    for (FieldDefinition f = getClassDefinition().getFirstField() ; f != null ; f = f.getNextField()) {
			if (!f.isStatic()) {
			    f.codeInit(env, ctx, asm);
			}
		    }
		}
		*/
		if (s != null) {
		    s.code(env, ctx, asm);
		}
		if (getType().getReturnType().isType(TC_VOID) && !isInitializer()) {
		    asm.add(getWhere(), opc_return);
		}
	    }
	    return;
	}
    }

    public void codeInit(Environment env, Context ctx, Assembler asm) throws ClassNotFound {
	if (isMethod()) {
	    return;
	}
	switch (status) {
	  case PARSED:
	    check(env);
	    codeInit(env, ctx, asm);
	    return;

	  case CHECKED:
	    inline(env);
	    codeInit(env, ctx, asm);
	    return;

	  case INLINED:
	    // Actually generate code
	    if (env.dump()) {
		System.out.println("[code initializer  " + getClassDeclaration().getName() + "." + getName() + "]");
	    }
	    if (getValue() != null) {
		Expression e = (Expression)getValue();
		if (isStatic()) {
		    if ((getInitialValue() == null) && !e.equalsDefault()) {
			e.codeValue(env, ctx, asm);
			asm.add(getWhere(), opc_putstatic, this);
		    }
		} else if (!e.equalsDefault()) {
		    asm.add(getWhere(), opc_aload, new Integer(0));
		    e.codeValue(env, ctx, asm);
		    asm.add(getWhere(), opc_putfield, this);
		}
	    }
	    return;
	}
    }

    /**
     * Print for debugging
     */
    public void print(PrintStream out) {
	super.print(out);
	if (getValue() != null) {
	    getValue().print(out);
	    out.println();
	}
    }
}
