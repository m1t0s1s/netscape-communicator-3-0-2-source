package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

public class MMenuBarPeer implements MenuBarPeer {

    int		pData;
    MenuBar	target;

    native void create(Frame containerFrame);
    
	public MMenuBarPeer(MenuBar target) {
		this.target = target;
		create((Frame)target.getParent());
	}

	public native void dispose();
	public native void addMenu(Menu m);
	public native void delMenu(int index);
	public native void addHelpMenu(Menu m);
	
}
