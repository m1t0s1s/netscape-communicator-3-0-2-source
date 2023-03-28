/*
 * @(#)Type.java	1.16 95/11/15 Arthur van Hoff
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

package sun.tools.java;

import java.util.Hashtable;

/**
 * This class represents an Java Type.<p>
 *
 * It encapsulates an Java type signature and it provides
 * quick access to the components of the type. Note that
 * all types are hashed into a hashtable (typeHash), that
 * means that each distinct type is only allocated once,
 * saving space and making equality checks cheap.<p>
 *
 * For simple types use the constants defined in this class.
 * (Type.tInt, Type.tShort, ...). To create complex types use
 * the static methods Type.tArray, Type.tMethod or Type.tClass.
 *
 * For classes, arrays and method types a sub class of class
 * type is created which defines the extra type components.
 *
 * @see		ArrayType
 * @see		ClassType
 * @see		MethodType
 * @author 	Arthur van Hoff
 * @version 	1.16, 15 Nov 1995
 */
public
class Type implements Constants {
    /**
     * This hashtable is used to cache types
     */
    private static final Hashtable typeHash = new Hashtable(231);

    /**
     * The TypeCode of this type. The value of this field is one
     * of the TC_* contant values defined in Constants.
     * @see Constants
     */
    protected int typeCode;

    /**
     * The TypeSignature of this type. This type signature is
     * equivalent to the runtime type signatures used by the
     * interpreter.
     */
    protected String typeSig;

    /*
     * Predefined types.
     */
    public static final Type noArgs[] 	= new Type[0];
    public static final Type tError 	= new Type(TC_ERROR, 	"?");
    public static final Type tNull 	= new Type(TC_NULL, 	"*");
    public static final Type tVoid 	= new Type(TC_VOID, 	SIG_VOID);
    public static final Type tBoolean 	= new Type(TC_BOOLEAN, 	SIG_BOOLEAN);
    public static final Type tByte 	= new Type(TC_BYTE, 	SIG_BYTE);
    public static final Type tChar 	= new Type(TC_CHAR, 	SIG_CHAR);
    public static final Type tShort 	= new Type(TC_SHORT, 	SIG_SHORT);
    public static final Type tInt 	= new Type(TC_INT, 	SIG_INT);
    public static final Type tFloat 	= new Type(TC_FLOAT, 	SIG_FLOAT);
    public static final Type tLong 	= new Type(TC_LONG, 	SIG_LONG);
    public static final Type tDouble 	= new Type(TC_DOUBLE, 	SIG_DOUBLE);
    public static final Type tObject 	= Type.tClass(idJavaLangObject);
    public static final Type tString 	= Type.tClass(idJavaLangString);
    public static final Type tCloneable = Type.tClass(idJavaLangCloneable);

    /**
     * Create a type given a typecode and a type signature. 
     */
    protected Type(int typeCode, String typeSig) {
	this.typeCode = typeCode;
	this.typeSig = typeSig;
	typeHash.put(typeSig, this);
    }

    /**
     * Return the Java type signature.
     */
    public final String getTypeSignature() {
	return typeSig;
    }

    /**
     * Return the type code.
     */
    public final int getTypeCode() {
	return typeCode;
    }

    /**
     * Return the type mask. The bits in this mask correspond
     * to the TM_* constants defined in Constants. Only one bit
     * is set at a type.
     * @see Constants
     */
    public final int getTypeMask() {
	return 1 << typeCode;
    }

    /**
     * Check for a certain type.
     */
    public final boolean isType(int tc) {
	return typeCode == tc;
    }

    /**
     * Check for a certain set of types.
     */
    public final boolean inMask(int tm) {
	return ((1 << typeCode) & tm) != 0;
    }

    /**
     * Create an array type.
     */
    public static synchronized Type tArray(Type elem) {
	String sig = new String(SIG_ARRAY + elem.getTypeSignature());
	Type t = (Type)typeHash.get(sig);
	if (t == null) {
	    t = new ArrayType(sig, elem);
	}
	return t;
    }
    
    /**
     * Return the element type of an array type. Only works
     * for array types.
     */
    public Type getElementType() {
	throw new CompilerError("getElementType");
    }

    /**
     * Return the array dimension. Only works for
     * array types.
     */
    public int getArrayDimension() {
	return 0;
    }

    /**
     * Create a class type.
     * @arg className the fully qualified class name
     */
    public static synchronized Type tClass(Identifier className) {
	String sig = new String(SIG_CLASS + className.toString().replace('.', SIGC_PACKAGE) + SIG_ENDCLASS);
	Type t = (Type)typeHash.get(sig);
	if (t == null) {
	    t = new ClassType(sig, className);
	}
	return t;
    }

    /**
     * Return the ClassName. Only works on class types.
     */
    public Identifier getClassName() {
	throw new CompilerError("getClassName:" + this);
    }

    /**
     * Create a method type with no arguments.
     */
    public static synchronized Type tMethod(Type ret) {
	return tMethod(ret, noArgs);
    }

