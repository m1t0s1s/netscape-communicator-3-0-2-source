/*
 * @(#)BitNotExpression.java	1.11 95/09/14 Arthur van Hoff
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
class BitNotExpression extends UnaryExpression {
    /**
     * Constructor
     */
    public BitNotExpression(int where, Expression right) {
	super(BITNOT, where, right.type, right);
    }
 
    /**
     * Select the type of the expression
     */
    void selectType(Environment env, Context ctx, int tm) {
	if ((tm & TM_LONG) != 0) {
	    type = Type.tLong;
	} else {
	    type = Type.tInt;
	} 
	right = convert(env, ctx, type, right);
    }

    /**
     * Evaluate
     */
    Expression eval(int a) {
	return new IntExpression(where, ~a);
    }
    Expression eval(long a) {
	return new LongExpression(where, ~a);
    }

    /**
     * Simplify
     */
    Expression simplify() {
	if (right.op == BITNOT) {
	    return ((BitNotExpression)right).right;
	}
	return this;
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	right.codeValue(env, ctx, asm);
	if (type.isType(TC_INT)) {
	    asm.add(where, opc_ldc, new Integer(-1));
	    asm.add(where, opc_ixor);
	} else {
	    asm.add(where, opc_ldc2_w, new Long(-1));
	    asm.add(where, opc_lxor);
	}
    }
}
