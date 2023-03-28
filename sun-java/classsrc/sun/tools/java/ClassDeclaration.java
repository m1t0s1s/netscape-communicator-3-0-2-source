/*
 * @(#)ClassDeclaration.java	1.13 95/09/14 Arthur van Hoff
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

/**
 * This class represents an Java class declaration. It refers
 * to either a binary or source definition.
 *
 * ClassDefintions are loaded on demand, this means that
 * class declarations are late bound. The definition of the
 * class is obtained in stages. The status field describes
 * the state of the class defintion:
 *
 * CS_UNDEFINED - the definition is not yet loaded
 * CS_UNDECIDED - a binary definition is loaded, but it is
 *	          still unclear if the source defintion need to
 *		  be loaded
 * CS_BINARY    - the binary class is loaded
 * CS_PARSED	- the class is loaded from the source file, the
 *		  type information is available, but the class has
 *		  not yet been compiled.
 * CS_COMPILED  - the class is loaded from the source file and has
 *		  been compiled.
 * CS_NOTFOUND  - no class definition could be found
 */

public final
class ClassDeclaration implements Constants {
    int status;
    Identifier name;
    ClassDefinition definition;

    /**
     * Constructor
     */
    public ClassDeclaration(Identifier name) {
	this.name = name;
    }

    /**
     * Get the status of the class
     */
    public int getStatus() {
	return status;
    }

    /**
     * Get the name of the class
     */
    public Identifier getName() {
       return name;
    }

    /**
     * Get the type of the class
     */
    public Type getType() {
	return Type.tClass(name);
    }

    /**
     * Check if the class is defined
     */
    public boolean isDefined() {
	switch (status) {
	  case CS_BINARY:
	  case CS_PARSED:
	  case CS_COMPILED:
	    return true;
	}
	return false;
    }

    /**
     * Get the definition of this class. Returns null if
     * the class is not yet defined.
     */
    public ClassDefinition getClassDefinition() {
	return definition;
    }

    /**
     * Get the definition of this class, if the class is not
     * yet defined, load the definition. Loading a class may
     * throw various exceptions.
     */
    public ClassDefinition getClassDefinition(Environment env) throws ClassNotFound {
	for(;;) {
	    switch (status) {
	        case CS_UNDEFINED:
	        case CS_UNDECIDED:
	        case CS_SOURCE:
		    env.loadDefinition(this);
		    break;

	        case CS_BINARY:
	        case CS_PARSED:
	        case CS_COMPILED:
		    definition.basicCheck(env);
		    return definition;

	        default:
		    throw new ClassNotFound(getName());
		}
	}
    }

    /**
     * Set the class definition
     */
    public void setDefinition(ClassDefinition definition, int status) {
	this.definition = definition;
	this.status = status;
	if ((definition != null) && !getName().equals(definition.getName())) {
	    throw new CompilerError("invalid class defintion: " + this + ", " + definition);
	}
    }

    /**
     * Equality
     */
    public boolean equals(Object obj) {
	if ((obj != null) && (obj instanceof ClassDeclaration)) {
	    return name.equals(((ClassDeclaration)obj).name);
	}
	return false;
    }

    /**
     * toString
     */
    public String toString() {
	if (getClassDefinition() != null) {
	    if (getClassDefinition().isInterface()) {
		return "interface " + getName().toString();
	    } else {
		return "class " + getName().toString();
	    }
	} 
	return getName().toString();
    }
}
