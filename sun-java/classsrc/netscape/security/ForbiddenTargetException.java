//
// ForbiddenTargetException.java
//    -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 16 July 1996
//

// $Id: ForbiddenTargetException.java,v 1.2 1996/07/31 00:42:53 dwallach Exp $

package netscape.security;

import java.lang.*;

/**
 * This exception is thrown when CapManager isn't happy
 * <p>
 * Related documentation here:
 * <ul>
 * <a href="http://iapp15/java/signing/authorsigning.html">http://iapp15/java/signing/authorsigning.html</a>
 * </ul>
 *
 * @version 	$Id: ForbiddenTargetException.java,v 1.2 1996/07/31 00:42:53 dwallach Exp $
 * @author	Dan Wallach
 */

public
class ForbiddenTargetException extends java.lang.RuntimeException {
    /**
     * Constructs an IllegalArgumentException with no detail message.
     * A detail message is a String that describes this particular exception.
     */
    public ForbiddenTargetException() {
	super();
    }

    /**
     * Constructs an ForbiddenTargetException with the specified detail message.
     * A detail message is a String that describes this particular exception.
     * @param s the detail message
     */
    public ForbiddenTargetException(String s) {
	super(s);
    }
}

