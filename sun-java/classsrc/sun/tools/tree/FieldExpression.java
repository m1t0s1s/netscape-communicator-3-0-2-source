/*
 * @(#)FieldExpression.java	1.30 95/11/29 Arthur van Hoff
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
class FieldExpression extends UnaryExpression {
    Identifier id;
    FieldDefinition field;

    boolean parsed = false;

    /**
     * constructor
     */
    public FieldExpression(int where, Expression right, Identifier id) {
	super(FIELD, where, Type.tError, right);
	this.id = id;
    }
    public FieldExpression(int where, Context ctx, FieldDefinition field) {
	super(FIELD, where, field.getType(),
	      field.isStatic() ? null : new ThisExpression(where, ctx));
	this.id = field.getName();
	this.field = field;
    }

    /**
     * Convert an '.' expression to a qualified identifier
     */
    Identifier toIdentifier() {
	StringBuffer buf = new StringBuffer();
	FieldExpression e = this;
	buf.append('.');
	buf.append(id);
	while (e.right.op == FIELD) {
	    e = (FieldExpression)e.right;
	    buf.insert(0, e.id);
	    buf.insert(0, '.');
	}
	if (e.right.op != IDENT) {
	    return null;
	}
	buf.insert(0, ((IdentifierExpression)e.right).id);
	return Identifier.lookup(buf.toString());
    }

    /**
     * Convert an '.' expression to a type
     */
    Type toType(Environment env, Context ctx) {
	Identifier id = toIdentifier();
	if (id == null) {
	    env.error(where, "invalid.type.expr");
	    return Type.tError;
	}
	return Type.tClass(id);
    }


    // called on the "right" part of MethodExpression's and FieldExpression's.
    // It's return value will either be:
    //    an Expression (possibly the same as the original) to put into
    //         the right slot of the caller
    //    null, indicating that there was no reasonable parse of "right" as
    //         a type name or as a variable access.  In this case, the inner
    //         most part of right must be an IdentifierExpression, and
    //         errorName[0] will be set to that IdentifierExpression.
    static Expression 
    mungeFieldRight(Expression right, Environment env, Context ctx, 
		    IdentifierExpression errorName[]) 
                 throws ClassNotFound, AmbiguousField {	
	switch (right.op) { 
	    case IDENT: {
		Identifier ID = ((IdentifierExpression)right).id;
		if (ctx.getField(env, ID) != null) {
		    // if this is a local field, there's nothing more to do.
		    return right;	// we're done
		} else { 
		    // Can "right" be interpreted as a type?
		    Type type = right.toType(env, ctx);
		    Identifier fullID = type.getClassName();
		    // Is it a real type??
		    if (env.classExists(fullID)) {
			return new TypeExpression(right.where, type);
		    } else { 
			errorName[0] = ((IdentifierExpression)right);
			return null;
		    } 
		}
	    }

	    case FIELD: {
		FieldExpression rightF = (FieldExpression)right;
		// recursively look at right.right
		Expression mungeRightRight = 
		    mungeFieldRight(rightF.right, env, ctx, errorName);
		if (mungeRightRight != null) {
		    // we figured out what right.right meant.  So this must
		    // just be a field access
		    rightF.right = mungeRightRight;
		    return right;
		}
		// Nope.  Is this field expression a type?
		Identifier ID = rightF.toIdentifier();
		if ((ID != null) && env.classExists(ID)) {
		    Type type = right.toType(env, ctx);
		    return new TypeExpression(right.where, type);
		}
		return null;
	    }

	    default:
		// Anything but an IDENT or a FIELD must be some sort of
		// expression, and can't possibly be a type.
		return right;
	}
    }

    /**
     * Check the expression
     */

    public long checkValue(Environment env, Context ctx, 
			   long vset, Hashtable exp) {
	try {
	    boolean staticRef = false;
	    IdentifierExpression errorName[] = new IdentifierExpression[1];
	    Expression mungeRight = 
		   mungeFieldRight(right, env, ctx, errorName);

	    if (mungeRight == null) {
		env.error(errorName[0].where, "undef.var", errorName[0].id);
		return vset;
	    } else { 
		right = mungeRight;
		if (mungeRight instanceof TypeExpression) { 
		    staticRef = true;
		} else {
		    vset = right.checkValue(env, ctx, vset, exp);
		}
	    }

	    if (!right.type.isType(TC_CLASS)) {
		if (right.type.isType(TC_ARRAY) && id.equals(idLength)) {
		    type = Type.tInt;
		    return vset;
		}
		if (!right.type.isType(TC_ERROR)) {
		    env.error(where, "invalid.field.reference", 
			      id, right.type);
		}
		return vset;
	    }

	    ClassDefinition clazz = env.getClassDefinition(right.type);
	    field = clazz.getVariable(env, id);

	    if (field == null) {
		if ((field = clazz.findAnyMethod(env, id)) != null) {
		    env.error(where, "invalid.field", id, field.getClassDeclaration());
		} else {
		    env.error(where, "no.such.field", id, clazz);
		}
		return vset;
	    }

	    ClassDefinition sourceClass = ctx.field.getClassDefinition();
	    type = field.getType();
	    if (!sourceClass.canAccess(env, field)) {
		env.error(where, "no.field.access", id, clazz, ctx.field.getClassDeclaration());
		return vset;
	    }
	    if (staticRef && !field.isStatic()) {
		// if the current method isn't static, and it's making what
		// seems to be a static reference to a superclass's dynamic 
		// instance variable, it really wants a dynamic reference.
		ClassDefinition fc = field.getClassDefinition();
		if (fc.implementedBy(env, ctx.field.getClassDeclaration())
		       && !ctx.field.isStatic()) {
		    right = new ThisExpression(right.where);
		    vset = right.checkValue(env, ctx, vset, exp);
		    staticRef = false;
		} else {
		    env.error(where, "no.static.field.access", id, clazz);
		    return vset;
		}
	    }

	    if (field.isProtected() 
		 && !(right instanceof SuperExpression)
		 && !sourceClass.protectedAccess(env, field, right.type)){
		env.error(where, "invalid.protected.field.use", 
			  field.getName(), field.getClassDeclaration(), 
			  right.type);
		return vset;
	    }

	    if ((!field.isStatic()) && (right.op == THIS) && ((vset & 1) == 0)) {
		env.error(where, "access.inst.before.super", id);
	    }

	    ctx.field.getClassDefinition().addDependency(field.getClassDeclaration());
	} catch (ClassNotFound e) {
	    env.error(where, "class.not.found", e.name, ctx.field);

	} catch (AmbiguousField e) {
	    env.error(where, "ambig.field", id, e.field1.getClassDeclaration(), e.field2.getClassDeclaration());
	}
	return vset;
    }


    /**
     * Check the expression if it appears on the LHS of an assignment
     */
    public long checkLHS(Environment env, Context ctx, 
			 long vset, Hashtable exp) {
	checkValue(env, ctx, vset, exp);
	if (field == null) { 
	    // an error has already been reported by checkValue
	} else if (right.type.isType(TC_ARRAY) && id.equals(idLength)) {
	    env.error(where, "invalid.lhs.assignment");
	} else if (field.isFinal()) {
	    env.error(where, "assign.to.final", id);
	}
	return vset;
    }

    /**
     * Check the expression if it appears on the LHS of an op= expression 
     */
    public long checkAssignOp(Environment env, Context ctx, 
			      long vset, Hashtable exp, Expression outside) {
	checkValue(env, ctx, vset, exp);
	if (field == null) { 
	    // an error has already been reported by checkValue
	} else if (right.type.isType(TC_ARRAY) && id.equals(idLength)) {
	    // this will generate an appropriate error message
	    super.checkAssignOp(env, ctx, vset, exp, outside);
        } else if (field.isFinal()) {
	    env.error(where, "assign.to.final", id);
	}
	return vset;
    }



    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	return (right != null) ? right.inline(env, ctx) : null;
    }
    public Expression inlineValue(Environment env, Context ctx) {
	try {
	    if (field == null) {
		if (id.equals(idLength)) {
		    return new LengthExpression(where, right).inlineValue(env, ctx);
		}
		return this;
	    }

	    if ((right != null) && (right.op == TYPE)) {
		right = null;
	    }

	    if (field.isFinal()) {
		Expression e = (Expression)field.getValue(env);
		if ((e != null) && e.isConstant()) {
		    return new CommaExpression(where, right, e).inlineValue(env, ctx);
		}
	    }

	    if (right != null) {
		if (field.isStatic()) {
		    Expression e = right.inline(env, ctx);
		    right = null;
		    if (e != null) {
			return new CommaExpression(where, e, this);
		    }
		} else {
		    right = right.inlineValue(env, ctx);
		}
	    }
	    return this;

	} catch (ClassNotFound e) {
	    throw new CompilerError(e);
	}
    }
    public Expression inlineLHS(Environment env, Context ctx) {
	if ((right != null) && (right.op == TYPE)) {
	    right = null;
	}
	if (right != null) {
	    if (field.isStatic()) {
		Expression e = right.inline(env, ctx);
		right = null;
		if (e != null) {
		    return new CommaExpression(where, e, this);
		}
	    } else {
		right = right.inlineValue(env, ctx);
	    }
	}
	return this;
    }

    /**
     * The cost of inlining this expression
     */
    public int costInline(int thresh) {
	return 3 + ((right != null) ? right.costInline(thresh) : 0);
    }

    /**
     * Code
     */
    int codeLValue(Environment env, Context ctx, Assembler asm) {
	if (field.isStatic()) {
	    if (right != null) {
		right.code(env, ctx, asm);
		return 1;
	    }
	    return 0;
	}
	right.codeValue(env, ctx, asm);
	return 1;
    }
    void codeLoad(Environment env, Context ctx, Assembler asm) {
	if (field == null) {
	    throw new CompilerError("should not be null");
	}
	if (field.isStatic()) {
	    asm.add(where, opc_getstatic, field);
	} else {
	    asm.add(where, opc_getfield, field);
	}
    }
    void codeStore(Environment env, Context ctx, Assembler asm) {
	if (field.isStatic()) {
	    asm.add(where, opc_putstatic, field);
	} else {
	    asm.add(where, opc_putfield, field);
	}
    }

    public void codeValue(Environment env, Context ctx, Assembler asm) {
	codeLValue(env, ctx, asm);
	codeLoad(env, ctx, asm);
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(");
	if (right != null) {
	    right.print(out);
	} else {
	    out.print("<empty>");
	}
	out.print("." + id + ")");
    }
}
