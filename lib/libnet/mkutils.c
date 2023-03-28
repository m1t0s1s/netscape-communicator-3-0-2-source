/* -*- Mode: C; tab-width: 4 -*- */
#include "mkutils.h"
#include "gui.h"
#include "mknews.h"
#include "mkparse.h"
#include "mkaccess.h"
#include "libi18n.h"
#include "msgcom.h"
#include "mkcache.h"
#include "mkextcac.h"
#include "mime.h"
#include "secrng.h"
#include "ssl.h"

#include "xp_error.h"


extern int MK_OUT_OF_MEMORY;
extern int MK_UNABLE_TO_LOCATE_FILE;
extern int XP_ERRNO_EWOULDBLOCK;
extern int MK_TCP_WRITE_ERROR;
extern int XP_ERRNO_EALREADY;


/*
 * the trace flag
 */
#ifdef MARK
PUBLIC int MKLib_trace_flag=1;
#else
PUBLIC int MKLib_trace_flag=0;
#endif

/* counter for the CallNetlibAlltheTime call */
PUBLIC int net_call_all_the_time_count=0;

/* print network progress to the front end
 */
MODULE_PRIVATE void
NET_Progress(MWContext *context, char *msg)
{
	FE_Progress(context, msg);
}

PUBLIC char **
NET_AssembleAllFilesInDirectory(MWContext *context, char * local_dir_name)
{
	XP_Dir dir_ptr;
	XP_DirEntryStruct *dir_entry;
    XP_StatStruct stat_entry;
	char **files_to_post;
	char *file_to_post = 0;
	int32 i, cur_array_size;
    int end;
    Bool have_slash;
#define	INITIAL_ARRAY_SIZE 10
#define EXPAND_ARRAY_BY 5

	XP_ASSERT(local_dir_name);

	if(NULL == (dir_ptr = XP_OpenDir(local_dir_name, xpFileToPost)))
	  {
	  	FE_Alert(context, "Unable to open local directory");
	  	return NULL;
	  }

    /* make sure local_dir_name doesn't have a slash at the end */
    end = XP_STRLEN(local_dir_name)-1;
    have_slash = (local_dir_name[end] == '/') || (local_dir_name[end] == '\\');

	files_to_post = (char**) XP_ALLOC(INITIAL_ARRAY_SIZE * sizeof(char*));
	if(!files_to_post)
		return NULL;
	cur_array_size = INITIAL_ARRAY_SIZE;

    i=0;
	while((dir_entry = XP_ReadDir(dir_ptr)))
	  {
	  	/* skip . and .. */
		if(!XP_STRCMP(dir_entry->d_name, ".") || !XP_STRCMP(dir_entry->d_name, ".."))
			continue;
        
        /* assemble full pathname first so we can test if its a directory */
        file_to_post = XP_STRDUP(local_dir_name);
        if ( ! file_to_post ){
    		return NULL;
        }

        /* Append slash to directory if we don't already have one */
        if( !have_slash )
          {
#ifdef XP_WIN
            StrAllocCat(file_to_post, "\\");
#else
            StrAllocCat(file_to_post, "/");
#endif
          }
        if ( ! file_to_post )
          {
    		return NULL;
          }

		StrAllocCat(file_to_post, dir_entry->d_name);
        if ( ! file_to_post )
          {
    		return NULL;
          }

        /* skip over subdirectory names */
        if(-1 != XP_Stat(file_to_post, &stat_entry, xpFileToPost) && S_ISDIR(stat_entry.st_mode) )
          {
            XP_FREE(file_to_post);
            continue;
          }

		/* expand array if necessary 
		 * always leave room for the NULL terminator */
		if(i >= cur_array_size-1)
		  {
		  	files_to_post = (char**) XP_REALLOC(files_to_post, (cur_array_size + EXPAND_ARRAY_BY) * sizeof(char*)); 
			if(!files_to_post)
				return NULL;
			cur_array_size += EXPAND_ARRAY_BY;
		  }

        files_to_post[i++] = XP_STRDUP(file_to_post);

        XP_FREE(file_to_post);
       }

	/* NULL terminate the array, space is guarenteed above */
	files_to_post[i] = NULL;

	return(files_to_post);
}


/* upload a set of local files (array of char*)
 * all files must have full path name
 * first file is primary html document,
 * others are included images or other files
 * in the same directory as main file
 */
PUBLIC void
NET_PublishFiles(MWContext *context, 
                 char **files_to_publish,
                 char *remote_directory)
{
	URL_Struct *URL_s;	   

	XP_ASSERT(context && files_to_publish && remote_directory);
	if(!context || !files_to_publish || !*files_to_publish || !remote_directory)
		return;

	/* create a URL struct */
	URL_s = NET_CreateURLStruct(remote_directory, NET_SUPER_RELOAD);
	if(!URL_s)
      {    
        FE_Alert(context, "Error: not enough memory for file upload");
	  	return;  /* does not exist */
      }

	FREE_AND_CLEAR(URL_s->content_type);

	/* add the files that we are posting and set the method to POST */
	URL_s->files_to_post = files_to_publish;
	URL_s->method = URL_POST_METHOD;
    
	/* start the load */
	FE_GetURL(context, URL_s);
}

/* assemble username, password, and ftp:// or http:// URL into
 * ftp://user:password@/blah  format for uploading
*/
PUBLIC Bool
NET_MakeUploadURL( char **full_location, char *location, 
                   char *user_name, char *password )
{
    char *start;
    char *pSrc;
    char *destination;
    char *at_ptr;
    int iSize;

    if( !full_location || !location ) return FALSE;
    if( *full_location ) XP_FREE(*full_location);

    iSize = strlen(location) + 4;
    if( user_name ) iSize += strlen(user_name);
    if( password ) iSize += strlen(password);

    *full_location = (char*)XP_ALLOC(iSize);
    if( !*full_location ){
        /* Return an empty string */
        *full_location = strdup("");
        return FALSE;
    }
    **full_location = '\0';

    /* Find start just past http:// or ftp:// */
    start = XP_STRSTR(location, "//");
    if( !start ) return FALSE;

    /* Point to just past the host part */
    start += 2;
    pSrc = location;
    destination = *full_location;
    /* Copy up to and including "//" */
    while( pSrc < start ) *destination++ = *pSrc++;
    *destination = '\0';

    /* Skip over any user:password in supplied location */
    at_ptr = XP_STRCHR(start, '@');
    if( at_ptr ){
        start = at_ptr + 1;
    }
    /* Append supplied "user:password@"
     * (This can be used without password)
    */
    if( user_name && XP_STRLEN(user_name) > 0 ){
        XP_STRCAT(*full_location, user_name);
        if ( password  && XP_STRLEN(password) > 0 ){
            XP_STRCAT(*full_location,":");
            XP_STRCAT(*full_location, password);
        }
        XP_STRCAT(*full_location, "@");
    }
    /* Append the rest of location */
    XP_STRCAT(*full_location, start);

    return TRUE;
}

/* extract the username, password, and reassembled location string 
 * from an ftp:// or http:// URL
*/
PUBLIC Bool
NET_ParseUploadURL( char *full_location, char **location, 
                    char **user_name, char **password )
{
    char *start;
    char *skip_dest;
    char *at_ptr;
    char *colon_ptr;
    char at;
    char colon;

    if( !full_location || !location ) return FALSE;

    /* Empty exitisting strings... */   
    if(*location) XP_FREE(*location);
    if( user_name && *user_name) XP_FREE(*user_name);
    if( password && *password) XP_FREE(*password);

    /* Find start just past http:// or ftp://  */
    start = XP_STRSTR(full_location, "//");
    if( !start ) return FALSE;

    /* Point to just past the host part */
    start += 2;
    
    /* Start by simply copying full location
     * (may waste some bytes, but much simpler!)
    */
    *location = XP_STRDUP(full_location);
    if( !*location ) return FALSE;

    /* Destination to append location without
     *  user:password is same place copied string
    */
    skip_dest = *location + (start - full_location);

    /* Skip over any user:password in supplied location
     *  while copying those to other strings
    */
    at_ptr = XP_STRCHR(start, '@');
    colon_ptr = XP_STRCHR(start, ':');

    if( at_ptr ){
        /* save character */
        at = *at_ptr;

        /* Copy part past the @ over previously-copied full string */
        XP_STRCPY(skip_dest, (at_ptr + 1));
        
        /* Terminate for the password (or user) string */
        *at_ptr = '\0';
        if( colon_ptr ){
            /* save character */
            colon = *colon_ptr;

            if( password ){
                *password = XP_STRDUP((colon_ptr+1));
            }
            /* terminate for the user string */
            *colon_ptr = '\0';
        } else if( password ) {
            *password = XP_STRDUP("");
        }
        if( user_name ){
            *user_name = XP_STRDUP(start);
        }
        /* restore characters */
        *at_ptr = at;
        if( colon_ptr ) *colon_ptr = colon;

    } else {
        /* Supply empty strings for these */
        if( user_name ){
            *user_name = XP_STRDUP("");
        }
        if( password ){
            *password = XP_STRDUP("");
        }
    }
    return TRUE;
}


/* returns a malloc'd string containing a unique id 
 * generated from the sec random stuff.
 */
