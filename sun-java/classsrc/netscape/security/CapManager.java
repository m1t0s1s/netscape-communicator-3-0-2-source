//
// CapManager.java -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 16 July 1996
//

// $Id: CapManager.java,v 1.10 1996/08/02 01:59:23 dwallach Exp $

package netscape.security;

import java.io.*;
import java.net.*;
import java.util.*;
import java.lang.*;

import netscape.security.*;

/**
 * This class implements our capability-based security policy.
 * Related documentation here:
 * <ul>
 * <a href="http://iapp15/java/signing/authorsigning.html">http://iapp15/java/signing/authorsigning.html</a>
 * </ul>
 *
 * @version 	$Id: CapManager.java,v 1.10 1996/08/02 01:59:23 dwallach Exp $
 * @author	Dan Wallach
 */
public
class CapManager {
    private Hashtable itsPrinToPermTable;
    private CapPrincipal itsSystemPrincipal;

    /**
     * create a CapManager and seed it with the system principal.  Future
     * principals are added by the ClassLoader (not the applet programmer).
     */
    public CapManager() {
	// right now, initialization is a hack until we have cryptographic
	// credentials directly associated with a Class record on the stack
	//
	// TODO: fix this!

	//
	// assumption: the first principal on *this* class will be the
	// the system principal
	//
	// itsSystemPrincipal = getClassPrincipalsFromStackUnsafe(0)[0];

	itsSystemPrincipal = new CapPrincipal(CapPrincipal.CERT,
					      "de:ad:be:ef:ca:fe:ba:be");

	itsPrinToPermTable = new Hashtable();
	itsPrinToPermTable.put(itsSystemPrincipal, new CapSystemPermTable());
    }

    //////////////////////////////////////////////////////////////////////
    // Methods for managing principal and scope capabilities
    //////////////////////////////////////////////////////////////////////

    /**
     * some user-specified capabilities expire when a Web page terminates
     * or when the browser exits.  This method cleans up these principal
     * capabilities.  This is not intended to be invoked directly by applets.
     *
     * @param duration Principals of this duration will be removed.
     * Valid arguments are CapPermission.DOCUMENT or CapPermission.SESSION.
     * If the argument is SESSION, DOCUMENT-duration capabilities are also
     * removed.
     */
    public void cleanPrincipalCapabilities(int duration) {
	Enumeration e1, e2;
	CapPrincipal prin;
	CapPermTable permTable;
	CapPermission perm;
	CapTarget targ;

	if(duration != CapPermission.DOCUMENT &&
	   duration != CapPermission.SESSION)

	    return;  // bogus arguments, but not worth complaining...

	if(getStack().callerFrame().itsPrinAry[0] != itsSystemPrincipal)
	    throw new ForbiddenTargetException("only system code may call cleanPrincipalCapabilities");

	for(e1=itsPrinToPermTable.keys(); e1.hasMoreElements();) {
	    prin=(CapPrincipal)e1.nextElement();
	    permTable=(CapPermTable)itsPrinToPermTable.get(prin);

	    for(e2=permTable.keys(); e2.hasMoreElements();) {
		targ=(CapTarget)e2.nextElement();
		perm=permTable.get(targ);

		switch(perm.getDuration()) {
		case CapPermission.PERMANENT:
		    continue;
		case CapPermission.SESSION:
		    if(duration == CapPermission.DOCUMENT) continue;
		    // otherwise, fall through
		case CapPermission.DOCUMENT:
		    permTable.put(targ, CapPermission.getPermission(CapPermission.BLANK, CapPermission.PERMANENT));
		default:
		    // we shouldn't ever get here!
		}
	    }
	}
    }

    /**
     * check the current scope's permission for this target (optional
     * second argument says how many stack records to skip).  Note that
     * the both the current scope capabilities and the target will be
     * consulted in determining whether access is permitted or denied.
     */
    public void checkScopePermission(CapTarget target) {
	checkScopePermission(target, 1);  // skip the current frame
    }

