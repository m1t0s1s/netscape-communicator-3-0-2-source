/* -*- Mode: C; tab-width: 4 -*-
 */
#include "glhist.h"
#include "xp_hash.h"
#include "net.h"
#include "xp.h"
#include "mcom_db.h"
#include <time.h>
#include "merrors.h"
#include "xpgetstr.h"
#include "bkmks.h"

extern int XP_GLHIST_INFO_HTML;
extern int XP_GLHIST_DATABASE_CLOSED;
extern int XP_GLHIST_UNKNOWN;
extern int XP_GLHIST_DATABASE_EMPTY;
extern int XP_GLHIST_HTML_DATE    ;
extern int XP_GLHIST_HTML_TOTAL_ENTRIES;

/*PRIVATE XP_HashList * global_history_list = 0;*/
PRIVATE Bool          global_history_has_changed = FALSE;
PRIVATE time_t gh_cur_date = 0;
PRIVATE int32 global_history_timeout_interval = -1;

PRIVATE DB * gh_database = 0;
PRIVATE HASHINFO gh_hashinfo;

#define SYNC_RATE 30  /* number of stores before sync */

typedef struct _gh_HistEntry {
    char       * address;
    time_t       last_accessed;
} gh_HistEntry;

#ifndef BYTE_ORDER
Error!  byte order must be defined
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
#define COPY_INT32(_a,_b)  XP_MEMCPY(_a, _b, sizeof(int32));
#else
#define COPY_INT32(_a,_b)  /* swap */                   \
    ((char *)(_a))[0] = ((char *)(_b))[3];              \
    ((char *)(_a))[1] = ((char *)(_b))[2];              \
    ((char *)(_a))[2] = ((char *)(_b))[1];              \
    ((char *)(_a))[3] = ((char *)(_b))[0];
#endif

/* set the maximum time for an object in the Global history in
 * number of seconds
 */
PUBLIC void
GH_SetGlobalHistoryTimeout(int32 timeout_interval)
{
    global_history_timeout_interval = timeout_interval;
}

PRIVATE void
gh_set_hash_options(void)
{
    gh_hashinfo.bsize = 4*1024;
    gh_hashinfo.nelem = 0;
    gh_hashinfo.hash = NULL;
    gh_hashinfo.ffactor = 0;
    gh_hashinfo.cachesize = 64 * 1024U;
    gh_hashinfo.lorder = 0;
}


PRIVATE void
gh_open_database(void)
{
#ifndef NO_DBM
	static Bool have_tried_open=FALSE;

	if(gh_database)
	  {
		return;
	  }
	else
	  {
		char* filename;
    	gh_set_hash_options();
		filename = WH_FileName("", xpGlobalHistory);
		gh_database = dbopen(filename,
							 O_RDWR | O_CREAT,
							 0600,
							 DB_HASH,
							 &gh_hashinfo);
		if (filename) XP_FREE(filename);

		if(!have_tried_open && !gh_database)
	      {
            XP_StatStruct stat_entry;

			have_tried_open = TRUE; /* only try this once */

			TRACEMSG(("Could not open gh database -- errno: %d", errno));


            /* if the file is zero length remove it
             */
            if(XP_Stat("", &stat_entry, xpGlobalHistory) != -1)
              {
                  if(stat_entry.st_size <= 0)
				    {
					  char* filename = WH_FileName("", xpGlobalHistory);
					  if (!filename) return;
					  XP_FileRemove(filename, xpGlobalHistory);
					  XP_FREE(filename);
				    }
				  else
				    {
						XP_File fp;
#define BUFF_SIZE 1024
						char buffer[BUFF_SIZE];

						/* open the file and look for
						 * the old magic cookie.  If it's
						 * there delete the file
						 */
						fp = XP_FileOpen("", xpGlobalHistory, XP_FILE_READ);

						if(fp)
						  {
							XP_FileReadLine(buffer, BUFF_SIZE, fp);
		
							XP_FileClose(fp);

							if(XP_STRSTR(buffer, "Global-history-file")) {
								char* filename = WH_FileName("", xpGlobalHistory);
								if (!filename) return;
					  		    XP_FileRemove(filename, xpGlobalHistory);
								XP_FREE(filename);
							}
						  }
				    }
              }

            /* try it again */
			filename = WH_FileName("", xpGlobalHistory);
			gh_database = dbopen(filename,
							 O_RDWR | O_CREAT,
							 0600,
							 DB_HASH,
							 &gh_hashinfo);
			if (filename) XP_FREE(filename);

			return;

	      }

        if(gh_database && -1 == (*gh_database->sync)(gh_database, 0))
          {
            TRACEMSG(("Error syncing gh database"));
			(*gh_database->close)(gh_database);
            gh_database = 0;
          }
	  }
#endif /* NO_DBM */

}

