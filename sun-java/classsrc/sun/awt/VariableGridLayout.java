/*
 * @(#)VariableGridLayout.java	1.1 95/08/30 Herb Jellinek
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

package sun.awt;

import java.awt.*;

/**
 * A layout manager for a container that lays out grids.  Allows setting
 * the relative sizes of rows and columns.
 *
 * @version 1.1, 30 Aug 1995
 * @author Herb Jellinek
 */


public class VariableGridLayout extends GridLayout {

    double rowFractions[] = null;
    double colFractions[] = null;

    int rows;
    int cols;
    int hgap;
    int vgap;
    
    /**
     * Creates a grid layout with the specified rows and specified columns.
     * @param rows the rows
     * @param cols the columns
     */
    public VariableGridLayout(int rows, int cols) {
	this(rows, cols, 0, 0);

	if (rows != 0) {
	    stdRowFractions(rows);
	}
	
	if (cols != 0) {
	    stdColFractions(cols);
	}
    }

    /**
     * Creates a grid layout with the specified rows, columns,
     * horizontal gap, and vertical gap.
     * @param rows the rows
     * @param cols the columns
     * @param hgap the horizontal gap variable
     * @param vgap the vertical gap variable
     * @exception IllegalArgumentException If the rows and columns are invalid.
     */
    public VariableGridLayout(int rows, int cols, int hgap, int vgap) {
	super(rows, cols, hgap, vgap);

	this.rows = rows;
	this.cols = cols;
	this.hgap = hgap;
	this.vgap = vgap;
	
	if (rows != 0) {
	    stdRowFractions(rows);
	}
	
	if (cols != 0) {
	    stdColFractions(cols);
	}
    }

    void stdRowFractions(int nrows) {
	rowFractions = new double[nrows];
	for (int i = 0; i < nrows; i++) {
	    rowFractions[i] = 1.0 / nrows;
	}
    }

    void stdColFractions(int ncols) {
	colFractions = new double[ncols];
	for (int i = 0; i < ncols; i++) {
	    colFractions[i] = 1.0 / ncols;
	}
    }
    
    public void setRowFraction(int rowNum, double fraction) {
	rowFractions[rowNum] = fraction;
    }

    public void setColFraction(int colNum, double fraction) {
	colFractions[colNum] = fraction;
    }

    public double getRowFraction(int rowNum) {
	return rowFractions[rowNum];
    }

    public double getColFraction(int colNum) {
	return colFractions[colNum];
    }

    /** 
     * Lays out the container in the specified panel.  
     * @param parent the specified component being laid out
     * @see Container
     */
    public void layoutContainer(Container parent) {
	Insets insets = parent.insets();
	int ncomponents = parent.countComponents();
	int nrows = rows;
	int ncols = cols;

	if (nrows > 0) {
	    ncols = (ncomponents + nrows - 1) / nrows;
	} else {
	    nrows = (ncomponents + ncols - 1) / ncols;
	}

	if (rows == 0) {
	    stdRowFractions(nrows);
	}
	if (cols == 0) {
	    stdColFractions(ncols);
	}

	Dimension size = parent.size();
	int w = size.width - (insets.left + insets.right);
	int h = size.height - (insets.top + insets.bottom);

	w = (w - (ncols - 1) * hgap);
	h = (h - (nrows - 1) * vgap);

	for (int c = 0, x = insets.left ; c < ncols ; c++) {
	    int colWidth = (int)(getColFraction(c) * w);
	    for (int r = 0, y = insets.top ; r < nrows ; r++) {
		int i = r * ncols + c;
		int rowHeight = (int)(getRowFraction(r) * h);
		
		if (i < ncomponents) {
		    parent.getComponent(i).reshape(x, y, colWidth, rowHeight);
		}
		y += rowHeight + vgap;
	    }
	    x += colWidth + hgap;
	}
    }

    static String fracsToString(double array[]) {
	String result = "["+array.length+"]";
	
	for (int i = 0; i < array.length; i++) {
	    result += "<"+array[i]+">";
	}
	return result;
    }
    
    /**
     * Returns the String representation of this VariableGridLayout's values.
     */
    public String toString() {
	return getClass().getName() + "[hgap=" + hgap + ",vgap=" + vgap + 
	    			       ",rows=" + rows + ",cols=" + cols +
				       ",rowFracs=" +
				       fracsToString(rowFractions) +
				       ",colFracs=" +
				       fracsToString(colFractions) + "]";
    }
}
