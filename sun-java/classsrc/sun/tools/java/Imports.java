/*
 * @(#)Imports.java	1.9 95/09/14 Arthur van Hoff
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

import java.util.Hashtable;
import java.util.Vector;
import java.util.Enumeration;

/**
 * This class describes the classes and packages imported
 * from a source file. A Hashtable called bindings is maintained
 * to quickly map symbol names to classes. This table is flushed
 * everytime a new import is added.
 *
 * A class name is resolved as follows:
 *  - if it is a qualified name then return the corresponding class
 *  - if the name corresponds to an individually imported class then return that class
 *  - check if the class is defined in any of the imported packages,
 *    if it is then return it, make sure it is defined in only one package
 *  - assume that the class is defined in the current package
 *
 * REMIND: need to check for private classes
 */

public
class Imports implements Constants {
    /**
     * The imported classes
     */
    Hashtable classes = new Hashtable();

    /**
     * The imported package identifiers
     */
    Vector packages = new Vector();

    /**
     * Constructor, always import java.lang.
     */
    public Imports() {
	addPackage(idJavaLang);
    }
	    
    /**
     * Lookup a class, given the current set of imports,
     * AmbiguousClass exception is thrown if the name can be
     * resolved in more than one way. A ClassNotFound exception
     * is thrown if the class is not found in the imported classes
     * and packages.
     */
    public synchronized Identifier resolve(Environment env, Identifier nm) throws ClassNotFound {
	if (nm.isQualified()) {
	    // Don't bother it is already qualified
	    return nm;
	}

	// Check if it was imported before
	Identifier className = (Identifier)classes.get(nm);
	if (className != null) {
	    return className;
	}
	
	// look in one of the imported packages
	for (Enumeration e = packages.elements() ; e.hasMoreElements() ; ) {
	    Identifier id = Identifier.lookup((Identifier)e.nextElement(), nm);

	    if (env.classExists(id)) {
		if ((className == null) || className.equals(id)) {
		    className = id;
		} else {
		    throw new AmbiguousClass(id, className);
		}
	    }
	}

	// Make sure a class was found
	if (className == null) {
	    throw new ClassNotFound(nm);
	}

	// Remember the binding
	classes.put(nm, className);
	return className;
    }

    /**
     * Add a class import
     */
    public synchronized void addClass(Identifier id) throws AmbiguousClass {
	Identifier nm = id.getName();

	// make sure it isn't already imported explicitly
	Identifier className = (Identifier)classes.get(nm);
	if (className != null) {
	    throw new AmbiguousClass(id, className);
	}

	classes.put(nm, id);
    }

    /**
     * Add a package import
     */
    public synchronized void addPackage(Identifier id) {
	packages.addElement(id);
    }
}
