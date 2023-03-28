/*
 * @(#)BatchEnvironment.java	1.50 95/11/12 Arthur van Hoff
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
 * Main environment of the batch version of the Java compiler,
 * this needs more work.
 */
public 
class BatchEnvironment extends Environment {
    /**
     * The stream where error message are printed.
     */
    OutputStream out;
    
    /**
     * The class path
     */
    ClassPath path;
    
    /**
     * A hashtable of resource contexts.
     */
    Hashtable packages = new Hashtable(31);

    /**
     * The classes, this is only a cache, not a
     * complete list.
     */
    Hashtable classes = new Hashtable(351);

    /**
     * flags
     */
    public int flags;

    /**
     * The number of errors and warnings
     */
    public int nerrors;
    public int nwarnings;

    /**
     * Constructor
     */
    public BatchEnvironment(ClassPath path) {
	this(System.out, path);
    }

    /**
     * Constructor
     */
    public BatchEnvironment(OutputStream out, ClassPath path) {
	this.out = out;
	this.path = path;
    }

    /**
     * Return flags
     */
    public int getFlags() {
        return flags;
    }

    /**
     * Return an enumeration of all the currently defined classes
     */
    public Enumeration getClasses() {
	return classes.elements();
    }

    /**
     * Get a class, given the fully qualified class name
     */
    public ClassDeclaration getClassDeclaration(Identifier nm) {
	ClassDeclaration c = (ClassDeclaration)classes.get(nm);
	if (c == null) {
	    classes.put(nm, c = new ClassDeclaration(nm));
	}
	return c;
    }

    /**
     * Check if a class exists
     */
    public boolean classExists(Identifier nm) {
	try {
	    return (classes.get(nm) != null) ||
		getPackage(nm.getQualifier()).classExists(nm.getName());
	} catch (IOException e) {
	    return true;
	}
    }

    /**
     * Get the package path for a packate
     */
    public Package getPackage(Identifier pkg) throws IOException {
	Package p = (Package)packages.get(pkg);
	if (p == null) {
	    packages.put(pkg, p = new Package(path, pkg));
	}
	return p;
    }
    
    /**
     * Parse a source file
     */
    public void parseFile(ClassFile file) throws FileNotFoundException {
	long tm = System.currentTimeMillis();
	InputStream input;
	BatchParser p;
	try {
	    input = file.getInputStream();
	    p = new BatchParser(new Environment(this, file),
				new BufferedInputStream(input));
	} catch(IOException ex) {
	    throw new FileNotFoundException();
	}

	try {
	    p.parseFile();
	} catch(Exception e) {
	    throw new CompilerError(e);
	}

	try {	
	    input.close();
	} catch (IOException ex) {
	    // We're turn with the input, so ignore this.
	}

	if (verbose()) {
	    tm = System.currentTimeMillis() - tm;
	    output("[parsed " + file.getPath() + " in " + tm + "ms]");
	}
    }

    /**
     * Load a binary file
     */
    BinaryClass loadFile(ClassFile file) throws IOException {
	long tm = System.currentTimeMillis();
	InputStream input = file.getInputStream();
	BinaryClass c = null;

	try {
	    DataInputStream is = 
		new DataInputStream(new BufferedInputStream(input));
	    c = BinaryClass.load(new Environment(this, file), is, 
				 loadFileFlags());
	} catch (ClassFormatError e) {
	    error(0, "class.format", file.getPath(), e.getMessage());
	    return null;
	} catch (Exception e) {
	    e.printStackTrace();
	}

	input.close();
	if (verbose()) {
	    tm = System.currentTimeMillis() - tm;
	    output("[loaded " + file.getPath() + " in " + tm + "ms]");
	}

	return c;
    }
    
    /**
     * Default flags for loadFile.  Subclasses may override this.
     */
    int loadFileFlags() { 
	return 0; 
    }

