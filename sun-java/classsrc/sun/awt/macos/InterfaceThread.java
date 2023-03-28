package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

public class InterfaceThread implements Runnable {

 	static Object			mInterfaceQueueWaitObject = new Object();
	static Object			mInterfaceQueueSyncObject = new Object();
	static Thread			mInterfaceThread = null;

	public native void run();

	public void dispatchInterfaceEvent() {

   		InterfaceEvent		nextEvent;
   		Component			eventComponent;
   		ComponentPeer		peer;
   		MenuComponentPeer	menuPeer;
   		Graphics			g;

		try {
		
			synchronized(mInterfaceQueueWaitObject) {
				if (InterfaceEvent.firstEvent() == null)
			   		mInterfaceQueueWaitObject.wait();
		   	}
		   	
		}
		
		catch (java.lang.InterruptedException e) {
		}
		   		
		synchronized (mInterfaceQueueSyncObject) {
    		nextEvent = InterfaceEvent.firstEvent();
    		if (nextEvent != null)
    			nextEvent.dequeue();
		}
		    		
		if (nextEvent != null) {
		
    		eventComponent = nextEvent.getComponent();

			switch (nextEvent.getType()) {
				
				case InterfaceEvent.EXPOSE:
					Dimension d = eventComponent.size();
					g = eventComponent.getGraphics();
					g.setColor(eventComponent.getBackground());
					g.fillRect(0, 0, d.width, d.height);
					g.setColor(eventComponent.getForeground());
					g.setFont(eventComponent.getFont());
					eventComponent.paint(g);
					break;
			
				case InterfaceEvent.REPAINT:
					g = eventComponent.getGraphics();
					g.clipRect(nextEvent.getX(), 
						   nextEvent.getY(),
						   nextEvent.getWidth(),
						   nextEvent.getHeight());
					eventComponent.update(g);
					break;
					
				case InterfaceEvent.MOUSE_ENTER:
					eventComponent.postEvent(new MacintoshEvent(eventComponent, 
						nextEvent.getWhen(), 
						Event.MOUSE_ENTER, 
						nextEvent.getX(),
						nextEvent.getY(),
						0,
						nextEvent.getFlags(),
						null,
						0));
					break;
				
				case InterfaceEvent.MOUSE_EXIT:
					eventComponent.postEvent(new MacintoshEvent(eventComponent, 
						nextEvent.getWhen(), 
						Event.MOUSE_EXIT, 
						nextEvent.getX(),
						nextEvent.getY(),
						0,
						nextEvent.getFlags(),
						null,
						0));
					break;

				case InterfaceEvent.MOUSE_DOWN:
					Event newEvent = new MacintoshEvent(eventComponent, 
						nextEvent.getWhen(), 
						Event.MOUSE_DOWN, 
						nextEvent.getX(),
						nextEvent.getY(),
						0,
						nextEvent.getFlags(),
						null,
						nextEvent.getEventNumber());
					newEvent.clickCount = nextEvent.getClickCount();
					eventComponent.postEvent(newEvent);
					break;

				case InterfaceEvent.MOUSE_UP:
					eventComponent.postEvent(new MacintoshEvent(eventComponent, 
						nextEvent.getWhen(), 
						Event.MOUSE_UP, 
						nextEvent.getX(),
						nextEvent.getY(),
						0,
						nextEvent.getFlags(),
						null,
						nextEvent.getEventNumber()));
					break;

				case InterfaceEvent.MOUSE_DRAGGED:
					eventComponent.postEvent(new MacintoshEvent(eventComponent, 
						nextEvent.getWhen(), 
						Event.MOUSE_DRAG, 
						nextEvent.getX(),
						nextEvent.getY(),
						0,
						nextEvent.getFlags(),
						null,
						0));
					break;

				case InterfaceEvent.MOUSE_MOVED:
					eventComponent.postEvent(new MacintoshEvent(eventComponent, 
						nextEvent.getWhen(), 
						Event.MOUSE_MOVE, 
						nextEvent.getX(),
						nextEvent.getY(),
						0,
						nextEvent.getFlags(),
						null,
						0));
					break;

				case InterfaceEvent.KEY_PRESS:
					eventComponent.postEvent(new MacintoshEvent(eventComponent, 
						nextEvent.getWhen(), 
						Event.KEY_PRESS, 
						nextEvent.getX(),
						nextEvent.getY(),
						nextEvent.getKey(),
						nextEvent.getFlags(),
						null,
						nextEvent.getEventNumber()));
					break;

				case InterfaceEvent.KEY_RELEASE:
					eventComponent.postEvent(new MacintoshEvent(eventComponent, 
						nextEvent.getWhen(), 
						Event.KEY_RELEASE, 
						nextEvent.getX(),
						nextEvent.getY(),
						nextEvent.getKey(),
						nextEvent.getFlags(),
						null,
						nextEvent.getEventNumber()));
					break;

				case InterfaceEvent.KEY_ACTION:
					eventComponent.postEvent(new MacintoshEvent(eventComponent, 
						nextEvent.getWhen(), 
						Event.KEY_ACTION, 
						nextEvent.getX(),
						nextEvent.getY(),
						nextEvent.getKey(),
						nextEvent.getFlags(),
						null,
						nextEvent.getEventNumber()));
					break;

				case InterfaceEvent.KEY_ACTION_RELEASE:
					eventComponent.postEvent(new MacintoshEvent(eventComponent, 
						nextEvent.getWhen(), 
						Event.KEY_ACTION_RELEASE, 
						nextEvent.getX(),
						nextEvent.getY(),
						nextEvent.getKey(),
						nextEvent.getFlags(),
						null,
						nextEvent.getEventNumber()));
					break;

				case InterfaceEvent.ACTION_VOID:
					peer = eventComponent.getPeer();
					((ActionComponent)peer).action();
					break;
					
				case InterfaceEvent.ACTION_BOOL:
					peer = eventComponent.getPeer();
					((ActionComponent)peer).action(nextEvent.getActionState());
					break;

				case InterfaceEvent.ACTION_INT:
					peer = eventComponent.getPeer();
					((ActionComponent)peer).action(nextEvent.getFlags());
					break;
					
				case InterfaceEvent.ACTION_MENU:
					menuPeer = (nextEvent.getMenuComponent()).getPeer();
					((ActionComponent)menuPeer).action(nextEvent.getFlags());
					break;

				case InterfaceEvent.LIST_SELECT:
					eventComponent.postEvent(new Event(eventComponent, Event.LIST_SELECT,
						new Integer(nextEvent.getFlags())));
					break;

				case InterfaceEvent.LIST_DESELECT:
					eventComponent.postEvent(new Event(eventComponent, Event.LIST_DESELECT,
						new Integer(nextEvent.getFlags())));
					break;

				default:
					//	It is an error if we ever get here.
					break;
				
			}
    		
		}

	}

