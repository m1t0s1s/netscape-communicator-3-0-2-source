/*
** secutil.c - various functions used by security stuff
**
*/

#include "secutil.h"
#include "secpkcs7.h"
#include <sys/stat.h>
#include "dserr.h"

#ifdef XP_UNIX
#include <unistd.h>
#endif

/* for SEC_TraverseNames */
#include "cert.h"
#include "prtypes.h"
#include "prtime.h"

#include "prlong.h"
#include "secmod.h"
#include "pk11func.h"

static char consoleName[] =  {
#ifdef XP_UNIX
    "/dev/tty"
#else
    "CON:"
#endif
};

char *
XP_GetString(int16 error_number)
{

#ifdef XP_UNIX
    extern char *XP_GetBuiltinString(int16 error_number);

    return XP_GetBuiltinString(error_number);
#else
    return "Unknown Error String!!";
#endif
}

void 
SECU_PrintError(char *progName, char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(stderr, "%s: ", progName);
    vfprintf(stderr, msg, args);
    if (strlen(SECU_ErrorString(PORT_GetError())) > 0)
	fprintf(stderr, ": %s\n", SECU_ErrorString(PORT_GetError()));
    else
	fprintf(stderr, "\n");
    va_end(args);
}

void
SECU_PrintSystemError(char *progName, char *msg, ...)
{
    va_list args;

    va_start(args, msg);
    fprintf(stderr, "%s: ", progName);
    vfprintf(stderr, msg, args);
    fprintf(stderr, ": %s\n", strerror(errno));
    va_end(args);
}

static char SECUErrorBuf[64];

char *
SECU_ErrorStringRaw(int16 err)
{
    if (err == 0)
	sprintf(SECUErrorBuf, "");
    else if (err == SEC_ERROR_BAD_DATA)
	sprintf(SECUErrorBuf, "Bad data");
    else if (err == SEC_ERROR_BAD_DATABASE)
	sprintf(SECUErrorBuf, "Problem with database");
    else if (err == SEC_ERROR_BAD_DER)
	sprintf(SECUErrorBuf, "Problem with DER");
    else if (err == SEC_ERROR_BAD_KEY)
	sprintf(SECUErrorBuf, "Problem with key");
    else if (err == SEC_ERROR_BAD_PASSWORD)
	sprintf(SECUErrorBuf, "Incorrect password");
    else if (err == SEC_ERROR_BAD_SIGNATURE)
	sprintf(SECUErrorBuf, "Bad signature");
    else if (err == SEC_ERROR_EXPIRED_CERTIFICATE)
	sprintf(SECUErrorBuf, "Expired certificate");
    else if (err == SEC_ERROR_EXTENSION_VALUE_INVALID)
	sprintf(SECUErrorBuf, "Invalid extension value");
    else if (err == SEC_ERROR_INPUT_LEN)
	sprintf(SECUErrorBuf, "Problem with input length");
    else if (err == SEC_ERROR_INVALID_ALGORITHM)
	sprintf(SECUErrorBuf, "Invalid algorithm");
    else if (err == SEC_ERROR_INVALID_ARGS)
	sprintf(SECUErrorBuf, "Invalid arguments");
    else if (err == SEC_ERROR_INVALID_AVA)
	sprintf(SECUErrorBuf, "Invalid AVA");
    else if (err == SEC_ERROR_INVALID_TIME)
	sprintf(SECUErrorBuf, "Invalid time");
    else if (err == SEC_ERROR_IO)
	sprintf(SECUErrorBuf, "Security I/O error");
    else if (err == SEC_ERROR_LIBRARY_FAILURE)
	sprintf(SECUErrorBuf, "Library failure");
    else if (err == SEC_ERROR_NO_MEMORY)
	sprintf(SECUErrorBuf, "Out of memory");
    else if (err == SEC_ERROR_OLD_CRL)
	sprintf(SECUErrorBuf, "CRL is older than the current one");
    else if (err == SEC_ERROR_OUTPUT_LEN)
	sprintf(SECUErrorBuf, "Problem with output length");
    else if (err == SEC_ERROR_UNKNOWN_ISSUER)
	sprintf(SECUErrorBuf, "Unknown issuer");
    else if (err == SEC_ERROR_UNTRUSTED_CERT)
	sprintf(SECUErrorBuf, "Untrusted certificate");
    else if (err == SEC_ERROR_UNTRUSTED_ISSUER)
	sprintf(SECUErrorBuf, "Untrusted issuer");
    else if (err == SSL_ERROR_BAD_CERTIFICATE)
	sprintf(SECUErrorBuf, "Bad certificate");
    else if (err == SSL_ERROR_BAD_CLIENT)
	sprintf(SECUErrorBuf, "Bad client");
    else if (err == SSL_ERROR_BAD_SERVER)
	sprintf(SECUErrorBuf, "Bad server");
    else if (err == SSL_ERROR_EXPORT_ONLY_SERVER)
	sprintf(SECUErrorBuf, "Export only server");
    else if (err == SSL_ERROR_NO_CERTIFICATE)
	sprintf(SECUErrorBuf, "No certificate");
    else if (err == SSL_ERROR_NO_CYPHER_OVERLAP)
	sprintf(SECUErrorBuf, "No cypher overlap");
    else if (err == SSL_ERROR_UNSUPPORTED_CERTIFICATE_TYPE)
	sprintf(SECUErrorBuf, "Unsupported certificate type");
    else if (err == SSL_ERROR_UNSUPPORTED_VERSION)
	sprintf(SECUErrorBuf, "Unsupported version");
    else if (err == SSL_ERROR_US_ONLY_SERVER)
	sprintf(SECUErrorBuf, "U.S. only server");
    else if (err == XP_ERRNO_EIO)
	sprintf(SECUErrorBuf, "I/O error");

    return SECUErrorBuf;
}

char *
SECU_ErrorString(int16 err)
  {
  char *error_string;

  *SECUErrorBuf = 0;
  SECU_ErrorStringRaw (err);

  if (*SECUErrorBuf == 0)
    { 
    error_string = XP_GetString(err);
    if (error_string == NULL || *error_string == '\0') 
      sprintf(SECUErrorBuf, "No error string found for %d.",  err);
    else
      return error_string;
    }

  return SECUErrorBuf;
  }

static void
secu_ClearPassword(char *p)
{
    if (p) {
	PORT_Memset(p, 0, PORT_Strlen(p));
	PORT_Free(p);
    }
}

static SECItem *
secu_GetZeroLengthPassword(SECKEYKeyDBHandle *handle)
{
    SECItem *pwitem;
    SECStatus rv;

    /* hash the empty string as a password */
    pwitem = SECKEY_HashPassword("", handle->global_salt);
    if (pwitem == NULL) {
	return NULL;
    }

    /* check to see if this is the right password */
    rv = SECKEY_CheckKeyDBPassword(handle, pwitem);
    if (rv == SECFailure) {
	return NULL;
    }

    return pwitem;
}

char *
SECU_GetPasswordString(void *arg, char *prompt)
{
    char *p = NULL;
    SECStatus rv;
    SECItem *pwitem;
    FILE *input, *output;


    /* open terminal */
    input = fopen(consoleName, "r");
    if (input == NULL) {
	fprintf(stderr, "Error opening input terminal for read\n");
	return NULL;
    }

    output = fopen(consoleName, "w");
    if (output == NULL) {
	fprintf(stderr, "Error opening output terminal for write\n");
	return NULL;
    }

    p = SEC_GetPassword(input, output, prompt, SEC_BlindCheckPassword);
        

    fclose(input);
    fclose(output);
    return p;
}

SECItem *
SECU_GetPassword(void *arg, SECKEYKeyDBHandle *handle)
{
    char *p = NULL;
    SECStatus rv;
    SECItem *pwitem;
    FILE *input, *output;

    /* Check to see if zero length password or not */
    pwitem = secu_GetZeroLengthPassword(handle);
    if (pwitem) {
	return pwitem;
    }

    p = SECU_GetPasswordString(arg,"Password: ");

    /* Check to see if zero length password or not */
    pwitem = secu_GetZeroLengthPassword(handle);
    if (pwitem) {
	return pwitem;
    }
    /* hash the password */
    pwitem = SECKEY_HashPassword(p, handle->global_salt);
    
    /* clear out the password strings */
    secu_ClearPassword(p);
    

    if ( pwitem == NULL ) {
	fprintf(stderr, "Error hashing password\n");
	return NULL;
    }

    /* confirm the password */
    rv = SECKEY_CheckKeyDBPassword(handle, pwitem);
    if (rv) {
	fprintf(stderr, "Sorry\n");
	SECITEM_ZfreeItem(pwitem, PR_TRUE);
	return NULL;
    }
    
    return pwitem;
}

