/* -*- Mode: C; tab-width: 4 -*- */

/* Please leave outside of ifdef for windows precompiled headers */
#include "mkutils.h"

#ifdef MOZILLA_CLIENT

#include "merrors.h"

#include "mime.h"
#include "shist.h"
#include "glhist.h"
#include "xp_reg.h"
#include "mknews.h"
#include "mktcp.h"
#include "mkparse.h"
#include "mkformat.h"
#include "mkstream.h"
#include "mkaccess.h"
#include "mksort.h"
#include "mkcache.h"
#include "mknewsgr.h"
#include "libi18n.h"
#include "gui.h"
#include "cert.h"
#include "ssl.h"

#include "msgcom.h"
#include "libmime.h"

#include "prtime.h"

#include "xp_error.h"
#include "secnav.h"
#include "prefapi.h"	



/* for XP_GetString() */
#include "xpgetstr.h"
extern int MK_NEWS_ERROR_FMT;
extern int MK_NNTP_CANCEL_CONFIRM;
extern int MK_NNTP_CANCEL_DISALLOWED;
extern int MK_NNTP_NOT_CANCELLED;
extern int MK_OUT_OF_MEMORY;
extern int XP_CONFIRM_SAVE_NEWSGROUPS;
extern int XP_HTML_ARTICLE_EXPIRED;
extern int XP_HTML_NEWS_ERROR;
extern int XP_PROGRESS_READ_NEWSGROUPINFO;
extern int XP_PROGRESS_RECEIVE_ARTICLE;
extern int XP_PROGRESS_RECEIVE_LISTARTICLES;
extern int XP_PROGRESS_RECEIVE_NEWSGROUP;
extern int XP_PROGRESS_SORT_ARTICLES;
extern int XP_PROMPT_ENTER_USERNAME;
extern int MK_BAD_NNTP_CONNECTION;
extern int MK_NNTP_AUTH_FAILED;
extern int MK_NNTP_ERROR_MESSAGE;
extern int MK_NNTP_NEWSGROUP_SCAN_ERROR;
extern int MK_NNTP_SERVER_ERROR;
extern int MK_NNTP_SERVER_NOT_CONFIGURED;
extern int MK_SECURE_NEWS_PROXY_ERROR;
extern int MK_TCP_READ_ERROR;
extern int MK_TCP_WRITE_ERROR;
extern int MK_NNTP_CANCEL_ERROR;
extern int XP_CONNECT_NEWS_HOST_CONTACTED_WAITING_FOR_REPLY;
extern int XP_PLEASE_ENTER_A_PASSWORD_FOR_NEWS_SERVER_ACCESS;
extern int XP_GARBAGE_COLLECTING;
extern int XP_MESSAGE_SENT_WAITING_NEWS_REPLY;
extern int MK_MSG_DELIV_NEWS;


/* ### This very useful and quite wonderful function ought to be put somewhere
  in libutil, not in libmsg.  Oh, well. */
extern int msg_LineBuffer (const char *net_buffer, int32 net_buffer_size,
						   char **bufferP, uint32 *buffer_sizeP,
						   uint32 *buffer_fpP,
						   XP_Bool convert_newlines_p,
						   int32 (*per_line_fn) (char *line, uint32
												 line_length, void *closure),
						   void *closure);
						   

#ifdef PROFILE
#pragma profile on
#endif



#define LIST_WANTED     0
#define ARTICLE_WANTED  1
#define CANCEL_WANTED   2
#define GROUP_WANTED    3
#define NEWS_POST       4
#define READ_NEWS_RC    5
#define NEW_GROUPS      6


/* the output_buffer_size must be larger than the largest possible line
 * 2000 seems good for news
 *
 * jwz: I increased this to 4k since it must be big enough to hold the
 * entire button-bar HTML, and with the new "mailto" format, that can
 * contain arbitrarily long header fields like "references".
 */
#define OUTPUT_BUFFER_SIZE 4096

/* the amount of time to subtract from the machine time
 * for the newgroup command sent to the nntp server
 */
#define NEWGROUPS_TIME_OFFSET 60*60*12   /* 12 hours */

/* globals
 */
PRIVATE char * NET_NewsHost = NULL;
PRIVATE XP_List * nntp_connection_list=0;
PRIVATE XP_Bool net_news_last_username_probably_valid=FALSE;
PRIVATE int32 net_NewsChunkSize=-1;  /* default */

#define PUTSTRING(s)      (*connection_data->stream->put_block) \
                    (connection_data->stream->data_object, s, XP_STRLEN(s))
#define COMPLETE_STREAM   (*connection_data->stream->complete)  \
                    (connection_data->stream->data_object)
#define ABORT_STREAM(s)   (*connection_data->stream->abort) \
                    (connection_data->stream->data_object, s)

/* states of the machine
 */
typedef enum _StatesEnum {
NNTP_RESPONSE,
NNTP_CONNECT,
NNTP_CONNECT_WAIT,
NNTP_SEND_SSL_PROXY_REQUEST,
NNTP_PROCESS_FIRST_SSL_PROXY_HEADER,
NNTP_FINISH_SSL_PROXY_CONNECT,
NNTP_LOGIN_RESPONSE,
NNTP_SEND_MODE_READER,
NNTP_SEND_MODE_READER_RESPONSE,
SEND_FIRST_NNTP_COMMAND,
SEND_FIRST_NNTP_COMMAND_RESPONSE,
SETUP_NEWS_STREAM,
NNTP_BEGIN_AUTHORIZE,
NNTP_AUTHORIZE_RESPONSE,
NNTP_PASSWORD_RESPONSE,
NNTP_READ_LIST_BEGIN,
NNTP_READ_LIST,
DISPLAY_NEWSGROUPS,
NNTP_NEWGROUPS_BEGIN,
NNTP_NEWGROUPS,
NNTP_BEGIN_ARTICLE,
NNTP_READ_ARTICLE,
NNTP_XOVER_BEGIN,
NNTP_FIGURE_NEXT_CHUNK,
NNTP_XOVER_SEND,
NNTP_XOVER_RESPONSE,
NNTP_XOVER,
NEWS_PROCESS_XOVER,
NNTP_READ_GROUP,
NNTP_READ_GROUP_RESPONSE,
NNTP_READ_GROUP_BODY,
NNTP_SEND_GROUP_FOR_ARTICLE,
NNTP_SEND_GROUP_FOR_ARTICLE_RESPONSE,
NNTP_SEND_ARTICLE_NUMBER,
NEWS_PROCESS_BODIES,
NNTP_PRINT_ARTICLE_HEADERS,
NNTP_SEND_POST_DATA,
NNTP_SEND_POST_DATA_RESPONSE,
NNTP_CHECK_FOR_MESSAGE,
NEWS_NEWS_RC_POST,
NEWS_DISPLAY_NEWS_RC,
NEWS_DISPLAY_NEWS_RC_RESPONSE,
NEWS_START_CANCEL,
NEWS_DO_CANCEL,
NEWS_DONE,
NEWS_ERROR,
NNTP_ERROR,
NEWS_FREE
} StatesEnum;


/* structure to hold data about a tcp connection
 * to a news host
 */
typedef struct _NNTPConnection {
    char   *hostname;       /* hostname string (may contain :port) */
    NETSOCK csock;          /* control socket */
    XP_Bool busy;           /* is the connection in use already? */
    XP_Bool prev_cache;     /* did this connection come from the cache? */
    XP_Bool posting_allowed;/* does this server allow posting */
	XP_Bool default_host;
	XP_Bool no_xover;       /* xover command is not supported here */
	XP_Bool secure;         /* is it a secure connection? */

    char *current_group;	/* last GROUP command sent on this connection */

} NNTPConnection;

#define CD_CC_HOSTNAME         connection_data->control_con->hostname
#define CD_CC_DEFAULT_HOST     connection_data->control_con->default_host
#define CD_CC_POSTING_ALLOWED  connection_data->control_con->posting_allowed
#define CD_CC_NO_XOVER  	   connection_data->control_con->no_xover
#define CD_CC_SECURE           connection_data->control_con->secure
#define CD_CC_GROUP            connection_data->control_con->current_group

/* structure to hold all the state and data
 * for the state machine
 */
typedef struct _NewsConData {
    StatesEnum  next_state;
    StatesEnum  next_state_after_response;
    int         type_wanted;     /* Article, List, or Group */
    int         response_code;   /* code returned from NNTP server */
    char       *response_txt;    /* text returned from NNTP server */
    Bool     pause_for_read;  /* should we pause for the next read? */

	char       *ssl_proxy_server;    /* ssl proxy server hostname */
	XP_Bool     proxy_auth_required; /* is auth required */
	XP_Bool     sent_proxy_auth;     /* have we sent a proxy auth? */

	XP_Bool     newsrc_performed;    /* have we done a newsrc update? */
#ifdef XP_WIN
	XP_Bool     calling_netlib_all_the_time;
#endif
    NET_StreamClass * stream;

    NNTPConnection * control_con;  /* all the info about a connection */

    char   *data_buf;
    int32   data_buf_size;

    /* for group command */
    char   *path; /* message id */

    char   *group_name;
    int32   first_art;
    int32   last_art;
    int32   first_possible_art;
    int32   last_possible_art;

	int32	num_loaded;			/* How many articles we got XOVER lines for. */
	int32	num_wanted;			/* How many articles we wanted to get XOVER
								   lines for. */

  	XP_Bool isclipped;			/* Whether we had to limit the number of
								   articles down to MAX_XOVER_TO_SEND. */

    /* for cancellation: the headers to be used */
    char *cancel_from;
    char *cancel_newsgroups;
    char *cancel_distribution;
    char *cancel_id;
    char *cancel_msg_file;
    int cancel_status;

	/* variables for ReadNewsRC */
	char   **newsrc_list;
	int32    newsrc_list_index;
	int32    newsrc_list_count;

    XP_Bool  use_fancy_newsgroup_listing;  /* use LIST NEWSGROUPS or LIST */
    XP_Bool  destroy_graph_progress;       /* do we need to destroy 
											* graph progress? 
											*/    

    char  *output_buffer;                 /* use it to write out lines */

    int32  article_num;   /* current article number */

    char  *message_id;    /* for reading groups */
    char  *author;
    char  *subject;

	int32  original_content_length; /* the content length at the time of
                                     * calling graph progress
                                     */

    /* random pointer for libmsg state */
    void *xover_parse_state;

	TCP_ConData * tcp_con_data;

	void * write_post_data_data;

	XP_Bool some_protocol_succeeded; /* We know we have done some protocol
										with this newsserver recently, so don't
										respond to an error by closing and
										reopening it.  */
} NewsConData;

#define CD_NEXT_STATE      		connection_data->next_state
#define CD_NEXT_STATE_AFTER_RESPONSE	\
								connection_data->next_state_after_response
#define CD_TYPE_WANTED     		connection_data->type_wanted
#define CD_RESPONSE_CODE   		connection_data->response_code
#define CD_RESPONSE_TXT    		connection_data->response_txt
#define CD_PAUSE_FOR_READ  		connection_data->pause_for_read

#define CD_SSL_PROXY_SERVER     connection_data->ssl_proxy_server
#define CD_PROXY_AUTH_REQUIRED  connection_data->proxy_auth_required
#define CD_SENT_PROXY_AUTH      connection_data->sent_proxy_auth

#define CD_NEWSRC_PERFORMED     connection_data->newsrc_performed

#define CD_STREAM          		connection_data->stream
#define CD_CONTROL_CON     		connection_data->control_con

#define CD_DATA_BUF       		connection_data->data_buf
#define CD_DATA_BUF_SIZE  		connection_data->data_buf_size

#define CD_PATH             	connection_data->path
#define CD_GROUP_NAME       	connection_data->group_name
#define CD_FIRST_ART        	connection_data->first_art
#define CD_LAST_ART       		connection_data->last_art
#define CD_FIRST_POSSIBLE_ART	connection_data->first_possible_art
#define CD_LAST_POSSIBLE_ART	connection_data->last_possible_art

#define CD_NUM_WANTED			connection_data->num_wanted
#define CD_NUM_LOADED			connection_data->num_loaded

#define CD_USE_FANCY_NEWSGROUP_LISTING	\
								connection_data->use_fancy_newsgroup_listing

#define CD_OUTPUT_BUFFER        connection_data->output_buffer

#define CD_ARTICLE_NUM  		connection_data->article_num

#define CD_MESSAGE_ID   		connection_data->message_id
#define CD_AUTHOR       		connection_data->author
#define CD_SUBJECT  			connection_data->subject

#define CD_THREAD_ENTRY  		connection_data->thread_entry

#define CD_DESTROY_GRAPH_PROGRESS  \
								connection_data->destroy_graph_progress
#define CD_ORIGINAL_CONTENT_LENGTH \
								connection_data->original_content_length

#define CD_NEWSRC_LIST			connection_data->newsrc_list
#define CD_NEWSRC_LIST_INDEX	connection_data->newsrc_list_index
#define CD_NEWSRC_LIST_COUNT	connection_data->newsrc_list_count

#define CD_TCP_CON_DATA			connection_data->tcp_con_data

#define CE_XOVER_PARSE_STATE    connection_data->xover_parse_state

#define CD_SOME_PROTOCOL_SUCCEEDED	connection_data->some_protocol_succeeded


/* helpful variable name shortcuts */
#define CE_WINDOW_ID    	cur_entry->window_id
#define CE_URL_S        	cur_entry->URL_s
#define CE_SOCK     		cur_entry->socket
#define CE_CON_SOCK 		cur_entry->con_sock
#define CE_STATUS   		cur_entry->status
#define CE_BYTES_RECEIVED   cur_entry->bytes_received
#define CE_FORMAT_OUT      	cur_entry->format_out

/* publicly available function to set the news host
 * this will be a useful front end callable routine
 */
PUBLIC void NET_SetNewsHost (const char * value)
{
	TRACEMSG(("Setting news host to be: %s", value));

	if (NET_NewsHost)
	  XP_FREE (NET_NewsHost);
	if (value && *value)
	  NET_NewsHost = XP_STRDUP (value);
	else
	  NET_NewsHost = 0;

	if (NET_NewsHost)
	  {
		MWContext *news_context = XP_FindContextOfType (0, MWContextNews);
		if (news_context)
		  MSG_NewsHostChanged (news_context, NET_NewsHost);
	  }
}

/* Get the NNTP server name 
 */
CONST char * NET_GetNewsHost (MWContext *context, XP_Bool warnUserIfMissing) 
{

    if(NET_NewsHost)
	  {
		TRACEMSG(("Using news hostname: %s", NET_NewsHost));
        return NET_NewsHost;
	  }
	else if (warnUserIfMissing)
	  {
		char * alert = NET_ExplainErrorDetails(MK_NNTP_SERVER_NOT_CONFIGURED);
#ifdef XP_MAC
		/*FE_EditPreference(PREF_NewsHost); someone broke it*/
#endif
		FE_Alert(context, alert);
		FREE(alert);
	  }
	return NULL;
}

/* set the number of articles shown in news at any one time
 */
PUBLIC void 
NET_SetNumberOfNewsArticlesInListing(int32 number)
{
	net_NewsChunkSize = number;
}

/* Whether we cache XOVER data.  NYI. */
PUBLIC void
NET_SetCacheXOVER(XP_Bool value)
{
}



PUBLIC void NET_CleanTempXOVERCache(void)
{
}


/* gets the response code from the nntp server and the
 * response line
 *
 * returns the TCP return code from the read
 */