PUBLIC char * 
NET_GetUniqueIdString(void)
{
#define BYTES_OF_RANDOMNESS 20
	char rand_buf[BYTES_OF_RANDOMNESS+10]; 
	char *rv=0;


#ifdef HAVE_SECURITY /* added by jwz */
	RNG_GenerateGlobalRandomBytes(rand_buf, BYTES_OF_RANDOMNESS);
#else
	{
	  int i;
	  for (i = 0; i < BYTES_OF_RANDOMNESS; i++)
		rand_buf[i] = random();
	}
#endif /* !HAVE_SECURITY -- added by jwz */

	/* now uuencode it so it goes across the wire right */
	rv = (char *) XP_ALLOC((BYTES_OF_RANDOMNESS * (4/3)) + 10);
 	
	if(rv)
		NET_UUEncode((unsigned char *)rand_buf, (unsigned char *)rv, BYTES_OF_RANDOMNESS);

	return(rv);

#undef BYTES_OF_RANDOMNESS
}


/* The magic set of 64 chars in the uuencoded data */
PRIVATE unsigned char uuset[] = {
'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','+'
,'/' };

MODULE_PRIVATE int
NET_UUEncode(unsigned char *src, unsigned char *dst, int srclen)
{
   int  i, r;
   unsigned char *p;

/* To uuencode, we snip 8 bits from 3 bytes and store them as
6 bits in 4 bytes.   6*4 == 8*3 (get it?) and 6 bits per byte
yields nice clean bytes

It goes like this:
    AAAAAAAA BBBBBBBB CCCCCCCC
turns into the standard set of uuencode ascii chars indexed by numbers:
    00AAAAAA 00AABBBB 00BBBBCC 00CCCCCC

Snip-n-shift, snip-n-shift, etc....

*/

   for (p=dst,i=0; i < srclen; i += 3) {
        /* Do 3 bytes of src */
        register char b0, b1, b2;

        b0 = src[0];
        if (i==srclen-1)
            b1 = b2 = '\0';
        else if (i==srclen-2) {
            b1 = src[1];
            b2 = '\0';
        }
        else {
            b1 = src[1];
            b2 = src[2];
        }

        *p++ = uuset[b0>>2];
        *p++ = uuset[(((b0 & 0x03) << 4) | ((b1 & 0xf0) >> 4))];
        *p++ = uuset[(((b1 & 0x0f) << 2) | ((b2 & 0xc0) >> 6))];
        *p++ = uuset[b2 & 0x3f];
        src += 3;
   }
   *p = 0;      /* terminate the string */
   r = (unsigned char *)p - (unsigned char *)dst;       /* remember how many we
did */

   /* Always do 4-for-3, but if not round threesome, have to go
      clean up the last extra bytes */

   for( ; i != srclen; i--)
        *--p = '=';

   return r;
}


PUBLIC char *
NET_RemoveQuotes(char * string)
{
	char *rv;
	char *end;

	rv = XP_StripLine(string);

	if(*rv == '"' || *rv == '\'') 
		rv++;

	end = &rv[XP_STRLEN(rv)-1];

	if(*end == '"' || *end == '\'')
		*end = '\0';

	return(rv);
}

#define POST_DATA_BUFFER_SIZE 2048

struct WritePostDataData {
	char    *buffer;
	XP_Bool  last_line_was_complete;
	int32    amt_in_buffer;
	int32    amt_sent;
	int32    total_amt_sent;
	int32    file_size;
	XP_File  fp;
};

PRIVATE void
net_free_write_post_data_object(struct WritePostDataData *obj)
{
	XP_ASSERT(obj);

	if(!obj)
		return;

	FREEIF(obj->buffer);
	FREE(obj);
}

/* this function is called repeatably to write
 * the post data body onto the network.
 *
 * Returns negative on fatal error
 * Returns zero when done
 * Returns positive if not yet completed
 */
PUBLIC int
NET_WritePostData(MWContext  *context,
				  URL_Struct *URL_s,
				  NETSOCK     sock,
				  void      **write_post_data_data,
				  XP_Bool     add_crlf_to_line_endings)
{
	struct WritePostDataData *data_obj = (struct WritePostDataData *) 
														*write_post_data_data;

	if(!data_obj)
	  {
		data_obj = XP_NEW(struct WritePostDataData);
		*write_post_data_data = data_obj;

		if(!data_obj)
			return(MK_OUT_OF_MEMORY);

		XP_MEMSET(data_obj, 0, sizeof(struct WritePostDataData));

		data_obj->last_line_was_complete = TRUE;

		data_obj->buffer = (char *) XP_ALLOC(POST_DATA_BUFFER_SIZE);

		if(!data_obj->buffer)
		  {
			net_free_write_post_data_object(data_obj);
			return(0);
		  }
	  }

    if(!data_obj->fp && URL_s->post_data_is_file)
      {
	  	XP_StatStruct stat_entry;

	    /* send the headers if there are any
         */
        if(URL_s->post_headers)
          {
            int status = (int) NET_BlockingWrite(sock,
                                             URL_s->post_headers,
                                             XP_STRLEN(URL_s->post_headers));

			TRACEMSG(("Tx: %s", URL_s->post_headers));

			if(status < 0)
			  {
	    	    int err = SOCKET_ERRNO;
            
                if(err == XP_ERRNO_EWOULDBLOCK
					|| err == XP_ERRNO_EALREADY)
	                return(1); /* continue */

	            URL_s->error_msg = NET_ExplainErrorDetails(MK_TCP_WRITE_ERROR, err);

				net_free_write_post_data_object(data_obj);
				*write_post_data_data = NULL;

	            return(MK_TCP_WRITE_ERROR);
	          }
		  }
		/* stat the file to get the size
		 */
		if(-1 != XP_Stat(URL_s->post_data, &stat_entry, xpFileToPost))
		  {
		  	data_obj->file_size = stat_entry.st_size;
		  }
		  	
        /* open the post data file
         */
        data_obj->fp = XP_FileOpen(URL_s->post_data, xpFileToPost, XP_FILE_READ_BIN);

        if(!data_obj->fp)
          {
            URL_s->error_msg = NET_ExplainErrorDetails(
                                                    MK_UNABLE_TO_LOCATE_FILE,
                                                    URL_s->post_data);
			net_free_write_post_data_object(data_obj);
			*write_post_data_data = NULL;
            return(MK_UNABLE_TO_LOCATE_FILE);
          }
      }

    if(URL_s->post_data_is_file)
      {
        int32 amt_to_wrt;
        int32 amt_wrt;

		int type = NET_URL_Type (URL_s->address);
		XP_Bool quote_lines_p = (type == MAILTO_TYPE_URL ||
								 type == NEWS_TYPE_URL ||
								 type == INTERNAL_NEWS_TYPE_URL);

        /* do file based operations
         */
        if(data_obj->amt_in_buffer < 1 || data_obj->amt_sent >= data_obj->amt_in_buffer)
          {
            /* read more data from the file
             */
			if (quote_lines_p || add_crlf_to_line_endings)
			  {
				char *line;
				char *b = data_obj->buffer;
				int32 bsize = POST_DATA_BUFFER_SIZE;
				data_obj->amt_in_buffer =  0;
				do {
				  int L;

				  line = XP_FileReadLine(b, bsize-5, data_obj->fp);

				  if (!line)
					break;

				  L = XP_STRLEN(line);

				  /* escape periods only if quote_lines_p is set
				   */
				  if (quote_lines_p &&
					  data_obj->last_line_was_complete && line[0] == '.')
					{
					  /* This line begins with "." so we need to quote it
						 by adding another "." to the beginning of the line.
					   */
					  int i;
					  line[L+1] = 0;
					  for (i = L; i > 0; i--)
						line[i] = line[i-1];
					  L++;
					}

				  /* set default */
				  data_obj->last_line_was_complete = TRUE;

				  if (L > 1 && line[L-2] == CR && line[L-1] == LF)
				 	{
						/* already ok */
					}
				  else if(L > 0 && (line[L-1] == LF || line[L-1] == CR))
					{
					  /* only add the crlf if required
					   * we still need to do all the
					   * if comparisons here to know
					   * if the line was complete
					   */
					  if(add_crlf_to_line_endings)
					 	{
					  	  /* Change newline to CRLF. */
					  	  L--;
					  	  line[L++] = CR;
					  	  line[L++] = LF;
					  	  line[L] = 0;
						}
					}
				  else
				    {
					  data_obj->last_line_was_complete = FALSE;
					}

				  bsize -= L;
				  b += L;
				  data_obj->amt_in_buffer += L;
				} while (line && bsize > 100);
			  }
			else
			  {
				data_obj->amt_in_buffer = XP_FileRead(data_obj->buffer,
											   POST_DATA_BUFFER_SIZE-1,
											   data_obj->fp);
			  }

            if(data_obj->amt_in_buffer < 1)
              {
                /* end of file, all done
                 */
				XP_FileClose(data_obj->fp);
				XP_ASSERT(data_obj->total_amt_sent >= data_obj->file_size);
				net_free_write_post_data_object(data_obj);
				*write_post_data_data = NULL;
                return(0);
              }

            data_obj->amt_sent = 0;
          }

        amt_to_wrt = data_obj->amt_in_buffer-data_obj->amt_sent;

        /* write some data to the socket
         */
        amt_wrt = NETWRITE(sock,
                           data_obj->buffer+data_obj->amt_sent,
                           amt_to_wrt);

#if defined(JAVA)
#ifdef DEBUG
		NET_NTrace("Tx: ", 4);
		NET_NTrace(data_obj->buffer+data_obj->amt_sent,
                   amt_to_wrt);
#endif /* DEBUG */
#endif /* JAVA */

        if(amt_wrt < 0)
          {
           int err = SOCKET_ERRNO;

            if(err == XP_ERRNO_EWOULDBLOCK
				|| err == XP_ERRNO_EALREADY)
                return(1); /* continue */

            URL_s->error_msg = NET_ExplainErrorDetails(MK_TCP_WRITE_ERROR, err);

			XP_FileClose(data_obj->fp);

			net_free_write_post_data_object(data_obj);
			*write_post_data_data = NULL;

            return(MK_TCP_WRITE_ERROR);
          }

#if defined(XP_UNIX) && defined(DEBUG)
        if(amt_wrt < amt_to_wrt)
           fprintf(stderr, "Fwrite wrote less than requested!\n");
#endif

        /* safty for broken SSL_write */
        if(amt_wrt > amt_to_wrt)
            amt_wrt = amt_to_wrt;

#if defined(XP_UNIX) && defined(DEBUG)
    if(MKLib_trace_flag)
      {
        fwrite("Tx: ", 1, 4, stderr);
		fwrite(data_obj->buffer+data_obj->amt_sent, 1, amt_wrt, stderr);
        fwrite("\n", 1, 1, stderr);
      }
#endif

        data_obj->amt_sent += amt_wrt;
		data_obj->total_amt_sent += amt_wrt;

		/* return the amount of data written
		 * so that callers can provide progress
		 * if necessary.  If the amt_wrt was
		 * zero (I don't think it can ever happen)
		 * return 1 because zero means all done
		 */
		if(amt_wrt > 0)
			return(amt_wrt);
		else
			return(1);
      }
    else
      {
        /* do memory based operations
         *
         * This should be broken up to use
         * non-blocking calls, but since
         * we arn't going to use this much,
         * I'm going to leave it the way it is.
         */
		int status;

	    /* send the headers if there are any
         */
        if(URL_s->post_headers)
          {
            status = (int) NET_BlockingWrite(sock,
                                             URL_s->post_headers,
                                             XP_STRLEN(URL_s->post_headers));

			TRACEMSG(("Tx: %s", URL_s->post_headers));

            if(status < 0)
			  {
				net_free_write_post_data_object(data_obj);
				*write_post_data_data = NULL;
                return(status);
			  }
          }
   
        XP_STRCPY(data_obj->buffer, CRLF);
        TRACEMSG(("Adding %s to command", data_obj->buffer));
        status = (int) NET_BlockingWrite(sock, 
										 data_obj->buffer, 
										 XP_STRLEN(data_obj->buffer));

        if(status < 0)
		  {
			net_free_write_post_data_object(data_obj);
			*write_post_data_data = NULL;
            return(status);
		  }

        if(!URL_s->post_data)
		  {
			net_free_write_post_data_object(data_obj);
			*write_post_data_data = NULL;
            return(MK_OUT_OF_MEMORY);
		  }

        status = (int) NET_BlockingWrite(sock,
                                      	 URL_s->post_data,
                                      	 URL_s->post_data_size);

#if defined(XP_UNIX) && defined(DEBUG)
    if(MKLib_trace_flag)
      {
        fwrite("Tx: ", 1, 4, stderr);
		fwrite(URL_s->post_data, 1, URL_s->post_data_size, stderr);
        fwrite("\n", 1, 1, stderr);
      }
#endif
		if(status > -1)
		  {
		    return(0);
		  }
		else
		  {
			net_free_write_post_data_object(data_obj);
			*write_post_data_data = NULL;
    		return(status);
		  }

      }

	XP_ASSERT(0);
	net_free_write_post_data_object(data_obj);
	*write_post_data_data = NULL;
	return(-1); /* shouldn't ever get here */

}



