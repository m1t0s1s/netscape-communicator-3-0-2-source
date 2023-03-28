/*
 * @(#)Context.java	1.15 95/11/15 Arthur van Hoff
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

public
class Context implements Constants {
    Context prev;
    Node node;
    int varNumber;
    LocalField locals;
    FieldDefinition field;

    /**
     * Create the initial context for a method
     */
    public Context(FieldDefinition field) {
	this.field = field;
    }

    /**
     * Create a new nested context, for a block statement
     */
    Context(Context ctx, Node node) {
	this.prev = ctx;
	this.node = node;
	this.locals = ctx.locals;
	this.varNumber = ctx.varNumber;
	this.field = ctx.field;
    }

    /**
     * Declare local
     */
    public int declare(Environment env, LocalField local) {
	//System.out.println(	"DECLARE= " + local.getName() + "=" + varNumber + ", read=" + local.readcount + ", write=" + local.writecount + ", hash=" + local.hashCode());
	local.subModifiers(M_FINAL);
	local.setValue(null);
	local.prev = locals;
	locals = local;
	local.number = varNumber;
	varNumber += local.getType().stackSize();
	return local.number;
    }

    /**
     * Get a local variable by name
     */
    LocalField getLocalField(Identifier name) {
	for (LocalField f = locals ; f != null ; f = f.prev) {
	    if (name.equals(f.getName())) {
		return f;
	    }
	}
	return null;
    }

    /**
     * Get either a local variable, or a field in the current class
     */
    FieldDefinition getField(Environment env, Identifier name) throws AmbiguousField, ClassNotFound {
	FieldDefinition f = getLocalField(name);
	if (f == null) {
	    f = field.getClassDefinition().getVariable(env, name);
	}
	return f;
    }


    /**
     * Get the context that corresponds to a label, return null if
     * not found.
     */
    Context getLabelContext(Identifier lbl) {
	for (Context ctx = this ; ctx != null ; ctx = ctx.prev) {
	    if ((ctx.node != null) && (ctx.node instanceof Statement)) { 
		if (((Statement)(ctx.node)).hasLabel(lbl)) 
		    return ctx;
	    }
	}
	return null;
    }
    
    /**
     * Get the destination context of a break
     */
    Context getBreakContext(Identifier lbl) {
	if (lbl != null) {
	    return getLabelContext(lbl);
	} 
	for (Context ctx = this ; ctx != null ; ctx = ctx.prev) {
	    if (ctx.node != null) {
		switch (ctx.node.op) {
		  case SWITCH:
		  case FOR:
		  case DO:
		  case WHILE:
		    return ctx;
		}
	    }
	}
	return null;
    }

    /**
     * Get the destination context of a continue
     */
    Context getContinueContext(Identifier lbl) {
	if (lbl != null) {
	    return getLabelContext(lbl);
	} 
	for (Context ctx = this ; ctx != null ; ctx = ctx.prev) {
	    if (ctx.node != null) {
		switch (ctx.node.op) {
		  case FOR:
		  case DO:
		  case WHILE:
		    return ctx;
		}
	    }
	}
	return null;
    }

    /**
     * Get the nearest inlined context
     */
    Context getInlineContext() {
	for (Context ctx = this ; ctx != null ; ctx = ctx.prev) {
	    if (ctx.node != null) {
		switch (ctx.node.op) {
		  case INLINEMETHOD:
		  case INLINENEWINSTANCE:
		    return ctx;
		}
	    }
	}
	return null;
    }

    /**
     * Get the context of a field that is being inlined
     */
    Context getInlineFieldContext(FieldDefinition field) {
	for (Context ctx = this ; ctx != null ; ctx = ctx.prev) {
	    if (ctx.node != null) {
		switch (ctx.node.op) {
		  case INLINEMETHOD:
		    if (((InlineMethodExpression)ctx.node).field.equals(field)) {
			return ctx;
		    }
		    break;
		  case INLINENEWINSTANCE:
		    if (((InlineNewInstanceExpression)ctx.node).field.equals(field)) {
			return ctx;
		    }
		}
	    }
	}
	return null;
    }

    /**
     * Remove variables from the vset set  that are no longer part of 
     * this context.
     */
    long removeAdditionalVars(long vset) {
        return vset & (((1L << varNumber) - 1) | (1L << 63));
    }
}
