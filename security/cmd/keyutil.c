#include "secutil.h"
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>

#if defined(LINUX)
#include <getopt.h>
#endif

static PRArenaPool *arena;
static char *progName;

#if defined(__sun) && !defined(SVR4)
extern int fclose(FILE*);
extern int fprintf(FILE *, char *, ...);
extern int getopt(int, char**, char*);
extern int isatty(int);
extern char *optarg;
extern char *sys_errlist[];
#define strerror(errno) sys_errlist[errno]
#endif

#define MIN_KEY_BITS	256

#define MAX_KEY_BITS	1024

#define NUM_KEYSTROKES 120
#define RAND_BUF_SIZE 60

#define SEC_KEY_DB_EXISTS 0
#define SEC_CREATE_KEY_DB 1

#define ERROR_BREAK rv = SECFailure;break;

static SECKEYKeyDBHandle *
OpenKeyDB(int createnew)
{
    SECKEYKeyDBHandle *keyHandle;

    keyHandle = SECU_OpenKeyDB();
    if (keyHandle == NULL) {
        SECU_PrintError(progName, "could not open key database");
	return NULL;
    }

    return(keyHandle);
}

static RNGContext *
GetRNGContext(void)
{
    RNGContext *rng;
    SECStatus rv;
    char *randbuf;
    int fd, i, count;
    char c;
    struct termios tio;
    cc_t orig_cc_min, orig_cc_time;
    tcflag_t orig_lflag;

    printf("\n");
    printf("A random seed must be generated that will be used in the\n");
    printf("creation of your key.  One of the easiest ways to create a\n");
    printf("random seed is to use the timing of keystrokes on a keyboard.\n");
    printf("\n");
    printf("To begin, type keys on the keyboard until this progress meter\n");
    printf("is full.  DO NOT USE THE AUTOREPEAT FUNCTION ON YOUR KEYBOARD!\n");
    printf("\n");
    printf("\n");
    printf("Continue typing until the progress meter is full:\n\n");
    printf("|                                                            |\r");

    /* turn off echo on stdin & return on 1 char instead of NL */
    fd = fileno(stdin);
    tcgetattr(fd, &tio);
    orig_lflag = tio.c_lflag;
    orig_cc_min = tio.c_cc[VMIN];
    orig_cc_time = tio.c_cc[VTIME];
    tio.c_lflag &= ~ECHO;
    tio.c_lflag &= ~ICANON;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    tcsetattr(fd, TCSAFLUSH, &tio);
    
    /* Get random noise from keyboard strokes */
    randbuf = (char *) PORT_Alloc(RAND_BUF_SIZE);
    count = 0;
    while (count < NUM_KEYSTROKES+1) {
	c = getc(stdin);
	RNG_GetNoise(&randbuf[1], sizeof(randbuf)-1);
	RNG_RandomUpdate(randbuf, sizeof(randbuf));
	if (c != randbuf[0]) {
	    randbuf[0] = c;
	    printf("\r|");
	    for (i=0; i<count/(NUM_KEYSTROKES/RAND_BUF_SIZE); i++) {
		printf("*");
	    }
	    if (count%(NUM_KEYSTROKES/RAND_BUF_SIZE) == 1) printf("/");
	    count++;
	}
    }
    free(randbuf); 

    printf("\n\n");
    printf("Finished.  Press enter to continue: ");
    while (getc(stdin) != '\n')
	;

    /* set back termio the way it was */
    tio.c_lflag = orig_lflag;
    tio.c_cc[VMIN] = orig_cc_min;
    tio.c_cc[VTIME] = orig_cc_time;
    tcsetattr(fd, TCSAFLUSH, &tio);

    rng = RNG_CreateContext();
    if (!rng) goto loser;

    return rng;

  loser:
    SECU_PrintError(progName, "unable to create key");
    return NULL;
}