/* parse a mime header.
 * 
 * Both name and value strings will be modified during
 * parsing.  If you need to retain your strings make a
 * copy before passing them in.
 *
 * Values will be placed in the URL struct.
 *
 * returns TRUE if it found a valid header
 * and FALSE if it didn't
 */
PUBLIC Bool
NET_ParseMimeHeader(MWContext  *context, 
				    URL_Struct *URL_s, 
					char       *name, 
					char       *value)
{
	Bool  found_one = FALSE;
	Bool  ret_value = FALSE;
	char  empty_string[2];
	int   doc_csid;
	char  *colon_ptr;

	if(!name || !URL_s)
		return(FALSE);

	name = XP_StripLine(name);	

	if(value)
	  {
		value = XP_StripLine(value);	
	  }
	else
	  {
		*empty_string = '\0';
		value = empty_string;
	  }

    colon_ptr = XP_STRCHR(name, ':');
    if (colon_ptr) {
        *colon_ptr = '\0';
        }
    ret_value = NET_AddToAllHeaders(URL_s, name, value);
    if (colon_ptr) {
        *colon_ptr = ':';
        }
    if (!ret_value) {
        return(FALSE);
        }

	switch(toupper(*name))
	  {
        case 'A':
            if(!strncasecomp(name,"ACCEPT-RANGES:",14)
				|| !strncasecomp(name, "ALLOW-RANGES:",13))
              {
                char * next_arg = XP_STRTOK(value, ";");

                found_one = TRUE;

                while(next_arg)
                  {
                    next_arg = XP_StripLine(next_arg);

                    if(!strncasecomp(next_arg,"BYTES", 5))
                      {
                        TRACEMSG(("Document Allows for BYTERANGES!"));
						URL_s->server_can_do_byteranges = TRUE;
                      }

                    next_arg = XP_STRTOK(NULL, ";");
                  }
              }
        case 'C':
            if(!strncasecomp(name,"CONTENT-DISPOSITION:",20))
			  {
			    char *next_arg;

                found_one = TRUE;

                next_arg = XP_STRTOK(value, ";");

				while(next_arg)
				  {  
                    next_arg = XP_StripLine(next_arg);
				
                    if(!strncasecomp(next_arg,"filename=", 9))
					  {
						StrAllocCopy(URL_s->content_name,
                                     NET_RemoveQuotes(next_arg+9));
					  }
                    next_arg = XP_STRTOK(NULL, ";");
                  }
	
			  }
            else if(!strncasecomp(name,"CONTENT-TYPE:",13))
              {
                char *first_arg, *next_arg;

				found_one = TRUE;

                first_arg = XP_STRTOK(value, ";");

                StrAllocCopy(URL_s->content_type, XP_StripLine(value))
;
                TRACEMSG(("Found content_type: %s",URL_s->content_type));
   
                /* assign and compare
                 *
                 * search for charset
                 * and boundry
                 */
                while((next_arg = XP_STRTOK(NULL, ";")))
                  {
                    next_arg = XP_StripLine(next_arg);

                    if(!strncasecomp(next_arg,"CHARSET=", 8))
                      {
#ifdef MOZILLA_CLIENT
                        StrAllocCopy(URL_s->charset,
                                     NET_RemoveQuotes(next_arg+8));
						doc_csid = INTL_CharSetNameToID(URL_s->charset);
						if (doc_csid == CS_UNKNOWN)
						{
							doc_csid = CS_DEFAULT;
						}

						if (context->relayout == METACHARSET_NONE)
						{
							context->relayout = METACHARSET_HASCHARSET;
							if ((context->doc_csid & ~CS_AUTO) != doc_csid)
							{
								context->doc_csid = doc_csid;
								context->relayout = METACHARSET_REQUESTRELAYOUT;
							}
						}

                        TRACEMSG(("Found charset: %s", URL_s->charset));
#else
						XP_ASSERT(0);
#endif /* MOZILLA_CLIENT */
                      }
                    else if(!strncasecomp(next_arg,"BOUNDARY=", 9))
                      {
                        StrAllocCopy(URL_s->boundary,
                                     NET_RemoveQuotes(next_arg+9));
                        TRACEMSG(("Found boundary: %s", URL_s->boundary));
                      }
                    else if(!strncasecomp(next_arg,"AUTOSCROLL", 10))
                      {

#define DEFAULT_AUTO_SCROLL_BUFFER 100
						if(*next_arg+10 == '=')
							URL_s->auto_scroll = 
										atoi(NET_RemoveQuotes(next_arg+11));

						if(!URL_s->auto_scroll)
                        	URL_s->auto_scroll = DEFAULT_AUTO_SCROLL_BUFFER;

                        TRACEMSG(("Found autoscroll attr.: %ud", 
								  URL_s->auto_scroll));
                      }
                  }
              }
            else if(!strncasecomp(name,"CONTENT-LENGTH:",15))
              {
				found_one = TRUE;

                XP_STRTOK(value, ";"); /* terminate at ';' */
				
				/* don't reset the content-length if
				 * we have already set it from the content-range
				 * header.  If high_range is set then we must
				 * have seen a content-range header.
				 */
				if(!URL_s->high_range)
                	URL_s->content_length = atol(value);
              }
            else if(!strncasecomp(name,"CONTENT-ENCODING:",17))
              {
				found_one = TRUE;

                XP_STRTOK(value, ";"); /* terminate at ';' */
                StrAllocCopy(URL_s->content_encoding, value);
              }
            else if(!strncasecomp(name,"CONTENT-RANGE:",14))
              {
                XP_STRTOK(value, ";"); /* terminate at ';' */

                /* range header looks like:
                 * Range: bytes x-y/z
                 * where:
                 *
                 * X     is the number of the first byte returned
                 *       (the first byte is byte number zero).
                 *
                 * Y     is the number of the last byte returned
                 *       (in case of the end of the document this
                 *       is one smaller than the size of the document
                 *       in bytes).
                 *
                 * Z     is the total size of the document in bytes.
                 */
                sscanf(value, "bytes %lu-%lu/%lu", &URL_s->low_range,
                                                   &URL_s->high_range,
                                                   &URL_s->content_length);

                /* if we get a range header set the "can-do-byteranges"
                 * since it must be doing byteranges
                 */
                URL_s->server_can_do_byteranges = TRUE;
              }
            else if(!strncasecomp(name,"CONNECTION:",11))
              {
                XP_STRTOK(value, ";"); /* terminate at ';' */
                XP_STRTOK(value, ","); /* terminate at ',' */
				if(!strcasecomp("KEEP-ALIVE", XP_StripLine(value)))
					URL_s->can_reuse_connection = TRUE;
              }
            break;

		case 'D':
            if(!strncasecomp(name,"DATE:",5))
              {
				found_one = TRUE;
                URL_s->server_date = NET_ParseDate(value);
			  }
			break;
        case 'E':
            if(!strncasecomp(name,"EXPIRES:",8))
              {
				char *cp, *expires = NET_RemoveQuotes(value);
				Bool is_number=TRUE;
				
				/* Expires: 123   - number of seconds
				 * Expires: date  - absolute date
				 */

                XP_STRTOK(value, ";"); /* terminate at ';' */

				/* check to see if the expires date is just a
				 * value in seconds
				 */
				for(cp = expires; *cp != '\0'; cp++)
					if(!isdigit(*cp))
					  {
						is_number=FALSE;
						break;
					  }
				
				if(is_number)
                	URL_s->expires = time(NULL) + atol(expires);
				else
                	URL_s->expires = NET_ParseDate(expires);

				/* if we couldn't parse the date correctly
				 * make it already expired, as per the HTTP spec
				 */
				if(!URL_s->expires)
					URL_s->expires = 1;

				found_one = TRUE;
              }
			else if(!strncasecomp(name,"EXT-CACHE:",10))
			  {
#ifdef MOZILLA_CLIENT
                char * next_arg = XP_STRTOK(value, ";");
				char * name=0;
				char * instructions=0;

				found_one = TRUE;

				while(next_arg)
                  {
                    next_arg = XP_StripLine(next_arg);

                    if(!strncasecomp(next_arg,"name=", 5))
                      {
                        TRACEMSG(("Found external cache name: %s", next_arg+5));
						name = NET_RemoveQuotes(next_arg+5);
                      }
                    else if(!strncasecomp(next_arg,"instructions=", 13))
                      {
                        TRACEMSG(("Found external cache instructions: %s", 
								  next_arg+13));
						instructions = NET_RemoveQuotes(next_arg+13);
                      }

                	next_arg = XP_STRTOK(NULL, ";");
                  }
	
				if(name)
					NET_OpenExtCacheFAT(context, name, instructions);
#else
				XP_ASSERT(0);
#endif /* MOZILLA_CLIENT */
              }
            break;

        case 'L':
            if(!strncasecomp(name,"LOCATION:",9))
              {
				found_one = TRUE;

				/* don't do this here because a url can
				 * contain a ';'
                 * XP_STRTOK(value, ";"); 
				 */

				URL_s->redirecting_url = NET_MakeAbsoluteURL(
													URL_s->address,
													XP_StripLine(value));

                TRACEMSG(("Found location: %s\n",URL_s->redirecting_url));

				if(MOCHA_TYPE_URL == NET_URL_Type(URL_s->redirecting_url))
				  {
					/* don't allow mocha URL's as refresh
					 */
					FREE_AND_CLEAR(URL_s->redirecting_url);
				  }
              }
            else if(!strncasecomp(name,"LAST-MODIFIED:",14))
              {
				found_one = TRUE;

                URL_s->last_modified = NET_ParseDate(value);
                TRACEMSG(("Found last modified date: %d\n",URL_s->last_modified));
              }
            break;

        case 'P':
            if(!strncasecomp(name,"PROXY-AUTHENTICATE:",19))
              {
                char *auth = value;

				found_one = TRUE;

                XP_STRTOK(value, ";"); /* terminate at ';' */

                if (net_IsBetterAuth(auth, URL_s->proxy_authenticate))
                  StrAllocCopy(URL_s->proxy_authenticate, auth);
              }
            else if(!strncasecomp(name,"PROXY-CONNECTION:",17))
              {
                XP_STRTOK(value, ";"); /* terminate at ';' */
                XP_STRTOK(value, ","); /* terminate at ',' */
				if(!strcasecomp("KEEP-ALIVE", XP_StripLine(value)))
					URL_s->can_reuse_connection = TRUE;
              }
            else if(!strncasecomp(name,"PRAGMA:",7))
              {
				found_one = TRUE;

                XP_STRTOK(value, ";"); /* terminate at ';' */

				if(!strcasecomp(value, "NO-CACHE"))
					URL_s->dont_cache = TRUE;
              }
            break;

        case 'R':
			
            if(!strncasecomp(name,"RANGE:",6))
			  {
                XP_STRTOK(value, ";"); /* terminate at ';' */

				/* range header looks like:
				 * Range: bytes x-y/z
 				 * where:
				 *
      			 * X     is the number of the first byte returned 
				 *  	 (the first byte is byte number zero).
				 *
      			 * Y     is the number of the last byte returned 
				 *		 (in case of the end of the document this 
				 *       is one smaller than the size of the document
          		 *       in bytes).
				 *
      			 * Z     is the total size of the document in bytes.
				 */
				sscanf(value, "bytes %lu-%lu/%lu", &URL_s->low_range, 
												   &URL_s->high_range,
												   &URL_s->content_length);

				/* if we get a range header set the "can-do-byteranges"
				 * since it must be doing byteranges
				 */
				URL_s->server_can_do_byteranges = TRUE;
			  }
            else if(!strncasecomp(name,"REFRESH:",8) && !EDT_IS_EDITOR(context))
              {
                char *first_arg, *next_arg;

				found_one = TRUE;

                /* clear any previous refresh URL */
                if(URL_s->refresh_url)
                  {
                    FREE(URL_s->refresh_url);
                    URL_s->refresh_url=0;
                  }

                first_arg = XP_STRTOK(value, ";");

                URL_s->refresh = atol(value);
                TRACEMSG(("Found refresh header: %d",URL_s->refresh));

                /* assign and compare
                 */
                while((next_arg = XP_STRTOK(NULL, ";")))
                  {
                    next_arg = XP_StripLine(next_arg);


                    if(!strncasecomp(next_arg,"URL=", 4))
                      {
						URL_s->refresh_url = NET_MakeAbsoluteURL(
												URL_s->address,
												NET_RemoveQuotes(next_arg+4));
                        TRACEMSG(("Found refresh url: %s",
                                                URL_s->refresh_url));

						if(MOCHA_TYPE_URL == NET_URL_Type(URL_s->refresh_url))
						  {
							/* don't allow mocha URL's as refresh
							 */
							FREE_AND_CLEAR(URL_s->refresh_url);
						  }
                      }
                  }

                if(!URL_s->refresh_url)
                    StrAllocCopy(URL_s->refresh_url, URL_s->address);

#ifdef DEBUG_JWZ
                /* jwz: don't ever allow zero-length refreshes. */
                if(URL_s->refresh_url && URL_s->refresh <= 0)
                  URL_s->refresh = 1;
#endif
              }
            break;

        case 'S':
            if(!strncasecomp(name,"SET-COOKIE:",11))
              {
				found_one = TRUE;

                NET_SetCookieString(context, URL_s->address, value);
              }
            else if(!strncasecomp(name, "SERVER:", 7))
              {
				found_one = TRUE;

                if(strcasestr(value, "NETSITE"))
                    URL_s->is_netsite = TRUE;
                else if(strcasestr(value, "NETSCAPE"))
                    URL_s->is_netsite = TRUE;
              }
            break;

        case 'W':
            if(!strncasecomp(name,"WWW-AUTHENTICATE:",17))
              {
				found_one = TRUE;

                XP_STRTOK(value, ";"); /* terminate at ';' */

                StrAllocCopy(URL_s->authenticate, value);
              }
            else if(!strncasecomp(name, "WWW-PROTECTION-TEMPLATE:", 24))
              {
				found_one = TRUE;

                XP_STRTOK(value, ";"); /* terminate at ';' */

                StrAllocCopy(URL_s->protection_template, value);
               }
            else if(!strncasecomp(name, "WINDOW-TARGET:", 14))
              {
				found_one = TRUE;

                XP_STRTOK(value, ";"); /* terminate at ';' */

                if (URL_s->window_target == NULL)
		  {
			if ((XP_IS_ALPHA(value[0]) != FALSE)||
			    (XP_IS_DIGIT(value[0]) != FALSE)||
			    (value[0] == '_'))
			  {
				StrAllocCopy(URL_s->window_target, value);
			  }
		  }
               }
            break;

        default:
        /* ignore other headers */
            break;
      }

	return(found_one);
}


