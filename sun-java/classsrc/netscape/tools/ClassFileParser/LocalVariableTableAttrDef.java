// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public class LocalVariableTableAttrDef extends AttrDef {
    public LocalVariableAttrDef[]	localVariables;

    protected void describe(PrintStream out, int depth) {
	out.print("LocalVariableTable:");
	depth += 3;
	int i;
	for (i = 0; i < localVariables.length; i++) {
	    nl(out, depth);
	    localVariables[i].describe(out, depth);
	}
	depth -= 3;
    }
}
