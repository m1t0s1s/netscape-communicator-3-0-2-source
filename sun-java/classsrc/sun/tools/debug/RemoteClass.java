/*
 * @(#)RemoteClass.java	1.16 96/01/23
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
import java.io.*;

/**
 * The RemoteClass class allows access to a class in a remote Java
 * interpreter.  
 *
 * @see RemoteDebugger
 * @author Thomas Ball
 */
public class RemoteClass extends RemoteObject {
    String name;
    String sourceName;
    boolean intf;
    RemoteClass superclass;
    RemoteObject loader;
    RemoteClass interfaces[] = null;
    private RemoteField methods[] = null;
    private RemoteField instanceFields[] = null;
    private RemoteField staticFields[] = null;

    RemoteClass(RemoteAgent agent, int id) {
	super(agent, TP_CLASS, id, agent.classClass);
    }

    private void getClassInfo() throws Exception {
	if (interfaces == null) {
	    agent.getClassInfo(this);
	}
    }

    private void loadFields() throws Exception {
	if (staticFields == null || instanceFields == null) {
	    RemoteField fields[] = agent.getFields(id);

	    int nStaticFields = 0;
	    for (int i = 0; i < fields.length; i++) {
		if (fields[i].isStatic() == true) {
		    nStaticFields++;
		}
	    }

	    staticFields = new RemoteField[nStaticFields];
	    instanceFields = new RemoteField[fields.length - nStaticFields];
	    int iInst = 0;
	    int iStatic = 0;
	    for (int i = 0; i < fields.length; i++) {
		if (fields[i].isStatic() == true) {
		    staticFields[iStatic++] = fields[i];
		} else {
		    instanceFields[iInst++] = fields[i];
		}
	    }
	}
    }
    
    /** Returns the name of the class. */
    public String getName() throws Exception {
	getClassInfo();
	return name;
    }

    /** Returns the name of the class as its type. */
    public String typeName() throws Exception {
	getClassInfo();
	return getName();
    }

    /** Is this RemoteClass an interface? */
    public boolean isInterface() throws Exception {
	getClassInfo();
	return intf;
    }

    /** Return the superclass for this class. */
    public RemoteClass getSuperclass() throws Exception {
	getClassInfo();
	return superclass;
    }

    /** Return the classloader for this class. */
    public RemoteObject getClassLoader() throws Exception {
	getClassInfo();
	return loader;
    }

    /** Return the interfaces for this class. */
    public RemoteClass getInterfaces()[] throws Exception {
	getClassInfo();
	return interfaces;
    }

    /** Get the name of the source file referenced by this stackframe. */
    public String getSourceFileName() throws Exception {
	getClassInfo();
	return sourceName;
    };

    /** Get the source file referenced by this stackframe. */
    public InputStream getSourceFile() throws Exception {
	getClassInfo();
	return agent.getSourceFile(name, sourceName);
    };

    /** Return all the static fields for this class. */
    public RemoteField[] getFields() throws Exception {
	return getStaticFields();
    }

    /** Return all the static fields for this class. */
    public RemoteField[] getStaticFields() throws Exception {
	loadFields();
	return staticFields;
    }

    /** Return all the instance fields for this class.
     *  Note:  because this is a RemoteClass method, only the name and
     *         type methods will be valid, not the data.
     */
    public RemoteField[] getInstanceFields() throws Exception {
	loadFields();
	return instanceFields;
    }

    /**
     * Return the static field, specified by index.
     *
     * @exception ArrayIndexOutOfBoundsException when the index is greater than the number of instance variables
     */
    public RemoteField getField(int n) throws Exception {
	loadFields();
	if (n < 0 || n >= staticFields.length) {
	    throw new ArrayIndexOutOfBoundsException();
	}
	return staticFields[n];
    }

    /** Return the static field, specified by name. */
    public RemoteField getField(String name) throws Exception {
	loadFields();
	for (int i = 0; i < staticFields.length; i++) {
	    if (name.equals(staticFields[i].getName())) {
		return staticFields[i];
	    }
	}
	return null;
    }

    /**
     * Return the instance field, specified by its index.
     * Note:  because this is a RemoteClass method, only the name and
     *        type information is valid, not the data.
     *
     * @exception ArrayIndexOutOfBoundsException when the index is greater than the number of instance variables
     */
    public RemoteField getInstanceField(int n) throws Exception {
	loadFields();
	if (n < 0 || n >= instanceFields.length) {
	    throw new ArrayIndexOutOfBoundsException();
	}
	return instanceFields[n];
    }