static SECStatus
GenerateRSAKey(char *nickname, int keyBits, int publicExponent)
{
    RNGContext *rng;
    SECStatus rv;
    SECItem pe;
    SECKEYLowPrivateKey *key;
    SECKEYKeyDBHandle *handle;

    /* open key database if it exists */
    handle = OpenKeyDB(SEC_CREATE_KEY_DB);
    if (handle == NULL) {
	return(SECFailure);
    }    

    /* New interface stuffs. */
    printf("--------------------------------------------------------\n");
    printf("|         Netscape Communications Corporation          |\n");
    printf("|                  Key Generation                      |\n");
    printf("--------------------------------------------------------\n");
    printf("\n");
    printf("Welcome to the key generator.  With this program, you can\n");
    printf("generate the public and private keys that you use for\n");
    printf("secure communications.\n");
    printf("\n");
    printf("You have specified the name \"%s\" for your key\n\n", nickname);
    printf("If this is correct, press enter: ");
    while (getc(stdin) != '\n')
	;
    
    /* check if key already exists */
    if (SECU_CheckKeyNameExists(handle, nickname) == PR_TRUE) {
	printf("\n");
	SECU_PrintError(progName, "key \"%s\" already exists", nickname);
	return SECFailure;
    }

    rv = DER_SetInteger(arena, &pe, publicExponent);
    if (rv) goto loser;
    rng = GetRNGContext();

    printf("\n\n");
    printf("Generating key.  This may take a few moments...\n\n");
 
    key = RSA_NewKey(rng, keyBits, &pe);
    if (!key) goto loser;

    rv = SECU_PutPrivateKey(handle, nickname, key);
    if(rv) {
	goto loser;
    }

    printf("\n");
    printf("Your new key named \"%s\" has been stored in the database.",
	   nickname);
    printf("\n");
    return SECSuccess;

loser:
    return SECFailure;
}

static unsigned char P[] = { 0x00, 0x8d, 0xf2, 0xa4, 0x94, 0x49, 0x22, 0x76,
			     0xaa, 0x3d, 0x25, 0x75, 0x9b, 0xb0, 0x68, 0x69,
			     0xcb, 0xea, 0xc0, 0xd8, 0x3a, 0xfb, 0x8d, 0x0c,
			     0xf7, 0xcb, 0xb8, 0x32, 0x4f, 0x0d, 0x78, 0x82,
			     0xe5, 0xd0, 0x76, 0x2f, 0xc5, 0xb7, 0x21, 0x0e,
			     0xaf, 0xc2, 0xe9, 0xad, 0xac, 0x32, 0xab, 0x7a,
			     0xac, 0x49, 0x69, 0x3d, 0xfb, 0xf8, 0x37, 0x24,
			     0xc2, 0xec, 0x07, 0x36, 0xee, 0x31, 0xc8, 0x02,
			     0x91 };
static unsigned char Q[] = { 0x00, 0xc7, 0x73, 0x21, 0x8c, 0x73, 0x7e, 0xc8,
			     0xee, 0x99, 0x3b, 0x4f, 0x2d, 0xed, 0x30, 0xf4,
			     0x8e, 0xda, 0xce, 0x91, 0x5f };
static unsigned char G[] = { 0x00, 0x62, 0x6d, 0x02, 0x78, 0x39, 0xea, 0x0a,
			     0x13, 0x41, 0x31, 0x63, 0xa5, 0x5b, 0x4c, 0xb5,
			     0x00, 0x29, 0x9d, 0x55, 0x22, 0x95, 0x6c, 0xef,
			     0xcb, 0x3b, 0xff, 0x10, 0xf3, 0x99, 0xce, 0x2c,
			     0x2e, 0x71, 0xcb, 0x9d, 0xe5, 0xfa, 0x24, 0xba,
			     0xbf, 0x58, 0xe5, 0xb7, 0x95, 0x21, 0x92, 0x5c,
			     0x9c, 0xc4, 0x2e, 0x9f, 0x6f, 0x46, 0x4b, 0x08,
			     0x8c, 0xc5, 0x72, 0xaf, 0x53, 0xe6, 0xd7, 0x88,
			     0x02 };

static PQGParams default_pqg_params = {
    NULL,
    { 0, P, sizeof(P) },
    { 0, Q, sizeof(Q) },
    { 0, G, sizeof(G) }
};

