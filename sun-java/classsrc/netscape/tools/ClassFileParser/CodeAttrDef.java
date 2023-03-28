// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;
import java.io.PrintStream;

public class CodeAttrDef extends AttrDef {
    public short		maxStack;
    public short		maxLocals;
    public byte[]		code;
    public ExceptionAttrDef[]	exceptions;
    public AttrDef[]		attributes;

    protected void describe(PrintStream out, int depth) {
	out.print("Code: length=" + code.length
		  + " maxStack=" + maxStack
		  + " maxLocals=" + maxLocals);
	depth += 6;
	int i;
	boolean label = true;
	for (i = 0; i < exceptions.length; i++) {
	    if (label) {
		nl(out, depth - 3);
		out.print("Exceptions: ");
		label = false;
	    }
	    nl(out, depth);
	    exceptions[i].describe(out, depth);
	}
	label = true;
	for (i = 0; i < attributes.length; i++) {
	    if (label) {
		nl(out, depth - 3);
		out.print("Attributes: ");
		label = false;
	    }
	    nl(out, depth);
	    attributes[i].describe(out, depth);
	}
	depth -= 6;
    }
}
