/*
 * Fortezza Security stuff specific to the client (mozilla).
 *
 * Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: fortdlgs.c,v 1.5.26.1 1997/05/03 09:25:06 jwz Exp $
 */

#include "cert.h"
#include "net.h"
#include "xp.h"
#include "../cert/certdb.h"
#include "../ssl/sslimpl.h"
#include "htmldlgs.h"
#include "sslproto.h"
#include "sslerr.h"
#include "../crypto/fortezza.h"
#include "fortdlgs.h"
#include "ssl.h"
#include "prmem.h"
#include "xpgetstr.h"
#include "prtime.h"

extern FortezzaGlobal fortezzaGlobal;
extern int XP_ERRNO_EWOULDBLOCK;
extern int XP_SELECT_FORTEZZA_CARD;
extern int XP_SELECT_FORTEZZA_PERSONALITY;
extern int XP_SHOW_FORTEZZA_PERSONALITY;
extern int XP_SHOW_FORTEZZA_CARD_INFO;
extern int XP_PLAIN_CERT_DIALOG_STRINGS;
extern int XP_SEC_FORTEZZA_NO_CARD;
extern int XP_SEC_FORTEZZA_NONE_SELECTED;
extern int XP_SEC_FORTEZZA_MORE_INFO;
extern int XP_SEC_FORTEZZA_PERSON_NOT_FOUND;
extern int XP_SEC_FORTEZZA_NO_MORE_INFO;
extern int XP_SEC_FORTEZZA_BAD_PIN;
extern int XP_SEC_FORTEZZA_PERSON_ERROR;
extern int XP_SEC_FORTEZZA_NO_CERT;
extern int XP_SEC_FORTEZZA_SOCKET;
extern int XP_SEC_FORTEZZA_SERIAL;
extern int XP_SEC_FORTEZZA_EMPY;
extern int XP_SEC_FORTEZZA_LOGIN;
extern int XP_SEC_FORTEZZA_LOGOUT;
extern int XP_SEC_FORTEZZA_INIT;
extern int XP_FORTEZZA_CHOOSE_CARD;
extern int XP_FORTEZZA_CHOOSE_PERSONALITY;
extern int XP_FORTEZZA_VIEW_PERSONALITY;
extern int XP_FORTEZZA_PERSONALITY;
extern int XP_FORTEZZA_CARD_INFO;


/*
 * Dialog Helper routines... Stold from certdlg.c
 */


/*
 * Add a socket to the linked list of sockets waiting for client auth to
 * be approved by the user for the certificate.
 */
static SECStatus
AddSockToGlobal(SSLSocket *ss)
{
    SECSocketNode *node;
    
    node = PORT_ZAlloc(sizeof(SECSocketNode));
    if ( node ) {
	node->ss = ss;
	node->next = fortezzaGlobal.sslList;
	fortezzaGlobal.sslList = node;

	ss->dialogPending = PR_TRUE;
	
	return(SECSuccess);
    }

    return(SECFailure);
}

/*
 * Handshake function that blocks.  Used to force a
 * retry on a connection on the next read/write.
 */
static int
AlwaysBlock(SSLSocket *ss)
{
    PORT_SetError(XP_ERRNO_EWOULDBLOCK);
    return -2;
}

/*
 * set the handshake to block
 */
static void
SetAlwaysBlock(SSLSocket *ss)
{
    ss->handshake = AlwaysBlock;
    ss->nextHandshake = 0;
    return;
}

static void
FortezzaMarkSocketWait(SSLSocket *ss) {
    AddSockToGlobal(ss);
    SetAlwaysBlock(ss);

    PORT_SetError(XP_ERRNO_EWOULDBLOCK);
}

SECSocketNode *
FreeNode(SECSocketNode *np) {
    SECSocketNode *next = np->next;

    PORT_Free(np);
    return next;
}

static void
RestartOpen(PRBool restart) {
    SECSocketNode *node;
    SSLSocket *ss;
    SECStatus rv;

    if (!restart) fortezzaGlobal.state = GlobalSkip;

    for (node = fortezzaGlobal.sslList;  node; node = FreeNode(node)) {
	if (fortezzaGlobal.sslList) fortezzaGlobal.sslList = NULL;
	ss = node->ss;

	if ( ss->beenFreed ) {
	    ssl_FreeSocket(ss);
	    continue;
	}

	ss->dialogPending = PR_FALSE;

	rv = SSL_RestartHandshakeAfterFortezza(ss);
	if (rv == SECFailure) {
	    NET_InterruptSocket(ss->fd);
	}
    }

    fortezzaGlobal.state = GlobalOK;
}

/*
 * Family of routines to support choosing a card
 */
