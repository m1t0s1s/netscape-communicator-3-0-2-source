/*
 *  ZIG.C
 *
 *  Zignature.
 *  Routines common to signing and validating.
 *
 */

#include "zig.h"

/* from proto.h */
extern MWContext *XP_FindSomeContext(void);

#ifndef AKBAR
/* sigh */
extern MWContext *FE_GetInitContext(void);
#endif /* !AKBAR */

#define zigprint

#ifdef XP_UNIX
extern int errno;
#endif

#include "xp_error.h"
#include "xpgetstr.h"

/*
 *  S O B _ n e w 
 *
 *  Create a new instantiation of a manifest representation.
 *  Use this as a token to any calls to this API.
 *
 */

ZIG *SOB_new (void)
  {
  ZIG *zig;

  zig = XP_CALLOC (1, sizeof (ZIG));

  if (zig)
    {
    /* list */
    zig->list = XP_CALLOC (1, sizeof (ZSList));

    if (zig->list == NULL)
      return NULL;

    ZS_InitList (zig->list);


    /* cer */
    zig->cer  = XP_CALLOC (1, sizeof (ZSList));

    if (zig->cer == NULL)
      return NULL;

    ZS_InitList (zig->cer);


    /* manifest */
    zig->manifest = XP_CALLOC (1, sizeof (ZSList));

    if (zig->manifest == NULL)
      return NULL;

    ZS_InitList (zig->manifest);
    }
  else
    return NULL;

  return zig;
  }

/* 
 *  S O B _ d e s t r o y
 *
 *  Godzilla.
 *
 */

int PR_CALLBACK SOB_destroy (ZIG **zig)
  {
  SOBITEM *it;

  PHYZIG *phy;
  FINGERZIG *fing;
  METAINFO *met;

  ZSLink *link;
  ZSList *list, *cer;
  ZIG *z;

  if (zig == NULL) return 0;
  z = *zig;

  if (z == NULL) return 0;
  list = z->list;
  cer = z->cer;

  SOB_set_nailed (z, 0);

  if (z->url)      XP_FREE (z->url);
  if (z->filename) XP_FREE (z->filename);
  if (z->pattern)  XP_FREE (z->pattern);
  if (z->owner)    XP_FREE (z->owner);
  if (z->digest)   XP_FREE (z->digest);

  /* Free the linked list elements */

  if (list && !ZS_ListEmpty (list))
  {
      for (link = ZS_ListHead (list);
	   !ZS_ListIterDone (list, link);
	   link = link->next)
      {
	  it = (SOBITEM *) link->thing;
	  if (!it) continue;

	  if (it->pathname) XP_FREE (it->pathname);
 
	  switch (it->type)
          {
          case ZIG_INFO:
          case ZIG_MF:
          case ZIG_SF:
          case ZIG_SIG:
          case ZIG_ARCH:

            break;

          case ZIG_META:

            met = (METAINFO *) it->data;
            if (met)
              {
              if (met->header) XP_FREE (met->header);
              if (met->info) XP_FREE (met->info);
              }
            break;

	  case ZIG_PHY:

            phy = (PHYZIG *) it->data;
            if (phy && phy->path) XP_FREE (phy->path);
            break;

          case ZIG_FP:
          case ZIG_FP2:
          case ZIG_FP3:
            fing = (FINGERZIG *) it->data;
            if (fing)
              {
              if (fing->cert)
                CERT_DestroyCertificate (fing->cert);
              if (fing->key)
                XP_FREE (fing->key);
              }

	    break;
          }

	  XP_FREE (it);
      }
  }

  return 0;
  }

/*
 *  SOB_set_nailed
 *
 *  Open the zig archive file, or close it.
 *  For efficiency it is desirable to keep the file
 *  descriptor open between extractions. 
 *
 */

int SOB_set_nailed (ZIG *zig, int setting)
  {
  if (setting && zig->filename)
    {
    if ((zig->fp = XP_FileOpen (zig->filename, xpURL, "rb")) == NULL)
      {
      zigprint ("zig: error %d opening %s\n", errno, zig->filename);
      return -1;
      }
    }
  else if (zig->nailed)
    {
    if (zig->fp)
      XP_FileClose (zig->fp);
    }

  zig->nailed = setting;

  return 0;
  }

/*
 *  S O B _ g e t _ m e t a i n f o
 *
 *  Retrieve meta information from the manifest file.
 *  It doesn't matter whether it's from .MF or .SF, does it?
 *
 */

int SOB_get_metainfo
    (ZIG *zig, char *name, char *header, void **info, unsigned long *length)
  {
  SOBITEM *it;

  ZSLink *link;
  ZSList *list;

  METAINFO *met;

  if (zig == 0 || header == 0)
    return -1;

  list = (ZSList *) zig->list;

  if (ZS_ListEmpty (list))
    {
    zigprint ("empty list\n");
    return -1;
    }

  for (link = ZS_ListHead (list); 
       !ZS_ListIterDone (list, link); 
       link = link->next)
    {
    it = (SOBITEM *) link->thing;
    if (it->type == ZIG_META)
      {
      if ((name && !it->pathname) || (!name && it->pathname))
        continue;

      if (name && it->pathname && strcmp (it->pathname, name))
        continue;

      met = (METAINFO *) it->data;

      if (!XP_STRCASECMP (met->header, header))
        {
        *info = XP_STRDUP ((char *) met->info);
        *length = met->length;
        return 0;
        }
      }
    }

  return -1;
  }

/*
 *  S O B _ f i n d
 *
 *  Establish the search pattern for use
 *  by SOB_find_next, to traverse the filenames
 *  or certificates in the ZIG structure.
 *
 *  See zig.h for a description on how to use.
 *
 */

