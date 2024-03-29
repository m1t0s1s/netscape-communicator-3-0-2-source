/* -*- Mode: C; tab-width: 4 -*-
 * state machine to speak HTTP
 *
 * Lou Montulli
 */
#include "mkutils.h"

#include "merrors.h"

#include "mkgeturl.h"
#include "mkhttp.h"
#include "shist.h"
#include "glhist.h"
#include "mkparse.h"
#include "mktcp.h"
#include "mkstream.h"
#include "mkformat.h"
#include "mkaccess.h"
#include "mkcache.h"
#include "mkautocf.h"
#include "secnav.h"
#include "cert.h"
#include "ssl.h"

#include "xp_error.h"
#include "libi18n.h"
#include "prtime.h"


/* for XP_GetString() */
#include "xpgetstr.h"
extern int MK_HTTP_TYPE_CONFLICT;
extern int MK_OUT_OF_MEMORY;
extern int MK_TCP_READ_ERROR;
extern int MK_TCP_WRITE_ERROR;
extern int MK_CONNECTION_REFUSED;
extern int MK_CONNECTION_TIMED_OUT;
extern int MK_UNABLE_TO_CONNECT;
extern int MK_UNABLE_TO_CONNECT_TO_PROXY;
extern int MK_UNABLE_TO_LOCATE_HOST;
extern int MK_UNABLE_TO_LOCATE_FILE;
extern int MK_UNABLE_TO_OPEN_FILE;
extern int MK_ZERO_LENGTH_FILE;
extern int MK_COMPUSERVE_AUTH_FAILED;
extern int XP_ALERT_UNKNOWN_STATUS;
extern int XP_ERRNO_EWOULDBLOCK;
extern int XP_PROGRESS_TRANSFER_DATA;
extern int XP_PROGRESS_TRYAGAIN;
extern int XP_PROGRESS_WAIT_REPLY;
extern int XP_HR_TRANSFER_INTERRUPTED;
extern int XP_TRANSFER_INTERRUPTED;
extern int XP_ERRNO_EIO;			


#ifdef PROFILE
#pragma profile on
#endif

#define VERSION_STRING "HTTP/1.0"

#define MAX_CACHED_HTTP_CONNECTIONS 4

/* the maximum size of a HTTP servers first line of response
 */
#define MAX_FIRST_LINE_SIZE 250

/* temporary
 */
#ifdef XP_WIN
PUBLIC char *XP_AppName = 0;
PUBLIC char *XP_AppCodeName = 0;
PUBLIC char *XP_AppVersion = 0;
#else
PUBLIC CONST char *XP_AppName = 0;
PUBLIC CONST char *XP_AppCodeName = 0;
PUBLIC CONST char *XP_AppVersion = 0;
#endif

PUBLIC char * FE_UsersFromField=0;    /* User's name/email address not used yet */

PRIVATE XP_List * http_connection_list=0;
PRIVATE IdentifyMeEnum http_identification_method = DoNotIdentifyMe;

/* definitions of state for the state machine design
 */
typedef enum {
	HTTP_START_CONNECT,
	HTTP_FINISH_CONNECT,
	HTTP_SEND_SSL_PROXY_REQUEST,
	HTTP_BEGIN_UPLOAD_FILE,
	HTTP_SEND_REQUEST,
	HTTP_SEND_POST_DATA,
	HTTP_PARSE_FIRST_LINE,
	HTTP_PARSE_MIME_HEADERS,
	HTTP_SETUP_STREAM,
	HTTP_BEGIN_PUSH_PARTIAL_CACHE_FILE,
	HTTP_PUSH_PARTIAL_CACHE_FILE,
	HTTP_PULL_DATA,
	HTTP_DONE,
	HTTP_ERROR_DONE,
	HTTP_FREE
} StatesEnum;

/* structure to hold data about a tcp connection
 * to a news host
 */
typedef struct _HTTPConnection {
    char   *hostname;       /* hostname string (may contain :port) */
    NETSOCK sock;           /* socket */
    XP_Bool busy;           /* is the connection in use already? */
    XP_Bool prev_cache;     /* did this connection come from the cache? */
    XP_Bool secure;         /* is it a secure connection? */
} HTTPConnection;

/* structure to hold data pertaining to the active state of
 * a transfer in progress.
 *
 */
typedef struct _HTTPConData {
    StatesEnum  next_state;       /* the next state or action to be taken */
    char *      proxy_server;     /* name of proxy server if any */
	char *		proxy_conf;       /* proxy config ptr from proxy autoconfig */
    char *      line_buffer;      /* temporary string storage */
    char *      server_headers;   /* headers from the server for 
								   * the proxy client
								   */
	char *      orig_host;        /* if the host gets modified
								   * for my "netscape" -> "www.netscape.com"
								   * hack, the original host gets put here
								   */
	XP_File     partial_cache_fp;
	int32       partial_needed;  	/* the part missing from the cache */

	int32		total_size_of_files_to_post;
	int32       total_amt_written;

    int32       line_buffer_size; /* current size of the line buffer */
	
	HTTPConnection *connection;   /* struct to hold info about connection */

    NET_StreamClass * stream; /* The output stream */
    Bool     pause_for_read;   /* Pause now for next read? */
    Bool     send_http1;       /* should we send http/1.0? */
    Bool     receiving_http1;  /* is the server responding http/1.0? */
    Bool     acting_as_proxy;  /* are we acting as a proxy? */
	Bool     server_busy_retry; /* should we retry the get? */
    Bool     posting;          /* are we posting? */
    Bool     doing_redirect;   /* are we redirecting? */
	Bool     sent_authorization;     /* did we send auth with the request? */
    Bool     sent_proxy_auth;		 /* did we send proxy auth with the req? */
    Bool     authorization_required; /* did we get a 401 auth return code? */
    Bool     proxy_auth_required;    /* did we get a 407 auth return code? */
	Bool     destroy_graph_progress; /* destroy graph progress? */
    Bool     destroy_file_upload_progress_dialog;  
	int32    original_content_length; /* the content length at the time of
									   * calling graph progress
									   */
	TCP_ConData *tcp_con_data;  /* Data pointer for tcp connect state machine */
	Bool         use_ssl_proxy;          /* should we use the SSL proxy? */
	Bool         ssl_proxy_setup_done;   /* is setup done */
	Bool         use_copy_from_cache;    /* did we get a 304? */
	Bool         displayed_some_data;    /* have we displayed any data? */
	Bool         save_connection;        /* save this connection for reuse? */
	Bool         partial_cache_file;
	Bool         reuse_stream;
	Bool         connection_is_valid;
#ifdef XP_WIN
	Bool         calling_netlib_all_the_time;  /* is SetCallNetlibAllTheTime set? */
#endif
	void        *write_post_data_data;   /* a data object 
										  * for the WritePostData function 
										  */
} HTTPConData;

/* macro's to simplify variable names */
#define CD_NEXT_STATE        	  cd->next_state
#define CD_PROXY_SERVER      	  cd->proxy_server
#define CD_PROXY_CONF      	      cd->proxy_conf
#define CD_LINE_BUFFER       	  cd->line_buffer
#define CD_SERVER_HEADERS    	  cd->server_headers
#define CD_LINE_BUFFER_SIZE  	  cd->line_buffer_size
#define CD_STREAM        	 	  cd->stream
#define CD_SEND_HTTP1        	  cd->send_http1
#define CD_RECEIVING_HTTP1   	  cd->receiving_http1
#define CD_PAUSE_FOR_READ    	  cd->pause_for_read
#define CD_ACTING_AS_PROXY   	  cd->acting_as_proxy
#define CD_SERVER_BUSY_RETRY      cd->server_busy_retry
#define CD_POSTING       	 	  cd->posting
#define CD_DOING_REDIRECT    	  cd->doing_redirect
#define CD_SENT_AUTHORIZATION     cd->sent_authorization
#define CD_SENT_PROXY_AUTH        cd->sent_proxy_auth
#define CD_AUTH_REQUIRED     	  cd->authorization_required
#define CD_PROXY_AUTH_REQUIRED    cd->proxy_auth_required
#define CD_DESTROY_GRAPH_PROGRESS   cd->destroy_graph_progress
#define CD_ORIGINAL_CONTENT_LENGTH  cd->original_content_length
#define CD_TCP_CON_DATA			  cd->tcp_con_data
#define CD_USE_SSL_PROXY          cd->use_ssl_proxy
#define CD_SSL_PROXY_SETUP_DONE   cd->ssl_proxy_setup_done
#define CD_USE_COPY_FROM_CACHE    cd->use_copy_from_cache
#define CD_DISPLAYED_SOME_DATA    cd->displayed_some_data

#define CE_URL_S            ce->URL_s
#define CE_SOCK             ce->socket
#define CE_CON_SOCK         ce->con_sock
#define CE_STATUS           ce->status
#define CE_WINDOW_ID        ce->window_id
#define CE_BYTES_RECEIVED   ce->bytes_received
#define CE_FORMAT_OUT       ce->format_out

/*
 * replace
 *	  return(CE_STATUS)
 * with
 * 	  RETURN_CE_STATUS
 */
PRIVATE
int ReturnErrorStatus (int status)
{
	if (status < 0)
		status |= status; /* set a breakpoint HERE to find errors */
	return status;
}
#define STATUS(Status)			ReturnErrorStatus (Status)

#define PUTBLOCK(b, l)  (*cd->stream->put_block) \
                                    (cd->stream->data_object, b, l)
#define PUTSTRING(s)    (*cd->stream->put_block) \
                                    (cd->stream->data_object, s, XP_STRLEN(s))
#define COMPLETE_STREAM (*cd->stream->complete) \
                                    (cd->stream->data_object)
#define ABORT_STREAM(s) (*cd->stream->abort) \
                                    (cd->stream->data_object, s)


/* set the method that the user wishes to identify themselves
 * with.
 * Default is DoNotIdentify unless this is called at startup
 */
PUBLIC void 
NET_SetIdentifyMeType(IdentifyMeEnum method)
{
	http_identification_method = method;
}

/* look for names like "ford" and "netscape"
 * and turn them into "www.ford.com" and "www.netscape.com"
 *
 * returns 0 if found and replaced
 */
PRIVATE int
net_check_for_company_hostname(ActiveEntry *ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;

	if(!cd->orig_host)
	  {
	
		/* if the hostname doesn't have any
	 	* dots in it, then assume it's of
	 	* the form "netscape" or "ford".
	 	* If we add www. and .com to the front
	 	* and end we will get www.netscape.com :)
	 	*/
		char * host = NET_ParseURL(CE_URL_S->address, GET_HOST_PART);
	
		if(host && *host && !XP_STRCHR(host, '.'))
	  	  {
			/* no dots in hostname */
			char *new_address = NET_ParseURL(CE_URL_S->address, 
										 	GET_PROTOCOL_PART);
			char *tmp = NET_ParseURL(CE_URL_S->address, GET_HOST_PART);

			/* save the host for error messages
			 */
			cd->orig_host = tmp;
	
			StrAllocCat(new_address, "//www.");
			StrAllocCat(new_address, tmp);
	
			StrAllocCat(new_address, ".com");

			tmp = NET_ParseURL(CE_URL_S->address, 
						GET_PATH_PART | GET_SEARCH_PART | GET_HASH_PART);
					
			StrAllocCat(new_address, tmp);
			FREEIF(tmp);
	
			FREE(CE_URL_S->address);
			CE_URL_S->address = new_address;
	
	    	if(cd->connection->sock != SOCKET_INVALID)
		    	NET_TotalNumberOfOpenConnections--;
	
			return(0);  /* will repeat this step */
	  	  }
	  } 
	else
	  {
		/* the host mapping trick has already been applied and failed
		 * redo the error message and fail
		 */
		CE_URL_S->error_msg = NET_ExplainErrorDetails(
												MK_UNABLE_TO_LOCATE_HOST, 
												cd->orig_host);
				/* fall through for failure case */
	  }

	return(-1);
}

/* begins the connect process
 *
 * returns the TCP status code
 */
