package sun.awt.macos;

import java.awt.*;
import sun.awt.macos.*;

class MacintoshEvent extends java.awt.Event {

	private int privateData;

	public MacintoshEvent(Object target, long when, int id, int x, int y,
                 int key, int modifiers, Object arg, int privateDataIn)
	{
		super(target, when, id, x, y, key, modifiers, arg);
		privateData = privateDataIn;
	}

}

