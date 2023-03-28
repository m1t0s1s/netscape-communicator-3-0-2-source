/*
 *  ZIGVER
 *
 *  Zignature Parsing & Verification
 *
 *  Copyright 1996 Netscape Corporation
 * 
 */

/* This allows manifest files above 64k to be
   processed on non-win16 platforms */

#if !defined(XP_WIN16) && !defined(XP_MAC)
#define BIG_MEMORY
#endif

#include "zig.h"
#include "zigfile.h"

#include "base64.h"
#include "xp_error.h"
#include "xpgetstr.h"

#include "cert.h"
#include "key.h"
#include "secpkcs7.h"

/* to use huge pointers in win16 */

#if !defined(XP_WIN16)
#define xp_HUGE_STRCPY XP_STRCPY
#define xp_HUGE_STRLEN XP_STRLEN
#define xp_HUGE_STRNCASECMP XP_STRNCASECMP
#else
int xp_HUGE_STRNCASECMP (char XP_HUGE *buf, char *key, int len);
size_t xp_HUGE_STRLEN (char XP_HUGE *s);
char *xp_HUGE_STRCPY (char *to, char XP_HUGE *from);
#endif

#ifndef AKBAR
/* commercial compression */
/* #include " ../../modules/zlib/src/zlib.h" */
#include "zlib.h"
#endif /* !AKBAR */

/* from certdb.h */
#define CERTDB_USER (1<<6)

/* from certdb.h */
extern PRBool SEC_CertNicknameConflict
   (char *nickname, CERTCertDBHandle *handle);

/* from certdb.h */
extern SECStatus SEC_AddTempNickname
   (CERTCertDBHandle *handle, char *nickname, SECItem *certKey);

/* from certdb.h */
typedef SECStatus (* PermCertCallback)(CERTCertificate *cert, SECItem *k, void *pdata);

/* from certdb.h */
SECStatus SEC_TraversePermCerts
   (CERTCertDBHandle *handle, PermCertCallback certfunc, void *udata);

#ifdef XP_UNIX
extern int errno;
#endif

#define zigprint 
#define SZ 512

static int sob_validate_pkcs7 (ZIG *zig, char *data, long length);

static int sob_decode (ZIG *zig, char *data, long length);

static void sob_catch_bytes
     (void *arg, const char *buf, unsigned long len);

static int sob_gather_signers
     (ZIG *zig, SEC_PKCS7ContentInfo *cinfo);

static char XP_HUGE *sob_eat_line 
     (int lines, char XP_HUGE *data, long *len, int seps);

static DIGESTS *sob_digest_section 
     (char XP_HUGE *manifest, long length, int seps);

static DIGESTS *sob_get_mf_digest (ZIG *zig, char *path);

static int sob_parse_digital_signature 
     (char *raw_manifest, long length, ZIG *zig);

static int sob_add_cert
     (ZIG *zig, int type, CERTCertificate *cert);

static CERTCertificate *sob_get_certificate
            (ZIG *zig, long keylen, void *key, int *result);

extern CERTCertDBHandle *SOB_open_database (void);

extern int SOB_close_database (CERTCertDBHandle *certdb);

static char *sob_cert_element (char *name, char *tag, int occ);

static char *sob_choose_nickname (CERTCertificate *cert);

static char *sob_basename (const char *path);

#ifndef AKBAR
static int sob_inflate_memory (unsigned int method, long *length, char XP_HUGE **data);
#endif /* !AKBAR */

static int sob_signal (int status, ZIG *zig, const char *metafile, char *pathname);


#ifdef BIG_MEMORY
#undef XP_CALLOC
#define XP_CALLOC(num,sz) calloc((num),(sz)) 
#endif

/*
 *  S O B _ p a r s e _ m a n i f e s t
 * 
 *  Pass manifest files to this function. They are
 *  decoded and placed into internal representations.
 * 
 *  Accepts both signature and manifest files. Use
 *  the same "zig" for both. 
 *
 */

