#include "sec.h"

#if defined(__sun) && !defined(SVR4)
extern int getopt(int, char**, char*);
extern int fprintf(FILE *, char *, ...);
extern int fwrite(char *, size_t, size_t, FILE*);
extern void fflush(FILE *);
#endif
extern char *optarg;

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
    PRArenaPool *arena;
    char *progName;
    int o, rv, nb;
    FILE *inFile, *outFile;
    SECItem wrap;
    SECTypedData td;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena) {
	fprintf(stderr, "%s: unable to allocate memory\n",
		progName);
	return -1;
    }

    /* Parse command line arguments */
    inFile = 0;
    outFile = 0;
    while ((o = getopt(argc, argv, "t:i:o:")) != -1) {
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

    /* First read in the typed data der */
    rv = DER_Read(arena, &wrap, inFile);
    if (rv) {
	fprintf(stderr, "%s: input file is not good der (error=%d)\n",
		progName, PORT_GetError());
	return -1;
    }

    /* Now unwrap it */ 
    rv = DER_Decode(arena, &td, SECTypedDataTemplate, &wrap);
    if (rv) {
	fprintf(stderr, "%s: unable to der decode file (error=%d)\n",
		progName, PORT_GetError());
	return -1;
    }

    nb = fwrite(td.data.data, 1, td.data.len, outFile);
    if (nb != td.data.len) {
	fprintf(stderr, "%s: write error: errno=%d\n",
		progName, errno);
	return -1;
    }
    fflush(outFile);
    return 0;
}
