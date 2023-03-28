/*
 * @(#)AppletViewer.java	1.68 95/12/08
 *
 * Copyright (c) 1994-1995 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

package sun.applet;

import java.util.*;
import java.io.*;
import java.awt.*;
import java.applet.*;
import java.net.URL;
import java.net.MalformedURLException;
import sun.awt.image.URLImageSource;
import java.lang.SecurityManager;

/**
 * A frame to show the applet tag in.
 */
class TextFrame extends Frame {
    /**
     * Create the tag frame.
     */
    TextFrame(int x, int y, String title, String text) {
	setTitle(title);
	TextArea txt = new TextArea(20, 60);
	txt.setText(text);
	txt.setEditable(false);

	add("Center", txt);

	Panel p = new Panel();
	add("South", p);
	p.add(new Button("Dismiss"));
	pack();
	move(x, y);
	show();
    }

    /**
     * Handle events.
     */
    public boolean handleEvent(Event evt) {
	switch (evt.id) {
	case Event.ACTION_EVENT:
	    if (!"Dismiss".equals(evt.arg)) {
		break;
	    }
	case Event.WINDOW_DESTROY:
	    dispose();
	    return true;

	}
	return false;
    }
}

/**
 * The toplevel applet viewer.
 */
public class AppletViewer extends Frame implements AppletContext {
    /**
     * The panel in which the applet is being displayed.
     */
    AppletViewerPanel panel;

    /**
     * The status line.
     */
    Label label;

    /**
     * Create the applet viewer
     */
    AppletViewer(int x, int y, URL doc, Hashtable atts) {
	setTitle("Applet Viewer: " + atts.get("code"));
	MenuBar mb = new MenuBar();
	Menu m = new Menu("Applet");
	m.add(new MenuItem("Restart"));
	m.add(new MenuItem("Reload"));
	m.add(new MenuItem("Clone"));
	m.add(new MenuItem("-"));
	m.add(new MenuItem("Tag"));
	m.add(new MenuItem("Info"));
	m.add(new MenuItem("Edit")).disable();
	m.add(new MenuItem("-"));
	m.add(new MenuItem("Properties"));
	m.add(new MenuItem("-"));
	m.add(new MenuItem("Close"));
	m.add(new MenuItem("Quit"));
	mb.add(m);
	setMenuBar(mb);
	reshape(x, y, 400, 200);
	
	add("Center", panel = new AppletViewerPanel(doc, atts));
	add("South", label = new Label("Hello..."));
	panel.init();
	appletPanels.addElement(panel);

	pack();
	show();


	// Start the applet
	showStatus("starting applet...");
	panel.sendEvent(AppletPanel.APPLET_LOAD);
	panel.sendEvent(AppletPanel.APPLET_INIT);
	panel.sendEvent(AppletPanel.APPLET_START);
    }

    static Hashtable audioHash = new Hashtable();

    /**
     * Get a clip from the static audio cache.
     */
    static synchronized AudioClip getAudioClipFromCache(URL url) {
	System.getSecurityManager().checkConnect(url.getHost(), url.getPort());
	AudioClip clip = (AudioClip)audioHash.get(url);
	if (clip == null) {
	    audioHash.put(url, clip = new AppletAudioClip(url));
	}
	return clip;
    }

    /**
     * Get an audio clip.
     */
    public AudioClip getAudioClip(URL url) {
	return getAudioClipFromCache(url);
    }

    static Hashtable imgHash = new Hashtable();

    /**
     * Get an image from the static image cache.
     */
    static synchronized Image getImageFromHash(URL url) {
	System.getSecurityManager().checkConnect(url.getHost(), url.getPort());
	Image img = (Image)imgHash.get(url);
	if (img == null) {
	    try {	
		imgHash.put(url, img = Toolkit.getDefaultToolkit().createImage(new URLImageSource(url)));
	    } catch (Exception e) {
	    }
	}
	return img;
    }

    /**
     * Get an image.
     */
    public Image getImage(URL url) {
	return getImageFromHash(url);
    }

    static Vector appletPanels = new Vector();

