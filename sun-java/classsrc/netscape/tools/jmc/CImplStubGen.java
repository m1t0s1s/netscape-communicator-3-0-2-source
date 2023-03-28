// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.jmc;

import netscape.tools.jmc.*;
import netscape.tools.jric.*;
import netscape.tools.ClassFileParser.*;
import java.io.*;
import java.util.Vector;

public 
class CImplStubGen extends ImplStubGen {

    public CImplStubGen(netscape.tools.jmc.Main global, ClassDef clazz) {
	super(global, clazz);
    }

    protected void generateOutput() throws IOException {
	int i;
	    
	Vector interfaces = getInterfaces(clazz);
	String prefix = getImplName();

	out.println("#include \"M" + getImplName() + ".h\"\n");

	printBlockComment("Pre-Implemented Operations");

	// GetInterface
	out.println("JRI_PUBLIC_API(void*)");
	out.println(prefix + "_GetInterface("
		    + prefix + "* self, jint op, jint iid)");
	out.println("{");
	out.println("\treturn NULL;");
	out.println("}\n");

	// AddRef
	out.println("JRI_PUBLIC_API(void)");
	out.println(prefix + "_AddRef("
		    + prefix + "* self, jint op)");
	out.println("{");
	out.println("}\n");

	// Release
	out.println("JRI_PUBLIC_API(void)");
	out.println(prefix + "_Release("
		    + prefix + "* self, jint op)");
	out.println("{");
	out.println("}\n");

	// Description
	out.println("JRI_PUBLIC_API(const char**)");
	out.println(prefix + "_Description("
		    + prefix + "* self, jint op)");
	out.println("{");
	out.println("\treturn " + clazzCName + "Descriptor;");
	out.println("}\n");

	generateJumpTable(interfaces, prefix);

	printBlockComment("Factory Operations");

	out.println("JRI_PUBLIC_API(" + prefix + "*)");
	out.println(prefix + "Factory_Create(jint size)");
	out.println("{");
	out.println("\tJMCModule* self = (JMCModule*)malloc(size);");
	out.println("\tself->vtable = (void*)&" + prefix + "_interface;");
	out.println("\tself->refcount = 0;");
	out.println("\treturn (" + prefix + "*)self;");
	out.println("}\n");
    }

    //\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\
    
    protected void printMethodType(String prefix, MethodDef method,
				   int methodIndex, byte varargsType) 
		throws IOException {
	String resultType = getResultTypeName(method);
	out.print("(" + resultType + "\t(*)");
	printInterfaceArgDecl(method, methodIndex, varargsType);
	out.print(")\n\t\t" + prefix + "_"
		  + method.getCName(methodIndex));
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
