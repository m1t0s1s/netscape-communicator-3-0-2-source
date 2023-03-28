/* -*- Mode: C; tab-width: 4 -*- */

#include "mkutils.h"
#include "mkgeturl.h"
#include "mkstream.h"
#include "mkformat.h"
#include "mkparse.h"
#include "mkftp.h"
#include "mkfsort.h"
#include "mkfile.h"
#include "merrors.h"

#include "il.h"  /* for the image type recognition stuff */

#ifdef XP_WIN
#include "errno.h"
#endif

/* for XP_GetString() */
#include "xpgetstr.h"
extern int XP_ALERT_OUTMEMORY;
extern int XP_PROGRESS_DIRDONE;
extern int XP_PROGRESS_FILEDONE;
extern int XP_PROGRESS_FILEZEROLENGTH;
extern int XP_PROGRESS_READDIR;
extern int XP_PROGRESS_READFILE;
extern int MK_OUT_OF_MEMORY;
extern int MK_UNABLE_TO_LOCATE_FILE;
extern int MK_UNABLE_TO_OPEN_FILE;
extern int MK_ZERO_LENGTH_FILE;
extern int MK_UNABLE_TO_DELETE_FILE;
extern int MK_UNABLE_TO_DELETE_DIRECTORY;
extern int MK_MKDIR_FILE_ALREADY_EXISTS;
extern int MK_COULD_NOT_CREATE_DIRECTORY;
extern int MK_ERROR_WRITING_FILE;
extern int MK_NOT_A_DIRECTORY;
extern int XP_READING_SEGMENT;
extern int XP_TITLE_DIRECTORY_LISTING;
extern int XP_H1_DIRECTORY_LISTING;
extern int XP_UPTO_HIGHER_LEVEL_DIRECTORY;

/* states of the machine
 */
typedef enum {
	NET_CHECK_FILE_TYPE,
	NET_DELETE_FILE,
	NET_MOVE_FILE,
	NET_PUT_FILE,
	NET_MAKE_DIRECTORY,
	NET_FILE_SETUP_STREAM,
	NET_OPEN_FILE,
	NET_SETUP_FILE_STREAM,
	NET_OPEN_DIRECTORY,
	NET_READ_FILE_CHUNK,
	NET_READ_DIRECTORY_CHUNK,
	NET_BEGIN_PRINT_DIRECTORY,
	NET_PRINT_DIRECTORY,
	NET_FILE_DONE,
	NET_FILE_ERROR_DONE,
	NET_FILE_FREE
} net_FileStates;

/* allow URL byterange requests for backwards compatibility,
 * but only leave this in for a short while
 */
#define URL_BYTERANGE_METHOD

#define DIR_STRUCT struct dirent

typedef struct _FILEConData {
    XP_File           file_ptr;
    XP_Dir            dir_ptr;
    net_FileStates    next_state;
    NET_StreamClass * stream;
    char            * filename;
    Bool           is_dir;
    SortStruct      * sort_base;
    Bool           pause_for_read;
    Bool           is_cache_file;
    Bool destroy_graph_progress;  /* do we need to destroy graph progress? */
    int32   original_content_length; /* the content length at the time of
                                      * calling graph progress
                                      */
	char             * byterange_string; 
	int32              range_length;  /* the length of the current byte range */
} FILEConData;

#define CD_FILE_PTR       connection_data->file_ptr
#define CD_DIR_PTR        connection_data->dir_ptr
#define CD_NEXT_STATE     connection_data->next_state
#define CD_STREAM         connection_data->stream
#define CD_FILENAME       connection_data->filename
#define CD_IS_DIR         connection_data->is_dir
#define CD_SORT_BASE      connection_data->sort_base
#define CD_PAUSE_FOR_READ connection_data->pause_for_read
#define CD_IS_CACHE_FILE  connection_data->is_cache_file

#define CE_WINDOW_ID      cur_entry->window_id
#define CE_URL_S          cur_entry->URL_s
#define CE_STATUS         cur_entry->status
#define CE_SOCK           cur_entry->socket
#define CE_BYTES_RECEIVED cur_entry->bytes_received
#define CE_FORMAT_OUT     cur_entry->format_out
#define CD_DESTROY_GRAPH_PROGRESS   connection_data->destroy_graph_progress
#define CD_ORIGINAL_CONTENT_LENGTH  connection_data->original_content_length
#define CD_BYTERANGE_STRING         connection_data->byterange_string
#define CD_RANGE_LENGTH             connection_data->range_length

#define PUTS(s)           (*connection_data->stream->put_block) \
                                 (connection_data->stream->data_object, s, XP_STRLEN(s))
#define IS_WRITE_READY    (*connection_data->stream->is_write_ready) \
                                 (connection_data->stream->data_object)
#define PUTB(b,l)         (*connection_data->stream->put_block) \
                                (connection_data->stream->data_object, b, l)
#define COMPLETE_STREAM   (*connection_data->stream->complete) \
                                 (connection_data->stream->data_object)
#define ABORT_STREAM(s)   (*connection_data->stream->abort) \
                                 (connection_data->stream->data_object, s)

/* try and open the URL path as a normal file
 * if it fails set the next state to try and open
 * a directory.
 */
PRIVATE int
net_check_file_type (ActiveEntry * cur_entry)
{
    FILEConData * connection_data = (FILEConData *) cur_entry->con_data;
    XP_StatStruct stat_entry;

	TRACEMSG(("Looking for file: %s", CD_FILENAME));

    if(-1 == XP_Stat (CD_FILENAME, 
				&stat_entry, 
				CD_IS_CACHE_FILE ? 
					(CE_URL_S->ext_cache_file ? xpExtCache : xpCache) : xpURL))
	  {
		if(CE_URL_S->method == URL_PUT_METHOD)
	  	  {
			CD_NEXT_STATE = NET_PUT_FILE;
			return(0);
	  	  }
		else if(CE_URL_S->method == URL_MKDIR_METHOD)
	  	  {
			CD_NEXT_STATE = NET_MAKE_DIRECTORY;
			return(0);
	  	  }
		

		CE_URL_S->error_msg = NET_ExplainErrorDetails(
											MK_UNABLE_TO_LOCATE_FILE,
 											*CD_FILENAME ? CD_FILENAME : "/");
        return(MK_UNABLE_TO_LOCATE_FILE);
	  }

	if(!CD_IS_CACHE_FILE)
	  {
		CE_URL_S->last_modified = stat_entry.st_mtime;
	  }

    if(S_ISDIR(stat_entry.st_mode))
	  {
        /* if the current address doesn't end with a
         * slash, add it now.
         */
        if(CE_URL_S->address[XP_STRLEN(CE_URL_S->address)-1] != '/')
            StrAllocCat(CE_URL_S->address, "/");

		CE_URL_S->is_directory = TRUE;
        CD_IS_DIR = TRUE;
	  }
	else
	  {
        CE_URL_S->content_length = (int32) stat_entry.st_size;
	  }

	if(CD_IS_CACHE_FILE)
	  {
		/* if we are looking for a cache file then
		 * we are always looking to do a GET of
		 * the file regardless of the method in
		 * the URL_s
		 */
   		CD_NEXT_STATE = NET_FILE_SETUP_STREAM;
	  }
	else switch(CE_URL_S->method)
	  {
		case URL_PUT_METHOD:
			{
			  char * msg;
			  msg = PR_smprintf("Overwrite file %s?", CD_FILENAME);
			  if(FE_Confirm(CE_WINDOW_ID, msg))
				  CD_NEXT_STATE = NET_PUT_FILE;
			  else
				  CD_NEXT_STATE = NET_FILE_DONE;
			}
			break;

		case URL_MKDIR_METHOD:
			/* error, can't create a directory over
		 	 * an existing file or directory
		 	 */
			CE_URL_S->error_msg = NET_ExplainErrorDetails(
												MK_MKDIR_FILE_ALREADY_EXISTS,
												CD_FILENAME);
			return(MK_MKDIR_FILE_ALREADY_EXISTS);
	
		case URL_HEAD_METHOD:
			/* we just need the relavent file info.
			 * it is currently filled into the URL struct
			 */
    		CD_NEXT_STATE = NET_FILE_DONE;
			break;

		case URL_MOVE_METHOD:
			CD_NEXT_STATE = NET_MOVE_FILE;
			break;

		case URL_DELETE_METHOD:
			CD_NEXT_STATE = NET_DELETE_FILE;
			break;
	
		case URL_GET_METHOD:
    		CD_NEXT_STATE = NET_FILE_SETUP_STREAM;
			break;

        case URL_INDEX_METHOD:
            if(!CE_URL_S->is_directory)
              {
                CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_NOT_A_DIRECTORY, CD_FILENAME);
                return(MK_NOT_A_DIRECTORY);
              }
            CD_NEXT_STATE = NET_FILE_SETUP_STREAM;
            break;

		default:
			XP_ASSERT(0);
    		CD_NEXT_STATE = NET_FILE_DONE;
			break;
	  }
			
    return(0);
}

