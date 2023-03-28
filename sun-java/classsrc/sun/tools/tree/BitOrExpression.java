/*
 * @(#)BitOrExpression.java	1.10 95/09/14 Arthur van Hoff
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
import sun.tools.asm.Assembler;

public
class BitOrExpression extends BinaryBitExpression {
    /**
     * constructor
     */
    public BitOrExpression(int where, Expression left, Expression right) {
	super(BITOR, where, left, right);
    }

    /**
     * Evaluate
     */
    Expression eval(boolean a, boolean b) {
	return new BooleanExpression(where, a | b);
    }
    Expression eval(int a, int b) {
	return new IntExpression(where, a | b);
    }
    Expression eval(long a, long b) {
	return new LongExpression(where, a | b);
    }

    /**
     * Simplify
     */
    Expression simplify() {
	if (left.equals(false) || left.equals(0))
	    return right;
	if (right.equals(false) || right.equals(0))
	    return left;
	if (left.equals(true))
	    return new CommaExpression(where, right, left).simplify();
	if (right.equals(true)) 
	    return new CommaExpression(where, left, right).simplify();
	return this;
    }

    /**
     * Code
     */
    void codeOperation(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_ior + type.getTypeCodeOffset());
    }
}
