package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

class MChoicePeer extends MComponentPeer implements ChoicePeer, ActionComponent {

	int		mActualMenuContents;

	public MChoicePeer(Choice target) {
		super(target);
    }

	native void create(MComponentPeer parent);
    
	native public void select(int index);

    void initialize() {
    
		Choice opt = (Choice)target;
		
		int itemCount = opt.countItems();
		
		for (int i=0; i < itemCount; i++) {
		    addItem(opt.getItem(i), i);
		}
		
		select(opt.getSelectedIndex());
		super.initialize();

	}
	
    public Dimension minimumSize() {
    
    	//	The minimum size for the choice menu is the maximum
    	//	width of all of its items.
    
		FontMetrics fm = getFontMetrics(target.getFont());
		Choice c = (Choice)target;
		
		int w = 0;
		for (int i = c.countItems() ; i-- > 0 ;) {
		    w = Math.max(fm.stringWidth(c.getItem(i)), w);
		}
		
		return new Dimension(40 + w, fm.getHeight() + 4);
	    
    }

    public native void addItem(String item, int index);

    public native void remove(int index);
    
    public void action(int index) {
		Choice c = (Choice)target;
		c.select(index);
		target.postEvent(new Event(target, Event.ACTION_EVENT, c.getItem(index)));
    }

    public void action() {
		System.err.println("Internal AWT error");
	}
	
    public void action(boolean refcon) {
		System.err.println("Internal AWT error");
	}

	public void handleAction(int inFlags) {
    	InterfaceThread.postInterfaceEvent(target, inFlags);
	}
	
}

