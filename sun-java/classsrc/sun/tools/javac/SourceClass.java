/*
 * @(#)SourceClass.java	1.54 95/12/07 Arthur van Hoff
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
import sun.tools.tree.CompoundStatement;
import sun.tools.asm.Assembler;
import sun.tools.asm.ConstantPool;
import java.util.Vector;
import java.util.Enumeration;
import java.io.IOException;
import java.io.OutputStream;
import java.io.DataOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;

/**
 * This class represents an Java class as it is read from
 * an Java source file.
 */

public final
class SourceClass extends ClassDefinition {
    /**
     * The classes and packages imported by this class
     */
    Imports imports;

    /**
     * The default constructor
     */
    SourceField defConstructor;

    /**
     * The constant pool
     */
    ConstantPool tab = new ConstantPool();

    /**
     * Constructor
     */
    public SourceClass(Environment env, int where, ClassDeclaration declaration, String documentation,
		       int modifiers, ClassDeclaration superClass, ClassDeclaration interfaces[], 
		       Imports imports) {
	super(env.getSource(), where, declaration, modifiers, superClass, interfaces);
	this.imports = imports;
	this.documentation = documentation;
    }

    /**
     * Return imports
     */
    public final Imports getImports() {
	return imports;
    }

    /**
     * Add a dependency
     */
    public void addDependency(ClassDeclaration c) {
	if (tab != null) {
	    tab.put(c);
	}
    }

    /**
     * Check method override
     */
    void checkOverride(Environment env, 
		       ClassDeclaration clazz, 
		       FieldDefinition field) throws ClassNotFound {
        ClassDefinition c = clazz.getClassDefinition(env);
	for (FieldDefinition f = c.getFirstMatch(field.getName()); f != null;
	              f = f.getNextMatch()) {
	    if (f.isPrivate() || f.isVariable() || 
		      !field.getType().equalArguments(f.getType())) {
		continue;
	    }
	    if (f.isFinal()) {
		env.error(field.getWhere(), "final.meth.override", 
			  field, f.getClassDeclaration());
		return;
	    }
	    Type t1 = field.getType().getReturnType();
	    Type t2 = f.getType().getReturnType();
	    if (!t1.equals(t2)) {
		// We used to have 
		//  && (!t1.isType(TC_CLASS)) 
		//  && (!t2.isType(TC_CLASS)) 
		//  && (!env.getClassDefinition(t2)
		//         .implementedBy(env, env.getClassDeclaration(t1)))
		env.error(field.getWhere(), "redef.return.type", field, f);
		return;
	    }
	    if (field.isStatic() != f.isStatic()) {
		String errName = 
		    f.isStatic() ? "override.static.meth" 
		                 : "override.instance.method.static";
		env.error(field.getWhere(), errName, 
			  f, f.getClassDeclaration());
		return;
	    }
	    if (field.isConstructor()) {
		continue;
	    }
	    if ((!field.isPublic()) && (!f.isPrivate())) {
		if (f.isPublic()) {
		    env.error(field.getWhere(), "override.public", 
			      f, f.getClassDeclaration());
		    return;
		}
		if (f.isProtected() && !field.isProtected()) {
		    env.error(field.getWhere(), 
			      "override.protected", f, f.getClassDeclaration());
		    return;
		}
		if (field.isPrivate()) {
		    env.error(field.getWhere(), 
			      "override.private", f, f.getClassDeclaration());
		    return;
		}
	    }
	    ClassDeclaration e1[] = field.getExceptions(env);
	    ClassDeclaration e2[] = f.getExceptions(env);
	    for (int i = 0 ; i < e1.length ; i++) {
		ClassDefinition c1 = e1[i].getClassDefinition(env);
		boolean ok = false;
		for (int j = 0 ; j < e2.length ; j++) {
		    if (c1.subClassOf(env, e2[j])) {
			ok = true;
			break;
		    }
		}
		if (!ok) {
		    env.error(field.getWhere(), "invalid.throws", c1, f, f.getClassDeclaration());
		    return;
		}
	    }
	}

	// check the super class
    if (c.getSuperClass() != null) {
	    checkOverride(env, c.getSuperClass(), field);
	}

	// check the interfaces
	ClassDeclaration intf[] = c.getInterfaces();
	for (int i = 0 ; i < intf.length ; i++) {
	    checkOverride(env, intf[i], field);
	}
    }

