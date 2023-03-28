/*
 * @(#)MimeLauncher.java	1.11 95/08/22  James Gosling
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

package sun.net.www;
import java.net.URL;
import java.io.*;

class MimeLauncher extends Thread {
    java.net.URLConnection uc;
    MimeEntry m;
    String GenericTempTemplate;

    MimeLauncher (MimeEntry M, java.net.URLConnection uc, String gtt,
		  String name) {
	super(name);
	m = M;
	this.uc = uc;
	GenericTempTemplate = gtt;
    }

    public void run() {
	try {
	    InputStream is = uc.getInputStream();
	    String c = m.command;
	    int inx = 0;
	    boolean substituted = false;
	    String ofn = m.TempNameTemplate;
	    if (ofn == null)
		ofn = GenericTempTemplate;
	    while ((inx = ofn.indexOf("%s")) >= 0)
		ofn = ofn.substring(0, inx) + (System.currentTimeMillis() / 1000) + ofn.substring(inx + 2);
	    try {
		OutputStream os = new FileOutputStream(ofn);
		byte buf[] = new byte[2048];
		int i;
		try {
		    while ((i = is.read(buf)) >= 0)
			os.write(buf, 0, i);
		} catch(IOException e) {
		}
		os.close();
	    } catch(IOException e) {
	    }
	    is.close();
	    while ((inx = c.indexOf("%t")) >= 0)
		c = c.substring(0, inx) + uc.getContentType() + c.substring(inx + 2);
	    while ((inx = c.indexOf("%s")) >= 0) {
		c = c.substring(0, inx) + ofn + c.substring(inx + 2);
		substituted = true;
	    }
	    if (!substituted)
		c = c + " <" + ofn;
	    Runtime.getRuntime().exec(c);
	} catch(IOException e) {
	}
    }
}
