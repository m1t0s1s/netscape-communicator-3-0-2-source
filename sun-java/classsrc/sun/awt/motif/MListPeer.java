/*
 * @(#)MListPeer.java	1.14 95/11/22 Sami Shaio
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
package sun.awt.motif;

import java.awt.*;
import java.awt.peer.*;

class MListPeer extends MComponentPeer implements ListPeer {
    native void create(MComponentPeer parent);

    void initialize() {
	List li = (List)target;

	/* add any items that were already inserted in the target. */
	int  nitems = li.countItems();
	for (int i = 0; i < nitems; i++) {
	    addItem(li.getItem(i), -1);
	}

	/* set whether this list should allow multiple selections. */
	setMultipleSelections(li.allowsMultipleSelections());

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

    MListPeer(List target) {
	super(target);
    }
    public native void setMultipleSelections(boolean v);
    public native boolean isSelected(int index);
    public native void addItem(String item, int index);
    public native void delItems(int start, int end);
    public native void select(int index);
    public native void deselect(int index);
    public native void makeVisible(int index);

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

    public void action(int index) {
	List l = (List)target;
	l.select(index);
	target.postEvent(new Event(target, Event.ACTION_EVENT, l.getItem(index)));
    }

    public void handleListChanged(int index) {
	int selected[] = ((List)target).getSelectedIndexes();
	boolean isSelected = false;

	for (int i=0; i < selected.length; i++) {
	    if (index == selected[i]) {
		isSelected = true;
		break;
	    }
	}
	if (isSelected) {
	    target.postEvent(new Event(target, Event.LIST_SELECT,
				       new Integer(index)));
	} else {
	    target.postEvent(new Event(target, Event.LIST_DESELECT,
				       new Integer(index)));
	}
    }

    public Dimension minimumSize() {
	return minimumSize(4);
    }
    public Dimension preferredSize(int v) {
	return minimumSize(v);
    }
    public Dimension minimumSize(int v) {
	X11FontMetrics fm = (X11FontMetrics)getFontMetrics(target.getFont());
	List list = (List)target;
        int height = 0;
        Dimension size = new Dimension();

        // Calculate a height that will allow "v" rows to be displayed.
	for (int i = list.countItems() ; i-- > 0 ;) {
            size = fm.stringExtent( list.getItem(i) );
            height = Math.max( size.height, height );
	}
        size.height = (height + 2) * v + 11;

        // The desired width is 15 characters plus some padding.  Picking 15 random
        // characters like this is kind of bogus.  At least it results in a fixed width
        // for a given font.  Since Motif will supply scroll bars if any item is too
        // wide we'll leave it like this for now.
        size.width  = 20 + fm.stringWidth("0123456789abcde");
        return size;
    }
}