PRIVATE int
net_delete_file (ActiveEntry * ce)
{
#ifndef LIVEWIRE
	XP_ASSERT(0);
	return(MK_OUT_OF_MEMORY);

#else
	FILEConData * cd = (FILEConData *) ce->con_data;

	TRACEMSG(("Deleteing file: %s", cd->filename));

	if(cd->is_dir)
	  {
        if(0 != XP_RemoveDirectory(cd->filename, xpURL))
          {
            /* error */
            ce->URL_s->error_msg = NET_ExplainErrorDetails(
                                                MK_UNABLE_TO_DELETE_DIRECTORY, 
												cd->filename);
            return(MK_UNABLE_TO_DELETE_DIRECTORY);
          }
	  }
	else
	  {
		if(0 != XP_FileRemove(cd->filename, xpURL))
	  	  {
			/* error */
			ce->URL_s->error_msg = NET_ExplainErrorDetails(
													MK_UNABLE_TO_DELETE_FILE,
													cd->filename);
			return(MK_UNABLE_TO_DELETE_FILE);
	  	  }
	  }

	/* success */
	cd->next_state = NET_FILE_DONE;
	return(0);
#endif /* NOT LIVEWIRE */
}

PRIVATE int
net_move_file (ActiveEntry * ce)
{
#ifndef LIVEWIRE
	XP_ASSERT(0);
	return(MK_OUT_OF_MEMORY);

#else
	FILEConData * cd = (FILEConData *) ce->con_data;
	char *destination;

	XP_ASSERT(ce->URL_s->destination);
	if(!ce->URL_s->destination)
		return(MK_UNABLE_TO_LOCATE_FILE);

	destination = NET_ParseURL(ce->URL_s->destination, GET_PATH_PART);

	if(!destination)
		return(MK_OUT_OF_MEMORY);

	TRACEMSG(("Moving file: %s", cd->filename));

	if(0 != XP_FileRename(cd->filename, xpURL, destination, xpURL))
	  {
	  	/* error! */
		XP_ASSERT(0);
		/* @@@@@ */
		return(MK_UNABLE_TO_LOCATE_FILE);
	  }

	/* success */
	cd->next_state = NET_FILE_DONE;
	return(0);
#endif /* !LIVEWIRE */
}

PRIVATE int
net_put_file (ActiveEntry * ce)
{
#ifndef LIVEWIRE
	XP_ASSERT(0);
	return(MK_OUT_OF_MEMORY);

#else
    FILEConData * cd = (FILEConData *) ce->con_data;
	XP_File fp;
	int status;

	if(!(fp = XP_FileOpen(cd->filename, xpURL, XP_FILE_WRITE_BIN)))
	  {
		/* error */
		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_OPEN_FILE, 
													  cd->filename);
		return(MK_UNABLE_TO_OPEN_FILE);
	  }
		
	XP_ASSERT(ce->URL_s->post_data);
	XP_ASSERT(!ce->URL_s->post_data_is_file);

	status = XP_FileWrite(ce->URL_s->post_data, ce->URL_s->post_data_size, fp);

	XP_FileClose(fp);

	if(status < ce->URL_s->post_data_size)
	  {
		/* error */
		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_FILE_WRITE_ERROR, 
													  cd->filename);
		return(MK_FILE_WRITE_ERROR);
	  }

	/* success */
	cd->next_state = NET_FILE_DONE;
	return(0);
#endif /* !LIVEWIRE */
}

PRIVATE int
net_make_directory (ActiveEntry * ce)
{
#ifndef LIVEWIRE
	XP_ASSERT(0);
	return(MK_OUT_OF_MEMORY);

#else
	FILEConData * cd = (FILEConData *) ce->con_data;

	if(0 != XP_MakeDirectory(cd->filename, xpURL))
	  {
		/* error */
		ce->URL_s->error_msg = NET_ExplainErrorDetails(
												MK_COULD_NOT_CREATE_DIRECTORY,
                                                cd->filename);
        return(MK_COULD_NOT_CREATE_DIRECTORY);
	  }

	/* success */
	cd->next_state = NET_FILE_DONE;
	return (0);
#endif /* !LIVEWIRE */
}

/* setup the output stream
 */
PRIVATE int
net_file_setup_stream(ActiveEntry * cur_entry)
{
    FILEConData * connection_data = (FILEConData *) cur_entry->con_data;

    if(CD_IS_DIR)
      {        
        StrAllocCopy(CE_URL_S->content_type, TEXT_HTML);

        CD_STREAM = NET_StreamBuilder(CE_FORMAT_OUT, CE_URL_S, CE_WINDOW_ID);

        if(!CD_STREAM)
		  {
		    CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CONVERT);
            return(MK_UNABLE_TO_CONVERT);
		  }

#ifdef MOZILLA_CLIENT
		GH_UpdateGlobalHistory(CE_URL_S);
#endif /* MOZILLA_CLIENT */

        CD_NEXT_STATE = NET_OPEN_DIRECTORY;
      }
	else
      {
	    /* 
	     * don't open the stream yet for files
	     */
        CD_NEXT_STATE = NET_OPEN_FILE;
      }

    return(0);  /* OK */
}

PRIVATE int
net_open_file (ActiveEntry * cur_entry)
{
    FILEConData * connection_data = (FILEConData *) cur_entry->con_data;

    TRACEMSG(("MKFILE: Trying to open: %s\n",CD_FILENAME));

    if((CD_FILE_PTR = XP_FileOpen(CD_FILENAME, 
								  CD_IS_CACHE_FILE ? 
									(CE_URL_S->ext_cache_file ? 
										xpExtCache : xpCache) : xpURL, 
								  XP_FILE_READ_BIN)) != NULL)
	  {
        CD_NEXT_STATE = NET_READ_FILE_CHUNK;
	  }
    else
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_OPEN_FILE, 
													  CD_FILENAME);

		/* if errno is equal to "Too many open files"
		 * return a special error
		 */
#ifndef XP_MAC
		if(errno == EMFILE)
        	return(MK_TOO_MANY_OPEN_FILES);
		else
#endif
        	return(MK_UNABLE_TO_OPEN_FILE);
	  }

    if (!CE_URL_S->load_background)
        NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_READFILE));

    CE_SOCK = XP_Fileno(CD_FILE_PTR);
    cur_entry->local_file = TRUE;
    FE_SetFileReadSelect(CE_WINDOW_ID, XP_Fileno(CD_FILE_PTR));

	/* @@@ now that we have determined that it is a local file
	 * set URL_s->cache_file so that the streams think
	 * that this file came from the cache
	 */
    StrAllocCopy(CE_URL_S->cache_file, CD_FILENAME);

    CD_PAUSE_FOR_READ = TRUE;

	CD_NEXT_STATE = NET_SETUP_FILE_STREAM;

    if (!CE_URL_S->load_background) {
        FE_GraphProgressInit(CE_WINDOW_ID, CE_URL_S, CE_URL_S->content_length);
        CD_DESTROY_GRAPH_PROGRESS = TRUE;  /* we will need to destroy it */
    }
    CD_ORIGINAL_CONTENT_LENGTH = CE_URL_S->content_length;

    return(0);  /* ok */
}


#ifdef MOZILLA_CLIENT
extern int net_InitializeNewsFeData (ActiveEntry * cur_entry);
#endif /* MOZILLA_CLIENT */

PRIVATE int
net_setup_file_stream (ActiveEntry * cur_entry)
{
    FILEConData * connection_data = (FILEConData *) cur_entry->con_data;
	int32 count;

	/* set up the stream now
	 */
    if(!CE_URL_S->content_type || !CD_IS_CACHE_FILE)
	  {
		int img_type;
		
	    /* read the first chunk of the file
	     */
        count = XP_FileRead(NET_Socket_Buffer, NET_Socket_Buffer_Size, CD_FILE_PTR);
    
        if(!count)
          {
            FE_ClearFileReadSelect(CE_WINDOW_ID, XP_Fileno(CD_FILE_PTR));
            NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_FILEZEROLENGTH));
            XP_FileClose(CD_FILE_PTR);
            CE_SOCK = SOCKET_INVALID;
            CD_FILE_PTR = 0;
            CD_NEXT_STATE = NET_FILE_ERROR_DONE;
		    CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_ZERO_LENGTH_FILE, 
												          CD_FILENAME);
            return(MK_ZERO_LENGTH_FILE);
          }
    
