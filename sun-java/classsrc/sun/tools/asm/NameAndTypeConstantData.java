/*
 * @(#)NameAndTypeConstantData.java	1.8 95/09/14 Arthur van Hoff
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
 * This is a name and type constant pool data item
 */
final
class NameAndTypeConstantData extends ConstantPoolData {
    String name;
    String type;

    /**
     * Constructor
     */
    NameAndTypeConstantData(ConstantPool tab, NameAndTypeData nt) {
	name = nt.field.getName().toString();
	type = nt.field.getType().getTypeSignature();
	tab.put(name);
	tab.put(type);
    }

    /**
     * Write the constant to the output stream
     */
    void write(Environment env, DataOutputStream out, ConstantPool tab) throws IOException {
	out.writeByte(CONSTANT_NAMEANDTYPE);
	out.writeShort(tab.index(name));
	out.writeShort(tab.index(type));
    }

    /**
     * Return the order of the constant
     */
    int order() {
	return 3;
    }
}
