/*
 * @(#)ColorTest.java	1.1 95/08/21 Jim Graham
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
import java.awt.*;

public class ColorTest {
    public static void main(String args[]) {
	convert(Color.red, "red");
	convert(Color.yellow, "yellow");
	convert(Color.green, "green");
	convert(Color.cyan, "cyan");
	convert(Color.blue, "blue");
	convert(Color.magenta, "magenta");
	convert(Color.pink, "pink");
	convert(Color.orange, "orange");
	System.out.println();
	convert(Color.white, "white");
	convert(Color.lightGray, "lightGray");
	convert(Color.gray, "gray");
	convert(Color.darkGray, "darkGray");
	convert(Color.black, "black");
	System.out.println();
	verify(Color.red, "red");
	verify(Color.yellow, "yellow");
	verify(Color.green, "green");
	verify(Color.cyan, "cyan");
	verify(Color.blue, "blue");
	verify(Color.magenta, "magenta");
	verify(Color.pink, "pink");
	verify(Color.orange, "orange");
	System.out.println();
	verify(Color.white, "white");
	verify(Color.lightGray, "lightGray");
	verify(Color.gray, "gray");
	verify(Color.darkGray, "darkGray");
	verify(Color.black, "black");
    }

    static void convert(Color c, String name) {
	float[] hsbvals = Color.RGBtoHSB(c.getRed(), c.getGreen(), c.getBlue(),
					 null);
	System.out.println(name + " => [hue = " + hsbvals[0]
			   + ", sat = " + hsbvals[1]
			   + ", brite = " + hsbvals[2] + "]");
    }

    static void verify(Color c, String name) {
	int r = c.getRed();
	int g = c.getGreen();
	int b = c.getBlue();
	float[] hsbvals = Color.RGBtoHSB(r, g, b, null);
	float hue = hsbvals[0];
	float sat = hsbvals[1];
	float brite = hsbvals[2];
	int rgb = Color.HSBtoRGB(hue, sat, brite);
	int r2 = (rgb >> 16) & 0xff;
	int g2 = (rgb >> 8) & 0xff;
	int b2 = (rgb >> 0) & 0xff;
	System.out.println("[" + r + "," + g + "," + b + "]"
			   + " => HSB => RGB == "
			   + "[" + r2 + "," + g2 + "," + b2 + "]");
    }
}
