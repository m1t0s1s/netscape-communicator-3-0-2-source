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

int
main(int argc, char **argv)
{
    SECStatus rv;
    int ret;
    CERTCertificate *cert;
    
    progname = argv[0];

    if ( argc != 2 ) {
	fprintf(stderr, "Usage: %s certname\n", progname);
	exit(-1);
    }
    
    rv = CERT_OpenCertDBFilename(&dbhandle, "cert6.db", TRUE);
    if ( rv ) {
	quit("Error opening certificate database");
    }

    cert = CERT_FindCertByNickname(&dbhandle, argv[1]);
    if ( !cert ) {
	quit("Could not find certificate named %s", argv[1]);
    }

    ret = fwrite( cert->derCert.data, cert->derCert.len, 1, stdout );
    if ( ret != 1 ) {
	quit("Error writing certificate", argv[1]);
    }

    CERT_ClosePermCertDB(&dbhandle);
    
    return(0);
}
