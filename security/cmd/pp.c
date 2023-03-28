#include "secutil.h"

#if defined(__sun) && !defined(SVR4)
extern int getopt(int, char**, char*);
extern int fprintf(FILE *, char *, ...);
extern char *optarg;
#endif

extern void SEC_Init(void);		/* XXX */


static void Usage(char *progName)
{
    fprintf(stderr,
	    "Usage:  %s -t type [-a] [-i input] [-o output]\n",
	    progName);
    fprintf(stderr, "%-20s Specify the input type (must be one of %s,\n",
	    "-t type", SEC_CT_PRIVATE_KEY);
    fprintf(stderr, "%-20s %s, %s, %s,\n", "", SEC_CT_PUBLIC_KEY,
	    SEC_CT_CERTIFICATE, SEC_CT_CERTIFICATE_REQUEST);
    fprintf(stderr, "%-20s %s or %s)\n", "", SEC_CT_PKCS7);
    fprintf(stderr, "%-20s %s or %s)\n", "", SEC_CT_PKCS7, SEC_CT_CRL);    
    fprintf(stderr, "%-20s Input is in ascii encoded form (RFC1113)\n",
	    "-a");
    fprintf(stderr, "%-20s Define an input file to use (default is stdin)\n",
	    "-i input");
    fprintf(stderr, "%-20s Define an output file to use (default is stdout)\n",
	    "-o output");
    exit(-1);
}

int main(int argc, char **argv)
{
    int o, rv, ascii;
    char *progName;
    FILE *inFile, *outFile;
    SECItem der, data;
    char *typeTag;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    ascii = 0;
    inFile = 0;
    outFile = 0;
    typeTag = 0;
    while ((o = getopt(argc, argv, "at:i:o:")) != -1) {
	switch (o) {
	  case '?':
	    Usage(progName);
	    break;

	  case 'a':
	    ascii = 1;
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
	    typeTag = strdup(optarg);
	    break;
	}
    }

    if (!typeTag) Usage(progName);

    if (!inFile) inFile = stdin;
    if (!outFile) outFile = stdout;

    PR_Init("pp", 1, 1, 0);
    SEC_Init();

    if (ascii) {
	/* First convert ascii to binary */
	ATOBContext *atob;
	char *asc, *body, *trailer;
	unsigned part1, part2;

	/* Read in ascii data */
	asc = SECU_FileToString(inFile);
	if (!asc) {
	    fprintf(stderr, "%s: unable to read data from input file\n", progName);
	    return -1;
	}

	/* check for headers and trailers and remove them */
	if ((body = XP_STRSTR(asc, "-----BEGIN")) != NULL) {
	    body = PORT_Strchr(body, '\n') + 1;
	    trailer = XP_STRSTR(body, "-----END");
	    if (trailer != NULL) {
		*trailer = '\0';
	    } else {
		fprintf(stderr, "%s: input has header but no trailer\n", progName);
		return -1;
	    }
	} else {
	    body = asc;
	}
     
	/* Convert to binary */
	der.len = PORT_Strlen(body);
	der.data = (unsigned char*) PORT_Alloc(der.len);
	atob = ATOB_NewContext();
	rv = ATOB_Update(atob, der.data, &part1, der.len, body, der.len);
	if (rv) {
	  whine:
	    fprintf(stderr, "%s: error converting ascii to binary (%s)\n",
		    progName, SECU_ErrorString(PORT_GetError()));
	    return -1;
	}
	rv = ATOB_End(atob, der.data + part1, &part2, der.len - part1);
	if (rv) goto whine;
	der.len = part1 + part2;
	PORT_Free(asc);
    } else {
	/* Read in binary der */
	rv = SECU_DER_Read(&der, inFile);
	if (rv) {
	    fprintf(stderr, "%s: error converting der (%s)\n", progName,
		    SECU_ErrorString(PORT_GetError()));
	    return -1;
	}
    }

    /* Data is untyped, using the specified type */
    data.data = der.data;
    data.len = der.len;

    /* Pretty print it */
    if (PORT_Strcmp(typeTag, SEC_CT_CERTIFICATE) == 0) {
	rv = SECU_PrintSignedData(outFile, &data, "Certificate", 0,
			     SECU_PrintCertificate);
    } else if (PORT_Strcmp(typeTag, SEC_CT_CERTIFICATE_REQUEST) == 0) {
	rv = SECU_PrintSignedData(outFile, &data, "Certificate Request", 0,
			     SECU_PrintCertificateRequest);
    } else if (PORT_Strcmp (typeTag, SEC_CT_CRL) == 0) {
	rv = SECU_PrintSignedData (outFile, &data, "CRL", 0, SECU_PrintCrl);
    } else if (PORT_Strcmp(typeTag, SEC_CT_PRIVATE_KEY) == 0) {
	rv = SECU_PrintPrivateKey(outFile, &data, "Private Key", 0);
    } else if (PORT_Strcmp(typeTag, SEC_CT_PUBLIC_KEY) == 0) {
	rv = SECU_PrintPublicKey(outFile, &data, "Public Key", 0);
    } else if (PORT_Strcmp(typeTag, SEC_CT_PKCS7) == 0) {
	rv = SECU_PrintPKCS7ContentInfo(outFile, &data,
					"PKCS #7 Content Info", 0);
    } else {
	fprintf(stderr, "%s: don't know how to print out '%s' files\n",
		progName, typeTag);
	return -1;
    }

    if (rv) {
	fprintf(stderr, "%s: problem converting data (%s)\n",
		progName, SECU_ErrorString(PORT_GetError()));
	return -1;
    }
    return 0;
}