PRIVATE int
net_start_http_connect(ActiveEntry * ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;
	Bool use_security=FALSE;
	int def_port;

	def_port = DEF_HTTP_PORT;
	if(ce->protocol == SECURE_HTTP_TYPE_URL)
	  {

		if(CD_PROXY_SERVER)
		  {
#ifndef NO_SSL /* added by jwz */
			CD_USE_SSL_PROXY = TRUE;
			CD_SSL_PROXY_SETUP_DONE = FALSE;
#endif /* NO_SSL -- added by jwz */

			TRACEMSG(("HTTP: Using SSL proxy"));

			/* put in code to check for using SSL to a proxy
			 * if we need to use ssl set the options
			 * on the next two lines
			 *
			 * use_security = TRUE;
			 * def_port = DEF_HTTPS_PORT;
			 * 
			 * @@@@@
			 */
		  }
		else
		  {
			use_security = TRUE;
			def_port = DEF_HTTPS_PORT;
		  }
	  }

    /* if proxy_server is non NULL then use the string as a host:port
     * when a proxy server is used a connection is made to the proxy
     * host and port and the entire URL is sent instead of just
     * the path part.
     */ 
    if(CD_PROXY_SERVER) 
      {
    	/* MKConnect can take a URL or a host name as it's first
     	 * argument
     	 */
        CE_STATUS = NET_BeginConnect (CD_PROXY_SERVER,  
									  NULL,
									  "HTTP", 
									  def_port, 
									  &cd->connection->sock, 
									  use_security, 
									  &CD_TCP_CON_DATA, 
									  CE_WINDOW_ID, 
									  &CE_URL_S->error_msg,
									  ce->socks_host,
									  ce->socks_port);
      }
    else
      {
        CE_STATUS = NET_BeginConnect (CE_URL_S->address,
									  CE_URL_S->IPAddressString,
									  "HTTP", 
									  def_port, 
							          &cd->connection->sock, 
									  use_security,  
									  &CD_TCP_CON_DATA, 
									  CE_WINDOW_ID, 
									  &CE_URL_S->error_msg,
									  ce->socks_host,
									  ce->socks_port);
      }

	/* set this so mkgeturl can select on it */
	CE_SOCK = cd->connection->sock;

	if(cd->connection->sock != SOCKET_INVALID)
		NET_TotalNumberOfOpenConnections++;

    if (CE_STATUS < 0) 
      {
		if(CE_STATUS == MK_UNABLE_TO_LOCATE_HOST 
			&& 0 == net_check_for_company_hostname(ce))
		  {
			/* no need to set the state since this
			 * is the state we want again
			 */
			if(cd->connection->sock != SOCKET_INVALID)
			  {
				NETCLOSE(cd->connection->sock);
				cd->connection->sock = SOCKET_INVALID;
				CE_SOCK = SOCKET_INVALID;
			  }
		    return(0);
		  }

        TRACEMSG(("HTTP: Unable to connect to host for `%s' (errno = %d).", 
                          CE_URL_S->address, SOCKET_ERRNO));
		/*
		 * Proxy failover
		 */
		if (CD_PROXY_CONF && CD_PROXY_SERVER &&
			(CE_STATUS == MK_UNABLE_TO_CONNECT    ||
			 CE_STATUS == MK_CONNECTION_TIMED_OUT ||
			 CE_STATUS == MK_CONNECTION_REFUSED   ||
			 CE_STATUS == MK_UNABLE_TO_LOCATE_HOST))
		  {
#ifdef MOZILLA_CLIENT
			  if (pacf_get_proxy_addr(CE_WINDOW_ID, CD_PROXY_CONF,
									  &ce->proxy_addr,
									  &ce->socks_host,
									  &ce->socks_port))
				{
					CD_PROXY_SERVER = ce->proxy_addr;
					return net_start_http_connect(ce);
				}
#endif	/* MOZILLA_CLIENT */
		  }

		if(CE_STATUS == MK_UNABLE_TO_CONNECT && CD_PROXY_SERVER)
		  {
			StrAllocCopy (CE_URL_S->error_msg,
						  XP_GetString(MK_UNABLE_TO_CONNECT_TO_PROXY));
			CE_STATUS = MK_UNABLE_TO_CONNECT_TO_PROXY;
		  }

        CD_NEXT_STATE = HTTP_ERROR_DONE;
        return STATUS(CE_STATUS);
      }

    if(CE_STATUS == MK_CONNECTED)
      {
#ifndef NO_SSL /* added by jwz */
		if(CD_USE_SSL_PROXY)
            CD_NEXT_STATE = HTTP_SEND_SSL_PROXY_REQUEST;
		else
#endif /* NO_SSL -- added by jwz */
		  if(ce->URL_s->files_to_post)
			CD_NEXT_STATE = HTTP_BEGIN_UPLOAD_FILE;
		else
            CD_NEXT_STATE = HTTP_SEND_REQUEST;
        TRACEMSG(("Connected to HTTP server on sock #%d",cd->connection->sock));
        FE_SetReadSelect(CE_WINDOW_ID, cd->connection->sock);
      }
    else
      {
        CD_NEXT_STATE = HTTP_FINISH_CONNECT;
        CD_PAUSE_FOR_READ = TRUE;
        CE_CON_SOCK = cd->connection->sock;  /* set the socket for select */
        FE_SetConnectSelect(CE_WINDOW_ID, CE_CON_SOCK);
      }

    return STATUS(CE_STATUS);
}

/*  begins the connect process
 *
 *  returns the TCP status code
 */
PRIVATE int
net_finish_http_connect(ActiveEntry * ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;
    int def_port;

    def_port = DEF_HTTP_PORT;
    if(ce->protocol == SECURE_HTTP_TYPE_URL)
      {
        def_port = DEF_HTTPS_PORT;
      }

    /* if proxy_server is non NULL then use the string as a host:port
     * when a proxy server is used a connection is made to the proxy
     * host and port and the entire URL is sent instead of just
     * the path part.
     */
    if(CD_PROXY_SERVER)
      {
        /* MKConnect can take a URL or a host name as it's first
         * argument
         */
        CE_STATUS = NET_FinishConnect(CD_PROXY_SERVER,  
									  "HTTP",
									  def_port, 
									  &cd->connection->sock, 
									  &CD_TCP_CON_DATA, 
									  CE_WINDOW_ID,
									  &CE_URL_S->error_msg);
      }
    else
      {
        CE_STATUS = NET_FinishConnect(CE_URL_S->address,
									  "HTTP",
									  def_port,
									  &cd->connection->sock,  
									  &CD_TCP_CON_DATA, 
									  CE_WINDOW_ID,
									  &CE_URL_S->error_msg);
      }


    if (CE_STATUS < 0)
      {
		if(CE_STATUS == MK_UNABLE_TO_LOCATE_HOST 
			&& 0 == net_check_for_company_hostname(ce))
		  {
			if(cd->connection->sock != SOCKET_INVALID)
			  {
				FE_ClearConnectSelect(CE_WINDOW_ID, cd->connection->sock);
				NETCLOSE(cd->connection->sock);
				cd->connection->sock = SOCKET_INVALID;
				CE_SOCK = SOCKET_INVALID;
				CE_CON_SOCK = SOCKET_INVALID;
			  }
			cd->next_state = HTTP_START_CONNECT;
		  	return(0);
		  }

        FE_ClearConnectSelect(CE_WINDOW_ID, CE_CON_SOCK);
        TRACEMSG(("HTTP: Unable to connect to host for `%s' (errno = %d).",
                                              CE_URL_S->address, SOCKET_ERRNO));

		/*
		 * Proxy failover
		 */
		if (CD_PROXY_CONF && CD_PROXY_SERVER &&
			(CE_STATUS == MK_UNABLE_TO_CONNECT    ||
			 CE_STATUS == MK_CONNECTION_TIMED_OUT ||
			 CE_STATUS == MK_CONNECTION_REFUSED   ||
			 CE_STATUS == MK_UNABLE_TO_LOCATE_HOST))
		  {
#ifdef MOZILLA_CLIENT
			  if (pacf_get_proxy_addr(CE_WINDOW_ID, CD_PROXY_CONF,
									  &ce->proxy_addr,
									  &ce->socks_host,
									  &ce->socks_port))
				{
					CD_PROXY_SERVER = ce->proxy_addr;
					return net_start_http_connect(ce);
				}
#endif	/* MOZILLA_CLIENT */
		  }

		if(CE_STATUS == MK_UNABLE_TO_CONNECT && CD_PROXY_SERVER)
		  {
			StrAllocCopy (CE_URL_S->error_msg,
						  XP_GetString(MK_UNABLE_TO_CONNECT_TO_PROXY));
			CE_STATUS = MK_UNABLE_TO_CONNECT_TO_PROXY;
		  }

        CD_NEXT_STATE = HTTP_ERROR_DONE;
        return STATUS(CE_STATUS);
      }

    if(CE_STATUS == MK_CONNECTED)
      {
        FE_ClearConnectSelect(CE_WINDOW_ID, CE_CON_SOCK);
        CE_CON_SOCK = SOCKET_INVALID;  /* reset the socket so we don't select on it  */
		/* set this so mkgeturl can select on it */
		CE_SOCK = cd->connection->sock;
#ifndef NO_SSL /* added by jwz */
		if(CD_USE_SSL_PROXY)
            CD_NEXT_STATE = HTTP_SEND_SSL_PROXY_REQUEST;
		else
#endif /* NO_SSL -- added by jwz */
		  if(ce->URL_s->files_to_post)
			CD_NEXT_STATE = HTTP_BEGIN_UPLOAD_FILE;
		else
            CD_NEXT_STATE = HTTP_SEND_REQUEST;
        FE_SetReadSelect(CE_WINDOW_ID, cd->connection->sock);
        TRACEMSG(("Connected to HTTP server on sock #%d",cd->connection->sock));

      } 
    else
      {
		/* unregister the old CE_SOCK from the select list
	 	 * and register the new value in the case that it changes
	 	 */
		if(ce->con_sock != cd->connection->sock)
	  	  {
        	FE_ClearConnectSelect(ce->window_id, ce->con_sock);
        	ce->con_sock = cd->connection->sock; 
        	FE_SetConnectSelect(ce->window_id, ce->con_sock);
	  	  }
	
        CD_PAUSE_FOR_READ = TRUE;
      } 

    return STATUS(CE_STATUS);
}

PRIVATE int
net_send_ssl_proxy_request (ActiveEntry *ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;
	char *host = NET_ParseURL(CE_URL_S->address, GET_HOST_PART);
	char *command=0;
	char *auth = NULL;

    StrAllocCopy(command, "CONNECT ");
	StrAllocCat(command, host);

	if(!XP_STRCHR(host, ':'))
      {
	    char small_buf[20];
		XP_SPRINTF(small_buf, ":%d", DEF_HTTPS_PORT);
		StrAllocCat(command, small_buf);
	  }

	StrAllocCat(command, " HTTP/1.0\n");
    
	/*
	 * Check if proxy is requiring authorization.
	 * If NULL, not necessary, or the proxy will return 407 to
	 * require authorization.
	 *
	 */
	if (NULL != (auth=NET_BuildProxyAuthString(CE_WINDOW_ID,
											   CE_URL_S,
											   CD_PROXY_SERVER)))
	  {
		  char *line = (char *)XP_ALLOC(strlen(auth) + 30);
		  if (line) {
			  XP_SPRINTF(line, "Proxy-authorization: %s%c%c", auth, CR, LF);
			  StrAllocCat(command, line);
			  XP_FREE(line);
		  }
		  CD_SENT_PROXY_AUTH = TRUE;
		  TRACEMSG(("HTTP: Sending proxy-authorization: %s", auth));
	  }
	else
	  {
		  TRACEMSG(("HTTP: Not sending proxy authorization (yet)"));
	  }

	{
		char line[200];
		XP_SPRINTF(line, "User-Agent: %.100s/%.90s" CRLF CRLF,
				   XP_AppCodeName, XP_AppVersion);
		StrAllocCat(command, line);
	}

	TRACEMSG(("Tx: %s", command));

    CE_STATUS = NET_HTTPNetWrite(cd->connection->sock, command, XP_STRLEN(command));

	FREE(command);

	CD_PAUSE_FOR_READ = TRUE;

	CD_NEXT_STATE = HTTP_PARSE_FIRST_LINE;

	return(CE_STATUS);
}

/* begin sending a file
 */
PRIVATE int
net_begin_upload_file (ActiveEntry *ce)
{
	HTTPConData * cd = (HTTPConData *) ce->con_data;
    char * filename;
    char * file_to_post;
	char * header=0;
	char * last_slash;
	char * status_msg;
	XP_StatStruct stat_entry;
	int i;

	if(ce->URL_s->server_status == 401)
	  {
		/* retrying with auth, don't change the filename */
		cd->next_state = HTTP_SEND_REQUEST;
		return(0);
	  }

	XP_ASSERT(ce->URL_s->files_to_post && ce->URL_s->files_to_post[0]);
	if(!ce->URL_s->files_to_post || !ce->URL_s->files_to_post[0])
		return MK_UNABLE_TO_LOCATE_FILE;

	/* get a filename from the files_to_post array
	 */
	for(i=0; ce->URL_s->files_to_post[i]; i++)
		; /* find the end */

	XP_ASSERT(i>0);

	file_to_post = ce->URL_s->files_to_post[i-1];

	/* zero the file now so that we don't upload it again. */
	ce->URL_s->files_to_post[i-1] = NULL;

#ifdef XP_MAC
    filename = XP_STRRCHR(file_to_post, '/');
#elif defined(XP_WIN)
    filename = XP_STRRCHR(file_to_post, '\\');
#else
    filename = XP_STRRCHR(file_to_post, '/');
#endif

    if(!filename)
        filename = file_to_post;
    else
        filename++; /* go past delimiter */

	/* get the size of the file */
	if(-1 == XP_Stat(file_to_post, &stat_entry, xpFileToPost))
	  {																						   
		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_LOCATE_FILE, file_to_post);
		FREE(file_to_post);
		return MK_UNABLE_TO_LOCATE_FILE;
	  }
		
	status_msg = PR_smprintf("Uploading file %s", filename);
	if(status_msg)
	  {
		NET_Progress(ce->window_id, status_msg);
		FREE(status_msg);
	  }		

#ifdef EDITOR
    FE_SaveDialogSetFilename(ce->window_id, filename);
#endif /* EDITOR */

	header = PR_smprintf("Content-Length: %ld" CRLF CRLF, stat_entry.st_size);


	FREEIF(ce->URL_s->post_headers);
	/* if header is null this won't crash */
	ce->URL_s->post_headers = header;

	/* strip the filename from the last slash and append the
	 * new filename to the URL
	 */
	last_slash = XP_STRRCHR(ce->URL_s->address, '/');
	if(last_slash)
		*last_slash = '\0';
	StrAllocCat(ce->URL_s->address, "/");
	StrAllocCat(ce->URL_s->address, filename);

	/* make the method PUT */
	ce->URL_s->method = URL_PUT_METHOD;

	/* specify the file to be uploaded */
	StrAllocCopy(ce->URL_s->post_data, file_to_post);

	/* specify that the post data is a file */
	ce->URL_s->post_data_is_file = TRUE;

	/* do the request */
	cd->next_state = HTTP_SEND_REQUEST;
	
	FREE(file_to_post);

	return(0);
}

/* build the HTTP request.
 *
 * mallocs a big string that must be free'd
 * returns the string
 */