#ifdef MOZILLA_CLIENT
extern MSG_FONT MSG_CitationFont;
extern MSG_CITATION_SIZE MSG_CitationSize;
extern char* MSG_CitationColor;
#endif /* MOZILLA_CLIENT */

/* scans a line for references to URL's and turns them into active
 * links.  If the output size is exceeded the line will be
 * truncated.  "output" must be at least "output_size" characters
 * long
 *
 * This also quotes other HTML forms, and italicizes citations,
 * unless `urls_only' is true.
 */

#ifndef MOZILLA_CLIENT
 /* If we're not in the client, stub out the libmsg interface to the 
    citation-highlighting code.
  */
# define MSG_PlainFont      0
# define MSG_BoldFont       1
# define MSG_ItalicFont     2
# define MSG_BoldItalicFont 3
# define MSG_NormalSize     4
# define MSG_Bigger         5
# define MSG_Smaller        6
static int MSG_CitationFont = MSG_ItalicFont;
static int MSG_CitationSize = MSG_NormalSize;
static const char *MSG_CitationColor = 0;

#endif /* MOZILLA_CLIENT */

#define AKBAR

PUBLIC int
NET_ScanForURLs(
#ifndef AKBAR
				void *ignored_egregious_lossage,
#endif /* !AKBAR */
				const char *input, int32 input_size,
				char *output, int output_size, XP_Bool urls_only)
{
  int col = 0;
  const char *cp;
  const char *end = input + input_size;
  char *output_ptr = output;
  char *end_of_buffer = output + output_size - 40; /* add safty zone :( */
  Bool line_is_citation = FALSE;
  const char *cite_open1, *cite_close1;
  const char *cite_open2, *cite_close2;
#ifndef AKBAR
  const char* color = NULL;
#else  /* AKBAR */
  const char* color = MSG_CitationColor;
#endif /* AKBAR */

  if (urls_only)
	{
	  cite_open1 = cite_close1 = "";
	  cite_open2 = cite_close2 = "";
	}
  else
	{
#ifdef MOZILLA_CLIENT
# ifdef AKBAR
	  MSG_FONT font = MSG_CitationFont;
	  MSG_CITATION_SIZE size = MSG_CitationSize;
# else  /* AKBAR */
	  MSG_Prefs* prefs;
	  MSG_FONT font = MSG_ItalicFont;
	  MSG_CITATION_SIZE size = MSG_NormalSize;

	  if (pane) {
		prefs = MSG_GetPrefs(pane);
		MSG_GetCitationStyle(prefs, &font, &size, &color);
	  }
#endif /* !AKBAR */
	  switch (font)
		{
		case MSG_PlainFont:
		  cite_open1 = "", cite_close1 = "";
		  break;
		case MSG_BoldFont:
		  cite_open1 = "<B>", cite_close1 = "</B>";
		  break;
		case MSG_ItalicFont:
		  cite_open1 = "<I>", cite_close1 = "</I>";
		  break;
		case MSG_BoldItalicFont:
		  cite_open1 = "<B><I>", cite_close1 = "</I></B>";
		  break;
		default:
		  XP_ASSERT(0);
		  cite_open1 = cite_close1 = "";
		  break;
		}
	  switch (size)
		{
		case MSG_NormalSize:
		  cite_open2 = "", cite_close2 = "";
		  break;
		case MSG_Bigger:
		  cite_open2 = "<FONT SIZE=\"+1\">", cite_close2 = "</FONT>";
		  break;
		case MSG_Smaller:
		  cite_open2 = "<FONT SIZE=\"-1\">", cite_close2 = "</FONT>";
		  break;
		default:
		  XP_ASSERT(0);
		  cite_open2 = cite_close2 = "";
		  break;
		}
#else  /* !MOZILLA_CLIENT */
		XP_ASSERT(0);
#endif /* !MOZILLA_CLIENT */
	}

  if (!urls_only)
	{
	  /* Decide whether this line is a quotation, and should be italicized.
		 This implements the following case-sensitive regular expression:

			^[ \t]*[A-Z]*[]>]

	     Which matches these lines:

		    > blah blah blah
		         > blah blah blah
		    LOSER> blah blah blah
		    LOSER] blah blah blah
	   */
	  const char *s = input;
	  while (s < end && XP_IS_SPACE (*s)) s++;
	  while (s < end && *s >= 'A' && *s <= 'Z') s++;

	  if (s >= end)
		;
	  else if (input_size >= 6 && *s == '>' &&
			   !XP_STRNCMP (input, ">From ", 6))	/* fucking sendmail... */
		;
	  else if (*s == '>' || *s == ']')
		{
		  line_is_citation = TRUE;
		  XP_STRCPY(output_ptr, cite_open1);
		  output_ptr += XP_STRLEN(cite_open1);
		  XP_STRCPY(output_ptr, cite_open2);
		  output_ptr += XP_STRLEN(cite_open2);
		  if (color &&
			  output_ptr + XP_STRLEN(color) + 20 < end_of_buffer) {
			XP_STRCPY(output_ptr, "<FONT COLOR=");
			output_ptr += XP_STRLEN(output_ptr);
			XP_STRCPY(output_ptr, color);
			output_ptr += XP_STRLEN(output_ptr);
			XP_STRCPY(output_ptr, ">");
			output_ptr += XP_STRLEN(output_ptr);
		  }
		}
	}

  /* Normal lines are scanned for buried references to URL's
     Unfortunately, it may screw up once in a while (nobody's perfect)
   */
  for(cp = input; cp < end && output_ptr < end_of_buffer; cp++)
	{
	  /* if NET_URL_Type returns true then it is most likely a URL
		 But only match protocol names if at the very beginning of
		 the string, or if the preceeding character was not alphanumeric;
		 this lets us match inside "---HTTP://XXX" but not inside of
		 things like "NotHTTP://xxx"
	   */
	  int type = 0;
	  if(!XP_IS_SPACE(*cp) &&
		 (cp == input || (!XP_IS_ALPHA(cp[-1]) && !XP_IS_DIGIT(cp[-1]))) &&
		 (type = NET_URL_Type(cp)) != 0)
		{
		  const char *cp2;
#if 0
		  Bool commas_ok = (type == MAILTO_TYPE_URL);
#endif

		  for(cp2=cp; cp2 < end; cp2++)
			{
			  /* These characters always mark the end of the URL. */
			  if (XP_IS_SPACE(*cp2) ||
				  *cp2 == '<' || *cp2 == '>' ||
				  *cp2 == '`' || *cp2 == ')' ||
				  *cp2 == '\'' || *cp2 == '"' ||
				  *cp2 == ']' || *cp2 == '}'
#if 0
				  || *cp2 == '!'
#endif
				  )
				break;

#if 0
			  /* But a "," marks the end of the URL only if there was no "?"
				 earlier in the URL (this is so we can do imagemaps, like
				 "foo.html?300,400".)
			   */
			  else if (*cp2 == '?')
				commas_ok = TRUE;
			  else if (*cp2 == ',' && !commas_ok)
				break;
#endif
			}

		  /* Check for certain punctuation characters on the end, and strip
			 them off. */
		  while (cp2 > cp && 
				 (cp2[-1] == '.' || cp2[-1] == ',' || cp2[-1] == '!' ||
				  cp2[-1] == ';' || cp2[-1] == '-' || cp2[-1] == '?' ||
				  cp2[-1] == '#'))
			cp2--;

		  col += (cp2 - cp);

		  /* if the url is less than 7 characters then we screwed up
		   * and got a "news:" url or something which is worthless
		   * to us.  Exclude the A tag in this case.
		   *
		   * Also exclude any URL that ends in a colon; those tend
		   * to be internal and magic and uninteresting.
		   *
		   * And also exclude the builtin icons, whose URLs look
		   * like "internal-gopher-binary".
		   */
		  if (cp2-cp < 7 ||
			  (cp2 > cp && cp2[-1] == ':') ||
			  !XP_STRNCMP(cp, "internal-", 9))
			{
			  XP_MEMCPY(output_ptr, cp, cp2-cp);
			  output_ptr += (cp2-cp);
			  *output_ptr = 0;
			}
		  else
			{
			  char *quoted_url;
			  int32 size_left = output_size - (output_ptr-output);

			  if(cp2-cp > size_left)
				return MK_OUT_OF_MEMORY;

			  XP_MEMCPY(output_ptr, cp, cp2-cp);
			  output_ptr[cp2-cp] = 0;
			  quoted_url = NET_EscapeHTML(output_ptr);
			  if (!quoted_url) return MK_OUT_OF_MEMORY;
			  PR_snprintf(output_ptr, size_left,
						  "<A HREF=\"%s\">%s</A>",
						  quoted_url,
						  quoted_url);
			  output_ptr += XP_STRLEN(output_ptr);
			  XP_FREE(quoted_url);
			  output_ptr += XP_STRLEN(output_ptr);
			}

		  cp = cp2-1;  /* go to next word */
		}
	  else
		{
		  /* Make sure that special symbols don't screw up the HTML parser
		   */
		  if(*cp == '<')
			{
			  XP_STRCPY(output_ptr, "&lt;");
			  output_ptr += 4;
			  col++;
			}
		  else if(*cp == '>')
			{
			  XP_STRCPY(output_ptr, "&gt;");
			  output_ptr += 4;
			  col++;
			}
		  else if(*cp == '&')
			{
			  XP_STRCPY(output_ptr, "&amp;");
			  output_ptr += 5;
			  col++;
			}
		  else
			{
			  *output_ptr++ = *cp;
			  col++;
			}
		}
	}

  *output_ptr = 0;

  if (line_is_citation)	/* Close off the highlighting */
	{
	  if (color) {
		XP_STRCPY(output_ptr, "</FONT>");
		output_ptr += XP_STRLEN(output_ptr);
	  }

	  XP_STRCPY(output_ptr, cite_close2);
	  output_ptr += XP_STRLEN (cite_close2);
	  XP_STRCPY(output_ptr, cite_close1);
	  output_ptr += XP_STRLEN (cite_close1);
	}

  return 0;
}

