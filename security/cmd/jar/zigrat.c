/* FIX: verify_global */
/*
 *  ZIGRAT
 *
 *  Zignature Ratification.
 *
 *  To verify a tree:
 *
 *     zigrat manifest-file
 *
 *  To verify single files in archives:
 *
 *     zigrat -f archive.zip filename
 *
 *  Copyright 1996 Netscape Corporation
 * 
 */

#include "zig.h"
#include "base64.h"

#include "secutil.h"
#include "secpkcs7.h"

#ifdef _UNIX
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include "errno.h"

/* SVR4 is to prevent Solaris from blowing up --Rob */
#if defined(__sun) && !defined(SVR4)
extern int getopt(int, char**, char*);
extern char *optarg;
#endif

#define zigprint printf

int ssl_sign = 0;
char *cert_dir = NULL;

extern int zig_archive (char *filename, char *member);
extern int zig_list (char *filename);
extern int zigrat (char *dir, ZIG *zig);
extern int ratify (char *dir, ZIG *zig);
extern int zigshove (char *filename, ZIG *zig);
static int verify_global (ZIG *zig);
extern unsigned char *read_file (char *filename);
extern int usage (void);
static int init_crypto (void);
static int zig_who (char *filename);

/* Copies to prevent linking a whole new navigator */

XP_File XP_FileOpen 
    (const char* name, XP_FileType type, const XP_FilePerm permissions)
  {
  return fopen (name, permissions);
  }

#ifndef XP_FileClose
XP_FileClose (XP_File fp)
  {
  return fclose (fp);
  }
#endif

#ifndef XP_FileWrite
int XP_FileWrite (const void *source, int32 count, XP_File file)
  {
  return fwrite (source, 1, 
    count == -1 ? strlen((char*)source) : count, file);
  }
#endif

#if 0
int XP_FileRead
    (const void *dest, int32 count, XP_File file)
  {
  return fread (dest, 1, count, file);
  }

int XP_FileSeek (XP_File file, int32 offset, int32 whence)
  {
  return fseek (file, offset, whence);
  }
#endif

/* void FE_SetPasswordEnabled(MWContext *context, PRBool usePW) */
void FE_SetPasswordEnabled()
  {
  }

int tellwho = 0;
int check_crypto = 1;

int main (int argc, char *argv[])
  {
  int o;
  static char db [256];

  ZIG *zig;
  int zipfile = 0;

  char *jarfile = NULL;
  char *extract_file = NULL;

  if (argc < 2)
    {
    usage();
    exit (1);
    }

  zig = SOB_new();

  while ((o = getopt (argc, argv, "cd:f:sv:w:")) != -1) 
    {
    switch (o) 
      {
      case '?': usage(); exit (0);
	        break;

      case 'c': check_crypto = 0;
                /* not implemented */
                break;

      case 'd': cert_dir = XP_STRDUP (optarg);
                break;

      case 'f': extract_file = XP_STRDUP (optarg);
                zigprint ("extracting file: \"%s\"\n", extract_file);
                break;

      case 's': ssl_sign = 1;
                zigprint ("checking using SSL client auth certificates\n");
                break;

      case 'v': jarfile = XP_STRDUP (optarg);
                zigprint ("validating archive: \"%s\"\n", jarfile);
                break;

      case 'w': tellwho++;
                jarfile = XP_STRDUP (optarg);
                break;
      }
    }

  if (!cert_dir)
    {
    char *home;

    home = getenv ("HOME");

    if (home && *home)
      {
      sprintf (db, "%s/.netscape", home);
      cert_dir = db;
      }
    else
      {
      zigprint ("You must specify the location of your certificate directory\n");
      zigprint ("with the -d option. Example: -d ~/.netscape in many cases with Unix.\n");
      exit (0);
      }
    }

  zigprint ("using certificate directory: %s\n", cert_dir);

  if (extract_file)
    {
    zig_archive (argv [optind], extract_file);
    }
  else if (tellwho)
    {
    zig_who (jarfile);
    }
  else if (jarfile)
    {
    zig_list (jarfile);
    }
  else
    {
    zigrat (argv [optind], zig);
    ratify (argv [optind], zig);
    }

  exit (0);
  }

