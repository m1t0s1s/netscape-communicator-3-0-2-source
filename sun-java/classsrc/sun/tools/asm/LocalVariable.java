/*
 * @(#)LocalVariable.java	1.3 95/09/14 Arthur van Hoff
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
 * This class is used to assemble the local variables in the local
 * variable table.
 *
 * @author Arthur van Hoff
 * @version 	1.3, 14 Sep 1995
 */
public final
class LocalVariable {
    FieldDefinition field;
    int slot;
    int from;
    int to;

    public LocalVariable(FieldDefinition field, int slot) {
	if (field == null) {
	    new Exception().printStackTrace();
	}
	this.field = field;
	this.slot = slot;
	to = -1;
    }

    LocalVariable(FieldDefinition field, int slot, int from, int to) {
	this.field = field;
	this.slot = slot;
	this.from = from;
	this.to = to;
    }

    public String toString() {
	return field + "/" + slot;
    }
}
