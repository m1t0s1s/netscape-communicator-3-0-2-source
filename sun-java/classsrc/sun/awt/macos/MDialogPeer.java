package sun.awt.macos;

import java.util.Vector;
import java.awt.*;
import java.awt.peer.*;

class MDialogPeer extends MWindowPeer implements DialogPeer {

	native void create(MComponentPeer parent);

	MDialogPeer(Dialog target) { super(target); }

    public native void setTitle(String title);
    public native void setResizable(boolean resizeable);

}

