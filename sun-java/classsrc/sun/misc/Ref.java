/*
 * @(#)Ref.java	1.17 95/09/06
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

package sun.misc;

/**
 * A "Ref" is an indirect reference to an object that the garbage collector
 * knows about.  An application should override the reconstitute() method with one
 * that will construct the object based on information in the Ref, often by
 * reading from a file.  The get() method retains a cache of the result of the last call to
 * reconstitute() in the Ref.  When space gets tight, the garbage collector
 * will clear old Ref cache entries when there are no other pointers to the
 * object.  In normal usage, Ref will always be subclassed.  The subclass will add the 
 * instance variables necessary for the reconstitute() method to work.  It will also add a 
 * constructor to set them up, and write a version of reconstitute().
 * @version     1.17, 06 Sep 1995
 */

public abstract class Ref {
    static int	    lruclock;
    private Object  thing;
    private long    priority;

    /**
     * Returns a pointer to the object referenced by this Ref.  If the object
     * has been thrown away by the garbage collector, it will be
     * reconstituted. This method does everything necessary to ensure that the garbage
     * collector throws things away in Least Recently Used(LRU) order.  Applications should 
     * never override this method. The get() method effectively caches calls to
     * reconstitute().
     */
    public Object get() {
	Object p = thing;
	if (p == null) {
	    /* synchronize if thing is null, but then check again
	       in case somebody else beat us to it */
	    synchronized (this) {
		if (thing == null) {
		    p = reconstitute();
		    thing = p;
		}
	    }
	}
	priority = ++lruclock;
	return thing;
    }

    /**
     * Returns a pointer to the object referenced by this Ref by 
     * reconstituting it from some external source (such as a file).  This method should not 
     * bother with caching since the method get() will deal with that.
     * <p>
     * In normal usage, Ref will always be subclassed.  The subclass will add
     * the instance variables necessary for reconstitute() to work.  It will
     * also add a constructor to set them up, and write a version of
     * reconstitute().
     */
    public abstract Object reconstitute();

    /**
     * Flushes the cached object.  Forces the next invocation of get() to
     * invoke reconstitute().
     */
    public void flush() {
	thing = null;
    }
   
    /**
     * Sets the thing to the specified object.
     * @param thing the specified object
     */
    public void setThing(Object thing) {
	this.thing = thing;
    }

    /**
     * Checks to see what object is being pointed at by this Ref and returns it.
     */
    public Object check() {
	return thing;
    }

    /**
     * Constructs a new Ref.
     */
    public Ref() {
	priority = ++lruclock;
    }
}
