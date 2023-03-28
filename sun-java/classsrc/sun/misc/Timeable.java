/*
 * @(#)Timeable.java	1.4 95/09/06 Patrick Chan
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

package sun.misc;

/**
 * This interface is used by the Timer class.  A class that uses
 * Timer objects must implement this interface.
 * 
 * @see Timer
 * @author Patrick Chan
 */
public interface Timeable {
    /** 
     * This method is executed every time a timer owned by this
     * object ticks.  An object can own more than one timer but
     * all timers call this method when they tick;
     * you can use the supplied timer parameter to determine
     * which timer has ticked.
     * @param timer a handle to the timer that has just ticked.
     */
    public void tick(Timer timer);
}