PRIVATE int
net_news_response (ActiveEntry * cur_entry)
{
    char * line;
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

    CE_STATUS = NET_BufferedReadLine(CE_SOCK, &line, &CD_DATA_BUF, 
                    &CD_DATA_BUF_SIZE, (Bool*)&CD_PAUSE_FOR_READ);

    if(CE_STATUS == 0)
      {
        CD_NEXT_STATE = NNTP_ERROR;
        CD_PAUSE_FOR_READ = FALSE;
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_NNTP_SERVER_ERROR);
        return(MK_NNTP_SERVER_ERROR);
      }

    /* if TCP error of if there is not a full line yet return
     */
    if(CE_STATUS < 0)
	  {
        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, 
													  SOCKET_ERRNO);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
	  }
	else if(!line)
	  {
         return CE_STATUS;
	  }

    CD_PAUSE_FOR_READ = FALSE; /* don't pause if we got a line */

	if(CE_BYTES_RECEIVED == 0)
	  {
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
		CERT_DestroyCertificate(CE_URL_S->certificate);
#endif /* !HAVE_SECURITY -- added by jwz */

#ifndef NO_SSL /* added by jwz */
        SSL_SecurityStatus(CE_SOCK,
                       &CE_URL_S->security_on,
                       &CE_URL_S->key_cipher,
                       &CE_URL_S->key_size,
                       &CE_URL_S->key_secret_size,
                       &CE_URL_S->key_issuer,
                       &CE_URL_S->key_subject);
		CE_URL_S->certificate = SSL_PeerCertificate(CE_SOCK);
#endif /* NO_SSL -- added by jwz */
	  }

    /* almost correct
     */
    if(CE_STATUS > 1)
	  {
        CE_BYTES_RECEIVED += CE_STATUS;
        FE_GraphProgress(CE_WINDOW_ID, CE_URL_S, CE_BYTES_RECEIVED, CE_STATUS, CE_URL_S->content_length);
	  }

    TRACEMSG(("NNTP Rx: %s\n", line));

    StrAllocCopy(CD_RESPONSE_TXT, line+4);

    sscanf(line, "%d", &CD_RESPONSE_CODE);

	/* authentication required can come at any time
	 */
	if(480 == CD_RESPONSE_CODE || 450 == CD_RESPONSE_CODE)
	  {
        CD_NEXT_STATE = NNTP_BEGIN_AUTHORIZE;
	  }
	else
	  {
    	CD_NEXT_STATE = CD_NEXT_STATE_AFTER_RESPONSE;
	  }

    return(0);  /* everything ok */
}

#ifndef NO_SSL /* added by jwz */
PRIVATE int
net_nntp_send_ssl_proxy_request(ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
    char *host = NET_ParseURL(CE_URL_S->address, GET_HOST_PART);
    char *command=0;
	char *auth;
   
    StrAllocCopy(command, "CONNECT ");
    StrAllocCat(command, host);

    if(!XP_STRCHR(host, ':'))
      {
        char small_buf[20];
        PR_snprintf(small_buf, sizeof(small_buf), ":%d", SECURE_NEWS_PORT);
        StrAllocCat(command, small_buf);
      }

    StrAllocCat(command, " HTTP/1.0\n");
   
    /* see if we need to send the proxy authorization line
     * and send it if we do
     */
    /*
     * Check if proxy is requiring authorization.
     * If NULL, not necessary, or the proxy will return 407 to
     * require authorization.
     *
     */
    if ( NULL != (auth=NET_BuildProxyAuthString(CE_WINDOW_ID,
                                                 CE_URL_S,
                                                 CD_SSL_PROXY_SERVER)) )
      {
        CD_SENT_PROXY_AUTH = TRUE;
        PR_snprintf(CD_OUTPUT_BUFFER, 
				   OUTPUT_BUFFER_SIZE,
				   "Proxy-authorization: %.256s%c%c", 
				   auth, CR, LF);
        StrAllocCat(command, CD_OUTPUT_BUFFER); 
        TRACEMSG(("News: Sending proxy-authorization: %s", auth));
      }
    else
      {
        TRACEMSG(("News: Not sending proxy authorization (yet)"));
      }

	{
		char line[200];
		PR_snprintf(line, sizeof(line), "User-Agent: %.90s/%.90s" CRLF CRLF,
				   XP_AppCodeName, XP_AppVersion);
		StrAllocCat(command, line);
	}

    TRACEMSG(("Tx: %s", command));

    CE_STATUS = NET_BlockingWrite(CE_SOCK, command, XP_STRLEN(command));

    if(CE_STATUS < 0)
      {
        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_WRITE_ERROR, CE_STATUS);
      }

    FREE(command);

    CD_PAUSE_FOR_READ = TRUE;

    CD_NEXT_STATE = NNTP_PROCESS_FIRST_SSL_PROXY_HEADER;

    return(CE_STATUS);
}

PRIVATE int
nntp_process_first_ssl_proxy_header(ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
    char * line;
	char server_version[128];

    CE_STATUS = NET_BufferedReadLine(CE_SOCK, &line, &CD_DATA_BUF,
                                        &CD_DATA_BUF_SIZE, (Bool*)&CD_PAUSE_FOR_READ);

    if(CE_STATUS == 0)
      {
        CD_NEXT_STATE = NNTP_ERROR;
        CD_PAUSE_FOR_READ = FALSE;
        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_NNTP_SERVER_ERROR);
        return(MK_NNTP_SERVER_ERROR);
      }

    if(!line)
        return(CE_STATUS);  /* no line yet */

    if(CE_STATUS<0)
      {
        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, SOCKET_ERRNO);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
      }

	*server_version = 0;

    sscanf(line, "%20s %d", server_version, &CE_URL_S->server_status);

	 switch (CE_URL_S->server_status / 100)
       {

		case 2:   /* Succesful reply */
		    NET_Progress(CE_WINDOW_ID,
			 XP_GetString( XP_CONNECT_NEWS_HOST_CONTACTED_WAITING_FOR_REPLY ) );
			break;
			
        case 4:    /* client error */
            if(CE_URL_S->server_status == 407)
              {
				CD_PROXY_AUTH_REQUIRED = TRUE;
              }
			break;

		default:
			/* error? */
			CE_URL_S->error_msg = NET_ExplainErrorDetails(
												MK_SECURE_NEWS_PROXY_ERROR);
			return MK_SECURE_NEWS_PROXY_ERROR;
	   }


    CD_NEXT_STATE = NNTP_FINISH_SSL_PROXY_CONNECT;
    return(0);
}

PRIVATE int
nntp_process_ssl_proxy_headers(ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
    char * line;

    CE_STATUS = NET_BufferedReadLine(CE_SOCK, &line, &CD_DATA_BUF,
                                        &CD_DATA_BUF_SIZE, 
                                     (Bool*)&CD_PAUSE_FOR_READ);

    if(CE_STATUS == 0)
      {
        CD_NEXT_STATE = NNTP_ERROR;
        CD_PAUSE_FOR_READ = FALSE;
        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_NNTP_SERVER_ERROR);
        return(MK_NNTP_SERVER_ERROR);
      }

    if(!line)
        return(CE_STATUS);  /* no line yet */

    if(CE_STATUS<0)
      {
        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, SOCKET_ERRNO);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
      }

    /* check for end of MIME headers */
    if(*line == '\0' || *line == '\r')
      {
        TRACEMSG(("Finished parsing MIME headers"));

		if(CD_PROXY_AUTH_REQUIRED)
		  {

#ifndef NO_SSL /* added by jwz */
			if(NET_AskForProxyAuth(CE_WINDOW_ID,
                                    CD_SSL_PROXY_SERVER,
                                    CE_URL_S->proxy_authenticate,
                                    CD_SENT_PROXY_AUTH))
		  	  {
				/* close the current connection and try again */
                FE_ClearReadSelect(CE_WINDOW_ID, CD_CONTROL_CON->csock);
                NETCLOSE(CD_CONTROL_CON->csock);  /* close the socket */
				FREEIF(CD_CC_GROUP);
                NET_TotalNumberOfOpenConnections--;
                CE_SOCK = SOCKET_INVALID;

				CD_NEXT_STATE = NNTP_CONNECT;
				CD_PAUSE_FOR_READ = FALSE;
		  	  }
			else
#endif /* NO_SSL -- added by jwz */
			  {
				CD_NEXT_STATE = NNTP_ERROR;
				CD_PAUSE_FOR_READ = FALSE;
		  	  }
			CD_PROXY_AUTH_REQUIRED = FALSE;
		  }
		else
		  {
            /* we have now successfully initiated a ssl proxy
             * connection to a remote ssl host
             *
             * now we need to give the file descriptor
             * to the ssl library and let it initiate
             * a secure connection
             * after that we need to do normal news stuff
             */
#ifndef NO_SSL /* added by jwz */
            if (SSL_Enable(CE_SOCK, SSL_SECURITY, 1) ||
                    SSL_ResetHandshake(CE_SOCK, 0))
              {
                int err = XP_GetError();
                return err < 0 ? err : -err;
              }
#endif /* NO_SSL -- added by jwz */

            CD_NEXT_STATE = NNTP_RESPONSE;
            CD_NEXT_STATE_AFTER_RESPONSE = NNTP_LOGIN_RESPONSE;
            CD_PAUSE_FOR_READ = FALSE;
		  }
      }
	else
	  {
		  char *value = XP_STRCHR(line, ':');
		  if(value)
			  value++;

		  NET_ParseMimeHeader(CE_WINDOW_ID, CE_URL_S, line, value);
	  }

    return(0);
}
#endif /* NO_SSL -- added by jwz */


/* interpret the server response after the connect
 *
 * returns negative if the server responds unexpectedly
 */
PRIVATE int
net_nntp_login_response (ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

    if((CD_RESPONSE_CODE/100)!=2)
      {
		CE_URL_S->error_msg  = NET_ExplainErrorDetails(MK_NNTP_ERROR_MESSAGE, CD_RESPONSE_TXT);

    	CD_NEXT_STATE = NNTP_ERROR;
        CD_CONTROL_CON->prev_cache = FALSE; /* to keep if from reconnecting */
        return MK_BAD_NNTP_CONNECTION;
      }

    if(CD_RESPONSE_CODE == 200)
        CD_CC_POSTING_ALLOWED = TRUE;
    else
        CD_CC_POSTING_ALLOWED = FALSE;
    
    CD_NEXT_STATE = NNTP_SEND_MODE_READER;
    return(0);  /* good */
}

PRIVATE int
net_nntp_send_mode_reader (ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

    XP_STRCPY(CD_OUTPUT_BUFFER, "MODE READER" CRLF);

    CE_STATUS = (int) NET_BlockingWrite(CE_SOCK,CD_OUTPUT_BUFFER,XP_STRLEN(CD_OUTPUT_BUFFER));

	TRACEMSG(("News Tx: %s", CD_OUTPUT_BUFFER));

    CD_NEXT_STATE = NNTP_RESPONSE;
    CD_NEXT_STATE_AFTER_RESPONSE = NNTP_SEND_MODE_READER_RESPONSE;
    CD_PAUSE_FOR_READ = TRUE;

    return(CE_STATUS);

}

PRIVATE int
net_nntp_send_mode_reader_response (ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

	/* ignore the response code and continue
	 */
    CD_NEXT_STATE = SEND_FIRST_NNTP_COMMAND;

	return(0);
}

/* figure out what the first command is and send it
 *
 * returns the status from the NETWrite
 */
PRIVATE int
net_send_first_nntp_command (ActiveEntry *cur_entry)
{
    char *command=0;
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

	if (CD_TYPE_WANTED == ARTICLE_WANTED)
	  {
		const char *group = 0;
		uint32 number = 0;

		MSG_NewsGroupAndNumberOfID (CE_WINDOW_ID, CD_PATH,
									&group, &number);
		if (group && number)
		  {
			CD_ARTICLE_NUM = number;
			StrAllocCopy (CD_GROUP_NAME, group);

			if (CD_CC_GROUP && !XP_STRCMP (CD_CC_GROUP, group))
			  CD_NEXT_STATE = NNTP_SEND_ARTICLE_NUMBER;
			else
			  CD_NEXT_STATE = NNTP_SEND_GROUP_FOR_ARTICLE;

			CD_PAUSE_FOR_READ = FALSE;
			return 0;
		  }
	  }

    if(CD_TYPE_WANTED == NEWS_POST && !CE_URL_S->post_data)
      {
		XP_ASSERT(0);
        return(-1);
      }
    else if(CD_TYPE_WANTED == NEWS_POST)
      {  /* posting to the news group */
        StrAllocCopy(command, "POST");
      }
    else if(CD_TYPE_WANTED == READ_NEWS_RC)
      {
		if(CE_URL_S->method == URL_POST_METHOD ||
								XP_STRCHR(CE_URL_S->address, '?'))
        	CD_NEXT_STATE = NEWS_NEWS_RC_POST;
		else
        	CD_NEXT_STATE = NEWS_DISPLAY_NEWS_RC;
		return(0);
      } 
	else if(CD_TYPE_WANTED == NEW_GROUPS)
	  {
		time_t last_update = NET_NewsgroupsLastUpdatedTime(CD_CC_HOSTNAME, 
													 	   CD_CC_SECURE);
		char small_buf[64];
        PRTime  expandedTime;

		if(!last_update)
	 	  {
			
			CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_NNTP_NEWSGROUP_SCAN_ERROR);
			CD_NEXT_STATE = NEWS_ERROR;
			return(MK_INTERRUPTED);
		  }
	
		/* subtract some hours just to be sure */
		last_update -= NEWGROUPS_TIME_OFFSET;

        {
           int64  secToUSec, timeInSec, timeInUSec;
           LL_I2L(timeInSec, last_update);
           LL_I2L(secToUSec, PR_USEC_PER_SEC);
           LL_MUL(timeInUSec, timeInSec, secToUSec);
           PR_ExplodeTime( &expandedTime, timeInUSec );
        }
		PR_FormatTimeUSEnglish(small_buf, sizeof(small_buf), 
                               "NEWGROUPS %y%m%d %H%M%S", &expandedTime);
		
        StrAllocCopy(command, small_buf);

	  }
    else if(CD_TYPE_WANTED == LIST_WANTED)
      {
		
		CD_USE_FANCY_NEWSGROUP_LISTING = FALSE;
		if(NET_NewsgroupsLastUpdatedTime(CD_CC_HOSTNAME, CD_CC_SECURE))
		  {
			CD_NEXT_STATE = DISPLAY_NEWSGROUPS;
        	return(0);
	      }
		else
		  {
#ifdef BUG_21013
			if(!FE_Confirm(CE_WINDOW_ID, XP_GetString(XP_CONFIRM_SAVE_NEWSGROUPS)))
	  		  {
				CD_NEXT_STATE = NEWS_ERROR;
				return(MK_INTERRUPTED);
	  		  }
#endif /* BUG_21013 */

        	StrAllocCopy(command, "LIST");
		  }
      } 
    else if(CD_TYPE_WANTED == GROUP_WANTED) 
      {
        /* Don't use MKParse because the news: access URL doesn't follow traditional
         * rules. For instance, if the article reference contains a '#',
         * the rest of it is lost.
         */
        char * slash;

        StrAllocCopy(command, "GROUP ");

        slash = XP_STRCHR(CD_GROUP_NAME, '/');  /* look for a slash */
        CD_FIRST_ART = 0;
        CD_LAST_ART = 0;
        if (slash)
          {
            *slash = '\0';
#ifdef __alpha
            (void) sscanf(slash+1, "%d-%d", &CD_FIRST_ART, &CD_LAST_ART);
#else
            (void) sscanf(slash+1, "%ld-%ld", &CD_FIRST_ART, &CD_LAST_ART);
#endif
          }

		StrAllocCopy (CD_CC_GROUP, CD_GROUP_NAME);
        StrAllocCat(command, CD_CC_GROUP);
      }
    else  /* article or cancel */
      {
		if (CD_TYPE_WANTED == CANCEL_WANTED)
		  StrAllocCopy(command, "HEAD ");
		else
		  StrAllocCopy(command, "ARTICLE ");
        if (*CD_PATH != '<')
        	StrAllocCat(command,"<");
        StrAllocCat(command, CD_PATH);
        if (XP_STRCHR(command+8, '>')==0) 
        	StrAllocCat(command,">");
      }

    StrAllocCat(command, CRLF);
	
	TRACEMSG(("News Tx: %s", command));

    CE_STATUS = (int) NET_BlockingWrite(CE_SOCK, command, XP_STRLEN(command));
    XP_FREE(command);

    CD_NEXT_STATE = NNTP_RESPONSE;
    CD_NEXT_STATE_AFTER_RESPONSE = SEND_FIRST_NNTP_COMMAND_RESPONSE;
    CD_PAUSE_FOR_READ = TRUE;

    return(CE_STATUS);

} /* sent first command */

/* interprets the server response from the first command sent
 *
 * returns negative if the server responds unexpectedly
 */
