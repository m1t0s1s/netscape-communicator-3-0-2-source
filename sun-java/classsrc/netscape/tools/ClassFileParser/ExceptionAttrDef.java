// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public class ExceptionAttrDef extends AttrDef {
    public short		startPC;
    public short		endPC;
    public short		handlerPC;
    public String		catchType;

    protected void describe(PrintStream out, int depth) {
	out.print("Exception: startPC=" + startPC
	    + " endPC=" + endPC
	    + " handlerPC=" + handlerPC
	    + " catchType=" + catchType);
    }
}