/* (OK only the Serial: and the Socket).  */
#define OPTION_STR "<option>"
#define SELECT_STR " selected"
#define OPTIONSEL_STR "<option selected>"
#define OPTION_STR_LEN (sizeof(OPTION_STR)-1)
#define SELECT_STR_LEN (sizeof(SELECT_STR)-1)
#define OPTIONSEL_STR_LEN (sizeof(OPTIONSEL_STR)-1)
#define SERIAL_STR "</b> Serial: "
#define SERIAL_STR_LEN (sizeof(SERIAL_STR)-1)
/* Some of following strings need to be internationalizes! */
static char *card_states[] = {
    (char *)0, (char *)0, (char *)0, (char *)0
};

static int max_state_len = 0;

static char itoh(int x) {
   x &= 0xf;
   if (x > 9) return x + 'a' - 10;
   return x + '0';
}

static PRBool
fortezzaChooseCardDialogHandler(XPDialogState *state, char **argv, int argc,
					unsigned int button) {
    fortezzaGlobal.state = GlobalOK;
    if (button == XP_DIALOG_OK_BUTTON) {
	char *cardname = XP_FindValueInArgs("cards", argv, argc);
	char *cp;
	unsigned int socket = 0;

	/*
	 * lots of magic numbers below. We are extracting '_Socket_nn'
	 * on the 'nn' is of interest to use, all the rest was info for
	 * the user...
 	 */
	for (cp=cardname; *cp; cp++) {
	    if ((*cp >= '0') || (*cp <= '9')) {
		break;
	    }
	}
	socket = atoi(cp);

	socket -= 1; /* convert from socket number */

	/* do an unsigned compare in case socket is negative */
	if (socket < (unsigned)fortezzaGlobal.socketCount) {
	    FortezzaPCMCIASocket *psock=&fortezzaGlobal.sockets[socket];
	    if (psock->state != NoCard) {
		fortezzaGlobal.defaultSocket = psock;
		RestartOpen(PR_TRUE);
		return PR_FALSE;
	    }
	}
    }
    RestartOpen(PR_FALSE);
    return PR_FALSE;
}


	
 
static XPDialogInfo fortezzaChooseCardDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    fortezzaChooseCardDialogHandler,
    450,
    250
};


#define MAX_CARDS 32
SECStatus
FortezzaChooseCard(MWContext *cx,FortezzaCardInfo *info,uint32 mask,
								int count) {
	XPDialogStrings *strings;
	char *cards,*tmp;
	int i,j,len;
	char *sock_name = PORT_Strdup(XP_GetString(XP_SEC_FORTEZZA_SOCKET));
	int sock_len = PORT_Strlen(sock_name);
	char *serial_name = PORT_Strdup(XP_GetString(XP_SEC_FORTEZZA_SERIAL));
	int serial_len = PORT_Strlen(serial_name);

	if (mask == 0) {
	     /* FE_Alert */	
	     FE_Alert(cx,XP_GetString(XP_SEC_FORTEZZA_NO_CARD));
    	     fortezzaGlobal.state = GlobalOK;
	     PORT_Free(sock_name);
	     PORT_Free(serial_name);
	     return SECFailure;
	}
	if (max_state_len == 0) {
	    int i=0;

	    card_states[i++] = PORT_Strdup(XP_GetString(XP_SEC_FORTEZZA_EMPY));
	    card_states[i++] = PORT_Strdup(XP_GetString(XP_SEC_FORTEZZA_LOGIN));
	    card_states[i++] = PORT_Strdup(XP_GetString(XP_SEC_FORTEZZA_LOGOUT));
	    card_states[i++] = PORT_Strdup(XP_GetString(XP_SEC_FORTEZZA_INIT));

	    for(i=0; i < 4; i++) {
		int l = PORT_Strlen(card_states[i]);

		if (l > max_state_len) max_state_len = l;
	    }
	}

	strings = XP_GetDialogStrings(XP_SELECT_FORTEZZA_CARD);
	cards = tmp = (char *) PORT_Alloc(count * 
		(OPTION_STR_LEN + sock_len + 3/* sock_no */ +
	   	max_state_len + serial_len + 20) + SELECT_STR_LEN+10);
	for (i=0; i < count; i++) {
	    char sock_no[10];
	    char serial[16];

	    int socket = i+1;
	    if (mask & (1<<i)) {
		if (fortezzaGlobal.defaultSocket && 
			   fortezzaGlobal.defaultSocket->socketId == socket) { 
	        	PORT_Memcpy(tmp, OPTIONSEL_STR, OPTIONSEL_STR_LEN );
			tmp += OPTIONSEL_STR_LEN;
		} else {
	        	PORT_Memcpy(tmp, OPTION_STR, OPTION_STR_LEN );
			tmp += OPTION_STR_LEN;
		}
	       	PORT_Memcpy(tmp, sock_name, sock_len );
		tmp += sock_len;
		PR_snprintf(sock_no,sizeof(sock_no),"%d <b>",socket);
		len = PORT_Strlen(sock_no);
		PORT_Memcpy(tmp,sock_no,len);	
		tmp += len;
		len = PORT_Strlen(card_states[info[i].state]);
		PORT_Memcpy(tmp,card_states[info[i].state],len);	
		tmp +=len;
		/* someday we should build a data base between serial
		 * number and "primary" user cert names so the user
		 * has a little nicer info about the card they are using..
		 */
		PORT_Memcpy(tmp,serial_name,serial_len);
		tmp += serial_len;

		for (j=0; j < 8; j++) {
			unsigned char n = info[i].serial[j];
			serial[j*2] = itoh((n >> 4) & 0xf);
			serial[j*2+1] = itoh(n & 0xf);
		}
		PORT_Memcpy(tmp,serial,sizeof(serial));
		tmp += sizeof(serial);
	    }
	}
	*tmp = 0;
	XP_CopyDialogString(strings, 0, cards);
	PORT_Free(cards);

	XP_MakeHTMLDialog(cx,&fortezzaChooseCardDialog,
				 XP_FORTEZZA_CHOOSE_CARD,strings, NULL);
	XP_FreeDialogStrings(strings);
	PORT_Free(sock_name);
	PORT_Free(serial_name);
	return SECWouldBlock;
}