char *
SECU_GetModulePassword(PK11SlotInfo *slot,void *arg) {
    char *pw;
    char prompt[255];

    sprintf(prompt, "Enter Password or Pin for \"%s\":",
	    PK11_GetTokenName(slot));
    pw = SECU_GetPasswordString(arg, prompt);
    return pw;
}

static SECItem *
secu_PutPassword(void *arg, SECKEYKeyDBHandle *handle)
{
    char *p0 = NULL;
    char *p1 = NULL;
    int isTTY;
    SECStatus rv;
    SECItem *pwitem;
    FILE *input, *output;

    /* if already have password, ask for it and return SECItem */
    rv = SECKEY_HasKeyDBPassword(handle);
    if (rv == SECSuccess) {
	pwitem = SECU_GetPassword(arg, handle);
	return pwitem;
    }

    /* open terminal */
    input = fopen(consoleName, "r");
    if (input == NULL) {
	fprintf(stderr, "Error opening input terminal for read\n");
	return NULL;
    }

    output = fopen(consoleName, "w");
    if (output == NULL) {
	fprintf(stderr, "Error opening output terminal for write\n");
	return NULL;
    }

    /* we have no password, so initialize database with one */
    fprintf(stderr, "In order to finish creating your database, you\n");
    fprintf(stderr, "must enter a password which will be used to encrypt\n");
    fprintf(stderr, "this key and any future keys.\n\n");
    fprintf(stderr, "The password must be at least 8 characters long, and\n");
    fprintf(stderr, "must contain at least one non-alphabetic character.\n\n");
    for (;;) {
	p0 = SEC_GetPassword(input, output, "Enter new password: ",
			    SEC_BlindCheckPassword);
	if (isTTY) {
	    p1 = SEC_GetPassword(input, output, "Re-enter password: ",
				 SEC_BlindCheckPassword);
	}
	if (!isTTY || ( PORT_Strcmp(p0, p1) == 0) ) {
	    break;
	}
	fprintf(stderr, "Passwords do not match. Try again.\n");
    }
        
    /* hash the password */
    pwitem = SECKEY_HashPassword(p0, handle->global_salt);
    
    /* clear out the password strings */
    secu_ClearPassword(p0);
    secu_ClearPassword(p1);
    
    fclose(input);
    fclose(output);

    if ( pwitem == NULL ) {
	fprintf(stderr, "Error hashing password\n");
	return NULL;
    }
    
    /* now set the new password */
    rv = SECKEY_SetKeyDBPassword(handle, pwitem);
    if (rv) {
	fprintf(stderr, "Error setting database password\n");
	SECITEM_ZfreeItem(pwitem, PR_TRUE);
	return NULL;
    }
 
    return pwitem;
}

struct matchobj {
    SECItem index;
    unsigned char *nname;
    PRBool found;
};


static SECStatus
secu_match_nickname(DBT *k, DBT *d, void *pdata)
{
    struct matchobj *match;
    unsigned char *buf;
    unsigned char *nname;
    int nnlen;

    match = (struct matchobj *)pdata;
    buf = (unsigned char *)d->data;

    if (match->found == PR_TRUE)
	return SECSuccess;

    nnlen = buf[2];
    nname = buf + 3 + buf[1];
    if (PORT_Strncmp(match->nname, nname, nnlen) == 0) {
	match->index.len = k->size;
	match->index.data = PORT_ZAlloc(k->size + 1);
	PORT_Memcpy(match->index.data, k->data, k->size);
	match->found = PR_TRUE;
    }
    return SECSuccess;
}

SECItem *
SECU_GetKeyIDFromNickname(char *name)
{
    struct matchobj match;
    SECKEYKeyDBHandle *handle;
    SECItem *keyid;

    match.nname = name;
    match.found = PR_FALSE;

    handle = SECKEY_GetDefaultKeyDB();

    SECKEY_TraverseKeys(handle, secu_match_nickname, &match);

    if (match.found == PR_FALSE)
	return NULL;

    keyid = SECITEM_DupItem(&match.index);
    return keyid;
}

PRBool
SECU_CheckKeyNameExists(SECKEYKeyDBHandle *handle, char *nickname)
{
    SECItem *keyid;
    SECStatus rv;

    keyid = SECU_GetKeyIDFromNickname(nickname);
    if(keyid == NULL)
	return PR_FALSE;
    SECITEM_FreeItem(keyid, PR_TRUE);
    return PR_TRUE;
}

SECKEYPrivateKey *
SECU_FindPrivateKeyFromNickname(char *name)
{
    SECItem *keyid;
    SECKEYPrivateKey *key;
    PK11SlotInfo *slot = PK11_GetInternalKeySlot();

    keyid = SECU_GetKeyIDFromNickname(name);
    if (keyid == NULL)
	return NULL;

    PK11_SetPasswordFunc(SECU_GetModulePassword);
    if(PK11_NeedLogin(slot) && !PK11_IsLoggedIn(slot))
	PK11_DoPassword(slot, NULL);

    key = PK11_FindKeyByKeyID(slot, keyid, NULL);
    SECITEM_FreeItem(keyid, PR_TRUE);
    return key;
}

SECKEYLowPrivateKey *
SECU_FindLowPrivateKeyFromNickname(char *name)
{
    SECItem *keyID;
    SECKEYLowPrivateKey *key;

    keyID = SECU_GetKeyIDFromNickname(name);
    if (keyID == NULL)
	return NULL;

    key = SECKEY_FindKeyByPublicKey(SECKEY_GetDefaultKeyDB(), keyID,
				    SECU_GetPassword, NULL);
    SECITEM_FreeItem(keyID, PR_TRUE);
    return key;
}

SECStatus
SECU_DeleteKeyByName(SECKEYKeyDBHandle *handle, char *nickname)
{
    SECItem *keyID = NULL;
    SECStatus rv;

    keyID = SECU_GetKeyIDFromNickname(nickname);
    if (keyID == NULL)
	return SECFailure;

    rv = SECKEY_DeleteKey(handle, keyID);
    SECITEM_FreeItem(keyID, PR_TRUE);

    return rv;
}

SECKEYLowPrivateKey *
SECU_GetPrivateKey(SECKEYKeyDBHandle *handle, char *nickname)
{
    return SECU_FindLowPrivateKeyFromNickname(nickname);
}

SECStatus
SECU_PutPrivateKey(SECKEYKeyDBHandle *handle, char *nickname,
		   SECKEYLowPrivateKey *key)
{
    SECItem *pubKeyData;

    switch (key->keyType) {
      case rsaKey:
	pubKeyData = &key->u.rsa.modulus;
	break;
      case dsaKey:
	pubKeyData = &key->u.dsa.publicValue;
	break;
      default:
	return SECFailure;
    }

    return SECKEY_StoreKeyByPublicKey(handle, key, pubKeyData, nickname,
				      SECU_GetPassword, NULL);
}

SECStatus
SECU_ChangeKeyDBPassword(SECKEYKeyDBHandle *handle)
{
    static SECItem *newpwitem, *oldpwitem;
    char *p0 = 0;
    char *p1 = 0;
    int isTTY;
    SECStatus rv;
    int failed = 0;
    FILE *input, *output;
    PRBool newdb = PR_FALSE;

    if (SECKEY_HasKeyDBPassword(handle) == SECFailure) {
	fprintf(stderr, "Database not initialized.  Setting password.\n");
	newdb = PR_TRUE;
    }

    /* check if old password is empty string */
    oldpwitem = secu_GetZeroLengthPassword(handle);

    /* open terminal */
    input = fopen(consoleName, "r");
    if (input == NULL) {
	fprintf(stderr, "Error opening input terminal\n");
	return SECFailure;
    }

    output = fopen(consoleName, "w");
    if (output == NULL) {
	fprintf(stderr, "Error opening output terminal\n");
	return SECFailure;
    }

    /* if old password is not zero length, ask for new password */
    if ((newdb == PR_FALSE) && (oldpwitem == NULL)) {
	p0 = SEC_GetPassword(input, output, "Old Password: ",
			   SEC_BlindCheckPassword);

	oldpwitem = SECKEY_HashPassword(p0, handle->global_salt);
	secu_ClearPassword(p0);

	if (oldpwitem == NULL) {
	    fprintf(stderr, "Error hashing password\n");
	    fclose(input);
	    fclose(output);
	    return SECFailure;
	}

	rv = SECKEY_CheckKeyDBPassword(handle, oldpwitem);
	if (rv) {
	    fprintf(stderr, "Sorry\n");
	    SECITEM_ZfreeItem(oldpwitem, PR_TRUE);
	    fclose(input);
	    fclose(output);
	    return SECFailure;
	}
    }

    isTTY = isatty(0);
    for (;;) {
	p0 = SEC_GetPassword(input, output, "Enter new password: ",
				 SEC_BlindCheckPassword);
	if (isTTY) {
	    p1 = SEC_GetPassword(input, output, "Re-enter password: ",
				 SEC_BlindCheckPassword);
	}
	
	if (!isTTY || ( PORT_Strcmp(p0, p1) == 0) ) {
	    break;
	}
	fprintf(stderr, "Passwords do not match. Try again.\n");
    }
    
    newpwitem = SECKEY_HashPassword(p0, handle->global_salt);
    
    secu_ClearPassword(p0);
    secu_ClearPassword(p1);

    if (newpwitem == NULL) {
	fprintf(stderr, "Error hashing new password\n");
	SECITEM_ZfreeItem(oldpwitem, PR_TRUE);
	fclose(input);
	fclose(output);
	return SECFailure;
    }

    if (newdb == PR_TRUE) {
	rv = SECKEY_SetKeyDBPassword(handle, newpwitem);
	if (rv) {
	    fprintf(stderr, "Error setting database password\n");
	    failed = 1;
	}
    } else {
	rv = SECKEY_ChangeKeyDBPassword(handle, oldpwitem, newpwitem);
	if (rv) {
	    fprintf(stderr, "Error changing database password\n");
	    failed = 1;
	}
    }

    SECITEM_ZfreeItem(newpwitem, PR_TRUE);
    SECITEM_ZfreeItem(oldpwitem, PR_TRUE);

    fclose(input);
    fclose(output);

    if (failed) {
	return SECFailure;
    }

    return SECSuccess;
}

