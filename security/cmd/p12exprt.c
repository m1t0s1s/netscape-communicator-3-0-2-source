#include "secutil.h"
#include <stdio.h>
#include <stdlib.h>
#include "prarena.h"
#include "secitem.h"
#include "pkcs12.h"
#include "secport.h"
#include "secutil.h"
#include "secitem.h"
#include "cert.h"
#include "key.h"
#include "secrng.h"

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

#ifdef US_VERSION
#define MAX_KEY_BITS	1024
#else
#define MAX_KEY_BITS	512
#endif

#define NUM_KEYSTROKES 120
#define RAND_BUF_SIZE 60

#define SEC_KEY_DB_EXISTS 0
#define SEC_CREATE_KEY_DB 1

#define SEC_CERT_DB_EXISTS 0
#define SEC_CREATE_CERT_DB 1

static SECKEYKeyDBHandle *
OpenKeyDB(int createnew)
{
    SECKEYKeyDBHandle *keyHandle;
    struct stat stat_buf;
    int ret;
    char *fName;

    fName = SECU_DatabaseFileName(xpKeyDB);

    /* if we expect database to exist, check for it */
    if (!createnew) {
	ret = stat(fName, &stat_buf);
	if (ret < 0) {
	    if (errno == ENOENT) {
		/* no key.db */
		SECU_PrintError(progName, "unable to locate database");
		return NULL;
	    } else {
		/* stat error */
		SECU_PrintError(progName, "stat: ", strerror(errno));
		return NULL;
	    }
	}
    }

    keyHandle = SECKEY_OpenKeyDB(fName);
    if (keyHandle == NULL) {
        SECU_PrintError(progName, "could not open key database %s", fName);
	return NULL;
    }

    return(keyHandle);
}

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
      rv = CERT_OpenCertDB(certHandle, NULL, FALSE);
    }
    else {
      SECU_PrintError(progName, "Using Cert DB file: %s",dbFilename);
      rv = CERT_OpenCertDB(certHandle, dbFilename, FALSE);
    }

    if (rv) {
	SECU_PrintError(progName, "could not open certificate database");
	if (certHandle) free (certHandle);  /* we don't want to leave anything behind... */
	return NULL;
    }

    return certHandle;
}

void init(void)
{
    /* Call the libsec initialization routines */                               
    PR_Init("key", 1, 1, 0); 
    SEC_Init();
    RNG_SystemInfoForRNG(); 

    /* XXX Try to use these files for RNG seed.  If
	 * they're not there (i.e. 
	 *      * installed somewhere else) oh well. */ 
	RNG_FileForRNG("/usr/ns-home/https-443/config/magnus.conf"); 
	RNG_FileForRNG("/usr/ns-home/https-443/config/obj.conf"); 
}

int main(int argc, char **argv)
{
    char **nicks;
    FILE *out_file;
    SECItem *dest;
    int i;
    CERTCertDBHandle *certdb;
    SECKEYKeyDBHandle *keydb;

    if(argc < 3)
    {
    	fprintf(stdout, "Usage -- %s <out_file> <nickname 1> ... <nickname n>",
    	    progName);
    	exit(-1);
    }

    init();

    progName = argv[0];	
	
    certdb = OpenCertDB(0);
    keydb = OpenKeyDB(0);

    nicks = PORT_ZAlloc(argc-1);
    for(i = 0; i < argc-1; i++)
	nicks[i] = argv[i+2];
	
    dest = SEC_PKCS12GetPFX(nicks, keydb, SECU_GetPassword, NULL,
	PR_TRUE, SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4,
	SECU_GetPBEPassword, NULL, certdb, 
	SEC_OID_PKCS12_OFFLINE_TRANSPORT_MODE,
	SEC_OID_PKCS12_PBE_WITH_SHA1_AND_40_BIT_RC4);
    if(dest != NULL)
    {
	out_file = fopen(argv[1], "wb");
	for(i = 0; i < dest->len; i++)
	    fwrite(&dest->data[i], 1, 1, out_file);
	fclose(out_file);
    }
}
