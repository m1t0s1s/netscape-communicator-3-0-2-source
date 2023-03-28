// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public class LineNumberAttrDef extends AttrDef {
    public short		startPC;
    public short		lineNumber;

    protected void describe(PrintStream out, int depth) {
	out.print("LineNumber: startPC=" + startPC
	    + " lineNumber=" + lineNumber);
    }
}