#ifdef notdef
static SECItem *
secu_GetDonglePassword(void *arg, SECKEYKeyDBHandle *handle)
{
    SECItem *pwitem;
    char *p = NULL;
    char *pathname;
    SECStatus rv;
    int fd;
    
    pathname = (char *)arg;
    
    fd = open((char *)pathname, O_RDONLY);
    if (!fd) {
        fprintf(stderr, "Unable to open dongle file \"%s\".\n", (char *)arg);
    }
    
    p = SEC_ReadDongleFile(fd);
    if (!p) {
        fprintf(stderr, "Unable to obtain dongle password\n");
    }
    
    /* check if we need to update the key database */
    if ( handle->version < PRIVATE_KEY_DB_FILE_VERSION ) {
	SECKEY_UpdateKeyDB(handle, p);
    }

    /* hash the password */
    pwitem = SECKEY_HashPassword(p, handle->global_salt);

    /* clear out the password strings */
    secu_ClearPassword(p);

    if (pwitem == NULL) {
	fprintf(stderr, "Error hashing password\n");
	return NULL;
    }

    /* confirm the password */
    rv = SECKEY_CheckKeyDBPassword(handle, pwitem);
    if (rv) {
	fprintf(stderr, "Sorry, dongle password is invalid\n");
	SECITEM_ZfreeItem(pwitem, PR_TRUE);
	return NULL;
    }

    return pwitem;
}

SECKEYPrivateKey *
SECU_GetPrivateDongleKey(SECKEYKeyDBHandle *handle, char *nickname, 
			 char *pathname)
{
    SECKEYPrivateKey *key;
    char *fullpath;
    int rv;
    
    fullpath = SECU_AppendFilenameToDir(pathname, "dongle");
    
    /* If dongle file doesn't exist, prompt for password */
    rv = access(fullpath, R_OK);
    if (rv < 0) {
	return SECU_GetPrivateKey(handle, nickname);
    }
    
    /* try dongle file */
    key = SECKEY_FindKeyByName(handle, nickname, secu_GetDonglePassword,
			    fullpath);

    /* if no key, maybe dongle is broken, so prompt for password */
    if (key == NULL) {
	key = SECU_GetPrivateKey(handle, nickname);
    }

    return key;
}
#endif

char *
SECU_DefaultSSLDir(void)
{
    char *dir;
    static char sslDir[1000];

    dir = getenv("SSL_DIR");
    if (!dir)
	return NULL;

    sprintf(sslDir, "%s", dir);

    if (sslDir[strlen(sslDir)-1] == '/')
	sslDir[strlen(sslDir)-1] = 0;

    return sslDir;
}

char *
SECU_AppendFilenameToDir(char *dir, char *filename)
{
    static char path[1000];

    if (dir[strlen(dir)-1] == '/')
	sprintf(path, "%s%s", dir, filename);
    else
	sprintf(path, "%s/%s", dir, filename);
    return path;
}

char *
SECU_ConfigDirectory(const char* base)
{
    static PRBool initted = PR_FALSE;
    const char *dir = ".netscape";
    char *home;
    static char buf[1000];

    if (initted) return buf;
    

    if (base == NULL || *base == 0) {
	home = getenv("HOME");
	if (!home) home = "";

	if (*home && home[strlen(home) - 1] == '/')
	    sprintf (buf, "%.900s%s", home, dir);
	else
	    sprintf (buf, "%.900s/%s", home, dir);
    } else {
	sprintf(buf, "%.900s", base);
	if (buf[strlen(buf) - 1] == '/')
	    buf[strlen(buf) - 1] = 0;
    }


    initted = PR_TRUE;
    return buf;
}

char *
SECU_CertDBNameCallback(void *arg, int dbVersion)
{
    char *fnarg;
    char *dir;
    char *filename;
    
    dir = SECU_ConfigDirectory(NULL);
    
    switch ( dbVersion ) {
      case 7:
	fnarg = "7";
	break;
      case 6:
	fnarg = "6";
	break;
      case 5:
	fnarg = "5";
	break;
      case 4:
      default:
	fnarg = "";
	break;
    }
    filename = PR_smprintf("%s/cert%s.db", dir, fnarg);
    return(filename);
}

char *
SECU_KeyDBNameCallback(void *arg, int dbVersion)
{
    char *fnarg;
    char *dir;
    char *filename;
    
    dir = SECU_ConfigDirectory(NULL);
    
    switch ( dbVersion ) {
      case 3:
	fnarg = "3";
	break;
      case 2:
      default:
	fnarg = "";
	break;
    }
    filename = PR_smprintf("%s/key%s.db", dir, fnarg);
    return(filename);
}

SECKEYKeyDBHandle *
SECU_OpenKeyDB(void)
{
    SECKEYKeyDBHandle *handle;
    
    handle = SECKEY_OpenKeyDB(PR_FALSE, SECU_KeyDBNameCallback, NULL);
    SECKEY_SetDefaultKeyDB(handle);
    
    return(handle);
}

char *
SECU_DatabaseFileName(XP_FileType type)
{
    char *conf_dir;
    char *filename;

    conf_dir = SECU_ConfigDirectory(NULL);

    switch(type) {
    case xpKeyDB:
	filename = PR_smprintf("%s/key3.db", conf_dir);
	break;
    case xpCertDB:
	filename = PR_smprintf("%s/cert7.db", conf_dir);
	break;
    case xpCertDBNameIDX:
	filename = PR_smprintf("%s/cert-nameidx.db", conf_dir);
	break;
    case xpSecModuleDB:
	filename = PR_smprintf("%s/secmodule.db", conf_dir);
	break;
    default:
	abort();
    }
    return filename;
}

/*Turn off SSL for now */
/* This gets called by SSL when server wants our cert & key */
int
SECU_GetClientAuthData(void *arg, int fd, struct CERTDistNamesStr *caNames,
                      struct CERTCertificateStr **pRetCert,
                      struct SECKEYPrivateKeyStr **pRetKey)
{
    SECKEYKeyDBHandle *keydb;
    SECKEYPrivateKey *key;
    CERTCertificate *cert;
    int errsave;

    if (arg == NULL) {
        fprintf(stderr, "no key/cert name specified for client auth\n");
        return -1;
    }

    key = SECU_FindPrivateKeyFromNickname(arg);
    errsave = PORT_GetError();
    if (!key) {
        if (errsave == SEC_ERROR_BAD_PASSWORD)
            fprintf(stderr, "Bad password\n");
        else if (errsave > 0)
            fprintf(stderr, "Unable to read key (error %d)\n", errsave);
        else if (errsave == SEC_ERROR_BAD_DATABASE)
            fprintf(stderr, "Unable to get key from database (%d)\n", errsave);
        else
            fprintf(stderr, "SECKEY_FindKeyByName: internal error %d\n", errsave);
        return -1;
    }

    cert = PK11_FindCertFromNickname(arg, NULL);
    if (!cert) {
        fprintf(stderr, "Unable to get certificate (%d)\n", PORT_GetError());
        return -1;
    }

    *pRetCert = cert;
    *pRetKey = key;

    return 0;
}

/* XXX should try to preserve underlying os error for fread */
SECStatus
SECU_DER_Read(SECItem *dst, FILE *src)
{
    int rv;
    struct stat stat_buf;

#if defined (LINUX)
    rv = fstat(src->_fileno, &stat_buf);
#else
    rv = fstat(src->_file, &stat_buf);
#endif
    if (rv < 0) {
	PORT_SetError(SEC_ERROR_IO);
	return SECFailure;
    }

    dst->len = stat_buf.st_size;
    dst->data = (unsigned char*) PORT_Alloc(dst->len);
    if (!dst->data) {
      return SECFailure;
    }

    rv = fread(dst->data, 1, dst->len, src);
    if (rv != dst->len) {
	PORT_SetError(SEC_ERROR_IO);
	return SECFailure;
    }

    return SECSuccess;
}



