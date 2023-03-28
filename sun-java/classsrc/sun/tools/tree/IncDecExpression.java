/*
 * @(#)IncDecExpression.java	1.17 95/10/03 Arthur van Hoff
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

public
class IncDecExpression extends UnaryExpression {
    /**
     * Constructor
     */
    public IncDecExpression(int op, int where, Expression right) {
	super(op, where, right.type, right);
    }

    /**
     * Check an increment or decrement expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	vset = right.checkAssignOp(env, ctx, vset, exp, this);
	if (right.type.inMask(TM_NUMBER)) {
	    type = right.type;
	} else {
	    if (!right.type.isType(TC_ERROR)) {
		env.error(where, "invalid.arg.type", right.type, opNames[op]);
	    }
	    type = Type.tError;
	}
	return vset;
    }
 
    /**
     * Check void expression
     */
    public long check(Environment env, Context ctx, long vset, Hashtable exp) {
	return checkValue(env, ctx, vset, exp);
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	return inlineValue(env, ctx);
    }
    public Expression inlineValue(Environment env, Context ctx) {
	right = right.inlineValue(env, ctx);
	return this;
    }

    /**
     * Code
     */
    void codeIncDec(Environment env, Context ctx, Assembler asm, boolean inc, boolean prefix, boolean valNeeded) {
	if ((right.op == IDENT) && type.isType(TC_INT) &&
	    (((IdentifierExpression)right).field.isLocal())) {
	    if (valNeeded && !prefix) {
		right.codeLoad(env, ctx, asm);
	    }
	    int v = ((LocalField)((IdentifierExpression)right).field).number;
	    int[] operands = { v, inc ? 1 : -1 };
	    asm.add(where, opc_iinc, operands);
	    if (valNeeded && prefix) {
		right.codeLoad(env, ctx, asm);
	    }
	    return;
	    
	}
	int depth = right.codeLValue(env, ctx, asm);
	codeDup(env, ctx, asm, depth, 0);
	right.codeLoad(env, ctx, asm);
	if (valNeeded && !prefix) {
	    codeDup(env, ctx, asm, type.stackSize(), depth);
	}
	switch (type.getTypeCode()) {
	  case TC_BYTE:
	    asm.add(where, opc_ldc, new Integer(1));
	    asm.add(where, inc ? opc_iadd : opc_isub);
	    asm.add(where, opc_int2byte);
	    break;
	  case TC_SHORT:
	    asm.add(where, opc_ldc, new Integer(1));
	    asm.add(where, inc ? opc_iadd : opc_isub);
	    asm.add(where, opc_int2short);
	    break;
	  case TC_CHAR:
	    asm.add(where, opc_ldc, new Integer(1));
	    asm.add(where, inc ? opc_iadd : opc_isub);
	    asm.add(where, opc_int2char);
	    break;
	  case TC_INT:
	    asm.add(where, opc_ldc, new Integer(1));
	    asm.add(where, inc ? opc_iadd : opc_isub);
	    break;
	  case TC_LONG:
	    asm.add(where, opc_ldc2_w, new Long(1));
	    asm.add(where, inc ? opc_ladd : opc_lsub);
	    break;
	  case TC_FLOAT:
	    asm.add(where, opc_ldc, new Float(1));
	    asm.add(where, inc ? opc_fadd : opc_fsub);
	    break;
	  case TC_DOUBLE:
	    asm.add(where, opc_ldc2_w, new Double(1));
	    asm.add(where, inc ? opc_dadd : opc_dsub);
	    break;
	  default:
	    throw new CompilerError("invalid type");
	}
	if (valNeeded && prefix) {
	    codeDup(env, ctx, asm, type.stackSize(), depth);
	}
	right.codeStore(env, ctx, asm);
    }
}
