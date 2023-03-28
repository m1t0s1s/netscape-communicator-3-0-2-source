/* -*- Mode: C; tab-width: 4; -*- */
/*
 *  npassoc.h $Revision: 1.3 $
 *  xp filetype associations
 */

#ifndef _NPASSOC_H
#define _NPASSOC_H

#include "xp_core.h"

typedef struct _NPFileTypeAssoc {
    char*	type;				/* a MIME type */
    char*	description;		/* Intelligible description */
    char**  extentlist;			/* a NULL-terminated list of file extensions */
    char*	extentstring;		/* the same extensions, as a single string */
    void*   fileType;			/* platform-specific file selector magic  */
    struct _NPFileTypeAssoc* pNext;
} NPFileTypeAssoc;


XP_BEGIN_PROTOS

extern NPFileTypeAssoc*	NPL_NewFileAssociation(const char *type, const char *extensions,
								const char *description, void *fileType);
extern void*			NPL_DeleteFileAssociation(NPFileTypeAssoc *fassoc);
extern void				NPL_RegisterFileAssociation(NPFileTypeAssoc *fassoc);
extern NPFileTypeAssoc*	NPL_RemoveFileAssociation(NPFileTypeAssoc *fassoc);
extern NPFileTypeAssoc* NPL_GetFileAssociation(const char *type);

XP_END_PROTOS

#endif /* _NPASSOC_H */