    /**
     * Create a method type with arguments.
     */
    public static synchronized Type tMethod(Type returnType, Type argTypes[]) {
	StringBuffer buf = new StringBuffer();
	buf.append(SIG_METHOD);
	for (int i = 0 ; i < argTypes.length ; i++) {
	    buf.append(argTypes[i].getTypeSignature());
	}
	buf.append(SIG_ENDMETHOD);
	buf.append(returnType.getTypeSignature());

	String sig = buf.toString();
	Type t = (Type)typeHash.get(sig);
	if (t == null) {
	    t = new MethodType(sig, returnType, argTypes);
	}
	return t;
    }

    /**
     * Return the return type. Only works for method types.
     */
    public Type getReturnType() {
	throw new CompilerError("getReturnType");
    }

    /**
     * Return the argument types. Only works for method types.
     */
    public Type getArgumentTypes()[] {
	throw new CompilerError("getArgumentTypes");
    }

    /**
     * Create a Type from an Java type signature.
     * @exception CompilerError invalid type signature.
     */
    public static synchronized Type tType(String sig) {
	Type t = (Type)typeHash.get(sig);
	if (t != null) {
	    return t;
	}

	switch (sig.charAt(0)) {
	  case SIGC_ARRAY:
	    return Type.tArray(tType(sig.substring(1)));

	  case SIGC_CLASS:
	    return Type.tClass(Identifier.lookup(sig.substring(1, sig.length() - 1).replace(SIGC_PACKAGE, '.')));

	  case SIGC_METHOD: {
	    Type argv[] = new Type[8];
	    int argc = 0;
	    int i, j;

	    for (i = 1 ; sig.charAt(i) != SIGC_ENDMETHOD ; i = j) {
		for (j = i ; sig.charAt(j) == SIGC_ARRAY ; j++);
		if (sig.charAt(j++) == SIGC_CLASS) {
		    while (sig.charAt(j++) != SIGC_ENDCLASS);
		}
		if (argc == argv.length) {
		    Type newargv[] = new Type[argc * 2];
		    System.arraycopy(argv, 0, newargv, 0, argc);
		    argv = newargv;
		}
		argv[argc++] = tType(sig.substring(i, j));
	    }

	    Type argtypes[] = new Type[argc];
	    System.arraycopy(argv, 0, argtypes, 0, argc);
	    return Type.tMethod(tType(sig.substring(i + 1)), argtypes);
	  }    
	}

	throw new CompilerError("invalid TypeSignature:" + sig);
    }

    /**
     * Check if the type arguments are the same.
     * @return true if both types are method types and the
     * argument types are identical.
     */
    public boolean equalArguments(Type t) {
	return false;
    }

    /**
     * Return the amount of space this type takes up on the
     * Java operand stack. For a method this is equal to the
     * total space taken up by the arguments.
     */
    public int stackSize() {
	switch (typeCode) {
	  case TC_ERROR:
	  case TC_VOID:
	    return 0;
	  case TC_BOOLEAN:
	  case TC_BYTE:
	  case TC_SHORT:
	  case TC_CHAR:
	  case TC_INT:
	  case TC_FLOAT:
	  case TC_ARRAY:
	  case TC_CLASS:
	    return 1;
	  case TC_LONG:
	  case TC_DOUBLE:
	    return 2;
	}
	throw new CompilerError("stackSize " + toString());
    }

    /**
     * Return the type code offset. This offset can be added to
     * an opcode to get the right opcode type. Most opcodes
     * are ordered: int, long, float, double, array. For
     * example: iload, lload fload, dload, aload. So the
     * appropriate opcode is iadd + type.getTypeCodeOffset().
     */
    public int getTypeCodeOffset() {
	switch (typeCode) {
	  case TC_BOOLEAN:
	  case TC_BYTE:
	  case TC_SHORT:
	  case TC_CHAR:
	  case TC_INT:
	    return 0;
	  case TC_LONG:
	    return 1;
	  case TC_FLOAT:
	    return 2;
	  case TC_DOUBLE:
	    return 3;
	  case TC_NULL:
	  case TC_ARRAY:
	  case TC_CLASS:
	    return 4;
	}
	throw new CompilerError("invalid typecode: " + typeCode);
    }
    
    /**
     * Convert a Type to a string, if abbrev is true class names are
     * not fully qualified, if ret is true the return type is included.
     */
    public String typeString(String id, boolean abbrev, boolean ret) {
	String s = null;
	
	switch (typeCode) {
	  case TC_NULL:		s = "null";    break;
	  case TC_VOID:		s = "void";    break;
	  case TC_BOOLEAN:	s = "boolean"; break;
	  case TC_BYTE:		s = "byte";    break;
	  case TC_CHAR:		s = "char";    break;
	  case TC_SHORT:	s = "short";   break;
	  case TC_INT:		s = "int";     break;
	  case TC_LONG:		s = "long";    break;
	  case TC_FLOAT:	s = "float";   break;
	  case TC_DOUBLE:	s = "double";  break;
	  case TC_ERROR:	s = "<error>"; break;
	  default:	    	s = "unknown";
	  }

	return (id.length() > 0) ? s + " " + id : s;
    }

    /**
     * Create a type string, given an identifier.
     */
    public String typeString(String id) {
	return typeString(id, false, true);
    }

    /**
     * Convert to a String
     */
    public String toString() {
	return typeString("", false, true);
    }
}