/* returns true if the URL is a secure URL address
 */
PUBLIC Bool
NET_IsURLSecure(char * address)
{
   int type = NET_URL_Type (address);

   TRACEMSG(("NET_IsURLSecure called, type: %d", type));

	if(type == SECURE_HTTP_TYPE_URL
		|| type == INTERNAL_IMAGE_TYPE_URL)
		return(TRUE);

    if(!strncasecomp(address, "/mc-icons/", 10) ||
       !strncasecomp(address, "/ns-icons/", 10))
        return(TRUE);

    if(!strncasecomp(address, "internal-external-reconnect:", 28))
        return(TRUE);

    if(!strcasecomp(address, "internal-external-plugin"))
        return(TRUE);

    TRACEMSG(("NET_IsURLSecure: URL NOT SECURE"));

	return(FALSE);
}

/* escapes all '<', '>' and '&' characters in a string
 * returns a string that must be freed
 */
PUBLIC char *
NET_EscapeHTML(const char * string)
{
    char *rv = (char *) XP_ALLOC(XP_STRLEN(string)*4 + 1); /* The +1 is for
															  the trailing
															  null! */
	char *ptr = rv;

	if(rv)
	  {
		for(; *string != '\0'; string++)
		  {
			if(*string == '<')
			  {
				*ptr++ = '&';
				*ptr++ = 'l';
				*ptr++ = 't';
				*ptr++ = ';';
			  }
			else if(*string == '>')
			  {
				*ptr++ = '&';
				*ptr++ = 'g';
				*ptr++ = 't';
				*ptr++ = ';';
			  }
			else if(*string == '&')
			  {
				*ptr++ = '&';
				*ptr++ = 'a';
				*ptr++ = 'm';
				*ptr++ = 'p';
				*ptr++ = ';';
			  }
			else
			  {
				*ptr++ = *string;
			  }
		  }
		*ptr = '\0';
	  }

	return(rv);
}


