/*
 * @(#)ServerParameters.java	1.4 95/08/22
 * 
 * Copyright (c) 1995 Sun Microsystems, Inc.  All Rights reserved Permission to
 * use, copy, modify, and distribute this software and its documentation for
 * NON-COMMERCIAL purposes and without fee is hereby granted provided that
 * this copyright notice appears in all copies. Please refer to the file
 * copyright.html for further important copyright and licensing information.
 * 
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF THE
 * SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY
 * LICENSEE AS A RESULT OF USING, MODIFYING OR DISTRIBUTING THIS SOFTWARE OR
 * ITS DERIVATIVES.
 */

package sun.net.www.httpd;


import java.io.*;
import java.util.*;
import java.tools.jsc.ScriptContext;
import java.tools.jsc.ScriptCommand;

/**
 * A class to hold the parameters used by an http server
 * @author  James Gosling
 */

class ServerParameters {
    ScriptContext psc;
    String configFileName = "httpd.conf";
    String logFile = null;
    String exportDirectory;
    String serverRoot;
    String hostName;
    String welcome;
    String userDir;
    String metaDir;
    String metaSuffix;
    String directory;
    String serverType;
    String user;
    String group;
    String serverAdmin;
    String errorLog;
    String accessLog;
    String pidFile;
    String serverName;

    String CacheRoot;
    boolean Caching;
    boolean CacheNoConnect;
    boolean CacheExpiryCheck;
    int CacheSize;
    boolean Gc;

    Object failObject = "fail";
    Object passObject = "pass";
    Object redirectObject = "pass";
    boolean directoryBrowsing;
    boolean directoryReadme;
    boolean verbose;
    boolean identityCheck;
    boolean alwaysWelcome;
    int maxContentLengthBuffer;
    int maxRamCacheEntryBuffer;
    int port = -1;
    RegexpPool mimeMap = new RegexpPool();
    RegexpPool rules = new RegexpPool();

    void clerror(String s) {
	System.out.print("Error in command line: " + s + "\n");
    }

    ServerParameters (String nm, String argv[]) {
	int sp = 0;
	serverName = nm;
	if (argv != null)
	    while (sp < argv.length) {
		String arg = argv[sp];
		if (arg != null)
		    if (arg.equals("-r") && sp + 1 < argv.length) {
			configFileName = argv[sp + 1];
			sp += 2;
		    } else if (arg.equals("-p") && sp + 1 < argv.length) {
			try {
			    port = Integer.parseInt(argv[sp + 1]);
			}
			catch(Exception e) {
			    clerror(argv[sp + 1]);
			};
			sp += 2;
		    } else if (arg.equals("-v")) {
			verbose = true;
			sp++;
		    } else if (arg.equals("-dy")) {
			directoryBrowsing = true;
			sp++;
		    } else if (arg.equals("-dt")) {
			directoryReadme = true;
			sp++;
		    } else if (arg.startsWith("-"))
			clerror(arg);
		    else if (exportDirectory == null)
			exportDirectory = arg;
		    else
			clerror(arg + " (only one directory spec allowed)");
	    }
	config(configFileName);
	if (exportDirectory != null)
	    rules.add("*", new mapName(exportDirectory + "/*", null));
    }