int usage (void)
  {
  printf ("\nzigrat\n\n");
  printf ("   zigrat -f extract_file jar_archive\n");
  printf ("       (extracts a file from a jar archive)\n\n");
  printf ("   zigrat -v jar_archive\n");
  printf ("       (lists and validates the contents of a jar archive\n\n");
  printf ("   zigrat directory_tree\n");
  printf ("       (validates a jar directory tree splayed on disk, no jar file)\n\n");
  }

int zig_archive (char *filename, char *member)
  {
  ZIG *zig;
  int res = 0;

  zig = SOB_new();

  SOB_pass_archive (ZIG_F_GUESS, filename, "some-url", zig);

  if (member)
    {
    zigprint ("attempting to extract & verify member: %s\n", member);
    res = SOB_verified_extract (zig, member, "output");
    }

  if (res < 0)
    zigprint ("extraction failed, code %d\n", res);

  return res;
  }

static int zig_who (char *filename)
  {
  ZIG *zig;
  int status;

  SOBITEM *it;
  FINGERZIG *fing;

  CERTCertificate *cert;

  init_crypto();

  zig = SOB_new();

  status = SOB_pass_archive (ZIG_F_GUESS, filename, "some-url", zig);

  if (status < 0 || zig->valid < 0)
    {
    zigprint ("NOTE -- \"%s\" archive DID NOT PASS crypto verification.\n", filename);
    if (zig->valid < 0 && status != -1)
      zigprint ("  (purported reason: %s (%d) -- may be incorrect)\n", SECU_ErrorString (PORT_GetError()), status);
    }

  SOB_find (zig, "*", ZIG_FP2);

  while (SOB_find_next (zig, &it) >= 0)
    {
    fing = (FINGERZIG *) it->data;
    cert = (CERTCertificate *) fing->stuff;
    if (cert)
      {
      if (cert->nickname) 
        printf ("nickname: %s\n", cert->nickname);
      if (cert->subjectName)
        printf ("subject name: %s\n", cert->subjectName);
      if (cert->issuerName)
        printf ("issuer name: %s\n", cert->issuerName);
      }
    else
      printf ("no cert could be found\n");
    }

  SOB_destroy (&zig);
  }

int zig_list (char *filename)
  {
  int ret;
  int status;

  ZIG *zig;
  SOBITEM *it;

  init_crypto();

  zig = SOB_new();

  status = SOB_pass_archive (ZIG_F_GUESS, filename, "some-url", zig);

  if (status < 0 || zig->valid < 0)
    {
    zigprint ("NOTE -- \"%s\" archive DID NOT PASS crypto verification.\n", filename);
    if (zig->valid < 0 && status != -1)
      zigprint ("  (purported reason: %s -- may be incorrect)\n", SECU_ErrorString (PORT_GetError()));
    zigprint ("entries shown below will have their digests checked only.\n"); 
    zig->valid = 0;
    }
  else
    zigprint ("archive \"%s\" has passed crypto verification.\n", filename);

  if (SOB_set_nailed (zig, 1) < 0)
    return -1;

  verify_global (zig);

  zigprint ("\n");
  zigprint ("%16s   %s\n", "status", "path");
  zigprint ("%16s   %s\n", "------------", "-------------------");

  SOB_find (zig, "*", ZIG_MF);

  while (SOB_find_next (zig, &it) >= 0)
    {
    if (it->pathname)
      {
      ret = SOB_verified_extract (zig, it->pathname, "output");
      zigprint ("%16s   %s\n", 
        ret >= 0 ? "verified" : "NOT VERIFIED", it->pathname);
      }
    }

  if (status < 0 || zig->valid < 0)
    zigprint ("\nNOTE -- \"%s\" archive DID NOT PASS crypto verification.\n", filename);

  SOB_destroy (&zig);

  return 0;
  }

