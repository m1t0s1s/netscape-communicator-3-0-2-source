/*
 * @(#)Scanner.java	1.54 95/11/17 Arthur van Hoff
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
import java.util.Hashtable;

/**
 * A Scanner for Java tokens. Errors are reported 
 * to the environment object.<p>
 *
 * The scanner keeps track of the current token,
 * the value of the current token (if any), and the start
 * position of the current token.<p>
 *
 * The scan() method advances the scanner to the next
 * token in the input.<p>
 *
 * The match() method is used to quickly match opening
 * brackets (ie: '(', '{', or '[') with their closing
 * counter part. This is useful during error recovery.<p>
 *
 * An position consists of: ((linenr << OFFSETBITS) | offset)
 * this means that both the line number and the exact offset into
 * the file are encoded in each postion value.<p>
 *
 * The compiler treats either "\n", "\r" or "\r\n" as the
 * end of a line.<p>
 *
 * @author 	Arthur van Hoff
 * @version 	1.54, 17 Nov 1995
 */

public
class Scanner implements Constants {
    /**
     * The increment for each character.
     */
    protected static final int OFFSETINC = 1;

    /**
     * The increment for each line.
     */
    protected static final int LINEINC = 1 << OFFSETBITS;

    /**
     * End of input
     */
    public static final int EOF	= -1;

    /**
     * Where errors are reported
     */
    protected Environment env;

    /**
     * Input stream
     */
    protected ScannerInputStream in;

    /**
     * Current token
     */
    protected int token;

    /**
     * The position of the current token
     */
    protected int pos;

    /**
     * The position of the previous token
     */
    protected int prevPos;

    /**
     * The current character
     */
    private int ch;

    /*
     * Token values.
     */
    protected char charValue;
    protected int intValue;
    protected long longValue;
    protected float floatValue;
    protected double doubleValue;
    protected String stringValue;
    protected Identifier idValue;
    protected int radix;	// Radix, when reading int or long

    /* 
     * A doc comment preceding the most recent token 
     */
    protected String docComment; 

    /*
     * A growable character buffer.
     */
    private int count;
    private char buffer[] = new char[32];
    private void putc(int ch) {
	if (count == buffer.length) {
	    char newBuffer[] = new char[buffer.length * 2];
	    System.arraycopy(buffer, 0, newBuffer, 0, buffer.length);
	    buffer = newBuffer;
	}
	buffer[count++] = (char)ch;
    }
    private String bufferString() {
	char buf[] = new char[count];
	System.arraycopy(buffer, 0, buf, 0, count);
	return new String(buf);
    }

    /**
     * Create a scanner to scan an input stream.
     */
    protected Scanner(Environment env, InputStream in) throws IOException {
	this.env = env;
	this.in = new ScannerInputStream(env, in);

	ch = this.in.read();
	prevPos = this.in.pos; 

	scan();
    }

    /**
     * Define a keyword.
     */
    private static void defineKeyword(int val) {
	Identifier.lookup(opNames[val]).setType(val);
    }

