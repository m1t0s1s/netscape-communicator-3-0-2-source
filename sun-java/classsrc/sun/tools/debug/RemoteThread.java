/*
 * @(#)RemoteThread.java	1.13 95/09/18
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
 * The RemoteThread class allows access to a thread in a remote Java
 * interpreter.  
 *
 * @see RemoteDebugger
 * @see RemoteThreadGroup
 * @author Thomas Ball
 */
public class RemoteThread extends RemoteObject {

    private int currentFrame = 0;
    private RemoteStackFrame stack[];
    private boolean suspended = false;

    RemoteThread(RemoteAgent agent, int id, RemoteClass clazz) {
	super(agent, TP_THREAD, id, clazz);
    }

    /** Return the name of the thread. */
    public String getName() throws Exception {
	return agent.getThreadName(id);
    }

    /** Return the current stackframe index */
    public int getCurrentFrameIndex() {
	return currentFrame;
    }

    /** Set the current stackframe index */
    public void setCurrentFrameIndex(int iFrame) {
	currentFrame = iFrame;
    }

    /** Reset the current stackframe */
    public void resetCurrentFrameIndex() {
	currentFrame = 0;
    }

    /**
     * Change the current stackframe to be one or more frames higher
     * (as in, away from the current program counter).
     *
     * @param nFrames	the number of stackframes
     * @exception IllegalAccessError when the thread isn't suspended or waiting at a breakpoint
     * @exception ArrayIndexOutOfBoundsException when the requested frame is beyond the stack boundary
     */
    public void up(int nFrames) throws Exception {
	int status = agent.getThreadStatus(id);
	if (status != THR_STATUS_SUSPENDED && status != THR_STATUS_BREAK) {
	    throw new IllegalAccessError();
	}
	if (stack == null) {
	    stack = agent.dumpStack(this);
	}
	if ((currentFrame + nFrames) >= stack.length) {
	    throw new ArrayIndexOutOfBoundsException();
	}
	currentFrame += nFrames;
    }

    /**
     * Change the current stackframe to be one or more frames lower
     * (as in, toward the current program counter).
     *
     * @param nFrames	the number of stackframes
     * @exception IllegalAccessError when the thread isn't suspended or waiting at a breakpoint
     * @exception ArrayIndexOutOfBoundsException when the requested frame is beyond the stack boundary
     */
    public void down(int nFrames) throws Exception {
	int status = agent.getThreadStatus(id);
	if (status != THR_STATUS_SUSPENDED && status != THR_STATUS_BREAK) {
	    throw new IllegalAccessError();
	}
	if (stack == null) {
	    stack = agent.dumpStack(this);
	}
	if ((currentFrame - nFrames) < 0) {
	    throw new ArrayIndexOutOfBoundsException();
	}
	currentFrame -= nFrames;
    }

    /** Return the thread status description */
    public String getStatus() throws Exception {
	int status = agent.getThreadStatus(id);
	switch (status) {
	  case THR_STATUS_UNKNOWN:
	    return "";
	  case THR_STATUS_ZOMBIE:
	    return "zombie";
	  case THR_STATUS_RUNNING:
	    return "running";
	  case THR_STATUS_SLEEPING:
	    return "sleeping";
	  case THR_STATUS_MONWAIT:
	    return "waiting in a monitor";
	  case THR_STATUS_CONDWAIT:
	    return "cond. waiting";
	  case THR_STATUS_SUSPENDED:
	    return "suspended";
	  case THR_STATUS_BREAK:
	    return "at breakpoint";
	  default:
	    return "invalid status returned";
	}
    }

    /** Dump the stack. */
    public RemoteStackFrame[] dumpStack() throws Exception {
	stack = agent.dumpStack(this);
	return stack;
    }

    /**
     * Get the current stack frame.
     *
     * @exception IllegalAccessError when the thread isn't suspended or waiting at a breakpoint
     */
    public RemoteStackFrame getCurrentFrame() throws Exception {
	int status = agent.getThreadStatus(id);
	if (status != THR_STATUS_SUSPENDED && status != THR_STATUS_BREAK) {
	    throw new IllegalAccessError();
	}
	if (stack == null) {
	    stack = agent.dumpStack(this);
	}
	return stack[currentFrame];
    }

    /** Suspend execution of this thread. */
    public void suspend() throws Exception {
	if (suspended == false) {
	    SecurityManager.setScopePermission();
	    agent.suspendThread(id);
	    suspended = true;
	}
    }

    /** Resume execution of this thread. */
    public void resume() throws Exception {
	if (suspended == true) {
	    agent.resumeThread(id);
	    suspended = false;
	}
    }

    /**
     * Continue execution of this thread to the next instruction or line.
     *
     * @param	skipLine	true to execute to next source line, false to
     * next instruction.
     * @exception IllegalAccessError when the thread isn't suspended or waiting at a breakpoint
     */
    public void step(boolean skipLine) throws Exception {
	int status = agent.getThreadStatus(id);
	if (status != THR_STATUS_SUSPENDED && status != THR_STATUS_BREAK) {
	    throw new IllegalAccessError();
	}
	agent.stepThread(id, skipLine);
    }

    /**
     * Continue execution of this thread to the next line, but don't step
     * into a method call.  If no line information is available, next()
     * is equivalent to step().
     *
     * @exception IllegalAccessError when the thread isn't suspended or waiting at a breakpoint
     */
    public void next() throws Exception {
	int status = agent.getThreadStatus(id);
	if (status != THR_STATUS_SUSPENDED && status != THR_STATUS_BREAK) {
	    throw new IllegalAccessError();
	}
	agent.stepNextThread(id);
    }

    /** Return whether this thread is suspended. */
    public boolean isSuspended() {
	return suspended;
    }

    /**
     * Resume this thread from a breakpoint, unless it previously suspended.
     */
    public void cont() throws Exception {
	if (suspended == false) {
	    SecurityManager.setScopePermission();
	    agent.resumeThread(id);
	}
    }

    /** Stop the remote thread. */
    public void stop() throws Exception {
	SecurityManager.setScopePermission();
	agent.stopThread(id);
    }

    /**
     * Return a stack variable from the current stackframe.
     *
     * @return the variable as a RemoteValue, or null if not found.
     */
    public RemoteStackVariable getStackVariable(String name) throws Exception {
	agent.message("getStackVariable: thread=" + getName() +
		      ", currentFrame=" + currentFrame);
	int status = agent.getThreadStatus(id);
	if (status != THR_STATUS_SUSPENDED && status != THR_STATUS_BREAK) {
	    agent.message("getStackVariable: bogus thread status=" + status);
	    return null;
	}
	if (stack == null) {
	    stack = agent.dumpStack(this);
	}
	return stack[currentFrame].getLocalVariable(name);
    }

    /**
     * Return the arguments and local variable from the current stackframe.
     *
     * @return an array of RemoteValues.
     */
    public RemoteStackVariable[] getStackVariables() throws Exception {
	agent.message("getStackVariables: thread=" + getName() +
		      ", currentFrame=" + currentFrame);
	int status = agent.getThreadStatus(id);
	if (status != THR_STATUS_SUSPENDED && status != THR_STATUS_BREAK) {
	    agent.message("getStackVariables: bogus thread status=" + status);
	    return null;
	}
	if (stack == null) {
	    stack = agent.dumpStack(this);
	}
	return stack[currentFrame].getLocalVariables();
    }
}
