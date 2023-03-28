/*
** certutil.c
**
** utility for managing certificates and the cert database
**
*/

#include "secutil.h"
#include "pk11func.h"

#include "prtypes.h"
#include "prtime.h"
#include "prlong.h"

extern void SEC_Init(void);

#if defined(__sun) && !defined(SVR4)
extern int fwrite(char *, size_t, size_t, FILE*);
extern int getopt(int, char**, char*);
extern int fprintf(FILE *, char *, ...);
extern char *optarg;
#endif

#if defined(LINUX)
#include <getopt.h>
#endif

#define SEC_CERT_DB_EXISTS 0
#define SEC_CREATE_CERT_DB 1

#define GEN_BREAK(e) rv=e; break;

static char *progName;

static CERTGeneralName **GetGeneralName (PRArenaPool *arena)
{
    CERTGeneralName **namesList = NULL;
    CERTGeneralName *current;
    SECStatus rv = SECSuccess;
    int count = 0, intValue;
    char buffer[512];
    void *mark;

    PORT_Assert (arena);
    mark = PORT_ArenaMark (arena);
    do {
	fflush (stdin);
	puts ("\nSelect one of the following general name type: \n");
	puts ("\t1 - instance of other name\n\t2 - rfc822Name\n\t3 - dnsName\n");
	puts ("\t4 - x400Address\n\t5 - directoryName\n\t6 - ediPartyName\n");
	puts ("\t7 - uniformResourceidentifier\n\t8 - ipAddress\n\t9 - registerID\n");
	puts ("\tOther - omit\n\t\tChoice:");
	scanf ("%d", &intValue);
	if (intValue < certOtherName || intValue > certRegisterID) {
	    /* add NULL in the last entry to mark end of data */
	    namesList = PORT_ArenaGrow (arena, namesList,
					sizeof (*namesList) * count,
					sizeof (*namesList) * (count + 1));
	    if (namesList == NULL) {
		GEN_BREAK (SECFailure);
	    }	
	    namesList[count] = NULL;
	    break;
	}
	
	current = PORT_ArenaZAlloc (arena, sizeof (*current));
	if (current == NULL){
	    GEN_BREAK (SECFailure);
	}

	current->type = intValue;
	puts ("\nEnter data:");
	fflush (stdin);
	fflush (stdout);
	gets (buffer);
	switch (current->type) {
	    case certURI:
		current->name.uri.data = PORT_ArenaAlloc (arena, strlen (buffer));
		if (current->name.uri.data == NULL) {
		    GEN_BREAK (SECFailure);
		}
		PORT_Memcpy
		  (current->name.uri.data, buffer, current->name.uri.len = strlen(buffer));
		break;

	    case certDNSName:
	    case certEDIPartyName:
	    case certIPAddress:
	    case certOtherName:
	    case certRegisterID:
	    case certRFC822Name:
	    case certX400Address: {
		SECItem temp;

		current->name.other.data = PORT_ArenaAlloc (arena, strlen (buffer) + 2);
		if (current->name.other.data == NULL) {
		    GEN_BREAK (SECFailure);
		}

		PORT_Memcpy (current->name.other.data + 2, buffer, strlen (buffer));
		/* This may not be accurate for all cases.  For now, use this tag type */
		current->name.other.data[0] = (char)(((current->type - 1) & 0x1f)| 0x80);
		current->name.other.data[1] = (char)strlen (buffer);
		current->name.other.len = strlen (buffer) + 2;
		break;
	    }

	    case certDirectoryName: {
		CERTName *directoryName = NULL;

		directoryName = CERT_AsciiToName (buffer);
		if (!directoryName) {
		    fprintf(stderr, "certutil: improperly formatted name: \"%s\"\n", buffer);
		    break;
		}
			    
		rv = CERT_CopyName (arena, &current->name.directoryName, directoryName);
		CERT_DestroyName (directoryName);
		
		break;
	    }
	}
	if (rv != SECSuccess)
	    break;
	namesList = PORT_ArenaGrow (arena, namesList,
				     sizeof (*namesList) * count,
				     sizeof (*namesList) * (count + 1));
	if (namesList == NULL) {
	    GEN_BREAK (SECFailure);
	}
	namesList[count] = current;
	++count;
    }while (1);

    if (rv != SECSuccess) {
	PORT_SetError (rv);
	PORT_ArenaRelease (arena, mark);
	namesList = NULL;
    }
    return (namesList);
}

static SECStatus GetString (PRArenaPool *arena, char *prompt, SECItem *value)
{
    char buffer[251];

    value->data = NULL;
    value->len = 0;
    
    puts (prompt);
    fflush (stdin);
    gets (buffer);
    if (strlen (buffer) > 0) {
	value->data = PORT_ArenaAlloc (arena, strlen (buffer));
	if (value->data == NULL) {
	    PORT_SetError (SEC_ERROR_NO_MEMORY);
	    return (SECFailure);
	}
	PORT_Memcpy (value->data, buffer, value->len = strlen(buffer));
    }
    return (SECSuccess);
}

static CERTCertificateRequest *GetCertRequest (FILE *inFile)
{
    CERTCertificateRequest *certReq = NULL;
    CERTSignedData signedData;
    PRArenaPool *arena = NULL;
    SECItem reqDER;
    SECStatus rv;

    reqDER.data = NULL;
    do {
	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if (arena == NULL) {
	    GEN_BREAK (SEC_ERROR_NO_MEMORY);
	}
	
 	rv = SECU_DER_Read (&reqDER, inFile);
	if (rv) 
	    break;
        certReq = (CERTCertificateRequest*) PORT_ArenaZAlloc
		  (arena, sizeof(CERTCertificateRequest));
        if (!certReq) 
	    break;
	certReq->arena = arena;

	/* Since cert request is a signed data, must decode to get the inner
	   data
	 */
	PORT_Memset(&signedData, 0, sizeof(signedData));
	rv = DER_Decode(arena, &signedData, CERTSignedDataTemplate, &reqDER);
	if (rv)
	    break;
	
        rv = DER_Decode(arena, certReq, CERTCertificateRequestTemplate, &signedData.data);
   } while (0);

   if (rv) {
       fprintf(stderr, "%s: unable to create cert request (%s)\n", progName,
	       SECU_ErrorString(PORT_GetError()));
       return (NULL);
   }
   return (certReq);
}

static int GetYesNo (char *prompt) 
{
    char charValue;

    fflush (stdin);
    puts (prompt);
    scanf ("%c", &charValue);
    if (charValue != 'y' && charValue != 'Y')
	return (0);
    return (1);
}

