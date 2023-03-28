/*
 * @(#)BinaryCode.java	1.8 95/09/14 Frank Yellin
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




package sun.tools.java;

import sun.tools.javac.*;
import java.io.*;
import java.util.*;

public class BinaryCode implements Constants {
    int maxStack;		// maximum stack used by code
    int maxLocals;		// maximum locals used by code
    BinaryExceptionHandler exceptionHandlers[];
    BinaryAttribute atts;	// code attributes
    BinaryConstantPool cpool;	// constant pool of the class
    byte code[];		// the byte code

    /**
     * Construct the binary code from the code attribute
     */

    BinaryCode(byte data[], BinaryConstantPool cpool, Environment env) {
	DataInputStream in = new DataInputStream(new ByteArrayInputStream(data));
	try {
	    this.cpool = cpool;
	    this.maxStack = in.readShort();
	    this.maxLocals = in.readShort();
	    int code_length = in.readInt();
	    this.code = new byte[code_length];
	    in.read(this.code);
	    int exception_count = in.readShort();
	    this.exceptionHandlers = new BinaryExceptionHandler[exception_count];
	    for (int i = 0; i < exception_count; i++) {
		int start = in.readShort();
		int end = in.readShort();
		int handler = in.readShort();
		ClassDeclaration xclass = cpool.getDeclaration(env, in.readShort());
		this.exceptionHandlers[i]  = 
		    new BinaryExceptionHandler(start, end, handler, xclass);
	    }
	    this.atts = BinaryAttribute.load(in, cpool, ~0);
	    if (in.available() != 0) {
		System.err.println("Should have exhausted input stream!");
	    }
	} catch (IOException e) {
	    throw new CompilerError(e);
	}
    }
    

    /**
     * Accessors
     */

    public BinaryExceptionHandler getExceptionHandlers()[] {
	return exceptionHandlers;
    }

    public byte getCode()[] { return code; }

    public int getMaxStack() { return maxStack; }

    public int getMaxLocals() { return maxLocals; }

    /**
     * Load a binary class
     */
    public static
    BinaryCode load(BinaryField bf, BinaryConstantPool cpool, Environment env) {
	byte code[] = bf.getAttribute(idCode);
	return (code != null) ? new BinaryCode(code, cpool, env) : null;
    }
}
    