PUBLIC char *
NET_SpaceToPlus(char * string)
{

	char * ptr = string;

	if(!ptr)
		return(NULL);

	for(; *ptr != '\0'; ptr++)
		if(*ptr == ' ')
			*ptr = '+';

	return(string);
}


/* returns true if the functions thinks the string contains
 * HTML
 */
PUBLIC Bool
NET_ContainsHTML(char * string, int32 length)
{
	char * ptr = string;
	register int32 count=length;

	/* restrict searching to first K to limit false positives */
	if(count > 1024)
		count = 1024;

	/* if the string begins with "#!" or "%!" then it's a script of some kind,
	   and it doesn't matter how many HTML tags that program references in its
	   source -- it ain't HTML.  This false match happened all the time with,
	   for example, CGI scripts written in sh or perl that emit HTML. */
	if (count > 2 &&
		(string[0] == '#' || string[0] == '%') &&
		string[1] == '!')
	  return FALSE;

	/* If it begins with a mailbox delimiter, it's not HTML. */
	if (count > 5 &&
		(!XP_STRNCMP(string, "From ", 5) ||
		 !XP_STRNCMP(string, ">From ", 6)))
	  return FALSE;

	for(; count > 0; ptr++, count--)
		if(*ptr == '<')
		  {
			if(count > 3 && !strncasecomp(ptr+1, "HTML", 4))
				return(TRUE);

			if(count > 4 && !strncasecomp(ptr+1, "TITLE", 5))
				return(TRUE);
		
			if(count > 3 && !strncasecomp(ptr+1, "FRAMESET", 8))
				return(TRUE);

			if(count > 2 && 
				toupper(*(ptr+1)) == 'H' 
						&& isdigit(*(ptr+2)) && *(ptr+3) == '>')
				return(TRUE);
		  }

	return(FALSE);
}

/* take a Layout generated LO_FormSubmitData_struct
 * and use it to add post data to the URL Structure
 * generated by CreateURLStruct
 *
 * DOES NOT Generate the URL Struct, it must be created prior to
 * calling this function
 *
 * returns 0 on failure and 1 on success
 */