int zigrat (char *dir, ZIG *zig)
  {
  char fullpath [256];

  strcpy (fullpath, dir);
  strcat (fullpath, "/META-INF/manifest.mf");

  zigshove (fullpath, zig);

  /* FIX - have this check for *.sf */

  strcpy (fullpath, dir);
  strcat (fullpath, "/META-INF/zigbert.sf");

  zigshove (fullpath, zig);

  return 0;
  }

/*
 *  r a t i f y
 *
 *  Display ascii representation of the internal
 *  data structures for the SOB routines.
 *
 */

int ratify (char *dir, ZIG *zig)
  {
  int i = 0;

  ZSList *list;

  SOBITEM *it;
 
  DIGESTS dig;
  char fullpath [BUFSIZ];

  int result;

  list = zig->list;

  if (ZS_ListEmpty (list))
    {
    zigprint ("empty list\n");
    return -1;
    }

  SOB_find (zig, "*", ZIG_SF);

  while (SOB_find_next (zig, &it) >= 0)
    {
    if (it->pathname)
      {
      /* FIX - overflow? */

      strcpy (fullpath, dir);
      strcat (fullpath, "/");
      strcat (fullpath, it->pathname);

      memset (&dig, 0, sizeof (DIGESTS));
      SOB_digest_file (fullpath, &dig);

      SOB_verify_digest (&result, zig, it->pathname, &dig);

      printf ("%2d      %-16.16s  %d    %s\n", 
          ++i, it->pathname, it->type, result ? "verified" : "FAILED");
      }
    }

  return 0;
  }

/* 
 *  z i g s h o v e
 *
 *  Read a manifest file into memory and
 *  hand it off to the parser. 
 *
 */

int zigshove (char *filename, ZIG *zig)
  {
  unsigned char *dat;

  dat = read_file (filename);

  if (dat)
    {
    if (SOB_parse_manifest (dat, strlen (dat), filename, "url", zig) < 0)
      return -1;
    XP_FREE (dat);
    }

  return 0;
  }

/*
 *  r e a d _ f i l e
 *
 *  Read a file into memory,
 *  in one fell swoop.
 *  FIX: Need to return length.
 *
 */

unsigned char *read_file (char *filename)
  {
  FILE *fp;

  char *filebuf;
  long size, num;

  if ((fp = fopen (filename, "rb")) == NULL)
    {
    perror (filename);
    return NULL;
    }

  /* Determine the size of this file */

  if (fseek (fp, 0L, SEEK_END) != 0)
    perror (filename);

  size = ftell (fp);
  rewind (fp);

  /* Read the whole damn thing */

  filebuf = (unsigned char *) XP_CALLOC (1, size + 1);
  if (filebuf == NULL)
    {
    fprintf (stderr, "out of memory allocating space for: %s\n", filename);
    exit (1);
    }

  memset (filebuf, 0, size + 1);
 
  if ((num = fread (filebuf, 1, size, fp)) != size) 
    {
    fprintf (stderr, "error reading from file: %s\n", filename);
    perror (filename);
    exit (1);
    }

  fclose (fp);

  return filebuf;
  }

