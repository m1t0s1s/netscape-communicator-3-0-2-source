/*
 * @(#)WContainerPeer.java
 */

package sun.awt.windows;

import java.awt.*;
import java.awt.peer.*;

public abstract class WContainerPeer extends WComponentPeer implements ContainerPeer {

    Insets insets = new Insets(0, 0, 0, 0);

    // ContainerPeer interface
    public Insets insets() {
	    //return insets;
	    //return new Insets(insets.top, insets.left, insets.bottom, insets.right);
        return (Insets)insets.clone();
    }

    // constructor
    WContainerPeer(Component target) {

    	super(target);
    	calculateInsets(insets);
    }

    native void calculateInsets(Insets i);

    public void print(Graphics g) {
    	super.print(g);
    	((Container)target).printComponents(g);
    }
}

