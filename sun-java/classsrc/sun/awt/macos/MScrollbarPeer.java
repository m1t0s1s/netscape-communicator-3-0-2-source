package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

class MScrollbarPeer extends MComponentPeer implements ScrollbarPeer {
	
	int		mLineIncrement;
	int		mPageIncrement;

	MScrollbarPeer(Scrollbar target) {
		super(target);
	}

	native void create(MComponentPeer parent);

	public native void setValue(int value);    
	public native void setValues(int value, int visible, int minimum, int maximum);    
    
    public Dimension minimumSize() {
		if (((Scrollbar)target).getOrientation() == Scrollbar.VERTICAL) {
		    return new Dimension(16, 50);
		} else {
		    return new Dimension(50, 16);
		}
    }

    public void lineUp(int value) {
		Scrollbar sb = (Scrollbar)target;
		sb.setValue(value);
		target.postEvent(new Event(target, Event.SCROLL_LINE_UP, new Integer(value)));
    }

    public void lineDown(int value) {
		Scrollbar sb = (Scrollbar)target;
		sb.setValue(value);
		target.postEvent(new Event(target, Event.SCROLL_LINE_DOWN, new Integer(value)));
    }

    public void pageUp(int value) {
		Scrollbar sb = (Scrollbar)target;
		sb.setValue(value);
		target.postEvent(new Event(target, Event.SCROLL_PAGE_UP, new Integer(value)));
    }

    public void pageDown(int value) {
		Scrollbar sb = (Scrollbar)target;
		sb.setValue(value);
		target.postEvent(new Event(target, Event.SCROLL_PAGE_DOWN, new Integer(value)));
    }

    public void dragAbsolute(int value) {
		Scrollbar sb = (Scrollbar)target;
		sb.setValue(value);
		target.postEvent(new Event(target, Event.SCROLL_ABSOLUTE, new Integer(value)));
    }

	native public void setLineIncrement(int increment);

	native public void setPageIncrement(int increment);

}