static CERTCertDBHandle
*OpenCertDB(void)
  /* NOTE: This routine has been modified to allow the libsec/pcertdb.c
   * routines to automatically find and convert the old cert database
   * into the new v3.0 format (cert db version 5).
   */
{
    CERTCertDBHandle *certHandle;
    SECStatus rv;
    struct stat stat_buf;
    int ret;

    /* Allocate a handle to fill with CERT_OpenCertDB below */
    certHandle = (CERTCertDBHandle *)PORT_ZAlloc(sizeof(CERTCertDBHandle));
    if (!certHandle) {
	SECU_PrintError(progName, "unable to get database handle");
	return NULL;
    }

    rv = CERT_OpenCertDB(certHandle, FALSE, SECU_CertDBNameCallback, NULL);

    if (rv) {
	SECU_PrintError(progName, "could not open certificate database");
	if (certHandle) free (certHandle);  /* we don't want to leave anything behind... */
	return NULL;
    } else {
	CERT_SetDefaultCertDB(certHandle);
    }

    return certHandle;
}

static SECStatus
AddCert(CERTCertDBHandle *handle, char *name, char *trusts, FILE *inFile,
	int ascii, PRBool emailcert)
{
    CERTCertTrust *trust = NULL;
    CERTCertificate *cert = NULL, *tempCert = NULL;
    SECItem certDER;
    SECStatus rv;

    certDER.data = NULL;
    do {
	/* Read in the entire file specified with the -f argument */
	if (ascii) {
	    certDER.data = SECU_FileToString(inFile);
	    if (!certDER.data) {
		SECU_PrintError(progName, "unable to read input file");
		GEN_BREAK(SECFailure);
	    }
	    certDER.len = PORT_Strlen (certDER.data);
	}
	else {
	    rv = SECU_DER_Read(&certDER, inFile);
	    if (rv != SECSuccess) {
		SECU_PrintError(progName, "unable to read input file");
		rv = -1;
		break;
	    }
	}	

	/* Read in an ASCII cert and return a CERTCertificate */
	cert = CERT_DecodeCertFromPackage(certDER.data, certDER.len);
	if (!cert) {
	    SECU_PrintError(progName, "could not obtain certificate from file"); 
	    GEN_BREAK(SECFailure);
	}

	/* Create a cert trust to pass to SEC_AddPermCertificate */
	trust = (CERTCertTrust *)PORT_ZAlloc(sizeof(CERTCertTrust));
	if (!trust) {
	    SECU_PrintError(progName, "unable to allocate cert trust");
	    GEN_BREAK(SECFailure);
	}

	rv = CERT_DecodeTrustString(trust, trusts);
	if (rv) {
	    GEN_BREAK(SECFailure);
	}

	if ( emailcert ) {
	    CERTIssuerAndSN *issuerAndSN;
	    SECItem *derCert;
	    
	    issuerAndSN = CERT_GetCertIssuerAndSN(NULL, cert);
	    derCert = &cert->derCert;
	    
#ifdef NOTDEF /* take this out for now */
	    tempCert = CERT_ImportCertsFromSMIME(handle, 1, &derCert,
						 issuerAndSN, NULL, NULL,
						 NULL, PR_TRUE);
#else
	    tempCert = NULL;
#endif
	    if (tempCert == NULL) {
		SECU_PrintError(progName,
				"unable to add email cert (%s) to the temp database",
				SECU_ErrorString(PORT_GetError()));
		GEN_BREAK(SECFailure);
	    }

	} else {
	    /* Make sure there isn't a nickname conflict */
	    if (SEC_CertNicknameConflict(name, &cert->derSubject, handle)) {
		SECU_PrintError(progName, "certificate '%s' conflicts", name);
		GEN_BREAK(SECFailure);
	    }

	    /* CERT_ImportCert only collects certificates and returns the
	     * first certficate.  It does not insert these certificates into
	     * the dbase.  For now, just call CERT_NewTempCertificate.
	     * This will result in decoding the der twice.  This have to
	     * be handle properly.
	     */
	    tempCert = CERT_NewTempCertificate(handle, &cert->derCert, NULL,
					       PR_FALSE, PR_TRUE);

	    if (tempCert == NULL) {
		SECU_PrintError(progName,
				"unable to add cert (%s) to the temp database",
				SECU_ErrorString(PORT_GetError()));
		GEN_BREAK(SECFailure);
	    }

	    rv = CERT_AddTempCertToPerm(tempCert, name, trust);
	    if (rv) {
		SECU_PrintError(progName, "could not add certificate to database");
		GEN_BREAK(SECFailure);
	    }
	}
    } while (0);

    CERT_DestroyCertificate (tempCert);
    CERT_DestroyCertificate (cert);
    PORT_Free(trust);
    PORT_Free(certDER.data);

    return rv;
}

