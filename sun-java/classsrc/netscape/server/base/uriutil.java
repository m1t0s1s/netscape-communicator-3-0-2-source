

package netscape.server.base;

import netscape.server.base.pb_param;

import java.lang.String;
import java.util.Vector;


/**
 * uriutil provides utilities for dealing with URIs in ways a typical server
 * application might need to. The terms used in this document are as follows:
 *
 * URL: A fully specified Location of a document, including protocol and
 * host name. Port is optional. Example: http://foobar.org/baz/
 *
 * URI: A partial specification of a document, or everything following and
 * including the third slash in the URL. Example: /baz/
 *
 * @author	Rob McCool
 */

public class uriutil {
    private uriutil() {}

    /**
      uri_is_evil checks a URI to make sure it is not going to be used to
      circumvent server access controls. It scans for any instance of ../
      ./ or // in the URI and returns true when the URI is potentially 
      malicious, or false when it seems okay. Although such URIs are legal,
      the Netscape server will not allow them, and will not parse them
      into other forms because it wants to make sure that only one URL can
      refer to each document.
     */
    public static boolean uri_is_evil(String uri) {
        int x, l = uri.length();

        if(uri.charAt(0) != '/')
            return true;
        for(x = 0; x < (l - 1); ++x) {
            if(uri.charAt(x) == '/') {
                if(uri.charAt(x+1) == '/')
                    return true; /* double slashes are evil */
                else if(uri.charAt(x+1) == '.') {
                    if(((x + 2) == l) || (uri.charAt(x+2) == '/'))
                        return true; /* /./ or ends with /. */
                    else if(uri.charAt(x+2) == '.') {
                        if(((x+3) == l) || (uri.charAt(x+3) == '/'))
                            return true; /* /../ or ends with /.. */
                    }
                }
            }
        }
        return false;
    }
    private static String urx_escape(String echars, String urx) {
        StringBuffer t = new StringBuffer();
        char c;
        int x;
        for(x = 0; x < urx.length(); ++x) {
            c = urx.charAt(x);
            if(echars.indexOf(c) != -1) {
                t.append('%');
                t.append(Integer.toString((int) c & 0xff, 16));
            }
            else t.append(c);
        }
        return t.toString();
    }
    /**
      uri_escape takes an arbitrary string, and escapes it using the URL
      conventions. The ?, #, and : characters will be escaped, which is
      why this function should not be used for URLs. Returns a new string
      with characters that should be encoded replaced with %xx per URL
      conventions.
     */
    public static String uri_escape(String uri) {
        return urx_escape("% ?#:+&*\"<>\r\n", uri);
    }
    /**
      url_escape takes an arbitrary string, and escapes it using the URL 
      conventions. The ?, #, and : characters are not escaped, making this
      function suitable for URLs. Returns a new string with characters that
      should be encoded replaced with %xx per URL conventions.
     */
    public static String url_escape(String url) {
        return urx_escape("% +*\"<>\r\n", url);
    }

    /**
      url_unescape takes a string which is encoded using URL conventions,
      and converts any %xx characters into their character equivalents. 
      Returns a new String with the decoded message.

      Throws NumberFormatException when given a string with % characters
      but no number after it, such as foo%bar.
     */
    public static String url_unescape(String url) 
        throws NumberFormatException 
    {
        StringBuffer t = new StringBuffer();
        char c;
        int x, l = url.length();

        for(x = 0; x < l; ++x) {
            c = url.charAt(x);
            if((c == '%') && (x < l - 2)) {
                char numstr[] = new char[2];
                int num;

                numstr[0] = url.charAt(++x);
                numstr[1] = url.charAt(++x);
                num = Integer.parseInt(new String(numstr), 16);
                t.append((char) (num & 0xff));
            }
            else if(c == '+')
                t.append(' ');
            else
                t.append(c);
        }
        return t.toString();
    }

    /**
      formdata_split takes a string of form data, with each name=value pair
      separated by & characters, places them into a new array of pb_params,
      and decodes any encoded characters in the strings.
     */
    public static pb_param[] formdata_split(String data) {
        Vector v = new Vector();
        pb_param[] ret;
        int p, q, x;

        for(p = 0, q = 0; q != data.length(); p = q + 1) {
            q = data.indexOf((int) '&', p);
            if(q == -1)
                q = data.length();
            v.addElement(data.substring(p, q));
        }
        ret = new pb_param[v.size()];
        for(x = 0; x < ret.length; ++x) {
            String name, value, t;

            t = (String) v.elementAt(x);
            p = t.indexOf((int) '=');
            name = url_unescape(t.substring(0, p));
            value = url_unescape(t.substring(p+1));
            ret[x] = new pb_param(name, value);
        }
        return ret;
    }
}
