/*
 * @(#)SwitchData.java	1.7 95/09/14 Arthur van Hoff
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
import java.util.Hashtable;

public final
class SwitchData {
    int minValue, maxValue;
    Label defaultLabel = new Label();
    Hashtable tab = new Hashtable();

    /**
     * Get a label
     */
    public Label get(int n) {
	return (Label)tab.get(new Integer(n));
    }
    
    /**
     * Add a label
     */
    public void add(int n, Label lbl) {
	if (tab.size() == 0) {
	    minValue = n;
	    maxValue = n;
	} else {
	    if (n < minValue) {
		minValue = n;
	    }
	    if (n > maxValue) {
		maxValue = n;
	    }
	}
	tab.put(new Integer(n), lbl);
    }

    /**
     * Get the default label
     */
    public Label getDefaultLabel() {
	return defaultLabel;
    }
}
