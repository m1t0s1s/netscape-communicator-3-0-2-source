#include "xp_mcom.h"
#include "sec.h"

#if defined(__sun) && !defined(SVR4)
extern int fread(char *, size_t, size_t, FILE*);
extern int fwrite(char *, size_t, size_t, FILE*);
extern int fprintf(FILE *, char *, ...);
extern int fclose(FILE*);
extern int getopt(int, char**, char*);
extern int isatty(int);
extern char *optarg;
#endif

static void Usage(const char *progName)
{
    fprintf(stderr, "Usage: %s [-d] [-i file] [-o file]\n", progName);
    fprintf(stderr, "%20s: decrypt the input file\n", "-d");
    fprintf(stderr,
	    "%20s: specify the input file (otherwise stdin is used)\n",
	    "-i file");
    fprintf(stderr,
	    "%20s: specify the output file (otherwise stdout is used)\n",
	    "-o file");
    exit(-1);
}

int main(int argc, char **argv)
{
    int o, decrypt, nb;
    char *progName;
    FILE *inFile, *outFile;
    char *pw0, *pw1;
    RC4Context *rc4;
    SECStatus rv;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    /* Parse command line arguments */
    inFile = 0;
    outFile = 0;
    decrypt = 0;
    while ((o = getopt(argc, argv, "di:o:")) != -1) {
	switch (o) {
	  case '?':
	    Usage(progName);
	    break;

	  case 'd':
	    decrypt = 1;
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

  again:
    pw0 = SEC_GetPassword(stdin, stderr, "Password: ", SEC_CheckPassword);
    if (!decrypt && isatty(0)) {
	pw1 = SEC_GetPassword(stdin, stderr, "Re-enter Password: ",
			      SEC_CheckPassword);
	if (PORT_Strcmp(pw0, pw1) != 0) {
	    goto again;
	}
    }

    /* Transfer stdin to stdout encrypting/decrypting as appropriate */
    rc4 = RC4_MakeKey(pw0);
    if (!rc4) goto loser;
    for (;;) {
	unsigned char ibuf[1024], obuf[2048];
	unsigned part;

	if (feof(inFile)) break;
	nb = fread(ibuf, 1, sizeof(ibuf), inFile);
	if (nb == 0) {
	    if (ferror(inFile)) {
		fprintf(stderr, "%s: read error: %d\n", progName, errno);
		return -1;
	    }
	    break;
	}
	if (decrypt) {
	    rv = RC4_Decrypt(rc4, obuf, &part, sizeof(obuf), ibuf, nb);
	} else {
	    rv = RC4_Encrypt(rc4, obuf, &part, sizeof(obuf), ibuf, nb);
	}
	if (rv) goto loser;
	nb = fwrite(obuf, 1, part, outFile);
	if (nb != part) {
	    fprintf(stderr, "%s: write error: %d\n", progName, errno);
	    return -1;
	}
    }
    fclose(outFile);

    return 0;

  loser:
    fprintf(stderr, "%s: failure, error=%d\n", progName, PORT_GetError());
    return -1;
}
