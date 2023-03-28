#include "sec.h"

static void Usage(char *progName)
{
    fprintf(stderr, "Usage: %s -k key [-d] [-p] [-P] [-i input] [-o output]\n",
	    progName);
    fprintf(stderr, "%-20s Specify the key file\n",
	    "-k key");
    fprintf(stderr, "%-20s Decrypt the input (default is encrypt)\n",
	    "-d");
    fprintf(stderr, "%-20s Use PUBLIC key encryption/decryption\n",
	    "-p");
    fprintf(stderr, "%-20s Use PRIVATE key encryption/decryption\n",
	    "-P");
    fprintf(stderr, "%-20s Define an input file to use (default is stdin)\n",
	    "-i input");
    fprintf(stderr, "%-20s Define an output file to use (default is stdout)\n",
	    "-o output");
    exit(-1);
}

int main(int argc, char **argv)
{
    int o, i, rv, doPublic, doPrivate, decrypt;
    char *progName, *password;
    FILE *inFile, *outFile, *inKeyFile;
    SECPrivateKey *privKey;
    SECPublicKey *pubKey;
    RSAContext *rsa;
    unsigned dataPerBlock, modulusLen, zeroSpot;
    unsigned char blockType, *cp;
    unsigned char buf[1000], obuf[1000];
    unsigned part;
    SECItem eblock, data;
    int nb, amount;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    inKeyFile = 0;
    inFile = 0;
    outFile = 0;
    doPublic = 0;
    doPrivate = 0;
    decrypt = 0;
    while ((o = getopt(argc, argv, "k:i:o:pPd")) != -1) {
	switch (o) {
	  case '?':
	    Usage(progName);
	    break;

	  case 'd':
	    decrypt = 1;
	    break;

	  case 'p':
	    doPublic = 1;
	    break;

	  case 'P':
	    doPrivate = 1;
	    break;

	  case 'k':
	    inKeyFile = fopen(optarg, "r");
	    if (!inKeyFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for reading\n",
			progName, optarg);
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
	}
    }
    if (!inKeyFile || (doPublic && doPrivate) || (!doPublic && !doPrivate)) {
	Usage(progName);
    }

    if (!inFile) inFile = stdin;
    if (!outFile) outFile = stdout;

    /* Prompt for password and read in key pair */
    password = SEC_GetPassword(stdin, stderr, "Password: ", SEC_CheckPassword);
    privKey = SEC_ReadEncryptedPrivateKey(inKeyFile, password);
    if (!privKey) goto loser;

    /* Create rsa cypher */
    if (doPublic) {
	pubKey = SEC_ConvertToPublicKey(privKey);
	if (!pubKey) goto loser;
	rsa = RSA_CreateContext(0, pubKey);
    } else {
	rsa = RSA_CreateContext(privKey, 0);
    }
    if (!rsa) goto loser;

    /*
    ** Perform encryption operation on the data. Compute how much data we
    ** can handle at a time.
    **
    ** For public key encryption, block type 0x02 is used. This has an
    ** overhead of 11 bytes. See PKCS#1 for more information.
    **
    ** For private key encryption, block type 0x01 is used. This has an
    ** overhead of 11 bytes. See PKCS#1 for more information.
    */
    modulusLen = privKey->modulus.len - 1;
    dataPerBlock = modulusLen - 11;
    if (doPublic) {
	blockType = 0x02;
    } else {
	blockType = 0x01;
    }
    zeroSpot = modulusLen - dataPerBlock - 1;
    for (;;) {
	/* Read in a hunk of data */
	nb = fread(buf, 1, decrypt ? modulusLen : dataPerBlock, inFile);
	if (nb == 0) break;
	if (nb < 0) {
	    XP_SetError(errno);
	    goto loser;
	}

	if (decrypt) {
	    if (nb != modulusLen) {
		XP_SetError(SEC_ERROR_INPUT_LEN);
		goto loser;
	    }

	    /* Decrypt the block */
	    rv = RSA_Decrypt(rsa, obuf, &part, sizeof(obuf),
			     buf, nb);
	    if (rv) goto loser;

	    /* Examine the encryption block */
	    if ((obuf[0] != 0x00) ||
		((obuf[1] != 0x01) && (obuf[1] != 0x02))) {
		XP_SetError(SEC_ERROR_BAD_DATA);
		goto loser;
	    }

	    /*
	    ** Parse encryption block. The padding string will be
	    ** non-zero followed by a 0x00 followed by the data.  We take
	    ** a guess and look at the block at the spot where we think
	    ** the data is first. If that loses, then search the hard way
	    */
	    if ((obuf[zeroSpot] == 0x00) && (obuf[zeroSpot-1] != 0x00)) {
		/* Winner */
		amount = dataPerBlock;
		cp = &obuf[zeroSpot+1];
	    } else {
		/* Short block (must be the last block in the file) */
		cp = &obuf[2];
		for (i = 2; i < modulusLen; i++) {
		    if (*cp++ == 0x00) {
			/* Found the zero */
			amount = modulusLen - i - 1;
			break;
		    }
		}
	    }
	} else {
	    /* Format the encryption block */
	    data.data = buf;
	    data.len = nb;
	    rv = RSA_FormatBlock(&eblock, modulusLen, blockType, &data);
	    if (rv) goto loser;

	    /* Encrypt the block */
	    rv = RSA_Encrypt(rsa, obuf, &part, sizeof(obuf),
			     eblock.data, eblock.len);
	    if (rv) goto loser;
	    amount = part;
	    cp = obuf;
	}

	/* Write data */
	nb = fwrite(cp, 1, amount, outFile);
	if (nb != amount) {
	    XP_SetError(errno);
	    goto loser;
	}
    }
    fflush(outFile);
    return 0;

  loser:
    fprintf(stderr, "%s: something failed, error=%d (0x%x)\n",
	    progName, XP_GetError(), XP_GetError());
    return -1;
}