int SOB_parse_manifest 
    (char XP_HUGE *raw_manifest, long length, 
        const char *path, const char *url, ZIG *zig)
  {
  int status;

  char XP_HUGE *s;
  long raw_len;

  SOBITEM *it, *mit;

  METAINFO *met;
  DIGESTS *dig, *sfdig, *mfdig, *savdig;

  ZSLink *ent;

  int type;

  int seps;
  int cr = 0, lf = 0;

  char line [SZ];
  char x_name [SZ], x_md5 [SZ], x_sha [SZ];

  char *x_info;

  char *sf_md5 = NULL, *sf_sha1 = NULL;

  unsigned int binary_length;
  unsigned char *binary_digest;

  if (zig->filename == NULL && path)
    zig->filename = XP_STRDUP (path);

  if (zig->url == NULL && url)
    zig->url = XP_STRDUP (url);

  *x_name = 0;
  *x_md5 = 0;
  *x_sha = 0;

#if defined(XP_WIN16)
    XP_ASSERT( !IsBadHugeReadPtr(raw_manifest, length) );
#endif

  if (!xp_HUGE_STRNCASECMP (raw_manifest, "Manifest-Version: ", 18))
    {
    if (zig->globalmeta)
      {
      /* refuse a second manifest file, if passed for some reason */
      return ZIG_ERR_ORDER;
      }
    type = ZIG_MF;
    }
  else if (!xp_HUGE_STRNCASECMP (raw_manifest, "Signature-Version: ", 19))
    {
    type = ZIG_SF;

    if (zig->globalmeta == NULL)
      {
      /* It is a requirement that the MF file be passed before the SF file */
      return ZIG_ERR_ORDER;
      }

    if (zig->digest != NULL)
      {
      /* Right now, refuse a second SF file */
      return ZIG_ERR_ORDER;
      }

    /* remember its digest */

    sfdig = SOB_calculate_digest (raw_manifest, length);

    if (sfdig)
      {
      zig->digest_alg = 0; /* FIX */
      zig->digest_length = sizeof (sfdig->sha1);
      zig->digest = XP_CALLOC (1, sizeof (sfdig->sha1));

      if (zig->digest) 
        XP_MEMCPY (zig->digest, sfdig->sha1, sizeof (sfdig->sha1));
      else
        return ZIG_ERR_MEMORY;
      }
    else
      return ZIG_ERR_MEMORY;

    zig->owner = path ? sob_basename (path) : NULL;
    }
  else
    {
    /* This is probably a binary signature, but it might be something else */
    type = ZIG_SIG;
    }


  if (type == ZIG_SIG)
    {
    /* this is binary data, insanity check */

    if (length <= 128) 
      {
      /* signature is way too small */
      return ZIG_ERR_SIG;
      }

    /* make sure that MF and SF have already been processed */

    if (zig->globalmeta == NULL || zig->digest == NULL)
      return ZIG_ERR_ORDER;

    /* Do not pass a huge pointer to this function,
       since the underlying security code is unaware. We will
       never pass >64k through here. */

    if (length > 64000)
      {
      /* this digital signature is way too big */
      return ZIG_ERR_SIG;
      }

#ifdef XP_WIN16
      {
      unsigned char *manifest_copy;

      manifest_copy = (unsigned char *) XP_CALLOC (1,length);
      if (manifest_copy)
        {
        XP_MEMCPY (manifest_copy, raw_manifest, length);

        status = sob_parse_digital_signature (manifest_copy, length, zig);

        XP_FREE (manifest_copy);
        }
      else
        {
        /* out of memory */
        return ZIG_ERR_MEMORY;
        }
      }
#else
    /* don't expense unneeded calloc overhead on non-win16 */
    status = sob_parse_digital_signature (raw_manifest, length, zig);
#endif

    return status;
    }


  raw_len = length ? length : xp_HUGE_STRLEN (raw_manifest);

  /* Check in the first 255 characters for CR, LF */

  for (s = raw_manifest; *s && (s - raw_manifest < 256); s++)
    {
    if (*s == 13)
      cr++;
    if (*s == 10)
      lf++;
    }

  /* There are two real cases. Use CR's as separators, 
     or ignore any CR's and use LF's as separators. This handles
     all three major conventions. */

  seps = (lf == 0 && cr > 0);


  /* remember a digest for the global section */

  if (type == ZIG_MF)
    {
    zig->globalmeta = (void *) sob_digest_section (raw_manifest, raw_len, seps);
    }


  /* null terminate the first line */
  raw_manifest = sob_eat_line (0, raw_manifest, &raw_len, seps);


  /* skip over the preliminary section */
  /* This is one section at the top of the file with global metainfo */

  while (raw_len)
    {
    raw_manifest = sob_eat_line (1, raw_manifest, &raw_len, seps);
    if (!*raw_manifest) break;

    it = XP_CALLOC (1, sizeof (SOBITEM));
    met = XP_CALLOC (1, sizeof (METAINFO));

    if (it == NULL || met == NULL)
      return ZIG_ERR_MEMORY;

    it->type = ZIG_META;

    /* Parse out the header & info */

    if (xp_HUGE_STRLEN (raw_manifest) >= SZ)
      {
      /* almost certainly nonsense */
      continue;
      }

    xp_HUGE_STRCPY (line, raw_manifest);
    x_info = line;

    while (*x_info && *x_info != ' ' && *x_info != '\t' && *x_info != ':')
      x_info++;

    if (*x_info) *x_info++ = 0;

    while (*x_info == ' ' || *x_info == '\t' || *x_info == ':')
      x_info++;

    /* metainfo (name, value) pair is now (line, x_info) */

    met->header = XP_STRDUP (line);
    met->length = XP_STRLEN (x_info);
    met->info = (unsigned char *) XP_STRDUP (x_info); /* jrm */

    it->pathname = NULL;
    it->data = (unsigned char *) met;
    it->size = sizeof (METAINFO);

    ent = ZS_NewLink (it);
    ZS_AppendLink (zig->list, ent);

    if (type == ZIG_SF)
      {
      if (!XP_STRCASECMP (line, "MD5-Digest"))
        sf_md5 = (char *) met->info;

      if (!XP_STRCASECMP (line, "SHA1-Digest") || !XP_STRCASECMP (line, "SHA-Digest"))
        sf_sha1 = (char *) met->info;
      }
    }

  if (type == ZIG_SF && zig->globalmeta)
    {
    /* this is a SF file which may contain a digest of the manifest.mf's 
       global metainfo. */

    int match = 0;

    DIGESTS dig, *glob;
    unsigned char *md5_digest, *sha1_digest;

    glob = (DIGESTS *) zig->globalmeta;

    if (sf_md5)
      {
      md5_digest = ATOB_AsciiToData (sf_md5, &dig.md5_length);
      match = XP_MEMCMP (md5_digest, glob->md5, MD5_LENGTH);
      }

    if (sf_sha1 && match == 0)
      {
      sha1_digest = ATOB_AsciiToData (sf_sha1, &dig.sha1_length);
      match = XP_MEMCMP (sha1_digest, glob->sha1, SHA1_LENGTH);
      }

    if (match != 0)
      {
      /* global digest doesn't match, SF file therefore invalid */
      zig->valid = ZIG_ERR_METADATA;
      return ZIG_ERR_METADATA;
      }
    }

  /* done with top section of global data */


  while (raw_len)
    {
    *x_md5 = 0;
    *x_sha = 0;


    /* If this is a manifest file, attempt to get a digest of the following section, 
       without damaging it. This digest will be saved later. */

    if (type == ZIG_MF)
      {
      char XP_HUGE *sec;
      long sec_len = raw_len;

      if (!*raw_manifest || *raw_manifest == '\n')
        {     
        /* skip the blank line */ 
        sec = sob_eat_line (-1, raw_manifest, &sec_len, seps);
        }
      else
        sec = raw_manifest;

      if (!xp_HUGE_STRNCASECMP (sec, "Name:", 5))
        {
        if (type == ZIG_MF)
          mfdig = (void *) sob_digest_section (sec, sec_len, seps);
        else
          mfdig = NULL;
        }
      }


    while (raw_len)
      {
      raw_manifest = sob_eat_line (1, raw_manifest, &raw_len, seps);
      if (!*raw_manifest) break;


      /* Parse out the name/value pair */

      xp_HUGE_STRCPY (line, raw_manifest);
      x_info = line;

      while (*x_info && *x_info != ' ' && *x_info != '\t')
        x_info++;

      if (*x_info) *x_info++ = 0;

      while (*x_info == ' ' || *x_info == '\t') 
        x_info++;


      if (!XP_STRCASECMP (line, "Name:"))
        XP_STRCPY (x_name, x_info);

      else if (!XP_STRCASECMP (line, "MD5-Digest:"))
        XP_STRCPY (x_md5, x_info);

      else if (!XP_STRCASECMP (line, "SHA1-Digest:") || !XP_STRCASECMP (line, "SHA-Digest:"))
        XP_STRCPY (x_sha, x_info);

      /* Algorithm list is meta info we don't care about; keeping it out
         of metadata saves significant space for large jar files */

      else if (!XP_STRCASECMP (line, "Digest-Algorithms:")
                    || !XP_STRCASECMP (line, "Hash-Algorithms:"))
        {
        continue;
        }

      /* Meta info is only collected for the manifest.mf file,
         since the SOB_get_metainfo call does not support identity */

      else if (type == ZIG_MF)
        {
        /* this is meta-data */
        /* it is quite ugly with all its repetition */

        it = XP_CALLOC (1, sizeof (SOBITEM));
        met = XP_CALLOC (1, sizeof (METAINFO));

        if (it == NULL || met == NULL)
          return ZIG_ERR_MEMORY;

        it->type = ZIG_META;

        if (*x_name)
          it->pathname = XP_STRDUP (x_name);

        /* metainfo (name, value) pair is now (line, x_info) */

        met->header = XP_STRDUP (line);
        met->length = XP_STRLEN (x_info);
        met->info = (unsigned char *) XP_STRDUP (x_info); /* jrm */

        it->data = (unsigned char *) met;
        it->size = sizeof (METAINFO);

        ent = ZS_NewLink (it);
        ZS_AppendLink (zig->list, ent);
        }
      }

    it = XP_CALLOC (1, sizeof (SOBITEM));
    dig = XP_CALLOC (1, sizeof (DIGESTS));

    if (it == NULL || dig == NULL)
      return ZIG_ERR_MEMORY;

    it->type = type;

    if (*x_name) 
      it->pathname = XP_STRDUP (x_name);

    if (*x_md5 ) 
      {
      binary_digest = ATOB_AsciiToData (x_md5, &binary_length);
      memcpy (dig->md5, binary_digest, binary_length);
      dig->md5_length = binary_length;
      }

    if (*x_sha ) 
      {
      binary_digest = ATOB_AsciiToData (x_sha, &binary_length);
      memcpy (dig->sha1, binary_digest, binary_length);
      dig->sha1_length = binary_length;
      }

    it->data = (unsigned char *) dig;
    it->size = sizeof (DIGESTS);

    ent = ZS_NewLink (it);
    ZS_AppendLink (zig->list, ent);


    /* we're placing these calculated digests of manifest.mf 
       sections in a list where they can subsequently be forgotten */

    if (mfdig)
      {
      mit = XP_CALLOC (1, sizeof (SOBITEM));
      mit->type = ZIG_SECT;

      /* danger, don't free the below */
      mit->pathname = it->pathname;

      mit->data = mfdig;

      ent = ZS_NewLink (mit);
      ZS_AppendLink (zig->manifest, ent);
      }


    /* Retrieve our saved SHA1 digest from saved copy and check digests. If one fails, 
       fail the entire archive, since this indicates an inconsistency in the manifest files. 
       That indicates tampering, or more likely, damage of some kind. */

    if (type == ZIG_SF)
      {
      int cv;
      savdig = sob_get_mf_digest (zig, x_name);

      if (savdig == NULL)
        {
        /* no .mf digest for this pathname */
        status = sob_signal (ZIG_ERR_ENTRY, zig, path, x_name);
        if (status < 0) 
          continue; 
        else 
          return status;
        }

      /* check for md5 consistency */
      if (dig->md5_length)
        {
        cv = XP_MEMCMP (savdig->md5, dig->md5, dig->md5_length);
        /* md5 hash of .mf file is not what expected */
        if (cv) 
          {
          status = sob_signal (ZIG_ERR_HASH, zig, path, x_name);

          /* bad hash, man */

          dig->md5_length = ZIG_BAD_HASH;
          savdig->md5_length = ZIG_BAD_HASH;

          if (status < 0) 
            continue; 
          else 
            return status;
          }
        }

      /* check for sha1 consistency */
      if (dig->sha1_length)
        {
        cv = XP_MEMCMP (savdig->sha1, dig->sha1, dig->sha1_length);
        /* sha1 hash of .mf file is not what expected */
        if (cv) 
          {
          status = sob_signal (ZIG_ERR_HASH, zig, path, x_name);

          /* bad hash, man */

          dig->sha1_length = ZIG_BAD_HASH;
          savdig->sha1_length = ZIG_BAD_HASH;

          if (status < 0)
            continue;
          else
            return status;
          }
        }
      }
    }

  return 0;
  }

/*
 *  s o b _ p a r s e _ d i g i t a l _ s i g n a t u r e
 *
 *  Parse an RSA or DSA or perhaps PGP digital signature.
 *  Right now everything is PKCS7.
 *
 */

static int sob_parse_digital_signature 
     (char *raw_manifest, long length, ZIG *zig)
  {
#if defined(XP_WIN16)
  XP_ASSERT( LOWORD(raw_manifest) + length < 0xFFFF );
#endif
  return sob_validate_pkcs7 (zig, raw_manifest, length);
  }

/*
 *  s o b _ a d d _ c e r t
 * 
 *  Add information for the given certificate
 *  (or whatever) to the ZIG linked list. A pointer
 *  is passed for some relevant reference, say
 *  for example the original certificate.
 *
 */

