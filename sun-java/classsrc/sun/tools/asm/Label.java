/*
 * @(#)Label.java	1.11 95/10/24 Arthur van Hoff
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

import sun.tools.java.FieldDefinition;
import java.io.OutputStream;

/**
 * A label instruction. This is a 0 size instruction.
 * It is the only valid target of a branch instruction.
 */
public final
class Label extends Instruction {
    static int labelCount = 0;
    int ID;
    int depth;
    FieldDefinition locals[];

    /**
     * Constructor
     */
    public Label() {
	super(0, opc_label, null);
	this.ID = ++labelCount;
    }

    /**
     * Get the final destination, eliminate jumps gotos, and jumps to 
     * labels that are immediatly folowed by another label. The depth
     * field is used to leave bread crums to avoid infinite loops.
     */
    Label getDestination() {
	Label lbl = this;
	if ((next != null) && (next != this) && (depth == 0)) {
	    depth = 1;

	    switch (next.opc) {
	      case opc_label:
		lbl = ((Label)next).getDestination();
		break;

	      case opc_goto:
		lbl = ((Label)next.value).getDestination();
		break;

	      case opc_ldc:
	      case opc_ldc_w:
		if (next.value instanceof Integer) {
		    Instruction inst = next.next;
		    if (inst.opc == opc_label) {
			inst = ((Label)inst).getDestination().next;
		    }

		    if (inst.opc == opc_ifeq) {
			if (((Integer)next.value).intValue() == 0) {
			    lbl = (Label)inst.value;
			} else {
			    lbl = new Label();
			    lbl.next = inst.next;
			    inst.next = lbl;
			}
			lbl = lbl.getDestination();
			break;
		    }
		    if (inst.opc == opc_ifne) {
			if (((Integer)next.value).intValue() == 0) {
			    lbl = new Label();
			    lbl.next = inst.next;
			    inst.next = lbl;
			} else {
			    lbl = (Label)inst.value;
			}
			lbl = lbl.getDestination();
			break;
		    }
		}
		break;
	    }
	    depth = 0;
	}
	return lbl;
    }

    public String toString() { 
	String s = "$" + ID + ":";
	if (value != null)
	    s = s + " stack=" + value;
	return s;
    }
}
