/*
 * @(#)ScannerInputStream.java	1.9 95/11/08 Arthur van Hoff
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

package sun.tools.java;

import java.io.IOException;
import java.io.InputStream;
import java.io.FilterInputStream;

/**
 * An input stream for java programs. The stream treats either "\n", "\r" 
 * or "\r\n" as the end of a line, it always returns \n. It also parses
 * UNICODE characters expressed as \uffff. However, if it sees "\\", the
 * second slash cannot begin a unicode sequence. It keeps track of the current
 * position in the input stream.
 *
 * @author 	Arthur van Hoff
 * @version 	1.9, 08 Nov 1995
 */

public
class ScannerInputStream extends FilterInputStream implements Constants {
    Environment env;
    int pos;

    private int chpos;
    private int pushBack = -1;

    public ScannerInputStream(Environment env, InputStream in) {
	super(Runtime.getRuntime().getLocalizedInputStream(in));
	this.env = env;
	chpos = Scanner.LINEINC;
    }

    public int read() throws IOException {
	pos = chpos;
	chpos += Scanner.OFFSETINC;

	int c = pushBack;
	if (c == -1) {
	    c = in.read();
	} else {
	    pushBack = -1;
	}
	
	// parse special characters
	switch (c) {
	  case -2:
	    // -2 is a special code indicating a pushback of a backslash that
	    // definitely isn't the start of a unicode sequence.
	    return '\\';

	  case '\\':
	    if ((c = in.read()) != 'u') {
		pushBack = (c == '\\' ? -2 : c);
		return '\\';
	    }
	    // we have a unicode sequence
	    chpos += Scanner.OFFSETINC;
	    while ((c = in.read()) == 'u') {
		chpos += Scanner.OFFSETINC;
	    }
		
	    // unicode escape sequence
	    int d = 0;
	    for (int i = 0 ; i < 4 ; i++, chpos += Scanner.OFFSETINC, c = in.read()) {
		switch (c) {
		  case '0': case '1': case '2': case '3': case '4':
		  case '5': case '6': case '7': case '8': case '9':
		    d = (d << 4) + c - '0';
		    break;
		    
		  case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
		    d = (d << 4) + 10 + c - 'a';
		    break;
		    
		  case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
		    d = (d << 4) + 10 + c - 'A';
		    break;
		    
		  default:
		    env.error(pos, "invalid.escape.char");
		    pushBack = c;
		    return d;
		}
	    }
            pushBack = c;
   	    return d;

	  case '\n':
	    chpos += Scanner.LINEINC;
	    return '\n';

	  case '\r':
	    if ((c = in.read()) != '\n') {
		pushBack = c;
	    } else {
		chpos += Scanner.OFFSETINC;
	    }
	    chpos += Scanner.LINEINC;
	    return '\n';

	  default:
	    return c;
	}
    }
}