static SECStatus
GenerateDSAKey(char *nickname, char *pqgFile)
{
    RNGContext *rng;
    SECStatus rv;
    SECItem pe;
    SECKEYLowPrivateKey *privKey;
    SECKEYLowPublicKey *pubKey;
    SECKEYKeyDBHandle *handle;
    PQGParams *pqg;

    /* open key database if it exists */
    handle = OpenKeyDB(SEC_CREATE_KEY_DB);
    if (handle == NULL) {
	return(SECFailure);
    }    

    /* New interface stuffs. */
    printf("--------------------------------------------------------\n");
    printf("|         Netscape Communications Corporation          |\n");
    printf("|                  Key Generation                      |\n");
    printf("--------------------------------------------------------\n");
    printf("\n");
    printf("Welcome to the key generator.  With this program, you can\n");
    printf("generate the public and private keys that you use for\n");
    printf("secure communications.\n");
    printf("\n");
    printf("You have specified the name \"%s\" for your key\n\n", nickname);
    printf("If this is correct, press enter: ");
    while (getc(stdin) != '\n')
	;
    
    /* check if key already exists */
    if (SECU_CheckKeyNameExists(handle, nickname) == PR_TRUE) {
	printf("\n");
	SECU_PrintError(progName, "key \"%s\" already exists", nickname);
	return SECFailure;
    }

    pqg = &default_pqg_params;
    rng = GetRNGContext();

    printf("\n\n");
    printf("Generating key.  This may take a few moments...\n\n");
 
    rv = DSA_NewKey(pqg, &pubKey, &privKey);
    if (rv) goto loser;

    rv = SECU_PutPrivateKey(handle, nickname, privKey);
    if(rv) {
	goto loser;
    }

    printf("\n");
    printf("Your new key named \"%s\" has been stored in the database.",
	   nickname);
    printf("\n");
    return SECSuccess;

loser:
    return SECFailure;
}

static SECStatus
ListKeys(FILE *out)
{
    int rt;
    SECKEYKeyDBHandle *handle;

    /* open key database if it exists */
    handle = OpenKeyDB(SEC_KEY_DB_EXISTS);
    if (handle == NULL) {
	return(SECFailure);
    }

    rt = SECU_PrintKeyNames(handle, out);
    if (rt) {
	SECU_PrintError(progName, "unable to list nicknames");
	return SECFailure;
    }
    return SECSuccess;
}

static SECStatus
DumpPublicKey(char *nickname, FILE *out)
{
    SECKEYLowPrivateKey *privKey;
    SECKEYLowPublicKey *publicKey;
    SECKEYKeyDBHandle *handle;

    /* open key database if it exists */
    handle = OpenKeyDB(SEC_KEY_DB_EXISTS);
    if (handle == NULL) {
	return(SECFailure);
    }

    /* check if key actually exists */
    if (SECU_CheckKeyNameExists(handle, nickname) == PR_FALSE) {
	SECU_PrintError(progName, "the key \"%s\" does not exist", nickname);
	return SECFailure;
    }

    /* Read in key */
    privKey = SECU_GetPrivateKey(handle, nickname);
    if (!privKey) {
	return SECFailure;
    }

    publicKey = SECKEY_LowConvertToPublicKey(privKey);

    /* Output public key (in the clear) */
    switch(publicKey->keyType) {
      case rsaKey:
	fprintf(out, "RSA Public-Key:\n");
	SECU_PrintInteger(out, &publicKey->u.rsa.modulus, "modulus", 1);
	SECU_PrintInteger(out, &publicKey->u.rsa.publicExponent,
			  "publicExponent", 1);
	break;
      case dsaKey:
	fprintf(out, "DSA Public-Key:\n");
	SECU_PrintInteger(out, &publicKey->u.dsa.params.prime, "prime", 1);
	SECU_PrintInteger(out, &publicKey->u.dsa.params.subPrime,
			  "subPrime", 1);
	SECU_PrintInteger(out, &publicKey->u.dsa.params.base, "base", 1);
	SECU_PrintInteger(out, &publicKey->u.dsa.publicValue, "publicValue", 1);
	break;
      default:
	fprintf(out, "unknown key type\n");
	break;
    }
    return SECSuccess;
}