    /**
     * Add a field (check it first)
     */
    public void addField(Environment env, FieldDefinition f) {
	if (f.isMethod()) {
	    if (f.isConstructor()) {
		if (f.getClassDefinition().isInterface()) {
		    env.error(f.getWhere(), "intf.constructor");
		    return;
		}
		if (f.isNative() || f.isAbstract() || f.isStatic() || f.isSynchronized() || f.isFinal()) {
		    env.error(f.getWhere(), "constr.modifier", f);
		    f.subModifiers(M_NATIVE | M_ABSTRACT | M_STATIC | M_SYNCHRONIZED | M_FINAL);
		}
	    } else if (f.isInitializer()) {
		if (f.getClassDefinition().isInterface()) {
		    env.error(f.getWhere(), "intf.initializer");
		    return;
		}
	    }
	    if (f.getClassDefinition().isInterface() && 
		(f.isStatic() || f.isSynchronized() || f.isNative() 
		 || f.isFinal() || f.isPrivate() || f.isProtected())) {
		env.error(f.getWhere(), "intf.modifier.method", f);
		f.subModifiers(M_STATIC |  M_SYNCHRONIZED | M_NATIVE |
			       M_FINAL | M_PRIVATE);
	    }
	    if (f.isTransient()) {
		env.error(f.getWhere(), "transient.meth", f);
		f.subModifiers(M_TRANSIENT);
	    }
	    if (f.isVolatile()) {
		env.error(f.getWhere(), "volatile.meth", f);
		f.subModifiers(M_VOLATILE);
	    }
	    if (f.isStatic() && f.isAbstract()) {
		env.error(f.getWhere(), "static.modifier", f);
		f.subModifiers(M_STATIC);
	    }
	    if (f.isAbstract() || f.isNative()) {
		if (f.getValue() != null) {
		    env.error(f.getWhere(), "invalid.meth.body", f);
		    f.setValue(null);
		}
	    } else {
		if (f.getValue() == null) {
		    if (f.isConstructor()) {
			env.error(f.getWhere(), "no.constructor.body", f);
		    } else { 
			env.error(f.getWhere(), "no.meth.body", f);
		    }
		    f.addModifiers(M_ABSTRACT);
		}
	    }
            Vector arguments = f.getArguments();
	    if (arguments != null) { 
		// arguments can be null if this is an implicit abstract method
		int argumentLength = arguments.size();
		for (int i = 0; i < argumentLength; i++) {
		    FieldDefinition arg = 
			(FieldDefinition)arguments.elementAt(i);
		    if (arg.getType().isType(TC_VOID)) {
			env.error(arg.getWhere(), "void.argument", arg);
		    }
		}
	    }
	} else {
	    if (f.getType().isType(TC_VOID)) {
		env.error(f.getWhere(), "void.inst.var", f.getName());
		// REMIND: set type to error
		return;
	    }
	    if (f.isSynchronized() || f.isAbstract() || f.isNative()) {
		env.error(f.getWhere(), "var.modifier", f);
		f.subModifiers(M_SYNCHRONIZED | M_ABSTRACT | M_NATIVE);
	    }
	    if (f.isTransient() && (f.isFinal() || f.isStatic())) {
		env.error(f.getWhere(), "transient.modifier", f);
		f.subModifiers(M_FINAL | M_STATIC);
	    }
	    if (f.isFinal() && (f.getValue() == null)) {
		env.error(f.getWhere(), "initializer.needed", f);
		f.subModifiers(M_FINAL);
	    }
	    if (f.getClassDefinition().isInterface() &&
		  (f.isPrivate() || f.isProtected())) {
		env.error(f.getWhere(), "intf.modifier.field", f);
		f.subModifiers(M_PRIVATE);
	    }
	}
	if (!f.isInitializer()) {
	    for (FieldDefinition f2 = getFirstMatch(f.getName()) ; f2 != null ; f2 = f2.getNextMatch()) {
		if (f.isMethod()) {
		    if (f.getType().equals(f2.getType())) {
			env.error(f.getWhere(), "meth.multidef", f);
			return;
		    }
		    if (f.getType().equalArguments(f2.getType())) {
			env.error(f.getWhere(), "meth.redef.rettype", f, f2);
			return;
		    }
		} else {
		    env.error(f.getWhere(), "var.multidef", f, f2);
		    return;
		}
	    }
	}

	addField(f);
    }