static SECStatus CertReq(char *name, CERTName *subject, char *phone, int ascii)
{
    SECKEYKeyDBHandle *keydb;
    SECKEYPrivateKey *privk;
    SECKEYPublicKey *pubk;
    CERTSubjectPublicKeyInfo *spki;
    CERTCertificateRequest *cr;
    SECItem der, result;
    SECStatus rv;
    PRArenaPool *arena;

    /* Open the key.db database */
    keydb = SECU_OpenKeyDB();
    if (keydb == NULL) {
	SECU_PrintError(progName, "could not open key database");
      	return SECFailure;
    }

    privk = SECU_FindPrivateKeyFromNickname(name);

    /* Get public key from private key */
    pubk = SECKEY_ConvertToPublicKey(privk);
    if (!pubk) {
	SECU_PrintError(progName, "unable to extract public key");
	return SECFailure;
    }

    /* Create info about public key */
    spki = SECKEY_CreateSubjectPublicKeyInfo(pubk);
    if (!spki) {
	SECU_PrintError(progName, "unable to create subject public key");
	return SECFailure;
    }

    /* Generate certificate request */
    cr = CERT_CreateCertificateRequest(subject, spki, 0);
    if (!cr) {
	SECU_PrintError(progName, "unable to make certificate request");
	return SECFailure;
    }

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( !arena ) {
	SECU_PrintError(progName, "out of memory");
	return SECFailure;
    }
    
    /* Der encode the request */
    rv = DER_Encode(arena, &der, CERTCertificateRequestTemplate, cr);
    if (rv) {
	SECU_PrintError(progName, "der encoding of request failed");
	return SECFailure;
    }

    /* Sign the request */
    rv = SEC_DerSignData(arena, &result, der.data, der.len, privk,
			 SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION);

    if (rv) {
	SECU_PrintError(progName, "signing of data failed");
	return SECFailure;
    }

    /* Encode request in specified format */
    if (ascii) {
	unsigned char *obuf;
	char *name, *email, *org, *state, *country;
	BTOAContext *cvt;
	unsigned total, part1, part2, len;
	SECItem *it;

	it = &result;

	len = BTOA_GetLength(it->len);
	obuf = (unsigned char*) PORT_Alloc(len);
	cvt = BTOA_NewContext();
	if (!obuf || !cvt) {
	    SECU_PrintError(progName, "out of memory");
	    return SECFailure;
	}
	BTOA_Begin(cvt);
	BTOA_Update(cvt, obuf, &part1, len, it->data, it->len);
	BTOA_End(cvt, obuf + part1, &part2, len - part1);
	total = part1 + part2;

	name = CERT_GetCommonName(subject);
	if (!name) {
	    fprintf(stdout, "You must specify a common name\n");
	    return SECFailure;
	}

	if (!phone)
	    phone = strdup("(not specified)");

	email = CERT_GetCertEmailAddress(subject);
	if (!email)
	    email = strdup("(not specified)");

	org = CERT_GetOrgName(subject);
	if (!org)
	    org = strdup("(not specified)");

	state = CERT_GetStateName(subject);
	if (!state)
	    state = strdup("(not specified)");

	country = CERT_GetCountryName(subject);
	if (!country)
	    country = strdup("(not specified)");

	fprintf(stdout, "\nCertificate request generated by Netscape certutil\n");
	fprintf(stdout, "Phone: %s\n\n", phone);
	fprintf(stdout, "Common Name: %s\n", name);
	fprintf(stdout, "Email: %s\n", email);
	fprintf(stdout, "Organization: %s\n", org);
	fprintf(stdout, "State: %s\n", state);
	fprintf(stdout, "Country: %s\n\n", country);

	fprintf(stdout, "%s\n", NS_CERTREQ_HEADER);
	rv = fwrite(obuf, 1, total, stdout);
	if (rv != total) {
	    SECU_PrintSystemError(progName, "write error");
	    return SECFailure;
	}
	fprintf(stdout, "%s\n", NS_CERTREQ_TRAILER);
    } else {
        rv = fwrite(result.data, 1, result.len, stdout);
	rv = (rv != (int)result.len) ? -1 : 0;
	if (rv) {
	    SECU_PrintSystemError(progName, "write error");
	    return SECFailure;
	}
    }
    return SECSuccess;
}

static SECStatus ChangeTrustAttributes(CERTCertDBHandle *handle,
				       char *name, char *trusts)
{
    SECStatus rv;
    CERTCertificate *cert;
    CERTCertTrust *trust;
    
    cert = CERT_FindCertByNicknameOrEmailAddr(handle, name);
    if (!cert) {
	SECU_PrintError(progName, "could not find certificate named %s",
			name);
	return SECFailure;
    }

    trust = (CERTCertTrust *)PORT_ZAlloc(sizeof(CERTCertTrust));
    if (!trust) {
	SECU_PrintError(progName, "unable to allocate cert trust");
	return SECFailure;
    }

    rv = CERT_DecodeTrustString(trust, trusts);
    if (rv) {
	return SECFailure;
    }

    rv = CERT_ChangeCertTrust(handle, cert, trust);
    if (rv) {
	SECU_PrintError(progName, "unable to modify trust attributes");
	return SECFailure;
    }

    return SECSuccess;
}

static SECStatus
printCertCB(CERTCertificate *cert, void *arg)
{
    SECStatus rv;
    SECItem data;
    
    data.data = cert->derCert.data;
    data.len = cert->derCert.len;

    rv = SECU_PrintSignedData(stdout, &data, "Certificate", 0,
			      SECU_PrintCertificate);
    if (rv) {
	SECU_PrintError(progName, "problem printing certificate");
	return(SECFailure);
    }
    SECU_PrintTrustFlags(stdout, &cert->dbEntry->trust,
			 "Certificate Trust Flags", 1);
    return(SECSuccess);
}

static SECStatus
ListCerts(CERTCertDBHandle *handle, char *name, int raw, int ascii)
{
    SECStatus rv;
    CERTCertificate *cert;
    SECItem data;
    int ret;

    if (name == NULL) {
	rv = SECU_PrintCertificateNames(handle, stdout);
	if (rv) {
	    SECU_PrintError(progName, "problem printing certificate nicknames");
	    return SECFailure;
	}
    } else {
	if ( raw || ascii ) {
	    cert = CERT_FindCertByNicknameOrEmailAddr(handle, name);
	    if (!cert) {
		SECU_PrintError(progName, "could not find certificate named %s", name);
		return SECFailure;
	    }

	    data.data = cert->derCert.data;
	    data.len = cert->derCert.len;

	    if (ascii) {
		fprintf(stdout, "%s\n%s\n%s\n", NS_CERT_HEADER, 
			BTOA_DataToAscii(data.data, data.len), NS_CERT_TRAILER);
	    } else if (raw) {
		ret = fwrite(data.data, data.len, 1, stdout);
		if (ret != 1) {
		    SECU_PrintSystemError(progName, "error writing raw cert");
		    return SECFailure;
		}
	    }
	} else {
	    rv = CERT_TraversePermCertsForNickname(handle, name, printCertCB,
						   NULL);
	}
    }
	
    return SECSuccess;
}

static SECStatus DeleteCert(CERTCertDBHandle *handle, char *name)
{
    SECStatus rv;
    CERTCertificate *cert;

    cert = CERT_FindCertByNicknameOrEmailAddr(handle, name);
    if (!cert) {
	SECU_PrintError(progName, "could not find certificate named %s", name);
	return SECFailure;
    }

    rv = SEC_DeletePermCertificate(cert);
    if (rv) {
	SECU_PrintError(progName, "unable to delete certificate");
	return SECFailure;
    }

    return SECSuccess;
}