    /**
     * Get an applet by name.
     */
    public Applet getApplet(String name) {
	AppletSecurity security = (AppletSecurity)System.getSecurityManager();
	name = name.toLowerCase();
	for (Enumeration e = appletPanels.elements() ; e.hasMoreElements() ;) {
	    AppletPanel p = (AppletPanel)e.nextElement();
	    String param = p.getParameter("name");
	    if (param != null) {
		param = param.toLowerCase();
	    }
	    if (name.equals(param) && p.getDocumentBase().equals(panel.getDocumentBase())) {
		try {
		    security.checkConnect(panel.getCodeBase().getHost(), p.getCodeBase().getHost());
		    return p.applet;
		} catch (SecurityException ee) {
		}
	    }
	}
	return null;
    }

    /**
     * Return an enumeration of all the accessible
     * applets on this page.
     */
    public Enumeration getApplets() {
	AppletSecurity security = (AppletSecurity)System.getSecurityManager();
	Vector v = new Vector();
	for (Enumeration e = appletPanels.elements() ; e.hasMoreElements() ;) {
	    AppletPanel p = (AppletPanel)e.nextElement();
	    if (p.getDocumentBase().equals(panel.getDocumentBase())) {
		try {
		    security.checkConnect(panel.getCodeBase().getHost(), p.getCodeBase().getHost());
		    v.addElement(p.applet);
		} catch (SecurityException ee) {
		}
	    }
	}
	return v.elements();
    }

    /**
     * Ignore.
     */
    public void showDocument(URL url) {	
    }

    /**
     * Ignore.
     */
    public void showDocument(URL url, String target) {
    }

    /**
     * Show status.
     */
    public void showStatus(String status) {
	label.setText(status);
    }

    /**
     * System parameters.
     */
    static Hashtable systemParam = new Hashtable();

    static {
	systemParam.put("codebase", "codebase");
	systemParam.put("code", "code");
	systemParam.put("alt", "alt");
	systemParam.put("width", "width");
	systemParam.put("height", "height");
	systemParam.put("align", "align");
	systemParam.put("vspace", "vspace");
	systemParam.put("hspace", "hspace");
    }

    /**
     * Print the HTML tag.
     */
    static void printTag(PrintStream out, Hashtable atts) {
	out.print("<applet");

	String v = (String)atts.get("codebase");
	if (v != null) {
	    out.print(" codebase=\"" + v + "\"");
	}

	v = (String)atts.get("code");
	if (v == null) {
	    v = "applet.class";
	}
	out.print(" code=\"" + v + "\"");

	v = (String)atts.get("width");
	if (v == null) {
	    v = "150";
	}
	out.print(" width=" + v);

	v = (String)atts.get("height");
	if (v == null) {
	    v = "100";
	}
	out.print(" height=" + v);

	v = (String)atts.get("name");
	if (v != null) {
	    out.print(" name=\"" + v + "\"");
	}
	out.println(">");

	// A very slow sorting algorithm
	int len = atts.size();
	String params[] = new String[len];
	len = 0;
	for (Enumeration e = atts.keys() ; e.hasMoreElements() ;) {
	    String param = (String)e.nextElement();
	    int i = 0;
	    for (; i < len ; i++) {
		if (params[i].compareTo(param) >= 0) {
		    break;
		}
	    }
	    System.arraycopy(params, i, params, i + 1, len - i);
	    params[i] = param;
	    len++;
	}

	for (int i = 0 ; i < len ; i++) {
	    String param = params[i];
	    if (systemParam.get(param) == null) {
		out.println("<param name=" + param + " value=\"" + atts.get(param) + "\">");
	    }
	}
	out.println("</applet>");
    }

    /**
     * Make sure the atrributes are uptodate.
     */
    public void updateAtts() {
	Dimension d = panel.size();
	Insets in = panel.insets();
	panel.atts.put("width", new Integer(d.width - (in.left + in.right)).toString());
	panel.atts.put("height", new Integer(d.height - (in.top + in.bottom)).toString());
    }

    /**
     * Restart the applet.
     */
    void appletRestart() {
	panel.sendEvent(AppletPanel.APPLET_STOP);
	panel.sendEvent(AppletPanel.APPLET_DESTROY);
	panel.sendEvent(AppletPanel.APPLET_INIT);
	panel.sendEvent(AppletPanel.APPLET_START);
    }

    /**
     * Reload the applet.
     */
    void appletReload() {
	panel.sendEvent(AppletPanel.APPLET_STOP);
	panel.sendEvent(AppletPanel.APPLET_DESTROY);
	panel.sendEvent(AppletPanel.APPLET_DISPOSE);
	AppletPanel.flushClassLoader(panel.baseURL);
	panel.sendEvent(AppletPanel.APPLET_LOAD);
	panel.sendEvent(AppletPanel.APPLET_INIT);
	panel.sendEvent(AppletPanel.APPLET_START);
    }

