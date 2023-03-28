//
// CapStack.java -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 26 July 1996
//

// $Id: CapStack.java,v 1.4 1996/07/31 00:42:50 dwallach Exp $

package netscape.security;

import java.io.*;
import java.net.*;
import java.util.*;
import java.lang.*;

import netscape.security.*;

/**
 * This class implements a shadow stack to the actual system stack.
 * Right now, it's only a simulation of the real stack.  Eventually,
 * we'll replace methods here with native ones that poke at the <i>real</i>
 * stack.
 * <p>
 * Related documentation here:
 * <ul>
 * <a href="http://iapp15/java/signing/authorsigning.html">http://iapp15/java/signing/authorsigning.html</a>
 * </ul>
 *
 * @version 	$Id: CapStack.java,v 1.4 1996/07/31 00:42:50 dwallach Exp $
 * @author	Dan Wallach
 */
class CapStack extends Stack {
    CapStack() {
	super();
    }

    public CapStackEnumeration enumeration() {
	return new CapStackEnumeration(this);
    }

    public CapStackFrame currentFrame() {
	return (CapStackFrame) lastElement();
    }

    public CapStackFrame callerFrame() {
	return (CapStackFrame) elementAt(size() - 2);
    }

    /**
     * similar to java.lang.Vector.toString(), but has prettier output
     */
    public String toPrettyString() {
	CapStackEnumeration e = enumeration();
	StringBuffer sb = new StringBuffer();
	CapStackFrame frame;

	while(e.hasMoreElements()) {
	    frame=e.nextFrame();
	    sb.append(frame.toString());
	    sb.append("\n");
	}

	return sb.toString();
    }
}

/**
 * Used by CapStack - we store one of these for each frame on the stack
 */
class CapStackFrame {
    public CapPermTable itsPermTable;
    public CapPrincipal itsPrinAry[];

    public CapStackFrame(CapPrincipal prin, CapPermTable table) {
	itsPermTable = table;
	itsPrinAry = new CapPrincipal[1];
	itsPrinAry[0] = prin;
    }

    public CapStackFrame(CapPrincipal prin) {
	itsPermTable = new CapPermTable();
	itsPrinAry = new CapPrincipal[1];
	itsPrinAry[0] = prin;
    }

    public CapStackFrame(CapPrincipal prinAry[], CapPermTable table) {
	itsPermTable = table;
	itsPrinAry = prinAry;
    }

    public CapStackFrame(CapPrincipal prinAry[]) {
	itsPermTable = new CapPermTable();
	itsPrinAry = prinAry;
    }

    //
    // why isn't this a builtin?
    //
    private String arrayToString(Object ary[]) {
	StringBuffer sb = new StringBuffer("[");

	for(int i=0; i<ary.length; i++) {
	    if(i>0) sb.append(",");
	    sb.append(ary[i].toString());
	}
	sb.append("]");

	return sb.toString();
    }

    public String toString() {
	return "Prin:" + arrayToString(itsPrinAry) + ", PermTable:\n" +
	    itsPermTable.toString();
    }
}

/**
 * Used by CapStack - works just like VectorEnumeration except the
 * list begins at the end of stack and works its way forward
 */
class CapStackEnumeration implements Enumeration {
    private CapStack itsStack;
    private int itsPos;

    CapStackEnumeration(CapStack s) {
	itsStack = s;
	itsPos = s.size();
    }

    public boolean hasMoreElements() {
	return itsPos > 0;
    }

    public int numElementsLeft() {
	return itsPos;
    }

    public Object nextElement() {
	return nextFrame();
    }

    public CapStackFrame nextFrame() {
	CapPermTable retVal;
	if(itsPos < 1) throw new NoSuchElementException("VectorEnumerator");
	synchronized (itsStack) {
	    return (CapStackFrame) itsStack.elementAt(--itsPos);
	}
    }
}