static int verify_global (ZIG *zig)
  {
  FILE *fp;

  char *ext;

  SOBITEM *it;
  DIGESTS dig, *globaldig;

  char buf [BUFSIZ];

  unsigned char *md5_digest, *sha1_digest;

  SOB_find (zig, "*", ZIG_PHY);

  while (SOB_find_next (zig, &it) >= 0)
    {
    if (!XP_STRNCMP (it->pathname, "META-INF", 8))
      {
      for (ext = it->pathname; *ext; ext++);
      while (ext > it->pathname && *ext != '.') ext--;

      if (!XP_STRCASECMP (ext, ".rsa"))
        zigprint ("found a RSA signature file: %s\n", it->pathname);

      if (!XP_STRCASECMP (ext, ".dsa"))
        zigprint ("found a DSA signature file: %s\n", it->pathname);

      if (!XP_STRCASECMP (ext, ".mf"))
        zigprint ("found a MF master manifest file: %s\n", it->pathname);

      if (!XP_STRCASECMP (ext, ".sf"))
        {
        zigprint ("found a SF signature manifest file: %s\n", it->pathname);

        if (SOB_extract (zig, it->pathname, "output") < 0)
          {
          zigprint ("error extracting %s\n", it->pathname);
          continue;
          }

        md5_digest = NULL;
        sha1_digest = NULL;

        if ((fp = fopen ("output", "rb")) != NULL)
          {
          while (fgets (buf, BUFSIZ, fp))
            {
            char *s;

            if (*buf == 0 || *buf == '\n' || *buf == '\r')
              break;

            for (s = buf; *s && *s != '\n' && *s != '\r'; s++);
            *s = 0;

            if (!XP_STRNCMP (buf, "MD5-Digest: ", 12))
              {
              md5_digest = ATOB_AsciiToData (buf + 12, &dig.md5_length);
              }

            if (!XP_STRNCMP (buf, "SHA1-Digest: ", 13))
              {
              sha1_digest = ATOB_AsciiToData (buf + 13, &dig.sha1_length);
              }

            if (!XP_STRNCMP (buf, "SHA-Digest: ", 12))
              {
              sha1_digest = ATOB_AsciiToData (buf + 12, &dig.sha1_length);
              }
            }

          globaldig = zig->globalmeta;

          if (globaldig && md5_digest)
             {
             zigprint ("  md5 digest on global metainfo: %s\n", 
                 XP_MEMCMP (md5_digest, globaldig->md5, MD5_LENGTH) ? "no match" : "match");
             }

          if (globaldig && sha1_digest)
             {
             zigprint ("  sha digest on global metainfo: %s\n", 
                 XP_MEMCMP (sha1_digest, globaldig->sha1, SHA1_LENGTH) ? "no match" : "match");
             }

          if (globaldig == NULL)
             zigprint ("global metadigest is not available, strange.\n");

          fclose (fp);
          }
        }
      }
    }
  }

/* VERIFYING PORTION */

extern void SEC_Init(void); /* XXX */

static SECKEYKeyDBHandle *OpenKeyDB (char *progName)
  {
  int ret;
  char *fName;

  struct stat stat_buf;
  SECKEYKeyDBHandle *keyHandle;

  fName = SECU_DatabaseFileName(xpKeyDB);

  printf ("using key database: %s\n", fName);

  ret = stat (fName, &stat_buf);
  if (ret < 0) 
    {
    if (errno == ENOENT) 
      {
      /* no key.db */
      SECU_PrintError (progName, "unable to locate key database");
      exit (1);
      }
    else 
      {
      /* stat error */
      SECU_PrintError (progName, "stat error: ", strerror(errno));
      exit (1);
      }
    }

   keyHandle = SECKEY_OpenKeyDBFilename (fName, PR_FALSE);
   if (keyHandle == NULL) 
     {
     SECU_PrintError (progName, "could not open key database %s", fName);
     exit (1);
     }

  return (keyHandle);
  }

static char *
certDBNameCallback(void *arg, int dbVersion)
{
    char *fnarg;
    char *dir;
    char *filename;
    
    dir = SECU_ConfigDirectory (cert_dir);
 
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

static char *progName = "zigrat";

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

    rv = CERT_OpenCertDB(certHandle, FALSE, certDBNameCallback, NULL);

    if (rv) {
	SECU_PrintError(progName, "could not open certificate database");
	if (certHandle) free (certHandle);  /* we don't want to leave anything behind... */
	return NULL;
    } else {
	CERT_SetDefaultCertDB(certHandle);
    }

    return certHandle;
}

