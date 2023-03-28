/*
 * @(#)RemoteDebugger.java	1.16 95/12/09
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
import java.net.InetAddress;

/**
 * The RemoteDebugger class defines a client interface to the Java debugging
 * classes.  It is used to instantiate a connection with the Java interpreter
 * being debugged.  
 *
 * @version 1.0 26 June 1995
 * @author Thomas Ball
 */
public class RemoteDebugger {

    private RemoteAgent agent;

    /**
     * Create a remote debugger, connecting it with a running Java
     * interpreter.<P>To connect to a running interpreter, it must be 
     * started with the "-debug" option, whereupon it will print out 
     * the password for that debugging session.
     * @param host	the name of the system where a debuggable Java instance is running (default is localhost).
     * @param password	the password reported by the debuggable Java instance.  This should be null when starting a client interpreter.
     * @param client	the object to which notification messages are sent (it must support the DebuggerCallback interface)
     * @param verbose	turn on internal debugger message text
     */
    public RemoteDebugger(String host, String password, 
			  DebuggerCallback client, 
                          boolean verbose) throws Exception {
	agent = new RemoteAgent(host, password, "", client, verbose);
    }

    /**
     * Create a remote debugger, and connect it to a new client interpreter.<P>
     * @param javaArgs	optional java command-line parameters, such as -classpath
     * @param client	the object to which notification messages are sent (it must support the DebuggerCallback interface)
     * @param verbose	turn on internal debugger message text
     */
    public RemoteDebugger(String javaArgs, DebuggerCallback client, 
                          boolean verbose) throws Exception {
	agent = new RemoteAgent(InetAddress.getLocalHost().getHostName(), 
                                null, javaArgs, client, verbose);
    }

    /**
     * Close the connection to the remote debugging agent.
     */
    public void close() {
	agent.close();
    }

    /**
     * Get an object from the remote object cache.
     *
     * @param id	the remote object's id
     * @return the specified RemoteObject, or null if not cached.
     */
    public RemoteObject get(Integer id) {
	return agent.get(id);
    }

    /**
     * List the currently known classes.
     */
    public RemoteClass[] listClasses() throws Exception {
	return agent.listClasses();
    }

    /**
     * Find a specified class.  If the class isn't already known by the
     * remote debugger, the lookup request will be passed to the Java
     * interpreter being debugged.
     *
     * NOTE:  Substrings, such as "String" for "java.lang.String" will
     * return with the first match, and will not be successfully found
     * if the request is passed to the remote interpreter.
     *
     * @param name	the name (or a substring of the name)of the class
     * @return the specified (Remote)Class, or null if not found.
     */
    public RemoteClass findClass(String name) throws Exception {
	return agent.findClass(name);
    }

    /**
     * List threadgroups
     *
     * @param tg	the threadgroup which hold the groups to be listed, or null for all threadgroups
     */
    public RemoteThreadGroup[] listThreadGroups(RemoteThreadGroup tg) throws Exception {
	return agent.listThreadGroups(tg);
    }

    /**
     * Free all objects referenced by the debugger.
     *
     * The remote debugger maintains a copy of each object it has examined,
     * so that references won't become invalidated by the garbage collector
     * of the Java interpreter being debugged.  The gc() method frees all
     * all of these references, except those specified to save.
     * @param save_list	the list of objects to save.
     */
    public void gc(RemoteObject[] save_list) throws Exception {
	agent.gc(save_list);
    }

    /**
     * Turn on/off method call tracing.
     *
     * When turned on, each method call is reported to the stdout of the
     * Java interpreter being debugged.  This output is not captured in
     * any way by the remote debugger.
     *
     * @param traceOn	turn tracing on or off
     */
    public void trace(boolean traceOn) throws Exception {
	agent.trace(traceOn);
    }

    /**
     * Turn on/off instruction tracing.
     *
     * When turned on, each Java instruction is reported to the stdout of the
     * Java interpreter being debugged.  This output is not captured in
     * any way by the remote debugger.
     *
     * @param traceOn	turn tracing on or off
     */
    public void itrace(boolean traceOn) throws Exception {
	agent.itrace(traceOn);
    }

    /**
     * Report the total memory usage of the Java interpreter being debugged.
     */
    public int totalMemory() throws Exception {
	return agent.totalMemory();
    }

    /**
     * Report the free memory available to the Java interpreter being debugged.
     */
    public int freeMemory() throws Exception {
	return agent.freeMemory();
    }

    /**
     * Load and run a runnable Java class, with any optional parameters.
     * The class is started inside a new threadgroup in the Java interpreter
     * being debugged.
     *
     * NOTE:  Although it is possible to run multiple runnable classes from
     * the same Java interpreter, there is no guarantee that all applets
     * will work cleanly with each other.  For example, two applets may want
     * exclusive access to the same shared resource,such as a specific port.
     *
     * @param argc	the number of parameters
     * @param argv	the array of parameters:  the class to be run is
     * first, followed by any optional parameters used by that class.
     * @return the new ThreadGroup the class is running in, or null on error
     */
    public RemoteThreadGroup run(int argc, String argv[]) throws Exception {
	return agent.run(argc, argv);
    }

    /**
     * Return a list of the breakpoints which are currently set.
     *
     * @return an array of Strings of the form "class_name:line_number".
     */
    public String[] listBreakpoints() throws Exception {
	return agent.listBreakpoints();
    }

    /**
     * Return the list of the exceptions the debugger will stop on.
     *
     * @return an array of exception class names, which may be zero-length.
     */
    public String[] getExceptionCatchList() throws Exception {
	return agent.getExceptionCatchList();
    }

    /**
     * Return the source file path the Agent is currently using.
     *
     * @return a string consisting of a list of colon-delineated paths.
     */
    public String getSourcePath() throws Exception {
	return agent.getSourcePath();
    }

    /**
     * Specify the list of paths to use when searching for a source file.
     *
     * @param pathList	a string consisting of a list of colon-delineated paths.
     */
    public void setSourcePath(String pathList) throws Exception {
	agent.setSourcePath(pathList);
    }
}
