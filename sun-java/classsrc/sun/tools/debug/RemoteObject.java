/*
 * @(#)RemoteObject.java	1.10 96/01/31
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
import java.lang.Exception;

/**
 * The RemoteObject class allows access to an object in a remote
 * Java interpreter.  
 *
 * Remote objects are not created by the local debugger, but are returned
 * by the remote debugging agent when queried for the values of instance
 * or static variables of known objects (such as classes), or from local
 * (stack) variables.
 *
 * Each remote object has a reference cached by the remote Java interpreter,
 * so that the object will not be garbage-collected during examination.  The
 * RemoteDebugger's gc() operation frees references to objects that are no
 * longer being examined.
 *
 * @see RemoteDebugger
 * @see RemoteClass
 * @see RemoteString
 * @see RemoteThread
 * @see RemoteThreadGroup
 * @author Thomas Ball
 */
public class RemoteObject extends RemoteValue {
    RemoteAgent agent;
    RemoteClass clazz;
    int id;

    RemoteObject(RemoteAgent agent, int id, RemoteClass clazz) {
	this(agent, TP_OBJECT, id, clazz);
    }

    RemoteObject(RemoteAgent agent, int type, int id, RemoteClass clazz) {
	super(type);
	this.agent = agent;
	this.id = id;
	this.clazz = clazz;
    }

    /** Returns the RemoteValue's type name ("Object"). */
    public String typeName() throws Exception {
	return "Object";
    }

    /** Returns the id of the object. */
    public final int getId() {
	return id;
    }

    /** Returns the object's class. */
    public final RemoteClass getClazz() {
	return clazz;
    }

    /**
     * Returns the value of an object's instance variable.
     *
     * @param n	the slot number of the variable to be returned.
     */
    public RemoteValue getFieldValue(int n) throws Exception {
	return clazz.getInstanceField(n).getValue(id);
    }

    /**
     * Returns the value of an object's instance variable.
     *
     * @param name	the name of the instance variable
     * @return the variable as a RemoteValue, or null if name not found.
     */
    public RemoteValue getFieldValue(String name) throws Exception {
	RemoteField fields[] = clazz.getInstanceFields();
	for (int i = 0; i < fields.length; i++) {
	    if (name.equals(fields[i].getName())) {
		return fields[i].getValue(id);
	    }
	}
	return null;
    }

    /** Return the instance (non-static) fields of an object. */
    public RemoteField getFields()[] throws Exception {
	return clazz.getInstanceFields();
    }

    /**
     * Return an instance variable, specified by slot number.
     *
     * @param n	the slot number of the variable to be returned.
     */
    public RemoteField getField(int n) throws Exception {
	return clazz.getInstanceField(n);
    }

    /**
     * Return an instance variable, specified by name.
     *
     * @param name	the name of the instance variable
     * @return the variable as a RemoteField, or null if name not found.
     */
    public RemoteField getField(String name) throws Exception {
	RemoteField fields[] = clazz.getInstanceFields();
	for (int i = 0; i < fields.length; i++) {
	    if (name.equals(fields[i].getName())) {
		return fields[i];
	    }
	}
	return null;
    }

    /**
     * Set a boolean instance variable, specified by name.  If the instance
     * variable doesn't exist, an IllegalAccessException is thrown.
     *
     * @param name	the name of the instance variable
     * @param value	the value to use
     */
    public void setField(String name, boolean value) throws Exception {
	RemoteField rf = getField(name);
        if (rf == null) {
            throw new IllegalAccessException(
                name + " is not an instance variable of " + this);
        }
        rf.setValue(id, value);
    }

    /**
     * Set an int instance variable, specified by name.  If the instance
     * variable doesn't exist, an IllegalAccessException is thrown.
     *
     * @param name	the name of the instance variable
     * @param value	the value to use
     */
    public void setField(String name, int value) throws Exception {
	RemoteField rf = getField(name);
        if (rf == null) {
            throw new IllegalAccessException(
                name + " is not an instance variable of " + this);
        }
        rf.setValue(id, value);
    }

    /**
     * Set a char instance variable, specified by name.  If the instance
     * variable doesn't exist, an IllegalAccessException is thrown.
     *
     * @param name	the name of the instance variable
     * @param value	the value to use
     */
    public void setField(String name, char value) throws Exception {
	RemoteField rf = getField(name);
        if (rf == null) {
            throw new IllegalAccessException(
                name + " is not an instance variable of " + this);
        }
        rf.setValue(id, value);
    }

    /**
     * Set a long instance variable, specified by name.  If the instance
     * variable doesn't exist, an IllegalAccessException is thrown.
     *
     * @param name	the name of the instance variable
     * @param value	the value to use
     */
    public void setField(String name, long value) throws Exception {
	RemoteField rf = getField(name);
        if (rf == null) {
            throw new IllegalAccessException(
                name + " is not an instance variable of " + this);
        }
        rf.setValue(id, value);
    }

    /**
     * Set a float instance variable, specified by name.  If the instance
     * variable doesn't exist, an IllegalAccessException is thrown.
     *
     * @param name	the name of the instance variable
     * @param value	the value to use
     */
    public void setField(String name, float value) throws Exception {
	RemoteField rf = getField(name);
        if (rf == null) {
            throw new IllegalAccessException(
                name + " is not an instance variable of " + this);
        }
        rf.setValue(id, value);
    }

    /**
     * Set a double instance variable, specified by name.  If the instance
     * variable doesn't exist, an IllegalAccessException is thrown.
     *
     * @param name	the name of the instance variable
     * @param value	the value to use
     */
    public void setField(String name, double value) throws Exception {
	RemoteField rf = getField(name);
        if (rf == null) {
            throw new IllegalAccessException(
                name + " is not an instance variable of " + this);
        }
        rf.setValue(id, value);
    }

    /** Return a description of the object. */
    public String description() {
	try {
	    if (clazz == null) {
	        return toHex(id);
	    } else {
	        return "(" + getClazz().getName() + ")" + toHex(id);
	    }
	} catch (Exception ex) {
	    return ("<communications error>");
	}
    }

    /** Return object as a string. */
    public String toString() {
	String retStr = agent.objectToString(id);
	if (retStr.length() > 0) {
	    return retStr;
	} else {
	    return description();
	}
    }
}
