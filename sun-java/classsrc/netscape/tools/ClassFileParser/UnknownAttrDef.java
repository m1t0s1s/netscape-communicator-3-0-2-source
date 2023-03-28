// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public class UnknownAttrDef extends AttrDef {
    public String		name;
    public int			length;

    public void describe(PrintStream out, int depth) {
	out.print("UnknownAttribute: name=" + name
		  + " length=" + length);
    }
}
