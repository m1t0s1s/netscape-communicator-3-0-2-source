/*
** certutil.c
**
** utility for managing certificates and the cert database
**
*/
/* test only */

#include "cert.h"
#include "secutil.h"

#define SEC_CERT_DB_EXISTS 0
#define SEC_CREATE_CERT_DB 1

static char *progName;

static CERTCertDBHandle
*OpenCertDB(int createNew)
  /* NOTE: This routine has been modified to allow the libsec/pcertdb.c  routines to automatically
  ** find and convert the old cert database into the new v3.0 format (cert db version 5).
  */
{
    CERTCertDBHandle *certHandle;
    SECStatus rv;
    struct stat stat_buf;
    char *conf_dir, *dbFilename;
    int ret;

    /* Allocate a handle to fill with CERT_OpenCertDB below */
    certHandle = (CERTCertDBHandle *)PORT_ZAlloc(sizeof(CERTCertDBHandle));
    if (!certHandle) {
	SECU_PrintError(progName, "unable to get database handle");
	return NULL;
    }

    dbFilename = SECU_DatabaseFileName(xpCertDB); /* This is the latest name, defined in Secutil.c */

    ret = stat(dbFilename, &stat_buf);

    if (ret == -1) {
      SECU_PrintError(progName, "Did not find Cert DB file: %s",dbFilename);
      SECU_PrintError(progName, "Will attempt to update any existing Cert.db file.","");
      rv = CERT_OpenCertDBFilename(certHandle, NULL, FALSE);
    }
    else {
      SECU_PrintError(progName, "Using Cert DB file: %s",dbFilename);
      rv = CERT_OpenCertDBFilename(certHandle, dbFilename, FALSE);
    }

    if (rv) {
	SECU_PrintError(progName, "could not open certificate database");
	if (certHandle) free (certHandle);  /* we don't want to leave anything behind... */
	return NULL;
    }

    return certHandle;
}
static CERTSignedCrl *FindCRL
   (CERTCertDBHandle *certHandle, char *name, int type)
{
    CERTSignedCrl *crl = NULL;    
    CERTCertificate *cert = NULL;


    cert = CERT_FindCertByNickname(certHandle, name);
    if (!cert) {
	SECU_PrintError(progName, "could not find certificate named %s", name);
	return ((CERTSignedCrl *)NULL);
    }
	
    crl = SEC_FindCrlByKey(certHandle, &cert->derSubject, type);
    if (crl ==NULL) 
	SECU_PrintError
		(progName, "could not find %s's CRL", name);
    CERT_DestroyCertificate (cert);
    return (crl);
}

static void DisplayCRL (CERTCertDBHandle *certHandle, char *nickName, int crlType)
{
    CERTCertificate *cert = NULL;
    CERTSignedCrl *crl = NULL;

    crl = FindCRL (certHandle, nickName, crlType);
	
    if (crl) {
	SECU_PrintCRLInfo (stdout, &crl->crl, "CRL Info:\n", 0);
	CERT_DestroyCrl (crl);
    }
}

static void ListCRLNames (CERTCertDBHandle *certHandle, int crlType)
{
    CERTCrlHeadNode *crlList = NULL;
    CERTCrlNode *crlNode = NULL;
    CERTName *name = NULL;
    PRArenaPool *arena = NULL;
    SECStatus rv;
    void *mark;

    do {
	arena = PORT_NewArena (SEC_ASN1_DEFAULT_ARENA_SIZE);
	if (arena == NULL) {
	    fprintf(stderr, "%s: fail to allocate memory\n", progName);
	    break;
	}
	
	name = PORT_ArenaZAlloc (arena, sizeof(*name));
	if (name == NULL) {
	    fprintf(stderr, "%s: fail to allocate memory\n", progName);
	    break;
	}
	name->arena = arena;
	    
	rv = SEC_LookupCrls (certHandle, &crlList, crlType);
	if (rv != SECSuccess) {
	    fprintf(stderr, "%s: fail to look up CRLs (%s)\n", progName,
	    SECU_ErrorString(PORT_GetError()));
	    break;
	}
	
	/* just in case */
	if (!crlList)
	    break;

	crlNode  = crlList->first;

        fprintf (stdout, "\n");
	fprintf (stdout, "\n%-40s %-5s\n\n", "CRL names", "CRL Type");
	while (crlNode) {
	    mark = PORT_ArenaMark (arena); 	    
	    rv = SEC_ASN1DecodeItem
		   (arena, name, CERT_NameTemplate, &(crlNode->crl->crl.derName));
	    if (!name){
		fprintf(stderr, "%s: fail to get the CRL issuer name\n", progName,
		SECU_ErrorString(PORT_GetError()));
		break;
	    }
		
	    fprintf (stdout, "\n%-40s %-5s\n", CERT_NameToAscii(name), "CRL");
	    crlNode = crlNode->next;
	    PORT_ArenaRelease (arena, mark);
	} 
	
    } while (0);
    if (crlList)
	PORT_FreeArena (crlList->arena, PR_FALSE);
    PORT_FreeArena (arena, PR_FALSE);
}