    /** Return the value of a static field, specified by its index */
    public RemoteValue getFieldValue(int n) throws Exception {
	return getField(n).getValue(id);
    }

    /** Return the value of a static field, specified by name. */
    public RemoteValue getFieldValue(String name) throws Exception {
	loadFields();
	for (int i = 0; i < staticFields.length; i++) {
	    if (name.equals(staticFields[i].getName())) {
		return staticFields[i].getValue(id);
	    }
	}
	return null;
    }

    /** Return the method, specified by name. */
    public RemoteField getMethod(String name) throws Exception {
	if (methods == null) {
	    methods = agent.getMethods(id);
	}
	for (int i = 0; i < methods.length; i++) {
	    if (name.equals(methods[i].getName())) {
		return methods[i];
	    }
	}
	return null;
    }

    /** Return the class's methods. */
    public RemoteField[] getMethods() throws Exception {
	if (methods == null) {
	    methods = agent.getMethods(id);
	}
	return methods;
    }

    /** Return the names of all methods supported by this class. */
    public String[] getMethodNames() throws Exception {
	if (methods == null) {
	    methods = agent.getMethods(id);
	}
	String ret[] = new String[methods.length];
	for (int i = 0; i < methods.length; i++) {
	    ret[i] = methods[i].getName();
	}
	return ret;
    }

    /**
     * Set a breakpoint at a specified source line number in a class.
     *
     * @param lineno	the line number where the breakpoint is set
     * @return an empty string if successful, otherwise a description of the error.
     */
    public String setBreakpointLine(int lineno) throws Exception {
	return agent.setBreakpointLine(this, lineno);
    }

    /**
     * Set a breakpoint at the first line of a class method.
     *
     * @param method	the method where the breakpoint is set
     * @return an empty string if successful, otherwise a description of the error.
     */
    public String setBreakpointMethod(RemoteField method) throws Exception {
	return agent.setBreakpointMethod(this, method);
    }

    /**
     * Clear a breakpoint at a specific address in a class.
     *
     * @param pc	the address of the breakpoint to be cleared
     * @return an empty string if successful, otherwise a description of the error.
     */
    public String clearBreakpoint(int pc) throws Exception {
	return agent.clearBreakpoint(this, pc);
    }

    /**
     * Clear a breakpoint at a specified line.
     *
     * @param lineno	the line number of the breakpoint to be cleared
     * @return an empty string if successful, otherwise a description of the error.
     */
    public String clearBreakpointLine(int lineno) throws Exception {
	return agent.clearBreakpointLine(this, lineno);
    }

    /**
     * Clear a breakpoint at the start of a specified method.
     *
     * @param method	the method of the breakpoint to be cleared
     * @return an empty string if successful, otherwise a description of the error.
     */
    public String clearBreakpointMethod(RemoteField method) throws Exception {
	return agent.clearBreakpointMethod(this, method);
    }

    private boolean isExceptionClass() throws Exception {
	if (getName().equals("java.lang.Exception") ||
	    getName().equals("java.lang.Error")) {
	    return true;
	}
	RemoteClass superClass = superclass;
	do {
	    agent.message("isExceptionClass: superClass=" +
			  superClass.getName());
	    if (superClass.getName().equals("java.lang.Exception") ||
	    	superClass.getName().equals("java.lang.Error")) {
		return true;
	    }
	    superClass = superClass.getSuperclass();
	} while (superClass.getName().equals("java.lang.Object") == false);
	return false;
    }

    /**
     * Enter the debugger when an instance of this class is thrown.
     *
     * @exception ClassCastException when this class isn't an exception class
     */
    public void catchExceptions() throws Exception {
	if (isExceptionClass()) {
	    agent.catchExceptionClass(this);
	} else {
	    throw new ClassCastException();
	}
    }

    /**
     * Don't enter the debugger when an instance of this class is thrown.
     *
     * @exception ClassCastException when this class isn't an exception class
     */
    public void ignoreExceptions() throws Exception {
	if (isExceptionClass()) {
	    agent.ignoreExceptionClass(this);
	} else {
	    throw new ClassCastException();
	}
    }

    /** Return a (somewhat verbose) description. */
    public String description() {
	return toString();
    }

    /** Return a (somewhat verbose) description. */
    public String toString() {
	try {
	    getClassInfo();
	    return toHex(id) + ":" + (intf ? "interface" : "class") +
	        "(" + getName() + ")";
	} catch (Exception ex) {
	    return ("<communications errors>");
	}
    }
}
