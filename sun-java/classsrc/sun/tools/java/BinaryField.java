/*
 * @(#)BinaryField.java	1.16 95/09/14 Arthur van Hoff
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

import sun.tools.tree.*;
import java.util.Vector;
import java.util.Hashtable;
import java.io.IOException;
import java.io.DataInputStream;
import java.io.ByteArrayInputStream;

/**
 * This class represents a binary field
 */
public final
class BinaryField extends FieldDefinition {
    Expression value;
    BinaryAttribute atts;
    
    /**
     * Constructor
     */
    public BinaryField(ClassDefinition clazz, int modifiers, Type type,
		       Identifier name, BinaryAttribute atts) {
	super(0, clazz, modifiers, type, name, null, null);
	this.atts = atts;
    }

    /**
     * Inline allowed (currently only allowed for the constructor of Object).
     */
    public boolean isInlineable(Environment env, boolean fromFinal) {
	return isConstructor() && (getClassDefinition().getSuperClass() == null);
    }

    /**
     * Get arguments
     */
    public Vector getArguments() {
	if (isConstructor() && (getClassDefinition().getSuperClass() == null)) {
	    Vector v = new Vector();
	    v.addElement(new LocalField(0, getClassDefinition(), 0,
					getClassDefinition().getType(), idThis));
	    return v;
	}
	return null;
    }

    /**
     * Get exceptions
     */
    public ClassDeclaration[] getExceptions(Environment env) {
	if ((!isMethod()) || (exp != null)) {
	    return exp;
	}
	byte data[] = getAttribute(idExceptions);
	if (data == null) {
	    return new ClassDeclaration[0];
	}

	try {
	    BinaryConstantPool cpool = ((BinaryClass)getClassDefinition()).getConstants();
	    DataInputStream in = new DataInputStream(new ByteArrayInputStream(data));
	    int n = in.readShort();
	    exp = new ClassDeclaration[n];
	    for (int i = 0 ; i < n ; i++) {
		exp[i] = cpool.getDeclaration(env, in.readShort());
	    }
	    return exp;
	} catch (IOException e) {
	    throw new CompilerError(e);
	}
    }

    /**
     * Get documentation
     */
    public String getDocumentation() {
	if (documentation != null) {
	    return documentation;
	}
	byte data[] = getAttribute(idDocumentation);
	if (data == null) {
	    return null;
	}
	try {
	    return documentation = new DataInputStream(new ByteArrayInputStream(data)).readUTF();
	} catch (IOException e) {
	    throw new CompilerError(e);
	}
    }

    /**
     * Get the value
     */
    public Node getValue(Environment env) {
	if (isMethod()) {
	    return null;
	}
	if (!isFinal()) {
	    return null;
	}
	if (getValue() != null) {
	    return (Expression)getValue();
	}
	byte data[] = getAttribute(idConstantValue);
	if (data == null) {
	    return null;
	}

	try {
	    BinaryConstantPool cpool = ((BinaryClass)getClassDefinition()).getConstants();
	    Object obj = cpool.getValue(new DataInputStream(new ByteArrayInputStream(data)).readShort());
	    switch (getType().getTypeCode()) {
	      case TC_BOOLEAN:
		setValue(new BooleanExpression(0, ((Number)obj).intValue() != 0));
		break;
	      case TC_BYTE:
	      case TC_SHORT:
	      case TC_CHAR:
	      case TC_INT:
		setValue(new IntExpression(0, ((Number)obj).intValue()));
		break;
	      case TC_LONG:
		setValue(new LongExpression(0, ((Number)obj).longValue()));
		break;
	      case TC_FLOAT:
		setValue(new FloatExpression(0, ((Number)obj).floatValue()));
		break;
	      case TC_DOUBLE:
		setValue(new DoubleExpression(0, ((Number)obj).doubleValue()));
		break;
	      case TC_CLASS:
		setValue(new StringExpression(0, (String)cpool.getValue(((Number)obj).intValue())));
		break;
	    }
	    return (Expression)getValue();
	} catch (IOException e) {
	    throw new CompilerError(e);
	}
    }

    /**
     * Get a field attribute
     */
    public byte[] getAttribute(Identifier name) {
	for (BinaryAttribute att = atts ; att != null ; att = att.next) {
	    if (att.name.equals(name)) {
		return att.data;
	    }
	}
	return null;
    }

    /*
     * Add an attribute to a field
     */
    public void addAttribute(Identifier name, byte data[], Environment env) {
	this.atts = new BinaryAttribute(name, data, this.atts);
	// Make sure that the new attribute is in the constant pool
	((BinaryClass)(this.clazz)).cpool.indexString(name.toString(), env);
    }

}
