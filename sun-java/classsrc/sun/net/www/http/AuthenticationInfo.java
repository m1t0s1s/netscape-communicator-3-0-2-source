/*
 * @(#)AuthenticationInfo.java	1.6 95/09/18 Jonathan Payne
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
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

package sun.net.www.http;

import java.net.URL;
import java.util.Hashtable;

public class AuthenticationInfo {
    String  host;
    int	    port;
    String  realm;
    String  auth;

    static Hashtable	cache = new Hashtable();
    static Hashtable 	preemptiveCache = new Hashtable();

    public static void cacheInfo(AuthenticationInfo info) {
	cache.put(info, info);
    }

    public static void uncacheInfo(AuthenticationInfo info) {
	cache.remove(info);
    }

    public static void cacheInfo(AuthenticationInfo info, URL url) {
	cacheInfo(info);
	preemptiveCache.put(url, info);
    }

    public static void uncacheInfo(AuthenticationInfo info, URL url) {
	uncacheInfo(info);
	preemptiveCache.remove(url);
    }

    public static AuthenticationInfo getAuth(URL url) {
	AuthenticationInfo info = (AuthenticationInfo)
	    preemptiveCache.get(url);
	return info;
    }
    
    public static AuthenticationInfo getAuth(URL url, String realm) {
	AuthenticationInfo  info = (AuthenticationInfo)
	    cache.get(new AuthenticationInfo(url.getHost(),
					     url.getPort(), realm));

	return info;
    }

    public AuthenticationInfo(URL url, String realm, String auth) {
	this(url, auth);
	this.realm = realm;
    }

    public AuthenticationInfo(URL url, String auth) {
	this.realm = "";
	this.host = url.getHost();
	this.port = url.getPort();
	this.auth = auth;
	cacheInfo(this, url);
    }
    
    public AuthenticationInfo(String host, int port, String realm) {
	this.host = host;
	this.port = port;
	this.realm = realm;
    }

    public AuthenticationInfo(String host, int port, String realm, String auth) {
	this(host, port, realm);
	this.auth = auth;
	cacheInfo(this);
    }

    public int hashCode() {
	return host.hashCode() ^ port ^ realm.hashCode();
    }


    public boolean equals(Object o) {
	if (o instanceof AuthenticationInfo) {
	    AuthenticationInfo	i = (AuthenticationInfo) o;

	    return i.host.equals(host) && i.port == port &&
		i.realm.equals(realm);
	}
	return false;
    }

    public String toString() {
	return "AuthenticationInfo[" + realm + "@" + host + ":" + port + "] -> "+
	    auth;
	   
    }
}
