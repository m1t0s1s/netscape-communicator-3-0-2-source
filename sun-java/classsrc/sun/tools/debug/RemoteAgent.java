/*
 * @(#)RemoteAgent.java	1.45 96/01/31
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


class RemoteAgent implements AgentConstants {
    /* The agent socket */
    private Socket socket;

    /* The input stream from the remote agent */
    private DataInputStream in;

    /* The output stream to the remote agent */
    private DataOutputStream out;

    /* Hashtable of active objects */
    private Hashtable objects = new Hashtable();

    /* The DebuggerCallback object. */
    private DebuggerCallback client;

    /* The client side of the Agent's stdout redirector */
    private AgentIn debugIn = null;
    private Thread debugInThread = null;

    private RemoteClass classObject;
    RemoteClass classClass;
    RemoteClass classString;
    boolean closeRemoteInterpreter = false;

    private RemoteThreadGroup systemThreadGroup;

    private boolean verbose = false;
    private boolean closing = false;

    private int readPassword(String s) {
        int cookie = 0;
        int radix = PSWDCHARS.length();
        for (int i = 0; i < s.length(); i++) {
            int x = PSWDCHARS.indexOf(s.charAt(i));
            if (x == -1) {
                return 0;	// bad password
            }
            cookie = (cookie * radix) + x;
        }
        int port = 0;
        for (int i = 0; i < 8; i++) {
            int bits = (cookie & (6 << (i * 3)));
            port |= (bits >> (i + 1));
        }
        return port;
    }

    /* Connect to a remote agent */
    RemoteAgent(String host, String password, String javaArgs,
                DebuggerCallback client, boolean verbose)  throws Exception {
	this.verbose = verbose;
        if (host == InetAddress.getLocalHost().getHostName() &&
            password == null) {
            // No remote (or local) interpreter to debug, so spawn one.
	    SecurityManager.setScopePermission();
            String slash = System.getProperty("file.separator");
            String cmd = new String(System.getProperty("java.home") +
                                    slash + "bin" + slash +
                                    "java_g -debug " + javaArgs +
                                    " sun.tools.debug.EmptyApp");
	    SecurityManager.resetScopePermission();
            Process pr = Runtime.getRuntime().exec(cmd);
            DataInputStream dis = new DataInputStream(pr.getInputStream());

            String agentStatus = dis.readLine();
            if (agentStatus == null) {
                error("Failed to exec a child java interpreter.");
                System.exit(1);
            }
            password = agentStatus.substring(agentStatus.indexOf("=") + 1);
            message("password returned: " + password);
            closeRemoteInterpreter = true;
        }

        this.client = client;
	socket = new Socket(host, readPassword(password));

	debugIn = new AgentIn(this, new Socket(host, getDebugPort()),
			      client, verbose);
	debugInThread = new Thread(debugIn, "Agent input");
	debugInThread.start();

	in = new DataInputStream(new
		BufferedInputStream(socket.getInputStream()));
	out = new DataOutputStream(new
		BufferedOutputStream(socket.getOutputStream()));
	message("connected");

	/* Check that agent is the same version */
	int version = in.readInt();
	if (version != AGENT_VERSION) {
	    System.err.println(
		"Version mismatch between debugger and remote agent.");
	    System.exit(1);
	}

	/* Load current Agent classes */
	classObject = (RemoteClass)readValue(in);
	classClass = (RemoteClass)readValue(in);
	classString = (RemoteClass)readValue(in);
	message("loading classes ...");
	int nClasses = in.readInt();
	for (int i = 0 ; i < nClasses ; i++) {
	    RemoteClass c = (RemoteClass)readValue(in);
	}

	systemThreadGroup = (RemoteThreadGroup)readValue(in);

	setVerbose(verbose);
    }

    /* Return the socket port being used. */
    int getDebugPort() {
	return socket.getPort();
    }

    /* Read in a remote source file. */
    InputStream getSourceFile(String clsName, String name) throws Exception {
        synchronized (out) {
            out.write(CMD_GET_SOURCE_FILE);
            out.writeUTF(name);
            out.writeUTF(clsName);
            try {
                getReply(CMD_GET_SOURCE_FILE);
            } catch (Exception e) {
                return null;
            }

            int length = in.readInt();
            if (length == -1) {
                return null;
            }
            message("getSourceFile: allocating " + length + " bytes.");
            byte buffer[] = new byte[length];
            try {
                in.readFully(buffer);
            } catch (IOException e) {
                error("unable to read " + name);
                return null;
            }
            return new ByteArrayInputStream(buffer);
        }
    }
    
    /* Ask a remote object for a string description of itself. */
    String objectToString(int id)  {
        synchronized (out) {
            try {
                out.write(CMD_OBJECT_TO_STRING);
                out.writeInt(id);
                getReply(CMD_OBJECT_TO_STRING);
                return in.readUTF();
            } catch (Exception ex) {
                return("<communication error>");
            }
        }
    }

    /* Close the connection */
    void close() {
	message("close(" + closeRemoteInterpreter + ")");
	closing = true;
        try {
	    SecurityManager.setScopePermission();
            debugInThread.stop();
	    SecurityManager.resetScopePermission();
            if (closeRemoteInterpreter) {
                synchronized (out) {
                    out.write(CMD_QUIT);
                    getReply(CMD_QUIT);
                }
            }
            socket.close();
        } catch (Exception e) {}
    }

    /* Get an object from the cache */
    RemoteObject get(Integer h) {
	return (RemoteObject)objects.get(h);
    }

    /* Put an object in the cache */
    private void put(Integer h, RemoteObject obj) {
	objects.put(h, obj);
    }

    /* Read a value */
    RemoteValue readValue(DataInputStream in) throws Exception {
	while(true) {
	    int type = in.readInt();
	    switch (type) {
	      case TP_OBJECT: {
		  int id = in.readInt();
		  if (id == 0) {
		      return null;
		  }

		  Integer h = new Integer(id);
		  RemoteClass c = (RemoteClass)get(h);
		  if (c == null) {
		      put(h, c = new RemoteClass(this, id));
		  }

		  id = in.readInt();
		  h = new Integer(id);
		  RemoteObject obj = get(h);
		  if (obj == null) {
		      put(h, obj = new RemoteObject(this, id, c));
		  }
		  return obj;
	      }

	      case TP_CLASS: {
		  int id = in.readInt();
		  Integer h = new Integer(id);
		  RemoteClass c = (RemoteClass)get(h);
		  if (c == null) {
		      put(h, c = new RemoteClass(this, id));
		  }
		  return c;
	      }

	      case TP_THREAD: {
		  int id = in.readInt();
		  Integer h = new Integer(id);
		  RemoteClass c = (RemoteClass)get(h);
		  if (c == null) {
		      put(h, c = new RemoteClass(this, id));
		  }

		  id = in.readInt();
		  h = new Integer(id);
		  RemoteThread t = (RemoteThread)get(h);
		  if (t == null) {
		      put(h, t = new RemoteThread(this, id, c));
		  }
		  return t;
	      }

	      case TP_THREADGROUP: {
		  int id = in.readInt();
		  Integer h = new Integer(id);
		  RemoteClass c = (RemoteClass)get(h);
		  if (c == null) {
		      put(h, c = new RemoteClass(this, id));
		  }

		  id = in.readInt();
		  h = new Integer(id);
		  RemoteThreadGroup tg = (RemoteThreadGroup)get(h);
		  if (tg == null) {
		      SecurityManager.setScopePermission();
		      put(h, tg = new RemoteThreadGroup(this, id, c));
		  }
		  return tg;
	      }

	      case TP_STRING: {
		  int id = in.readInt();
		  Integer h = new Integer(id);
		  RemoteString s = (RemoteString)get(h);
		  if (s == null) {
		      put(h, s = new RemoteString(this, id));
		  }
		  return s;
	      }

	      case TC_ARRAY: {
		  int id = in.readInt();
		  if (id == 0) {
		      return null;
		  }
		  Integer h = new Integer(id);
		  RemoteClass c = (RemoteClass)get(h);
		  if (c == null) {
		      put(h, c = new RemoteClass(this, id));
		  }
		  id = in.readInt();
		  h = new Integer(id);
		  int size = in.readInt();
		  RemoteArray array = (RemoteArray)get(h);
		  if (array == null) {
		      put(h, array = new RemoteArray(this, id, c, size));
		  }
		  return array;
	      }

	      case TC_BOOLEAN:
		  return (RemoteValue)(new RemoteBoolean(in.readBoolean()));

	      case TC_BYTE:
		  return (RemoteValue)(new RemoteByte(in.readByte()));

	      case TC_CHAR:
		  return (RemoteValue)(new RemoteChar(in.readChar()));

	      case TC_SHORT:
		  return (RemoteValue)(new RemoteShort(in.readShort()));

	      case TC_INT:
		  return (RemoteValue)(new RemoteInt(in.readInt()));

	      case TC_LONG:
		  return (RemoteValue)(new RemoteLong(in.readLong()));

	      case TC_FLOAT:
		  return (RemoteValue)(new RemoteFloat(in.readFloat()));

	      case TC_DOUBLE:
		  return (RemoteValue)(new RemoteDouble(in.readDouble()));

	      default: {
		  error("invalid type code: " + type);
		  return null;
	      }
	    }
	}
    }

    /* get a reply */
    private void getReply(int cmd) throws Exception {
	out.flush();

	while (true) {
	    int c = 0;
	    try {
		c = in.readInt();
	    } catch (EOFException e) {
		if (!closing) {
		    error("unexpected eof");
		}
		return;
	    }

	    if (c == CMD_EXCEPTION) {
		Exception e = new Exception(in.readUTF());
		throw e;
	    } else if (c != cmd) {
		error("unexpected reply: wanted " + cmd + ", got " + c);
	    } else {
		return;
	    }
	}
    }

    /* List the currently known classes. */
    RemoteClass[] listClasses() throws Exception  {
        synchronized (out) {
            out.write(CMD_LIST_CLASSES);
            getReply(CMD_LIST_CLASSES);
            int n = in.readInt();
            message("reading " + n + " classes...");
            RemoteClass list[] = new RemoteClass[n];

            for (int i = 0 ; i < n ; i++) {
                list[i] = (RemoteClass)readValue(in);
            }
            return list;
        }
    }

    /* Find a specified class. */
    RemoteClass findClass(String name) throws Exception {
	RemoteClass list[] = listClasses();
	for (int i = 0; i < list.length; i++) {
	    String className = list[i].getName();

	    /* Try full name (ie "java.lang.Object") */
	    if (name.equals(className)) {
		return list[i];
	    }

	    /* Next, try short name (ie "Object") */
	    int iDot = className.lastIndexOf(".");
	    if (iDot >= 0 && className.length() > iDot) {
		className = className.substring(iDot + 1);
	    }
	    if (name.equals(className)) {
		return list[i];
	    }
	}
	return getClassByName(name);
    }

    /* List threadgroups */
    RemoteThreadGroup[] listThreadGroups(RemoteThreadGroup tg) 
      throws Exception {
	RemoteThreadGroup list[] = null;
        synchronized (out) {
            out.write(CMD_LIST_THREADGROUPS);
            out.writeInt((tg == null) ? 0 : tg.getId());

            getReply(CMD_LIST_THREADGROUPS);
            int n = in.readInt();
	    SecurityManager.setScopePermission();
            list = new RemoteThreadGroup[n];
	    SecurityManager.resetScopePermission();
            message("listThreadGroups: " + n + " groups");
            for (int i = 0 ; i < n ; i++) {
                list[i] = (RemoteThreadGroup)readValue(in);
            }
        }
	return list;
    }

    /* List a threadgroup's threads */
    RemoteThread[] listThreads(RemoteThreadGroup tg,
                               boolean recurse) throws Exception {
        synchronized (out) {
            out.write(CMD_LIST_THREADS);
            out.writeInt((tg == null) ? systemThreadGroup.getId() : tg.getId());
            out.writeBoolean(recurse);

            getReply(CMD_LIST_THREADS);
            int n = in.readInt();
            RemoteThread list[] = new RemoteThread[n];

            for (int i = 0 ; i < n ; i++) {
                list[i] = (RemoteThread)readValue(in);
            }
            return list;
        }
    }

    /* Suspend a specified thread. */
    void suspendThread(int id) throws Exception {
        synchronized (out) {
            out.write(CMD_SUSPEND_THREAD);
            out.writeInt(id);
            getReply(CMD_SUSPEND_THREAD);
        }
    }

    /* Resume a specified thread. */
    void resumeThread(int id) throws Exception {
        synchronized (out) {
            out.write(CMD_RESUME_THREAD);
            out.writeInt(id);
            getReply(CMD_RESUME_THREAD);
        }
    }

    /* Dump a thread's stack. */
    RemoteStackFrame[] dumpStack(RemoteThread thread) throws Exception {
        synchronized (out) {
            out.write(CMD_GET_STACK_TRACE);
            out.writeInt(thread.getId());

            getReply(CMD_GET_STACK_TRACE);
            int n = in.readInt();
            RemoteStackFrame list[] = new RemoteStackFrame[n];
            for (int i = 0 ; i < n ; i++) {
                RemoteObject remoteFrame = (RemoteObject)readValue(in);
                list[i] = new RemoteStackFrame(remoteFrame, thread, i, this);
                list[i].className = in.readUTF();
                list[i].methodName = in.readUTF();
                list[i].lineno = in.readInt();
                list[i].pc = in.readInt();
                list[i].setRemoteClass((RemoteClass)readValue(in));

                int nVars = in.readInt();
                list[i].localVariables = new LocalVariable[nVars];
                for (int j = 0; j < nVars; j++) {
                    LocalVariable lvar = new LocalVariable();
                    lvar.slot = in.readInt();
                    lvar.name = in.readUTF();
                    lvar.signature = in.readUTF();
                    lvar.methodArgument = in.readBoolean();
                    message("lvar " + j + ": slot=" + lvar.slot +
                            ", name=" + lvar.name + ", sig=" + 
                            lvar.signature + ", arg=" + lvar.methodArgument);
                    list[i].localVariables[j] = lvar;
                }
            }
            return list;
        }
    }

    /* Stop the specified thread */
    void stopThread(int id) throws Exception {
        synchronized (out) {
            out.write(CMD_STOP_THREAD);
            out.writeInt(id);
            getReply(CMD_STOP_THREAD);
        }
    }

    /* Stop the specified threadgroup */
    void stopThreadGroup(int id) throws Exception {
        synchronized (out) {
            out.write(CMD_STOP_THREAD_GROUP);
            out.writeInt(id);
            getReply(CMD_STOP_THREAD_GROUP);
        }
    }

    /* Get the class name of a class */
    void getClassInfo(RemoteClass c) throws Exception {
        synchronized (out) {
            out.write(CMD_GET_CLASS_INFO);
            out.writeInt(c.id);

            getReply(CMD_GET_CLASS_INFO);
            c.name = in.readUTF();
            c.sourceName = in.readUTF();
            c.intf = in.readInt() != 0;
            c.superclass = (RemoteClass)readValue(in);
            c.loader = (RemoteObject)readValue(in);
            c.interfaces = new RemoteClass[in.readInt()];
            for (int i = 0 ; i < c.interfaces.length ; i++) {
                c.interfaces[i] = (RemoteClass)readValue(in);
            }
        }
    }

    /* Get a remote class by name */
    RemoteClass getClassByName(String name) throws Exception {
        synchronized (out) {
            out.write(CMD_GET_CLASS_BY_NAME);
            out.writeUTF(name);

            getReply(CMD_GET_CLASS_BY_NAME);
            return (RemoteClass)readValue(in);
        }
    }	

    /* Get threadgroup internal information */
    void getThreadGroupInfo(RemoteThreadGroup c) throws Exception {
        synchronized (out) {
            out.write(CMD_GET_THREADGROUP_INFO);
            out.writeInt(c.id);

            getReply(CMD_GET_THREADGROUP_INFO);
            c.parent = (RemoteThreadGroup)readValue(in);
            c.name = in.readUTF();
            c.maxPriority = in.readInt();
            c.daemon = in.readBoolean();
        }
    }

    /* Get the name of a thread */
    String getThreadName(int id) throws Exception {
        synchronized (out) {
            out.write(CMD_GET_THREAD_NAME);
            out.writeInt(id);

            getReply(CMD_GET_THREAD_NAME);
            return in.readUTF();
        }
    }

    /* Get the current status of a thread */
    int getThreadStatus(int id) throws Exception  {
        synchronized (out) {
            out.write(CMD_GET_THREAD_STATUS);
            out.writeInt(id);

            getReply(CMD_GET_THREAD_STATUS);
            return in.readInt();
        }
    }

    /* Free all unreferenced objects */
    void gc(RemoteObject[] objs) throws Exception  {
        synchronized (out) {
            out.write(CMD_MARK_OBJECTS);

            /* Send a list of all objects we want to keep. */
            out.writeInt(objs.length);
            for (int i = 0; i < objs.length; i++) {
                if (objs[i] == null) {
                    out.writeInt(0);
                } else {
                    out.writeInt(objs[i].id);
                }
            }
            getReply(CMD_MARK_OBJECTS);
            out.write(CMD_FREE_OBJECTS);
            getReply(CMD_FREE_OBJECTS);

            // Read in a new list of cache objects.
            objects = new Hashtable();
            while (readValue(in) != null) {
                // Side effect: readValue() fills the objects cache as needed.
            }
        }
    }

    /* Turn on/off method call tracing. */
    void trace(boolean traceOn) throws Exception  {
        synchronized (out) {
            out.write(CMD_SYSTEM);
            out.writeInt(SYS_TRACE_METHOD_CALLS);
            out.writeInt(traceOn ? 1 : 0);
            getReply(CMD_SYSTEM);
        }
    }

    /* Turn on/off instruction tracing */
    void itrace(boolean traceOn) throws Exception  {
        synchronized (out) {
            out.write(CMD_SYSTEM);
            out.writeInt(SYS_TRACE_INSTRUCTIONS);
            out.writeInt(traceOn ? 1 : 0);
            getReply(CMD_SYSTEM);
        }
    }

    /* Report Agent's total memory usage.  */
    int totalMemory() throws Exception  {
        synchronized (out) {
            out.write(CMD_SYSTEM);
            out.writeInt(SYS_MEMORY_TOTAL);
            getReply(CMD_SYSTEM);
            return in.readInt();
        }
    }

    /* Report Agent's free memory.  */
    int freeMemory() throws Exception  {
        synchronized (out) {
            out.write(CMD_SYSTEM);
            out.writeInt(SYS_MEMORY_FREE);
            getReply(CMD_SYSTEM);
            return in.readInt();
        }
    }

    /* Run main program */
    RemoteThreadGroup run(int argc, String argv[]) throws Exception  {
        synchronized (out) {
            out.write(CMD_RUN);
            out.write(argc);
            for (int i = 0 ; i < argc ; i++) {
                out.writeUTF(argv[i]);
            }
            getReply(CMD_RUN);
            return (RemoteThreadGroup)readValue(in);
        }
    }

    /* Get remote copies of all the fields of an Object. */
    RemoteField[] getFields(int id) throws Exception {
        synchronized (out) {
            out.write(CMD_GET_FIELDS);
            out.writeInt(id);

            getReply(CMD_GET_FIELDS);
            int n = in.readInt();
            RemoteField list[] = new RemoteField[n];
            for (int i = 0 ; i < n ; i++) {
                int slot = in.readInt();
                String name = in.readUTF();
                String signature = in.readUTF();
                short access = in.readShort();
                RemoteClass clazz = (RemoteClass)readValue(in);
                list[i] = new RemoteField(this, slot, name, signature,
                                          access, clazz);
            }
            return list;
        }
    }

    /* Get all the elements of an array. */
    RemoteValue[] getElements(int id, int type,
                              int beginIndex, int endIndex) throws Exception 
    {
        synchronized (out) {
            out.write(CMD_GET_ELEMENTS);
            out.writeInt(id);
            out.writeInt(type);
            out.writeInt(beginIndex);
            out.writeInt(endIndex);

            getReply(CMD_GET_ELEMENTS);
            RemoteValue elements[] = new RemoteValue[in.readInt()];
            for (int i = 0 ; i < elements.length ; i++) {
                elements[i] = readValue(in);
            }
            return elements;
        }
    }

    /* Get remote copies of all the methods of an Object. */
    RemoteField[] getMethods(int id) throws Exception {
        synchronized (out) {
            out.write(CMD_GET_METHODS);
            out.writeInt(id);

            getReply(CMD_GET_METHODS);
            int n = in.readInt();
            message("getting (" + n + ") methods");
            RemoteField list[] = new RemoteField[n];
            for (int i = 0 ; i < n ; i++) {
                int slot = in.readInt();
                String name = in.readUTF();
                String signature = in.readUTF();
                short access = in.readShort();
                RemoteClass clazz = (RemoteClass)readValue(in);
                list[i] = new RemoteField(this, slot, name, signature,
                                          access, null);
            }
            return list;
        }
    }

    /* Get the type signature of a Class's field. */
    int getSlotSignature(int id, int n)[] throws Exception {
        synchronized (out) {
            out.write(CMD_GET_SLOT_SIGNATURE);
            out.writeInt(id);
            out.writeInt(n);

            getReply(CMD_GET_SLOT_SIGNATURE);
            int sig[] = new int[in.readInt()];
            for (int i = 0; i < sig.length; i++) {
                sig[i] = in.readInt();
            }
            return sig;
        }
    }

    /* Get the object in slot n of an Object. */
    RemoteValue getSlotValue(int id, int slot) throws Exception {
        synchronized (out) {
            out.write(CMD_GET_SLOT_VALUE);
            out.writeInt(id);
            out.writeInt(slot);

            getReply(CMD_GET_SLOT_VALUE);
            RemoteValue rv = readValue(in);
            if (rv != null && rv.isObject() && rv.getClass().getName().equals(
                "java.lang.ArrayIndexOutOfBoundsException")) {
                throw new 
                    ArrayIndexOutOfBoundsException("invalid slot index "+slot);
            }
            return rv;
        }
    }

    /* Get the object in slot n of a StackFrame. */
    RemoteValue getStackValue(int threadId, int frame, int slot,
                              char type) throws Exception {
        synchronized (out) {
            out.write(CMD_GET_STACK_VALUE);
            out.writeInt(threadId);
            out.writeInt(frame);
            out.writeInt(slot);
            out.writeChar(type);

            getReply(CMD_GET_STACK_VALUE);
            return readValue(in);
        }
    }

    /* Set slot n of an Object to an integer value. */
    void setSlotValue(int id, int slot, int value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_SLOT_VALUE);
            out.writeInt(id);
            out.writeInt(slot);
            out.writeInt(TC_INT);
            out.writeInt(value);
            getReply(CMD_SET_SLOT_VALUE);
        }
    }

    /* Set slot n of an Object to a boolean value. */
    void setSlotValue(int id, int slot, boolean value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_SLOT_VALUE);
            out.writeInt(id);
            out.writeInt(slot);
            out.writeInt(TC_BOOLEAN);
            out.writeBoolean(value);
            getReply(CMD_SET_SLOT_VALUE);
        }
    }

    /* Set slot n of an Object to a char value. */
    void setSlotValue(int id, int slot, char value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_SLOT_VALUE);
            out.writeInt(id);
            out.writeInt(slot);
            out.writeInt(TC_INT);
            out.writeInt((int)value);
            getReply(CMD_SET_SLOT_VALUE);
        }
    }

    /* Set slot n of an Object to a long value. */
    void setSlotValue(int id, int slot, long value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_SLOT_VALUE);
            out.writeInt(id);
            out.writeInt(slot);
            out.writeInt(TC_LONG);
            out.writeLong(value);
            getReply(CMD_SET_SLOT_VALUE);
        }
    }

    /* Set slot n of an Object to a float value. */
    void setSlotValue(int id, int slot, float value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_SLOT_VALUE);
            out.writeInt(id);
            out.writeInt(slot);
            out.writeInt(TC_DOUBLE);
            out.writeDouble((double)value);
            getReply(CMD_SET_SLOT_VALUE);
        }
    }

    /* Set slot n of an Object to a double value. */
    void setSlotValue(int id, int slot, double value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_SLOT_VALUE);
            out.writeInt(id);
            out.writeInt(slot);
            out.writeInt(TC_DOUBLE);
            out.writeDouble(value);
            getReply(CMD_SET_SLOT_VALUE);
        }
    }

    /* Set slot n of a stackframe to an integer value. */
    void setStackValue(int threadId, int frame, int slot, int value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_STACK_VALUE);
            out.writeInt(threadId);
            out.writeInt(frame);
            out.writeInt(slot);
            out.writeInt(TC_INT);
            out.writeInt(value);
            getReply(CMD_SET_STACK_VALUE);
        }
    }

    /* Set slot n of a stackframe to a boolean value. */
    void setStackValue(int threadId, int frame, int slot, boolean value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_STACK_VALUE);
            out.writeInt(threadId);
            out.writeInt(frame);
            out.writeInt(slot);
            out.writeInt(TC_BOOLEAN);
            out.writeBoolean(value);
            getReply(CMD_SET_STACK_VALUE);
        }
    }

    /* Set slot n of a stackframe to a char value. */
    void setStackValue(int threadId, int frame, int slot, char value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_STACK_VALUE);
            out.writeInt(threadId);
            out.writeInt(frame);
            out.writeInt(slot);
            out.writeInt(TC_INT);
            out.writeInt((int)value);
            getReply(CMD_SET_STACK_VALUE);
        }
    }

    /* Set slot n of a stackframe to a long value. */
    void setStackValue(int threadId, int frame, int slot, long value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_STACK_VALUE);
            out.writeInt(threadId);
            out.writeInt(frame);
            out.writeInt(slot);
            out.writeInt(TC_LONG);
            out.writeLong(value);
            getReply(CMD_SET_STACK_VALUE);
        }
    }

    /* Set slot n of a stackframe to a float value. */
    void setStackValue(int threadId, int frame, int slot, float value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_STACK_VALUE);
            out.writeInt(threadId);
            out.writeInt(frame);
            out.writeInt(slot);
            out.writeInt(TC_DOUBLE);
            out.writeDouble((double)value);
            getReply(CMD_SET_STACK_VALUE);
        }
    }

    /* Set slot n of a stackframe to a double value. */
    void setStackValue(int threadId, int frame, int slot, double value) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_STACK_VALUE);
            out.writeInt(threadId);
            out.writeInt(frame);
            out.writeInt(slot);
            out.writeInt(TC_DOUBLE);
            out.writeDouble(value);
            getReply(CMD_SET_STACK_VALUE);
        }
    }

    /* Set a breakpoint at a specified source line number in a class. */
    String setBreakpointLine(RemoteClass clazz, int lineno) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_BRKPT_LINE);
            out.writeInt(clazz.getId());
            out.writeInt(lineno);

            getReply(CMD_SET_BRKPT_LINE);
            return in.readUTF();
        }
    }

    /* Set a breakpoint at the first line of a specified class method. */
    String setBreakpointMethod(RemoteClass clazz,
                               RemoteField method) throws Exception 
    {
        synchronized (out) {
            out.write(CMD_SET_BRKPT_METHOD);
            out.writeInt(clazz.getId());
            out.writeInt(method.slot);

            getReply(CMD_SET_BRKPT_METHOD);
            return in.readUTF();
        }
    }

    /* Return a list of breakpoints that are currently set. */
    String listBreakpoints()[] throws Exception {
        synchronized (out) {
            out.write(CMD_LIST_BREAKPOINTS);
            getReply(CMD_LIST_BREAKPOINTS);
            String list[] = new String[in.readInt()];
            for (int i = 0; i < list.length; i++) {
                list[i] = in.readUTF();
            }
            return list;
        }
    }

    /* Clear a breakpoint at a specific address. */
    String clearBreakpoint(RemoteClass clazz, int pc) throws Exception {
        synchronized (out) {
            message("clearing bkpt at " + clazz.getName() + ", pc " + pc);
            out.write(CMD_CLEAR_BRKPT);
            out.writeInt(clazz.getId());
            out.writeInt(pc);

            getReply(CMD_CLEAR_BRKPT);
            return in.readUTF();
        }
    }

    /* Clear a breakpoint at a specified line. */
    String clearBreakpointLine(RemoteClass clazz, int lineno) throws Exception {
        synchronized (out) {
            message("clearing bkpt at " + clazz.getName() + ":" + lineno);
            out.write(CMD_CLEAR_BRKPT_LINE);
            out.writeInt(clazz.getId());
            out.writeInt(lineno);

            getReply(CMD_CLEAR_BRKPT_LINE);
            return in.readUTF();
        }
    }

    /* Clear a breakpoint at the start of a specified method. */
    String clearBreakpointMethod(RemoteClass clazz,
                                 RemoteField method) throws Exception 
    {
        synchronized (out) {
            message("clearing bkpt at " + clazz.getName() + ":" + method.getName());
            out.write(CMD_CLEAR_BRKPT_METHOD);
            out.writeInt(clazz.getId());
            out.writeInt(method.slot);

            getReply(CMD_CLEAR_BRKPT_METHOD);
            return in.readUTF();
        }
    }

    /* Add an exception class to be "caught", i.e. enter the debugger when
     * thrown.
     */
    void catchExceptionClass(RemoteClass c) throws Exception {
        synchronized (out) {
            out.write(CMD_CATCH_EXCEPTION_CLS);
            out.writeInt(c.getId());
            getReply(CMD_CATCH_EXCEPTION_CLS);
        }
    }	

    /* Don't enter the debugger when the specified exception class is
     * thrown.
     */
    void ignoreExceptionClass(RemoteClass c) throws Exception {
        synchronized (out) {
            out.write(CMD_IGNORE_EXCEPTION_CLS);
            out.writeInt(c.getId());
            getReply(CMD_IGNORE_EXCEPTION_CLS);
        }
    }

    /* Return a list of the exceptions the debugger will stop on. */
    String[] getExceptionCatchList() throws Exception {
        synchronized (out) {
            out.write(CMD_GET_CATCH_LIST);
            getReply(CMD_GET_CATCH_LIST);
            int n = in.readInt();
            String list[] = new String[n];
            for (int i = 0; i < n; i++) {
                RemoteClass c = (RemoteClass)readValue(in);
                list[i] = c.getName();
            }
            return list;
        }
    }

    /* Get the source file path the Agent is currently using. */
    String getSourcePath() throws Exception {
        synchronized (out) {
            out.write(CMD_GET_SOURCEPATH);
            getReply(CMD_GET_SOURCEPATH);
            return in.readUTF();
        }
    }

    /* Set the Agent's source file path. */
    void setSourcePath(String pathList) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_SOURCEPATH);
            out.writeUTF(pathList);
            getReply(CMD_SET_SOURCEPATH);
        }
    }

    /* Step through code an instruction or line. */
    void stepThread(int id, boolean stepLine) throws Exception {
        synchronized (out) {
            out.write(CMD_STEP_THREAD);
            out.writeInt(id);
            out.writeBoolean(stepLine);
            getReply(CMD_STEP_THREAD);
        }
    }

    /* Step through code an instruction or line. */
    void stepNextThread(int id) throws Exception {
        synchronized (out) {
            out.write(CMD_NEXT_STEP_THREAD);
            out.writeInt(id);
            getReply(CMD_NEXT_STEP_THREAD);
        }
    }

    /* Set whether internal debugging messages are reported. */
    private void setVerbose(boolean b) throws Exception {
        synchronized (out) {
            out.write(CMD_SET_VERBOSE);
            out.writeBoolean(b);
            getReply(CMD_SET_VERBOSE);
        }
    }

    /* Error message */
    void error(String msg) {
        try {
            client.printToConsole("[Internal debugger error: " + msg + "]\n");
        } catch (Exception e) {
            System.out.println("[Internal debugger error: " + msg + "]");
        }
    }

    /* Agent debugging message (only printed in verbose mode) */
    void message(String msg) {
	if (verbose) {
            try {
                client.printToConsole("[debugger: " + msg + "]\n");
            } catch (Exception e) {
                System.out.println("[debugger: " + msg + "]");
            }
	}
    }
}
