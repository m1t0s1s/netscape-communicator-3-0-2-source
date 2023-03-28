/*
 * @(#)ArrayExpression.java	1.21 95/09/14 Arthur van Hoff
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
import sun.tools.asm.*;
import java.io.PrintStream;
import java.util.Hashtable;

public
class ArrayExpression extends NaryExpression {
    /**
     * Constructor
     */
    public ArrayExpression(int where, Expression args[]) {
	super(ARRAY, where, Type.tError, null, args);
    }

    /**
     * Check expression type
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	env.error(where, "invalid.array.expr");
	return vset;
    }
    public long checkInitializer(Environment env, Context ctx, long vset, Type t, Hashtable exp) {
	if (!t.isType(TC_ARRAY)) {
	    if (!t.isType(TC_ERROR)) {
		env.error(where, "invalid.array.init", t);
	    }
	    return vset;
	}
	type = t;
	t = t.getElementType();
	for (int i = 0 ; i < args.length ; i++) {
	    vset = args[i].checkInitializer(env, ctx, vset, t, exp);
	    args[i] = convert(env, ctx, t, args[i]);
	}
	return vset;
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	Expression e = null;
	for (int i = 0 ; i < args.length ; i++) {
	    args[i] = args[i].inline(env, ctx);
	    if (args[i] != null) {
		e = (e == null) ? args[i] : new CommaExpression(where, e, args[i]);
	    }
	}
	return e;
    }
    public Expression inlineValue(Environment env, Context ctx) {
	for (int i = 0 ; i < args.length ; i++) {
	    args[i] = args[i].inlineValue(env, ctx);
	}
	return this;
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	int t = 0;
	asm.add(where, opc_ldc, new Integer(args.length));
	switch (type.getElementType().getTypeCode()) {
	  case TC_BOOLEAN: 	asm.add(where, opc_newarray, new Integer(T_BYTE)); 	break;
	  case TC_BYTE: 	asm.add(where, opc_newarray, new Integer(T_BYTE)); 	break;
	  case TC_SHORT: 	asm.add(where, opc_newarray, new Integer(T_SHORT)); 	break;
	  case TC_CHAR: 	asm.add(where, opc_newarray, new Integer(T_CHAR)); 	break;
	  case TC_INT: 		asm.add(where, opc_newarray, new Integer(T_INT)); 	break;
	  case TC_LONG: 	asm.add(where, opc_newarray, new Integer(T_LONG)); 	break;
	  case TC_FLOAT: 	asm.add(where, opc_newarray, new Integer(T_FLOAT)); 	break;
	  case TC_DOUBLE: 	asm.add(where, opc_newarray, new Integer(T_DOUBLE)); 	break;

	  case TC_ARRAY:
	    asm.add(where, opc_anewarray, type.getElementType());
	    break;

	  case TC_CLASS:
	    asm.add(where, opc_anewarray, env.getClassDeclaration(type.getElementType()));
	    break;

	  default:
	    throw new CompilerError("codeValue");
	}

	for (int i = 0 ; i < args.length ; i++) {
	    asm.add(where, opc_dup);
	    asm.add(where, opc_ldc, new Integer(i));
	    args[i].codeValue(env, ctx, asm);
	    switch (type.getElementType().getTypeCode()) {
	      case TC_BOOLEAN:
	      case TC_BYTE:
		asm.add(where, opc_bastore);
		break;
	      case TC_CHAR:
		asm.add(where, opc_castore);
		break;
	      case TC_SHORT:
		asm.add(where, opc_sastore);
		break;
	      default:
		asm.add(where, opc_iastore + type.getElementType().getTypeCodeOffset());
	    }
	}
    }
}
