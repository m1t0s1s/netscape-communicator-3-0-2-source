/*
 * @(#)NameAndTypeData.java	1.7 95/09/14 Arthur van Hoff
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

/**
 * An object to represent a name and type constant pool data item.
 */
final
class NameAndTypeData {
    FieldDefinition field;

    /**
     * Constructor
     */
    NameAndTypeData(FieldDefinition field) {
	this.field = field;
    }

    /**
     * Hashcode
     */
    public int hashCode() {
	return field.getName().hashCode() * field.getType().hashCode();
    }

    /**
     * Equality
     */
    public boolean equals(Object obj) {
	if ((obj != null) && (obj instanceof NameAndTypeData)) {
	    NameAndTypeData nt = (NameAndTypeData)obj;
	    return field.getName().equals(nt.field.getName()) &&
		field.getType().equals(nt.field.getType());
	}
	return false;
    }

    /**
     * Convert to string
     */
    public String toString() {
	return "%%" + field.toString() + "%%";
    }
}
