#include "sec.h"

#if defined(__sun) && !defined(SVR4)
extern int fread(char *, size_t, size_t, FILE*);
extern int fprintf(FILE *, char *, ...);
extern int fclose(FILE*);
extern int getopt(int, char**, char*);
extern char *optarg;
#endif

static PRArenaPool *arena;

SECStatus SEC_ATOBFile(SECItem *result, FILE *inFile)
{
    unsigned char *buf;
    unsigned count, rem, part, bufSize, newSize;
    ATOBContext *cx;
    int nb;
    SECStatus rv;

    cx = ATOB_NewContext();
    if (!cx) {
	return PORT_GetError();
    }

    buf = 0;
    count = 0;
    bufSize = 0;
    rv = ATOB_Begin(cx);
    if (rv) goto loser;
    for (;;) {
	char ibuf[4096];

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
	newSize = count + nb + 128;
	if (buf) {
	    buf = (unsigned char*) PORT_Realloc(buf, newSize);
	} else {
	    buf = (unsigned char*) PORT_Alloc(newSize);
	}
	if (!buf) goto loser;
	bufSize = newSize;

	rv = ATOB_Update(cx, buf + count, &part, bufSize-count, ibuf, nb);
	if (rv) goto loser;
	count += part;
    }
    rv = ATOB_End(cx, buf+count, &rem, bufSize-count);
    if (rv) goto loser;
    count += rem;

    /* Trim buffer down to fit data actually decoded */
    buf = (unsigned char*) PORT_Realloc(buf, count);
    if (!buf) goto loser;
    result->data = buf;
    result->len = count;
    rv = 0;
    goto done;

  loser:
    PORT_Free(buf);

  done:
    ATOB_DestroyContext(cx);
    return rv;
}

static SECKEYPublicKey *ReadCert(char *fileName)
{
    char *typeTag;
    int rv;
    SECItem certder, rawder;
    FILE *fp;
    CERTCertificate *c;
    SECTypedData td;
    CERTSignedData sd;

    fp = fopen(fileName, "r");
    if (!fp) return 0;

    /* Get data. It might be wrapped and it might not be... */
    rv = DER_Read(&rawder, fp);
    if (rv) return 0;
    PORT_Memset(&td, 0, sizeof(td));
    rv = DER_Decode(arena, &td, SECTypedDataTemplate, &rawder);
    if (rv) {
	/* data was not wrapped, maybe */
	certder = rawder;
    } else {
	/* Data is wrapped ... */
	if ((td.type.len != sizeof(SEC_CT_CERTIFICATE)-1) ||
	    PORT_Memcmp(td.type.data, SEC_CT_CERTIFICATE,
		      sizeof(SEC_CT_CERTIFICATE)-1)) {
	    /* Then again, maybe not... */
	    certder = rawder;
	} else
	    certder = td.data;
    }

    PORT_Memset(&sd, 0, sizeof(sd));
    rv = DER_Decode(arena, &sd, CERTSignedDataTemplate, &certder);
    if (rv) return 0;
    c = SEC_NewCertificate();
    if (!c) return 0;
    rv = DER_Decode(arena, c, CERTCertificateTemplate, &sd.data);
    if (rv) return 0;
    return CERT_ExtractPublicKey(&c->subjectPublicKeyInfo);
}

static SECKEYPublicKey *ReadCertRequest(char *fileName)
{
    char *typeTag;
    int rv;
    SECItem certder;
    FILE *fp;
    CERTCertificateRequest *cr;
    CERTSignedData sd;
    SECTypedData td;
    SECItem rawder;

    fp = fopen(fileName, "r");
    if (!fp) return 0;

    /* Get data. It might be wrapped and it might not be... */
    rv = DER_Read(&rawder, fp);
    if (rv) return 0;
    PORT_Memset(&td, 0, sizeof(td));
    rv = DER_Decode(arena, &td, SECTypedDataTemplate, &rawder);
    if (rv) {
	/* data was not wrapped, maybe */
	certder = rawder;
    } else {
	/* Data is wrapped ... */
	if ((td.type.len != sizeof(SEC_CT_CERTIFICATE_REQUEST)-1) ||
	    PORT_Memcmp(td.type.data, SEC_CT_CERTIFICATE_REQUEST,
		      sizeof(SEC_CT_CERTIFICATE_REQUEST)-1)) {
	    /* Then again, maybe not... */
	    certder = rawder;
	} else
	    certder = td.data;
    }

    PORT_Memset(&sd, 0, sizeof(sd));
    rv = DER_Decode(arena, &sd, CERTSignedDataTemplate, &certder);
    if (rv) return 0;
    cr = (CERTCertificateRequest*) PORT_ZAlloc(sizeof(CERTCertificateRequest));
    if (!cr) return 0;
    rv = DER_Decode(arena, cr, CERTCertificateRequestTemplate, &sd.data);
    if (rv) return 0;
    return CERT_ExtractPublicKey(&cr->subjectPublicKeyInfo);
}