#ifdef MOZILLA_CLIENT
		if((img_type = IL_Type(NET_Socket_Buffer, count))
			&& img_type != IL_NOTFOUND)
		  {
			switch(img_type)
			  {
				case IL_GIF:
            		StrAllocCopy(CE_URL_S->content_type, IMAGE_GIF);
					break;
				case IL_XBM:
            		StrAllocCopy(CE_URL_S->content_type, IMAGE_XBM);
					break;
				case IL_JPEG:
            		StrAllocCopy(CE_URL_S->content_type, IMAGE_JPG);
					break;
				case IL_PPM:
            		StrAllocCopy(CE_URL_S->content_type, IMAGE_PPM);
					break;
				default:
					assert(0);  /* should NEVER get this!! */
					break;
			  }
					
		  }
		else
#endif /* MOZILLA_CLIENT */
		{
#ifdef XP_MAC
		  	char *	macType, *macEncoding;
		  	Bool	useDefault;
			FE_FileType(CE_URL_S->address, &useDefault, &macType, &macEncoding);
			if (useDefault)
			{
#endif
				NET_cinfo * type = NET_cinfo_find_type(CE_URL_S->address);
				NET_cinfo * enc = NET_cinfo_find_enc(CE_URL_S->address);

#ifdef XP_MAC
				if (macType)
					XP_FREE(macType);
#endif
	            StrAllocCopy(CE_URL_S->content_type, type->type); 
	            StrAllocCopy(CE_URL_S->content_encoding, enc->encoding); 
	
				if(type->is_default && NET_ContainsHTML(NET_Socket_Buffer, count))
	            	StrAllocCopy(CE_URL_S->content_type, TEXT_HTML);
#ifdef XP_MAC
			}
			else
			{
				CE_URL_S->content_type = macType;
				CE_URL_S->content_encoding = macEncoding;
			}
#endif
		  }
	  }

	CD_RANGE_LENGTH = 0;
	CE_BYTES_RECEIVED = 0;

	/* check to see if we need to do a byterange here
     * If we are then set the ranges in the URL
     * so that the streams know the byte range.
	 * Once we parse one byterange copy
	 */
	if(CD_BYTERANGE_STRING && *CD_BYTERANGE_STRING)
	  {
		int32 low=0, high=0;
		char *cp, *dash;

		cp = XP_STRCHR(CD_BYTERANGE_STRING, ',');
		if(cp)
		    *cp = '\0';

		dash = XP_STRCHR(CD_BYTERANGE_STRING, '-');

		if(!dash)
		  {
			/* no dash is an invalid byte range
			 * return zero and we will come back
			 * in here and give the whole file or
			 * the next byterange
			 */
			return(0);
		  }
		else
		  {
			*dash = '\0';
			
			if(CD_BYTERANGE_STRING == dash)
			    low = -1;
			else
			    low = atol(CD_BYTERANGE_STRING);
			high = atol(dash+1);
		  }


		if(low < 0 && high < 1)
		  {
			/* if both low and high are non-positive
			 * it's an error
			 */
			return(0);
		  }
		else if(low < 0)
		  {
			/* This is a byte range that specifies
			 * the last bytes of a file.  For
			 * instance  http://host/dir/foo;byterange=-500
			 */
			low = CE_URL_S->content_length-(atoi(CD_BYTERANGE_STRING+1));
			high = CE_URL_S->content_length;
		  }
		else if(high < 1)
		  {
			/* This is a byte range that specifies
			 * the a byte range from a specified point 
			 * till the end of the file.  For
			 * instance  http://host/dir/foo;byterange=500-
			 */
			high = CE_URL_S->content_length;
		  }

		if(low >= CE_URL_S->content_length)
		  {
			/* error to have the low byte range be larger than
			 * the file
			 */
			return(0);
		  }
		else if(high >= CE_URL_S->content_length)
		  {
			high = CE_URL_S->content_length;
		  }

		CE_URL_S->low_range = low;
		CE_URL_S->high_range = high;

		CD_RANGE_LENGTH = (high-low)+1;

		if(cp)
		    XP_MEMCPY(CD_BYTERANGE_STRING, cp+1, XP_STRLEN(cp+1)+1);
		else
			CD_BYTERANGE_STRING = 0;
	  }

	if (CLEAR_CACHE_BIT(CE_FORMAT_OUT) == FO_PRESENT &&
		NET_URL_Type(CE_URL_S->address) == NEWS_TYPE_URL)
	  {
#ifdef MOZILLA_CLIENT
		/* #### DISGUSTING KLUDGE to make cacheing work for news articles. */
		CE_STATUS = net_InitializeNewsFeData (cur_entry);
		if (CE_STATUS < 0)
		  {
			/* #### what error message? */
			return CE_STATUS;
		  }
#else
		XP_ASSERT(0);
#endif /* MOZILLA_CLIENT */
	  }

    CD_STREAM = NET_StreamBuilder(CE_FORMAT_OUT, CE_URL_S, CE_WINDOW_ID);

    if(!CD_STREAM)
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CONVERT);
        return(MK_UNABLE_TO_CONVERT);
	  }

#ifdef MOZILLA_CLIENT
	GH_UpdateGlobalHistory(CE_URL_S);
#endif /* MOZILLA_CLIENT */

	/* seek back to the beginning so that
	 * we are not forced to write out this
	 * first block without calling is_write_ready
	 *
	 * this also seeks to the beginning of the range that we
	 * are going to send
 	 */
	XP_FileSeek(CD_FILE_PTR, CE_URL_S->low_range, SEEK_SET);

	CD_NEXT_STATE = NET_READ_FILE_CHUNK;
    CD_PAUSE_FOR_READ = TRUE;

    return(CE_STATUS);  /* ok */
}

PRIVATE int
net_open_directory (ActiveEntry * cur_entry)
{
    FILEConData * connection_data = (FILEConData *) cur_entry->con_data;

	TRACEMSG(("Trying to open directory: %s", CD_FILENAME));

    if((CD_DIR_PTR = XP_OpenDir (CD_FILENAME, xpURL)) != NULL)
	  {
        CD_NEXT_STATE = NET_READ_DIRECTORY_CHUNK;
	  }
    else
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_OPEN_FILE,
													  CD_FILENAME);
        return(MK_UNABLE_TO_OPEN_FILE);
	  }

    CD_IS_DIR = TRUE;

    FE_GraphProgressInit(CE_WINDOW_ID, CE_URL_S, CE_URL_S->content_length);
    CD_DESTROY_GRAPH_PROGRESS = TRUE;  /* we will need to destroy it */
    CD_ORIGINAL_CONTENT_LENGTH = CE_URL_S->content_length;
    NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_READDIR));

    /* make sure the last character isn't a slash
     */
    if(CD_FILENAME[XP_STRLEN(CD_FILENAME)-1] == '/')
        CD_FILENAME[XP_STRLEN(CD_FILENAME)-1] = '\0';

    return(0);  /* ok */
}

/* read part of a file and push it out the stream
 */
