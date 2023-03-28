/*
 * @(#)BooleanExpression.java	1.15 95/11/12 Arthur van Hoff
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
import sun.tools.asm.Label;
import java.io.PrintStream;
import java.util.Hashtable;

public
class BooleanExpression extends ConstantExpression {
    boolean value;
    
    /**
     * Constructor
     */
    public BooleanExpression(int where, boolean value) {
	super(BOOLEANVAL, where, Type.tBoolean);
	this.value = value;
    }

    /**
     * Get the value
     */
    public Object getValue() {
	return new Integer(value ? 1 : 0);
    }

    /**
     * Check if the expression is equal to a value
     */
    public boolean equals(boolean b) {
	return value == b;
    }

    
    /**
     * Check if the expression is equal to its default static value
     */
    public boolean equalsDefault() {
        return !value;
    }


    /*
     * Check a "not" expression.
     * 
     * cvars is modified so that 
     *    cvar.vsTrue indicates variables with a known value if 
     *         the expression is true.
     *    cvars.vsFalse indicates variables with a known value if
     *         the expression is false
     * 
     * For constant expressions, set the side that corresponds to our
     * already known value to vset.  Set the side that corresponds to the
     * other way to "impossible"
     */

    public void checkCondition(Environment env, Context ctx, 
			       long vset, Hashtable exp, ConditionVars cvars) {
	if (value) { 
	    cvars.vsFalse = -1; 
	    cvars.vsTrue = vset;
	} else { 
	    cvars.vsFalse = vset;
	    cvars.vsTrue = -1;
	}
    }


    /**
     * Code
     */
    void codeBranch(Environment env, Context ctx, Assembler asm, Label lbl, boolean whenTrue) {
	if (value == whenTrue) {
	    asm.add(where, opc_goto, lbl);
	}
    }
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_ldc, new Integer(value ? 1 : 0));
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print(value ? "true" : "false");
    }
}
