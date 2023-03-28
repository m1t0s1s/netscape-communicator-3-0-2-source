/*
 * @(#)Node.java	1.23 95/12/01 Arthur van Hoff
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

package sun.tools.tree;

import sun.tools.java.*;
import java.io.PrintStream;
import java.io.ByteArrayOutputStream;

public
class Node implements Constants, Cloneable {
    int op;
    int where;

    /**
     * Constructor
     */
    Node(int op, int where) {
	this.op = op;
	this.where = where;
    }

    /**
     * Get the operator
     */
    public int getOp() {
	return op;
    }

    /**
     * Get where 
     */
    public int getWhere() {
	return where;
    }

    /**
     * Implicit conversions
     */
    public Expression convert(Environment env, Context ctx, Type t, Expression e) {
	if (e.type.isType(TC_ERROR) || t.isType(TC_ERROR)) {
	    // An error was already reported
	    return e;
	}

	if (e.type.equals(t)) {
	    // The types are already the same
	    return e;
	}

	try {
	    if (e.fitsType(env, t)) { 
		return new ConvertExpression(where, t, e);
	    }

	    if (env.explicitCast(e.type, t)) {
		env.error(where, "explicit.cast.needed", opNames[op], e.type, t);
		return new ConvertExpression(where, t, e);
	    }
	} catch (ClassNotFound ee) {
	    env.error(where, "class.not.found", ee.name, opNames[op]);
	}

	// The cast is not allowed
	env.error(where, "incompatible.type", opNames[op], e.type, t);
	return new ConvertExpression(where, Type.tError, e);
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	throw new CompilerError("print");
    }

    /**
     * Clone this object.
     */
    public Object clone() {
	try { 
	    return super.clone();
	} catch (CloneNotSupportedException e) {
	    // this shouldn't happen, since we are Cloneable
	    throw new InternalError();
	}
    }

    /*
     * Useful for simple debugging
     */
    public String toString() { 
	ByteArrayOutputStream bos = new ByteArrayOutputStream();
	print(new PrintStream(bos));
	return bos.toString();
    }

}
