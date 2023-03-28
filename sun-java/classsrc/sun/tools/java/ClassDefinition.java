/*
 * @(#)ClassDefinition.java	1.32 95/11/29 Arthur van Hoff
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

package sun.tools.java;

import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;
import java.io.OutputStream;
import java.io.PrintStream;

/**
 * This class is an Java class definition
 */
public
class ClassDefinition implements Constants {
    protected Object source;
    protected int where;
    protected int modifiers;
    protected ClassDeclaration declaration;
    protected ClassDeclaration superClass;
    protected ClassDeclaration interfaces[];
    protected FieldDefinition firstField;
    protected FieldDefinition lastField;
    protected String documentation;
    protected boolean error;
    private Hashtable fieldHash = new Hashtable(31);
    private int abstr;

    /**
     * Constructor
     */
    protected ClassDefinition(Object source, int where, ClassDeclaration declaration,
			      int modifiers, ClassDeclaration superClass, ClassDeclaration interfaces[]) {
	this.source = source;
	this.where = where;
	this.declaration = declaration;
	this.modifiers = modifiers;
	this.superClass = superClass;
	this.interfaces = interfaces;
    }

    /**
     * Get the source of the class
     */
    public final Object getSource() {
	return source;
    }

    /**
     * Check if there were any errors
     */
    public final boolean getError() {
	return error;
    }

    /**
     * Set errors
     */
    public final void setError(boolean error) {
	this.error = error;
    }

    /**
     * Get the position in the input
     */
    public final int getWhere() {
	return where;
    }
    
    /**
     * Get the class declaration
     */
    public final ClassDeclaration getClassDeclaration() {
	return declaration;
    }

    /**
     * Get the class' modifiers
     */
    public final int getModifiers() {
	return modifiers;
    }

    /**
     * Get the class' super class
     */
    public final ClassDeclaration getSuperClass() {
	return superClass;
    }

    /**
     * Get the class' interfaces
     */
    public final ClassDeclaration getInterfaces()[] {
	return interfaces;
    }

    /**
     * Get the class' first field or first match
     */
    public final FieldDefinition getFirstField() {
	return firstField;
    }
    public final FieldDefinition getFirstMatch(Identifier name) {
	return (FieldDefinition)fieldHash.get(name);
    }

    /**
     * Get the class' name
     */
    public final Identifier getName() {
	return declaration.getName();
    }

    /**
     * Get the class' type
     */
    public final Type getType() {
	return declaration.getType();
    }

    /**
     * Get the class' documentation
     */
    public String getDocumentation() {
	return documentation;
    }
    
    /**
     * Checks
     */
    public final boolean isInterface() {
	return (getModifiers() & M_INTERFACE) != 0;
    }
    public final boolean isClass() {
	return (getModifiers() & M_INTERFACE) == 0;
    }
    public final boolean isPublic() {
	return (getModifiers() & M_PUBLIC) != 0;
    }
    public final boolean isFinal() {
	return (getModifiers() & M_FINAL) != 0;
    }
    public final boolean isAbstract() {
	return (getModifiers() & M_ABSTRACT) != 0;
    }
    public final boolean isAbstract(Environment env) throws ClassNotFound {
	if (isAbstract()) {
	    return true;
	}
	if (abstr == 0) {
	    abstr = (isInterface() || hasAbstractFields(env)) ? 1 : -1;
	}
	return abstr == 1;
    }

    /**
     * Return an enumeration of all the abstract methods
     */
    public Enumeration getAbstractFields(Environment env) throws ClassNotFound {
	Vector v = new Vector();
	getAbstractFields(env, this, v);
	return v.elements();
    }

    private void getAbstractFields(Environment env, ClassDefinition c, Vector v)
	   throws ClassNotFound {
	for (FieldDefinition f = c.getFirstField() ;  
	             f != null ; f = f.getNextField()) {
	    if (f.isAbstract()) {
		FieldDefinition m = findMethod(env, f.getName(), f.getType());
		if (m == null) {
		    m = f;
		}
		if (m.isAbstract() && !v.contains(m)) {
		    v.addElement(m);
		}
	    }
	}

	// Super class
	ClassDeclaration sup = c.getSuperClass();
	if (sup != null) { 
	    ClassDefinition supDef = sup.getClassDefinition(env);
	    if (supDef.isAbstract(env))
		getAbstractFields(env, supDef, v);
	}
    }


    /**
     * Return true if this class has abstract fields.
     */
    public boolean hasAbstractFields(Environment env) throws ClassNotFound {
	return hasAbstractFields(env, this);
    }

    private boolean hasAbstractFields(Environment env, ClassDefinition c)
	   throws ClassNotFound {
	for (FieldDefinition f = c.getFirstField() ;  
	             f != null ; f = f.getNextField()) {
	    if (f.isAbstract()) {
		FieldDefinition m = findMethod(env, f.getName(), f.getType());
		if (m == null || m.isAbstract()) 
		    return true;
	    }
	}
	// Super class
	ClassDeclaration sup = c.getSuperClass();
	if (sup != null) { 
	    ClassDefinition supDef = sup.getClassDefinition(env);
	    return (supDef.isAbstract(env) && hasAbstractFields(env, supDef));
	}
	return false;
    }


