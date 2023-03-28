/*
 * @(#)LocalVariableTable.java	1.6 95/09/14 Arthur van Hoff
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

package sun.tools.asm;

import sun.tools.java.*;
import java.io.IOException;
import java.io.DataOutputStream;

/**
 * This class is used to assemble the local variable table.
 *
 * @author Arthur van Hoff
 * @version 	1.6, 14 Sep 1995
 */
final
class LocalVariableTable {
    LocalVariable locals[] = new LocalVariable[8];
    int len;

    /**
     * Define a new local variable. Merge entries where possible.
     */
    void define(FieldDefinition field, int slot, int from, int to) {
	if (from >= to) {
	    return;
	}
	for (int i = 0 ; i < len ; i++) {
	    if ((locals[i].field == field) && (locals[i].slot == slot) &&
		(from <= locals[i].to) && (to >= locals[i].from)) {
		locals[i].from = Math.min(locals[i].from, from);
		locals[i].to = Math.max(locals[i].to, to);
		return;
	    }
	}
	if (len == locals.length) {
	    LocalVariable newlocals[] = new LocalVariable[len * 2];
	    System.arraycopy(locals, 0, newlocals, 0, len);
	    locals = newlocals;
	}
	locals[len++] = new LocalVariable(field, slot, from, to);
    }

    /**
     * Write out the data.
     */
    void write(Environment env, DataOutputStream out, ConstantPool tab) throws IOException {
	out.writeShort(len);
	for (int i = 0 ; i < len ; i++) {
	    //System.out.println("pc=" + locals[i].from + ", len=" + (locals[i].to - locals[i].from) + ", nm=" + locals[i].field.getName() + ", slot=" + locals[i].slot);
	    out.writeShort(locals[i].from);
	    out.writeShort(locals[i].to - locals[i].from);
	    out.writeShort(tab.index(locals[i].field.getName().toString()));
	    out.writeShort(tab.index(locals[i].field.getType().getTypeSignature()));
	    out.writeShort(locals[i].slot);
	}
    }
}