    /**
     * Clone the viewer and the applet.
     */
    void appletClone() {
	Point p = location();
	updateAtts();
	new AppletViewer(p.x + 30, p.y + 10, panel.documentURL, (Hashtable)panel.atts.clone());
    }

    /**
     * Show the applet tag.
     */
    void appletTag() {
	ByteArrayOutputStream out = new ByteArrayOutputStream();
	updateAtts();
	printTag(new PrintStream(out), panel.atts);
	showStatus("Tag shown");

	Point p = location();
	new TextFrame(p.x + 50, p.y + 20, "Applet HTML Tag", out.toString());
    }

    /**
     * Show the applet tag.
     */
    void appletInfo() {
	String str = panel.applet.getAppletInfo();
	if (str == null) {
	    str = "-- no applet info --";
	}
	str += "\n\n";

	String atts[][] = panel.applet.getParameterInfo();
	if (atts != null) {
	    for (int i = 0 ; i < atts.length ; i++) {
		str += atts[i][0] + " -- " + atts[i][1] + " -- " + atts[i][2] + "\n";
	    }
	} else {
	    str += "-- no parameter info --";
	}
	
	Point p = location();
	new TextFrame(p.x + 50, p.y + 20, "Applet Info", str);
    }

    /**
     * Edit the applet.
     */
    void appletEdit() {
    }

    /**
     * Properties.
     */
    static AppletProps props;
    static synchronized void networkProperties() {
	if (props == null) {
	    props = new AppletProps();
	}
	props.show();
    }

    /**
     * Start the applet.
     */
    void appletStart() {
	panel.sendEvent(AppletPanel.APPLET_START);
    }

    /**
     * Stop the applet.
     */
    void appletStop() {
	panel.sendEvent(AppletPanel.APPLET_STOP);
    }

    /**
     * Close this viewer.
     */
    void appletClose() {
	panel.sendEvent(AppletPanel.APPLET_STOP);
	panel.sendEvent(AppletPanel.APPLET_DESTROY);
	panel.sendEvent(AppletPanel.APPLET_DISPOSE);
	panel.sendEvent(AppletPanel.APPLET_QUIT);
	appletPanels.removeElement(panel);
	dispose();

	if (appletPanels.size() == 0) {
	    System.exit(0);
	}
    }

    /**
     * Quit this all viewers.
     */
    void appletQuit() {
	System.exit(0);
    }

    /**
     * Handle events.
     */
    public boolean handleEvent(Event evt) {
	switch (evt.id) {
	  case AppletPanel.APPLET_RESIZE:
	    resize(preferredSize());
	    validate();
	    return true;

	  case Event.WINDOW_ICONIFY:
	    appletStop();
	    return true;

	  case Event.WINDOW_DEICONIFY:
	    appletStart();
	    return true;

	  case Event.WINDOW_DESTROY:
	    appletClose();
	    return true;

	  case Event.ACTION_EVENT:
	    if ("Restart".equals(evt.arg)) {
		appletRestart();
		return true;
	    }
	    if ("Reload".equals(evt.arg)) {
		appletReload();
		return true;
	    }
	    if ("Clone".equals(evt.arg)) {
		appletClone();
		return true;
	    } 
	    if ("Tag".equals(evt.arg)) {
		appletTag();
		return true;
	    }
	    if ("Info".equals(evt.arg)) {
		appletInfo();
		return true;
	    }
	    if ("Edit".equals(evt.arg)) {
		appletEdit();
		return true;
	    }
	    if ("Properties".equals(evt.arg)) {
		networkProperties();
		return true;
	    }
	    if ("Close".equals(evt.arg)) {
		appletClose();
		return true;
	    }
	    if ("Quit".equals(evt.arg)) {
		appletQuit();
		return true;
	    }
	    break;

	  case Event.MOUSE_ENTER:
	  case Event.MOUSE_EXIT:
	  case Event.MOUSE_MOVE:
	  case Event.MOUSE_DRAG:
	    return super.handleEvent(evt);
	}
	//System.out.println("evt = " + evt);
	return super.handleEvent(evt);
    }

