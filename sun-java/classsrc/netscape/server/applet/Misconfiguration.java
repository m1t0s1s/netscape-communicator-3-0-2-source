package netscape.server.applet;

/**
 * Record a misconfiguration.
 */
public class Misconfiguration extends ServerAppletException {
    public Misconfiguration() {
	super();
    }

    public Misconfiguration(String msg) {
	super(msg);
    }
}