PRIVATE int
net_send_first_nntp_command_response (ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
	int major_opcode = CD_RESPONSE_CODE/100;

	if((major_opcode == 3 && CD_TYPE_WANTED == NEWS_POST)
	 	|| (major_opcode == 2 && CD_TYPE_WANTED != NEWS_POST) )
      {

        CD_NEXT_STATE = SETUP_NEWS_STREAM;
		CD_SOME_PROTOCOL_SUCCEEDED = TRUE;

        return(0);  /* good */
      }
    else
      {
		/* #### maybe make this a dialog box
		   to do that, just return an error.
		 */

		/* ERROR STATE
         *  Set up the HTML stream
         */
		CE_FORMAT_OUT = CLEAR_CACHE_BIT(CE_FORMAT_OUT);
        StrAllocCopy(CE_URL_S->content_type, TEXT_HTML);

		if (CE_FORMAT_OUT == FO_PRESENT &&
			(CE_WINDOW_ID->type == MWContextBrowser ||
			 CE_WINDOW_ID->type == MWContextNews))
		  {
			CD_STREAM = NET_StreamBuilder(CE_FORMAT_OUT, CE_URL_S,
										  CE_WINDOW_ID);
			if(!CD_STREAM)
			  {
				CE_URL_S->error_msg =
				  NET_ExplainErrorDetails(MK_UNABLE_TO_CONVERT);
				return MK_UNABLE_TO_CONVERT;
			  }

			PR_snprintf(CD_OUTPUT_BUFFER,
						OUTPUT_BUFFER_SIZE,
						XP_GetString(XP_HTML_NEWS_ERROR),
					    CD_RESPONSE_TXT);
			if(CE_STATUS > -1)
			  CE_STATUS = PUTSTRING(CD_OUTPUT_BUFFER);

			if(CD_TYPE_WANTED == ARTICLE_WANTED ||
			   CD_TYPE_WANTED == CANCEL_WANTED)
			  {
				XP_STRCPY(CD_OUTPUT_BUFFER,
						   XP_GetString(XP_HTML_ARTICLE_EXPIRED));
				if(CE_STATUS > -1)
				  PUTSTRING(CD_OUTPUT_BUFFER);

				if (CD_PATH && *CD_PATH && CE_STATUS > -1)
				  {
					PR_snprintf(CD_OUTPUT_BUFFER, 
								OUTPUT_BUFFER_SIZE,
								"<P>&lt;%.512s&gt;", CD_PATH);
					if (CD_ARTICLE_NUM > 0)
					  PR_snprintf(CD_OUTPUT_BUFFER+XP_STRLEN(CD_OUTPUT_BUFFER),
								 OUTPUT_BUFFER_SIZE-XP_STRLEN(CD_OUTPUT_BUFFER),
								 " (%lu)", (unsigned long) CD_ARTICLE_NUM);
					PUTSTRING(CD_OUTPUT_BUFFER);
				  }

				if(CE_STATUS > -1)
				  {
					COMPLETE_STREAM;
					CD_STREAM = 0;
				  }
			  }

			CE_URL_S->expires = 1;
			/* CE_URL_S->error_msg =
			   NET_ExplainErrorDetails(MK_NNTP_SERVER_ERROR); 
			 */
		  }
		else
		  {
			/* FORMAT_OUT is not FO_PRESENT - so instead of emitting an HTML
			   error message, present it in a dialog box. */

			PR_snprintf(CD_OUTPUT_BUFFER,  /* #### i18n */
					   OUTPUT_BUFFER_SIZE,
					   XP_GetString(MK_NEWS_ERROR_FMT),
					   CD_RESPONSE_TXT);
			CE_URL_S->error_msg = XP_STRDUP (CD_OUTPUT_BUFFER);
		  }

		/* if the server returned a 400 error then it is an expected
		 * error.  the NEWS_ERROR state will not sever the connection
		 */
		if(major_opcode == 4)
		  CD_NEXT_STATE = NEWS_ERROR;
		else
		  CD_NEXT_STATE = NNTP_ERROR;

		/* If we ever get an error, let's re-issue the GROUP command
		   before the next ARTICLE command; the overhead isn't very high,
		   and weird stuff seems to be happening... */
		FREEIF(CD_CC_GROUP);

        /* CD_CONTROL_CON->prev_cache = FALSE;  to keep if from reconnecting */
        return MK_NNTP_SERVER_ERROR;
      }

	/* start the graph progress indicator
     */
    FE_GraphProgressInit(CE_WINDOW_ID, CE_URL_S, CE_URL_S->content_length);
    CD_DESTROY_GRAPH_PROGRESS = TRUE;  /* we will need to destroy it */
    CD_ORIGINAL_CONTENT_LENGTH = CE_URL_S->content_length;

	return(CE_STATUS);
}


PRIVATE int
net_send_group_for_article (ActiveEntry * cur_entry)
{
  NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

  StrAllocCopy (CD_CC_GROUP, CD_GROUP_NAME);
  PR_snprintf(CD_OUTPUT_BUFFER, 
			OUTPUT_BUFFER_SIZE, 
			"GROUP %.512s" CRLF, 
			CD_CC_GROUP);

  CE_STATUS = (int) NET_BlockingWrite(CE_SOCK,CD_OUTPUT_BUFFER,
									  XP_STRLEN(CD_OUTPUT_BUFFER));
	TRACEMSG(("News Tx: %s", CD_OUTPUT_BUFFER));

    CD_NEXT_STATE = NNTP_RESPONSE;
    CD_NEXT_STATE_AFTER_RESPONSE = NNTP_SEND_GROUP_FOR_ARTICLE_RESPONSE;
    CD_PAUSE_FOR_READ = TRUE;

    return(CE_STATUS);
}

PRIVATE int
net_send_group_for_article_response (ActiveEntry * cur_entry)
{
  NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

  /* ignore the response code and continue
   */
  CD_NEXT_STATE = NNTP_SEND_ARTICLE_NUMBER;

  return(0);
}


PRIVATE int
net_send_article_number (ActiveEntry * cur_entry)
{
  NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

  PR_snprintf(CD_OUTPUT_BUFFER, 
			OUTPUT_BUFFER_SIZE, 
			"ARTICLE %lu" CRLF, 
			CD_ARTICLE_NUM);

  CE_STATUS = (int) NET_BlockingWrite(CE_SOCK,CD_OUTPUT_BUFFER,
									  XP_STRLEN(CD_OUTPUT_BUFFER));
	TRACEMSG(("News Tx: %s", CD_OUTPUT_BUFFER));

    CD_NEXT_STATE = NNTP_RESPONSE;
    CD_NEXT_STATE_AFTER_RESPONSE = SEND_FIRST_NNTP_COMMAND_RESPONSE;
    CD_PAUSE_FOR_READ = TRUE;

    return(CE_STATUS);
}




static char *
news_generate_news_url_fn (const char *dest, void *closure,
						   MimeHeaders *headers)
{
  ActiveEntry *cur_entry = (ActiveEntry *) closure;
  /* NOTE: you can't use NewsConData here, because cur_entry might refer
	 to a file in the disk cache rather than an NNTP connection. */
  char *prefix = NET_ParseURL(CE_URL_S->address,
							  (GET_PROTOCOL_PART | GET_HOST_PART));
  char *new_dest = NET_Escape (dest, URL_XALPHAS);
  char *result = (char *) XP_ALLOC (XP_STRLEN (prefix) +
									(new_dest ? XP_STRLEN (new_dest) : 0)
									+ 40);
  if (result && prefix)
	{
	  XP_STRCPY (result, prefix);
	  if (prefix[XP_STRLEN(prefix) - 1] != ':')
		/* If there is a host, it must be terminated with a slash. */
		XP_STRCAT (result, "/");
	  XP_STRCAT (result, new_dest);
	}
  FREEIF (prefix);
  FREEIF (new_dest);
  return result;
}

static char *
news_generate_reference_url_fn (const char *dest, void *closure,
								MimeHeaders *headers)
{
  return news_generate_news_url_fn (dest, closure, headers);
}



/* Initiates the stream and sets state for the transfer
 *
 * returns negative if unable to setup stream
 */
PRIVATE int
net_setup_news_stream (ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
    
    if (CD_TYPE_WANTED == NEWS_POST)
	  {
        FE_ClearReadSelect(CE_WINDOW_ID, CE_SOCK);
        FE_SetConnectSelect(CE_WINDOW_ID, CE_SOCK);
#ifdef XP_WIN
		connection_data->calling_netlib_all_the_time;
		net_call_all_the_time_count++;
		FE_SetCallNetlibAllTheTime(CE_WINDOW_ID);
#endif /* XP_WIN */

		CE_CON_SOCK = CE_SOCK;
        CD_NEXT_STATE = NNTP_SEND_POST_DATA;

		NET_Progress(CE_WINDOW_ID, XP_GetString(MK_MSG_DELIV_NEWS));
	  }
    else if(CD_TYPE_WANTED == LIST_WANTED)
	  {
        CD_NEXT_STATE = NNTP_READ_LIST_BEGIN;
	  }
    else if(CD_TYPE_WANTED == GROUP_WANTED)
	  {
        CD_NEXT_STATE = NNTP_XOVER_BEGIN;
	  }
	else if(CD_TYPE_WANTED == NEW_GROUPS)
	  {
		CD_NEXT_STATE = NNTP_NEWGROUPS_BEGIN;
	  }
    else if(CD_TYPE_WANTED == ARTICLE_WANTED ||
			CD_TYPE_WANTED == CANCEL_WANTED)
	  {
	    CD_NEXT_STATE = NNTP_BEGIN_ARTICLE;
	  }
	else
	  {
		XP_ASSERT(0);
		return -1;
	  }

   return(0); /* good */
}

/* This doesn't actually generate HTML - but it is our hook into the 
   message display code, so that we can get some values out of it
   after the headers-to-be-displayed have been parsed.
 */
static char *
news_generate_html_header_fn (const char *dest, void *closure,
							  MimeHeaders *headers)
{
  ActiveEntry *cur_entry = (ActiveEntry *) closure;
  /* NOTE: you can't always use NewsConData here if, because cur_entry might
	 refer to a file in the disk cache rather than an NNTP connection. */
  NewsConData *connection_data = 0;

  XP_ASSERT(cur_entry->protocol == NEWS_TYPE_URL ||
			cur_entry->protocol == FILE_CACHE_TYPE_URL ||
			cur_entry->protocol == MEMORY_CACHE_TYPE_URL);
  if (cur_entry->protocol == NEWS_TYPE_URL)
	connection_data = (NewsConData *)cur_entry->con_data;

  if (connection_data && CD_TYPE_WANTED == CANCEL_WANTED)
	{
	  /* Save these for later (used in NEWS_DO_CANCEL state.) */
	  connection_data->cancel_from =
		MimeHeaders_get(headers, HEADER_FROM, FALSE, FALSE);
	  connection_data->cancel_newsgroups =
		MimeHeaders_get(headers, HEADER_NEWSGROUPS, FALSE, TRUE);
	  connection_data->cancel_distribution =
		MimeHeaders_get(headers, HEADER_DISTRIBUTION, FALSE, FALSE);
	  connection_data->cancel_id =
		MimeHeaders_get(headers, HEADER_MESSAGE_ID, FALSE, FALSE);
	}
  else
	{
	  char *id;
	  XP_ASSERT (!connection_data || CD_TYPE_WANTED == ARTICLE_WANTED);
	  id = MimeHeaders_get(headers, HEADER_MESSAGE_ID, FALSE, FALSE);
	  if (id)
		{
		  char *xref = MimeHeaders_get(headers, HEADER_XREF, FALSE, FALSE);
		  MSG_MarkMessageIDRead (CE_WINDOW_ID, id, xref);
		  FREEIF(xref);
		  XP_FREE(id);
		}
	  MSG_ActivateReplyOptions (cur_entry->window_id, headers);
	}
  return 0;
}



XP_Bool NET_IsNewsMessageURL (const char *url);

int
net_InitializeNewsFeData (ActiveEntry * cur_entry)
{
  MimeDisplayOptions *opt;

  if (!NET_IsNewsMessageURL (CE_URL_S->address))
	{
	  XP_ASSERT(0);
	  return -1;
	}

  if (CE_URL_S->fe_data)
	{
	  XP_ASSERT(0);
	  return -1;
	}

  opt = XP_NEW (MimeDisplayOptions);
  if (!opt) return MK_OUT_OF_MEMORY;
  XP_MEMSET (opt, 0, sizeof(*opt));

  opt->generate_reference_url_fn      = news_generate_reference_url_fn;
  opt->generate_news_url_fn           = news_generate_news_url_fn;
  opt->generate_header_html_fn		  = 0;
  opt->generate_post_header_html_fn	  = news_generate_html_header_fn;
  opt->html_closure					  = cur_entry;

  CE_URL_S->fe_data = opt;
  return 0;
}


PRIVATE int
net_begin_article(ActiveEntry * cur_entry)
{
  NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
  if (CD_TYPE_WANTED != ARTICLE_WANTED &&
	  CD_TYPE_WANTED != CANCEL_WANTED)
	return 0;

  /*  Set up the HTML stream
   */ 
  FREEIF (CE_URL_S->content_type);
  CE_URL_S->content_type = XP_STRDUP (MESSAGE_NEWS);

#ifdef NO_ARTICLE_CACHEING
  CE_FORMAT_OUT = CLEAR_CACHE_BIT (CE_FORMAT_OUT);
#endif

  if (CD_TYPE_WANTED == CANCEL_WANTED)
	{
	  XP_ASSERT(CE_FORMAT_OUT == FO_PRESENT);
	  CE_FORMAT_OUT = FO_PRESENT;
	}

  /* Only put stuff in the fe_data if this URL is going to get
	 passed to MIME_MessageConverter(), since that's the only
	 thing that knows what to do with this structure. */
  if (CLEAR_CACHE_BIT(CE_FORMAT_OUT) == FO_PRESENT)
	{
	  CE_STATUS = net_InitializeNewsFeData (cur_entry);
	  if (CE_STATUS < 0)
		{
		  /* #### what error message? */
		  return CE_STATUS;
		}
	}

  CD_STREAM = NET_StreamBuilder(CE_FORMAT_OUT, CE_URL_S, CE_WINDOW_ID);
  XP_ASSERT (CD_STREAM);
  if (!CD_STREAM) return -1;

  CD_NEXT_STATE = NNTP_READ_ARTICLE;

  return 0;
}


PRIVATE int
net_news_begin_authorize(ActiveEntry * cur_entry)
{
	char * command = 0;
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
	char * username = 0;
	char * cp;
	static char * last_username=0;
	static char * last_username_hostname=0;

	/* reused saved username if we think it is
	 * valid
	 */
    if(net_news_last_username_probably_valid
		&& last_username_hostname
		&& !strcasecomp(last_username_hostname, CD_CC_HOSTNAME))
	  {
	    StrAllocCopy(username, last_username);
	  }
	else if((cp = XP_STRCHR(CD_CC_HOSTNAME, '@')))
	  {
		/* in this case the username and possibly
		 * the password are in the URL
		 */
		char * colon;
		*cp = '\0';

		colon = XP_STRCHR(CD_CC_HOSTNAME, ':');
		if(colon)
			*colon = '\0';

		StrAllocCopy(username, CD_CC_HOSTNAME);
		StrAllocCopy(last_username, CD_CC_HOSTNAME);
		StrAllocCopy(last_username_hostname, cp+1);

		*cp = '@';

		if(colon)
			*colon = ':';
	  }
	else
	  {
		username = FE_Prompt(CE_WINDOW_ID,
              			 XP_GetString(XP_PROMPT_ENTER_USERNAME), 
			  			 last_username ? last_username : "");

		StrAllocCopy(last_username, username);
		StrAllocCopy(last_username_hostname, CD_CC_HOSTNAME);
	  }

	if(!username)
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(
									MK_NNTP_AUTH_FAILED,
									"Aborted by user");
        return(MK_NNTP_AUTH_FAILED);
	  }

	StrAllocCopy(command, "AUTHINFO user ");
	StrAllocCat(command, username);
	StrAllocCat(command, CRLF);

	TRACEMSG(("Tx: %s", command));

	CE_STATUS = (int) NET_BlockingWrite(CE_SOCK, command, XP_STRLEN(command));

	FREE(command);
	FREE(username);

	CD_NEXT_STATE = NNTP_RESPONSE;
	CD_NEXT_STATE_AFTER_RESPONSE = NNTP_AUTHORIZE_RESPONSE;;

	CD_PAUSE_FOR_READ = TRUE;

	return CE_STATUS;
}