PRIVATE int
net_read_file_chunk(ActiveEntry * cur_entry)
{
    FILEConData * connection_data = (FILEConData *) cur_entry->con_data;
    int count;
	int32 read_size = IS_WRITE_READY;

    /* If the stream sink doesn't want any data yet, do nothing. */
    if (read_size == 0)
      {
        CD_PAUSE_FOR_READ = TRUE;
        return NET_READ_FILE_CHUNK;
      }

	read_size = MIN(read_size, NET_Socket_Buffer_Size);

	if(CD_RANGE_LENGTH)
		read_size = MIN(read_size, CD_RANGE_LENGTH-CE_BYTES_RECEIVED);
	
    count = XP_FileRead(NET_Socket_Buffer, read_size, CD_FILE_PTR);

    if(count < 1 || (CD_RANGE_LENGTH && CE_BYTES_RECEIVED >= CD_RANGE_LENGTH))
      {
		if(CE_URL_S->real_content_length > CE_URL_S->content_length)
		  {
			/* this is a case of a partial cache file.
			 * we need to fall out of here and
			 * get the rest of the file from the
			 * network
			 */
			cur_entry->save_stream = CD_STREAM;
			CD_STREAM = NULL;
			FE_ClearFileReadSelect(CE_WINDOW_ID, XP_Fileno(CD_FILE_PTR));
            if (!CE_URL_S->load_background)
                NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_FILEDONE));
        	XP_FileClose(CD_FILE_PTR);
			CE_SOCK = SOCKET_INVALID;
        	CD_FILE_PTR = 0;
        	CD_NEXT_STATE = NET_FILE_FREE;
        	return(MK_GET_REST_OF_PARTIAL_FILE_FROM_NETWORK);
		  }
		else if(CD_BYTERANGE_STRING && *CD_BYTERANGE_STRING)
		  {
			/* go get more byte ranges */
			COMPLETE_STREAM;
			CD_NEXT_STATE = NET_SETUP_FILE_STREAM;
            if (!CE_URL_S->load_background)
                NET_Progress(CE_WINDOW_ID, XP_GetString( XP_READING_SEGMENT ) );
			return(0);
		  }

		FE_ClearFileReadSelect(CE_WINDOW_ID, XP_Fileno(CD_FILE_PTR));
        if (!CE_URL_S->load_background)
            NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_FILEDONE));
        XP_FileClose(CD_FILE_PTR);
		CE_SOCK = SOCKET_INVALID;
        CD_FILE_PTR = 0;
        CD_NEXT_STATE = NET_FILE_DONE;
        return(MK_DATA_LOADED);
      }

    CE_BYTES_RECEIVED += count;

    CE_STATUS = PUTB(NET_Socket_Buffer, count);

    if (!CE_URL_S->load_background)
        FE_GraphProgress(CE_WINDOW_ID, CE_URL_S, CE_BYTES_RECEIVED, count,
                         CE_URL_S->content_length);

    CD_PAUSE_FOR_READ = TRUE;

    return(CE_STATUS);
}

PRIVATE int
net_read_directory_chunk (ActiveEntry * cur_entry)
{

    FILEConData * connection_data = (FILEConData *) cur_entry->con_data;
    NET_FileEntryInfo * file_entry;
    XP_DirEntryStruct * dir_entry;
    XP_StatStruct       stat_entry;
    char *full_path=0;
	int32 len;

    CD_SORT_BASE = NET_SortInit();

    while((dir_entry = XP_ReadDir(CD_DIR_PTR)) != 0)
      {
	
		/* skip . and ..
		 */
		if(!XP_STRCMP(dir_entry->d_name, "..") || !XP_STRCMP(dir_entry->d_name, "."))
			continue;

        file_entry = NET_CreateFileEntryInfoStruct();
        if(!file_entry)
		  {
		    CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
            return(MK_OUT_OF_MEMORY);
		  }

		/* escape and copy
		 */
		file_entry->filename = NET_Escape(dir_entry->d_name, URL_PATH);

        /* make a full path */
		len = XP_STRLEN(CD_FILENAME) + XP_STRLEN(dir_entry->d_name) + 30;
		FREEIF(full_path);
		full_path = (char *)XP_ALLOC(len*sizeof(char));

		if(!full_path)
			return(MK_OUT_OF_MEMORY);

        XP_STRCPY(full_path, CD_FILENAME);
        XP_STRCAT(full_path, "/");
        XP_STRCAT(full_path, dir_entry->d_name);

        if(XP_Stat(full_path, &stat_entry, xpURL) != -1)
          {
            TRACEMSG(("Found stat info for file %s\n",full_path));

            if(S_ISDIR(stat_entry.st_mode))
			  {
                file_entry->special_type = NET_DIRECTORY;
				StrAllocCat(file_entry->filename, "/");
			  }
            else
			  {
                file_entry->cinfo = NET_cinfo_find_type(file_entry->filename);
			  }

            file_entry->size = (int32) stat_entry.st_size;
            file_entry->date = stat_entry.st_mtime;

            TRACEMSG(("Got file: %s, %ld",file_entry->filename, file_entry->size));

            NET_SortAdd(CD_SORT_BASE, file_entry);
          }
      }

	FREEIF(full_path);

    NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_DIRDONE));

    XP_CloseDir(CD_DIR_PTR);
    CD_DIR_PTR = 0;

    CD_NEXT_STATE = NET_BEGIN_PRINT_DIRECTORY;

    return(0); 
}

PRIVATE int
net_BeginPrintDirectory(ActiveEntry * cur_entry)
{
    FILEConData * connection_data = (FILEConData *) cur_entry->con_data;
	char out_buf[2048];

#ifdef XP_WIN
	{
		char *host = NET_ParseURL(CE_URL_S->address, GET_HOST_PART);

		/* if the host contains C: get rid of
	 	 * the c: portion in the path since it's 
	 	 * already the host portion of the URL
	 	 */
		if(XP_STRLEN(host) == 2
		   && XP_IS_ALPHA(host[0])
	   	   && (host[1] == ':' || host[1] == '|'))
			XP_MEMCPY(CD_FILENAME, CD_FILENAME+3, XP_STRLEN(CD_FILENAME+2));
	}
#endif /* XP_WIN */

#ifndef LIVEWIRE

    {
      char *s = XP_ALLOC (XP_STRLEN (CD_FILENAME) + 10); /* jwz */
      XP_STRCPY (s, CD_FILENAME);
      XP_STRCAT(s, "/");
      PR_snprintf(out_buf, sizeof(out_buf),
                  XP_GetString( XP_TITLE_DIRECTORY_LISTING ), s);
      CE_STATUS = PUTS(out_buf);
      XP_FREE(s);
    }


#ifdef DEBUG_jwz /* mostly from Kartik Subbarao <subbarao@computer.org> */
    {
      char *path = XP_STRDUP(CD_FILENAME);
      char hpath[1024];
      char *p, *prel;

      /* Generate hyperlinks for each directory in the path 
       * except for the last one. */
      for (prel=path, p=path, hpath[0]='\0'; *p; p++)
        {
          char op;
          if (*p != '/') continue;
          p++;

          op = *p;
          *p = '\0';

          XP_STRCAT(hpath, "<A HREF=\"");
          XP_STRCAT(hpath, path);
          XP_STRCAT(hpath, "\">");
          XP_STRCAT(hpath, prel);
          XP_STRCAT(hpath, "</A>");

          *p = op;
          prel = p;
        }

      /* Add on the last directory element, unhyperlinked */
      XP_STRCAT(hpath, prel);
      XP_STRCAT(hpath, "/");

      /* output "directory of ..." message */
      PR_snprintf(out_buf, sizeof(out_buf),
                  XP_GetString( XP_H1_DIRECTORY_LISTING ), hpath);

      FREEIF (path);

      if(CE_STATUS > -1)
    	CE_STATUS = PUTS(out_buf);
    }
#endif /* DEBUG_jwz */

#ifndef DEBUG_jwz
    /* allow the user to go up if CD_FILENAME is not /
     */
    if(XP_STRCMP(CD_FILENAME, "/"))
      {
        char *cp = XP_STRRCHR(CD_FILENAME, '/');
        if(cp)
          {
            PR_snprintf(out_buf, sizeof(out_buf), "<A HREF=\"");
			if(CE_STATUS > -1)
            	CE_STATUS = PUTS(out_buf);
            *cp = '\0';
			if(CE_STATUS > -1)
            	CE_STATUS = PUTS(CD_FILENAME);
            *cp = '/';
            PR_snprintf(out_buf, sizeof(out_buf), "/");
			if(CE_STATUS > -1)
            	CE_STATUS = PUTS(out_buf);
            PR_snprintf(out_buf, sizeof(out_buf),
				XP_GetString( XP_UPTO_HIGHER_LEVEL_DIRECTORY ) );
			if(CE_STATUS > -1)
            	CE_STATUS = PUTS(out_buf);
          }
      }
#endif /* !DEBUG_jwz */


#endif /* not LIVEWIRE */

	CD_NEXT_STATE = NET_PRINT_DIRECTORY;

	return(CE_STATUS);
}

#define PD_PUTS(s)  \
do { \
if(status > -1) \
	status = (*stream->put_block)(stream->data_object, s, XP_STRLEN(s)); \
} while(0)

