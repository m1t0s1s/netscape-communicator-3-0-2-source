//
// testPermTable.java -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 22 July 1996
//

// $Id: testPermTable.java,v 1.3 1996/08/01 23:06:33 dwallach Exp $

package netscape.security;

import java.lang.*;
import java.io.*;
import java.util.*;

/**
 * testPermTable exercises the netscape.security.CapPermTable and CapTarget
 * classes.  Hashtables are your friends.
 *
 * @see netscape.security.CapPermTable
 */

public
class testPermTable {
    static CapPermTable pt = new CapPermTable();
    static final int nTargets = 5;
    static String targetNames[] = {
	"microphone",
	"cd-player",
	"salami-slicer",
	"retina-scanner",
	"weed-whacker",
	"orange-juicer"
    };
    static CapTarget t[] = new CapTarget[nTargets];

    public static void main(String args[]) {
	int i;

	//
	// first, register a bunch of targets
	//
	for(i=0; i<nTargets; i++)
	    t[i] = new CapTarget(targetNames[i], null);

	//
	// now, set up some permissions for them
	//
	System.out.println("==== Initial configuration");
	System.out.println(pt);

	pt.put(CapTarget.getTarget("microphone"),
	       CapPermission.getPermission(CapPermission.ALLOWED,
					   CapPermission.SCOPE));
	System.out.println("==== After insertion #1");
	System.out.println(pt);

	pt.put(CapTarget.getTarget("salami-slicer"),
	       CapPermission.getPermission(CapPermission.ALLOWED,
					   CapPermission.SESSION));
	System.out.println("==== After insertion #2");
	System.out.println(pt);

	pt.put(CapTarget.getTarget("retina-scanner"),
	       CapPermission.getPermission(CapPermission.FORBIDDEN,
					   CapPermission.SESSION));
	System.out.println("==== After final insertion");
	System.out.println(pt);

	//
	// print out the table -- blatantly inefficient use of
	// new CapTarget to stress the target equality function
	//
	for(i=0; i<nTargets; i++) {
	    System.out.println(t[i].toString() +
			       " ==> " +
			       pt.get(CapTarget.getTarget(targetNames[i])));
	}

	//
	// try a target that we didn't register earlier
	//
	System.out.println(targetNames[nTargets] +
			   " ==> " +
			   pt.get(CapTarget.getTarget(targetNames[nTargets])));
    }
}