static SECStatus
ValidateCert(CERTCertDBHandle *handle, char *name, char *date,
	     char *certUsage, char *checkSignature, PRBool logit)
{
    SECStatus rv;
    CERTCertificate *cert;
    int64 timeBoundary;
    SECCertUsage usage;
    PRBool chkSign;
    CERTVerifyLog reallog;
    CERTVerifyLog *log = NULL;
    
    switch (*certUsage) {
	case 'C':
	    usage = certUsageSSLClient;
	    break;
	case 'V':
	    usage = certUsageSSLServer;
	    break;
	case 'S':
	    usage = certUsageEmailSigner;
	    break;
	case 'R':
	    usage = certUsageEmailRecipient;
	    break;
	default:
	    PORT_SetError (SEC_ERROR_INVALID_ARGS);
	    return (SECFailure);
    }
    chkSign = *checkSignature == 'Y' ? PR_TRUE : PR_FALSE;
    do {
	cert = CERT_FindCertByNicknameOrEmailAddr(handle, name);
	if (!cert) {
	    SECU_PrintError(progName, "could not find certificate named %s", name);
	    GEN_BREAK (SECFailure)
	}

	rv = DER_AsciiToTime(&timeBoundary, date);
	if (rv) {
	    SECU_PrintError(progName, "invalid input date");
	    GEN_BREAK (SECFailure)
	}

	if ( logit ) {
	    log = &reallog;
	    
	    log->count = 0;
	    log->head = NULL;
	    log->tail = NULL;
	    log->arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	    if ( log->arena == NULL ) {
		SECU_PrintError(progName, "out of memory");
		GEN_BREAK (SECFailure)
	    }
	}
 
	/*rv = CERT_CheckCertValidTimes(cert, timeBoundary); */
	rv = CERT_VerifyCert(handle, cert, chkSign, usage,
			     timeBoundary, NULL, log);
	if ( log ) {
	    if ( log->head == NULL ) {
		SECU_PrintError(progName, "certificate is valid");
		GEN_BREAK (SECSuccess)
	    } else {
		char *name;
		CERTVerifyLogNode *node;
		
		node = log->head;
		while ( node ) {
		    if ( node->cert->nickname != NULL ) {
			name = node->cert->nickname;
		    } else {
			name = node->cert->subjectName;
		    }
		    fprintf(stderr, "%s : %s\n", name, SECU_ErrorString(node->error));
		    CERT_DestroyCertificate(node->cert);
		    node = node->next;
		}
	    }
	} else {
	    if (rv == SECSuccess) {
		SECU_PrintError(progName, "certificate is valid");
		GEN_BREAK (SECSuccess)
	    }
	    SECU_PrintError(progName, "certificate is invalid");
	    GEN_BREAK (SECFailure)
	}
    } while (0);
    free (name);
    free (date);
    return (rv);
}

static void Usage(char *progName)
{
    fprintf(stderr, "Type %s -H for more detailed descriptions\n", progName);
    fprintf(stderr,
	    "Usage:  %s -R -n key-name -s subj [-d keydir] [-p phone] [-a]\n",
	    progName);
    fprintf(stderr,
	    "        %s -N [-d certdir]\n", progName);
    fprintf(stderr,
	    "        %s -A -n cert-name -t trustargs [-d certdir] [-i input]\n", progName);
    fprintf(stderr,
	    "        %s -D -n cert-name [-d certdir]\n", progName);
    fprintf(stderr,
	    "        %s -L [-n cert-name] [-d certdir] [-r] [-a]\n", progName);
    fprintf(stderr, 
	    "        %s -M -n cert-name -t trustargs [-d certdir]\n",
	    progName);
    fprintf(stderr, 
	    "        %s -V -n cert-name -b time -u usage -e checksignature [-d certdir]\n",
	    progName);
    fprintf(stderr, 
	    "        %s -C -n issuer-name -i cert-request -o output-cert -x [-m serial-number] [-w warp-months] [-d certdir]\n"
	    "                 [-1 keyusage extension] \n"
	    "                 [-2 basicConstraint extension] \n"
	    "                 [-3 authKeyID extension]\n"
	    "                 [-4 crlDistributionPoints extension]\n",
	    progName);
    
    exit(-1);
}