PUBLIC int
NET_PrintDirectory(SortStruct **sort_base, NET_StreamClass * stream, char * path)
{
    NET_FileEntryInfo * file_entry;
    char out_buf[3096];
    char *date, *ptr;
	char *esc_path = NET_Escape(path, URL_PATH);
    int i;
	int status=0;
	int len;

#ifdef DEBUG_jwz
	NET_cinfo * cinfo;
    char *name;
#endif /* DEBUG_jwz */

#ifndef DEBUG_jwz
	int max_name_length;

	/* Ask layout how wide the window is in the <PRE> font, to decide
	   how much space we should allocate for the file name column. */
	{
# define LISTING_OVERHEAD 59
#ifdef MOZILLA_CLIENT
	  int window_width = LO_WindowWidthInFixedChars (stream->window_id);
#else
	  int window_width = 80;
#endif /* MOZILLA_CLIENT */

	  max_name_length = (window_width - LISTING_OVERHEAD);
	  if (max_name_length < 12) /* 8.3 */
		max_name_length = 12;
	  else if (max_name_length > 50)
		max_name_length = 50;
# undef LISTING_OVERHEAD
	}
#endif /* !DEBUG_jwz */


    NET_DoFileSort(*sort_base);

#ifdef DEBUG_jwz
        XP_STRCPY(out_buf,
                  "<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0>\n"
                  " <TR>\n"
                  "  <TD></TD>\n"
                  "  <TH NOWRAP ALIGN=LEFT><SPACER SIZE=10>Name</TH>\n"
                  "  <TH NOWRAP ALIGN=RIGHT><SPACER SIZE=10>Size</TH>\n"
                  "  <TH NOWRAP ALIGN=LEFT><SPACER SIZE=10>Date</TH>\n"
                  "  <TH NOWRAP ALIGN=LEFT><SPACER SIZE=10>Type</TH>\n"
                  " </TR>\n"
                  " <TR>\n"
                  "  <TD COLSPAN=9><HR></TD>\n"
                  " </TR>\n");
        PD_PUTS(out_buf);
#endif  /* !DEBUG_jwz */

    for(i=0; status > -1 && (file_entry = (NET_FileEntryInfo *) 
                                NET_SortRetrieveNumber(*sort_base, i)) != 0;
			 i++)
      {

#ifdef LIVEWIRE
        /* print out the listing like the server does
         * filename content-type sizeinbytes time_t_of_last_modified
         */
        XP_STRTOK(file_entry->filename, "/");
        PR_snprintf(out_buf, sizeof(out_buf), "%s %s %ld %lu"CRLF, 
                    file_entry->filename, /*NET_Escape(file_entry->filename, URL_XALPHAS),*/
                    file_entry->special_type == NET_DIRECTORY ? "directory" :
                        file_entry->cinfo ? file_entry->cinfo->type : TEXT_PLAIN,
                    file_entry->size,
                    file_entry->date);
        
        PD_PUTS(out_buf);

#else /* NOT LIVEWIRE */        

#ifdef DEBUG_jwz

        /* print the icon
         */
        XP_STRCPY (out_buf, "<TD NOWRAP VALIGN=TOP ALIGN=RIGHT>");
        PD_PUTS(out_buf);

        PR_snprintf(out_buf, sizeof(out_buf), "<A HREF=\"%s\">",
                    /* jwz: don't unescape it -- if there is a file
                       with a % in its name, we need that to stay escaped
                       to show up properly on the screen and in the href.
                     */
/*                    NET_UnEscape(file_entry->filename)*/
                    file_entry->filename
);
        PD_PUTS(out_buf);

        /* do content_type icon stuff here */
        if (file_entry->content_type)
          cinfo = NET_cinfo_find_info_by_type(file_entry->content_type);
        else
          cinfo = NET_cinfo_find_type(file_entry->filename);

        if(file_entry->special_type == NET_DIRECTORY 
           || file_entry->special_type == NET_SYM_LINK
           || file_entry->special_type == NET_SYM_LINK_TO_DIR
           || file_entry->special_type == NET_SYM_LINK_TO_FILE)
          {
            PR_snprintf(out_buf, 
                        sizeof(out_buf), 
                        "<IMG ALIGN=ABSBOTTOM "
                        "BORDER=0 SRC=\"internal-gopher-menu\">");
          }
        else if(cinfo && cinfo->icon)
          {
            PR_snprintf(out_buf, sizeof(out_buf),
                        "<IMG ALIGN=ABSBOTTOM BORDER=0 SRC=\"%.512s\">",
                        cinfo->icon);
          }
        else 
          {
            PR_snprintf(out_buf, 
                        sizeof(out_buf), 
                        "<IMG ALIGN=ABSBOTTOM BORDER=0 "
                        "SRC=\"internal-gopher-unknown\">");
          }

        /* output the line */
        XP_STRCAT (out_buf, "</A></TD>\n");
        PD_PUTS(out_buf);

    
        /* print the filename
         */
        XP_STRCPY (out_buf, "<TD NOWRAP VALIGN=TOP ALIGN=LEFT>");
        XP_STRCAT (out_buf, "<SPACER SIZE=10>");
        PD_PUTS(out_buf);

        PR_snprintf(out_buf, sizeof(out_buf), "<A HREF=\"%s\">", 
                    /* jwz: don't unescape it -- if there is a file
                       with a % in its name, we need that to stay escaped
                       to show up properly on the screen and in the href.
                     */
/*                    NET_UnEscape(file_entry->filename)*/
                    file_entry->filename
                    );
        PD_PUTS(out_buf);

        name = NET_UnEscape (file_entry->filename);
        len = XP_STRLEN (name);

        XP_STRCPY (out_buf, name);
        XP_STRCAT (out_buf, "</A>");
        XP_STRCAT (out_buf, "</TD>\n");
        PD_PUTS(out_buf);


        /* print the size
         */
        XP_STRCPY (out_buf, "<TD NOWRAP VALIGN=TOP ALIGN=RIGHT>");
        XP_STRCAT (out_buf, "<SPACER SIZE=10>");
        PD_PUTS(out_buf);

        if(file_entry->size)
          {
            PR_snprintf(out_buf, sizeof(out_buf), "%5ld%s",
                        file_entry->size >= 1024 ?
                        file_entry->size/1024 : file_entry->size,
                        file_entry->size >= 1024 ? "K" : "");
            PD_PUTS(out_buf); /* output the line */
          }

        XP_STRCPY (out_buf, "</TD>\n");
        PD_PUTS(out_buf);

        /* print the date
         */
        XP_STRCPY (out_buf, "<TD NOWRAP VALIGN=TOP ALIGN=RIGHT>");
        XP_STRCAT (out_buf, "<SPACER SIZE=10>");
        PD_PUTS(out_buf);

        if(file_entry->date)
          {
            date = ctime(&file_entry->date);
            if(date)
                date[24] = '\0';  /* strip return */
            else
                date = "";

            PD_PUTS(date);
          }

        XP_STRCPY (out_buf, "</TD>\n");
        PD_PUTS(out_buf);


        /* Print the textual description
         */
        XP_STRCPY (out_buf, "<TD NOWRAP VALIGN=TOP ALIGN=LEFT>");
        XP_STRCAT (out_buf, "<SPACER SIZE=10>");
        PD_PUTS(out_buf);

        if(file_entry->special_type == NET_SYM_LINK
           || file_entry->special_type == NET_SYM_LINK_TO_DIR
           || file_entry->special_type == NET_SYM_LINK_TO_FILE)
          {
            XP_STRCPY(out_buf, "Symbolic link");
            PD_PUTS(out_buf);
          }
        else if(file_entry->special_type == NET_DIRECTORY)
          {
            XP_STRCPY(out_buf, "Directory");
            PD_PUTS(out_buf);
          }
        else if(cinfo && cinfo->desc)
          {
            PR_snprintf(out_buf, 
                        sizeof(out_buf)-1, 
                        "%s", 
                        cinfo->desc);
            PD_PUTS(out_buf);
          }

        XP_STRCPY (out_buf, "</TD>\n");
        XP_STRCPY (out_buf, "</TR><TR>\n");
        PD_PUTS(out_buf);

#else  /* !DEBUG_jwz */

		/* it will have a date if it's a fancy file listing
		 */
		if(!file_entry->date)
		  {
            PR_snprintf(out_buf, sizeof(out_buf), " <A HREF=\"%.1024s/%.1024s\">", esc_path,
					   file_entry->filename);
       		PD_PUTS(out_buf); /* output the line */
            PR_snprintf(out_buf, sizeof(out_buf), "%.1024s</A>\n",
					   NET_UnEscape(file_entry->filename));
       		PD_PUTS(out_buf); /* output the line */
		  }
		else
		  {
            if(file_entry->date)
              {
                date = ctime(&file_entry->date);
                if(date)
                    date[24] = '\0';  /* strip return */
                else
                    date = "";
              }
            else
              {
                date = "";
              }
    
            PR_snprintf(out_buf, sizeof(out_buf), " <A HREF=\"%.1024s/%.1024s\">", esc_path,
					   file_entry->filename);
       		PD_PUTS(out_buf); /* output the line */
    
            if(file_entry->cinfo && file_entry->cinfo->icon)
              {
                PR_snprintf(out_buf, sizeof(out_buf),
						   "<IMG ALIGN=absbottom BORDER=0 SRC=\"%.512s\">",
						   file_entry->cinfo->icon);
              }
            else if(file_entry->special_type == NET_DIRECTORY ||
		    file_entry->special_type == NET_SYM_LINK)
              {
				PR_snprintf(out_buf, sizeof(out_buf), "<IMG ALIGN=absbottom "
									"BORDER=0 SRC=\"internal-gopher-menu\">");
			  }
            else 
              {
                PR_snprintf(out_buf, sizeof(out_buf), "<IMG ALIGN=absbottom BORDER=0 "
						   			"SRC=\"internal-gopher-unknown\">");
		      }
    
       		PD_PUTS(out_buf); /* output the line */
    
			/* print the filename
			 */
			{
			  char *name = NET_UnEscape (file_entry->filename);
			  len = XP_STRLEN (name);
			  if(len > max_name_length)
				{
				  XP_STRCPY (out_buf, " ");
				  XP_MEMCPY (out_buf + 1, name, max_name_length - 3);
				  XP_STRCPY (out_buf + 1 + max_name_length - 3, "...</A>");
				}
			  else
				{
				  XP_STRCPY (out_buf, " ");
				  XP_STRCAT (out_buf, name);
				  XP_STRCAT (out_buf, "</A>");
				}
       		  PD_PUTS(out_buf); /* output the line */
			
			  /* put a standard number of spaces between the name and date
			   */
			  for(ptr = out_buf; len < max_name_length; len++)
				{
				  *ptr++ = ' ';
				}
			  *ptr = '\0';
			}

			/* start from the end of out_buf
			 */
            if(file_entry->size)
              {
                 PR_snprintf(&out_buf[XP_STRLEN(out_buf)], 
						sizeof(out_buf) - XP_STRLEN(out_buf), 
						" %5ld %-5s %s ",
                        file_entry->size < 1024
							? file_entry->size
							: file_entry->size/1024,
                        file_entry->size < 1024 ? "bytes" : "Kb",
                        date);
			  }
			else
			  {
				PR_snprintf(&out_buf[XP_STRLEN(out_buf)],
						   sizeof(out_buf) - XP_STRLEN(out_buf), 
						   "             %s ", date);
			  }
TRACEMSG(("String: %s", out_buf));

       		PD_PUTS(out_buf); /* output the line */
    
            if(file_entry->special_type == NET_SYM_LINK)
		      {
                PR_snprintf(out_buf, sizeof(out_buf), "Symbolic link\n");
		      }
            else if(file_entry->special_type == NET_DIRECTORY)
		      {
                PR_snprintf(out_buf, sizeof(out_buf), "Directory\n");
		      }
            else if(file_entry->cinfo && file_entry->cinfo->desc)
		      {
                PR_snprintf(out_buf, sizeof(out_buf), "%.256s", file_entry->cinfo->desc);
                XP_STRCAT(out_buf, "\n");
		      }
		    else
		      {
                XP_STRCPY(out_buf, "\n");
		      }
		    	
		    PD_PUTS(out_buf);
		  }

#endif  /* !DEBUG_jwz */

#endif /* LIVEWIRE */
    
        NET_FreeEntryInfoStruct(file_entry);
      }



#ifndef LIVEWIRE    
# ifdef DEBUG_jwz
    XP_STRCPY(out_buf, "\n</TABLE>");
# else /* !DEBUG_jwz */
    XP_STRCPY(out_buf, "\n</PRE>");
# endif /* DEBUG_jwz */
	PD_PUTS(out_buf);
#endif /* NOT LIVEWIRE */

    NET_SortFree(*sort_base);
    *sort_base = 0;

	FREEIF(esc_path);
    if(status < 0)
        return(status);
	return(MK_DATA_LOADED);
}