PRIVATE unsigned int
net_build_http_request (URL_Struct * URL_s,
						FO_Present_Types format_out,
						Bool          http1, 
						char           * proxy_server, 
						Bool          posting, 
						MWContext      * window_id,
						char          ** command,
						Bool        * sent_authorization,
						Bool        * sent_proxy_auth)
{
    char line_buffer[512];  /* buffer */
    char *auth;         /* authorization header string */
	size_t csize;        /* size of command */
        
	/* begin the request
	 */
	switch(URL_s->method)
	  {
		case URL_MKDIR_METHOD:
#define MKDIR_WORD "MKDIR "
        	BlockAllocCopy(*command, MKDIR_WORD, 5);
			csize = sizeof(MKDIR_WORD)-1;
			break;

		case URL_DELETE_METHOD:
#define DELETE_WORD "DELETE "
#define RMDIR_WORD "RMDIR "
			if(URL_s->is_directory)
			  {
        		BlockAllocCopy(*command, RMDIR_WORD, 5);
				csize = sizeof(RMDIR_WORD)-1;
			  }
			else
			  {
        		BlockAllocCopy(*command, DELETE_WORD, 5);
				csize = sizeof(DELETE_WORD)-1;
			  }
			break;

		case URL_POST_METHOD:
#define POST_WORD "POST "
        	BlockAllocCopy(*command, POST_WORD, 5);
			csize = sizeof(POST_WORD)-1;
			break;

    	case URL_PUT_METHOD:
#define PUT_WORD "PUT "
        	BlockAllocCopy(*command, PUT_WORD, 4);
			csize = sizeof(PUT_WORD)-1;
			break;

    	case URL_HEAD_METHOD:
#define HEAD_WORD "HEAD "
        	BlockAllocCopy(*command, HEAD_WORD, 5);
			csize = sizeof(HEAD_WORD)-1;
			break;

		default:
			XP_ASSERT(0);
			/* fall through to GET */

    	case URL_GET_METHOD:
#define GET_WORD "GET "
        	BlockAllocCopy(*command, GET_WORD, 4);
			csize = sizeof(GET_WORD)-1;
			break;
	  }


    /* if we are using a proxy gateway copy in the entire URL
	 * minus the hash mark
     */
    if(proxy_server)
	  {
		char * url_minus_hash = NET_ParseURL(URL_s->address, GET_PROTOCOL_PART | GET_HOST_PART | GET_PATH_PART | GET_SEARCH_PART);
		if(url_minus_hash)
		{
		  BlockAllocCat(*command, (size_t) csize, url_minus_hash, XP_STRLEN(url_minus_hash));
		csize += XP_STRLEN(url_minus_hash);
			XP_FREE(url_minus_hash);
		}

	  }
    else 
	  {
		/* else use just the path and search stuff
		 */
    	char *path = NET_ParseURL(URL_s->address, 
										GET_PATH_PART | GET_SEARCH_PART);
        BlockAllocCat(*command, csize, path, XP_STRLEN(path));
		csize += XP_STRLEN(path);

    	XP_FREE(path);
	  }

    if(http1) 
      {
        BlockAllocCat(*command, csize, " ", 1);
		csize += 1;
        BlockAllocCat(*command, csize, 
					  VERSION_STRING, 
					  XP_STRLEN(VERSION_STRING));
		csize += XP_STRLEN(VERSION_STRING);
        /* finish the line */
        BlockAllocCat(*command, csize, CRLF, 2); /* CR LF, as in rfc 977 */
		csize += 2;

        if(URL_s->last_modified && URL_s->force_reload != NET_SUPER_RELOAD)
          {
            /* add if modified since
             *
             * right now this is being added even if the server sent pragma: no-cache
             * I'm not sure if this is the right thing to do but it seems
             * very efficient since we can still use the cache.
             */

	    /* add tmp character storage for strings in order for assert
             * to work properly in debug compile.
             * IBM compiler does not put "\" to make " work properly.
             */
            char *tmp_str = "http";
            {
               /* A conversion from time_t to NSPR's int64 is needed in order to
                * use the NSPR time functions.  URL_Struct should really be changed
                * to use the NSPR time type instead of time_t.
                */
               uint64   timeInSec;
               uint64	timeInUSec;
               uint64   secToUSec;
               PRTime	expandedTime;

               LL_I2L(timeInSec, URL_s->last_modified);
               LL_I2L(secToUSec, PR_USEC_PER_SEC);
               LL_MUL(timeInUSec, timeInSec, secToUSec);
#ifndef XP_MAC
               timeInUSec = PR_ToGMT(timeInUSec);
#endif /* XP_MAC */
               PR_ExplodeTime(&expandedTime, timeInUSec);
               PR_FormatTimeUSEnglish(line_buffer, 400,
                                      "If-Modified-Since: %A, %d-%b-%y %H:%M:%S GMT",
                                      &expandedTime);
            }

			/* must not be zero for http URL's 
			 * or else I screwed up the cache logic 
			 */
#ifndef AIX
			XP_ASSERT(strncasecomp(URL_s->address, tmp_str, 4)
					  || URL_s->real_content_length > 0);
#endif

			if(URL_s->real_content_length)
            	XP_SPRINTF(&line_buffer[XP_STRLEN(line_buffer)], 
							"; length=%ld" CRLF,
							URL_s->real_content_length);
            BlockAllocCat(*command, csize, line_buffer, XP_STRLEN(line_buffer));
			csize += XP_STRLEN(line_buffer);

            /* reset the expires since we will want to
             * either get a new one from the server or
             * set it to zero
             */
            URL_s->expires = 0;
          }

        if(URL_s->http_headers)  /* use headers that were passed in */
          {

            BlockAllocCat(*command, csize, URL_s->http_headers, XP_STRLEN(URL_s->http_headers));
			csize += XP_STRLEN(URL_s->http_headers);
          }
        else
          {
			if(URL_s->referer && strncasecomp(URL_s->referer, "mailbox:", 8))
			  {
				TRACEMSG(("Sending referer field"));
            	XP_STRCPY(line_buffer, "Referer: ");
                BlockAllocCat(*command, csize, line_buffer, XP_STRLEN(line_buffer));
				csize += XP_STRLEN(line_buffer);
                BlockAllocCat(*command,  csize, URL_s->referer,
							  XP_STRLEN(URL_s->referer));
				csize += XP_STRLEN(URL_s->referer);
                BlockAllocCat(*command, csize, CRLF, 2);
				csize += 2;
			  }

			assert (XP_AppCodeName);
			assert (XP_AppVersion);

			if(proxy_server)
            	XP_STRCPY(line_buffer, "Proxy-Connection: Keep-Alive" CRLF);
			else
            	XP_STRCPY(line_buffer, "Connection: Keep-Alive" CRLF);

            XP_SPRINTF (&line_buffer[XP_STRLEN(line_buffer)], 
						"User-Agent: %.100s/%.90s" CRLF,
						XP_AppCodeName, XP_AppVersion);
            BlockAllocCat(*command, csize, line_buffer, XP_STRLEN(line_buffer));
			csize += XP_STRLEN(line_buffer);

            if(URL_s->force_reload)
			  {
                BlockAllocCat(*command, csize, "Pragma: no-cache" CRLF, 18);
				csize += 18;
			  }

			if(URL_s->range_header)
			  {
				char *tmp_str = CRLF;
#ifndef AIX
				XP_ASSERT(!XP_STRSTR(URL_s->range_header, tmp_str));
#endif
#define REQUEST_RANGE_HEADER "Range: "
            	BlockAllocCat(*command, csize, 
							  REQUEST_RANGE_HEADER,
							  XP_STRLEN(REQUEST_RANGE_HEADER));
				csize += XP_STRLEN(REQUEST_RANGE_HEADER);
            	BlockAllocCat(*command, csize, 
							  URL_s->range_header,
							  XP_STRLEN(URL_s->range_header));
				csize += XP_STRLEN(URL_s->range_header);
            	BlockAllocCat(*command, csize, 
							  CRLF, XP_STRLEN(CRLF));
				csize += XP_STRLEN(CRLF);
			  }

#define OLD_RANGE_SUPPORT
#ifdef OLD_RANGE_SUPPORT
			/* this is here for backwards compatibility with
			 * the old range spec
			 * It should be removed before final beta 2
			 */
            if(URL_s->range_header)
              {
		char *tmp_str = CRLF;
#ifndef AIX
                XP_ASSERT(!XP_STRSTR(URL_s->range_header, tmp_str));
#endif
#undef REQUEST_RANGE_HEADER
#define REQUEST_RANGE_HEADER "Request-Range: "
                BlockAllocCat(*command, csize,
                              REQUEST_RANGE_HEADER,
                              XP_STRLEN(REQUEST_RANGE_HEADER));
                csize += XP_STRLEN(REQUEST_RANGE_HEADER);
                BlockAllocCat(*command, csize,
                              URL_s->range_header,
                              XP_STRLEN(URL_s->range_header));
                csize += XP_STRLEN(URL_s->range_header);
                BlockAllocCat(*command, csize,
                              CRLF, XP_STRLEN(CRLF));
                csize += XP_STRLEN(CRLF);
              }
#endif

			if(1)
			  {
    			char * host = NET_ParseURL(URL_s->address, GET_HOST_PART);

				if(host)
				  {
					int len;
					char *at_sign = XP_STRCHR(host, '@');

					/* strip authorization info if there is any
					 */
					if(at_sign)
						*at_sign = '\0';

#define HOST_HEADER "Host: "
                	BlockAllocCat(*command, 
									csize, 	
									HOST_HEADER, 	
									sizeof(HOST_HEADER)-1);
					csize += sizeof(HOST_HEADER)-1;

					if(at_sign)
					  {
						len = XP_STRLEN(at_sign+1);
                		BlockAllocCat(*command, csize, at_sign+1, len);
					  }
					else
					  {
						len = XP_STRLEN(host);
                		BlockAllocCat(*command, csize, host, len);
					  }
	
					csize += len;

					FREE(host);

                	BlockAllocCat(*command, csize, CRLF, 2);
					csize += 2;
				  }
			  }

#ifdef SEND_FROM_FIELD
            XP_SPRINTF(line_buffer, "From: %.256s%c%c", 
                			FE_UsersMailAddress() ? FE_UsersMailAddress() : "unregistered", CR,LF);
            BlockAllocCat(*command, csize, line_buffer, XP_STRLEN(line_buffer));
			csize += XP_STRLEN(line_buffer);
#endif /* SEND_FROM_FIELD */


			if(CLEAR_CACHE_BIT(format_out) != FO_INTERNAL_IMAGE)
			  {
				/* send Accept: *(slash)* as well as the others
				 */
				XP_SPRINTF(line_buffer, "Accept: %s, %s, %s, %s, */*" CRLF, 
						    IMAGE_GIF, IMAGE_XBM, IMAGE_JPG, IMAGE_PJPG);
			  }
			else
			  {
				XP_SPRINTF(line_buffer, "Accept: %s, %s, %s, %s" CRLF, 
						    IMAGE_GIF, IMAGE_XBM, IMAGE_JPG, IMAGE_PJPG);
			  }

            BlockAllocCat(*command, csize, line_buffer, XP_STRLEN(line_buffer));
			csize += XP_STRLEN(line_buffer);
			
#ifdef MOZILLA_CLIENT
#define SEND_ACCEPT_LANGUAGE 1
#ifdef SEND_ACCEPT_LANGUAGE	/* Added by ftang */
			{
				char *acceptlang = INTL_GetAcceptLanguage();
				if((acceptlang != NULL) && ( *acceptlang != '\0') )
				{
					XP_SPRINTF(line_buffer, "Accept-Language: %s" CRLF, 
							  	 acceptlang );
		            BlockAllocCat(*command, csize, line_buffer, XP_STRLEN(line_buffer));
					csize += XP_STRLEN(line_buffer);
				}
			}
#endif /* SEND_ACCEPT_LANGUAGE */
#endif /* MOZILLA_CLIENT */

			/*
			 * Check if proxy is requiring authorization.
			 * If NULL, not necessary, or the proxy will return 407 to
			 * require authorization.
			 *
			 */
            if (proxy_server &&
				NULL != (auth=NET_BuildProxyAuthString(window_id,
													   URL_s,
													   proxy_server)))
              {
				*sent_proxy_auth = TRUE;
                XP_SPRINTF(line_buffer, "Proxy-authorization: %.256s%c%c", auth, CR, LF);
            	BlockAllocCat(*command, csize, line_buffer, XP_STRLEN(line_buffer));
				csize += XP_STRLEN(line_buffer);
                TRACEMSG(("HTTP: Sending proxy-authorization: %s", auth));
              }
            else 
              {
                TRACEMSG(("HTTP: Not sending proxy authorization (yet)"));
              }

            /* NET_BuildAuthString will return non-NULL if authorization is
             * known to be required for this document.
             *
             * If NULL is returned authorization is not required or is
             * not known to be required yet so we will look for a 401
             * return code later on to see if authorization will be
             * neccessary
             */
            if (NULL!=(auth=NET_BuildAuthString(window_id, URL_s))) 
              {
				*sent_authorization = TRUE;
            	BlockAllocCat(*command, csize, auth, XP_STRLEN(auth));
				csize += XP_STRLEN(auth);
                TRACEMSG(("HTTP: Sending authorization: %s", auth));
              }
            else 
              {
                TRACEMSG(("HTTP: Not sending authorization (yet)"));
              }

            if (NULL!=(auth=NET_GetCookie(window_id, URL_s->address))) 
			  {
				int len;
                XP_STRCPY(line_buffer, "Cookie: ");
                BlockAllocCat(*command, csize, 
							  line_buffer, XP_STRLEN(line_buffer));
				
                csize += XP_STRLEN(line_buffer);
				len = XP_STRLEN(auth);
                BlockAllocCat(*command, csize, auth, len);
                csize += len;
                BlockAllocCat(*command, csize, CRLF, XP_STRLEN(CRLF));
                csize += XP_STRLEN(CRLF);
                TRACEMSG(("HTTP: Sending Cookie: %s", auth));
              }
            else
              {
                TRACEMSG(("HTTP: Not sending cookie"));
              }

          }  /* end of if passed in headers */
      }  /* end of if(http1) */

    /* end of Get/Post request */

	/* Blank line means "end" of headers
	 * If we are posting WritePostData will
	 * add the extra line feed for us
	 */
	if(!posting)
	  {
    	BlockAllocCat(*command, csize, CRLF, 2); 
		csize += 2;
	  }

    return((unsigned int) csize);
}

