//
// testCapMgr1.java -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 22 July 1996
//

// $Id: testCapMgr1.java,v 1.4 1996/08/02 01:59:27 dwallach Exp $

package netscape.security;

import java.lang.*;
import java.io.*;
import java.util.*;


/**
 * make a CapManager, fake up a call stack, and exercise some of the
 * CapManager methods
 */
public class testCapMgr1 {
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

    static CapTarget targ[] = new CapTarget[nTargets];
    static CapPrincipal prin[] = new CapPrincipal[nTargets];

    static public void main(String args[]) {
	DebugCapManager mgr = new DebugCapManager();
	CapPrincipal sysPrin = mgr.getSystemPrincipal();
	int i;

	//
	// start with a blank system-only stack
	//
	mgr.getStack().push(new CapStackFrame(sysPrin, new CapPermTable()));
	mgr.getStack().push(new CapStackFrame(sysPrin, new CapPermTable()));
	mgr.getStack().push(new CapStackFrame(sysPrin, new CapPermTable()));
	mgr.getStack().push(new CapStackFrame(sysPrin, new CapPermTable()));

	//
	// register some targets
	//

	for(i=0; i<nTargets; i++) {
	    prin[i] = new CapPrincipal(prinTypes[i], prinNames[i]);
	    targ[i] = new CapTarget(targetNames[i], prin[i]);
	}


	System.out.println("===== Original stack:");
	System.out.println(mgr.getStack().toPrettyString());

	mgr.requestPrincipalPermission(targ[0]);
	mgr.requestPrincipalPermission(targ[1]);

	System.out.println("===== Stack after prin permissions (should be same):");
	System.out.println(mgr.getStack().toPrettyString());

	mgr.requestScopePermission(targ[0]);
	mgr.requestScopePermission(targ[1]);

	System.out.println("===== Stack after first request:");
	System.out.println(mgr.getStack().toPrettyString());

	System.out.println("===== These should all fail:");
	for(i=0; i<nTargets; i++) {
	    System.out.print("Access to " + targetNames[i] + ": ");
	    try {
		mgr.checkScopePermission(targ[i]);
		System.out.println("Allowed.");
	    } catch (Exception e) {
		System.out.println("Blocked.");
		System.out.println("    " + e);
	    }
	}

	//
	// the implementation of checkScopePermission uses two stack
	// frames inside CapManager.  The above example skipped two
	// frames, missing the frame with the non-blank capabilities.
	// This time, I'm adding another blank frame, so the security
	// check should see the non-blank capabilities.
	//
	System.out.println("===== The first two should succeed:");
	mgr.getStack().push(new CapStackFrame(sysPrin, new CapPermTable()));
	for(i=0; i<nTargets; i++) {
	    System.out.print("Access to " + targetNames[i] + ": ");
	    try {
		mgr.checkScopePermission(targ[i]);
		System.out.println("Allowed.");
	    } catch (Exception e) {
		System.out.println("Blocked.");
		System.out.println("    " + e);
	    }
	}
    }
}