    /**
     * Load a binary class
     */
    boolean needsCompilation(Hashtable check, ClassDeclaration c) {
	switch (c.getStatus()) {
	  case CS_UNDEFINED:
	    loadDefinition(c);
	    return needsCompilation(check, c);

	  case CS_UNDECIDED:
	    if (check.get(c) == null) {
		check.put(c, c);

		BinaryClass bin = (BinaryClass)c.getClassDefinition();
		for (Enumeration e = bin.getDependencies() ; e.hasMoreElements() ;) {
		    ClassDeclaration dep = (ClassDeclaration)e.nextElement();
		    if (needsCompilation(check, dep)) {
			// It must be source, dependencies need compilation
			//output(c + " must be source, dependencies need compilation: " + dep);
			c.setDefinition(bin, CS_SOURCE);
			return true;
		    }
		}
	    }
	    return false;

	  case CS_BINARY:
	    return false;
	}
	return true;
    }

    /**
     * Load the definition of a class
     */
    public void loadDefinition(ClassDeclaration c) {
	switch (c.getStatus()) {
	  case CS_UNDEFINED: {
	    Identifier nm = c.getName();
	    Package pkg;
	    try {
		pkg = getPackage(nm.getQualifier());
	    } catch (IOException e) {
		error(0, "io.exception", c);
		return;
	    }
	    ClassFile binfile = pkg.getBinaryFile(nm.getName());
	    if (binfile == null) {
		// must be source, there is no binary
		//output(c + " must be source, there is no binary");
		c.setDefinition(null, CS_SOURCE);
		return;
	    }

	    ClassFile srcfile = pkg.getSourceFile(nm.getName());
	    if (srcfile == null) {
		BinaryClass bc = null;
		try {
		    bc = loadFile(binfile);
		} catch (IOException e) {
		    error(0, "io.exception", binfile);
		    return;
		}
		if ((bc != null) && !bc.getName().equals(nm)) {
		    error(0, "wrong.class", binfile.getPath(), c, bc);
		    bc = null;
		}
		if (bc == null) {
		    // no source nor binary found
		    //output(c + " is not found, no source nor binary");
		    c.setDefinition(null, CS_NOTFOUND);
		    return;
		}

		// Couldn't find the source, try the one mentioned in the binary
		if (bc.getSource() != null) {
		    srcfile = new ClassFile(new File((String)bc.getSource()));
		    // Look for the source file
		    srcfile = pkg.getSourceFile(srcfile.getName());
		    if ((srcfile != null) && srcfile.exists()) {
			if (srcfile.lastModified() > binfile.lastModified()) {
			    // must be source, it is newer than the binary
			    //output(c + " must be source, source is newer");
			    c.setDefinition(bc, CS_SOURCE);
			    return;
			}
			c.setDefinition(bc, dependencies() ? CS_UNDECIDED : CS_BINARY);
			return;
		    }
		} 

		// It must be binary, there is no source
		//output(c + " must be binary, there is no source");
		c.setDefinition(bc, CS_BINARY);
		return;
	    }
	    BinaryClass bc = null;
	    try {
		if (srcfile.lastModified() > binfile.lastModified()) {
		    // must be source, it is newer than the binary
		    //output(c + " must be source, source is younger");
		    c.setDefinition(null, CS_SOURCE);
		    return;
		}
		bc = loadFile(binfile);
	    } catch (IOException e) {
		error(0, "io.exception", binfile);
	    }
	    if ((bc != null) && !bc.getName().equals(nm)) {
		error(0, "wrong.class", binfile.getPath(), c, bc);
		bc = null;
	    }
	    if (bc != null) {
		if (bc.getName().equals(c.getName())) {
		    c.setDefinition(bc, dependencies() ? CS_UNDECIDED : CS_BINARY);
		} else {
		    c.setDefinition(null, CS_NOTFOUND);
		    getClassDeclaration(bc.getName()).setDefinition(bc, dependencies() ? CS_UNDECIDED : CS_BINARY);
		}
	    } else {
		c.setDefinition(null, CS_NOTFOUND);
	    }
	    return;
	  }
	    
	  case CS_UNDECIDED: {
	    Hashtable tab = new Hashtable();
	    if (!needsCompilation(tab, c)) {
		// All undecided classes that this class depends on must be binary
		for (Enumeration e = tab.keys() ; e.hasMoreElements() ; ) {
		    ClassDeclaration dep = (ClassDeclaration)e.nextElement();
		    if (dep.getStatus() == CS_UNDECIDED) {
			// must be binary, dependencies need compilation
			//output(dep + " must be binary, dependencies need no compilation");
			dep.setDefinition(dep.getClassDefinition(), CS_BINARY);
		    }
		}
	    }
	    return;
	  }

	  case CS_SOURCE: {
	    ClassFile srcfile = null;
	    if (c.getClassDefinition() != null) {
		// Use the source file name from the binary class file
		Package pkg = null;
		try {
		    pkg = getPackage(c.getName().getQualifier());
		    srcfile = pkg.getSourceFile((String)c.getClassDefinition().getSource());
		} catch (IOException e) {
		    error(0, "io.exception", c);
		}
		if (srcfile == null) {
		    String fn = (String)c.getClassDefinition().getSource();
		    srcfile = new ClassFile(new File(fn));
		}
	    } else {
		// Get a source file name from the package
		Identifier nm = c.getName();
		try {
		    srcfile = getPackage(nm.getQualifier()).getSourceFile(nm.getName());
		} catch (IOException e)  {
		    error(0, "io.exception", c);
		}
		if (srcfile == null) {
		    // not found, there is no source
		    //output(c + " is not found, no source");
		    c.setDefinition(null, CS_NOTFOUND);
		    return;
		}
	    }
	    try {
		parseFile(srcfile);
	    } catch (FileNotFoundException e) {
		error(0, "io.exception", srcfile);
	    }
	    
	    if ((c.getClassDefinition() == null) || (c.getStatus() == CS_SOURCE)) {
		// not found after parsing the file
		c.setDefinition(null, CS_NOTFOUND);
	    } 
	    return;
	  }
	}
    }

