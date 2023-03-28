/*
 * @(#)StringExpressionConstantData.java	1.9 95/09/14 Arthur van Hoff
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
import sun.tools.tree.StringExpression;
import java.io.IOException;
import java.io.DataOutputStream;

/**
 * This is a string expression constant. This constant
 * represents an Java string constant.
 */
final
class StringExpressionConstantData extends ConstantPoolData {
    StringExpression str;

    /**
     * Constructor
     */
    StringExpressionConstantData(ConstantPool tab, StringExpression str) {
	this.str = str;
	tab.put(str.getValue());
    }

    /**
     * Write the constant to the output stream
     */
    void write(Environment env, DataOutputStream out, ConstantPool tab) throws IOException {
	out.writeByte(CONSTANT_STRING);
	out.writeShort(tab.index(str.getValue()));
    }

    /**
     * Return the order of the constant
     */
    int order() {
	return 0;
    }

    /**
     * toString
     */
    public String toString() {
	return "StringExpressionConstantData[" + str.getValue() + "]=" + str.getValue().hashCode();
    }
}
