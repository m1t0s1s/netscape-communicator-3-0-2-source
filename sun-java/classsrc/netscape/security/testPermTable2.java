//
// testPermTable2.java -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 22 July 1996
//

// $Id: testPermTable2.java,v 1.2 1996/07/31 05:39:53 dwallach Exp $

package netscape.security;

import java.lang.*;
import java.io.*;
import java.util.*;

/**
 * testPermTable2 is a lot like testPermTable except it uses principals
 */

public
class testPermTable2 {
    static CapPermTable pt = new CapPermTable();
    static final int nTargets = 4;
    static String targetNames[] = {
	"microphone",
	"cd-player",
	"salami-slicer",
	"retina-scanner",
    };

    static int prinTypes[] = {
        CapPrincipal.CODEBASE_EXACT,
        CapPrincipal.CODEBASE_REGEXP,
        CapPrincipal.CERT,
        CapPrincipal.CERT_FINGERPRINT
    };

    static String prinNames[] = {
        "http://www.idiots-r-us.com/stuff/",
        "http://*.idiots-r-us.com/stuff/",
        "a8:d7:e3:b2:45:87:aa:ff:04:95:23",
        "a8:d7:e3:b2:45:87:aa:ff:04:95:23",
    };

    //
    // it's perfectly legal java to use base-10 numbers here, but
    // apparently not base-16.  How lame.
    //
    static byte bytePrinData[] = {
        (byte)0xa8, (byte)0xd7, (byte)0xe3, (byte)0xb2, (byte)0x45, (byte)0x87,
        (byte)0xaa, (byte)0xff, (byte)0x04, (byte)0x95, (byte)0x23 };

    static byte bytePrinData2[] = {
        (byte)0xa8, (byte)0xd7, (byte)0xe3, (byte)0xb2, (byte)0x45, (byte)0x87,
        (byte)0xaa, (byte)0xff, (byte)0x04, (byte)0x95, (byte)0x23, (byte)0x99 };

    static CapTarget t[] = new CapTarget[nTargets];
    static CapPrincipal prin[] = new CapPrincipal[nTargets];
    static CapPrincipal bytePrin[] = new CapPrincipal[2];

    public static void main(String args[]) {
	int i;

	//
	// first, register a bunch of targets
	//
	for(i=0; i<nTargets; i++) {
	    prin[i] = new CapPrincipal(prinTypes[i], prinNames[i]);
	    t[i] = new CapTarget(targetNames[i], prin[i]);
	}

	bytePrin[0] = new CapPrincipal(CapPrincipal.CERT_FINGERPRINT, bytePrinData);
	bytePrin[1] = new CapPrincipal(CapPrincipal.CERT_FINGERPRINT, bytePrinData2);

	//
	// now, set up some permissions for them
	//

	pt.put(t[0],
	       CapPermission.getPermission(CapPermission.ALLOWED,
					   CapPermission.PERMANENT));

	pt.put(t[1],
	       CapPermission.getPermission(CapPermission.ALLOWED,
					   CapPermission.SESSION));

	pt.put(t[2],
	       CapPermission.getPermission(CapPermission.FORBIDDEN,
					   CapPermission.SESSION));
	System.out.println("==== State of permission table ====");
	System.out.println(pt);

	//
	// print out the table -- blatantly inefficient use of
	// new CapTarget to stress the target equality function
	//
	System.out.println("");
	System.out.println("==== Verify we can get the targets back ====");
	for(i=0; i<nTargets; i++) {
	    CapTarget tRegistry = CapTarget.getTarget(targetNames[i], prin[i]);
	    System.out.println("Desired==> " + t[i].toString() +
			       " ==> " + pt.get(t[i]));
	    System.out.println("Actual ==> " + t[i].toString() +
			       " ==> " + pt.get(tRegistry));

	}

	System.out.println("");
	System.out.println("==== Same thing, good byte-array principal ====");
	for(i=0; i<nTargets; i++) {
	    System.out.println(t[i].toString() +
			       " ==> " +
			       pt.get(CapTarget.getTarget(targetNames[i], bytePrin[0])));
	}

	System.out.println("");
	System.out.println("==== Same thing, bad byte-array principal ====");
	for(i=0; i<nTargets; i++) {
	    System.out.println(t[i].toString() +
			       " ==> " +
			       pt.get(CapTarget.getTarget(targetNames[i], bytePrin[1])));
	}
    }
}