/* send_http_request  makes a connection to the http server
 * and sends a request (ethier http1 or http/.9) to the server
 *
 * returns the TCP status code
 */
#define SOCK_CHUNK_SIZE 1024
PRIVATE int
net_send_http_request (ActiveEntry *ce)
{
    /* assign it so that we have a typed structure pointer */
    HTTPConData * cd = (HTTPConData *)ce->con_data;
    char * command=0;
	unsigned int  command_size;

    /* we make the assumption that we are always acting as a proxy
     * server if we get http_headers passed in the URL structure.
     *
     * when acting as a proxy_server the clients headers are sent
     * to the server instead of being generated here and the headers
     * from the server are unparsed and sent on the the client.
     */
    if(ce->format_out == FO_OUT_TO_PROXY_CLIENT ||
			ce->format_out == FO_CACHE_AND_OUT_TO_PROXY_CLIENT)
        CD_ACTING_AS_PROXY=YES;

    TRACEMSG(("Entered \"send_http_request\""));

    /* we make the assumption that we will always use the POST
     * method if there is post data in the URL structure
     * Is that a bad assumption?
     */
    if(CE_URL_S->method == URL_POST_METHOD
		|| CE_URL_S->method == URL_PUT_METHOD) {
        CD_POSTING = YES;
#ifdef MOZILLA_CLIENT
#ifdef HAVE_SECURITY /* added by jwz */
		SECNAV_Posting(cd->connection->sock);
#endif /* !HAVE_SECURITY -- added by jwz */
#endif /* MOZILLA_CLIENT */
	} else if (CE_URL_S->method == URL_HEAD_METHOD) {
#ifdef MOZILLA_CLIENT
#ifdef HAVE_SECURITY /* added by jwz */
		SECNAV_HTTPHead(cd->connection->sock);
#endif /* !HAVE_SECURITY -- added by jwz */
#endif /* MOZILLA_CLIENT */
	}
	
    /* zero out associated mime fields in URL structure
	 *
 	 * all except content_length since that may be passed in
     */
    CE_URL_S->protection_template = 0;
    FREE_AND_CLEAR(CE_URL_S->redirecting_url);
    FREE_AND_CLEAR(CE_URL_S->authenticate);
    FREE_AND_CLEAR(CE_URL_S->proxy_authenticate);

    /* Build the request command.  (It must be free'd!!!) 
     *
     * build_http_request will assemble a string containing
     * the main request line, HTTP/1.0 MIME headers and
     * the post data (if it exits)
     */
    command_size = net_build_http_request(CE_URL_S,
								  CE_FORMAT_OUT,
								  CD_SEND_HTTP1, 
								  (CD_USE_SSL_PROXY ? NULL : CD_PROXY_SERVER),
								  CD_POSTING,
								  CE_WINDOW_ID,
								  &command,
								  &CD_SENT_AUTHORIZATION,
								  &CD_SENT_PROXY_AUTH);

    TRACEMSG(("Sending HTTP Request:\n---------------------------------"));

    CE_STATUS = (int) NET_BlockingWrite(cd->connection->sock, command, command_size);

#if defined(JAVA)
#if defined(DEBUG)
	if(MKLib_trace_flag)
	  {
		NET_NTrace("Tx: ", 4);
		NET_NTrace(command, command_size);
	  }
#endif /* DEBUG */
#endif /* JAVA */
    XP_FREE (command);  /* freeing the request */

	if (CE_STATUS < 0) 
	  {
        int err = SOCKET_ERRNO;

        if(err == XP_ERRNO_EWOULDBLOCK)
            return(1); /* continue */

        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_WRITE_ERROR, err);

        CD_NEXT_STATE= HTTP_ERROR_DONE;

        return(MK_TCP_WRITE_ERROR);
      }


	/* make sure these are empty
	 */
	FREE_AND_CLEAR(CD_LINE_BUFFER); /* reset */
	CD_LINE_BUFFER_SIZE=0;          /* reset */

	if(CD_POSTING
		&& CE_URL_S->post_data)
	  {
        FE_ClearReadSelect(CE_WINDOW_ID, cd->connection->sock);
        FE_SetConnectSelect(CE_WINDOW_ID, cd->connection->sock);
#ifdef XP_WIN
		cd->calling_netlib_all_the_time = TRUE;
        net_call_all_the_time_count++;
		FE_SetCallNetlibAllTheTime(CE_WINDOW_ID);
#endif /* XP_WIN */
		CE_CON_SOCK = cd->connection->sock;
    	CD_NEXT_STATE = HTTP_SEND_POST_DATA;
		return(0);
	  }
  
    CD_NEXT_STATE = HTTP_PARSE_FIRST_LINE;

	/* Don't pause for read any more because we need to do
	 * at least a single read right away to detect if the
	 * connection is bad.  Apparently windows will queue a
	 * write and not return a failure, but in fact the
	 * connection has already been closed by the server.
	 * Windows select will not even detect the exception
 	 * so we end up deadlocked.  By doing one read we
	 * can detect errors immediately
	 *
   	 * CD_PAUSE_FOR_READ = TRUE;
	 */

	{
		char * nonProxyHost = NET_ParseURL(CE_URL_S->address, GET_HOST_PART);
		if (nonProxyHost) {
			char* msg = PR_smprintf(XP_GetString(XP_PROGRESS_WAIT_REPLY),
									nonProxyHost);
			if (msg) {
				NET_Progress(CE_WINDOW_ID, msg);
				XP_FREE(msg);
			}
			XP_FREE(nonProxyHost);
		}
	}

    return STATUS(CE_STATUS);

} /* end of net_send_http_request */

PRIVATE int
net_http_send_post_data (ActiveEntry *ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;

	/* make sure the line buffer is large enough
	 * for us to use as a buffer
	 */
	if(cd->line_buffer_size < 200)
	  {
		FREEIF(cd->line_buffer);
		cd->line_buffer = (char*)XP_ALLOC(256);
		cd->line_buffer_size = 256;
	  }

    /* returns 0 on done and negative on error
     * positive if it needs to continue.
     */
    CE_STATUS = NET_WritePostData(CE_WINDOW_ID, CE_URL_S,
                                  cd->connection->sock,
								  &cd->write_post_data_data,
								  FALSE);

    cd->pause_for_read = TRUE;

    if(CE_STATUS == 0)
      {
        /* normal done
         */

		/* make sure these are empty
	 	 */
	 	FREE_AND_CLEAR(cd->line_buffer); /* reset */
		cd->line_buffer_size=0;          /* reset */

        FE_ClearConnectSelect(CE_WINDOW_ID, cd->connection->sock);
        FE_SetReadSelect(CE_WINDOW_ID, cd->connection->sock);
		CE_CON_SOCK = 0;

		{
			char * nonProxyHost = NET_ParseURL(CE_URL_S->address, GET_HOST_PART);
			if (nonProxyHost) {
				char* msg = PR_smprintf(XP_GetString(XP_PROGRESS_WAIT_REPLY),
										nonProxyHost);
				if (msg) {
					NET_Progress(CE_WINDOW_ID, msg);
					XP_FREE(msg);
				}
				XP_FREE(nonProxyHost);
			}
		}

    	cd->next_state = HTTP_PARSE_FIRST_LINE;
        return(0);
      }
	else if(cd->total_size_of_files_to_post && ce->status > 0)
	  {
	  	cd->total_amt_written += ce->status;

      	FE_GraphProgress(ce->window_id, 
						 ce->URL_s, 
						 cd->total_amt_written, 
						 ce->status, 
						 cd->total_size_of_files_to_post);
		FE_SetProgressBarPercent(ce->window_id, 
				cd->total_amt_written*100/cd->total_size_of_files_to_post);
	  }



    return(CE_STATUS);
}

/* parse_http_mime_headers
 *
 * parses the mime headers as they are received from the
 * HTTP server.
 *
 * Returns the TCP status code
 * 
 */

PRIVATE int 
net_parse_http_mime_headers (ActiveEntry *ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;
    char *line;
	char *value;

    CE_STATUS = NET_BufferedReadLine(cd->connection->sock, &line, &CD_LINE_BUFFER, 
                        			&CD_LINE_BUFFER_SIZE, &CD_PAUSE_FOR_READ);

    if(CE_STATUS < 0)
	  {
        NET_ExplainErrorDetails(MK_TCP_READ_ERROR, SOCKET_ERRNO);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
	  }

    if(CE_STATUS == 0)
	  {
		/* if this is set just return so that we can use
	 	 * the cached copy
	 	 */
		if(CD_USE_COPY_FROM_CACHE)
		  {
            /* clear the URL content fields so that
             * the 304 object doesn't effect the actual
             * cache file
             */
			if(!CE_URL_S->preset_content_type)
            	FREE_AND_CLEAR(CE_URL_S->content_type);
            CE_URL_S->content_length = 0;
            CE_URL_S->real_content_length = 0;
            CE_URL_S->last_modified = 0;
            FREE_AND_CLEAR(CE_URL_S->content_encoding);
            FREE_AND_CLEAR(CE_URL_S->content_name);
            CD_NEXT_STATE = HTTP_DONE;
			CD_PAUSE_FOR_READ = FALSE;
            return(MK_USE_COPY_FROM_CACHE);
		  }

		/* no mo data */
#ifndef NO_SSL /* added by jwz */
		if(CD_USE_SSL_PROXY && !CD_SSL_PROXY_SETUP_DONE)
		  {
			/* we have now successfully initiated a ssl proxy
			 * connection to a remote ssl host
			 *
			 * now we need to give the file descriptor
			 * to the ssl library and let it initiate
			 * a secure connection
			 * after that we need to send a normal
			 * http request
			 *
		 	 * ADD STUFF HERE KIPP!!!!
			 *
			 * fd is cd->connection->sock
			 */
			if(ce->URL_s->files_to_post)
				CD_NEXT_STATE = HTTP_BEGIN_UPLOAD_FILE;
			else
            	CD_NEXT_STATE = HTTP_SEND_REQUEST;
			CD_SSL_PROXY_SETUP_DONE = TRUE;
		  }
		else
#endif /* NO_SSL -- added by jwz */
		  if(ce->URL_s->files_to_post)
		  {
			if(ce->URL_s->files_to_post[0]
				&& ce->URL_s->server_status/100 == 2)
			  {
			  	if(ce->URL_s->can_reuse_connection)
				  {
					CD_NEXT_STATE = HTTP_BEGIN_UPLOAD_FILE;
				  }
				else
				  {
		            FE_ClearReadSelect(CE_WINDOW_ID, cd->connection->sock);
					NET_TotalNumberOfOpenConnections--;

					/* close the old connection
					 */
	            	NETCLOSE(cd->connection->sock);
					cd->connection->sock = SOCKET_INVALID;

			  		CD_NEXT_STATE = HTTP_START_CONNECT;
				  }
			  }
			else
			  {
				CD_NEXT_STATE = HTTP_DONE;
			  }
		  }
		else
		  {
            CD_NEXT_STATE = HTTP_SETUP_STREAM;
		  }
		CD_PAUSE_FOR_READ = FALSE;
        return(0);
	  }

    if(!line)
        return(0);  /* wait for next line */

    TRACEMSG(("parse_http_mime_headers: Parsing headers, got line: %s",line));

    if(CD_ACTING_AS_PROXY)
	  {
		/* save all the headers so we can pass them to the client
		 * when neccessary
		 */
		StrAllocCat(CD_SERVER_HEADERS, line);
		StrAllocCat(CD_SERVER_HEADERS, "\r\n");  /* add the \n back on */
	  }

    /* check for end of MIME headers */
    if(*line == '\0' || *line == '\r')
      {
        TRACEMSG(("Finished parsing MIME headers"));

		CD_PAUSE_FOR_READ = FALSE;

#ifndef NO_SSL /* added by jwz */
        if(CD_USE_SSL_PROXY && !CD_SSL_PROXY_SETUP_DONE)
          {
			if(CE_URL_S->server_status == 200)
			  {
				int rv;

                /* we have now successfully initiated a ssl proxy
                 * connection to a remote ssl host
                 *
                 * now we need to give the file descriptor
                 * to the ssl library and let it initiate
                 * a secure connection
                 * after that we need to send a normal
                 * http request
                 */
			    if (SSL_Enable(cd->connection->sock, SSL_SECURITY, 1) ||
				    SSL_ResetHandshake(cd->connection->sock, 0))
			    {
				    /* Loser */
				    int err = XP_GetError();
				    return err < 0 ? err : -err;
			    }

#ifdef MOZILLA_CLIENT
#ifdef HAVE_SECURITY /* added by jwz */
				rv = SECNAV_SetupSecureSocket(cd->connection->sock,
											  CE_URL_S->address,
											  CE_WINDOW_ID);
				if ( rv ) {
				    int err = XP_GetError();
				    return err < 0 ? err : -err;
				}
#endif /* !HAVE_SECURITY -- added by jwz */
#else
				XP_ASSERT(0);
#endif /* MOZILLA_CLIENT */
    
				if(ce->URL_s->files_to_post)
					CD_NEXT_STATE = HTTP_BEGIN_UPLOAD_FILE;
				else
            		CD_NEXT_STATE = HTTP_SEND_REQUEST;
			    CD_SSL_PROXY_SETUP_DONE = TRUE;
			  }
			else
			  {
                CD_NEXT_STATE = HTTP_SETUP_STREAM;
			  }
          }
		else
#endif /* NO_SSL -- added by jwz */
		  if(ce->URL_s->files_to_post
				&& ce->URL_s->server_status/100 == 2)
		  {
			if(ce->URL_s->files_to_post[0])
			  {
			  	if(ce->URL_s->can_reuse_connection)
				  {
					CD_NEXT_STATE = HTTP_BEGIN_UPLOAD_FILE;
				  }
				else
				  {
		            FE_ClearReadSelect(CE_WINDOW_ID, cd->connection->sock);
					NET_TotalNumberOfOpenConnections--;
				
				  	/* close the old connection
					 */
	            	NETCLOSE(cd->connection->sock);
					cd->connection->sock = SOCKET_INVALID;

			  		CD_NEXT_STATE = HTTP_START_CONNECT;
				  }
			  }
			else
			  {
				CD_NEXT_STATE = HTTP_DONE;
			  }
		  }
        else
          {
            CD_NEXT_STATE = HTTP_SETUP_STREAM;
          }

        return(0);
      }

	value = XP_STRCHR(line, ':');
	if(value)
		value++;
	NET_ParseMimeHeader(CE_WINDOW_ID, CE_URL_S, line, value);

    return(1);
}

