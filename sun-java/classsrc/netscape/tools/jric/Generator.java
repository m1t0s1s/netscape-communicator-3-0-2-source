// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.jric;

import netscape.tools.ClassFileParser.*;
import java.io.*;
import java.util.*;

public abstract
class Generator {
    Vector forwardDecls;

    protected Main global;
    protected PrintStream out;
    protected ClassDef clazz;
    protected String clazzCName;

    protected Generator(Main global, ClassDef clazz) {
	this.global = global;
	this.clazz = clazz;
	clazzCName = clazz.getCName();
	forwardDecls = new Vector();
    }

    abstract protected String getHeaderString();

    static final String DATELINE = "Class file date: ";

    public void generate() throws IOException {
	FileOutputStream f = null;
	try {
	    String filename = getFileName();
	    if (global.outputDir != null) {
		File dir = new File(global.outputDir);
		if (!dir.exists()) {
		    dir.mkdir();
		}
		else if (!dir.isDirectory()) {
		    throw new IOException(global.outputDir + " is not a directory.");
		}
		filename = global.outputDir + "/" + filename;
	    }
	    File file = new File(filename);
	    if (file.exists()) {
		DataInputStream in =
		    new DataInputStream(new FileInputStream(file));
		// discard the first few lines
		in.readLine();
		in.readLine();
		in.readLine();
		String dateLine = in.readLine();
		
		int dateLineIndex = dateLine.indexOf(DATELINE);
		if (dateLineIndex != -1) {
		    String dateString = dateLine.substring(dateLineIndex + DATELINE.length());
		    Date lastModDate = new Date(dateString);
		    Date currentModDate = new Date(clazz.modDate);
		    if (currentModDate.equals(lastModDate)) {
			if (global.verboseMode)
			    System.err.println(clazz.name + " is up to date!");
			return;
		    }
		}
	    }
	    f = new FileOutputStream(file);
	    out = new PrintStream(f);
	    printBlockComment(getHeaderString() + " -- DO NOT EDIT"
			      + "\n * " + DATELINE + (new Date(clazz.modDate)));
	    generateOutput();
	} finally {
	    if (f != null) f.close();
	    out = null;
	}
    }

    protected abstract String getFileName();

    protected abstract void generateOutput() throws IOException;

    protected void printBlockComment(String label) {
	int i;
	out.print('/');
	for (i = 0; i < 79; i++)
	    out.print('*');
	out.print("\n * ");
	out.print(label);
	out.print("\n ");
	for (i = 0; i < 78; i++)
	    out.print('*');
	out.print("/\n\n");
    }

    //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
    // Signature Manipulation

    protected static String getTypeName(String sig, int sigIndex) {
	int index = MemberDef.getSigType(sig.charAt(sigIndex));
	String[] resultTypes = { "jbool",
				 "jbyte",
				 "jchar",
				 "jshort",
				 "jint",
				 "jlong",
				 "jfloat",
				 "jdouble",
				 "void" };
	if (index <= MemberDef.TYPE_VOID) 
	    return resultTypes[index];
	else if (index == MemberDef.TYPE_CLASS) {
	    int startIndex = sigIndex + 1;
	    int endIndex = sig.indexOf(ClassFileParser.SIG_ENDCLASS, startIndex);
	    String clName = sig.substring(startIndex, endIndex);
//	    if (clName.equals("java/lang/Object"))
//		return "void*";
	    String ctype;
	    try {
		ClassDef cl = ClassFileParser.getClassDef(clName);
		ctype = cl.getCType();
	    } catch (Exception e) {
		ctype = null;
	    }
	    if (ctype != null)
		return ctype;
	    else
		return "struct " + clName.replace('/', '_') + "*";
	}
	else
	    return "jref";
    }

    protected String getResultTypeName(MethodDef method) {
	if (method.isConstructor())
	    return "struct " + clazzCName + "*";
	return getTypeName(method.signature, method.getResultIndex());
    }

    protected String getSuffix(String sig, int sigIndex) {
	byte type = MemberDef.getSigType(sig.charAt(sigIndex));
	String[] resultTypes = { "Boolean",
				 "Byte",
				 "Char",
				 "Short",
				 "Int",
				 "Long",
				 "Float",
				 "Double" };
	if (type <= MemberDef.TYPE_DOUBLE) 
	    return resultTypes[type];
	else 
	    return "";
    }

    protected String getResultSuffix(MethodDef method) {
	return getSuffix(method.signature, method.getResultIndex());
    }

    protected String getUnionType(MethodDef method) {
	String sig = method.signature;
	int endArgs = sig.indexOf(ClassFileParser.SIG_ENDMETHOD);
	int type = MemberDef.getSigType(sig.charAt(endArgs + 1));
	String[] resultTypes = { ".z",
				 ".b",
				 ".c",
				 ".s",
				 ".i",
				 ".l",
				 ".f",
				 ".d" };
	if (type <= MemberDef.TYPE_DOUBLE) 
	    return resultTypes[type];
	else
	    return ".r";
    }

    protected void forwardDeclare(MemberDef member) throws IOException {
	int i;
	String sig = member.signature;
	for (i = 0; i < sig.length(); i++) {
	    if (sig.charAt(i) == ClassFileParser.SIG_CLASS) {
		int j = sig.indexOf(ClassFileParser.SIG_ENDCLASS, i);
                String clName = sig.substring(i + 1, j);
                String file;
                try {
                    ClassDef cl = ClassFileParser.getClassDef(clName);
                    file = cl.getInclude();
                } catch (ClassNotFoundException e) {
                    file = null;
                }
                String decl;
                if (file != null)
                    decl = "#include \"" + file + "\"";
                else
                    decl = "struct " + clName.replace('/', '_') + ";";
                if (!forwardDecls.contains(decl)) {
                    out.println(decl);
                    forwardDecls.addElement(decl);
                }
                i = j;
	    }
	}
    }

    protected int consumeArg(String sig, int i) {
	switch (sig.charAt(i++)) {
	  case ClassFileParser.SIG_CLASS:
	    while (sig.charAt(i++) != ClassFileParser.SIG_ENDCLASS);
	    return i;
	  case ClassFileParser.SIG_ARRAY:
	    return consumeArg(sig, i);
	  default:
	    return i;
	}
    }

}
