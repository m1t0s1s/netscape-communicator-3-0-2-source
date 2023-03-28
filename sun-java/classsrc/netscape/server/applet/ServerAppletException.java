package netscape.server.applet;

/**
 * Superclass of all ServerApplet exceptions.
 */
public class ServerAppletException extends Exception {
    public ServerAppletException() {
	super();
    }

    public ServerAppletException(String msg) {
	super(msg);
    }

    public String toString() {
	String msg = getMessage();
	if (msg != null)
	    return msg;
	return getClass().getName();
    }
}