    /**
     * Prepare the enviroment for executing applets.
     */
    public static void init() {
	SecurityManager.setScopePermission();
	Properties props = new Properties(System.getProperties());

	// Define a number of standard properties
	props.put("acl.read", "+");
	props.put("acl.read.default", "");
	props.put("acl.write", "+");
	props.put("acl.write.default", "");

	// Standard browser properties
	props.put("browser", "sun.applet.AppletViewer");
	props.put("browser.version", "1.04");
	props.put("browser.vendor", "Sun Microsystems Inc.");
	props.put("http.agent", "JDK/beta2");

	// These are needed to run inside Sun
	props.put("firewallSet", "true");
	props.put("firewallHost", "sunweb.ebay");
	props.put("firewallPort", "80");

	// Define which packages can be accessed by applets
	props.put("package.restrict.access.sun", "true");
	props.put("package.restrict.access.netscape", "true");

	// Define which packages can be extended by applets
	props.put("package.restrict.definition.java", "true");
	props.put("package.restrict.definition.sun", "true");
	props.put("package.restrict.definition.netscape", "true");

	// Define which properties can be read by applets.
	// A property named by "key" can be read only when its twin
	// property "key.applet" is true.  The following ten properties
	// are open by default.  Any other property can be explicitly
	// opened up by the browser user setting key.applet=true in
	// ~/.hotjava/properties.   Or vice versa, any of the following can
	// be overridden by the user's properties. 
	props.put("java.version.applet", "true");
	props.put("java.vendor.applet", "true");
	props.put("java.vendor.url.applet", "true");
	props.put("java.class.version.applet", "true");
	props.put("os.name.applet", "true");
	props.put("os.version.applet", "true");
	props.put("os.arch.applet", "true");
	props.put("file.separator.applet", "true");
	props.put("path.separator.applet", "true");
	props.put("line.separator.applet", "true");
	
	// Access controls for awt,etc system properties. We could either 
        // enpower the methods where these powers are accessed or 
        // we list them here, so that they can be accessed publicly.
	put("awt.image.incrementaldraw.applet", "true");
	put("awt.image.redrawrate.applet", "true");
	put("awt.toolkit.applet", "true");
	put("awt.appletWarning.applet", "true");
	put("awt.imagefetchers.applet", "true");

	put("console.bufferlength.applet", "true");

	// User properties list
	props = new Properties(props);

	// Try loading the hotjava properties file to override some
        // of the above defaults.
	try {
	    FileInputStream in = new FileInputStream(System.getProperty("user.home") + 
						     File.separator + ".hotjava" +
						     File.separator + "properties");
	    props.load(in);
	    in.close();
	} catch (Exception e) {
	    System.out.println("[no properties loaded, using defaults]");
	}

	// Install a property list.
	System.setProperties(props);

	// Create and install the security manager
	System.setSecurityManager(new AppletSecurity());

	// REMIND: Create and install a socket factory!
    }

    /**
     * The current character.
     */
    static int c;

    /**
     * Scan spaces.
     */
    static void skipSpace(InputStream in) throws IOException {
	while((c >= 0) &&
	      ((c == ' ') || (c == '\t') || (c == '\n') || (c == '\r'))) {
	    c = in.read();
	}
    }

    /**
     * Scan identifier
     */
    static String scanIdentifier(InputStream in) throws IOException {
	StringBuffer buf = new StringBuffer();
	while (true) {
	    if ((c >= 'a') && (c <= 'z')) {
		buf.append((char)c);
		c = in.read();
	    } else if ((c >= 'A') && (c <= 'Z')) {
		buf.append((char)('a' + (c - 'A')));
		c = in.read();
	    } else if ((c >= '0') && (c <= '9')) {
		buf.append((char)c);
		c = in.read();
	    } else {
		return buf.toString();
	    }
	}
    }

    /**
     * Scan tag
     */
    static Hashtable scanTag(InputStream in) throws IOException {
	Hashtable atts = new Hashtable();
	skipSpace(in);
	while (c >= 0 && c != '>') {
	    String att = scanIdentifier(in);
	    String val = "";
	    skipSpace(in);
	    if (c == '=') {
		int quote = -1;
		c = in.read();
		skipSpace(in);
		if ((c == '\'') || (c == '\"')) {
		    quote = c;
		    c = in.read();
		}
		StringBuffer buf = new StringBuffer();
		while ((c > 0) &&
		       (((quote < 0) && (c != ' ') && (c != '\t') && 
			 (c != '\n') && (c != '\r') && (c != '>'))
			|| ((quote >= 0) && (c != quote)))) {
		    buf.append((char)c);
		    c = in.read();
		}
		if (c == quote) {
		    c = in.read();
		}
		skipSpace(in);
		val = buf.toString();
	    }
	    //System.out.println("PUT " + att + " = '" + val + "'");
	    atts.put(att, val);
	    skipSpace(in);
	}
	return atts;
    }

