/*
 * @(#)ServerApplet.java	1.4 95/07/31
 * 
 * Copyright (c) 1995 Netscape Communications Corporation. All Rights Reserved.
 * 
 */

package netscape.server.applet;

import netscape.net.*;
import java.net.*;
import java.io.*;

/**
 * ServerApplet.
 *
 * @version 	1.0, 15 Jan 1996
 * @author 	Warren Harris
 */
abstract public class ServerApplet {
    protected String name;
    protected Socket socket;
    protected boolean responseStarted = false;
    int Cargs;
    int Csession;
    int Crequest;

    ////////////////////////////////////////////////////////////////////////////
    // Running
    ////////////////////////////////////////////////////////////////////////////

    /**
     * This method is overridden by the individual server applets.
     */
    public void run() throws Exception {
	warn("no 'run' method defined for " + name);
    }

    /**
     * Constant return code: NSAPI function was successful and performed an
     * action.
     */
    public static final int PROCEED = 0;

    /**
     * Constant return code: NSAPI function encountered an error and the 
     * request cannot proceed.
     */
    public static final int ABORTED = -1;

    /**
     * Constant return code: NSAPI function determined that it could not
     * do anything for the request.
     */
    public static final int NOACTION = -2;

    /**
     * Constant return code: Service function found that the client is 
     * unreachable, stop processing the request but do not report an error.
     */
    public static final int EXIT = -3;

    /**
     * Called by the system. 
     */
    protected int handleRequest(int rq) {
	Crequest = rq;
 	try {
	    run();
	    return PROCEED;
	}
	catch (Warning e) {
	    warn(e.getMessage());
	}
	catch (Misconfiguration e) {
	    reportMisconfiguration(e.toString());
	}
	catch (SecurityException e) {
	    reportSecurity(e.toString());
	}
	catch (Catastrophe e) {
	    reportCatastrophe(e.toString());
	}
	catch (Throwable e) {
	    reportFailure(e.toString());
	}

	if (responseStarted)
	    return EXIT;
	else
	    return ABORTED;
    }
    
    ////////////////////////////////////////////////////////////////////////////
    // Accessors
    ////////////////////////////////////////////////////////////////////////////

    static Server server = new Server();

    public static Server getServer() {
	return server;
    }

    /**
     * Returns the socket which connects the ServerApplet to the client.
     * The IP address of the client can be obtained by calling getAddress
     * on the socket returned.
     */
    public Socket getClientSocket() {
	return socket;
    }

    InputStream instr;
    PrintStream outstr;

    /**
     * This convenience routine returns an InputStream to be used for
     * reading input. 
     */
    public InputStream getInputStream() throws IOException {
	if (instr == null)
	    instr = socket.getInputStream();
	return instr;
    }

    /**
     * This convenience routine returns a PrintStream to be used for
     * writing text output. 
     */
    public PrintStream getOutputStream() throws IOException {
	if (outstr == null)
	    outstr = new PrintStream(socket.getOutputStream());
	return outstr;
    }

    /**
     * Retrieves things from the client pblock of the Session struct.
     */
    public native String getClientProperty(String name);

    /**
     * Retrieves things from the vars pblock of the Request struct.
     */
    public native String getServerProperty(String name);

    /**
     * Retrieves things from the args pblock associated with the request.
     * This is used to retrieve properties set up in the obj.conf file.
     */
    public native String getConfigProperty(String name);

    /**
     * Retrieves things from the headers pblock of the Request struct.
     * HTTP requests have attached RFC822 headers, which are actually
     * name=value pairs. Convert the header name you want to access to
     * lower case and this function will return its value if the client
     * sent it. Example:<p>
     * Client sends: <code>User-agent: Mozilla/1.1N</code><br>
     * Then: <code>getHeader("user-agent") == "Mozilla/1.1N"</code>
     */
    public native String getHeader(String name);
    
    /**
     * Retrieves things from the reqpb pblock of the Request struct.
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
    public native String getRequestProperty(String name);
    
    /**
     * Sets response properties into the srvhdrs pblock of the Request 
     * struct.
     */
    public native void setResponseProperty(String name, String value);

    /**
     * Gets response properties from the srvhdrs pblock of the Request 
     * struct. This is handy if you want to see what responses are
     * going to be sent.
     */
    public native String getResponseProperty(String name);

    ////////////////////////////////////////////////////////////////////////////
    // Error Logging
    ////////////////////////////////////////////////////////////////////////////

    public static final int WARN = 0;
    public static final int MISCONFIG = 1;
    public static final int SECURITY = 2;
    public static final int FAILURE = 3;
    public static final int CATASTROPHE = 4;
    public static final int INFORM = 5;

    native void report(int degree, String name, String error);

    /**
    * Record a warning. name identifies your function to the administrator,
    * sn and rq are the Session and Request generating the error (these
    * can be null), and error is a string describing what went wrong.
     */
    public void warn(String error) { 
        report(WARN, name, error);
    }

    /**
     * Record a misconfiguration, or a missing or illegal parameter. name
     * identifies your function to the administrator,
     * sn and rq are the Session and Request generating the error (these
     * can be null), and error is a string describing what went wrong.
     */
    public void reportMisconfiguration(String error) { 
	report(MISCONFIG, name, error);
    }

    /**
     * Record a security violation, or someone trying to access a resource
     * they shouldn't. name identifies your function to the
     * administrator, sn and rq are the Session and Request generating the 
     * error (these can be null), and error is a string describing what went
     * wrong.
     */
    public void reportSecurity(String error) { 
        report(SECURITY, name, error);
    }

    /**
     * Record a general failure. name identifies your function to the
     * administrator, sn and rq are the Session and Request generating the 
     * error (these can be null), and error is a string describing what went
     * wrong.
     */
    public void reportFailure(String error) { 
       report(FAILURE, name, error);
    }

    /**
     * Record a catastrophic failure. name identifies your function to the
     * administrator, sn and rq are the Session and Request generating the 
     * error (these can be null), and error is a string describing what went
     * wrong.
     */
    public void reportCatastrophe(String error) { 
        report(CATASTROPHE, name, error);
    }

    /**
     * Record an informational message. name identifies your function to the
     * administrator, sn and rq are the Session and Request generating the 
     * error (these can be null), and error is a string describing what went
     * wrong.
     */
    public void inform(String error) { 
        report(INFORM, name, error);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Static Initialization
    ////////////////////////////////////////////////////////////////////////////

    static {
	try {
	    Socket.setSocketImplFactory(new NSAPISocketImplFactory());
	} catch (IOException e) {
	    System.err.println("Failed to set SocketImplFactory");
	}
    }
}
