package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

class MListPeer extends MComponentPeer implements ListPeer, ActionComponent {

	MListPeer(List target) {
		super(target);
	}
    
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

    public void action(boolean refcon) {
		System.err.println("Internal AWT error");
	}
	
    public void action() {
		System.err.println("Internal AWT error");
	}

	public void handleAction(int inFlags) {
    	InterfaceThread.postInterfaceEvent(target, inFlags);
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
 		    InterfaceThread.postInterfaceEvent(InterfaceEvent.LIST_SELECT, target, index);
		} else {
		    if (((List)target).allowsMultipleSelections())
 		    	InterfaceThread.postInterfaceEvent(InterfaceEvent.LIST_DESELECT, target, index);
		}

	}

    public Dimension minimumSize() {
		return minimumSize(4);
    }
    
    public Dimension preferredSize(int v) {	
    	return minimumSize(v);
    }
    
    public Dimension minimumSize(int v) {
		FontMetrics fm = getFontMetrics(target.getFont());
		return new Dimension(20 + fm.stringWidth("0123456789abcde"), (fm.getHeight() + 2) * v + 11);
    }

}
