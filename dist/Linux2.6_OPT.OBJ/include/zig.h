#ifndef __ZIG_h_
#define __ZIG_h_

/*
 *  ZIG.H
 *
 *  Support code for Zignature signed
 *  object specification.
 *
 *  tdell x3565 - 10/29/96
 *
 */

/*
 *  In general, any functions that return pointers
 *  have memory owned by the caller.
 *
 */

/*
 *  API CHANGES:
 *
 *    parse_manifest new parameter for length. path.
 *    some void*'s changed to DIGEST*'s.
 *    new find and find_next functions for iteration.
 *    functions to get filename and URL from a zig.
 *    new set_nailed to keep fp open for multiple extracts.
 *
 *  TO DO (in this file only):
 *
 *    Support for multiple signatures (a bug)
 *    New function SOB_verified_extract equivalent
 *      except extracting to memory (future feature)
 *    Make parameter names more relevant and standard
 *    URL shouldn't be in SOBITEM structure.
 *    Make comments in this file represent reality.
 *    Some functions should be private.
 *
 */

#include "xp_core.h"
#include "ntypes.h"
#include "sechash.h"
#include "cert.h"

#include "zig-ds.h"

/* #define ZIGBERT */

#ifdef ZIGBERT

/* These are hacky little things to help the cmd tool */

extern XP_File ZIG_XP_FileOpen
    (const char* name, XP_FileType type, const XP_FilePerm permissions);

extern ZIG_XP_FileClose (XP_File fp);

extern int ZIG_XP_FileWrite
    (const void* source, int32 count, XP_File file);

#define XP_FileOpen ZIG_XP_FileOpen
#define XP_FileClose ZIG_XP_FileClose
#define XP_FileWrite ZIG_XP_FileWrite

#endif 

/* pass "token" as a pointer to a ZIG. The ZIG
   contains parsed manifest information */

typedef struct ZIG_
  {
  int format;		/* ZIG_F_something */
  char *url;		/* Where it came from */
  char *filename;	/* Disk location */
  int nailed;           /* For multiple extractions */
  XP_File fp;		/* For multiple extractions */
  ZSList *list;		/* ZIG linked list */
  ZSList *cer;          /* Signing information */
  ZSList *manifest;     /* Digest summary */
  void *globalmeta;     /* digest of .MF global portion */

  /* Context for find, find_next */

  char *pattern;        /* Regular expression */
  int finding;          /* Type of item to find */
  ZSLink *next;         /* Next item in find */

  /* Below will change to a linked list to support multiple sigs */

  int pkcs7;            /* Enforced opaqueness */
  int valid;            /* PKCS7 signature validated */
  char *owner;          /* name of .RSA file */
  int digest_alg;       /* of .SF file */
  int digest_length;    /* of .SF file */
  void *digest;         /* of .SF file */

  /* Window context, very necessary for PKCS11 now */

  int mwset;            /* whether a context is set */
  MWContext *mw;        /* window context */

  /* Signal callback function */

  int (*signal) (int status, struct ZIG_ *zig, 
     const char *metafile, char *pathname, char *errorstring);
  }
ZIG;

/* void data in ZSList's contain SOBITEM type */

typedef struct SOBITEM_
  {
  char *pathname;        /* relative. inside zip file */
  int type;              /* cert or sig */
  unsigned long size;    /* size of data below */
  void *data;            /* totally opaque */
  }
SOBITEM;

/* The length of this structure is unspecified, since
   additional digests may be added at any given time.. */

/* This is the length of a hash that has failed */
#define ZIG_BAD_HASH 1

typedef struct DIGESTS_
  {
  unsigned int  md5_length;
  unsigned char md5 [16];
  unsigned int  sha1_length;
  unsigned char sha1 [20];
  }
DIGESTS;

/* Meta informaton, or "policy", from the manifest file.
   Right now just one tuple per SOBITEM. */

typedef struct METAINFO_
  {
  char *header;
  unsigned int length;
  unsigned char *info;
  }
METAINFO;

/* This should not be global */

typedef struct PHYZIG_
  {
  char *path;
  unsigned char compression;
  unsigned long offset;
  unsigned long length;
  }
PHYZIG;

typedef struct FINGERZIG_
  {
  unsigned char fingerprint [16];
  int length;
  void *key;
  CERTCertificate *cert;
  }
FINGERZIG;

/* for "type" in SOBITEM structure */