    static int x = 100;
    static int y = 50;

    /**
     * Scan an html file for <applet> tags
     */
    static void parse(URL url) throws IOException {
	InputStream in = url.openStream();
	Hashtable atts = null;
	c = in.read();
	while (c >= 0) {
	    if (c == '<') {
		c = in.read();
		if (c == '/') {
		    c = in.read();
		    String nm = scanIdentifier(in);
		    if (nm.equals("applet")) {
			if (atts != null) {
			    new AppletViewer(x, y, url, atts);
			    x += 50;
			    y += 20;
			}
			atts = null;
		    }
		} else {
		    String nm = scanIdentifier(in);
		    if (nm.equals("param")) {
			Hashtable t = scanTag(in);
			String att = (String)t.get("name");
			if (att == null) {
			    System.out.println("Warning: <param name=... value=...> tag requires name attribute.");
			} else {
			    String val = (String)t.get("value");
			    if (val == null) {
				System.out.println("Warning: <param name=... value=...> tag requires value attribute.");
			    } else if (atts != null) {
				atts.put(att.toLowerCase(), val);
			    } else {
				System.out.println("Warning: <param> tag outside <applet> ... </applet>.");
			    }
			}
		    } else if (nm.equals("applet")) {
			atts = scanTag(in);
			if (atts.get("code") == null) {
			    System.out.println("Warning: <applet> tag requires code attribute.");
			    atts = null;
			} else if (atts.get("width") == null) {
			    System.out.println("Warning: <applet> tag requires width attribute.");
			    atts = null;
			} else if (atts.get("height") == null) {
			    System.out.println("Warning: <applet> tag requires height attribute.");
			    atts = null;
			}
		    } else if (nm.equals("app")) {
			System.out.println("Warning: <app> tag no longer supported, use <applet> instead:");
			Hashtable atts2 = scanTag(in);
			nm = (String)atts2.get("class");
			if (nm != null) {
			    atts2.remove("class");
			    atts2.put("code", nm + ".class");
			}
			nm = (String)atts2.get("src");
			if (nm != null) {
			    atts2.remove("src");
			    atts2.put("codebase", nm);
			}
			if (atts2.get("width") == null) {
			    atts2.put("width", "100");
			}
			if (atts2.get("height") == null) {
			    atts2.put("height", "100");
			}
			printTag(System.out, atts2);
			System.out.println();
		    }
		} 
	    } else {
		c = in.read();
	    }
	}
	in.close();
    }

    /**
     * Print usage
     */
    static void usage() {
	System.out.println("use: appletviewer [-debug] url|file ...");
    }

    /**
     * Main
     */
    public static void main(String argv[]) {
	init();

	// Parse arguments
	if (argv.length == 0) {
	    System.out.println("No input files specified.");
	    usage();
	    return;
	}

	SecurityManager.setScopePermission();
	// Show copyright notice unless it was shown before
	if (!"beta2".equals(System.getProperty("appletviewer.version"))) {
	    new AppletCopyright().waitForUser();
	}

	// Parse the documents
	for (int i = 0 ; i < argv.length ; i++) {
	    try {
		URL url;

		if (argv[i].indexOf(':') <= 1) {
		    url = new URL("file:" + System.getProperty("user.dir").replace(File.separatorChar, '/') + "/");
		    url = new URL(url, argv[i]);
		} else {
		    url = new URL(argv[i]);
		}

		parse(url);
	    } catch (MalformedURLException e) {
		System.out.println("Bad URL: " + argv[i]
				   + " (" + e.getMessage() + ")");
		System.exit(1);
	    } catch (IOException e) {
		System.out.println("I/O exception while reading: " + e.getMessage());
		if (argv[i].indexOf(':') < 0) {
		    System.out.println("Make sure that " + argv[i] + " is a file and is readable.");
		} else {
		    System.out.println("Is " + argv[i] + " the correct URL?");
		}
		System.exit(1);
	    }
	}
	if (appletPanels.size() == 0) {
	    System.out.println("Warning: No Applets were started, make sure the input contains an <applet> tag.");
	    usage();
	    System.exit(1);
	}
    }
}