	//	Event posting messages
	
    static void postInterfaceEvent(int eventType, Component eventComponent, MenuComponent menuComponent, int eventX, int eventY, int eventWidth, int eventHeight, long eventWhen, int eventFlags, int eventKey, int eventClickCount, boolean state, int inEventNumber) {

		if (mInterfaceThread == null) {
			mInterfaceThread = new Thread(new InterfaceThread(), "AWT Interface Thread");
			mInterfaceThread.start();
		}

		InterfaceEvent 	newEvent = new InterfaceEvent(eventType, eventComponent, menuComponent, eventX, eventY, eventWidth, eventHeight, eventWhen, eventFlags, eventKey, eventClickCount, state, inEventNumber);
		
		synchronized (mInterfaceQueueSyncObject) {
			if (newEvent.enqueue()) {
				synchronized(mInterfaceQueueWaitObject)
					mInterfaceQueueWaitObject.notify();
			}
		}

    }

    static void postInterfaceEvent(int eventType, Component eventComponent, int eventX, int eventY, int eventWidth, int eventHeight, long eventWhen, int eventFlags, int eventKey, int eventClickCount, int eventNumber) {
    	postInterfaceEvent(eventType, eventComponent, null, eventX, eventY, eventWidth, eventHeight, eventWhen, eventFlags, eventKey, eventClickCount, false, eventNumber);
    }

    static void postInterfaceEvent(int eventType, Component eventComponent, int eventX, int eventY, int eventWidth, int eventHeight, long eventWhen, int eventFlags, int eventKey) {
    	postInterfaceEvent(eventType, eventComponent, null, eventX, eventY, eventWidth, eventHeight, eventWhen, eventFlags, eventKey, 0, false, 0);
    }

 	static void postInterfaceEvent(int eventType, Component eventComponent, int flags)
 	{
     	postInterfaceEvent(eventType, eventComponent, null, 0, 0, 0, 0, 0, flags, 0, 0, false, 0);
 	}

    static void postInterfaceEvent(Component eventComponent) {
    	postInterfaceEvent(InterfaceEvent.ACTION_VOID, eventComponent, null, 0, 0, 0, 0, 0, 0, 0, 0, false, 0);
    }

    static void postInterfaceEvent(Component eventComponent, int flags) {
    	postInterfaceEvent(InterfaceEvent.ACTION_INT, eventComponent, null, 0, 0, 0, 0, 0, flags, 0, 0, false,0);
    }

    static void postInterfaceEvent(Component eventComponent, boolean state) {
    	postInterfaceEvent(InterfaceEvent.ACTION_BOOL, eventComponent, null, 0, 0, 0, 0, 0, 0, 0, 0, state, 0);
    }

    static void postInterfaceEvent(MenuComponent eventComponent, int flags) {
    	postInterfaceEvent(InterfaceEvent.ACTION_MENU, null, eventComponent, 0, 0, 0, 0, 0, 0, flags, 0, false, 0);
    }

}

