/*
 * @(#)DocumentationGenerator.java	1.39 95/12/07 Arthur van Hoff
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

import java.util.*;
import java.io.*;
import sun.tools.java.*;
import sun.tools.javac.SourceClass;
import sun.tools.javac.BatchEnvironment;
import sun.tools.tree.LocalField;

abstract class DocumentationGenerator implements Constants {
    BatchEnvironment env;

   
    /**
     * Initialize the documentation generator 
     */
    public DocumentationGenerator init() {
	this.env = Main.env;
	return this;
    }

    /** 
     * Return true if we should document the given class
     */
    static boolean shouldDocument(ClassDefinition c) {
	return c.isPublic();
    }

    /** 
     * Return true if we should document the given field
     */
    static boolean shouldDocument(FieldDefinition f) {
	return f.isPublic() || f.isProtected();
    }

    /**
     * Return all variables defined by a given class.
     */
    static Vector allVariables(ClassDefinition def) {
	Vector v = new Vector();
	for (FieldDefinition f = def.getFirstField() ; 
	           f != null ; f = f.getNextField()) {
	     if (f.isVariable() && shouldDocument(f)) {
		v.addElement(f);
	     }
	}
	return v;
    }

    /**
     * Return all constructors defined by a given class.
     */
    static Vector allConstructors(ClassDefinition def) {
	Vector v = new Vector();
	for (FieldDefinition f = def.getFirstField() ; 
	        f != null ; f = f.getNextField()) {
	    if (f.isConstructor() && shouldDocument(f)) {
		v.addElement(f);
	    }
	}
	return v;
    }

    /**
     * Return all methods defined by a given class.
     */
    static Vector allMethods(ClassDefinition def) {
	Vector v = new Vector();
	for (FieldDefinition f = def.getFirstField() ; 
	        f != null ; f = f.getNextField()) {
	    if (f.isMethod() && (!f.isConstructor()) && shouldDocument(f)) {
		v.addElement(f);
	    }
	}
	return v;
    }

    /**
     * Given a vector of fields (methods, constructors, or variables) return
     * only those that are explicitly defined by this class.
     */
    static Vector localFieldsOf(ClassDefinition c, Vector fields) { 
	Vector v = new Vector();
	for (Enumeration e = fields.elements(); e.hasMoreElements(); ) { 
	    FieldDefinition f = (FieldDefinition)e.nextElement();
	    if (f.getClassDefinition().equals(c) &&
		f.getClassDeclaration() == f.getDefiningClassDeclaration()) {
		v.addElement(f);
	    }
	}
	return v;
    }

    /**
     * return a Vector of the class tree of this class.  It will start at
     * java/lang/Object and continue to this class
     */
    Vector superclassesOf(ClassDeclaration c) {
	if (c == null) 
	    return new Vector();
	try { 
	    ClassDeclaration superClass = 
		  c.getClassDefinition(env).getSuperClass();
	    Vector result = superclassesOf(superClass);
	    result.addElement(c);
	    return result;
	} catch (ClassNotFound e) {
	    System.err.println("Warning: Couldn't find superclass info for " + 
			       c.getName());
	    return new Vector();
	}
    }
    
    /** 
     * Given a documentaiton string, massage it into a vector
     */
    static Vector mergeDoc(String doc) {
	Vector result = new Vector();
	for (StringTokenizer e = new StringTokenizer(doc, "\n"); 
	               e.hasMoreTokens(); ) {
	    String tok = e.nextToken();
	    if (tok.trim().length() == 0) continue;
	    if (tok.charAt(0) == '@') { 
		result.addElement(tok);
	    } else if (result.size() == 0) {
		result.addElement("@comment " + tok);
	    } else {
		int lastIndex = result.size() - 1;
		result.setElementAt(result.elementAt(lastIndex) + "\n" + tok, 
				    lastIndex);
	    }
	}
	return result;
    }

    abstract String commentString(String str);
    abstract String returnString(String str);
    abstract String versionString(String str);
    abstract String authorString(String str);

    /** Return the comment part of the documentation vector */
    String getComment(Vector docList) { 
	for (Enumeration e = docList.elements(); e.hasMoreElements(); ) { 
	    String tok = (String)e.nextElement();
	    if (tok.startsWith("@comment")) 
		return commentString(tok.substring(8).trim());
	}
	return null;
    }

    /** Return the returns part of the documentation vector */
    String getReturn(Vector docList) {
	String result = null;
	for (Enumeration e = docList.elements(); e.hasMoreElements(); ) { 
	    String tok = (String)e.nextElement();
	    if (tok.startsWith("@return")) {
		tok = tok.substring(7).trim();
		if (tok.length() == 0) { 
		    System.out.println("warning: empty @return statement.");
		} else if (result == null) {
		    result = returnString(tok);
		} else {
		    System.out.println("warning: multiple @return statements.");
		}
	    }
	}
	return result;
    }

    /** Return the version part of the documentation vector */
    String getVersion(Vector docList) { 
	if (!Main.showVersion) 
	    return null;
	String result = null;
	for (Enumeration e = docList.elements(); e.hasMoreElements(); ) { 
	    String tok = (String)e.nextElement();
	    if (tok.startsWith("@version")) {
		tok = tok.substring(8).trim();
		if (tok.length() == 0) { 
		    System.out.println("warning: empty @version statement.");
		} else if (result == null) { 
		    result = versionString(tok);
		} else { 
		   System.out.println("warning: multiple @version statements.");
		}
	    }
	} 
	return result;
    }


    /**
     * return the "See Also" part of the documentation vector.  If there are
     * multiple "see also"s, separate them by a comma, or whatever
     * seeStringSeparation() returns.
     */
    Vector getSees(Vector docList, ClassDeclaration currentClass) { 
	Vector result = null;
	for (Enumeration e = docList.elements(); e.hasMoreElements(); ) { 
	    String tok = (String)e.nextElement();
	    if (tok.startsWith("@see")) {
		tok = tok.substring(4).trim();
		if (tok.length() == 0) { 
		    System.out.println("warning: empty @see statement.");
		} else { 
		    if (result == null) 
			result = new Vector();
		    parseSeeString(tok, currentClass, result);
		}
	    }
	} 
	return result;
    }


    /**
     * return the "Authors" part of the documentation vector.  If there are
     * multiple authors, separate them by a comma.
     */
    String getAuthors(Vector docList) { 
	if (!Main.showAuthors) 
	    return null;
	String result = null;
	for (Enumeration e = docList.elements(); e.hasMoreElements(); ) { 
	    String tok = (String)e.nextElement();

	    if (tok.startsWith("@author")) { 
		tok = tok.substring(7).trim();
		if (tok.length() == 0) { 
		    System.out.println("warning: empty @author statement.");
		    continue;
		} else if (result == null) {
		    result = authorString(tok);
		} else {
		    result += ", " + authorString(tok);
		}
	    }
	}
	return result;
    } 
	
    /** 
     * return a vector of the parameters and their descriptions from the
     * documentation vector.
     */
    static Vector getParameters(Vector docList) { 
	Vector result = null;
	for (Enumeration e = docList.elements(); e.hasMoreElements(); ) { 
	    String tok = (String)e.nextElement();
	    if (tok.startsWith("@param")) { 
		int i = 0;
		tok = tok.substring(6).trim();
		for (; i < tok.length() ; i++) {
		    if ((tok.charAt(i) == ' ') || (tok.charAt(i) == '\t')) {
			break;
		    }
		}
		if (i < tok.length()) { 
		    if (result == null) 
			result = new Vector();
		    result.addElement(tok.substring(0, i));
		    result.addElement(tok.substring(i).trim());
		} else { 
		    System.out.println("warning: empty @param statement.");
		}
	    }
	}
	return result;
    }
	
    private static final Identifier idIOException 
                  = Identifier.lookup("java.io.IOException");
    /** 
     * return a vector of the exceptions and their descriptions from the
     * documentation vector.
     */
    Vector getThrows(Vector docList, FieldDefinition f) { 
	Vector result = new Vector();
	for (Enumeration e = docList.elements(); e.hasMoreElements(); ) { 
	    String tok = (String)e.nextElement();
	    if (tok.startsWith("@exception")) { 
		int i = 0;
		tok = tok.substring(10).trim();
		for (; i < tok.length() ; i++) {
		    if ((tok.charAt(i) == ' ') || (tok.charAt(i) == '\t')) {
			break;
		    }
		}
		if (i >= tok.length()) { 
		    System.out.println("warning: empty @exception statement.");
		    continue;
		}
		Identifier exceptionID = Identifier.lookup(tok.substring(0, i));
		ClassDeclaration ee;
		try {
		    ee = env.getClassDeclaration(((SourceClass)f.getClassDefinition()).getImports().resolve(env, exceptionID));
		} catch (ClassNotFound eee) {
		    ee = env.getClassDeclaration(exceptionID);
		}
		result.addElement(ee);
		result.addElement(tok.substring(i).trim());
	    }
	}

	ClassDeclaration ignore1 = env.getClassDeclaration(idJavaLangError);
	ClassDeclaration ignore2 = 
                        env.getClassDeclaration(idJavaLangRuntimeException);
	ClassDeclaration ignore3 = 
	                env.getClassDeclaration(idIOException);	// too many!

	ClassDeclaration[] realThrows = f.getExceptions(env);
    outer_loop:
	for (int i = 0; i < realThrows.length; i++) { 
	    ClassDeclaration realThrow = realThrows[i];
	    try { 
		ClassDefinition def = realThrow.getClassDefinition(env);
		if (def.subClassOf(env, ignore1)) continue;
		if (def.subClassOf(env, ignore2)) continue;
		if (def.subClassOf(env, ignore3)) continue;
		for (int j = 0; j < result.size(); j += 2) {
		    if (def.subClassOf(env, 
				       (ClassDeclaration)result.elementAt(j)))
			continue outer_loop;
		}
		System.out.println("Method " + f + " declares that it throws "
				   + def + " but there is no documentation");
	    } catch (ClassNotFound e) { 
		System.out.println("Couldn't find " + e.name);
	    }
	}
    outer_loop:
	for (int i = 0; i < result.size(); i += 2) {
	    ClassDeclaration declThrow = (ClassDeclaration)result.elementAt(i);
	    try { 
		ClassDefinition def = declThrow.getClassDefinition(env);
		if (def.subClassOf(env, ignore1)) continue;
		if (def.subClassOf(env, ignore2)) continue;
		if (def.subClassOf(env, ignore3)) continue;
		for (int j = 0; j < realThrows.length; j++) { 
		    if (def.subClassOf(env, realThrows[j])) 
			continue outer_loop;
		}
		System.out.println("Method " + f + " documents that it throws "
				   + declThrow + 
				   " but there is no declaration");
	    } catch (ClassNotFound e) { 
		System.out.println("Couldn't find " + e.name);
	    }
	}
	if (result.size() == 0) 
	    return null;
	return result;
    }

    /** Return the method that this method overrides, if any. */
    FieldDefinition getOverride(FieldDefinition f) { 
	FieldDefinition overrides = null;
	try {
  	    if ((!f.isStatic()) && (!f.isConstructor()) && 
		(f.getClassDefinition().getSuperClass() != null)) {
	        overrides = f.getClassDefinition().getSuperClass().
		                 getClassDefinition(env).
		                 findMethod(env, f.getName(), f.getType());
	    }
	} catch (ClassNotFound ex) {
	    overrides = null;
	}
	return overrides;
    }

    /** return true if this class is an exception. */
    static ClassDeclaration exDecl;
    boolean isException(ClassDeclaration c) {
	if (exDecl == null) {
	    Identifier exceptionID = Identifier.lookup("java.lang.Exception");
	    exDecl = env.getClassDeclaration(exceptionID);
	}
	try {
	    return c.getClassDefinition(env).subClassOf(env, exDecl);
	} catch (ClassNotFound ex) {
	    System.err.println("Warning: Couldn't find class info for " + 
			       exDecl.getName());

	    return (false);
	}
    }
 
    /** return true if this class is an error. */
    static ClassDeclaration errorDecl;
    boolean isError(ClassDeclaration c) {
	if (errorDecl == null) {
	    Identifier errorID = Identifier.lookup("java.lang.Error");
	    errorDecl = env.getClassDeclaration(errorID);
	}
	try {
	    return c.getClassDefinition(env).subClassOf(env, errorDecl);
	} catch (ClassNotFound ex) {
	    System.err.println("Warning: Couldn't find class info for " + 
			       errorDecl.getName());
	    return false;
	}
    }
 
    static PrintStream openFile(String name) {
	if (Main.destDir != null) {
	    name = Main.destDir.getPath() + File.separator + name;
	}

	FileOutputStream file;
	try {
	    file = new FileOutputStream(name);
	} catch (IOException ee) {
	    try {
	        new File(new File(name).getParent()).mkdirs();
	        file = new FileOutputStream(name);
	    } catch (IOException ex) {
		throw new Error("Can't open output file");
	    }
	}
	return new PrintStream(new BufferedOutputStream(file));
    }

    /** return the first sentence of the documentation of a class */
    String firstSentence(ClassDefinition c) {
	return firstSentence(c.getDocumentation());
    }

    /** return the first sentence of the documentation of a field */
    String firstSentence(FieldDefinition f) {
	return firstSentence(getDocumentation(f));
    }

    // return the first sentence of a string.
    private String firstSentence(String s) {
	if (s == null)
	    return "";
	int len = s.length();
	boolean period = false;
	for (int i = 0 ; i < len ; i++) {
	    switch (s.charAt(i)) {
	      case '.':
		period = true;
		break;
	      case '@':
		return s.substring(0, i);
	      case ' ':
	      case '\t':
	      case '\n':
		if (period) {
		    return s.substring(0, i);
		}
		break;
	      default:
		period = false;
	    }
	}
	return s;
    }

    /** Get the documentation for a field. */
    String getDocumentation(FieldDefinition f) {
	String doc = f.getDocumentation();
	if ((doc != null) || f.isVariable() || f.isConstructor()) {
	    return doc;
	}
	while ((f != null) && (doc == null)) {
	    ClassDeclaration sc = f.getClassDefinition().getSuperClass();
	    if (sc == null)
		break;
	    try {
	        f = sc.getClassDefinition(env).findMethod(env, f.getName(), f.getType());
	    } catch (ClassNotFound ex) {
		System.err.println("Warning: Couldn't find class info for " + sc.getName());
		f = null;

	    }
	    if (f != null) {
		doc = f.getDocumentation();
	    }
	}
	return (doc != null) ? firstSentence(doc) : null;
    }


    /**  return a string corresponding to a type, sans [] at the end */
    String typeString(Type t) {
	switch (t.getTypeCode()) {
	  case TC_VOID:		return "void";
	  case TC_BOOLEAN:	return "boolean";
	  case TC_BYTE:		return "byte";
	  case TC_CHAR:		return "char";
	  case TC_SHORT:	return "short";
	  case TC_INT:		return "int";
	  case TC_LONG:		return "long";
	  case TC_FLOAT:	return "float";
	  case TC_DOUBLE:	return "double";
	  case TC_CLASS:	return classString(env.getClassDeclaration(t));
	  case TC_ARRAY:	return typeString(t.getElementType());
	}
	return "error";
    }

    /** Return a string of []'s corresponding to the depth of the array */
    String typeArrayString(Type t) {
        String result = "";
	while (t.getTypeCode() == TC_ARRAY) {
	    result += "[]";
	    t = t.getElementType();
	}
	return result;
    }


    // return a string corresponding to the modifiers of a field
    static String modString(FieldDefinition f) {
	String str;
	if (f.isPublic()) {
	    str = "  public";
	} else if (f.isProtected()) {
	    str = "  protected";
	} else if (f.isPrivate()) {
	    str = "  private";
	} else {
	    str = "";
	}
	if (f.isFinal()) {
	    str += (str.length() > 0) ? " final" : "final";
	}
	if (f.isStatic()) {
	    str += (str.length() > 0) ? " static" : "static";
	}
	if (f.isSynchronized()) {
	    str += (str.length() > 0) ? " synchronized" : "synchronized";
	}
	if (f.isAbstract()) {
	    str += (str.length() > 0) ? " abstract" : "abstract";
	}
	/*
	if (f.isNative()) {
	    str += (str.length() > 0) ? " native" : "native";
	}
	*/
	if (f.isVolatile()) {
	    str += (str.length() > 0) ? " volatile" : "volatile";
	}
	if (f.isTransient()) {
	    str += (str.length() > 0) ? " transient" : "transient";
	}
	return str;
    }


    /** Sort an array of class declarations */
    static void sort(ClassDeclaration c[]) {
	boolean done;
	do {
	    done = true;
	    for (int i = c.length - 1 ; i > 0 ; i--) {
		if (compare(c[i-1], c[i]) > 0) {
		    ClassDeclaration decl = c[i];
		    c[i] = c[i-1];
		    c[i-1] = decl;
		    done = false;
		}
	    }
	} while (!done);
    }

    /** 
     * Compare the name of two classes.  Return <0, =0, or >0 depending
     * on how they sort.
     */
    static private int compare(ClassDeclaration a, ClassDeclaration b) { 
	Identifier aa = a.getName();
	Identifier bb = b.getName();
	int compTo = aa.getName().toString().compareTo(bb.getName().toString());
	if (compTo == 0) 
	    return aa.getQualifier().toString().
		         compareTo(bb.getQualifier().toString());
	else 
	    return compTo;
    }



    static Identifier debugID = Identifier.lookup("debug");

    /** 
     * Generate package documentation for a vector of package names, each
     * one represented by a string.
     * This routine sorts them into two categories, based on whether they
     * are "debugging" or not.  Then calls the abstract version 
     * of genPackagesDocumention on the two categories.
     */
    void genPackagesDocumentation(Vector packages) { 
	Vector v1 = new Vector();
	Vector v2 = new Vector();
	for (Enumeration e = packages.elements(); e.hasMoreElements(); ) { 
	    String s = (String)e.nextElement();
	    Identifier pkg = Identifier.lookup(s);
	    if (pkg.getName() != debugID) { 
		v1.addElement(s);
	    } else {
		v2.addElement(s);
	    }
	}
	String[] vv1 = null, vv2 = null;
	if (v1.size() > 0) { 
	    vv1 = new String[v1.size()];
	    v1.copyInto(vv1);
	    sort(vv1);
	} 
	if (v2.size() > 0) { 
	    vv2 = new String[v2.size()];
	    v2.copyInto(vv2);
	    sort(vv2);
	}
	genPackagesDocumentation(vv1, vv2);
    }

    abstract void genPackagesDocumentation(String v1[], String v2[]);


    /**
     * Generate package documention for a single package.  This ends up
     * calling the abstract version of genPackageDocumention, with the
     * classes of the package sorted into categories.
     */
    void genPackageDocumentation(Identifier pkg) {
	int intfCount = 0;
	int classCount = 0;
	int exceptCount = 0; 
	int errorCount = 0; 
	// Make sure that every class definition that ever is going to be 
	// loaded is loaded now.
	while (true) {
	    boolean changed = false;
	    for (Enumeration e = env.getClasses() ; e.hasMoreElements() ;) {
		ClassDeclaration decl = (ClassDeclaration)e.nextElement();
		if (!decl.isDefined()) {
		    try {
		        decl.getClassDefinition(env);
		        changed = true;
		    } catch (ClassNotFound ex) {
		    }
		}
	    }
	    if (!changed) 
		break;
	}
	for (Enumeration e = env.getClasses() ; e.hasMoreElements() ;) {
	    ClassDeclaration decl = (ClassDeclaration)e.nextElement();
	    try {
		ClassDefinition defn = decl.getClassDefinition(env);
	        if (shouldDocument(defn) &&
		    decl.getName().getQualifier().equals(pkg)) {
		    if (decl.getClassDefinition(env).isInterface()) {
		        intfCount++;
		    } else if (isException(decl)) {
		        exceptCount++;
		    } else if (isError(decl)) {
		        errorCount++;
		    } else {
		        classCount++;
		    }
		}
	    } catch (ClassNotFound ex) {
		// We've already complained about this, so be silent here.
	    }
	}
	ClassDeclaration intfDecls[] = new ClassDeclaration[intfCount];
	ClassDeclaration classDecls[] = new ClassDeclaration[classCount];
	ClassDeclaration exceptDecls[] = new ClassDeclaration[exceptCount];
	ClassDeclaration errorDecls[] = new ClassDeclaration[errorCount];
	for (Enumeration e = env.getClasses() ; e.hasMoreElements() ;) {
	    ClassDeclaration decl = (ClassDeclaration)e.nextElement();
	    try {
	        if (shouldDocument(decl.getClassDefinition(env)) &&
		    decl.getName().getQualifier().equals(pkg)) {
		    if (decl.getClassDefinition(env).isInterface()) {
		        intfDecls[--intfCount] = decl;
		    } else if (isException(decl)) {
		        exceptDecls[--exceptCount] = decl;
		    } else if (isError(decl)) {
		        errorDecls[--errorCount] = decl;
		    } else {
		        classDecls[--classCount] = decl;
		    }
	        }
	    } catch (ClassNotFound ex) {
		// We've already complained about this, so be silent here.
	    }	}

	genPackageDocumentation(pkg, intfDecls, classDecls, 
				exceptDecls, errorDecls);
    }



    /** 
     * Generate the documentation for a single package.  The classes in the 
     * package have been divided into four categories.
     */
    abstract void genPackageDocumentation(Identifier pkg, 
					  ClassDeclaration intfDecls[], 
					  ClassDeclaration classDecls[], 
					  ClassDeclaration exceptDecls[], 
					  ClassDeclaration errorDecls[]);



    /**
     * Generate the documentation for a single class.  Also pass the 
     * declaraton of the previous and next class in the package, in case the
     * page wants to make "buttons"
     */
    abstract void genClassDocumentation(ClassDefinition c, 
					ClassDeclaration prev, 
					ClassDeclaration next);

    /**
     * typeString() calls this function for classes.  In case you want to
     * output something fancy for a class declaration.  Like a hyperlink.
     */
    abstract String classString(ClassDeclaration c);


    /* Sort an array of Field Definitions.  This is a bad implementation
     * of bubble sort, but it's fast enough, since we're usually only talking
     * about a small aray.
     */
    static void sort(FieldDefinition f[]) {
	boolean done;
	do {
	    done = true;
	    for (int i = f.length - 1 ; i > 0 ; i--) {
		if (f[i - 1].getName().toString()
		        .compareTo(f[i].getName().toString()) > 0) {
		    FieldDefinition def = f[i];
		    f[i] = f[i-1];
		    f[i-1] = def;
		    done = false;
		}
	    }
	} while (!done);
    }

    /* Sort an array of Strings.  This is a bad implementation
     * of bubble sort, but it's fast enough, since we're usually only talking
     * about a small aray.
     */

    static void sort(String f[]) {
	boolean done;
	do {
	    done = true;
	    for (int i = f.length - 1 ; i > 0 ; i--) {
		if (f[i - 1].compareTo(f[i]) > 0) {
		    String  def = f[i]; f[i] = f[i-1]; f[i-1] = def;
		    done = false;
		}
	    }
	} while (!done);
    }


    /**
     * Given a field, generate a String sort key for it.
     */
    static String sortKey(FieldDefinition f) { 
	ClassDefinition c = f.getClassDefinition();
	String cstring = (c.getName().getName() + "." + 
			    c.getName().getQualifier()).toLowerCase();
	if (f.isVariable()) {
	    return f.getName().toString().toLowerCase() + " " + cstring;
	} else { 
	    Identifier name = 
		(f.isConstructor() 
		     ? f.getClassDeclaration().getName().getName()
		     : f.getName());
	    Type type = f.getType();
	    return type.typeString(name.toString(), true, false).toLowerCase()
		    + cstring;
	}
    }


    /**
     * Quick sort an array of fields and a corresponding array of Strings
     * that are the sort keys for the field.
     */
    static void xsort(FieldDefinition ff[], String ss[]) {
	xsort(ff, ss, 0, ff.length - 1);
    }

    // the actual quick sort implementation
    private
    static void xsort(FieldDefinition ff[], String ss[], int left, int right) {
	if (left >= right) 
	    return;
	String pivot = ss[left];
	int l = left;
	int r = right;
	while (l < r) { 
	    while (l <= right && ss[l].compareTo(pivot) <= 0) 
		l++;
	    while (r >= left && ss[r].compareTo(pivot) > 0) 
		r--;
	    if (l < r) { 
		// swap items at l and at r
		FieldDefinition def = ff[l]; 
		String name = ss[l];
		ff[l] = ff[r]; ff[r] = def;
		ss[l] = ss[r]; ss[r] = name;
	    }
	}
	int middle = r;
	// swap left and middle
	FieldDefinition def = ff[left]; 
	String name = ss[left];
	ff[left] = ff[middle]; ff[middle] = def;
	ss[left] = ss[middle]; ss[middle] = name;
	xsort(ff, ss, left, middle-1);
	xsort(ff, ss, middle + 1, right);
    }

    /**
     * Generate a hash table in which every class has, as its entry, 
     * a vector of all its subclasses.  If a class has no subclasses, 
     * then it will have no entry. 
     */
    Hashtable createClassTree() {
	Hashtable result = new Hashtable();
	// look at every known class
	for (Enumeration e = env.getClasses() ; e.hasMoreElements() ;) {
	    ClassDeclaration decl = (ClassDeclaration)e.nextElement();
	    try {
	        if (shouldDocument(decl.getClassDefinition(env)))
		    // indicate that this class is a child of its parent.
		    // also indicate that its parent is a child of its
		    // grandparent, and so forth.
		    genClassTree(decl, result);
	    } catch (ClassNotFound ex) {
		// We've already complained about this, so be silent here.
	    }
	}
	for (Enumeration e = result.keys(); e.hasMoreElements(); ) { 
	    // Look at the children of each class.  Sort them into order.
	    // Also remove the vector corresponding to any childless classes
	    ClassDeclaration c = (ClassDeclaration)e.nextElement();
	    Vector children = (Vector)result.get(c);
	    if (children.size() > 0) { 
		ClassDeclaration cc[] = new ClassDeclaration[children.size()];
		children.copyInto(cc);
		sort(cc);
		result.put(c, cc);
	    } else {
		result.remove(c);
	    }
	}
	return result;
    }

    // At decl to the children of its parent, if we haven't yet seen this
    // class before.  Also, add the parent to its grandparent, etc.
    // Return the vector corresponding to this nodes children
    private Vector genClassTree(ClassDeclaration decl, Hashtable h) { 
	Vector children = (Vector) h.get(decl);
	if (children != null) 
	    // we've already seen this node.
	    return children;
	children = new Vector();
	// This new vector corresponds to my children.
	h.put(decl, children);
	try { 
	    ClassDeclaration superClass = 
		    decl.getClassDefinition(env).getSuperClass();
	    if (superClass != null) { 
		// Add me to my parent's children.
		Vector superChildren = genClassTree(superClass, h);
		superChildren.addElement(decl);
	    }
	} catch (ClassNotFound e) {
	    // We've already complained about this, so be silent here.
	}
	return children;
    }

    /**
     * Create an index of all the fields, if that is appropriate.
     */

    abstract void genFieldIndex();

    /**
     * Create a class hierarchy.
     * Call an abstract method with the two arguments
     *    1) the hash table representing the hierarchy.
     *    2) the class declaration for java.lang.Object, the root of the tree
     */

    void genClassTree() {
	genClassTree(createClassTree(), 
		     env.getClassDeclaration(Type.tObject.getClassName()));
    }

    /**
     * The abstract method that prints out the class tree.
     */
    abstract void genClassTree(Hashtable tree, ClassDeclaration objectDecl);


    void parseSeeString(String string, ClassDeclaration currentClass,
			Vector result) { 
	ClassDeclaration decl;
	String what, where;

	string = string.trim();
	int i = string.indexOf('#');
	if (i >= 0) {
	    what = string.substring(i + 1); // the tag
	    string = string.substring(0, i);   // the class
	} else { 
	    what = null;	         // no tag, str contains class
	}
	if (string.length() > 0) { 
	    try {
		Identifier ID = Identifier.lookup(string);
		SourceClass sc = 
		    (SourceClass)currentClass.getClassDefinition(env);
		Identifier resolve = sc.getImports().resolve(env, ID);
		decl = env.getClassDeclaration(resolve);
	    } catch (ClassNotFound ee) {
		decl = env.getClassDeclaration(Identifier.lookup(string));
	    }

	} else { 
	    decl = currentClass;
	}

	if (what == null) { 
	    result.addElement(decl); 
	    result.addElement(null); 
	    result.addElement(null); 
	} else { 
	    i = what.indexOf('(');
	    String fieldName = (i >= 0 ? what.substring(0, i) : what);
	    Identifier ID = Identifier.lookup(fieldName);
	    try { 
		FieldDefinition f =
		      decl.getClassDefinition(env).findAnyMethod(env, ID);
		if (f == null) { 
		    System.out.println("@see warning:  Can't find " + 
				       fieldName + " in " + decl);
		} else if (f.getClassDeclaration() != decl) { 
		    System.out.println("@see warning:  Found " + fieldName + 
				       " in parent" + f.getClassDeclaration() + 
				       " but not " + decl);
		    decl = f.getClassDeclaration();
		}
	    } catch (ClassNotFound ee) { 
		System.out.println("@see warning:  Can't find class " + 
				   decl.getName());
	    }
	    result.addElement(decl);
	    result.addElement(fieldName);
	    result.addElement(what);
	}
    }
}








