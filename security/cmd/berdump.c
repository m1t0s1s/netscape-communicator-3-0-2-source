#include "secnew.h"

#if defined(__sun) && !defined(SVR4)
extern int fprintf(FILE *, char *, ...);
extern int getopt(int, char**, char*);
extern char *optarg;
#endif

static void Usage(char *progName)
{
    fprintf(stderr,
	    "Usage: %s [-e encoded-copy] [-i input] [-o output] [-t]\n",
	    progName);
    fprintf(stderr, "%-20s Write the re-encoded output to a file.)\n",
	    "-e encoded-copy");
    fprintf(stderr, "%-20s Define an input file to use (default is stdin)\n",
	    "-i input");
    fprintf(stderr, "%-20s Keep long leaves\n",
	    "-k");
    fprintf(stderr, "%-20s Define an output file to use (default is stdout)\n",
	    "-o output");
    fprintf(stderr, "%-20s Write trace of nodes visited (default is false)\n",
	    "-t");
    exit(-1);
}

static SECStatus WriteEncoded(void *file, unsigned char *data, int length,
			 SECArb *arb)
{
    FILE *f = file;
    int rv;
    rv = fwrite(data, 1, length, f);
    if (rv != length) return SECFailure;
    return SECSuccess;
}

static void TraceProc(void *file, SECArb *arb, int depth, PRBool before)
{
    int i;
    FILE *f = file;
    for (i = 0; i < depth; i++)
	fprintf(f, "    ");
    fprintf(f, "tag = %02x (%d)\n", arb->tag, arb->length);
}

static void NoBigLeaves(void *h, SECArb *arb, int depth, PRBool before)
{
    BER_SetLeafStorage(h, arb->length < 200);
}

int main(int argc, char **argv)
{
    char *progName;
    int o;
    FILE *inFile, *outFile, *encodedFile, *tFile;
    unsigned char buf[1000];
    SECStatus rv;
    PRArenaPool *arena;
    BERParse *h;
    SECArb *a;
    PRBool trace = PR_FALSE;
    PRBool keepLong = PR_FALSE;
    
    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    /* Parse command line arguments */
    inFile = NULL;
    outFile = NULL;
    encodedFile = NULL;
    while ((o = getopt(argc, argv, "e:i:ko:t")) != -1) {
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

	case 'k':
	    keepLong = PR_TRUE;
	    break;

	case 'o':
	case 'e':
	    tFile = fopen(optarg, "w");
	    if (!tFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for writing\n",
			progName, optarg);
		return -1;
	    }
	    if (o == 'e')
		encodedFile = tFile;
	    else
		outFile = tFile;
	    break;


	case 't':
	    trace = PR_TRUE;
	    break;
	}
    }

    if (!inFile) inFile = stdin;
    if (!outFile) outFile = stdout;

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE); /* for the Arbs */
    if ( !arena ) {
	goto loser;
    }

    h = BER_ParseInit(arena, PR_FALSE);
    if (h == NULL) goto loser;
    if (trace)
	BER_SetNotifyProc(h, TraceProc, outFile, PR_FALSE);
    if (!keepLong)
	BER_SetNotifyProc(h, NoBigLeaves, h, PR_TRUE);
    for (;;) {
      rv = fread(buf, 1, sizeof(buf), inFile);
      if (rv < 0) goto loser;
      if (rv == 0) break;
      rv = BER_ParseSome(h, buf, rv);
      if (rv) goto loser;
    }
    a = BER_ParseFini(h);
    if (a == NULL) goto loser;
    rv = BER_PrettyPrintArb(outFile, a);
    if (rv) goto loser;

    if (encodedFile) {
	rv = BER_Unparse(a, WriteEncoded, encodedFile);
	if (rv) goto loser;
    }

    PORT_FreeArena(arena, PR_TRUE);
    return 0;

  loser:
    fprintf(stderr, "%s: lossage: rv=%d error=%d errno=%d\n",
	    progName, rv, PORT_GetError(), errno);
    return -1;
}