    /**
     * True if the properties have been loaded.
     */
    boolean propsLoaded;

    /**
     * Only load the properties when needed.
     */
    String getProperty(String nm) {
	if (!propsLoaded) {
	    propsLoaded = true;
	    String props = System.getProperty("java.home") + 
		System.getProperty("file.separator") + "lib" +
		System.getProperty("file.separator") + "javac.properties";
	    try {
		Properties p = new Properties(System.getProperties());
		InputStream in = new BufferedInputStream(new FileInputStream(props));
		p.load(in);
		in.close();
		System.setProperties(p);
	    } catch (IOException e) {
		output("failed to read: " + props);
	    }
	}
	return System.getProperty(nm);
    }

    /**
     * Error String
     */
    String errorString(String err, Object arg1, Object arg2, Object arg3) {
	String str = null;

	SecurityManager.setScopePermission();
	if (err.startsWith("warn.")) {
	    str = getProperty("javac." + err);
	} else {
	    str = getProperty("javac.err." + err);
	}
	SecurityManager.resetScopePermission();
	if (str == null) {
	    return "error message '" + err + "' not found";
	}
	
	StringBuffer buf = new StringBuffer();
	for (int i = 0 ; i < str.length() ; i++) {
	    char c = str.charAt(i);
	    if ((c == '%') && (i + 1 < str.length())) {
		switch (str.charAt(++i)) {
		  case 's':
		    String arg = arg1.toString();
		    for (int j = 0 ; j < arg.length() ; j++) {
			switch (c = arg.charAt(j)) {
			  case ' ':
			  case '\t':
			  case '\n':
			  case '\r':
			    buf.append((char)c);
			    break;

			  default:
			    if ((c > ' ') && (c <= 255)) {
				buf.append((char)c);
			    } else {
				buf.append('\\');
				buf.append('u');
				buf.append(Integer.toString(c, 16));
			    }
			}
		    }
		    arg1 = arg2;
		    arg2 = arg3;
		    break;
		    
		  case '%':
		    buf.append('%');
		    break;

		  default:
		    buf.append('?');
		    break;
		}
	    } else {
		buf.append((char)c);
	    }
	}
	return buf.toString();
    }

