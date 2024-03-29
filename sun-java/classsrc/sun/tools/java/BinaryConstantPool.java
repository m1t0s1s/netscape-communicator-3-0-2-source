/*
 * @(#)BinaryConstantPool.java	1.16 95/09/14 Arthur van Hoff
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

import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.util.Vector;
import java.util.Hashtable;

/**
 * This class is used to represent a constant table once
 * it is read from a class file.
 */
public final
class BinaryConstantPool implements Constants {
    private byte types[];
    private Object cpool[];

    /**
     * Constructor
     */
    BinaryConstantPool(DataInputStream in) throws IOException {
	types = new byte[in.readShort()];
	cpool = new Object[types.length];
	for (int i = 1 ; i < cpool.length ; i++) {
	    int j = i;
	    switch(types[i] = in.readByte()) {
	      case CONSTANT_UTF8:
		cpool[i] = in.readUTF();
		break;

	      case CONSTANT_INTEGER:
		cpool[i] = new Integer(in.readInt());
		break;
	      case CONSTANT_FLOAT:
		cpool[i] = new Float(in.readFloat());
		break;
	      case CONSTANT_LONG:
		cpool[i++] = new Long(in.readLong());
		break;
	      case CONSTANT_DOUBLE:
		cpool[i++] = new Double(in.readDouble());
		break;

	      case CONSTANT_CLASS:
	      case CONSTANT_STRING:
		cpool[i] = new Integer(in.readShort());
		break;
		
	      case CONSTANT_FIELD:
	      case CONSTANT_METHOD:
	      case CONSTANT_INTERFACEMETHOD:
	      case CONSTANT_NAMEANDTYPE:
		cpool[i] = new Integer((in.readShort() << 16) | in.readShort());
		break;

	      case 0:
	      default:
		throw new ClassFormatError("invalid constant type: " + (int)types[i]);
	    }
	}
    }

    /**
     * get a integer
     */
    public int getInteger(int n) {
	return (n == 0) ? 0 : ((Number)cpool[n]).intValue();
    }

    /**
     * get a value
     */
    public Object getValue(int n) {
	return (n == 0) ? null : cpool[n];
    }
    
    /**
     * get a string
     */
    public String getString(int n) {
	return (n == 0) ? null : (String)cpool[n];
    }

    /**
     * get an identifier
     */
    public Identifier getIdentifier(int n) {
	return (n == 0) ? null : Identifier.lookup(getString(n));
    }

    /**
     * get class declaration
     */
    public ClassDeclaration getDeclarationFromName(Environment env, int n) {
	return (n == 0) ? null : env.getClassDeclaration(Identifier.lookup(getString(n).replace('/','.')));
    }

    /**
     * get class declaration
     */
    public ClassDeclaration getDeclaration(Environment env, int n) {
	return (n == 0) ? null : getDeclarationFromName(env, getInteger(n));
    }

    /**
     * get a type from a type signature
     */
    public Type getType(int n) {
	return Type.tType(getString(n));
    }

    /**
     * get the type of constant given an index
     */
    public int getConstantType(int n) {
	return types[n];
    }

    /** 
     * get the n-th constant from the constant pool
     */
    public Object getConstant(int n, Environment env) {
	int constant_type = getConstantType(n);
	switch (constant_type) {
	    case CONSTANT_INTEGER:
	    case CONSTANT_FLOAT:
	    case CONSTANT_LONG:
	    case CONSTANT_DOUBLE:
		return getValue(n);

	    case CONSTANT_CLASS:
		return getDeclaration(env, n);

	    case CONSTANT_STRING:
		return getString(getInteger(n));
		
	    case CONSTANT_FIELD:
	    case CONSTANT_METHOD:
	    case CONSTANT_INTERFACEMETHOD: 
		try {
		    int key = getInteger(n);
		    ClassDefinition clazz = 
			getDeclaration(env, key >> 16).getClassDefinition(env);
		    int name_and_type = getInteger(key & 0xFFFF);
		    Identifier id = getIdentifier(name_and_type >> 16);
		    Type type = getType(name_and_type & 0xFFFF);

		    for (FieldDefinition field = clazz.getFirstMatch(id); 
		         field != null; 
			 field = field.getNextMatch()) {
			Type field_type = field.getType();
			if ((constant_type == CONSTANT_FIELD) 
			    ? (field_type == type) 
			    : (field_type.equalArguments(type)))
			    return field;
		    }
		} catch (ClassNotFound e) {
		}
		return null;

	    default:
		throw new ClassFormatError("invalid constant type: " + 
					      constant_type);
	}
    }
    

