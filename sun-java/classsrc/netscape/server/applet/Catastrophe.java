package netscape.server.applet;

/**
 * Records a catastrophe.
 */
public class Catastrophe extends ServerAppletException {
    public Catastrophe() {
	super();
    }

    public Catastrophe(String msg) {
	super(msg);
    }
}

