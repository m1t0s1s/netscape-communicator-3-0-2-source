/*
 * @(#)Agent.java	1.62 96/02/16
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

import java.io.*;
import java.net.*;
import java.util.*;
import java.lang.SecurityManager;
import sun.tools.java.Package;
import sun.tools.java.ClassPath;
import sun.tools.java.ClassFile;
import sun.tools.java.Identifier;

class MainThread extends Thread {
    Agent agent;
    Class clazz;
    String argv[];
    
    public MainThread(Agent agent, ThreadGroup group,
		      Class clazz, String argv[]) throws IOException {
	super(group, "main");
	setPriority(NORM_PRIORITY);
	setDaemon(false);

	this.agent = agent;
	this.clazz = clazz;
	this.argv = argv;

	start();
    }

    public void run() {
	try {
	    agent.runMain(clazz, argv);
	} catch (IllegalAccessError e) {
	    agent.error("\"public static void main(String argv[])\" " +
			"method not found in " + clazz.getName());
	}
    }
}

class Agent implements Runnable, AgentConstants {
//class Agent extends Thread implements AgentConstants {

    /* The agent socket */
    ServerSocket socket;

    /* The output stream back to the remote debugger */
    DataOutputStream asyncOutputStream;

    /* The hashtable of objects */
    Hashtable objects;

    /* The breakpoint handler process */
    private BreakpointHandler bkptHandler;

    /* The EmptyApp thread, if any. */
    static Thread emptyAppThread = null;

    private DataOutputStream out = null;
    private ResponseStream outBuffer = null;
    private ClassPath sourcePath = null;
    static boolean verbose = false;

    static String toHex(int n) {
	char str[] = new char[10];
	for (int i = 0 ; i < 10 ; i++) {
	    str[i] = '0';
	}
	str[1] = 'x';

	for (int i = 9 ; n != 0 ; n >>>= 4, i--) {
	    int d = n & 0xF;
	    str[i] = (char)((d < 10) ? ('0' + d) : ('a' + d - 10));
	}
	return new String(str);
    }

    /* Send all currently known classes to the RemoteAgent. */
    private void dumpClasses() throws IOException {
	message("dumpClasses()");

	/* Create a vector of classes. */
	Enumeration enum = objects.elements();
	Vector v = new Vector();
	int nClasses = 0;
	while (enum.hasMoreElements()) {
	    try {
		Object o = enum.nextElement();
		if (o != null && o instanceof Class) {
		    v.addElement(o);
		    nClasses++;
		}
	    } catch (Exception e) {
		error("dumpClasses() failed: " + e.toString());
	    }
	}

	/* Pass the vector's contents to the RemoteAgent. */
	out.writeInt(nClasses);
	enum = v.elements();
	while (enum.hasMoreElements()) {
	    writeObject(enum.nextElement());
	}
    }

    private String cookieToString(int cookie) {
        int radix = PSWDCHARS.length();
	int digitCount = 0;
	boolean minval = false;

	StringBuffer buf = new StringBuffer(32);
	while (cookie > 0) {
	    if (cookie < radix) {
		buf.append(PSWDCHARS.charAt(cookie));
		cookie = 0;
	    } else {
		int j = cookie % radix;
		cookie /= radix;
		buf.append(PSWDCHARS.charAt(j));
	    }
	    digitCount++;
	}
	if (digitCount > 0) {
	    int j = buf.length();
	    char tmp[] = new char[j];
	    int i = 0;
	    while (j-- > 0) {
		tmp[j] = buf.charAt(i++);
	    }
	    return String.valueOf(tmp);
	} else
	    return "0";
    }

    private String makePassword(int port) {
	if (port > 0xffff) {
	    System.err.println("Invalid port number (" + port + ")???");
	    System.exit(1);
	}
	int mask = (int)(Math.round(Math.random() * 0x800) % 0x800);
        int cookie = 0;
        int slice = 3;
        for (int i = 0; i < 8; i++) {
            int portbits = ((port & slice) << (i + 1));
            int maskbits = ((mask & (1 << i)) << (i * 2));
            cookie |= portbits | maskbits;
            slice <<= 2;
        }
        cookie |= (mask & 0x700) << 16;
        return cookieToString(cookie);
    }

    /* Create an Agent. */
    Agent(int port) {
	for (int i = 0; ; i++) { 
	    try {
		socket = new ServerSocket(port, 10);
		System.out.println("Agent password=" +
				   makePassword(socket.getLocalPort()));
		return;
	    } catch(Exception e) {
		message("[Waiting to create port]\n");
		try {
		    Thread.sleep(5000);
		} catch (InterruptedException ex) {
		    // re-interrupt and drop through.
		    Thread.currentThread().interrupt();
	 	}
	    }
	    if (i >= 10) {
		error("**Failed to create port\n");
		return;
	    }
	}
    }

    /* Report the empty app thread, so it can be resumed after a real app is
     * run. */
    static void setEmptyAppThread(Thread t) {
	emptyAppThread = t;
    }

    /* Main agent event loop */
    public void run() {
	PrintStream stdout = null;
	Socket cmdSocket = null;
	Socket asyncSocket = null;
	
	SecurityManager.setScopePermission();
	String sourcePathString = System.getProperty("java.class.path");
	SecurityManager.resetScopePermission();
	if (sourcePathString == null) {
	    sourcePathString = ".";
	}
	sourcePath = new ClassPath(sourcePathString);

	try {
	    SecurityManager.setScopePermission();
	    // Really should be MAX_PRIORITY + 1
	    Thread.currentThread().setPriority(Thread.MAX_PRIORITY);

	    objects = new Hashtable();
	    bkptHandler = new BreakpointHandler(this);
	    bkptHandler.setPriority(Thread.MAX_PRIORITY - 1);
	    SecurityManager.resetScopePermission();
	    bkptHandler.start();
            initAgent();

	    while (true) {
		cmdSocket = socket.accept();
		message("cmd socket: " + cmdSocket.toString());
		DataInputStream in = new DataInputStream(
		    new BufferedInputStream(cmdSocket.getInputStream()));
		outBuffer = new ResponseStream(
		    cmdSocket.getOutputStream(), 8192);
		out = new DataOutputStream(outBuffer);

		/* Redirect agent stdout to client via an AgentOutputStream. */
		asyncSocket = socket.accept();
		stdout = System.out;
		asyncOutputStream = new DataOutputStream(
		    new BufferedOutputStream(asyncSocket.getOutputStream()));
                setSystemIO(System.in, 
		            new PrintStream(
		             new BufferedOutputStream(
			      new AgentOutputStream(asyncOutputStream), 128), true),
                           System.err);
		message("connection accepted");

		out.writeInt(AGENT_VERSION);

		try {
		    writeObject(Class.forName("java.lang.Object"));
		    writeObject(Class.forName("java.lang.Class"));
		    writeObject(Class.forName("java.lang.String"));
		} catch (ClassNotFoundException e) {
		    // If *these* classes can't be found, just bail.
		    System.exit(1);
		}
		dumpClasses();
		writeObject(Thread.currentThread().getThreadGroup());
		out.flush();

		try {
		    for (int cmd = in.read() ; cmd != -1 ; cmd = in.read()) {
			try {
			    handle(cmd, in, out);
			} catch (Throwable t) {
			    error(exceptionStackTrace(t));
			    outBuffer.reset();  // discard normal output.
			    out.writeInt(CMD_EXCEPTION);
			    out.writeUTF(t.toString());
			}
			out.flush();
		    }
		} catch (Exception e) {
		    setSystemIO(System.in, stdout, System.err);
		    error(exceptionStackTrace(e));
		}
		setSystemIO(System.in, stdout, System.err);
		message("connection closed");
	    }
	} catch (ThreadDeath td) {
	    setSystemIO(System.in, stdout, System.err);
	    message("ThreadDeath caught.");
	} catch (IOException ex) {
	    setSystemIO(System.in, stdout, System.err);
	    message("IOException caught.");
	}
        if (asyncSocket != null) {
	    try {
		asyncSocket.close();
	    } catch (Exception ex) {
		// Ignore exceptions here
	    }
	}
	if (cmdSocket != null) {
	    try {
		cmdSocket.close();
	    } catch (Exception ex) {
		// Ignore exceptions here
	    }
	}
    }

    /* hack: Unless we have "mutable" in/out/err in System.java,
	we need to force the setting into "final static" members.
	For now, we use a native method.  Me *might* some day
	switch to fancy mutable streams that can be properly
	retargeted with Java accessCheck*() calls as protection.
	This native method forwards to a native in system.c, 
        bypassing all our inter-class-security. */
    private native static void setSystemIO(InputStream in, 
					   PrintStream out, 
					   PrintStream err);


    /*
     * Return true if this is a special system thread that shouldn\'t
     * be suspended.
     */
    public static boolean systemThread(Thread t) {
	String	tname = String.valueOf(t.getName());

	if (tname.equals("Idle thread") ||
	    tname.equals("clock handler") ||
	    tname.equals("Debugger agent") ||
	    tname.equals("Agent output") ||
	    tname.equals("Breakpoint handler")) {
	    return true;
	}
	return false;
    }

    private void suspendThread(Thread t) {
	if (!systemThread(t)) {
	    message("suspending " + t.getName());
	    try {
		SecurityManager.setScopePermission();
		t.suspend();
	    } catch (IllegalThreadStateException e) {
		error("suspend failed: " + e.getMessage());
	    }
	}
    }

    private void stepThread(Thread t, boolean stepLine) {
	if (!systemThread(t)) {
	    message("stepping " + t.getName());
	    try {
		if (stepLine) {
		    StackFrame sf = getStackFrame(t, 0);
		    LineNumber ln = lineno2pc(sf.clazz, sf.lineno);
		    if (ln != null) { /* If line info available. */
			ln.thread = t;
			bkptHandler.addSkipLine(ln);
		    }
		}
		setSingleStep(t, true);
                resumeAllThreads();
	    } catch (IllegalThreadStateException e) {
		error("step failed: " + e.getMessage());
	    }
	}
    }

    private void stepNextThread(Thread t) {
	if (!systemThread(t)) {
	    message("next stepping " + t.getName());
	    try {
                StackFrame sf = getStackFrame(t, 0);
                LineNumber ln = lineno2pc(sf.clazz, sf.lineno);
                if (ln == null) {
                    message("no line information available, single-stepping.");
                    stepThread(t, false);
                    return;
                }
                try {
                    bkptHandler.addBreakpoint(ln.clazz, ln.endPC + 1, 
                                              BKPT_ONESHOT, null);
                } catch (Exception e) {
                    message("next-step failed: " + e.toString() + 
                            ", single-stepping.");
                    stepThread(t, false);
                    return;
                }
                resumeAllThreads();
	    } catch (IllegalThreadStateException e) {
		error("next step failed: " + e.getMessage());
	    }
	}
    }

    /* Used by the BreakpointHandler. */
    void suspendAllThreads() {
	message("suspendAllThreads()");
	Thread list[] = new Thread[100];
	int n = Thread.enumerate(list);
	for (int i = 0 ; i < n ; i++) {
	    suspendThread(list[i]);
	}
    }

    void resumeAllThreads() {
	message("resumeAllThreads()");
	Thread list[] = new Thread[100];
	int n = Thread.enumerate(list);
	for (int i = 0 ; i < n ; i++) {
	    resumeThread(list[i]);
	}
    }

    private void resumeThread(Thread t) {
        if (!systemThread(t)) {
	    message("resuming " + t.getName());
	    try {
	        SecurityManager.setScopePermission();
	        t.resume();
	    } catch (IllegalThreadStateException e) {
	        error("resume failed: " + e.getMessage());
	    }
	}
    }

    native int getThreadStatus(Thread t);
    native StackFrame getStackFrame(Thread t, int frameNum);
    native Field getMethods(Class c)[];
    native Field getFields(Class c)[];
    native Class getClasses()[];

    native Object getSlotObject(Object o, int slot);
    native int getSlotSignature(Class c, int slot)[];
    native boolean getSlotBoolean(Object o, int slot);
    native int getSlotInt(Object o, int slot);
    native long getSlotLong(Object o, int slot);
    native float getSlotFloat(Object o, int slot);
    native Object getSlotArray(Object o, int slot);
    native int getSlotArraySize(Object o, int slot);

    native Object getStackObject(Thread t, int frameNum, int slot);
    native boolean getStackBoolean(Thread t, int frameNum, int slot);
    native int getStackInt(Thread t, int frameNum, int slot);
    native long getStackLong(Thread t, int frameNum, int slot);
    native float getStackFloat(Thread t, int frameNum, int slot);

    native LineNumber lineno2pc(Class c, int lineno);
    native int pc2lineno(Class c, int pc);
    native int method2pc(Class c, int method_slot);
    native String exceptionStackTrace(Throwable exc);
    native void setSingleStep(Thread t, boolean step);
    native void initAgent();
    native String getClassSourceName(Class c);

    native void setSlotBoolean(Object o, int slot, boolean v);
    native void setSlotInt(Object o, int slot, int v);
    native void setSlotLong(Object o, int slot, long v);
    native void setSlotDouble(Object o, int slot, double v);
    native void setStackBoolean(Thread t, int frameNum, int slot, boolean v);
    native void setStackInt(Thread t, int frameNum, int slot, int v);
    native void setStackLong(Thread t, int frameNum, int slot, long v);
    native void setStackDouble(Thread t, int frameNum, int slot, double v);

    /* handle command */
    synchronized void handle(int cmd, DataInputStream in, DataOutputStream out) throws IOException {
	out.writeInt(cmd);

	switch (cmd) {

	  case CMD_LIST_CLASSES: {
	      Class list[] = getClasses();
	      out.writeInt(list.length);
	      for (int i = 0 ; i < list.length ; i++) {
		  writeObject(list[i]);
	      }
	      return;
	  }
	    
	  case CMD_GET_CLASS_INFO: {
	      Class c = (Class)objects.get(new Integer(in.readInt()));
	      out.writeUTF(c.getName());
              out.writeUTF(getClassSourceName(c));
	      out.writeInt(c.isInterface() ? 1 : 0);
	      writeObject(c.getSuperclass());
	      writeObject(c.getClassLoader());
	      Class interfaces[] = c.getInterfaces();
	      out.writeInt(interfaces.length);
	      for (int i = 0 ; i < interfaces.length ; i++) {
		  writeObject(interfaces[i]);
	      }
	      return;
	  }

 	  case CMD_GET_THREAD_NAME:
 	    out.writeUTF(((Thread)objects.get(new Integer(in.readInt()))).getName());
 	    return;

	  case CMD_GET_CLASS_BY_NAME: {
	      String name = in.readUTF();
	      try {
		  writeObject(Class.forName(name));
	      } catch (ClassNotFoundException e) {
		  message("no such class: " + name);
		  writeObject(null);
	      } catch (NoClassDefFoundError e) {
		  message("no such class: " + name);
		  writeObject(null);
	      }
	      return;
	  }

	  case CMD_GET_THREAD_STATUS:
	    out.writeInt(getThreadStatus(
		    (Thread)objects.get(new Integer(in.readInt()))));
	    return;

	  case CMD_GET_STACK_TRACE: {
	      Thread t = (Thread)objects.get(new Integer(in.readInt()));
	      int nFrames = t.countStackFrames();
	      out.writeInt(nFrames);
	      for (int i = 0; i < nFrames; i++) {
		  StackFrame sf = getStackFrame(t, i);
		  writeObject(sf);
		  out.writeUTF(sf.className);
		  out.writeUTF(sf.methodName);
		  out.writeInt(sf.lineno);
		  out.writeInt(sf.pc);
		  writeObject(sf.clazz);
		  out.writeInt(sf.localVariables.length);
		  for (int j = 0; j < sf.localVariables.length; j++) {
		      message("lvar " + j + ": slot=" +
			      sf.localVariables[j].slot +
			      ", name=" + sf.localVariables[j].name +
			      ", sig=" + sf.localVariables[j].signature + 
                              ", argument=" + sf.localVariables[j].methodArgument);
		      out.writeInt(sf.localVariables[j].slot);
		      out.writeUTF(sf.localVariables[j].name);
		      out.writeUTF(sf.localVariables[j].signature);
		      out.writeBoolean(sf.localVariables[j].methodArgument);
		  }
	      }
	      return;
	  }

	  case CMD_GET_STACK_VALUE: {
	      Thread t = (Thread)objects.get(new Integer(in.readInt()));
	      int frameNum = in.readInt();
	      int slot = in.readInt();
	      char type = in.readChar();
	      switch (type) {
		case 'Z':
		  write(getStackBoolean(t, frameNum, slot));
		  return;
		case 'B':
		  write((byte)getStackInt(t, frameNum, slot));
		  return;
		case 'C':
		  write((char)getStackInt(t, frameNum, slot));
		  return;
		case 'S':
		  write((short)getStackInt(t, frameNum, slot));
		  return;
		case 'I':
		  write(getStackInt(t, frameNum, slot));
		  return;
		case 'J':
		  write(getStackLong(t, frameNum, slot));
		  return;
		case 'F':
		  write(getStackFloat(t, frameNum, slot));
		  return;
		case 'D':
		  write((double)getStackFloat(t, frameNum, slot));
		  return;
		case '[':
		case 'L': 
		  writeObject(getStackObject(t, frameNum, slot));
		  return;
		case 'V':
		  writeObject(null);
		  return;
		default:
		  String strError = new String("bogus signature(" +
					       type + ")");
		  error(strError);
		  writeObject(strError);
	      }
	      return;
	  }
	  
	  case CMD_SET_STACK_VALUE: {
	      Thread t = (Thread)objects.get(new Integer(in.readInt()));
	      int frameNum = in.readInt();
	      int slot = in.readInt();
	      int type = in.readInt();
	      switch (type) {
		case TC_BOOLEAN:
                  setStackBoolean(t, frameNum, slot, in.readBoolean());
                  break;
		case TC_INT:
                  setStackInt(t, frameNum, slot, in.readInt());
                  break;
		case TC_LONG:
                  setStackLong(t, frameNum, slot, in.readLong());
                  break;
		case TC_DOUBLE:
                  setStackDouble(t, frameNum, slot, in.readDouble());
                  break;
		default:
                  error("bogus type(" + type + ")");
                  break;
	      }
	      return;
	  }
	  
          case CMD_MARK_OBJECTS: {
              int nMarked = in.readInt();
              Hashtable marked_objects = new Hashtable();
              for (int i = 0; i < nMarked; i++) {
                  Object obj = objects.get(new Integer(in.readInt()));
                  if (obj != null) {
                      marked_objects.put(new Integer(objectId(obj)), obj);
                  }
              }

              out.flush();
              cmd = in.read();
              out.writeInt(cmd);
              switch (cmd) {
                case CMD_FREE_OBJECTS :
                    try {
                        // Clone the existing objects hashtable.
                        Hashtable saved_objects = new Hashtable();
                        Enumeration enum = objects.elements();
                        while (enum.hasMoreElements()) {
                            Object obj = enum.nextElement();
                            saved_objects.put(new Integer(objectId(obj)), obj);
                        }

                        // Now remove unmarked objects, but not classes.
                        enum = objects.elements();
                        while (enum.hasMoreElements()) {
                            Object obj = enum.nextElement();
                            if (!(obj instanceof Class)) {
                                Integer id = new Integer(objectId(obj));
                                if (marked_objects.get(id) == null) {
                                    message("gc: freeing " + obj);
                                    saved_objects.remove(id);
                                }
                            }
                        }
                        objects = saved_objects;
                        System.gc();

                        // Return saved object ids.
                        enum = objects.elements();
                        while (enum.hasMoreElements()) {
                            Object obj = enum.nextElement();
                            if (obj != null) {
                                writeObject(obj);
                            }
                        }
                        writeObject(null);
                    } catch (Exception e) {
                        error("CMD_MARK_OBJECTS failed: " + e.toString());
                    }
                    break;
                default:
                    error("mark objects command failed");
                    break;
              }
              out.flush();
              return;
          }

	  case CMD_LIST_THREADGROUPS: {
	      SecurityManager.setScopePermission();
	      ThreadGroup list[] = new ThreadGroup[20];
	      SecurityManager.resetScopePermission();
	      ThreadGroup tg = (ThreadGroup)objects.get(
		  new Integer(in.readInt()));
	      int n = 0;
	      if (tg == null) {
		  /* List all threadgroups, including "system". */
		  message("Getting all threadgroups");
		  tg = Thread.currentThread().getThreadGroup();
		  n = tg.enumerate(list);
		  out.writeInt(n + 1);
		  writeObject(tg);
	      } else {
		  message("Getting threadgroups for " + tg.getName());
		  n = tg.enumerate(list);
		  out.writeInt(n);
	      }
	      for (int i = 0; i < n; i++) {
		  writeObject(list[i]);
	      }
	      return;
	  }

	  case CMD_GET_THREADGROUP_INFO: {
	      ThreadGroup tg = (ThreadGroup)objects.get(
		  new Integer(in.readInt()));
	      writeObject(tg.getParent());
	      out.writeUTF(tg.getName());
	      out.writeInt(tg.getMaxPriority());
	      out.writeBoolean(tg.isDaemon());
	      return;
	  }

	  case CMD_LIST_THREADS: {
	      Thread list[] = new Thread[40];
	      ThreadGroup tg = (ThreadGroup)objects.get(
		  new Integer(in.readInt()));
	      boolean recurse = in.readBoolean();
	      message("Getting threads for " + tg.getName());
	      int n = tg.enumerate(list, recurse);
	      out.writeInt(n);
	      for (int i = 0; i < n; i++) {
		  writeObject(list[i]);
	      }
	      return;
	  }

	  case CMD_RUN: {
	      String argv[] = new String[in.read() - 1];
	      String nm = in.readUTF();
	      String errorStr = new String("");
	      for (int i = 0 ; i < argv.length ; i++) {
		  argv[i] = in.readUTF();
	      }
	      try {
		  Class c = Class.forName(nm);
		  SecurityManager.setScopePermission();
		  ThreadGroup newGroup =
		      new ThreadGroup(c.getName() + ".main");
		  MainThread t = new MainThread(this, newGroup, c, argv);
		  SecurityManager.resetScopePermission();
		  writeObject(t.getThreadGroup());

		  /* kick the empty app, so it can wake up and exit */
		  if (emptyAppThread != null) {
		      SecurityManager.setScopePermission();
		      emptyAppThread.resume();
		      SecurityManager.resetScopePermission();
		      emptyAppThread = null;
		  }
	      } catch (Exception e) {
		  message(exceptionStackTrace(e));
		  writeObject(null);
	      }
	      return;
	  }

	  case CMD_SUSPEND_THREAD:
	      suspendThread((Thread)objects.get(new Integer(in.readInt())));
	      return;

	  case CMD_RESUME_THREAD:
	      resumeThread((Thread)objects.get(new Integer(in.readInt())));
	      return;

	  case CMD_GET_FIELDS: {
	      Class c = (Class)objects.get(new Integer(in.readInt()));
	      Field fields[] = getFields(c);
	      out.writeInt(fields.length);
	      for (int i = 0; i < fields.length ; i++) {
		  out.writeInt(fields[i].slot);
		  out.writeUTF(fields[i].name);
		  out.writeUTF(fields[i].signature);
		  out.writeShort(fields[i].access);
		  writeObject(fields[i].clazz);
	      }
	      return;
	  }

	  case CMD_GET_METHODS: {
	      Class c = (Class)objects.get(new Integer(in.readInt()));
	      Field methods[] = getMethods(c);
	      out.writeInt(methods.length);
	      for (int i = 0; i < methods.length ; i++) {
		  out.writeInt(methods[i].slot);
		  out.writeUTF(methods[i].name);
		  out.writeUTF(methods[i].signature);
		  out.writeShort(methods[i].access);
		  writeObject(methods[i].clazz);
	      }
	      return;
	  }

	  case CMD_GET_ELEMENTS: {
	      Object obj = (Object)objects.get(new Integer(in.readInt()));
	      int type = in.readInt();
	      int beginIndex = in.readInt();
	      int endIndex = in.readInt();
	      out.writeInt(endIndex - beginIndex + 1);
	      switch(type) {
		case TC_CHAR: {
		    char array[] = (char[])obj;
		    for (int i = beginIndex; i <= endIndex ; i++) {
			write(array[i]);
		    }
		    return;
		}
		case TC_BYTE: {
		    byte array[] = (byte[])obj;
		    for (int i = beginIndex; i <= endIndex ; i++) {
			write(array[i]);
		    }
		    return;
		}
	        case TC_CLASS:
		case TP_OBJECT: {
		    Object array[] = (Object[])obj;
		    for (int i = beginIndex; i <= endIndex ; i++) {
			writeObject(array[i]);
		    }
		    return;
		}
		case TC_FLOAT: {
		    float array[] = (float[])obj;
		    for (int i = beginIndex; i <= endIndex ; i++) {
			write(array[i]);
		    }
		    return;
		}
		case TC_DOUBLE: {
		    double array[] = (double[])obj;
		    for (int i = beginIndex; i <= endIndex ; i++) {
			write(array[i]);
		    }
		    return;
		}
		case TC_INT: {
		    int array[] = (int[])obj;
		    for (int i = beginIndex; i <= endIndex ; i++) {
			write(array[i]);
		    }
		    return;
		}
		case TC_LONG: {
		    long array[] = (long[])obj;
		    for (int i = beginIndex; i <= endIndex ; i++) {
			write(array[i]);
		    }
		    return;
		}
		case TC_SHORT: {
		    short array[] = (short[])obj;
		    for (int i = beginIndex; i <= endIndex ; i++) {
			write(array[i]);
		    }
		    return;
		}
		case TC_BOOLEAN: {
		    boolean array[] = (boolean[])obj;
		    for (int i = beginIndex; i <= endIndex ; i++) {
			write(array[i]);
		    }
		    return;
		}
		default:
		case TC_VOID: {
		    for (int i = beginIndex; i <= endIndex ; i++) {
			writeObject(null);
		    }
		    return;
		}
	      }
	  }

	  case CMD_GET_SLOT_SIGNATURE: {
	      Class c = (Class)objects.get(new Integer(in.readInt()));
	      int slot = in.readInt();
	      int sig[] = getSlotSignature(c, slot);
              if (sig == null) {
                  out.writeInt(0);
                  return;
              }
	      out.writeInt(sig.length);
	      for (int i = 0; i < sig.length; i++) {
		  out.writeInt(sig[i]);
	      }
	      return;
	  }

	  case CMD_GET_SLOT_VALUE: {
	      Object obj = (Object)objects.get(new Integer(in.readInt()));
	      int slot = in.readInt();
	      int sig[] = getSlotSignature(
		  (obj instanceof Class) ? (Class)obj : obj.getClass(), slot);
              if (sig == null) {
                  writeObject(new ArrayIndexOutOfBoundsException(
                      "invalid slot index " + slot));
                  return;
              }
	      switch (sig[0]) {
		case TC_BOOLEAN:
		  write(getSlotBoolean(obj, slot));
		  return;
		case TC_BYTE:
		  write((byte)getSlotInt(obj, slot));
		  return;
		case TC_CHAR:
		  write((char)getSlotInt(obj, slot));
		  return;
		case TC_SHORT:
		  write((short)getSlotInt(obj, slot));
		  return;
		case TC_INT:
		  write(getSlotInt(obj, slot));
		  return;
		case TC_LONG:
		  write(getSlotLong(obj, slot));
		  return;
		case TC_FLOAT:
		  write(getSlotFloat(obj, slot));
		  return;
		case TC_DOUBLE:
		  write((double)getSlotFloat(obj, slot));
		  return;
		case TC_ARRAY: {
		    Object array = getSlotArray(obj, slot);
		    int size = getSlotArraySize(obj, slot);
		    writeArray(array, size);
		    return;
		}
		case TP_OBJECT: 
		case TP_CLASS:
		  Object objectRet = getSlotObject(obj, slot);
		  writeObject(objectRet);
		  return;
		case TC_VOID:
		  writeObject(null);
		  return;
		default:
		  String strError = new String("bogus signature(");
		  for (int i = 0; i < sig.length; i++) {
		      strError = strError.concat(" " +
			  new Integer(sig[i]).toString());
		  }
		  strError = strError.concat(" )");
		  error(strError);
		  writeObject(strError);
	      }
	  }

	  case CMD_SET_SLOT_VALUE: {
	      Object obj = (Object)objects.get(new Integer(in.readInt()));
	      int slot = in.readInt();
	      int sig = in.readInt();
	      switch (sig) {
		case TC_BOOLEAN:
		  setSlotBoolean(obj, slot, in.readBoolean());
		  break;
		case TC_INT:
		  setSlotInt(obj, slot, in.readInt());
		  break;
		case TC_LONG:
		  setSlotLong(obj, slot, in.readLong());
		  break;
		case TC_DOUBLE:
		  setSlotDouble(obj, slot, in.readDouble());
		  break;
		default:
                  error("bogus type(" + sig + ")");
                  break;
	      }
              return;
	  }

	  case CMD_SET_BRKPT_LINE: {
	      Class c = (Class)objects.get(new Integer(in.readInt()));
	      int lineno = in.readInt();
	      LineNumber ln = lineno2pc(c, lineno);
	      String errorStr = "";
	      if (ln == null) {
		  errorStr = "No code at line " + lineno +
		      ", or class is optimized.";
	      } else {
		  try {
		      bkptHandler.addBreakpoint(c, ln.startPC,
						BKPT_NORMAL, null);
		  } catch (Exception e) {
		      message(exceptionStackTrace(e));
		      errorStr = e.toString();
		  }
	      }
	      out.writeUTF(errorStr);
	      return;
	  }

	  case CMD_SET_BRKPT_METHOD: {
	      Class c = (Class)objects.get(new Integer(in.readInt()));
	      int method_slot = in.readInt();
	      int pc = method2pc(c, method_slot);
	      message("method2pc(" + c.getName() + "," + method_slot +
		      ") = " + pc);
	      String errorStr = "";
	      if (pc == -1) {
		  errorStr = "Not a Java method";
	      } else {
		  try {
		      bkptHandler.addBreakpoint(c, pc, BKPT_NORMAL, null);
		  } catch (Exception e) {
		      message(exceptionStackTrace(e));
		      errorStr = e.toString();
		  }
	      }
	      out.writeUTF(errorStr);
	      return;
	  }

	  case CMD_LIST_BREAKPOINTS: {
	      BreakpointSet bkpts[] = bkptHandler.listBreakpoints();
	      out.writeInt(bkpts.length);
	      for (int i = 0; i < bkpts.length; i++) {
		  out.writeUTF(bkpts[i].clazz.getName() + ":" + 
			       pc2lineno(bkpts[i].clazz, bkpts[i].pc));
	      }
	      return;
	  }

	  case CMD_CLEAR_BRKPT: {
	      Class c = (Class)objects.get(new Integer(in.readInt()));
	      int pc = in.readInt();
	      message("clearing bkpt at " + c.getName() + ":" + pc);
	      String errorStr = "";
	      if (pc == -1) {
		  errorStr = "No code at pc " + pc;
	      } else {
		  try {
		      bkptHandler.deleteBreakpoint(c, pc);
		  } catch (Exception e) {
		      message(exceptionStackTrace(e));
		      errorStr = e.toString();
		  }
	      }
	      out.writeUTF(errorStr);
	      return;
	  }

	  case CMD_CLEAR_BRKPT_LINE: {
	      Class c = (Class)objects.get(new Integer(in.readInt()));
	      int lineno = in.readInt();
	      message("clearing bkpt at " + c.getName() + ":" + lineno);
	      LineNumber ln = lineno2pc(c, lineno);
	      String errorStr = "";
	      if (ln == null) {
		  errorStr = "No code at line " + lineno +
		      ", or class is optimized.";
	      } else {
		  try {
		      bkptHandler.deleteBreakpoint(c, ln.startPC);
		  } catch (Exception e) {
		      message(exceptionStackTrace(e));
		      errorStr = e.toString();
		  }
	      }
	      out.writeUTF(errorStr);
	      return;
	  }

	  case CMD_CLEAR_BRKPT_METHOD: {
	      Class c = (Class)objects.get(new Integer(in.readInt()));
	      int slotno = in.readInt();
	      message("clearing bkpt at " + c.getName());
	      int pc = method2pc(c, slotno);
	      String errorStr = "";
	      if (pc == -1) {
		  errorStr = "Not a Java method";
	      } else {
		  try {
		      bkptHandler.deleteBreakpoint(c, pc);
		  } catch (Exception e) {
		      message(exceptionStackTrace(e));
		      errorStr = e.toString();
		  }
	      }
	      out.writeUTF(errorStr);
	      return;
	  }

	  case CMD_CATCH_EXCEPTION_CLS: {
	      String errorStr = "";
	      try {
		  Class c = (Class)objects.get(new Integer(in.readInt()));
		  bkptHandler.catchExceptionClass(c);
	      } catch (Exception e) {
		  error("catchExceptionClass failed: " + e);
	      }
	      return;
	  }

	  case CMD_IGNORE_EXCEPTION_CLS: {
	      String errorStr = "";
	      try {
		  Class c = (Class)objects.get(new Integer(in.readInt()));
		  bkptHandler.ignoreExceptionClass(c);
	      } catch (Exception e) {
		  error("ignoreExceptionClass failed: " + e);
	      }
	      return;
	  }

	  case CMD_GET_CATCH_LIST: {
	      Class list[] = bkptHandler.getCatchList();
	      out.writeInt(list.length);
	      for (int i = 0; i < list.length; i++) {
		  writeObject(list[i]);
	      }
	      return;
	  }

	  case CMD_STOP_THREAD: {
	      Thread t = (Thread)objects.get(new Integer(in.readInt()));
	      SecurityManager.setScopePermission();
	      t.stop();
              message(t.getName() + " stopped.");
	      return;
	  }

	  case CMD_STOP_THREAD_GROUP: {
	      ThreadGroup tg = (ThreadGroup)objects.get(
		  new Integer(in.readInt()));
	      SecurityManager.setScopePermission();
	      tg.stop();
              message(tg.getName() + " stopped.");
	      return;
	  }

	  case CMD_SET_VERBOSE:
	      verbose = in.readBoolean();
	      return;

	  case CMD_SYSTEM: {
	      switch (in.readInt()) {
		case SYS_MEMORY_FREE:
		  // REMIND: use writeLong instead
		  out.writeInt((int)Runtime.getRuntime().freeMemory());
		  break;

		case SYS_MEMORY_TOTAL:
		  // REMIND: use writeLong instead
		  out.writeInt((int)Runtime.getRuntime().totalMemory());
		  break;

		case SYS_TRACE_METHOD_CALLS:
		  Runtime.getRuntime().traceMethodCalls(in.readInt() != 0);
		  break;

		case SYS_TRACE_INSTRUCTIONS:
		  Runtime.getRuntime().traceInstructions(in.readInt() != 0);
		  break;
	      }
	      return;
	  }

	  case CMD_GET_SOURCE_FILE: {
	      String sourceName = in.readUTF();
	      String className = in.readUTF();

	      // Extract the package name from the class (if any).
	      int iDot = className.lastIndexOf('.');
	      String pkgName = (iDot >= 0) ?
		  className.substring(0, iDot) : "";
	      Package pkg = null;
	      try {
		  pkg = new Package(sourcePath, Identifier.lookup(pkgName));
	      } catch (Exception e) {
		  error("cannot create a Package for " + pkgName);
		  out.writeInt(-1);
		  return;
	      }	      
	      
	      ClassFile sourceFile = null;
	      InputStream input = null;
	      try {
		  sourceFile = pkg.getSourceFile(sourceName);
		  input = sourceFile.getInputStream();
	      } catch (Exception e) {
		  error("cannot find " + sourceName);
		  out.writeInt(-1);
		  return;
	      }
	      int length = (int)sourceFile.length();
	      out.writeInt(length);
	      byte buffer[] = new byte[length];
	      try {
		  int off = 0, len = buffer.length;
		  while (len > 0) {
		      int n = input.read(buffer, off, len);
		      if (n == -1) {
			  throw new IOException();
		      }
		      off += n;
		      len -= n;
		  }
		  out.write(buffer);
	      } catch (IOException e) {
		  error("unable to read " + sourceName);
		  return;
	      }
	      input.close();
	      return;
	  }

	  case CMD_OBJECT_TO_STRING: {
	      Object o = objects.get(new Integer(in.readInt()));
	      String retStr = "";
	      try {
		  retStr = o.toString();
	      } catch (Exception e) {}
	      out.writeUTF(retStr);
	      return;
	  }

	  case CMD_GET_SOURCEPATH: {
	      out.writeUTF(sourcePath.toString());
	      return;
	  }

	  case CMD_SET_SOURCEPATH: {
	      String sourcePathString = in.readUTF();
	      if (sourcePathString == null) {
		  sourcePathString = ".";
	      }
	      sourcePath = new ClassPath(sourcePathString);
	      return;
	  }

	  case CMD_STEP_THREAD:
	      stepThread((Thread)objects.get(new Integer(in.readInt())),
		     in.readBoolean());
	      return;

	  case CMD_NEXT_STEP_THREAD:
	      stepNextThread((Thread)objects.get(new Integer(in.readInt())));
	      return;

	  case CMD_QUIT: {
	      System.exit(0);
	  }
	}
	error("command not understood: " + cmd);
    }

    native int objectId(Object obj);
    native void runMain(Class c, String argv[]);

    void writeObject(Object obj, DataOutputStream out) throws IOException {
	int h = 0;

	if (obj == null) {
	    out.writeInt(TP_OBJECT);
	} else if (obj instanceof Class) {
	    out.writeInt(TP_CLASS);
	    objects.put(new Integer(h = objectId(obj)), obj);
	} else if (obj instanceof String) {
	    out.writeInt(TP_STRING);
	    objects.put(new Integer(h = objectId(obj)), obj);
	} else {
	    if (obj instanceof Thread) {
		out.writeInt(TP_THREAD);
	    } else if (obj instanceof ThreadGroup) {
		out.writeInt(TP_THREADGROUP);
	    } else {
		out.writeInt(TP_OBJECT);
	    }
	    Class c = obj.getClass();
	    objects.put(new Integer(h = objectId(c)), c);
	    out.writeInt(h);
	    objects.put(new Integer(h = objectId(obj)), obj);
	}
	out.writeInt(h);
    }

    /* Write an object */
    void writeObject(Object obj) throws IOException{
	writeObject(obj, out);
    }

    void writeArray(Object obj, int size) throws IOException {
	int h = 0;

	out.writeInt(TC_ARRAY);
	if (obj == null) {
	    out.writeInt(0);
	} else {
	    Class c = obj.getClass();
	    objects.put(new Integer(h = objectId(c)), c);
	    out.writeInt(h);
	    objects.put(new Integer(h = objectId(obj)), obj);
	    out.writeInt(h);
	    out.writeInt(size);
	}
    }

    void write(boolean b) throws IOException {
	out.writeInt(TC_BOOLEAN);
	out.writeBoolean(b);
    }

    void write(byte b) throws IOException {
	out.writeInt(TC_BYTE);
	out.writeByte(b);
    }
    
    void write(char c) throws IOException {
	out.writeInt(TC_CHAR);
	out.writeChar(c);
    }
    
    void write(short s) throws IOException {
	out.writeInt(TC_SHORT);
	out.writeShort(s);
    }
    
    void write(int i) throws IOException {
	out.writeInt(TC_INT);
	out.writeInt(i);
    }
    
    void write(long l) throws IOException {
	out.writeInt(TC_LONG);
	out.writeLong(l);
    }
    
    void write(float f) throws IOException {
	out.writeInt(TC_FLOAT);
	out.writeFloat(f);
    }
    
    void write(double d) throws IOException {
	out.writeInt(TC_DOUBLE);
	out.writeDouble(d);
    }

    void reportAppExit() throws IOException {
        /* The application has either called System.exit, or its
         * main has returned.
         */
        synchronized (asyncOutputStream) {
            asyncOutputStream.write(CMD_QUIT_NOTIFY);
            asyncOutputStream.flush();
        }
    }

    /* Error, status messages */
    static void error(String msg) {
	System.out.println("[AGENT: " + msg + "]");
    }
    static void message(String msg) {
	if (verbose) {
	    System.out.println("[agent: " + msg + "]");
	}
    }

    /* Bootstrap the agent */
    public static void boot(int port) {
	SecurityManager.setScopePermission();
	Agent agent = new Agent(port);
	Thread t = new Thread(agent);
	t.setDaemon(true);
	t.setName("Debugger agent");
	SecurityManager.resetScopePermission();
	t.start();
    }

    static {
	SecurityManager.setScopePermission();
	System.loadLibrary("agent");
    }
}