static int sob_add_cert
     (ZIG *zig, int type, CERTCertificate *cert)
  {
  SOBITEM *it;

  DIGESTS *dig;
  FINGERZIG *fing;

  ZSLink *ent;
  char *x_data; long x_length;

  if (cert == NULL)
    return ZIG_ERR_ORDER;

  it = XP_CALLOC (1, sizeof (SOBITEM));
  fing = XP_CALLOC (1, sizeof (FINGERZIG));

  if (it == NULL || fing == NULL)
    return ZIG_ERR_MEMORY;

  x_data = (char *) cert->derCert.data;
  x_length = cert->derCert.len;

  dig = SOB_calculate_digest (x_data, x_length);

  if (dig == NULL)
    return ZIG_ERR_MEMORY;

  memcpy (fing->fingerprint, dig->md5, sizeof (fing->fingerprint));

  fing->cert = CERT_DupCertificate (cert);

  it->type = type;
  it->pathname = NULL;   /* wild for dummy match */

  it->data = (unsigned char *) fing;
  it->size = sizeof (FINGERZIG);

  /* these are appended to the "cer" linked list, not the "list" one */

  ent = ZS_NewLink (it);
  ZS_AppendLink (zig->cer, ent);

  XP_FREE (dig);

  /* Use the certificate key instead of the fingerprint. Actually
     for now add it to FINGERZIG in addition to the fingerprint. */

  fing->length = cert->certKey.len;

  fing->key = (char *) XP_CALLOC (1, fing->length);

  if (fing->key)
    XP_MEMCPY (fing->key, cert->certKey.data, fing->length);
  else
    return ZIG_ERR_MEMORY;

  return 0;
  }

/*
 *  e a t _ l i n e 
 *
 *  Consume an ascii line from the top of a file kept
 *  in memory. This destroys the file in place. This function
 *  handles PC, Mac, and Unix style text files.
 *
 */

static char XP_HUGE *sob_eat_line 
    (int lines, char XP_HUGE *data, long *len, int seps)
  {
  char XP_HUGE *ret;
  int eating;

  ret = data;
  if (!*len) return ret;

  eating = (lines >= 0);
  if (!eating) lines = 1;

  /* Eat the requisite number of lines, if any; 
     prior to terminating the current line with a 0. */

  for (/* yip */ ; lines; lines--)
    {
    while (*data && *data != '\n')
      data++;

    /* After the CR, ok to eat one LF */

    if (*data == '\n')
      data++;

    /* If there are zeros, we put them there */

    while (*data == 0 && data - ret < *len)
      data++;
    }

  *len -= data - ret;
  ret = data;

  if (eating)
    {
    /* Terminate this line with a 0 */ 

    while (*data && *data != '\n' && *data != '\r')
      data++;

    /* In any case we are allowed to eat CR */

    if (*data == '\r')
      *data++ = 0;

    /* After the CR, ok to eat one LF */

    if (*data == '\n')
      *data++ = 0;
    }

  return ret;
  }

/*
 *  s o b _ d i g e s t _ s e c t i o n
 *
 *  Return the digests of the next section of the manifest file.
 *  Does not damage the manifest file, unlike parse_manifest.
 * 
 */

static DIGESTS *sob_digest_section 
    (char XP_HUGE *manifest, long length, int seps)
  {
  long global_len;
  char XP_HUGE *global_end;

  global_end = manifest;
  global_len = length;

  while (global_len)
    {
    global_end = sob_eat_line (-1, global_end, &global_len, seps);
    if (*global_end == 0 || *global_end == '\n')
      break;
    }

  return SOB_calculate_digest (manifest, global_end - manifest);
  }

/*
 *  S O B _ v e r i f y _ d i g e s t
 *
 *  Verifies that a precalculated digest matches the
 *  expected value in the manifest.  The fact that this 
 *  returns a result and a status is historical, it
 *  should just return a status.
 *
 */

int PR_CALLBACK SOB_verify_digest
    (int *result, ZIG *zig, const char *name, DIGESTS *dig)
  {
  SOBITEM *it;

  DIGESTS *shindig;

  ZSLink *link;
  ZSList *list;

  int result1, result2;

  list = zig->list;

  *result = 0;
  result1 = result2 = 0;

  if (zig->valid < 0)
    {
    /* signature not valid */
    return ZIG_ERR_SIG;
    }

  if (ZS_ListEmpty (list))
    {
    /* empty list */
    return ZIG_ERR_PNF;
    }

  for (link = ZS_ListHead (list); 
       !ZS_ListIterDone (list, link); 
       link = link->next)
    {
    it = (SOBITEM *) link->thing;
    if (it->type == ZIG_MF && it->pathname && !XP_STRCMP (it->pathname, name))
      {
      shindig = (DIGESTS *) it->data;

      if (shindig->md5_length)
        {
        if (shindig->md5_length == ZIG_BAD_HASH)
          return ZIG_ERR_HASH;
        else
          result1 = memcmp (dig->md5, shindig->md5, shindig->md5_length);
        }

      if (shindig->sha1_length)
        {
        if (shindig->md5_length == ZIG_BAD_HASH)
          return ZIG_ERR_HASH;
        else
          result2 = memcmp (dig->sha1, shindig->sha1, shindig->sha1_length);
        }

      return (result1 == 0 && result2 == 0) ? 0 : ZIG_ERR_HASH;
      }
    }

  return ZIG_ERR_PNF;
  }

/* 
 *  S O B _ c e r t _ a t t r i b u t e
 *
 *  Return the named certificate attribute from the
 *  certificate specified by the given key.
 *
 */

