/*
 * @(#)BinaryExceptionHandler.java	1.5 95/09/14 Frank Yellin
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

package sun.tools.java;

/**
 * A single exception handler.  This class hangs off BinaryCode.
 */

public class BinaryExceptionHandler {
    public int startPC;		
    public int endPC;
    public int handlerPC;
    public ClassDeclaration exceptionClass;

    BinaryExceptionHandler(int start, int end, 
			   int handler, ClassDeclaration xclass) {
	startPC = start;
	endPC = end;
	handlerPC = handler;
	exceptionClass = xclass;
    }
}