    private void 
    addAbstractMethodsFromInterfaces(Environment env, ClassDefinition c) 
	     throws ClassNotFound { 
	FieldDefinition f;
	if (c.isInterface()) {
	    // "this" implements c.  Look at each of the methods in c and
	    // and ensure that this method implements them.
	    for (f = c.getFirstField(); f != null ; f = f.getNextField()) {
		if (f.isMethod() && f.isAbstract() && 
		        (findMethod(env, f.getName(), f.getType()) == null)) {
		    // No, we don't implement it.  Implicitly create the
		    // abstract method for this guy.
		    SourceField fieldCopy = new SourceField(f, this, env);
		    // System.out.println("Adding " + fieldCopy + " to class " + this);
		    addField(env, fieldCopy);
		}
	    }
	}

	// we don't need to look at the superclass.  It must already have
	// declared the appropriate methods abstract within itself.
	// ClassDeclaration sup = def.getSuperClass();
	// if (sup != null) 
	//   addAbstractMethodsFromInterfaces(env, sup.getClassDefinition(env));
	
	ClassDeclaration interfaces[] = c.getInterfaces();
	for (int i = 0 ; i < interfaces.length ; i++) {
	    ClassDefinition intDef = interfaces[i].getClassDefinition(env);
	    addAbstractMethodsFromInterfaces(env, intDef);
	}
    }