PRIVATE int
net_news_authorize_response(ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
	static char * last_password = 0;
	static char * last_password_hostname = 0;

    if (281 == CD_RESPONSE_CODE || 250 == CD_RESPONSE_CODE) 
	  {
		/* successfull login, all done
		 */
		CD_NEXT_STATE = SEND_FIRST_NNTP_COMMAND;
		net_news_last_username_probably_valid = TRUE;
		return(0); 
	  }
	else if (381 == CD_RESPONSE_CODE)
	  {
		/* password required
		 */	
		char * command = 0;
		char * password = 0;
		char * cp;

    	/* reused saved username if we think it is
         * valid
         */
        if(net_news_last_username_probably_valid 
			&& last_password
			&& last_password_hostname
			&& !strcasecomp(last_password_hostname, CD_CC_HOSTNAME))
          {
            StrAllocCopy(password, last_password);
          }
        else if((cp = XP_STRCHR(CD_CC_HOSTNAME, '@')))
          {
            /* in this case the username and possibly
             * the password are in the URL
             */
            char * colon;
            *cp = '\0';
    
            colon = XP_STRCHR(CD_CC_HOSTNAME, ':');
            if(colon)
			  {
                *colon = '\0';
    
            	StrAllocCopy(password, colon+1);
            	StrAllocCopy(last_password, colon+1);
            	StrAllocCopy(last_password_hostname, cp+1);

                *colon = ':';
			  }
			else
              {

		        password = FE_PromptPassword(CE_WINDOW_ID, 
				XP_GetString( XP_PLEASE_ENTER_A_PASSWORD_FOR_NEWS_SERVER_ACCESS ) );
			    StrAllocCopy(last_password, password);
            	StrAllocCopy(last_password_hostname, CD_CC_HOSTNAME);
		      }
    
            *cp = '@';
    
          }
        else
          {

		    password = FE_PromptPassword(CE_WINDOW_ID, 
				XP_GetString( XP_PLEASE_ENTER_A_PASSWORD_FOR_NEWS_SERVER_ACCESS ) );
			StrAllocCopy(last_password, password);
            StrAllocCopy(last_password_hostname, CD_CC_HOSTNAME);
		  }

		StrAllocCopy(command, "AUTHINFO pass ");
		StrAllocCat(command, password);
		StrAllocCat(command, CRLF);
	
		TRACEMSG(("Tx: %s", command));

		CE_STATUS = (int) NET_BlockingWrite(CE_SOCK, command, XP_STRLEN(command));

		FREE(command);
		FREE(password);

		CD_NEXT_STATE = NNTP_RESPONSE;
		CD_NEXT_STATE_AFTER_RESPONSE = NNTP_PASSWORD_RESPONSE;;

		return CE_STATUS;
	  }
	else
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(
									MK_NNTP_AUTH_FAILED,
									CD_RESPONSE_TXT ? CD_RESPONSE_TXT : "");
		net_news_last_username_probably_valid = FALSE;
        return(MK_NNTP_AUTH_FAILED);
	  }
		
	XP_ASSERT(0); /* should never get here */
	return(-1);

}

PRIVATE int
net_news_password_response(ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

    if (281 == CD_RESPONSE_CODE || 250 == CD_RESPONSE_CODE) 
	  {
        /* successfull login, all done
         */
        CD_NEXT_STATE = SEND_FIRST_NNTP_COMMAND;
		net_news_last_username_probably_valid = TRUE;
		MSG_ResetXOVER( CE_WINDOW_ID, &CE_XOVER_PARSE_STATE );
        return(0);
	  }
	else
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(
									MK_NNTP_AUTH_FAILED,
									CD_RESPONSE_TXT ? CD_RESPONSE_TXT : "");
        return(MK_NNTP_AUTH_FAILED);
	  }
		
	XP_ASSERT(0); /* should never get here */
	return(-1);
}

PRIVATE int
net_display_newsgroups (ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

	CD_NEXT_STATE = NEWS_DONE;
    CD_PAUSE_FOR_READ = FALSE;

	TRACEMSG(("about to display newsgroups. path: %s",CD_PATH));

#if 0
	/* #### Now ignoring "news:alt.fan.*"
	   Need to open the root tree of the default news host and keep
	   opening one child at each level until we've exhausted the
	   wildcard...
	 */
	if(rv < 0)
       return(rv);  
	else
#endif
       return(MK_DATA_LOADED);  /* all finished */
}

PRIVATE int
net_newgroups_begin (ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
	time_t last_modified_time = NET_NewsgroupsLastUpdatedTime(CD_CC_HOSTNAME,
															  CD_CC_SECURE);

	last_modified_time -= NEWGROUPS_TIME_OFFSET;

	CD_NEXT_STATE = NNTP_NEWGROUPS;
	NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_RECEIVE_NEWSGROUP));

	CE_BYTES_RECEIVED = 0;

	return(CE_STATUS);

}

PRIVATE int
net_process_newgroups (ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
	char *line, *s, *s1, *s2;
	int32 oldest, youngest;

    CE_STATUS = NET_BufferedReadLine(CE_SOCK, &line, &CD_DATA_BUF,
                                        &CD_DATA_BUF_SIZE, (Bool*)&CD_PAUSE_FOR_READ);

    if(CE_STATUS == 0)
      {
        CD_NEXT_STATE = NNTP_ERROR;
        CD_PAUSE_FOR_READ = FALSE;
        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_NNTP_SERVER_ERROR);
        return(MK_NNTP_SERVER_ERROR);
      }

    if(!line)
        return(CE_STATUS);  /* no line yet */

    if(CE_STATUS<0)
      {
        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, SOCKET_ERRNO);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
      }

    /* End of list? 
	 */
    if (line[0]=='.' && line[1]=='\0')
      {
        CD_NEXT_STATE = NEWS_DONE;
        CD_PAUSE_FOR_READ = FALSE;

		if(CE_BYTES_RECEIVED == 0)
		  {
			/* #### no new groups */
		  }

		NET_SortNewsgroups(CD_CC_HOSTNAME, CD_CC_SECURE);
        NET_SaveNewsgroupsToDisk(CD_CC_HOSTNAME, CD_CC_SECURE);
		if(CE_STATUS > 0)
        	return MK_DATA_LOADED;
		else
        	return CE_STATUS;
      }
    else if (line [0] == '.' && line [1] == '.')
      /* The NNTP server quotes all lines beginning with "." by doubling it. */
      line++;

    /* almost correct
     */
    if(CE_STATUS > 1)
      {
        CE_BYTES_RECEIVED += CE_STATUS;
        FE_GraphProgress(CE_WINDOW_ID, CE_URL_S, CE_BYTES_RECEIVED, CE_STATUS, CE_URL_S->content_length);
      }

    /* format is "rec.arts.movies.past-films 7302 7119 y"
	 */
	s = XP_STRCHR (line, ' ');
	if (s)
	  {
		*s = 0;
		s1 = s+1;
		s = XP_STRCHR (s1, ' ');
		if (s)
		  {
			*s = 0;
			s2 = s+1;
			s = XP_STRCHR (s2, ' ');
			if (s)
			  *s = 0;
		  }
	  }
	youngest = s2 ? atol(s1) : 0;
	oldest   = s1 ? atol(s2) : 0;

	CE_BYTES_RECEIVED++;  /* small numbers of groups never seem
						   * to trigger this
						   */

	/* Tell libmsg to display this group. */
	CE_STATUS = MSG_DisplayNewNewsGroup(CE_WINDOW_ID, CD_CC_HOSTNAME,
										CD_CC_SECURE,
										line, oldest, youngest);

    /* store the group name in the active file cache - note that this
	   destroys the value in `line'. */
    NET_StoreNewsGroup(CD_CC_HOSTNAME, CD_CC_SECURE, line);

    return(CE_STATUS);
}

		
/* Ahhh, this like print's out the headers and stuff
 *
 * always reterns 0
 */
PRIVATE int
net_read_news_list_begin (ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

    CD_NEXT_STATE = NNTP_READ_LIST;

	NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_RECEIVE_NEWSGROUP));
	 
    return(CE_STATUS);


}

/* display a list of all or part of the newsgroups list
 * from the news server
 */
PRIVATE int
net_read_news_list (ActiveEntry *cur_entry)
{
    char * line;
    char * description;
    int i=0;
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
   
    CE_STATUS = NET_BufferedReadLine(CE_SOCK, &line, &CD_DATA_BUF, 
                       					&CD_DATA_BUF_SIZE, (Bool*) &CD_PAUSE_FOR_READ);

    if(CE_STATUS == 0)
      {
        CD_NEXT_STATE = NNTP_ERROR;
        CD_PAUSE_FOR_READ = FALSE;
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_NNTP_SERVER_ERROR);
        return(MK_NNTP_SERVER_ERROR);
      }

    if(!line)
        return(CE_STATUS);  /* no line yet */

    if(CE_STATUS<0)
	  {
        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, SOCKET_ERRNO);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
	  }

            /* End of list? */
    if (line[0]=='.' && line[1]=='\0')
      {
        CD_NEXT_STATE = DISPLAY_NEWSGROUPS;
        CD_PAUSE_FOR_READ = FALSE;

		NET_SortNewsgroups(CD_CC_HOSTNAME, CD_CC_SECURE);
		NET_SaveNewsgroupsToDisk(CD_CC_HOSTNAME, CD_CC_SECURE);
        return 0;  
      }
	else if (line [0] == '.' && line [1] == '.')
	  /* The NNTP server quotes all lines beginning with "." by doubling it. */
	  line++;

    /* almost correct
     */
    if(CE_STATUS > 1)
      {
    	CE_BYTES_RECEIVED += CE_STATUS;
        FE_GraphProgress(CE_WINDOW_ID, CE_URL_S, CE_BYTES_RECEIVED, CE_STATUS, CE_URL_S->content_length);
	  }

    /* find whitespace seperator if it exits */
    for(i=0; line[i] != '\0' && !XP_IS_SPACE(line[i]); i++)
        ;  /* null body */

    if(line[i] == '\0')
        description = &line[i];
    else
        description = &line[i+1];

    line[i] = 0; /* terminate group name */

	/* store all the group names 
	 */
	NET_StoreNewsGroup(CD_CC_HOSTNAME, CD_CC_SECURE, line);

    return(CE_STATUS);
}




/* start the xover command
 */
PRIVATE int
net_read_xover_begin (ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
    int32 count;     /* Response fields */

	/* Make sure we never close and automatically reopen the connection at this
	   point; we'll confuse libmsg too much... */

	CD_SOME_PROTOCOL_SUCCEEDED = TRUE;


	/* We have just issued a GROUP command and read the response.
	   Now parse that response to help decide which articles to request
	   xover data for.
	 */
    sscanf(CD_RESPONSE_TXT,
#ifdef __alpha
		   "%d %d %d", 
#else
		   "%ld %ld %ld", 
#endif
		   &count, 
		   &CD_FIRST_POSSIBLE_ART, 
		   &CD_LAST_POSSIBLE_ART);

	/* We now know there is a summary line there; make sure it has the
	   right numbers in it. */
	CE_STATUS = MSG_DisplaySubscribedGroup(CE_WINDOW_ID, CD_GROUP_NAME,
										   CD_FIRST_POSSIBLE_ART,
										   CD_LAST_POSSIBLE_ART,
										   count, TRUE);
	if (CE_STATUS < 0) return CE_STATUS;

	CD_NUM_LOADED = 0;
	CD_NUM_WANTED = net_NewsChunkSize > 0 ? net_NewsChunkSize : 1L << 30;

	CD_NEXT_STATE = NNTP_FIGURE_NEXT_CHUNK;
	CD_PAUSE_FOR_READ = FALSE;
	return 0;
}



PRIVATE int
net_figure_next_chunk(ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

	XP_Bool secure_p = (CE_URL_S->address[0] == 's' ||
						CE_URL_S->address[0] == 'S');
	char *host_and_port = NET_ParseURL (CE_URL_S->address, GET_HOST_PART);

	if (!host_and_port) return MK_OUT_OF_MEMORY;

	if (CD_FIRST_ART > 0) {
	  CE_STATUS = MSG_AddToKnownArticles(CE_WINDOW_ID, host_and_port, secure_p,
										 CD_GROUP_NAME,
										 CD_FIRST_ART, CD_LAST_ART);
	  if (CE_STATUS < 0) {
		FREEIF (host_and_port);
		return CE_STATUS;
	  }
	}
										 

	if (CD_NUM_LOADED >= CD_NUM_WANTED) {
	  FREEIF (host_and_port);
	  CD_NEXT_STATE = NEWS_PROCESS_XOVER;
	  CD_PAUSE_FOR_READ = FALSE;
	  return 0;
	}


	CE_STATUS = MSG_GetRangeOfArtsToDownload(CE_WINDOW_ID, host_and_port,
											 secure_p, CD_GROUP_NAME,
											 CD_FIRST_POSSIBLE_ART,
											 CD_LAST_POSSIBLE_ART,
											 CD_NUM_WANTED - CD_NUM_LOADED,
											 &(CD_FIRST_ART),
											 &(CD_LAST_ART));

	if (CE_STATUS < 0) {
	  FREEIF (host_and_port);
	  return CE_STATUS;
	}


	if (CD_FIRST_ART <= 0 || CD_FIRST_ART > CD_LAST_ART) {
	  /* Nothing more to get. */
	  FREEIF (host_and_port);
	  CD_NEXT_STATE = NEWS_PROCESS_XOVER;
	  CD_PAUSE_FOR_READ = FALSE;
	  return 0;
	}

    TRACEMSG(("    Chunk will be (%ld-%ld)\n", CD_FIRST_ART, CD_LAST_ART));

	CD_ARTICLE_NUM = CD_FIRST_ART;
	CE_STATUS = MSG_InitXOVER (CE_WINDOW_ID,
							   host_and_port, secure_p, CD_GROUP_NAME,
							   CD_FIRST_ART, CD_LAST_ART,
							   CD_FIRST_POSSIBLE_ART, CD_LAST_POSSIBLE_ART,
							   &CE_XOVER_PARSE_STATE);
	FREEIF (host_and_port);

	if (CE_STATUS < 0) {
	  return CE_STATUS;
	}

	CD_PAUSE_FOR_READ = FALSE;
	if (CD_CC_NO_XOVER) CD_NEXT_STATE = NNTP_READ_GROUP;
	else CD_NEXT_STATE = NNTP_XOVER_SEND;

	return 0;
}

PRIVATE int
net_xover_send (ActiveEntry *cur_entry)
{		
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

    PR_snprintf(CD_OUTPUT_BUFFER, 
				OUTPUT_BUFFER_SIZE,
				"XOVER %ld-%ld" CRLF, 
				CD_FIRST_ART, 
				CD_LAST_ART);

	/* printf("XOVER %ld-%ld\n", CD_FIRST_ART, CD_LAST_ART); */

	TRACEMSG(("Tx: %s", CD_OUTPUT_BUFFER));

    CD_NEXT_STATE = NNTP_RESPONSE;
    CD_NEXT_STATE_AFTER_RESPONSE = NNTP_XOVER_RESPONSE;
    CD_PAUSE_FOR_READ = TRUE;

	NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_RECEIVE_LISTARTICLES));

    return((int) NET_BlockingWrite(CE_SOCK, 
								   CD_OUTPUT_BUFFER, 
								   XP_STRLEN(CD_OUTPUT_BUFFER)));

}

/* see if the xover response is going to return us data
 * if the proper code isn't returned then assume xover
 * isn't supported and use
 * normal read_group
 */
PRIVATE int
net_read_xover_response (ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
    if(CD_RESPONSE_CODE != 224)
      {
        /* If we didn't get back "224 data follows" from the XOVER request,
		   then that must mean that this server doesn't support XOVER.  Or
		   maybe the server's XOVER support is busted or something.  So,
		   in that case, fall back to the very slow HEAD method.

		   But, while debugging here at HQ, getting into this state means
		   something went very wrong, since our servers do XOVER.  Thus
		   the assert.
         */
		/*XP_ASSERT (0);*/
		CD_NEXT_STATE = NNTP_READ_GROUP;
		CD_CC_NO_XOVER = TRUE;
      }
    else
      {
        CD_NEXT_STATE = NNTP_XOVER;
      }

    return(0);  /* continue */
}

