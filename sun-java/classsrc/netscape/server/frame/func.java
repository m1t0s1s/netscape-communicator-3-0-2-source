package netscape.server.frame;

import netscape.server.base.pblock;
import netscape.server.base.Session;
import netscape.server.frame.Request;


/**
 * func provides an interface to other NSAPI functions.
 * @author	Rob McCool
 */


public class func {
    /**
      Execute an NSAPI function. pb should contain a parameter with the
      name "fn" to identify the function to be called. 
     */
    public static native int exec(pblock pb, Session sn, Request rq);
    private func() {}
}
