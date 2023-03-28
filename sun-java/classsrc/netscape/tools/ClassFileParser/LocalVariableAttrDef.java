// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public class LocalVariableAttrDef extends AttrDef {
    public short	startPC;
    public short	length;
    public String	name;
    public String	signature;
    public short	slot;

    protected void describe(PrintStream out, int depth) {
	out.print("LocalVariable: startPC=" + startPC
		  + " length=" + length
		  + " name=" + name
		  + " signature=" + signature
		  + " slot=" + slot);
    }
}