    void addMime(String ext, String mtct) {
	if (ext.indexOf('*') < 0)
	    ext = "*" + ext;
	mimeMap.add(ext, mtct);
    }
    String find(String name, String df) {
	Object o = psc.get(name);
	if (o == null)
	    return df;
	return o.toString();
    }
    int find(String name, int df) {
	Object o = psc.get(name);
	if (o == null)
	    return df;
	if (o instanceof Number)
	    return ((Number) o).intValue();
	System.out.print(o + ": the value for " + name + " must be a number\n");
	return df;
    }
    boolean find(String name, boolean df) {
	Object o = psc.get(name);
	if (o == null)
	    return df;
	return o.equals("true") || o.equals("on");
    }
    void config(String cnm) {
	psc = new ScriptContext("sun.net.www.httpd.cmd_");
	ScriptCommand vararg = new VariableArg();
	psc.defun("port", vararg);
	psc.defun("serverroot", vararg);
	psc.defun("hostname", vararg);
	psc.defun("identitycheck", vararg);
//	psc.defun("verbose", vararg);
	psc.defun("welcome", vararg);
	psc.defun("alwayswelcome", vararg);
	psc.defun("userdir", vararg);
	psc.defun("metadir", vararg);
	psc.defun("metasuffix", vararg);
	psc.defun("maxcontentlengthbuffer", vararg);
	psc.defun("maxramcacheentrybuffer", vararg);
	psc.defun("servertype", vararg);
	psc.defun("user", vararg);
	psc.defun("group", vararg);
	psc.defun("serveradmin", vararg);
	psc.defun("serverroot", vararg);
	psc.defun("accesslog", vararg);
	psc.defun("errorlog", vararg);
	psc.defun("pidfile", vararg);
	psc.defun("servername", vararg);

	psc.defun("cacheroot", vararg);
	psc.defun("cachesize", vararg);
	psc.defun("caching", vararg);
	psc.defun("cachenoconnect", vararg);
	psc.defun("cacheexpirycheck", vararg);
	psc.defun("gc", vararg);

	ScriptCommand mimearg = new MimeArg();
	psc.defun("addtype", mimearg);
	psc.defun("suffix", mimearg);
	ScriptCommand maparg = new MapArg();
	psc.defun("fail", maparg);
	psc.defun("map", maparg);
	psc.defun("redirect", maparg);
	psc.defun("pass", maparg);
	psc.defun("exec", maparg);
	psc.defun("plugin", maparg);
	psc.put("ctx", this);
	try {
	    psc.parseEval(new BufferedInputStream(new FileInputStream(cnm)));
	} catch(Exception e) {
	    System.out.print("Couldn't read " + cnm + " (" + e + ")\n");
	    return;
	}
	verbose = psc.verbose;
	if (port < 0)
	    port = find("port", 80);
	if (directory == null)
	    directory = "/Public";

	CacheRoot = find("cacheroot", null);
	CacheSize = find("cachesize", 0);
	Caching = find("caching", CacheRoot != null);
	CacheNoConnect = find("cachnoconnect", false);
	CacheExpiryCheck = find("cacheexpirycheck", true);
	if (CacheRoot == null || CacheRoot.length()<=0)
	    Caching = false;
	Gc = find("gc", Caching);

	serverRoot = find("serverroot", "UNKNOWN");
	hostName = find("hostname", "UNKNOWN");
	identityCheck = find("identitycheck", false);
	verbose = find("verbose", verbose);
	welcome = find("welcome", "index.html");
	alwaysWelcome = find("alwayswelcome", false);
	userDir = find("userdir", "public_html");
	metaDir = find("metadir", ".web");
	metaSuffix = find("metasuffix", ".meta");
	maxContentLengthBuffer = find("maxcontentlengthbuffer", 50000);
	maxRamCacheEntryBuffer = find("maxramcacheentrybuffer", 8000);
	serverType = find("servertype", "standalone");
	user = find("user", "nobody");
	group = find("group", "nogroup");
	serverAdmin = find("serveradmin", "postmaster");
	serverRoot = find("serverroot", "/opt/httpd");
	accessLog = find("accesslog", "logs/access_log");
	errorLog = find("errorlog", "logs/error_log");
	pidFile = find("pidfile", "httpd-pid");
	serverName = find("servername", "CafeJava 0.1");
    }

    synchronized Object applyRules(String s) {
	Object v;
	rules.reset();
	boolean pass = false;
	if (s.length() > 1 && s.charAt(1) == '~') {
	    /* ~uname */
	    int sl = s.indexOf('/', 2);
	    String uname;
	    String tail;
	    if (sl < 0) {
		uname = s.substring(2);
		tail = "/";
	    } else {
		uname = s.substring(2, sl);
		tail = s.substring(sl);
	    }
	    s = "/home/" + uname + "/public_html" + tail;
	    pass = true;
	}
	while ((v = rules.matchNext(s)) != null) {
	    if (v == failObject) {
		if (!pass)
		    return null;
	    } else if (v == passObject)
		pass = true;
	    else if (v instanceof String) {
		if (!pass)
		    s = (String) v;
	    } else {
		if (v instanceof pass) {
		    pass p = (pass) v;
		    if (p.o == passObject) {
			if (!pass)
			    s = p.s;
			pass = true;
			continue;
		    } else if (p.o == null) {
			if (!pass)
			    s = p.s;
			continue;
		    }
		}
		return v;
	    }
	}
	return s;
    }

    public String mimeType(String fn) {
	Object r = mimeMap.match(fn);
	if (r == null || !(r instanceof String))
	    return "Unknown";
	return (String) r;
    }

    public String toString() {
	return "configFileName=" + configFileName +
	    "\nlogFile=" + logFile +
	    "\nexportDirectory=" + exportDirectory +
	    "\nserverRoot=" + serverRoot +
	    "\nwelcome=" + welcome +
	    "\nuserDir=" + userDir +
	    "\nmetaDir=" + metaDir +
	    "\nmetaSuffix=" + metaSuffix +
	    "\nserverName=" + serverName +
	    "\ndirectory=" + directory +
	    "\ndirectoryBrowsing=" + directoryBrowsing +
	    "\ndirectoryReadme=" + directoryReadme +
	    "\nverbose=" + verbose +
	    "\nidentityCheck=" + identityCheck +
	    "\nalwaysWelcome=" + alwaysWelcome +
	    "\nmaxContentLengthBuffer=" + maxContentLengthBuffer +
	    "\nport=" + port + "\n";
    }
    public void print() {
	System.out.print(toString());
	System.out.print("Rules: ");
	rules.print();
	System.out.print("\n");
	System.out.print("mimeMap: ");
	mimeMap.print();
	System.out.print("\n");
    }
}