int PR_CALLBACK SOB_cert_attribute
    (int attrib, ZIG *zig, long keylen, void *key, 
        void **result, unsigned long *length)
  {
  int status = 0;
  char *ret = NULL;

  CERTCertificate *cert;

  CERTCertDBHandle *certdb;

  DIGESTS *dig;
  SECItem hexme;

  *length = 0;

  if (attrib == 0 || key == 0)
    return ZIG_ERR_GENERAL;

  if (attrib == ZIG_C_JAVA)
    {
    cert = (CERTCertificate *) NULL;
    certdb = SOB_open_database();

    if (certdb)
      {
      cert = CERT_FindCertByNickname (certdb, key);
      if (cert)
        {
        *length = cert->certKey.len;

        *result = (void *) XP_CALLOC (1, *length);
        XP_MEMCPY (*result, cert->certKey.data, *length);
        }
      SOB_close_database (certdb);
      }

    return cert ? 0 : ZIG_ERR_GENERAL;
    }

  if (zig && zig->pkcs7 == 0)
    return ZIG_ERR_GENERAL;

  cert = sob_get_certificate (zig, keylen, key, &status);

  if (cert == NULL || status < 0)
    return ZIG_ERR_GENERAL;

  /* below taken out because I wanted Raman to use the html call.
     However he needs this information for java dialogs. */

#define SEP " <br> "
#define SEPLEN (XP_STRLEN(SEP))

  switch (attrib)
    {
    case ZIG_C_COMPANY:

      ret = cert->subjectName;

      /* This is pretty ugly looking but only used
         here for this one purpose. */

      if (ret)
        {
        int retlen = 0;

        char *cer_ou1, *cer_ou2, *cer_ou3;
	char *cer_cn, *cer_e, *cer_o, *cer_l;

	cer_cn  = CERT_GetCommonName (&cert->subject);
        cer_e   = CERT_GetCertEmailAddress (&cert->subject);
        cer_ou3 = sob_cert_element (ret, "OU=", 3);
        cer_ou2 = sob_cert_element (ret, "OU=", 2);
        cer_ou1 = sob_cert_element (ret, "OU=", 1);
        cer_o   = CERT_GetOrgName (&cert->subject);
        cer_l   = CERT_GetCountryName (&cert->subject);

        if (cer_cn)  retlen += SEPLEN + XP_STRLEN (cer_cn);
        if (cer_e)   retlen += SEPLEN + XP_STRLEN (cer_e);
        if (cer_ou1) retlen += SEPLEN + XP_STRLEN (cer_ou1);
        if (cer_ou2) retlen += SEPLEN + XP_STRLEN (cer_ou2);
        if (cer_ou3) retlen += SEPLEN + XP_STRLEN (cer_ou3);
        if (cer_o)   retlen += SEPLEN + XP_STRLEN (cer_o);
        if (cer_l)   retlen += SEPLEN + XP_STRLEN (cer_l);

        ret = (char *) XP_CALLOC (1, 1 + retlen);

        if (cer_cn)  { XP_STRCPY (ret, cer_cn);  XP_STRCAT (ret, SEP); }
        if (cer_e)   { XP_STRCAT (ret, cer_e);   XP_STRCAT (ret, SEP); }
        if (cer_ou1) { XP_STRCAT (ret, cer_ou1); XP_STRCAT (ret, SEP); }
        if (cer_ou2) { XP_STRCAT (ret, cer_ou2); XP_STRCAT (ret, SEP); }
        if (cer_ou3) { XP_STRCAT (ret, cer_ou3); XP_STRCAT (ret, SEP); }
        if (cer_o)   { XP_STRCAT (ret, cer_o);   XP_STRCAT (ret, SEP); }
        if (cer_l)     XP_STRCAT (ret, cer_l);

	/* return here to avoid unsightly memory leak */

        *result = ret;
        *length = XP_STRLEN (ret);

        return 0;
        }
      break;

    case ZIG_C_CA:

      ret = cert->issuerName;

      if (ret)
        {
        int retlen = 0;

        char *cer_ou1, *cer_ou2, *cer_ou3;
	char *cer_cn, *cer_e, *cer_o, *cer_l;

        /* This is pretty ugly looking but only used
           here for this one purpose. */

	cer_cn  = CERT_GetCommonName (&cert->issuer);
        cer_e   = CERT_GetCertEmailAddress (&cert->issuer);
        cer_ou3 = sob_cert_element (ret, "OU=", 3);
        cer_ou2 = sob_cert_element (ret, "OU=", 2);
        cer_ou1 = sob_cert_element (ret, "OU=", 1);
        cer_o   = CERT_GetOrgName (&cert->issuer);
        cer_l   = CERT_GetCountryName (&cert->issuer);

        if (cer_cn)  retlen += SEPLEN + XP_STRLEN (cer_cn);
        if (cer_e)   retlen += SEPLEN + XP_STRLEN (cer_e);
        if (cer_ou1) retlen += SEPLEN + XP_STRLEN (cer_ou1);
        if (cer_ou2) retlen += SEPLEN + XP_STRLEN (cer_ou2);
        if (cer_ou3) retlen += SEPLEN + XP_STRLEN (cer_ou3);
        if (cer_o)   retlen += SEPLEN + XP_STRLEN (cer_o);
        if (cer_l)   retlen += SEPLEN + XP_STRLEN (cer_l);

        ret = (char *) XP_CALLOC (1, 1 + retlen);

        if (cer_cn)  { XP_STRCPY (ret, cer_cn);  XP_STRCAT (ret, SEP); }
        if (cer_e)   { XP_STRCAT (ret, cer_e);   XP_STRCAT (ret, SEP); }
        if (cer_ou1) { XP_STRCAT (ret, cer_ou1); XP_STRCAT (ret, SEP); }
        if (cer_ou2) { XP_STRCAT (ret, cer_ou2); XP_STRCAT (ret, SEP); }
        if (cer_ou3) { XP_STRCAT (ret, cer_ou3); XP_STRCAT (ret, SEP); }
        if (cer_o)   { XP_STRCAT (ret, cer_o);   XP_STRCAT (ret, SEP); }
        if (cer_l)     XP_STRCAT (ret, cer_l);

	/* return here to avoid unsightly memory leak */

        *result = ret;
        *length = XP_STRLEN (ret);

        return 0;
        }

      break;

    case ZIG_C_SERIAL:

      ret = CERT_Hexify (&cert->serialNumber, 1);
      break;

    case ZIG_C_EXPIRES:

      ret = DER_UTCDayToAscii (&cert->validity.notAfter);
      break;

    case ZIG_C_NICKNAME:

      ret = sob_choose_nickname (cert);
      break;

    case ZIG_C_FP:

      dig = SOB_calculate_digest 
         ((char *) cert->derCert.data, cert->derCert.len);

      if (dig)
        {
        hexme.len = sizeof (dig->md5);
        hexme.data = dig->md5;
        ret = CERT_Hexify (&hexme, 1);
        }
      break;

    default:

      return ZIG_ERR_GENERAL;
    }

  *result = ret ? XP_STRDUP (ret) : NULL;
  *length = XP_STRLEN (ret);

  return 0;
  }

/* 
 *  s o b  _ c e r t _ e l e m e n t
 *
 *  Retrieve an element from an x400ish ascii
 *  designator, in a hackish sort of way. The right
 *  thing would probably be to sort AVATags.
 *
 */

static char *sob_cert_element (char *name, char *tag, int occ)
  {
  if (name && tag)
    {
    char *s;
    int found = 0;

    while (occ--)
      {
      if (XP_STRSTR (name, tag))
        {
        name = XP_STRSTR (name, tag) + XP_STRLEN (tag);
        found = 1;
        }
      else
        {
        name = XP_STRSTR (name, "=");
        if (name == NULL) return NULL;
        found = 0;
        }
      }

    if (!found) return NULL;

    /* must mangle only the copy */
    name = XP_STRDUP (name);

    /* advance to next equal */
    for (s = name; *s && *s != '='; s++)
      /* yip */ ;

    /* back up to previous comma */
    while (s > name && *s != ',') s--;

    /* zap the whitespace and return */
    *s = 0;
    }

  return name;
  }

/* 
 *  s o b _ c h o o s e _ n i c k n a m e
 *
 *  Attempt to determine a suitable nickname for
 *  a certificate with a computer-generated "tmpcertxxx" 
 *  nickname. It needs to be something a user can
 *  understand, so try a few things.
 *
 */

static char *sob_choose_nickname (CERTCertificate *cert)
  {
  char *cert_cn;
  char *cert_o;
  char *cert_cn_o;

  /* is the existing name ok */

  if (cert->nickname && XP_STRNCMP (cert->nickname, "tmpcert", 7))
    return XP_STRDUP (cert->nickname);

  /* we have an ugly name here people */

  /* Try the CN */
  cert_cn = sob_cert_element (cert->subjectName, "CN=", 1);

  if (cert_cn)
    {
    /* check for duplicate nickname */

    if (CERT_FindCertByNickname (CERT_GetDefaultCertDB(), cert_cn) == NULL)
      return cert_cn;

    /* Try the CN plus O */
    cert_o = sob_cert_element (cert->subjectName, "O=", 1);

    cert_cn_o = XP_CALLOC 
      (1, XP_STRLEN (cert_cn) + 3 + XP_STRLEN (cert_o) + 20);

    XP_SPRINTF (cert_cn_o, "%s's %s Certificate", cert_cn, cert_o);

    if (CERT_FindCertByNickname (CERT_GetDefaultCertDB(), cert_cn_o) == NULL)
      return cert_cn;
    }

  /* If all that failed, use the ugly nickname */
  return cert->nickname ? XP_STRDUP (cert->nickname) : NULL;
  }

/*
 *  S O B _ c e r t _ h t m l 
 *
 *  Return an HTML representation of the certificate
 *  designated by the given fingerprint, in specified style.
 *
 *  ZIG is optional, but supply it if you can in order
 *  to optimize.
 *
 */

char *SOB_cert_html
    (int style, ZIG *zig, long keylen, void *key, int *result)
  {
  char *html;
  CERTCertificate *cert;

  *result = -1;

  if (style != 0)
    return NULL;

  cert = sob_get_certificate (zig, keylen, key, result);

  if (cert == NULL || *result < 0)
    return NULL;

  *result = 0;

  html = CERT_HTMLCertInfo (cert, /* show images */ PR_TRUE);

  if (html == NULL)
    *result = -1;

  return html;
  }

/*
 *  S O B _ s t a s h _ c e r t
 *
 *  Stash the certificate pointed to by this
 *  fingerprint, in persistent storage somewhere.
 *
 */

extern int PR_CALLBACK SOB_stash_cert
    (ZIG *zig, long keylen, void *key)
  {
  int result = 0;

  char *nickname;
  CERTCertTrust trust;

  CERTCertDBHandle *certdb;
  CERTCertificate *cert, *newcert;

  cert = sob_get_certificate (zig, keylen, key, &result);

  if (result < 0)
    return result;

  if (cert == NULL)
    return ZIG_ERR_GENERAL;

  if ((certdb = SOB_open_database()) == NULL)
    return ZIG_ERR_GENERAL;

  /* Attempt to give a name to the newish certificate */
  nickname = sob_choose_nickname (cert);

  newcert = CERT_FindCertByNickname (certdb, nickname);

  if (newcert && newcert->isperm) 
    {
    /* already in permanant database */
    return 0;
    }

  if (newcert) cert = newcert;

  /* FIX, since FindCert returns a bogus dbhandle
     set it ourselves */

  cert->dbhandle = certdb;

#if 0
  nickname = cert->subjectName;
  if (nickname)
    {
    /* Not checking for a conflict here. But this should
       be a new cert or it would have been found earlier. */

    nickname = sob_cert_element (nickname, "CN=", 1);

    if (SEC_CertNicknameConflict (nickname, cert->dbhandle))
      {
      /* conflict */
      nickname = XP_REALLOC (&nickname, XP_STRLEN (nickname) + 3);

      /* Beyond one copy, there are probably serious problems 
         so we will stop at two rather than counting.. */

      XP_STRCAT (nickname, " #2");
      }
    }
#endif

  if (nickname != NULL)
    {
    XP_MEMSET ((void *) &trust, 0, sizeof(trust));
    if (CERT_AddTempCertToPerm (cert, nickname, &trust) != SECSuccess) 
      {
      /* XXX might want to call XP_GetError here */
      result = ZIG_ERR_GENERAL;
      }
    }

  SOB_close_database (certdb);

  return result;
  }