#define INDENT_MULT	4
static void secu_Indent(FILE *out, int level)
{
    int i;
    for (i = 0; i < level; i++) {
	fprintf(out, "    ");
    }
}

static void secu_Newline(FILE *out)
{
    fprintf(out, "\n");
}

void
SECU_PrintAsHex(FILE *out, SECItem *data, char *m, int level)
{
    unsigned i;
    int column;

    if ( m ) {
	secu_Indent(out, level); fprintf(out, "%s:\n", m);
	level++;
    }
    
    secu_Indent(out, level); column = level*INDENT_MULT;
    for (i = 0; i < data->len; i++) {
	if (i != data->len - 1) {
	    fprintf(out, "%02x:", data->data[i]);
	    column += 4;
	} else {
	    fprintf(out, "%02x", data->data[i]);
	    column += 3;
	    break;
	}
	if (column > 76) {
	    secu_Newline(out);
	    secu_Indent(out, level); column = level*INDENT_MULT;
	}
    }
    level--;
    if (column != level*INDENT_MULT) {
	secu_Newline(out);
    }
}

void
SECU_PrintInteger(FILE *out, SECItem *i, char *m, int level)
{
    int iv;

    if (i->len > 4) {
	SECU_PrintAsHex(out, i, m, level);
    } else {
	iv = DER_GetInteger(i);
	secu_Indent(out, level); fprintf(out, "%s: %d (0x%x)\n", m, iv, iv);
    }
}

static void
secu_PrintBoolean(FILE *out, SECItem *i, char *m, int level)
{
    int val = 0;
    
    if ( i->data ) {
	val = i->data[0];
    }

    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    if ( val ) {
	secu_Indent(out, level+1); fprintf(out, "%s\n", "True");
    } else {
	secu_Indent(out, level+1); fprintf(out, "%s\n", "False");
    }    
}

int
SECU_PrintUTCTime(FILE *out, SECItem *t, char *m, int level)
{
    PRTime printableTime; 
    int64 time;
    char *timeString;
    int rv;

    rv = DER_UTCTimeToTime(&time, t);
    if (rv) return rv;

    secu_Indent(out, level);

    /* Converse to local time */
    PR_ExplodeTime(&printableTime, time);
    
    timeString = (char *)PORT_Alloc(100);

    if ( timeString ) {
	PR_FormatTime( timeString, 100, "%a %b %d %H:%M:%S %Y", &printableTime );
	fprintf(out, "%s: %s\n", m, timeString);
	PORT_Free(timeString);
	return 0;
    }
    rv = SECFailure;

}

static void
secu_PrintAny(FILE *out, SECItem *i, char *m, int level)
{
    if ( i->len ) {
	switch (i->data[0] & 0x1f) {
	  case DER_INTEGER:
	    SECU_PrintInteger(out, i, m, level);
	    break;
	  case DER_NULL:
	    secu_Indent(out, level); fprintf(out, "%s: NULL\n", m);
	    break;
	  default:
	    SECU_PrintAsHex(out, i, m, level);
	    break;
	}
    }
}

static int
secu_PrintValidity(FILE *out, CERTValidity *v, char *m, int level)
{
    int rv;

    secu_Indent(out, level);  fprintf(out, "%s:\n", m);
    rv = SECU_PrintUTCTime(out, &v->notBefore, "Not Before", level+1);
    if (rv) return rv;
    SECU_PrintUTCTime(out, &v->notAfter, "Not After", level+1);
    return rv;
}

static void
secu_PrintObjectID(FILE *out, SECItem *oid, char *m, int level)
{
    char *name;
    SECOidData *oiddata;
    
    oiddata = SECOID_FindOID(oid);
    if (oiddata == NULL) {
	SECU_PrintAsHex(out, oid, m, level);
	return;
    }
    name = oiddata->desc;

    secu_Indent(out, level);
    if (m != NULL)
	fprintf(out, "%s: ", m);
    fprintf(out, "%s\n", name);
}

static void
secu_PrintAlgorithmID(FILE *out, SECAlgorithmID *a, char *m, int level)
{
    secu_PrintObjectID(out, &a->algorithm, m, level);

    if ((a->parameters.len != 2) ||
	(PORT_Memcmp(a->parameters.data, "\005\000", 2) != 0)) {
	/* Print args to algorithm */
	SECU_PrintAsHex(out, &a->parameters, "Args", level+1);
    }
}

static void
secu_PrintAttribute(FILE *out, SEC_PKCS7Attribute *attr, char *m, int level)
{
    SECItem *value;
    int i;
    char om[100];

    secu_Indent(out, level); fprintf(out, "%s:\n", m);

    /*
     * XXX Make this smarter; look at the type field and then decode
     * and print the value(s) appropriately!
     */
    secu_PrintObjectID(out, &(attr->type), "Type", level+1);
    if (attr->values != NULL) {
	i = 0;
	while ((value = attr->values[i++]) != NULL) {
	    sprintf(om, "Value (%d)%s", i, attr->encoded ? " (encoded)" : ""); 
	    if (attr->encoded || attr->typeTag == NULL) {
		SECU_PrintAsHex(out, value, om, level+1);
	    } else {
		switch (attr->typeTag->offset) {
		  default:
		    SECU_PrintAsHex(out, value, om, level+1);
		    break;
		  case SEC_OID_PKCS9_CONTENT_TYPE:
		    secu_PrintObjectID(out, value, om, level+1);
		    break;
		  case SEC_OID_PKCS9_SIGNING_TIME:
		    SECU_PrintUTCTime(out, value, om, level+1);
		    break;
		}
	    }
	}
    }
}

static void
secu_PrintRSAPublicKey(FILE *out, SECKEYPublicKey *pk, char *m, int level)
{
#if 0	/*
	 * um, yeah, that might be nice, but if you look at the callers
	 * you will see that they do not *set* this, so this will not work!
	 * Instead, somebody needs to fix the callers to be smarter about
	 * public key stuff, if that is important.
	 */
    PORT_Assert(pk->keyType == rsaKey);
#endif

    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    SECU_PrintInteger(out, &pk->u.rsa.modulus, "Modulus", level+1);
    SECU_PrintInteger(out, &pk->u.rsa.publicExponent, "Exponent", level+1);
}

static void
secu_PrintDSAPublicKey(FILE *out, SECKEYPublicKey *pk, char *m, int level)
{
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    SECU_PrintInteger(out, &pk->u.dsa.params.prime, "Prime", level+1);
    SECU_PrintInteger(out, &pk->u.dsa.params.subPrime, "Subprime", level+1);
    SECU_PrintInteger(out, &pk->u.dsa.params.base, "Base", level+1);
    SECU_PrintInteger(out, &pk->u.dsa.publicValue, "PublicValue", level+1);
}

static int
secu_PrintSubjectPublicKeyInfo(FILE *out, PRArenaPool *arena,
		       CERTSubjectPublicKeyInfo *i,  char *msg, int level)
{
    SECKEYPublicKey *pk;
    int rv;

    secu_Indent(out, level); fprintf(out, "%s:\n", msg);
    secu_PrintAlgorithmID(out, &i->algorithm, "Public Key Algorithm", level+1);

    pk = (SECKEYPublicKey*) PORT_ZAlloc(sizeof(SECKEYPublicKey));
    if (!pk)
	return PORT_GetError();

    DER_ConvertBitString(&i->subjectPublicKey);
    switch(SECOID_FindOIDTag(&i->algorithm.algorithm)) {
      case SEC_OID_PKCS1_RSA_ENCRYPTION:
	rv = SEC_ASN1DecodeItem(arena, pk, SECKEY_RSAPublicKeyTemplate,
				&i->subjectPublicKey);
	if (rv)
	    return rv;
	secu_PrintRSAPublicKey(out, pk, "RSA Public Key", level +1);
	break;
      case SEC_OID_ANSIX9_DSA_SIGNATURE:
	rv = SEC_ASN1DecodeItem(arena, pk, SECKEY_DSAPublicKeyTemplate,
				&i->subjectPublicKey);
	if (rv)
	    return rv;
	secu_PrintDSAPublicKey(out, pk, "DSA Public Key", level +1);
	break;
      default:
	fprintf(out, "bad SPKI algorithm type\n");
	return 0;
    }

    return 0;
}

