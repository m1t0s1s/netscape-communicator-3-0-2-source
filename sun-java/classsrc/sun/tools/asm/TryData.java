/*
 * @(#)TryData.java	1.9 95/09/14 Arthur van Hoff
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
import java.util.Vector;

public final
class TryData {
    Vector catches = new Vector();
    Label endLabel = new Label();
    
    /**
     * Add a label
     */
    public CatchData add(Object type) {
	CatchData cd = new CatchData(type);
	catches.addElement(cd);
	return cd;
    }

    /**
     * Get a label
     */
    public CatchData getCatch(int n) {
	return (CatchData)catches.elementAt(n);
    }

    /**
     * Get the default label
     */
    public Label getEndLabel() {
	return endLabel;
    }
}
