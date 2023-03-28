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
	    "Usage:  %s -k keyname [-d keydir] [-i input] [-o output]\n",
	    progName);
    fprintf(stderr, "%-20s Nickname of key to use for signature\n",
	    "-k keyname");
    fprintf(stderr, "%-20s Key database directory (default is ~/.netscape)\n",
	    "-d keydir");
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

static void
SignOut(void *arg, const char *buf, unsigned long len)
{
   FILE *out;

   out = arg; 
   fwrite (buf, len, 1, out);
}

static int
SignFile(FILE *outFile, FILE *inFile, CERTCertificate *cert)
{
    int nb;
    char ibuf[4096], digestdata[32];
    SECHashObject *hashObj;
    void *hashcx;
    unsigned int len;
    SECItem digest;
    SEC_PKCS7ContentInfo *cinfo;
    SECStatus rv;

    if (outFile == NULL || inFile == NULL || cert == NULL)
	return -1;

    /* XXX probably want to extend interface to allow other hash algorithms */
    hashObj = &SECHashObjects[HASH_AlgSHA1];

    hashcx = (* hashObj->create)();
    if (hashcx == NULL)
	return -1;

    (* hashObj->begin)(hashcx);

    for (;;) {
	if (feof(inFile)) break;
	nb = fread(ibuf, 1, sizeof(ibuf), inFile);
	if (nb == 0) {
	    if (ferror(inFile)) {
		PORT_SetError(SEC_ERROR_IO);
		(* hashObj->destroy)(hashcx, PR_TRUE);
		return -1;
	    }
	    /* eof */
	    break;
	}
	(* hashObj->update)(hashcx, ibuf, nb);
    }

    (* hashObj->end)(hashcx, digestdata, &len, 32);
    (* hashObj->destroy)(hashcx, PR_TRUE);

    digest.data = digestdata;
    digest.len = len;

    /* XXX Need a better way to handle that usage stuff! */
    cinfo = SEC_PKCS7CreateSignedData (cert, certUsageEmailSigner,
		 NULL, SEC_OID_SHA1, &digest, NULL, NULL);
    if (cinfo == NULL)
	return -1;

    rv = SEC_PKCS7IncludeCertChain (cinfo, NULL);
    if (rv != SECSuccess) {
	SEC_PKCS7DestroyContentInfo (cinfo);
	return -1;
    }

    rv = SEC_PKCS7Encode (cinfo, SignOut, outFile, NULL,
			  SECU_GetPassword, NULL);

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
    char *keyName;
    SECKEYKeyDBHandle *keyHandle;
    CERTCertDBHandle *certHandle;
    CERTCertificate *cert;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    inFile = NULL;
    outFile = NULL;
    keyName = NULL;

    /*
     * Parse command line arguments
     */
    while ((opt = getopt(argc, argv, "d:k:i:o:")) != -1) {
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

	  case 'k':
	    keyName = strdup(optarg);
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

    if (!keyName) Usage(progName);

    if (!inFile) inFile = stdin;
    if (!outFile) outFile = stdout;

    /* Call the libsec initialization routines */
    PR_Init("sign", 1, 1, 0);
    SECU_PKCS11Init();
    SEC_Init();

    /* open key database */
    keyHandle = OpenKeyDB(progName);
    if (keyHandle == NULL) {
	return -1;
    }

#if 0
    /* check if key actually exists */
    if (! SECU_CheckKeyNameExists(keyHandle, keyName)) {
	SECU_PrintError(progName, "the key \"%s\" does not exist", keyName);
	return -1;
    }
#endif

    SECKEY_SetDefaultKeyDB(keyHandle);

    /* open cert database */
    certHandle = OpenCertDB(progName);
    if (certHandle == NULL) {
	return -1;
    }

    /* find cert */
    cert = CERT_FindCertByNickname(certHandle, keyName);
    if (cert == NULL) {
	SECU_PrintError(progName,
		        "the corresponding cert for key \"%s\" does not exist",
			keyName);
	return -1;
    }

    CERT_SetDefaultCertDB(certHandle);

    if (SignFile(outFile, inFile, cert)) {
	fprintf(stderr, "%s: problem signing data (%s)\n",
		progName, SECU_ErrorString(PORT_GetError()));
	return -1;
    }

    return 0;
}