    /**
     * Check this class.
     */
    public void check(Environment env) throws ClassNotFound {
	Identifier nm = getClassDeclaration().getName();
	if (env.verbose()) {
	    env.output("[checking class " + nm + "]");
	}

	basicCheck(env);

	// Make sure it was defined in the right file
	if (isPublic()) {
	    String fname = getName().getName() + ".java";
	    String src = ((ClassFile)getSource()).getName();
	    if (env.warnings() && !src.equals(fname)) {
		env.error(getWhere(), "public.class.file", this, fname);
	    }
	}

	if (isInterface()) {
	    if (isFinal()) {
		env.error(getWhere(), "final.intf", nm);
	    }
	} else {
	    // Check super class
	    if (getSuperClass() != null) {
		try {
		    ClassDefinition def = getSuperClass().getClassDefinition(env);
		    if (!canAccess(env, getSuperClass())) {
			env.error(getWhere(), "cant.access.class", getSuperClass());
			superClass = null;
		    } else if (def.isFinal()) {
			env.error(getWhere(), "super.is.final", getSuperClass());
			superClass = null;
		    } else if (def.isInterface()) {
			env.error(getWhere(), "super.is.intf", getSuperClass());
			superClass = null;
		    } else if (superClassOf(env, getSuperClass())) {
			env.error(getWhere(), "cyclic.super");
			superClass = null;
		    }
		} catch (ClassNotFound e) {
		    env.error(getWhere(), "super.not.found", e.name, this);
		    superClass = null;
		}
	    }
	}

	// Check interfaces
	for (int i = 0 ; i < interfaces.length ; i++) {
	    ClassDeclaration intf = interfaces[i];
	    try {
		if (!canAccess(env, intf)) {
		    env.error(getWhere(), "cant.access.class", intf);
		} else if (!intf.getClassDefinition(env).isInterface()) {
		    env.error(getWhere(), "not.intf", intf);
		} else if (isInterface() && implementedBy(env, intf)) {
		    env.error(getWhere(), "cyclic.intf", intf);
		}
	    } catch (ClassNotFound ee) {
		env.error(getWhere(), "intf.not.found", ee.name, this);
	    }
	}

	// bail out if there were any errors
	if (getError()) {
	    return;
	}

	try {
	    // Check override
	    for (FieldDefinition f = getFirstField() ; f != null ; f = f.getNextField()) {
		if (f.isVariable()) {
		    continue;
		}
		
		// check the super class
		if (getSuperClass() != null) {
		    checkOverride(env, getSuperClass(), f);
		}

		// check the interfaces
		for (int i = 0 ; i < interfaces.length ; i++) {
		    checkOverride(env, interfaces[i], f);
		}
	    }
	} catch (ClassNotFound ee) {
	    env.error(getWhere(), "class.not.found", ee.name, this);
	}

	// bail out if there were any errors
	if (getError()) {
	    return;
	}

	if (isFinal() && isAbstract()) {
	    env.error(where, "final.abstract", this.getName().getName());
	}

	// Check if abstract
	if ((!isInterface()) && (!isAbstract()) && isAbstract(env)) {
	    modifiers |= M_ABSTRACT;
	    String errName = 
		  isFinal() ? "abstract.class.not.final" : "abstract.class";
	    Enumeration e = getAbstractFields(env);
	    while (e.hasMoreElements()) { 
		FieldDefinition f = (FieldDefinition)e.nextElement();
		env.error(where, errName, this, 
			  f, f.getDefiningClassDeclaration());
	    }
	}

	// Check field definitions
	for (FieldDefinition f = getFirstField() ; f != null ; f = f.getNextField()) {
	    try {
		f.check(env);
	    } catch (ClassNotFound ee) {
		env.error(f.getWhere(), "class.not.found", ee.name, this);
	    }
	}

	// bail out if there were any errors
	if (getError()) {
	    return;
	}
    }

    /**
     * Check this class has its superclass and its interfaces.  Also
     * force it to have an <init> method (if it doesn't already have one)
     * and to have all the abstract methods of its parents.
     */
    private boolean basicChecking = false;
    private boolean basicCheckDone = false;
    protected void basicCheck(Environment env) throws ClassNotFound {
	if (basicChecking || basicCheckDone) 
	    return;
	basicChecking = true;

	env = new Environment(env, this);

	// check the existence of the superclass
	if (getSuperClass() != null) {
	    try {
		getSuperClass().getClassDefinition(env);
	    } catch (ClassNotFound e) {
		env.error(getWhere(), "super.not.found", e.name, this);
		superClass = null;
		setError(true);
	    }
	}
	
	// Check the existence of all the interfaces
	for (int i = 0 ; i < interfaces.length ; i++) {
	    try { 
		interfaces[i].getClassDefinition(env);
	    } catch (ClassNotFound e) {	
		env.error(getWhere(), "intf.not.found", e.name, this);
		ClassDeclaration newInterfaces[] = 
		    new ClassDeclaration[interfaces.length - 1];
		System.arraycopy(interfaces, 0, newInterfaces, 0, i);
		System.arraycopy(interfaces, i + 1, newInterfaces, i, 
				 newInterfaces.length - i);
		interfaces = newInterfaces;
		setError(true);
	    }
	}

	if (!isInterface()) {
	    // add implicit <init> method, if necessary
	    if (getFirstMatch(idInit) == null) {
		Node code = new CompoundStatement(getWhere(), new Statement[0]);
		Type t = Type.tMethod(Type.tVoid);
	        SourceField f = 
		    new SourceField(getWhere(), this, null, 
				    isPublic() ? M_PUBLIC : 0,
				    t, idInit, 
				    null, new ClassDeclaration[0], code);
		addField(f);
		
	    }
	    // add implicit abstract methods, if necessary
	    if (!getError()) 
		addAbstractMethodsFromInterfaces(env, this);
	}
	basicChecking = false;
	basicCheckDone = true;
    }




