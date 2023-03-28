#include "sec.h"

/*
** Convert a file into a single item object
*/
static DSStatus FileToItem(SECItem *result, FILE *in)
{
    char buf[4096];
    int nb;
    unsigned len;

    len = 0;
    result->data = 0;
    result->len = 0;
    for (;;) {
	if (feof(in)) break;
	nb = fread(buf, 1, sizeof(buf), in);
	if (nb < 0) {
	    XP_SetError(errno);
	    return DSFailure;
	}
	if (result->data) {
	    result->data = (unsigned char*) DS_Realloc(result->data, len + nb);
	    if (!result->data) return DSFailure;
	    XP_MEMCPY(result->data + len, buf, nb);
	    len += nb;
	} else {
	    result->data = (unsigned char*) DS_Alloc(nb);
	    if (!result->data) return DSFailure;
	    XP_MEMCPY(result->data, buf, nb);
	    len = nb;
	}
    }
    result->len = len;
    return DSSuccess;
}

static void Usage(char *progName)
{
    fprintf(stderr,
	    "Usage: %s [-i input] [-o output] [-s | -v]\n",
	    progName);
    exit(-1);
}

int main(int argc, char **argv)
{
    int o, sign, verify;
    FILE *inFile, *outFile;
    SECItem data;
    SECSignedData sd;
    SECCertificate *c;
    char *progName;
    DSStatus rv;
    SECPublicKey *key;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    sign = verify = 0;
    while ((o = getopt(argc, argv, "i:o:sv")) != -1) {
	switch (o) {
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

	  case 'v':
	    verify = 1;
	    break;
	}
    }

    if (!inFile) inFile = stdin;
    if (!outFile) outFile = stdout;

    if (verify) {
	/* Read input into a buffer */
	rv = FileToItem(&data, inFile);
	if (rv) {
	    fprintf(stderr, "%s: oops, read error: %d\n",
		    progName, XP_GetError());
	    return -1;
	}

	/* Look at the input and see if it's DER */
	XP_MEMSET(&sd, 0, sizeof(sd));
	rv = DER_Decode(&sd, SECSignedDataTemplate, &data);
	if (rv) {
	    /* OK, it's not good der. What is it? */
	    fprintf(stderr,
		    "%s: don't know how to process that kind of input (%d)\n",
		    progName, XP_GetError());
	    return -1;
	}

	/* Cool; it's signed data. Verify the signature */
	c = SEC_NewCertificate();
	rv = DER_Decode(c, SECCertificateTemplate, &sd.data);
	if (rv) {
	    fprintf(stderr,
		    "%s: the der object is not a certificate (%d)\n",
		    progName, XP_GetError());
	    return -1;
	}

	/* Extract public key from certificate */
	key = SEC_ExtractPublicKey(c);
	if (!key) {
	    fprintf(stderr, "%s: can't extract public key (%d)\n",
		    progName, XP_GetError());
	    return -1;
	}

	/* Check signature */
	DER_ConvertBitString(&sd.signature);
	rv = SEC_VerifyData(sd.data.data, sd.data.len, key, &sd.signature);
	if (rv) {
	    fprintf(stderr, "%s: signature check failed: %d\n",
		    progName, XP_GetError());
	    return -1;
	}
	fprintf(stderr, "%s: signature check ok\n", progName);
    }

    return 0;
}
