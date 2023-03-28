// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.jric;

import netscape.tools.ClassFileParser.*;
import java.io.*;

/**
 * Holder of common utility routines for the javah header and stub
 * generators.
 */
public abstract
class NativesGenerator extends Generator {
    protected NativesGenerator(Main global, ClassDef clazz) {
	super(global, clazz);
    }

    protected void printGetterArgDecl(FieldDef field) {
	out.print("(JRIEnv* env, ");
	if (field.hasAccess(ClassFileParser.ACC_STATIC))
	    out.print("struct java_lang_Class* clazz)");
	else
	    out.print(clazzCName + "* obj)");
    }

    protected void printSetterArgDecl(FieldDef field) {
	out.print("(JRIEnv* env, ");
	if (field.hasAccess(ClassFileParser.ACC_STATIC))
	    out.print("struct java_lang_Class* clazz, ");
	else
	    out.print(clazzCName + "* obj, ");
	String resultType = getTypeName(field.signature, 0);
	out.print(resultType + " v)");
    }

    protected void printGetterArgUse(FieldDef field, boolean withFieldThunk) {
	out.print("(env, ");
	if (field.hasAccess(ClassFileParser.ACC_STATIC))
	    out.print("clazz");
	else
	    out.print("obj");
	if (withFieldThunk) {
	    out.print(", fieldID_" + field.getCName());
	}
	out.print(")");
    }

    protected void printSetterArgUse(FieldDef field, boolean withFieldThunk) {
	out.print("(env, ");
	if (field.hasAccess(ClassFileParser.ACC_STATIC))
	    out.print("clazz, ");
	else
	    out.print("obj, ");
	if (withFieldThunk) {
	    out.print("fieldID_" + field.getCName() + ", ");
	}
	out.print("v)");
    }

    protected void printMethodArgUse(String prefix, MethodDef method, int methodIndex,
				     boolean withMethodThunk, boolean cpp) {
	boolean isStatic = method.hasAccess(ClassFileParser.ACC_STATIC);
	out.print("(env, ");
	if (withMethodThunk) {
	    if (method.isConstructor())
		out.print("JRI_NewObject_op, ");
	    else
		out.print("JRI_Call" + (isStatic ? "Static" : "") 
			  + "Method" + getResultSuffix(method) + "_op, ");
	}
	if (isStatic || method.isConstructor())
	    out.print("clazz");
	else if (cpp)
	    out.print("this");
	else
	    out.print("obj");
	if (withMethodThunk) {
	    out.print(", methodID_" + prefix + "_" + method.getCName(methodIndex));
	}
	int i = 1;
	String sig = method.signature;
	String[] args = method.getParamNames();
	char dummyArg = 'a';
	int j = (isStatic ? 0 : 1);	// start with 1 to avoid 'this'
	while (sig.charAt(i) != ClassFileParser.SIG_ENDMETHOD) {
	    out.print(", ");
	    i = consumeArg(sig, i);
	    if (args != null)
		out.print(args[j++]);
	    else
		out.print(dummyArg++);
	}
	out.print(")");
    }

    protected void printMethodFunctionSig(String prefix, MethodDef method,
					  int methodIndex, boolean printAsNative) {
	String resultType = getResultTypeName(method);
	out.println("JRI_PUBLIC_API(" + resultType + ")");
	if (printAsNative)
	    out.print("native_");
	out.print(prefix + "_" + method.getCName(methodIndex));
	printMethodArgDecl(method, false);
    }

    protected void printMethodArgDecl(MethodDef method, boolean cpp) {
	boolean isStatic = method.hasAccess(ClassFileParser.ACC_STATIC);
	out.print("(JRIEnv* env");
	if (isStatic || method.isConstructor())
	    out.print(", struct java_lang_Class* clazz");
	else if (!cpp)
	    out.print(", struct " + clazzCName + "* obj");
	int i = 1;
	String sig = method.signature;
	
	String[] args = method.getParamNames();
	char dummyArg = 'a';
	int j = (isStatic ? 0 : 1);	// start with 1 to avoid 'this'
	while (sig.charAt(i) != ClassFileParser.SIG_ENDMETHOD) {
	    out.print(", ");
	    out.print(getTypeName(sig, i) + " ");
	    i = consumeArg(sig, i);
	    if (args != null)
		out.print(args[j++]);
	    else
		out.print(dummyArg++);
	}
	out.print(")");
    }

}
