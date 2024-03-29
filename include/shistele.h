/* -*- Mode: C; tab-width: 4 -*- */

#ifndef SHISTELE_H
#define SHISTELE_H

#include "xp_list.h"

/*
** This structure holds pointer so all saved data in the history. It is
** also used by URL_Structs and is ultimately fed to layout.
**
** Warning: Don't change the names of the fields because they're used by
** macros.
*/
typedef struct SHIST_SavedData {
	void*		FormList;		/* layout data to recreate forms */
	void*		EmbedList;		/* layout data to recreate embeds and applets */
	void*		Grid;			/* layout data to recreate grids */
#ifdef MOCHA
    void*		Window;			/* window object for grid being resized */

	/* XXX none of this would be necessary if frameset docs were reparsed */
	void*		OnLoad;			/* JavaScript onload event handler source */
	void*		OnUnload;		/* JavaScript onunload event handler source */
	void*		OnFocus;		/* JavaScript onfocus event handler source */
	void*		OnBlur;			/* JavaScript onblur event handler source */
#endif
} SHIST_SavedData;

/*
    This structure encapsulates all of the information needed for the
    session history.  It should contain stuff like a list of all of the
    documents in the current history and a pointer to where the currently
    viewed document sits in the chain.

	WARNING!!  Some slots of this structure are shared with URL_Struct and
	net_CacheObject.  If you add a slot, decide whether it needs to be
	shared as well.
*/

struct _History_entry {
    char * title;                   /* title for this entry */
    char * address;                 /* URL address string */
    char * content_name;            /* Server-provided "real name", used for
									   default file name when saving. */
    int    method;                  /* method of acessing URL */
    char * referer;					/* What document points to this url */
    char * post_data;               /* post data */
    int32  post_data_size;          /* post data size */
    Bool   post_data_is_file;       /* is the post data a filename? */
    char * post_headers;            /* content type for posted data */
    int32  position_tag;            /* layout specific data for determining
                                     * where in the document the user was
                                     */
	time_t last_modified;           /* time of last modification */
	time_t last_access;             /* time of last access */
	int    history_num;	 		    /* special hack to add navigation */

	SHIST_SavedData savedData;		/* layout data */

	PRPackedBool
		   is_binary,               /* is this a binary object pretending
									 * to be HTML? 
									 */
		   is_active,               /* is it an active stream? */
		   is_netsite,              /* did it come from netsite? */
		   replace;                 /* did it come from netsite? */

	int    transport_method;        /* network, disk cache, memory cache */

	uint32  refresh;                /* refresh interval */
	char   *refresh_url;			/* URL to refresh */
	char   *wysiwyg_url;			/* URL for WYSIWYG printing/saving */

	/* Security information */
    int     security_on;				  /* is security on? */
    char   *key_cipher;					  /* cipher being used */
	int     key_size;					  /* size of the total key (in bits) */
	int     key_secret_size;			  /* secret portion (in bits) */
    char   *key_issuer;					  /* XXX obsolete */
    char   *key_subject;  				  /* XXX obsolete */

	/* Unique identifier */
	int32	unique_id;
};

#define SHIST_CAME_FROM_NETWORK       0
#define SHIST_CAME_FROM_DISK_CACHE    1
#define SHIST_CAME_FROM_MEMORY_CACHE  2

struct History_ {
    XP_List  * list_ptr;		/* pointer to linked list */
	int         cur_doc;		/* an index into the list that points to the current document */
	History_entry *cur_doc_ptr;  /* a ptr to the current doc entry */
	int32       num_entries;    /* size of the history list */
	int32		max_entries;	/* maximum size of list; -1 == no limit */
};


#endif /* SHISTELE_H */
