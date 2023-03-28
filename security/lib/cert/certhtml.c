/* -*- Mode: C; tab-width: 8 -*-
 * certhtml.c --- convert a cert to html
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: certhtml.c,v 1.50.2.4 1997/05/24 00:23:02 jwz Exp $
 */

#include "cert.h"
#include "secder.h"
#include "sechash.h"
#include "prprf.h"

static char *hex = "0123456789ABCDEF";

/*
** Convert a der-encoded integer to a hex printable string form
*/
char *CERT_Hexify (SECItem *i, int do_colon)
{
    unsigned char *cp, *end;
    char *rv, *o;

    if (!i->len) {
	return PORT_Strdup("00");
    }

    rv = o = (char*) PORT_Alloc(i->len * 3);
    if (!rv) return rv;

    cp = i->data;
    end = cp + i->len;
    while (cp < end) {
	unsigned char ch = *cp++;
	*o++ = hex[(ch >> 4) & 0xf];
	*o++ = hex[ch & 0xf];
	if (cp != end) {
	    if (do_colon) {
		*o++ = ':';
	    }
	} else {
	    *o++ = 0;
	}
    }
    return rv;
}

static char *
gatherStrings(char **strings)
{
    char **strs;
    int len;
    char *ret;
    char *s;

    /* find total length of all strings */
    strs = strings;
    len = 0;
    while ( *strs ) {
	len += PORT_Strlen(*strs);
	strs++;
    }
    
    /* alloc enough memory for it */
    ret = (char*)PORT_Alloc(len + 1);
    if ( !ret ) {
	return(ret);
    }

    s = ret;
    
    /* copy the strings */
    strs = strings;
    while ( *strs ) {
	PORT_Strcpy(s, *strs);
	s += PORT_Strlen(*strs);
	strs++;
    }

    return( ret );
}

#define BREAK "<br>"
#define BREAKLEN 4
#define COMMA ", "
#define COMMALEN 2

#define MAX_OUS 20

char *CERT_FormatName (CERTName *name)
{
    CERTRDN** rdns;
    CERTRDN *rdn;
    CERTAVA** avas;
    CERTAVA* ava;
    char *buf = 0;
    char *tmpbuf = 0;
    unsigned len = 0;
    int tag;
    SECItem *cn, *email, *org, *loc, *state, *country;
    SECItem *orgunit[MAX_OUS];
    int i, ou_count = 0;
    PRBool first;
    
    cn = 0;
    email = 0;
    org = 0;
    loc = 0;
    state = 0;
    country = 0;
    
    /* Loop over name components and gather the interesting ones */
    rdns = name->rdns;
    while ((rdn = *rdns++) != 0) {
	avas = rdn->avas;
	while ((ava = *avas++) != 0) {
	    tag = CERT_GetAVATag(ava);
	    switch(tag) {
	      case SEC_OID_AVA_COMMON_NAME:
		cn = &ava->value;
		len += cn->len;
		break;
	      case SEC_OID_AVA_COUNTRY_NAME:
		country = &ava->value;
		len += country->len;
		break;
	      case SEC_OID_AVA_LOCALITY:
		loc = &ava->value;
		len += loc->len;
		break;
	      case SEC_OID_AVA_STATE_OR_PROVINCE:
		state = &ava->value;
		len += state->len;
		break;
	      case SEC_OID_AVA_ORGANIZATION_NAME:
		org = &ava->value;
		len += org->len;
		break;
	      case SEC_OID_AVA_ORGANIZATIONAL_UNIT_NAME:
		if (ou_count < MAX_OUS) {
			orgunit[ou_count] = &ava->value;
			len += orgunit[ou_count++]->len;
		}
		break;
	      case SEC_OID_PKCS9_EMAIL_ADDRESS:
	      case SEC_OID_RFC1274_MAIL:
		email = &ava->value;
		len += email->len;
		break;
	      default:
		break;
	    }
	}
    }

    /* XXX - add some for formatting */
    len += 128;

    /* allocate buffer */
    buf = (char *)PORT_Alloc(len);
    if ( !buf ) {
	return(0);
    }

    tmpbuf = buf;
    
    if ( cn ) {
	PORT_Memcpy(tmpbuf, cn->data + 2, cn->len - 2);
	tmpbuf += ( cn->len - 2 );
	PORT_Memcpy(tmpbuf, BREAK, BREAKLEN);
	tmpbuf += BREAKLEN;
    }
    if ( email ) {
	PORT_Memcpy(tmpbuf, email->data + 2, email->len - 2);
	tmpbuf += ( email->len - 2 );
	PORT_Memcpy(tmpbuf, BREAK, BREAKLEN);
	tmpbuf += BREAKLEN;
    }
    for (i=ou_count-1; i >= 0; i--) {
	PORT_Memcpy(tmpbuf, orgunit[i]->data + 2, orgunit[i]->len - 2);
	tmpbuf += ( orgunit[i]->len - 2 );
	PORT_Memcpy(tmpbuf, BREAK, BREAKLEN);
	tmpbuf += BREAKLEN;
    }
    if ( org ) {
	PORT_Memcpy(tmpbuf, org->data + 2, org->len - 2);
	tmpbuf += ( org->len - 2 );
	PORT_Memcpy(tmpbuf, BREAK, BREAKLEN);
	tmpbuf += BREAKLEN;
    }
    first = PR_TRUE;
    if ( loc ) {
	PORT_Memcpy(tmpbuf, loc->data + 2, loc->len - 2);
	tmpbuf += ( loc->len - 2 );
	first = PR_FALSE;
    }
    if ( state ) {
	if ( !first ) {
	    PORT_Memcpy(tmpbuf, COMMA, COMMALEN);
	    tmpbuf += COMMALEN;
	}
	PORT_Memcpy(tmpbuf, state->data + 2, state->len - 2);
	tmpbuf += ( state->len - 2 );
	first = PR_FALSE;
    }
    if ( country ) {
	if ( !first ) {
	    PORT_Memcpy(tmpbuf, COMMA, COMMALEN);
	    tmpbuf += COMMALEN;
	}
	PORT_Memcpy(tmpbuf, country->data + 2, country->len - 2);
	tmpbuf += ( country->len - 2 );
	first = PR_FALSE;
    }
    if ( !first ) {
	PORT_Memcpy(tmpbuf, BREAK, BREAKLEN);
	tmpbuf += BREAKLEN;
    }

    *tmpbuf = 0;

    return(buf);
}

