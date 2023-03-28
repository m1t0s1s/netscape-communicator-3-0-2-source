#include "secutil.h"

/* SVR4 is to prevent Solaris from blowing up --Rob */
#if defined(__sun) && !defined(SVR4)
extern int fread(char *, size_t, size_t, FILE*);
extern int fwrite(char *, size_t, size_t, FILE*);
extern int getopt(int, char**, char*);
extern int fprintf(FILE *, char *, ...);
extern char *optarg;
#endif

static int ATOBFile(FILE *outFile, FILE *inFile)
{
    ATOBContext *cx;
    unsigned part;
    int nb;
    SECStatus rv;
    char ibuf[4096], obuf[4096];

    cx = ATOB_NewContext();
    if (!cx) {
	return PORT_GetError();
    }

    rv = ATOB_Begin(cx);
    if (rv) goto loser;
    for (;;) {
	if (feof(inFile)) break;
	nb = fread(ibuf, 1, sizeof(ibuf), inFile);
	if (nb != sizeof(ibuf)) {
	    if (nb == 0) {
		if (ferror(inFile)) {
		    PORT_SetError(SEC_ERROR_IO);
		    rv = SECFailure;
		    goto loser;
		}
		/* eof */
		break;
	    }
	}

	/* Over grow buffer to accomodate new data */
	rv = ATOB_Update(cx, obuf, &part, sizeof(obuf), ibuf, nb);
	if (rv) goto loser;
	nb = fwrite(obuf, 1, part, outFile);
	if (nb != part) {
	    PORT_SetError(SEC_ERROR_IO);
	    rv = SECFailure;
	    goto loser;
	}
    }
    rv = ATOB_End(cx, obuf, &part, sizeof(obuf));
    if (rv) goto loser;
    if (part) {
	nb = fwrite(obuf, 1, part, outFile);
	if (nb != part) {
	    PORT_SetError(SEC_ERROR_IO);
	    rv = SECFailure;
	    goto loser;
	}
    }
    /* FALL THROUGH */

  loser:
    ATOB_DestroyContext(cx);
    return rv;
}

static void Usage(char *progName)
{
    fprintf(stderr,
	    "Usage: %s [-i input] [-o output]\n",
	    progName);
    fprintf(stderr, "%-20s Define an input file to use (default is stdin)\n",
	    "-i input");
    fprintf(stderr, "%-20s Define an output file to use (default is stdout)\n",
	    "-o output");
    exit(-1);
}

int main(int argc, char **argv)
{
    char *progName;
    int o, rv;
    FILE *inFile, *outFile;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    /* Parse command line arguments */
    inFile = 0;
    outFile = 0;
    while ((o = getopt(argc, argv, "i:o:")) != -1) {
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
	}
    }
    if (!inFile) inFile = stdin;
    if (!outFile) outFile = stdout;
    rv = ATOBFile(outFile, inFile);
    if (rv) {
	fprintf(stderr, "%s: lossage: rv=%d error=%d errno=%d\n",
		progName, rv, PORT_GetError(), errno);
	return -1;
    }
    return 0;
}
