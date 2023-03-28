/*
 * @(#)NewArrayExpression.java	1.21 95/11/16 Arthur van Hoff
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
import sun.tools.asm.ArrayData;
import java.io.PrintStream;
import java.util.Hashtable;

public 
class NewArrayExpression extends NaryExpression {
    /**
     * Constructor
     */
    public NewArrayExpression(int where, Expression right, Expression args[]) {
	super(NEWARRAY, where, Type.tError, right, args);
    }

    /**
     * Check
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	type = right.toType(env, ctx);

	boolean flag = false;
	for (int i = 0 ; i < args.length ; i++) {
	    Expression dim = args[i];
	    if (dim == null) {
		if (i == 0) {
		    env.error(where, "array.dim.missing");
		}
		flag = true;
	    } else {
		if (flag) {
		    env.error(dim.where, "invalid.array.dim");
		}
		vset = dim.checkValue(env, ctx, vset, exp);
		args[i] = convert(env, ctx, Type.tInt, dim);
	    }
	    type = Type.tArray(type);
	}
	return vset;
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	Expression e = null;
	for (int i = 0 ; i < args.length ; i++) {
	    if (args[i] != null) {
		e = (e != null) ? new CommaExpression(where, e, args[i]) : args[i];
	    }
	}
	return (e != null) ? e.inline(env, ctx) : null;
    }
    public Expression inlineValue(Environment env, Context ctx) {
	for (int i = 0 ; i < args.length ; i++) {
	    if (args[i] != null) {
		args[i] = args[i].inlineValue(env, ctx);
	    }
	}
	return this;
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	int t = 0;
	for (int i = 0 ; i < args.length ; i++) {
	    if (args[i] != null) {
		args[i].codeValue(env, ctx, asm);
		t++;
	    }
	}
	if (args.length > 1) {
	    asm.add(where, opc_multianewarray, new ArrayData(type, t));
	    return;
	}

	switch (type.getElementType().getTypeCode()) {
	    case TC_BOOLEAN: 	
		asm.add(where, opc_newarray, new Integer(T_BOOLEAN)); 	break;
	    case TC_BYTE: 	
		asm.add(where, opc_newarray, new Integer(T_BYTE)); 	break;
	    case TC_SHORT:
		asm.add(where, opc_newarray, new Integer(T_SHORT)); 	break;
	    case TC_CHAR:
		asm.add(where, opc_newarray, new Integer(T_CHAR)); 	break;
	    case TC_INT:
 		asm.add(where, opc_newarray, new Integer(T_INT)); 	break;
	    case TC_LONG:
		asm.add(where, opc_newarray, new Integer(T_LONG)); 	break;
	    case TC_FLOAT:
		asm.add(where, opc_newarray, new Integer(T_FLOAT)); 	break;
	    case TC_DOUBLE:
		asm.add(where, opc_newarray, new Integer(T_DOUBLE)); 	break;
	    case TC_ARRAY:
		asm.add(where, opc_anewarray, type.getElementType());   break;
	    case TC_CLASS:
		asm.add(where, opc_anewarray, 
			env.getClassDeclaration(type.getElementType()));
		break;
	    default:
		throw new CompilerError("codeValue");
	}
    }    
}
