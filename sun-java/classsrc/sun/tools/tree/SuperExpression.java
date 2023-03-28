/*
 * @(#)SuperExpression.java	1.13 95/09/14 Arthur van Hoff
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
class SuperExpression extends Expression {
    LocalField field;

    /**
     * Constructor
     */
    public SuperExpression(int where) {
	super(SUPER, where, Type.tObject);
    }

    /**
     * Check expression
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	ClassDeclaration superClass = ctx.field.getClassDefinition().getSuperClass();
	if (ctx.field.isStatic() || ctx.field.isVariable() || (superClass == null)) {
	    env.error(where, "undef.var", idSuper);
	    type = Type.tError;
	    return vset;
	}
	if ((vset & 1) == 0) {
	    env.error(where, "access.inst.before.super", idSuper);
	    type = Type.tError;
	    return vset;
	}
	type = superClass.getType();
	field = ctx.getLocalField(idThis);
	field.readcount++;
	return vset;
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	asm.add(where, opc_aload, new Integer(field.number));
    }
}