PUBLIC int
NET_AddLOSubmitDataToURLStruct(LO_FormSubmitData * sub_data, URL_Struct * url_struct)
{

	int32 i;
	int32 total_size;
	char *end, *tmp_ptr;
	char *encoding;
	char *target;
	char **name_array;
	char **value_array;
	uint8 *type_array;
	char *esc_string;

	if(!sub_data || !url_struct)
		return(0);

	if(sub_data->method == FORM_METHOD_GET)
		url_struct->method = URL_GET_METHOD;
	else
		url_struct->method = URL_POST_METHOD;
	if (!strncasecomp(url_struct->address, "mailto:", 7)) {
		url_struct->mailto_post = TRUE;
	}

	/* free any previous url_struct->post_data
	 */
	FREEIF(url_struct->post_data);

	PA_LOCK(name_array,  char**, sub_data->name_array);
	PA_LOCK(value_array, char**, sub_data->value_array);
	PA_LOCK(type_array,  uint8*, sub_data->type_array);
	PA_LOCK(encoding, char*, sub_data->encoding);

	/* free any previous target
	 */
	FREEIF(url_struct->window_target);
	PA_LOCK(target, char*, sub_data->window_target);
	if (target == NULL)
		url_struct->window_target = NULL;
	else
		url_struct->window_target = XP_STRDUP (target);

	FREEIF (url_struct->post_headers);

	/* If we're posting to mailto, then generate the full complement
	   of mail headers; and allow the url to specify additional headers
	   as well. */
	if (!strncasecomp(url_struct->address, "mailto:", 7))
	  {
#ifdef MOZILLA_CLIENT
		int status;
		char *new_url = 0;
		char *headers = 0;

		status = MIME_GenerateMailtoFormPostHeaders (url_struct->address,
													 url_struct->referer,
													 &new_url, &headers);
		if (status < 0)
		  {
			FREEIF (new_url);
			FREEIF (headers);
			return status;
		  }
		XP_ASSERT (new_url);
		XP_ASSERT (headers);
		url_struct->address_modified = TRUE;
		XP_FREE (url_struct->address);
		url_struct->address = new_url;
		url_struct->post_headers = headers;
#else
		XP_ASSERT(0);
#endif /* MOZILLA_CLIENT */
	  }

	if(encoding && !strcasecomp(encoding, "text/plain"))
	  {
		char *tmpfilename;
		char buffer[512];
		XP_File fp;

		/* always use post for this encoding type
		 */
		url_struct->method = URL_POST_METHOD;

		/* write all the post data to a file first
		 * so that we can send really big stuff
		 */
#ifdef XP_MAC	/* This should really be for all platforms but I am fixing a bug for final release */
  		tmpfilename = WH_TempName (xpFileToPost, "nsform");
		if (!tmpfilename) return 0;
  		fp = XP_FileOpen (tmpfilename, xpFileToPost, XP_FILE_WRITE_BIN);
#else
  		tmpfilename = WH_TempName (xpTemporary, "nsform");
		if (!tmpfilename) return 0;
  		fp = XP_FileOpen (tmpfilename, xpTemporary, XP_FILE_WRITE_BIN);
#endif
  		if (!fp) {
			XP_FREE(tmpfilename);
			return 0;
		}

		if (url_struct->post_headers)
		  {
			XP_FileWrite(url_struct->post_headers,
						 XP_STRLEN (url_struct->post_headers),
						 fp);
			XP_FREE (url_struct->post_headers);
			url_struct->post_headers = 0;
		  }

		XP_STRCPY (buffer,
				   "Content-type: text/plain" CRLF
				   "Content-Disposition: inline; form-data" CRLF CRLF);
		XP_FileWrite(buffer, XP_STRLEN(buffer), fp);

		for(i=0; i<sub_data->value_cnt; i++)
		  {
			if(name_array[i])
				XP_FileWrite(name_array[i], XP_STRLEN(name_array[i]), fp);
			XP_FileWrite("=", 1, fp);
			if(value_array[i])
				XP_FileWrite(value_array[i], XP_STRLEN(value_array[i]), fp);
			XP_FileWrite(CRLF, 2, fp);
		  }
		XP_FileClose(fp);
	
		StrAllocCopy(url_struct->post_data, tmpfilename);
		XP_FREE(tmpfilename);
		url_struct->post_data_is_file = TRUE;
	  }
	else if(encoding && !strcasecomp(encoding, "multipart/form-data"))
	  {
		/* encoding using a variant of multipart/mixed
 	 	 * and add files to it as well
		 */
		char *tmpfilename;
		char separator[80];
		char buffer[512];
		XP_File fp;
		int boundary_len;
		int cont_disp_len;
		NET_cinfo * ctype;


		/* always use post for this encoding type
		 */
		url_struct->method = URL_POST_METHOD;

		/* write all the post data to a file first
		 * so that we can send really big stuff
		 */
  		tmpfilename = WH_TempName (xpFileToPost, "nsform");
		if (!tmpfilename) return 0;
  		fp = XP_FileOpen (tmpfilename, xpFileToPost, XP_FILE_WRITE_BIN);
  		if (!fp) {
			XP_FREE(tmpfilename);
    		return 0;
		}

		sprintf(separator, "---------------------------%d%d%d",
                rand(), rand(), rand());

		if(url_struct->post_headers)
		  {
			XP_FileWrite(url_struct->post_headers,
					 	XP_STRLEN (url_struct->post_headers),
					 	fp);
			XP_FREE (url_struct->post_headers);
			url_struct->post_headers = 0;
		  }

		sprintf(buffer,
				"Content-type: multipart/form-data;"
				" boundary=%s" CRLF,
			    separator);
		XP_FileWrite(buffer, XP_STRLEN(buffer), fp);

#define CONTENT_DISPOSITION "Content-Disposition: form-data; name=\""
#define PLUS_FILENAME "\"; filename=\""
#define CONTENT_TYPE_HEADER "Content-Type: "

		/* compute the content length */
		total_size = -2; /* start at negative 2 to disregard the
						  * CRLF that act as a header separator 
						  */
		boundary_len = XP_STRLEN(separator) + 6;
		cont_disp_len = XP_STRLEN(CONTENT_DISPOSITION);

        for(i=0; i<sub_data->value_cnt; i++)
          {
			total_size += boundary_len;

			/* The size of the content-disposition line is hard
			 * coded and must be modified any time you change
			 * the sprintf in the next for loop
			 */
			total_size += cont_disp_len;
			if(name_array[i])
				total_size += XP_STRLEN(name_array[i]);
			total_size += 5;  /* quote plus two CRLF's */

			if(type_array[i] == FORM_TYPE_FILE)
			  {
				XP_StatStruct stat_entry;

				/* in this case we are going to send an extra
				 * ; filename="value_array[i]"
				 */
			    total_size += XP_STRLEN(PLUS_FILENAME);
				if(value_array[i])
				  {
                    /* only write the filename, not the whole path */
                    char * slash = XP_STRRCHR(value_array[i], '/');
                    if(slash)
                        slash++;
                    else
                        slash = value_array[i];
			    	total_size += XP_STRLEN(slash);

				    /* try and determine the content-type of the file
				     */
					ctype = NET_cinfo_find_type(value_array[i]);

					if(!ctype->is_default)
					  {
						/* we have determined it's type. Add enough
						 * space for it
						 */
						total_size += XP_STRLEN(CONTENT_TYPE_HEADER);
						total_size += XP_STRLEN(ctype->type);
						total_size += 2;  /* for the CRLF terminator */
					  }
				  }

			    /* if the type is a FILE type then we 
			     * need to stat the file to get the size
			     */
				if(value_array[i] && *value_array[i])
				    if(-1 != XP_Stat (value_array[i], &stat_entry, xpFileToPost))
					    total_size += stat_entry.st_size;

				/* if we can't stat the file just add zero */
			  }
			else
			  {
				if(value_array[i])
					total_size += XP_STRLEN(value_array[i]);
			  }
          }
		/* add the size of the last separator plus
		 * two for the extra two dashes
		 */
		total_size += boundary_len+2;

		sprintf(buffer, "Content-Length: %ld%s", total_size, CRLF);
		XP_FileWrite(buffer, XP_STRLEN(buffer), fp);

		for(i=0; i<sub_data->value_cnt; i++)
		  {
			sprintf(buffer, "%s--%s%s", CRLF, separator, CRLF);
			XP_FileWrite(buffer, XP_STRLEN(buffer), fp);
			
			/* WARNING!!! If you change the size of any of the
			 * sprintf's here you must change the size
			 * in the counting for loop above
			 */
			XP_FileWrite(CONTENT_DISPOSITION, cont_disp_len, fp);
			if(name_array[i])
				XP_FileWrite(name_array[i], XP_STRLEN(name_array[i]), fp);

			if(type_array[i] == FORM_TYPE_FILE)
			  {
				XP_FileWrite(PLUS_FILENAME, XP_STRLEN(PLUS_FILENAME), fp);
				if(value_array[i])
				  {
					/* only write the filename, not the whole path */
					char * slash = XP_STRRCHR(value_array[i], '/');
					if(slash)
						slash++;
					else
						slash = value_array[i];
					XP_FileWrite(slash, XP_STRLEN(slash), fp);

				  }
			  }
  			/* end the content disposition line */
			XP_FileWrite("\"" CRLF, XP_STRLEN("\"" CRLF), fp);

			if(type_array[i] == FORM_TYPE_FILE && value_array[i])
			  {
				/* try and determine the content-type of the file
             	 */
            	ctype = NET_cinfo_find_type(value_array[i]);

            	if(!ctype->is_default)
                  {
                	/* we have determined it's type. Send the
                 	* content-type
                 	*/
                	XP_FileWrite(CONTENT_TYPE_HEADER, 
									 	XP_STRLEN(CONTENT_TYPE_HEADER),
									 	fp);
                	XP_FileWrite(ctype->type,
                        			 	XP_STRLEN(ctype->type),
									 	fp);
					XP_FileWrite(CRLF, XP_STRLEN(CRLF), fp);
                  }
			  }

			/* end the header */
			XP_FileWrite(CRLF, XP_STRLEN(CRLF), fp);

			/* send the value of the form field */

			/* if the type is a FILE type, send the whole file,
			 * the filename is in the value field
			 */
			if(type_array[i] == FORM_TYPE_FILE)
			  {
				XP_File ext_fp=0;
				int32 size;

				if(value_array[i] && *value_array[i])
					ext_fp = XP_FileOpen(value_array[i], 
								 	xpFileToPost, 
								 	XP_FILE_READ_BIN);

				if(ext_fp)
				  {
					while((size = XP_FileRead(NET_Socket_Buffer, 
									  		  NET_Socket_Buffer_Size, 
									  		  ext_fp)))
					  {
						XP_FileWrite(NET_Socket_Buffer, size, fp);
					  }
					XP_FileClose(ext_fp);
				  }
			  }
			else
			  {
				if(value_array[i])
					XP_FileWrite(value_array[i], XP_STRLEN(value_array[i]), fp);
			  }
		  }

		sprintf(buffer, "%s--%s--%s", CRLF, separator, CRLF);
		XP_FileWrite(buffer, XP_STRLEN(buffer), fp);

		XP_FileClose(fp);
	
		StrAllocCopy(url_struct->post_data, tmpfilename);
		XP_FREE(tmpfilename);
		url_struct->post_data_is_file = TRUE;

	  }
	else
	  {
		total_size=1; /* start at one for the terminator char */

		/* find out how much space we need total
		 *
		 * and also convert all spaces to pluses at the same time
	 	 */
		for(i=0; i<sub_data->value_cnt; i++)
		  {
			total_size += NET_EscapedSize(name_array[i], URL_XPALPHAS);
			total_size += NET_EscapedSize(value_array[i], URL_XPALPHAS);
			total_size += 2;  /* & = */
		  }

		if(sub_data->method == FORM_METHOD_GET)
		  {
			char *punc;

			if(!url_struct->address)
				return(0);

			/* get rid of ? or # in the url string since we are adding it
			 */
			punc = XP_STRCHR(url_struct->address, '?');
			if(punc)
			   *punc = '\0';  /* terminate here */
			punc = XP_STRCHR(url_struct->address, '#');
			if(punc)
			   *punc = '\0';  /* terminate here */

			/* add the size of the url plus one for the '?'
			 */
			total_size += XP_STRLEN(url_struct->address)+1;
		  }

		url_struct->post_data = (char *) XP_ALLOC(total_size);

		if(!url_struct->post_data)
		  {
			PA_UNLOCK(sub_data->name_array);
			PA_UNLOCK(sub_data->value_array);
			PA_UNLOCK(sub_data->type_array);
			PA_UNLOCK(sub_data->encoding);
			return(0);
		  }

		if(sub_data->method == FORM_METHOD_GET)
		  {
		  	end = url_struct->post_data;
			for(tmp_ptr = url_struct->address; *tmp_ptr != '\0'; tmp_ptr++)
				*end++ = *tmp_ptr;

			/* add the '?'
			 */
			*end++ = '?';

			/* swap the post data and address data */
			FREE(url_struct->address);
			url_struct->address = url_struct->post_data;
			url_struct->post_data = 0;

			/* perform  hack:
			 * To be compatible with old pre-form 
			 * indexes, other web browsers had a hack
			 * wherein if a form was being submitted, 
			 * and its method was get, and it
			 * had only one name/value pair, and the 
			 * name of that pair was "isindex", then
			 * it would create the query url
			 *
			 * URL?value instead of URL?name=value
			 */
			if(sub_data->value_cnt == 1 && !strcasecomp(name_array[0], "isindex"))
			  {
			    if(value_array && value_array[0])
					XP_STRCPY(end, NET_Escape(value_array[0], URL_XPALPHAS));
				else
					*end = '\0';
				PA_UNLOCK(sub_data->name_array);
				PA_UNLOCK(sub_data->value_array);
				PA_UNLOCK(sub_data->type_array);
				PA_UNLOCK(sub_data->encoding);
                return(1);
			  }
			
			/* the end ptr is still set to the correct address!
			 */

		  }
		else
		  {
			/* Append a content-type to the headers we just generated. */
            StrAllocCat(url_struct->post_headers, 
						"Content-type: application/x-www-form-urlencoded"
						CRLF);

            if(!url_struct->post_headers)
              {
                FREE_AND_CLEAR(url_struct->post_data);
				PA_UNLOCK(sub_data->name_array);
				PA_UNLOCK(sub_data->value_array);
				PA_UNLOCK(sub_data->type_array);
				PA_UNLOCK(sub_data->encoding);
                return(0);
              }

		  	end = url_struct->post_data;

          }

		/* build the string 
		 */
		for(i=0; i<sub_data->value_cnt; i++)
		  {
			/* add the name
			 */
			esc_string = NET_Escape(name_array[i], URL_XPALPHAS);
			if(esc_string)
			  {
			    for(tmp_ptr = esc_string; *tmp_ptr != '\0'; tmp_ptr++)
				    *end++ = *tmp_ptr;
			    XP_FREE(esc_string);
			  }

			/* join the name and value with a '='
		 	 */
			*end++ = '=';

			/* add the value
			 */
			esc_string = NET_Escape(value_array[i], URL_XPALPHAS);
			if(esc_string)
			  {
			    for(tmp_ptr = esc_string; *tmp_ptr != '\0'; tmp_ptr++)
				    *end++ = *tmp_ptr;
			    XP_FREE(esc_string);
			  }

			/* join pairs with a '&' 
			 * make sure there is another pair before adding
			 */
			if(i+1 < sub_data->value_cnt)
				*end++ = '&';
		  }
		
		/* terminate the string
		 */
		*end = '\0';

	    if(sub_data->method == FORM_METHOD_POST)
	      {
			char buffer[64];
	        url_struct->post_data_size = XP_STRLEN(url_struct->post_data);
			XP_SPRINTF(buffer, 
					   "Content-length: %ld" CRLF, 
					   url_struct->post_data_size);
            StrAllocCat(url_struct->post_headers, buffer);

			/* munge for broken CGIs.  Add an extra CRLF to the 
			 * post data.
			 */
			BlockAllocCat(url_struct->post_data, 
						  url_struct->post_data_size,
						  CRLF, 
						  3);
			/* don't add the eol terminator */
			url_struct->post_data_size += 2;

	      }
	  }

	PA_UNLOCK(sub_data->name_array);
	PA_UNLOCK(sub_data->value_array);
	PA_UNLOCK(sub_data->type_array);
	PA_UNLOCK(sub_data->encoding);

	return(1); /* success */
}

