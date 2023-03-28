
package netscape.server.frame;

/**
 * The global configuration parameters for the server.
 * @author	Rob McCool
 */

public class conf {
    private static conf currentConfig = new conf();

    private int port;
    private String address;
    private boolean security_active;
    private String server_hostname;

    private synchronized native void getvars();

    private conf() {
        getvars();
    }

    /**
      The port number that this server listens to.
     */
    public static int getPort() {
        return currentConfig.port;
    }

    /**
      If the server does not listen to every IP address on the system, 
      this specifies the specific address that it listens to in dotted
      decimal notation.
     */
    public static String getAddress() {
        return currentConfig.address;
    }

    /**
      Set to true if a Commerce is encrypting data to and from the 
      clients.
     */
    public static boolean securityActive() {
        return currentConfig.security_active;
    }

    /**
      The fully qualified domain name that the server thinks is its own.
      This does not necessarily have to be the machine's hostname, but 
      it's what the administrator wants to appear in URLs.
     */
    public static String getHostname() {
        return currentConfig.server_hostname;
    }
}
