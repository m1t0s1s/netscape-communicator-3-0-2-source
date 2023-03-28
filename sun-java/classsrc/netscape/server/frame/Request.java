package netscape.server.frame;

import netscape.server.base.pblock;
import java.lang.String;

/**
 * A Request represents one request from a client. There are parameter 
 * blocks describing what the client requested and how, and variables 
 * which other NSAPI functions set.
 *
 * @author	Rob McCool
 */

public class Request {
    /**
     * Public parameter blocks. You are free to modify the contents of 
     * these pblocks as you need to.
     */
    public pblock vars, srvhdrs;

    /* These are private so that they can be made read-only. */
    private pblock reqpb, headers;
    private int Crequest;

    private native void create();
    private native void free();

    /**
     * There are no parameters when you construct this object from Java.
     */
    public Request() {
        create();
    }

    /**
     * For native import methods. Using this to construct a Java instance 
     * is certain to cause you NullPointerException chaos.
     */
    public Request(boolean nocreate) {
        Crequest = 0;
    }


    public void finalize() {
        if(Crequest != 0) free();
    }

    /**
     * HTTP requests have attached RFC822 headers, which are actually
     * name=value pairs. Convert the header name you want to access to
     * lower case and this function will return its value if the client
     * sent it. Example:<p>
     * Client sends: <code>User-agent: Mozilla/1.1N</code><br>
     * Then: <code>getHeader("user-agent") == "Mozilla/1.1N"</code>
     */
    public String getHeader(String name) {
        if(Crequest == 0) create();
        return headers.findval(name);
    }

    /**
     * Gets an aspect of the request. For HTTP, the request consists of:<p>
     * <pre>
     * Name               Value
     * ------------------------------------------------------------------------
     * method             HTTP method used in the request, usually GET or POST.
     * uri                The URI originally requested
     * protocol           The protocol identifier sent by the client
     * query              Any query string (data following a ?) sent in the URI
     * </pre>
     */
    public String getRequestParameter(String name) {
        if(Crequest == 0) create();
        return reqpb.findval(name);
    }

    /**
     * Sets the content-type of the server response to ct. Overrides any
     * prior setting. Convenience function.
     */
    public void setContentType(String ct) {
        srvhdrs.remove("content-type");
        srvhdrs.insert("content-type", ct);
    }

    /**
     * Sets the server to respond with a new header. n provides the name of
     * the header to send, and v provides its value. Follows the same rules
     * as getHeader: convert the header name to lowercase.
     */
    public void setHeader(String n, String v) {
        srvhdrs.remove(n);
        srvhdrs.insert(n, v);
    }

    /**
     * Constant return code: NSAPI function was successful and performed an
     * action.
     */
    public final int PROCEED = 0;

    /**
     * Constant return code: NSAPI function encountered an error and the 
     * request cannot proceed.
     */
    public final int ABORTED = -1;

    /**
     * Constant return code: NSAPI function determined that it could not
     * do anything for the request.
     */
    public final int NOACTION = -2;

    /**
     * Constant return code: Service function found that the client is 
     * unreachable, stop processing the request but do not report an error.
     */
    public final int EXIT = -3;
}
