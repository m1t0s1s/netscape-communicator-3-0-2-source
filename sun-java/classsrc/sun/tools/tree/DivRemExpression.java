/*
 * @(#)DivRemExpression.java	1.1 95/10/06 Arthur van Hoff
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
import java.util.Hashtable;

abstract public
class DivRemExpression extends BinaryArithmeticExpression {
    /**
     * constructor
     */
    public DivRemExpression(int op, int where, Expression left, Expression right) {
	super(op, where, left, right);
    }
  
    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
        // Do not toss out integer divisions or remainders since they 
        // can cause a division by zero.
        if (type.inMask(TM_INTEGER)) { 
	    right = right.inlineValue(env, ctx);
	    if (right.isConstant() && !right.equals(0)) { 
	        // We know the division can be elided
	        left = left.inline(env, ctx);
		return left;
	    } else { 
	        left = left.inlineValue(env, ctx);
		try {
		    return eval().simplify();
		} catch (ArithmeticException e) {
		    env.error(where, "arithmetic.exception");
		    return this;
		}
	    }
	} else { 
   	    // float & double divisions don't cause arithmetic errors
	    return super.inline(env, ctx);
	}
    }
}
