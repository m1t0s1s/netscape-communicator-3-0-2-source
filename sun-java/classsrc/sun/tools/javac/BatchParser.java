/*
 * @(#)BatchParser.java	1.26 95/11/13 Arthur van Hoff
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

import java.io.IOException;
import java.io.InputStream;
import java.util.Vector;
import java.util.Enumeration;

/**
 * Batch file parser, this needs more work.
 */

public final
class BatchParser extends Parser {
    /**
     * The current package
     */
    Identifier pkg;

    /**
     * The current imports
     */
    Imports imports;

    /**
     * The classes defined in this file
     */
    Vector classes;
     

    /**
     * The current class
     */
    SourceClass sourceClass;

    /**
     * The toplevel environment
     */
    Environment toplevelEnv;
    
    /**
     * Create a batch file parser
     */
    public BatchParser(Environment env, InputStream in) throws IOException {
	super(env, in);

	imports = new Imports();
	classes = new Vector();
	toplevelEnv = env;
    }

    /**
     * Resolve a name into a full class name
     */
    protected Identifier resolveClass(Identifier nm) {
	try {
	    return imports.resolve(env, nm);
	} catch (AmbiguousClass e) {
	    env.error(pos, "ambig.class", e.name1, e.name2);
	} catch (ClassNotFound e) {
	}
	return (pkg != null) ? Identifier.lookup(pkg, nm) : nm;
    }

    /**
     * Package declaration
     */
    protected void packageDeclaration(int where, Identifier nm) {
	//System.out.println("package " + nm);

	if (pkg == null) {
	    imports.addPackage(pkg = nm);
	} else {
	    env.error(where, "package.repeated");
	}
    }

    /**
     * Import class
     */
    protected void importClass(int where, Identifier nm) {
	//System.out.println("import class " + nm);
	try {
	    if (!env.classExists(nm)) {
		env.error(where, "class.not.found", nm, "import");
	    }
	    imports.addClass(nm);
	} catch (AmbiguousClass e) {
	    env.error(where, "ambig.class", e.name1, e.name2);
	}
    }

    /**
     * Import package
     */
    protected void importPackage(int where, Identifier nm) {
	try {
	    //println("import package " + nm);
	    Package p = env.getPackage(nm);
	    if (!p.exists()) {
		env.error(where, "package.not.found", nm, "import");
	    }
	    imports.addPackage(nm);
	} catch (IOException e) {
	    env.error(where, "io.exception", "import");
	}
    }

    /**
     * Define class
     */
    protected void beginClass(int where, String doc, int mod, Identifier nm,
			       Identifier sup, Identifier impl[]) {

	Identifier pkgNm = (pkg != null) ? Identifier.lookup(pkg, nm) : nm;

	// Does the name already exist in an imported package?
	try {
	    // We want this to throw a ClassNotFound exception
	    Identifier ID = imports.resolve(env, nm);
	    if (ID != pkgNm) 
		env.error(pos, "class.multidef.import", nm, ID);
	} catch (AmbiguousClass e) {
	    // At least one of e.name1 and e.name2 must be different
	    Identifier ID = (e.name1 != pkgNm) ? e.name1 : e.name2;
	    env.error(pos, "class.multidef.import", nm, ID);
	 }  catch (ClassNotFound e) {
	     // we want this to happen
	 }


	// Find the class
	ClassDeclaration c = env.getClassDeclaration(pkgNm);

	// Make sure this is the first definition
	if (c.isDefined()) {
	    env.error(where, "class.multidef", c.getName(), c.getClassDefinition().getSource());
	}

	// Find the super class
	ClassDeclaration superClass = null;
	if ((sup == null) || ((mod & M_INTERFACE) != 0)) {
	    superClass = env.getClassDeclaration(idJavaLangObject);
	} else if (!c.getName().equals(idJavaLangObject)) {
	    superClass = env.getClassDeclaration(sup);
	}

	// Find interfaces
	ClassDeclaration interfaces[] = new ClassDeclaration[impl.length];
	for (int i = 0 ; i < interfaces.length ; i++) {
	    interfaces[i] = env.getClassDeclaration(impl[i]);
	}

	sourceClass = new SourceClass(toplevelEnv, where, c, doc, mod, superClass, interfaces, imports);
	sourceClass.getClassDeclaration().setDefinition(sourceClass, CS_PARSED);
	env = new Environment(toplevelEnv, sourceClass);

	// Setup interdependencies between the classes in this file,
	// this is needed so that when one is recompiled, all are.
	for (Enumeration e = classes.elements() ; e.hasMoreElements() ;) {
	    ClassDefinition otherClass = (ClassDefinition)e.nextElement();
	    otherClass.addDependency(sourceClass.getClassDeclaration());
	    sourceClass.addDependency(otherClass.getClassDeclaration());
	}
	classes.addElement(sourceClass);
    }

    /**
     * End class
     */
    protected void endClass(int where, Identifier nm) {
	sourceClass = null;
	env = toplevelEnv;
    }

    /**
     * Define a method
     */
    protected void defineField(int where, String doc, int mod, Type t, Identifier nm,
			       Identifier args[], Identifier exp[], Node val) {
	if (sourceClass.isInterface()) {
	    mod |= M_PUBLIC;
	    if (t.isType(TC_METHOD)) {
		mod |= M_ABSTRACT;
	    } else {
		mod |= M_STATIC | M_FINAL;
	    }
	}
	if (nm.equals(idInit)) {
	    if (sourceClass.getType().equals(t.getReturnType())) {
		t = Type.tMethod(Type.tVoid, t.getArgumentTypes());
	    } else {
		env.error(where, "invalid.method.decl");
		return;
	    }
	}

	Vector v = null;
	if (args != null) {
	    v = new Vector(args.length);
	    for (int i = 0 ; i < args.length ; i++) {
		v.addElement(args[i]);
	    }
	} else if (t.isType(TC_METHOD)) {
	    v = new Vector();
	}

	ClassDeclaration exceptions[] = null;
	if (exp != null) {
	    exceptions = new ClassDeclaration[exp.length];
	    for (int i = 0 ; i < exp.length ; i++) {
		exceptions[i] = env.getClassDeclaration(exp[i]);
	    }
	} else if (t.isType(TC_METHOD)) {
	    exceptions = new ClassDeclaration[0];
	}

	SourceField f = new SourceField(where, sourceClass, doc, mod, t, nm, v, exceptions, val);
	sourceClass.addField(env, f);
	if (env.dump()) {
	    f.print(System.out);
	}
    }
}
