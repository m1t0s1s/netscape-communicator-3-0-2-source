/*
 * @(#)RemoteThreadGroup.java	1.7 95/09/14
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

/**
 * The RemoteThreadGroup class allows access to a threadgroup in a
 * remote Java interpreter.  
 *
 * @see RemoteDebugger
 * @see RemoteThreadGroup
 * @author Thomas Ball
 */
public class RemoteThreadGroup extends RemoteObject {
    RemoteThreadGroup parent;
    String name;
    int maxPriority;
    boolean daemon;

    RemoteThreadGroup(RemoteAgent agent, int id, RemoteClass clazz) {
	super(agent, TP_THREADGROUP, id, clazz);
    }

    private void getThreadGroupInfo() throws Exception {
	if (name == null) {
	    agent.getThreadGroupInfo(this);
	}
    }

    /** Return the threadgroup's name. */
    public String getName() throws Exception {
	getThreadGroupInfo();
	return name;
    }

    /** Stop the remote threadgroup. */
    public void stop() throws Exception {
	agent.stopThreadGroup(id);
    }

    /** List a threadgroup's threads */
    public RemoteThread[] listThreads(boolean recurse) throws Exception {
	return agent.listThreads(this, recurse);
    }
}