    public void checkScopePermission(CapTarget target, int skipDepth) {
	CapStackEnumeration stackEnum = getStack().enumeration();
	CapStackFrame frame;
	CapPermission perm;

	skipDepth++;   // add current frame to frames being skipped
	if(skipDepth >= stackEnum.numElementsLeft())
	    throw new IndexOutOfBoundsException("security stack underflow");
	while(skipDepth-- > 0 && stackEnum.hasMoreElements())
	    stackEnum.nextFrame();

	while(stackEnum.hasMoreElements()) {
	    frame=stackEnum.nextFrame();

	    perm = frame.itsPermTable.get(target);
	    switch(perm.getState()) {
	    case CapPermission.BLANK:
		continue;
	    case CapPermission.ALLOWED:
		return;
	    case CapPermission.FORBIDDEN:
	    default: // shouldn't happen, but just in case...
		throw new ForbiddenTargetException("no target access: " + perm.toString());
	    }
	}
	throw new ForbiddenTargetException("no non-blank capability on stack");
    }

    /**
     * helper function used by checkPrincipalPermission() and
     * requestPrincipalPermission()
     *
     * @return allowed, forbidden, or blank - the judgement of
     * the various principals about whether the target may be accessed
     */
    private int getPrincipalPermission(CapTarget target, int skipDepth) {
	CapPrincipal callerPrinAry[] =
	    getClassPrincipalsFromStackUnsafe(skipDepth+1);
	return getPrincipalPermission(target, callerPrinAry);
    }

    private int getPrincipalPermission(CapTarget target, CapPrincipal callerPrinAry[]) {
	CapPermTable permTable;
	CapPermission perm;
	boolean isAllowed = false;

	for(int i=0; i<callerPrinAry.length; i++) {
	    permTable = (CapPermTable) itsPrinToPermTable.get(callerPrinAry[i]);
	    if(permTable == null) {
		// the principal isn't registered, so ignore it
		continue;
	    }

	    perm = permTable.get(target);
	    switch(perm.getState()) {
	    case CapPermission.ALLOWED:
		isAllowed = true;
		break;
	    case CapPermission.FORBIDDEN:
		return CapPermission.FORBIDDEN;
	    case CapPermission.BLANK:
		continue;
	    }
	}

	if(isAllowed) return CapPermission.ALLOWED;
	else return CapPermission.BLANK;
    }

    /**
     * check the current principals' permission for this target
     */
    public void checkPrincipalPermission(CapTarget target) {
	int state = getPrincipalPermission(target, 1);  // skip current frame
	switch(state) {
	case CapPermission.ALLOWED:
	    return;
	case CapPermission.FORBIDDEN:
	case CapPermission.BLANK:
	    throw new ForbiddenTargetException("access to target denied");
	}
    }

    /**
     * guaranteed to fail if the principal doesn't already
     * have the capability.  Absolutely no dialog
     * boxes can happen as a result of this call.
     */
    public void requestScopePermission(CapTarget target) {
	CapPermTable permTable;
	CapPermission allowedScope;

	checkPrincipalPermission(target);
	permTable = getStack().callerFrame().itsPermTable;
	allowedScope = CapPermission.getPermission(CapPermission.ALLOWED,
						   CapPermission.SCOPE);
	permTable.put(target, allowedScope);
    }

    /**
     * may involve giving the user a dialog box...
     */
    public void requestPrincipalPermission(CapTarget target) {
	requestPrincipalPermissionHelper(target, null, 1);
    }

    public void requestPrincipalPermission(CapTarget target,
                                           CapPrincipal preferredPrin) {
	requestPrincipalPermissionHelper(target, preferredPrin, 1);
    }