/* process the xover list as it comes from the server
 * and load it into the sort list.  
 */
PRIVATE int
net_read_xover (ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
    char *line;

    CE_STATUS = NET_BufferedReadLine(CE_SOCK, &line, &CD_DATA_BUF,
									 &CD_DATA_BUF_SIZE,
									 (Bool*)&CD_PAUSE_FOR_READ);

    if(CE_STATUS == 0)
      {
		TRACEMSG(("received unexpected TCP EOF!!!!  aborting!"));
        CD_NEXT_STATE = NNTP_ERROR;
        CD_PAUSE_FOR_READ = FALSE;
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_NNTP_SERVER_ERROR);
        return(MK_NNTP_SERVER_ERROR);
      }

    if(!line)
	  {
	    return(CE_STATUS);  /* no line yet or TCP error */
	  }

	if(CE_STATUS<0) 
	  {
        CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR,
													  SOCKET_ERRNO);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
	  }

    if(line[0] == '.' && line[1] == '\0')
      {
		CD_NEXT_STATE = NNTP_FIGURE_NEXT_CHUNK;
		CD_PAUSE_FOR_READ = FALSE;
		return(0);
      }
	else if (line [0] == '.' && line [1] == '.')
	  /* The NNTP server quotes all lines beginning with "." by doubling it. */
	  line++;

    /* almost correct
     */
    if(CE_STATUS > 1)
      {
        CE_BYTES_RECEIVED += CE_STATUS;
        FE_GraphProgress(CE_WINDOW_ID, CE_URL_S, CE_BYTES_RECEIVED, CE_STATUS,
						 CE_URL_S->content_length);
	  }

	CE_STATUS = MSG_ProcessXOVER (CE_WINDOW_ID, line, &CE_XOVER_PARSE_STATE);

	CD_NUM_LOADED++;

    return CE_STATUS; /* keep going */
}


/* Finished processing all the XOVER data.
 */
PRIVATE int
net_process_xover (ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;


	if (CE_XOVER_PARSE_STATE) {
	  CE_STATUS = MSG_FinishXOVER (CE_WINDOW_ID, &CE_XOVER_PARSE_STATE, 0);
	  XP_ASSERT (!CE_XOVER_PARSE_STATE);
	  if (CE_STATUS < 0) return CE_STATUS;
	}

	CD_NEXT_STATE = NEWS_DONE;

    return(MK_DATA_LOADED);
}
 



PRIVATE int
net_read_news_group (ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

    if(CD_ARTICLE_NUM > CD_LAST_ART)
      {  /* end of groups */

		CD_NEXT_STATE = NNTP_FIGURE_NEXT_CHUNK;
		CD_PAUSE_FOR_READ = FALSE;
		return(0);
      }
    else
      {
        PR_snprintf(CD_OUTPUT_BUFFER, 
				   OUTPUT_BUFFER_SIZE,  
				   "HEAD %ld" CRLF, 
				   CD_ARTICLE_NUM++);
        CD_NEXT_STATE = NNTP_RESPONSE;
		CD_NEXT_STATE_AFTER_RESPONSE = NNTP_READ_GROUP_RESPONSE;

        CD_PAUSE_FOR_READ = TRUE;

        return((int) NET_BlockingWrite(CE_SOCK, CD_OUTPUT_BUFFER, XP_STRLEN(CD_OUTPUT_BUFFER)));
      }
}

/* See if the "HEAD" command was successful
 */
PRIVATE int
net_read_news_group_response (ActiveEntry *cur_entry)
{
  NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

  if (CD_RESPONSE_CODE == 221)
	{     /* Head follows - parse it:*/
	  CD_NEXT_STATE = NNTP_READ_GROUP_BODY;

	  if(CD_MESSAGE_ID)
		*CD_MESSAGE_ID = '\0';

	  /* Give the message number to the header parser. */
	  return MSG_ProcessNonXOVER (CE_WINDOW_ID, CD_RESPONSE_TXT,
								  &CE_XOVER_PARSE_STATE);
	}
  else
	{
	  TRACEMSG(("Bad group header found!"));
	  CD_NEXT_STATE = NNTP_READ_GROUP;
	  return(0);
	}
}

/* read the body of the "HEAD" command
 */
PRIVATE int
net_read_news_group_body (ActiveEntry *cur_entry)
{
  NewsConData * connection_data = (NewsConData *)cur_entry->con_data;
  char *line;

  CE_STATUS = NET_BufferedReadLine(CE_SOCK, &line, &CD_DATA_BUF,
								   &CD_DATA_BUF_SIZE, (Bool*)&CD_PAUSE_FOR_READ);

  if(CE_STATUS == 0)
	{
	  CD_NEXT_STATE = NNTP_ERROR;
	  CD_PAUSE_FOR_READ = FALSE;
	  CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_NNTP_SERVER_ERROR);
	  return(MK_NNTP_SERVER_ERROR);
	}

  /* if TCP error of if there is not a full line yet return
   */
  if(!line)
	return CE_STATUS;

  if(CE_STATUS < 0)
	{
	  CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, SOCKET_ERRNO);

	  /* return TCP error
	   */
	  return MK_TCP_READ_ERROR;
	}

  TRACEMSG(("read_group_body: got line: %s|",line));

  /* End of body? */
  if (line[0]=='.' && line[1]=='\0')
	{
	  CD_NEXT_STATE = NNTP_READ_GROUP;
	  CD_PAUSE_FOR_READ = FALSE;
	}
  else if (line [0] == '.' && line [1] == '.')
	/* The NNTP server quotes all lines beginning with "." by doubling it. */
	line++;

  return MSG_ProcessNonXOVER (CE_WINDOW_ID, line, &CE_XOVER_PARSE_STATE);
}


PRIVATE int
net_send_news_post_data(ActiveEntry * cur_entry)
{
    NewsConData * cd = (NewsConData *) cur_entry->con_data;

    /* returns 0 on done and negative on error
     * positive if it needs to continue.
     */
    CE_STATUS = NET_WritePostData(CE_WINDOW_ID, CE_URL_S,
                                  CE_SOCK,
								  &cd->write_post_data_data,
								  TRUE);

    cd->pause_for_read = TRUE;

    if(CE_STATUS == 0)
      {
        /* normal done
         */
        XP_STRCPY(cd->output_buffer, CRLF "." CRLF CRLF);
        TRACEMSG(("sending %s", cd->output_buffer));
        CE_STATUS = (int) NET_BlockingWrite(CE_SOCK,
                                            cd->output_buffer,
                                            XP_STRLEN(cd->output_buffer));

		NET_Progress(CE_WINDOW_ID,
					XP_GetString(XP_MESSAGE_SENT_WAITING_NEWS_REPLY));

        FE_ClearConnectSelect(CE_WINDOW_ID, CE_SOCK);
#ifdef XP_WIN
		if(cd->calling_netlib_all_the_time)
		{
			net_call_all_the_time_count--;
			cd->calling_netlib_all_the_time = FALSE;
			if(net_call_all_the_time_count == 0)
				FE_ClearCallNetlibAllTheTime(CE_WINDOW_ID);
		}
#endif
        FE_SetReadSelect(CE_WINDOW_ID, CE_SOCK);
		CE_CON_SOCK = 0;

        cd->next_state = NNTP_RESPONSE;
        cd->next_state_after_response = NNTP_SEND_POST_DATA_RESPONSE;
        return(0);
      }

    return(CE_STATUS);

}


/* interpret the response code from the server
 * after the post is done
 */   
PRIVATE int
net_send_news_post_data_response(ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *) cur_entry->con_data;


	if (CD_RESPONSE_CODE != 240) {
	  CE_URL_S->error_msg =
		NET_ExplainErrorDetails(MK_NNTP_ERROR_MESSAGE, 
								CD_RESPONSE_TXT ? CD_RESPONSE_TXT : "");
	  if (CD_RESPONSE_CODE == 441 && MSG_IsDuplicatePost(CE_WINDOW_ID) &&
		  MSG_GetMessageID(CE_WINDOW_ID)) {
		/* The news server won't let us post.  We suspect that we're submitting
		   a duplicate post, and that's why it's failing.  So, let's go see
		   if there really is a message out there with the same message-id.
		   If so, we'll just silently pretend everything went well. */
		PR_snprintf(CD_OUTPUT_BUFFER, OUTPUT_BUFFER_SIZE, "STAT %s" CRLF,
					MSG_GetMessageID(CE_WINDOW_ID));
		CD_NEXT_STATE = NNTP_RESPONSE;
		CD_NEXT_STATE_AFTER_RESPONSE = NNTP_CHECK_FOR_MESSAGE;
		return (int) NET_BlockingWrite(CE_SOCK, CD_OUTPUT_BUFFER,
									   XP_STRLEN(CD_OUTPUT_BUFFER));
	  }

	  MSG_ClearMessageID(CE_WINDOW_ID); /* So that if the user tries to just
										   post again, we won't immediately
										   decide that this was a duplicate
										   message and ignore the error.*/
	  CD_NEXT_STATE = NEWS_ERROR;
	  return(MK_NNTP_ERROR_MESSAGE);
	}
    CD_NEXT_STATE = NEWS_ERROR; /* even though it worked */
	CD_PAUSE_FOR_READ = FALSE;
    return(MK_DATA_LOADED);
}


PRIVATE int
net_check_for_message(ActiveEntry* cur_entry)
{
  NewsConData * connection_data = (NewsConData *) cur_entry->con_data;

  CD_NEXT_STATE = NEWS_ERROR;
  if (CD_RESPONSE_CODE >= 220 && CD_RESPONSE_CODE <= 223) {
	/* Yes, this article is already there, we're all done. */
	return MK_DATA_LOADED;
  } else {
	/* The article isn't there, so the failure we had earlier wasn't due to
	   a duplicate message-id.  Return the error from that previous
	   posting attempt (which is already in CE_URL_S->error_msg). */
	MSG_ClearMessageID(CE_WINDOW_ID);
	return MK_NNTP_ERROR_MESSAGE;
  }
}

PRIVATE int
net_DisplayNewsRC(ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *) cur_entry->con_data;

	if(!CD_NEWSRC_PERFORMED)
	  {
		CD_NEWSRC_PERFORMED = TRUE;

		CD_NEWSRC_LIST = MSG_GetNewsRCList (CE_WINDOW_ID,
											CD_CC_HOSTNAME, CD_CC_SECURE);
		CD_NEWSRC_LIST_COUNT = 0;
		if (CD_NEWSRC_LIST)
		  while (CD_NEWSRC_LIST[CD_NEWSRC_LIST_COUNT])
			CD_NEWSRC_LIST_COUNT++;
	  }
	
	if(CD_NEWSRC_LIST && CD_NEWSRC_LIST [CD_NEWSRC_LIST_INDEX])
      {
		/* send group command to server
		 */
		int32 percent;
		StrAllocCopy (CD_CC_GROUP, CD_NEWSRC_LIST [CD_NEWSRC_LIST_INDEX]);
		PR_snprintf(NET_Socket_Buffer, OUTPUT_BUFFER_SIZE, "GROUP %.512s" CRLF, CD_CC_GROUP);
    	CE_STATUS = (int) NET_BlockingWrite(CE_SOCK, NET_Socket_Buffer,
											XP_STRLEN(NET_Socket_Buffer));

		TRACEMSG(("Tx: %s", NET_Socket_Buffer));

		percent = 100 * (((double) CD_NEWSRC_LIST_INDEX) /
						 ((double) CD_NEWSRC_LIST_COUNT));
		FE_SetProgressBarPercent (CE_WINDOW_ID, percent);

		CD_NEWSRC_LIST_INDEX++;
		if (!CD_NEWSRC_LIST [CD_NEWSRC_LIST_INDEX])
		  {
			XP_FREE (CD_NEWSRC_LIST);
			CD_NEWSRC_LIST = 0;
		  }

		CD_PAUSE_FOR_READ = TRUE;
		CD_NEXT_STATE = NNTP_RESPONSE;
		CD_NEXT_STATE_AFTER_RESPONSE = NEWS_DISPLAY_NEWS_RC_RESPONSE;
      }
	else
	  {
		if (CD_NEWSRC_LIST)
		  {
			FE_SetProgressBarPercent (CE_WINDOW_ID, -1);
			XP_FREE (CD_NEWSRC_LIST);
			CD_NEWSRC_LIST = 0;
		  }
		else if (CD_RESPONSE_CODE == 215)  
		  {
			/*
			 * 5-9-96 jefft 
			 * If for some reason the news server returns an empty 
			 * newsgroups list with a nntp response code 215 -- list of
			 * newsgroups follows. We set CE_STATUS to MK_EMPTY_NEWS_LIST
			 * to end the infinite dialog loop.
			 */
			CE_STATUS = MK_EMPTY_NEWS_LIST;
		  }
		CD_NEXT_STATE = NEWS_DONE;
	
		if(CE_STATUS > -1)
		  return MK_DATA_LOADED; 
		else
		  return(CE_STATUS);
	  }

	return(CE_STATUS); /* keep going */

}

/* Parses output of GROUP command */
PRIVATE int
net_DisplayNewsRCResponse(ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *) cur_entry->con_data;

    if(CD_RESPONSE_CODE == 211)
      {
		char *num_arts = 0, *low = 0, *high = 0, *group = 0;
		int32 first_art, last_art;

		/* line looks like:
		 *     211 91 3693 3789 comp.infosystems
		 */

		num_arts = CD_RESPONSE_TXT;
		low = XP_STRCHR(num_arts, ' ');

		if(low)
		  {
			first_art = atol(low);
			*low++ = '\0';
			high= XP_STRCHR(low, ' ');
		  }
		if(high)
		  {
			*high++ = '\0';
			group = XP_STRCHR(high, ' ');
		  }
		if(group)
		  {
			*group++ = '\0';
			/* the group name may be contaminated by "group selected" at
			   the end.  This will be space separated from the group name.
			   If a space is found in the group name terminate at that
			   point. */
			XP_STRTOK(group, " ");
			last_art = atol(high);
		  }
    
		CE_STATUS = MSG_DisplaySubscribedGroup (CE_WINDOW_ID, group,
												low  ? atol(low)  : 0,
												high ? atol(high) : 0,
												atol(num_arts), FALSE);
		if (CE_STATUS < 0)
		  return CE_STATUS;
	  }
	else
	  {
		/* only on news server error or when zero articles
		 */
		/* #### nonexistent group, -or- 0 articles */
	  }

	CD_NEXT_STATE = NEWS_DISPLAY_NEWS_RC;
		
	return 0;
}


PRIVATE int
net_NewsRCProcessPost(ActiveEntry *cur_entry)
{
	return(0);
}



static void
net_cancel_done_cb (MWContext *context, void *data, int status,
					const char *file_name)
{
  ActiveEntry *cur_entry = (ActiveEntry *) data;
  NewsConData *connection_data = (NewsConData *) cur_entry->con_data;
  connection_data->cancel_status = status;
  XP_ASSERT(status < 0 || file_name);
  connection_data->cancel_msg_file = (status < 0 ? 0 : XP_STRDUP(file_name));
}


static int
net_start_cancel (ActiveEntry *cur_entry)
{
  NewsConData *connection_data = (NewsConData *) cur_entry->con_data;
  char *command = "POST" CRLF;

  CE_STATUS = (int) NET_BlockingWrite(CE_SOCK, command, XP_STRLEN(command));

  CD_NEXT_STATE = NNTP_RESPONSE;
  CD_NEXT_STATE_AFTER_RESPONSE = NEWS_DO_CANCEL;
  CD_PAUSE_FOR_READ = TRUE;
  return (CE_STATUS);
}