/* parses the first line of the http server's response
 * and determines if it is an http1 or http/.9 server
 *
 * returns the tcp status code
 */
PRIVATE int 
net_parse_first_http_line (ActiveEntry *ce)
{
    char *line_feed=0;
	char *ptr;
	int line_size;
    int num_fields;
    char server_version[36];
    HTTPConData * cd = (HTTPConData *)ce->con_data;
	char small_buf[MAX_FIRST_LINE_SIZE+16];
	Bool do_redirect = TRUE;
	
	TRACEMSG(("Entered: net_parse_first_http_line"));

	CE_STATUS = NETREAD(cd->connection->sock, small_buf, MAX_FIRST_LINE_SIZE+10);

	if(CE_STATUS == 0)
	  {
		/* the server dropped the connection? */
	 	CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_ZERO_LENGTH_FILE);
       	CD_NEXT_STATE = HTTP_ERROR_DONE;
		CD_PAUSE_FOR_READ = FALSE;
		return(MK_ZERO_LENGTH_FILE);
	  }
	else if(CE_STATUS < 0)
	  { 
		int s_error = SOCKET_ERRNO;
		if (s_error == XP_ERRNO_EWOULDBLOCK)
		  {
			CD_PAUSE_FOR_READ = TRUE;
			return(0);
		  }
		else
		  {
            CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, s_error);

            /* return TCP error
             */
            return MK_TCP_READ_ERROR;
		  }
	  }

	/* this is where kipp says that I can finally query
     * for the security data.  We can't do it after the
	 * connect since the handshake isn't done yet...
	 */
    /* clear existing data
     */
    FREEIF(CE_URL_S->key_cipher);
    FREEIF(CE_URL_S->key_issuer);
    FREEIF(CE_URL_S->key_subject);
#ifdef HAVE_SECURITY /* added by jwz */
	if ( CE_URL_S->certificate ) {
	   CERT_DestroyCertificate(CE_URL_S->certificate);
	}
#endif /* !HAVE_SECURITY -- added by jwz */

#ifndef NO_SSL /* added by jwz */
    SSL_SecurityStatus(cd->connection->sock,
                       &CE_URL_S->security_on,
                       &CE_URL_S->key_cipher,
					   &CE_URL_S->key_size,
					   &CE_URL_S->key_secret_size,
                       &CE_URL_S->key_issuer,
                       &CE_URL_S->key_subject);
	CE_URL_S->certificate = SSL_PeerCertificate(cd->connection->sock);
#endif /* NO_SSL -- added by jwz */

#ifdef MOZILLA_CLIENT
#ifdef HAVE_SECURITY /* added by jwz */
	if ( ce->URL_s->redirecting_cert &&
		( ! (CD_USE_SSL_PROXY && !CD_SSL_PROXY_SETUP_DONE) ) ) {
		/* don't do the redirect check when talking to the proxy,
		 * wait for it to come through here a second time, when
		 * we are really using SSL to talk to the real server.
		 */

		PRBool compare = PR_TRUE;
		
		compare = CERT_CompareCertsForRedirection(ce->URL_s->certificate,
												  ce->URL_s->redirecting_cert);

		/* now that we are done with the redirecting cert destroy it */
		CERT_DestroyCertificate(ce->URL_s->redirecting_cert);
		ce->URL_s->redirecting_cert = NULL;
		
		if ( compare != PR_TRUE ) {
			/* certs are different */
		
			do_redirect = (Bool)SECNAV_SecurityDialog(CE_WINDOW_ID, 
										SD_REDIRECTION_TO_SECURE_SITE);
			if ( !do_redirect ) {
				/* stop connection here!! */
				CE_URL_S->error_msg = 0; /* XXX - is this right? */

				/* return TCP error
				 */
				return MK_TCP_READ_ERROR;
			}
		}
	}
#endif /* !HAVE_SECURITY -- added by jwz */
#endif /* MOZILLA_CLIENT */
	
	/* CE_STATUS greater than 0 
	 */
    BlockAllocCat(CD_LINE_BUFFER, CD_LINE_BUFFER_SIZE, small_buf, CE_STATUS);
    CD_LINE_BUFFER_SIZE += CE_STATUS;

	for(ptr = CD_LINE_BUFFER, line_size=0; line_size < CD_LINE_BUFFER_SIZE; ptr++, line_size++)
	    if(*ptr == LF)
	  	  {
			line_feed = ptr;
			break;
	  	  }

    /* assume HTTP/.9 until we know better 
	 */
    CD_RECEIVING_HTTP1 = FALSE;

    if(line_feed)
      {
        *server_version = 0;

		*line_feed = '\0';
    
        num_fields = sscanf(CD_LINE_BUFFER, "%20s %d", server_version, &CE_URL_S->server_status);
    
        TRACEMSG(("HTTP: Scanned %d fields from first_line: %s", num_fields, CD_LINE_BUFFER));
    
        /* Try and make sure this is an HTTP/1.0 reply
	 	 */
        if (num_fields == 2 || !XP_STRNCMP("HTTP/", server_version, 5))
          {
            /* HTTP1 */
            CD_RECEIVING_HTTP1 = TRUE;
          }

		/* put the line back the way it should be 
		 */
		*line_feed = LF;
      }
    else if(CD_LINE_BUFFER_SIZE < MAX_FIRST_LINE_SIZE)
      {
        return(0); /* not ready to process */
      }

	if(cd->connection->prev_cache && CD_RECEIVING_HTTP1 == FALSE)
 	  {
		/* got a non HTTP/1.0 or above response from
		 * server that must support HTTP/1.0 since it
		 * supports keep-alive.  The connection
		 * must be in a bad state now so
		 * restart the whole thang by going to the ERROR state
		 */
		CD_NEXT_STATE = HTTP_ERROR_DONE;
	  }

	/* if we are getting a successful read here
 	 * then the connection is valid
 	 */
	cd->connection_is_valid = TRUE;

    if(CD_RECEIVING_HTTP1 == FALSE)
      {  /* HTTP/0.9 */
		NET_cinfo * ctype;

		TRACEMSG(("Receiving HTTP/0.9"));
        
		CE_URL_S->content_length = 0;
		CE_URL_S->real_content_length = 0;
		FREE_AND_CLEAR(CE_URL_S->content_encoding);
		FREE_AND_CLEAR(CE_URL_S->content_name);

		if(!CE_URL_S->preset_content_type)
		  {
			FREE_AND_CLEAR(CE_URL_S->content_type);

         	/* fake the content_type since we can't get it */
         	ctype = NET_cinfo_find_type(CE_URL_S->address);

		 	/* treat unknown types as HTML
		  	 */
		 	if(ctype->is_default)
             	StrAllocCopy(CE_URL_S->content_type, TEXT_HTML);
		 	else
         	 	StrAllocCopy(CE_URL_S->content_type, ctype->type);

         	/* fake the content_encoding since we can't get it */
         	StrAllocCopy(CE_URL_S->content_encoding, 
						 (NET_cinfo_find_enc(CE_URL_S->address))->encoding);
		  }

		CD_NEXT_STATE = HTTP_SETUP_STREAM;

      } 
    else 
      {
        /* Decode full HTTP/1.0 response */
    
        TRACEMSG(("Receiving HTTP1 reply, status: %d", CE_URL_S->server_status));

		if(CE_URL_S->server_status != 304
			&& (!CD_USE_SSL_PROXY || CD_SSL_PROXY_SETUP_DONE))
	 	  {
			/* if we don't have a 304, zero all
			 * the content headers so that they
			 * don't interfere with the
			 * incoming object
			 *
			 * don't zero these in the case of a SSL proxy
			 * since this isn't the real request this
			 * is just the connection open request response
			 */
			if(!CE_URL_S->preset_content_type)
				FREE_AND_CLEAR(CE_URL_S->content_type);
			CE_URL_S->content_length = 0;
			CE_URL_S->real_content_length = 0;
			FREE_AND_CLEAR(CE_URL_S->content_encoding);
			FREE_AND_CLEAR(CE_URL_S->content_name);
		  }

        switch (CE_URL_S->server_status / 100) 
          {
              case 2:   /* Succesful reply */

				/* Since we
			 	 * are getting a new copy, delete the old one
			 	 * from the cache
			 	 */
#ifdef MOZILLA_CLIENT
				NET_RemoveURLFromCache(CE_URL_S);
#endif

                if((CE_URL_S->server_status == 204 
				    || CE_URL_S->server_status == 201)
				   && !CD_ACTING_AS_PROXY) 
				  {
					if(ce->URL_s->files_to_post && ce->URL_s->files_to_post[0])
					  {
					  	/* fall through since we need to get
						 * the headers to complete the
						 * file transfer
						 */
					  }
					else
					  {
                    	CE_STATUS = MK_NO_ACTION;
                      	CD_NEXT_STATE = HTTP_ERROR_DONE;
	                    return STATUS(CE_STATUS);
					  }
                  }
            	else if(cd->partial_cache_file
                   		&& ce->URL_s->range_header
                   		&& CE_URL_S->server_status != 206)
              	  {
                	/* we asked for a range and got back
                 	 * the whole document.  Something
                 	 * went wrong, error out.
                 	 */
               		CE_STATUS = MK_TCP_READ_ERROR;
					ce->URL_s->error_msg = NET_ExplainErrorDetails(
															MK_TCP_READ_ERROR, 
															XP_ERRNO_EIO);
					CD_NEXT_STATE = HTTP_ERROR_DONE;
					return(CE_STATUS);
              	  }

                break;
                
              case 3:   /* redirection and other such stuff */

				if(!CD_ACTING_AS_PROXY 
                     && (CE_URL_S->server_status == 301 || CE_URL_S->server_status == 302))
                  {
                   /* Supported within the HTTP module.  
				    * Will retry after mime parsing 
					*/
                    TRACEMSG(("Got Redirect code"));
                    CD_DOING_REDIRECT = TRUE;
                  }
                else if(CE_URL_S->server_status == 304)
                  {
					/* use the copy from the cache since it wasn't modified 
					 *
					 *  note: this will work with proxy clients too
					 */
        			if(CE_URL_S->last_modified)
					  {
#ifdef MOZILLA_CLIENT
                        /* check to see if we just now entered a secure space
                         *
                         * don't do if this is coming from history
                         * don't do this if about to redirect
                         */
                        if(CE_URL_S->security_on
                            && (CE_FORMAT_OUT == FO_CACHE_AND_PRESENT 
								|| CE_FORMAT_OUT == FO_PRESENT)
                            && !CE_URL_S->history_num)
                          {
                            History_entry * h = SHIST_GetCurrent(&CE_WINDOW_ID->hist);
                    
#ifdef HAVE_SECURITY /* added by jwz */
                            if(!h || !h->security_on)
                                SECNAV_SecurityDialog(CE_WINDOW_ID, 
													  SD_ENTERING_SECURE_SPACE);
#endif /* !HAVE_SECURITY -- added by jwz */
                          }
#endif /* MOZILLA_CLIENT */

					   	CD_USE_COPY_FROM_CACHE = TRUE;
                        /* no longer return since we need to parse
						 * headers from the server
						 *
						 * return(MK_USE_COPY_FROM_CACHE);  
						 */
					  }

					/* else continue since the server fucked up
					 * and sent us a 304 even without us having sent
					 * it an if-modified-since header
					 */
                  }
                break;
                
              case 4:    /* client error */

                if(CE_URL_S->server_status == 401 && !CD_ACTING_AS_PROXY)
                  {
                    /* never do this if acting as a proxy 
                     * If we are a proxy then just pass on 
					 * the headers and the document.
                     *
                     * if authorization_required is set we will check
                     * below after parsing the MIME headers to see
                     * if we should redo the request with an authorization
                     * string 
                     */
                    CD_AUTH_REQUIRED = TRUE;

					/* Since we
					 * are getting a new copy, delete the old one
					 * from the cache
					 */
#ifdef MOZILLA_CLIENT
					NET_RemoveURLFromCache(CE_URL_S);
#endif

					/* we probably want to cache this unless
					 * the user chooses not to authorize himself
					 */
                  }
				else if (CE_URL_S->server_status == 407 && !CD_ACTING_AS_PROXY)
				  {
					/* This happens only if acting as a client */
					CD_PROXY_AUTH_REQUIRED = TRUE;

					/* we probably want to cache this unless
					 * the user chooses not to authorize himself
					 */
				  }
				else
				  {
                	/* don't cache unless we have a succesful reply
                 	*/
                	TRACEMSG(("Server did not return success: NOT CACHEING!!"));
					CE_URL_S->dont_cache = TRUE;

                	/* all other codes should be displayed */
				  }
                break;
    
              case 5:    /* server error code */
                TRACEMSG(("Server did not return success: NOT CACHEING!!!"));
				CE_URL_S->dont_cache = TRUE;
#ifdef DO_503
				if(CE_URL_S->server_status == 503 && !CD_ACTING_AS_PROXY)
				  {
					CD_SERVER_BUSY_RETRY = TRUE;
				  }
#endif /* DO_503 */
				
                /* display the error results */
                break;
                
              default:  /* unexpected reply code */
                {
                  char message_buffer[256];
                  XP_SPRINTF(message_buffer,
							 XP_GetString(XP_ALERT_UNKNOWN_STATUS),
                             CE_URL_S->server_status);
                  FE_Alert(CE_WINDOW_ID, message_buffer);

                  /* don't cache unless we have a succesful reply
                   */
                  TRACEMSG(("Server did not return 200: NOT CACHEING!!!"));
				  CE_URL_S->dont_cache = TRUE;
                }
                break;
            } /* Switch on server_status/100 */

        CD_NEXT_STATE = HTTP_PARSE_MIME_HEADERS;

      } /* Full HTTP reply */

    return(0); /* good */
    
}

