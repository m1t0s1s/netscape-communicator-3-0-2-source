// Copyright (c) 1995 Netscape Communications Corporation. All rights reserved. 

package netscape.tools.ClassFileParser;

import netscape.tools.ClassFileParser.*;
import java.io.PrintStream;

public class MemberDef {
    public short	flags;
    public String	name;
    public String	signature;
    public AttrDef[]	attributes;
    public ClassDef	clazz;	// back pointer

    public boolean hasAccess(int accessMask) {
	return (flags & accessMask) == accessMask;
    }

    public void describe(PrintStream out) {
	out.print("/*** ");
	if (hasAccess(ClassFileParser.ACC_PUBLIC))
	    out.print("public ");
	if (hasAccess(ClassFileParser.ACC_PROTECTED))
	    out.print("protected ");
	if (hasAccess(ClassFileParser.ACC_PRIVATE))
	    out.print("private ");
	if (hasAccess(ClassFileParser.ACC_STATIC))
	    out.print("static ");
	if (hasAccess(ClassFileParser.ACC_FINAL))
	    out.print("final ");
	if (hasAccess(ClassFileParser.ACC_NATIVE))
	    out.print("native ");
	out.print(name + " " + signature);
	out.println(" ***/");
    }

    public static final byte TYPE_BOOLEAN	= 0;
    public static final byte TYPE_BYTE		= 1;
    public static final byte TYPE_CHAR		= 2;
    public static final byte TYPE_SHORT		= 3;
    public static final byte TYPE_INT		= 4;
    public static final byte TYPE_LONG		= 5;
    public static final byte TYPE_FLOAT		= 6;
    public static final byte TYPE_DOUBLE	= 7;
    public static final byte TYPE_VOID		= 8;
    public static final byte TYPE_CLASS		= 9;
    public static final byte TYPE_ARRAY		= 10;
    public static final byte TYPE_OTHER		= 11;
    
    public static byte getSigType(char sigChar) {
        switch (sigChar) {
	  case ClassFileParser.SIG_BOOLEAN:	return TYPE_BOOLEAN;
	  case ClassFileParser.SIG_BYTE:	return TYPE_BYTE;
	  case ClassFileParser.SIG_CHAR:	return TYPE_CHAR;
	  case ClassFileParser.SIG_SHORT:	return TYPE_SHORT;
	  case ClassFileParser.SIG_INT:		return TYPE_INT;
	  case ClassFileParser.SIG_LONG:	return TYPE_LONG;
	  case ClassFileParser.SIG_FLOAT:	return TYPE_FLOAT;
	  case ClassFileParser.SIG_DOUBLE:	return TYPE_DOUBLE;
	  case ClassFileParser.SIG_VOID:	return TYPE_VOID;
	  case ClassFileParser.SIG_CLASS:	return TYPE_CLASS;
	  case ClassFileParser.SIG_ARRAY:	return TYPE_ARRAY;
	  default:				return TYPE_OTHER;
	}
    }
}
