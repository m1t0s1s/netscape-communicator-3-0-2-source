/*
 * @(#)BreakpointHandler.java	1.17 96/02/16
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

import java.util.*;
import java.io.IOException;

class BreakpointHandler extends Thread implements AgentConstants {
    static BreakpointQueue	the_bkptQ;
    static Hashtable		the_bkptHash;

    private Agent		agent;
    private Hashtable		catchHash;
    private Vector		skipLines;

    static final boolean	debug = false;

    native int setBreakpoint(int pc) throws sun.tools.debug.InvalidPCException;
    native void clrBreakpoint(int pc, int oldOpcode) 
        throws sun.tools.debug.InvalidPCException;

    synchronized public static int getOpcode(int pc) {
        BreakpointSet bkpt = (BreakpointSet)the_bkptHash.get(new Integer(pc));
        return (bkpt == null) ? -1 : bkpt.opcode;
    }

    synchronized void addBreakpoint(Class classObj, int pc,
				    int type, byte cond[]) throws IOException {
	BreakpointSet	bkpt;
	int		opcode;

	bkpt = (BreakpointSet)the_bkptHash.get(new Integer(pc));
	if (bkpt == null) {
            if (debug) {
                Agent.message("Adding breakpoint for pc " + pc);
            }
	    if (classObj == null) {
                if (debug) {
                    Agent.message("Invalid classObj in addBreakpoint");
                }
		return;
	    }
	    try {
	        opcode = setBreakpoint(pc);
	    } catch (InvalidPCException ex) {
		throw new IOException("invalid PC");
	    }
	    bkpt = new BreakpointSet(pc, opcode, classObj, type, cond);
	    the_bkptHash.put(new Integer(pc), bkpt);
            if (cond != null && cond.length > 0) {
                bkpt.addCondition(cond);
            }
	}
    }

    synchronized void deleteBreakpoint(Class classObj, int pc) throws IOException {
	BreakpointSet	bkpt;

	bkpt = (BreakpointSet)the_bkptHash.get(new Integer(pc));
	if (bkpt == null) {
	    return;
	}
	try {
	    clrBreakpoint(pc, bkpt.opcode);
	} catch (InvalidPCException ex) {
	    throw new IOException("invalid PC");
	}
	the_bkptHash.remove(new Integer(pc));
    }

    synchronized BreakpointSet listBreakpoints()[] {
	BreakpointSet ret[] = new BreakpointSet[the_bkptHash.size()];
	Enumeration e = the_bkptHash.elements();
	for (int i = 0; i < ret.length; i++) {
	    try {
		ret[i] = (BreakpointSet)e.nextElement();
	    } catch (NoSuchElementException nse) {
		// Should never happen!
		Agent.error("Internal error" + agent.exceptionStackTrace(nse));
		return null;
	    }
	}
	return ret;
    }

    synchronized void catchExceptionClass(Class c) {
	if (catchHash.get(c) == null) {
	    catchHash.put(c, c);
	}
    }

    synchronized void ignoreExceptionClass(Class c) {
	catchHash.remove(c);
    }

    Class getCatchList()[] {
	Class list[] = new Class[catchHash.size()];
	Enumeration enum = catchHash.elements();
	for (int i = 0; i < list.length; i++) {
	    list[i] = (Class)enum.nextElement();
	}
	return list;
    }

    void addSkipLine(LineNumber line) {
        if (debug) {
            Agent.message("addSkipLine: thread=" + line.thread.getName() +
                          ", startPC=" + line.startPC + 
                          ", endPC = " + line.endPC);
        }
	skipLines.addElement(line);
    }

    void removeSkipLine(LineNumber line) {
	skipLines.removeElement(line);
    }

    boolean ignoreSingleStep(Thread thread, int pc) {
        if (debug) {
            Agent.message("ignoreSingleStep(" + thread.getName() + 
                          ", " + pc + ")");
        }
	Enumeration e = skipLines.elements();
	while (e.hasMoreElements()) {
	    LineNumber ln = (LineNumber)e.nextElement();
            if (debug) {
                Agent.message("   ln: thread=" + ln.thread.getName() +
                              ", startPC=" + ln.startPC + 
                              ", endPC = " + ln.endPC);
            }
	    if (ln.thread == thread) {
		if (pc >= ln.startPC && pc <= ln.endPC) {
		    return true;
		} else {
		    // Only one skip line per thread.
		    removeSkipLine(ln);
		}
	    }
	}
        if (debug) {
            Agent.message("   no match.");
        }
	return false;
    }

    BreakpointHandler(Agent ag) {
	super("Breakpoint handler");
	the_bkptQ = new BreakpointQueue();
	the_bkptHash = new Hashtable();
	catchHash = new Hashtable();
	skipLines = new Vector();
	agent = ag;
    }

    public void run() {
	try {
	    BreakpointSet bkpt;

	    while (the_bkptQ.nextEvent()) {

		if (agent.asyncOutputStream == null) {
		    continue;
		}

		if (the_bkptQ.exception != null) {
		    if (the_bkptQ.exception instanceof ThreadDeath) {
                        if (debug) {
                            Agent.message("Received ThreadDeath event" +
                                          ", pc=" + the_bkptQ.pc);
                        }
                        synchronized (agent.asyncOutputStream) {
                            agent.asyncOutputStream.write(CMD_THREADDEATH_NOTIFY);
                            agent.writeObject(the_bkptQ.thread,
                                              agent.asyncOutputStream);
                            agent.asyncOutputStream.flush();
                        }
		    } else {
                        if (debug) {
                            Agent.message("Received exception " +
                                          the_bkptQ.exception.toString()
                                          + ", pc=" + the_bkptQ.pc);
                        }
			Class excClass = the_bkptQ.exception.getClass();
			if (catchHash.containsKey(excClass) ||
			    the_bkptQ.catch_pc == 0) {

			    agent.suspendAllThreads();
			    String errorText = (the_bkptQ.catch_pc == 0) ?
				"Uncaught exception: " : "Exception: ";
			    String excText = agent.exceptionStackTrace(
				the_bkptQ.exception);
			    errorText = errorText.concat(excText);

                            synchronized (agent.asyncOutputStream) {
                                agent.asyncOutputStream.write(CMD_EXCEPTION_NOTIFY);
                                agent.writeObject(the_bkptQ.thread,
                                                  agent.asyncOutputStream);
                                agent.asyncOutputStream.writeUTF(errorText);
                                agent.asyncOutputStream.flush();
                            }
			}
		    }
		} else if (the_bkptQ.opcode != -1) {
		    addBreakpoint(/*the_bkptQ.clazz*/ null,
				  the_bkptQ.pc, BKPT_NORMAL, null);
		} else {
		    bkpt = (BreakpointSet)the_bkptHash.get(
			new Integer(the_bkptQ.pc));
		    boolean ignoreBreakpoint = false;
		    if (bkpt == null) {
                        if (debug) {
                            Agent.message("Single step event: pc=" +
                                          the_bkptQ.pc);
                        }
			if (ignoreSingleStep(the_bkptQ.thread,
					     the_bkptQ.pc)) {
			    continue;
			}
			agent.setSingleStep(the_bkptQ.thread, false);
			the_bkptQ.opcode = -1;
		    } else {
                        if (debug) {
                            Agent.message("Received a breakpoint event: pc=" +
                                          the_bkptQ.pc);
                        }
			the_bkptQ.opcode = bkpt.opcode;
			the_bkptQ.updated = true;
		    }
		    agent.suspendAllThreads();

		    if (bkpt != null && bkpt.type == BKPT_ONESHOT) {
			deleteBreakpoint(bkpt.clazz, bkpt.pc);
		    }
                    synchronized (agent.asyncOutputStream) {
                        agent.asyncOutputStream.write(CMD_BRKPT_NOTIFY);
                        agent.writeObject(the_bkptQ.thread,
                                          agent.asyncOutputStream);
                        agent.asyncOutputStream.flush();
		    }
		}
	    }
	} catch (ThreadDeath td) {
            if (debug) {
                Agent.message("BreakpointHandler: ThreadDeath received.");
            }
	    /* Clear all breakpoints before exiting. */
	    Enumeration enum = the_bkptHash.elements();
	    while (enum.hasMoreElements()) {
		BreakpointSet bkpt = (BreakpointSet)enum.nextElement();
	        try {
		    clrBreakpoint(bkpt.pc, bkpt.opcode);
		} catch (Exception x) {
		    // Ignore exceptions here
		}
		the_bkptHash.remove(new Integer(bkpt.pc));
	    }
	    throw td;
	} catch (Exception ex) {
	    Agent.error(agent.exceptionStackTrace(ex));
	    Enumeration enum = the_bkptHash.elements();
	    while (enum.hasMoreElements()) {
		BreakpointSet bkpt = (BreakpointSet)enum.nextElement();
	        try {
		    clrBreakpoint(bkpt.pc, bkpt.opcode);
		} catch (Exception x) {
		    // Ignore exceptions here
		}
		the_bkptHash.remove(new Integer(bkpt.pc));
	    }
	    return;
	}
    }
}
