/*
 * @(#)WListPeer.java	1.11 95/12/01 Sami Shaio
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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
package sun.awt.windows;

import java.awt.*;
import java.awt.peer.*;

class WListPeer extends WComponentPeer implements ListPeer {
    native void create(WComponentPeer parent);
    //native void createWidget(WComponentPeer parent);

    void initialize() {
		List li = (List)target;

		/* set whether this list should allow multiple selections. */
		setMultipleSelections(li.allowsMultipleSelections());

        // workaround for the multiple list box switching on the fly
        //Container parent = target.getParent();
        //createWidget((WComponentPeer)((parent != null) ? parent.getPeer() : null));

		/* add any items that were already inserted in the target. */
		int  nitems = li.countItems();
		for (int i = 0; i < nitems; i++) {
			addItem(li.getItem(i), -1);
		}

		/* make the visible position visible. */
		int index = li.getVisibleIndex();
		if (index >= 0) {
			makeVisible(index);
		}

		/* select the item if necessary. */
		int sel[] = li.getSelectedIndexes();
		for (int i = 0 ; i < sel.length ; i++) {
			select(sel[i]);
		}

		super.initialize();
    }

    WListPeer(List target) {
		super(target);
    }

	// WComponent overrides
    public Dimension minimumSize() {
		return minimumSize(4);
    }

	//
	// ListPeer interface
	//
	// interface native 
    public native void setMultipleSelections(boolean v);
    public native void addItem(String item, int index);
    public native void delItems(int start, int end);
    public native void select(int index);
    public native void deselect(int index);
    public native void makeVisible(int index);

	// interface java impl.
    public void clear() {
		List l = (List)target;
		delItems(0, l.countItems());
    }

    public int[] getSelectedIndexes() {
		List l = (List)target;
		int len = l.countItems();
		int sel[] = new int[len];
		int nsel = 0;
		for (int i = 0 ; i < len ; i++) {
			if (isSelected(i)) {
				sel[nsel++] = i;
			}
		}
		int selected[] = new int[nsel];	
		System.arraycopy(sel, 0, selected, 0, nsel);
		return selected;
    }

    public Dimension preferredSize(int v) {
		return minimumSize(v);
    }
    
	public Dimension minimumSize(int v) {
		FontMetrics fm = getFontMetrics(target.getFont());
		return new Dimension(20 + fm.stringWidth("0123456789abcde"), (fm.getHeight() + 2) * v);
    }

	//
	// class public native
    public native boolean isSelected(int index);

	// 
	// class public java impl.
    public void handleAction(long time, int msgData, int index) {
		List l = (List)target;
		l.select(index);
		target.postEvent(new Event(target, Event.ACTION_EVENT, l.getItem(index)));
    }

    public void handleListChanged(long time, int msgData, int index, boolean isSelected) {
		List l = (List)target;
        boolean fireEvent = true;

/*************************************************************************************
        // tring not to send an event when in a multiple selection list box
        // we just walk the list using the cursor keys

        // if not a multiple selection fire the event
        if (l.allowsMultipleSelections()) {

//            System.out.println("Multiple selection list box, index: " + index);

		    int selected[] = getSelectedIndexes();
            int i;

		    for (i=0; i < selected.length - 1; i++) {

//                System.out.println("Item in array: " + selected[i]);

			    if (index == selected[i] && isSelected) {
//                    System.out.println("index already selected, just walking");
                    fireEvent = false;
				    break;
			    }
		    }
            if (i == selected.length && !isSelected) {
//                System.out.println("All list scanned and no selection, just walking");
                fireEvent = false;
            }
        }
*************************************************************************************/

        if (fireEvent) {
		    if (isSelected) {
			    target.postEvent(new Event(target, Event.LIST_SELECT,
						       new Integer(index)));
		    } else {
			    target.postEvent(new Event(target, Event.LIST_DESELECT,
						       new Integer(index)));
		    }
        }
    }

}