static int
fortezzaCertIndexByName(FortezzaPCMCIASocket *psock,char *personality,int len) {
	char **personalities;
	int *indexes;
	int maxCount, i;
	SECStatus rv;
	int index = -1;

	     
	rv = FortezzaLookupPersonality(psock, &personalities, &maxCount,
								&indexes);
	if (rv < 0) goto loser;

	for (i=0; i < maxCount; i++) {
	    if (PORT_Strncmp(personalities[i],personality,len) == 0) {
		index= indexes[i];
	    }
	}
	FortezzaFreePersonality(personalities);
	PORT_Free(indexes);
loser:
	return index;
}
/*
 * Family of routines to support choosing a personality
 */

static PRBool
fortezzaSelectPersonalityDialogHandler(XPDialogState *state, char **argv,
				 	     int argc, unsigned int button) {
    FortezzaPCMCIASocket *psock = fortezzaGlobal.defaultSocket;
    PRBool found = PR_FALSE;

    /* defensive... */
    fortezzaGlobal.state = GlobalOK;
    if ((psock == NULL) || (psock->state == NoCard)) {
	/* FE_ALER */
	goto loser;
    }

    if (button == XP_DIALOG_OK_BUTTON) {
	char *personality = XP_FindValueInArgs("personality", argv, argc);
	int index;

	index = fortezzaCertIndexByName(psock,personality,
						PORT_Strlen(personality));
	if (index > 0) {
		psock->template.certificate = index;
		psock->state = Ready;
		found = PR_TRUE;
	}
    }
loser:
    RestartOpen(found);
    return PR_FALSE;
}


	
 
static XPDialogInfo fortezzaSelectPersonalityDialog = {
    XP_DIALOG_OK_BUTTON | XP_DIALOG_CANCEL_BUTTON,
    fortezzaSelectPersonalityDialogHandler,
    450,
    250
};


SECStatus
FortezzaSelectPersonality(MWContext *cx,char **names, int count, int def_cert) {
	XPDialogStrings *strings;
	char *personalities,*tmp;
	int i,len;

	strings = XP_GetDialogStrings(XP_SELECT_FORTEZZA_PERSONALITY);

	len = 0;
	for (i=0 ; i < count; i++) {
	    len += PORT_Strlen(names[i]);
	}
	personalities = tmp = (char *) 
		PORT_Alloc(count*OPTION_STR_LEN+SELECT_STR_LEN+len+1);
	for (i=0; i < count; i++) {
	    if (i == def_cert) {
	    	PORT_Memcpy(tmp, OPTIONSEL_STR, OPTIONSEL_STR_LEN );
	    	tmp += OPTIONSEL_STR_LEN;
	    } else {
	    	PORT_Memcpy(tmp, OPTION_STR, OPTION_STR_LEN );
	        tmp += OPTION_STR_LEN;
	    }
	    len = PORT_Strlen(names[i]);
	    PORT_Memcpy(tmp,names[i],len);
	    tmp += len;
	}
	*tmp = 0;
	XP_CopyDialogString(strings, 0, personalities);
	PORT_Free(personalities);

	XP_MakeHTMLDialog(cx,&fortezzaSelectPersonalityDialog, 
			XP_FORTEZZA_CHOOSE_PERSONALITY, strings, NULL);
	XP_FreeDialogStrings(strings);
	return SECWouldBlock;
}