static void LongUsage(char *progName)
{
    fprintf(stderr, "%-15s Generate a certificate request (stdout)\n",
	    "-R");
    fprintf(stderr, "%-20s Specify the nickname of the key to use\n",
	    "   -n key-name");
    fprintf(stderr, "%-20s Specify the subject name (using RFC1485)\n",
	    "   -s subject");
    fprintf(stderr,"%-20s Key database directory (default is ~/.netscape)\n",
	    "   -d keydir");
    fprintf(stderr, "%-20s Specify the contact phone number (\"123-456-7890\")\n",
	    "   -p phone");
    fprintf(stderr,"%-20s Output the certificate request in ascii (RFC1113)\n",
	    "   -a");
    fprintf(stderr, "\n");
    fprintf(stderr, "%-15s Create a new certificate database\n",
	    "-N");
    fprintf(stderr, "%-20s Cert database directory (default is ~/.netscape)\n",
	    "   -d certdir");
    fprintf(stderr, "\n");
    fprintf(stderr, "%-15s Add a certificate to the database (create if needed)\n",
	    "-A");
    fprintf(stderr, "%-20s Specify the nickname of the certificate to add\n",
	    "   -n cert-name");
    fprintf(stderr, "%-20s Set the certificate trust attributes:\n",
	    "   -t trustargs");
    fprintf(stderr, "%-25s p \t valid peer\n", "");
    fprintf(stderr, "%-25s P \t trusted peer (implies p)\n", "");
    fprintf(stderr, "%-25s c \t valid CA\n", "");
    fprintf(stderr, "%-25s C \t trusted CA to issue client certs (implies c)\n", "");
    fprintf(stderr, "%-25s S \t trusted CA to issue server certs (implies c)\n", "");
    fprintf(stderr, "%-25s u \t user cert\n", "");
    fprintf(stderr, "%-25s w \t send warning\n", "");
    fprintf(stderr, "%-20s Cert database directory (default is ~/.netscape)\n",
	    "   -d certdir");
    fprintf(stderr, "%-20s Specify the ASCII certificate file (default is stdin)\n",
	    "   -i input");
    fprintf(stderr, "\n");
    fprintf(stderr, "%-15s Delete a certificate from the database\n",
	    "-D");
    fprintf(stderr, "%-20s The nickname of the cert to delete\n",
	    "   -n cert-name");
    fprintf(stderr, "%-20s Cert database directory (default is ~/.netscape)\n",
	    "   -d certdir");
    fprintf(stderr, "\n");
    fprintf(stderr, "%-15s List all certs, or print out a single named cert\n",
	    "-L");
    fprintf(stderr, "%-20s Pretty print named cert (list all if unspecified)\n",
	    "   -n cert-name");
    fprintf(stderr, "%-20s Cert database directory (default is ~/.netscape)\n",
	    "   -d certdir");
    fprintf(stderr, "%-20s For single cert, print binary DER encoding\n",
	    "   -r");
    fprintf(stderr, "%-20s For single cert, print ascii encoding (RFC1113)\n",
	    "   -a");
    fprintf(stderr, "\n");
    fprintf(stderr, "%-15s Modify trust attributes of certificate\n",
	    "-M");
    fprintf(stderr, "%-20s The nickname of the cert to modify\n",
	    "   -n cert-name");
    fprintf(stderr, "%-20s Set the certificate trust attributes (see -A above)\n",
	    "   -t trustargs");
    fprintf(stderr, "%-20s Cert database directory (default is ~/.netscape)\n",
	    "   -d certdir");
    fprintf(stderr, "\n");
    fprintf(stderr, "%-15s Validate a certificate\n",
	    "-V");
    fprintf(stderr, "%-20s The nickname of the cert to modify\n",
	    "   -n cert-name");
    fprintf(stderr, "%-20s Specify the time boundary (\"YYMMDDHHMMSS[[+][-][HH][MM]][Z]\")\n",
	    "   -b time");
    fprintf(stderr, "%-20s Check certificate signature (Y/N)\n",
	    "   -e checksignature");   
    fprintf(stderr, "%-20s Specify certificate usage:\n", "   -u certusage");
    fprintf(stderr, "%-25s C \t SSL Client\n", "");
    fprintf(stderr, "%-25s V \t SSL Server\n", "");
    fprintf(stderr, "%-25s S \t Email signer\n", "");
    fprintf(stderr, "%-25s R \t Email Recipient\n", "");   
    fprintf(stderr, "%-20s Cert database directory (default is ~/.netscape)\n",
	    "   -d certdir");
    fprintf(stderr, "\n");

    fprintf(stderr, "%-15s Create a new certificate\n",
	    "-C");
    fprintf(stderr, "%-20s \tThe nickname of the issuer cert\n",
	    "   -n issuer-name");
    fprintf(stderr, "%-20s \tThe certificate request\n",
	    "   -i cert-request file");
    fprintf(stderr, "%-20s Define an output file to use (default is stdout)\n",
	    "   -o output");
    fprintf(stderr, "%-20s \tSelf Sign\n",
	    "   -x");
    fprintf(stderr, "%-20s \tSerial Number\n",
	    "   -m serial-number");
    fprintf(stderr, "%-20s \tTime Warp\n",
	    "   -w warp-months");
    fprintf(stderr, "%-20s Cert database directory (default is ~/.netscape)\n",
	    "   -d certdir");
    fprintf(stderr, "%-20s \tThe certificate key usage\n",
	    "   -1 create key usage extension");
    fprintf(stderr, "%-20s \tThe certificate basic constraint\n",
	    "   -2 create basicConstraint extension");
    fprintf(stderr, "%-20s \tThe certificate authority key ID\n",
	    "   -3 create authKeyID extension");
    fprintf(stderr, "%-20s \tThe certificate crl distribution point\n",
	    "   -4 create crlDistributionPoints extension");
    fprintf(stderr, "\n");
    
    exit(-1);
}

static CERTCertificate
*MakeV1Cert(CERTCertDBHandle *handle, CERTCertificateRequest *req,
	    char *issuerNickName, PRBool selfsign, int serialNumber,
	    int warpmonths)
{
    CERTCertificate *issuerCert = NULL;
    CERTValidity *validity;
    CERTCertificate *cert = NULL;
#ifndef NSPR20    
    PRTime printableTime;
    int64 now, after;
#else
    PRExplodedTime printableTime;
    PRTime now, after;
#endif    
   
    

    if ( !selfsign ) {
	issuerCert = CERT_FindCertByNicknameOrEmailAddr(handle, issuerNickName);
	if (!issuerCert) {
	    SECU_PrintError(progName, "could not find certificate named %s", issuerNickName);
	    return NULL;
	}
    }

    now = PR_Now();
#ifndef NSPR20
    PR_ExplodeGMTTime (&printableTime, now);
#else    
    PR_ExplodeTime (&printableTime, PR_GMTParameters, now);
#endif
    if ( warpmonths ) {
	printableTime.tm_mon += warpmonths;
#ifndef	NSPR20    
	now = PR_ImplodeTime (&printableTime, 0, 0);
	PR_ExplodeGMTTime (&printableTime, now);
#else
	now = PR_ImplodeTime (printableTime);
	PR_ExplodeTime (&printableTime, PR_GMTParameters, now);
#endif
    }
    
    printableTime.tm_mon += 3;
#ifndef	NSPR20    
    after = PR_ImplodeTime (&printableTime, 0, 0);

#else
    after = PR_ImplodeTime (printableTime);
#endif    

    /* note that the time is now in micro-second unit */
    validity = CERT_CreateValidity (now, after);

    if ( selfsign ) {
	cert = CERT_CreateCertificate
	    (serialNumber,&(req->subject), validity, req);
    } else {
	cert = CERT_CreateCertificate
	    (serialNumber,&(issuerCert->subject), validity, req);
    }
    
    CERT_DestroyValidity(validity);
    if ( issuerCert ) {
	CERT_DestroyCertificate (issuerCert);
    }
    
    return(cert);
}

static SECStatus AddKeyUsage (void *extHandle)
{
    SECStatus rv;
    SECItem bitStringValue;
    char keyUsage = 0x0, buffer[5];
    int value;
    int i;

    while (1) {
	fprintf(stdout, "%-25s 0 - Digital Signature\n", "");
	fprintf(stdout, "%-25s 1 - Non-repudiation\n", "");
	fprintf(stdout, "%-25s 2 - Key encipherment\n", "");
	fprintf(stdout, "%-25s 3 - Data encipherment\n", "");   
	fprintf(stdout, "%-25s 4 - Key agreement\n", "");
	fprintf(stdout, "%-25s 5 - Cert signning key\n", "");   
	fprintf(stdout, "%-25s 6 - CRL signning key\n", "");
	fprintf(stdout, "%-25s Other to finish\n", "");
	gets (buffer);
	value = atoi (buffer);
	if (value < 0 || value > 6)
	    break;
	keyUsage |= (0x80 >> value);
    }

    bitStringValue.data = &keyUsage;
    bitStringValue.len = 1;
    buffer[0] = 'n';
    puts ("Is this a critical extension [y/n]? ");
    gets (buffer);	

    return (CERT_EncodeAndAddBitStrExtension
	    (extHandle, SEC_OID_X509_KEY_USAGE, &bitStringValue,
	     (buffer[0] == 'y' || buffer[0] == 'Y') ? PR_TRUE : PR_FALSE));

}