static char *sec_FortezzaClearance(SECItem *clearance) {
    unsigned char clr = 0;

    if (clearance->len > 0) { clr = clearance->data[0]; }

    if (clr & 0x4) return "Top Secret";
    if (clr & 0x8) return "Secret";
    if (clr & 0x10) return "Confidential";
    if (clr & 0x20) return "Sensitive";
    if (clr & 0x40) return "Unclassified";
    return "None";
}

static char *sec_FortezzaMessagePriviledge(SECItem *priv) {
    unsigned char clr = 0;

    if (priv->len > 0) { clr = priv->data[0]; }

    switch (clr & 0x78) {
    case 0x00:
	break;
    case 0x08:
	return("Rekey Agent");
    case 0x10:
	return("Routine/Deferred");
    case 0x18:
	return("Routine/Deferred, Rekey Agent");
    case 0x20:
	return("Immediate/Priority");
    case 0x28:
	return("Immediate/Priority, Rekey Agent");
    case 0x30:
	return("Immediate/Priority");
    case 0x38:
	return("Immediate/Priority, Rekey Agent");
    case 0x40:
	return("Critical/Flash");
    case 0x48:
	return("Critical/Flash, Rekey Agent");
    case 0x50:
	return("Critical/Flash");
    case 0x58:
	return("Critical/Flash, Rekey Agent");
    case 0x60:
	return("Critical/Flash");
    case 0x68:
	return("Critical/Flash, Rekey Agent");
    case 0x70:
	return("Critical/Flash");
    case 0x78:
	return("Critical/Flash, Rekey Agent");
    }
    return "None";
}
					
static char *sec_FortezzaCertPriviledge(SECItem *priv) {
    unsigned char clr = 0;

    if (priv->len > 0) { clr = priv->data[0]; }

    return PR_smprintf("%s%s%s%s%s%s%s%s%s%s%s%s",
	clr&0x40?"Organizational Releaser":"",
	(clr&0x40) && (clr&0x3e) ? "," : "" ,
	clr&0x20?"Policy Creation Authority":"",
	(clr&0x20) && (clr&0x1e) ? "," : "" ,
	clr&0x10?"Certificate Authority":"",
	(clr&0x10) && (clr&0x0e) ? "," : "" ,
	clr&0x08?"Local Managment Authority":"",
	(clr&0x08) && (clr&0x06) ? "," : "" ,
	clr&0x04?"Configuration Vector Authority":"",
	(clr&0x04) && (clr&0x02) ? "," : "" ,
	clr&0x02?"No Signature Capability":"",
	clr&0x7e?"":"Signing Only"
    );
}