/*
 * Family of routines to support listing the personalities
 */
/* The following strings need to be internationalizes! */

static XPDialogInfo certDialog = {
	XP_DIALOG_OK_BUTTON,
	0,
	600,
	400
};

/* return true if the cert for this bit is set in the cert flags */
static PRBool
FortezzaCertBits(unsigned char *bits, int cert) {
	int bitIndex = cert >> 3; /* cert div 8 */
	int bitLoc = 0x80 >> (cert & 0x7); /* cert mod 8 */

	return ((bits[bitIndex] & bitLoc) == bitLoc);
}

static int
fortezzaAnyCertIndexByName(FortezzaPCMCIASocket *psock,char *personality,
								int len) {
	int i;
	int index = -1;
	CI_PERSON *pp = psock->personalityList;
	char *name;

	for (i=0; i < psock->certificateCount; i++) {
	    if (!FortezzaCertBits(psock->certFlags,i)) continue;
            name = (char *)&pp[i].CertLabel[8];
	    if (PORT_Strncmp(name,personality,len) == 0) {
		index = pp[i].CertificateIndex;
		break;
	    }
	}

	return index;
}

static PRBool
fortezzaShowPersonalityDialogHandler(XPDialogState *state, char **argv,
				 	     int argc, unsigned int button) {
    char *certstr;
    XPDialogStrings *strings;
    
    
    /* bring up the "More Info..." dialog, which gives detailed
     * certificate info.
     */
    if ( button == XP_DIALOG_MOREINFO_BUTTON ) {
	CERTCertificate *cert;
	char *personality = XP_FindValueInArgs("personality", argv, argc);
	int len,index; 
	MWContext *cx = (MWContext *)state->arg;
	FortezzaPCMCIASocket *psock = fortezzaGlobal.defaultSocket;

	if (personality == NULL) {
	    FE_Alert(cx,XP_GetString(XP_SEC_FORTEZZA_MORE_INFO));
	    return PR_TRUE;
	}

	/* find up to the first '(' */
	for (len=0;personality[len] && personality[len] != '(';len++); 

	index = fortezzaAnyCertIndexByName(psock,personality,len);
	if (index < 0) {
	    FE_Alert(cx, XP_GetString(XP_SEC_FORTEZZA_PERSON_NOT_FOUND));
	    return PR_FALSE;
	}
	cert = FortezzaGetCertificate(psock->socketId,index, psock->serial);
	if (cert == NULL) {
	    FE_Alert(cx,XP_GetString(XP_SEC_FORTEZZA_NO_MORE_INFO));
	    return PR_TRUE;
	}
	certstr = CERT_HTMLCertInfo(cert, PR_TRUE);

	strings = XP_GetDialogStrings(XP_PLAIN_CERT_DIALOG_STRINGS);
	if ( strings ) {
	    XP_SetDialogString(strings, 0, certstr);
	    XP_MakeHTMLDialog(cx, &certDialog, XP_FORTEZZA_PERSONALITY,
			      strings, 0);
	    XP_FreeDialogStrings(strings);
	}
	PORT_Free(certstr);
	CERT_DestroyCertificate(cert);
	return PR_TRUE;
    }
    
    return PR_FALSE;
}

static XPDialogInfo fortezzaShowPersonalityDialog = {
    XP_DIALOG_OK_BUTTON,
    fortezzaShowPersonalityDialogHandler,
    450,
    350
};


