package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

class MButtonPeer extends MComponentPeer implements ButtonPeer, ActionComponent {

    MButtonPeer(Button target) {
		super(target);
    }

	native void create(MComponentPeer peer);
	native public void setLabel(String label);
 
    public Dimension minimumSize() {
		FontMetrics fm = getFontMetrics(target.getFont());
		return new Dimension(fm.stringWidth(((Button)target).getLabel()) + 25, 
				fm.getHeight() + 7);
    }

    public void action() {
		target.postEvent(new Event(target, Event.ACTION_EVENT, ((Button)target).getLabel()));
    }

    public void action(int refcon) {
		System.err.println("Internal AWT error");
	}
	
    public void action(boolean refcon) {
		System.err.println("Internal AWT error");
	}

	public void handleAction() {
    	InterfaceThread.postInterfaceEvent(target);
 		Checkbox target = (Checkbox)this.target;
	}

	
}