    /**
     * The filename where the last errors have occurred
     */
    String errorFileName;

    /**
     * List of outstanding error messages
     */
    ErrorMessage errors;

    /**
     * Insert an error message in the list of outstanding error messages.
     * The list is sorted on input position.
     */
    void insertError(int where, String message) {
	//output("ERR = " + message);
	ErrorMessage msg = new ErrorMessage(where, message);
	if (errors == null) {
	    errors = msg;
	} else if (errors.where > where) {
	    msg.next = errors;
	    errors = msg;
	} else {
	    ErrorMessage m = errors;
	    for (; (m.next != null) && (m.next.where <= where) ; m = m.next);
	    msg.next = m.next;
	    m.next = msg;
	}
    }

    /**
     * Flush outstanding errors
     */
    public void flushErrors() {
	if (errors == null) {
	    return;
	}
	
	try {
	    // Read the file
	    FileInputStream in = new FileInputStream(errorFileName);
	    byte data[] = new byte[in.available()];
	    in.read(data);
	    in.close();

	    // Report the errors
	    for (ErrorMessage msg = errors ; msg != null ; msg = msg.next) {
		int ln = msg.where >>> OFFSETBITS;
		int off = msg.where & ((1 << OFFSETBITS) - 1);
		
		int i, j;
		for (i = off ; (i > 0) && (data[i - 1] != '\n') && (data[i - 1] != '\r') ; i--);
		for (j = off ; (j < data.length) && (data[j] != '\n') && (data[j] != '\r') ; j++);

		String prefix = errorFileName + ":" + ln + ":";
		output(prefix + " " + msg.message);
		output(new String(data, 0, i, j - i));

		char strdata[] = new char[(off - i) + 1];
		for (j = i ; j < off ; j++) {
		    strdata[j-i] = (data[j] == '\t') ? '\t' : ' ';
		}
		strdata[off-i] = '^';
		output(new String(strdata));
	    }
	} catch (IOException e) {
	    output("I/O exception");
	}
	errors = null;
    }

    /**
     * Report error
     */
    void reportError(Object src, int where, String err, String msg) {
	if (src == null) {
	    if (errorFileName != null) {
		flushErrors();
		errorFileName = null;
	    }
	    output("error: " + msg);
	    nerrors++;

	} else if (src instanceof String) {
	    String fileName = (String)src;

	    if (!fileName.equals(errorFileName)) {
		flushErrors();
		errorFileName = fileName;
	    }
	    if (err.startsWith("warn.")) {
		nwarnings++;
	    } else {
		nerrors++;
	    }
	    insertError(where, msg);

	} else if (src instanceof ClassFile) {
	    reportError(((ClassFile)src).getPath(), where, err, msg);

	} else if (src instanceof Identifier) {
	    reportError(src.toString(), where, err, msg);

	} else if (src instanceof ClassDeclaration) {
	    try {
		reportError(((ClassDeclaration)src).getClassDefinition(this), where, err, msg);
	    } catch (ClassNotFound e) {
		reportError(((ClassDeclaration)src).getName(), where, err, msg);
	    }
	} else if (src instanceof ClassDefinition) {
	    ClassDefinition c = (ClassDefinition)src;
	    if (!err.startsWith("warn.")) {
		c.setError(true);
	    } 
	    reportError(c.getSource(), where, err, msg);

	} else if (src instanceof FieldDefinition) {
	    reportError(((FieldDefinition)src).getClassDeclaration(), where, err, msg);

	} else {
	    output(src + ":error=" + err + ":" + msg);
	}
    }

    /**
     * Issue an error
     */
    public void error(Object source, int where, String err, Object arg1, Object arg2, Object arg3) {
	reportError(source, where, err, errorString(err, arg1, arg2, arg3));
    }

    /**
     * Output a string. This can either be an error message or something
     * for debugging. This should be used instead of println.
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
}