static char *htmlcertstrings[] = {
    "<table border=0 cellspacing=0 cellpadding=0><tr><td valign=top>"
    "<font size=2><b>This Certificate belongs to:</b><br>"
    "<table border=0 cellspacing=0 cellpadding=0><tr><td>",
    0, /* image goes here */
    0,
    0,
    "</td><td width=10> </td><td><font size=2>",
    0, /* subject name goes here */
    "</td></tr></table></font></td><td width=20> </td><td valign=top>"
    "<font size=2><b>This Certificate was issued by:</b><br>"
    "<table border=0 cellspacing=0 cellpadding=0><tr><td>",
    0, /* image goes here */
    0,
    0,
    "</td><td width=10> </td><td><font size=2>",
    0, /* issuer name goes here */
    "</td></tr></table></font></td></tr></table>"
    "<b>Serial Number:</b> ",
    0,
    "<br><b>This Certificate is valid from ",
    0, /* notBefore goes here */
    " to ",
    0, /* notAfter does here */
    "</b><br><b>Clearance:</b>",
    0,
    "<br><b>DSS Priviledges:</b>",
    0,
    "<br><b>KEA Priviledges:</b>",
    0,
    "<br><b>KMID:</b>",
    0,
    "<br><b>Certificate Fingerprint:</b>"
    "<table border=0 cellspacing=0 cellpadding=0><tr>"
    "<td width=10> </td><td><font size=2>",
    0, /* fingerprint goes here */
    "</td></tr></table>",
    0, /* comment header goes here */
    0, /* comment goes here */
    0, /* comment trailer goes here */
    0
};

