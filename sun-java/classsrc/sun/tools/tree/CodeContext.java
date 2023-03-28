/*
 * @(#)CodeContext.java	1.11 95/11/15 Arthur van Hoff
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
import sun.tools.asm.Label;

class CodeContext extends Context {
    Label breakLabel;
    Label contLabel;

    /**
     * Create a new nested context, for a block statement
     */
    CodeContext(Context ctx, Node node) {
	super(ctx, node);
	switch (node.op) {
	  case DO:
	  case WHILE:
	  case FOR:
	  case FINALLY:
	  case SYNCHRONIZED:
	    this.breakLabel = new Label();
	    this.contLabel = new Label();
	    break;
	  case SWITCH:
	  case TRY:
	  case INLINEMETHOD:
	  case INLINENEWINSTANCE:
	    this.breakLabel = new Label();
	    break;
	  default:
	    if ((node instanceof Statement) && (((Statement)node).labels != null)) {
		this.breakLabel = new Label();
	    }
	}
    }
}