class mapName implements RegexpTarget {
    private String RE;
    boolean nonre;
    boolean head;
    Object attached;
    mapName (String s, Object a) {
	attached = a;
	if (s == null || s.length() == 0) {
	    RE = "";
	    nonre = true;
	} else if (s.charAt(0) == '*') {
	    nonre = false;
	    head = true;
	    RE = s.substring(1);
	} else if (s.charAt(s.length() - 1) == '*') {
	    nonre = false;
	    head = false;
	    RE = s.substring(0, s.length() - 1);
	} else {
	    nonre = true;
	    RE = s;
	}
    }
    public Object found(String wild) {
	wild = (nonre ? RE
		: (head ? wild + RE
		   : RE + wild));
	return (attached != null
		? (Object) new pass(wild, attached)
		: (Object) wild);
    }
}

class pass {
    String s;
    Object o;
    pass (String S, Object O) {
	s = S;
    }
}

class execName implements RegexpTarget {
    private String prefix;
    execName (String s) {
	if (s == null || s.length() == 0 || s.charAt(s.length() - 1) != '*')
	    throw new Exception("exec scriptnames must end with *");
	prefix = s.substring(0, s.length() - 1);
    }
    public Object found(String wild) {
	int slash = wild.indexOf('/');
	String path = null;
	String pathInfo = null;
	if (slash < 0)
	    path = prefix + wild;
	else {
	    path = prefix + wild.substring(0, slash);
	    pathInfo = wild.substring(slash);
	}
	return new pass (path, pathInfo);
    }
}

class execPlugin implements RegexpTarget {
    private Object args[];
    Class who;
    execPlugin (ScriptContext ctx) {
	int nargs = ctx.nargs()-2;
	if (nargs<0)
	    throw new Exception("Class name expected");
	who = Class.forName(ctx.arg(1,"NoName"));
	if (nargs>0) {
	    args = new Object[nargs];
	    for (int i = 0; i<nargs; i++)
		args[i] = ctx.evarg(i+2);
	}
	ServerPlugin p;
	try {
	    p = (ServerPlugin) who.newInstance();
	} catch(Exception e) {
	    throw new Exception(who.getName() + " should be a subclass of sun.net.www.httpd.ServerPlugin");
	}
	p.init(null, args);
	String msg = p.validate();
	if (msg != null)
	    throw new Exception(msg);
    }
    public Object found(String wild) {
	ServerPlugin p = (ServerPlugin) who.newInstance();
	p.init(wild, args);
	return p;
    }
}

class MimeArg implements ScriptCommand {
    public Object eval(ScriptContext ctx) {
	ServerParameters sp = (ServerParameters) ctx.get("ctx");
	sp.addMime(ctx.arg(0, ""), ctx.arg(1, "text/plain"));
	return null;
    }
}

class MapArg implements ScriptCommand {
    public Object eval(ScriptContext ctx) {
	ServerParameters sp = (ServerParameters) ctx.get("ctx");
	String cmd = ctx.command();
	if (cmd.equalsIgnoreCase("fail"))
	    sp.rules.add(ctx.arg(0, ""), sp.failObject);
	else if (cmd.equalsIgnoreCase("map"))
	    sp.rules.add(ctx.arg(0, ""), new mapName (ctx.arg(1, ""), null));
	else if (cmd.equalsIgnoreCase("redirect"))
	    sp.rules.add(ctx.arg(0, ""),
			 new mapName (ctx.arg(1, ""),
				      sp.redirectObject));
	else if (cmd.equalsIgnoreCase("pass")) {
	    if (ctx.nargs() <= 1)
		sp.rules.add(ctx.arg(0, ""), sp.passObject);
	    else
		sp.rules.add(ctx.arg(0, ""), new mapName (ctx.arg(1, ""), sp.passObject));
	} else if (cmd.equalsIgnoreCase("exec"))
	    sp.rules.add(ctx.arg(0, ""), new execName (ctx.arg(1, "")));
	else if (cmd.equalsIgnoreCase("plugin"))
	    sp.rules.add(ctx.arg(0, ""), new execPlugin (ctx));
	else
	    throw new Exception("Undefined map command: " + cmd);
	return null;
    }
}

/**
 * A class to implements commands that just set the value of
 * a variable.
 * @author  James Gosling
 */

class VariableArg implements ScriptCommand {
    public Object eval(ScriptContext ctx) {
	ctx.put(ctx.command(), ctx.evarg(0));
	return null;
    }
}
