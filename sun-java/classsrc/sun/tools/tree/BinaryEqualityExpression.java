/*
 * @(#)BinaryEqualityExpression.java	1.11 95/09/14 Arthur van Hoff
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

public
class BinaryEqualityExpression extends BinaryExpression {
    /**
     * constructor
     */
    public BinaryEqualityExpression(int op, int where, Expression left, Expression right) {
	super(op, where, Type.tBoolean, left, right);
    }

    /**
     * Select the type
     */
    void selectType(Environment env, Context ctx, int tm) {
	Type t = Type.tInt;
	if (tm == (TM_ARRAY | TM_NULL)) {
	    return;
	}
	if (tm == (TM_CLASS | TM_NULL)) {
	    return;
	}
	if (tm == TM_ARRAY) {
	    t = Type.tObject;    
	} else if ((tm & TM_CLASS) != 0) {
	    t = Type.tObject;
	} else if ((tm & TM_DOUBLE) != 0) {
	    t = Type.tDouble;
	} else if ((tm & TM_FLOAT) != 0) {
	    t = Type.tFloat;
	} else if ((tm & TM_LONG) != 0) {
	    t = Type.tLong;
	} else if ((tm & TM_BOOLEAN) != 0) {
	    t = Type.tBoolean;
	} else {
	    t = Type.tInt;
	} 
	if (tm == (TM_CLASS | TM_ARRAY)) {
	    if ((left.type == Type.tObject) || (right.type == Type.tObject)) {
		return;
	    }
	    env.error(where, "incompatible.type", left.type, left.type, right.type);
	    return;
	}
	if ((tm == TM_CLASS)  || (tm == TM_ARRAY)) {
	    try {
		if (env.explicitCast(left.type, right.type) ||
		    env.explicitCast(right.type, left.type)) {
		    return;
		}
		env.error(where, "incompatible.type", left.type, left.type, right.type);
	    } catch (ClassNotFound e) {
		env.error(where, "class.not.found", e.name, opNames[op]);
	    }
	    return;
	}
	left = convert(env, ctx, t, left);
	right = convert(env, ctx, t, right);
    }
}
