#include "secutil.h"
#include "secpkcs7.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#if defined(LINUX)
#include <getopt.h>
#endif

#if defined(__sun) && !defined(SVR4)
extern int fread(char *, size_t, size_t, FILE*);
extern int fwrite(char *, size_t, size_t, FILE*);
extern int fprintf(FILE *, char *, ...);
extern int getopt(int, char**, char*);
extern char *optarg;
extern char *sys_errlist[];
#define strerror(errno) sys_errlist[errno]
#endif

extern void SEC_Init(void);		/* XXX */


static void
Usage(char *progName)
{
    fprintf(stderr,
	    "Usage:  %s -r recipient [-d dbdir] [-i input] [-o output]\n",
	    progName);
    fprintf(stderr, "%-20s Nickname of cert to use for encryption\n",
	    "-r recipient");
    fprintf(stderr, "%-20s Cert database directory (default is ~/.netscape)\n",
	    "-d dbdir");
    fprintf(stderr, "%-20s Define an input file to use (default is stdin)\n",
	    "-i input");
    fprintf(stderr, "%-20s Define an output file to use (default is stdout)\n",
	    "-o output");
    exit(-1);
}

static CERTCertDBHandle certHandleStatic;	/* avoid having to allocate */

static CERTCertDBHandle *
OpenCertDB(char *progName)
{
    CERTCertDBHandle *certHandle;
    struct stat stat_buf;
    char *fName;
    SECStatus rv;
    int ret;

    fName = SECU_DatabaseFileName(xpCertDB);

    ret = stat(fName, &stat_buf);
    if (ret < 0) {
	if (errno == ENOENT) {
	    /* no cert5.db */
	    SECU_PrintError(progName, "unable to locate cert database");
	    return NULL;
	} else {
	    /* stat error */
	    SECU_PrintError(progName, "stat: ", strerror(errno));
	    return NULL;
	}
    }

    certHandle = &certHandleStatic;
    rv = CERT_OpenCertDBFilename(certHandle, fName, FALSE);
    if (rv != SECSuccess) {
        SECU_PrintError(progName, "could not open cert database");
	return NULL;
    }

    return certHandle;
}

struct recipient {
    struct recipient *next;
    char *nickname;
    CERTCertificate *cert;
};

static void
EncryptOut(void *arg, const char *buf, unsigned long len)
{
   FILE *out;

   out = arg; 
   fwrite (buf, len, 1, out);
}

static int
EncryptFile(FILE *outFile, FILE *inFile, struct recipient *recipients)
{
    SEC_PKCS7ContentInfo *cinfo;
    SEC_PKCS7EncoderContext *ecx;
    struct recipient *rcpt;
    SECStatus rv;

    if (outFile == NULL || inFile == NULL || recipients == NULL)
	return -1;

    /* XXX Need a better way to handle that certUsage stuff! */
    /* XXX keysize? */
    cinfo = SEC_PKCS7CreateEnvelopedData (recipients->cert,
					  certUsageEmailRecipient,
					  NULL, SEC_OID_DES_EDE3_CBC, 0, 
					  NULL, NULL);
    if (cinfo == NULL)
	return -1;

    for (rcpt = recipients->next; rcpt != NULL; rcpt = rcpt->next) {
	rv = SEC_PKCS7AddRecipient (cinfo, rcpt->cert, certUsageEmailRecipient,
				    NULL);
	if (rv != SECSuccess) {
	    /* XXX should print progName in error message */
	    fprintf(stderr, "error adding recipient \"%s\" (%s)\n",
		    rcpt->nickname, SECU_ErrorString(PORT_GetError()));
	    return -1;
	}
    }

    ecx = SEC_PKCS7EncoderStart (cinfo, EncryptOut, outFile, NULL);
    if (ecx == NULL)
	return -1;

    for (;;) {
	char ibuf[1024];
	int nb;
 
	if (feof(inFile))
	    break;
	nb = fread(ibuf, 1, sizeof(ibuf), inFile);
	if (nb == 0) {
	    if (ferror(inFile)) {
		PORT_SetError(SEC_ERROR_IO);
		rv = SECFailure;
	    }
	    break;
	}
	rv = SEC_PKCS7EncoderUpdate(ecx, ibuf, nb);
	if (rv != SECSuccess)
	    break;
    }

    if (SEC_PKCS7EncoderFinish(ecx, NULL, NULL) != SECSuccess)
	rv = SECFailure;

    SEC_PKCS7DestroyContentInfo (cinfo);

    if (rv != SECSuccess)
	return -1;

    return 0;
}

int
main(int argc, char **argv)
{
    char *progName;
    int opt;
    FILE *inFile, *outFile;
    char *certName;
    CERTCertDBHandle *certHandle;
    CERTCertificate *cert;
    struct recipient *recipients, *rcpt;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    inFile = NULL;
    outFile = NULL;
    certName = NULL;
    recipients = NULL;
    rcpt = NULL;

    /*
     * Parse command line arguments
     * XXX This needs to be enhanced to allow selection of algorithms
     * and key sizes (or to look up algorithms and key sizes for each
     * recipient in the magic database).
     */
    while ((opt = getopt(argc, argv, "d:i:o:r:")) != -1) {
	switch (opt) {
	  case '?':
	    Usage(progName);
	    break;

	  case 'd':
	    SECU_ConfigDirectory(optarg);
	    break;

	  case 'i':
	    inFile = fopen(optarg, "r");
	    if (!inFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for reading\n",
			progName, optarg);
		return -1;
	    }
	    break;

	  case 'o':
	    outFile = fopen(optarg, "w");
	    if (!outFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for writing\n",
			progName, optarg);
		return -1;
	    }
	    break;

	  case 'r':
	    if (rcpt == NULL) {
		recipients = rcpt = PORT_Alloc (sizeof(struct recipient));
	    } else {
		rcpt->next = PORT_Alloc (sizeof(struct recipient));
		rcpt = rcpt->next;
	    }
	    if (rcpt == NULL) {
		fprintf(stderr, "%s: unable to allocate recipient struct\n",
			progName);
		return -1;
	    }
	    rcpt->nickname = strdup(optarg);
	    rcpt->cert = NULL;
	    rcpt->next = NULL;
	    break;
	}
    }

    if (!recipients) Usage(progName);

    if (!inFile) inFile = stdin;
    if (!outFile) outFile = stdout;

    /* Call the libsec initialization routines */
    PR_Init("sign", 1, 1, 0);
    SECU_PKCS11Init();
    SEC_Init();

    /* open cert database */
    certHandle = OpenCertDB(progName);
    if (certHandle == NULL) {
	return -1;
    }

    /* find certs */
    for (rcpt = recipients; rcpt != NULL; rcpt = rcpt->next) {
	rcpt->cert = CERT_FindCertByNickname(certHandle, rcpt->nickname);
	if (rcpt->cert == NULL) {
	    SECU_PrintError(progName,
			    "the cert for name \"%s\" not found in database",
			    rcpt->nickname);
	    return -1;
	}
    }

    CERT_SetDefaultCertDB(certHandle);

    if (EncryptFile(outFile, inFile, recipients)) {
	fprintf(stderr, "%s: problem encrypting data (%s)\n",
		progName, SECU_ErrorString(PORT_GetError()));
	return -1;
    }

    return 0;
}
