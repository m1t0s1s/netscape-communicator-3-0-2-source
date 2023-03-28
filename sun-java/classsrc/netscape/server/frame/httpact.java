

package netscape.server.frame;

import netscape.server.base.Session;


/**
 * Server actions specific to the HTTP server.
 *
 * @author	Rob McCool
 */

public class httpact {
    /**
      Given a URI or virtual path, translate it into a filesystem path
      using the server's name translation facilities. Returns null
      if no mapping could be generated.
     */
    public static native String translate_uri(String uri, Session sn);
}