    /**
     * Get a list of dependencies, ie: all the classes referenced in this
     * constant pool.
     */
    public Vector getDependencies(Environment env) {
	Vector v = new Vector();
	for (int i = 1 ; i < cpool.length ; i++) {
	    switch(types[i]) {
	      case CONSTANT_CLASS:
		v.addElement(getDeclarationFromName(env, getInteger(i)));
		break;
	    }
	}
	return v;
    }

    Hashtable indexHashObject, indexHashAscii;
    Vector MoreStuff;

    /** 
     * Find the index of an Object in the constant pool 
     */
    public int indexObject(Object obj, Environment env) {
	if (indexHashObject == null) 
	    createIndexHash(env);
	Integer result = (Integer)indexHashObject.get(obj);
	if (result == null) 
	    throw new IndexOutOfBoundsException("Cannot find object " + obj + " of type " + 
				obj.getClass() + " in constant pool");
	return result.intValue();
    }

    /** 
     * Find the index of an ascii string in the constant pool.  If it's not in
     * the constant pool, then add it at the end.
     */
    public int indexString(String string, Environment env) {
	if (indexHashObject == null) 
	    createIndexHash(env);
	Integer result = (Integer)indexHashAscii.get(string);
	if (result == null) { 
	    if (MoreStuff == null) MoreStuff = new Vector();
	    result = new Integer(cpool.length + MoreStuff.size());
	    MoreStuff.addElement(string);
	    indexHashAscii.put(string, result);
	} 
	return result.intValue();
    }

    /** 
     * Create a hash table of all the items in the constant pool that could
     * possibly be referenced from the outside.
     */

    public void createIndexHash(Environment env) {
	indexHashObject = new Hashtable();
	indexHashAscii = new Hashtable();
	for (int i = 1; i < cpool.length; i++) {
	    if (types[i] == CONSTANT_UTF8) { 
		indexHashAscii.put(cpool[i], new Integer(i));
	    } else { 
		try {
		    indexHashObject.put(getConstant(i, env), new Integer(i));
		} catch (ClassFormatError e) { }
	    }
	}
    }


    /** 
     * Write out the contents of the constant pool, including any additions
     * that have been added.
     */
    public void write(DataOutputStream out, Environment env) throws IOException {
	int length = cpool.length;
	if (MoreStuff != null) 
	    length += MoreStuff.size();
	out.writeShort(length);
	for (int i = 1 ; i < cpool.length; i++) {
	    int type = types[i];
	    Object x = cpool[i];
	    out.writeByte(type);
	    switch (type) {
	        case CONSTANT_UTF8:
		    out.writeUTF((String) x);
		    break;
	        case CONSTANT_INTEGER:
		    out.writeInt(((Number)x).intValue());
		    break;
	        case CONSTANT_FLOAT:
		    out.writeFloat(((Number)x).floatValue());
		    break;
	        case CONSTANT_LONG:
		    out.writeLong(((Number)x).longValue());
		    i++;
		    break;
	        case CONSTANT_DOUBLE:
		    out.writeDouble(((Number)x).doubleValue());
		    i++;
		    break;
	        case CONSTANT_CLASS:
	        case CONSTANT_STRING:
		    out.writeShort(((Number)x).intValue());
		    break;
	        case CONSTANT_FIELD:
	        case CONSTANT_METHOD:
	        case CONSTANT_INTERFACEMETHOD:
	        case CONSTANT_NAMEANDTYPE: {
		    int value = ((Number)x).intValue();
		    out.writeShort(value >> 16);
		    out.writeShort(value & 0xFFFF);
		    break;
		}
	        default:
	             throw new ClassFormatError("invalid constant type: " 
						   + (int)types[i]);
	    }
	}
	for (int i = cpool.length; i < length; i++) {
	    String string = (String)(MoreStuff.elementAt(i - cpool.length));
	    out.writeByte(CONSTANT_UTF8);
	    out.writeUTF(string);
	}
    }

}