static SECStatus
PrintInvalidDateExten  (FILE *out, SECItem *value, char *msg, int level)
{
    SECItem decodedValue;
    SECStatus rv;
    int64 invalidTime;
    char *formattedTime = NULL;

    decodedValue.data = NULL;
    rv = SEC_ASN1DecodeItem (NULL, &decodedValue, SEC_GeneralizedTimeTemplate,
			    value);
    if (rv == SECSuccess) {
	rv = DER_GeneralizedTimeToTime(&invalidTime, &decodedValue);
	if (rv == SECSuccess) {
	    formattedTime = CERT_GenTime2FormattedAscii
			    (invalidTime, "%a %b %d %H:%M:%S %Y");
	    secu_Indent(out, level +1);
	    fprintf (out, "%s: %s\n", msg, formattedTime);
	    PORT_Free (formattedTime);
	}
    }
    PORT_Free (decodedValue.data);
    return (rv);
}

static int
secu_PrintExtensions(FILE *out, CERTCertExtension **extensions,
		     char *msg, int level)
{
    SECOidData *oid;
    SECOidTag oidTag;
    
    if ( extensions ) {
	secu_Indent(out, level); fprintf(out, "%s:\n", msg);
	
	while ( *extensions ) {
	    SECItem *tmpitem;
	    secu_Indent(out, level+1); fprintf(out, "Name:\n");

	    tmpitem = &(*extensions)->id;
	    secu_PrintObjectID(out, tmpitem, NULL, level+2);

	    tmpitem = &(*extensions)->critical;
	    if ( tmpitem->len ) {
		secu_PrintBoolean(out, tmpitem, "Critical", level+1);
	    }

	    oidTag = SECOID_FindOIDTag (&((*extensions)->id));

	    tmpitem = &((*extensions)->value);
	    if (oidTag == SEC_OID_X509_INVALID_DATE) 
		PrintInvalidDateExten  (out, tmpitem,"", level + 1);
            else			    
		SECU_PrintAsHex(out,tmpitem, "Data", level+1);		    
		    
	    secu_Newline(out);
	    extensions++;
	}
    }
    
    return 0;
}


static int
secu_PrintName(FILE *out, CERTName *name, char *msg, int level)
{
    CERTRDN **rdns, *rdn;
    CERTAVA **avas, *ava;
    char *tagName;
    int first, tag;
    char *str;
    
    secu_Indent(out, level); fprintf(out, "%s: ", msg);

    str = CERT_NameToAscii(name);
    fprintf(out, str);
	
    secu_Newline(out);
    return 0;
}

static SECStatus
secu_PrintKeyNickname(DBT *k, DBT *d, void *data)
{
    FILE *out;
    char *name;
    unsigned char *buf;

    buf = (unsigned char *)d->data;
    out = (FILE *)data;

    name = (char *)PORT_Alloc(buf[2]);
    if (name == NULL) {
	return(SECFailure);
    }

    PORT_Memcpy(name, (buf + 3 + buf[1]), buf[2]);

    /* print everything but password-check entry */
    if (PORT_Strcmp(name, "password-check") != 0) {
	fprintf(out, "%s\n", name);
    }
    PORT_Free(name);

    return (SECSuccess);
}

int
SECU_PrintKeyNames(SECKEYKeyDBHandle *handle, FILE *out)
{
    int rv;

    secu_Indent(out, 0);
    fprintf(out, "Version %d database\n\n", handle->version);
    fprintf(out, "Key Name\n--------\n");
    rv = SECKEY_TraverseKeys(handle, secu_PrintKeyNickname, out);
    if (rv) {
	return -1;
    }

    return 0;
}

void
printflags(char *trusts, unsigned int flags)
{
    if (flags & CERTDB_VALID_CA)
	if (!(flags & CERTDB_TRUSTED_CA) &&
	    !(flags & CERTDB_TRUSTED_CLIENT_CA))
	    XP_STRCAT(trusts, "c");
    if (flags & CERTDB_VALID_PEER)
	if (!(flags & CERTDB_TRUSTED))
	    XP_STRCAT(trusts, "p");
    if (flags & CERTDB_TRUSTED_CA)
	XP_STRCAT(trusts, "C");
    if (flags & CERTDB_TRUSTED_CLIENT_CA)
	XP_STRCAT(trusts, "T");
    if (flags & CERTDB_TRUSTED)
	XP_STRCAT(trusts, "P");
    if (flags & CERTDB_USER)
	XP_STRCAT(trusts, "u");
    if (flags & CERTDB_SEND_WARN)
	XP_STRCAT(trusts, "w");
    if (flags & CERTDB_INVISIBLE_CA)
	XP_STRCAT(trusts, "I");
    if (flags & CERTDB_GOVT_APPROVED_CA)
	XP_STRCAT(trusts, "G");
    return;
}

static SECStatus
secu_PrintCertNickname(CERTCertificate *cert, SECItem *k, void *data)
{
    CERTCertTrust *trust;
    FILE *out;
    char trusts[30];
    char *name;

    PORT_Memset (trusts, 0, sizeof (trusts));
    out = (FILE *)data;
    
    if ( cert->dbEntry ) {
	name = cert->dbEntry->nickname;
	if ( name == NULL ) {
	    name = cert->emailAddr;
	}
	
        trust = &cert->dbEntry->trust;
	printflags(trusts, trust->sslFlags);
	XP_STRCAT(trusts, ",");
	printflags(trusts, trust->emailFlags);
	XP_STRCAT(trusts, ",");
	printflags(trusts, trust->objectSigningFlags);
	fprintf(out, "%-35s %-5s\n", name, trusts);
    }

    return (SECSuccess);
}

int
SECU_PrintCertificateNames(CERTCertDBHandle *handle, FILE *out)
{
    int rv;

    secu_Indent(out, 0);
    fprintf(out, "\n%-30s %-5s\n\n", "Certificate Name", "Trust Attributes");
    rv = SEC_TraversePermCerts(handle, secu_PrintCertNickname, out);
    if (rv)
	return -1;

    fprintf(out, "\n");
    fprintf(out, "p    Valid peer\n");
    fprintf(out, "P    Trusted peer (implies p)\n");
    fprintf(out, "c    Valid CA\n");
    fprintf(out, "T    Trusted CA to issue client certs (implies c)\n");
    fprintf(out, "C    Trusted CA to certs(only server certs for ssl) (implies c)\n");
    fprintf(out, "u    User cert\n");
    fprintf(out, "w    Send warning\n");
    return 0;
}

int
SECU_PrintCertificateRequest(FILE *out, SECItem *der, char *m, int level)
{
    PRArenaPool *arena = NULL;
    CERTCertificateRequest *cr;
    int rv;

    /* Decode certificate request */
    cr = (CERTCertificateRequest*) PORT_ZAlloc(sizeof(CERTCertificateRequest));
    if (!cr)
	return PORT_GetError();

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena)
	return DS_ERROR_OUT_OF_MEMORY;

    rv = DER_Decode(arena, cr, CERTCertificateRequestTemplate, der);
    if (rv) {
	PORT_FreeArena(arena, PR_FALSE);
	return rv;
    }

    /* Pretty print it out */
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    SECU_PrintInteger(out, &cr->version, "Version", level+1);
    secu_PrintName(out, &cr->subject, "Subject", level+1);
    rv = secu_PrintSubjectPublicKeyInfo(out, arena, &cr->subjectPublicKeyInfo,
			      "Subject Public Key Info", level+1);
    if (rv) {
	PORT_FreeArena(arena, PR_FALSE);
	return rv;
    }
    secu_PrintAny(out, &cr->attributes, "Attributes", level+1);

    PORT_FreeArena(arena, PR_FALSE);
    return 0;
}

int
SECU_PrintCertificate(FILE *out, SECItem *der, char *m, int level)
{
    PRArenaPool *arena = NULL;
    CERTCertificate *c;
    int rv;
    int iv;
    
    /* Decode certificate */
    c = (CERTCertificate*) PORT_ZAlloc(sizeof(CERTCertificate));
    if (!c)
	return PORT_GetError();

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena)
	return DS_ERROR_OUT_OF_MEMORY;

    rv = DER_Decode(arena, c, CERTCertificateTemplate, der);
    if (rv) {
	PORT_FreeArena(arena, PR_FALSE);
	return rv;
    }

    /* Pretty print it out */
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    iv = DER_GetInteger(&c->version);
    secu_Indent(out, level+1); fprintf(out, "%s: %d (0x%x)\n", "Version", iv + 1, iv);

    SECU_PrintInteger(out, &c->serialNumber, "Serial Number", level+1);
    secu_PrintAlgorithmID(out, &c->signature, "Signature Algorithm", level+1);
    secu_PrintName(out, &c->issuer, "Issuer", level+1);
    secu_PrintValidity(out, &c->validity, "Validity", level+1);
    secu_PrintName(out, &c->subject, "Subject", level+1);
    rv = secu_PrintSubjectPublicKeyInfo(out, arena, &c->subjectPublicKeyInfo,
			      "Subject Public Key Info", level+1);
    if (rv) {
	PORT_FreeArena(arena, PR_FALSE);
	return rv;
    }
    secu_PrintExtensions(out, c->extensions, "Signed Extensions", level+1);
    
    PORT_FreeArena(arena, PR_FALSE);
    return 0;
}

