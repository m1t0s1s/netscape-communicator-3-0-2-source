/*
 * @(#)Identifier.java	1.9 95/09/14 Arthur van Hoff
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

package sun.tools.java;

import java.util.Hashtable;
import java.io.PrintStream;
import java.util.Enumeration;

/**
 * A class to represent identifiers.<p>
 *
 * An identifier instance is very similar to a String. The difference
 * is that identifier can't be instanciated directly, instead they are
 * looked up in a hash table. This means that identifiers with the same
 * name map to the same identifier object. This makes comparisons of
 * identifiers much faster.<p>
 *
 * A lot of identifiers are qualified, that is they have '.'s in them.
 * Each qualified identifier is chopped up into the qualifier and the
 * name. The qualifier is cached in the value field.<p>
 *
 * Unqualified identifiers can have a type. This type is an integer that
 * can be used by a scanner as a token value. This value has to be set
 * using the setType method.<p>
 *
 * @author 	Arthur van Hoff
 * @version 	1.9, 14 Sep 1995
 */

public final
class Identifier implements Constants {
    /**
     * The hashtable of identifiers
     */
    static Hashtable hash = new Hashtable(3001, 0.5f);

    /**
     * The name of the identifier
     */
    String name;

    /**
     * The value of the identifier, for keywords this is an
     * instance of class Integer, for qualified names this is
     * another identifier (the qualifier).
     */
    Object value;

    /**
     * Construct an identifier. Don't call this directly,
     * use lookup instead.
     * @see Identifier.lookup
     */
    private Identifier(String name) {
	this.name = name;
    }

    /**
     * Get the type of the identifier.
     */
    int getType() {
	return ((value != null) && (value instanceof Integer)) ?
	    	((Integer)value).intValue() : IDENT;
    }

    /**
     * Set the type of the identifier.
     */
    void setType(int t) {
	value = new Integer(t);
	//System.out.println("type(" + this + ")=" + t);
    }

    /**
     * Lookup an identifier.
     */
    public static synchronized Identifier lookup(String s) {
	//System.out.println("lookup(" + s + ")");
	Identifier id = (Identifier)hash.get(s);
	if (id == null) {
	    hash.put(s, id = new Identifier(s));
	}
	return id;
    }

    /**
     * Lookup a qualified identifier.
     */
    public static Identifier lookup(Identifier q, Identifier n) {
	Identifier id = lookup(q + "." + n);
	id.value = q;
	return id;
    }

    /**
     * Convert to a string.
     */
    public String toString() {
	return name;
    }

    /**
     * Check if the name is qualified (ie: it contains a '.').
     */
    public boolean isQualified() {
	if (value == null) {
	    int index = name.lastIndexOf('.');
	    value = (index < 0) ? idNull : Identifier.lookup(name.substring(0, index));
	}
	return (value instanceof Identifier) && (value != idNull);
    }

    /**
     * Return the qualifier. The null identifier is returned if
     * the name was not qualified.
     */
    public Identifier getQualifier() {
	return isQualified() ? (Identifier)value : idNull;
    }

    /**
     * Return the unqualified name.
     */
    public Identifier getName() {
	return isQualified() ?
	    Identifier.lookup(name.substring(((Identifier)value).name.length() + 1)) : this;
    }
}