    /**
     * Check if this is a super class of another class
     */
    public boolean superClassOf(Environment env, ClassDeclaration otherClass) throws ClassNotFound {
	while (otherClass != null) {
	    if (getClassDeclaration().equals(otherClass)) {
		return true;
	    }
	    otherClass = otherClass.getClassDefinition(env).getSuperClass();
	}
	return false;
    }

    /**
     * Check if this is a sub class of another class
     */
    public boolean subClassOf(Environment env, ClassDeclaration otherClass) throws ClassNotFound {
	ClassDeclaration c = getClassDeclaration();
	while (c != null) {
	    if (c.equals(otherClass)) {
		return true;
	    }
	    c = c.getClassDefinition(env).getSuperClass();
	}
	return false;
    }

    /**
     * Check if this class is implemented by another class
     */
    public boolean implementedBy(Environment env, ClassDeclaration c) throws ClassNotFound {
	for (; c != null ; c = c.getClassDefinition(env).getSuperClass()) {
	    if (getClassDeclaration().equals(c)) {
		return true;
	    }
	    ClassDeclaration intf[] = c.getClassDefinition(env).getInterfaces();
	    for (int i = 0 ; i < intf.length ; i++) {
		if (implementedBy(env, intf[i])) {
		    return true;
		}
	    }
	}
	return false;
    }


    /**
     * Check if a class can be accessed from another class
     */
    public boolean canAccess(Environment env, ClassDeclaration c) throws ClassNotFound {
	// Public access is always ok
	if (c.getClassDefinition(env).isPublic()) {
	    return true;
	}

	// It must be in the same package
	return getName().getQualifier().equals(c.getName().getQualifier());
    }

    /**
     * Check if a field can be accessed from a class
     */
    public boolean canAccess(Environment env, FieldDefinition f) 
               throws ClassNotFound {
	// Public access is always ok
	if (f.isPublic()) {
	    return true;
	}
	// Proteced access is ok from a subclass
	if (f.isProtected() && subClassOf(env, f.getClassDeclaration())) {
	    return true;
	}
	// Private access is ok only from the same class
	if (f.isPrivate()) {
	    return getClassDeclaration().equals(f.getClassDeclaration());
	}
	// It must be in the same package
	return getName().getQualifier().equals(f.getClassDeclaration().getName().getQualifier());
    }

 
    /**
     * We know the the field is marked protected (and not public) and that
     * the field is visible (as per canAccess).  Can we access the field as
     * <accessor>.<field>, where <accessor> has the type <accessorType>
     *
     * protected fields can only be accessed when the accessorType is a 
     * subclass of the current class
     */
    public boolean protectedAccess(Environment env, FieldDefinition f,
				   Type accessorType)
        throws ClassNotFound
    { 
               
	return 
	       // static protected fields are accessible
	       f.isStatic() 
	    || // allow array.clone()
	       accessorType.isType(TC_ARRAY)
	    || // <accessorType> is a subtype of the current class
	       (env.getClassDefinition(accessorType.getClassName())
		         .subClassOf(env, getClassDeclaration()))
	    || // we are accessing the field from a friendly class
	       (!f.isPrivate() &&
		  getName().getQualifier()
	           .equals(f.getClassDeclaration().getName().getQualifier()));
    }


   /**
     * Get a variable
     */
    public FieldDefinition getVariable(Environment env, Identifier nm) throws AmbiguousField, ClassNotFound {
	// Check if it is defined in the current class
	for (FieldDefinition field = getFirstMatch(nm) ; field != null ; field = field.getNextMatch()) {
	    if (field.isVariable()) {
		return field;
	    }
	}

	// Get it from the super class
	ClassDeclaration sup = getSuperClass();
	FieldDefinition field = (sup != null) ? sup.getClassDefinition(env).getVariable(env, nm) : null;

	// Get it from an interface
	for (int i = 0 ; i < interfaces.length ; i++) {
	    FieldDefinition f = interfaces[i].getClassDefinition(env).getVariable(env, nm);
	    if (f != null) {
		if ((field != null) && (f != field)) {
		    throw new AmbiguousField(f, field);
		}
		field = f;
	    }
	}
	return field;
    }

