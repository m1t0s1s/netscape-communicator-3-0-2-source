/*
 * @(#)ClassConstantData.java	1.10 95/09/14 Arthur van Hoff
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
 * This is a class constant pool item.
 */
final
class ClassConstantData extends ConstantPoolData {
    String name;

    /**
     * Constructor
     */

    ClassConstantData(ConstantPool tab, ClassDeclaration clazz) {
	name = clazz.getName().toString().replace('.', '/');
	tab.put(name);
    }

    // REMIND: this case should eventually go away.
    ClassConstantData(ConstantPool tab, Type t) {
	name = t.getTypeSignature();
	tab.put(name);
    }

    /**
     * Write the constant to the output stream
     */
    void write(Environment env, DataOutputStream out, ConstantPool tab) throws IOException {
	out.writeByte(CONSTANT_CLASS);
	out.writeShort(tab.index(name));
    }

    /**
     * Return the order of the constant
     */
    int order() {
	return 1;
    }

    public String toString() { 
	return "ClassConstantData[" + name + "]";
    }
}
