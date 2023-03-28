/*
 * @(#)DebuggerCallback.java	1.8 95/11/01
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
 * The DebuggerCallback interface is used to communicate asynchronous
 * information from the debugger to its client.  This may be the actual
 * client object, or a delegate of its choosing.
 */
public interface DebuggerCallback {

    /** Print text to the debugger's console window. */
    void printToConsole(String text) throws Exception;

    /** A breakpoint has been hit in the specified thread. */
    void breakpointEvent(RemoteThread t) throws Exception;

    /** An exception has occurred. */
    void exceptionEvent(RemoteThread t, String errorText) throws Exception;

    /** A thread has died. */
    void threadDeathEvent(RemoteThread t) throws Exception;

    /** The client interpreter has exited, either by returning from its
     *  main thread, or by calling System.exit(). */
    void quitEvent() throws Exception;
}