    /**
     * Initialized keyword and token Hashtables
     */
    static {
	// Statement keywords
	defineKeyword(FOR);
	defineKeyword(IF);
	defineKeyword(ELSE);
	defineKeyword(WHILE);
	defineKeyword(DO);
	defineKeyword(SWITCH);
	defineKeyword(CASE);
	defineKeyword(DEFAULT);
	defineKeyword(BREAK);
	defineKeyword(CONTINUE);
	defineKeyword(RETURN);
	defineKeyword(TRY);
	defineKeyword(CATCH);
	defineKeyword(FINALLY);
	defineKeyword(THROW);

	// Type defineKeywords
	defineKeyword(BYTE);
	defineKeyword(CHAR);
	defineKeyword(SHORT);
	defineKeyword(INT);
	defineKeyword(LONG);
	defineKeyword(FLOAT);
	defineKeyword(DOUBLE);
	defineKeyword(VOID);
	defineKeyword(BOOLEAN);

	// Expression keywords
	defineKeyword(INSTANCEOF);
	defineKeyword(TRUE);
	defineKeyword(FALSE);
	defineKeyword(NEW);
	defineKeyword(THIS);
	defineKeyword(SUPER);
	defineKeyword(NULL);

	// Declaration keywords
	defineKeyword(IMPORT);
	defineKeyword(CLASS);
	defineKeyword(EXTENDS);
	defineKeyword(IMPLEMENTS);
	defineKeyword(INTERFACE);
	defineKeyword(PACKAGE);
	defineKeyword(THROWS);

	// Modifier keywords
	defineKeyword(PRIVATE);
	defineKeyword(PUBLIC);
	defineKeyword(PROTECTED);
	defineKeyword(STATIC);
	defineKeyword(TRANSIENT);
	defineKeyword(SYNCHRONIZED);
	defineKeyword(NATIVE);
	defineKeyword(ABSTRACT);
	defineKeyword(VOLATILE);
	defineKeyword(FINAL);

	// reserved keywords
	defineKeyword(CONST);
	defineKeyword(GOTO);
    }

    /**
     * Returns true if the character is a Unicode digit.
     * @param ch the character to be checked
     */
    public static boolean isUCDigit(char ch) {
	if ((ch >= '0') && (ch <= '9'))
	    return true;
	switch (ch>>8) {
	  case 0x06:
	    return 
	        ((ch >= 0x0660) && (ch <=0x0669)) ||	// Arabic-Indic
	        ((ch >= 0x06f0) && (ch <=0x06f9));	// Eastern Arabic-Indic
	  case 0x07:
	  case 0x08:
	  default:
	    return false;
	  case 0x09:
	    return
	        ((ch >= 0x0966) && (ch <=0x096f)) ||	// Devanagari
	        ((ch >= 0x09e6) && (ch <=0x09ef));	// Bengali
          case 0x0a:
	    return
	        ((ch >= 0x0a66) && (ch <=0x0a6f)) ||	// Gurmukhi
	        ((ch >= 0x0ae6) && (ch <=0x0aef));	// Gujarati
	  case 0x0b:
	    return
	        ((ch >= 0x0b66) && (ch <=0x0b6f)) ||	// Oriya
	        ((ch >= 0x0be7) && (ch <=0x0bef));	// Tamil
	  case 0x0c:
	    return
	        ((ch >= 0x0c66) && (ch <=0x0c6f)) ||	// Telugu
	        ((ch >= 0x0ce6) && (ch <=0x0cef));	// Kannada
	  case 0x0d:
	    return
	        ((ch >= 0x0d66) && (ch <=0x0d6f));	// Malayalam
	  case 0x0e:
	    return
	        ((ch >= 0x0e50) && (ch <=0x0e59)) ||	// Thai
	        ((ch >= 0x0ed0) && (ch <=0x0ed9));	// Lao
	  case 0x0f:
	    return false;
	  case 0x10:
	    return
	        ((ch >= 0x1040) && (ch <=0x1049));	// Tibetan
	}
     }

    /**
     * Returns true if the character is a Unicode letter.
     * @param ch the character to be checked
     */
    public static boolean isUCLetter(char ch) {
	// fast check for Latin capitals and small letters
	if (((ch >= 'A') && (ch <= 'Z')) ||
	    ((ch >= 'a') && (ch <= 'z'))) {
	    return true;
	}
	// rest of ISO-LATIN-1
	if (ch < 0x0100) {
	    // fast check
	    if (ch < 0x00c0) {
	        return (ch == '_') || (ch == '$');
	    }
	    // various latin letters and diacritics,
	    // but *not* the multiplication and division symbols
	    return
	        ((ch >= 0x00c0) && (ch <= 0x00d6)) ||
	        ((ch >= 0x00d8) && (ch <= 0x00f6)) ||
	        ((ch >= 0x00f8) && (ch <= 0x00ff));
	}
	// other non CJK alphabets and symbols, but not digits
	if (ch <= 0x1fff) {
	    return !isUCDigit(ch);
	}
	// rest are letters only in five ranges:
	//	Hiragana, Katakana, Bopomofo and Hangul
	//	CJK Squared Words
	//	Korean Hangul Symbols
	//	Han (Chinese, Japanese, Korean)
	//	Han compatibility
	return
	    ((ch >= 0x3040) && (ch <= 0x318f)) ||
	    ((ch >= 0x3300) && (ch <= 0x337f)) ||
	    ((ch >= 0x3400) && (ch <= 0x3d2d)) ||
	    ((ch >= 0x4e00) && (ch <= 0x9fff)) ||
	    ((ch >= 0xf900) && (ch <= 0xfaff));
    }