    /**
     * A list of active ongoing compilations. This list
     * is used to stop two compilations from saving the
     * same class.
     */
    private static Vector active = new Vector();

    /**
     * Compile this class
     */
    public void compile(Environment env, OutputStream out) throws InterruptedException, IOException {
	synchronized (active) {
	    while (active.contains(getName())) {
		active.wait();
	    }
	    active.addElement(getName());
	}

	try {
	    compileClass(env, out);
	} catch (ClassNotFound e) {
	    throw new CompilerError(e);
	} finally {
	    synchronized (active) {
		active.removeElement(getName());
		active.notifyAll();
	    }
	}
    }

    private void compileClass(Environment env, OutputStream out) throws IOException, ClassNotFound {
	env = new Environment(env, this);
	check(env);

	// bail out if there were any errors
	if (getError()) {
	    return;
	}

	Vector variables = new Vector();
	Vector methods = new Vector();
	CompilerField init = new CompilerField(new FieldDefinition(getWhere(), this, M_STATIC, Type.tMethod(Type.tVoid), idClassInit, new ClassDeclaration[0], null), new Assembler());
	Context ctx = new Context(init.field);

	// System.out.println("compile class " + getName());

	// Generate code for all fields
	for (FieldDefinition field = getFirstField() ; field != null ; field = field.getNextField()) {
	    //System.out.println("compile field " + field.getName());

	    try {
		if (field.isMethod()) {
		    if (field.isInitializer()) {
			((SourceField)field).code(env, init.asm);
		    } else {
			CompilerField f = new CompilerField(field, new Assembler());
			((SourceField)field).code(env, f.asm);
			methods.addElement(f);
		    }
		} else {
		    CompilerField f = new CompilerField(field, null);
		    variables.addElement(f);
		    if (field.isStatic()) {
			field.codeInit(env, ctx, init.asm);
		    }
		}
	    } catch (CompilerError ee) {
		ee.printStackTrace();
		env.error(field, 0, "generic", field.getClassDeclaration() + ":" + field + "@" + ee.toString(), null, null);
	    }
	}
	if (!init.asm.empty()) {
	    init.asm.add(getWhere(), opc_return);
	    methods.addElement(init);
	}

	// bail out if there were any errors
	if (getError()) {
	    return;
	}

	// Insert constants
	tab.put("Code");
	tab.put("ConstantValue");
	tab.put("LocalVariables");
	tab.put("SourceFile");
	tab.put("Exceptions");
	if (!env.optimize()) {
	    tab.put("LineNumberTable");
	}
	if (env.debug()) {
	    tab.put("LocalVariableTable");
	}

	String sourceFile = ((ClassFile)getSource()).getName();
	tab.put(sourceFile);
	tab.put(getClassDeclaration());
	if (getSuperClass() != null) {
	    tab.put(getSuperClass());
	}
	for (int i = 0 ; i < interfaces.length ; i++) {
	    tab.put(interfaces[i]);
	}

	// Optimize Code and Collect method constants
	for (Enumeration e = methods.elements() ; e.hasMoreElements() ; ) {
	    CompilerField f = (CompilerField)e.nextElement();
	    try {
		f.asm.optimize(env);
		f.asm.collect(env, f.field, tab);
		tab.put(f.name);
		tab.put(f.sig);
		ClassDeclaration exp[] = f.field.getExceptions(env);
		for (int i = 0 ; i < exp.length ; i++) {
		    tab.put(exp[i]);
		}
	    } catch (Exception ee) {
		ee.printStackTrace();
		env.error(f.field, -1, "generic", f.field.getName() + "@" + ee.toString(), null, null);
		f.asm.listing(System.out);
	    }
	}

	// Collect field constants
	for (Enumeration e = variables.elements() ; e.hasMoreElements() ; ) {
	    CompilerField f = (CompilerField)e.nextElement();
	    tab.put(f.name);
	    tab.put(f.sig);

	    Object val = f.field.getInitialValue();
	    if (val != null) {
		tab.put((val instanceof String) ? new StringExpression(f.field.getWhere(), (String)val) : val);
	    }
	}

	// Write header
	DataOutputStream data = new DataOutputStream(out);
	data.writeInt(JAVA_MAGIC);
	data.writeShort(JAVA_MINOR_VERSION);
	data.writeShort(JAVA_VERSION);
	tab.write(env, data);

	// Write class information
	data.writeShort(getModifiers() & MM_CLASS);
	data.writeShort(tab.index(getClassDeclaration()));
	data.writeShort((getSuperClass() != null) ? tab.index(getSuperClass()) : 0);
	data.writeShort(interfaces.length);
	for (int i = 0 ; i < interfaces.length ; i++) {
	    data.writeShort(tab.index(interfaces[i]));
	}

	// write variables
	ByteArrayOutputStream buf = new ByteArrayOutputStream(256);
	ByteArrayOutputStream attbuf = new ByteArrayOutputStream(256);
	DataOutputStream databuf = new DataOutputStream(buf);

	data.writeShort(variables.size());
	for (Enumeration e = variables.elements() ; e.hasMoreElements() ; ) {
	    CompilerField f = (CompilerField)e.nextElement();
	    Object val = f.field.getInitialValue();

	    data.writeShort(f.field.getModifiers() & MM_FIELD);
	    data.writeShort(tab.index(f.name));
	    data.writeShort(tab.index(f.sig));

	    if (val != null) {
		data.writeShort(1);
		data.writeShort(tab.index("ConstantValue"));
		data.writeInt(2);
		data.writeShort(tab.index((val instanceof String) ? new StringExpression(f.field.getWhere(), (String)val) : val));
	    } else {
		data.writeShort(0);
	    }
	}

	// write methods

	data.writeShort(methods.size());
	for (Enumeration e = methods.elements() ; e.hasMoreElements() ; ) {
	    CompilerField f = (CompilerField)e.nextElement();
	    data.writeShort(f.field.getModifiers() & MM_METHOD);
	    data.writeShort(tab.index(f.name));
	    data.writeShort(tab.index(f.sig));
	    ClassDeclaration exp[] = f.field.getExceptions(env);

	    if (!f.asm.empty()) {
		data.writeShort((exp.length > 0) ? 2 : 1);
		f.asm.write(env, databuf, f.field, tab);
		int natts = 0;
		if (!env.optimize()) {
		    natts++;
		}
		if (env.debug()) {
		    natts++;
		}
		databuf.writeShort(natts);

		if (!env.optimize()) {
		    f.asm.writeLineNumberTable(env, new DataOutputStream(attbuf), tab);
		    databuf.writeShort(tab.index("LineNumberTable"));
		    databuf.writeInt(attbuf.size());
		    attbuf.writeTo(buf);
		    attbuf.reset();
		}

		if (env.debug()) {
		    f.asm.writeLocalVariableTable(env, f.field, new DataOutputStream(attbuf), tab);
		    databuf.writeShort(tab.index("LocalVariableTable"));
		    databuf.writeInt(attbuf.size());
		    attbuf.writeTo(buf);
		    attbuf.reset();
		}

		data.writeShort(tab.index("Code"));
		data.writeInt(buf.size());
		buf.writeTo(data);
		buf.reset();
	    } else {
		data.writeShort((exp.length > 0) ? 1 : 0);
	    }

	    if (exp.length > 0) {
		data.writeShort(tab.index("Exceptions"));
		data.writeInt(2 + exp.length * 2);
		data.writeShort(exp.length);
		for (int i = 0 ; i < exp.length ; i++) {
		    data.writeShort(tab.index(exp[i]));
		}
	    }
	}

	// class attributes
	data.writeShort(1);
	data.writeShort(tab.index("SourceFile"));
	data.writeInt(2);
	data.writeShort(tab.index(sourceFile));

	// Cleanup
	data.flush();
	tab = null;
    }
}