/* sets up the stream and performs special actions like redirect and
 * retry on authorization
 *
 * returns the tcp status code
 */
PRIVATE int
net_setup_http_stream(ActiveEntry * ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;
	XP_Bool need_to_do_again = FALSE;
	MWContext * stream_context;

    TRACEMSG(("NET_ProcessHTTP: setting up stream"));

	/* save this since it can be changed in lots
	 * of places.  This will be used for graph progress
	 * and to terminate the tranfer
	 */
	if(!ce->URL_s->high_range)
		cd->original_content_length = CE_URL_S->content_length;
	else
		cd->original_content_length = ce->URL_s->high_range 
									  - ce->URL_s->low_range 
									  + 1;

	if (ce->URL_s->method == URL_HEAD_METHOD) {
	  /* We only wanted the head, so we should stop doing anything else. */
	  CD_NEXT_STATE = HTTP_DONE;
	  return 0;
	}

	/* if this is set just return so that we can use
	 * the cached copy
	 */
	if(CD_USE_COPY_FROM_CACHE)
	  {
		/* if this is a partial cache file situation
		 * then we will keep going and switch later
		 * on in this file.
		 *
 		 * If it's not a partial cache file then
		 * leave the HTTP module and go to the
		 * file module to display the file
		 */
		if(!cd->partial_cache_file)
		  {
			/* clear the URL content fields so that
			 * the 304 object doesn't effect the actual
			 * cache file
			 */
			if(!CE_URL_S->preset_content_type)
				FREE_AND_CLEAR(CE_URL_S->content_type);
			CE_URL_S->content_length = 0;
			CE_URL_S->real_content_length = 0;
			FREE_AND_CLEAR(CE_URL_S->content_encoding);
			FREE_AND_CLEAR(CE_URL_S->content_name);
			CD_NEXT_STATE = HTTP_DONE;
			CD_PAUSE_FOR_READ = FALSE;
        	return(MK_USE_COPY_FROM_CACHE);  
		  }
		else
		  {
			/* set the correct content length so that
			 * the cache gets it right
			 */
			ce->URL_s->content_length = ce->URL_s->real_content_length;
		  }
	  }
		 
    /* do we need to start the tranfer over with authorization? 
     */
    if(CD_AUTH_REQUIRED)
	  {
		/* clear to prevent tight loop */
		int status;
        FE_ClearReadSelect(CE_WINDOW_ID, cd->connection->sock);
		status = NET_AskForAuthString(CE_WINDOW_ID, 
								CE_URL_S, 
								CE_URL_S->authenticate,
                                CE_URL_S->protection_template,
								CD_SENT_AUTHORIZATION);

		if(status == NET_RETRY_WITH_AUTH)
	    	need_to_do_again = TRUE;
		else
		    CE_URL_S->dont_cache = TRUE;

        FE_SetReadSelect(CE_WINDOW_ID, cd->connection->sock);
	  }
#if  defined(XP_WIN) && defined(MOZILLA_CLIENT)

#define COMPUSERVE_HEADER_NAME "Remote-Passphrase"

	else if(CE_URL_S->authenticate && !strncasecomp(CE_URL_S->authenticate, 
						 							COMPUSERVE_HEADER_NAME, 
						 							sizeof(COMPUSERVE_HEADER_NAME) - 1))
	  {
		/* compuserve auth requires us to send all authenticate
	 	 * headers into their code to verify the authentication
	 	 */
		int status = WFE_DoCompuserveAuthenticate(CE_WINDOW_ID, 
												  CE_URL_S, 
												  CE_URL_S->authenticate);

		if(status == NET_AUTH_FAILED_DONT_DISPLAY)
		  {												 
		  	CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_COMPUSERVE_AUTH_FAILED);

			return(MK_COMPUSERVE_AUTH_FAILED);			
		  }
	  }
#endif /* XP_WIN and MOZILLA_CLIENT */
			  	
    if(CD_PROXY_AUTH_REQUIRED)
	  {
		if(NET_AskForProxyAuth(CE_WINDOW_ID,
							   CD_PROXY_SERVER,
							   CE_URL_S->proxy_authenticate,
							   CD_SENT_PROXY_AUTH))
			need_to_do_again = TRUE;
		else
		    CE_URL_S->dont_cache = TRUE;
	  }

	if (need_to_do_again)
      {
        FE_ClearReadSelect(CE_WINDOW_ID, cd->connection->sock);
        NETCLOSE(cd->connection->sock);
		NET_TotalNumberOfOpenConnections--;
        cd->connection->sock = SOCKET_INVALID;
        CD_SEND_HTTP1 = TRUE;
        CD_AUTH_REQUIRED = FALSE;
		CD_PROXY_AUTH_REQUIRED = FALSE;
        CD_NEXT_STATE = HTTP_START_CONNECT;
		CE_URL_S->last_modified = 0;  /* clear this so we don't get 304 loopage */

		/* we don't know if the connection is valid anymore */
		cd->connection_is_valid = FALSE;

        /* clear the buffer 
         */ 
        FREE_AND_CLEAR(CD_LINE_BUFFER);
        CD_LINE_BUFFER_SIZE = 0;

		/* if necessary */
		FREE_AND_CLEAR(CD_SERVER_HEADERS);

        return(0); /* continue */
      }
    else if (CD_DOING_REDIRECT && CE_URL_S->redirecting_url &&
            /* try and prevent a circular loop. wont work for dual doc loop
             */
                XP_STRCMP(CE_URL_S->redirecting_url, CE_URL_S->address))
      {
		Bool do_redirect=TRUE;

#ifdef MOZILLA_CLIENT
		/* update the global history since we wont have the same
		 * url later
		 */
		GH_UpdateGlobalHistory(CE_URL_S);
#endif /* MOZILLA_CLIENT */

        if(CE_FORMAT_OUT == FO_CACHE_AND_PRESENT) {
			if(NET_IsURLSecure(CE_URL_S->address)) { /* current URL is secure*/
				if(NET_IsURLSecure(CE_URL_S->redirecting_url)) {
					/* new URL is secure */
					/* save the cert of the guy doing the redirect */
#ifdef HAVE_SECURITY /* added by jwz */
					ce->URL_s->redirecting_cert =
						CERT_DupCertificate(ce->URL_s->certificate);
#endif /* !HAVE_SECURITY -- added by jwz */
				} else { /* new URL is not secure */
					/* ask the user to confirm */
#ifdef HAVE_SECURITY /* added by jwz */
					do_redirect = (Bool)SECNAV_SecurityDialog(CE_WINDOW_ID, 
													SD_REDIRECTION_TO_INSECURE_DOC);
#endif /* !HAVE_SECURITY -- added by jwz */
				}
			}
          }

        /* OK, now we've got the redirection URL stored
         * in redirecting_url.
         * Now change the URL in the URL structure and do the whole
         * thing again
         */
        StrAllocCopy(CE_URL_S->address, CE_URL_S->redirecting_url);
		FREE_AND_CLEAR(CE_URL_S->post_data);
		FREE_AND_CLEAR(CE_URL_S->post_headers);
		CE_URL_S->post_data_size = 0;
		CE_URL_S->method = URL_GET_METHOD;

		/* clear these */
		if(!CE_URL_S->preset_content_type)
    		FREE_AND_CLEAR(CE_URL_S->content_type);
    	FREE_AND_CLEAR(CE_URL_S->content_encoding);
		CE_URL_S->content_length = 0;       /* reset */
		CE_URL_S->real_content_length = 0;  /* reset */
		CE_URL_S->last_modified = 0;        /* reset */

		CD_POSTING = FALSE;

        /* free all this stuff */
        FREE_AND_CLEAR(CE_URL_S->post_data);

        CE_URL_S->address_modified = TRUE;

		if(do_redirect)
            return(MK_DO_REDIRECT); /*fall out of HTTP and load the redirecting url */
		else
			return(MK_INTERRUPTED);
      }
	else if(CD_SERVER_BUSY_RETRY)
	  {
		/* the redirecting mechanism works well for the retry
		 * since it is really the same thing except the url stays
		 * the same
		 */
        return(MK_DO_REDIRECT); /* fall out of HTTP and reload the url */
	  }

#ifdef MOZILLA_CLIENT
    /* check to see if we just now entered a secure space
     *
     * don't do if this is coming from history
     * don't do this if about to redirect
     */
    if(CE_URL_S->security_on
		&& (CE_FORMAT_OUT == FO_CACHE_AND_PRESENT || CE_FORMAT_OUT == FO_PRESENT)
        && !CE_URL_S->history_num)
      {
        History_entry * h = SHIST_GetCurrent(&CE_WINDOW_ID->hist);

#ifdef HAVE_SECURITY /* added by jwz */
        if(!h || !h->security_on)
            SECNAV_SecurityDialog(CE_WINDOW_ID, SD_ENTERING_SECURE_SPACE);
#endif /* !HAVE_SECURITY -- added by jwz */
      }
#endif /* MOZILLA_CLIENT */

    /* set a default content type if one wasn't given 
     */
    if(!CE_URL_S->content_type
		|| !*CE_URL_S->content_type)
        StrAllocCopy(CE_URL_S->content_type, TEXT_HTML);

	/* If a stream previously exists from a partial cache
	 * situation, reuse it
	 */
	if(!CD_STREAM)
	  {
		/* clear to prevent tight loop */
		FE_ClearReadSelect(ce->window_id, cd->connection->sock);

#ifdef MOZILLA_CLIENT
		/* if the context can't handle HTML then we
		 * need to generate an HTML dialog to handle
		 * the message
		 */
		if(ce->URL_s->files_to_post && EDT_IS_EDITOR(ce->window_id))
		  {
			Chrome chrome_struct;

		    XP_MEMSET(&chrome_struct, 0, sizeof(Chrome));

			
   			chrome_struct.is_modal = TRUE;
   			chrome_struct.allow_close = TRUE;
   			chrome_struct.allow_resize = TRUE;
   			chrome_struct.show_scrollbar = TRUE;
   			chrome_struct.w_hint = 400;
   			chrome_struct.h_hint = 300;
/*   			chrome_struct.topmost = TRUE;*/
        

			stream_context = FE_MakeNewWindow(ce->window_id, 
												  NULL, 
												  NULL, 
												  &chrome_struct);
			if(!stream_context)
        		return (MK_OUT_OF_MEMORY);

			/* zero out the post_data field so that it doesn't get
			 * pushed onto the history stack.  Otherwise it can
			 * get deleted when the history gets cleared
			 */
			FREE_AND_CLEAR(ce->URL_s->post_data);
			ce->URL_s->post_data_is_file = FALSE;
		  }
		else
#endif /* MOZILLA_CLIENT */
		  {
			stream_context = CE_WINDOW_ID;
		  }

		/* we can get here on server or proxy errors
		 * if we proceed to build the stream with post_data
		 * set then the file could get deleted by history
		 * cleanup code.  Make sure we zero the field
		 */
        if(ce->URL_s->files_to_post && ce->URL_s->post_data)
          {
            /* we shoved the file to post into the post data.
             * remove it so the history doesn't get confused
             * and try and delete the file.
             */
            FREE_AND_CLEAR(ce->URL_s->post_data);
            ce->URL_s->post_data_is_file = FALSE;
          }

    	/* Set up the stream stack to handle the body of the message */
#ifdef FORTEZZA
		if (CE_URL_S->dont_cache) CE_FORMAT_OUT=CLEAR_CACHE_BIT(CE_FORMAT_OUT);
#endif
    	CD_STREAM = NET_StreamBuilder(CE_FORMAT_OUT, 
									  CE_URL_S, 
									  stream_context);

    	if (!CD_STREAM)
      	  {
        	CE_STATUS = MK_UNABLE_TO_CONVERT;
			CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CONVERT);
        	return STATUS(CE_STATUS);
      	  }

#ifdef FORTEZZA
	    if (XP_STRCMP(CD_STREAM->name, "preencrypted stream") == 0) {
           SEC_SetPreencryptedSocket(CD_STREAM->data_object, CE_SOCK);
		}