int SOB_find (ZIG *zig, char *pattern, int type)
  {
  if (!zig)
    return -1;

  if (zig->pattern)
    {
    XP_FREE (zig->pattern);
    zig->pattern = NULL;
    }

  if (pattern)
    zig->pattern = XP_STRDUP (pattern);

  zig->finding = type;

  switch (type)
    {
    case ZIG_FP2:
    case ZIG_FP3:   zig->next = ZS_ListHead (zig->cer);
                    break;

    case ZIG_SECT:  zig->next = ZS_ListHead (zig->manifest);
                    break;

    default:        zig->next = ZS_ListHead (zig->list);
                    break;
    }

  return 0;
  }

/*
 *  S O B _ f i n d _ n e x t
 *
 *  Return the next item of the given type
 *  from one of the ZIG linked lists.
 *
 */

int SOB_find_next (ZIG *zig, SOBITEM **it)
  {
  ZSList *list;

  switch (zig->finding)
    {
    case ZIG_FP2:
    case ZIG_FP3:   list = zig->cer;
                    break;

    case ZIG_SECT:  list = zig->manifest;
                    break;

    default:        list = zig->list;
                    break;
    }

  while (!ZS_ListIterDone (list, zig->next))
    {
    *it = (SOBITEM *) zig->next->thing;
    zig->next = zig->next->next;

    if (*it && (*it)->type == zig->finding)
      return 0;
    }

  if (zig->pattern)
    {
    XP_FREE (zig->pattern);
    zig->pattern = NULL;
    }

  *it = NULL;
  return -1;
  }

/*
 *  S O B _ g e t _ f i l e n a m e
 *
 *  Returns the filename associated with
 *  a ZIG structure.
 *
 */

char *SOB_get_filename (ZIG *zig)
  {
  return zig->filename;
  }

/*
 *  S O B _ g e t _ u r l
 *
 *  Returns the URL associated with
 *  a ZIG structure. Nobody really uses this now.
 *
 */

char *SOB_get_url (ZIG *zig)
  {
  return zig->url;
  }

/*
 *  S O B _ s e t _ c a l l b a c k
 *
 *  Register some manner of callback function for this zig. 
 *
 */

int SOB_set_callback (int type, ZIG *zig, 
      int (*fn) (int status, ZIG *zig, 
         const char *metafile, char *pathname, char *errortext))
  {
  if (type == ZIG_CB_SIGNAL)
    {
    zig->signal = fn;
    return 0;
    }
  else
    return -1;
  }

/*
 *  S O B _ g e t _ e r r o r
 *
 *  This is provided to map internal SOB errors to strings for
 *  the Java console. Also, a DLL may call this function if it does
 *  not have access to the XP_GetString function.
 *
 *  These strings aren't UI, since they are Java console only.
 *  Please do not scream about a UI change.
 *
 */

char *SOB_get_error (status)
  { 
  char *errstring = NULL;

  switch (status)
    {
    case ZIG_ERR_GENERAL:
      errstring = "General JAR file error";
      break;

    case ZIG_ERR_FNF:
      errstring = "JAR file not found";
      break;

    case ZIG_ERR_CORRUPT:
      errstring = "Corrupt JAR file";
      break;

    case ZIG_ERR_MEMORY:
      errstring = "Out of memory";
      break;

    case ZIG_ERR_DISK:
      errstring = "Disk error (perhaps out of space)";
      break;

    case ZIG_ERR_ORDER:
      errstring = "Inconsistent files in META-INF directory";
      break;

    case ZIG_ERR_SIG:
      errstring = "Invalid digital signature file";
      break;

    case ZIG_ERR_METADATA:
      errstring = "JAR metadata failed verification";
      break;

    case ZIG_ERR_ENTRY:
      errstring = "No Manifest entry for this JAR entry";
      break;

    case ZIG_ERR_HASH:
      errstring = "Invalid Hash of this JAR entry";
      break;

    case ZIG_ERR_PK7:
      errstring = "Strange PKCS7 or RSA failure";
      break;

    case ZIG_ERR_PNF:
      errstring = "Path not found inside JAR file";
      break;

    default:
      errstring = XP_GetString (status);
      break;
    }

  return errstring;
  }

/*
 *  Jartool Functions
 *
 */

int SOB_JAR_validate_archive (char *filename)
  {
  ZIG *zig;
  int status = -1;

  zig = SOB_new();

  if (zig)
    {
    status = SOB_pass_archive (ZIG_F_GUESS, filename, "", zig);

    if (status == 0)
      status = zig->valid;

    SOB_destroy (&zig);
    }

  return status;
  }

char *SOB_JAR_get_error (int status)
  {
  return SOB_get_error (status);
  }

/*
 *  S O B _ s e t _ c o n t e x t
 *
 *  Set the zig window context for use by PKCS11, since
 *  it may be needed to prompt the user for a password.
 *
 */

int SOB_set_context (ZIG *zig, MWContext *mw)
  {
  if (mw)
    {
    zig->mw = mw;
    zig->mwset = 1;
    }
  else
    {
    /* zig->mw = XP_FindSomeContext(); */
    zig->mw = NULL;

    /*
     * We can't find a context because we're in startup state and none
     * exist yet. go get an FE_InitContext that only works at initialization
     * time.
     */
#ifndef AKBAR
    /* Turn on the mac when we get the FE_ function */
    if (zig->mw == NULL)
    {
    zig->mw = FE_GetInitContext();
    }
#endif /* !AKBAR */
   }

  return 0;
  }