/*
 *  S O B _ f e t c h _ c e r t
 *
 *  Given an opaque identifier of a certificate, 
 *  return the full certificate.
 *
 * The new function, which retrieves by key.
 *
 */

void *SOB_fetch_cert (long length, void *key)
  {
  SECItem seckey;
  CERTCertificate *cert = NULL;

  CERTCertDBHandle *certdb;

  certdb = SOB_open_database();

  if (certdb)
    {
    seckey.len = length;
    seckey.data = key;

    cert = CERT_FindCertByKey (certdb, &seckey);

    SOB_close_database (certdb);
    }

  return (void *) cert;
  }

/*
 *  + + + + + + + + + + + + + + +
 *
 *  ZIP ROUTINES FOR ZIG
 *
 *  The following functions are the interface
 *  to underlying physical archive files.
 *
 *  + + + + + + + + + + + + + + +
 *
 */

static PHYZIG *get_physical (ZIG *zig, char *pathname);
static int sob_physical_extraction 
   (XP_File fp, char *outpath, long offset, long length);
#ifndef AKBAR
static int sob_physical_inflate
   (XP_File fp, char *outpath, long offset, long length, unsigned int method);
#endif /* !AKBAR */
static int verify_extract (ZIG *zig, char *path, char *physical_path);
static int zig_arch (int format, char *filename, const char *url, ZIG *zig);
static int sob_extract_manifests (int format, XP_File fp, ZIG *zig);
static int sob_extract_mf (int format, XP_File fp, ZIG *zig, char *ext);
static int sob_gen_index (int format, XP_File fp, ZIG *zig);
static int sob_listzip (XP_File fp, ZIG *zig);
static int sob_listtar (XP_File fp, ZIG *zig);
static int dosdate (char *time, char *s);
static int dostime (char *time, char *s);
static unsigned int xtoint (unsigned char *ii);
static unsigned long xtolong (unsigned char *ll);
static long atoo (char *s);
static int sob_guess_jar (char *filename, XP_File fp);

#define SIZE 256


/*
 *  S O B _ p a s s _ a r c h i v e
 *
 *  For use by naive clients. Slam an entire archive file
 *  into this function. We extract manifests, parse, index
 *  the archive file, and do whatever nastiness.
 *
 */

int SOB_pass_archive
    (int format, char *filename, const char *url, ZIG *zig)
  {
  XP_File fp;
  int status = 0;

  if (filename == NULL)
    return ZIG_ERR_GENERAL;

  if ((fp = XP_FileOpen (filename, xpURL, "rb")) != NULL)
    {
    if (format == ZIG_F_GUESS)
      format = sob_guess_jar (filename, fp);

    zig->format = format;
    zig->url = url ? XP_STRDUP (url) : NULL;
    zig->filename = XP_STRDUP (filename);

    status = sob_gen_index (format, fp, zig);

    if (status == 0)
      status = sob_extract_manifests (format, fp, zig);

    XP_FileClose (fp);

    if (status < 0)
      return status;

    /* people were expecting it this way */
    return zig->valid;
    }
  else
    {
    /* file not found */
    return ZIG_ERR_FNF;
    }
  }

/*
 *  S O B _ v e r i f i e d _ e x t r a c t
 *
 *  Optimization: keep a file descriptor open
 *  inside the ZIG structure, so we don't have to
 *  open the file 25 times to run java. 
 *
 */

int SOB_verified_extract
    (ZIG *zig, char *path, char *outpath)
  {
  int status;

  status = SOB_extract (zig, path, outpath);

  if (status >= 0)
    return verify_extract (zig, path, outpath);
  else
    return status;
  }

int SOB_extract
    (ZIG *zig, char *path, char *outpath)
  {
  int result;

  XP_File fp;
  char *filename;

  PHYZIG *phy;

  filename = SOB_get_filename (zig);

  if (zig->nailed == 0)
    {
    if ((fp = XP_FileOpen (filename, xpURL, "rb")) == NULL)
      {
      /* can't open archive file */
      return ZIG_ERR_FNF;
      }
    }
  else 
    fp = zig->fp;

  phy = get_physical (zig, path);

  if (phy)
    {
    if (phy->compression != 0 && phy->compression != 8)
      {
      /* unsupported compression method */
      result = ZIG_ERR_CORRUPT;
      }

    if (phy->compression == 0)
      result = sob_physical_extraction (fp, outpath, phy->offset, phy->length);
    else
#ifndef AKBAR
      result = sob_physical_inflate (fp, outpath, phy->offset, phy->length, (unsigned int) phy->compression);
#else  /* AKBAR */
      result = -1;
#endif /* AKBAR */
    }
  else
    {
    /* pathname not found in archive */
    result = ZIG_ERR_PNF;
    }

  if (zig->nailed == 0)
    XP_FileClose (fp);

  return result;
  }

/* 
 *  p h y s i c a l _ e x t r a c t i o n
 *
 *  This needs to be done in chunks of say 32k, instead of
 *  in one bulk calloc. (Necessary under Win16 platform.)
 *  This is done for uncompressed entries only.
 *
 */

#define CHUNK 32768

static int sob_physical_extraction 
     (XP_File fp, char *outpath, long offset, long length)
  {
  XP_File out;

  char *buffer;
  long at, chunk;

  int status = 0;

  buffer = (char *) XP_CALLOC (1, CHUNK);

  if (buffer == NULL)
    return ZIG_ERR_MEMORY;

  if ((out = XP_FileOpen (outpath, xpURL, "wb")) != NULL)
    {
    at = 0;

    XP_FileSeek (fp, offset, 0);

    while (at < length)
      {
      chunk = (at + CHUNK <= length) ? CHUNK : length - at;

      if (XP_FileRead (buffer, chunk, fp) != chunk)
        {
        status = ZIG_ERR_DISK;
        break;
        }

      at += chunk;

      if (XP_FileWrite (buffer, chunk, out) < chunk)
        {
        /* most likely a disk full error */
        status = ZIG_ERR_DISK;
        break;
        }
      }
    XP_FileClose (out);
    }
  else
    {
    /* error opening output file */
    status = ZIG_ERR_DISK;
    }

  XP_FREE (buffer);
  return status;
  }

#ifndef AKBAR
/*
 *  s o b _ p h y s i c a l _ i n f l a t e
 *
 *  Inflate a range of bytes in a file, writing the inflated
 *  result to "outpath". Chunk based.
 *
 */

/* input and output chunks differ, assume 4x compression */

#define ICHUNK 8192
#define OCHUNK 32768

static int sob_physical_inflate
     (XP_File fp, char *outpath, long offset, long length, unsigned int method)
  {
  z_stream zs;

  XP_File out;

  long at, chunk;
  char *inbuf, *outbuf;

  int status = 0;

  unsigned long prev_total, ochunk, tin;

  if ((inbuf = (char *) XP_CALLOC (1, ICHUNK)) == NULL)
    return ZIG_ERR_MEMORY;

  if ((outbuf = (char *) XP_CALLOC (1, OCHUNK)) == NULL)
    {
    XP_FREE (inbuf);
    return ZIG_ERR_MEMORY;
    }

  XP_MEMSET (&zs, 0, sizeof (zs));
  status = inflateInit2 (&zs, -MAX_WBITS);

  if (status != Z_OK)
    return ZIG_ERR_GENERAL;

  if ((out = XP_FileOpen (outpath, xpURL, "wb")) != NULL)
    {
    at = 0;

    XP_FileSeek (fp, offset, 0);

    while (at < length)
      {
      chunk = (at + ICHUNK <= length) ? ICHUNK : length - at;

      if (XP_FileRead (inbuf, chunk, fp) != chunk)
        {
        /* incomplete read */
        return ZIG_ERR_CORRUPT;
        }

      at += chunk;

      zs.next_in = (Bytef *) inbuf;
      zs.next_out = (Bytef *) outbuf;

      zs.avail_in = chunk;
      zs.avail_out = OCHUNK;

      tin = zs.total_in;

      while (zs.total_in - tin < chunk)
        {
        prev_total = zs.total_out;

        status = inflate (&zs, Z_NO_FLUSH);

        if (status != Z_OK && status != Z_STREAM_END)
          {
          /* error during decompression */
          return ZIG_ERR_CORRUPT;
          }

	ochunk = zs.total_out - prev_total;

        if (XP_FileWrite (outbuf, ochunk, out) < ochunk)
          {
          /* most likely a disk full error */
          status = ZIG_ERR_DISK;
          break;
          }

        if (status == Z_STREAM_END)
          break;
        }
      }

    XP_FileClose (out);
    status = inflateEnd (&zs);
    }
  else
    {
    /* error opening output file */
    status = ZIG_ERR_DISK;
    }

  XP_FREE (inbuf);
  XP_FREE (outbuf);

  return status;
  }

