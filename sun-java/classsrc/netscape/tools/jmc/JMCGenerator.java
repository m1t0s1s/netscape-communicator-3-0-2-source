// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.jmc;

import netscape.tools.jmc.*;
import netscape.tools.jric.*;
import netscape.tools.ClassFileParser.*;
import java.io.*;
import java.util.Vector;

public abstract
class JMCGenerator extends netscape.tools.jric.Generator {

    public JMCGenerator(netscape.tools.jmc.Main global, ClassDef clazz) {
	super(global, clazz);
    }

    protected static final byte VARARGS_NONE	= 0;
    protected static final byte VARARGS_ELIPSIS	= 1;
    protected static final byte VARARGS_VA_LIST	= 2;
    protected static final byte VARARGS_ARRAY	= 3;

    protected Vector getInterfaces(ClassDef clazz) {
	Vector result = new Vector();
	addInterfaces(clazz, result);
	return result;
    }

    protected void addInterfaces(ClassDef clazz, Vector interfaces) {
	interfaces.removeElement(clazz);
	interfaces.addElement(clazz);	// move it to the end
	if (clazz.interfaces != null) {
	    int i;
	    for (i = 0; i < clazz.interfaces.length; i++) {
		addInterfaces(clazz.interfaces[i], interfaces);
	    }
	    /*
	    if (clazz.superclass != null)
		addInterfaces(clazz.superclass, interfaces);
	    */
	}
    }

    protected String getImplName() {
	String name = ((netscape.tools.jmc.Main)global).implName;
	return name;
    }

    protected void printInterfaceArgDecl(MethodDef method, int methodIndex, byte varargsType)
		throws IOException {
	if (method.hasAccess(ClassFileParser.ACC_STATIC) || method.isConstructor())
	    out.print("(struct java_lang_Class* clazz, jint op");
	else
	    out.print("(" + clazzCName + "* self, jint op");
	int i = 1;
	String sig = method.signature;
	String[] args = method.getParamNames();
//	method.describeAttributes(System.out);
	char dummyArg = 'a';
	boolean isStatic = method.hasAccess(ClassFileParser.ACC_STATIC);
	int j = (isStatic ? 0 : 1);	// start with 1 to avoid 'this'
	while (sig.charAt(i) != ClassFileParser.SIG_ENDMETHOD) {
	    String arg;
	    if (args != null)
		arg = args[j++];
	    else
		arg = String.valueOf(dummyArg++);
	    out.print(", ");
	    boolean isArray = sig.charAt(i) == ClassFileParser.SIG_ARRAY;
	    if (isArray) i++;
	    String type = getTypeName(sig, i);
	    i = consumeArg(sig, i);
	    if (type.equals("struct netscape_tools_jmc_Varargs*")) {
		switch (varargsType) {
		  case VARARGS_ELIPSIS:
		    out.print("...");
		    break;
		  case VARARGS_VA_LIST:
		    out.print("va_list " + arg);
		    break;
		  case VARARGS_ARRAY:
		    out.print("JRIValue* " + arg);
		    break;
		  default:
		    throw new IOException("Bad argument to printInterfaceArgDecl");
		}
	    }
	    else {
		out.print(type + (isArray ? "* " : " ") + arg);	/* XXX wrong */
	    }
	    if (isArray)
		out.print(", jsize " + arg + "_length");
	}
	String[] exceptions = method.getExceptionsThrown();
	if (exceptions != null) {
	    out.print(", JMCException* *exceptionThrown");
	}
	out.print(")");
    }

    protected void printMethodFunctionSig(String prefix, MethodDef method,
					  int methodIndex) {
	String resultType = getResultTypeName(method);
	out.println("JRI_PUBLIC_API(" + resultType + ")");
	out.print(prefix + "_" + method.getCName(methodIndex));
	printMethodArgDecl(prefix, method, false);
    }

    protected void printMethodArgDecl(String prefix, MethodDef method, boolean cpp) {
	boolean isStatic = method.hasAccess(ClassFileParser.ACC_STATIC);
	out.print("(");
	if (isStatic || method.isConstructor())
	    out.print("struct java_lang_Class* clazz");
	else if (!cpp)
	    out.print(prefix + "* self");
	out.print(", jint op");
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
	String[] exceptions = method.getExceptionsThrown();
	if (exceptions != null) {
	    out.print(", JMCException* *exceptionThrown");
	}
	out.print(")");
    }

}




