#include "secutil.h"

#if defined(__sun) && !defined(SVR4)
extern int fwrite(char *, size_t, size_t, FILE*);
extern int fprintf(FILE *, char *, ...);
extern int fclose(FILE*);
extern int getopt(int, char**, char*);
extern char *optarg;
#endif

static int DerFile(FILE *inFile, FILE *outFile, SECKEYPrivateKey *k, SECItem *res)
{
    SECItem file;
    SECStatus rv;
    PRArenaPool *arena;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( !arena ) {
	return(0);
    }

    /* Read der item from file and then sign it */
    rv = DER_Read(arena, &file, inFile);
    if (rv) return rv;
    rv = SEC_DerSignData(arena, res, file.data, file.len, k);
    PORT_Free(file.data);
    return rv;
}

static void Usage(char *progName)
{
    fprintf(stderr,
	    "Usage: %s -k keyfile [-d] [-e] [-o output] [-i input]\n",
	    progName);
    fprintf(stderr, "%-20s Specify the file that contains the key pair\n",
	    "-k keyfile");
    fprintf(stderr, "%-20s Sign a file, writing the signature to sigfile\n",
	    "-o output");
    fprintf(stderr, "%-20s Define an input file to use (default is stdin)\n",
	    "-i input");
    fprintf(stderr, "%-20s Expect DER input and write DER output\n",
	    "-d");
    fprintf(stderr, "%-20s Ascii encode signature output using RFC1113\n",
	    "-a");
    exit(-1);
}

int main(int argc, char **argv)
{
    char *progName;
    int o, rv, der, ascii;
    SECItem result;
    SECKEYPrivateKey *k;
    FILE *inFile, *outFile;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    /* Parse command line arguments */
    inFile = 0;
    outFile = 0;
    k = 0;
    der = 0;
    ascii = 0;
    while ((o = getopt(argc, argv, "k:i:o:da")) != -1) {
	switch (o) {
	  case '?':
	    Usage(progName);
	    break;

	  case 'k':
	    k = SECU_GetPrivateKey(optarg);
	    if (!k) {
		fprintf(stderr, "%s: bad key file\n", progName);
		return -1;
	    }
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

	  case 'd':
	    der = 1;
	    break;

	  case 'a':
	    ascii = 1;
	    break;
	}
    }
    if (!k) {
	Usage(progName);
    }
    if (!inFile) inFile = stdin;
    if (!outFile) outFile = stdout;

    if (der) {
	rv = DerFile(inFile, outFile, k, &result);
    } else {
	/* Construct signature of input file */
	rv = SEC_SignFile(&result, inFile, k);
    }
    if (rv) {
	/* Lossage */
	fprintf(stderr, "%s: lossage: rv=%d error=%d\n",
		progName, rv, PORT_GetError());
	return -1;
    }

    /* Ascii encode result if asked to */
    if (ascii) {
	unsigned char *enc;
	unsigned enclen, first, rem;
	BTOAContext *cx;

	enclen = BTOA_GetLength(result.len);
	enc = (unsigned char*) PORT_Alloc(enclen);
	cx = BTOA_NewContext();
	if (!enc || !cx) {
	    fprintf(stderr, "%s: lossage: not enough memory\n", progName);
	    return -1;
	}
	do {
	    rv = BTOA_Begin(cx);
	    if (rv) break;
	    rv = BTOA_Update(cx, enc, &first, enclen, result.data, result.len);
	    if (rv) break;
	    rv = BTOA_End(cx, enc+first, &rem, enclen-first);
	} while (0);
	if (rv) {
	    fprintf(stderr, "%s: ascii encoding lossage: rv=%d error=%d\n",
		    progName, rv, PORT_GetError());
	    return -1;
	}
	result.data = enc;
	result.len = first + rem;
    }

    /* Write out signature */
    rv = fwrite(result.data, 1, result.len, outFile);
    if (rv != result.len) {
	fprintf(stderr, "%s: write output lossage: rv=%d errno=%d\n",
		progName, rv, errno);
	return -1;
    }
    fclose(outFile);

    return 0;
}