SECStatus
FortezzaShowPersonality(MWContext *cx,CI_PERSON *pp, int size, int count,
						unsigned char *certflags) {
	XPDialogStrings *strings;
	char *personalities,*tmp;
	int i,len;

	strings = XP_GetDialogStrings(XP_SHOW_FORTEZZA_PERSONALITY);

#define TABLE_SIZE (9 /*size of <option>'s */ + \
		24 /* pers name */ + 24 /* parent name */ + 40 /* type */)
	personalities = tmp = (char *) PORT_Alloc(count * TABLE_SIZE + 1);
	for (i=0; i < size; i++) {
	    int user = 0, kea = 0, sig = 0;
	    int j,index;
	    char *parent,*name, *typeName;
	    char type[5];

	    if (!FortezzaCertBits(certflags,i)) continue;
	    if (pp[i].CertLabel[0] == 0) continue;
	    if (pp[i].CertLabel[2] == 'K') {
		kea++; user++;
	    }
	    if (pp[i].CertLabel[3] == 'S') {
		sig++; user++;
	    }
	    name = (char *)&pp[i].CertLabel[8];
	    PORT_Memcpy(type,(char *)pp[i].CertLabel,4);
	    type[4] = 0;
	    index = atoi((char *)&pp[i].CertLabel[4]);

	    if ((pp[i].CertLabel[0] == 'I') && (pp[i].CertLabel[1] == 'N')) {
		typeName="Individual";
	    } else if ((pp[i].CertLabel[0] == 'O') &&
	       ((pp[i].CertLabel[1] == 'N') || (pp[i].CertLabel[1] == 'R'))) {
		typeName="Organization";
	    } else if ((pp[i].CertLabel[0] == 'R') && 
						(pp[i].CertLabel[1] == 'R')) {
		typeName="Policy Approval Authority";
	    } else if ((pp[i].CertLabel[0] == 'R') && 
						(pp[i].CertLabel[1] == 'T')) {
		typeName="Policy Creation Authority";
	    } else if ((pp[i].CertLabel[0] == 'L') && 
						(pp[i].CertLabel[1] == 'A')) {
		typeName="Certification Authority (CA)";
	    } else if ((pp[i].CertLabel[0] == 'S') && 
						(pp[i].CertLabel[1] == 'T')) {
		typeName="Secure Telephone Unit";
	    } else if ((pp[i].CertLabel[0] == 'F') && 
						(pp[i].CertLabel[1] == 'A')) {
		typeName="Fax Data";
	    } else {
		typeName=type;
	    }

	    if (index == 0) {
	    	parent = "PAA Root";
	    } else {
		parent = "unknown";
		for (j=0; j < count; j++) {
		    if (index == pp[j].CertificateIndex) {
			parent = (char *)&pp[j].CertLabel[8];
		    }
		}
	    }
	    len = PR_snprintf(tmp,TABLE_SIZE,
  		"<option>%s (%s)%s\n",name,typeName,user? "*": " ");
	    tmp += len;
	}
	*tmp = 0;
	XP_CopyDialogString(strings, 0, personalities);
	PORT_Free(personalities);

	XP_MakeHTMLDialog(cx,&fortezzaShowPersonalityDialog,
		XP_FORTEZZA_VIEW_PERSONALITY, strings, cx);
	XP_FreeDialogStrings(strings);
	return SECWouldBlock;
}
/*
 * Family of routines to support choosing a card
 */
/* The following strings need to be internationalizes! */

static PRBool
fortezzaCardInfoDialogHandler(XPDialogState *state, char **argv,
				 	     int argc, unsigned int button) {
    return PR_FALSE;
}


	
 
static XPDialogInfo fortezzaCardInfoDialog = {
    XP_DIALOG_OK_BUTTON,
    fortezzaCardInfoDialogHandler,
    450,
    370
};



#define MK_VERSION(x) \
      ((unsigned)(x)) >> 8, (((unsigned)(x)) >> 4) & 0xf, ((x)&0xf)
