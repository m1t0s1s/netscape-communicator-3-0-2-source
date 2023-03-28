//
// DebugCapManager.java -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 1 August 1996
//

// $Id: DebugCapManager.java,v 1.1 1996/08/02 01:59:24 dwallach Exp $

package netscape.security;

import java.io.*;
import java.net.*;
import java.util.*;
import java.lang.*;

import netscape.security.*;

/**
 * This class implements a debugging version of the CapManager class.
 * The main difference is the security information stack does <i>not</i>
 * parallel the actual call stack, but rather can be tweaked by hand
 * (mainly for debugging purposes).
 *
 * @version 	$Id: DebugCapManager.java,v 1.1 1996/08/02 01:59:24 dwallach Exp $
 * @author	Dan Wallach
 */
public
class DebugCapManager extends CapManager {
    public CapStack itsStack;

    public DebugCapManager() {
	super();
	itsStack = new CapStack();
    }
    /**
     * @return a handle to the security information stack
     */
    CapStack getStack() {
	return itsStack;
    }
}