/* if the url was found in the global history then a number between
 * 0 and 99 is returned representing the percentage of time that
 * has elapsed in the expiration cycle.
 *  0  means most recently accessed
 * 99  means least recently accessed (about to be expired)
 *
 * If the url was not found -1 is returned
 *
 * define USE_PERCENTS if you want to get percent of time
 * through expires cycle.
 */
PUBLIC int
GH_CheckGlobalHistory(char * url)
{
	DBT key;
    DBT data;
	int status;
	time_t entry_date;

	if(!url)
	    return(-1);

	if(!gh_database)
		return(-1);

	key.data = (void *) url;
	key.size = (XP_STRLEN(url)+1) * sizeof(char);

	status = (*gh_database->get)(gh_database, &key, &data, 0);

	if(status < 0)
	  {
		TRACEMSG(("Database ERROR retreiving global history entry"));
		return(-1);
	  }
	else if(status > 0)
	  {
		return(-1);
	  }

	/* otherwise */

	/* object was found.
	 * check the time to make sure it hasn't expired
	 */
	COPY_INT32(&entry_date, data.data);
    if(global_history_timeout_interval > 0
		&& entry_date+global_history_timeout_interval < gh_cur_date)
	  {
		/* remove the object
	 	 */
	    (*gh_database->del)(gh_database, &key, 0);
		
		/* return not found
		 */
		return(-1);
	  }

	return(1);
}

/* start global history tracking
 */
PUBLIC void
GH_InitGlobalHistory(void)
{
	gh_open_database();
}

PRIVATE void
gh_RemoveDatabase(void)
{
	char* filename;
	if(gh_database)
	  {
		(*gh_database->close)(gh_database);
		gh_database = 0;
	  }
	filename = WH_FileName("", xpGlobalHistory);
	if (!filename) return;
	XP_FileRemove(filename, xpGlobalHistory);
	XP_FREE(filename);
}

/* add or update the url in the global history
 */
PUBLIC void
GH_UpdateGlobalHistory(URL_Struct * URL_s)
{
	DBT key, data;
	int status;
	static int32 count=0;
	uint32 date;

	/* check for NULL's
	 * and also don't allow ones with post-data in here
 	 */
	if(!URL_s || !URL_s->address || URL_s->post_data)
	    return;

	/* Never save these in the history database */
	if (!strncasecomp(URL_s->address, "about:", 6) ||
		!strncasecomp(URL_s->address, "javascript:", 11) ||
		!strncasecomp(URL_s->address, "livescript:", 11) ||
		!strncasecomp(URL_s->address, "mailbox:", 8) ||
		!strncasecomp(URL_s->address, "mailto:", 7) ||
		!strncasecomp(URL_s->address, "mocha:", 6) ||
		!strncasecomp(URL_s->address, "news:", 5) ||
		!strncasecomp(URL_s->address, "pop3:", 5) ||
		!strncasecomp(URL_s->address, "snews:", 6) ||
		!strncasecomp(URL_s->address, "view-source:", 12))
	  return;

	gh_cur_date = time(NULL);

	BM_UpdateBookmarksTime(URL_s, gh_cur_date);

	if(global_history_timeout_interval == 0)
	    return;  /* don't add ever */

	gh_open_database();

	if(!gh_database)
		return;

	global_history_has_changed = TRUE;

	count++;  /* increment change count */

	key.data = (void *) URL_s->address;
	key.size = (XP_STRLEN(URL_s->address)+1) * sizeof(char);

	COPY_INT32(&date, &gh_cur_date);
	data.data = (void *) &date;
	data.size = sizeof(int32);

	status = (*gh_database->put)(gh_database, &key, &data, 0);

	if(status < 0)
	  {
		TRACEMSG(("Global history update failed due to database error"));
		gh_RemoveDatabase();
	  }
    else if(count >= SYNC_RATE)
      {
		count = 0;
		if(-1 == (*gh_database->sync)(gh_database, 0))
		  {
            TRACEMSG(("Error syncing gh database"));
		  }
	  }

	
}

