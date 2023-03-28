package netscape.server.frame;

import netscape.server.base.pblock;
import netscape.server.base.Session;
import netscape.server.frame.Request;

import netscape.server.base.filestat;
import netscape.server.base.uriutil;
import netscape.server.base.pb_param;

import java.io.IOException;
import java.lang.IllegalArgumentException;
import java.lang.Integer;


/**
 * The http class provides ways to easily perform server-related HTTP
 * functions. Mechanisms are provided to send an HTTP response header
 * to the client, 
 *
 * @author	Rob McCool
 */

public class http {
    private http() {}

    /**
      Start the HTTP response to the request. This function returns 
      Request::NOACTION if the request was a HEAD request and thus no
      body should be sent. Otherwise, it returns Request::PROCEED.
      Any headers contained in Request::srvhdrs will be sent, and the
      status code set with http::status will be sent.
     */
    public static native int start_response(Session sn, Request rq)
        throws IOException;


    /**
      Convenience function: Sets the status to 200 or OK, and starts the
      request. Returns what start_response returns.
     */
    public static int normal_response(Session sn, Request rq) 
        throws IOException
    {
        status(sn, rq, OK);
        return start_response(sn, rq);
    }
    /**
      Status sets the status to the given integer code. The reason string
      will be used to describe the status.
     */
    private static native void Cstatus(Session sn, Request rq, 
                                       int n, String reason);
    public static void status(Session sn, Request rq, int n, String reason) {
        Cstatus(sn, rq, n, reason);
    }
    /**
      Status sets the status to the given integer code. The reason string
      will be a default internal string for the given code.
     */
    public static void status(Session sn, Request rq, int n) {
        Cstatus(sn, rq, n, null);
    }

    /**
      Set HTTP response headers according to the given file. If the client
      gave a conditional request, that is, only send the object if it has
      changed since a certain date, this function will return 
      Request::ABORTED to indicate that the calling function should not
      proceed with the request.
     */
    public static native int set_finfo(Session sn, Request rq, filestat fi);

    /**
      This generates a full URL from a prefix and suffix string. The 
      prefix and suffix strings are concatenated to form the URL after the
      http://host:port part. Either prefix or suffix can be null.
     */
    public static String uri2url(String prefix, String suffix) {
        StringBuffer ret = new StringBuffer("http");
        int port = conf.getPort();

        if(conf.securityActive())
            ret.append("s");
        ret.append(":" + conf.getHostname());
        if(port != DEFAULT_PORT)
            ret.append(":" + port);
        if(prefix == null)
            ret.append(prefix);
        if(suffix == null)
            ret.append(suffix);
        return ret.toString();
    }

    /**
      Takes the form data from this request, and converts it into an array
      of decoded name=value strings. If the request does not have form
      data attached, or any of the parameters the client gave are not sane,
      an IllegalArgumentException is thrown. 
     */
    public static pb_param[] getFormData(Session sn, Request rq) 
        throws IOException, IllegalArgumentException
    {
        String method = rq.getRequestParameter("method");
        String formdata;

        if(method.equals("GET") || method.equals("HEAD")) {
            formdata = rq.getRequestParameter("query");
            if(formdata == null)
                throw new IllegalArgumentException("missing query string");
        }
        else if(method.equals("POST")) {
            String t;
            int cl;
            byte[] b;

            /* Get content length, and sanity check it */
            t = rq.getHeader("content-length");
            if(t == null)
                throw new IllegalArgumentException("no content length");
            try {
                cl = Integer.parseInt(t, 10);
            }
            catch (NumberFormatException e) {
                cl = -1;
            }
            if((cl < 0) || (cl > MAXFORMLENGTH))
                throw new IllegalArgumentException("illegal content length");

            /* Get content type, and sanity check it. */
            t = rq.getHeader("content-type");
            if((t == null) || (!t.equals(FORMTYPE)))
                throw new IllegalArgumentException("illegal content type");

            b = new byte[cl];
            sn.inbuf.getbytes(b);
            formdata = new String(b, 0, 0, b.length);
        }
        else
            throw new IllegalArgumentException("unknown method");

        return uriutil.formdata_split(formdata);
    }


    public static final int OK = 200;
    public static final int NO_RESPONSE = 204;
    public static final int REDIRECT = 302;
    public static final int NOT_MODIFIED = 304;
    public static final int BAD_REQUEST = 400;
    public static final int UNAUTHORIZED = 401;
    public static final int FORBIDDEN = 403;
    public static final int NOT_FOUND = 404;
    public static final int SERVER_ERROR = 500;
    public static final int NOT_IMPLEMENTED = 501;

    private static final int DEFAULT_PORT = 80;
    private static final int MAXFORMLENGTH = 1048576;
    private static final String FORMTYPE = "application/x-www-form-urlencoded";
}
