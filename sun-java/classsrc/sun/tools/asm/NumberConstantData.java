/*
 * @(#)NumberConstantData.java	1.8 95/09/14 Arthur van Hoff
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
 * A numeric constant pool item. Can either be integer, float, long or double.
 */
final
class NumberConstantData extends ConstantPoolData {
    Number num;

    /**
     * Constructor
     */
    NumberConstantData(ConstantPool tab, Number num) {
	this.num = num;
    }

    /**
     * Write the constant to the output stream
     */
    void write(Environment env, DataOutputStream out, ConstantPool tab) throws IOException {
	if (num instanceof Integer) {
	    out.writeByte(CONSTANT_INTEGER);
	    out.writeInt(num.intValue());
	} else if (num instanceof Long) {
	    out.writeByte(CONSTANT_LONG);
	    out.writeLong(num.longValue());
	} else if (num instanceof Float) {
	    out.writeByte(CONSTANT_FLOAT);
	    out.writeFloat(num.floatValue());
	} else if (num instanceof Double) {
	    out.writeByte(CONSTANT_DOUBLE);
	    out.writeDouble(num.doubleValue());
	}
    }
    /**
     * Return the order of the constant
     */
    int order() {
	return (width() == 1) ? 0 : 3;
    }

    /**
     * Return the number of entries that it takes up in the constant pool
     */
    int width() {
	return ((num instanceof Double) || (num instanceof Long)) ? 2 : 1;
    }
}
