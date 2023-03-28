/*
 * @(#)AssignAddExpression.java	1.12 95/09/14 Arthur van Hoff
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
class AssignAddExpression extends AssignOpExpression {
    /**
     * Constructor
     */
    public AssignAddExpression(int where, Expression left, Expression right) {
	super(ASGADD, where, left, right);
    }

    /**
     * Select the type
     */
    void selectType(Environment env, Context ctx, int tm) {
	if (left.type == Type.tString) {
	    type = Type.tString;
	    return;
	} else if ((tm & TM_DOUBLE) != 0) {
	    type = Type.tDouble;
	} else if ((tm & TM_FLOAT) != 0) {
	    type = Type.tFloat;
	} else if ((tm & TM_LONG) != 0) {
	    type = Type.tLong;
	} else {
	    type = Type.tInt;
	}
	right = convert(env, ctx, type, right);
    }

    /**
     * The cost of inlining this statement
     */
    public int costInline(int thresh) {
	return type.isType(TC_CLASS) ? 25 : super.costInline(thresh);
    }

    /**
     * Code
     */
    void code(Environment env, Context ctx, Assembler asm, boolean valNeeded) {
	if (type.isType(TC_CLASS)) {
	    int depth = left.codeLValue(env, ctx, asm);
	    codeDup(env, ctx, asm, depth, 0);
	    left.codeLoad(env, ctx, asm);

	    // new StringBuffer
	    ClassDeclaration c = env.getClassDeclaration(idJavaLangStringBuffer);
	    asm.add(where, opc_new, c);
	    asm.add(where, opc_dup);

	    try {
		ClassDefinition sourceClass = ctx.field.getClassDefinition();
		FieldDefinition f = c.getClassDefinition(env).matchMethod(env, sourceClass, idInit);
		asm.add(where, opc_invokenonvirtual, f);
		asm.add(where, opc_swap);

		// append(String)
		Type argTypes[] = {Type.tString};
		f = c.getClassDefinition(env).matchMethod(env, sourceClass, idAppend, argTypes);
		asm.add(where, opc_invokevirtual, f);

		// append right hand side
		right.codeAppend(env, ctx, asm, c);

		// toString()
		f = c.getClassDefinition(env).matchMethod(env, sourceClass, idToString);
		asm.add(where, opc_invokevirtual, f);

		// dup when needed
		if (valNeeded) {
		    codeDup(env, ctx, asm, type.stackSize(), depth);
		}

		// store
		left.codeStore(env, ctx, asm);
		return;

	    } catch (ClassNotFound e) {
		throw new CompilerError(e);
	    } catch (AmbiguousField e) {
		throw new CompilerError(e);
	    }
	}
	super.code(env, ctx, asm, valNeeded);
    }

    /**
     * Code
     */
    void codeOperation(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_iadd + type.getTypeCodeOffset());
    }
}