#endif

		FE_SetReadSelect(CE_WINDOW_ID, cd->connection->sock);

		if(ce->URL_s->files_to_post)
		  {
		  	char * tmp_string = XP_STRDUP("<h2>Error uploading files</h2><b>The server responded:<b><hr><p>\n");

			if(tmp_string)
				PUTSTRING(tmp_string);
		  } 
	  }

	if(CD_USE_COPY_FROM_CACHE)
	  {
		/* we can only get here if it's a partial cache file */
    	CD_NEXT_STATE = HTTP_BEGIN_PUSH_PARTIAL_CACHE_FILE;
		CD_USE_COPY_FROM_CACHE = FALSE;
	  }
	else
	  {
		/* start the graph progress indicator
	 	 */
    	FE_GraphProgressInit(CE_WINDOW_ID, 
							 CE_URL_S, 
							 cd->original_content_length);
		CD_DESTROY_GRAPH_PROGRESS = TRUE;  /* we will need to destroy it */

   		CD_NEXT_STATE = HTTP_PULL_DATA;

    	if(CD_ACTING_AS_PROXY && CD_SERVER_HEADERS)
	  	  {
			CE_STATUS = PUTBLOCK(CD_SERVER_HEADERS, 
								 XP_STRLEN(CD_SERVER_HEADERS));
			CD_DISPLAYED_SOME_DATA = TRUE;
	  	  }

		{
			char * nonProxyHost = NET_ParseURL(CE_URL_S->address, GET_HOST_PART);
			if (nonProxyHost) {
				char* msg = PR_smprintf(XP_GetString(XP_PROGRESS_TRANSFER_DATA),
										nonProxyHost);
				if (msg) {
					NET_Progress(CE_WINDOW_ID, msg);
					XP_FREE(msg);
				}
				XP_FREE(nonProxyHost);
			}
		}

    	/* Push though buffered data */
		if(CD_LINE_BUFFER_SIZE)
	  	  {
			/* @@@ bug, check return status and only send
		 	* up to the return value
		 	*/
    		(*CD_STREAM->is_write_ready)(CD_STREAM->data_object);
        	CE_STATUS = PUTBLOCK(CD_LINE_BUFFER, CD_LINE_BUFFER_SIZE);
			CE_BYTES_RECEIVED = CD_LINE_BUFFER_SIZE;
        	FE_GraphProgress(CE_WINDOW_ID, 
						 	CE_URL_S, 
						 	CE_BYTES_RECEIVED, 
						 	CD_LINE_BUFFER_SIZE, 
						 	CD_ORIGINAL_CONTENT_LENGTH);
			CD_DISPLAYED_SOME_DATA = TRUE;
	  	  }
	  }

    FREE_AND_CLEAR(CD_LINE_BUFFER);
    CD_LINE_BUFFER_SIZE=0;

	/* check to see if we have read the whole object,
	 * and finish the transfer if so.
	 */
    if(CE_STATUS > -1
	   && CD_ORIGINAL_CONTENT_LENGTH
       && CE_BYTES_RECEIVED >= CD_ORIGINAL_CONTENT_LENGTH)
      {
        /* normal end of transfer */
        CE_STATUS = MK_DATA_LOADED;
        CD_NEXT_STATE = HTTP_DONE;
        CD_PAUSE_FOR_READ = FALSE;
      }

    return STATUS(CE_STATUS);
}

/* begin pushing a partial cache file down the stream
 */
PRIVATE int
net_http_push_partial_cache_file(ActiveEntry *ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;
	int32 write_ready, status;
	
	write_ready = (*cd->stream->is_write_ready)(cd->stream->data_object);

	write_ready = MIN(write_ready, NET_Socket_Buffer_Size);

	status = XP_FileRead(NET_Socket_Buffer, write_ready, cd->partial_cache_fp);

	if(status < 0)
	  {
		/* @@@ This is the wrong error code
		 */
		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, 
													   SOCKET_ERRNO);
		return(MK_TCP_READ_ERROR);
	  }
	else if(status == 0)	
	  {
		/* all done with reading this file
		 */
		FE_ClearFileReadSelect(ce->window_id, XP_Fileno(cd->partial_cache_fp));
		XP_FileClose(cd->partial_cache_fp);
		cd->partial_cache_fp = NULL;

		/* set these back in preperation for
		 * using the http connection again
	 	 */
		ce->socket = cd->connection->sock;
		ce->local_file = FALSE;

		/* add a request-range header
		 */
		XP_ASSERT(!ce->URL_s->range_header);
		ce->URL_s->range_header = PR_smprintf("bytes=%ld-", 
											  cd->partial_needed);

		/* the byterange part has not been gotten yet */
		ce->URL_s->last_modified = 0;

		/* we don't know if the connection is valid anymore 
		 * because we are going to try it again
		 */
		cd->connection_is_valid = FALSE;

		if(ce->URL_s->can_reuse_connection)
		  {
			FE_SetReadSelect(ce->window_id, cd->connection->sock);
			cd->next_state = HTTP_SEND_REQUEST;

			/* set the connection to be from the connection cache
		 	 * since we have used it once
		 	 */
			cd->connection->prev_cache = TRUE;
		  }
		else
		  {
            FE_ClearReadSelect(CE_WINDOW_ID, cd->connection->sock);
            FE_ClearConnectSelect(CE_WINDOW_ID, cd->connection->sock);
            NETCLOSE(cd->connection->sock);
			NET_TotalNumberOfOpenConnections--;
			cd->connection->sock = SOCKET_INVALID;
			cd->next_state = HTTP_START_CONNECT;
		  }

		cd->reuse_stream = TRUE;

		return(0);
	  }

	/* else, push the data read up the stream 
	 */	
	status = (*cd->stream->put_block)(cd->stream->data_object, 
									  NET_Socket_Buffer, 
									  status);

	cd->pause_for_read = TRUE;

	return(status);
}

/* begin pushing a partial cache file down the stream
 */
PRIVATE int
net_http_begin_push_partial_cache_file(ActiveEntry *ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;
	char *cache_file = ce->URL_s->cache_file;
	XP_File fp;

	if(!cache_file 
	   || NULL == (fp = XP_FileOpen(cache_file, xpCache, XP_FILE_READ_BIN)))
	  {
		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_OPEN_FILE, 
													   cache_file);
		return(MK_UNABLE_TO_OPEN_FILE);
	  }

	/* set up read select on the file instead of the socket
	 */
	FE_ClearReadSelect(ce->window_id, cd->connection->sock);
	FE_SetFileReadSelect(ce->window_id, XP_Fileno(fp));
	ce->socket = XP_Fileno(fp);
	ce->local_file = TRUE;

	cd->next_state = HTTP_PUSH_PARTIAL_CACHE_FILE;

	cd->partial_cache_fp = fp;
		
	return(net_http_push_partial_cache_file(ce));
}

/* pulls down all the data
 *
 * returns the tcp status code
 */
PRIVATE int
net_pull_http_data(ActiveEntry * ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;
    unsigned int write_ready, read_size;

    TRACEMSG(("NET_ProcessHTTP: pulling data"));

    /* check to see if the stream is ready for writing
	 */
    write_ready = (*CD_STREAM->is_write_ready)(CD_STREAM->data_object);

	if(!write_ready)
	  {
		CD_PAUSE_FOR_READ = TRUE;
		return(0);  /* wait until we are ready to write */
	  }
	else if(write_ready < (unsigned int) NET_Socket_Buffer_Size)
	  {
		read_size = write_ready;
	  }
	else
	  {
		read_size = NET_Socket_Buffer_Size;
	  }

    CE_STATUS = NETREAD(cd->connection->sock, NET_Socket_Buffer, read_size);

    CD_PAUSE_FOR_READ = TRUE;  /* pause for the next read */

    if(CE_STATUS > 0)
      {
        CE_BYTES_RECEIVED += CE_STATUS;
        FE_GraphProgress(CE_WINDOW_ID, 
						 CE_URL_S, 
						 CE_BYTES_RECEIVED, 
						 CE_STATUS, 
						 CD_ORIGINAL_CONTENT_LENGTH);

        CE_STATUS = PUTBLOCK(NET_Socket_Buffer, CE_STATUS); /* ALEKS */
		CD_DISPLAYED_SOME_DATA = TRUE;

		if(CD_ORIGINAL_CONTENT_LENGTH 
			&& CE_BYTES_RECEIVED >= CD_ORIGINAL_CONTENT_LENGTH)
		  {
			/* normal end of transfer */
			CE_STATUS = MK_DATA_LOADED;
         	CD_NEXT_STATE = HTTP_DONE;
         	CD_PAUSE_FOR_READ = FALSE;
		  }

      }
    else if(CE_STATUS == 0)
      {
		/* transfer finished
         */
	    TRACEMSG(("MKHTTP.c: Caught TCP EOF ending stream"));

		if(!CD_DISPLAYED_SOME_DATA)
		  {
		 	CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_ZERO_LENGTH_FILE);
         	CD_NEXT_STATE = HTTP_ERROR_DONE;
			CD_PAUSE_FOR_READ = FALSE;
			return(MK_ZERO_LENGTH_FILE);
		   }

		 /* return the server status instead of data loaded
		  */
         CE_STATUS = MK_DATA_LOADED;
         CD_NEXT_STATE = HTTP_DONE;
         CD_PAUSE_FOR_READ = FALSE;
      }
	else /* error */
	  {
		int err = SOCKET_ERRNO;

		TRACEMSG(("TCP Error: %d", err));

		if (err == XP_ERRNO_EWOULDBLOCK)
		  {
			CD_PAUSE_FOR_READ = TRUE;
			return (0);
		  }

        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, err);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
	  }

     return STATUS(CE_STATUS);

}

/* begin loading an object from an http host
 *
 * proxy_server  If not empty, the name of a host and/or port that will
 *               act as a proxy server for the request.  If proxy_server
 *               is NULL then no proxy server is used
 *
 */
MODULE_PRIVATE int 
NET_HTTPLoad (ActiveEntry * ce)
{
    /* get memory for Connection Data */
    HTTPConData * cd = XP_NEW(HTTPConData);
	XP_Bool url_is_secure = FALSE;
	char *use_host;

    ce->con_data = cd;
    if(!ce->con_data)
      {
        CE_STATUS = MK_OUT_OF_MEMORY;
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
        return STATUS(CE_STATUS);
      }

    /* init */
    XP_MEMSET(cd, 0, sizeof(HTTPConData));
    CD_PROXY_SERVER     = ce->proxy_addr;
	CD_PROXY_CONF       = ce->proxy_conf;
    CD_SEND_HTTP1       = TRUE;
    CD_RECEIVING_HTTP1  = TRUE;

	/* set partial_cache_file if the whole file is not 
	 * cached
	 */
	if(ce->URL_s->content_length < ce->URL_s->real_content_length)
	  {
		cd->partial_cache_file = TRUE;
		cd->partial_needed = ce->URL_s->content_length;

#ifdef MOZILLA_CLIENT
		/* if this isn't true then partial cacheing is screwed */
		XP_ASSERT(NET_IsPartialCacheFile(ce->URL_s));
#else
		XP_ASSERT(0);
#endif /* MOZILLA_CLIENT */
	  }
  
	if(CD_PROXY_SERVER)
	  {
		use_host = XP_STRDUP(CD_PROXY_SERVER);
	  }
	else
	  {
		use_host = NET_ParseURL(ce->URL_s->address, GET_HOST_PART);
#ifndef NO_SSL /* added by jwz */
		if(!strncasecomp(ce->URL_s->address, "https:", 6))
			url_is_secure = TRUE;
#endif /* NO_SSL */
	  }

	if(!use_host)
 	  {
    	FREE(ce->con_data);
        CE_STATUS = MK_OUT_OF_MEMORY;
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
        return STATUS(CE_STATUS);
	  }

	/* if we are useing the files_to_post method
	 * make sure that the directory specified in the
	 * URL contains a slash at the end, otherwise
	 * it wont work
	 */
	if(ce->URL_s->files_to_post)
	  {
		int32 end = XP_STRLEN(ce->URL_s->address)-1;
		XP_StatStruct stat_entry;
		int i;

		if(ce->URL_s->address[end] != '/')
			StrAllocCat(ce->URL_s->address, "/");

		/* run through the list of files and
		 * gather the total size
		 */
		for(i=0; ce->URL_s->files_to_post[i]; i++)
			if(-1 != XP_Stat(ce->URL_s->files_to_post[i], 
							 &stat_entry,
							 xpFileToPost))
				cd->total_size_of_files_to_post += stat_entry.st_size;

		/* start the graph progress indicator
	 	 */
    	FE_GraphProgressInit(ce->window_id, 
							 ce->URL_s, 
							 cd->total_size_of_files_to_post);
                                                    
		CD_DESTROY_GRAPH_PROGRESS = TRUE;  /* we will need to destroy it */
                                                                           
#ifdef EDITOR
		/* Don't show the dialog if the data is being delivered to a plug-in */
		if (CLEAR_CACHE_BIT(ce->format_out) != FO_PLUGIN)
		{
	        FE_SaveDialogCreate(ce->window_id, i, TRUE);
	        cd->destroy_file_upload_progress_dialog = TRUE;
	    }
#endif /* EDITOR */
	  }

    /* check for established connection and use it if available
     */
    if(http_connection_list)
      {
        HTTPConnection * tmp_con;
        XP_List * list_entry = http_connection_list;

  
        while((tmp_con = (HTTPConnection *)XP_ListNextObject(list_entry))
              != NULL)
          {
            /* if the hostnames match up exactly 
			 * and security matches up.
			 * and the connection
             * is not busy at the moment then reuse this connection.
             */
            if(!XP_STRCMP(tmp_con->hostname, use_host)
               && url_is_secure == tmp_con->secure
               && !tmp_con->busy)
              {
                cd->connection = tmp_con;
                cd->connection->sock = cd->connection->sock;
                FE_SetReadSelect(CE_WINDOW_ID, cd->connection->sock);
				CE_SOCK = cd->connection->sock;
                cd->connection->prev_cache = TRUE;  /* this was from the cache */
				TRACEMSG(("Found cached HTTP connection: %d", cd->connection->sock));

				/* reorder the connection in the list to keep most recently
				 * used connections at the end
				 */
				XP_ListRemoveObject(http_connection_list, tmp_con);
				XP_ListAddObjectToEnd(http_connection_list, tmp_con);

                break;
              }
          }
      }
    else
      {
        /* initialize the connection list
         */
        http_connection_list = XP_ListNew();
      }

    if(cd->connection)
      {
		if(CD_USE_SSL_PROXY)
            CD_NEXT_STATE = HTTP_SEND_SSL_PROXY_REQUEST;
		else if(ce->URL_s->files_to_post)
			CD_NEXT_STATE = HTTP_BEGIN_UPLOAD_FILE;
		else
			CD_NEXT_STATE = HTTP_SEND_REQUEST;

        /* set the connection busy
         */
        cd->connection->busy = TRUE;
        NET_TotalNumberOfOpenConnections++;
      }
    else
      {
        /* build a control connection structure so we
         * can store the data as we go along
         */
        cd->connection = XP_NEW(HTTPConnection);
        if(!cd->connection)
          {
            CE_STATUS = MK_OUT_OF_MEMORY;
			CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
			FREE(use_host);
			FREE(ce->con_data);
			return(-1);
          }
        XP_MEMSET(cd->connection, 0, sizeof(HTTPConnection));
  
        StrAllocCopy(cd->connection->hostname, use_host);
  
        cd->connection->secure = url_is_secure;
  
        cd->connection->prev_cache = FALSE;  /* this wasn't from the cache */
  
        cd->connection->sock = SOCKET_INVALID;
  
        /* add this structure to the connection list even
         * though it's not really valid yet.
         * we will fill it in as we go and if
         * an error occurs will will remove it from the
         * list.  No one else will be able to use it since
         * we will mark it busy.
         */
        XP_ListAddObject(http_connection_list, cd->connection);

        /* set the connection busy
         */
        cd->connection->busy = TRUE;

		/* if the connection list is larger than MAX_CACHED_HTTP_CONNECTIONS 
		 * trim it down
		 */
		if(XP_ListCount(http_connection_list) > MAX_CACHED_HTTP_CONNECTIONS)
		  {
			HTTPConnection *tmp_con;
        	XP_List * list_entry = http_connection_list;

			TRACEMSG(("More than %d cached connections.  Deleteing one...",
					  MAX_CACHED_HTTP_CONNECTIONS));
	
        	while((tmp_con = (HTTPConnection *)XP_ListNextObject(list_entry)) != NULL)
          	  {
				if(!tmp_con->busy)
				  {
					XP_ListRemoveObject(http_connection_list, tmp_con);
            		NETCLOSE(tmp_con->sock);
					FREE(tmp_con->hostname);
					FREE(tmp_con);
					break;
				  }
			  }
		  }
  
		CD_NEXT_STATE = HTTP_START_CONNECT;

      }

	FREE(use_host);

    return STATUS (NET_ProcessHTTP(ce));
}