static SECStatus
DumpPrivateKey(char *nickname, FILE *out)
{
    SECKEYLowPrivateKey *key;
    SECKEYKeyDBHandle *handle;

    /* open key database if it exists */
    handle = OpenKeyDB(SEC_KEY_DB_EXISTS);
    if (handle == NULL) {
	return(SECFailure);
    }

    /* check if key actually exists */
    if (SECU_CheckKeyNameExists(handle, nickname) == PR_FALSE) {
	SECU_PrintError(progName, "the key \"%s\" does not exist", nickname);
	return SECFailure;
    }

    /* Read in key */
    key = SECU_GetPrivateKey(handle, nickname);
    if (!key) {
	SECU_PrintError(progName, "error retrieving key");
	return SECFailure;
    }

    switch(key->keyType) {
      case rsaKey:
	fprintf(out, "RSA Private-Key:\n");
	SECU_PrintInteger(out, &key->u.rsa.modulus, "modulus", 1);
	SECU_PrintInteger(out, &key->u.rsa.publicExponent, "publicExponent", 1);
	SECU_PrintInteger(out, &key->u.rsa.privateExponent,
			  "privateExponent", 1);
	SECU_PrintInteger(out, &key->u.rsa.prime[0], "prime[0]", 1);
	SECU_PrintInteger(out, &key->u.rsa.prime[1], "prime[1]", 1);
	SECU_PrintInteger(out, &key->u.rsa.primeExponent[0],
			  "primeExponent[0]", 1);
	SECU_PrintInteger(out, &key->u.rsa.primeExponent[1],
			  "primeExponent[1]", 1);
	SECU_PrintInteger(out, &key->u.rsa.coefficient, "coefficient", 1);
	break;
      case dsaKey:
	fprintf(out, "DSA Private-Key:\n");
	SECU_PrintInteger(out, &key->u.dsa.params.prime, "prime", 1);
	SECU_PrintInteger(out, &key->u.dsa.params.subPrime, "subPrime", 1);
	SECU_PrintInteger(out, &key->u.dsa.params.base, "base", 1);
	SECU_PrintInteger(out, &key->u.dsa.publicValue, "publicValue", 1);
	SECU_PrintInteger(out, &key->u.dsa.privateValue, "privateValue", 1);
	break;
      default:
	fprintf(out, "unknown key type\n");
	break;
    }
    return SECSuccess;
}

static SECStatus
ChangePassword(void)
{
    SECKEYKeyDBHandle *handle;
    SECStatus rv;

    /* open key database if it exists */
    handle = OpenKeyDB(SEC_KEY_DB_EXISTS);
    if (handle == NULL) {
	return(SECFailure);
    }

    /* Write out database with a new password */
    rv = SECU_ChangeKeyDBPassword(handle);
    if (rv) {
	SECU_PrintError(progName, "unable to change key password");
    }
    return rv;
}

static AddPrivateKey (char *nickName, FILE *inFile)
{
    PRArenaPool *poolp = NULL;
    SECItem privKeyDER;    
    SECKEYKeyDBHandle *keyHandle;
    SECKEYPrivateKeyInfo *pki;
    SECKEYLowPrivateKey *pk;
    SECStatus rv;

    privKeyDER.data = NULL;
    
    /* open key database if it exists */
    keyHandle = OpenKeyDB(SEC_CREATE_KEY_DB);
    if (keyHandle == NULL) 
	return (-1);
    
    do {
	rv = SECU_DER_Read(&privKeyDER, inFile);
	if (rv != SECSuccess) 
	    break;
	
	poolp = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if (poolp == NULL) {
	    ERROR_BREAK;
	}
	pki = PORT_ArenaZAlloc(poolp, sizeof(SECKEYPrivateKeyInfo));
	if (pki == NULL) {
	    ERROR_BREAK;
	}
	rv = SEC_ASN1DecodeItem(poolp, pki, SECKEY_PrivateKeyInfoTemplate,
				&privKeyDER);

	if (rv != SECSuccess) {
	    ERROR_BREAK;
	}
	pk = PORT_ArenaZAlloc(poolp, sizeof(SECKEYLowPrivateKey));
	if (pk == NULL) {
	    ERROR_BREAK;
	}
	rv = SEC_ASN1DecodeItem(poolp, pk, SECKEY_RSAPrivateKeyTemplate,
				&pki->privateKey);
	if (rv != SECSuccess) {
	    ERROR_BREAK;
	}
	pk->keyType = rsaKey; /* XXX RSA */
	rv = SECU_PutPrivateKey (keyHandle, nickName, pk);
    } while (0);
    PORT_Free (privKeyDER.data);    
    PORT_FreeArena (poolp, PR_TRUE);

    if (rv != SECSuccess) {
	fprintf(stderr, "%s: problem adding private key (%s)\n",
		progName, SECU_ErrorString(PORT_GetError()));
	return (-1);
    }

    return (rv);
}