typedef SECStatus (* EXTEN_VALUE_ENCODER)
		(PRArenaPool *extHandle, void *value, SECItem *encodedValue);

static SECStatus EncodeAndAddExtensionValue
   (PRArenaPool *arena, void *extHandle, void *value, PRBool criticality,
    int extenType, EXTEN_VALUE_ENCODER EncodeValueFn)
{
    SECItem encodedValue;
    SECStatus rv;
	

    encodedValue.data = NULL;
    encodedValue.len = 0;
    do {
	rv = (*EncodeValueFn)(arena, value, &encodedValue);
	if (rv != SECSuccess)
	break;

	rv = CERT_AddExtension
	     (extHandle, extenType, &encodedValue, criticality,PR_TRUE);
    } while (0);
	
    return (rv);
}

static SECStatus AddBasicConstraint (void *extHandle)
{
    CERTBasicConstraints basicConstraint;    
    SECItem encodedValue;
    SECStatus rv;
    char buffer[10];

    encodedValue.data = NULL;
    encodedValue.len = 0;
    do {
	basicConstraint.pathLenConstraint = CERT_UNLIMITED_PATH_CONSTRAINT;
	puts ("Is this a CA certificate [y/n]?");
	fflush (stdin);
	gets (buffer);
	basicConstraint.isCA = (buffer[0] == 'Y' || buffer[0] == 'y') ?
                                PR_TRUE : PR_FALSE;

	puts ("Enter the path length constraint, enter to skip [<0 for unlimited path]:");
	gets (buffer);
	if (PORT_Strlen (buffer) > 0)
	    basicConstraint.pathLenConstraint = atoi (buffer);
	
	rv = CERT_EncodeBasicConstraintValue (NULL, &basicConstraint, &encodedValue);
	if (rv)
	    return (rv);

	buffer[0] = 'n';
	puts ("Is this a critical extension [y/n]? ");
	gets (buffer);	
	rv = CERT_AddExtension
	     (extHandle, SEC_OID_X509_BASIC_CONSTRAINTS,
	      &encodedValue, (buffer[0] == 'y' || buffer[0] == 'Y') ?
              PR_TRUE : PR_FALSE ,PR_TRUE);
    } while (0);
    PORT_Free (encodedValue.data);
    return (rv);
}

static SECItem *
SignCert(CERTCertificate *cert, char *issuerNickName)
{
    SECItem der;
    SECItem *result = NULL;
    SECKEYPrivateKey *caPrivateKey = NULL;    
    SECStatus rv;
    PRArenaPool *arena;
    SECOidTag algID;

    caPrivateKey = SECU_FindPrivateKeyFromNickname(issuerNickName); 
    if (caPrivateKey == NULL) {
	SECU_PrintError(progName, "unable to retrieve key %s", issuerNickName);
	return NULL;
    }
	
    arena = cert->arena;

    switch(caPrivateKey->keyType) {
      case rsaKey:
	algID = SEC_OID_PKCS1_MD5_WITH_RSA_ENCRYPTION;
	break;
      case dsaKey:
	algID = SEC_OID_ANSIX9_DSA_SIGNATURE_WITH_SHA1_DIGEST;
	break;
      default:
	fprintf(stderr, "Unknown key type for issuer.");
	goto done;
	break;
    }

    rv = SECOID_SetAlgorithmID(arena, &cert->signature, algID, 0);
    if (rv != SECSuccess) {
	fprintf(stderr, "Could not set signature algorithm id.");
	goto done;
    }

    /* we only deal with cert v3 here */
    *(cert->version.data) = 2;
    cert->version.len = 1;

    rv = DER_Encode (arena, &der, CERTCertificateTemplate, cert);
    if (rv != SECSuccess) {
	fprintf (stderr, "Could not encode certificate.\n");
	goto done;
    }

    result = (SECItem *) PORT_ArenaZAlloc (arena, sizeof (SECItem));
    if (result == NULL) {
	fprintf (stderr, "Could not allocate item for certificate data.\n");
	goto done;
    }

    rv = SEC_DerSignData (arena, result, der.data, der.len, caPrivateKey,
			  algID);
    if (rv != SECSuccess) {
	fprintf (stderr, "Could not sign encoded certificate data.\n");
	PORT_Free(result);
	result = NULL;
	goto done;
    }
    cert->derCert = *result;
done:
    SECKEY_DestroyPrivateKey(caPrivateKey);
    return result;
}
static SECStatus AddAuthKeyID (void *extHandle)
{
    CERTAuthKeyID *authKeyID = NULL;    
    PRArenaPool *arena = NULL;
    SECStatus rv = SECSuccess;
    char charValue;
    char buffer[512];

    do {
	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if ( !arena ) {
	    SECU_PrintError(progName, "out of memory");
	    GEN_BREAK (SECFailure);
	}

	if (GetYesNo ("Enter value for the authKeyID extension [y/n]?\n") == 0)
	    break;
	
	authKeyID = PORT_ArenaZAlloc (arena, sizeof (CERTAuthKeyID));
	if (authKeyID == NULL) {
	    GEN_BREAK (SECFailure);
	}

	fflush (stdin);
	rv = GetString (arena, "Enter value for the key identifier fields, enter to omit:",
			&authKeyID->keyID);
	if (rv != SECSuccess)
	    break;
	authKeyID->authCertIssuer = GetGeneralName (arena);
	if (authKeyID->authCertIssuer == NULL && SECFailure == PORT_GetError ())
		break;
	

	rv = GetString (arena, "Enter value for the authCertSerial field, enter to omit:",
			&authKeyID->authCertSerialNumber);

	buffer[0] = 'n';
	puts ("Is this a critical extension [y/n]? ");
	gets (buffer);	

	rv = EncodeAndAddExtensionValue
	    (arena, extHandle, authKeyID,
	     (buffer[0] == 'y' || buffer[0] == 'Y') ? PR_TRUE : PR_FALSE,
	     SEC_OID_X509_AUTH_KEY_ID, CERT_EncodeAuthKeyID);
	if (rv)
	    break;
	
    } while (0);
    if (arena)
	PORT_FreeArena (arena, PR_FALSE);
    return (rv);
}   
    
