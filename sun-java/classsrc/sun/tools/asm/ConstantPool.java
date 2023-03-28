/*
 * @(#)ConstantPool.java	1.11 95/09/14 Arthur van Hoff
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
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Vector;
import java.io.IOException;
import java.io.DataOutputStream;

/**
 * A table of constants
 */
public final
class ConstantPool implements RuntimeConstants {
    Hashtable hash = new Hashtable(101);

    /**
     * Find an entry, may return 0
     */
    public int index(Object obj) {
	return ((ConstantPoolData)hash.get(obj)).index;
    }

    /**
     * Add an entry
     */
    public void put(Object obj) {
	ConstantPoolData data = (ConstantPoolData)hash.get(obj);
	if (data == null) {
	    if (obj instanceof String) {
		data = new StringConstantData(this, (String)obj);
	    } else if (obj instanceof StringExpression) {
		data = new StringExpressionConstantData(this, (StringExpression)obj);
	    } else if (obj instanceof ClassDeclaration) {
		data = new ClassConstantData(this, (ClassDeclaration)obj);
	    } else if (obj instanceof Type) {
		data = new ClassConstantData(this, (Type)obj);
	    } else if (obj instanceof FieldDefinition) {
		data = new FieldConstantData(this, (FieldDefinition)obj);
	    } else if (obj instanceof NameAndTypeData) {
		data = new NameAndTypeConstantData(this, (NameAndTypeData)obj);
	    } else if (obj instanceof Number) {
		data = new NumberConstantData(this, (Number)obj);
	    }
	    hash.put(obj, data);
	} 
    }

    /**
     * Write to output
     */
    public void write(Environment env, DataOutputStream out) throws IOException {
	ConstantPoolData list[] = new ConstantPoolData[hash.size()];
	int index = 1, count = 0;

	// Assign an index to each constant pool item
	for (int n = 0 ; n < 5 ; n++) {
	    for (Enumeration e = hash.elements() ; e.hasMoreElements() ;) {
		ConstantPoolData data = (ConstantPoolData)e.nextElement();
		if (data.order() == n) {
		    list[count++] = data;
		    data.index = index;
		    index += data.width();
		}
	    }
	}

	// Write length
	out.writeShort(index);
	
	// Write each constant pool item
	for (int n = 0 ; n < count ; n++) {
	    list[n].write(env, out, this);
	}
    }
}

