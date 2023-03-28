/*
 * @(#)AddExpression.java	1.23 95/10/11 Arthur van Hoff
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
class AddExpression extends BinaryArithmeticExpression {
    /**
     * constructor
     */
    public AddExpression(int where, Expression left, Expression right) {
	super(ADD, where, left, right);
    }

    /**
     * Select the type
     */
    void selectType(Environment env, Context ctx, int tm) {
	if ((left.type == Type.tString) && !right.type.isType(TC_VOID)) {
	    type = Type.tString;
	    return;
	} else if ((right.type == Type.tString) && !left.type.isType(TC_VOID)) {
	    type = Type.tString;
	    return;
	}
	super.selectType(env, ctx, tm);
    }

    /**
     * Evaluate
     */
    Expression eval(int a, int b) {
	return new IntExpression(where, a + b);
    }
    Expression eval(long a, long b) {
	return new LongExpression(where, a + b);
    }
    Expression eval(float a, float b) {
	return new FloatExpression(where, a + b);
    }
    Expression eval(double a, double b) {
	return new DoubleExpression(where, a + b);
    }
    Expression eval(String a, String b) {
	return new StringExpression(where, a + b);
    }

    /**
     * Simplify
     */
    Expression simplify() {
        if (!type.isType(TC_CLASS)) {
	    // Can't simplify floating point add because of -0.0 strangeness
	    if (type.inMask(TM_INTEGER)) { 
		if (left.equals(0)) {
		    return right;
		}
		if (right.equals(0)) {
		    return left;
		}
	    }
	} else if (right.type.isType(TC_NULL)) {
	    right = new StringExpression(right.where, "null");
	} else if (left.type.isType(TC_NULL)) {
	    left = new StringExpression(right.where, "null");
	}
	return this;
    }

    /**
     * The cost of inlining this expression
     */
    public int costInline(int thresh) {
	return (type.isType(TC_CLASS) ? 12 : 1) +
	    left.costInline(thresh) + right.costInline(thresh);
    }

    /**
     * Code
     */
    void codeOperation(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_iadd + type.getTypeCodeOffset());
    }
    void codeAppend(Environment env, Context ctx, Assembler asm, ClassDeclaration c) {
	if (type.isType(TC_CLASS)) {
	    left.codeAppend(env, ctx, asm, c);
	    right.codeAppend(env, ctx, asm, c);
	} else {
	    super.codeAppend(env, ctx, asm, c);
	}
    }
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	if (type.isType(TC_CLASS)) {
	    ClassDeclaration c = env.getClassDeclaration(idJavaLangStringBuffer);
	    asm.add(where, opc_new, c);
	    asm.add(where, opc_dup);

	    try {
		ClassDefinition sourceClass = ctx.field.getClassDefinition();
		FieldDefinition f = c.getClassDefinition(env).matchMethod(env, sourceClass, idInit);
		asm.add(where, opc_invokenonvirtual, f);
		codeAppend(env, ctx, asm, c);

		f = c.getClassDefinition(env).matchMethod(env, sourceClass, idToString);
		asm.add(where, opc_invokevirtual, f);
	    } catch (ClassNotFound e) {
		throw new CompilerError(e);
	    } catch (AmbiguousField e) {
		throw new CompilerError(e);
	    }
	} else {
	    super.codeValue(env, ctx, asm);
	}
    }
}
