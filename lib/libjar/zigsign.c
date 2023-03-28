/*
 *  ZIGSIGN
 *
 *  Routines used in signing archives.
 *
 *  Copyright 1996 Netscape Corporation
 * 
 */

#define USE_MOZ_ENCODE

#include "zig.h"

#include "cert.h"
#include "key.h"
#include "secpkcs7.h"
#include "base64.h"
#include "pk11func.h"

#include "xp_error.h"
#include "xpgetstr.h"
/* #include "allxpstr.h" */

#ifdef XP_UNIX
extern int errno;
#endif

#ifndef AKBAR
/* from libevent.h */
typedef void (*ETVoidPtrFunc) (void * data);
extern void ET_moz_CallFunction (ETVoidPtrFunc fn, void *data);
#endif /* !AKBAR */

/* from proto.h */
extern MWContext *XP_FindSomeContext(void);

/* imported from zigver.c */

extern CERTCertDBHandle *SOB_open_database (void);
extern int SOB_close_database (CERTCertDBHandle *certdb);

/* key database wrapper */

static SECKEYKeyDBHandle *sob_open_key_database (void);
static int sob_close_key_database (SECKEYKeyDBHandle *keydb);

/* pkcs7 code signing */

static int sob_create_pk7 
   (CERTCertDBHandle *certdb, SECKEYKeyDBHandle *keydb, 
       CERTCertificate *cert, char *password, XP_File infp, XP_File outfp);

#ifdef USE_MOZ_ENCODE
SECStatus sob_moz_encode
      (
      SEC_PKCS7ContentInfo *cinfo,
      SEC_PKCS7EncoderOutputCallback  outputfn,
      void *outputarg,
      PK11SymKey *bulkkey,
      SECKEYGetPasswordKey pwfn,
      void *pwfnarg
      );
#endif

/* CHUNQ is our bite size */

#define CHUNQ 64000
#define FILECHUNQ 32768

/*
 *  S O B _ c a l c u l a t e _ d i g e s t 
 *
 *  Quick calculation of a digest for
 *  the specified block of memory. Will calculate
 *  for all supported algorithms, now MD5.
 *
 *  This version supports huge pointers for WIN16.
 * 
 */

DIGESTS * PR_CALLBACK SOB_calculate_digest (void XP_HUGE *data, long length)
  {
  DIGESTS *dig;
  long chunq;

  PK11Context *md5  = 0;
  PK11Context *sha1 = 0;

  dig = (DIGESTS *) XP_CALLOC (1, sizeof (DIGESTS));

  if (dig == 0) 
    {
    /* out of memory allocating digest */
    return (DIGESTS *) NULL;
    }

#if defined(XP_WIN16)
  XP_ASSERT( !IsBadHugeReadPtr(data, length) );
#endif

  md5  = PK11_CreateDigestContext (SEC_OID_MD5);
  sha1 = PK11_CreateDigestContext (SEC_OID_SHA1);

  if (length > 0) 
    {
    PK11_DigestBegin (md5);
    PK11_DigestBegin (sha1);

    do {
       chunq = length;

#ifdef XP_WIN16
       if (length > CHUNQ) chunq = CHUNQ;

       /*
        *  If the block of data crosses one or more segment 
        *  boundaries then only pass the chunk of data in the 
        *  first segment.
        * 
        *  This allows the data to be treated as FAR by the
        *  PK11_DigestOp(...) routine.
        *
        */

       if (OFFSETOF(data) + chunq >= 0x10000) 
         chunq = 0x10000 - OFFSETOF(data);
#endif

       PK11_DigestOp (md5,  data, chunq);
       PK11_DigestOp (sha1, data, chunq);

       length -= chunq;
       data = ((char XP_HUGE *) data + chunq);
       } 
    while (length > 0);

    PK11_DigestFinal (md5,  dig->md5,  &dig->md5_length,  MD5_LENGTH);
    PK11_DigestFinal (sha1, dig->sha1, &dig->sha1_length, SHA1_LENGTH);

    PK11_DestroyContext (md5,  PR_TRUE);
    PK11_DestroyContext (sha1, PR_TRUE);
    }

  return dig;
  }

/*
 *  S O B _ d i g e s t _ f i l e
 *
 *  Calculates the MD5 and SHA1 digests for a file 
 *  present on disk, and returns these in DIGESTS struct.
 *
 */

