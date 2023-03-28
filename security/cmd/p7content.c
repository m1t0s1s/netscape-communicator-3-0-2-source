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
	    "Usage:  %s [-d dbdir] [-i input] [-o output]\n",
	    progName);
    fprintf(stderr,
	    "%-20s Key/Cert database directory (default is ~/.netscape)\n",
	    "-d dbdir");
    fprintf(stderr, "%-20s Define an input file to use (default is stdin)\n",
	    "-i input");
    fprintf(stderr, "%-20s Define an output file to use (default is stdout)\n",
	    "-o output");
    exit(-1);
}

static SECKEYKeyDBHandle *
OpenKeyDB(char *progName)
{
    SECKEYKeyDBHandle *keyHandle;

    keyHandle = SECU_OpenKeyDB();
    if (keyHandle == NULL) {
        SECU_PrintError(progName, "could not open key database");
	return NULL;
    }

    return(keyHandle);
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

static PRBool saw_content;

static void
PrintBytes(void *arg, const char *buf, unsigned long len)
{
    FILE *out;

    out = arg; 
    fwrite (buf, len, 1, out);

    saw_content = PR_TRUE;
}

int
DecodeAndPrintFile(FILE *out, FILE *in, char *progName)
{
    SECItem derdata;
    SEC_PKCS7ContentInfo *cinfo;
    SEC_PKCS7DecoderContext *dcx;

    if (SECU_DER_Read(&derdata, in)) {
	fprintf(stderr, "%s: error converting der (%s)\n", progName,
		SECU_ErrorString(PORT_GetError()));
	return -1;
    }

    fprintf(out,
	    "Content printed between bars (newline added before second bar):");
    fprintf(out, "\n---------------------------------------------\n");

    saw_content = PR_FALSE;
    dcx = SEC_PKCS7DecoderStart(PrintBytes, out, SECU_GetPassword, NULL);
    if (dcx != NULL) {
#if 0	/* Test that decoder works when data is really streaming in. */
	{
	    unsigned long i;
	    for (i = 0; i < derdata.len; i++)
		SEC_PKCS7DecoderUpdate(dcx, derdata.data + i, 1);
	}
#else
	SEC_PKCS7DecoderUpdate(dcx, derdata.data, derdata.len);
#endif
	cinfo = SEC_PKCS7DecoderFinish(dcx);
    }

    fprintf(out, "\n---------------------------------------------\n");

    if (cinfo == NULL)
	return -1;

    fprintf(out, "Content was%s encrypted.\n",
	    SEC_PKCS7ContentIsEncrypted(cinfo) ? "" : " not");

    if (SEC_PKCS7ContentIsSigned(cinfo)) {
	char *signer_cname, *signer_ename;
	SECItem *signing_time;

	if (saw_content) {
	    fprintf(out, "Signature is ");
	    PORT_SetError(0);
	    if (SEC_PKCS7VerifySignature(cinfo, certUsageEmailSigner, PR_FALSE))
		fprintf(out, "valid.\n");
	    else
		fprintf(out, "invalid (Reason: %s).\n",
			SECU_ErrorString(PORT_GetError()));
	} else {
	    fprintf(out,
		    "Content is detached; signature cannot be verified.\n");
	}

	signer_cname = SEC_PKCS7GetSignerCommonName(cinfo);
	if (signer_cname != NULL) {
	    fprintf(out, "The signer's common name is %s\n", signer_cname);
	    PORT_Free(signer_cname);
	} else {
	    fprintf(out, "No signer common name.\n");
	}

	signer_ename = SEC_PKCS7GetSignerEmailAddress(cinfo);
	if (signer_ename != NULL) {
	    fprintf(out, "The signer's email address is %s\n", signer_ename);
	    PORT_Free(signer_ename);
	} else {
	    fprintf(out, "No signer email address.\n");
	}

	signing_time = SEC_PKCS7GetSigningTime(cinfo);
	if (signing_time != NULL) {
	    SECU_PrintUTCTime(out, signing_time, "Signing time", 0);
	} else {
	    fprintf(out, "No signing time included.\n");
	}
    } else {
	fprintf(out, "Content was not signed.\n");
    }

    fprintf(out, "There were%s certs or crls included.\n",
	    SEC_PKCS7ContainsCertsOrCrls(cinfo) ? "" : " no");

    SEC_PKCS7DestroyContentInfo(cinfo);
    return 0;
}


/*
 * Print the contents of a PKCS7 message, indicating signatures, etc.
 */

int
main(int argc, char **argv)
{
    char *progName;
    int opt;
    FILE *inFile, *outFile;
    SECKEYKeyDBHandle *keyHandle;
    CERTCertDBHandle *certHandle;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    inFile = NULL;
    outFile = NULL;

    /*
     * Parse command line arguments
     */
    while ((opt = getopt(argc, argv, "d:i:o:")) != -1) {
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
	}
    }

    if (!inFile) inFile = stdin;
    if (!outFile) outFile = stdout;

    /* Call the initialization routines */
    PR_Init("p7content", 1, 1, 0);
    SECU_PKCS11Init();
    SEC_Init();

    /* open key database */
    keyHandle = OpenKeyDB(progName);
    if (keyHandle == NULL) {
	return -1;
    }
    SECKEY_SetDefaultKeyDB(keyHandle);

    /* open cert database */
    certHandle = OpenCertDB(progName);
    if (certHandle == NULL) {
	return -1;
    }
    CERT_SetDefaultCertDB(certHandle);

    if (DecodeAndPrintFile(outFile, inFile, progName)) {
	fprintf(stderr, "%s: problem decoding data (%s)\n",
		progName, SECU_ErrorString(PORT_GetError()));
	return -1;
    }

    return 0;
}
