
package netscape.server.frame;

import netscape.server.base.Session;
import netscape.server.frame.Request;


/**
 * Interface to the server's error log. Record in here any errors you
 * encounter, or any warnings you want to bring to the administrator's
 * attention.
 *
 * @author	Rob McCool
 */

public class errorLog {
    private static native void report(int degree, String name, 
                                      Session sn, Request rq,
                                      String error);

    /**
      Record a warning. name identifies your function to the administrator,
      sn and rq are the Session and Request generating the error (these
      can be null), and error is a string describing what went wrong.
     */
    public static void warn(String name, Session sn, Request rq, String error)
    { 
        report(WARN, name, sn, rq, error);
    }

    /**
      Record a misconfiguration, or a missing or illegal parameter. name
      identifies your function to the administrator,
      sn and rq are the Session and Request generating the error (these
      can be null), and error is a string describing what went wrong.
     */
    public static void reportMisconfig(String name, Session sn, Request rq,
                                       String error)
    { 
        report(MISCONFIG, name, sn, rq, error);
    }

    /**
      Record a security violation, or someone trying to access a resource
      they shouldn't. name identifies your function to the
      administrator, sn and rq are the Session and Request generating the 
      error (these can be null), and error is a string describing what went
      wrong.
     */
    public static void reportSecurity(String name, Session sn, Request rq,
                                      String error)
    { 
        report(SECURITY, name, sn, rq, error);
    }


    /**
      Record a general failure. name identifies your function to the
      administrator, sn and rq are the Session and Request generating the 
      error (these can be null), and error is a string describing what went
      wrong.
     */
    public static void reportFailure(String name, Session sn, Request rq,
                                     String error)
    { 
        report(FAILURE, name, sn, rq, error);
    }

    /**
      Record a catastrophic failure. name identifies your function to the
      administrator, sn and rq are the Session and Request generating the 
      error (these can be null), and error is a string describing what went
      wrong.
     */
    public static void reportCatastrophe(String name, Session sn, Request rq,
                                         String error)
    { 
        report(CATASTROPHE, name, sn, rq, error);
    }

    /**
      Record an informational message. name identifies your function to the
      administrator, sn and rq are the Session and Request generating the 
      error (these can be null), and error is a string describing what went
      wrong.
     */
    public static void inform(String name, Session sn, Request rq,
                              String error)
    { 
        report(INFORM, name, sn, rq, error);
    }

    private static final int WARN = 0;
    private static final int MISCONFIG = 1;
    private static final int SECURITY = 2;
    private static final int FAILURE = 3;
    private static final int CATASTROPHE = 4;
    private static final int INFORM = 5;
}
