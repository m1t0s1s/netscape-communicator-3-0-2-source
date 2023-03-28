package netscape.server.applet;

/**
 * Records a warning.
 */
public class Warning extends ServerAppletException {
    public Warning() {
	super();
    }

    public Warning(String msg) {
	super(msg);
    }
}

