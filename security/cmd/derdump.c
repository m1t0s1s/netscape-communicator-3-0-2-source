#include "secutil.h"

#if defined(__sun) && !defined(SVR4)
extern int fprintf(FILE *, char *, ...);
extern int getopt(int, char**, char*);
extern char *optarg;
#endif

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
    int option;
    FILE *inFile, *outFile;
    SECItem der;
    SECStatus rv;
    int16 xp_error;
    
    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    /* Parse command line arguments */
    inFile = 0;
    outFile = 0;
    while ((option = getopt(argc, argv, "i:o:")) != -1) {
	switch (option) {
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

    rv = SECU_DER_Read(&der, inFile);
    if (rv == SECSuccess) {
	rv = DER_PrettyPrint(outFile, &der);
	if (rv == SECSuccess)
	    return 0;
    }

    xp_error = PORT_GetError();
    if (xp_error) {
	SECU_PrintError(progName, "error %d", xp_error);
    }
    if (errno) {
	SECU_PrintSystemError(progName, "errno=%d", errno);
    }
    return 1;
}