/*
 *  s o b _ i n f l a t e _ m e m o r y
 *
 *  Call zlib to inflate the given memory chunk. It is re-XP_ALLOC'd, 
 *  and thus appears to operate inplace to the caller.
 *
 */

static int sob_inflate_memory (unsigned int method, long *length, char XP_HUGE **data)
  {
  int status;
  z_stream zs;

  long insz, outsz;

  char *inbuf, *outbuf;

  inbuf =  *data;
  insz = *length;

  /* factor of 10. FIX: Should use stored size in archive though! */

  outsz = insz * 10;
  outbuf = XP_CALLOC (1, outsz);

  XP_MEMSET (&zs, 0, sizeof (zs));

  status = inflateInit2 (&zs, -MAX_WBITS);

  if (status < 0)
    {
    /* error initializing zlib stream */
    return ZIG_ERR_GENERAL;
    }

  zs.next_in = (Bytef *) inbuf;
  zs.next_out = (Bytef *) outbuf;

  zs.avail_in = insz;
  zs.avail_out = outsz;

  status = inflate (&zs, Z_FINISH);

  if (status != Z_OK && status != Z_STREAM_END)
    {
    /* error during deflation */
    return ZIG_ERR_GENERAL; 
    }

  status = inflateEnd (&zs);

  if (status != Z_OK)
    {
    /* error during deflation */
    return ZIG_ERR_GENERAL;
    }

  *data = outbuf;
  *length = zs.total_out;

  return 0;
  }
#endif /* !AKBAR */

/*
 *  v e r i f y _ e x t r a c t
 *
 *  Validate signature on the freshly extracted file.
 *
 */

static int verify_extract (ZIG *zig, char *path, char *physical_path)
  {
  DIGESTS dig;
  int result, status;

  XP_MEMSET (&dig, 0, sizeof (DIGESTS));
  status = SOB_digest_file (physical_path, &dig);

  if (!status)
    status = SOB_verify_digest (&result, zig, path, &dig);

  return status < 0 ? status : result;
  }

/*
 *  g e t _ p h y s i c a l
 *
 *  Let's get physical. An ode to Olivia Newton John.
 *  Obtains the offset and length of this file in the jar file.
 *
 */

static PHYZIG *get_physical (ZIG *zig, char *pathname)
  {
  SOBITEM *it;

  PHYZIG *phy;

  ZSLink *link;
  ZSList *list;

  list = (ZSList *) zig->list;

  if (ZS_ListEmpty (list))
    return NULL;

  for (link = ZS_ListHead (list);
       !ZS_ListIterDone (list, link);
       link = link->next)
    {
    it = (SOBITEM *) link->thing;
    if (it->type == ZIG_PHY && it->pathname && !XP_STRCMP (it->pathname, pathname))
      {
      phy = (PHYZIG *) it->data;
      return phy;
      }
    }

  return NULL;
  }

/*
 *  s o b _ g e t _ m f _ d i g e s t
 *
 *  Retrieve a corresponding saved digest over a section
 *  of the main manifest file. 
 *
 */

static DIGESTS *sob_get_mf_digest (ZIG *zig, char *pathname)
  {
  SOBITEM *it;

  DIGESTS *dig;

  ZSLink *link;
  ZSList *list;

  list = (ZSList *) zig->manifest;

  if (ZS_ListEmpty (list))
    return NULL;

  for (link = ZS_ListHead (list);
       !ZS_ListIterDone (list, link);
       link = link->next)
    {
    it = (SOBITEM *) link->thing;
    if (it->type == ZIG_SECT && it->pathname && !XP_STRCMP (it->pathname, pathname))
      {
      dig = (DIGESTS *) it->data;
      return dig;
      }
    }

  return NULL;
  }

/*
 *  s o b _ e x t r a c t _ m a n i f e s t s
 *
 *  Extract the manifest files and parse them,
 *  from an open archive file whose contents are known.
 *
 */

static int sob_extract_manifests (int format, XP_File fp, ZIG *zig)
  {
  int status;

  if (format != ZIG_F_ZIP && format != ZIG_F_TAR)
    return ZIG_ERR_CORRUPT;

  if ((status = sob_extract_mf (format, fp, zig, "mf")) < 0)
    return status;

  if ((status = sob_extract_mf (format, fp, zig, "sf")) < 0)
    return status;

  if ((status = sob_extract_mf (format, fp, zig, "rsa")) < 0)
    return status;

  if ((status = sob_extract_mf (format, fp, zig, "dsa")) < 0)
    return status;

  return 0;
  }

/*
 *  s o b _ e x t r a c t _ m f
 *
 *  Extracts manifest files based on an extension, which 
 *  should be .MF, .SF, .RSA, etc. Order of the files is now no 
 *  longer important when zipping jar files.
 *
 */

static int sob_extract_mf (int format, XP_File fp, ZIG *zig, char *ext)
  {
  SOBITEM *it;

  PHYZIG *phy;

  ZSLink *link;
  ZSList *list;

  char *fn, *e;
  char XP_HUGE *manifest;

  long length;
  int status, ret = 0, num;

  list = (ZSList *) zig->list;

  if (ZS_ListEmpty (list))
    return ZIG_ERR_PNF;

  for (link = ZS_ListHead (list);
       !ZS_ListIterDone (list, link);
       link = link->next)
    {
    it = (SOBITEM *) link->thing;
    if (it->type == ZIG_PHY && !XP_STRNCMP (it->pathname, "META-INF", 8))
      {
      phy = (PHYZIG *) it->data;

      if (XP_STRLEN (it->pathname) < 8)
        continue;

      fn = it->pathname + 8;
      if (*fn == '/' || *fn == '\\') fn++;

      if (*fn == 0)
        {
        /* just a directory entry */
        continue;
        }

      /* skip to extension */
      for (e = fn; *e && *e != '.'; e++)
        /* yip */ ;

      /* and skip dot */
      if (*e == '.') e++;

      if (XP_STRCASECMP (ext, e))
        {
        /* not the right extension */
        continue;
        }

      if (phy->length == 0)
        {
        /* manifest files cannot be zero length! */
        return ZIG_ERR_CORRUPT;
        }

      /* Read in the manifest and parse it */
      /* FIX? Does this break on win16 for very very large manifest files? */

#ifdef XP_WIN16
      XP_ASSERT( phy->length+1 < 0xFFFF );
#endif

      manifest = (char XP_HUGE *) XP_CALLOC (1, phy->length + 1);
      if (manifest)
        {
        XP_FileSeek (fp, phy->offset, 0);
        num = XP_FileRead (manifest, phy->length, fp);

        if (num != phy->length)
          {
          /* corrupt archive file */
          return ZIG_ERR_CORRUPT;
          }

        if (phy->compression == 8)
          {
#ifndef AKBAR
          length = phy->length;

          status = sob_inflate_memory ((unsigned int) phy->compression, &length, &manifest);

          if (status < 0)
            return status;
#endif /* !AKBAR */
	  return -1;
          }
        else if (phy->compression)
          {
          /* unsupported compression method */
          return ZIG_ERR_CORRUPT;
          }
        else
          length = phy->length;

        status = SOB_parse_manifest 
           (manifest, length, it->pathname, "url", zig);

        XP_FREE (manifest);

        if (status < 0 && ret == 0) ret = status;
        }
      else
        return ZIG_ERR_MEMORY;
      }
    else if (it->type == ZIG_PHY)
      {
      /* ordinary file */
      }
    }

  return ret;
  }

/*
 *  s o b _ g e n _ i n d e x
 *
 *  Generate an index for the various types of
 *  known archive files. Right now .ZIP and .TAR
 *
 */

static int sob_gen_index (int format, XP_File fp, ZIG *zig)
  {
  int result = ZIG_ERR_CORRUPT;
  rewind (fp);

  switch (format)
    {
    case ZIG_F_ZIP:
      result = sob_listzip (fp, zig);
      break;

    case ZIG_F_TAR:
      result = sob_listtar (fp, zig);
      break;
    }

  rewind (fp);
  return result;
  }


/*
 *  s o b _ l i s t z i p
 *
 *  List the physical contents of a Phil Katz 
 *  style .ZIP file into the ZIG linked list.
 *
 */

