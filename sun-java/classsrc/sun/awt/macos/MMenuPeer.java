package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

public class MMenuPeer extends MMenuItemPeer implements MenuPeer {

	int		mVisited;

    public MMenuPeer(Menu target) {
   		this.target = target;
	}
    
    public void addSeparator() {}
    
    public void addItem(MenuItem item) {}
    
    public void delItem(int index) {}
    
}
