// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.jmc;

import netscape.tools.jmc.*;
import netscape.tools.ClassFileParser.*;
import java.io.*;

public class Main extends netscape.tools.jric.Main {
    protected static final int MODE_INTERFACE	 = 2;
    protected static final int MODE_MODULE	 = 3;

    // main entry point (why doesn't this get inherited?)
    public static void main(String[] argv) throws Exception {
	new Main(argv);
    }

    protected Main(String[] argv) {
	super(argv);
    }

    String implName;
    boolean javaImpl;

    protected int processArg(String[] argv, int i) {
	if (argv[i].equals("-interface")) {
	    outputMode = MODE_INTERFACE;
	    return ++i;
	}
	if (argv[i].equals("-cimpl")) {
	    implName = argv[++i];
	    javaImpl = false;
	    return ++i;
	}
	if (argv[i].equals("-jimpl")) {
	    implName = argv[++i];
	    javaImpl = true;
	    return ++i;
	}
	if (argv[i].equals("-module")) {
	    outputMode = MODE_MODULE;
	    return ++i;
	}
	else
	    return super.processArg(argv, i);
    }

    protected netscape.tools.jric.Generator 
    getGenerator(ClassDef clazz) throws IOException {
	if (outputMode == MODE_INTERFACE) {
	    if (implName != null) {
		if (javaImpl)
		    return new JavaImplHeaderGen(this, clazz);
		else
		    return new CImplHeaderGen(this, clazz);
	    }
	    else
		return new InterfaceGenerator(this, clazz);
	}
	if (outputMode == MODE_MODULE) {
	    if (implName != null) {
		if (javaImpl)
		    return new JavaImplStubGen(this, clazz);
		else
		    return new CImplStubGen(this, clazz);
	    }
	    else
		return new ModuleGenerator(this, clazz);
	}
	else {
	    return super.getGenerator(clazz);
	}
    }

}