static void ListCRL (CERTCertDBHandle *certHandle, char *nickName, int crlType)
{
    if (nickName == NULL)
	ListCRLNames (certHandle, crlType);
    else
	DisplayCRL (certHandle, nickName, crlType);
}



static SECStatus DeleteCRL (CERTCertDBHandle *certHandle, char *name, int type)
{
    CERTSignedCrl *crl = NULL;    
    SECStatus rv = SECFailure;

    crl = FindCRL (certHandle, name, type);
    if (!crl) {
	SECU_PrintError
		(progName, "could not find the issuer %s's CRL", name);
	return SECFailure;
    }
    rv = SEC_DeletePermCRL (crl);
    if (rv != SECSuccess) {
	SECU_PrintError
		(progName, "fail to delete the issuer %s's CRL from the perm dbase (reason: %s)",
		 name, SECU_ErrorString(PORT_GetError()));
	return SECFailure;
    }

    rv = SEC_DeleteTempCrl (crl);
    if (rv != SECSuccess) {
	SECU_PrintError
		(progName, "fail to delete the issuer %s's CRL from the temp dbase (reason: %s)",
		 name, SECU_ErrorString(PORT_GetError()));
	return SECFailure;
    }
    return (rv);
}

SECStatus ImportCRL (CERTCertDBHandle *certHandle, char *url, int type, FILE *inFile)
{
    CERTCertificate *cert = NULL;
    CERTSignedCrl *crl = NULL;
    SECItem crlDER;
    int rv;

    crlDER.data = NULL;


    /* Read in the entire file specified with the -f argument */
    rv = SECU_DER_Read(&crlDER, inFile);
    if (rv != SECSuccess) {
	SECU_PrintError(progName, "unable to read input file");
	return (SECFailure);
    }
    
    crl = CERT_ImportCRL (certHandle, &crlDER, url, type, NULL);
    if (!crl) {
	char *errString;
	
	errString = SECU_ErrorString(PORT_GetError());
	if (PORT_Strlen (errString) == 0)
	    SECU_PrintError
		    (progName, "CRL is not import (error: input CRL is not up to date.)");
	else    
	    SECU_PrintError
		    (progName, "unable to import CRL");
    }
    PORT_Free (crlDER.data);
    CERT_DestroyCrl (crl);
    return (rv);
}
	    

static void Usage(char *progName)
{
    fprintf(stderr,
	    "Usage:  %s -L [-n nickname[ [-d keydir] [-t crlType]\n"
	    "        %s -D -n nickname [-d keydir]\n"
	    "        %s -I -i crl -t crlType [-u url] [-d keydir]\n",
	    progName, progName, progName);

    fprintf (stderr, "%-15s List CRL\n", "-L");
    fprintf(stderr, "%-20s Specify the nickname of the CA certificate\n",
	    "-n nickname");
    fprintf(stderr, "%-20s Key database directory (default is ~/.netscape)\n",
	    "-d keydir");
   
    fprintf (stderr, "%-15s Delete a CRL from the cert dbase\n", "-D");    
    fprintf(stderr, "%-20s Specify the nickname for the CA certificate\n",
	    "-n nickname");
    fprintf(stderr, "%-20s Specify the crl type.\n", "-t crlType");

    fprintf (stderr, "%-15s Import a CRL to the cert dbase\n", "-I");    
    fprintf(stderr, "%-20s Specify the file which contains the CRL to import\n",
	    "-i crl");
    fprintf(stderr, "%-20s Specify the url.\n", "-u url");
    fprintf(stderr, "%-20s Specify the crl type.\n", "-t crlType");

    fprintf(stderr, "%-20s CRL Types (default is SEC_CRL_TYPE):\n", " ");
    fprintf(stderr, "%-20s \t 0 - SEC_KRL_TYPE\n", " ");
    fprintf(stderr, "%-20s \t 1 - SEC_CRL_TYPE\n", " ");        

    exit(-1);
}

