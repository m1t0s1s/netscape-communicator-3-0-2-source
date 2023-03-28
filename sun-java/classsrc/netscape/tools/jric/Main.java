// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.jric;

import netscape.tools.jric.*;
import netscape.tools.ClassFileParser.*;
import java.io.*;

public class Main {
    // main entry point
    public static void main(String[] argv) throws Exception {
	new Main(argv);
    }

    protected static final int MODE_HEADERS	 = 0;
    protected static final int MODE_STUBS	 = 1;

    // instance variables
    protected int outputMode = MODE_HEADERS;
    protected boolean verboseMode = false;
    protected boolean macMode = false;
    protected String prefix;
    protected String outputDir;

    // constructor 
    protected Main(String[] argv) {
	int i = 0;
	while (i < argv.length) {
	    i = processArg(argv, i);
	}
    }

    protected int processArg(String[] argv, int i) {
	if (argv[i].equals("-classpath")) {
	    i++;
	    ClassFileParser.classpath = parseClassPath(argv[i]);
	}
	else if (argv[i].equals("-stubs")) {
	    outputMode = MODE_STUBS;
	}
	else if (argv[i].equals("-verbose")) {
	    verboseMode = true;
	}
	else if (argv[i].equals("-mac")) {
	    macMode = true;
	}
	else if (argv[i].equals("-d")) {
	    outputDir = argv[++i];
	}
	else if (argv[i].equals("-debug")) {
	    ClassFileParser.trace = true;
	}
	else if (argv[i].equals("-prefix")) {
	    prefix = argv[++i];
	}
	else {
	    try {
		String classname = argv[i].replace('.', '/');
		ClassDef clazz = ClassFileParser.getClassDef(classname);
		clazz.setPrintInfo(prefix, macMode);
		Generator gen = getGenerator(clazz);
//		long startTime = System.currentTimeMillis();
		gen.generate();
//		long stopTime = System.currentTimeMillis();
//		System.err.println("Time to generate " + classname + " = " + (stopTime - startTime));
	    }
	    catch (Exception e) {
		if (verboseMode)
		    e.printStackTrace();
		else
		    System.err.println(e.toString());
	    }
	}
	return ++i;
    }

    protected Generator getGenerator(ClassDef clazz) throws IOException {
	if (outputMode == MODE_STUBS) {
	    return new StubGenerator(this, clazz);
	}
	else {
	    return new HeaderGenerator(this, clazz);
	}
    }

    static String[] parseClassPath(String path) {
	char c = System.getProperty("path.separator").charAt(0);
	int ldlen = path.length();
	int i, j, n;
	// Count the separators in the path
	i = path.indexOf(c);
	n = 0;
	while (i >= 0) {
	    n++;
	    i = path.indexOf(c, i+1);
	}

	// allocate the array of paths - n :'s = n + 1 path elements
	String[] paths = new String[n + 1];

	// Fill the array with paths from the path
	n = i = 0;
	j = path.indexOf(c);
	while (j >= 0) {
	    if (j - i > 0) {
		paths[n++] = path.substring(i, j);
	    }
	    i = j + 1;
	    j = path.indexOf(c, i);
	}
	paths[n] = path.substring(i, ldlen);
	return paths;
    }

}