char *
CERT_HTMLCertInfo(CERTCertificate *cert, PRBool showImages)
{
    SECStatus rv;
    char *issuer, *subject, *serialNumber, *version;
    char *notBefore, *notAfter;
    char *ret;
    char *nickname;
    SECItem dummyitem;
    unsigned char fingerprint[16];   /* result of MD5, always 16 bytes */
    char *fpstr;
    SECItem fpitem;
    char *commentstring = NULL;
    SECKEYPublicKey *pubk;
    char *DSSPriv;
    char *KMID = NULL;
    char *servername;
    
    if (!cert) {
	return(0);
    }

    issuer = CERT_FormatName (&cert->issuer);
    subject = CERT_FormatName (&cert->subject);
    version = CERT_Hexify (&cert->version,1);
    serialNumber = CERT_Hexify (&cert->serialNumber,1);
    notBefore = DER_UTCDayToAscii(&cert->validity.notBefore);
    notAfter = DER_UTCDayToAscii(&cert->validity.notAfter);
    servername = CERT_FindNSStringExtension(cert,
				   SEC_OID_NS_CERT_EXT_SSL_SERVER_NAME);

    nickname = cert->nickname;
    if ( nickname == NULL ) {
	showImages = PR_FALSE;
    }

    dummyitem.data = NULL;
    rv = CERT_FindCertExtension(cert, SEC_OID_NS_CERT_EXT_SUBJECT_LOGO,
			       &dummyitem);
    if ( dummyitem.data ) {
	PORT_Free(dummyitem.data);
    }
    
    if ( rv || !showImages ) {
	htmlcertstrings[1] = "";
	htmlcertstrings[2] = "";
	htmlcertstrings[3] = "";
    } else {
	htmlcertstrings[1] = "<img src=\"about:security?subject-logo=";
	htmlcertstrings[2] = nickname;
	htmlcertstrings[3] = "\">";
    }

    if ( servername ) {
	char *tmpstr;
	tmpstr = (char *)PORT_Alloc(PORT_Strlen(subject) +
				    PORT_Strlen(servername) +
				    sizeof("<br>") + 1);
	if ( tmpstr ) {
	    PORT_Strcpy(tmpstr, servername);
	    PORT_Strcat(tmpstr, "<br>");
	    PORT_Strcat(tmpstr, subject);
	    PORT_Free(subject);
	    subject = tmpstr;
	}
    }
    
    htmlcertstrings[5] = subject;

    dummyitem.data = NULL;
    
    rv = CERT_FindCertExtension(cert, SEC_OID_NS_CERT_EXT_ISSUER_LOGO,
			       &dummyitem);
    if ( dummyitem.data ) {
	PORT_Free(dummyitem.data);
    }
    
    if ( rv || !showImages ) {
	htmlcertstrings[7] = "";
	htmlcertstrings[8] = "";
	htmlcertstrings[9] = "";
    } else {
	htmlcertstrings[7] = "<img src=\"about:security?issuer-logo=";
	htmlcertstrings[8] = nickname;
	htmlcertstrings[9] = "\">";
    }

    htmlcertstrings[11] = issuer;
    htmlcertstrings[13] = serialNumber;
    htmlcertstrings[15] = notBefore;
    htmlcertstrings[17] = notAfter;

    pubk = CERT_ExtractPublicKey(&cert->subjectPublicKeyInfo);
    DSSPriv = NULL;
    if (pubk && (pubk->keyType == fortezzaKey)) {
	htmlcertstrings[18] = "</b><br><b>Clearance:</b>";
	htmlcertstrings[19] = sec_FortezzaClearance(
					&pubk->u.fortezza.clearance);
	htmlcertstrings[20] = "<br><b>DSS Priviledges:</b>";
	DSSPriv = sec_FortezzaCertPriviledge(
					&pubk->u.fortezza.DSSpriviledge);
	htmlcertstrings[21] = DSSPriv;
	htmlcertstrings[22] = "<br><b>KEA Priviledges:</b>";
	htmlcertstrings[23] = sec_FortezzaMessagePriviledge(
					&pubk->u.fortezza.KEApriviledge);
	htmlcertstrings[24] = "<br><b>KMID:</b>";
	dummyitem.data = &pubk->u.fortezza.KMID[0];
	dummyitem.len = sizeof(pubk->u.fortezza.KMID);
	KMID = CERT_Hexify (&dummyitem,0);
	htmlcertstrings[25] = KMID;
    } else {
	/* clear out the headers in the non-fortezza cases */
	htmlcertstrings[18] = "";
	htmlcertstrings[19] = "";
	htmlcertstrings[20] = "";
	htmlcertstrings[21] = "";
	htmlcertstrings[22] = "";
	htmlcertstrings[23] = "";
	htmlcertstrings[24] = "";
	htmlcertstrings[25] = "</b>";
    }
#define HTML_OFF 27
    MD5_HashBuf(fingerprint, cert->derCert.data, cert->derCert.len);
    
    fpitem.data = fingerprint;
    fpitem.len = sizeof(fingerprint);

    fpstr = CERT_Hexify (&fpitem,1);
    
    htmlcertstrings[HTML_OFF] = fpstr;

    commentstring = CERT_GetCertCommentString(cert);

    if (commentstring == NULL) {
	htmlcertstrings[HTML_OFF+2] = "";
	htmlcertstrings[HTML_OFF+3] = "";
	htmlcertstrings[HTML_OFF+4] = "";
    } else {
	htmlcertstrings[HTML_OFF+2] =
	    "<b>Comment:</b>"
	    "<table border=0 cellspacing=0 cellpadding=0><tr>"
	    "<td width=10> </td><td><font size=3>"
	    "<textarea name=foobar rows=4 cols=55 onfocus=\"this.blur()\">";
	htmlcertstrings[HTML_OFF+3] = commentstring;
	htmlcertstrings[HTML_OFF+4] = "</textarea></font></td></tr></table>";
    }
    
    ret = gatherStrings(htmlcertstrings);
    
    if ( issuer ) {
	PORT_Free(issuer);
    }
    
    if ( subject ) {
	PORT_Free(subject);
    }
    
    if ( version ) {
	PORT_Free(version);
    }
    
    if ( serialNumber ) {
	PORT_Free(serialNumber);
    }
    
    if ( notBefore ) {
	PORT_Free(notBefore);
    }
    
    if ( notAfter ) {
	PORT_Free(notAfter);
    }
    
    if ( fpstr ) {
	PORT_Free(fpstr);
    }
    if (DSSPriv) {
	PORT_Free(DSSPriv);
    }

    if (KMID) {
	PORT_Free(KMID);
    }

    if ( commentstring ) {
	PORT_Free(commentstring);
    }
    
    if ( servername ) {
	PORT_Free(servername);
    }
    
    return(ret);
}