static void PrintBytes
     (void *arg, const char *buf, unsigned long len)
  {
  fwrite (buf, len, 1, (FILE *) arg);
  }

int DecodeAndPrintFile (FILE *out, FILE *in, char *progName)
  {
  SECItem derdata;
  SEC_PKCS7ContentInfo *cinfo;
  SEC_PKCS7DecoderContext *dcx;

  SECCertUsage signwith;

  /* Right now use the SSL certificates (a hack until 
     object signing is available */

  signwith = ssl_sign ? certUsageSSLClient : certUsageObjectSigner;

  if (SECU_DER_Read(&derdata, in)) 
    {
    fprintf (stderr, "%s: error converting der (%s)\n", 
       progName, SECU_ErrorString(PORT_GetError()));
    return -1;
    }

  fprintf (out,
    "Content printed between bars (newline added before second bar):");

  fprintf (out, 
    "\n---------------------------------------------\n");

  dcx = SEC_PKCS7DecoderStart (PrintBytes, out, SECU_GetPassword, NULL);
  if (dcx != NULL) 
    {
    SEC_PKCS7DecoderUpdate(dcx, derdata.data, derdata.len);
    cinfo = SEC_PKCS7DecoderFinish (dcx);
    }

  fprintf(out, "\n---------------------------------------------\n");

  if (cinfo == NULL)
    return -1;

  fprintf (out, "Content %s encrypted.\n",
	    SEC_PKCS7ContentIsEncrypted (cinfo) ? "was" : "was not");

  if (SEC_PKCS7ContentIsSigned(cinfo)) 
    {
    PORT_SetError (0);

    if (SEC_PKCS7VerifySignature (cinfo, signwith, PR_FALSE))
      fprintf (out, "Signature is valid.\n");
    else
      fprintf (out, "Signature is invalid (Reason: %s).\n",
         SECU_ErrorString (PORT_GetError()));
    }
  else 
    fprintf (out, "Content was not signed.\n");

  SEC_PKCS7DestroyContentInfo(cinfo);
  return 0;
  }


/*
 * Print the contents of a PKCS7 message, indicating signatures, etc.
 */

int do_decode (FILE *inFile, FILE *outFile)
  {
  char *progName = "zigrat";
  int opt;
  SECKEYKeyDBHandle *keyHandle;
  CERTCertDBHandle *certHandle;

  /* Call the libsec initialization routines */
  PR_Init ("p7content", 1, 1, 0);
  SEC_Init();

  /* open key database */
  keyHandle = OpenKeyDB (progName);

  if (keyHandle == NULL) 
    return -1;

  SECKEY_SetDefaultKeyDB (keyHandle);

  /* open cert database */
  certHandle = OpenCertDB();

  if (certHandle == NULL) 
    return -1;

  CERT_SetDefaultCertDB (certHandle);

  if (DecodeAndPrintFile (outFile, inFile, progName)) 
    {
    fprintf (stderr, "%s: problem decoding data (%s)\n",
       progName, SECU_ErrorString (PORT_GetError()));
    return -1;
    }

  return 0;
  }

static int init_crypto (void)
  {
  SECStatus rv;

  CERTCertDBHandle *certHandle;

  /* Call the libsec initialization routines */
  PR_Init ("verify", 1, 1, 0);

  /* ugly */
  SECU_ConfigDirectory (cert_dir);

  rv = SECU_PKCS11Init();
  if (rv != SECSuccess)
      return -1;

  SEC_Init();

  /* open cert database */
  certHandle = OpenCertDB(); 

  if (certHandle == NULL) 
    return -1;

  CERT_SetDefaultCertDB (certHandle);

  return 0;
  }
