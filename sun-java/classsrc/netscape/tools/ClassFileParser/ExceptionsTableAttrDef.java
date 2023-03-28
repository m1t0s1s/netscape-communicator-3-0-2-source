// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public class ExceptionsTableAttrDef extends AttrDef {
    public String[]	exceptions;

    protected void describe(PrintStream out, int depth) {
	out.print("ExceptionsTable:");
	depth += 3;
	int i;
	for (i = 0; i < exceptions.length; i++) {
	    nl(out, depth);
	    out.print(exceptions[i]);
	}
	depth -= 3;
    }
}
