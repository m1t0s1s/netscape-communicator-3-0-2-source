/*
 * @(#)StackFrame.java	1.10 95/12/02
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
 * The StackFrame class represents a stack frame of a suspended thread.  
 *
 * @see RemoteDebugger
 * @see RemoteStackFrame
 * @see RemoteThread
 * @author Thomas Ball
 */
public class StackFrame {
    Class  clazz;
    String className;
    String methodName;
    int    pc;
    int    lineno;
    LocalVariable localVariables[];

    public String toString() {
	if (pc == -1) {
	    // Native method.
	    return className + "." + methodName + " (native method)";
	} else if (lineno == -1) {
	    // No line number information available, use pc.
	    return className + "." + methodName + " (pc " + pc + ")";
	} else {
	    int iPkg = className.lastIndexOf('.');
	    String baseName = (iPkg >= 0) ?
		className.substring(iPkg + 1) : className;
	    return className + "." + methodName +
		" (" + baseName + ":" + Integer.toString(lineno) + ")";
	}
    }
}
