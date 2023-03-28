/*
 * @(#)UNIXProcess.java	1.10 95/11/12 Chris Warth
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
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

package java.lang;

import java.io.*; 
import java.util.Hashtable;
import java.lang.SecurityManager;

class ProcessReaper extends Thread {

    ProcessReaper() {
	SecurityManager.setScopePermission();
	setDaemon(true);
    }

    /*
     * This routine is called once from the Process Reaper thread and
     * never returns.  It's job is to catch sigchld events and pass them
     * on to class UNIXProcess.
     */ 
    private native void waitForDeath();


    public void run() {
	SecurityManager.setScopePermission();
	setName("ProcessReaper");
	SecurityManager.resetScopePermission();
	waitForDeath();
    }
}


public class UNIXProcess extends Process {
    static Hashtable subprocs = null;

    private boolean isalive = false;
    private int exit_code = 0;
    private int pid = 0;
    private FileDescriptor sync_fd;
    private FileDescriptor stdin_fd;
    private FileDescriptor stdout_fd;
    private FileDescriptor stderr_fd;
    private OutputStream stdin_stream;
    private InputStream stdout_stream;
    private InputStream stderr_stream;

    /*
     * This version of exec both forks a new subprocess and executes
     * the given command.
     */
    private native void exec(String cmd[], String env[]) throws java.io.IOException;
    private native int fork() throws java.io.IOException;

    private static synchronized void newChild(UNIXProcess p, int pid) {
	subprocs.put(new Integer(pid), p);
    }

    /*
    This method is called in response to receiving a signal
    */
    public static synchronized void deadChild(int pid, int exitcode) {
	System.err.println("Received sigchild for "+pid+" exit="+exitcode);

	UNIXProcess p = (UNIXProcess) subprocs.get(new Integer(pid));
	if (p == null) {
	    System.err.println("Warning: received SIGCHLD for nonexistant process "+pid);
	} else {
	    p.isalive = false;
	    subprocs.remove(new Integer(pid));
	    p.exit_code = exitcode;
	    try {
		/* if there is anyone doing io to the process - wake them up*/
		p.stdin_stream.close();
		p.stdout_stream.close();
		p.stderr_stream.close();
	    } catch (IOException e) {
	    }	
	    try {
		/* also wake up anyone who is doing a waitFor() */
		Class.forName("java.lang.UNIXProcess").notify();
	    } catch (ClassNotFoundException e) {
	    }
	}
    }

    private static synchronized void waitChild(int pid) throws InterruptedException {
	UNIXProcess p = (UNIXProcess) subprocs.get(new Integer(pid));
	if (p != null) {
	    try {
		Class.forName("java.lang.UNIXProcess").wait();
	    } catch (ClassNotFoundException e) {
	    }
	}
    }

    UNIXProcess(String cmdarray[], String env[]) throws java.io.IOException {
	SecurityManager.setScopePermission();
	if (subprocs == null) {
	    subprocs = new Hashtable();
	    (new ProcessReaper()).start();
	}
	SecurityManager.resetScopePermission();

	stdin_fd = new FileDescriptor();
	stdout_fd = new FileDescriptor();
	stderr_fd = new FileDescriptor();
	sync_fd = new FileDescriptor();

	if ((pid = fork()) == 0) {
	    /* child process */
	    exec(cmdarray, env);
	} else {
	    /* parent process */
	    isalive = true;
	    stdin_stream = new BufferedOutputStream(new FileOutputStream(stdin_fd));
	    stdout_stream = new BufferedInputStream(new FileInputStream(stdout_fd));
	    stderr_stream = new FileInputStream(stderr_fd);
	    newChild(this, pid);

	    FileOutputStream fp = new FileOutputStream(sync_fd);
	    fp.write(0);
	    fp.close();
	    fp = null;
	}

    }

    public OutputStream getOutputStream() {
	return stdin_stream;
    }

    public InputStream getInputStream() {
	return stdout_stream;
    }

    public InputStream getErrorStream() {
	return stderr_stream;
    }

    public int waitFor() throws InterruptedException {
	waitChild(pid);
	return exit_code;
    }

    private native int waitForUNIXProcess() throws InterruptedException;

    public int exitValue() { return exit_code; }

    public native void destroy();
}