static int sob_listzip (XP_File fp, ZIG *zig)
  {
  long pos = 0L;
  char filename [SIZE];

  char date [9], time [9];
  char sig [4];

  unsigned int compression;
  unsigned int filename_len;

  struct ZipLocal *Local;
  struct ZipCentral *Central;
  struct ZipEnd *End;

  /* phy things */

  ZSLink  *ent;
  SOBITEM *it;
  PHYZIG  *phy;

  Local = (struct ZipLocal *) XP_CALLOC (1, 30);
  Central = (struct ZipCentral *) XP_CALLOC (1, 46);
  End = (struct ZipEnd *) XP_CALLOC (1, 22);

  if (!Local || !Central || !End)
    {
    /* out of memory */
    return ZIG_ERR_MEMORY;
    }

  while (1)
    {
    XP_FileSeek (fp, pos, 0);

    if (XP_FileRead ((char *) sig, 4, fp) != 4)
      {
      /* zip file ends prematurely */
      return ZIG_ERR_CORRUPT;
      }

    XP_FileSeek (fp, pos, 0);

    if (xtolong ((unsigned char *)sig) == LSIG)
      {
      XP_FileRead ((char *) Local, 30, fp);

      filename_len = xtoint ((unsigned char *) Local->filename_len);

      if (filename_len >= SIZE)
        {
        /* corrupt zip file */
        return ZIG_ERR_CORRUPT;
        }

      if (XP_FileRead (filename, filename_len, fp) != filename_len)
        {
        /* truncated archive file */
        return ZIG_ERR_CORRUPT;
        }

      filename [filename_len] = 0;

      /* Add this to our zig chain */

      phy = (PHYZIG *) XP_CALLOC (1, sizeof (PHYZIG));

      phy->path = XP_STRDUP (filename);

      /* We will index any file that comes our way, but when it comes
         to actually extraction, compression must be 0 or 8 */

      compression = xtoint ((unsigned char *) Local->method);
      phy->compression = compression >= 0 && compression <= 255 ? compression : 222;

      phy->offset = pos + 30 + filename_len
       + xtoint ((unsigned char *) Local->extrafield_len);
      phy->length = xtolong ((unsigned char *) Local->size);

      dosdate (date, Local->date);
      dostime (time, Local->time);

      it = XP_CALLOC (1, sizeof (SOBITEM));

      if (it == NULL)
        return ZIG_ERR_MEMORY;

      it->pathname = XP_STRDUP (filename);

      it->type = ZIG_PHY;

      it->data = (unsigned char *) phy;
      it->size = sizeof (PHYZIG);

      ent = ZS_NewLink (it);
      ZS_AppendLink (zig->list, ent);
 
      pos = phy->offset + phy->length;
      }
    else if (xtolong ( (unsigned char *)sig) == CSIG)
      {
      if (XP_FileRead ((char *) Central, 46, fp) != 46)
        {
        /* apparently truncated archive */
        return ZIG_ERR_CORRUPT;
        }

      pos += 46 + xtoint ( (unsigned char *)Central->filename_len)
                + xtoint ( (unsigned char *)Central->commentfield_len)
                + xtoint ( (unsigned char *)Central->extrafield_len);
      }
    else if (xtolong ( (unsigned char *)sig) == ESIG)
      {
      if (XP_FileRead ((char *) End, 22, fp) != 22)
        return ZIG_ERR_CORRUPT;
      else
        break;
      }
    else
      {
      /* garbage in zip archive */
      return ZIG_ERR_CORRUPT;
      }
    }

  XP_FREE (Local);
  XP_FREE (Central);
  XP_FREE (End);

  return 0;
  }

/*
 *  s o b _ l i s t t a r
 *
 *  List the physical contents of a Unix 
 *  .tar file into the ZIG linked list.
 *
 */

static int sob_listtar (XP_File fp, ZIG *zig)
  {
  long pos = 0L;

  long sz, mode;
  time_t when;
  union TarEntry tarball;

  char *s;

  /* phy things */

  ZSLink  *ent;
  SOBITEM *it;
  PHYZIG  *phy;

  while (1)
    {
    XP_FileSeek (fp, pos, 0);

    if (XP_FileRead ((char *) &tarball, 512, fp) < 1)
      break;

    if (!*tarball.val.filename)
      break;

    when = atoo (tarball.val.time);
    sz = atoo (tarball.val.size);
    mode = atoo (tarball.val.mode);

    phy = (PHYZIG *) XP_CALLOC (1, sizeof (PHYZIG));

    if (phy == NULL)
      return ZIG_ERR_MEMORY;

    /* Tag the end of filename */

    s = tarball.val.filename;
    while (*s && *s != ' ') s++;
    *s = 0;

    phy->path = XP_STRDUP (tarball.val.filename);
    phy->compression = 0;
    phy->offset = pos + 512;
    phy->length = sz;

    it = XP_CALLOC (1, sizeof (SOBITEM));

    if (it == NULL)
      return ZIG_ERR_MEMORY;

    it->pathname = XP_STRDUP (tarball.val.filename);
    it->type = ZIG_PHY;

    it->data = (unsigned char *) phy;
    it->size = sizeof (PHYZIG);

    ent = ZS_NewLink (it);
    ZS_AppendLink (zig->list, ent);

    /* Advance to next file entry */

    sz += 511;
    sz = (sz / 512) * 512;

    pos += sz + 512;
    }

  return 0;
  }

/*
 *  d o s d a t e
 *
 *  Not used right now, but keep it in here because
 *  it will be needed. 
 *
 */

static int dosdate (char *date, char *s)
  {
  int num = xtoint ( (unsigned char *)s);

  sprintf (date, "%02d-%02d-%02d",
     ((num >> 5) & 0x0F), (num & 0x1F), ((num >> 9) + 80));

  return 0;
  }

/*
 *  d o s t i m e
 *
 *  Not used right now, but keep it in here because
 *  it will be needed. 
 *
 */

static int dostime (char *time, char *s)
  {
  int num = xtoint ( (unsigned char *)s);

  sprintf (time, "%02d:%02d",
     ((num >> 11) & 0x1F), ((num >> 5) & 0x3F));

  return 0;
  }

/*
 *  x t o i n t
 *
 *  Converts a two byte ugly endianed integer
 *  to our platform's integer.
 *
 */

static unsigned int xtoint (unsigned char *ii)
  {
  return (int) (ii [0]) | ((int) ii [1] << 8);
  }

/*
 *  x t o l o n g
 *
 *  Converts a four byte ugly endianed integer
 *  to our platform's integer.
 *
 */

static unsigned long xtolong (unsigned char *ll)
  {
  unsigned long ret;

  ret =  (
         (((unsigned long) ll [0]) <<  0) |
         (((unsigned long) ll [1]) <<  8) |
         (((unsigned long) ll [2]) << 16) |
         (((unsigned long) ll [3]) << 24)
         );

  return ret;
  }

/*
 *  a t o o
 *
 *  Ascii octal to decimal.
 *  Used for integer encoding inside tar files.
 *
 */

static long atoo (char *s)
  {
  long num = 0L;

  while (*s == ' ') s++;

  while (*s >= '0' && *s <= '7')
    {
    num <<= 3;
    num += *s++ - '0';
    }

  return num;
  }

/*
 *  g u e s s _ j a r
 *
 *  Try to guess what kind of JAR file this is.
 *  Maybe tar, maybe zip. Look in the file for magic
 *  or at its filename.
 *
 */

static int sob_guess_jar (char *filename, XP_File fp)
  {
  char *ext;

  ext = filename + XP_STRLEN (filename) - 4;

  if (!XP_STRCMP (ext, ".tar"))
    return ZIG_F_TAR;

  return ZIG_F_ZIP;
  }

/*
 *  s o b _ b a s e n a m e
 *
 *  Return the basename -- leading components of path stripped off,
 *  extension ripped off -- of a path.
 *
 */

static char *sob_basename (const char *path)
  {
  char *pith, *e, *basename, *ext;

  if (path == NULL)
    return XP_STRDUP ("");

  pith = XP_STRDUP (path);

  basename = pith;

  while (1)
    {
    for (e = basename; *e && *e != '/' && *e != '\\'; e++)
      /* yip */ ;
    if (*e) 
      basename = ++e; 
    else
      break;
    }

  if ((ext = XP_STRRCHR (basename, '.')) != NULL)
    *ext = 0;

  /* We already have the space allocated */
  XP_STRCPY (pith, basename);

  return pith;
  }

/*
 *  + + + + + + + + + + + + + + +
 *
 *  CRYPTO ROUTINES FOR ZIG
 *
 *  The following functions are the cryptographic 
 *  interface to PKCS7 for Zignatures.
 *
 *  + + + + + + + + + + + + + + +
 *
 */

/*
 *  s o b _ c a t c h _ b y t e s
 *
 *  In the event signatures contain enveloped data (ie JavaSoft
 *  might be trying this), it will show up here. But note that the
 *  lib/pkcs7 routines aren't ready for it.
 *
 */

static void sob_catch_bytes
     (void *arg, const char *buf, unsigned long len)
  {
  /* Actually this should never be called, since there is
     presumably no data in the signature itself. */
  }

/*
 *  s o b _ v a l i d a t e _ p k c s 7
 *
 *  Validate (and decode, if necessary) a binary pkcs7
 *  signature in DER format.
 *
 */