    /**
     * Scan a comment. This method should be
     * called once the initial /, * and the next
     * character have been read.
     */
    private void skipComment() throws IOException {
	while (true) {
	    switch (ch) {
	      case EOF:
	        env.error(pos, "eof.in.comment");
		return;

	      case '*':
		if ((ch = in.read()) == '/')  {
		    ch = in.read();
		    return;
		}
		break;

	      default:
		ch = in.read();
		break;
	    }
	}
    }

    /**
     * Scan a doc comment. This method should be called
     * once the initial /, * and * have been read. It gathers
     * the content of the comment (witout leading spaces and '*'s)
     * in the string buffer.
     */
    private String scanDocComment() throws IOException {
	count = 0;

	if (ch == '*') {
	    do {
		ch = in.read();
	    } while (ch == '*');
	    if (ch == '/') {
		ch = in.read();
		return "";
	    }
	}
	switch (ch) {
	  case '\n':
	  case ' ':
	    ch = in.read();
	    break;
	}

	boolean seenstar = false;
	int c = count;
	while (true) {
	    switch (ch) {
	      case EOF:
	        env.error(pos, "eof.in.comment");
		return bufferString();

	      case '\n':
		putc('\n');
		ch = in.read();
		seenstar = false;
		c = count;
		break;

	      case ' ':
	      case '\t':
		putc(ch);
		ch = in.read();
		break;

	      case '*':
		if (seenstar) {
		    if ((ch = in.read()) == '/') {
			ch = in.read();
			count = c;
			return bufferString();
		    }
		    putc('*');
		} else {
		    seenstar = true;
		    count = c;
		    while ((ch = in.read()) == '*');
		    switch (ch) {
		      case ' ':
			ch = in.read();
			break;

		      case '/':
			ch = in.read();
			count = c;
			return bufferString();
		    }
		}
		break;

	      default:
		if (!seenstar) {
		    seenstar = true;
		}
		putc(ch);
		ch = in.read();
		c = count;
		break;
	    }
	}
    }

