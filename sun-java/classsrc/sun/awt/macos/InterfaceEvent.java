package sun.awt.macos;

import java.awt.*;

public class InterfaceEvent {

	public static final int NONE				= 0;
	public static final int EXPOSE				= 1;
	public static final int REPAINT				= 2;
	public static final int MOUSE_DOWN			= 3;
	public static final int MOUSE_UP			= 4;
	public static final int MOUSE_MOVED			= 5;
	public static final int MOUSE_DRAGGED			= 6;
	public static final int KEY_PRESS			= 7;
	public static final int KEY_RELEASE			= 8;
	public static final int KEY_ACTION			= 9;
	public static final int KEY_ACTION_RELEASE		= 10;
	public static final int MOUSE_ENTER			= 11;
	public static final int MOUSE_EXIT			= 12;
	public static final int ACTION_VOID			= 13;
	public static final int ACTION_BOOL	 		= 14;
	public static final int ACTION_INT			= 15;
	public static final int ACTION_MENU			= 16;
	public static final int LIST_SELECT			= 17;
	public static final int LIST_DESELECT			= 18;
  	public static final int kRepaintsBetweenSleeps		= 2;
	
	static InterfaceEvent					gFirstInterfaceEvent = null;
	static InterfaceEvent					gLastInterfaceEvent = null;
	static Thread						mInterfaceThread = null;

	static int						gCountTilSleep = kRepaintsBetweenSleeps;

	InterfaceEvent						next;
	InterfaceEvent						prev;
	int							type;
	Component						component;
	MenuComponent						menuComponent;
	int							x;
	int							y;
	int							width;
	int							height;
	long							when;
	int							flags;
	int							key;
	int							clickCount;
	boolean							actionState;
	int							eventNumber;

	public InterfaceEvent(int inType, Component inComponent, MenuComponent inMenuComponent, int inX, int inY, int inWidth, int inHeight, long inWhen, int inFlags, int inKey, int inClickCount, boolean state, int inEventNumber)
	{
		next = null;
		prev = null;
		component = inComponent;
		type = inType;
		width = inWidth;
		height = inHeight;
		x = inX;
		y = inY;
		when = inWhen;
		flags = inFlags;
		key = inKey;
		clickCount = inClickCount;
		actionState = state;
		menuComponent = inMenuComponent;
		eventNumber = inEventNumber;
	}
	
	public int getEventNumber() {
		return eventNumber;
	}
	
	public int getClickCount() {
		return clickCount;
	}

	public int getType() { 
		return type; 
	}

	public Component getComponent() { 
		return component; 
	}
	
	public MenuComponent getMenuComponent() { 
		return menuComponent; 
	}

	public int getKey() {
		return key;
	}

	public long getWhen() {
		return when;
	}

	public int getFlags() {
		return flags;
	}
	
	public int getX() {
		return x;
	}
	
	public int getY() {
		return y;
	}

	public int getWidth() {
		return width;
	}

	public int getHeight() {
		return height;
	}
	
	public boolean getActionState() {
		return actionState;	
	}
	
	public boolean enqueue() {
		
		//	If the event is a repaint or expose event, then search 
		//	through the queue of events to see if there is already one
		//	for this component.  If so, then we don't add this one
		//	to the queue.
		
		if ((type == REPAINT) || (type == EXPOSE)) {

			InterfaceEvent	currentEvent = gLastInterfaceEvent;
			while (currentEvent != null) {
				if ((currentEvent.getType() == type) &&
				    (currentEvent.getComponent() == getComponent())) {
					
					// Coalesce the events.

					if (x < currentEvent.x) {
						currentEvent.width += (currentEvent.x - x);
						currentEvent.x = x; 
					}

					if (y < currentEvent.y) {
						currentEvent.height += currentEvent.y - y;
						currentEvent.y = y;
					}

					if (x + width > currentEvent.x + currentEvent.width) {
						currentEvent.width = x + width - currentEvent.x;	
					}

					if (y + height > currentEvent.y + currentEvent.height) {
						currentEvent.height = y + height - currentEvent.y;
					}

					return false;
				}
				currentEvent = currentEvent.next;
			}

			// 	This is awful but necessary.  It is possible
			//	for an applet to starve other applets and
			//	ImageFetchers by repeatedly posting repaint
			//	events.  For applets that do this, we give
			//	up some time.
			
			if (type == REPAINT) {
				try {
					if (gCountTilSleep == 0) {
						Thread.currentThread().sleep(15);
						gCountTilSleep = kRepaintsBetweenSleeps;
					}
					else {
						gCountTilSleep--;
					}
				}

				catch (InterruptedException i) {
				}
			}
	
		}
		
		//	Enqueue the new event.
	
		next = null;
		prev = gLastInterfaceEvent;
		
		if (gLastInterfaceEvent != null)
			gLastInterfaceEvent.next = this;

		gLastInterfaceEvent = this;
			
		if (gFirstInterfaceEvent == null)
			gFirstInterfaceEvent = this;
	
		return true;
	
	}

	public void dequeue() {
	
		if (next != null)
			next.prev = prev;
		else
			gLastInterfaceEvent = prev;
			
		if (prev != null)
			prev.next = next;
		else
			gFirstInterfaceEvent = next;	
	
	}	
	
	public static InterfaceEvent firstEvent() {
		return gFirstInterfaceEvent;
	}

	public static void flushInterfaceQueue(MComponentPeer deadComponent) {
		InterfaceEvent		currentEvent = gFirstInterfaceEvent;

		while (currentEvent != null) {
			if (currentEvent.component == deadComponent.target) {
				if (currentEvent.prev == null)
					gFirstInterfaceEvent = currentEvent.next;
				else
					currentEvent.prev.next = currentEvent.next;
				if (currentEvent.next == null)
					gLastInterfaceEvent = currentEvent.prev;
				else
					currentEvent.next.prev = currentEvent.prev;

			}
			currentEvent = currentEvent.next;
		}
	}

}

