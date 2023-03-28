package sun.awt.macos;


import java.awt.*;
import java.awt.peer.*;

class MCheckboxMenuItemPeer extends MMenuItemPeer implements CheckboxMenuItemPeer, ActionComponent {

    MCheckboxMenuItemPeer(CheckboxMenuItem target) {
		this.target = target;
		isCheckItem = true;
    }

    public void setState(boolean t) { }
    
    public void action(int modifiers) {
		CheckboxMenuItem target = (CheckboxMenuItem)this.target;
		target.setState(target.getState() ? false : true);
		super.action(modifiers);
    }

    public void action() {
		System.err.println("Internal AWT error");
	}
	
    public void action(boolean refcon) {
		System.err.println("Internal AWT error");
	}

	public void handleAction(int inFlags) {
		MenuComponent target = this.target;
    	InterfaceThread.postInterfaceEvent(target, inFlags);
	}


}

	