SECStatus
FortezzaShowCardInfo(MWContext *cx,FortezzaPCMCIASocket *psock) {
	XPDialogStrings *strings;
	CI_CONFIG config;
	CI_STATUS status;
	char serial[17];
	unsigned char *serialPtr;
	char *buf;
	char **personalities;
	int  *indexes;
	char *none_selected =
		PORT_Strdup(XP_GetString(XP_SEC_FORTEZZA_NONE_SELECTED));
	char *no_cert= PORT_Strdup(XP_GetString(XP_SEC_FORTEZZA_NO_CERT));
	int	rv,i,maxCount;
	int	next=0;
	int 	italic = 0;

	strings = XP_GetDialogStrings(XP_SHOW_FORTEZZA_CARD_INFO);

	/* deal with the socket number
	 * italicize the whole thing if we had any problems.
	 */
	rv = FortezzaGetConfig(psock,&config,&status);
	if (rv == SECSuccess) {
	    XP_CopyDialogString(strings, next, ""); next ++;
	} else {
	    XP_CopyDialogString(strings, next, "<i>"); next ++;
	     italic++;
	}
	     
	if (psock) {
	     buf = PR_smprintf("%d",psock->socketId);
	} else {
	     buf = PR_smprintf(none_selected);
	}
	XP_CopyDialogString(strings, next, buf); next ++;
	PR_DELETE(buf);

	/* deal with card state */
	if (psock) {
	    XP_CopyDialogString(strings, next,
					card_states[psock->state]); next++;
	} else {
	    XP_CopyDialogString(strings, next, none_selected); next++;
	}

	/* deal with card the serial number */
	if (psock) {
	    serialPtr = psock->serial;
	} else {
	    serialPtr = status.SerialNumber;
	}
	for (i=0; i < 8; i++) {
		unsigned char n = serialPtr[i];
		serial[i*2] = itoh((n >> 4) & 0xf);
		serial[i*2+1] = itoh(n & 0xf);
	}
	serial[16] = 0;
	XP_CopyDialogString(strings, next, serial); next ++;

	/* deal with card the user certificate */
	buf = no_cert;
	rv = SECFailure;
	if (psock && psock->state == Ready) {
	    rv = FortezzaLookupPersonality(psock, &personalities, &maxCount,
								&indexes);
	    if (rv == SECSuccess) {
		for (i=0; i < maxCount; i++) {
		    if (psock->template.certificate == indexes[i]) {
			buf = personalities[i];
			break;
		    }
	    	}
	    }
	}
	XP_CopyDialogString(strings, next, buf); next ++;
	if (rv == SECSuccess) {
	    FortezzaFreePersonality(personalities);
	    PORT_Free(indexes);
	}


	/* now deal with the config parameters */
	buf = PR_smprintf("%d.%d.%d", MK_VERSION(config.LibraryVersion));
	XP_CopyDialogString(strings, next, buf); next ++;
	PR_DELETE(buf);
	
	buf = PR_smprintf("%d.%d.%d", MK_VERSION(config.ManufacturerVersion));
	XP_CopyDialogString(strings, next, buf); next ++;
	PR_DELETE(buf);

	XP_CopyDialogString(strings, next, config.ManufacturerName); next ++;
	XP_CopyDialogString(strings, next, config.ProductName); next ++;
	XP_CopyDialogString(strings, next, config.ProcessorType); next ++;

	buf = PR_smprintf("%ld", config.UserRAMSize);
	XP_CopyDialogString(strings, next, buf); next ++;
	PR_DELETE(buf);

	buf = PR_smprintf("%ld", config.LargestBlockSize);
	XP_CopyDialogString(strings, next, buf); next ++;
	PR_DELETE(buf);

	buf = PR_smprintf("%d",config.KeyRegisterCount);
	XP_CopyDialogString(strings, next, buf); next ++;
	PR_DELETE(buf);

	buf = PR_smprintf("%d",config.CertificateCount);
	XP_CopyDialogString(strings, next, buf); next ++;
	PR_DELETE(buf);

	XP_CopyDialogString(strings, next, 
			config.CryptoCardFlag ? "on" : "off"); next ++;

	buf = PR_smprintf("%d.%d.%d", MK_VERSION(config.ICDVersion));
	XP_CopyDialogString(strings, next, buf); next ++;
	PR_DELETE(buf);

	buf = PR_smprintf("%d.%d.%d", MK_VERSION(config.ManufacturerSWVer));
	XP_CopyDialogString(strings, next, buf); next ++;
	PR_DELETE(buf);

	buf = PR_smprintf("%d.%d.%d%s", MK_VERSION(config.DriverVersion),
							italic?"</i>":" ");
	XP_CopyDialogString(strings, next, buf); next ++;
	PR_DELETE(buf);

	XP_MakeHTMLDialog(cx,&fortezzaCardInfoDialog,
					 XP_FORTEZZA_CARD_INFO,strings, NULL);
	XP_FreeDialogStrings(strings);
	PORT_Free(none_selected);
	PORT_Free(no_cert);
	return SECSuccess;
}