int SOB_digest_file (char *filename, DIGESTS *dig)
    {
    XP_File fp;

    int num;
    unsigned char *buf;

    PK11Context *md5 = 0;
    PK11Context *sha1 = 0;

    buf = (unsigned char *) XP_CALLOC (1, FILECHUNQ);
    if (buf == NULL)
      {
      /* out of memory */
      return ZIG_ERR_MEMORY;
      }
 
    if ((fp = XP_FileOpen (filename, xpURL, "rb")) == 0)
      {
      perror (filename);
      XP_FREE (buf);
      return ZIG_ERR_FNF;
      }

    md5 = PK11_CreateDigestContext (SEC_OID_MD5);
    sha1 = PK11_CreateDigestContext (SEC_OID_SHA1);

    if (md5 == NULL || sha1 == NULL) 
      {
      /* can't generate digest contexts */
      XP_FREE (buf);
      XP_FileClose (fp);
      return ZIG_ERR_GENERAL;
      }

    PK11_DigestBegin (md5);
    PK11_DigestBegin (sha1);

    while (1)
      {
      if ((num = XP_FileRead (buf, FILECHUNQ, fp)) == 0)
        break;

      PK11_DigestOp (md5, buf, num);
      PK11_DigestOp (sha1, buf, num);
      }

    PK11_DigestFinal (md5, dig->md5, &dig->md5_length, MD5_LENGTH);
    PK11_DigestFinal (sha1, dig->sha1, &dig->sha1_length, SHA1_LENGTH);

    PK11_DestroyContext (md5, PR_TRUE);
    PK11_DestroyContext (sha1, PR_TRUE);

    XP_FREE (buf);
    XP_FileClose (fp);

    return 0;
    }

/*
 *  S O B _ J A R _  h a s h
 *
 *  Hash algorithm interface for use by the Jartool. Since we really
 *  don't know the private sizes of the context, and Java does need to
 *  know this number, allocate 512 bytes for it.
 *
 *  In april 1997 hashes in this file were changed to call PKCS11,
 *  as FIPS requires that when a smartcard has failed validation, 
 *  hashes are not to be performed. But because of the difficulty of
 *  preserving pointer context between calls to the SOB_JAR hashing
 *  functions, the hash routines are called directly, though after
 *  checking to see if hashing is allowed.
 *
 */

void *SOB_JAR_new_hash (int alg)
  {
  void *context;

  MD5Context *md5;
  SHA1Context *sha1;

  /* this is a hack because this whole XP_CALLOC stuff looks scary */

  if (!PK11_HashOK (alg == 1 ? SEC_OID_MD5 : SEC_OID_SHA1))
    return NULL;

  context = XP_CALLOC (1, 512);

  if (context)
    {
    switch (alg)
      {
      case 1:  /* MD5 */
               md5 = (MD5Context *) context;
               MD5_Begin (md5);
               break;

      case 2:  /* SHA1 */
               sha1 = (SHA1Context *) context;
               SHA1_Begin (sha1);
               break;
      }
    }

  return context;
  }

void *SOB_JAR_hash (int alg, void *cookie, int length, void *data)
  {
  MD5Context *md5;
  SHA1Context *sha1;

  /* this is a hack because this whole XP_CALLOC stuff looks scary */

  if (!PK11_HashOK (alg == 1 ? SEC_OID_MD5 : SEC_OID_SHA1))
    return NULL;

  if (length > 0)
    {
    switch (alg)
      {
      case 1:  /* MD5 */
               md5 = (MD5Context *) cookie;
               MD5_Update (md5, data, length);
               break;

      case 2:  /* SHA1 */
               sha1 = (SHA1Context *) cookie;
               SHA1_Update (sha1, data, length);
               break;
      }
    }

  return cookie;
  }

void *SOB_JAR_end_hash (int alg, void *cookie)
  {
  int length;
  unsigned char *data;
  char *ascii; 

  MD5Context *md5;
  SHA1Context *sha1;

  unsigned int md5_length;
  unsigned char md5_digest [MD5_LENGTH];

  unsigned int sha1_length;
  unsigned char sha1_digest [SHA1_LENGTH];

  /* this is a hack because this whole XP_CALLOC stuff looks scary */

  if (!PK11_HashOK (alg == 1 ? SEC_OID_MD5 : SEC_OID_SHA1)) 
    return NULL;

  switch (alg)
    {
    case 1:  /* MD5 */

             md5 = (MD5Context *) cookie;

             MD5_End (md5, md5_digest, &md5_length, MD5_LENGTH);
             /* MD5_DestroyContext (md5, PR_TRUE); */

             data = md5_digest;
             length = md5_length;

             break;

    case 2:  /* SHA1 */

             sha1 = (SHA1Context *) cookie;

             SHA1_End (sha1, sha1_digest, &sha1_length, SHA1_LENGTH);
             /* SHA1_DestroyContext (sha1, PR_TRUE); */

             data = sha1_digest;
             length = sha1_length;

             break;

    default: return NULL;
    }

  /* Instead of destroy context, since we created it */
  /* XP_FREE(cookie); */

  ascii = BTOA_DataToAscii(data, length);

  return ascii ? XP_STRDUP (ascii) : NULL;
  }

