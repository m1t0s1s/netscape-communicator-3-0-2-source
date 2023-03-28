// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public class SourceFileAttrDef extends AttrDef {
    public String	sourceFile;

    protected void describe(PrintStream out, int depth) {
	out.print("SourceFile: " + sourceFile);
    }
}
