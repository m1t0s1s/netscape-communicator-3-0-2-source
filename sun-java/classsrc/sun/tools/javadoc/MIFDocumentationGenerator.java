/*
 * @(#)MIFDocumentationGenerator.java	1.4 95/12/07 Frank Yellin
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


class MIFDocumentationGenerator extends DocumentationGenerator 
                     implements Constants {

    ClassDeclaration currentClass;
    Vector allFields = new Vector();

    static final int C_VARIABLE = 0;
    static final int C_CONSTRUCTOR = 1;
    static final int C_METHOD = 2;

    static final int MARKER_package = 18;
    static final int MARKER_classType = 19;
    static final int MARKER_class = 20;
    static final int MARKER_methodName = 22;

    static final char hardSpaceChar = MIFPrintStream.hardSpaceChar;


    /** Generate the title page for multiple packages.  
     *  v1 will be an array of package names for all "general" packages
     *  v2 will be an array of package names for all "debugging"
     */
    void genPackagesDocumentation(String v1[], String v2[]) { }


    /**
     * Generate the documentation for a single package.  The classes in
     * the documentation will be sorted into four categories 
     */
    void genPackageDocumentation(Identifier pkg, 
				 ClassDeclaration intfDecls[], 
				 ClassDeclaration classDecls[], 
				 ClassDeclaration exceptDecls[], 
				 ClassDeclaration errorDecls[]) {
        PrintStream ps = DocumentationGenerator.openFile(pkg + ".mif");
	MIFPrintStream out = new MIFPrintStream(ps);

	out.newParagraph("Package")
	   .markReference("Package " + pkg)
	   .mark(MARKER_package, pkg.toString())  // maybe get rid of this?
	   .literal(pkg);
	genPackageDocumentationType(out, classDecls,  "Classes");
	genPackageDocumentationType(out, exceptDecls, "Exceptions");
	genPackageDocumentationType(out, errorDecls,  "Errors");
	genPackageDocumentationType(out, intfDecls,   "Interfaces");
	out.close();
    }

    /** 
     * Generate the documentation for a specific category of classes
     * in a package 
     */

    private void 
    genPackageDocumentationType(MIFPrintStream out, 
			       ClassDeclaration classes[],
			       String mark) {
	if (classes.length == 0) 
	    return;
	sort(classes);

	out.newParagraph("ClassType")
	   .mark(MARKER_classType , mark)
	   .literal(mark);

	for (int i = 0 ; i < classes.length ; i++) {
	    try {
		ClassDefinition def = classes[i].getClassDefinition(env);
		genClassDocumentation(out, def);
	    } catch (ClassNotFound ex) {
		System.err.println("Warning: Couldn't find class info for "
				   + classes[i].getName());
	    }
	}
    }

    /** Generate the documentation for a single class */
    public void genClassDocumentation(ClassDefinition c, 
				      ClassDeclaration prev, 
				      ClassDeclaration next) {
        PrintStream ps = DocumentationGenerator.openFile(c.getName() + ".mif");
	MIFPrintStream out = new MIFPrintStream(ps);
	genClassDocumentation(out, c);
	out.close();
    }

   private void genClassDocumentation(MIFPrintStream out, ClassDefinition c) {
	System.out.println("generating documentation for " + c);
	currentClass = c.getClassDeclaration();
	Identifier classID = c.getName();
	Identifier baseID = c.getName().getName();
	Identifier pkgID = c.getName().getQualifier();
	String type = (c.isInterface() ? "Interface" : "Class");
	String longTitle = type + " " + classID;
	String shortTitle = type + " " + baseID;
	
	out.newParagraph("Title")
	   .emitPendingMarks()	// any pending package marks??
	   .markReference(classTag(currentClass))
	   .mark(MARKER_class, shortTitle)
	    // we'll want package and class in the header and footer
	   .mark(MIFPrintStream.MarkerHF1, pkgID)  // get rid of this?
	   .mark(MIFPrintStream.MarkerHF2, shortTitle)
	   .index((c.isInterface() ? "Interfaces" : "Classes") + ":" + 
		  classID)
	   .index(baseID + ":" + type + " in package " + pkgID)
	   .literal(shortTitle)
	   .topLevel();

	genSuperClasses(out, c);



	// Document header
	String cdoc = c.getDocumentation();
	if (cdoc != null) {
	    // put the comments into a more parseable form.
	    Vector mergeDoc = mergeDoc(cdoc);

	    String comment = getComment(mergeDoc);
	    String authors = getAuthors(mergeDoc);
	    String version = getVersion(mergeDoc);
	    Vector seeAlso = getSees(mergeDoc, currentClass);
	    
	    if (comment != null)
		out.html("ClassComment", comment);

	    if (version != null) { 
		out.newParagraph("ClassInfo").literal("Version:");
		out.newParagraph("ClassInfoData").literal(version);	
	    } 

	    if (authors != null) {
		out.newParagraph("ClassInfo").literal("Author:");
		out.newParagraph("ClassInfoData").literal(authors);
	    }

	    if (seeAlso != null) {
		out.newParagraph("ClassInfo").literal("See Also:");
		out.newParagraph("ClassInfoData");
		handleSeeStrings(out, seeAlso);
	    }
	}

	if (c.isPublic() || idNull.equals(c.getName().getQualifier())) {
	    // Generate lists
	    Vector variables = allVariables(c);
	    Vector constructors = allConstructors(c);
	    Vector methods = allMethods(c);

	    variables = localFieldsOf(c, variables);
	    methods = localFieldsOf(c, methods);
	    constructors = localFieldsOf(c, constructors);

	    // Full documentation on this methods's variables
	    genDocumentation(out, variables,    C_VARIABLE);
	    genDocumentation(out, constructors, C_CONSTRUCTOR);
	    genDocumentation(out, methods,      C_METHOD);
	} else {
	    out.newParagraph("body");
	    out.literal("This class is not public and can therefore ")
	       .literal("cannot be used outside this package.");
	}
    }

    // print the nice picture showing this class and its superclasses
    private static final double firstMargin = .5;
    private static final double tabIncrement = .25;
    private void 

    genSuperClasses(MIFPrintStream out, ClassDefinition c) { 
	ClassDeclaration clazz = c.getClassDeclaration();
	Vector tree = superclassesOf(clazz);
	int treeLength = tree.size();
	ClassDeclaration intf[] = c.getInterfaces();

	if (treeLength == 1) // java.lang.Object
	    return;
	out.newParagraph("Parents")
	   .indent(firstMargin, firstMargin, 0)
	   .println("   <PgfSpAfter  12pt>")
	   .println("   <PgfNumTabs " + (treeLength - 1) + ">");
	for (int i = 0; i < treeLength - 1; i++) {
	    double tab = firstMargin + (tabIncrement * (i + 1)); 
	    out.println("      <TabStop <TSX " + tab + "in> " + 
			               "<TSType Left> <TSLeaderStr ` ' > >");
	}

	if (c.isPublic()) 
	    out.literal("public ");
	if (c.isFinal()) 
	    out.literal("final ");
	out.literal(c.isInterface() ? "interface " : "class ");
	Identifier pkg = c.getName().getQualifier();
	Identifier baseName = c.getName().getName();
	if (pkg != null) 
	    out.literal(pkg).literal('.');;
	out.bold(baseName);

	if (!c.isInterface()) { 
	    for (int i = treeLength - 1, count = 1; --i >= 0; count++) { 
		ClassDeclaration item = (ClassDeclaration)tree.elementAt(i);
		out.literal('\n');
		for (int j = 0; j < count; j++) 
		    out.literal('\t');
		out.literal("extends").literal(hardSpaceChar);
		pkg = item.getName().getQualifier();
		baseName = item.getName().getName();
		if (pkg != null) 
		    out.literal(pkg).literal('.');;
		out.bold(baseName);
		XRef(out, item);
	    }
	} 
	if (intf.length > 0) {
	    out.literal(c.isInterface() ? "\n\textends " : "\n\timplements ");
	    for (int i = 0 ; i < intf.length ; i++) {
		if (i > 0) 
		    out.literal(", ");
		pkg = intf[i].getName().getQualifier();
		baseName = intf[i].getName().getName();
		if (pkg != null) 
		    out.literal(pkg).literal('.');;
		out.bold(baseName);
		XRef(out, intf[i]);
	    }
	}
    }

    static String DocumentationStrings[] = 
          { "Variables", "Constructors", "Methods" };

    // generate the documentation for the specific fields.
    private void 
    genDocumentation(MIFPrintStream out, Vector fields, int type) { 
	Hashtable seenNames = null;
	if (fields.size() <= 0) 
	    return;
	out.newParagraph("Heading1");
	if (type == C_CONSTRUCTOR) {
	    out.markReference(classTag(currentClass, 
				       currentClass.getName().getName()));
	}
	out.literal(DocumentationStrings[type]);
	for (int i = 0; i < fields.size(); i++) {
	    FieldDefinition f = (FieldDefinition)fields.elementAt(i);
	    if (type == C_VARIABLE) 
		genVariableDocumentation(out, f);
	    else if (type == C_CONSTRUCTOR) {
		genMethodDocumentation(out, f, false);
	    } else { 
		Identifier name = f.getName();
		if (seenNames == null) 
		    seenNames = new Hashtable();
		genMethodDocumentation(out, f, seenNames.get(name) == null);
		seenNames.put(name, name);
	    }
	    if (Main.showIndex) 
		allFields.addElement(f);
	}
    }

    // generate the documentation for a specific variable
    void genVariableDocumentation(MIFPrintStream out, FieldDefinition f) { 
	ClassDefinition fieldClass = f.getClassDefinition();
	String name = f.getName().toString();
	out.newParagraph("FieldBullet")
	   .markReference(classTag(currentClass, name))
	   .mark(MARKER_methodName, name)
           .index(name + ":" + 
		  f.getType().typeString(name.toString(), true, false) + ". " +
		  (f.isStatic() ? "Static variable in " : "Variable in ") +
		  (fieldClass.isInterface() ? "interface " : "class ") + 
		  f.getClassDeclaration().getName().getName())
	   .literal(name);

	out.newParagraph("FieldTty")
	   .literal(f.isStatic() ? "static " : "")
	   .literal(modString(f).trim())
	   .literal(' ')
	   .literal(typeString(f.getType()))
	   .literal(' ')
	   .literal(name)
	   .literal(typeArrayString(f.getType()));
    
	String doc = f.getDocumentation();
	if (doc != null) {
	    // put the comments into a more parseable form.
	    Vector mergeDoc = mergeDoc(doc);
	    String comment = getComment(mergeDoc);
	    Vector seeAlso = getSees(mergeDoc, currentClass);

	    if (comment != null) 
		out.html("FieldComment", comment);

	    if (seeAlso != null) {
		out.newParagraph("FieldInfo").literal("See Also:");
		out.newParagraph("FieldInfoData");
		handleSeeStrings(out, seeAlso);
	    }
	}
    }

    // generate the documentation for a specific method
    void genMethodDocumentation(MIFPrintStream out,
				FieldDefinition f,
				boolean nameMarker) {
	ClassDefinition fieldClass = f.getClassDefinition();
	Type fieldType = f.getType();
	boolean isConstructor = f.isConstructor();
	Identifier name = isConstructor ? fieldClass.getName().getName()
	                                : f.getName();

	out.newParagraph("FieldBullet");
	if (nameMarker)
	    // we want a reference to just the method name
	    out.markReference(classTag(currentClass, f.getName()));
	out.mark(MARKER_methodName, methodFullName(f))
	   .markReference(classTag(currentClass, methodFullName(f)))
 	   .index(name + ":" + 
		  f.getType().typeString(name.toString(), true, true) + ". " +
		  (f.isConstructor() ? "Constructor for " :
		        f.isStatic() ? "Static method in " : "Method in ") +
		  (fieldClass.isInterface() ? "interface " : "class ") + 
		  f.getClassDeclaration().getName().getName())
           .literal(name);


	out.newParagraph("FieldTty")
	   .literal(f.isStatic() ? "static " : "")
	   .literal(modString(f).trim())
	   .literal(' ');
	if (!isConstructor) {
	    out.literal(typeString(f.getType().getReturnType()))
	       .literal(typeArrayString(f.getType()));
	}
	// this is really gross
	out.newParagraph("FieldTty")
	   .indent(.25, .25 + (name.toString().length() + 1)* 0.080, 0)
	   .literal(name).literal('(');

	Type args[] = f.getType().getArgumentTypes();
	if (f.getArguments() == null) {
	    throw new RuntimeException("getArguments returns null for " + f);
	}
	Enumeration e = f.getArguments().elements();
	if (!f.isStatic()) 
	    e.nextElement();
	for (int i = 0; i < args.length ; i++) {
	    LocalField l = (LocalField)e.nextElement();
	    if (i > 0) 
		out.literal(", ");
	    out.literal(typeString(args[i]))
	       .literal(hardSpaceChar)
	       .literal(l.getName())
	       .literal(typeArrayString(args[i]));
	}
	out.literal(")");
	
	ClassDeclaration[] exp = f.getExceptions(env);
	if (exp.length > 0) {
	    out.newParagraph("FieldTty")
	       .indent(.25, .5, .25)
	       .literal("throws");
	    for (int i = 0; i < exp.length; i++) 
		out.literal((i > 0 ? ", " : " ") + exp[i].getName().getName());
	}
	out.topLevel();

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

	    if (comment != null) {
		out.html("FieldComment", comment);
	    }

	    if (parameters != null) { 
		out.newParagraph("FieldInfo").literal("Parameters:");
		for (int i = 0; i < parameters.size(); i += 2) {
		    String arg = (String)parameters.elementAt(i);
		    String argdoc = (String)parameters.elementAt(i + 1);
		    out.newParagraph("ParameterName").literal(arg);
		    out.html("ParameterData", argdoc);
		}
	    }

	    if (returns != null) { 
		out.newParagraph("FieldInfo").literal("Returns:");
		out.html("FieldInfoData", returns);
	    }

	    if (exceptions != null) { 
		for (int i = 0; i < exceptions.size(); i += 2) { 
		    ClassDeclaration exc = 
			  (ClassDeclaration)exceptions.elementAt(i);
		    String what = (String)exceptions.elementAt(i + 1);
		    out.newParagraph("FieldInfo")
		       .println("   <PgfPlacementStyle RunIn>")
		       .println("   <PgfRunInDefaultPunct `: '>")
		       .literal("Throws")
		       .newParagraph("FieldInfoData")
		       .charTag("ClassName", exc.getName().getName());
		    XRef(out, exc);
		    out.html("FieldInfoData", what);
		}
	    }

	    if (overrides != null) { 
		ClassDeclaration from = overrides.getClassDeclaration();
		out.newParagraph("FieldInfo").literal("Overrides:");
		out.newParagraph("FieldInfoData")
		   .charTag("MethodName", overrides.getName())
		   .literal(" in class ")
		   .charTag("ClassName", from.getName().getName());
		XRef(out, from, methodFullName(f));
		out.literal('.');
		overrides = null;
	    }

	    if (seeAlso != null) { 
		out.newParagraph("FieldInfo").literal("See Also:");
		out.newParagraph("FieldInfoData");
		handleSeeStrings(out, seeAlso);
	    }
	}
    }

    // Generate the index.  Not necessary for MIF, since Frame can do this
    // automatically
    void genFieldIndex() { 
	System.out.println("No need for index for MIF");
    }


    /** Generate a page containing the class hierarcy */
    void genClassTree(Hashtable tree, ClassDeclaration objectDecl) { 
	// Generate a picture of the tree
	currentClass = null;
        PrintStream ps = DocumentationGenerator.openFile("tree.mif");
        MIFPrintStream out = new MIFPrintStream(ps);
	out.newParagraph("Title").literal("Hierarchy");
	genClassTree(out, tree, objectDecl, 0);
	out.close();
    }

    private
    void genClassTree(MIFPrintStream out, Hashtable tree, 
		      ClassDeclaration decl, int level) {
      Identifier pkg = decl.getName().getQualifier();
      Identifier baseName = decl.getName().getName();

	out.newParagraph("Body").skip(0).indent(.25 * level, 0, 
						.25 * (level + 2));
	try { 
	    ClassDefinition defn = decl.getClassDefinition(env);
	    ClassDeclaration interfaces[] = defn.getInterfaces();

	    out.literal(defn.isClass() ? "class " : "interface ")
	       .literal(pkg).literal('.').bold(baseName);
	    XRef(out, decl);
	    if (interfaces.length > 0) {
	        out.literal(defn.isInterface() ? " extends" : " implements")
		   .literal(hardSpaceChar);
		for (int i = 0 ; i < interfaces.length ; i++) {
		    ClassDeclaration intf = interfaces[i];
		    if (i > 0)  
			out.literal(", ");
		    out.literal(intf.getName().getName());
		    XRef(out, intf);
		}
	    }
	} catch (ClassNotFound e) {
	}

	ClassDeclaration[] children = (ClassDeclaration[])tree.get(decl);
	if (children != null && children.length > 0) { 
	    level++;
	    for (int i = 0; i < children.length; i++) {
		genClassTree(out, tree, children[i], level);
	    }
	}
    }

    String authorString(String str) { return str; }
    String commentString(String str) { return str; } 
    String versionString(String str) { return str; } 
    String returnString(String str) { return str; } 

    String classString(ClassDeclaration c) { 
	return c.getName().getName().toString();
    } 

    private void handleSeeStrings(MIFPrintStream out, Vector seeAlso) {
	boolean first = true;
	for (Enumeration e = seeAlso.elements(); e.hasMoreElements(); ) {
	    ClassDeclaration decl = (ClassDeclaration)(e.nextElement());
	    String fieldName = (String)(e.nextElement());
	    String what = (String)(e.nextElement());

	    if (!first) 
		out.literal(", ");

	    if (fieldName == null) { 
		out.charTag("ClassName", decl.getName().getName());
		XRef(out, decl);
	    } else { 
		out.charTag("MethodName",  fieldName);
		if (decl != currentClass) 
		    out.literal(" in class ")
		       .charTag("ClassName", decl.getName().getName());
		XRef(out, decl, what);
	    }
	    first = false;
	}
	out.literal('.');
    }

    String methodFullName(FieldDefinition f) {
	Identifier name = f.isConstructor()
			    ? f.getClassDeclaration().getName().getName()
			    : f.getName();
	return f.getType().typeString(name.toString(), false, false);
    }

    String classTag(ClassDeclaration c) { 
	return "class " + c.getName();
    }

    String classTag(ClassDeclaration c, Object thing) { 
	return "class " + c.getName() + "." + thing;
    }

    String sourceFile(ClassDeclaration decl) {
        Identifier pkg = decl.getName().getQualifier();
	return pkg + ".fm";
    }

    void XRef(MIFPrintStream out, ClassDeclaration decl) {
	XRef(out, decl, null);
    } 

    void XRef(MIFPrintStream out, ClassDeclaration decl, Object what) {
	String itsSource = sourceFile(decl);
	if (currentClass != null && sourceFile(currentClass).equals(itsSource))
	    itsSource = null;
	String thing = (what == null) ? classTag(decl) : classTag(decl, what);
	out.literal(hardSpaceChar)
	   .XRef("(page)", thing, itsSource);
    }
}


