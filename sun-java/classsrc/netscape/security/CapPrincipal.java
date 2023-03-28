//
// CapPrincipal.java -- Copyright 1996, Netscape Communications Corp.
// Dan Wallach <dwallach@netscape.com>
// 16 July 1996
//

// $Id: CapPrincipal.java,v 1.5 1996/07/31 05:39:49 dwallach Exp $

package netscape.security;

import java.lang.*;
import java.util.*;

/**
 * This class represents a principal.  Ultimately, the user is assigning
 * their trust to a real person or company, and this holds their crypto
 * public key or other identifying information.
 * <p>
 * Related documentation here:
 * <ul>
 * <a href="http://iapp15/java/signing/authorsigning.html">http://iapp15/java/signing/authorsigning.html</a>
 * </ul>
 *
 * @version 	$Id: CapPrincipal.java,v 1.5 1996/07/31 05:39:49 dwallach Exp $
 * @author	Dan Wallach
 */
public final
class CapPrincipal {
    public static final int CODEBASE_EXACT=0;
    public static final int CODEBASE_REGEXP=1;
    public static final int CERT=2;
    public static final int CERT_FINGERPRINT=3;

    private int itsType;
    private int itsHashCode;
    private String itsStringRep;
    private byte itsBinaryRep[];
    private byte itsFingerprint[];

    /**
     * This constructor allows you to specify a principal based on an ASCII
     * string.  This is the typical way CODEBASE Principals are created.
     * CERT_FINGERPRINT's can also be made this way, using an ASCII coded
     * representation like so:  ##:##:##, where each ## represents one byte
     * (two hex digits).
     */
    public CapPrincipal(int type, String value) {
	itsType=type;

	switch(type) {
	case CODEBASE_EXACT:
	case CODEBASE_REGEXP:
	    itsStringRep = value;
	    break;

	case CERT:
	case CERT_FINGERPRINT:
	    //
	    // we need to convert from string to binary rep
	    //
	    StringTokenizer st;

	    st = new StringTokenizer(value, ":;, \t");
	    itsBinaryRep = new byte[st.countTokens()];

	    for(int i=0; st.hasMoreElements(); i++)
		itsBinaryRep[i] = (byte)Integer.parseInt(st.nextToken(), 16);

	    if(type == CERT) {
		certToFingerprint();
	    } else {
		itsFingerprint = itsBinaryRep;
	    }

	    break;

	default:
	    throw new IllegalArgumentException("unknown principal type");
	}
	computeHashCode();
    }

    /**
     * This constructor allows you to specify CERT's and CERT_FINGERPRINT's
     * using a more compact input representation.  It's not appropriate for
     * CODEBASE principals.
     */
    public CapPrincipal(int type, byte[] value) {
	itsType=type;

	switch(type) {
	case CERT:
	    itsBinaryRep = value;
	    certToFingerprint();
	    break;
	case CERT_FINGERPRINT:
	    itsBinaryRep = value;
	    itsFingerprint = itsBinaryRep;
	    break;

	case CODEBASE_EXACT:
	case CODEBASE_REGEXP:
	    //
	    // they must have fed us a string as a byte array... *sigh*
	    //
	    itsStringRep = new String(value, 0);
	    break;

	default:
	    throw new IllegalArgumentException("unknown principal type");
	}
	computeHashCode();
    }

    /**
     * Compares with the given CapPrincipal and returns if
     * they <i>represent the same actual principal</i>.
     */
    public boolean equals(Object obj) {
        if(obj == this) return true;
        if(obj == null || this == null) return false;
        if(!(obj instanceof CapPrincipal)) return false;

        CapPrincipal prin = (CapPrincipal) obj;

	switch(itsType) {
	case CERT:
	case CERT_FINGERPRINT:
	    switch(prin.itsType) {
	    case CERT:
	    case CERT_FINGERPRINT:
		if(prin.itsFingerprint.length != itsFingerprint.length)
		    return false;

		for(int i=0; i<itsFingerprint.length; i++)
		    if(prin.itsFingerprint[i] != itsFingerprint[i])
			return false;
		return true;

		//
		// codebases and certs are never equal
		//
	    case CODEBASE_EXACT:
	    case CODEBASE_REGEXP:
		return false;
	    }
	case CODEBASE_EXACT:
	case CODEBASE_REGEXP:
	    switch(prin.itsType) {
	    case CERT:
	    case CERT_FINGERPRINT:
	        return false;
	    case CODEBASE_EXACT:
	    case CODEBASE_REGEXP:
	        // TODO: regular expression handling -- for now,
	        // regular expressions are the same as bare strings
	        return itsStringRep.equals(prin.itsStringRep);
	    }
	}
	// if control gets here, we've got a serious bug
	return false;
    }

    /**
     * optimized hashCode() for fast dictionary lookups
     */
    public int hashCode() {
	return itsHashCode;
    }

    private void computeHashCode() {
	switch(itsType) {
	case CERT:
	case CERT_FINGERPRINT:
	    itsHashCode = 0;
	    //
	    // Same basic hash algorithm as used in java.lang.String --
	    // no security relevance, only a performance optimization.
	    // The security comes from the equals() method.
	    //
	    for(int i=0; i<itsFingerprint.length; i++)
		itsHashCode = itsHashCode * 37 + itsFingerprint[i];
	    break;

	case CODEBASE_EXACT:
	case CODEBASE_REGEXP:
	    itsHashCode = itsStringRep.hashCode();
	    break;
	}
    }

    public int getType() {
	return itsType;
    }

    public boolean isCodebaseExact() {
	return itsType == CODEBASE_EXACT;
    }

    public boolean isCodebaseRegexp() {
	return itsType == CODEBASE_REGEXP;
    }

    public boolean isCert() {
	return itsType == CERT;
    }

    public boolean isCertFingerprint() {
	return itsType == CERT_FINGERPRINT;
    }

    public String toString() {
	switch(itsType) {
	case CERT:
	case CERT_FINGERPRINT:
	    StringBuffer resultBuf = new StringBuffer();
	    for(int i=0; i<itsBinaryRep.length; i++) {
		if(i>0) resultBuf.append(":");

		//
		// *sigh* -- Java tries to sign-extend the byte when
		// converting to an int, and the language doesn't have
		// unsigned types.
		//
		if(itsBinaryRep[i] < 0)
		    resultBuf.append(Integer.toString((int)
						      (itsBinaryRep[i]&0x7f) +
						      0x80, 16));
		else
		    resultBuf.append(Integer.toString(itsBinaryRep[i], 16));
	    }
	    return resultBuf.toString();

	case CODEBASE_EXACT:
	case CODEBASE_REGEXP:
	    return itsStringRep;  // security: copying not necessary any more?
	}

	// control never actually reaches here, but the compiler complains
	return itsStringRep;
    }

    private void certToFingerprint() {
	// TODO: call builtin SHA, MD5, or something
	itsFingerprint = itsBinaryRep;
    }
}
