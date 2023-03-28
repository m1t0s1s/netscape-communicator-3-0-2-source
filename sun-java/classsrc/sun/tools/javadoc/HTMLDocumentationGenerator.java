/*
 * @(#)HTMLDocumentationGenerator.java	1.6 95/12/07 Arthur van Hoff, Frank Yellin
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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

import sun.awt.image.*;


class HTMLDocumentationGenerator extends DocumentationGenerator 
   implements Constants {

    ClassDeclaration currentClass;
    Vector allFields = new Vector();


    static final String InterfaceIndexImage = 
        imgString("interface-index.gif", 257, 38, "Interface Index");
    static final String ClassIndexImage = 
        imgString("class-index.gif", 216, 37, "Class Index");
    static final String ExceptionIndexImage = 
        imgString("exception-index.gif", 284, 38, "Exception Index");
    static final String ErrorIndexImage = 
        imgString("error-index.gif", 174, 38, "Error Index");

    static final int C_VARIABLE = 0;
    static final int C_CONSTRUCTOR = 1;
    static final int C_METHOD = 2;

    /** Generate the title page for multiple packages.  
     *  v1 will be an array of package names for all "general" packages
     *  v2 will be an array of package names for all "debugging"
     */
    void genPackagesDocumentation(String v1[], String v2[]) { 
	// Generate package documentation
	System.out.println("Generating packages.html");
        PrintStream out = openFile("packages.html");
	htmlStart(out, "Package Index");
	
	// generate hyperlinks to other documents
	out.print("<pre>");
	out.print(refString("API_users_guide.html", "API User's Guide"));
	out.print("  ");
	out.print(refString("tree.html", "Class Hierarchy"));
	out.print("  ");
	out.print(refString("AllNames.html", "Index"));
	out.print("</pre>");
	out.println("<hr>");

	// The title
	out.println("<h1>");
	out.println(imgString("package-index.gif", 238, 37, "Package Index"));
	out.println("</h1>");
	if (v1 != null) { 
	    out.println("<h2> Applet API Packages </h2>");
	    out.println("<menu>");
	    for (int i = 0; i < v1.length; i++) { 
		String s = v1[i];
		Identifier pkg = Identifier.lookup(s);
	        out.println("<li> package " + pkgString(pkg, s));
	    }
	    out.println("</menu>");
	}
	if (v2 != null) { 
	    out.println("<h2> Other Packages </h2>");
	    out.println("<menu>");
	    for (int i = 0; i < v2.length; i++) { 
		String s = v2[i];
		Identifier pkg = Identifier.lookup(s);
	        out.println("<li> package " + pkgString(pkg, s));
	    }
	    out.println("</menu>");
	}
	htmlEnd(out);
    }

    /**
     * Generate the documentation for a single package.  The classes in
     * the documentation will be sorted into four categories 
     */

    void genPackageDocumentation(Identifier pkg, 
				 ClassDeclaration intfDecls[], 
				 ClassDeclaration classDecls[], 
				 ClassDeclaration exceptDecls[], 
				 ClassDeclaration errorDecls[]) {
	// Generate package documentation
	PrintStream out = openFile("Package-" + pkg.toString() + ".html");
        htmlStart(out, "Package " + pkg);

	out.print("<pre>");
	out.print(refString("packages.html", "All Packages"));
	out.print("  ");
	out.print(refString("tree.html", "Class Hierarchy"));
	out.print("  ");
	out.print(refString("AllNames.html", "Index"));
	out.println("</pre>");

	out.println("<hr>");

	out.println("<h1>");
	out.println("  package " + pkg);
	out.println("</h1>");

	genPackageDocumentationType(out, intfDecls,   InterfaceIndexImage);
	genPackageDocumentationType(out, classDecls,  ClassIndexImage);
	genPackageDocumentationType(out, exceptDecls, ExceptionIndexImage);
	genPackageDocumentationType(out, errorDecls,  ErrorIndexImage);
        // the end
	htmlEnd(out);
    }

    /** 
     * Generate the documentation for a specific category of classes
     * in a package 
     */
    private void 
    genPackageDocumentationType(PrintStream out, 
				ClassDeclaration classes[], 
				String imageString) { 
	if (classes.length == 0) 
	    return;
	sort(classes);
	out.println("<h2>");
	out.println("  " + imageString);
	out.println("</h2>");

	// create a menu of the classes for the package page.
	out.println("<menu>");
	for (int i = 0 ; i < classes.length ; i++) {
	    try {
		ClassDefinition def = classes[i].getClassDefinition(env);
		out.println("  <li> "+ classString(classes[i]));
		// create the documentation for the class on a separate page
		genClassDocumentation(def, 
				      ((i > 0) ? classes[i-1] : null),
				      ((i+1 < classes.length) ? 
				       classes[i+1] : null));
	    } catch (ClassNotFound ex) {
		System.err.println("Warning: Couldn't find class info for "
				   + classes[i].getName());
	    }
	}
	out.println("</menu>");
    }


    /** Generate the documentation for a single class */
    void genClassDocumentation(ClassDefinition c, ClassDeclaration prev, 
			       ClassDeclaration next) {
	System.out.println("generating documentation for " + c);
	currentClass = c.getClassDeclaration();
	String title = 
	      (c.isInterface() ? "Interface " : "Class ") + c.getName();

        // open the file
	PrintStream out = openFile(c.getName() + ".html");

        htmlStart(out, title);

	// Global references
	if (!idNull.equals(c.getName().getQualifier())) {
	    genButtons(out, c, prev, next);
	    out.println("<hr>");
	}

	// Document header
	out.println("<h1>");
	out.println("  " + title);
	out.println("</h1>");
	if (!c.isInterface()) 
	    genSuperClasses(out, currentClass);

	out.println("<dl>");
	out.print("  <dt> ");
	if (c.isPublic()) 
	    out.print("public ");
	if (c.isFinal()) 
	    out.print("final ");
	out.print(c.isInterface() ? "interface " : "class ");
	out.println("<b>" + c.getName().getName() + "</b>");
	ClassDeclaration sup  = c.getSuperClass();
	if (sup != null) {
	    out.println("  <dt> extends " + classString(sup));
	}
	ClassDeclaration intf[] = c.getInterfaces();
	if (intf.length > 0) {
	    out.print("  <dt> ");
	    out.print(c.isInterface() ? "extends " : "implements ");
	    for (int i = 0 ; i < intf.length ; i++) {
		if (i > 0) 
		    out.print(", ");
		out.print(classString(intf[i]));
	    }
	    out.println();
	}
	out.println("</dl>");

	int what = 0;
	String cdoc = c.getDocumentation();
	if (cdoc != null) {
	    // put the comments into a more parseable form.
	    Vector mergeDoc = mergeDoc(cdoc);

	    String comment = getComment(mergeDoc);
	    String authors = getAuthors(mergeDoc);
	    String version = getVersion(mergeDoc);
	    Vector seeAlso = getSees(mergeDoc, currentClass);
	    
	    boolean extraDope = (authors != null || version != null || 
				     seeAlso != null);
	    if (comment != null) 
		out.println(comment);
	    if (extraDope) 
		out.println("<dl>");
	    if (version != null) { 
		out.println("  <dt> <b>Version:</b>");
		out.println("  <dd> " + version);
	    } 
	    if (authors != null) {
		out.println("  <dt> <b>Author:</b>");
		out.println("  <dd> " + authors);
	    }
	    if (seeAlso != null) {
		handleSeeAlso(out, seeAlso);
	    }
	    if (extraDope) 
		out.println("</dl>");
	}

	if (c.isPublic() || idNull.equals(c.getName().getQualifier())) {
	    out.println("<hr>");
	    out.println(anchorString("index"));

	    // Generate lists
	    Vector variables = allVariables(c);
	    Vector constructors = allConstructors(c);
	    Vector methods = allMethods(c);
	    
	    genIndex(out, c, variables,    C_VARIABLE);
	    genIndex(out, c, constructors, C_CONSTRUCTOR);
	    genIndex(out, c, methods,      C_METHOD);

	    variables = localFieldsOf(c, variables);
	    methods = localFieldsOf(c, methods);
	    constructors = localFieldsOf(c, constructors);

	    // Full documentation on this methods's variables
	    genDocumentation(out, variables,    C_VARIABLE);
	    genDocumentation(out, constructors, C_CONSTRUCTOR);
	    genDocumentation(out, methods,      C_METHOD);
	} else {
	    out.println("<p>");
	    out.println("<em>");
	    out.println("This class is not public and can therefore " + 
			"cannot be used outside this package.");
	    out.println("</em>");
	}

	// Global references
	if (!idNull.equals(c.getName().getQualifier())) {
	    out.println("<hr>");
	    genButtons(out, c, prev, next);
	}

	// the end
	htmlEnd(out);
    }


    // print the nice picture showing this class and its superclasses
    private void genSuperClasses(PrintStream out, ClassDeclaration clazz) { 
	Vector tree = superclassesOf(clazz);
	int treeLength = tree.size();
	out.println("<pre>");
	for (int n = 0; n < treeLength; n++) { 
	    ClassDeclaration c = (ClassDeclaration)tree.elementAt(n);
	    for (int i = 1 ; i < n ; i++) 
		out.print("        ");
	    if (n > 0)
		out.println("   |");
	    for (int i = 1 ; i < n ; i++) 
		out.print("        ");
	    if (n > 0)
	        out.print("   +----");
	    if (c.equals(clazz)) { 
		out.println(c.getName());
	    } else { 
		out.println(classString(c, "_top_", c.getName()));
	    }
	}
	out.println("</pre>");
	out.println("<hr>");
    }
    
    static String indexImages[] = {
        imgString("variable-index.gif", 207, 38, "Variable Index"),
        imgString("constructor-index.gif", 275, 38, "Constructor Index"),
        imgString("method-index.gif", 207, 38, "Method Index")
    };
    
    static final String staticVariableIndexImage = 
        imgString("blue-ball-small.gif", 6, 6, " o ");
    static final String dynamicVariableIndexImage = 
        imgString("magenta-ball-small.gif", 6, 6, " o ");
    static final String constructorIndexImage = 
        imgString("yellow-ball-small.gif", 6, 6, " o ");
    static final String staticMethodIndexImage = 
        imgString("green-ball-small.gif", 6, 6, " o ");
    static final String dynamicMethodIndexImage = 
        imgString("red-ball-small.gif", 6, 6, " o ");


    // Create a index of the specific fields for the beginning of the page
    private void 
    genIndex(PrintStream out, ClassDefinition c, Vector fields, int type) {
	if (fields.size() <= 0) 
	    return;
	FieldDefinition ff[] = new FieldDefinition[fields.size()];
	fields.copyInto(ff);
	sort(ff);
	out.println("<h2>");
	out.println("  " + indexImages[type]);
	out.println("</h2>");
	out.println("<dl>");
	for (int i = 0 ; i < ff.length ; i++) {
	    FieldDefinition f = ff[i];
	    out.print("  <dt> ");
	    if (type == C_VARIABLE) {
		Identifier name = f.getName();
		out.println(f.isStatic() ? staticVariableIndexImage 
			                 : dynamicVariableIndexImage);
		out.println("\t" + classString(f.getClassDeclaration(), name, 
					       "<b>" + name + "</b>"));
	    } else {
	        Identifier name;
		String image;
		if (f.isConstructor()) { 
		    name = f.getClassDeclaration().getName().getName();
		    image = constructorIndexImage;
		} else { 
		    name = f.getName(); 
		    image = f.isStatic() ? staticMethodIndexImage 
                                         : dynamicMethodIndexImage;
		}
		out.println(image);
		out.print("\t" + 
			  refString(methodTag(f), "<b>" + name + "</b>"));
		out.println(f.getType().typeString("", true, false));
	    }
	    out.print("  <dd> ");
	    if (!c.equals(f.getClassDefinition())) {
		out.print("Inherited from ");
		out.print(classString(f.getClassDeclaration()));
		out.print(".  ");
	    }
	    out.println(firstSentence(f));
	}
	out.println("</dl>");
    }

    static String docImages[] = {
        imgString("variables.gif",    153, 38, "Variables"),
        imgString("constructors.gif", 231, 38, "Constructors"),
        imgString("methods.gif",      151, 38, "Methods")
    };
    
    static String docAnchors[] = { "variables", "constructors", "methods" };

    static final String staticVariableDocImage = 
        imgString("blue-ball.gif", 12, 12, " o ");
    static final String dynamicVariableDocImage = 
        imgString("magenta-ball.gif", 12, 12, " o ");
    static final String constructorDocImage = 
        imgString("yellow-ball.gif", 12, 12, " o ");
    static final String staticMethodDocImage = 
        imgString("green-ball.gif", 12, 12, " o ");
    static final String dynamicMethodDocImage = 
        imgString("red-ball.gif", 12, 12, " o ");

    // Create the documentation for the specific fields.
    void genDocumentation(PrintStream out, Vector fields, int type) { 
	if (fields.size() <= 0) 
	    return;
	out.println(anchorString(docAnchors[type]));
	out.println("<h2>");
	out.println("  " + docImages[type]);
	out.println("</h2>");
	if (type == C_CONSTRUCTOR) { 
	    String name = currentClass.getName().getName().toString();
	    out.println(anchorString(name));
	}
	for (int i = 0; i < fields.size(); i++) {
	    FieldDefinition f = (FieldDefinition)fields.elementAt(i);
	    if (type == C_VARIABLE) 
		genVariableDocumentation(out, f);
	    else 
		genMethodDocumentation(out, f);
	    if (Main.showIndex) 
		allFields.addElement(f);
	}
    }

    // Create the documentation for a specific variable 
    void genVariableDocumentation(PrintStream out, FieldDefinition f) {
	String image = f.isStatic() ? staticVariableDocImage 
	                            : dynamicVariableDocImage;
	out.println(anchorString(f.getName().toString(),image));
	out.println("<b>" + f.getName() + "</b>");
	out.println("<pre>");
	out.print(modString(f) + " " + typeString(f.getType()) + " " + 
		  f.getName());
	for (int i = f.getType().getArrayDimension() ; i > 0 ; i--) {
	    out.print("[]");
	}
	out.println();
	out.println("</pre>");

	String doc = f.getDocumentation();
	if (doc != null) {
	    // put the comments into a more parseable form.
	    Vector mergeDoc = mergeDoc(doc);

	    String comment = getComment(mergeDoc);
	    Vector seeAlso = getSees(mergeDoc, currentClass);

	    out.println("<dl>");
	    if (comment != null) 
		out.println("  <dd> " + comment);
	    if (seeAlso != null) {
		out.println("  <dl> ");
		handleSeeAlso(out, seeAlso);
		out.println("  </dl>");
	    }
	    out.println("</dl>");
	}
    }

    // Create the documentation for a specific method
    void genMethodDocumentation(PrintStream out, FieldDefinition f) {
	ClassDeclaration fieldClass = f.getClassDeclaration();
	Type fieldType = f.getType();
	boolean isConstructor = f.isConstructor();
	Identifier name = isConstructor ? fieldClass.getName().getName()
	                                : f.getName();

	String image =  f.isConstructor() ? constructorDocImage 
	              : f.isStatic()      ? staticMethodDocImage 
                      :                     dynamicMethodDocImage;
	out.println(anchorString(methodTag(f), image));
	if (isConstructor) 
	    out.println("<b>" + name + "</b>");
	else 
	    out.println(anchorString(name.toString(), "<b>" + name + "</b>"));
	out.println("<pre>");

	String sig = modString(f) + " ";
	if (isConstructor) {
	    sig += name + "(";
	} else {
	    sig += typeString(f.getType().getReturnType());
	    for (int i = f.getType().getReturnType().getArrayDimension(); 
		       i > 0 ; i--) {
		sig += "[]";
	    }
	    sig += " " + f.getName() + "(";
	}
	out.print(sig);

	// compute the ACTUAL length of the signature,
	// not counting the stuff in <>'s.
	boolean inbracket = false;
	int siglen = 0;
	for (int i = 0 ; i < sig.length() ; i++) {
	    if (inbracket) {
		if (sig.charAt(i) == '>') {
		    inbracket = false;
		}
	    } else {
		if (sig.charAt(i) == '<') {
		    inbracket = true;
		} else {
		    siglen++;
		}
	    }
	}
	Type args[] = f.getType().getArgumentTypes();
	if (f.getArguments() == null) {
	    System.out.println(f);
	}
	Enumeration e = f.getArguments().elements();
	if (!f.isStatic()) {
	    e.nextElement();
	}
	for (int i = 0; i < args.length ; i++) {
	    if (i > 0) {
		out.println(",");
		for (int j = siglen ; j > 0 ; j--) {
		    out.print(" ");
		}
	    }
	    out.print(typeString(args[i]));
	    LocalField l = (LocalField)e.nextElement();
	    out.print(" ");
	    out.print(l.getName());
	    for (int j = args[i].getArrayDimension() ; j > 0 ; j--) {
		out.print("[]");
	    }
	}
	out.print(")");
	ClassDeclaration[] exp = f.getExceptions(env);
	if (exp.length > 0)
	    out.print(" throws");
	for (int i = 0; i < exp.length; i++) {
	    if (i > 0) {
		out.print(",");
	    }
	    out.print(" " + classString(exp[i]));
	}
	out.println();
	out.println("</pre>");

	String doc = getDocumentation(f);
	FieldDefinition overrides = getOverride(f);
	if (overrides != null && doc == null) 
	    doc = "";

	if (doc != null) {
	    // put the comments into a more parseable form.
	    Vector mergeDoc = mergeDoc(doc);

	    String comment = getComment(mergeDoc);
	    String returns = getReturn(mergeDoc);
	    Vector seeAlso = getSees(mergeDoc, currentClass);
	    Vector parameters = getParameters(mergeDoc);
	    Vector exceptions = getThrows(mergeDoc, f);

	    boolean extraDope = (parameters != null || returns != null 
				  || exceptions != null || overrides != null
				  || seeAlso != null);
	    
	    out.println("<dl>");
	    if (comment != null) 
		out.println("  <dd> " + comment);
	    if (extraDope) 
		out.println("  <dl>");
	    if (parameters != null) { 
		out.println("    <dt> <b>Parameters:</b>");
		for (int i = 0; i < parameters.size(); i += 2) 
		    out.println("    <dd> " + parameters.elementAt(i) + 
				    " - " + parameters.elementAt(i + 1));
	    }
	    if (returns != null) { 
		out.println("    <dt> <b>Returns:</b>");
		out.println("    <dd> " + returns);
	    }
	    if (exceptions != null) { 
		for (int i = 0; i < exceptions.size(); i += 2) { 
		    ClassDeclaration exc = 
			  (ClassDeclaration)exceptions.elementAt(i);
		    String what = (String)exceptions.elementAt(i + 1);
		    out.println("    <dt> <b>Throws:</b> " + classString(exc));
		    out.println("    <dd> " + what);
		}
	    }
	    if (overrides != null) { 
		out.println("    <dt> <b>Overrides:</b>");
		out.println("    <dd> " 
			    + refString(methodTag(overrides), 
					overrides.getName())
			    + " in class " 
			    + classString(overrides.getClassDeclaration()));
		overrides = null;
	    }
	    if (seeAlso != null) { 
		handleSeeAlso(out, seeAlso);
	    }
	    if (extraDope) 
		out.println("  </dl>");
	    out.println("</dl>");
	}
    }

    // Create an index of all the fields, from the fields saved in
    // allFields
    void genFieldIndex() {
       // Generate package documentation
	PrintStream out = openFile("AllNames.html");
        htmlStart(out, "Index of all Fields and Methods");

	out.print("<pre>");
	out.print(refString("packages.html", "All Packages"));
	out.print("  ");
	out.print(refString("tree.html", "Class Hierarchy"));
	out.print("</pre>");

	out.println("<hr>");

	for (char ch = 'A'; ch <= 'Z'; ch++) 
	    out.println(refString(("#Thumb-" + ch), String.valueOf(ch))); 
	out.println();
	out.println("<hr>");

	out.println("<h1>");
	out.println("  Index of all Fields and Methods");
	out.println("</h1>");

	if (allFields.size() == 0) { 
	    // a degenerate case that should only happen
	    // the end
	    htmlEnd(out);
	    return;
	}

	FieldDefinition ff[] = new FieldDefinition[allFields.size()];
	allFields.copyInto(ff);
	String ss[] = new String[ff.length];
	System.out.print("Sorting " + ff.length + " items . . . ");
	System.out.flush();


	for (int i = 0; i < ss.length; i++) 
	    ss[i] = sortKey(ff[i]);
	xsort(ff, ss);
	System.out.println("done");

	out.println("<h2>");
	out.println(anchorString("Thumb-A", "<b> A </b>"));
	out.println("</h2>");
	out.println("<dl>");
				 
	for (int i = 0; i < ff.length; i++) {
	    FieldDefinition f = ff[i];
	    if (i > 0 && ss[i].charAt(0) != ss[i-1].charAt(0)) { 
		out.println("</dl>");
		out.println("<hr>");
		char newChar =  Character.toUpperCase(ss[i].charAt(0));
		char prevChar = Character.toUpperCase(ss[i-1].charAt(0));
		for (char ch = (char)(prevChar + 1); ch < newChar; ch++) 
		    out.println(anchorString("Thumb-" + ch));
		out.println("<h2>");
		out.println(anchorString("Thumb-" + newChar, 
					 "<b> " + newChar + " </b>"));
		out.println("</h2>");
		out.println("<dl>");
	    }
	    try { 
		ClassDeclaration c = f.getClassDeclaration();
		ClassDefinition def = c.getClassDefinition(env);
		out.print("  <dt> ");
		if (f.isVariable()) { 
		    Identifier name = f.getName();
		    /* 
		    out.println(f.isStatic() ? staticVariableIndexImage 
				            : dynamicVariableIndexImage);
		    */
		    out.println("\t" + 
				classString(f.getClassDeclaration(), name, 
					    "<b>" + name + "</b>") + ".");
		    out.print(f.isStatic() ? "Static variable in " 
			                   : "Variable in ");
		    
		} else {
		    Identifier name;
		    String image;
		    if (f.isConstructor()) { 
		        name = f.getClassDeclaration().getName().getName();
			image = constructorIndexImage;
		    } else { 
			name = f.getName(); 
			image = f.isStatic() ? staticMethodIndexImage 
			                     : dynamicMethodIndexImage;
		    }
		    // out.println(image);
		    out.print("\t" + 
			      refString(methodTag(f), "<b>" + name + "</b>"));
		    out.println(f.getType().typeString("", true, false) + ".");
		    out.print(f.isConstructor() ? "Constructor for " :
			      f.isStatic() ? "Static method in " 
			                   : "Method in ");
		}
		out.println((def.isInterface() ? "interface " : "class ")
			    + longClassString(c));
		out.println("  <dd> " + firstSentence(f));
	    } catch (ClassNotFound e) { }
	}

	for (char ch = (char)(ss[ss.length - 1].charAt(0) + 1); ch <= 'z'; ch++)
	    out.println(anchorString(("Thumb-" + Character.toUpperCase(ch))));
	
	out.println("</dl>");	

        // the end
	htmlEnd(out);
    }


    /** Generate a page containing the class hierarcy */

    void genClassTree(Hashtable tree, ClassDeclaration objectDecl) { 
	// Generate a picture of the tree
        PrintStream out = openFile("tree.html");
        htmlStart(out, "Class Hierarchy");

	out.print("<pre>");
	out.print(refString("packages.html", "All Packages"));
	out.print("  ");
	out.print(refString("AllNames.html", "Index"));
	out.print("</pre>");
	out.println("<hr>");

	out.println("<h1>");
	out.println("  Class Hierarchy");
	out.println("</h1>");

	out.println("<ul>");
	genClassTree(out, tree, objectDecl, "  ");
	out.println("</ul>");
	htmlEnd(out);
    }

    private void 
    genClassTree(PrintStream out, Hashtable tree, 
		 ClassDeclaration decl, String prefix) {
	try { 
	    ClassDefinition defn = decl.getClassDefinition(env);
	    ClassDeclaration intf[] = defn.getInterfaces();
	    out.print(prefix + "<li> ");
	    out.print(defn.isClass() ? "class " : "interface ");
	    out.print(longClassString(decl));
	    if (intf.length > 0) {
		out.print(" (");
		out.print(defn.isInterface() ? "extends " : "implements ");
		for (int j = 0 ; j < intf.length ; j++) {
		    if (j > 0)  
			out.print(", ");
		    out.print(longClassString(intf[j]));
		}
		out.print(")");
	    }
	} catch (ClassNotFound e) {
	}
	out.println();
	ClassDeclaration[] children = (ClassDeclaration[])tree.get(decl);
	if (children != null && children.length > 0) { 
	    String newPrefix = prefix + "  ";
	    out.println(prefix + "<ul>");
	    for (int i = 0; i < children.length; i++)
		genClassTree(out, tree, children[i], newPrefix);
	    out.println(prefix + "</ul>");
	}
    }

    // Create a link to the given reference, with the specified content
    private String refString(String ref, Object content) {
	return "<a href=\"" + ref + "\">" + content + "</a>";
    }

    // Create an anchor string.
    private String anchorString(String ref) {
	if (ref.charAt(0) == '#') 
	    ref = ref.substring(1);
	return "<a name=\"" + ref + "\"></a>";
    }

    // Create an anchor string around a specific content
    private String anchorString(String ref, Object content) {
	if (ref.charAt(0) == '#') 
	    ref = ref.substring(1);
	return "<a name=\"" + ref + "\">" + content + "</a>";
    }


    // Create a string for a specific image.
    private static String 
    imgString(String img, int width, int height, String altText) {
	return "<img src=\"images/" + img + 
	          "\" width=" + width + " height=" + height + 
	          " alt=\"" + altText + "\">";
    }

    // Create string representing the long version of a class name.  Include
    // a link to the class page
    private String 
    longClassString(ClassDeclaration c) {
	return c.getName().getQualifier() + "." +
	       classString(c, "_top_", c.getName().getName());
    }


    // Create string representing the short version of a class name.  Include
    // a link to the class page
    String classString(ClassDeclaration c) {
	return classString(c, "_top_", c.getName().getName());
    }

    // Create a string representing a link to a place on a page.  The 
    // reference will have the value "content".  The link will be to
    // "where" on the page representing the class C.
    
    private String 
    classString(ClassDeclaration c, Object where, Object content) {
	return refString(classTag(c, where), content);
    }

    // A string representing a reference to Class c
    private String classTag(ClassDeclaration c) { 
	return classTag(c, "_top_");
    }

    // A string representing a reference to Class c, with anchor where
    // on that page
    private String classTag(ClassDeclaration c, Object where) { 
	if (c.equals(currentClass) || (c.getName().toString().length() == 0)) {
	    return "#" + where;
	} else {
	    return c.getName().toString() + ".html#" + where;
	}
    }
    
    // A string giving the full anchor location of a method.
    private String 
    methodTag(FieldDefinition f) {
	Identifier name = f.isConstructor()
			    ? f.getClassDeclaration().getName().getName()
			    : f.getName();
	return classTag(f.getClassDeclaration(), 
			f.getType().typeString(name.toString(), false, false));
    }


    // A string giving the full anchor location of a package
    private String 
    pkgString(Identifier pkg, Object content) {
	return refString("Package-" + pkg.toString() + ".html", content);
    }

    String authorString(String str) { return str; }
    String commentString(String str) { return str; } 
    String versionString(String str) { return str; } 
    String returnString(String str) { return str; } 

    // create the buttons for a class page
    private void 
    genButtons(PrintStream out, ClassDefinition c, 
	       ClassDeclaration prev, ClassDeclaration next) {

	// Global references
	out.println("<pre>");
	out.print(refString("packages.html", "All Packages"));
	out.print("  ");
	out.print(refString("tree.html", "Class Hierarchy"));
	out.print("  ");
	out.print(pkgString(c.getName().getQualifier(), "This Package"));
	out.print("  ");
	if (prev != null) {
	    out.print(classString(prev, "_top_", "Previous"));
	} else {
	    out.print(pkgString(c.getName().getQualifier(), "Previous"));
	}
	out.print("  ");
	if (next != null) {
	    out.print(classString(next, "_top_", "Next"));
	} else {
	    out.print(pkgString(c.getName().getQualifier(), "Next"));
	}
	out.print("  ");
	out.print(refString("AllNames.html", "Index"));
	out.println("</pre>");
    }

    static Date today = new Date();
    private void htmlStart(PrintStream out, String title) {
        out.println("<!--NewPage-->");
        out.println("<html>");
        out.println("<head>");
        out.println("<!-- Generated by javadoc on " + today + " -->");
	out.println(anchorString("_top_"));
	out.println("<title>");
	out.println("  "  + title);
	out.println("</title>");
	out.println("</head>");
        out.println("<body>");
    }

    private static void htmlEnd(PrintStream out) {
        out.println("</body>");
        out.println("</html>");
        out.close();
    }

    private void handleSeeAlso(PrintStream out, Vector seeAlso) { 
	String stuff = "";
	for (Enumeration e = seeAlso.elements(); e.hasMoreElements(); ) {
	    ClassDeclaration decl = (ClassDeclaration)(e.nextElement());

	    String fieldName = (String)(e.nextElement());
	    String what = (String)(e.nextElement());

	    if (stuff.length() >  0)
		stuff += ", ";
	    if (what == null) {
		stuff += classString(decl);
	    } else { 
		stuff += classString(decl, what, fieldName);
	    }
	}
	out.println("    <dt> <b>See Also:</b>");
	out.println("    <dd> " + stuff);
    }
}