    /**
     * Scan a number. The first digit of the number should be the current 
     * character.  We may be scanning hex, decimal, or octal at this point
     */
    private void scanNumber() throws IOException {
	boolean seenNonOctal = false;
	boolean overflow = false;
	radix = (ch == '0' ? 8 : 10);
	long value = ch - '0';
	count = 0;
	putc(ch);		// save character in buffer
    numberLoop:
	for (;;) {
	    switch (ch = in.read()) {
	      case '.': 
		if (radix == 16) 
		    break numberLoop; // an illegal character
		scanReal();  
		return;

	      case '8': case '9':
		// We can't yet throw an error if reading an octal.  We might
		// discover we're really reading a real.
		seenNonOctal = true;
	      case '0': case '1': case '2': case '3': 
	      case '4': case '5': case '6': case '7': 
		putc(ch);
		if (radix == 10) {
		    overflow = overflow || (value * 10)/10 != value;
		    value = (value * 10) + (ch - '0');
		    overflow = overflow || (value - 1 < -1);
		} else if (radix == 8) { 
		    overflow = overflow || (value >>> 61) != 0;
		    value = (value << 3) + (ch - '0');
		} else { 
		    overflow = overflow || (value >>> 60) != 0;
		    value = (value << 4) + (ch - '0');
		}
		break;
		
	      case 'd': case 'D': case 'e': case 'E': case 'f': case 'F':
		if (radix != 16) {
		    scanReal();
		    return;
		}
		// fall through
	      case 'a': case 'A': case 'b': case 'B': case 'c': case 'C':
		putc(ch); 
		if (radix != 16) 
		    break numberLoop; // an illegal character
		overflow = overflow || (value >>> 60) != 0;
		value = (value << 4) + 10 + 
		         Character.toLowerCase((char)ch) - 'a';
	        break;

	      case 'l': case 'L':
		ch = in.read();	// skip over 'l'
	        longValue = value;
	        token = LONGVAL;
		break numberLoop;
		
	      case 'x': case 'X':
		// if the first character is a '0' and this is the second
		// letter, then read in a hexadecimal number.  Otherwise, error.
		if (count == 1 && radix == 8) {
		    radix = 16;
		    break;
		} else { 
		    // we'll get an illegal character error 
		    break numberLoop;
		}
		
	      default:
		intValue = (int)value;
		token = INTVAL;
		break numberLoop;
	    }
	}			// while true
	// we have just finished reading the number.  The next thing better
	// not be a letter or digit.
	if (isUCDigit((char)ch) || isUCLetter((char)ch) || ch == '.') { 
	    env.error(in.pos, "invalid.number");
	    do { ch = in.read(); } 
	    while (isUCDigit((char)ch) || isUCLetter((char)ch) || ch == '.');
	    intValue = 0;
	    token = INTVAL;
	} else if (radix == 8 && seenNonOctal) { 
	    intValue = 0;
	    token = INTVAL;
	    env.error(in.pos, "invalid.octal.number");	    
	} else if (overflow || 
		   (token == INTVAL &&  
		      ((radix == 10) ? (intValue - 1 < -1) 
		                     : ((value & 0xFFFFFFFF00000000L) != 0)))) {
	     intValue = 0;	// so we don't get second overflow in Parser
	     longValue = 0;
	     env.error(pos, "overflow");
	}
    }

    /**
     * Scan a float.  We are either looking at the decimal, or we have already
     * seen it and put it into the buffer.  We haven't seen an exponent.
     * Scan a float.  Should be called with the current character is either
     * the 'e', 'E' or '.'
     */
    private void scanReal() throws IOException {
	boolean seenExponent = false;
	boolean isSingleFloat = false;
	char lastChar;
	if (ch == '.') { 
	    putc(ch); 
	    ch = in.read();
	}

    numberLoop:
	for ( ; ; ch = in.read()) {
	    switch (ch) {
	        case '0': case '1': case '2': case '3': case '4':
	        case '5': case '6': case '7': case '8': case '9':
		    putc(ch);
		    break;
			
	        case 'e': case 'E':
		    if (seenExponent) 
			break numberLoop; // we'll get a format error
		    putc(ch);
		    seenExponent = true;
		    break;

	        case '+': case '-': 
		    lastChar = buffer[count - 1];
		    if (lastChar != 'e' && lastChar != 'E') 
			break numberLoop; // this isn't an error, though!
		    putc(ch);
		    break;

	        case 'f': case 'F':
		    ch = in.read(); // skip over 'f'
		    isSingleFloat = true;
		    break numberLoop;

	        case 'd': case 'D':
		    ch = in.read(); // skip over 'd'
		    // fall through
	        default:
		    break numberLoop;
	    } // sswitch
	} // loop

	// we have just finished reading the number.  The next thing better
	// not be a letter or digit.
	if (isUCDigit((char)ch) || isUCLetter((char)ch) || ch == '.') { 
	    env.error(in.pos, "invalid.number");
	    do { ch = in.read(); } 
	    while (isUCDigit((char)ch) || isUCLetter((char)ch) || ch == '.');
	    doubleValue = 0;
	    token = DOUBLEVAL;
	} else { 
	    token = isSingleFloat ? FLOATVAL : DOUBLEVAL;
	    try {
		lastChar = buffer[count - 1];
		if (lastChar == 'e' || lastChar == 'E' 
		       || lastChar == '+' || lastChar == '-') {
		    env.error(in.pos -1, "float.format");
		} else if (isSingleFloat) { 
		    floatValue = Float.valueOf(bufferString()).floatValue();
		    if (Float.isInfinite(floatValue)) {
			env.error(pos, "overflow");
		    }
		} else { 
		    doubleValue = Double.valueOf(bufferString()).doubleValue();
		    if (Double.isInfinite(doubleValue)) {
			env.error(pos, "overflow");
		    }
		}
	    } catch (NumberFormatException ee) {
		env.error(pos, "float.format");
		doubleValue = 0;
		floatValue = 0;
	    }
	}
	return;
    }