static int sob_validate_pkcs7 (ZIG *zig, char *data, long length)
  {
  SECItem detdig;

  int encrypted, sign;

  SEC_PKCS7ContentInfo *cinfo;
  SEC_PKCS7DecoderContext *dcx;

  int status = 0;
  char *errstring = NULL;

  zig->valid = -1;

  /* We need a context if we can get one */

  if (zig->mw == NULL)
    SOB_set_context (zig, NULL);

  dcx = SEC_PKCS7DecoderStart 
           (sob_catch_bytes, NULL /*cb_arg*/, NULL /*getpassword*/, zig->mw);

  if (dcx != NULL) 
    {
    SEC_PKCS7DecoderUpdate (dcx, data, length);
    cinfo = SEC_PKCS7DecoderFinish (dcx);
    }

  if (cinfo == NULL)
    {
    /* strange pkcs7 failure */
    return ZIG_ERR_PK7;
    }

  encrypted = SEC_PKCS7ContentIsEncrypted (cinfo);

  if (encrypted)
    {
    /* content was encrypted, fail */
    return ZIG_ERR_PK7;
    }

  sign = SEC_PKCS7ContentIsSigned (cinfo);

  if (sign == PR_FALSE)
    {
    /* content was not signed, fail */
    return ZIG_ERR_PK7;
    }

  XP_SetError (0);

  detdig.len = zig->digest_length;
  detdig.data = zig->digest;

  if (SEC_PKCS7VerifyDetachedSignature 
        (cinfo, certUsageObjectSigner, &detdig, HASH_AlgSHA1, PR_FALSE))
    {
    /* signature is valid */
    zig->valid = 0;
    sob_gather_signers (zig, cinfo);
    }
  else
    {
    zig->valid = -1;
    status = XP_GetError();
    errstring = XP_GetString (status);
    XP_TRACE (("JAR signature invalid (reason %d = %s)", status, errstring));
    }

  zig->pkcs7 = 1;
  SEC_PKCS7DestroyContentInfo (cinfo);

  return status;
  }

/*
 *  s o b _ g a t h e r _ s i g n e r s
 *
 *  Add the single signer of this signature to the
 *  certificate linked list.
 *
 */

static int sob_gather_signers
     (ZIG *zig, SEC_PKCS7ContentInfo *cinfo)
  {
  int result;

  CERTCertificate *cert, *issuer;
  CERTCertDBHandle *certdb;

  SEC_PKCS7SignedData *sdp;
  SEC_PKCS7SignerInfo **signers, *signer;

  sdp = cinfo->content.signedData;

  if (sdp == NULL)
    return ZIG_ERR_PK7;

  signers = sdp->signerInfos;

  /* permit exactly one signer */

  if (signers == NULL || signers [0] == NULL || signers [1] != NULL)
    return ZIG_ERR_PK7;

  signer = *signers;
  cert = signer->cert;

  if (cert == NULL)
    return ZIG_ERR_PK7;

  certdb = SOB_open_database();

  if (certdb == NULL)
    return ZIG_ERR_GENERAL;

  result = sob_add_cert (zig, ZIG_SIGN, cert);

  /* gather the cert chain */

  while (result == 0 && cert)
    {
    result = sob_add_cert (zig, ZIG_WALK, cert);

    issuer = CERT_FindCertIssuer (cert);  

    if (cert == issuer) 
      break;

    cert = issuer;
    }

  SOB_close_database (certdb);

  return result;
  }

/*
 *  s o b _ o p e n _ d a t a b a s e
 *
 *  Open the certificate database,
 *  for use by ZIG functions.
 *
 */

CERTCertDBHandle *SOB_open_database (void)
  {
  int keepcerts = 0;
  CERTCertDBHandle *certdb;

  /* local_certdb will only be used if calling from a command line tool */
  static CERTCertDBHandle local_certdb;

  certdb = CERT_GetDefaultCertDB();

  if (certdb == NULL) 
    {
    if (CERT_OpenCertDBFilename (&local_certdb, NULL, !keepcerts) != SECSuccess)
      {
      return NULL;
      }
    certdb = &local_certdb;
    }

  return certdb;
  }

/*
 *  s o b _ c l o s e _ d a t a b a s e
 *
 *  Close the certificate database.
 *  For use by ZIG functions.
 *
 */

int SOB_close_database (CERTCertDBHandle *certdb)
  {
  CERTCertDBHandle *defaultdb;

  /* This really just retrieves the handle, nothing more */
  defaultdb = CERT_GetDefaultCertDB();

  /* If there is no default db, it means we opened 
     the permanent database for some reason */

  if (defaultdb == NULL && certdb != NULL)
    CERT_ClosePermCertDB (certdb);

  return 0;
  }

/*
 *  s o b _ g e t _ c e r t i f i c a t e
 *
 *  Return the certificate referenced
 *  by a given fingerprint, or NULL if not found.
 *  Error code is returned in result.
 *
 */

static CERTCertificate *sob_get_certificate
      (ZIG *zig, long keylen, void *key, int *result)
  {
  int found = 0;

  SOBITEM *it;
  FINGERZIG *fing;

  ZSLink *link;
  ZSList *list;

  if (zig == NULL) 
    {
    void *cert;
    cert = SOB_fetch_cert (keylen, key);
    *result = (cert == NULL) ? ZIG_ERR_GENERAL : 0;
    return (CERTCertificate *) cert;
    }

  /* list must point to zig->cer, not zig->list */
  list = (ZSList *) zig->cer;

  if (ZS_ListEmpty (list))
    return NULL;

  /* Loop over all the FP's we've stashed, and
     check for this specific fingerprint */

  for (link = ZS_ListHead (list);
       !ZS_ListIterDone (list, link);
       link = link->next)
    {
    it = (SOBITEM *) link->thing;
    if (it->type == ZIG_FP2)
      {
      fing = (FINGERZIG *) it->data;

      if (keylen != fing->length)
        continue;

      XP_ASSERT( keylen < 0xFFFF );
      if (!XP_MEMCMP (fing->key, key, keylen))
        {
        found = 1;
        break;
        }
      }
    }

  if (found == 0)
    {
    *result = ZIG_ERR_GENERAL;
    return NULL;
    }

  *result = 0;
  return fing->cert;
  }

/*
 *  S O B _ l i s t _ c e r t s
 *
 *  Return a list of newline separated certificate nicknames
 *  (this function used by the Jartool)
 * 
 */

static SECStatus sob_list_cert_callback (CERTCertificate *cert, SECItem *k, void *data)
  {
  char *name;
  char **ugly_list;

  int trusted;

  ugly_list = (char **) data;

  if (cert && cert->dbEntry)
    {
    /* name = cert->dbEntry->nickname; */
    name = cert->nickname;

    trusted = cert->trust->objectSigningFlags & CERTDB_USER;

    /* Add this name or email to list */

    if (name && trusted)
      {
      *ugly_list = XP_REALLOC (*ugly_list, XP_STRLEN (*ugly_list) + XP_STRLEN (name) + 2);

      if (*ugly_list)
        {
        if (**ugly_list)
          XP_STRCAT (*ugly_list, "\n");

        XP_STRCAT (*ugly_list, name);
        }
      }
    }

  return (SECSuccess);
  }

/*
 *  S O B _ J A R _ l i s t _ c e r t s
 *
 *  Return a linfeed separated ascii list of certificate
 *  nicknames for the Jartool.
 *
 */

char *SOB_JAR_list_certs (void)
  {
  SECStatus status;
  CERTCertDBHandle *certdb;

  char *ugly_list;

  certdb = SOB_open_database();

  /* a little something */
  ugly_list = XP_CALLOC (1, 16);

  if (ugly_list)
    {
    *ugly_list = 0;

    status = SEC_TraversePermCerts 
            (certdb, sob_list_cert_callback, (void *) &ugly_list);
    }

  SOB_close_database (certdb);

  return status ? NULL : ugly_list;
  }

/*
 *  s o b _ s i g n a l
 *
 *  Nonfatal errors come here to callback Java.
 *  
 */

static int sob_signal (int status, ZIG *zig, const char *metafile, char *pathname)
  {
  char *errstring;

  errstring = SOB_get_error (status);

  if (zig->signal)
    {
    (*zig->signal) (status, zig, metafile, pathname, errstring);
    return 0;
    }

  return status;
  }

/* 
 *  W I N 1 6   c r u f t
 *
 *  These functions possibly belong in xp_mem.c, they operate 
 *  on huge string pointers for win16.
 *
 */

#if defined(XP_WIN16)
int xp_HUGE_STRNCASECMP (char XP_HUGE *buf, char *key, int len)
  {
  while (len--) 
    {
    char c1, c2;

    c1 = *buf++;
    c2 = *key++;

    if (c1 >= 'a' && c1 <= 'z') c1 -= ('a' - 'A');
    if (c2 >= 'a' && c2 <= 'z') c2 -= ('a' - 'A');

    if (c1 != c2) 
      return (c1 < c2) ? -1 : 1;
    }
  return 0;
  }

size_t xp_HUGE_STRLEN (char XP_HUGE *s)
  {
  size_t len = 0L;
  while (*s++) len++;
  return len;
  }

char *xp_HUGE_STRCPY (char *to, char XP_HUGE *from)
  {
  char *ret = to;

  while (*from)
    *to++ = *from++;
  *to = 0;

  return ret;
  }
#endif
