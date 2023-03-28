/*
 * @(#)RemoteField.java	1.11 96/01/31
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.tools.debug;
import sun.tools.java.Type;

/**
 * A RemoteField allows access to a variable or method of an object
 * or class in a remote Java interpreter.
 *
 * @see RemoteStackVariable
 */
public class RemoteField extends Field implements AgentConstants {

    RemoteAgent agent;

    RemoteField(RemoteAgent agent, int slot, String name,
		String signature, short access, RemoteClass clazz) {
	this.agent = agent;
	this.slot = slot;
	this.name = name;
	this.signature = signature;
	this.access = access;
	this.clazz = clazz;
    }

    /**
     * Returns the name of the field.
     */
    public String getName() {
	return name;
    }

    /*
     * Returns the value of the field.
     */
    RemoteValue getValue(int id) throws Exception {
	return agent.getSlotValue(id, slot);
    }

    /*
     * Sets the value of an integer field.  
     */
    void setValue(int id, int value) throws IllegalAccessException, Exception {
        int type = getType().getTypeCode();
        if (type == TC_LONG) {
            setValue(id, (long)value);
            return;
        }
        if (type != TC_INT && type != TC_BYTE && type != TC_SHORT ) {
            throw new IllegalAccessException();
        }
        agent.setSlotValue(id, slot, value);
    }

    /*
     * Sets the value of a boolean field.  
     */
    void setValue(int id, boolean value) throws IllegalAccessException, Exception {
        if (getType().getTypeCode() != TC_BOOLEAN) {
            throw new IllegalAccessException();
        }
        agent.setSlotValue(id, slot, value);
    }

    /*
     * Sets the value of a char field.  
     */
    void setValue(int id, char value) throws IllegalAccessException, Exception {
        if (getType().getTypeCode() != TC_CHAR) {
            throw new IllegalAccessException();
        }
        agent.setSlotValue(id, slot, value);
    }

    /*
     * Sets the value of a long field.  
     */
    void setValue(int id, long value) throws IllegalAccessException, Exception {
        if (getType().getTypeCode() != TC_LONG) {
            throw new IllegalAccessException();
        }
        agent.setSlotValue(id, slot, value);
    }

    /*
     * Sets the value of a float field.  
     */
    void setValue(int id, float value) throws IllegalAccessException, Exception {
        if (getType().getTypeCode() != TC_FLOAT) {
            throw new IllegalAccessException();
        }
        agent.setSlotValue(id, slot, value);
    }

    /*
     * Sets the value of a double field.  
     */
    void setValue(int id, double value) throws IllegalAccessException, Exception {
        if (getType().getTypeCode() != TC_DOUBLE) {
            throw new IllegalAccessException();
        }
        agent.setSlotValue(id, slot, value);
    }

    /**
     * Returns a Type object for the field.
     */
    public Type getType() {
	return Type.tType(signature);
    }

    /**
     * Returns a string describing the field with its full type.  
     * This string is similar to the one defining the field or
     * its source code, such as "int enumerate(Thread[])" or
     * "char name[]".
     */
    public String getTypedName() {
	return Type.tType(signature).typeString(name, true, true);
    }

    /**
     * Returns a string with the field's modifiers, such as "public",
     * "static", "final", etc.  If the field has no modifiers, an empty
     * String is returned.
     */
    public String getModifiers() {
	String s = new String();

	if ((access & M_PUBLIC) == M_PUBLIC) {
	    s = s.concat("public ");
	}
	if ((access & M_PRIVATE) == M_PRIVATE) {
	    s = s.concat("private ");
	}
	if ((access & M_PROTECTED) == M_PROTECTED) {
	    s = s.concat("protected ");
	}
	if ((access & M_STATIC) == M_STATIC) {
	    s = s.concat("static ");
	}
	if ((access & M_TRANSIENT) == M_TRANSIENT) {
	    s = s.concat("transient ");
	}
	if ((access & M_FINAL) == M_FINAL) {
	    s = s.concat("final ");
	}
	if ((access & M_VOLATILE) == M_VOLATILE) {
	    s = s.concat("volatile ");
	}
	return s;
    }

    /**
     * Returns whether the field is static (a class variable or method).
     */
    public boolean isStatic() {
	return ((access & ACC_STATIC) == ACC_STATIC);
    }

    public String toString() {
	return new String(signature + " " + name);
    }
}