static void Usage(char *progName)
{
    fprintf(stderr,
	    "Usage: %s [-k keyfile] [-c certfile] [-r certreqfile] [-q] [-d] [-i input] [-s sig-file]\n",
	    progName);
    fprintf(stderr, "%-20s Specify the file that contains the key pair\n",
	    "-k keyfile");
    fprintf(stderr, "%-20s Specify the file that contains a certificate\n",
	    "-c certfile");
    fprintf(stderr, "%-20s Specify the file that contains a certificate request\n",
	    "-r certreqfile");
    fprintf(stderr, "%-20s Define an input file to use (default is stdin)\n",
	    "-i input");
    fprintf(stderr, "%-20s Define a signature file to use\n",
	    "-s sig-file");
    fprintf(stderr, "%-20s Expect DER signed input\n",
	    "-d");
    fprintf(stderr, "%-20s Ascii decode signature input using RFC1113\n",
	    "-a");
    fprintf(stderr, "%-20s Be quiet; No messages\n",
	    "-q");
    fprintf(stderr, "A signature file must be specified if -d is not used\n");
    fprintf(stderr, "One of -k or -c must be used\n");
    fprintf(stderr, "Can't mix ascii and der yet\n");
    exit(-1);
}

int main(int argc, char **argv)
{
    char *progName;
    int o, rv, der, quiet, ascii;
    SECKEYPrivateKey *k;
    SECKEYPublicKey *pubKey;
    FILE *inFile, *sigFile;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena) {
	fprintf(stderr, "%s: unable to allocate memory\n", progName);
	return -1;
    }
    
    /* Parse command line arguments */
    inFile = 0;
    sigFile = 0;
    k = 0;
    pubKey = 0;
    der = 0;
    quiet = 0;
    ascii = 0;
    while ((o = getopt(argc, argv, "c:r:k:i:s:daq")) != -1) {
	switch (o) {
	  case '?':
	    Usage(progName);
	    break;

	  case 'c':
	    pubKey = ReadCert(optarg);
	    break;

	  case 'r':
	    pubKey = ReadCertRequest(optarg);
	    break;

	  case 'k':
	    k = SEC_GetPrivateKey(optarg, stdin, stderr);
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

	  case 's':
	    sigFile = fopen(optarg, "r");
	    if (!sigFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for reading\n",
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

	  case 'q':
	    quiet = 1;
	    break;
	}
    }
    if (!k && !pubKey) {
	Usage(progName);
    }
    if (k && pubKey) {
	Usage(progName);
    }
    if (!inFile) inFile = stdin;

    if (k) {
	pubKey = SECKEY_ConvertToPublicKey(k);
    }

    if (der) {
	SECItem d;
	CERTSignedData sd;

	PORT_Memset(&sd, 0, sizeof(sd));
	do {
	    rv = DER_Read(&d, inFile);
	    if (rv) break;
	    rv = DER_Decode(arena, &sd, CERTSignedDataTemplate, &d);
	    if (rv) break;
	    DER_ConvertBitString(&sd.signature);
	    rv = VFY_VerifyData(sd.data.data, sd.data.len,
				pubKey, &sd.signature);
	} while (0);
    } else {
	char buf[4096];
	SECItem sig;

	if (!sigFile) {
	    Usage(progName);
	}

	/* Read it in, whatever it is */
	if (ascii) {
	    rv = SEC_ATOBFile(&sig, sigFile);
	    if (rv && !quiet) {
		fprintf(stderr, "%s: lossage: rv=%d error=%d errno=%d\n",
			progName, rv, PORT_GetError(), errno);
		return -1;
	    }
	} else {
	    rv = fread(buf, 1, sizeof(buf), sigFile);
	    if (rv == 0) {
		if (!quiet)
		    fprintf(stderr, "%s: bad signature file\n", progName);
		return -1;
	    }
	    sig.data = buf;
	    sig.len = rv;
	}
	rv = SEC_VerifyFile(inFile, pubKey, &sig);
    }
    if (!rv && !quiet) {
	fprintf(stderr, "%s: signature check passed\n", progName);
    } else
    if (rv && !quiet) {
	fprintf(stderr, "%s: signature check FAILED\n", progName);
	return -1;
    }

    return 0;
}