void
SSL_FortezzaMenu(MWContext *cx, int type) {
	FortezzaCardInfo *info;
	uint32 cardMask;
	int maxCount,found,rv;
	char **personalities;
	int *indexes;
	FortezzaPCMCIASocket *psock;
	int default_cert;


	/* hack.. only works on on-threaded systems!!!!! */
	fortezzaGlobal.fortezzaAlertArg = cx;

	switch (type) {
	case SSL_FORTEZZA_CARD_SELECT:
	    /* Don't changes cards if we are already monkeying with
	     * picking a personality...
	     */
	    if (fortezzaGlobal.state == GlobalOpen) {
		break;
	    }
	    rv = FortezzaLookupCard(&info,&cardMask,&maxCount,
						&found,NULL,NULL,PR_FALSE);
	    if (rv < 0) goto loser; /* shouldn't happen unless we trash
				      * something... */
	    fortezzaGlobal.state = GlobalOpen;
	    (void) FortezzaChooseCard(cx,info,cardMask,maxCount);
	    PORT_Free(info);
	    break;
	case SSL_FORTEZZA_CHANGE_PERSONALITY:
	case SSL_FORTEZZA_VIEW_PERSONALITY:
	    psock = fortezzaGlobal.defaultSocket;
	    if ((psock == NULL) || (psock->state == NoCard)) {
		/* FE_Alert */
	        FE_Alert(cx,XP_GetString(XP_SEC_FORTEZZA_NONE_SELECTED));
		goto loser;
	    }

	    /* Don't changes cards if we are already monkeying with
	     * picking a personality...
	     */
	    if (fortezzaGlobal.state == GlobalOpen) {
		break;
	    }

	    if (!LL_IS_ZERO(fortezzaGlobal.timeout)) {
		/* time out fortezza cards */
		int64 timeout;
		int64 now = PR_NowS();
		LL_ADD(timeout,psock->lasttime,fortezzaGlobal.timeout);
		if (LL_CMP(now, > , timeout)) {
		    psock->state = NeedPin;
		}
	    }

	    psock->lasttime = PR_NowS();

	    if (psock->state == NeedPin) {
		if (SECMOZ_GetPin(cx,psock) != SECSuccess) {
		    /* FE_Alert */
	            FE_Alert(cx,XP_GetString(XP_SEC_FORTEZZA_BAD_PIN));
		    goto loser;
		}
	        psock->state = NeedCert;
		rv = FortezzaCertInit(psock);
		if (rv < 0) {
		     /* FE_Alert */
		     FE_Alert(cx,XP_GetString(XP_SEC_FORTEZZA_PERSON_ERROR));
		     psock->state = NeedPin;
		     goto loser;
		}
	    }

	    if (type == SSL_FORTEZZA_VIEW_PERSONALITY) {
		int count = 0,i;

		for (i=0; i < psock->certificateCount; i++) {
		    if (FortezzaCertBits(psock->certFlags,i)) count++;
		}

		FortezzaShowPersonality(cx,psock->personalityList,
			psock->certificateCount,count,psock->certFlags);
		break;
	    }
	     
	    rv = FortezzaLookupPersonality(psock, &personalities, &maxCount,
								&indexes);

	    /* figure out which is our default so it can be marked selected */
	    default_cert = -1;
	    if (psock->state == Ready) {
		int i;

		for (i=0; i < maxCount; i++) {
		   if (indexes[i] == psock->template.certificate) {
			default_cert = i;
		   }
		}
	    }
		
	    if (rv < 0) {
		 goto loser;
	    }

	    if (maxCount == 0) {
		/* FE_Alert */
		goto loser;
	    }
	    fortezzaGlobal.state = GlobalOpen;
	    (void) FortezzaSelectPersonality(cx,personalities,maxCount,
								default_cert);
	    FortezzaFreePersonality(personalities);
	    break;
	     
	case SSL_FORTEZZA_CARD_INFO:
	    psock = fortezzaGlobal.defaultSocket;
	    (void) FortezzaShowCardInfo(cx,psock);
	    break;

	case SSL_FORTEZZA_LOGOUT:
	    FortezzaLogout();
	    break;
	}
loser:
    return;
}

/*
 * fortezza already has a certificate selected... use that one
 */
SECStatus
FortezzaClientAuthData(SSLSocket *ss,
			 CERTCertificate **pRetCert,
			 SECKEYPrivateKey **pRetKey)
{
    CERTCertificate *cert;
    SECKEYPrivateKey *privkey;
    FortezzaKEAContext *cv = (FortezzaKEAContext *) ss->ssl3->kea_context;

    PORT_Assert(cv != NULL);
    cert = FortezzaGetCertificate(cv->socket->socketId,
					cv->certificate,cv->socket->serial);
    if (cert == NULL) {
	return SECFailure;
    }

    privkey = FortezzaGetPrivateKey(cv);
    if (privkey == NULL) {
	CERT_DestroyCertificate(cert);
	return SECFailure;
    }
    
    *pRetCert = cert;
    *pRetKey = privkey;
    
    return(SECSuccess);
}

SECStatus
SECMOZ_FortezzaInit() {
    fortezzaGlobal.fortezzaMarkSocket = 
			(FortezzaMarkSocket) FortezzaMarkSocketWait;
    return SECSuccess;
}

/*
 * this needs to become a dialog some day
 */
#define PIN_STRING "Enter the PIN for your Fortezza Card in\nSocket %d. Note: 10 consecutive failures disables the card."
SECStatus
SECMOZ_GetPin(void *arg,void *psock)
{
    MWContext *cx;
    char *pin;
    SECStatus rv;
    char buf[200]; 
    
    cx = (MWContext *)arg;

    /* Use XP_Getstrings for internationalization */
    PR_snprintf(buf,sizeof(buf),
		PIN_STRING,((FortezzaPCMCIASocket *)psock)->socketId);
    while (1) {
    	pin = FE_PromptPassword(cx,buf);

	if (pin == 0) {
    	    return SECFailure;
	}
	rv = FortezzaPinCheck((FortezzaPCMCIASocket *)psock,pin);
	PORT_ZFree(pin,PORT_Strlen(pin));
	/* We use SECWouldBlock to indicate a soft failure (the pin did
	 * not match. Hard failures are when the card finally locks up:).
	 */
	if (rv == SECWouldBlock) {
		continue;
	}
	return rv;
    }
    return SECFailure; /* Not Reached */
}

/*
 * this one also need to become a dialog some day.
 */