static int
net_do_cancel (ActiveEntry *cur_entry)
{
  int status = 0;
  NewsConData *connection_data = (NewsConData *) cur_entry->con_data;
  char *id, *subject, *newsgroups, *distribution, *other_random_headers, *body;
  char *from, *old_from, *news_url;
  int L;

  /* #### Should we do a more real check than this?  If the POST command
	 didn't respond with "340 Ok", then it's not ready for us to throw a
	 message at it...   But the normal posting code doesn't do this check.
	 Why?
   */
  XP_ASSERT (CD_RESPONSE_CODE == 340);

  /* These shouldn't be set yet, since the headers haven't been "flushed" */
  XP_ASSERT (!connection_data->cancel_id &&
			 !connection_data->cancel_from &&
			 !connection_data->cancel_newsgroups &&
			 !connection_data->cancel_distribution);

  /* Write out a blank line.  This will tell mimehtml.c that the headers
	 are done, and it will call news_generate_html_header_fn which will
	 notice the fields we're interested in.
   */
  XP_STRCPY (CD_OUTPUT_BUFFER, CRLF); /* CRLF used to be LINEBREAK. 
  										 LINEBREAK is platform dependent
  										 and is only <CR> on a mac. This
										 CRLF is the protocol delimiter 
										 and not platform dependent  -km */
  CE_STATUS = PUTSTRING(CD_OUTPUT_BUFFER);
  if (CE_STATUS < 0) return CE_STATUS;

  /* Now news_generate_html_header_fn should have been called, and these
	 should have values. */
  id = connection_data->cancel_id;
  old_from = connection_data->cancel_from;
  newsgroups = connection_data->cancel_newsgroups;
  distribution = connection_data->cancel_distribution;

  XP_ASSERT (id && newsgroups);
  if (!id || !newsgroups) return -1; /* "unknown error"... */

  connection_data->cancel_newsgroups = 0;
  connection_data->cancel_distribution = 0;
  connection_data->cancel_from = 0;
  connection_data->cancel_id = 0;

  L = XP_STRLEN (id);

  from = MIME_MakeFromField ();
  subject = (char *) XP_ALLOC (L + 20);
  other_random_headers = (char *) XP_ALLOC (L + 20);
  body = (char *) XP_ALLOC (XP_STRLEN (XP_AppCodeName) + 100);

  /* Make sure that this loser isn't cancelling someone else's posting.
	 Yes, there are occasionally good reasons to do so.  Those people
	 capable of making that decision (news admins) have other tools with
	 which to cancel postings (like telnet.)
   */
  {
	char *us = MSG_ExtractRFC822AddressMailboxes (from);
	char *them = MSG_ExtractRFC822AddressMailboxes (old_from);
	XP_Bool ok = (us && them && !strcasecomp (us, them));
	FREEIF(us);
	FREEIF(them);
	if (!ok)
	  {
		status = MK_NNTP_CANCEL_DISALLOWED;
		CE_URL_S->error_msg = XP_GetString(status);
		CD_NEXT_STATE = NEWS_ERROR; /* even though it worked */
		CD_PAUSE_FOR_READ = FALSE;
		goto FAIL;
	  }
  }

  /* Last chance to cancel the cancel.
   */
  if (!FE_Confirm (CE_WINDOW_ID, XP_GetString(MK_NNTP_CANCEL_CONFIRM)))
	{
	  status = MK_NNTP_NOT_CANCELLED;
	  goto FAIL;
	}

  news_url = CE_URL_S->address;  /* we can just post here. */

  if (!from || !subject || !other_random_headers || !body)
	{
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}

  XP_STRCPY (subject, "cancel ");
  XP_STRCAT (subject, id);

  XP_STRCPY (other_random_headers, "Control: cancel ");
  XP_STRCAT (other_random_headers, id);
  XP_STRCAT (other_random_headers, CRLF);
  if (distribution)
	{
	  XP_STRCAT (other_random_headers, "Distribution: ");
	  XP_STRCAT (other_random_headers, distribution);
	  XP_STRCAT (other_random_headers, CRLF);
	}

  XP_STRCPY (body, "This message was cancelled from within ");
  XP_STRCAT (body, XP_AppCodeName);
  XP_STRCAT (body, "." CRLF);

  connection_data->cancel_status = 0;

  /* #### Would it be a good idea to S/MIME-sign all "cancel" messages? */

  MSG_StartMessageDelivery (CE_WINDOW_ID, (void *) cur_entry,
							from,
							0, /* reply_to */
							0, /* to */
							0, /* cc */
							0, /* bcc */
							0, /* fcc */
							newsgroups,
							0, /* followup_to */
							0, /* organization */
							0, /* message_id */
							news_url,
							subject,
							id, /* references */
							other_random_headers,
							FALSE, /* digest_p */
							TRUE,  /* dont_deliver_p */
							FALSE, /* queue_for_later_p */
							FALSE, /* encrypt_p */
							FALSE, /* sign_p */
							TEXT_PLAIN, body, XP_STRLEN (body),
							0, /* other attachments */
							net_cancel_done_cb);

  /* Since there are no attachments, MSG_StartMessageDelivery will run
	 net_cancel_done_cb right away (it will be called before the return.) */

  if (!connection_data->cancel_msg_file)
	{
	  status = connection_data->cancel_status;
	  XP_ASSERT (status < 0);
	  if (status >= 0) status = -1;
	  goto FAIL;
	}

  /* Now send the data - do it blocking, who cares; the message is known
	 to be very small.  First suck the whole file into memory.  Then delete
	 the file.  Then do a blocking write of the data.

	 (We could use file-posting, maybe, but I couldn't figure out how.)
   */
  {
	char *data;
	uint32 data_size, data_fp;
	XP_StatStruct st;
	int nread = 0;
	XP_File file = XP_FileOpen (connection_data->cancel_msg_file,
								xpFileToPost, XP_FILE_READ);
	if (! file) return -1; /* "unknown error"... */
	XP_Stat (connection_data->cancel_msg_file, &st, xpFileToPost);

	data_fp = 0;
	data_size = st.st_size + 20;
	data = (char *) XP_ALLOC (data_size);
	if (! data)
	  {
		status = MK_OUT_OF_MEMORY;
		goto FAIL;
	  }

	while ((nread = XP_FileRead (data + data_fp, data_size - data_fp, file))
		   > 0)
	  data_fp += nread;
	data [data_fp] = 0;
	XP_FileClose (file);
	XP_FileRemove (connection_data->cancel_msg_file, xpFileToPost);

	XP_STRCAT (data, CRLF "." CRLF CRLF);
	status = NET_BlockingWrite(CE_SOCK, data, XP_STRLEN(data));
	XP_FREE (data);
    if (status < 0)
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_TCP_WRITE_ERROR,
													  status);
		goto FAIL;
	  }

    connection_data->pause_for_read = TRUE;
	connection_data->next_state = NNTP_RESPONSE;
	connection_data->next_state_after_response = NNTP_SEND_POST_DATA_RESPONSE;
  }

 FAIL:
  FREEIF (id);
  FREEIF (from);
  FREEIF (old_from);
  FREEIF (subject);
  FREEIF (newsgroups);
  FREEIF (distribution);
  FREEIF (other_random_headers);
  FREEIF (body);
  FREEIF (connection_data->cancel_msg_file);
  return status;
}



/* The main news load init routine, and URL parser.
   The syntax of news URLs is:

	 To list all hosts:
	   news:

	 To list all groups on a host, or to post a new message:
	   news://HOST

	 To list some articles in a group:
	   news:GROUP
	   news:/GROUP
	   news://HOST/GROUP

	 To list a particular range of articles in a group:
	   news:GROUP/N1-N2
	   news:/GROUP/N1-N2
	   news://HOST/GROUP/N1-N2

	 To retrieve an article:
	   news:MESSAGE-ID                (message-id must contain "@")

    To cancel an article:
	   news:MESSAGE-ID?cancel

	 (Note that message IDs may contain / before the @:)

	   news:SOME/ID@HOST?headers=all
	   news:/SOME/ID@HOST?headers=all
	   news:/SOME?ID@HOST?headers=all
        are the same as
	   news://HOST/SOME/ID@HOST?headers=all
	   news://HOST//SOME/ID@HOST?headers=all
	   news://HOST//SOME?ID@HOST?headers=all
        bug: if the ID is <//xxx@host> we will parse this correctly:
          news://non-default-host///xxx@host
        but will mis-parse it if it's on the default host:
          news://xxx@host
        whoever had the idea to leave the <> off the IDs should be flogged.
		So, we'll make sure we quote / in message IDs as %2F.

 */
static int
net_parse_news_url (const char *url,
					char **host_and_portP,
					XP_Bool *securepP,
					char **groupP,
					char **message_idP,
					char **search_dataP)
{
  int status = 0;
  XP_Bool secure_p = FALSE;
  char *host_and_port = 0;
  int32 port = 0;
  char *group = 0;
  char *message_id = 0;
  char *search_data = 0;
  const char *path_part;
  char *s;

  if (url[0] == 's' || url[1] == 'S')
	{
	  secure_p = TRUE;
	  url++;
	}

  host_and_port = NET_ParseURL (url, GET_HOST_PART);
  if (!host_and_port || !*host_and_port)
	{
	  FREEIF (host_and_port);
	  host_and_port = NET_NewsHost ? XP_STRDUP (NET_NewsHost) : 0;
	}

  if (!host_and_port)
	{
	  status = MK_NO_NEWS_SERVER;
	  goto FAIL;
	}

  /* If a port was specified, but it was the default port, pretend
	 it wasn't specified.
   */
  s = XP_STRCHR (host_and_port, ':');
  if (s && sscanf (s+1, " %lu ", &port) == 1 &&
	  port == (secure_p ? SECURE_NEWS_PORT : NEWS_PORT))
	*s = 0;

  path_part = XP_STRCHR (url, ':');
  XP_ASSERT (path_part);
  if (!path_part)
	{
	  status = -1;
	  goto FAIL;
	}
  path_part++;
  if (path_part[0] == '/' && path_part[1] == '/')
	{
	  /* Skip over host name. */
	  path_part = XP_STRCHR (path_part + 2, '/');
	  if (path_part)
		path_part++;
	}
  if (!path_part)
	path_part = "";

  group = XP_STRDUP (path_part);
  if (!group)
	{
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}

  NET_UnEscape (group);

  /* "group" now holds the part after the host name:
	 "message@id?search" or "/group/xxx?search" or "/message?id@xx?search"

	 If there is an @, this is a message ID; else it is a group.
	 Either way, there may be search data at the end.
   */

  s = XP_STRCHR (group, '@');
  if (s)
	{
	  message_id = group;
	  group = 0;
	}
  else if (!*group)
	{
	  XP_FREE (group);
	  group = 0;
	}

  /* At this point, the search data is attached to `message_id' (if there
	 is one) or `group' (if there is one) or `host_and_port' (otherwise.)
	 Seperate the search data from what it is clinging to, being careful
	 to interpret the "?" only if it comes after the "@" in an ID, since
	 the syntax of message IDs is tricky.  (There may be a ? in the
	 random-characters part of the ID (before @), but not in the host part
	 (after @).)
   */
  if (message_id || group || host_and_port)
	{
	  char *start;
	  if (message_id)
		{
		  /* Move past the @. */
		  XP_ASSERT (s && *s == '@');
		  start = s;
		}
	  else
		{
		  start = group ? group : host_and_port;
		}

	  /* Take off the "?" or "#" search data */
	  for (s = start; *s; s++)
		if (*s == '?' || *s == '#')
		  break;
	  if (*s)
		{
		  search_data = XP_STRDUP (s);
		  *s = 0;
		  if (!search_data)
			{
			  status = MK_OUT_OF_MEMORY;
			  goto FAIL;
			}
		}

	  /* Discard any now-empty strings. */
	  if (message_id && !*message_id)
		{
		  XP_FREE (message_id);
		  message_id = 0;
		}
	  else if (group && !*group)
		{
		  XP_FREE (group);
		  group = 0;
		}
	}

  FAIL:
  XP_ASSERT (!message_id || message_id != group);
  if (status >= 0)
	{
	  *host_and_portP = host_and_port;
	  *securepP = secure_p;
	  *groupP = group;
	  *message_idP = message_id;
	  *search_dataP = search_data;
	}
  else
	{
	  FREEIF (host_and_port);
	  FREEIF (group);
	  FREEIF (message_id);
	  FREEIF (search_data);
	}
  return status;
}


#if 0
static void
test (const char *url)
{
  char *host_and_port = 0;
  XP_Bool secure_p = FALSE;
  char *group = 0;
  char *message_id = 0;
  char *search_data = 0;
  int status = net_parse_news_url (url, &host_and_port, &secure_p,
								   &group, &message_id, &search_data);
  if (status < 0)
	fprintf (stderr, "%s: status %d\n", url, status);
  else
	{
	  fprintf (stderr,
			   "%s:\n"
			   " host \"%s\", %ssecure,\n"
			   " group=\"%s\",\n"
			   " id=\"%s\",\n"
			   " search=\"%s\"\n\n",
			   url,
			   (host_and_port ? host_and_port : "(none)"),
			   (secure_p ? "" : "in"),
			   (group ? group : "(none)"),
			   (message_id ? message_id : "(none)"),
			   (search_data ?  search_data : "(none)"));
	}
  FREEIF (host_and_port);
  FREEIF (group);
  FREEIF (message_id);
  FREEIF (search_data);
}

static void
test2 (void)
{
  test ("news:");
  test ("news:?search");
  test ("news:#anchor");
  test ("news:?search#anchor");
  test ("news:#anchor?search");
  test ("news://HOST");
  test ("news://HOST?search");
  test ("news://HOST#anchor");
  test ("news://HOST?search#anchor");
  test ("news://HOST#anchor?search");
  test ("news://HOST/");
  test ("news://HOST/?search");
  test ("news://HOST/#anchor");
  test ("news://HOST/?search#anchor");
  test ("news://HOST/#anchor?search");
  test ("news:GROUP");
  test ("news:GROUP?search");
  test ("news:GROUP#anchor");
  test ("news:GROUP?search#anchor");
  test ("news:GROUP#anchor?search");
  test ("news:/GROUP");
  test ("news:/GROUP?search");
  test ("news:/GROUP#anchor");
  test ("news:/GROUP?search#anchor");
  test ("news:/GROUP#anchor?search");
  test ("news://HOST/GROUP");
  test ("news://HOST/GROUP?search");
  test ("news://HOST/GROUP#anchor");
  test ("news://HOST/GROUP?search#anchor");
  test ("news://HOST/GROUP#anchor?search");

  test ("news:GROUP/N1-N2");
  test ("news:GROUP/N1-N2?search");
  test ("news:GROUP/N1-N2#anchor");
  test ("news:GROUP/N1-N2?search#anchor");
  test ("news:GROUP/N1-N2#anchor?search");
  test ("news:/GROUP/N1-N2");
  test ("news:/GROUP/N1-N2?search");
  test ("news:/GROUP/N1-N2#anchor");
  test ("news:/GROUP/N1-N2?search#anchor");
  test ("news:/GROUP/N1-N2#anchor?search");

  test ("news://HOST/GROUP/N1-N2");
  test ("news://HOST/GROUP/N1-N2?search");
  test ("news://HOST/GROUP/N1-N2#anchor");
  test ("news://HOST/GROUP/N1-N2?search#anchor");
  test ("news://HOST/GROUP/N1-N2#anchor?search");
  test ("news:<ID??##??ID@HOST>");
  test ("news:<ID??##??ID@HOST>?search");
  test ("news:<ID??##??ID@HOST>#anchor");
  test ("news:<ID??##??ID@HOST>?search#anchor");
  test ("news:<ID??##??ID@HOST>#anchor?search");

  test ("news:<ID/ID/ID??##??ID@HOST>");
  test ("news:<ID/ID/ID??##??ID@HOST>?search");
  test ("news:<ID/ID/ID??##??ID@HOST>#anchor");
  test ("news:<ID/ID/ID??##??ID@HOST>?search#anchor");
  test ("news:<ID/ID/ID??##??ID@HOST>#anchor?search");
  test ("news:</ID/ID/ID??##??ID@HOST>");
  test ("news:</ID/ID/ID??##??ID@HOST>?search");
  test ("news:</ID/ID/ID??##??ID@HOST>#anchor");
  test ("news:</ID/ID/ID??##??ID@HOST>?search#anchor");
  test ("news:</ID/ID/ID??##??ID@HOST>#anchor?search");

  test ("news://HOST/<ID??##??ID@HOST>");
  test ("news://HOST/<ID??##??ID@HOST>?search");
  test ("news://HOST/<ID??##??ID@HOST>#anchor");
  test ("news://HOST/<ID??##??ID@HOST>?search#anchor");
  test ("news://HOST/<ID??##??ID@HOST>#anchor?search");
  test ("news://HOST/<ID/ID/ID??##??ID@HOST>");
  test ("news://HOST/<ID/ID/ID??##??ID@HOST>?search");
  test ("news://HOST/<ID/ID/ID??##??ID@HOST>#anchor");
  test ("news://HOST/<ID/ID/ID??##??ID@HOST>?search#anchor");
  test ("news://HOST/<ID/ID/ID??##??ID@HOST>#anchor?search");

  test ("news:ID/ID/ID??##??ID@HOST");
  test ("news:ID/ID/ID??##??ID@HOST?search");
  test ("news:ID/ID/ID??##??ID@HOST#anchor");
  test ("news:ID/ID/ID??##??ID@HOST?search#anchor");
  test ("news:ID/ID/ID??##??ID@HOST#anchor?search");
  test ("news:/ID/ID/ID??##??ID@HOST");
  test ("news:/ID/ID/ID??##??ID@HOST?search");
  test ("news:/ID/ID/ID??##??ID@HOST#anchor");
  test ("news:/ID/ID/ID??##??ID@HOST?search#anchor");
  test ("news:/ID/ID/ID??##??ID@HOST#anchor?search");

  test ("news://HOST/ID??##??ID@HOST");
  test ("news://HOST/ID??##??ID@HOST?search");
  test ("news://HOST/ID??##??ID@HOST#anchor");
  test ("news://HOST/ID??##??ID@HOST?search#anchor");
  test ("news://HOST/ID??##??ID@HOST#anchor?search");
  test ("news://HOST/ID/ID/ID??##??ID@HOST");
  test ("news://HOST/ID/ID/ID??##??ID@HOST?search");
  test ("news://HOST/ID/ID/ID??##??ID@HOST#anchor");
  test ("news://HOST/ID/ID/ID??##??ID@HOST?search#anchor");
  test ("news://HOST/ID/ID/ID??##??ID@HOST#anchor?search");
}
#endif /* 0 */


