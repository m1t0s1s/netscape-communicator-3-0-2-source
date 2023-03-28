//
// CapPermission.java -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 16 July 1996
//

// $Id: CapPermission.java,v 1.4 1996/07/27 07:02:16 dwallach Exp $

package netscape.security;

import java.lang.*;

/**
 * This class represents a system permission.  When you ask somebody if
 * an action is allowed, they return one of these to say it's allowed,
 * it's forbidden, or they don't care.
 *
 * Related documentation here:
 * <ul>
 * <a href="http://iapp15/java/signing/authorsigning.html">http://iapp15/java/signing/authorsigning.html</a>
 * </ul>
 *
 * @version 	$Id: CapPermission.java,v 1.4 1996/07/27 07:02:16 dwallach Exp $
 * @author	Dan Wallach
 */
public final
class CapPermission {
    /**
     * number of permission states (allowed, forbidden, or blank)
     */
    public static final int N_STATES = 3;

    /**
     * allowed permission
     */
    public static final int ALLOWED = 0;
    /**
     * forbidden permission
     */
    public static final int FORBIDDEN = 1;
    /**
     * blank permission
     */
    public static final int BLANK = 2;

    private int itsState;

    /**
     * number of possible durations (scope, webpage, session, or permanent)
     */
    public static final int N_DURATIONS = 4;
    /**
     * scope duration (until current procedure returns)
     */
    public static final int SCOPE = 0;
    /**
     * webpage duration (until browser loads a new web page)
     */
    public static final int DOCUMENT = 1;
    /**
     * session duration (until browser exits)
     */
    public static final int SESSION = 2;
    /**
     * permanent duration (until certificate expires)
     */
    public static final int PERMANENT = 3;

    private int itsDuration;

    private static CapPermission itsPermissionCache[][] =
	    new CapPermission[N_STATES][N_DURATIONS];

    /*
     * static initializer: set up the permission cache
     */
    static {
	for(int i=0; i<N_STATES; i++)
	    for(int j=0; j<N_DURATIONS; j++)
		itsPermissionCache[i][j] = new CapPermission(i,j);
    }

    /**
     * The constructor isn't supposed to be used by normal users -- they
     * should use getPermission().  It wouldn't hurt anything to make it
     * public, but people would get confused and use the wrong one.
     */
    CapPermission(int state, int duration) {
	itsState=state;
	itsDuration=duration;
    }

    /**
     * getPermission() is the preferred way of getting a CapPermission.
     * Because the number of possible permissions is reasonably small,
     * this function returns a handle to a pre-allocated CapPermission,
     * which accelerates comparison tests.
     *
     * @param state One of: ALLOWED, FORBIDDEN, or BLANK
     * @param duration One of: SCOPE, DOCUMENT, SESSION, or PERMANENT
     * @return the CapPermission corresponding to the arguments
     */
    public static CapPermission getPermission(int state, int duration) {
	if(state < 0 || state >= N_STATES ||
	   duration < 0 || duration >= N_DURATIONS) {
	    throw new IllegalArgumentException("unknown state or duration");
	}

	return itsPermissionCache[state][duration];
    }

    /**
     * tests for equality both in permission and duration
     */
    public boolean equals(Object p) {
	return p == this;       // shallow equality is faster
    }

    /**
     * tests for same state (allowed, forbidden, or blank) but doesn't
     * care about duration
     */
    public boolean samePermission(CapPermission p) {
	return p.itsState == itsState;
    }

    /**
     * tests for same duration (scope, webpage, session, or permanent)
     * but doesn't care about state
     */
    public boolean sameDuration(CapPermission p) {
	return p.itsDuration == itsDuration;
    }

    /**
     * is the state of this permission <i>allowed</i>
     */
    public boolean isAllowed() {
	return itsState == ALLOWED;
    }

    /**
     * is the state of this permission <i>forbidden</i>
     */
    public boolean isForbidden() {
	return itsState == FORBIDDEN;
    }

    /**
     * is the state of this permission <i>blank</i>
     */
    public boolean isBlank() {
	return itsState == BLANK;
    }

    /**
     * returns the state of this permission (allowed, forbidden, or blank)
     */
    public int getState() {
	return itsState;
    }

    /**
     * returns the state of this permission (scope, webpage, session, or permanent)
     */
    public int getDuration() {
	return itsDuration;
    }

    /**
     * convert the permission into a printable string
     *
     * @return A string of the form {allowed | forbidden | blank} {in the current scope | in the current Web page | in the curent browser session | forever}
     */
    public String toString() {
	String stateStr=null;
	String resultStr=null;

	switch(itsState) {
	case ALLOWED:
	    stateStr = "allowed";
	    break;
	case FORBIDDEN:
	    stateStr = "forbidden";
	    break;
	case BLANK:
	    stateStr = "blank";
	    break;
	}

	switch(itsDuration) {
	case SCOPE:
	    resultStr = stateStr + " in the current scope";
	    break;
	case DOCUMENT:
	    resultStr = stateStr + " in the current document";
	    break;
	case SESSION:
	    resultStr = stateStr + " in the current browser session";
	    break;
	case PERMANENT:
	    resultStr = stateStr + " forever";
	    break;
	}

	return resultStr;
    }

    public int hashCode() {
	return itsDuration * N_STATES + itsState;
    }
}