#define MAX_HIST_DBT_SIZE 1024

PRIVATE DBT *
gh_HistDBTDup(DBT *obj)
{
    DBT * rv = XP_NEW(DBT);

	if(!rv || obj->size > MAX_HIST_DBT_SIZE)
		return NULL;

    rv->size = obj->size;
    rv->data = XP_ALLOC(rv->size);
	if(!rv->data)
	  {
		XP_FREE(rv);
		return NULL;
	  }

    XP_MEMCPY(rv->data, obj->data, rv->size);

    return(rv);

}

PRIVATE void
gh_FreeHistDBTdata(DBT *stuff)
{
    XP_FREE(stuff->data);
    XP_FREE(stuff);
}


/* runs through a portion of the global history
 * database and removes all objects that have expired
 *
 */
PUBLIC void
GH_CollectGarbage(void)
{
#define OLD_ENTRY_ARRAY_SIZE 100
    DBT *old_entry_array[OLD_ENTRY_ARRAY_SIZE];

	DBT key, data;	
	DBT *newkey;
	time_t entry_date;
	int i, old_entry_count=0;
	
	if(!gh_database || global_history_timeout_interval < 1)
		return;

	gh_cur_date = time(NULL);

	if(0 != (*gh_database->seq)(gh_database, &key, &data, R_FIRST))
		return;
	
	global_history_has_changed = TRUE;

	do
	  {
    	COPY_INT32(&entry_date, data.data);
    	if(global_history_timeout_interval > 0
           && entry_date+global_history_timeout_interval < gh_cur_date)
      	  {
        	/* put the object on the delete list since it is expired
         	 */
			if(old_entry_count < OLD_ENTRY_ARRAY_SIZE)
				old_entry_array[old_entry_count++] = gh_HistDBTDup(&key);
			else
				break;
      	  }
	  }
	while(0 == (gh_database->seq)(gh_database, &key, &data, R_NEXT));

	for(i=0; i < old_entry_count; i++)
	  {
		newkey = old_entry_array[i];
		if(newkey)
		  {
       	    (*gh_database->del)(gh_database, newkey, 0);
		    gh_FreeHistDBTdata(newkey);
		  }
	  }
}

/* save the global history to a file while leaving the object in memory
 */
PUBLIC void
GH_SaveGlobalHistory(void)
{

	if(!gh_database)
		return;

	GH_CollectGarbage();

	if(global_history_has_changed)
      {
	    if(-1 == (*gh_database->sync)(gh_database, 0))
	      {
		    TRACEMSG(("Error syncing gh database"));
			(*gh_database->close)(gh_database);
			gh_database = 0;
	      }
	    global_history_has_changed = FALSE;
	  }
}

/* free the global history list
 */
PUBLIC void
GH_FreeGlobalHistory(void)
{
	if(!gh_database)
		return;

	if(-1 == (*gh_database->close)(gh_database))
	  {
		 TRACEMSG(("Error closing gh database"));
	  }

	gh_database = 0;
	
}

/* clear the global history list
 */
