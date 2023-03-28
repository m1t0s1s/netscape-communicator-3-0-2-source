/*
 * @(#)Main.java	1.40 95/11/12 Arthur van Hoff
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

import java.util.*;
import java.io.*;
import java.lang.SecurityManager;

/**
 * Main program of the Java compiler
 */

public
class Main implements Constants {
    /**
     * Name of the program.
     */
    String program;

    /**
     * The stream where error message are printed.
     */
    OutputStream out;

    /**
     * Constructor.
     */
    public Main(OutputStream out, String program) {
	this.out = out;
	this.program = program;
    }

    /**
     * Output a message.
     */
    public void output(String msg) {
	try {
	    int len = msg.length();
	    for (int i = 0 ; i < len ; i++) {
		out.write(msg.charAt(i));
	    }
	    out.write('\n');
	} catch (IOException e) {
	}
    }

    /**
     * Top level error message
     */
    public void error(String msg) {
	output(program + ": " + msg);
    }
    
    /**
     * Usage
     */
    public void usage() {
	output("use: " + program + " [-g][-O][-debug][-depend][-nowarn][-verbose][-classpath path][-nowrite][-d dir] file.java...");
    }
    
    /**
     * Run the compiler
     */
    public synchronized boolean compile(String argv[]) {
	SecurityManager.setScopePermission();
	String classPathString = System.getProperty("java.class.path");
	SecurityManager.resetScopePermission();
	File destDir = null;
	int flags = F_WARNINGS;
	long tm = System.currentTimeMillis();
	Vector v = new Vector();
	boolean nowrite = false;
	String props = null;

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
	    } else if (argv[i].equals("-classpath")) {
		if ((i + 1) < argv.length) {
		    classPathString = argv[++i];
		} else {
		    error("-classpath requires argument");
		    usage();
		    return false;
		}
	    } else if (argv[i].equals("-d")) {
		if ((i + 1) < argv.length) {
		    destDir = new File(argv[++i]);
		    if (!destDir.exists()) {
			error(destDir.getPath() + " does not exist");
			return false;
		    }
		} else {
		    error("-d requires argument");
		    usage();
		    return false;
		}
	    } else if (argv[i].startsWith("-")) {
		error("invalid flag: " + argv[i]);
		usage();
		return false;
	    } else if (argv[i].endsWith(".java")) {
		v.addElement(argv[i]);
	    } else {
		error("invalid argument: " + argv[i]);
		usage();
		return false;
	    }
	}
	if (v.size() == 0) {
	    usage();
	    return false;
	}

	// Create batch environment
	if (classPathString == null) {
	    classPathString = ".";
	}
	ClassPath classPath = new ClassPath(classPathString);
	BatchEnvironment env = new BatchEnvironment(out, classPath);
	env.flags |= flags;

	try {
	    // Parse all input files
	    for (Enumeration e = v.elements() ; e.hasMoreElements() ;) {
		File file = new File((String)e.nextElement());
		try {
		    env.parseFile(new ClassFile(file));
		} catch (FileNotFoundException ee) {
		    env.error(0, "cant.read", file.getPath());
		}
	    }

	    // compile all classes that need compilation
	    ByteArrayOutputStream buf = new ByteArrayOutputStream(4096);
	    boolean done;

	    do {
		done = true;
		env.flushErrors();
		for (Enumeration e = env.getClasses() ; e.hasMoreElements() ; ) {
		    ClassDeclaration c = (ClassDeclaration)e.nextElement();
		    switch (c.getStatus()) {
		      case CS_UNDEFINED:
			if (!env.dependencies()) {
			    break;
			}
			// fall through

		      case CS_SOURCE:
			done = false;
			env.loadDefinition(c);
			if (c.getStatus() != CS_PARSED) {
			    break;
			}
			// fall through
			
		      case CS_PARSED:
			done = false;
			buf.reset();
			SourceClass src = (SourceClass)c.getClassDefinition(env);
			src.compile(env, buf);
			c.setDefinition(src, CS_COMPILED);

			if (src.getError() || nowrite) {
			    continue;
			}

			String pkgName = c.getName().getQualifier().toString().replace('.', File.separatorChar);
			String className = c.getName().getName().toString() + ".class";

			File file;
			if (destDir != null) {
			    if (pkgName.length() > 0) {
				file = new File(destDir, pkgName);
				if (!file.exists()) {
				    file.mkdirs();
				}
				file = new File(file, className);
			    } else {
				file = new File(destDir, className);
			    }
			} else {
			    ClassFile classfile = (ClassFile)src.getSource();
			    if (classfile.isZipped()) {
				env.error(0, "cant.write", classfile.getPath());
				continue;
			    }
			    file = new File(classfile.getPath());
			    file = new File(file.getParent(), className);
			}

			// Create the file
			try {
			    FileOutputStream out = new FileOutputStream(file.getPath());
			    buf.writeTo(out);
			    out.close();
			    if (env.verbose()) {
				output("[wrote " + file.getPath() + "]");
			    }
			} catch (IOException ee) {
			    env.error(0, "cant.write", file.getPath());
			}
		    }
		}
	    } while (!done);
	} catch (Error ee) {
	    ee.printStackTrace();
	    env.error(0, "fatal.error");
	} catch (Exception ee) {
	    ee.printStackTrace();
	    env.error(0, "fatal.exception");
	}

	env.flushErrors();

	boolean status = true;
	if (env.nerrors > 0) {
	    String msg = "";
	    if (env.nerrors > 1) {
		msg = env.nerrors + " errors";
	    } else {
		msg = "1 error";
	    }
	    if (env.nwarnings > 0) {
		if (env.nwarnings > 1) {
		    msg += ", " + env.nwarnings + " warnings";
		} else {
		    msg += ", 1 warning";
		}
	    }
	    output(msg);
	    status = false;
	} else {
	    if (env.nwarnings > 0) {
		if (env.nwarnings > 1) {
		    output(env.nwarnings + " warnings");
		} else {
		    output("1 warning");
		}
	    }
	}

	// We're done
	if (env.verbose()) {
	    tm = System.currentTimeMillis() - tm;
	    output("[done in " + tm + "ms]");
	}
	
	return status;
    }

    /**
     * Main program
     */
    public static void main(String argv[]) {
	Main compiler = new Main(System.out, "javac");
	System.exit(compiler.compile(argv) ? 0 : 1);
    }
}