SECStatus
SECMOZ_CertificateSelect(void *arg, int fd, char **names, int certCount, 
								int *cert) {
    MWContext *cx;
    SSLSocket *ss;
    
    cx = (MWContext *)arg;

    ss = ssl_FindSocket(fd);
    (void) FortezzaSelectPersonality(cx,names,certCount,-1);
    FortezzaMarkSocketWait(ss);
    fortezzaGlobal.state = GlobalOpen;

    return SECWouldBlock;
}

/*
 * and guess what... so does this one...
 */
SECStatus
SECMOZ_CardSelect(void *arg, int fd, void *ia, int cardmask,  int *psockId) {
    MWContext *cx;
    SSLSocket *ss;
    FortezzaCardInfo *info = (FortezzaCardInfo *) ia;
    
    cx = (MWContext *)arg;

    ss = ssl_FindSocket(fd);
    (void) FortezzaChooseCard(cx,info,cardmask,fortezzaGlobal.socketCount);
    FortezzaMarkSocketWait(ss);
    fortezzaGlobal.state = GlobalOpen;

    return SECWouldBlock;

}

/*
 * pop-up an alert box
 */
SECStatus
SECMOZ_FortezzaAlert(void *arg,PRBool onOpen,char *string,
						int value1,void *value2) {
    MWContext *cx;
    char *output_string;

    cx = (MWContext *)arg;


    if (onOpen) {
	output_string = PR_smprintf(string,value1,value2);
	FE_Alert(cx, output_string);
    }
    return SECSuccess;
} 

/*
 * go get a card based on it's serial number.
 */
FortezzaKEAContext *
SECMOZ_FindCardBySerial(void *wcx,unsigned char *serial) {
	int rv,i;
	uint32 cards = 0;
	int count=0;
	FortezzaPCMCIASocket *last_sock = NULL;
	FortezzaPCMCIASocket *min_sock = NULL;
	FortezzaKEAContext *cx = NULL;


	/* hack -- not thread safe... */
	fortezzaGlobal.fortezzaAlertArg = wcx;
	rv = FortezzaLookupCard( NULL,&cards,NULL,
					&count,&min_sock,&last_sock,PR_FALSE);
	if (rv < 0) return NULL;

	for (i=0; i < fortezzaGlobal.socketCount; i++) {
	    if (cards & (1<<i)) {
		FortezzaPCMCIASocket *psock = &fortezzaGlobal.sockets[i];
		if (PORT_Memcmp(serial,psock->serial,
					sizeof(CI_SERIAL_NUMBER)) == 0) {
		    /* SIGH, I'd like to to a FortezzaCardOpen here, but
		     * I may need to call SECMOZ_GetPin.. I don't have an
		     * SSL Socket get my callback from, so I have to call
		     * it directly. That's why it's here, so it doesn't
		     * pull in a bunch of User code into the server .
		     */
		    FORTEZZALOCK(psock->stateLock,psock->stateLockInit);

		    if (psock->state == NoCard) {
	    		FORTEZZAUNLOCK(psock->stateLock);
	    		return NULL;
		    }

		    
		    if (!LL_IS_ZERO(fortezzaGlobal.timeout)) {
			/* time out fortezza cards */
			int64 timeout;
			int64 now = PR_NowS();
			LL_ADD(timeout,psock->lasttime,fortezzaGlobal.timeout);
			if (LL_CMP(now, > , timeout)) {
			    psock->state = NeedPin;
			}
		    }

		    psock->lasttime = PR_NowS();

		    if (psock->state == Ready) {
			FORTEZZAUNLOCK(psock->stateLock);
			cx = FortezzaCreateKEAContext(psock);
			if (cx == NULL) return NULL;

			rv = FortezzaSelectCertificate(NULL, psock,
						       &cx->certificate);
			if (rv < 0) return NULL;
			return cx;
		    }

	    	    FORTEZZAUNLOCK(psock->stateLock); 
		    /* does *NOT* return SECWouldBlock! */
		    if (psock->state == NeedPin) {
			rv = SECMOZ_GetPin(wcx,psock);
			if (rv < 0) return NULL;
		    } else {
			rv = SECSuccess;
		    }
		    /* assumes assignment to int is atomic ! */
		    psock->state = NeedCert;
		    FORTEZZAWAKEUP(psock->stateLock);
		    rv = FortezzaCertInit(psock);
		    if (rv < 0) return NULL;

		    cx = FortezzaCreateKEAContext(psock);
		    if (cx == NULL) return NULL;

                    rv = FortezzaSelectCertificate(NULL,psock,
                                  &cx->certificate);
		    if (rv < 0) return NULL;
		    psock->state = Ready;

		    return cx;
		}
	    }
	}
	return NULL;
}