/* Returns true if this URL is a reference to a news message,
   that is, the kind of news URL entity that can be displayed
   in a regular browser window, and doesn't involve all kinds
   of other magic state and callbacks like listing a group.
 */
XP_Bool
NET_IsNewsMessageURL (const char *url)
{
  char *host_and_port = 0;
  XP_Bool secure_p = FALSE;
  char *group = 0;
  char *message_id = 0;
  char *search_data = 0;
  int status = net_parse_news_url (url, &host_and_port, &secure_p,
								   &group, &message_id, &search_data);
  XP_Bool result = FALSE;
  if (status >= 0 && message_id && *message_id)
	result = TRUE;
  FREEIF (host_and_port);
  FREEIF (group);
  FREEIF (message_id);
  FREEIF (search_data);
  return result;
}



MODULE_PRIVATE int
NET_NewsLoad (ActiveEntry *cur_entry, char *ssl_proxy_server)
{
  int status = 0;
  NewsConData *connection_data = XP_NEW(NewsConData);
  XP_Bool secure_p = FALSE;
  XP_Bool default_host = FALSE;
  char *url = CE_URL_S->address;
  char *host_and_port = 0;
  char *group = 0;
  char *message_id = 0;
  char *search_data = 0;
  XP_Bool cancel_p = FALSE;

  if(!connection_data)
	{
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}

  XP_MEMSET(connection_data, 0, sizeof(NewsConData));
  CE_URL_S->content_length = 0;
  StrAllocCopy(CD_SSL_PROXY_SERVER, ssl_proxy_server);

  CD_OUTPUT_BUFFER = (char *) XP_ALLOC(OUTPUT_BUFFER_SIZE);
  if(!CD_OUTPUT_BUFFER)
	{
	  status = MK_OUT_OF_MEMORY;
	  goto FAIL;
	}

  cur_entry->con_data = connection_data;
  CE_SOCK = SOCKET_INVALID;
  CD_ARTICLE_NUM = -1;

  XP_ASSERT (url);
  if (!url)
	{
	  status = -1;
	  goto FAIL;
	}


  status = net_parse_news_url (url, &host_and_port, &secure_p,
							   &group, &message_id, &search_data);
  if (status < 0)
	goto FAIL;

  if (message_id && search_data && !XP_STRCMP (search_data, "?cancel"))
	cancel_p = TRUE;

  StrAllocCopy(CD_PATH, message_id);
  StrAllocCopy(CD_GROUP_NAME, group);

  /* make sure the user has a news host configured */
  if(NULL == NET_GetNewsHost(CE_WINDOW_ID, TRUE))
    {
      status = -1;
      goto FAIL;
    }

  default_host = !XP_STRCMP (host_and_port, NET_GetNewsHost(CE_WINDOW_ID,
															FALSE));

  if (cancel_p && !CE_URL_S->internal_url)
	{
	  /* Don't allow manual "news:ID?cancel" URLs, only internal ones. */
	  status = -1;
	  goto FAIL;
	}
  else if (CE_URL_S->method == URL_POST_METHOD)
	{
	  /* news://HOST done with a POST instead of a GET;
		 this means a new message is being posted.
		 Don't allow this unless it's an internal URL.
	   */
	  if (!CE_URL_S->internal_url)
		{
		  status = -1;
		  goto FAIL;
		}
	  XP_ASSERT (!group && !message_id && !search_data);
	  CD_TYPE_WANTED = NEWS_POST;
	  StrAllocCopy(CD_PATH, "");
	}
  else if (message_id)
	{
	  /* news:MESSAGE_ID
		 news:/GROUP/MESSAGE_ID (useless)
		 news://HOST/MESSAGE_ID
		 news://HOST/GROUP/MESSAGE_ID (useless)
	   */
	  if (cancel_p)
		CD_TYPE_WANTED = CANCEL_WANTED;
	  else
		CD_TYPE_WANTED = ARTICLE_WANTED;
	}
  else if (search_data && XP_STRSTR (search_data, "?newgroups"))
	{
	  /* news://HOST/?newsgroups
	     news:/GROUP/?newsgroups (useless)
	     news:?newsgroups (default host)
	   */
	  CD_TYPE_WANTED = NEW_GROUPS;
	}
  else if (group)
	{
	  /* news:GROUP
		 news:/GROUP
		 news://HOST/GROUP
	   */
	  if (CE_WINDOW_ID->type != MWContextNews)
		{
		  status = -1;
		  goto FAIL;
		}
	  if (XP_STRCHR (group, '*'))
		CD_TYPE_WANTED = LIST_WANTED;
	  else
		CD_TYPE_WANTED = GROUP_WANTED;
	}
  else
	{
	  /* news:
	     news://HOST
	   */
	  CD_TYPE_WANTED = READ_NEWS_RC;
	}


  /* At this point, we're all done parsing the URL, and know exactly
	 what we want to do with it.
   */

#ifndef NO_ARTICLE_CACHEING
  /* Turn off caching on all news entities, except articles. */
  /* It's very important that this be turned off for CANCEL_WANTED;
	 for the others, I don't know what cacheing would cause, but
	 it could only do harm, not good. */
  if(CD_TYPE_WANTED != ARTICLE_WANTED)
#endif
  	CE_FORMAT_OUT = CLEAR_CACHE_BIT (CE_FORMAT_OUT);

  /* check for established connection and use it if available
   */
  if(nntp_connection_list)
	{
	  NNTPConnection * tmp_con;
	  XP_List * list_entry = nntp_connection_list;

	  while((tmp_con = (NNTPConnection *)XP_ListNextObject(list_entry))
			!= NULL)
		{
		  /* if the hostnames match up exactly and the connection
		   * is not busy at the moment then reuse this connection.
		   */
		  if(!XP_STRCMP(tmp_con->hostname, host_and_port)
			 && secure_p == tmp_con->secure
			 &&!tmp_con->busy)
			{
			  CD_CONTROL_CON = tmp_con;
			  /* set select on the control socket */
			  CE_SOCK = CD_CONTROL_CON->csock;
			  FE_SetReadSelect(CE_WINDOW_ID, CD_CONTROL_CON->csock);
			  CD_CONTROL_CON->prev_cache = TRUE;  /* this was from the cache */
			  break;
			}
		}
	}
  else
	{
	  /* initialize the connection list
	   */
	  nntp_connection_list = XP_ListNew();
	}


  if(CD_CONTROL_CON)
	{
	  CD_NEXT_STATE = SEND_FIRST_NNTP_COMMAND;
	  /* set the connection busy
	   */
	  CD_CONTROL_CON->busy = TRUE;
	  NET_TotalNumberOfOpenConnections++;
	}
  else
	{
	  /* build a control connection structure so we
	   * can store the data as we go along
	   */
	  CD_CONTROL_CON = XP_NEW(NNTPConnection);
	  if(!CD_CONTROL_CON)
		{
		  status = MK_OUT_OF_MEMORY;
		  goto FAIL;
		}
	  XP_MEMSET(CD_CONTROL_CON, 0, sizeof(NNTPConnection));

	  CD_CONTROL_CON->default_host = default_host;
	  StrAllocCopy(CD_CONTROL_CON->hostname, host_and_port);
	  TRACEMSG(("Set host string: %s", CD_CONTROL_CON->hostname));

	  CD_CONTROL_CON->secure = secure_p;

	  CD_CONTROL_CON->prev_cache = FALSE;  /* this wasn't from the cache */

	  CD_CONTROL_CON->csock = SOCKET_INVALID;

	  /* define this to test support for older NNTP servers
	   * that don't support the XOVER command
	   */
#ifdef TEST_NO_XOVER_SUPPORT
	  CD_CC_NO_XOVER = TRUE;
#endif /* TEST_NO_XOVER_SUPPORT */

	  /* add this structure to the connection list even
	   * though it's not really valid yet.
	   * we will fill it in as we go and if
	   * an error occurs will will remove it from the
	   * list.  No one else will be able to use it since
	   * we will mark it busy.
	   */
	  XP_ListAddObject(nntp_connection_list, CD_CONTROL_CON);

	  /* set the connection busy
	   */
	  CD_CONTROL_CON->busy = TRUE;

	  /* gate the maximum number of cached connections
	   * to one
	   * but don't delete busy ones.
	   */
	  if(XP_ListCount(nntp_connection_list) > 1)
		{
		  XP_List * list_ptr = nntp_connection_list->next;
		  NNTPConnection * con;

		  while(list_ptr)
			{
			  con = (NNTPConnection *) list_ptr->object;
			  list_ptr = list_ptr->next;
			  if(!con->busy)
				{
				  XP_ListRemoveObject(nntp_connection_list, con);
				  NETCLOSE(con->csock);
				  FREEIF(con->current_group);
				  FREEIF(con->hostname);
				  XP_FREE(con);
				  break;
				}
			}
		}

	  CD_NEXT_STATE = NNTP_CONNECT;
	}

 FAIL:

  FREEIF (host_and_port);
  FREEIF (group);
  FREEIF (message_id);
  FREEIF (search_data);

  if (status < 0)
	{
	  FREEIF (CD_OUTPUT_BUFFER);
	  FREEIF (connection_data);
	  CE_URL_S->error_msg = NET_ExplainErrorDetails(status);
	  return status;
	}
  else
	{
	  return (NET_ProcessNews (cur_entry));
	}
}

/* process the state machine
 *
 * returns negative when completely done
 */