typedef struct CommandContext {
    int tokenCount;
    int index;
}CommandContext;

int GetNextOpt (int argc, char **argv, char *commands, char **optarg)
{
    static CommandContext context;
    char cletter;
    static int init= 0;
    int i, len;
    PRBool error;

    if (init == 0) {
	PORT_Memset (&context, 0, sizeof (context));
	context.tokenCount = 1;
	init =1;
    }
    if (context.tokenCount == argc)
	return (-1);
    
    if (argv[context.tokenCount][0] != '-')
	return ('?');

    cletter = argv[context.tokenCount][1];
    error = PR_FALSE;
    *optarg = NULL;
    len = PORT_Strlen (commands);

    for (i = 0; i < len; ++i) {
	if (cletter == commands[i]) {
	    if (i+1 <len) {

		/* expecting parameter for input */
		if (commands[i+1] ==':') {
		    if (argc - 1 - context.tokenCount > 0) {
			if (argv[context.tokenCount + 1][0] == '-') {
			    error = PR_TRUE;
			    break;
                 	}
			*optarg = argv[context.tokenCount + 1 ];
			++context.tokenCount;
		    }
		    else {
			error = PR_TRUE;
			break;
		    }
		    break;
		}
		break;
	    }
	    break;
	}
    }
    if (error || i == len)
	return ('?');

    ++context.tokenCount;
    return (cletter);
    
}
#define WIN 1

int main(int argc, char **argv)
{
    SECItem privKeyDER;
    CERTCertDBHandle *certHandle;
    FILE *certFile;
    FILE *inFile;
    int listCRL;
    int addKey;
    int importCRL;
    int opt;
    int deleteCRL;
    int rv;
    char *nickName;
    char *progName;
    char *url;
    char *optarg;
    int crlType;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    rv = 0;
    deleteCRL = importCRL = listCRL = 0;
    certFile = inFile = NULL;
    nickName = url = NULL;
    privKeyDER.data = NULL;
    certHandle = NULL;
    crlType = SEC_CRL_TYPE;
    /*
     * Parse command line arguments
     */
#ifdef WIN
    while ((opt = GetNextOpt (argc, argv, "IALd:i:Dn:Ct:u:", &optarg)) != -1) {
#else
    while ((opt = getopt(argc, argv, "IALd:i:Dn:Ct:u:")) != -1) {
#endif	
	switch (opt) {
	  case '?':
	    Usage(progName);
	    break;

	  case 'C':
	      listCRL = 1;
	      break;

	  case 'D':
	      deleteCRL = 1;
	      break;

	  case 'I':
	      importCRL = 1;
	      break;
	           
	  case 'L':
	      listCRL = 1;
	      break;
	           
	  case 'd':
	    SECU_ConfigDirectory(optarg);
	    break;

	  case 'i':
	    inFile = fopen(optarg, "r");
	    if (!inFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for reading\n",
			progName, optarg);
		return -1;
	    }
	    break;
	    
	  case 'n':
	    nickName = strdup(optarg);
	    break;
	    
	  case 'u':
	    url = strdup(optarg);
	    break;

	  case 't': {
	    char *type;
	    
	    type = strdup(optarg);
	    crlType = atoi (type);
	    if (crlType != SEC_CRL_TYPE && crlType != SEC_KRL_TYPE) {
		fprintf(stderr, "%s: invalid crl type\n", progName);
		return -1;
	    }
	    break;
          }
	}
    }

    if (deleteCRL && !nickName) Usage (progName);
    if (!(listCRL || deleteCRL || importCRL)) Usage (progName);
    if (importCRL && !inFile) Usage (progName);
    
    PR_Init("crlutil", 1, 1, 0);
    SECU_PKCS11Init();
    SEC_Init();

    certHandle = OpenCertDB(SEC_CREATE_CERT_DB);
    if (certHandle == NULL) {
	SECU_PrintError(progName, "unable to open the cert db");	    	
	return (-1);
    }

    /* Read in the private key info */
    if (deleteCRL) 
	DeleteCRL (certHandle, nickName, crlType);
    else if (listCRL)
	ListCRL (certHandle, nickName, crlType);
    else if (importCRL) 
	rv = ImportCRL (certHandle, url, crlType, inFile);
    
    return (rv);
}
