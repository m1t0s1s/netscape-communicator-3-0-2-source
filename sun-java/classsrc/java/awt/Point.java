/*
 * @(#)Point.java	1.7 95/08/29 Sami Shaio
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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
package java.awt;

/**
 * An x,y coordinate.
 *
 * @version 	1.7, 29 Aug 1995
 * @author 	Sami Shaio
 */
public class Point {
    /**
     * The x coordinate.
     */
    public int x;

    /**
     * The y coordinate.
     */
    public int y;

    /**
     * Constructs and initializes a Point from the specified x and y 
     * coordinates.
     * @param x the x coordinate
     * @param y the y coordinate
     */
    public Point(int x, int y) {
	this.x = x;
	this.y = y;
    }

    /**
     * Move the point.
     */
    public void move(int x, int y) {
	this.x = x;
	this.y = y;
    }	

    /**
     * Translate the point.
     */
    public void translate(int x, int y) {
	this.x += x;
	this.y += y;
    }	

    /**
     * Hashcode
     */
    public int hashCode() {
	return x ^ (y*31);
    }

    /**
     * Check if two pointers are equal.
     */
    public boolean equals(Object obj) {
	if (obj instanceof Point) {
	    Point pt = (Point)obj;
	    return (x == pt.x) && (y == pt.y);
	}
	return false;
    }

    /**
     * Returns the String representation of this Point's coordinates.
     */
    public String toString() {
	return getClass().getName() + "[x=" + x + ",y=" + y + "]";
    }
}
