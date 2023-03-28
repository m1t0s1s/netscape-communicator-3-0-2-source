/*
 * @(#)BinaryLogicalExpression.java	1.13 95/11/12 Arthur van Hoff
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
import java.util.Hashtable;

abstract public
class BinaryLogicalExpression extends BinaryExpression {
    /**
     * constructor
     */
    public BinaryLogicalExpression(int op, int where, Expression left, Expression right) {
	super(op, where, Type.tBoolean, left, right);
    }

    /**
     * Check a binary expression
     */
    public long checkValue(Environment env, Context ctx, 
			   long vset, Hashtable exp) {
	ConditionVars cvars = new ConditionVars();
	// evaluate the logical expression, determining which variables are
	// set if the resulting value is true or false
	checkCondition(env, ctx, vset, exp, cvars);
	// return the intersection.
	return cvars.vsTrue & cvars.vsFalse;
    }

    /*
     * Every subclass of this class must define a genuine implementation
     * of this method.  It cannot inherit the method of Expression.
     */
    abstract
    public void checkCondition(Environment env, Context ctx, long vset, 
			       Hashtable exp, ConditionVars cvars);
}
