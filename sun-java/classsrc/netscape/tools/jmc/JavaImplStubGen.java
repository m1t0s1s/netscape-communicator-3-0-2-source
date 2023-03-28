// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.jmc;

import netscape.tools.jmc.*;
import netscape.tools.jric.*;
import netscape.tools.ClassFileParser.*;
import java.io.*;
import java.util.Vector;

public 
class JavaImplStubGen extends ImplStubGen {

    public JavaImplStubGen(netscape.tools.jmc.Main global, ClassDef clazz) {
	super(global, clazz);
    }

    protected void generateOutput() throws IOException {
	int i;
	    
	Vector interfaces = getInterfaces(clazz);
	String prefix = getImplName();

	out.println("#include \"M" + prefix + ".h\"");
	out.println("#define IMPLEMENT_" + prefix + "");
	out.println("#include \"" + prefix + ".h\"\n");

	generateJumpTable(interfaces, "JMCJavaObject");

	printBlockComment("Factory Operations");

	out.println("JRI_PUBLIC_API(" + prefix + "*)");
	out.println(prefix + "Factory_Create(jint size)");
	out.println("{");
	out.println("\tJRIEnv* env = JRI_GetCurrentEnv();");
	out.println("\tstruct java_lang_Class* clazz = use_" + prefix + "(env);");
	out.println("\tJMCJavaObject* self = (JMCJavaObject*)");
	out.println("\t\tmalloc(sizeof(JMCJavaObject));");
	out.println("\tself->super.vtable = (void*)&" + prefix + "_interface;");
	out.println("\tself->super.refcount = 0;");
	out.println("\tself->obj = " + prefix + "_new(env, clazz);");
	out.println("\tself->methodIDs = (JRIMethodID*)");
	out.println("\t\tmalloc(sizeof(JRIMethodID) * (sizeof("
		    + prefix + "_interface) / sizeof(void*)));");
	out.println("\tself->description = " + clazzCName + "Descriptor;");
	out.println("\t");
	out.println("\treturn (" + prefix + "*)self;");
	out.println("}\n");
    }

    //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//
    
    protected void printMethodType(String prefix, MethodDef method, 
				   int methodIndex, byte varargsType) 
		throws IOException {
	String resultType = getResultTypeName(method);
	out.print("(" + resultType + "\t(*)");
	printInterfaceArgDecl(method, methodIndex, varargsType);
	String resultSuffix = getResultSuffix(method);
	out.print(")\n\t\t" + "JMCJavaObject_CallMethod" + resultSuffix);
	String str;
	switch (varargsType) {
	  case VARARGS_VA_LIST:
	    str = "V";
	    break;
	  case VARARGS_ARRAY:
	    str = "A";
	    break;
	  default:
	    str = "";
	    break;
	}
	out.print(str);
    }

}