    private void requestPrincipalPermissionHelper(CapTarget target,
						  CapPrincipal preferredPrin,
						  int skipDepth) {

	CapPrincipal callerPrinAry[] =
	    getClassPrincipalsFromStackUnsafe(skipDepth+1);

	int state = getPrincipalPermission(target, callerPrinAry);

	switch(state) {
	case CapPermission.ALLOWED:
	    return;
	case CapPermission.FORBIDDEN:
	    throw new ForbiddenTargetException("access to target denied");
	case CapPermission.BLANK:
	    // Do a user dialog
	    break;
	default:
	    // shouldn't ever happen!
	    throw new ForbiddenTargetException("internal error!");
	}


	// TODO: User dialog!

	CapPermTable permTable;
	CapPermission perm;
	CapPrincipal useThisPrin = null;

	if(callerPrinAry.length == 0)
	    throw new ForbiddenTargetException("request's caller has no principal!");

	//
	// before we do the user dialog, we need to figure out which principal
	// gets the user's blessing.  The applet is allowed to bias this
	// by offering a preferred principal.  We only honor this if the
	// principal is *registered* (stored in itsPrinToPermTable) and
	// is based on cryptographic credentials, rather than a codebase.
	//
	// if no preferredPrin is specified, or we don't like preferredPrin,
	// we'll use the first principal on the calling class.  We know that
	// cryptographic credentials go first in the list, so this should
	// get us something reasonable.
	//

	if(preferredPrin != null)
	    for(int i=0; i<callerPrinAry.length; i++) {
		permTable = (CapPermTable)
		    itsPrinToPermTable.get(callerPrinAry[i]);
		if(permTable == null) {
		    // the principal isn't registered, so ignore it
		    continue;
		}
		if(callerPrinAry[i] == null)
		    throw new ForbiddenTargetException("internal error!");
		if(callerPrinAry[i].equals(preferredPrin) &&
		   (callerPrinAry[i].isCert() ||
		    callerPrinAry[i].isCertFingerprint()))

		    useThisPrin = callerPrinAry[i];
	    }
	if(useThisPrin == null) useThisPrin = callerPrinAry[0];
	permTable = (CapPermTable)
	    itsPrinToPermTable.get(useThisPrin);
	if(permTable == null) {
	    // shouldn't ever happen
	    throw new ForbiddenTargetException("internal error!");
	}

	//
	// TODO: here's where the UI dialog goes.
	//

	//
	// update the principal permissions (hack for now: make it
	// "allowed / permanent")
	//
	permTable.put(target,
		      CapPermission.getPermission(CapPermission.ALLOWED,
						  CapPermission.PERMANENT));
    }

    /**
     * array-based versions of the above -- you pass an array of
     * targets you're requesting.  If there was no exception, you're
     * permitted to access all the targets.
     * <p>
     * If there was an exception, you'll want to call the appropriate
     * check...() function to read the current state.
     */
    public void requestScopePermissionArray(CapTarget[] targets) {
    }

    public void requestPrincipalPermissionArray(CapTarget[] targets) {
    }

    public void requestPrincipalPermissionArray(CapTarget[] targets,
                                                CapPrincipal preferredPrin) {
    }

    /**
     * Regardless of the current state of the scope capability, it is
     * changed to `blank', possibly useful before calling an untrusted class
     * as a call-back.
     */
    public void blankScopePermission(CapTarget target) {
	CapPermTable permTable;

	permTable = getStack().callerFrame().itsPermTable;
	permTable.put(target,
		      CapPermission.getPermission(CapPermission.BLANK,
						  CapPermission.SCOPE));
    }

    public void blankScopePermissionArray(CapTarget[] targets) {
    }

    /**
     * anybody is allowed to forbid a scope capability, although somebody
     * they call could potentially get it back again.  Regardless of the
     * current permission for the given target, this will set it to
     * ``forbidden''.
     */
    public void forbidScopePermission(CapTarget target) {
	CapPermTable permTable;

	permTable = getStack().callerFrame().itsPermTable;
	permTable.put(target,
		      CapPermission.getPermission(CapPermission.FORBIDDEN,
						  CapPermission.SCOPE));
    }

    public void forbidScopePermissionArray(CapTarget[] targets) {
    }

    //////////////////////////////////////////////////////////////////////
    // Methods for security assertions
    //////////////////////////////////////////////////////////////////////