    /**
     * Scan an escape character.
     * @return the character or -1 if it escaped an
     * end-of-line.
     */
    private int scanEscapeChar() throws IOException {
	int p = in.pos;
	
	switch (ch = in.read()) {
	  case '0': case '1': case '2': case '3':
	  case '4': case '5': case '6': case '7': {
	    int n = ch - '0';
	    for (int i = 2 ; i > 0 ; i--) {
		switch (ch = in.read()) {
		  case '0': case '1': case '2': case '3':
		  case '4': case '5': case '6': case '7':
		    n = (n << 3) + ch - '0';
		    break;

		  default:
		    if (n > 0xFF) {
			env.error(p, "invalid.escape.char");
		    }
		    return n;
		}
	    }
	    ch = in.read();
	    if (n > 0xFF) {
		env.error(p, "invalid.escape.char");
	    }
	    return n;
	  }

	  case 'r':  ch = in.read(); return '\r';
	  case 'n':  ch = in.read(); return '\n';
	  case 'f':  ch = in.read(); return '\f';
	  case 'b':  ch = in.read(); return '\b';
	  case 't':  ch = in.read(); return '\t';
	  case '\\': ch = in.read(); return '\\';
	  case '\"': ch = in.read(); return '\"';
	  case '\'': ch = in.read(); return '\'';
	}

	env.error(p, "invalid.escape.char");
	ch = in.read();
	return -1;
    }

    /**
     * Scan a string. The current character
     * should be the opening " of the string.
     */
    private void scanString() throws IOException {
	token = STRINGVAL;
	count = 0;
	ch = in.read();

	// Scan a String
	while (true) {
	    switch (ch) {
	      case EOF:
		env.error(pos, "eof.in.string");
		stringValue = bufferString();
		return;

	      case '\n':
		ch = in.read();
		env.error(pos, "newline.in.string");
		stringValue = bufferString();
		return;

	      case '"':
		ch = in.read();
		stringValue = bufferString();
		return;

	      case '\\': {
		int c = scanEscapeChar();
		if (c >= 0) {
		    putc((char)c);
		}
		break;
	      }	  

	      default:
		putc(ch);
		ch = in.read();
		break;
	    }
	}
    }

    /**
     * Scan a character. The current character should be
     * the opening ' of the character constant.
     */
    private void scanCharacter() throws IOException {
	token = CHARVAL;

	switch (ch = in.read()) {
	  case '\\':
	    int c = scanEscapeChar();
	    charValue = (char)((c >= 0) ? c : 0);
	    break;

	  case '\n':
	    charValue = 0;
	    env.error(pos, "invalid.char.constant");
	    return;

	  default:
	    charValue = (char)ch;
	    ch = in.read();
	    break;
	}

	if (ch == '\'') {
	    ch = in.read();
	} else {
	    env.error(pos, "invalid.char.constant");
	    while (true) {
		switch (ch) {
		  case '\'':
		    ch = in.read();
		    return;
		  case ';':
		  case '\n':
		  case EOF:
		    return;
		  default:
		    ch = in.read();
		}
	    }
	}
    }