PUBLIC int
NET_AddCoordinatesToURLStruct(URL_Struct * url_struct, int32 x_coord, int32 y_coord)
{

	if(url_struct->address)
	  {
		char buffer[32];

		XP_SPRINTF(buffer, "?%ld,%ld", x_coord, y_coord);

        /* get rid of ? or # in the url string since we are adding it
         */
#undef STRIP_SEARCH_DATA_FROM_ISMAP_URLS
#ifdef STRIP_SEARCH_DATA_FROM_ISMAP_URLS
	  {
		char *punc;
        punc = XP_STRCHR(url_struct->address, '?');
        if(punc)
            *punc = '\0';  /* terminate here */
        punc = XP_STRCHR(url_struct->address, '#');
        if(punc)
            *punc = '\0';  /* terminate here */
	  }
#endif /* STRIP_SEARCH_DATA_FROM_ISMAP_URLS */

		StrAllocCat(url_struct->address, buffer);
	  }

	return(1); /* success */
}

/* toggle the trace messages on and off
 *
 */
PUBLIC void
NET_ToggleTrace(void)
{
    if(MKLib_trace_flag)
		MKLib_trace_flag = 0;
	else
		MKLib_trace_flag = 2;	

}

/* FREE_AND_CLEAR will free a pointer if it is non-zero and
 * then set it to zero
 */
MODULE_PRIVATE void 
NET_f_a_c (char **pointer)
{
    if(*pointer) {
	XP_FREE(*pointer);
        *pointer = 0;
    }
}

/* recognizes URLs and their types.  Returns 0 (zero) if
 * it is unrecognized.
 */
PUBLIC int 
NET_URL_Type (CONST char *URL)
{
    /* Protect from SEGV */
    if (!URL || (URL && *URL == '\0'))
        return(0);

    switch(*URL) {
    case 'a':
    case 'A':
		if(!strncasecomp(URL,"about:security", 14))
		    return(SECURITY_TYPE_URL);
		else if(!strncasecomp(URL,"about:",6))
		    return(ABOUT_TYPE_URL);
		break;
	case 'f':
	case 'F':
		if(!strncasecomp(URL,"ftp:",4))
		    return(FTP_TYPE_URL);
		else if(!strncasecomp(URL,"file:",5))
		    return(FILE_TYPE_URL);
		break;
	case 'g':
	case 'G':
    	if(!strncasecomp(URL,"gopher:",7)) 
	        return(GOPHER_TYPE_URL);
		break;
	case 'h':
	case 'H':
		if(!strncasecomp(URL,"http:",5))
		    return(HTTP_TYPE_URL);
#ifndef _blargh_NO_SSL /* added by jwz */
		else if(!strncasecomp(URL,"https:",6))
		    return(SECURE_HTTP_TYPE_URL);
#endif /* NO_SSL */
		break;
	case 'i':
	case 'I':
		if(!strncasecomp(URL,"internal-gopher-",16))
			return(INTERNAL_IMAGE_TYPE_URL);
		else if(!strncasecomp(URL,"internal-news-",14))
			return(INTERNAL_IMAGE_TYPE_URL);
		else if(!strncasecomp(URL,"internal-edit-",14))
			return(INTERNAL_IMAGE_TYPE_URL);
		else if(!strncasecomp(URL,"internal-sa-",12))
			return(INTERNAL_IMAGE_TYPE_URL);
		else if(!strncasecomp(URL,"internal-smime-",15))
			return(INTERNAL_IMAGE_TYPE_URL);
		else if(!strncasecomp(URL,"internal-dialog-handler",23))
			return(HTML_DIALOG_HANDLER_TYPE_URL);
		else if(!strncasecomp(URL,"internal-panel-handler",22))
			return(HTML_PANEL_HANDLER_TYPE_URL);
		else if(!strncasecomp(URL,"internal-security-",18))
			return(INTERNAL_SECLIB_TYPE_URL);
		break;
	case 'j':
	case 'J':
		if(!strncasecomp(URL, "javascript:",11))
		    return(MOCHA_TYPE_URL);
		break;
	case 'l':
	case 'L':
		if(!strncasecomp(URL, "livescript:",11))
		    return(MOCHA_TYPE_URL);
		break;
	case 'm':
	case 'M':
    	if(!strncasecomp(URL,"mailto:",7)) 
		    return(MAILTO_TYPE_URL);
		else if(!strncasecomp(URL,"mailbox:",8))
		    return(MAILBOX_TYPE_URL);
		else if(!strncasecomp(URL, "mocha:",6))
		    return(MOCHA_TYPE_URL);
		break;
	case 'n':
	case 'N':
		if(!strncasecomp(URL,"news:",5))
		    return(NEWS_TYPE_URL);
		break;
	case 'p':
	case 'P':
		if(!strncasecomp(URL,"pop3:",5))
		    return(POP3_TYPE_URL);
		break;
	case 'r':
	case 'R':
		if(!strncasecomp(URL,"rlogin:",7))
		    return(RLOGIN_TYPE_URL);
		break;
	case 's':
	case 'S':
		if(!strncasecomp(URL,"snews:",6))
		    return(NEWS_TYPE_URL);
	case 't':
	case 'T':
		if(!strncasecomp(URL,"telnet:",7))
		    return(TELNET_TYPE_URL);
		else if(!strncasecomp(URL,"tn3270:",7))
		    return(TN3270_TYPE_URL);
		break;
	case 'u':
	case 'U':
		if(!strncasecomp(URL,"URN:",4))
		    return(URN_TYPE_URL);
		break;
	case 'v':
	case 'V':
		if(!strncasecomp(URL, VIEW_SOURCE_URL_PREFIX, 
							  sizeof(VIEW_SOURCE_URL_PREFIX)-1))
		    return(VIEW_SOURCE_TYPE_URL);
		break;
	case 'w':
	case 'W':
		if(!strncasecomp(URL,"wais:",5))
		    return(WAIS_TYPE_URL);
		if(!strncasecomp(URL,"wysiwyg:",8))
		    return(WYSIWYG_TYPE_URL);
		break;
    }

    /* no type match :( */
    return(0);
}

PUBLIC void
NET_PlusToSpace(char *str)
{
    for (; *str != '\0'; str++)
		if (*str == '+')
			*str = ' ';
}
