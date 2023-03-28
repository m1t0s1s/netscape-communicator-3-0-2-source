// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public class LineNumberTableAttrDef extends AttrDef {
    public LineNumberAttrDef[]	lineNumbers;

    protected void describe(PrintStream out, int depth) {
	out.print("LineNumberTable:");
	depth += 3;
	int i;
	for (i = 0; i < lineNumbers.length; i++) {
	    nl(out, depth);
	    lineNumbers[i].describe(out, depth);
	}
	depth -= 3;
    }
}
