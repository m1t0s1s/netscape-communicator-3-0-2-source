/*
 * @(#)MimeEntry.java	1.8 95/08/22
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

class MimeEntry {
    String name;
    String command;
    String TempNameTemplate;
    MimeEntry next;
    boolean starred;

    MimeEntry (String nm, String cmd) {
	this(nm, cmd, null);
    }
    MimeEntry (String nm, String cmd, String tnt) {
	name = nm;
	command = cmd;
	TempNameTemplate = tnt;
	if (nm != null && nm.length() > 0 && nm.charAt(nm.length() - 1) == '/')
	    starred = true;
    }

    Object launch(java.net.URLConnection urlc, MimeTable mt) {
	if (command.equalsIgnoreCase("loadtofile")) {
	    try {
		return urlc.getInputStream();
	    } catch(Exception e) {
		return "Load to file failed:\n" + e;
	    }
	}
	if (command.equalsIgnoreCase("plaintext")) {
	    try {
		StringBuffer sb = new StringBuffer();
		InputStream is = urlc.getInputStream();
		int c;
		try {
		    while ((c = is.read()) >= 0)
			sb.append((char) c);
		} catch(IOException e) {
		}
		return sb.toString();
	    } catch(IOException e) {
		return "Failure fetching file:\n" + e.toString();
	    }
	}
	String message = command;
	int fst = message.indexOf(' ');
	if (fst > 0)
	    message = message.substring(0, fst);
	return new MimeLauncher(this, urlc, mt.TempTemplate(),
				message);
	// new MimeLauncher(this, is, u, mt.TempTemplate()).start();
	// String message = command;
	// int fst = message.indexOf(' ');
	// if (fst > 0)
	// message = message.substring(0, fst);
	// System.out.print("Launched " + message + "\n");
	// return null;
    }

    boolean matches(String type) {
	if (starred)
	    return type.startsWith(name);
	else
	    return type.equals(name);
    }
}
