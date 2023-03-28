/*
 * @(#)TypeExpression.java	1.12 95/11/26 Arthur van Hoff
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
import java.util.Hashtable;

public
class TypeExpression extends Expression {
    /**
     * Constructor
     */
    public TypeExpression(int where, Type type) {
	super(TYPE, where, type);
    }


    /**
     * Convert to a type
     */
    Type toType(Environment env, Context ctx) {
	return type;
    }

    /**
     * Check an expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	env.error(where, "invalid.term");
	type = Type.tError;
	return vset;
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print(type.toString());
    }
}
