/*
 * @(#)IdentifierExpression.java	1.25 95/12/03 Arthur van Hoff
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
import sun.tools.asm.LocalVariable;
import java.io.PrintStream;
import java.util.Hashtable;

public
class IdentifierExpression extends Expression {
    Identifier id;
    FieldDefinition field;
    
    /**
     * Constructor
     */
    public IdentifierExpression(int where, Identifier id) {
	super(IDENT, where, Type.tError);
	this.id = id;
    }

    /**
     * Check if the expression is equal to a value
     */
    public boolean equals(Identifier id) {
	return this.id.equals(id);
    }


    /**
     * Assign a value to this identifier.  [It must already be "bound"]
     */
    private long assign(Environment env, long vset) {
	if (field.isLocal()) {
	    LocalField local = (LocalField)field;
	    vset |= (long)1 << local.number;
	    local.writecount++;
	}
	if (field.isFinal()) {
	    env.error(where, "assign.to.final", id);
	}
	return vset;
    }

    /** 
     * Get the value of this identifier.  [ It must already be "bound"]
     */
    private long get(Environment env, long vset) {
	if (field.isLocal()) {
	    LocalField local = (LocalField)field;
	    if ((vset & ((long)1 << local.number)) == 0) {
		env.error(where, "var.not.initialized", id);
		vset |= (long)1 << local.number;
	    }
	    local.readcount++;
	} else if (!field.isStatic()) {
	    if ((vset & 1) == 0) {
		env.error(where, "access.inst.before.super", id);
	    }
	}
	return vset;
    }

    /**
     * Bind to a field
     */
    boolean bind(Environment env, Context ctx) {
	try {
	    field = ctx.getField(env, id);
	    if (field == null) {
		env.error(where, "undef.var", id);
		return false;
	    }

	    type = field.getType();

	    // Check access permission
	    if (!ctx.field.getClassDefinition().canAccess(env, field)) {
		env.error(where, "no.field.access", id, field.getClassDeclaration(), ctx.field.getClassDeclaration());
		return false;
	    }

	    // Check acces of static variables
	    if (!ctx.field.canAccess(env, field)) {
		env.error(where, "no.static.field.access", id, field.getClassDeclaration());
		return false;
	    }

	    // Since the reference to the field is implicitly this.field, it
	    // must be a reasonable reference to a protected field.  No need
	    // to check for invalid.protected.use
	    ;


	    // Check forward reference
	    if (!ctx.field.canReach(env, field)) {
		env.error(where, "forward.ref", id, field.getClassDeclaration());
		return false;
	    }
	    return true;
	} catch (ClassNotFound e) {
	    env.error(where, "class.not.found", e.name, ctx.field);
	} catch (AmbiguousField e) {
	    env.error(where, "ambig.field", id, e.field1.getClassDeclaration(), e.field2.getClassDeclaration());
	}
	return false;
    }

    /**
     * Check expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	if (bind(env, ctx)) {
	    vset = get(env, vset);
	    ctx.field.getClassDefinition().addDependency(field.getClassDeclaration());
	}
	return vset;
    }

    /**
     * Check the expression if it appears on the LHS of an assignment
     */
    public long checkLHS(Environment env, Context ctx, 
			 long vset, Hashtable exp) {
	if (bind(env, ctx)) 
	    return assign(env, vset);
	else 
	    return vset;
    }

    /**
     * Check the expression if it appears on the LHS of an op= expression 
     */
    public long checkAssignOp(Environment env, Context ctx, 
			      long vset, Hashtable exp, Expression outside) {
	if (bind(env, ctx)) 
	    return assign(env, get(env, vset));
	else 
	    return vset;
    }

    /**
     * Convert an identifier to a type
     */
    Type toType(Environment env, Context ctx) {
	try {
	    return Type.tClass(ctx.field.resolve(env, id));
	} catch (AmbiguousClass ee) {
	    env.error(where, "ambig.class", ee.name1, ee.name2);
	} catch (ClassNotFound ee) {
	}
	ClassDefinition currentClass = ctx.field.getClassDefinition();
	Identifier pkg = currentClass.getName().getQualifier();
	Identifier fullID = ((pkg == null || pkg == idNull)) 
	                            ? id : Identifier.lookup(pkg, id);
	return Type.tClass(fullID);
    }

    /**
     * Inline
     */
    public Expression inline(Environment env, Context ctx) {
	return null;
    }
    public Expression inlineValue(Environment env, Context ctx) {
	if (field == null) {
	    return this;
	}
	try {
	    if (field.isLocal()) {
		if (field.isFinal()) {
		    Expression e = (Expression)field.getValue(env);
		    return (e != null) ? e : this;
		}
		return this;
	    }
	    return new FieldExpression(where, ctx, field).inlineValue(env, ctx);
	} catch (ClassNotFound e) {
	    throw new CompilerError(e);
	}
    }
    public Expression inlineLHS(Environment env, Context ctx) {
	if (field.isLocal()) {
	    return this;
	}
	return new FieldExpression(where, ctx, field).inlineValue(env, ctx);
    }

    /**
     * Code
     */
    int codeLValue(Environment env, Context ctx, Assembler asm) {
	return 0;
    }
    void codeLoad(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_iload + type.getTypeCodeOffset(),
		new Integer(((LocalField)field).number));
    }
    void codeStore(Environment env, Context ctx, Assembler asm) {
	LocalField local = (LocalField)field;
	asm.add(where, opc_istore + type.getTypeCodeOffset(),
		new LocalVariable(local, local.number));
    }
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	codeLValue(env, ctx, asm);
	codeLoad(env, ctx, asm);
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print(id + "#" + ((field != null) ? field.hashCode() : 0));
    }
}
