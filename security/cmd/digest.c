#include "secutil.h"

#if defined(__sun) && !defined(SVR4)
extern int fread(char *, size_t, size_t, FILE*);
extern int fwrite(char *, size_t, size_t, FILE*);
extern int fprintf(FILE *, char *, ...);
extern int getopt(int, char**, char*);
extern char *optarg;
#endif

static SECOidData *
HashTypeToOID(HASH_HashType hashtype)
{
    SECOidTag hashtag;

    if (hashtype <= HASH_AlgNULL || hashtype >= HASH_AlgTOTAL)
	return NULL;

    switch (hashtype) {
      case HASH_AlgMD2:
	hashtag = SEC_OID_MD2;
	break;
      case HASH_AlgMD5:
	hashtag = SEC_OID_MD5;
	break;
      case HASH_AlgSHA1:
	hashtag = SEC_OID_SHA1;
	break;
      default:
	fprintf(stderr, "A new hash type has been added to HASH_HashType.\n");
	fprintf(stderr, "This program needs to be updated!\n");
	return NULL;
    }

    return SECOID_FindOIDByTag(hashtag);
}

static SECOidData *
HashNameToOID(const char *hashName)
{
    HASH_HashType htype;
    SECOidData *hashOID;

    for (htype = HASH_AlgNULL + 1; htype < HASH_AlgTOTAL; htype++) {
	hashOID = HashTypeToOID(htype);
	if (PORT_Strcasecmp(hashName, hashOID->desc) == 0)
	    break;
    }

    if (htype == HASH_AlgTOTAL)
	return NULL;

    return hashOID;
}

static HASH_HashType
OIDToHashType(SECOidData *hashOID)
{
    switch (hashOID->offset) {
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

static void
Usage(char *progName)
{
    HASH_HashType htype;

    fprintf(stderr,
	    "Usage:  %s -t type [-i input] [-o output]\n",
	    progName);
    fprintf(stderr, "%-20s Specify the digest method (must be one of\n",
	    "-t type");
    fprintf(stderr, "%-20s ", "");
    for (htype = HASH_AlgNULL + 1; htype < HASH_AlgTOTAL; htype++) {
	fprintf(stderr, HashTypeToOID(htype)->desc);
	if (htype == (HASH_AlgTOTAL - 2))
	    fprintf(stderr, " or ");
	else if (htype != (HASH_AlgTOTAL - 1))
	    fprintf(stderr, ", ");
    }
    fprintf(stderr, " (case ignored))\n");
    fprintf(stderr, "%-20s Define an input file to use (default is stdin)\n",
	    "-i input");
    fprintf(stderr, "%-20s Define an output file to use (default is stdout)\n",
	    "-o output");
    exit(-1);
}

static int
DigestFile(FILE *outFile, FILE *inFile, SECOidData *hashOID)
{
    int nb;
    char ibuf[4096], digest[32];
    HASH_HashType hashType;
    SECHashObject *hashObj;
    void *hashcx;
    unsigned int len;

    hashType = OIDToHashType(hashOID);
    if (hashType == HASH_AlgNULL)
	return -1;

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

    (* hashObj->end)(hashcx, digest, &len, 32);
    (* hashObj->destroy)(hashcx, PR_TRUE);

    nb = fwrite(digest, 1, len, outFile);
    if (nb != len) {
	PORT_SetError(SEC_ERROR_IO);
	return -1;
    }

    return 0;
}

int
main(int argc, char **argv)
{
    char *progName;
    int opt;
    FILE *inFile, *outFile;
    char *hashName;
    SECOidData *hashOID;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    inFile = NULL;
    outFile = NULL;
    hashName = NULL;

    /*
     * Parse command line arguments
     */
    while ((opt = getopt(argc, argv, "t:i:o:")) != -1) {
	switch (opt) {
	  case '?':
	    Usage(progName);
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

	  case 't':
	    hashName = strdup(optarg);
	    break;
	}
    }

    if (!hashName) Usage(progName);

    if (!inFile) inFile = stdin;
    if (!outFile) outFile = stdout;

    hashOID = HashNameToOID(hashName);
    if (hashOID == NULL) {
	fprintf(stderr, "%s: invalid digest type\n", progName);
	Usage(progName);
    }

    if (DigestFile(outFile, inFile, hashOID)) {
	fprintf(stderr, "%s: problem digesting data (%s)\n",
		progName, SECU_ErrorString(PORT_GetError()));
	return -1;
    }

    return 0;
}