/* NET_process_HTTP  will control the state machine that
 * loads an HTTP document
 *
 * returns negative if the transfer is finished or error'd out
 *
 * returns zero or more if the transfer needs to be continued.
 */
MODULE_PRIVATE int 
NET_ProcessHTTP (ActiveEntry *ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;

    TRACEMSG(("Entering NET_ProcessHTTP"));

    CD_PAUSE_FOR_READ = FALSE; /* already paused; reset */

    while(!CD_PAUSE_FOR_READ)
    {

        switch(CD_NEXT_STATE)
        {
        case HTTP_START_CONNECT:
            CE_STATUS = net_start_http_connect(ce);
            break;
        
        case HTTP_FINISH_CONNECT:
            CE_STATUS = net_finish_http_connect(ce);
            break;
        
#ifndef NO_SSL /* added by jwz */
	    case HTTP_SEND_SSL_PROXY_REQUEST:
			/* send ssl proxy init stuff
			 */
            CE_STATUS = net_send_ssl_proxy_request(ce);
            break;
#endif /* NO_SSL -- added by jwz */

		case HTTP_BEGIN_UPLOAD_FILE:
			/* form a put request */
			ce->status = net_begin_upload_file (ce);
			break;

        case HTTP_SEND_REQUEST:
            /* send HTTP request */
            CE_STATUS = net_send_http_request(ce);
            break;

		case HTTP_SEND_POST_DATA:
            CE_STATUS = net_http_send_post_data(ce);
            break;
        
        case HTTP_PARSE_FIRST_LINE:
            CE_STATUS = net_parse_first_http_line(ce);
            break;
        
        case HTTP_PARSE_MIME_HEADERS:
            CE_STATUS = net_parse_http_mime_headers(ce);
            break;
        
        case HTTP_SETUP_STREAM:
            CE_STATUS = net_setup_http_stream(ce);
            break;

		case HTTP_BEGIN_PUSH_PARTIAL_CACHE_FILE:
			ce->status = net_http_begin_push_partial_cache_file(ce);
			break;

		case HTTP_PUSH_PARTIAL_CACHE_FILE:
			ce->status = net_http_push_partial_cache_file(ce);
			break;
        
        case HTTP_PULL_DATA:
            CE_STATUS = net_pull_http_data(ce);
            break;
        
        case HTTP_DONE:
            FE_ClearReadSelect(CE_WINDOW_ID, cd->connection->sock);
			NET_TotalNumberOfOpenConnections--;

			if(ce->URL_s->can_reuse_connection && !CD_USE_SSL_PROXY)
			  {
				cd->connection->busy = FALSE;
			  }
			else
			  {
            	NETCLOSE(cd->connection->sock);

            	/* remove the connection from the cache list
             	 * and free the data
             	 */
                XP_ListRemoveObject(http_connection_list, cd->connection);
                if(cd->connection)
                  {
                    FREEIF(cd->connection->hostname);
                    FREE(cd->connection);
                  }
			  }

#ifdef MOZILLA_CLIENT
			/* make any meta tag changes take effect
		 	 */
         	NET_RefreshCacheFileExpiration(CE_URL_S);  
#endif /* MOZILLA_CLIENT */

			if(CD_STREAM)
			  {
            	COMPLETE_STREAM;
				FREE(CD_STREAM);
				CD_STREAM = 0;
			  }
            CD_NEXT_STATE = HTTP_FREE;
            break;
        
        case HTTP_ERROR_DONE:
            if(cd->connection->sock != SOCKET_INVALID)
              {
#if defined(XP_WIN) || defined(XP_UNIX)
				  FE_ClearDNSSelect(CE_WINDOW_ID, cd->connection->sock);
#endif /* XP_WIN || XP_UNIX */
                FE_ClearReadSelect(CE_WINDOW_ID, cd->connection->sock);
                FE_ClearConnectSelect(CE_WINDOW_ID, cd->connection->sock);
                NETCLOSE(cd->connection->sock);
				NET_TotalNumberOfOpenConnections--;
				cd->connection->sock = SOCKET_INVALID;
              }

			if(cd->partial_cache_fp)
			  {
				FE_ClearFileReadSelect(ce->window_id, 
									   XP_Fileno(cd->partial_cache_fp));
				XP_FileClose(cd->partial_cache_fp);
				cd->partial_cache_fp = 0;
			  }


			if(cd->connection->prev_cache 
				&& !cd->connection_is_valid
				&& ce->status != MK_INTERRUPTED)
			  {
				if(CD_STREAM && !cd->reuse_stream)
				  {
                	ABORT_STREAM(CE_STATUS);
					FREE(CD_STREAM);
					CD_STREAM = 0;
				  }
			
				/* the connection came from the cache and
				 * may have been stale.  Try it again
				 */
				/* clear any error message */
				if(ce->URL_s->error_msg)
				  {
					FREE(ce->URL_s->error_msg);
					ce->URL_s->error_msg = NULL;
				  }
				cd->next_state = HTTP_START_CONNECT;
				ce->status = 0;

				/* we don't know if the connection is valid anymore 
		 		 * because we are going to try it again
		 		 */
				cd->connection_is_valid = FALSE;

				cd->connection->prev_cache = FALSE;
			  }
			else
			  {
            	CD_NEXT_STATE = HTTP_FREE;

            	if(CD_STREAM)
				  {
                	ABORT_STREAM(CE_STATUS);
					FREE(CD_STREAM);
					CD_STREAM = 0;
				  }

                /* remove the connection from the cache list
                 * and free the data
                 */
                XP_ListRemoveObject(http_connection_list, cd->connection);
                FREEIF(cd->connection->hostname);
                FREE(cd->connection);
			  }

            break;
        
        case HTTP_FREE:

			/* close the file upload progress.  If a stream was created
			 * then some sort of HTTP error occured.  Send in an error
			 * code
			 */         
#ifdef EDITOR
            if(cd->destroy_file_upload_progress_dialog)
                FE_SaveDialogDestroy(ce->window_id, ce->URL_s->server_status != 204 ? -1 : ce->status, ce->URL_s->post_data);
#endif /* EDITOR */

			if(ce->URL_s->files_to_post && ce->URL_s->post_data)
			  {
			  	/* we shoved the file to post into the post data.
				 * remove it so the history doesn't get confused
				 * and try and delete the file.
				 */
				FREE_AND_CLEAR(ce->URL_s->post_data);
				ce->URL_s->post_data_is_file = FALSE;
			  }

	        if(CD_DESTROY_GRAPH_PROGRESS)
     		    FE_GraphProgressDestroy(CE_WINDOW_ID, 
										CE_URL_S, 
										CD_ORIGINAL_CONTENT_LENGTH,
										CE_BYTES_RECEIVED);
      
            FREEIF(CD_LINE_BUFFER);
            FREEIF(CD_STREAM); /* don't forget the stream */
			FREEIF(CD_SERVER_HEADERS);
			FREEIF(cd->orig_host);
			if(CD_TCP_CON_DATA)
				NET_FreeTCPConData(CD_TCP_CON_DATA);
            FREEIF(cd);
            return STATUS (-1); /* final end */
        
        default: /* should never happen !!! */
            TRACEMSG(("HTTP: BAD STATE!"));
            CD_NEXT_STATE = HTTP_ERROR_DONE;
            break;
        }

        /* check for errors during load and call error 
         * state if found
         */
        if(CE_STATUS < 0 
		   && CE_STATUS != MK_USE_COPY_FROM_CACHE
		   && CD_NEXT_STATE != HTTP_FREE)
        {

			if (CE_STATUS == MK_MULTIPART_MESSAGE_COMPLETED)
			  {
        		/* We found the end of a multipart/mixed response
				 * from a CGI script in a http keep-alive response
				 * That signifies the end of a message.
         		 */
        		TRACEMSG(("mkhttp.c: End of multipart keep-alive response"));
		
         		CE_STATUS = MK_DATA_LOADED;
         		CD_NEXT_STATE = HTTP_DONE;

			  }
            else if (CE_STATUS == MK_HTTP_TYPE_CONFLICT
                /* Don't retry if were HTTP/.9 */
                && !CD_SEND_HTTP1
                /* Don't retry if we're posting. */
                && !CD_POSTING)
              {
                /* Could be a HTTP 0/1 compability problem. */
                TRACEMSG(("HTTP: Read error trying again with HTTP0 request."));
                NET_Progress (CE_WINDOW_ID, XP_GetString(XP_PROGRESS_TRYAGAIN));

                FE_ClearReadSelect(CE_WINDOW_ID, cd->connection->sock);
                FE_ClearConnectSelect(CE_WINDOW_ID, cd->connection->sock);
#ifdef XP_WIN
				if(cd->calling_netlib_all_the_time)
				{
                    net_call_all_the_time_count--;
					if(net_call_all_the_time_count == 0)
						FE_ClearCallNetlibAllTheTime(CE_WINDOW_ID);
				}
#endif /* XP_WIN */
#if defined(XP_WIN) || defined(XP_UNIX)
                FE_ClearDNSSelect(CE_WINDOW_ID, cd->connection->sock);
#endif /* XP_WIN || XP_UNIX */
                NETCLOSE(cd->connection->sock);
				NET_TotalNumberOfOpenConnections--;
                cd->connection->sock = SOCKET_INVALID;

                if(CD_STREAM)
                    (*CD_STREAM->abort) (CD_STREAM->data_object, CE_STATUS);
                CD_SEND_HTTP1 = FALSE;
                /* go back and send an HTTP0 request */
                CD_NEXT_STATE = HTTP_START_CONNECT;
              }
            else
              {
                CD_NEXT_STATE = HTTP_ERROR_DONE;
              }
            /* don't exit! loop around again and do the free case */
            CD_PAUSE_FOR_READ = FALSE;
        }
    } /* while(!CD_PAUSE_FOR_READ) */
    
    return STATUS(CE_STATUS);
}


/* abort the connection in progress
 */
MODULE_PRIVATE int
NET_InterruptHTTP(ActiveEntry * ce)
{
    HTTPConData * cd = (HTTPConData *)ce->con_data;

    /* if we are currently pulling data and the data is
	 * Textual then truncate the file, leave notification and
     * end normally
	 */
	if(CD_NEXT_STATE == HTTP_PULL_DATA 
		&& CE_URL_S->content_type
		 && !strcasecomp(CE_URL_S->content_type, TEXT_HTML))
	  {
		char buffer[127];

		if(!strcasecomp(CE_URL_S->content_type, TEXT_HTML))
			PR_snprintf(buffer, sizeof(buffer),
				XP_GetString(XP_HR_TRANSFER_INTERRUPTED));
		else
			PR_snprintf(buffer, sizeof(buffer),
				XP_GetString(XP_TRANSFER_INTERRUPTED));

		PUTSTRING(buffer);

		/* steps from HTTP_DONE duplicated, with the addition of
		 * NET_RefreshCacheFileExpiration 
	 	 */
        FE_ClearReadSelect(CE_WINDOW_ID, cd->connection->sock);
        NETCLOSE(cd->connection->sock);
		NET_TotalNumberOfOpenConnections--;
        ABORT_STREAM(MK_INTERRUPTED);
		FREE(CD_STREAM);
		CD_STREAM = 0;

		CE_URL_S->last_modified = 0;
#ifdef MOZILLA_CLIENT
		/* to make the last_modified change take effect 
		 */
        NET_RefreshCacheFileExpiration(CE_URL_S);  
#endif /* MOZILLA_CLIENT */

        CD_NEXT_STATE = HTTP_FREE;

        CE_STATUS = MK_DATA_LOADED;
	  }
	else
	  {
        CD_NEXT_STATE = HTTP_ERROR_DONE;
        CE_STATUS = MK_INTERRUPTED;
	  }
 
    return STATUS (NET_ProcessHTTP(ce));
}

/* Free any memory that might be used in caching etc.
 */
MODULE_PRIVATE void
NET_CleanupHTTP(void)
{
    /* nothing so far needs freeing */
    return;
}

#ifdef PROFILE
#pragma profile off
#endif