int
SECU_PrintPublicKey(FILE *out, SECItem *der, char *m, int level)
{
    PRArenaPool *arena = NULL;
    SECKEYPublicKey key;
    int rv;

    PORT_Memset(&key, 0, sizeof(key));
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena)
	return DS_ERROR_OUT_OF_MEMORY;

    rv = SEC_ASN1DecodeItem(arena, &key, SECKEY_RSAPublicKeyTemplate, der);
    if (rv) {
	PORT_FreeArena(arena, PR_FALSE);
	return rv;
    }

    /* Pretty print it out */
    secu_PrintRSAPublicKey(out, &key, m, level);

    PORT_FreeArena(arena, PR_FALSE);
    return 0;
}

int
SECU_PrintPrivateKey(FILE *out, SECItem *der, char *m, int level)
{
    PRArenaPool *arena = NULL;
    SECKEYEncryptedPrivateKeyInfo key;
    int rv;

    PORT_Memset(&key, 0, sizeof(key));
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena)
	return DS_ERROR_OUT_OF_MEMORY;

    rv = SEC_ASN1DecodeItem(arena, &key, SECKEY_EncryptedPrivateKeyInfoTemplate,
			    der);
    if (rv) {
	PORT_FreeArena(arena, PR_TRUE);
	return rv;
    }

    /* Pretty print it out */
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    secu_PrintAlgorithmID(out, &key.algorithm, "Encryption Algorithm", 
			  level+1);
    SECU_PrintAsHex(out, &key.encryptedData, "Encrypted Data", level+1);

    PORT_FreeArena(arena, PR_TRUE);
    return 0;
}

/*
** PKCS7 Support
*/

/* forward declaration */
static int
secu_PrintPKCS7ContentInfo(FILE *, SEC_PKCS7ContentInfo *, char *, int);

/*
** secu_PrintPKCS7EncContent
**   Prints a SEC_PKCS7EncryptedContentInfo (without decrypting it)
*/
static void
secu_PrintPKCS7EncContent(FILE *out, SEC_PKCS7EncryptedContentInfo *src, 
			  char *m, int level)
{
    if (src->contentTypeTag == NULL)
	src->contentTypeTag = SECOID_FindOID(&(src->contentType));

    secu_Indent(out, level);
    fprintf(out, "%s:\n", m);
    secu_Indent(out, level + 1); 
    fprintf(out, "Content Type: %s\n",
	    (src->contentTypeTag != NULL) ? src->contentTypeTag->desc
					  : "Unknown");
    secu_PrintAlgorithmID(out, &(src->contentEncAlg),
			  "Content Encryption Algorithm", level+1);
    SECU_PrintAsHex(out, &(src->encContent), 
		    "Encrypted Content", level+1);
}

/*
** secu_PrintRecipientInfo
**   Prints a PKCS7RecipientInfo type
*/
static void
secu_PrintRecipientInfo(FILE *out, SEC_PKCS7RecipientInfo *info, char *m, 
			int level)
{
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    SECU_PrintInteger(out, &(info->version), "Version", level + 1);	

    secu_PrintName(out, &(info->issuerAndSN->issuer), "Issuer", 
		 level + 1);
    SECU_PrintInteger(out, &(info->issuerAndSN->serialNumber), 
		      "Serial Number", level + 1);

    /* Parse and display encrypted key */
    secu_PrintAlgorithmID(out, &(info->keyEncAlg), 
			"Key Encryption Algorithm", level + 1);
    SECU_PrintAsHex(out, &(info->encKey), "Encrypted Key", level + 1);
}

/* 
** secu_PrintSignerInfo
**   Prints a PKCS7SingerInfo type
*/
static void
secu_PrintSignerInfo(FILE *out, SEC_PKCS7SignerInfo *info, char *m, int level)
{
    SEC_PKCS7Attribute *attr;
    SECItem *value;
    int iv;
    char om[100];
    
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    SECU_PrintInteger(out, &(info->version), "Version", level + 1);	

    secu_PrintName(out, &(info->issuerAndSN->issuer), "Issuer", 
		 level + 1);
    SECU_PrintInteger(out, &(info->issuerAndSN->serialNumber), 
		      "Serial Number", level + 1);
  
    secu_PrintAlgorithmID(out, &(info->digestAlg), "Digest Algorithm",
			  level + 1);
    
    if (info->authAttr != NULL) {
	secu_Indent(out, level + 1); 
	fprintf(out, "Authenticated Attributes:\n");
	iv = 0;
	while ((attr = info->authAttr[iv++]) != NULL) {
	    sprintf(om, "Attribute (%d)", iv); 
	    secu_PrintAttribute(out, attr, om, level + 2);
	}
    }
    
    /* Parse and display signature */
    secu_PrintAlgorithmID(out, &(info->digestEncAlg), 
			"Digest Encryption Algorithm", level + 1);
    SECU_PrintAsHex(out, &(info->encDigest), "Encrypted Digest", level + 1);
    
    if (info->unAuthAttr != NULL) {
	secu_Indent(out, level + 1); 
	fprintf(out, "Unauthenticated Attributes:\n");
	iv = 0;
	while ((attr = info->unAuthAttr[iv++]) != NULL) {
	    sprintf(om, "Attribute (%x)", iv); 
	    secu_PrintAttribute(out, attr, om, level + 2);
	}
    }
}

void
SECU_PrintCRLInfo(FILE *out, CERTCrl *crl, char *m, int level)
{
    CERTCrlEntry *entry;
    int iv;
    char om[100];
    
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    secu_PrintAlgorithmID(out, &(crl->signatureAlg), "Signature Algorithm",
			  level + 1);
    secu_PrintName(out, &(crl->name), "Name", level + 1);
    SECU_PrintUTCTime(out, &(crl->lastUpdate), "Last Update", level + 1);
    SECU_PrintUTCTime(out, &(crl->nextUpdate), "Next Update", level + 1);
    
    if (crl->entries != NULL) {
	iv = 0;
	while ((entry = crl->entries[iv++]) != NULL) {
	    sprintf(om, "Entry (%x):\n", iv); 
	    secu_Indent(out, level + 1); fprintf(out, om);
	    SECU_PrintInteger(out, &(entry->serialNumber), "Serial Number",
			      level + 2);
	    SECU_PrintUTCTime(out, &(entry->revocationDate), "Revocation Date",
			      level + 2);
	    secu_PrintExtensions
		   (out, entry->extensions, "Signed CRL Entries Extensions", level + 1);
	}
    }
    secu_PrintExtensions
	   (out, crl->extensions, "Signed CRL Extension", level + 1);
}

/*
** secu_PrintPKCS7Signed
**   Pretty print a PKCS7 signed data type (up to version 1).
*/
static int
secu_PrintPKCS7Signed(FILE *out, SEC_PKCS7SignedData *src, char *m, int level)
{
    SECAlgorithmID *digAlg;		/* digest algorithms */
    SECItem *aCert;			/* certificate */
    CERTSignedCrl *aCrl;		/* certificate revocation list */
    SEC_PKCS7SignerInfo *sigInfo;	/* signer information */
    int rv, iv;
    char om[100];

    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    SECU_PrintInteger(out, &(src->version), "Version", level + 1);

    /* Parse and list digest algorithms (if any) */
    if (src->digestAlgorithms != NULL) {
	secu_Indent(out, level + 1);  fprintf(out, "Digest Algorithm List:\n");
	iv = 0;
	while ((digAlg = src->digestAlgorithms[iv++]) != NULL) {
	    sprintf(om, "Digest Algorithm (%x)", iv);
	    secu_PrintAlgorithmID(out, digAlg, om, level + 2);
	}
    }

    /* Now for the content */
    rv = secu_PrintPKCS7ContentInfo(out, &(src->contentInfo), 
				    "Content Information", level + 1);
    if (rv != 0)
	return rv;

    /* Parse and list certificates (if any) */
    if (src->rawCerts != NULL) {
	secu_Indent(out, level + 1);  fprintf(out, "Certificate List:\n");
	iv = 0;
	while ((aCert = src->rawCerts[iv++]) != NULL) {
	    sprintf(om, "Certificate (%x)", iv);
	    rv = SECU_PrintSignedData(out, aCert, om, level + 2, 
				      SECU_PrintCertificate);
	    if (rv)
		return rv;
	}
    }

    /* Parse and list CRL's (if any) */
    if (src->crls != NULL) {
	secu_Indent(out, level + 1);  
	fprintf(out, "Signed Revocation Lists:\n");
	iv = 0;
	while ((aCrl = src->crls[iv++]) != NULL) {
	    sprintf(om, "Signed Revocation List (%x)", iv);
	    secu_Indent(out, level + 2);  fprintf(out, "%s:\n", om);
	    secu_PrintAlgorithmID(out, &aCrl->signatureWrap.signatureAlgorithm, 
				  "Signature Algorithm", level+3);
	    DER_ConvertBitString(&aCrl->signatureWrap.signature);
	    SECU_PrintAsHex(out, &aCrl->signatureWrap.signature, "Signature",
			    level+3);
	    SECU_PrintCRLInfo(out, &aCrl->crl, "Certificate Revocation List", 
			  level + 3); 
	}
    }

    /* Parse and list signatures (if any) */
    if (src->signerInfos != NULL) {
	secu_Indent(out, level + 1);
	fprintf(out, "Signer Information List:\n");
	iv = 0;
	while ((sigInfo = src->signerInfos[iv++]) != NULL) {
	    sprintf(om, "Signer Information (%x)", iv);
	    secu_PrintSignerInfo(out, sigInfo, om, level + 2);
	}
    }  

    return 0;
}