PUBLIC void
GH_ClearGlobalHistory(void)
{
	char* filename;
	if(!gh_database)
		return;


	GH_FreeGlobalHistory();

    gh_set_hash_options();

#ifndef NO_DBM
	filename = WH_FileName("", xpGlobalHistory);
	gh_database = dbopen(filename,
						 O_RDWR | O_TRUNC,
						 0600,
						 DB_HASH,
						 &gh_hashinfo);
	if (filename) XP_FREE(filename);
#endif /* NO_DBM */
    if(gh_database && -1 == (*gh_database->sync)(gh_database, 0))
      {
        TRACEMSG(("Error syncing gh database"));
		(*gh_database->close)(gh_database);
        gh_database = 0;
      }


	global_history_has_changed = FALSE;
}


/* create an HTML stream and push a bunch of HTML about
 * the global history
 */
MODULE_PRIVATE int
NET_DisplayGlobalHistoryInfoAsHTML(MWContext *context,
								   URL_Struct *URL_s,
								   int format_out)
{
	char *buffer = (char*)XP_ALLOC(256);
   	NET_StreamClass * stream;
	DBT key, data;
	Bool long_form = FALSE;
	time_t entry_date;
	int status = MK_NO_DATA;
	int32 count=0;

	if(!buffer)
	  {
		return(MK_UNABLE_TO_CONVERT);
	  }

	if(strcasestr(URL_s->address, "?long"))
		long_form = TRUE;

	StrAllocCopy(URL_s->content_type, TEXT_HTML);

	format_out = CLEAR_CACHE_BIT(format_out);
	stream = NET_StreamBuilder(format_out,
							   URL_s,
							   context);

	if(!stream)
	  {
		return(MK_UNABLE_TO_CONVERT);
	  }


	/* define a macro to push a string up the stream
	 * and handle errors
	 */
#define PUT_PART(part)													\
status = (*stream->put_block)(stream->data_object,			\
										part ? part : XP_GetString(XP_GLHIST_UNKNOWN),	 \
										part ? XP_STRLEN(part) : 7);	\
if(status < 0)												\
  goto END;

	XP_SPRINTF(buffer, XP_GetString(XP_GLHIST_INFO_HTML));

	PUT_PART(buffer);

	if(!gh_database)
	  {
		XP_STRCPY(buffer, XP_GetString(XP_GLHIST_DATABASE_CLOSED));
		PUT_PART(buffer);
		goto END;
	  }

    if(0 != (*gh_database->seq)(gh_database, &key, &data, R_FIRST))
	  {
		XP_STRCPY(buffer, XP_GetString(XP_GLHIST_DATABASE_EMPTY));
		PUT_PART(buffer);
		goto END;
	  }

	/* define some macros to help us output HTML tables
	 */
#define TABLE_TOP(arg1)				\
	XP_SPRINTF(buffer, 				\
"<TR><TD ALIGN=RIGHT><b>%s</TD>\n"	\
"<TD>", arg1);						\
PUT_PART(buffer);

#define TABLE_BOTTOM				\
	XP_SPRINTF(buffer, 				\
"</TD></TR>");						\
PUT_PART(buffer);

    do
      {
		count++;

		COPY_INT32(&entry_date, data.data);

		/* print url */
		XP_STRCPY(buffer, "<TT>&nbsp;URL:</TT> ");
		PUT_PART(buffer);

		/* push the key special since we know the size */
		status = (*stream->put_block)(stream->data_object,
									  (char*)key.data, key.size);
		if(status < 0)
  			goto END;

		XP_SPRINTF(buffer, XP_GetString(XP_GLHIST_HTML_DATE), ctime(&entry_date));
		PUT_PART(buffer);

      }
    while(0 == (*gh_database->seq)(gh_database, &key, &data, R_NEXT));

	
	XP_SPRINTF(buffer, XP_GetString(XP_GLHIST_HTML_TOTAL_ENTRIES), count);
	PUT_PART(buffer);

END:
	XP_FREE(buffer);
	
	if(status < 0)
		(*stream->abort)(stream->data_object, status);
	else
		(*stream->complete)(stream->data_object);
	XP_FREE(stream);

	return(status);
}
