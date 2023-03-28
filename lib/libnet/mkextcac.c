/* -*- Mode: C; tab-width: 4 -*- */

/* Please leave outside of ifdef for windows precompiled headers. */
#include "mkutils.h"

#ifdef MOZILLA_CLIENT

#include "mkcache.h"
#include "glhist.h"
#include "xp_hash.h"
#include "xp_mcom.h"
#include "client.h"
#include "mkgeturl.h"
#include "mkstream.h"
#include "il.h"
#include "cert.h" /* for CERT_DupCertificate() */
#include "extcache.h"
#include "mkextcac.h"

#ifdef PROFILE
#pragma profile on
#endif

/* for XP_GetString() */
#include "xpgetstr.h"
extern int XP_DATABASE_CANT_BE_VALIDATED_MISSING_NAME_ENTRY;
extern int XP_DB_SELECTED_DB_NAMED;
extern int XP_REQUEST_EXTERNAL_CACHE;

PRIVATE XP_List * ext_cache_database_list = 0;
PRIVATE Bool AtLeastOneOpenCache = FALSE;

typedef struct _ExtCacheDBInfo {
	DB   *database;
	char *filename;
	char *path;
	char *name;
	Bool queried_this_session;
} ExtCacheDBInfo;

/* close an external cache 
 */
PRIVATE void
net_CloseExtCacheFat(ExtCacheDBInfo *db_info)
{
	if(db_info->database)
	  {
		(*db_info->database->close)(db_info->database);
		db_info->database = 0;
	  }

}


PRIVATE char *
net_GetExtCacheNameFromDB(ExtCacheDBInfo *db_info)
{
	DBT key;
	DBT data;

	if(!db_info->database)
		return NULL;

	key.data = EXT_CACHE_NAME_STRING;
	key.size = XP_STRLEN(EXT_CACHE_NAME_STRING);

	if(0 == (*db_info->database->get)(db_info->database, &key, &data, 0))
	  {
		/* make sure it's a valid cstring */
		char *name = (char *)data.data;
		if(name[data.size-1] == '\0')
			return(XP_STRDUP(name));
		else
			return(NULL);
	  }

	return(NULL);

}

/* read/open an External Cache File allocation table.
 *
 * Returns a True if successfully opened
 * Returns False if not.
 */
PRIVATE Bool
net_OpenExtCacheFat(MWContext *ctxt, ExtCacheDBInfo *db_info)
{
	char *slash;
	char *db_name;
	XP_Bool close_db=FALSE;

    if(!db_info->database)
      {
		/* @@@ add .db to the end of the files
		 */
		char* filename = WH_FilePlatformName(db_info->filename);
        db_info->database = dbopen(filename,
                             		O_RDONLY,
                             		0600,
                             		DB_HASH,
                             		0);
#ifdef XP_WIN
/* This is probably the last checkin into Akbar */
/* What really needs to be fixed is that Warren's implementation */
/* of WH_FilePlatformName needs to return a malloc'd string */
/* Right now, on Mac & X, it does not. See xp_file.c */
		FREEIF(filename);
#endif
		if(!db_info->database)
			return(FALSE);
		else
			AtLeastOneOpenCache = TRUE;

		StrAllocCopy(db_info->path, db_info->filename);

		/* figure out the path to the database */
#ifdef XP_WIN
  		slash = XP_STRRCHR(db_info->path, '\\');						
#elif defined(XP_MAC)
  		slash = XP_STRRCHR(db_info->path, '/');						
#else
  		slash = XP_STRRCHR(db_info->path, '/');						
#endif

		if(slash)
		  {
		    *(slash+1) = '\0';
		  }
		else
		  {
			*db_info->path = '\0';
		  }

		db_name = net_GetExtCacheNameFromDB(db_info);

		if(!db_name)
		  {
			close_db = !FE_Confirm(ctxt,
			XP_GetString( XP_DATABASE_CANT_BE_VALIDATED_MISSING_NAME_ENTRY ) );
		  }
		else if(XP_STRCMP(db_name, db_info->name))
		  {
			char buffer[2048];
			
			PR_snprintf(buffer, sizeof(buffer),
							  XP_GetString( XP_DB_SELECTED_DB_NAMED ),
							  db_name, db_info->name);
			
			close_db = !FE_Confirm(ctxt, buffer);
		  }

		if(close_db)
		  {
			(*db_info->database->close)(db_info->database);
			db_info->database = 0;
			return(FALSE);
		  }
	  }

 	return(TRUE);
}