/*
** secu_PrintPKCS7Enveloped
**  Pretty print a PKCS7 enveloped data type (up to version 1).
*/
static void
secu_PrintPKCS7Enveloped(FILE *out, SEC_PKCS7EnvelopedData *src,
			 char *m, int level)
{
    SEC_PKCS7RecipientInfo *recInfo;   /* pointer for signer information */
    int iv;
    char om[100];

    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    SECU_PrintInteger(out, &(src->version), "Version", level + 1);

    /* Parse and list recipients (this is not optional) */
    if (src->recipientInfos != NULL) {
	secu_Indent(out, level + 1);
	fprintf(out, "Recipient Information List:\n");
	iv = 0;
	while ((recInfo = src->recipientInfos[iv++]) != NULL) {
	    sprintf(om, "Recipient Information (%x)", iv);
	    secu_PrintRecipientInfo(out, recInfo, om, level + 2);
	}
    }  

    secu_PrintPKCS7EncContent(out, &src->encContentInfo, 
			      "Encrypted Content Information", level + 1);
}

/*
** secu_PrintPKCS7SignedEnveloped
**   Pretty print a PKCS7 singed and enveloped data type (up to version 1).
*/
static int
secu_PrintPKCS7SignedAndEnveloped(FILE *out,
				  SEC_PKCS7SignedAndEnvelopedData *src,
				  char *m, int level)
{
    SECAlgorithmID *digAlg;  /* pointer for digest algorithms */
    SECItem *aCert;           /* pointer for certificate */
    CERTSignedCrl *aCrl;        /* pointer for certificate revocation list */
    SEC_PKCS7SignerInfo *sigInfo;   /* pointer for signer information */
    SEC_PKCS7RecipientInfo *recInfo; /* pointer for recipient information */
    int rv, iv;
    char om[100];

    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    SECU_PrintInteger(out, &(src->version), "Version", level + 1);

    /* Parse and list recipients (this is not optional) */
    if (src->recipientInfos != NULL) {
	secu_Indent(out, level + 1);
	fprintf(out, "Recipient Information List:\n");
	iv = 0;
	while ((recInfo = src->recipientInfos[iv++]) != NULL) {
	    sprintf(om, "Recipient Information (%x)", iv);
	    secu_PrintRecipientInfo(out, recInfo, om, level + 2);
	}
    }  

    /* Parse and list digest algorithms (if any) */
    if (src->digestAlgorithms != NULL) {
	secu_Indent(out, level + 1);  fprintf(out, "Digest Algorithm List:\n");
	iv = 0;
	while ((digAlg = src->digestAlgorithms[iv++]) != NULL) {
	    sprintf(om, "Digest Algorithm (%x)", iv);
	    secu_PrintAlgorithmID(out, digAlg, om, level + 2);
	}
    }

    secu_PrintPKCS7EncContent(out, &src->encContentInfo, 
			      "Encrypted Content Information", level + 1);

    /* Parse and list certificates (if any) */
    if (src->rawCerts != NULL) {
	secu_Indent(out, level + 1);  fprintf(out, "Certificate List:\n");
	iv = 0;
	while ((aCert = src->rawCerts[iv++]) != NULL) {
	    sprintf(om, "Certificate (%x)", iv);
	    rv = SECU_PrintSignedData(out, aCert, om, level + 2, 
				      SECU_PrintCertificate);
	    if (rv)
		return rv;
	}
    }

    /* Parse and list CRL's (if any) */
    if (src->crls != NULL) {
	secu_Indent(out, level + 1);  
	fprintf(out, "Signed Revocation Lists:\n");
	iv = 0;
	while ((aCrl = src->crls[iv++]) != NULL) {
	    sprintf(om, "Signed Revocation List (%x)", iv);
	    secu_Indent(out, level + 2);  fprintf(out, "%s:\n", om);
	    secu_PrintAlgorithmID(out, &aCrl->signatureWrap.signatureAlgorithm, 
				  "Signature Algorithm", level+3);
	    DER_ConvertBitString(&aCrl->signatureWrap.signature);
	    SECU_PrintAsHex(out, &aCrl->signatureWrap.signature, "Signature",
			    level+3);
	    SECU_PrintCRLInfo(out, &aCrl->crl, "Certificate Revocation List", 
			  level + 3); 
	}
    }

    /* Parse and list signatures (if any) */
    if (src->signerInfos != NULL) {
	secu_Indent(out, level + 1);
	fprintf(out, "Signer Information List:\n");
	iv = 0;
	while ((sigInfo = src->signerInfos[iv++]) != NULL) {
	    sprintf(om, "Signer Information (%x)", iv);
	    secu_PrintSignerInfo(out, sigInfo, om, level + 2);
	}
    }  

    return 0;
}

int
SECU_PrintCrl (FILE *out, SECItem *der, char *m, int level)
{
    PRArenaPool *arena = NULL;
    CERTCrl *c = NULL;
    int rv;

    do {
	/* Decode CRL */
	c = (CERTCrl*) PORT_ZAlloc(sizeof(CERTCrl));
	if (!c) {
	    rv = PORT_GetError();
	    break;
	}

	arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
	if (!arena) {
	    rv = DS_ERROR_OUT_OF_MEMORY;
	    break;
	}

	rv = SEC_ASN1DecodeItem(arena, c, CERT_CrlTemplate, der);
	if (rv != SECSuccess)
	    break;
	SECU_PrintCRLInfo (out, c, m, level);
    } while (0);
    PORT_FreeArena (arena, PR_FALSE);
    PORT_Free (c);
    return (rv);
}


/*
** secu_PrintPKCS7Encrypted
**   Pretty print a PKCS7 encrypted data type (up to version 1).
*/
static void
secu_PrintPKCS7Encrypted(FILE *out, SEC_PKCS7EncryptedData *src,
			 char *m, int level)
{
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    SECU_PrintInteger(out, &(src->version), "Version", level + 1);

    secu_PrintPKCS7EncContent(out, &src->encContentInfo, 
			      "Encrypted Content Information", level + 1);
}

/*
** secu_PrintPKCS7Digested
**   Pretty print a PKCS7 digested data type (up to version 1).
*/
static void
secu_PrintPKCS7Digested(FILE *out, SEC_PKCS7DigestedData *src,
			char *m, int level)
{
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    SECU_PrintInteger(out, &(src->version), "Version", level + 1);
    
    secu_PrintAlgorithmID(out, &src->digestAlg, "Digest Algorithm",
			  level + 1);
    secu_PrintPKCS7ContentInfo(out, &src->contentInfo, "Content Information",
			       level + 1);
    SECU_PrintAsHex(out, &src->digest, "Digest", level + 1);  
}

/*
** secu_PrintPKCS7ContentInfo
**   Takes a SEC_PKCS7ContentInfo type and sends the contents to the 
** appropriate function
*/
static int
secu_PrintPKCS7ContentInfo(FILE *out, SEC_PKCS7ContentInfo *src,
			   char *m, int level)
{
    char *desc;
    SECOidTag kind;
    int rv;

    secu_Indent(out, level);  fprintf(out, "%s:\n", m);
    level++;

    if (src->contentTypeTag == NULL)
	src->contentTypeTag = SECOID_FindOID(&(src->contentType));

    if (src->contentTypeTag == NULL) {
	desc = "Unknown";
	kind = SEC_OID_PKCS7_DATA;
    } else {
	desc = src->contentTypeTag->desc;
	kind = src->contentTypeTag->offset;
    }

    if (src->content.data == NULL) {
	secu_Indent(out, level); fprintf(out, "%s:\n", desc);
	level++;
	secu_Indent(out, level); fprintf(out, "<no content>\n");
	return 0;
    }

    rv = 0;
    switch (kind) {
      case SEC_OID_PKCS7_SIGNED_DATA:  /* Signed Data */
	rv = secu_PrintPKCS7Signed(out, src->content.signedData, desc, level);
	break;

      case SEC_OID_PKCS7_ENVELOPED_DATA:  /* Enveloped Data */
        secu_PrintPKCS7Enveloped(out, src->content.envelopedData, desc, level);
	break;

      case SEC_OID_PKCS7_SIGNED_ENVELOPED_DATA:  /* Signed and Enveloped */
	rv = secu_PrintPKCS7SignedAndEnveloped(out,
					src->content.signedAndEnvelopedData,
					desc, level);
	break;

      case SEC_OID_PKCS7_DIGESTED_DATA:  /* Digested Data */
	secu_PrintPKCS7Digested(out, src->content.digestedData, desc, level);
	break;

      case SEC_OID_PKCS7_ENCRYPTED_DATA:  /* Encrypted Data */
	secu_PrintPKCS7Encrypted(out, src->content.encryptedData, desc, level);
	break;

      default:
	SECU_PrintAsHex(out, src->content.data, desc, level);
	break;
    }

    return rv;
}

