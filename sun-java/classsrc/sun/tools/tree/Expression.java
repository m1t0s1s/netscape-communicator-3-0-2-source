/*
 * @(#)Expression.java	1.35 95/11/26 Arthur van Hoff
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
import sun.tools.asm.Label;
import sun.tools.asm.Assembler;
import java.io.PrintStream;
import java.util.Hashtable;

public
class Expression extends Node {
    Type type;

    /**
     * Constructor
     */
    Expression(int op, int where, Type type) {
	super(op, where);
	this.type = type;
    }

    /**
     * Return the precedence of the operator
     */
    int precedence() {
	return (op < opPrecedence.length) ? opPrecedence[op] : 100;
    }

    /**
     * Order the expression based on precedence
     */
    public Expression order() {
	return this;
    }

    /**
     * Return true if constant
     */
    public boolean isConstant() {
        return false;
    }

    /**
     * Return the constant value
     */
    public Object getValue() {
        return null;
    }

    /**
     * Check if the expression is equal to a value
     */
    public boolean equals(int i) {
	return false;
    }
    public boolean equals(boolean b) {
	return false;
    }
    public boolean equals(Identifier id) {
	return false;
    }

    /**
     * Check if the expression is equal to its default static value
     */
    public boolean equalsDefault() {
        return false;
    }


    /**
     * Convert an expresion to a type
     */
    Type toType(Environment env, Context ctx) {
	env.error(where, "invalid.type.expr");
	return Type.tError;
    }

    /**
     * See if this expression fits in the given type.
     * This is useful because some larger number fit into
     * smaller types.
     */
    public boolean fitsType(Environment env, Type t) {
	try { 
	    return env.isMoreSpecific(this.type, t);
	} catch (ClassNotFound e) {
	    return false;
	}
    }

    /**
     * Check an expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	return vset;
    }
    public long checkInitializer(Environment env, Context ctx, long vset, Type t, Hashtable exp) {
	return checkValue(env, ctx, vset, exp);
    }
    public long check(Environment env, Context ctx, long vset, Hashtable exp) {
	throw new CompilerError("check failed");
    }

    public long checkLHS(Environment env, Context ctx, 
			    long vset, Hashtable exp) {
	env.error(where, "invalid.lhs.assignment");
	type = Type.tError;
	return vset;
    }

    public long checkAssignOp(Environment env, Context ctx, 
			      long vset, Hashtable exp, Expression outside) {
	if (outside instanceof IncDecExpression) 
	    env.error(where, "invalid.arg", opNames[outside.op]);
	else 
	    env.error(where, "invalid.lhs.assignment");
	type = Type.tError;
	return vset;
    }


    /**
     * Check a condition.  Return a ConditionVars(), which indicates when
     * which variables are set if the condition is true, and which are set if
     * the condition is false. 
     */
    public ConditionVars checkCondition(Environment env, Context ctx, 
					long vset, Hashtable exp) {
	ConditionVars cvars = new ConditionVars();
	checkCondition(env, ctx, vset, exp, cvars);
	return cvars;
    }

    /*
     * Check a condition.  
     * 
     * cvars is modified so that 
     *    cvar.vsTrue indicates variables with a known value if result = true
     *    cvars.vsFalse indicates variables with a known value if !result
     *
     * The default action is to simply call checkValue on the expression, and
     * to see both vsTrue and vsFalse to the result.
     */

    public void checkCondition(Environment env, Context ctx, 
			       long vset, Hashtable exp, ConditionVars cvars) {
	cvars.vsTrue = cvars.vsFalse = checkValue(env, ctx, vset, exp);
    }



    /**
     * Evaluate
     */
    Expression eval() {
	return this;
    }

    /**
     * Simplify
     */
    Expression simplify() {
	return this;
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	return null;
    }
    public Expression inlineValue(Environment env, Context ctx) {
	return this;
    }
    public Expression inlineLHS(Environment env, Context ctx) {
	return null;
    }

    /**
     * The cost of inlining this expression
     */
    public int costInline(int thresh) {
	return 1;
    }

    /**
     * Code
     */
    void codeBranch(Environment env, Context ctx, Assembler asm, Label lbl, boolean whenTrue) {
	if (type.isType(TC_BOOLEAN)) {
	    codeValue(env, ctx, asm);
	    asm.add(where, whenTrue ? opc_ifne : opc_ifeq, lbl);
	} else {
	    throw new CompilerError("codeBranch " + opNames[op]);
	}
    }
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	if (type.isType(TC_BOOLEAN)) {
	    Label l1 = new Label();
	    Label l2 = new Label();

	    codeBranch(env, ctx, asm, l1, true);
	    asm.add(where, opc_ldc, new Integer(0));
	    asm.add(where, opc_goto, l2);
	    asm.add(l1);
	    asm.add(where, opc_ldc, new Integer(1));
	    asm.add(l2);
	} else {
	    throw new CompilerError("codeValue");
	}
    }
    public void code(Environment env, Context ctx, Assembler asm) {
	codeValue(env, ctx, asm);

	switch (type.getTypeCode()) {
	  case TC_VOID:
	    break;

	  case TC_DOUBLE:
	  case TC_LONG:
	    asm.add(where, opc_pop2);
	    break;
	    
	  default:
	    asm.add(where, opc_pop);
	    break;
	}
    }
    int codeLValue(Environment env, Context ctx, Assembler asm) {
	print(System.out);
	throw new CompilerError("invalid lhs");
    }
    void codeLoad(Environment env, Context ctx, Assembler asm) {
	print(System.out);
	throw new CompilerError("invalid load");
    }
    void codeStore(Environment env, Context ctx, Assembler asm) {
	print(System.out);
	throw new CompilerError("invalid store");
    }
    void codeAppend(Environment env, Context ctx, Assembler asm, ClassDeclaration c) {
	try {
	    codeValue(env, ctx, asm);
	    Type argTypes[] = {type};
	    ClassDefinition sourceClass = ctx.field.getClassDefinition();
	    FieldDefinition f = c.getClassDefinition(env).matchMethod(env, sourceClass, idAppend, argTypes);
	    asm.add(where, opc_invokevirtual, f);
	} catch (ClassNotFound e) {
	    throw new CompilerError(e);
	} catch (AmbiguousField e) {
	    throw new CompilerError(e);
	}
    }

    /**
     * Code
     */
    void codeDup(Environment env, Context ctx, Assembler asm, int items, int depth) {
	switch (items) {
	  case 0:
	    return;
	    
	  case 1:
	    switch (depth) {
	      case 0:
		asm.add(where, opc_dup);
		return;
	      case 1:
		asm.add(where, opc_dup_x1);
		return;
	      case 2:
		asm.add(where, opc_dup_x2);
		return;
		
	    }
	    break;
	  case 2:
	    switch (depth) {
	      case 0:
		asm.add(where, opc_dup2);
		return;
	      case 1:
		asm.add(where, opc_dup2_x1);
		return;
	      case 2:
		asm.add(where, opc_dup2_x2);
		return;
		
	    }
	    break;
	}
 	throw new CompilerError("can't dup: " + items + ", " + depth);
    }

    void codeConversion(Environment env, Context ctx, Assembler asm, Type f, Type t) {
	int from = f.getTypeCode();
	int to = t.getTypeCode();

	switch (to) {
 	  case TC_BOOLEAN:
	    if (from != TC_BOOLEAN) {
		break;
	    }
	    return;
	  case TC_BYTE:
	    if (from != TC_BYTE) {
		codeConversion(env, ctx, asm, f, Type.tInt);
		asm.add(where, opc_int2byte);
	    }
	    return;
	  case TC_CHAR:
	    if (from != TC_CHAR) {
		codeConversion(env, ctx, asm, f, Type.tInt);
		asm.add(where, opc_int2char);
	    }
	    return;
	  case TC_SHORT:
	    if (from != TC_SHORT) {
		codeConversion(env, ctx, asm, f, Type.tInt);
		asm.add(where, opc_int2short);
	    }
	    return;
	  case TC_INT:
	    switch (from) {
	      case TC_BYTE:
	      case TC_CHAR:
	      case TC_SHORT:
	      case TC_INT:
		return;
	      case TC_LONG:
		asm.add(where, opc_l2i);
		return;
	      case TC_FLOAT:
		asm.add(where, opc_f2i);
		return;
	      case TC_DOUBLE:
		asm.add(where, opc_d2i);
		return;
	    }
	    break;
	  case TC_LONG:
	    switch (from) {
	      case TC_BYTE:
	      case TC_CHAR:
	      case TC_SHORT:
	      case TC_INT:
		asm.add(where, opc_i2l);
		return;
	      case TC_LONG:
		return;
	      case TC_FLOAT:
		asm.add(where, opc_f2l);
		return;
	      case TC_DOUBLE:
		asm.add(where, opc_d2l);
		return;
	    }
	    break;
	  case TC_FLOAT:
	    switch (from) {
	      case TC_BYTE:
	      case TC_CHAR:
	      case TC_SHORT:
	      case TC_INT:
		asm.add(where, opc_i2f);
		return;
	      case TC_LONG:
		asm.add(where, opc_l2f);
		return;
	      case TC_FLOAT:
		return;
	      case TC_DOUBLE:
		asm.add(where, opc_d2f);
		return;
	    }
	    break;
	  case TC_DOUBLE:
	    switch (from) {
	      case TC_BYTE:
	      case TC_CHAR:
	      case TC_SHORT:
	      case TC_INT:
		asm.add(where, opc_i2d);
		return;
	      case TC_LONG:
		asm.add(where, opc_l2d);
		return;
	      case TC_FLOAT:
		asm.add(where, opc_f2d);
		return;
	      case TC_DOUBLE:
		return;
	    }
	    break;

	  case TC_CLASS:
	    switch (from) {
	      case TC_NULL:
		return;
	      case TC_CLASS:
	      case TC_ARRAY:
		try {
		    if (!env.implicitCast(f, t)) {
			asm.add(where, opc_checkcast, env.getClassDeclaration(t));
		    }
		} catch (ClassNotFound e) {
		    throw new CompilerError(e);
		}
		return;
	    }
	    
	    break;
	    
	  case TC_ARRAY:
	    switch (from) {
	      case TC_NULL:
		return;
	      case TC_CLASS:
	      case TC_ARRAY:
		try {
		    if (!env.implicitCast(f, t)) {
			asm.add(where, opc_checkcast, t);
		    }
		    return;
		} catch (ClassNotFound e) {
		    throw new CompilerError(e);
		}
	    }
	    break;
	}
	throw new CompilerError("codeConversion: " + from + ", " + to);
    }

    /**
     * Check if the first thing is a constructor invocation
     */
    public boolean firstConstructor() {
	return false;
    }

    /**
     * Create a copy of the expression for method inlining
     */
    public Expression copyInline(Context ctx) {
	return (Expression)clone();
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print(opNames[op]);
    }
}
