/*
 * @(#)AssignExpression.java	1.13 95/10/02 Arthur van Hoff
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
import java.io.PrintStream;
import java.util.Hashtable;

public
class AssignExpression extends BinaryAssignExpression {
    /**
     * Constructor
     */
    public AssignExpression(int where, Expression left, Expression right) {
	super(ASSIGN, where, left, right);
    }

    /**
     * Check an assignment expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = left.checkLHS(env, ctx, vset, exp);
	vset = right.checkValue(env, ctx, vset, exp);
	type = left.type;
	right = convert(env, ctx, type, right);
	return vset;
    }

    /**
     * The cost of inlining this expression
     */
    public int costInline(int thresh) {
	return 2 + super.costInline(thresh);
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	int depth = left.codeLValue(env, ctx, asm);
	right.codeValue(env, ctx, asm);
	codeDup(env, ctx, asm, right.type.stackSize(), depth);
	left.codeStore(env, ctx, asm);
    }
    public void code(Environment env, Context ctx, Assembler asm) {
	left.codeLValue(env, ctx, asm);
	right.codeValue(env, ctx, asm);
	left.codeStore(env, ctx, asm);
    }
}
