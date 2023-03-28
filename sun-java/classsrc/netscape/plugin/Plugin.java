/* -*- Mode: C; tab-width: 4; -*- */
/*
 * @@(#)Plugin.java	1.4 95/07/31
 * 
 * Copyright (c) 1996 Netscape Communications Corporation. All Rights Reserved.
 * 
 */

package netscape.plugin;

import netscape.javascript.JSObject;

/**
 * This class represents the Java reflection of a plugin. Plugins which wish
 * to have Java methods associated with them should subclass this class and 
 * add new (possibly native) methods to it. This allows other Java entities
 * (such as applets and JavaScript code) to manipulate the plugin.
 */
public
class Plugin {

	int peer;

	/**
	 * Returns the native NPP object -- the plugin instance which is
	 * the native part of a Java Plugin object. This field is set by the
	 * system, but can be read from plugin native methods by calling:
	 * <pre>
	 *   NPP npp = (NPP)netscape_plugin_Plugin_getPeer(env, thisPlugin);
	 * </pre>
	 */
	public int getPeer() {
		return peer;
	}

    /**
     * Called when the plugin is initialized. You never need to call this 
	 * method directly, it is called when the plugin is created.
     * @see #destroy
     */
	public void init() {
	}

    /**
     * Called when the plugin is destroyed. You never need to call this 
	 * method directly, it is called when the plugin is destroyed. At the
	 * point this method is called, the plugin will still be active.
     * @see #init
     */
	public void destroy() {
	}

	/**
	 * This method determines whether the Java reflection of a plugin still
	 * refers to an active plugin. Plugin instances are destroyed whenever 
	 * the page containing the plugin is left, thereby causing the plugin
	 * to no longer be active. 
	 */
	public boolean isActive() {
		return peer != 0;
	}

	JSObject window;

	/**
	 * Returns the JavaScript window on which the plugin is embedded.
	 */
	public JSObject getWindow() {
		return window;
	}
}
