/*
 * @(#)Main.java	1.24 95/12/07 Arthur van Hoff
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

package sun.tools.javadoc;

import sun.tools.java.*;
import sun.tools.javac.BatchEnvironment;
import sun.tools.javac.SourceClass;

import java.util.Hashtable;
import java.util.Vector;
import java.util.Enumeration;
import java.io.FileOutputStream;
import java.io.File;
import java.io.IOException;
import java.lang.SecurityManager;

/**
 * Main program of the Java compiler
 */

public final
class Main implements Constants {
    /**
     * Name of the program
     */
    public static final String program = "javadoc";
    public static BatchEnvironment env;
    public static File destDir;

    /**
     * Top level error message
     */
    static void error(String msg) {
	System.out.println(program + ": " + msg);
	env.nerrors++;
    }

    public static DocumentationGenerator generator;
            
    public static boolean showAuthors = false;
    public static boolean showVersion = false;
    public static boolean showIndex = true;
    public static boolean showTree = true;




    /**
     * Exit, reporting errors and warnings.
     */
    static void exit() {
	int status = 0;

	env.flushErrors();
	if (env.nerrors > 0) {
	    System.out.print(env.nerrors);
	    if (env.nerrors > 1) {
		System.out.print(" errors");
	    } else {
		System.out.print(" error");
	    }
	    if (env.nwarnings > 0) {
		System.out.print(", " + env.nwarnings);
		if (env.nwarnings > 1) {
		    System.out.print(" warnings");
		} else {
		    System.out.print(" warning");
		}
	    }
	    System.out.println();
	    status = 1;
	} else {
	    if (env.nwarnings > 0) {
		System.out.print(env.nwarnings);
		if (env.nwarnings > 1) {
		    System.out.println(" warnings");
		} else {
		    System.out.println(" warning");
		}
	    }
	}

	System.exit(status);
    }
	
    /**
     * Usage
     */
    static void usage() {
	System.out.println("use: " + program + " [-g] [-O] [-debug] ");
	System.out.println("\t[-depend] [-nowarn] [-verbose] [-nowrite]");
	System.out.println("\t[-authors] [-version] [-noindex] [-notree]");
	System.out.println("[-sourcepath path] [-doctype str] [-d dir] [class...");
    }
    
