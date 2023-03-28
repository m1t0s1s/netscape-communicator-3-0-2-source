/*
 * @(#)BinaryAttribute.java	1.12 95/09/14 Arthur van Hoff
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

package sun.tools.java;

import java.io.IOException;
import java.io.DataInputStream;
import java.io.DataOutputStream;

/**
 * This class is used to represent an attribute from a binary class.
 * This class should go away once arrays are objects.
 */
public final
class BinaryAttribute implements Constants {
    Identifier name;
    byte data[];
    BinaryAttribute next;

    /**
     * Constructor
     */
    BinaryAttribute(Identifier name, byte data[], BinaryAttribute next) {
	this.name = name;
	this.data = data;
	this.next = next;
    }

    /**
     * Load a list of attributes
     */
    static BinaryAttribute load(DataInputStream in, BinaryConstantPool cpool, int mask) throws IOException {
	BinaryAttribute atts = null;
	int natt = in.readShort();
	
	for (int i = 0 ; i < natt ; i++) {
	    Identifier id = cpool.getIdentifier(in.readShort());
	    int len = in.readInt();

	    if (id.equals(idCode) && ((mask & ATT_CODE) == 0)) {
		in.skipBytes(len);
	    } else {
		byte data[] = new byte[len];
		in.readFully(data);
		atts = new BinaryAttribute(id, data, atts);
	    }
	}
	return atts;
    }

    // write out the Binary attributes to the given stream
    // (note that attributes may be null)
    static void write(BinaryAttribute attributes, DataOutputStream out, 
		      BinaryConstantPool cpool, Environment env) throws IOException {
	// count the number of attributes
	int attributeCount = 0;
	for (BinaryAttribute att = attributes; att != null; att = att.next)
	    attributeCount++;
	out.writeShort(attributeCount);
	
	// write out each attribute
	for (BinaryAttribute att = attributes; att != null; att = att.next) {
	    Identifier name = att.name;
	    byte data[] = att.data;
	    // write the identifier
	    out.writeShort(cpool.indexString(name.toString(), env));
	    // write the length
	    out.writeInt(data.length);
	    // write the data
	    out.write(data, 0, data.length);
	}
    }
}
