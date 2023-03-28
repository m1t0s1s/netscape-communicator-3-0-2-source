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

static HASH_HashType
AlgorithmToHashType(SECAlgorithmID *digestAlgorithms)
{

    SECOidTag tag;

    tag = SECOID_GetAlgorithmTag(digestAlgorithms);
    
    switch (tag) {
      case SEC_OID_MD2:
	return HASH_AlgMD2;
      case SEC_OID_MD5:
	return HASH_AlgMD5;
      case SEC_OID_SHA1:
	return HASH_AlgSHA1;
      default:
	fprintf(stderr, "should never get here\n");
	return HASH_AlgNULL;
    }
}



static int DigestFile
   (unsigned char *digest, unsigned int *len, unsigned int maxLen,
    FILE *inFile, HASH_HashType hashType)
{
    int nb;
    char ibuf[4096];
    SECHashObject *hashObj;
    void *hashcx;

    hashObj = &SECHashObjects[hashType];

    hashcx = (* hashObj->create)();
    if (hashcx == NULL)
	return -1;

    (* hashObj->begin)(hashcx);

    for (;;) {
	if (feof(inFile)) break;
	nb = fread(ibuf, 1, sizeof(ibuf), inFile);
	if (nb != sizeof(ibuf)) {
	    if (nb == 0) {
		if (ferror(inFile)) {
		    PORT_SetError(SEC_ERROR_IO);
		    (* hashObj->destroy)(hashcx, PR_TRUE);
		    return -1;
		}
		/* eof */
		break;
	    }
	}
	(* hashObj->update)(hashcx, ibuf, nb);
    }

    (* hashObj->end)(hashcx, digest, len, maxLen);
    (* hashObj->destroy)(hashcx, PR_TRUE);

    return 0;
}


static void
Usage(char *progName)
{
    fprintf(stderr,
	    "Usage:  %s -i input -c content [-d dbdir] [-o output] [-u certusage]\n",
	    progName);
    fprintf(stderr, "%-20s Define an input file to use\n",
	    "-i input");
    fprintf(stderr, "%-20s content file that was signed\n",
	    "-c content");
    fprintf(stderr,
	    "%-20s Key/Cert database directory (default is ~/.netscape)\n",
	    "-d dbdir");
    fprintf(stderr, "%-20s Define an output file to use (default is stdout)\n",
	    "-o output");
    fprintf(stderr, "%-20s Define the type of certificate usage (default is certUsageEmailSigner)\n",
	    "-u certusage");
    fprintf(stderr, "%-25s 0 - certUsageSSLClient\n", " ");
    fprintf(stderr, "%-25s 1 - certUsageSSLServer\n", " ");
    fprintf(stderr, "%-25s 2 - certUsageSSLServerWithStepdown\n", " ");
    fprintf(stderr, "%-25s 3 - certUsageSSLCA\n", " ");
    fprintf(stderr, "%-25s 4 - certUsageEmailSigner\n", " ");
    fprintf(stderr, "%-25s 5 - certUsageEmailRecipient\n", " ");
    fprintf(stderr, "%-25s 6 - certUsageObjectSigner\n", " ");

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
PrintBytes(void *arg, const char *buf, unsigned long len)
{
   FILE *out;

   out = arg; 
   fwrite (buf, len, 1, out);
}

int
DecodeAndPrintFile (FILE *out, FILE *in, FILE *content,
		    char *progName, SECCertUsage usage)
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

    fprintf(out, "Content %s encrypted.\n",
	    SEC_PKCS7ContentIsEncrypted(cinfo) ? "was" : "was not");

    if (SEC_PKCS7ContentIsSigned(cinfo)) {
	SEC_PKCS7SignedData *signedData;
	HASH_HashType digestType;
	SECItem digest;
	unsigned char buffer[32];

	signedData = cinfo->content.signedData;
	
	/* assume that there is only one digest algorithm for now */
	digestType = AlgorithmToHashType(signedData->digestAlgorithms[0]);
	if (digestType == HASH_AlgNULL) {
	    fprintf (out, "Invalid hash algorithmID\n");
	    return (-1);
	}
	digest.data = buffer;
	if (DigestFile (digest.data, &digest.len, 32, content, digestType)) {
	    fprintf (out, "Fail to compute message digest for verification. Reason: %s\n",
		    SECU_ErrorString(PORT_GetError()));
	    return (-1);
	}
	fprintf(out, "Signature is ");
	PORT_SetError(0);
	if (SEC_PKCS7VerifyDetachedSignature
	    (cinfo, usage, &digest, digestType, PR_FALSE))
	    fprintf(out, "valid.\n");
	else
	    fprintf(out, "invalid (Reason: %s).\n",
		    SECU_ErrorString(PORT_GetError()));
    } else {
	fprintf(out, "Content was not signed.\n");
    }

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
    FILE *contentFile, *inFile, *outFile;
    SECKEYKeyDBHandle *keyHandle;
    SECCertUsage usage = certUsageEmailSigner;
    CERTCertDBHandle *certHandle;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    contentFile = NULL;
    inFile = NULL;
    outFile = NULL;

    /*
     * Parse command line arguments
     */
    while ((opt = getopt(argc, argv, "c:d:i:o:u:")) != -1) {
	switch (opt) {
	  case '?':
	    Usage(progName);
	    break;

	  case 'c':
	    contentFile = fopen(optarg, "r");
	    if (!contentFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for reading\n",
			progName, optarg);
		return -1;
	    }
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

	  case 'u': {
	    int usageType;

	    usageType = atoi (strdup(optarg));
	    if (usageType < certUsageSSLClient || usageType > certUsageObjectSigner)
		return -1;
	    usage = (SECCertUsage)usageType;
	    break;
	  }
	      
	}
    }

    if (!contentFile) Usage (progName);
    if (!inFile) inFile = stdin;
    if (!outFile) outFile = stdout;

    /* Call the libsec initialization routines */
    PR_Init("p7verify", 1, 1, 0);
    SECU_PKCS11Init();
    SEC_Init();

    /* open key database */
    keyHandle = OpenKeyDB(progName);
    if (keyHandle == NULL) {
        fprintf(stderr, "%s Problem open the key dbase\n",
		    progName);
	return -1;
    }
    SECKEY_SetDefaultKeyDB(keyHandle);
    /* open cert database */
    certHandle = OpenCertDB(progName);
    if (certHandle == NULL) {
        fprintf(stderr, "%s Problem open the cert dbase\n",
		    progName);
	return -1;
    }
    CERT_SetDefaultCertDB(certHandle);

    if (DecodeAndPrintFile(outFile, inFile, contentFile, progName, usage)) {
	fprintf(stderr, "%s: problem decoding data (%s)\n",
		progName, SECU_ErrorString(PORT_GetError()));
	return -1;
    }

    return 0;
}
