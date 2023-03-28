/*
 * @(#)RemoteStackFrame.java	1.12 96/01/18
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
 * The RemoteStackFrame class provides access to a stackframe of a
 * suspended thread.  
 *
 * @see RemoteDebugger
 * @see RemoteThread
 * @author Thomas Ball
 */
public class RemoteStackFrame extends StackFrame {
    private RemoteObject idFrame;
    private RemoteAgent agent;
    private RemoteThread thread;
    private RemoteClass clazz;
    private int iFrame;

    RemoteStackFrame(RemoteObject id, RemoteThread t, int i,
			    RemoteAgent ag) {
	idFrame = id;
	thread = t;
	iFrame = i;
	agent = ag;
    }

    /** 
     * Return a specific (named) stack variable.  A slot number of -1
     * indicates that the variable is not currently in scope. 
     */
    public RemoteStackVariable getLocalVariable(String name) throws Exception {
	agent.message("getLocalVariable: name=" + name);
	agent.message("   stackframe=" + toString());
	agent.message("   " + localVariables.length + " local vars");
	for (int i = 0; i < localVariables.length; i++) {
	    agent.message("   trying " + localVariables[i].name);
	    if (localVariables[i].name.equals(name)) {
		RemoteValue v = agent.getStackValue(thread.getId(), iFrame,
		    localVariables[i].slot,
		    localVariables[i].signature.charAt(0));
		return new RemoteStackVariable(localVariables[i].slot, name,
					       localVariables[i].signature, 
                                               localVariables[i].methodArgument, v);
	    }
	}
	return null;
    }

    /**
     * Return an array of all valid local variables and method arguments
     * for this stack frame.
     */
    public RemoteStackVariable[] getLocalVariables() throws Exception {
	agent.message("getLocalVariables:");
	agent.message("   stackframe=" + toString());
	agent.message("   " + localVariables.length + " local vars");
	RemoteStackVariable ret[] =
	    new RemoteStackVariable[localVariables.length];
	for (int i = 0; i < ret.length; i++) {
	    RemoteValue v = agent.getStackValue(thread.getId(), iFrame,
		localVariables[i].slot,
		localVariables[i].signature.charAt(0));
	    ret[i] = new RemoteStackVariable(localVariables[i].slot,
					     localVariables[i].name,
					     localVariables[i].signature, 
                                             localVariables[i].methodArgument,
                                             v);
	}
	return ret;
    }

    /** Return the source file line number. */
    public int getLineNumber() {
	return lineno;
    }
    
    /** Get the method name referenced by this stackframe. */
    public String getMethodName() {
	return methodName;
    }

    /** Get the program counter referenced by this stackframe. */
    public int getPC() {
        return pc;
    }

    /** Get the class this stackframe references. */
    public RemoteClass getRemoteClass() {
	return clazz;
    }

    /* Set the class this stackframe references. */
    void setRemoteClass(RemoteClass c) {
	clazz = c;
    }
}
