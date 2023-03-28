//
// CapTarget.java -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 16 July 1996
//

// $Id: CapTarget.java,v 1.5 1996/07/31 05:39:50 dwallach Exp $

package netscape.security;

import java.lang.*;
import java.util.*;

/**
 * This class represents a system target -- either a primitive device or
 * a macro ("typical game privileges").
 * <p>
 * Related documentation here:
 * <ul>
 * <a href="http://iapp15/java/signing/authorsigning.html">http://iapp15/java/signing/authorsigning.html</a>
 * </ul>
 *
 * @version 	$Id: CapTarget.java,v 1.5 1996/07/31 05:39:50 dwallach Exp $
 * @author	Dan Wallach
 */
public
class CapTarget {
    private String itsName;
    private CapPrincipal itsPrincipal;
    private boolean isRegistered;
    static private Hashtable theTargetRegistry = new Hashtable();

    private void registerCapTarget(String name, CapPrincipal p) {
	CapTarget t;

	itsName = name;
	itsPrincipal = p;
	isRegistered = false;

	//
	// security concern: Hashtable currently calls the equals() method
	// on objects already stored in the hash table.  This is good, because
	// it means an intruder can't hack a subclass of CapTarget with its
	// own equals function and get it registered.
	//
	// it's extremely important that Hashtable continues to work this way.
	//
	t = (CapTarget) theTargetRegistry.get(this);

	//
	// if the target is already registered, just return this one
	// without registering it.
	//
	if(t != null) return;

	//
	// otherwise, add the target to the registry
	//
	// TODO: make sure the caller has the given principal -- you
	// shouldn't be allowed to register a target under a principal
	// you don't own.
	//
	theTargetRegistry.put(this, this);
	isRegistered = true;
    }

    /**
     * Create a new target with the given name and principal and, if
     * it hasn't been registered, add it to the registry.  The calling
     * method must have the principal it's using to register the target.
     * If not, the target you get back will *not* be registered, but you
     * can still play with it yourself.
     *
     * Normally, applets won't create their own targets, but will instead
     * reference system targets.  However, a sufficiently complex applet
     * could use the capability system to manage security within itself.
     *
     * @param name Name of the target (e.g.: "system microphone")
     * @param p Principal who owns the target (e.g.: CapManager.getSystemPrincipal())
     *
     * @see CapManager#getSystemPrincipal
     */
    public CapTarget(String name, CapPrincipal p) {
	registerCapTarget(name, p);
    }

    /**
     * create a new target without a principal.  Any applet could
     * potentially create one of these, and maybe even (uggh) use it
     * as a method of inter-applet communication by subclassing
     * CapTarget.
     */
    public CapTarget(String name) {
	registerCapTarget(name, null);
    }

    /**
     * these methods attempt to find a pre-existing instance of
     * the desired target.  Eventually, principals will be able to
     * register their own targets (using the capability system
     * to manage their own resources), so the principal is an optional
     * second argument, used for disambiguation.  By default, only
     * the "system" principal is searched.
     */
    public static CapTarget getTarget(String name) {
	return getTarget(name, null);
    }

    public static CapTarget getTarget(String name, CapPrincipal p) {
	CapTarget t = (CapTarget)
	    theTargetRegistry.get(new CapTarget(name, p));
	return t;
    }

    /**
     * for "simple" targets, these will always return "blank"
     * (and the principal will store the "allowed" bit, if necessary),
     * but "smart" targets may do more interesting things.  In
     * particular, smart targets will subclass this class and
     * add more arguments to these functions.
     */
    public CapPermission checkScopePermission(CapPrincipal p) {
	return new CapPermission(CapPermission.BLANK, CapPermission.PERMANENT);
    }

    public CapPermission checkPrincipalPermission(CapPrincipal p) {
	return new CapPermission(CapPermission.BLANK, CapPermission.PERMANENT);
    }


    /**
     * generic utility functions
     */
    public boolean equals(Object obj) {
        boolean bSameName, bSamePrin;

	if(obj == this) return true;
	if(obj == null || this == null) return false;
        if(!(obj instanceof CapTarget)) return false;

        CapTarget t = (CapTarget) obj;

        //
        // this is deliberately inefficient so I can debug it
        //
        bSameName = (itsName.equals(t.itsName));

	//
	// this extra garbage is necessary because you can't
	// invoke a method on a null instance of a class, even
	// if that methods tries to check this == null first.
	// *sigh*
	//
	if(itsPrincipal == null)
	    bSamePrin = (t.itsPrincipal == null);
	else
	    bSamePrin = (itsPrincipal.equals(t.itsPrincipal));

        return bSameName && bSamePrin;
    }

    public String toString() {
	if(itsPrincipal != null)
	    return "Target: " + itsName + " Prin: " + itsPrincipal.toString();
	else
	    return "Target: " + itsName + " Prin: <none>";
    }

    public int hashCode() {
	return hashCode(itsName, itsPrincipal);
    }

    public int hashCode(String name, CapPrincipal prin) {
	return name.hashCode() +
	    ((prin != null) ?prin.hashCode() :0);
    }
}
