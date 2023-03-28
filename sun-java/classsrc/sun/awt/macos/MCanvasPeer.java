package sun.awt.macos;

import java.awt.*;
import java.awt.peer.*;

class MCanvasPeer extends MComponentPeer implements CanvasPeer {

    native void create(MComponentPeer parent);

    MCanvasPeer(Component target) {
		super(target);
    }

}