PRIVATE void
net_SaveExtCacheInfo(void)
{
	XP_File fp;
	ExtCacheDBInfo *db_info;
	XP_List *list_ptr;

	if(!ext_cache_database_list)
		return;

	fp = XP_FileOpen("", xpExtCacheIndex, XP_FILE_WRITE);

	if(!fp)
		return;

	XP_FileWrite("# Netscape External Cache Index" LINEBREAK
				 "# This is a generated file!  Do not edit." LINEBREAK
				 LINEBREAK,
				 -1, fp);

    /* file format is:
     *   Filename  <TAB> database_name
     */
	list_ptr = ext_cache_database_list;
    while((db_info = (ExtCacheDBInfo *)XP_ListNextObject(list_ptr)))
      {
	
		if(!db_info->name || !db_info->filename)
			continue;

		XP_FileWrite(db_info->filename, -1, fp);
		XP_FileWrite("\t", -1, fp);
		XP_FileWrite(db_info->name, -1, fp);
		XP_FileWrite(LINEBREAK, -1, fp);
      }

	XP_FileClose(fp);
}

#define BUF_SIZE 2048
PRIVATE void
net_ReadExtCacheInfo(void)
{
	XP_File fp;
	ExtCacheDBInfo *new_db_info;
	char buf[BUF_SIZE];
	char *name;

	if(!ext_cache_database_list)
	  {
		ext_cache_database_list = XP_ListNew();
		if(!ext_cache_database_list)
			return;
	  }
	
	fp = XP_FileOpen("", xpExtCacheIndex, XP_FILE_READ);

	if(!fp)
		return;

	/* file format is:
	 *   Filename  <TAB> database_name
	 */
	while(XP_FileReadLine(buf, BUF_SIZE-1, fp))
	  {
		if (*buf == 0 || *buf == '#' || *buf == CR || *buf == LF)
		  continue;

		/* remove /r and /n from the end of the line */
		XP_StripLine(buf);

		name = XP_STRCHR(buf, '\t');

		if(!name)
			continue;

		*name++ = '\0';

		new_db_info = XP_NEW(ExtCacheDBInfo);
        if(!new_db_info)
            return;
        XP_MEMSET(new_db_info, 0, sizeof(ExtCacheDBInfo));

        StrAllocCopy(new_db_info->filename, buf);
        StrAllocCopy(new_db_info->name, name);

        XP_ListAddObject(ext_cache_database_list, new_db_info);
	  }
	
	XP_FileClose(fp);
	
}


PUBLIC void
net_OpenExtCacheFATCallback(MWContext *ctxt, char * filename, void *closure)
{
	ExtCacheDBInfo *db_info = (ExtCacheDBInfo*)closure;

	if(!db_info || !filename)
		return;

	StrAllocCopy(db_info->filename, filename);

	/* once we have the name try and open the
	 * cache fat
	 */
	if(net_OpenExtCacheFat(ctxt, db_info))
	  {
		AtLeastOneOpenCache = TRUE;
	  }
	else
	  {
		if(FE_Confirm(ctxt, "Unable to open External Cache.  Try again?"))
		  {
			/* try and open again */

			FE_PromptForFileName (ctxt,					/* context */
                              db_info->name,			/* prompt string */
                              db_info->filename,	/* default file/path */
                              TRUE,					/* file must exist */
                              FALSE,				/* directories allowed */
							  net_OpenExtCacheFATCallback, /* callback */
							  (void *)db_info);     /* closure */

		
			return; /* don't save ExtCache info */
		  }
	  }

	net_SaveExtCacheInfo();
}