/* if the url is a local file this function returns
 * the portion of the url that represents the
 * file path as a malloc'd string.  If the
 * url is not a local file url this function
 * returns NULL
 */
PRIVATE char *
net_return_local_file_part_from_url(char *address)
{
	char *host;
#ifdef XP_WIN
	char *new_address;
	char *rv;
#endif
	
	if (address == NULL)
		return NULL;

	/* mailbox url's are always local */
	if(!strncasecomp(address, "mailbox:", 8))
		return(XP_STRDUP(""));  /* there is no local path part */

	if(strncasecomp(address, "file:", 5))
		return(NULL);
	
	host = NET_ParseURL(address, GET_HOST_PART);

    if(!host || *host == '\0' || !strcasecomp(host, "LOCALHOST"))
	  {
		FREEIF(host);
		return(NET_UnEscape(NET_ParseURL(address, GET_PATH_PART)));
	  }

#ifdef XP_WIN
	/* get the address minus the search and hash data */
	new_address = NET_ParseURL(address, 
							GET_PROTOCOL_PART | GET_HOST_PART | GET_PATH_PART);
	NET_UnEscape(new_address);

	/* check for local drives as hostnames
	 */
	if(XP_STRLEN(host) == 2
		&& isalpha(host[0]) 
		&& (host[1] == '|' || host[1] == ':'))
	  {
		FREE(host);
		/* skip "file:/" */
		rv = XP_STRDUP(new_address+6);
		FREE(new_address);
		return(rv);
	  }

	if(1)
	  {
    	XP_StatStruct       stat_entry;
		/* try stating the url just in case since
		 * the local file system can
		 * have url's of the form \\prydain\dist
		 */
		if(-1 != XP_Stat(address+5, &stat_entry, xpURL))
		  {
			FREE(host);
			/* skip "file:" */
			rv = XP_STRDUP(address+5);
			FREE(new_address);
			return(rv);
		  }
	  }
#endif /* XP_WIN */

	FREE(host);

	return(NULL);
}

/* returns TRUE if the url is a local file
 * url
 */
PUBLIC XP_Bool
NET_IsLocalFileURL(char *address)
{
	char * cp = net_return_local_file_part_from_url(address);

	if(cp)
	  {
		FREE(cp);
		return(TRUE);
	  }
	else
	  {
		return(FALSE);
	  }
}

/* load local files and
 * display directories
 *
 * if cache_file is not null then we this module is being used to
 * read a localy cached url.  Use the passed in cache_file variable
 * as the location of the file and use the URL->address as the
 * URL for StreamBuilding etc.
 */
MODULE_PRIVATE int
NET_FileLoad (ActiveEntry * cur_entry)
{
	char * cp;
    FILEConData * connection_data;

    /* make space for the connection data */
    cur_entry->con_data = XP_NEW(FILEConData);
    if(!cur_entry->con_data)
      {
        FE_Alert(CE_WINDOW_ID, XP_GetString(XP_ALERT_OUTMEMORY));
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
        CE_STATUS = MK_OUT_OF_MEMORY;
        return(CE_STATUS);
      }

    /* zero out the structure 
     */
    XP_MEMSET(cur_entry->con_data, 0, sizeof(FILEConData));

	/* set this to make the CD_ macros work */
	connection_data = cur_entry->con_data;

    if(!CE_URL_S->cache_file)
      {
		char *path;

		if(!(path = net_return_local_file_part_from_url(CE_URL_S->address)))
      	  {
        	FREE(cur_entry->con_data);
        	cur_entry->con_data = 0;
        	return(MK_USE_FTP_INSTEAD); /* use ftp */
      	  }

        CD_FILENAME = path;
      }
    else
      {
        StrAllocCopy(CD_FILENAME, CE_URL_S->cache_file);
		CD_IS_CACHE_FILE = TRUE;
      }

    TRACEMSG(("Load File: looking for file: %s\n",CD_FILENAME));

	/* don't cache local files or local directory listngs */
	cur_entry->format_out = CLEAR_CACHE_BIT(cur_entry->format_out);

	/* all non-cache filenames must begin with slash
	 */
	if(!CD_IS_CACHE_FILE == TRUE && *CD_FILENAME != '/')
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(
											MK_UNABLE_TO_LOCATE_FILE,
 											*CD_FILENAME ? CD_FILENAME : "/");
		CE_STATUS = MK_UNABLE_TO_LOCATE_FILE;
        return(MK_UNABLE_TO_LOCATE_FILE);
	  }