    /**
     * Main program
     */
    public static void main(String argv[]) {
	SecurityManager.setScopePermission();
	String classPathString = System.getProperty("java.class.path");
	SecurityManager.resetScopePermission();
	int flags = F_WARNINGS;
	long tm = System.currentTimeMillis();
	Vector packages = new Vector();
	Vector classes = new Vector();
	boolean nowrite = false;
	String docType = "HTML";

	// Parse arguments
	for (int i = 0 ; i < argv.length ; i++) {
	    if (argv[i].equals("-g")) {
		flags &= ~F_OPTIMIZE;
		flags |= F_DEBUG;
	    } else if (argv[i].equals("-O")) {
		flags &= ~F_DEBUG;
		flags |= F_OPTIMIZE | F_DEPENDENCIES;
	    } else if (argv[i].equals("-nowarn")) {
		flags &= ~F_WARNINGS;
	    } else if (argv[i].equals("-debug")) {
		flags |= F_DUMP;
	    } else if (argv[i].equals("-depend")) {
		flags |= F_DEPENDENCIES;
	    } else if (argv[i].equals("-verbose")) {
		flags |= F_VERBOSE;
	    } else if (argv[i].equals("-nowrite")) {
		nowrite = true;
	    } else if (argv[i].equals("-version")) { 
	        showVersion = true;
	    } else if (argv[i].equals("-author") || 
		       argv[i].equals("-authors")) { 
	        showAuthors = true;
	    } else if (argv[i].equals("-sourcepath") ||
		       argv[i].equals("-classpath")) {
		if ((i + 1) < argv.length) {
		    classPathString = argv[++i];
		} else {
		    error("-sourcepath requires argument");
		    usage();
		    exit();
		}
	    } else if (argv[i].equals("-doctype")) {
	        if ((i + 1) < argv.length) {
		    docType = argv[++i];
		} else {
		    error("-docType requires argument");
		    usage();
		    exit();
		}
	    } else if (argv[i].equals("-noindex")) {
	        showIndex = false;
	    } else if (argv[i].equals("-notree")) {
	        showTree = false;
	    } else if (argv[i].equals("-d")) {
		if ((i + 1) < argv.length) {
		    destDir = new File(argv[++i]);
		    if (!destDir.exists()) {
		      error(destDir.getPath() + " does not exist");
		      exit();
		    }
		} else {
		    error("-d requires argument");
		    usage();
		    exit();
		}
	    } else if (argv[i].startsWith("-")) {
		error("invalid flag: " + argv[i]);
		usage();
		exit();
	    } else if (argv[i].endsWith(".java")
		          && (argv[i].indexOf('.') == argv[i].length() - 5)) {
		classes.addElement(argv[i]);
	    } else {
		packages.addElement(argv[i]);
	    }
	}
	if ((packages.size() == 0) && (classes.size() == 0)) {
	    error("No packages or classes specified.");
	    usage();
	    exit();
	}

	// Create class path and environment
	if (classPathString == null) {
	    classPathString = ".";
	}

	ClassPath classPath = new ClassPath(classPathString);
	env = new BatchEnvironment(classPath);
	env.flags |= flags;

	try { 
	    Class t = Class.forName("sun.tools.javadoc." + docType + 
				    "DocumentationGenerator");
	    generator = ((DocumentationGenerator)t.newInstance()).init();
	} catch (Exception e) { 
	    System.out.println("Cannot generate " + docType + " documentation");
	    return;
	}

	try {
	    // Parse class files
	    for (Enumeration e1 = classes.elements() ; e1.hasMoreElements() ;) {
		File file = new File((String)e1.nextElement());
		try {
		    env.parseFile(new ClassFile(file));
		} catch (IOException ee) {
		    env.error(0, "cant.read", file.getPath());
		}
	    }

	    // Figure out which classes where encountered
	    classes = new Vector();
	    for (Enumeration e1 = env.getClasses() ; e1.hasMoreElements() ;) {
		ClassDeclaration decl = (ClassDeclaration)e1.nextElement();
		if (decl.isDefined() && (decl.getClassDefinition() instanceof SourceClass)) {
		    classes.addElement(decl.getClassDefinition());
		}
	    }

	    // Parse input files
	    Hashtable purged = new Hashtable();
	    for (Enumeration e1 = packages.elements() ; e1.hasMoreElements() ;) {
		Identifier pkg = Identifier.lookup((String)e1.nextElement());
		Package p = new Package(classPath, pkg);
		System.out.println("Loading source files for " + pkg);
		for (Enumeration e2 = p.getSourceFiles() ; e2.hasMoreElements() ;) {
		    ClassFile file = (ClassFile)e2.nextElement();
		    try {
			env.parseFile(file);
		    } catch (IOException ee) {
			env.error(0, "cant.read", file.getPath());
		    }
		}
		for (Enumeration e = env.getClasses() ; e.hasMoreElements() ;) {
		    ClassDeclaration decl = (ClassDeclaration)e.nextElement();
		    ClassDefinition defn = decl.getClassDefinition();
		    if (defn == null || purged.get(decl) != null)
			continue;
		    for (FieldDefinition f = defn.getFirstField(); 
			    f != null; f = f.getNextField()) { 
			if (f.isMethod() && f.getValue() != null)
			    f.setValue(null);
		    }
		    purged.put(decl, decl);
		}
	    }

	    // Generate documentation for packages
	    generator.genPackagesDocumentation(packages);
	    for (Enumeration e1 = packages.elements() ; e1.hasMoreElements() ;) {
		Identifier pkg = Identifier.lookup((String)e1.nextElement());
		generator.genPackageDocumentation(pkg);
	    }

	    // Generate documentation for classes
	    for (Enumeration e1 = classes.elements() ; e1.hasMoreElements() ;) {
		generator.genClassDocumentation(((ClassDefinition)e1.nextElement()), null, null);
	    }
	    if (showIndex) {
	        System.out.println("Generating index");
	        generator.genFieldIndex();
	    }
	    if (showTree) {
	        System.out.println("Generating tree");
	        generator.genClassTree();
	    }
	} catch (Error ee) {
	    ee.printStackTrace();
	    env.error(0, "fatal.error");
	    env.flushErrors();
	    System.exit(1);
	} catch (Exception ee) {
	    ee.printStackTrace();
	    env.error(0, "fatal.exception");
	    env.flushErrors();
	    System.exit(1);
	}

	// We're done.
	if (env.verbose()) {
	    tm = System.currentTimeMillis() - tm;
	    System.out.println("[done in " + tm + "ms]");
	}

	exit();
    }
}