static SECStatus AddCrlDistPoint (void *extHandle)
{
    PRArenaPool *arena = NULL;
    CERTCrlDistributionPoints *crlDistPoints = NULL;
    CRLDistributionPoint *current;
    SECStatus rv = SECSuccess;
    int count = 0, intValue;
    char charValue, buffer[5];

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if ( !arena )
	return (SECFailure);

    do {
	current = NULL;
	current = PORT_ArenaZAlloc (arena, sizeof (*current));
        if (current == NULL) {
	    GEN_BREAK (SECFailure);
	}   

	fflush (stdin);
	/* Get the distributionPointName fields - this field is optional */
	puts ("Enter the type of the distribution point name:\n");
	puts ("\t1 - Full Name\n\t2 - Relative Name\n\tOther - omit\n\t\tChoice: ");
	scanf ("%d", &intValue);
	switch (intValue) {
	    case generalName:
		current->distPointType = intValue;
		current->distPoint.fullName = GetGeneralName (arena);
		rv = PORT_GetError();
		break;
		
	    case relativeDistinguishedName: {
		CERTName *name;
		char buffer[512];

		current->distPointType = intValue;
		puts ("Enter the relative name: ");
		fflush (stdout);
		fflush (stdin);
		gets (buffer);
		/* For simplicity, use CERT_AsciiToName to converse from a string
		   to NAME, but we only interest in the first RDN */
		name = CERT_AsciiToName (buffer);
		if (!name) {
		    GEN_BREAK (SECFailure);
		}
		rv = CERT_CopyRDN (arena, &current->distPoint.relativeName, name->rdns[0]);
		CERT_DestroyName (name);
		break;
	    }
	}
	if (rv != SECSuccess)
	    break;

	/* Get the reason flags */
	puts ("\nSelect one of the following for the reason flags\n");
	puts ("\t0 - unused\n\t1 - keyCompromise\n\t2 - caCompromise\n\t3 - affiliationChanged\n");
	puts ("\t4 - superseded\n\t5 - cessationOfOperation\n\t6 - certificateHold\n");
	puts ("\tother - omit\t\tChoice: ");
	
	scanf ("%d", &intValue);
	if (intValue >= 0 && intValue <8) {
	    current->reasons.data = PORT_ArenaAlloc (arena, sizeof(char));
	    if (current->reasons.data == NULL) {
		GEN_BREAK (SECFailure);
	    }
	    *current->reasons.data = (char)(0x80 >> intValue);
	    current->reasons.len = 1;
	}
	puts ("Enter value for the CRL Issuer name:\n");
        current->crlIssuer = GetGeneralName (arena);
	if (current->crlIssuer == NULL && (rv = PORT_GetError()) == SECFailure)
	    break;

	if (crlDistPoints == NULL) {
	    crlDistPoints = PORT_ArenaZAlloc (arena, sizeof (*crlDistPoints));
	    if (crlDistPoints == NULL) {
		GEN_BREAK (SECFailure);
	    }
	}
	    
	crlDistPoints->distPoints = PORT_ArenaGrow
				    (arena, crlDistPoints->distPoints,
			             sizeof (*crlDistPoints->distPoints) * count,
				     sizeof (*crlDistPoints->distPoints) *(count + 1));
	if (crlDistPoints->distPoints == NULL) {
	    GEN_BREAK (SECFailure);
	}
	
	crlDistPoints->distPoints[count] = current;
	++count;
	if (GetYesNo ("Enter more value for the CRL distribution point extension [y/n]\n") == 0) {
	    /* Add null to the end of the crlDistPoints to mark end of data */
	    crlDistPoints->distPoints = PORT_ArenaGrow
					(arena, crlDistPoints->distPoints,
					 sizeof (*crlDistPoints->distPoints) * count,
					 sizeof (*crlDistPoints->distPoints) *(count + 1));
	    crlDistPoints->distPoints[count] = NULL;	    
	    break;
	}
	

    } while (1);
    
    if (rv == SECSuccess) {
	fflush (stdin);
	buffer[0] = 'n';
	puts ("Is this a critical extension [y/n]? ");
	gets (buffer);	
	
	rv = EncodeAndAddExtensionValue
	     (arena, extHandle, crlDistPoints,
	      (buffer[0] == 'Y' || buffer[0] == 'y') ? PR_TRUE : PR_FALSE,
	      SEC_OID_X509_CRL_DIST_POINTS, CERT_EncodeCRLDistributionPoints);
    }
    if (arena)
	PORT_FreeArena (arena, PR_FALSE);
    return (rv);
}

static SECStatus
CreateCert(CERTCertDBHandle *handle, char *issuerNickName, FILE *inFile,
	   PRBool keyUsage, PRBool basicConstraint, PRBool authKeyID,
	   PRBool crlDistPoints, FILE *outFile, PRBool selfsign,
	   int serialNumber, int warpmonths)
{
    void *extHandle;
    SECItem *certDER;
    SECItem reqDER;
    SECStatus rv = SECSuccess;
    PRArenaPool *arena = NULL;
    CERTCertificate *subjectCert = NULL, *issuerCert = NULL;
    CERTCertificateRequest *certReq = NULL;
    CERTCertDBHandle *certHandle;
    
    reqDER.data = NULL;
    do {
	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if (!arena) {
	    GEN_BREAK (SEC_ERROR_NO_MEMORY);
	}
	
	/* Create a certrequest object from the input cert request der */
	certReq = GetCertRequest (inFile);
	if (certReq == NULL) {
	    GEN_BREAK (SECFailure)
	}

	subjectCert = MakeV1Cert (handle, certReq, issuerNickName, selfsign,
				  serialNumber, warpmonths);
	if (subjectCert == NULL) {
	    GEN_BREAK (SECFailure)
	}

	extHandle = CERT_StartCertExtensions (subjectCert);
	if (extHandle == NULL) {
	    GEN_BREAK (SECFailure)
	}

	/* Add key usage extension */
	if (keyUsage) {
	    rv = AddKeyUsage(extHandle);
	    if (rv)
		break;
	}

	/* Add basic constraint extension */
	if (basicConstraint) {
	    rv = AddBasicConstraint(extHandle);
	    if (rv)
		break;
	}

	if (authKeyID) {
	    rv = AddAuthKeyID (extHandle);
	    if (rv)
		break;
	}    


       if (crlDistPoints) {
	   rv = AddCrlDistPoint (extHandle);
	   if (rv)
	       break;
       }
       

	CERT_FinishExtensions(extHandle);

	certDER = SignCert (subjectCert, issuerNickName);
	if (certDER)
   	   fwrite (certDER->data, 1, certDER->len, outFile);

    } while (0);
    CERT_DestroyCertificateRequest (certReq);
    CERT_DestroyCertificate (subjectCert);
    PORT_FreeArena (arena, PR_FALSE);
    if (rv != SECSuccess) {
       fprintf(stderr, "%s: unable to create cert (%s)\n", progName,
	       SECU_ErrorString(PORT_GetError()));
    }
    return (rv);
}