static SECStatus DeletePrivateKey (char *nickName)
{
    SECKEYKeyDBHandle *keyHandle;
    int rv;

    /* open key database if it exists */
    keyHandle = OpenKeyDB(SEC_CREATE_KEY_DB);
    if (keyHandle == NULL) 
	return (-1);
    
    rv = SECU_DeleteKeyByName (keyHandle, nickName);
    if (rv != SECSuccess)
	fprintf(stderr, "%s: problem deleting private key (%s)\n",
		progName, SECU_ErrorString(PORT_GetError()));
    return (rv);

}


static void
Usage(const char *progName)
{
    fprintf(stderr,
	    "Usage:  %s -n name -g [-t rsa] [-s num] [-e exp] [-d keydir]\n"
	    "        %s -n name -g -t dsa [-q pqgfile] [-d keydir]\n"
	    "        %s -A -n nickname -i privatekey [-d keydir]\n"
	    "        %s -D -n nickname [-d keydir]\n",	    
	    progName, progName, progName, progName);
    fprintf(stderr, "        %s -n name [-p|-P] [-d keydir]\n", progName);
    fprintf(stderr, "        %s [-c|-l] [-d keydir]\n", progName);

    /* generate new key pair */
    fprintf(stderr, "%-20s Nickname attributed to the key\n",
	    "-n name");
    fprintf(stderr, "%-20s Generate a new key\n",
	    "-g");
    fprintf(stderr, "%-20s Specify type of key to generate\n",
	    "-t [rsa|dsa]");
    fprintf(stderr, "%-20s Set key size (min of %d, max of %d, default %d)\n",
	    "-s size", MIN_KEY_BITS, MAX_KEY_BITS, MAX_KEY_BITS);
    fprintf(stderr, "%-20s Set the public exponent value (3, 17, 65537)\n",
	    "-e exp");
    fprintf(stderr, "%-20s read PQG value from pqgfile\n",
	    "-q pqgfile");
    fprintf(stderr, "%-20s Pretty print the public key information\n",
	    "-p");
    fprintf(stderr, "%-20s Pretty print the private key information\n",
	    "-P");
    fprintf(stderr, "%-20s Change the key database password\n",
	    "-c");
    fprintf(stderr, "%-20s List the nicknames for the keys in a database\n",
	    "-l");
    fprintf(stderr, "%-20s Key database directory (default is ~/.netscape)\n",
	    "-d keydir");
    /* Add a private key to the dbase */
    fprintf (stderr, "%-15s Add a private key to the key dbase\n", "-A");
    fprintf(stderr, "%-20s Specify the nickname for the key to add\n",
	    "-n nickname");
    fprintf(stderr, "%-20s Key database directory (default is ~/.netscape)\n",
	    "-d keydir");
    fprintf(stderr, "%-20s Specify the binary private key file\n",
	    "-i privatekey");

    /* Remove a private key from the dbase */
    fprintf (stderr, "%-15s Delete a private key from the key dbase\n", "-D");    
    fprintf(stderr, "%-20s Specify the nickname for the key to add\n",
	    "-n nickname");

    exit(-1);
}