    /**
     * Match a method
     */
    private FieldDefinition 
    matchMethod(Environment env, ClassDefinition sourceClass, 
		Identifier nm, Type argTypes[], 
		FieldDefinition tentative, boolean pass1)
	     throws AmbiguousField, ClassNotFound {
		 // look in the current class
	for (FieldDefinition f = getFirstMatch(nm); 
	         f != null ; f = f.getNextMatch()) {
	    if (!env.isApplicable(f, argTypes))
		continue;
	    if (sourceClass != null && !sourceClass.canAccess(env, f))
		continue;
	    if (pass1) {
		if (tentative == null || env.isMoreSpecific(f, tentative)) {
		    tentative = f;
		}
	    } else { 
		// tentative better be more applicable than anything else
		if (f != tentative && !env.isMoreSpecific(tentative, f)) 
		    throw new AmbiguousField(tentative, f);
	    }
	}
	// constructors are not inherited
	if (nm.equals(idInit)) {
	    return tentative;
	}

	// Look in the superclasses
	if (tentative == null || !pass1) { 
	    // If we already have a tentative in pass1, there is no need to
	    // go up the superclass chain.  We're not going to find anything
	    // more specific in the "receiver" argument
	    ClassDeclaration sup = getSuperClass();
	    if (sup != null) { 
		ClassDefinition supDef = sup.getClassDefinition(env);
		tentative = supDef.matchMethod(env, sourceClass, nm, argTypes, 
					       tentative, pass1);
		if (isInterface()) { 
		    ClassDeclaration intDecls[] = getInterfaces();
		    for (int i = 0; i < intDecls.length; i++) {
			tentative = intDecls[i].getClassDefinition(env).
			    matchMethod(env, sourceClass, nm,
					argTypes, tentative, pass1);
		    }
		}
	    }
	}
	return tentative;
    }

    public FieldDefinition matchMethod(Environment env, 
				       ClassDefinition sourceClass, 
				       Identifier nm, Type argTypes[]) 
	    throws AmbiguousField, ClassNotFound {
	// pass1.  Find a "top" in the hierarchy;
	FieldDefinition f = matchMethod(env, sourceClass, 
					nm, argTypes, null, true);
	if (f != null) 
	    matchMethod(env, sourceClass, nm, argTypes, f, false);
	return f;
    }

    public FieldDefinition matchMethod(Environment env, 
				       ClassDefinition sourceClass, 
				       Identifier nm) 
	   throws AmbiguousField, ClassNotFound {
	return matchMethod(env, sourceClass, nm, Type.noArgs);
    }

    /**
     * Find a method, ie: exact match in this class or any of the super classes.
     */
    public FieldDefinition findMethod(Environment env, Identifier nm, Type t) throws ClassNotFound {
	// look in the current class
	FieldDefinition f;
	for (f = getFirstMatch(nm) ; f != null ; f = f.getNextMatch()) {
	    if (f.getType().equalArguments(t)) {
		return f;
	    }
	}

	// constructors are not inherited
	if (nm.equals(idInit)) {
	    return null;
	}

	// look in the super class
	ClassDeclaration sup = getSuperClass();
	if (sup == null) 
	    return null;
	return sup.getClassDefinition(env).findMethod(env, nm, t);
    }

 
    // We create a stub for this.  Source classes do more work.
    protected void basicCheck(Environment env) throws ClassNotFound {}    



   /**
     * Find any method with a given name.
     */
    public FieldDefinition findAnyMethod(Environment env, Identifier nm) throws ClassNotFound {
	FieldDefinition f = getFirstMatch(nm);
	if (f != null) {
	    return f;
	}

	// look in the super class
	ClassDeclaration sup = getSuperClass();
	if (sup == null) 
	    return null;
	return sup.getClassDefinition(env).findAnyMethod(env, nm);
    }
    
    /**
     * Add a field (no checks)
     */
    public void addField(FieldDefinition field) {
	//System.out.println("ADD = " + field);
	if (firstField == null) {
	    firstField = lastField = field;
	} else {
	    lastField.nextField = field;
	    lastField = field;
	    field.nextMatch = (FieldDefinition)fieldHash.get(field.name);
	}
	fieldHash.put(field.name, field);
    }

    /**
     * Add a dependency
     */
    public void addDependency(ClassDeclaration c) {
	throw new CompilerError("addDependency");
    }

    /**
     * Print for debugging
     */
    public void print(PrintStream out) {
	if (isPublic()) {
	    out.print("public ");
	} 
	if (isInterface()) {
	    out.print("interface ");
	} else {
	    out.print("class ");
	}
	out.print(getName() + " ");
	if (getSuperClass() != null) {
	    out.print("extends " + getSuperClass().getName() + " ");
	}
	if (interfaces.length > 0) {
	    out.print("implements ");
	    for (int i = 0 ; i < interfaces.length ; i++) {
		if (i > 0) {
		    out.print(", ");
		}
		out.print(interfaces[i].getName());
		out.print(" ");
	    }
	}
	out.println("{");

	for (FieldDefinition f = getFirstField() ; f != null ; f = f.getNextField()) {
	    out.print("    ");
	    f.print(out);
	}

	out.println("}");
    }

    /**
     * Convert to String
     */
    public String toString() {
    	return getClassDeclaration().toString();
    }
}
