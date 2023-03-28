package sun.awt.macos;

import java.util.Vector;
import java.awt.*;
import java.awt.peer.*;

class MWindowPeer extends MFramePeer implements WindowPeer {

	MWindowPeer(Window target) { 
		super(target); 
	}

	native void create(MComponentPeer parent);
	
}

