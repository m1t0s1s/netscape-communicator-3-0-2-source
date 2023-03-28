/*
 * @(#)AppletThreadGroup.java	1.1 95/07/21  
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

package sun.applet;

import java.lang.SecurityManager;

/**
 * This class defines an applet thread group.
 *
 * @version 	1.1, 21 Jul 1995
 * @author 	Arthur van Hoff
 */
class AppletThreadGroup extends ThreadGroup {
    /**
     * Constructs a thread group for an applet.
     */
    AppletThreadGroup(String name) {
	super(name);
	SecurityManager.setScopePermission();
	setMaxPriority(Thread.NORM_PRIORITY + 1);
    }
}
