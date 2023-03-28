package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

class MMenuItemPeer implements MenuItemPeer, ActionComponent {

    boolean		isCheckItem;
    MenuItem	target;

    MMenuItemPeer() { }

    MMenuItemPeer(MenuItem target) {
		this.target = target;
    	isCheckItem = false;
    }
    
	private int modifiersToJavaFlags(int modifiers) {
	
		int	flags = 0;
	
   		if ((modifiers & 0x0100) != 0)		// cmdKey
   			flags |= Event.META_MASK;

   		if ((modifiers & 0x0200) != 0)		// shiftKey
   			flags |= Event.SHIFT_MASK;
   			
   		if ((modifiers & 0x0800) != 0)		// optionKey
   			flags |= Event.ALT_MASK;
   			
   		if ((modifiers & 0x1000) != 0)		// controlKey
   			flags |= Event.CTRL_MASK;
   		
   		return flags;
   		 
	}

    public void action(int modifiers) {
		target.postEvent(new Event(target, System.currentTimeMillis(), Event.ACTION_EVENT, 0, 0, 0, 
				modifiersToJavaFlags(modifiers), target.getLabel()));
    }

    public void action(boolean refcon) {
		System.err.println("Internal AWT error");
	}
	
    public void action() {
		System.err.println("Internal AWT error");
	}

	public void handleAction(int inFlags) {
		MenuComponent target = this.target;
    	InterfaceThread.postInterfaceEvent(target, inFlags);
	}

    public void enable() { }
    public void disable() { }
    public void dispose() { }
    public void setLabel(String label) { }

}
