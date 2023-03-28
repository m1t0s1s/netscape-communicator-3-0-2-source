#include "secutil.h"


static char *progname;

CERTCertDBHandle dbhandle;

static void
quit (char *msg, ...)
{
    va_list args;

    va_start (args, msg);
    fprintf (stderr, "%s: ", progname);
    vfprintf (stderr, msg, args);
    fprintf (stderr, "\n");
    va_end (args);

    exit (1);
}

static SECStatus
printnickname(CERTCertificate *cert, SECItem *k, void *data)
{
    char *keydata;
    int len;
    char *strbuf;
    
    keydata = (char *)k->data;
    
    if ( (*keydata) == DER_PRINTABLE_STRING ) {

	/* XXX - need real der len code */
	len = keydata[1];

	strbuf = ( char *)PORT_Alloc(len + 1);
	if ( !strbuf ) {
	    return(SECFailure);
	}
	
	PORT_Memcpy(strbuf, &keydata[2], len);

	strbuf[len] = 0;
	
	printf("%s\n", strbuf);
	
	PORT_Free(strbuf);
	
    }

    return(SECSuccess);
}


int
main(int argc, char **argv)
{
    SECStatus rv;
    
    progname = argv[0];

    if ( argc != 1 ) {
	fprintf(stderr, "Usage: %s\n", progname);
	exit(-1);
    }
    
    rv = CERT_OpenCertDBFilename(&dbhandle, SECU_DatabaseFileName(xpCertDB), TRUE);
    if ( rv ) {
	quit("Error opening certificate database");
    }

    rv = SEC_TraversePermCerts(&dbhandle, printnickname, 0);


    /* This is the insertion point for the fix
    **rv = SEC_TraverseNames(&dbhandle, printnickname, 0);
    */

    if ( rv ) {
	quit("Error traversing certificate database");
    }

    CERT_ClosePermCertDB(&dbhandle);
    
    return(0);
}