    /**
     * Scan an Identifier. The current character should
     * be the first character of the identifier.
     */
    private void scanIdentifier() throws IOException {
	count = 0;

	while (true) {
	    putc(ch);
	    switch (ch = in.read()) {
	      case 'a': case 'b': case 'c': case 'd': case 'e':
	      case 'f': case 'g': case 'h': case 'i': case 'j':
	      case 'k': case 'l': case 'm': case 'n': case 'o':
	      case 'p': case 'q': case 'r': case 's': case 't':
	      case 'u': case 'v': case 'w': case 'x': case 'y':
	      case 'z':
	      case 'A': case 'B': case 'C': case 'D': case 'E':
	      case 'F': case 'G': case 'H': case 'I': case 'J':
	      case 'K': case 'L': case 'M': case 'N': case 'O':
	      case 'P': case 'Q': case 'R': case 'S': case 'T':
	      case 'U': case 'V': case 'W': case 'X': case 'Y':
	      case 'Z':
	      case '0': case '1': case '2': case '3': case '4':
	      case '5': case '6': case '7': case '8': case '9':
	      case '$': case '_':
		break;
		
	      default:
		if ((!isUCDigit((char)ch)) && (!isUCLetter((char)ch))) {
		    idValue = Identifier.lookup(bufferString());
		    token = idValue.getType();
		    return;
		}
	    }
	}
    }

    /**
     * Scan the next token.
     * @return the position of the previous token.
     */
   protected int scan() throws IOException {
       int result = xscan();
       return result;
   }