MODULE_PRIVATE int
NET_ProcessNews (ActiveEntry *cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

    CD_PAUSE_FOR_READ = FALSE;

    while(!CD_PAUSE_FOR_READ)
      {

        TRACEMSG(("In ProcessNews switch with state: #%d\n",CD_NEXT_STATE));

        switch(CD_NEXT_STATE)
          {
            case NNTP_RESPONSE:
                CE_STATUS = net_news_response(cur_entry);
                break;
        
            case NNTP_CONNECT:
                CE_STATUS = NET_BeginConnect(
							  (CD_SSL_PROXY_SERVER && CD_CC_SECURE ?
								CD_SSL_PROXY_SERVER :CD_CONTROL_CON->hostname), 
							 NULL,
							 "NNTP", 
						 	 (CD_CC_SECURE ? SECURE_NEWS_PORT : NEWS_PORT), 
							 &CE_SOCK, 
							 (CD_SSL_PROXY_SERVER ? FALSE : CD_CC_SECURE), 
							 &CD_TCP_CON_DATA, 
							 CE_WINDOW_ID,
							 &CE_URL_S->error_msg,
							  cur_entry->socks_host,
							  cur_entry->socks_port);

			    if(CE_SOCK != SOCKET_INVALID)
				    NET_TotalNumberOfOpenConnections++;

                CD_PAUSE_FOR_READ = TRUE;
                if(CE_STATUS == MK_CONNECTED)
                  {
                    if(CD_SSL_PROXY_SERVER && CD_CC_SECURE)
					  {
                        CD_NEXT_STATE = NNTP_SEND_SSL_PROXY_REQUEST;
                		CD_PAUSE_FOR_READ = FALSE;
					  }
					else
					  {
                    	CD_NEXT_STATE = NNTP_RESPONSE;
                        CD_NEXT_STATE_AFTER_RESPONSE = NNTP_LOGIN_RESPONSE;
					  }
				    FE_SetReadSelect(CE_WINDOW_ID, CE_SOCK);
				    CD_CONTROL_CON->csock = CE_SOCK;
                  }
                else if(CE_STATUS > -1)
                  {
                    CE_CON_SOCK = CE_SOCK;  /* set con_sock so we can select on it */
				    CD_CONTROL_CON->csock = CE_SOCK;
                    FE_SetConnectSelect(CE_WINDOW_ID, CE_CON_SOCK);
                    CD_NEXT_STATE = NNTP_CONNECT_WAIT;
                  }
                break;

            case NNTP_CONNECT_WAIT:
                CE_STATUS = NET_FinishConnect(
							  (CD_SSL_PROXY_SERVER && CD_CC_SECURE ?
								CD_SSL_PROXY_SERVER : CD_CONTROL_CON->hostname),
							  "NNTP", 
							  (CD_CC_SECURE ? SECURE_NEWS_PORT : NEWS_PORT), 
							  &CE_SOCK, 
							  &CD_TCP_CON_DATA, 
							  CE_WINDOW_ID,
							  &CE_URL_S->error_msg);
  
                CD_PAUSE_FOR_READ = TRUE;
                if(CE_STATUS == MK_CONNECTED)
                  {
				    CD_CONTROL_CON->csock = CE_SOCK;
                    if(CD_SSL_PROXY_SERVER && CD_CC_SECURE)
                      {
                        CD_NEXT_STATE = NNTP_SEND_SSL_PROXY_REQUEST;
                		CD_PAUSE_FOR_READ = FALSE;
                      }
                    else
                      {
                        CD_NEXT_STATE = NNTP_RESPONSE;
                        CD_NEXT_STATE_AFTER_RESPONSE = NNTP_LOGIN_RESPONSE;
                      }

					FE_ClearConnectSelect(CE_WINDOW_ID, CE_CON_SOCK);
                    CE_CON_SOCK = SOCKET_INVALID;  /* reset con_sock so we don't select on it */
					FE_SetReadSelect(CE_WINDOW_ID, CE_SOCK);
                  }
				else if(CE_STATUS < 0)
                  {
                    	FE_ClearConnectSelect(CE_WINDOW_ID, CE_CON_SOCK);
                  }
				else
                  {
					/* the not yet connected state */

        			/* unregister the old CE_SOCK from the select list
         			 * and register the new value in the case that it changes
         			 */
        			if(CE_CON_SOCK != CE_SOCK)
          			  {
            			FE_ClearConnectSelect(CE_WINDOW_ID, CE_CON_SOCK);
            			CE_CON_SOCK = CE_SOCK;
            			FE_SetConnectSelect(CE_WINDOW_ID, CE_CON_SOCK);
          			  }
                  }
                break;

#ifndef NO_SSL /* added by jwz */
			case NNTP_SEND_SSL_PROXY_REQUEST:
				CE_STATUS = net_nntp_send_ssl_proxy_request(cur_entry);
				break;

			case NNTP_PROCESS_FIRST_SSL_PROXY_HEADER:
				CE_STATUS = nntp_process_first_ssl_proxy_header(cur_entry);
				break;

			case NNTP_FINISH_SSL_PROXY_CONNECT:
				CE_STATUS = nntp_process_ssl_proxy_headers(cur_entry);
				break;
#endif /* NO_SSL -- added by jwz */

            case NNTP_LOGIN_RESPONSE:
                CE_STATUS = net_nntp_login_response(cur_entry);
                break;

			case NNTP_SEND_MODE_READER:
                CE_STATUS = net_nntp_send_mode_reader(cur_entry);
                break;

			case NNTP_SEND_MODE_READER_RESPONSE:
                CE_STATUS = net_nntp_send_mode_reader_response(cur_entry);
                break;

            case SEND_FIRST_NNTP_COMMAND:
                CE_STATUS = net_send_first_nntp_command(cur_entry);
                break;

            case SEND_FIRST_NNTP_COMMAND_RESPONSE:
                CE_STATUS = net_send_first_nntp_command_response(cur_entry);
                break;

            case NNTP_SEND_GROUP_FOR_ARTICLE:
                CE_STATUS = net_send_group_for_article(cur_entry);
                break;
            case NNTP_SEND_GROUP_FOR_ARTICLE_RESPONSE:
                CE_STATUS = net_send_group_for_article_response(cur_entry);
                break;
            case NNTP_SEND_ARTICLE_NUMBER:
                CE_STATUS = net_send_article_number(cur_entry);
                break;

            case SETUP_NEWS_STREAM:
                CE_STATUS = net_setup_news_stream(cur_entry);
                break;

			case NNTP_BEGIN_AUTHORIZE:
				CE_STATUS = net_news_begin_authorize(cur_entry);
                break;

			case NNTP_AUTHORIZE_RESPONSE:
				CE_STATUS = net_news_authorize_response(cur_entry);
                break;

			case NNTP_PASSWORD_RESPONSE:
				CE_STATUS = net_news_password_response(cur_entry);
                break;
    
            case NNTP_READ_LIST_BEGIN:
                CE_STATUS = net_read_news_list_begin(cur_entry);
                break;

            case NNTP_READ_LIST:
                CE_STATUS = net_read_news_list(cur_entry);
                break;

			case DISPLAY_NEWSGROUPS:
				CE_STATUS = net_display_newsgroups(cur_entry);
                break;

			case NNTP_NEWGROUPS_BEGIN:
				CE_STATUS = net_newgroups_begin(cur_entry);
                break;
    
			case NNTP_NEWGROUPS:
				CE_STATUS = net_process_newgroups(cur_entry);
                break;
    
            case NNTP_BEGIN_ARTICLE:
				CE_STATUS = net_begin_article(cur_entry);
                break;

            case NNTP_READ_ARTICLE:
			  {
				char *line;
				NewsConData * connection_data =
				  (NewsConData *)cur_entry->con_data;

				CE_STATUS = NET_BufferedReadLine(CE_SOCK, &line,
												 &CD_DATA_BUF, 
												 &CD_DATA_BUF_SIZE,
												 (Bool*)&CD_PAUSE_FOR_READ);

				if(CE_STATUS == 0)
				  {
					CD_NEXT_STATE = NNTP_ERROR;
					CD_PAUSE_FOR_READ = FALSE;
					CE_URL_S->error_msg =
					  NET_ExplainErrorDetails(MK_NNTP_SERVER_ERROR);
					return(MK_NNTP_SERVER_ERROR);
				  }

				if(CE_STATUS > 1)
				  {
					CE_BYTES_RECEIVED += CE_STATUS;
					FE_GraphProgress(CE_WINDOW_ID, CE_URL_S,
									 CE_BYTES_RECEIVED, CE_STATUS,
									 CE_URL_S->content_length);
				  }

				if(!line)
				  return(CE_STATUS);  /* no line yet or error */

				if (CD_TYPE_WANTED == CANCEL_WANTED &&
					CD_RESPONSE_CODE != 221)
				  {
					/* HEAD command failed. */
					return MK_NNTP_CANCEL_ERROR;
				  }

				if (line[0] == '.' && line[1] == 0)
				  {
					if (CD_TYPE_WANTED == CANCEL_WANTED)
					  CD_NEXT_STATE = NEWS_START_CANCEL;
					else
					  CD_NEXT_STATE = NEWS_DONE;

					CD_PAUSE_FOR_READ = FALSE;
				  }
				else
				  {
					if (line[0] == '.')
					  XP_STRCPY (CD_OUTPUT_BUFFER, line + 1);
					else
					  XP_STRCPY (CD_OUTPUT_BUFFER, line);

					/* When we're sending this line to a converter (ie,
					   it's a message/rfc822) use the local line termination
					   convention, not CRLF.  This makes text articles get
					   saved with the local line terminators.  Since SMTP
					   and NNTP mandate the use of CRLF, it is expected that
					   the local system will convert that to the local line
					   terminator as it is read.
					 */
					XP_STRCAT (CD_OUTPUT_BUFFER, LINEBREAK);
					CE_STATUS = PUTSTRING(CD_OUTPUT_BUFFER);
				  }
				break;
			  }

            case NNTP_XOVER_BEGIN:
			    NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_READ_NEWSGROUPINFO));
			    CE_STATUS = net_read_xover_begin(cur_entry);
                break;

		  case NNTP_FIGURE_NEXT_CHUNK:
				CE_STATUS = net_figure_next_chunk(cur_entry);
				break;

			case NNTP_XOVER_SEND:
				CE_STATUS = net_xover_send(cur_entry);
				break;
    
            case NNTP_XOVER:
                CE_STATUS = net_read_xover(cur_entry); 
                break;

            case NNTP_XOVER_RESPONSE:
                CE_STATUS = net_read_xover_response(cur_entry); 
                break;

            case NEWS_PROCESS_XOVER:
		    case NEWS_PROCESS_BODIES:
			    if (CD_GROUP_NAME && XP_STRSTR (CD_GROUP_NAME, ".emacs"))
				  /* This is a joke!  Don't internationalize it... */
			      NET_Progress(CE_WINDOW_ID, "Garbage collecting...");
			    else
			      NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_SORT_ARTICLES));
				CE_STATUS = net_process_xover(cur_entry);
                break;

            case NNTP_READ_GROUP:
                CE_STATUS = net_read_news_group(cur_entry);
                break;
    
            case NNTP_READ_GROUP_RESPONSE:
                CE_STATUS = net_read_news_group_response(cur_entry);
                break;

            case NNTP_READ_GROUP_BODY:
                CE_STATUS = net_read_news_group_body(cur_entry);
                break;

	        case NNTP_SEND_POST_DATA:
	            CE_STATUS = net_send_news_post_data(cur_entry);
	            break;

	        case NNTP_SEND_POST_DATA_RESPONSE:
	            CE_STATUS = net_send_news_post_data_response(cur_entry);
	            break;

			case NNTP_CHECK_FOR_MESSAGE:
				CE_STATUS = net_check_for_message(cur_entry);
				break;

			case NEWS_NEWS_RC_POST:
		        CE_STATUS = net_NewsRCProcessPost(cur_entry);
		        break;

            case NEWS_DISPLAY_NEWS_RC:
		        CE_STATUS = net_DisplayNewsRC(cur_entry);
		        break;

            case NEWS_DISPLAY_NEWS_RC_RESPONSE:
		        CE_STATUS = net_DisplayNewsRCResponse(cur_entry);
		        break;

            case NEWS_START_CANCEL:
		        CE_STATUS = net_start_cancel(cur_entry);
		        break;

            case NEWS_DO_CANCEL:
		        CE_STATUS = net_do_cancel(cur_entry);
		        break;

	        case NEWS_DONE:
			  /* call into libmsg and see if the article counts
			   * are up to date.  If they are not then we
			   * want to do a "news://host/group" URL so that we
			   * can finish up the article counts.
			   */
			  if(!CD_NEWSRC_PERFORMED)
				{
				  CD_NEXT_STATE = NEWS_DISPLAY_NEWS_RC;
				  break;
				}

			  if (CD_STREAM)
				COMPLETE_STREAM;

	            CD_NEXT_STATE = NEWS_FREE;
                /* set the connection unbusy
     	         */
    		    CD_CONTROL_CON->busy = FALSE;
                NET_TotalNumberOfOpenConnections--;

				FE_ClearReadSelect(CE_WINDOW_ID, CD_CONTROL_CON->csock);
				NET_RefreshCacheFileExpiration(CE_URL_S);
	            break;

	        case NEWS_ERROR:
	            if(CD_STREAM)
		             ABORT_STREAM(CE_STATUS);
	            CD_NEXT_STATE = NEWS_FREE;
    	        /* set the connection unbusy
     	         */
    		    CD_CONTROL_CON->busy = FALSE;
                NET_TotalNumberOfOpenConnections--;

				if(CD_CONTROL_CON->csock != SOCKET_INVALID)
				  {
					FE_ClearReadSelect(CE_WINDOW_ID, CD_CONTROL_CON->csock);
				  }
	            break;

	        case NNTP_ERROR:
	            if(CD_STREAM)
				  {
		            ABORT_STREAM(CE_STATUS);
					CD_STREAM=0;
				  }
    
				if(CD_CONTROL_CON->csock != SOCKET_INVALID)
				  {
					TRACEMSG(("Clearing read and connect select on socket %d",
															CD_CONTROL_CON->csock));
					FE_ClearConnectSelect(CE_WINDOW_ID, CD_CONTROL_CON->csock);
					FE_ClearReadSelect(CE_WINDOW_ID, CD_CONTROL_CON->csock);
					FE_ClearFileReadSelect(CE_WINDOW_ID,
										   CD_CONTROL_CON->csock);

#ifdef XP_WIN
					if(connection_data->calling_netlib_all_the_time)
					{
						net_call_all_the_time_count--;
						connection_data->calling_netlib_all_the_time = FALSE;
						if(net_call_all_the_time_count == 0)
							FE_ClearCallNetlibAllTheTime(CE_WINDOW_ID);
					}
#endif /* XP_WIN */
#if defined(XP_WIN) || defined(XP_UNIX)
                    FE_ClearDNSSelect(CE_WINDOW_ID, CD_CONTROL_CON->csock);
#endif /* XP_WIN || XP_UNIX */

				    NETCLOSE(CD_CONTROL_CON->csock);  /* close the socket */
					FREEIF(CD_CC_GROUP);
					NET_TotalNumberOfOpenConnections--;
					CE_SOCK = SOCKET_INVALID;
				  }

                /* check if this connection came from the cache or if it was
                 * a new connection.  If it was not new lets start it over
				 * again.  But only if we didn't have any successful protocol
				 * dialog at all.
                 */
                if(CD_CONTROL_CON && CD_CONTROL_CON->prev_cache &&
				   !CD_SOME_PROTOCOL_SUCCEEDED)
                  {
                     CD_CONTROL_CON->prev_cache = FALSE;
                     CD_NEXT_STATE = NNTP_CONNECT;
                     CE_STATUS = 0;  /* keep going */
                  }
                else
                  {
                    CD_NEXT_STATE = NEWS_FREE;
            
                    /* remove the connection from the cache list
                     * and free the data
                     */
                    XP_ListRemoveObject(nntp_connection_list, CD_CONTROL_CON);
                    if(CD_CONTROL_CON)
                      {
                        FREEIF(CD_CONTROL_CON->hostname);
                        FREE(CD_CONTROL_CON);
                      }
                  }
                break;
    
            case NEWS_FREE:

			    if (CE_XOVER_PARSE_STATE)
				  {
					/* If we've gotten to NEWS_FREE and there is still XOVER
					   data, there was an error or we were interrupted or
					   something.  So, tell libmsg there was an abnormal
					   exit so that it can free its data. */
					int status;
					XP_ASSERT (CE_STATUS < 0);
					status = MSG_FinishXOVER (CE_WINDOW_ID,
											  &CE_XOVER_PARSE_STATE,
											  CE_STATUS);
					XP_ASSERT (!CE_XOVER_PARSE_STATE);
					if (CE_STATUS >= 0 && status < 0)
					  CE_STATUS = status;
				  }


                FREEIF(CD_PATH);
                FREEIF(CD_RESPONSE_TXT);
                FREEIF(CD_DATA_BUF);

				FREEIF(CD_OUTPUT_BUFFER);
				FREEIF(CD_SSL_PROXY_SERVER);
                FREEIF(CD_STREAM);  /* don't forget the stream */
            	if(CD_TCP_CON_DATA)
                	NET_FreeTCPConData(CD_TCP_CON_DATA);

                FREEIF(CD_GROUP_NAME);
				FREEIF (CD_NEWSRC_LIST);

				FREEIF (connection_data->cancel_id);
				FREEIF (connection_data->cancel_from);
				FREEIF (connection_data->cancel_newsgroups);
				FREEIF (connection_data->cancel_distribution);

                if(CD_DESTROY_GRAPH_PROGRESS)
                    FE_GraphProgressDestroy(CE_WINDOW_ID, 
                                            CE_URL_S, 
                                            CD_ORIGINAL_CONTENT_LENGTH,
											CE_BYTES_RECEIVED);
                XP_FREE(connection_data);

        		/* gate the maximum number of cached connections
				 * to one but don't delete busy ones.
                 */
                if(XP_ListCount(nntp_connection_list) > 1)
                  {
                    XP_List * list_ptr = nntp_connection_list->next;
                    NNTPConnection * con;

					TRACEMSG(("killing cached news connection"));

                    while(list_ptr)
                      {
                        con = (NNTPConnection *) list_ptr->object;
                        list_ptr = list_ptr->next;
                        if(!con->busy)
                          {
                            XP_ListRemoveObject(nntp_connection_list, con);
                            NETCLOSE(con->csock);
							FREEIF(con->current_group);
                            FREEIF(con->hostname);
                            XP_FREE(con);
                            break;
                          }
                      }
				  }

                return(-1); /* all done */

            default:
                /* big error */
                return(-1);
          }

        if(CE_STATUS < 0 && CD_NEXT_STATE != NEWS_ERROR &&
                    CD_NEXT_STATE != NNTP_ERROR && CD_NEXT_STATE != NEWS_FREE)
          {
            CD_NEXT_STATE = NNTP_ERROR;
            CD_PAUSE_FOR_READ = FALSE;
          }
      } /* end big while */

      return(0); /* keep going */
}

/* abort the connection in progress
 */
MODULE_PRIVATE int
NET_InterruptNews(ActiveEntry * cur_entry)
{
    NewsConData * connection_data = (NewsConData *)cur_entry->con_data;

    CD_NEXT_STATE = NNTP_ERROR;
    CE_STATUS = MK_INTERRUPTED;

    CD_CONTROL_CON->prev_cache = FALSE;   /* to keep if from reconnecting */
 
    return(NET_ProcessNews(cur_entry));
}

/* Free any memory used up
 * close cached connections and
 * free newsgroup listings
 */
MODULE_PRIVATE void
NET_CleanupNews(void)
{
	TRACEMSG(("Cleanup News called!!!!!!!!!!!!!!!"));

	/* this is a small leak but since I don't have another function to
	 * clear my connections I need to call this one alot and I don't
	 * want to free the newshost
	 *
     * FREE_AND_CLEAR(NET_NewsHost);
	 */

    if(nntp_connection_list)
      {
        NNTPConnection * tmp_con;

        while((tmp_con = (NNTPConnection *)XP_ListRemoveTopObject(nntp_connection_list)) != NULL)
          {
			if(!tmp_con->busy)
			  {
			    FREE(tmp_con->hostname);
			    FREEIF(tmp_con->current_group);
			    if(tmp_con->csock != SOCKET_INVALID)
			      {
			        NETCLOSE(tmp_con->csock);
			      }
			    FREE(tmp_con);
			  }
          }

		if(XP_ListIsEmpty(nntp_connection_list))
		  {
			XP_ListDestroy(nntp_connection_list);
			nntp_connection_list = 0;
		  }
      }

    return;
}



#ifdef PROFILE
#pragma profile off
#endif

#endif /* MOZILLA_CLIENT */

