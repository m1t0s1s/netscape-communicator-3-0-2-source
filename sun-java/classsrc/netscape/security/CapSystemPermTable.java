//
// CapSystemPermTable.java -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 30 July 1996 (originally part of CapPermTable.java, but removed
// to make Symantec Cafe function properly -- piece 'o crap compiler)
//

// $Id: CapSystemPermTable.java,v 1.1 1996/07/31 00:42:51 dwallach Exp $

package netscape.security;

import java.lang.*;
import java.util.*;
import java.io.*;

/**
 * This class handles the system principal and always allows everything.
 * Exactly one instance of this is installed when CapManager is created
 * and otherwise nobody can get one of these into the system.
 *
 * Related documentation here:
 * <ul>
 * <a href="http://iapp15/java/signing/authorsigning.html">http://iapp15/java/signing/authorsigning.html</a>
 * </ul>
 *
 * @version 	$Id: CapSystemPermTable.java,v 1.1 1996/07/31 00:42:51 dwallach Exp $
 * @author	Dan Wallach
 */
class CapSystemPermTable extends CapPermTable {
    public CapSystemPermTable() {
	super();
    }

    public CapPermission get(CapTarget t) {
	return CapPermission.getPermission(CapPermission.ALLOWED,
					   CapPermission.PERMANENT);
    }
}