PUBLIC void
NET_OpenExtCacheFAT(MWContext *ctxt, char * cache_name, char * instructions)
{
	ExtCacheDBInfo *db_info=0, *db_ptr;
	Bool done = FALSE;
	XP_List *list_ptr;

	if(!ext_cache_database_list)
	  {
		net_ReadExtCacheInfo();
		if(!ext_cache_database_list)
			return;
	  }

	/* look up the name in a list and open the file
	 * if it's not open already
	 */
	list_ptr = ext_cache_database_list;
	while((db_ptr = (ExtCacheDBInfo *)XP_ListNextObject(list_ptr)))
	  {
		if(db_ptr->name && !XP_STRCMP(db_ptr->name, cache_name))
		  {
			db_info = db_ptr;
			break;
		  }
	  }

	if(!db_info)
	  {
		db_info = XP_NEW(ExtCacheDBInfo);
		if(!db_info)
			return;
		XP_MEMSET(db_info, 0, sizeof(ExtCacheDBInfo));
		StrAllocCopy(db_info->name, cache_name);

		XP_ListAddObject(ext_cache_database_list, db_info);
	  }
	else if(db_info->queried_this_session)
	  {
		/* don't query or try and open again.
		 */
		return;
	  }

	if(db_info->filename)
	  {
		done = net_OpenExtCacheFat(ctxt, db_info);

		if(done)
		  {
			char buffer[1024];

			PR_snprintf(buffer, sizeof(buffer), 
						"Now using external cache: %.900s", 
					   	db_info->name);
			if(!FE_Confirm(ctxt, buffer))
				net_CloseExtCacheFat(db_info);
		  }
	  }

	/* If we don't have the correct filename, query the
	 * user for the name
	 */
	if(!done)
	  {

		if(instructions)
			done = !FE_Confirm(ctxt, instructions);
		else
			done = !FE_Confirm(ctxt, XP_GetString( XP_REQUEST_EXTERNAL_CACHE ) );

		if(!done)
			FE_PromptForFileName (ctxt,					/* context */
                              db_info->name,			/* prompt string */
                              db_info->filename,	/* default file/path */
                              TRUE,					/* file must exist */
                              FALSE,				/* directories allowed */
							  net_OpenExtCacheFATCallback, /* callback */
							  (void *)db_info);     /* closure */
	  }

	db_info->queried_this_session = TRUE;

	return;
}

/* lookup routine
 *
 * builds a key and looks for it in 
 * the database.  Returns an access
 * method and sets a filename in the
 * URL struct if found
 */
