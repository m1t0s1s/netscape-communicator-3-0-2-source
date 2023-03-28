// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public class MethodDef extends MemberDef {
    static final String CONSTRUCTOR_NAME	= "<init>";
    static final String CLASS_INITIALIZER_NAME	= "<clinit>";

    int isConstr = -1;

    public boolean isConstructor() {
	if (isConstr == 0)
	    return false;
	if (isConstr == 1)
	    return true;
	else if (name.equals(CONSTRUCTOR_NAME)) {
	    isConstr = 1;
	    return true;
	}
	else {
	    isConstr = 0;
	    return false;
	}
    }

    String cName = null;

    public String getCName(int methodIndex) {
	if (cName != null)
	    return cName;

	if (isConstructor())
	    cName = "new";
	else if (name.equals(CLASS_INITIALIZER_NAME))
	    cName = "_clinit";
	else {
	    // Substitute underscores for two underscores so that we
	    // don't collide with underscores used to separate method 
	    // and class names:
	    StringBuffer buf = new StringBuffer();
	    for (int i = 0; i < name.length(); i++) {
		char c = name.charAt(i);
		if (c == '_')
		    buf.append("__");
		else
		    buf.append(c);
	    }
	    cName = buf.toString();
	}

	int i, gensym = 0;
	for (i = 0; i < methodIndex; i++) {
	    MethodDef otherMethod = clazz.methods[i];
	    if (otherMethod.name.equals(name))
		gensym++;
	}
	
	if (gensym > 0)
	    cName += "_" + gensym;
	return cName;
    }

    String cppName = null;

    public String getCppName(int methodIndex) {
	if (cppName != null)
	    return cppName;

	if (isConstructor())
	    cppName = "_new";
	else if (name.equals(CLASS_INITIALIZER_NAME))
	    cppName = "_clinit";
	else
	    cppName = name;
	return cppName;
    }

    public int getResultIndex() {
	String sig = signature;
	int endArgs = sig.indexOf(ClassFileParser.SIG_ENDMETHOD);
	return endArgs + 1;
    }

    //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\
    // Varargs Stuff

    static final String VARARG_CLASSNAME = "netscape/tools/jmc/Varargs";

    int vararg = -1;

    public boolean isVararg() {
	if (vararg != -1)
	    return vararg == 1;

	vararg = 0;
	int i;
	for (i = 1; true; i++) {
	    char c = signature.charAt(i);
	    switch (c) {
	      case ClassFileParser.SIG_ENDMETHOD:
		return vararg == 1;
	      case ClassFileParser.SIG_CLASS:
		int j = signature.indexOf(ClassFileParser.SIG_ENDCLASS, i);
		if (signature.substring(i + 1, j).equals(VARARG_CLASSNAME))
		    vararg = 1;
		i = j;
		break;
	      default:
		// if Vararg isn't last, then treat it as just a String
		// in a non-vararg method
		vararg = 0;
	    }
	}
    }

    String[] paramNames;
    boolean paramNamesComputed = false;

    public String[] getParamNames() {
	if (!paramNamesComputed) {
//	    System.out.println("attributes.length = "+attributes.length);
	    for (int i = 0; i < attributes.length; i++) {
		AttrDef attr = attributes[i];
//		System.out.println("attr = "+attr);
		if (attr instanceof CodeAttrDef) {
		    CodeAttrDef codeAttr = (CodeAttrDef)attr;
//		    System.out.println("\tcodeAttr = "+codeAttr);
		    for (int j = 0; j < codeAttr.attributes.length; j++) {
			AttrDef attrAttr = codeAttr.attributes[j];
//			System.out.println("\t\tattrAttr = "+attrAttr);
			if (attrAttr instanceof LocalVariableTableAttrDef) {
			    LocalVariableTableAttrDef table =
				(LocalVariableTableAttrDef)attrAttr;
			    LocalVariableAttrDef[] locals = table.localVariables;
//			    System.out.println("\t\t\tlocals = "+locals);
			    paramNames = new String[locals.length];
			    for (int k = 0; k < locals.length; k++) {
				paramNames[k] = locals[k].name;
			    }
			}
		    }
		}
	    }
	    paramNamesComputed = true;
	}
	return paramNames;
    }

    public String[] getExceptionsThrown() {
	for (int i = 0; i < attributes.length; i++) {
	    AttrDef attr = attributes[i];
	    if (attr instanceof ExceptionsTableAttrDef) {
		ExceptionsTableAttrDef table = (ExceptionsTableAttrDef)attr;
		return table.exceptions;
	    }
	}
	return null;
    }

    public void describeAttributes(PrintStream out) {
	out.println("/* Attributes of " + name + signature);
	for (int i = 0; i < attributes.length; i++) {
	    attributes[i].describe(out);
	}
	out.println("*/");
    }
}
