/*
 * @(#)FieldDefinition.java	1.24 95/09/27 Arthur van Hoff
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

import sun.tools.tree.Node;
import sun.tools.tree.Expression;
import sun.tools.tree.Context;
import sun.tools.asm.Assembler;
import java.io.PrintStream;
import java.util.Vector;

/**
 * This class defines an Java field.
 */
public
class FieldDefinition implements Constants {
    protected int where;
    protected int modifiers;
    protected Type type;
    protected String documentation;
    protected ClassDeclaration exp[];
    protected Node value;
    protected ClassDefinition clazz;
    protected Identifier name;
    protected FieldDefinition nextField;
    protected FieldDefinition nextMatch;

    /**
     * Constructor
     */
    public FieldDefinition(int where, ClassDefinition clazz, int modifiers,
			      Type type, Identifier name, ClassDeclaration exp[], Node value) {
	this.where = where;
	this.clazz = clazz;
	this.modifiers = modifiers;
	this.type = type;
	this.name = name;
	this.exp = exp;
	this.value = value;
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
	return clazz.getClassDeclaration();
    }

    /**
     * Get the class declaration in which the field is actually defined
     */
    public ClassDeclaration getDefiningClassDeclaration() {
	return getClassDeclaration(); 
    }

    /**
     * Get the class definition
     */
    public final ClassDefinition getClassDefinition() {
	return clazz;
    }

    /**
     * Get the field's modifiers
     */
    public final int getModifiers() {
	return modifiers;
    }
    public final void subModifiers(int mod) {
	modifiers &= ~mod;
    }
    public final void addModifiers(int mod) {
	modifiers |= mod;
    }

    /**
     * Get the field's type
     */
    public final Type getType() {
	return type;
    }

    /**
     * Get the field's name
     */
    public final Identifier getName() {
	return name;
    }

    /**
     * Get arguments (a vector of LocalField)
     */
    public Vector getArguments() {
	return isMethod() ? new Vector() : null;
    }

    /**
     * Get the exceptions that are thrown by this method.
     */
    public ClassDeclaration[] getExceptions(Environment env) {
	return exp;
    }

    /**
     * Get the field's final value (may return null)
     */
    public Node getValue(Environment env) throws ClassNotFound {
	return value;
    }
    public final Node getValue() {
	return value;
    }
    public final void setValue(Node value) {
	this.value = value;
    }
    public Object getInitialValue() {
	return null;
    }

    /**
     * Get the next field or the next match
     */
    public final FieldDefinition getNextField() {
	return nextField;
    }
    public final FieldDefinition getNextMatch() {
	return nextMatch;
    }

    /**
     * Get the field's documentation
     */
    public String getDocumentation() {
	return documentation;
    }

    /**
     * Resolve a class name
     */
    public Identifier resolve(Environment env, Identifier nm) throws ClassNotFound {
	throw new CompilerError("resolve");
    }

    /**
     * Check the field definition
     */
    public void check(Environment env) throws ClassNotFound {
    }

    /**
     * Generate code
     */
    public void code(Environment env, Assembler asm) throws ClassNotFound {
	throw new CompilerError("code");
    }
    public void codeInit(Environment env, Context ctx, Assembler asm) throws ClassNotFound {
	throw new CompilerError("codeInit");
    }

    /**
     * Check if a field can access another field (only considers
     * static versus instance access, not the access modifiers).
     */
    public final boolean canAccess(Environment env, FieldDefinition f) {
	return (!isStatic()) || f.isLocal() || f.isStatic();
    }

    /**
     * Check if a field can reach another field (only considers
     * forward references, not the access modifiers).
     */
    public final boolean canReach(Environment env, FieldDefinition f) {
	if ((isVariable() || isInitializer()) && (isStatic() || !f.isStatic()) &&
	    (!f.isLocal()) && getClassDeclaration().equals(f.getClassDeclaration())) {

	    while (((f = f.getNextField()) != null) && (f != this));
	    return f != null;
	}
	return true;
    }

    /**
     * Checks
     */
    public final boolean isPublic() {
	return (modifiers & M_PUBLIC) != 0;
    }
    public final boolean isPrivate() {
	return (modifiers & M_PRIVATE) != 0;
    }
    public final boolean isProtected() {
	return (modifiers & M_PROTECTED) != 0;
    }
    public final boolean isFinal() {
	return (modifiers & M_FINAL) != 0;
    }
    public final boolean isStatic() {
	return (modifiers & M_STATIC) != 0;
    }
    public final boolean isSynchronized() {
	return (modifiers & M_SYNCHRONIZED) != 0;
    }
    public final boolean isAbstract() {
	return (modifiers & M_ABSTRACT) != 0;
    }
    public final boolean isNative() {
	return (modifiers & M_NATIVE) != 0;
    }
    public final boolean isVolatile() {
	return (modifiers & M_VOLATILE) != 0;
    }
    public final boolean isTransient() {
	return (modifiers & M_TRANSIENT) != 0;
    }
    public final boolean isMethod() {
	return type.isType(TC_METHOD);
    }
    public final boolean isVariable() {
	return !type.isType(TC_METHOD);
    }
    public final boolean isInitializer() {
	return getName().equals(idClassInit);
    }
    public final boolean isConstructor() {
	return getName().equals(idInit);
    }
    public boolean isLocal() {
	return false;
    }
    public boolean isInlineable(Environment env, boolean fromFinal) throws ClassNotFound {
	return (isStatic() || isPrivate() || isFinal() || isConstructor() || fromFinal) && 
	    !(isSynchronized() || isNative());
    }

    /**
     * toString
     */
    public String toString() {
	Identifier name = getClassDefinition().getName();
	if (isInitializer()) {
	    return "static {}";
	} else if (isConstructor()) {
	    StringBuffer buf = new StringBuffer();
	    buf.append(name);
	    buf.append('(');
	    Type argTypes[] = getType().getArgumentTypes();
	    for (int i = 0 ; i < argTypes.length ; i++) {
		if (i > 0) {
		    buf.append(',');
		}
		buf.append(argTypes[i].toString());
	    }
	    buf.append(')');
	    return buf.toString();
	}
	return type.typeString(getName().toString());
    }

    /**
     * Print for debugging
     */
    public void print(PrintStream out) {
	if (isPublic()) {
	    out.print("public ");
	}
	if (isPrivate()) {
	    out.print("private ");
	}
	if (isProtected()) {
	    out.print("protected ");
	}
	if (isFinal()) {
	    out.print("final ");
	}
	if (isStatic()) {
	    out.print("static ");
	}
	if (isSynchronized()) {
	    out.print("synchronized ");
	}
	if (isAbstract()) {
	    out.print("abstract ");
	}
	if (isNative()) {
	    out.print("native ");
	}
	if (isVolatile()) {
	    out.print("volatile ");
	}
	if (isTransient()) {
	    out.print("transient ");
	}
	out.println(toString() + ";");
    }
}
