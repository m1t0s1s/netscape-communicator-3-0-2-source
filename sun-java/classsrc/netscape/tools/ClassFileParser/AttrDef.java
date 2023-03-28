// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public abstract class AttrDef {
    protected abstract void describe(PrintStream out, int depth);

    protected void nl(PrintStream out, int depth) {
	out.print("\n");
	for (int i = 0; i < depth; i++)
	    out.print(" ");
    }

    public void describe(PrintStream out) {
	describe(out, 0);
    }
}