    /**
     * most applets will want to ask two questions:
     * <ul>
     * <li>Am I being called by another class with the same principal(s)?
     * <li>Does the class I'm about to call have the same principal(s) as me?
     * </ul>
     *
     * They'll need to store the desired principal name inside the class,
     * to avoid spoofing attacks.  Since many applets have no issue with
     * calling system classes, we provide a mechanism for this, too.
     */


    /**
     * A method may inquire if it was invoked by a given principal or
     * not.  Since this may need to be wrapped deep inside some other
     * checks, an optional second argument says how many stack frames
     * to skip.
     *
     * @param prin The principal you're asking about -- is the method
     * which called you belong to a class owned by this principal?
     *
     * @param skipFrames How many stack frames to skip upward.  If
     * skipFrames is zero, and you just invoked calledByPrincipal(),
     * you're asking about who invoked you.  Positive arguments inquire
     * about earlier stack frames.
     *
     * @return True if the invoking class has the given principal,
     * false otherise.
     */
    public boolean calledByPrincipal(CapPrincipal prin, int skipFrames) {
	CapPrincipal[] classPrinAry = getClassPrincipalsFromStackUnsafe(skipFrames+1);
	for(int i=0; i<classPrinAry.length; i++) {
	    if(classPrinAry[i] == null)
		throw new ForbiddenTargetException("internal error!");
	    if(classPrinAry[i].equals(prin)) return true;
	}
	return false;
    }

    public boolean calledByPrincipal(CapPrincipal prin) {
	return calledByPrincipal(prin, 2);  // skip our stack frame
    }

    public CapPrincipal getSystemPrincipal() {
	return itsSystemPrincipal;
    }

    // now, here are all the methods to implement those calls

    /**
     * @return an array of principals for the current class (the one
     * which called into CapManager, not CapManager itself)
     */
    public CapPrincipal[] getMyPrincipals() {
	return getClassPrincipalsFromStackUnsafe(1);
    }

    /**
     * This method returns the principals associated with a given stack
     * frame.  This may be useful to make sure you're being called by
     * somebody you trust.
     *
     * @param skipFrames How many stack frames to skip upward.  If
     * skipFrames is zero, and you just invoked calledByPrincipal(),
     * you're asking about who invoked you.  Positive arguments inquire
     * about earlier stack frames.
     *
     * @return an array of principals for the given stack frame - cryptographic
     * principals (e.g.: certificates) will be first in the array, then
     * codebase principals will follow.
     */
    public CapPrincipal[] getClassPrincipalsFromStack(int skipDepth) {
	CapPrincipal[] tmpAry =
	    getClassPrincipalsFromStackUnsafe(skipDepth+1);
	CapPrincipal[] returnAry = null;

	//
	// we need to make a backup copy of tmpAry, to avoid a malicious
	// applet from trying to mutate the array!  Note that clone()
	// on an array will return a new array with references to the
	// original objects.  Since CapPrincipals are immutable (unlike
	// arrays), this is safe.
	//

	try {
	    returnAry = (CapPrincipal[]) tmpAry.clone();
	} catch (Exception e) {}  // exception will never happen

	return returnAry;
    }

    private CapPrincipal[] getClassPrincipalsFromStackUnsafe(int skipDepth) {
	CapPrincipal returnPrinAry[];
	CapStackEnumeration stackEnum = getStack().enumeration();
	CapStackFrame frame;
	CapPermission perm;

	skipDepth++;   // add current frame to frames being skipped
	if(skipDepth >= stackEnum.numElementsLeft())
	    throw new IndexOutOfBoundsException("security stack underflow");
	while(skipDepth-- > 0 && stackEnum.hasMoreElements())
	    stackEnum.nextFrame();

	return stackEnum.nextFrame().itsPrinAry;
    }

    /**
     * @return a handle to the security information stack (which parallels
     * the thread's normal stack)
     */
    CapStack getStack() {
	throw new RuntimeException("CapManager.getStack() not yet implemented");
    }
}