#define ZIG_INFO  1      /* from manifest header */
#define ZIG_MF    2      /* manifest hash entry */
#define ZIG_SF    3      /* signature hash entry */
#define ZIG_CERT  4      /* certificate */
#define ZIG_SIG   5      /* digital signature */
#define ZIG_META  6      /* meta info */
#define ZIG_PHY   7      /* physical zip offsets */
#define ZIG_ARCH  8      /* obsolete */
#define ZIG_FP    9      /* cert fingerprint */
#define ZIG_FP2  10      /* signer fingerprint */
#define ZIG_SIGN 10      /* same as FP2 */
#define ZIG_SECT 11      /* sectional mf digest */
#define ZIG_FP3  12      /* cert chain */
#define ZIG_WALK 12      /* same as FP3 */

/* archive formats */

#define ZIG_F_GUESS  0   /* do the right thing */
#define ZIG_F_NONE   1   /* weird */
#define ZIG_F_ZIP    2   /* normal default, ZIP */
#define ZIG_F_TAR    3   /* tar archive, we will support */

/* certificate stuff */

#define ZIG_C_COMPANY   1
#define ZIG_C_CA        2
#define ZIG_C_SERIAL    3
#define ZIG_C_EXPIRES   4
#define ZIG_C_NICKNAME  5
#define ZIG_C_FP        6
#define ZIG_C_JAVA      100

/* callback types */

#define ZIG_CB_SIGNAL	1


/* 
 *  This is the base for the ZIG error codes. It will
 *  change when these are incorporated into allxpstr.c,
 *  but right now they won't let me put them there.
 *
 */

#ifndef SEC_ERR_BASE
#define SEC_ERR_BASE		(-0x2000)
#endif
 
#define ZIG_BASE		SEC_ERR_BASE + 300

/* Jar specific error definitions */

#define ZIG_ERR_GENERAL         (ZIG_BASE + 1)
#define ZIG_ERR_FNF		(ZIG_BASE + 2)
#define ZIG_ERR_CORRUPT		(ZIG_BASE + 3)
#define ZIG_ERR_MEMORY		(ZIG_BASE + 4)
#define ZIG_ERR_DISK		(ZIG_BASE + 5)
#define ZIG_ERR_ORDER           (ZIG_BASE + 6)
#define ZIG_ERR_SIG		(ZIG_BASE + 7)
#define ZIG_ERR_METADATA        (ZIG_BASE + 8)
#define ZIG_ERR_ENTRY		(ZIG_BASE + 9)
#define ZIG_ERR_HASH		(ZIG_BASE + 10)
#define ZIG_ERR_PK7		(ZIG_BASE + 11)
#define ZIG_ERR_PNF		(ZIG_BASE + 12)


XP_BEGIN_PROTOS

/*
 *  Birth and death 
 *
 */

extern ZIG *SOB_new (void);

extern int PR_CALLBACK SOB_destroy (ZIG **zig);

extern char *SOB_get_error (int status);

extern int SOB_set_callback (int type, ZIG *zig, 
  int (*fn) (int status, ZIG *zig, 
  const char *metafile, char *pathname, char *errortext));

/*
 *  SOB_set_context
 *
 *  PKCS11 may require a password to be entered by the user
 *  before any crypto routines may be called. This will require
 *  a window context if used from inside Mozilla.
 *
 *  Call this routine with your context before calling 
 *  verifying or signing. If you have no context, call with NULL
 *  and one will be chosen for you.
 *
 */

int SOB_set_context (ZIG *zig, MWContext *mw);

/*
 *  Iterative operations
 *
 *  SOB_find sets up for repeated calls with SOB_find_next.
 *  I never liked findfirst and findnext, this is nicer.
 *
 *  Pattern contains a relative pathname to match inside the
 *  archive. It is currently assumed to be "*".
 *
 *  To use:
 *
 *     SOBITEM *item;
 *     SOB_find (zig, "*.class", ZIG_MF);
 *     while (SOB_find_next (zig, &item) >= 0) 
 *       { do stuff }
 *
 */

extern int SOB_find (ZIG *zig, char *pattern, int type);

extern int SOB_find_next (ZIG *zig, SOBITEM **it);

/*
 *  Function to parse manifest file:
 *
 *  Many signatures may be attached to a single filename located
 *  inside the zip file. We only support one.
 *
 *  Several manifests may be included in the zip file. 
 *
 *  You must pass the MANIFEST.MF file before any .SF files.
 *
 *  Right now this returns a big ole list, privately in the zig structure.
 *  If you need to traverse it, use SOB_find if possible.
 *
 *  The path is needed to determine what type of binary signature is
 *  being passed, though it is technically not needed for manifest files.
 *
 *  When parsing an ASCII file, null terminate the ASCII raw_manifest
 *  prior to sending it, and indicate a length of 0. For binary digital
 *  signatures only, indicate the true length of the signature.
 *  (This is legacy behavior.)
 *
 *  You may free the manifest after parsing it.
 *
 */