/*
 *  S O B _ o p e n _ k e y _ d a t a b a s e
 *
 */

static SECKEYKeyDBHandle *sob_open_key_database (void)
  {
  SECKEYKeyDBHandle *keydb;

  keydb = SECKEY_GetDefaultKeyDB();

  if (keydb == NULL)
    { /* open by file if this fails, if zigbert is to call this */ ; }

  return keydb;
  }

static int sob_close_key_database (SECKEYKeyDBHandle *keydb)
  {
  /* We never do close it */
  return 0;
  }

/*
 *  S O B _ J A R _ s i g n _ a r c h i v e
 *
 *  A simple API to sign a JAR archive, too simple perhaps.
 *
 */

int SOB_JAR_sign_archive 
      (char *nickname, char *password, char *sf, char *outsig)
  {
  char *out_fn;

  int status = ZIG_ERR_GENERAL;
  XP_File sf_fp, out_fp;

  CERTCertDBHandle *certdb;
  SECKEYKeyDBHandle *keydb;

  CERTCertificate *cert;

  /* open cert and key databases */

  certdb = SOB_open_database();
  if (certdb == NULL)
    return ZIG_ERR_GENERAL;

  keydb = sob_open_key_database();
  if (keydb == NULL)
    return ZIG_ERR_GENERAL;

  out_fn = XP_STRDUP (sf);

  if (out_fn == NULL || XP_STRLEN (sf) < 5)
    return ZIG_ERR_GENERAL;

  sf_fp = XP_FileOpen (sf, xpURL, "rb");
  out_fp = XP_FileOpen (outsig, xpURL, "wb");

  cert = CERT_FindCertByNickname (certdb, nickname);

  if (cert && sf_fp && out_fp)
    {
    status = sob_create_pk7 (certdb, keydb, cert, password, sf_fp, out_fp);
    }

  /* remove password from prying eyes */
  XP_MEMSET (password, 0, XP_STRLEN (password));

  XP_FileClose (sf_fp);
  XP_FileClose (out_fp);

  SOB_close_database (certdb);
  sob_close_key_database (keydb);

  return status;
  }

/*
 *  p a s s w o r d _ h a r d c o d e 
 *
 *  A function to use the password passed in the -p(password) argument
 *  of the command line. This is only to be used for build & testing purposes,
 *  as it's extraordinarily insecure. 
 *
 */

static SECItem *sob_check_null_password (SECKEYKeyDBHandle *handle)
  {
  SECStatus rv;
  SECItem *pwitem;

  /* hash the empty string as a password */
  pwitem = SECKEY_HashPassword ("", handle->global_salt);

  if (pwitem == NULL)
    return NULL;

  /* check to see if this is the right password */
  rv = SECKEY_CheckKeyDBPassword (handle, pwitem);

  if (rv == SECFailure)
    return NULL;

  return pwitem;
  }

static SECItem *sob_password_hardcode (void *arg, SECKEYKeyDBHandle *handle)
  {
  char *password;

  SECStatus rv;
  SECItem *pwitem;

  /* Check to see if zero length password or not */
  pwitem = sob_check_null_password (handle);

  if (pwitem)
    return pwitem;

  password = (char *) arg;

  /* hash the password */
  pwitem = SECKEY_HashPassword (password, handle->global_salt);

  /* remove from prying eyes */
  XP_MEMSET (password, 0, XP_STRLEN (password));

  if (pwitem == NULL)
    {
    /* error hashing password */
    return NULL;
    }
    
    /* confirm the password */
    rv = SECKEY_CheckKeyDBPassword (handle, pwitem);
    if (rv) 
      {
      /* bad password */
      SECITEM_ZfreeItem (pwitem, PR_TRUE);
      return NULL;
      }
    
  return pwitem;
  }

/*
 *  s o b _ c r e a t e _ p k 7
 *
 */

static void sob_pk7_out (void *arg, const char *buf, unsigned long len)
  {
  XP_FileWrite (buf, len, (XP_File) arg);
  }