MODULE_PRIVATE int
NET_FindURLInExtCache(URL_Struct * URL_s, MWContext *ctxt)
{
	DBT *key;
	DBT data;
	net_CacheObject *found_cache_obj;
	ExtCacheDBInfo *db_ptr;
	int status;
    XP_StatStruct    stat_entry;
	char *filename=0;
	XP_List *list_ptr;

	TRACEMSG(("Checking for URL in external cache"));

	/* zero the last modified date so that we don't
	 * screw up the If-modified-since header by
     * having it in even when the document isn't
	 * cached.
	 */
	URL_s->last_modified = 0;

	if(!AtLeastOneOpenCache)
	  {
		TRACEMSG(("No External Cache databases open"));
	 	return(0); 
	  }

	key = net_GenCacheDBKey(URL_s->address, 
							URL_s->post_data, 
							URL_s->post_data_size);

	status = 1;
    list_ptr = ext_cache_database_list;
    while((db_ptr = (ExtCacheDBInfo *)XP_ListNextObject(list_ptr)))
      {
        if(db_ptr->database)
          {
			TRACEMSG(("Searching databse: %s", db_ptr->name));
			status = (*db_ptr->database->get)(db_ptr->database, key, &data, 0);
			if(status == 0)
				break;
          }
      }

	if(status != 0)
	  {
		TRACEMSG(("Key not found in any database"));
		net_FreeCacheDBTdata(key);
	 	return(0); 
	  }

	found_cache_obj = net_Fast_DBDataToCacheStruct(&data);

    TRACEMSG(("mkcache: found URL in cache!"));

	if(!found_cache_obj)
	    return(0);

	/* copy in the cache file name
	 */
	if(db_ptr->path 
#if REAL_TIME  /* use this for real */
		&& found_cache_obj->is_relative_path
#endif
         )
	  {
		StrAllocCopy(filename, db_ptr->path);
		StrAllocCat(filename, found_cache_obj->filename);
	  }
	else
	  {
		StrAllocCopy(filename, found_cache_obj->filename);
	  }

	/* make sure the file still exists
	 */
	if(XP_Stat(filename, &stat_entry, xpExtCache) == -1)
	  {
		/* file does not exist!!
		 * remove the entry 
		 */
		TRACEMSG(("Error! Cache file missing: %s", filename));

	    net_FreeCacheDBTdata(key);

		FREE(filename);

		return(0);
			
	  }

    /*
     * if the last accessed date is before the startup date set the
     * expiration date really low so that the URL is forced to be rechecked
     * again.  We don't just not return the URL as being in the
     * cache because we want to use "If-modified-since"
	 *
     * This works correctly because mkhttp will zero the
	 * expires field.
	 *
	 * if it's not an http url then just delete the entry
	 * since we can't do an If-modified-since
     */

/* since we can't set a last accessed time
 * we can't do the once per session thing.
 * Always assume EVERY TIME
 *
 *  if(found_cache_obj->last_accessed < NET_StartupTime
 *  	  && NET_CacheUseMethod != CU_NEVER_CHECK)
 */
    if(URL_s->use_local_copy || URL_s->history_num)
      {
        /* we already did an IMS get or it's coming out
		 * of the history.
         * Set the expires to 0 so that we can now use the
         * object
         */
         URL_s->expires = 0;
      }
    else if(NET_CacheUseMethod != CU_NEVER_CHECK)
	  {
		if(!strncasecomp(URL_s->address, "http", 4))
		  {
        	URL_s->expires = 1;
		  }
		else
		  {
			/* object has expired and cant use IMS. Don't return it */

	    	net_FreeCacheDBTdata(key);
			FREE(filename);

			return(0);
		  }
	  }
	else
	  {
		/* otherwise use the normal expires date for CU_NEVER_CHECK */
 		URL_s->expires = found_cache_obj->expires;
	  }

	/* won't need the key anymore */
	net_FreeCacheDBTdata(key);

	URL_s->cache_file = filename;

    /* copy the contents of the URL struct so that the content type
     * and other stuff gets recorded
     */
    StrAllocCopy(URL_s->content_type,     found_cache_obj->content_type);
    StrAllocCopy(URL_s->charset,          found_cache_obj->charset);
    StrAllocCopy(URL_s->content_encoding, found_cache_obj->content_encoding);
    URL_s->content_length = found_cache_obj->content_length;
    URL_s->real_content_length = found_cache_obj->real_content_length;
 	URL_s->last_modified  = found_cache_obj->last_modified;
 	URL_s->is_netsite     = found_cache_obj->is_netsite;

	/* copy security info */
	URL_s->security_on             = found_cache_obj->security_on;
	URL_s->key_size                = found_cache_obj->key_size;
	URL_s->key_secret_size         = found_cache_obj->key_secret_size;
	StrAllocCopy(URL_s->key_cipher,  found_cache_obj->key_cipher);

	/* delete any existing certificate first */
#ifdef HAVE_SECURITY /* added by jwz */
	CERT_DestroyCertificate(URL_s->certificate);
#endif /* !HAVE_SECURITY -- added by jwz */
	URL_s->certificate = NULL;

#ifdef HAVE_SECURITY /* added by jwz */
	if (found_cache_obj->certificate)
		  URL_s->certificate =
			CERT_DupCertificate(found_cache_obj->certificate);
#endif /* !HAVE_SECURITY -- added by jwz */

	TRACEMSG(("Cached copy is valid. returning method"));

	TRACEMSG(("Using Disk Copy"));

	URL_s->ext_cache_file = TRUE;

	return(FILE_CACHE_TYPE_URL);
}

#endif /* MOZILLA_CLIENT */