/*
** SECU_PrintPKCS7ContentInfo
**   Decode and print any major PKCS7 data type (up to version 1).
*/
int
SECU_PrintPKCS7ContentInfo(FILE *out, SECItem *der, char *m, int level)
{
    SEC_PKCS7ContentInfo *cinfo;
    int rv;

    cinfo = SEC_PKCS7DecodeItem(der, NULL, NULL, NULL, NULL);
    if (cinfo != NULL) {
	/* Send it to recursive parsing and printing module */
	rv = secu_PrintPKCS7ContentInfo(out, cinfo, m, level);
	SEC_PKCS7DestroyContentInfo(cinfo);
    } else {
	rv = -1;
    }

    return rv;
}

/*
** End of PKCS7 functions
*/

void
printFlags(FILE *out, unsigned int flags, int level)
{
    if ( flags & CERTDB_VALID_PEER ) {
	secu_Indent(out, level); fprintf(out, "Valid Peer\n");
    }
    if ( flags & CERTDB_TRUSTED ) {
	secu_Indent(out, level); fprintf(out, "Trusted\n");
    }
    if ( flags & CERTDB_SEND_WARN ) {
	secu_Indent(out, level); fprintf(out, "Warn When Sending\n");
    }
    if ( flags & CERTDB_VALID_CA ) {
	secu_Indent(out, level); fprintf(out, "Valid CA\n");
    }
    if ( flags & CERTDB_TRUSTED_CA ) {
	secu_Indent(out, level); fprintf(out, "Trusted CA\n");
    }
    if ( flags & CERTDB_NS_TRUSTED_CA ) {
	secu_Indent(out, level); fprintf(out, "Netscape Trusted CA\n");
    }
    if ( flags & CERTDB_USER ) {
	secu_Indent(out, level); fprintf(out, "User\n");
    }
    if ( flags & CERTDB_TRUSTED_CLIENT_CA ) {
	secu_Indent(out, level); fprintf(out, "Trusted Client CA\n");
    }
}

void
SECU_PrintTrustFlags(FILE *out, CERTCertTrust *trust, char *m, int level)
{
    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    secu_Indent(out, level+1); fprintf(out, "SSL Flags:\n");
    printFlags(out, trust->sslFlags, level+2);
    secu_Indent(out, level+1); fprintf(out, "Email Flags:\n");
    printFlags(out, trust->emailFlags, level+2);
    secu_Indent(out, level+1); fprintf(out, "Object Signing Flags:\n");
    printFlags(out, trust->objectSigningFlags, level+2);
}

int SECU_PrintSignedData(FILE *out, SECItem *der, char *m,
			   int level, SECU_PPFunc inner)
{
    PRArenaPool *arena = NULL;
    CERTSignedData *sd;
    int rv;

    /* Strip off the signature */
    sd = (CERTSignedData*) PORT_ZAlloc(sizeof(CERTSignedData));
    if (!sd)
	return PORT_GetError();

    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (!arena)
	return DS_ERROR_OUT_OF_MEMORY;

    rv = DER_Decode(arena, sd, CERTSignedDataTemplate, der);
    if (rv) {
	PORT_FreeArena(arena, PR_FALSE);
	return rv;
    }

    secu_Indent(out, level); fprintf(out, "%s:\n", m);
    rv = (*inner)(out, &sd->data, "Data", level+1);
    if (rv) {
	PORT_FreeArena(arena, PR_FALSE);
	return rv;
    }

    secu_PrintAlgorithmID(out, &sd->signatureAlgorithm, "Signature Algorithm",
			  level+1);
    DER_ConvertBitString(&sd->signature);
    SECU_PrintAsHex(out, &sd->signature, "Signature", level+1);

    PORT_FreeArena(arena, PR_FALSE);
    return 0;

}

static char *secu_AppendStr(char *in, char *append)
{
    int alen, inlen;

    alen = PORT_Strlen(append);
    if (in) {
	inlen = PORT_Strlen(in);
	in = (char*) PORT_Realloc(in,inlen+alen+1);
	PORT_Memcpy(in+inlen, append, alen+1);
    } else {
	in = (char*) PORT_Alloc(alen+1);
	PORT_Memcpy(in, append, alen+1);
    }
    return in;
}

char *SECU_FileToString(FILE *fp)
{
    char buf[1000], *cp, *s = 0;

    for (;;) {
	if (feof(fp)) break;
	cp = fgets(buf, sizeof(buf), fp);
	if (!cp) {
	    if (feof(fp)) break;
	    PORT_SetError(errno);
	    return 0;
	}
	s = secu_AppendStr(s, buf);
    }
    return s;
}

SECStatus
SECU_PKCS11Init(void) {
    static PRBool isInit = PR_FALSE;
    CERTCertDBHandle *cdb_handle;
    char *certdbname;
    char *keydbname;
    SECKEYKeyDBHandle *kdb_handle;
    char *secmodule;
    SECStatus rv;


    if (isInit) return SECSuccess;

    isInit = PR_TRUE;

    /*
     * Initialize the private key database.
     */
    /* in the "ideal world"(tm), the Key database would be initialized in
     * the Software PKCS#11 module. Unfortunately there's not enough of
     * an interface to pass all the info to the PCKS#11 module, so go ahead
     * and initialize here...
     */
    RNG_RNGInit(); /* #$@$#@ Guess what SECKEY_OpenKeyDB calls if there is
		    * no keyDB? You got it Get Random Data... just one more
		    * reason to want to move this call into pkcs11.c
		    */

    keydbname = SECU_DatabaseFileName(xpKeyDB);
    
    kdb_handle = SECKEY_OpenKeyDB(PR_FALSE, SECU_KeyDBNameCallback, NULL);
    
    if (kdb_handle != NULL) {
	SECKEY_SetDefaultKeyDB(kdb_handle);
    }

    /*
     * set our default password function
     */
    PK11_SetPasswordFunc(SECU_GetModulePassword);
    /*
     * OK, now we initialize the PKCS11 subsystems
     */
    secmodule = SECU_DatabaseFileName(xpSecModuleDB);
    SECMOD_init(secmodule);
    return SECSuccess; /* XXX check the return codes?? */
}

SECKEYLowPublicKey *SECU_ConvHighToLow(SECKEYPublicKey *pubk) 
{
    SECKEYLowPublicKey *copyk;
    PRArenaPool *arena;
    
    arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
    if (arena == NULL) {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
	return NULL;
    }

    copyk = (SECKEYLowPublicKey *) 
			PORT_ArenaZAlloc (arena, sizeof (SECKEYLowPublicKey));
    if (copyk != NULL) {
	SECStatus rv = SECSuccess;

	copyk->arena = arena;
	copyk->keyType = pubk->keyType;
	switch (pubk->keyType) {
	  case rsaKey:
	    rv = SECITEM_CopyItem(arena, &copyk->u.rsa.modulus,
				  &pubk->u.rsa.modulus);
	    if (rv == SECSuccess) {
		rv = SECITEM_CopyItem (arena, &copyk->u.rsa.publicExponent,
				       &pubk->u.rsa.publicExponent);
		if (rv == SECSuccess)
		    return copyk;
	    }
	    break;
	  case nullKey:
	    return copyk;
	  default:
	    rv = SECFailure;
	    break;
	}
	if (rv == SECSuccess)
	    return copyk;

	SECKEY_LowDestroyPublicKey (copyk);
    } else {
	PORT_SetError (SEC_ERROR_NO_MEMORY);
    }

    PORT_FreeArena (arena, PR_FALSE);
    return NULL;
}


#ifdef AIX
int _OS_SELECT (int nfds, void *readfds, void *writefds,
               void *exceptfds, struct timeval *timeout) {
   return select (nfds,readfds,writefds,exceptfds,timeout);
}
#endif 

SECItem *
SECU_GetPBEPassword(void *arg)
{
    char *p = NULL;
    SECStatus rv;
    SECItem *pwitem;
    FILE *input, *output;

    p = SECU_GetPasswordString(arg,"Password: ");


    if ( pwitem == NULL ) {
	fprintf(stderr, "Error hashing password\n");
	return NULL;
    }
    
    return pwitem;
}