int main(int argc, char **argv)
{
    int o, generateKey, changePassword, dumpPublicKey, dumpPrivateKey, list;
    int keyBits, publicExponent;
    int addKey, deleteKey;
    char *nickname;
    FILE *inFile;
    SECStatus rv;
    char *pqgFile = NULL;
    KeyType keyType = rsaKey;

    addKey = deleteKey = 0;
    inFile = NULL;    
    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena) {
	fprintf(stderr, "%s: unable to allocate memory\n", progName);
	return -1;
    }

    /* Parse command line arguments */
    keyBits = MAX_KEY_BITS;
    publicExponent = 0x010001;
    generateKey = changePassword = dumpPublicKey = dumpPrivateKey = list = 0;
    nickname = 0;

    while ((o = getopt(argc, argv, "Ai:Dn:clgpPd:s:e:t:q:")) != -1) {
	switch (o) {
	  case '?':
	    Usage(progName);
	    break;

	  case 'A':
	      addKey = 1;
	      break;

	  case 'D':
	      deleteKey = 1;
	      break;

          case 'n':
	    nickname = optarg;
	    break;

	  case 'l':
	    list = 1;
	    break;

	  case 'g':
	    generateKey = 1;
	    break;

	  case 'c':
	    changePassword = 1;
	    break;

	  case 'i':
	    inFile = fopen(optarg, "r");
	    if (!inFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for reading\n",
			progName, optarg);
		return -1;
	    }
	    break;

	  case 'p':
	    dumpPublicKey = 1;
	    break;

	  case 'd':
	    SECU_ConfigDirectory(optarg);
	    break;

	  case 'P':
	    dumpPrivateKey = 1;
	    break;

	  case 's':
	    keyBits = atoi(optarg);
	    if ((keyBits < MIN_KEY_BITS) || (keyBits > MAX_KEY_BITS)) {
		Usage(progName);
	    } 
	    break;

	  case 'e':
	    publicExponent = atoi(optarg);
	    if ((publicExponent != 3) &&
		(publicExponent != 17) &&
		(publicExponent != 65537)) {
		Usage(progName);
	    }
	    break;
	  case 't':
	    if (strcmp(optarg, "rsa") == 0) {
		keyType = rsaKey;
	    } else if (strcmp(optarg, "dsa") == 0) {
		keyType = dsaKey;
	    } else {
		Usage(progName);
	    }
	    break;
	  case 'q':
	    if (keyType != dsaKey)
		Usage(progName);
	    pqgFile = strdup(optarg);
	    break;
	}
    }

    if (generateKey+dumpPublicKey+changePassword+
	dumpPrivateKey+list+addKey+deleteKey != 1)
	Usage(progName);

    if (addKey && (!inFile || !nickname))
	Usage (progName);
    
    if ((list || changePassword) && nickname)
	Usage(progName);

    if ((generateKey || dumpPublicKey || dumpPrivateKey) && !nickname)
	Usage(progName);

    if (deleteKey && !nickname)
	Usage (progName);    
    


    /* Call the libsec initialization routines */
    PR_Init("key", 1, 1, 0);
    SEC_Init();
    RNG_SystemInfoForRNG();

    /* XXX Try to use these files for RNG seed.  If they're not there (i.e.
     * installed somewhere else) oh well. */
    RNG_FileForRNG("/usr/ns-home/https-443/config/magnus.conf");
    RNG_FileForRNG("/usr/ns-home/https-443/config/obj.conf");
    
    if (generateKey) {
	if (keyType == rsaKey) {
	    rv = GenerateRSAKey(nickname, keyBits, publicExponent);
	} else {
	    rv = GenerateDSAKey(nickname, pqgFile);
	}
    } else
    if (dumpPublicKey) {
	rv = DumpPublicKey(nickname, stdout);
    } else
    if (changePassword) {
	rv = ChangePassword();
    } else
    if (dumpPrivateKey) {
	rv = DumpPrivateKey(nickname, stdout);
    } else
    if (list) {
	rv = ListKeys(stdout);
    } else
    if (addKey) {
	rv = AddPrivateKey (nickname, inFile);
    } else
    if (deleteKey) {
	rv = DeletePrivateKey (nickname);
    }

    
    return rv ? -1 : 0;
}
