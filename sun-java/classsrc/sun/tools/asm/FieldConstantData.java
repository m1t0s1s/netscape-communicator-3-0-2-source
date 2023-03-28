/*
 * @(#)FieldConstantData.java	1.8 95/09/14 Arthur van Hoff
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
 * This is a field constant pool data item
 */
final
class FieldConstantData extends ConstantPoolData {
    FieldDefinition field;
    NameAndTypeData nt;

    /**
     * Constructor
     */
    FieldConstantData(ConstantPool tab, FieldDefinition field) {
	this.field = field;
	nt = new NameAndTypeData(field);
	tab.put(field.getClassDeclaration());
	tab.put(nt);
    }

    /**
     * Write the constant to the output stream
     */
    void write(Environment env, DataOutputStream out, ConstantPool tab) throws IOException {
	if (field.isMethod()) {
	    if (field.getClassDefinition().isInterface()) {
		out.writeByte(CONSTANT_INTERFACEMETHOD);
	    } else {
		out.writeByte(CONSTANT_METHOD);
	    }
	} else {
	    out.writeByte(CONSTANT_FIELD);
	}
	out.writeShort(tab.index(field.getClassDeclaration()));
	out.writeShort(tab.index(nt));
    }

    /**
     * Return the order of the constant
     */
    int order() {
	return 2;
    }
}