#define BYTERANGE_TOKEN "bytes="

#ifdef URL_BYTERANGE_METHOD
	/* if the URL has ";bytes=" in it then it
	 * is a special partial byterange url.
	 * Parse out the byterange part and store it
     * away
	 */
#define URL_BYTERANGE_TOKEN ";"BYTERANGE_TOKEN
	if(CD_IS_CACHE_FILE 
		&& (cp = strcasestr(CE_URL_S->address, URL_BYTERANGE_TOKEN)))
	  {
		StrAllocCopy(CD_BYTERANGE_STRING, cp+XP_STRLEN(URL_BYTERANGE_TOKEN));
		XP_STRTOK(CD_BYTERANGE_STRING, ";");
	  }
	else if((cp = strcasestr(CD_FILENAME, URL_BYTERANGE_TOKEN)))
	  {
		*cp = '\0';
		/* remove any other weird ; stuff */
		XP_STRTOK(cp+1, ";");
		StrAllocCopy(CD_BYTERANGE_STRING, cp+XP_STRLEN(URL_BYTERANGE_TOKEN));
	  }
#endif /* URL_BYTERANGE_METHOD */

	/* this is the byterange header method. 
	 * both the URL byterange and the header
	 * byterange methods can coexist peacefully
	 */
	if(CE_URL_S->range_header && !XP_STRNCMP(CE_URL_S->range_header, BYTERANGE_TOKEN, XP_STRLEN(BYTERANGE_TOKEN)))
	  {
		StrAllocCopy(CD_BYTERANGE_STRING, CE_URL_S->range_header+XP_STRLEN(BYTERANGE_TOKEN));
	  }

    /* lets do a local file read 
     */
    cur_entry->local_file = TRUE;  /* se we don't select on the socket id */

   	CD_NEXT_STATE = NET_CHECK_FILE_TYPE;

	return(NET_ProcessFile(cur_entry));
}

MODULE_PRIVATE int
NET_ProcessFile (ActiveEntry * cur_entry)
{
    FILEConData * connection_data = (FILEConData *) cur_entry->con_data;

    TRACEMSG(("Entering ProcessFile with state #%d\n",CD_NEXT_STATE));

    CD_PAUSE_FOR_READ = FALSE;

    while(!CD_PAUSE_FOR_READ)
      {

        switch (CD_NEXT_STATE)
          {
            case NET_CHECK_FILE_TYPE:
                CE_STATUS = net_check_file_type(cur_entry);
                break;
    
			case NET_DELETE_FILE:
				CE_STATUS = net_delete_file(cur_entry);
				break;

			case NET_PUT_FILE:
				CE_STATUS = net_put_file(cur_entry);
				break;

			case NET_MOVE_FILE:
				CE_STATUS = net_move_file(cur_entry);
				break;

			case NET_MAKE_DIRECTORY:
				CE_STATUS = net_make_directory(cur_entry);
				break;

            case NET_FILE_SETUP_STREAM:
                CE_STATUS = net_file_setup_stream(cur_entry);
                break;
    
            case NET_OPEN_FILE:
                CE_STATUS = net_open_file(cur_entry);
                break;

			case NET_SETUP_FILE_STREAM:
				CE_STATUS = net_setup_file_stream(cur_entry);
				break;
    
            case NET_OPEN_DIRECTORY:
                CE_STATUS = net_open_directory(cur_entry);
                break;
    
            case NET_READ_FILE_CHUNK:
                CE_STATUS = net_read_file_chunk(cur_entry);
                break;
    
            case NET_READ_DIRECTORY_CHUNK:
                CE_STATUS = net_read_directory_chunk(cur_entry);
                break;

            case NET_BEGIN_PRINT_DIRECTORY:
				CE_STATUS = net_BeginPrintDirectory(cur_entry);
                break;
    
            case NET_PRINT_DIRECTORY:
				CE_STATUS = NET_PrintDirectory(&CD_SORT_BASE, CD_STREAM, CD_FILENAME);
				CD_NEXT_STATE = NET_FILE_DONE;
                break;
    
            case NET_FILE_DONE:
    			if(CD_STREAM)
               		COMPLETE_STREAM;
                CD_NEXT_STATE = NET_FILE_FREE;
                break;
    
            case NET_FILE_ERROR_DONE:
                if(CD_STREAM)
                    ABORT_STREAM(CE_STATUS);
                if(CD_DIR_PTR)
                    XP_CloseDir(CD_DIR_PTR);
                if(CD_FILE_PTR)
				  {
					CE_SOCK = SOCKET_INVALID;
					FE_ClearFileReadSelect(CE_WINDOW_ID, XP_Fileno(CD_FILE_PTR));
                    XP_FileClose(CD_FILE_PTR);
				  }
                CD_NEXT_STATE = NET_FILE_FREE;
                break;
    
            case NET_FILE_FREE:
               if(CD_DESTROY_GRAPH_PROGRESS)
                    FE_GraphProgressDestroy(CE_WINDOW_ID,
                                            CE_URL_S,
                                            CD_ORIGINAL_CONTENT_LENGTH,
											CE_BYTES_RECEIVED);
               FREE(CD_FILENAME);
				FREEIF(CD_STREAM);
                FREE(cur_entry->con_data);
                return(-1); /* done */
          }
    
        if(CE_STATUS < 0 && CD_NEXT_STATE != NET_FILE_FREE)
		  {
    		CD_PAUSE_FOR_READ = FALSE;
            CD_NEXT_STATE = NET_FILE_ERROR_DONE;
		  }

      }

    return(0);  /* keep going */
}

MODULE_PRIVATE int
NET_InterruptFile(ActiveEntry * cur_entry)
{
    FILEConData * connection_data = (FILEConData *)cur_entry->con_data;

    CD_NEXT_STATE = NET_FILE_ERROR_DONE;
    CE_STATUS = MK_INTERRUPTED;

    return(NET_ProcessFile(cur_entry));
}

MODULE_PRIVATE void
NET_CleanupFile(void)
{
    return;
}

typedef struct {
	HTTPIndexParserData *parse_data;
	MWContext 			*context;
	int		 			 format_out;
	URL_Struct			*URL_s;
} index_format_conv_data_object;

PRIVATE int net_IdxConvPut(index_format_conv_data_object *obj, char *s, int32 l)
{
	return(NET_HTTPIndexParserPut(obj->parse_data, s, l));
}

PRIVATE int net_IdxConvWriteReady(index_format_conv_data_object *obj)
{
	return(MAX_WRITE_READY);
}

/* What the hell is this?  This seems to be a handler
   for application/http-listing or something like that --
   which I have never seen occur in nature.  jwz, 11-Mar-99
 */