static int sob_create_pk7 
   (CERTCertDBHandle *certdb, SECKEYKeyDBHandle *keydb, 
       CERTCertificate *cert, char *password, XP_File infp, XP_File outfp)
  {
  int nb;
  unsigned char buffer [4096], digestdata[32];
  SECHashObject *hashObj;
  void *hashcx;
  unsigned int len;

  int status = 0;
  char *errstring;

  SECItem digest;
  SEC_PKCS7ContentInfo *cinfo;
  SECStatus rv;

  MWContext *mw;

  if (outfp == NULL || infp == NULL || cert == NULL)
    return ZIG_ERR_GENERAL;

  /* we sign with SHA */
  hashObj = &SECHashObjects [HASH_AlgSHA1];

  hashcx = (* hashObj->create)();
  if (hashcx == NULL)
    return ZIG_ERR_GENERAL;

  (* hashObj->begin)(hashcx);

  while (1)
    {
    if (feof (infp)) break;
    nb = XP_FileRead (buffer, sizeof (buffer), infp);
    if (nb == 0) 
      {
      if (ferror(infp)) 
        {
        /* XP_SetError(SEC_ERROR_IO); */ /* FIX */
	(* hashObj->destroy) (hashcx, PR_TRUE);
	return ZIG_ERR_GENERAL;
        }
      /* eof */
      break;
      }
    (* hashObj->update) (hashcx, buffer, nb);
    }

  (* hashObj->end) (hashcx, digestdata, &len, 32);
  (* hashObj->destroy) (hashcx, PR_TRUE);

  digest.data = digestdata;
  digest.len = len;

  /* Find a context. This can be called by two things,
     zigbert or the jartool. Zigbert will always use NULL, and
     jartool must use any old context it can find since it's
     calling from inside javaland. */

  mw = XP_FindSomeContext();

  XP_SetError (0);

  cinfo = SEC_PKCS7CreateSignedData 
             (cert, certUsageObjectSigner, NULL, 
                SEC_OID_SHA1, &digest, NULL, (void *) mw);

  if (cinfo == NULL)
    return ZIG_ERR_PK7;

  rv = SEC_PKCS7IncludeCertChain (cinfo, NULL);
  if (rv != SECSuccess) 
    {
    status = XP_GetError();
    SEC_PKCS7DestroyContentInfo (cinfo);
    return status;
    }

  /* Having this here forces the jartool to always include
     signing time. Zigbert has an option to disable this feature. */

  rv = SEC_PKCS7AddSigningTime (cinfo);
  if (rv != SECSuccess)
    {
    /* don't check error */
    }

  XP_SetError (0);

#ifdef USE_MOZ_ENCODE
  /* if calling from mozilla */
  rv = sob_moz_encode
             (cinfo, sob_pk7_out, outfp, 
                 NULL,  /* pwfn */ NULL,  /* pwarg */ (void *) mw);
#else
  /* if calling from zigbert or mozilla thread*/
  rv = SEC_PKCS7Encode 
             (cinfo, sob_pk7_out, outfp, 
                 NULL,  /* pwfn */ NULL,  /* pwarg */ (void *) mw):
#endif

  if (rv != SECSuccess)
    status = XP_GetError();

  SEC_PKCS7DestroyContentInfo (cinfo);

  if (rv != SECSuccess)
    {
    errstring = XP_GetString (status);
    XP_TRACE (("Jar signing failed (reason %d = %s)", status, errstring));
    return status < 0 ? status : ZIG_ERR_GENERAL;
    }

  return 0;
  }


#ifdef USE_MOZ_ENCODE

/*
 *  SOB_MOZ_encode
 *
 *  Mozillize the PKCS7 encoding. This must be done to
 *  give the user a password dialog, which cannot be done
 *  in a java thread.
 *
 */

struct SOB_MOZ_encode
  {
  int error;
  SECStatus status;
  SEC_PKCS7ContentInfo *cinfo;
  SEC_PKCS7EncoderOutputCallback outputfn;
  void *outputarg;
  PK11SymKey *bulkkey;
  SECKEYGetPasswordKey pwfn;
  void *pwfnarg;
  };


/* This is called inside the mozilla thread */

void sob_moz_encode_fn (void *sausage)
  {
  SECStatus status;
  struct SOB_MOZ_encode *vs;

  vs = (struct SOB_MOZ_encode *) sausage;
  XP_SetError (vs->error);

  status = SEC_PKCS7Encode (vs->cinfo, vs->outputfn, vs->outputarg, vs->bulkkey, vs->pwfn, vs->pwfnarg);

  vs->error = XP_GetError();
  vs->status = status;
  }


/* Wrapper for the ET_MOZ call */
 
SECStatus sob_moz_encode
      (
      SEC_PKCS7ContentInfo *cinfo,
      SEC_PKCS7EncoderOutputCallback  outputfn,
      void *outputarg,
      PK11SymKey *bulkkey,
      SECKEYGetPasswordKey pwfn,
      void *pwfnarg
      )
  {
  struct SOB_MOZ_encode borscht;

  XP_MEMSET (&borscht, 0, sizeof (borscht));
  borscht.error = XP_GetError();

  borscht.cinfo = cinfo;
  borscht.outputfn = outputfn;
  borscht.outputarg = outputarg;
  borscht.bulkkey = bulkkey;
  borscht.pwfn = pwfn;
  borscht.pwfnarg = pwfnarg;

#ifndef AKBAR
  ET_moz_CallFunction (sob_moz_encode_fn, &borscht);
#endif /* !AKBAR */

  XP_SetError (borscht.error);
  return SECSuccess;
  }

#endif
