//
// CapPermTable.java -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 16 July 1996
//

// $Id: CapPermTable.java,v 1.4 1996/07/31 05:39:48 dwallach Exp $

package netscape.security;

import java.lang.*;
import java.util.*;
import java.io.*;

/**
 * This class is used by CapManager to manage simple user-specified
 * permissions.  There should be one of these per principal, and they
 * map targets to permissions.
 *
 * Related documentation here:
 * <ul>
 * <a href="http://iapp15/java/signing/authorsigning.html">http://iapp15/java/signing/authorsigning.html</a>
 * </ul>
 *
 * @version 	$Id: CapPermTable.java,v 1.4 1996/07/31 05:39:48 dwallach Exp $
 * @author	Dan Wallach
 */
public
class CapPermTable extends Dictionary {
    //
    // implementation note: rather than just extending Hashtable, I'm
    // delegating to a Hashtable.  In most cases, these tables will
    // be completely empty.  Hashtables take up a fair amount of memory,
    // even when nothing is in them.  CapPermTable will allocate a Hashtable
    // the first time you store something in it.  Otherwise, it just returns
    // the default permission ("blank forever"), and consumes a whole lot
    // less RAM.
    //
    private Hashtable itsTable;
    static private Hashtable theEmptyTable;

    static {
	//
	// another terrible hack, but saves buckets of memory down the road
	// when we need to return empty Hashtable enumerations.
	//
	theEmptyTable = new Hashtable();
    }

    public CapPermTable() {
	itsTable=null;
    }

    public int size() {
	if(itsTable != null) return itsTable.size();
	else return 0;
    }

    public boolean isEmpty() {
	if(itsTable == null) return true;
	else return itsTable.isEmpty();
    }

    public Enumeration keys() {
	if(itsTable == null) return theEmptyTable.keys();
	else return itsTable.keys();
    }

    public Enumeration elements() {
	if(itsTable == null) return theEmptyTable.elements();
	else return itsTable.keys();
    }

    public String toString() {
	StringBuffer sb = new StringBuffer();
	Enumeration e = keys();

        while(e.hasMoreElements()) {
            CapTarget t = (CapTarget) e.nextElement();
            sb.append(t.toString());
	    sb.append("\n    ");
            sb.append(get(t).toString());
	    sb.append("\n");
        }

	return sb.toString();
    }

    public Object get(Object t) {
	if(! (t instanceof CapTarget)) return null;
	else return get((CapTarget) t);
    }

    public CapPermission get(CapTarget t) {
	if(itsTable == null)
	    return CapPermission.getPermission(CapPermission.BLANK,
					       CapPermission.PERMANENT);

	CapPermission p = (CapPermission) itsTable.get(t);
	if(p == null)
	    return CapPermission.getPermission(CapPermission.BLANK,
					       CapPermission.PERMANENT);
	else
	    return p;
    }

    //
    // This is such a lame hack to get around Java's lack of
    // type polymorphism.  All I want is a typed Hashtable.  *sigh*
    //

    public Object put(Object key, Object value) {
	if(!(key instanceof CapTarget) || !(value instanceof CapPermission))
	    return null;
	else
	    return put((CapTarget)key, (CapPermission)value);
    }

    public CapPermission put(CapTarget key, CapPermission perm) {
	if(itsTable == null) itsTable = new Hashtable();
	return (CapPermission) itsTable.put(key, perm);
    }

    public Object remove(Object key) {
	if(itsTable == null) return null;
	return itsTable.remove(key);
    }

    public CapPermission remove(CapTarget key) {
	if(itsTable == null) return null;
	return (CapPermission) itsTable.remove(key);
    }
}