/* take the parsed data and generate HTML */
PRIVATE void net_IdxConvComplete(index_format_conv_data_object *obj)
{
	int32 num_files = NET_HTTPIndexParserGetTotalFiles(obj->parse_data);
    NET_FileEntryInfo * file_entry;
	int32  max_name_length, i, status, window_width, len;
	char   out_buf[3096], *name, *date, *ptr;
	NET_StreamClass *stream;
	NET_cinfo * cinfo;

	/* direct the stream to the html parser */
	StrAllocCopy(obj->URL_s->content_type, TEXT_HTML);

	stream = NET_StreamBuilder(obj->format_out,
								obj->URL_s,
								obj->context);
	if(!stream)
		goto cleanup;

	/* Ask layout how wide the window is in the <PRE> font, to decide
	   how much space we should allocate for the file name column. */
# define LISTING_OVERHEAD 59
#ifdef MOZILLA_CLIENT
	window_width = LO_WindowWidthInFixedChars (obj->context);
#else
	window_width = 80;
#endif /* MOZILLA_CLIENT */

	max_name_length = (window_width - LISTING_OVERHEAD);
	if (max_name_length < 12) /* 8.3 */
		max_name_length = 12;
	else if (max_name_length > 50)
		max_name_length = 50;

	if(NET_HTTPIndexParserGetHTMLMessage(obj->parse_data))
	  {
		PD_PUTS(NET_HTTPIndexParserGetHTMLMessage(obj->parse_data));
	  }

    PR_snprintf(out_buf, sizeof(out_buf), "<PRE>\n");
	PD_PUTS(out_buf); /* output the line */

	if(NET_HTTPIndexParserGetTextMessage(obj->parse_data))
	  {
		PD_PUTS(NET_HTTPIndexParserGetTextMessage(obj->parse_data));
	  }

    for(i=0; i < num_files; i++)
      {

		file_entry = NET_HTTPIndexParserGetFileNum(obj->parse_data, i);

		if(!file_entry)
			continue;

        if(!file_entry->date)
          {
            PR_snprintf(out_buf, sizeof(out_buf), " <A HREF=\"%s\">",
                       file_entry->filename);
            PD_PUTS(out_buf); /* output the line */

            PR_snprintf(out_buf, sizeof(out_buf), "%s</A>\n",
                       NET_UnEscape(file_entry->filename));
            PD_PUTS(out_buf); /* output the line */
          }
		else
		  {
		
            date = ctime(&file_entry->date);
            if(date)
                date[24] = '\0';  /* strip return */
            else
                date = "";
    
            PR_snprintf(out_buf, sizeof(out_buf), " <A HREF=\"%s\">", 
					   file_entry->filename);
       		PD_PUTS(out_buf); /* output the line */
    
			/* do content_type icon stuff here */
			cinfo = NET_cinfo_find_info_by_type(file_entry->content_type);

            if(file_entry->special_type == NET_DIRECTORY 
		    		|| file_entry->special_type == NET_SYM_LINK
		    		|| file_entry->special_type == NET_SYM_LINK_TO_DIR
		    		|| file_entry->special_type == NET_SYM_LINK_TO_FILE)
              {
				PR_snprintf(out_buf, 
							sizeof(out_buf), 
							"<IMG ALIGN=absbottom "
							"BORDER=0 SRC=\"internal-gopher-menu\">");
			  }
            else if(cinfo && cinfo->icon)
              {
                PR_snprintf(out_buf, sizeof(out_buf),
						   "<IMG ALIGN=absbottom BORDER=0 SRC=\"%.512s\">",
						   cinfo->icon);
              }
            else 
              {
                PR_snprintf(out_buf, 
							sizeof(out_buf), 
							"<IMG ALIGN=absbottom BORDER=0 "
						   	"SRC=\"internal-gopher-unknown\">");
		      }

       		PD_PUTS(out_buf); /* output the line */
    
			/* print the filename
			 */
			name = NET_UnEscape (file_entry->filename);
			len = XP_STRLEN (name);
			if(len > max_name_length)
			  {
				  XP_STRCPY (out_buf, " ");
				  XP_MEMCPY (out_buf + 1, name, max_name_length - 3);
				  XP_STRCPY (out_buf + 1 + max_name_length - 3, "...</A>");
				}
			  else
				{
				  XP_STRCPY (out_buf, " ");
				  XP_STRCAT (out_buf, name);
				  XP_STRCAT (out_buf, "</A>");
				}
       		PD_PUTS(out_buf); /* output the line */
			
			/* put a standard number of spaces between the name and date
			 */
			for(ptr = out_buf; len < max_name_length; len++)
			  {
				*ptr++ = ' ';
			  }
			*ptr = '\0';

			/* start from the end of out_buf
			 */
            if(file_entry->size)
              {
                 PR_snprintf(&out_buf[XP_STRLEN(out_buf)], 
						sizeof(out_buf) - XP_STRLEN(out_buf), 
						" %5ld %-5s %s ",
                        file_entry->size < 1024
							? file_entry->size
							: file_entry->size/1024,
                        file_entry->size < 1024 ? "bytes" : "Kb",
                        date);
			  }
			else
			  {
				PR_snprintf(&out_buf[XP_STRLEN(out_buf)],
						   sizeof(out_buf) - XP_STRLEN(out_buf), 
						   "             %s ", date);
			  }

			PD_PUTS(out_buf); /* output the line */
    
            if(file_entry->special_type == NET_SYM_LINK
            	|| file_entry->special_type == NET_SYM_LINK_TO_DIR
            	|| file_entry->special_type == NET_SYM_LINK_TO_FILE)
		      {
                PR_snprintf(out_buf, sizeof(out_buf), "Symbolic link\n");
		      }
            else if(file_entry->special_type == NET_DIRECTORY)
		      {
                PR_snprintf(out_buf, sizeof(out_buf), "Directory\n");
		      }
            else if(cinfo && cinfo->desc)
		      {
                PR_snprintf(out_buf, 
							sizeof(out_buf)-1, 
							"%s", 
							cinfo->desc);
                XP_STRCAT(out_buf, "\n");
		      }
		    else
		      {
                XP_STRCPY(out_buf, "\n");
		      }
		    	
		    PD_PUTS(out_buf);
		  }
      }

cleanup:
	NET_HTTPIndexParserFree(obj->parse_data);

}

PRIVATE void net_IdxConvAbort(index_format_conv_data_object *obj, int32 status)
{
	NET_HTTPIndexParserFree(obj->parse_data);
}

PUBLIC NET_StreamClass *
NET_HTTPIndexFormatToHTMLConverter(int         format_out,
                               void       *data_object,
                               URL_Struct *URL_s,
                               MWContext  *window_id)
{
	index_format_conv_data_object* obj;
    NET_StreamClass* stream;

    TRACEMSG(("Setting up display stream. Have URL: %s\n", URL_s->address));

	stream = XP_NEW(NET_StreamClass);
    if(stream == NULL)
        return(NULL);

    obj = XP_NEW(index_format_conv_data_object);
    if (obj == NULL)
	  {
		FREE(stream);
        return(NULL);
	  }

    XP_MEMSET(obj, 0, sizeof(index_format_conv_data_object));

	obj->parse_data = NET_HTTPIndexParserInit();

	if(!obj->parse_data)
	  {
		FREE(stream);
		FREE(obj);
	  }

	obj->context = window_id;
	obj->URL_s = URL_s;
	obj->format_out = format_out;

    stream->name           = "HTTP index format converter";
    stream->complete       = (MKStreamCompleteFunc) net_IdxConvComplete;
    stream->abort          = (MKStreamAbortFunc) net_IdxConvAbort;
    stream->put_block      = (MKStreamWriteFunc) net_IdxConvPut;
    stream->is_write_ready = (MKStreamWriteReadyFunc) net_IdxConvWriteReady;
    stream->data_object    = obj;  /* document info object */
    stream->window_id      = window_id;

	return(stream);
}

#include "libmocha.h"

#define SANE_BUFLEN	1024

#ifdef MOCHA /* added by jwz */
NET_StreamClass *
net_CloneWysiwygLocalFile(MWContext *window_id, URL_Struct *URL_s,
						  uint32 nbytes)
{
	char *filename;
	XP_File fromfp;
	NET_StreamClass *stream;
	int32 buflen, len;
	char *buf;

	filename = net_return_local_file_part_from_url(URL_s->address);
	if (!filename)
		return NULL;
	fromfp = XP_FileOpen(filename, xpURL, XP_FILE_READ_BIN);
	FREE(filename);
	if (!fromfp)
		return NULL;
	stream = LM_WysiwygCacheConverter(window_id, URL_s, NULL);
	if (!stream)
	  {
		XP_FileClose(fromfp);
		return 0;
	  }
	buflen = stream->is_write_ready(stream->data_object);
	if (buflen > SANE_BUFLEN)
		buflen = SANE_BUFLEN;
	buf = (char *)XP_ALLOC(buflen * sizeof(char));
	if (!buf)
	  {
		XP_FileClose(fromfp);
		return 0;
	  }
	while (nbytes != 0)
	  {
		len = buflen;
		if ((uint32)len > nbytes)
			len = (int32)nbytes;
		len = XP_FileRead(buf, len, fromfp);
		if (len <= 0)
			break;
		if (stream->put_block(stream->data_object, buf, len) < 0)
			break;
		nbytes -= len;
	  }
	FREE(buf);
	XP_FileClose(fromfp);
	if (nbytes != 0)
	  {
		/* NB: Our caller must clear top_state->mocha_write_stream. */
		stream->abort(stream->data_object, MK_UNABLE_TO_CONVERT);
		XP_DELETE(stream);
		return 0;
	  }
	return stream;
}
#endif /* MOCHA -- added by jwz */
