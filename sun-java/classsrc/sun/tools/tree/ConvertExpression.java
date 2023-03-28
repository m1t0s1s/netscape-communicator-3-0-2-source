/*
 * @(#)ConvertExpression.java	1.12 95/09/14 Arthur van Hoff
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
import java.io.PrintStream;
import java.util.Hashtable;

public
class ConvertExpression extends UnaryExpression {
    /**
     * Constructor
     */
    public ConvertExpression(int where, Type type, Expression right) {
	super(CONVERT, where, type, right);
    }

    /**
     * Check the value
     */
    public long checkValue(Environment env, Context ctx, long vset, Hashtable exp) {
	return right.checkValue(env, ctx, vset, exp);
    }
    
    /**
     * Simplify
     */
    Expression simplify() {
	switch (right.op) {
	  case BYTEVAL:
	  case CHARVAL:
	  case SHORTVAL:
	  case INTVAL: {
	    int value = ((IntegerExpression)right).value;
	    switch (type.getTypeCode()) {
	      case TC_BYTE: 	return new ByteExpression(right.where, (byte)value);
	      case TC_CHAR: 	return new CharExpression(right.where, (char)value);
	      case TC_SHORT: 	return new ShortExpression(right.where, (short)value);
	      case TC_INT: 	return new IntExpression(right.where, (int)value);
	      case TC_LONG: 	return new LongExpression(right.where, (long)value);
	      case TC_FLOAT: 	return new FloatExpression(right.where, (float)value);
	      case TC_DOUBLE: 	return new DoubleExpression(right.where, (double)value);
	    }
	    break;
	  }
	  case LONGVAL: {
	    long value = ((LongExpression)right).value;
	    switch (type.getTypeCode()) {
	      case TC_BYTE: 	return new ByteExpression(right.where, (byte)value);
	      case TC_CHAR: 	return new CharExpression(right.where, (char)value);
	      case TC_SHORT: 	return new ShortExpression(right.where, (short)value);
	      case TC_INT: 	return new IntExpression(right.where, (int)value);
	      case TC_FLOAT: 	return new FloatExpression(right.where, (float)value);
	      case TC_DOUBLE: 	return new DoubleExpression(right.where, (double)value);
	    }
	    break;
	  }
	  case FLOATVAL: {
	    float value = ((FloatExpression)right).value;
	    switch (type.getTypeCode()) {
	      case TC_BYTE: 	return new ByteExpression(right.where, (byte)value);
	      case TC_CHAR: 	return new CharExpression(right.where, (char)value);
	      case TC_SHORT: 	return new ShortExpression(right.where, (short)value);
	      case TC_INT: 	return new IntExpression(right.where, (int)value);
	      case TC_LONG: 	return new LongExpression(right.where, (long)value);
	      case TC_DOUBLE: 	return new DoubleExpression(right.where, (double)value);
	    }
	    break;
	  }
	  case DOUBLEVAL: {
	    double value = ((DoubleExpression)right).value;
	    switch (type.getTypeCode()) {
	      case TC_BYTE: 	return new ByteExpression(right.where, (byte)value);
	      case TC_CHAR: 	return new CharExpression(right.where, (char)value);
	      case TC_SHORT: 	return new ShortExpression(right.where, (short)value);
	      case TC_INT: 	return new IntExpression(right.where, (int)value);
	      case TC_LONG: 	return new LongExpression(right.where, (long)value);
	      case TC_FLOAT: 	return new FloatExpression(right.where, (float)value);
	    }
	    break;
	  }
	}
	return this;
    }
 
    /**
     * Check if the expression is equal to a value
     */
    public boolean equals(int i) {
	return right.equals(i);
    }
    public boolean equals(boolean b) {
	return right.equals(b);
    }

    /**
     * Code
     */
    public void codeValue(Environment env, Context ctx, Assembler asm) {
	right.codeValue(env, ctx, asm);
	codeConversion(env, ctx, asm, right.type, type);
    }

    /**
     * Print
     */
    public void print(PrintStream out) {
	out.print("(" + opNames[op] + " " + type.toString() + " ");
	right.print(out);
	out.print(")");
    }
}