extern int SOB_parse_manifest 
    (char XP_HUGE *raw_manifest, long length, 
       const char *path, const char *url, ZIG *zig);

/*
 *  Verify data (nonstreaming). The signature is actually
 *  checked by SOB_parse_manifest or SOB_pass_archive.
 *
 */

extern DIGESTS * PR_CALLBACK SOB_calculate_digest (void XP_HUGE *data, long length);

extern int PR_CALLBACK SOB_verify_digest
    (int *result, ZIG *siglist, const char *name, DIGESTS *dig);

extern int SOB_digest_file (char *filename, DIGESTS *dig);

/*
 *  Get attribute from certificate:
 *
 *  Returns any special signed attribute associated with this cert
 *  or signature (passed in "data"). Attributes ZIG_C_*. Most of the time
 *  this will return a zero terminated string.
 *
 */

extern int PR_CALLBACK SOB_cert_attribute
    (int attrib, ZIG *zig, long keylen, void *key, 
       void **result, unsigned long *length);

/*
 *  Meta information
 *
 *  Currently, since this call does not support passing of an owner
 *  (certificate, or physical name of the .sf file), it is restricted to
 *  returning information located in the manifest.mf file. 
 *
 *  Meta information is a name/value pair inside the archive file. Here,
 *  the name is passed in *header and value returned in **info.
 *
 *  Pass a NULL as the name to retrieve metainfo from the global section.
 *
 *  Data is returned in **info, of size *length. The return value
 *  will indicate if no data was found.
 *
 */

extern int SOB_get_metainfo
    (ZIG *zig, char *name, char *header, void **info, unsigned long *length);

extern char *SOB_get_filename (ZIG *zig);

extern char *SOB_get_url (ZIG *zig);

/*
 *  Return an HTML mockup of a certificate or signature.
 *
 *  Returns a zero terminated ascii string
 *  in raw HTML format.
 *
 */

extern char *SOB_cert_html
    (int style, ZIG *zig, long keylen, void *key, int *result);

/* save the certificate with this fingerprint in persistent
   storage, somewhere, for retrieval in a future session when there 
   is no corresponding ZIG structure. */

extern int PR_CALLBACK SOB_stash_cert
    (ZIG *zig, long keylen, void *key);

/* retrieve a certificate presumably stashed with the above
   function, but may be any certificate. Type is &CERTCertificate */

void *SOB_fetch_cert (long length, void *key);

/*
 *  New functions to handle archives alone
 *    (call SOB_new beforehand)
 *
 *  SOB_pass_archive acts much like parse_manifest. Certificates
 *  are returned in the ZIG structure but as opaque data. When calling 
 *  SOB_verified_extract you still need to decide which of these 
 *  certificates to honor. 
 *
 *  Code to examine a ZIG structure is in zigbert.c. You can obtain both 
 *  a list of filenames and certificates from traversing the linked list.
 *
 */

extern int SOB_pass_archive
    (int format, char *filename, const char *url, ZIG *zig);

/*
 *  Extracts a relative pathname from the archive and places it
 *  in the filename specified. 
 * 
 *  Call SOB_set_nailed if you want to keep the file descriptors
 *  open between multiple calls to SOB_verify_extract.
 *
 */

extern int SOB_verified_extract
    (ZIG *zig, char *path, char *outpath);

extern int SOB_set_nailed (ZIG *zig, int setting);

/*
 *  SOB_extract does no crypto checking. This can be used if you
 *  need to extract a manifest file or signature, etc.
 *
 */

extern int SOB_extract
    (ZIG *zig, char *path, char *outpath);

/*
 *  SOB_JAR_*
 *
 *  Functions for the Jartool, which is written in Java and
 *  requires wrappers located elsewhere in Navigator.
 *
 *  Do not call these unless you are the Jartool, no matter
 *  how convenient they may appear.
 *
 */

/* return a list of certificate nicknames, separated by \n's */
extern char *SOB_JAR_list_certs (void);

/* validate archive, simple api */
extern int SOB_JAR_validate_archive (char *filename);

/* begin hash */
extern void *SOB_JAR_new_hash (int alg);

/* hash a streaming pile */
extern void *SOB_JAR_hash (int alg, void *cookie, int length, void *data);

/* end hash */
extern void *SOB_JAR_end_hash (int alg, void *cookie);

/* sign the archive (given an .SF file) with the given cert.
   The password argument is a relic, PKCS11 now handles that. */

extern int SOB_JAR_sign_archive 
   (char *nickname, char *password, char *sf, char *outsig);

/* convert status to text */
extern char *SOB_JAR_get_error (int status);


XP_END_PROTOS

#endif /* __ZIG_h_ */ 