    protected int xscan() throws IOException {
	int retPos = pos;
	prevPos = in.pos;
	docComment = null;
	while (true) {
	    pos = in.pos;

	    switch (ch) {
	      case EOF:
		token = EOF;
		return retPos;
		
	      case '\n':
	      case ' ':
	      case '\t':
	      case '\f':
		ch = in.read();
		break;
		
	      case '/':
		switch (ch = in.read()) {
		  case '/':
		    // Parse a // comment
		    while (((ch = in.read()) != EOF) && (ch != '\n'));
		    break;
		    
		  case '*':
		    ch = in.read();
		    if (ch == '*') {
		        docComment = scanDocComment();
		    } else {
			skipComment();
		    }
		    break;
		    
		  case '=':
		    ch = in.read();
		    token = ASGDIV;
		    return retPos;
		    
		  default:
		    token = DIV;
		    return retPos;
		}
		break;
		
	      case '"':
		scanString();
		return retPos;
		
	      case '\'':
		scanCharacter();
		return retPos;

	      case '0': case '1': case '2': case '3': case '4': 
	      case '5': case '6': case '7': case '8': case '9': 
		scanNumber();
		return retPos;

	      case '.':
		switch (ch = in.read()) { 
		  case '0': case '1': case '2': case '3': case '4':
		  case '5': case '6': case '7': case '8': case '9':
		    count = 0;
		    putc('.');
		    scanReal();
		    break;
		  default:
		    token = FIELD;
		}		
		return retPos;
		
	      case '{':
		ch = in.read();
		token = LBRACE;
		return retPos;

	      case '}':
		ch = in.read();
		token = RBRACE;
		return retPos;

	      case '(':
		ch = in.read();
		token = LPAREN;
		return retPos;

	      case ')':
		ch = in.read();
		token = RPAREN;
		return retPos;

	      case '[':
		ch = in.read();
		token = LSQBRACKET;
		return retPos;

	      case ']':
		ch = in.read();
		token = RSQBRACKET;
		return retPos;

	      case ',':
		ch = in.read();
		token = COMMA;
		return retPos;

	      case ';':
		ch = in.read();
		token = SEMICOLON;
		return retPos;

	      case '?':
		ch = in.read();
		token = QUESTIONMARK;
		return retPos;

	      case '~':
		ch = in.read();
		token = BITNOT;
		return retPos;

	      case ':':
		ch = in.read();
		token = COLON;
		return retPos;
		
	      case '-':
		switch (ch = in.read()) {
		  case '-':
		    ch = in.read();
		    token = DEC;
		    return retPos;

		  case '=':
		    ch = in.read();
		    token = ASGSUB;
		    return retPos;
		}
		token = SUB;
		return retPos;
		
	      case '+':
		switch (ch = in.read()) {
		  case '+':
		    ch = in.read();
		    token = INC;
		    return retPos;

		  case '=':
		    ch = in.read();
		    token = ASGADD;
		    return retPos;
		}
		token = ADD;
		return retPos;
		
	      case '<':
		switch (ch = in.read()) {
		  case '<':
		    if ((ch = in.read()) == '=') {
			ch = in.read();
			token = ASGLSHIFT;
			return retPos;
		    }
		    token = LSHIFT;
		    return retPos;

		  case '=':
		    ch = in.read();
		    token = LE;
		    return retPos;
		}
		token = LT;
		return retPos;
		
	      case '>':
		switch (ch = in.read()) {
		  case '>':
		    switch (ch = in.read()) {
		      case '=':
			ch = in.read();
			token = ASGRSHIFT;
			return retPos;
			
		      case '>':
			if ((ch = in.read()) == '=') {
			    ch = in.read();
			    token = ASGURSHIFT;
			    return retPos;
			}
			token = URSHIFT;
			return retPos;
		    }
		    token = RSHIFT;
		    return retPos;

		  case '=':
		    ch = in.read();
		    token = GE;
		    return retPos;
		}
		token = GT;
		return retPos;
		
	      case '|':
		switch (ch = in.read()) {
		  case '|':
		    ch = in.read();
		    token = OR;
		    return retPos;
		    
		  case '=':
		    ch = in.read();
		    token = ASGBITOR;
		    return retPos;
		}
		token = BITOR;
		return retPos;
		
	      case '&':
		switch (ch = in.read()) {
		  case '&':
		    ch = in.read();
		    token = AND;
		    return retPos;
		    
		  case '=':
		    ch = in.read();
		    token = ASGBITAND;
		    return retPos;
		}
		token = BITAND;
		return retPos;
		
	      case '=':
		if ((ch = in.read()) == '=') {
		    ch = in.read();
		    token = EQ;
		    return retPos;
		}
		token = ASSIGN;
		return retPos;
		
	      case '%':
		if ((ch = in.read()) == '=') {
		    ch = in.read();
		    token = ASGREM;
		    return retPos;
		}
		token = REM;
		return retPos;

	      case '^':
		if ((ch = in.read()) == '=') {
		    ch = in.read();
		    token = ASGBITXOR;
		    return retPos;
		}
		token = BITXOR;
		return retPos;

	      case '!':
		if ((ch = in.read()) == '=') {
		    ch = in.read();
		    token = NE;
		    return retPos;
		}
		token = NOT;
		return retPos;

	      case '*':
		if ((ch = in.read()) == '=') {
		    ch = in.read();
		    token = ASGMUL;
		    return retPos;
		}
		token = MUL;
		return retPos;
		
	      case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': 
	      case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': 
	      case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': 
	      case 's': case 't': case 'u': case 'v': case 'w': case 'x': 
	      case 'y': case 'z':
	      case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': 
	      case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': 
	      case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': 
	      case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': 
	      case 'Y': case 'Z':
	      case '$': case '_':
		scanIdentifier();
		return retPos;

	      case '\u001a':  
  		// Our one concession to DOS.
		if ((ch = in.read()) == EOF) {
		    token = EOF;
		    return retPos;
		}
		env.error(pos, "funny.char");
		ch = in.read();
		break;
		  
		
	      default:
		if (isUCLetter((char)ch)) {
		    scanIdentifier();
		    return retPos;
		}
		env.error(pos, "funny.char");
		ch = in.read();
		break;
 	    }
	}
    }

    /**
     * Scan to a matching '}', ']' or ')'. The current token must be
     * a '{', '[' or '(';
     */
    protected void match(int open, int close) throws IOException {
	int depth = 1;

	while (true) {
	    scan();
	    if (token == open) {
		depth++;
	    } else if (token == close) {
		if (--depth == 0) {
		    return;
		}
	    } else if (token == EOF) {
		env.error(pos, "unbalanced.paren");
		return;
	    }
	}
    }
}