int main(int argc, char **argv)
{
    PRBool authKeyID, basicConstraint, crlDistPoints, keyUsage;
    CERTName *subject;

    FILE *inFile, *outFile;
    SECStatus rv = SECSuccess;
      
    char *name;
    char *trusts, *phone, *date, *certUsage, *checkSignature;
    int o, i;
    int ascii, newdb, addcert, certreq, listcerts, delete,
        modify, raw, validateCert, createCert;
    static int genNameCount = 0, genNameTypeCount = 0;
    int currentDistPoint;
    PRBool selfsign = PR_FALSE;
    int serialNumber = 0;
    CERTCertDBHandle *handle;
    PRBool emailcert = PR_FALSE;
    int warpmonths = 0;
    PRBool logit = PR_FALSE;
    
    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

    authKeyID = basicConstraint = crlDistPoints = keyUsage = PR_FALSE;
    name = trusts = phone = date = certUsage = checkSignature = NULL;
    inFile = outFile = NULL;
    subject = 0;
    ascii = newdb = addcert = certreq = listcerts =
    delete = modify = raw = validateCert = createCert = 0;

    /* Parse command line arguments */
    while ((o = getopt
		(argc, argv, "HNAEn:d:t:i:Rs:p:aVb:u:e:C1234o:LrDMxm:w:l")) != -1) {
	switch (o) {
	  case '?':
	    Usage(progName);
	    break;

	  case 'H':
	    LongUsage(progName);
	    break;

	  case 'N':
	    newdb = 1;
	    break;

  	  case 'A':
	    addcert = 1;
	    break;

  	  case 'E':
	    addcert = 1;
	    emailcert = PR_TRUE;
	    break;

	  case 'R':
	    certreq = 1;
	    break;

	  case 'L':
	    listcerts = 1;
	    break;

	  case 'D':
	    delete = 1;
	    break;

	  case 'M':
	    modify = 1;
	    break;

	  case 'V':
	    validateCert = 1;
	    break;

	  case 'C':
	    createCert = 1;
	    break;

	  case 'd':
	    SECU_ConfigDirectory(optarg);
	    break;

	  case 'i':
	    inFile = fopen(optarg, "r");
	    if (!inFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for reading\n",
			progName, optarg);
		rv = -1;
	    }
	    break;

	  case 'n':
	    name = strdup(optarg);
	    break;

	  case 'e':
	    checkSignature = strdup(optarg);
	    break;

	  case 'u':
	    certUsage = strdup(optarg);
	    break;

	  case 't':
	    trusts = strdup(optarg);
	    break;

	  case 'p':
	    phone = strdup(optarg);
	    break;

	  case 's':
	    subject = CERT_AsciiToName(optarg);
	    if (!subject) {
		fprintf(stderr, "%s: improperly formatted name: \"%s\"\n",
			progName, optarg);
		return -1;
	    }
	    break;

	  case 'a':
	    ascii = 1;
	    break;

	  case 'r':
	    raw = 1;
	    break;
	  case 'b':
	    date = strdup(optarg);
	    break;
	  case 'o':
	    outFile = fopen(optarg, "wb");
	    if (!outFile) {
		fprintf(stderr, "%s: unable to open \"%s\" for writing\n",
			progName, optarg);
		rv = -1;
	    }
	    break;
	  case '1':
	    /* key usage extension */
	    keyUsage = PR_TRUE;
	    break;

	  case '2':
	    basicConstraint = PR_TRUE;  
	    break;

	  case '3':
	    authKeyID = PR_TRUE;
	    break;
	  case '4':
	      crlDistPoints = 1;
	      break;	

	  case 'x':
	      selfsign = PR_TRUE;
	      break;
	  case 'm':
	      serialNumber = atoi(optarg);
	      break;
	  case 'w': /* timewarp */
	      warpmonths = atoi(optarg);
	      break;
	  case 'l': /* log on verify */
	      logit = PR_TRUE;
	      break;
	}
	if (rv)
	    return (rv);
    }

    /* check command line arguments */
    if (addcert || certreq || delete || modify || validateCert || createCert)
	if (!name) Usage(progName);

    if ((addcert || modify) && !trusts) Usage(progName);

    if (newdb+certreq+addcert+listcerts+delete+modify+validateCert+createCert != 1)
        Usage(progName);

    if ((listcerts && (raw || ascii)) && !name)
	Usage(progName);

    if (listcerts && raw && ascii)
	Usage(progName);

    if (certreq) {
	if (!subject) Usage(progName);
    } else if (addcert) {
	if (!inFile) inFile = stdin;
    }

    if (validateCert && !(date && name && certUsage && checkSignature))
	Usage(progName);

    if (createCert && (!(name && inFile && outFile) || (genNameCount != genNameTypeCount)) )
	Usage (progName);

    

    PR_Init("certutil", 1, 1, 0);
    SECU_PKCS11Init();     
    SEC_Init();
    handle = OpenCertDB();
    if ( handle == NULL ) {
	SECU_PrintError(progName, "unable to open cert database");
	return(-1);
    }
    
    if (addcert) {
	rv = AddCert(handle, name, trusts, inFile, ascii, emailcert);
    } else
    if (certreq) {
	rv = CertReq(name, subject, phone, ascii);
    } else
    if (listcerts) {
	rv = ListCerts(handle, name, raw, ascii);
    } else
    if (delete) {
	rv = DeleteCert(handle, name);
    } else
    if (modify) {
	rv = ChangeTrustAttributes(handle, name, trusts);
    } else
    if (validateCert) {
	rv = ValidateCert (handle, name, date, certUsage, checkSignature,
			   logit);
    } else
    if (createCert) {
	rv = CreateCert(handle, name, inFile, keyUsage, basicConstraint,
			authKeyID, crlDistPoints, outFile, selfsign,
			serialNumber, warpmonths);
    }

    if ( handle ) {
	CERT_ClosePermCertDB(handle);
    }
    
    return rv;  
}

