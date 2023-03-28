/*
 * @(#)plain.java	1.7 95/08/22
 * 
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for NON-COMMERCIAL purposes and without fee is hereby
 * granted provided that this copyright notice appears in all copies. Please
 * refer to the file "copyright.html" for further important copyright and
 * licensing information.
 * 
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
 * SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY
 * LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR
 * ITS DERIVATIVES.
 */

/*
 * Plain text file handler
 */
package sun.net.www.content.text;
import java.net.*;
import java.io.*;

public class plain extends ContentHandler {
    public Object getContent(URLConnection uc) {
	try {
	    InputStream is = uc.getInputStream();
	    StringBuffer sb = new StringBuffer();
	    int c;
	    while ((c = is.read()) >= 0) {
		sb.append((char) c);
	    }
	    is.close();
	    return sb.toString();
	} catch(IOException e) {
	    return "Error reading document:\n" + e.toString();
	}
    }
}
