/* -*- Mode:C++; tab-width: 4 -*- */
/* state machine to read FTP sites
 */

#include "mkutils.h"

#define FTP_PORT 21

#define FTP_MODE_UNKNOWN 0
#define FTP_MODE_ASCII   1
#define FTP_MODE_BINARY  2

#include "mkftp.h"  
#include "glhist.h"
#include "mkgeturl.h"
#include "mkparse.h"
#include "mktcp.h"
#include "mksort.h"
#include "mkfsort.h"   /* file sorting */
#include "mkformat.h"
#include "mkfile.h"  
#include "merrors.h"

#ifdef XP_UNIX
#if !defined(__osf__) && !defined(AIXV3) && !defined(_HPUX_SOURCE) && !defined(__386BSD__) && !defined(__linux) && !defined(SCO_SV)
#include <sys/filio.h>
#endif
#endif /* XP_UNIX */ 

#include "xp_error.h"

/* for XP_GetString() */
#include "xpgetstr.h"
extern int XP_HTML_FTPERROR_NOLOGIN;
extern int XP_HTML_FTPERROR_TRANSFER;
extern int XP_PROGRESS_RECEIVE_FTPDIR;
extern int XP_PROGRESS_RECEIVE_FTPFILE;
extern int XP_PROMPT_ENTER_PASSWORD;
extern int MK_COULD_NOT_PUT_FILE;
extern int MK_TCP_READ_ERROR;
extern int MK_TCP_WRITE_ERROR;
extern int MK_UNABLE_TO_ACCEPT_SOCKET;
extern int MK_UNABLE_TO_CHANGE_FTP_MODE;
extern int MK_UNABLE_TO_CREATE_SOCKET;
extern int MK_UNABLE_TO_LOCATE_FILE;
extern int MK_UNABLE_TO_SEND_PORT_COMMAND;
extern int MK_UNABLE_TO_DELETE_DIRECTORY;
extern int MK_COULD_NOT_CREATE_DIRECTORY;
extern int XP_ERRNO_EWOULDBLOCK;
extern int MK_OUT_OF_MEMORY;
extern int XP_COULD_NOT_LOGIN_TO_FTP_SERVER;
extern int XP_ERROR_COULD_NOT_MAKE_CONNECTION_NON_BLOCKING;
extern int XP_POSTING_FILE;
extern int XP_TITLE_DIRECTORY_OF_ETC;
extern int XP_UPTO_HIGHER_LEVEL_DIRECTORY ;

#ifdef DEBUG_jwz
extern int XP_H1_DIRECTORY_LISTING;
#endif /* DEBUG_jwz */



#define PUTSTRING(s)           (*cd->stream->put_block) \
                                 (cd->stream->data_object, s, XP_STRLEN(s))
#define PUTBLOCK(b,l)         (*cd->stream->put_block) \
                                (cd->stream->data_object, b, l)
#define COMPLETE_STREAM   (*cd->stream->complete) \
                                 (cd->stream->data_object)
#define ABORT_STREAM(s)   (*cd->stream->abort) \
                                 (cd->stream->data_object, s)


#define FTP_GENERIC_TYPE     0
#define FTP_UNIX_TYPE        1
#define FTP_DCTS_TYPE        2
#define FTP_NCSA_TYPE        3
#define FTP_PETER_LEWIS_TYPE 4
#define FTP_MACHTEN_TYPE     5
#define FTP_CMS_TYPE         6
#define FTP_TCPC_TYPE        7
#define FTP_VMS_TYPE         8
#define FTP_NT_TYPE          9

#define OUTPUT_BUFFER_SIZE 2048

PRIVATE XP_List * connection_list=0;  /* a pointer to a list of open connections */

PRIVATE Bool   net_use_pasv=TRUE;
PRIVATE char * ftp_last_password=0;
PRIVATE char * ftp_last_password_host=0;
PRIVATE Bool   net_send_email_address_as_password=FALSE;

typedef struct _FTPConnection {
    char   *hostname;       /* hostname string (may contain :port) */
    NETSOCK csock;          /* control socket */
    int     server_type;    /* type of server */
    int     cur_mode;       /* ascii, binary, unknown */
    Bool use_list;       /* use LIST? (or NLST) */
    Bool busy;           /* is the connection in use already? */
    Bool no_pasv;        /* should we NOT use pasv mode since is doesn't work? */
    Bool prev_cache;     /* did this connection come from the cache? */
} FTPConnection;

/* define the states of the machine 
 */
typedef enum _FTPStates {
FTP_WAIT_FOR_RESPONSE,
FTP_CONTROL_CONNECT,
FTP_CONTROL_CONNECT_WAIT,
FTP_SEND_USERNAME,
FTP_SEND_USERNAME_RESPONSE,
FTP_GET_PASSWORD,
FTP_GET_PASSWORD_RESPONSE,
FTP_SEND_PASSWORD,
FTP_SEND_PASSWORD_RESPONSE,
FTP_SEND_ACCT,
FTP_SEND_ACCT_RESPONSE,
FTP_SEND_SYST,
FTP_SEND_SYST_RESPONSE,
FTP_SEND_MAC_BIN,
FTP_SEND_MAC_BIN_RESPONSE,
FTP_SEND_PWD,
FTP_SEND_PWD_RESPONSE,
FTP_SEND_PASV,
FTP_SEND_PASV_RESPONSE,
FTP_PASV_DATA_CONNECT,
FTP_PASV_DATA_CONNECT_WAIT,
FTP_FIGURE_OUT_WHAT_TO_DO,
FTP_SEND_DELETE_FILE,
FTP_SEND_DELETE_FILE_RESPONSE,
FTP_SEND_DELETE_DIR,
FTP_SEND_DELETE_DIR_RESPONSE,
FTP_SEND_MKDIR,
FTP_SEND_MKDIR_RESPONSE,
FTP_SEND_PORT,
FTP_SEND_PORT_RESPONSE,
FTP_GET_FILE_TYPE,
FTP_SEND_BIN,
FTP_SEND_BIN_RESPONSE,
FTP_SEND_ASCII,
FTP_SEND_ASCII_RESPONSE,
FTP_GET_FILE_SIZE,
FTP_GET_FILE_SIZE_RESPONSE,
FTP_GET_FILE,
FTP_BEGIN_PUT,
FTP_BEGIN_PUT_RESPONSE,
FTP_DO_PUT,
FTP_SEND_RETR,
FTP_SEND_RETR_RESPONSE,
FTP_SEND_CWD,
FTP_SEND_CWD_RESPONSE,
FTP_SEND_LIST_OR_NLST,
FTP_SEND_LIST_OR_NLST_RESPONSE,
FTP_SETUP_STREAM,
FTP_WAIT_FOR_ACCEPT,
FTP_START_READ_FILE,
FTP_READ_FILE,
FTP_START_READ_DIRECTORY,
FTP_READ_DIRECTORY,
FTP_DONE,
FTP_ERROR_DONE,
FTP_FREE_DATA
} FTPStates;

typedef struct _FTPConData {
    FTPStates     next_state;        /* the next state of the machine */
    Bool          pause_for_read;    /* should we pause (return) for read? */

    NET_StreamClass *stream;         /* the outgoing data stream */

    FTPConnection   *cc;  /* information about the control connection */

    NETSOCK dsock;                /* data socket */
    NETSOCK listen_sock;          /* socket to listen on */
    Bool    use_list;             /* use LIST or NLST */
    Bool    pasv_mode;            /* using pasv mode? */
    Bool    is_directory;         /* is the url a directory? */
    Bool    retr_already_failed;  /* did RETR fail? */
    char   *data_buf;             /* unprocessed data from the socket */
    int32   data_buf_size;        /* size of the unprocesed data */

	Bool use_default_path;        /* set if we should use PWD to figure out
							       * current path and use that 
							       */

    /* used by response */
    FTPStates   next_state_after_response;  /* the next state after 
											 * full response recieved */
    int         response_code;              /* code returned in response */
    char       *return_msg;                 /* message from response */
    int         cont_response;              /* a continuation code */

    char   *data_con_address;     /* address and port for PASV data connection*/
    char   *filename;             /* name of the file we are getting */ 
    char   *file_to_post;         /* local filename to put */
    XP_File file_to_post_fp;      /* filepointer */
	int32   total_size_of_files_to_post;   /* total bytes */
    char   *path;                 /* path as returned by MKParse */
    int     type;                 /* a type indicator for the file */
    char   *username;             /* the username if not anonymous */
    char   *password;             /* the password if not anonymous */

    char   *cwd_message_buffer;   /* used to print out server help messages */
    char   *login_message_buffer; /* used to print out server help messages */

	char   *output_buffer;		  /* buffer before sending to PUTSTRING */

    SortStruct *sort_base;        /* base pointer to Qsorting list */

    Bool destroy_graph_progress;  /* do we need to destroy graph progress? */    
    Bool destroy_file_upload_progress_dialog;  /* do we need to destroy the dialog? */
#ifdef XP_WIN
	Bool calling_netlib_all_the_time;  /* is SetCallNetlibAllTheTime set? */
#endif
	int32   original_content_length; /* the content length at the time of
                                      * calling graph progress
                                      */

    TCP_ConData *tcp_con_data;     /* Data pointer for tcp connect 
									* state machine */

	int32   buff_size;             /* used by ftp_send_file */
	int32   amt_of_buff_wrt;       /* amount of data already written
									* used by ftp_send_file()
									*/
	void   *write_post_data_data;
} FTPConData;

/* function prototypes
 */
PRIVATE NET_FileEntryInfo * net_parse_dir_entry (char *entry, int server_type);

/* call this to enable/disable FTP PASV mode.
 * Note: PASV mode is on by default
 */
PUBLIC void
NET_UsePASV(Bool do_it)
{
	net_use_pasv = do_it;
}

/* call this with TRUE to enable the sending of the email
 * address as the default user "anonymous" password.
 * The default is OFF.  
 */
PUBLIC void
NET_SendEmailAddressAsFTPPassword(Bool do_it)
{
	net_send_email_address_as_password = do_it;
}

PRIVATE NET_StreamClass *
net_ftp_make_stream(FO_Present_Types format_out, 
					URL_Struct *URL_s, 
					MWContext *window_id)
{
	MWContext *stream_context;

#ifdef MOZILLA_CLIENT
	/* if the context can't handle HTML then we
	 * need to generate an HTML dialog to handle
	 * the message
	 */
	if(URL_s->files_to_post && EDT_IS_EDITOR(window_id))
	  {
		Chrome chrome_struct;
		XP_MEMSET(&chrome_struct, 0, sizeof(Chrome));
		chrome_struct.is_modal = TRUE;
		chrome_struct.allow_close = TRUE;
		chrome_struct.allow_resize = TRUE;
		chrome_struct.show_scrollbar = TRUE;
		chrome_struct.w_hint = 400;
		chrome_struct.h_hint = 300;
		stream_context = FE_MakeNewWindow(window_id,
		NULL,
		NULL,
		&chrome_struct);
		if(!stream_context)
			return (NULL);
	  }
	else
#endif /* MOZILLA_CLIENT */
	  {
		stream_context = window_id;
	  }

	return(NET_StreamBuilder(format_out, URL_s, stream_context));

}

PRIVATE int
net_ftp_show_error(ActiveEntry *ce, char *line)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

	/* show the error
	 */
	StrAllocCopy(ce->URL_s->content_type, TEXT_HTML);
	ce->URL_s->expires = 1; /* immediate expire date */
	cd->stream = net_ftp_make_stream(ce->format_out, ce->URL_s, ce->window_id);

	if(cd->stream)
	  {
		XP_STRCPY(cd->output_buffer, XP_GetString(XP_HTML_FTPERROR_TRANSFER));
		if(ce->status > -1)
			ce->status = PUTSTRING(cd->output_buffer);
		if(line && ce->status > -1)
			ce->status = PUTSTRING(cd->return_msg);
		XP_STRCPY(cd->output_buffer, "</PRE>");
		if(ce->status > -1)
			ce->status = PUTSTRING(cd->output_buffer);
	  }
		
	cd->next_state = FTP_ERROR_DONE;
	return MK_INTERRUPTED;
}

/* get a reply from the server
 */
PRIVATE int 
net_ftp_response (ActiveEntry *ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;
    int result_code;        /* numerical result code from server */
    int status;
    char cont_char;
    char * line;

    status = NET_BufferedReadLine(cd->cc->csock, &line, &cd->data_buf, 
                        &cd->data_buf_size, &cd->pause_for_read);

	if(status == 0)
	  {
		/* this shouldn't really happen, but...
		 */
     	cd->next_state = cd->next_state_after_response;
     	cd->pause_for_read = FALSE; /* don't pause */
		return(0);
	  }
    else if(status < 0)
      {
		int rv = SOCKET_ERRNO;

        if (rv == XP_ERRNO_EWOULDBLOCK)
          {
            cd->pause_for_read = TRUE;
            return(0);
          }

		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, rv);

    	/* return TCP error
         */
		return MK_TCP_READ_ERROR;
      }
	else if(!line)
	  {
         return status; /* wait for line */
	  }

    TRACEMSG(("    Rx: %s", line));

	/* Login messages or CWD messages 
     * Their so handy, those help messages are :)
     */
    if(cd->cc->server_type == FTP_UNIX_TYPE && !XP_STRNCMP(line, "250-",4))
      {
        StrAllocCat(cd->cwd_message_buffer, line+4);
        StrAllocCat(cd->cwd_message_buffer, "\n");  /* nasty */
      }
    else if(!XP_STRNCMP(line,"230-",4))
      {
        StrAllocCat(cd->login_message_buffer, line+4);
        StrAllocCat(cd->login_message_buffer, "\n");  /* nasty */
      }
    else if(cd->cont_response == 230)
      {
	 	/* this does the same thing as the one directly above it
		 * but it checks for 230 response code instead
	  	 * this is to fix microsofts stupid buggy ftp server
		 * that doesn't follow the specs
		 */
		if(XP_STRNCMP(line, "230", 3))
		  {
			/* make sure it's not  "230 Guest login ok...
			 */
            StrAllocCat(cd->login_message_buffer, line);
            StrAllocCat(cd->login_message_buffer, "\n");  /* nasty */
		  }
      }

	 result_code = 0;          /* defualts */
	 cont_char = ' ';  /* defualts */
     sscanf(line, "%d%c", &result_code, &cont_char);

     if(cd->cont_response == -1) 
       {
         if (cont_char == '-')  /* start continuation */
             cd->cont_response = result_code;

         StrAllocCopy(cd->return_msg, line+4);
       } 
     else 
       {    /* continuing */
      	 if (cd->cont_response == result_code && cont_char == ' ')
             cd->cont_response = -1;    /* ended */

         StrAllocCat(cd->return_msg, "\n");
		 if(XP_STRLEN(line) > 3)
		   {
			 if(isdigit(line[0]))
                 StrAllocCat(cd->return_msg, line+4);
			 else
                 StrAllocCat(cd->return_msg, line);
		   }
		 else
		   {
             StrAllocCat(cd->return_msg, line);
		   }
       }    

     if(cd->cont_response == -1)  /* all done with this response? */
       {
     	 cd->next_state = cd->next_state_after_response;
     	 cd->pause_for_read = FALSE; /* don't pause */
       }
	 if(result_code == 551)
	   {
		 return net_ftp_show_error(ce, line + 4);
	   }
        
    cd->response_code = result_code/100;

    return(1);  /* everything ok */
}

PRIVATE int
net_send_username(FTPConData * cd)
{

	/* look for the word UNIX in the hello message
	 * if it is there assume a unix server that can
	 * use the LIST command
	 */	
	if(cd->return_msg && strcasestr(cd->return_msg, "UNIX"))
	  {
        cd->cc->server_type = FTP_UNIX_TYPE;
		cd->cc->use_list = TRUE;
	  }

    if (cd->username)
      {
        PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "USER %.256s%c%c", cd->username, CR, LF);
      }
    else
      {
        PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "USER anonymous%c%c", CR, LF);
      }
	
	TRACEMSG(("FTP Rx: %.256s", cd->output_buffer));

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_USERNAME_RESPONSE;
    cd->pause_for_read = TRUE;

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}

PRIVATE int
net_send_username_response(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    if (cd->response_code == 3)  /* Send password */
      {
		if(!cd->password)
            cd->next_state = FTP_GET_PASSWORD;
		else 
			cd->next_state = FTP_SEND_PASSWORD;
      }
    else if (cd->response_code == 2) /* Logged in */
      {
        cd->next_state = FTP_SEND_SYST;
      }
    else
      {
		/* show the error
		 */
	 	StrAllocCopy(ce->URL_s->content_type, TEXT_HTML);
		ce->URL_s->expires = 1; /* immediate expire date */
        cd->stream = net_ftp_make_stream(ce->format_out, ce->URL_s, ce->window_id);

		if(cd->stream)
		  {
#if 0 /* disable ftp retries */
			PR_snprintf(cd->output_buffer, 
						OUTPUT_BUFFER_SIZE, 
						"<META HTTP-EQUIV=REFRESH CONTENT=\"10\">");
			if(ce->status > -1)
    			ce->status = PUTSTRING(cd->output_buffer);
#endif /* 0 */

			PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, XP_GetString(XP_HTML_FTPERROR_NOLOGIN)) ;
			if(ce->status > -1)
    			ce->status = PUTSTRING(cd->output_buffer);
			if(cd->return_msg && ce->status > -1)
    		    ce->status = PUTSTRING(cd->return_msg);
			PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "</PRE>");
			if(ce->status > -1)
    			ce->status = PUTSTRING(cd->output_buffer);

			if(ce->status > -1)
           		COMPLETE_STREAM;
			else
                ABORT_STREAM(ce->status);

			FREE(cd->stream);
			cd->stream = NULL;
		  }
		
        cd->next_state = FTP_ERROR_DONE;

		/* make sure error_msg is empty */
		FREE_AND_CLEAR(ce->URL_s->error_msg);

        return MK_UNABLE_TO_LOGIN;

      }

    return(0); /* good */
}


PRIVATE int
net_send_password(FTPConData * cd)
{

    PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, 
				"PASS %.256s%c%c", 
				NET_UnEscape(cd->password), 
				CR, LF);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_PASSWORD_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Tx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}

PRIVATE int
net_send_password_response(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    if (cd->response_code == 3)
      {
        cd->next_state = FTP_SEND_ACCT;
      }
    else if (cd->response_code == 2) /* logged in */
      {
        cd->next_state = FTP_SEND_SYST;
      }
    else
      {
		FREEIF(ftp_last_password_host);
		FREEIF(ftp_last_password);

        cd->next_state = FTP_ERROR_DONE;
        FE_Alert(ce->window_id, cd->return_msg ? cd->return_msg :
								 XP_GetString( XP_COULD_NOT_LOGIN_TO_FTP_SERVER ) );
		/* no error status message in this case 
		 */
        return MK_UNABLE_TO_LOGIN;  /* negative return code */
      }

    return(0); /* good */
}
    

PRIVATE int
net_send_acct(FTPConData * cd)
{

    PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "ACCT noaccount%c%c", CR, LF);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_ACCT_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Tx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}

PRIVATE int
net_send_acct_response(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    if (cd->response_code == 2)
      {
        cd->next_state = FTP_SEND_SYST;
      }
    else
      {
        cd->next_state = FTP_ERROR_DONE;
        FE_Alert(ce->window_id, cd->return_msg ? cd->return_msg : 
								 XP_GetString( XP_COULD_NOT_LOGIN_TO_FTP_SERVER ) );
		/* no error status message in this case 
		 */
        return MK_UNABLE_TO_LOGIN;  /* negative return code */
      }

    return(0); /* good */
}

PRIVATE int
net_send_syst(FTPConData * cd)
{
    

    PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "SYST%c%c", CR, LF);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_SYST_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Tx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}


PRIVATE int
net_send_syst_response(FTPConData * cd)
{
    if (cd->response_code == 2)
      {
        TRACEMSG(("SYST response: %s",cd->return_msg));

        /* default next state will be to try PASV mode
		 *
		 * TEST: set this default next state to FTP_SEND_PORT
		 * to test the non-pasv mode ftp
         */
		if(cd->use_default_path)
			cd->next_state = FTP_SEND_PWD;
    	else
        	cd->next_state = FTP_FIGURE_OUT_WHAT_TO_DO;

                /* we got a line -- what kind of server are we talking to? */
        if (!XP_STRNCMP(cd->return_msg, "UNIX Type: L8 MAC-OS MachTen", 28))
          {
            cd->cc->server_type = FTP_MACHTEN_TYPE;
            cd->cc->use_list = TRUE;
          }
        else if (XP_STRSTR(cd->return_msg, "UNIX") != NULL)
          {
             cd->cc->server_type = FTP_UNIX_TYPE;
             cd->cc->use_list = TRUE;
          }
        else if (XP_STRSTR(cd->return_msg, "Windows_NT") != NULL)
          {
             cd->cc->server_type = FTP_NT_TYPE;
             cd->cc->use_list = TRUE;
          }
        else if (XP_STRNCMP(cd->return_msg, "VMS", 3) == 0)
          {
             cd->cc->server_type = FTP_VMS_TYPE;
             cd->cc->use_list = TRUE;
          }
        else if ((XP_STRNCMP(cd->return_msg, "VM/CMS", 6) == 0)
                                 || (XP_STRNCMP(cd->return_msg, "VM ", 3) == 0))
          {
             cd->cc->server_type = FTP_CMS_TYPE;
          }
        else if (XP_STRNCMP(cd->return_msg, "DCTS", 4) == 0)
          {
             cd->cc->server_type = FTP_DCTS_TYPE;
          }
        else if (XP_STRSTR(cd->return_msg, "MAC-OS TCP/Connect II") != NULL)
          {
             cd->cc->server_type = FTP_TCPC_TYPE;
         	 cd->next_state = FTP_SEND_PWD;
          }
        else if (XP_STRNCMP(cd->return_msg, "MACOS Peter's Server", 20) == 0)
          {
             cd->cc->server_type = FTP_PETER_LEWIS_TYPE;
             cd->cc->use_list = TRUE;
             cd->next_state = FTP_SEND_MAC_BIN; 
          }
        else if(cd->cc->server_type == FTP_GENERIC_TYPE)
          {
              cd->next_state = FTP_SEND_PWD; 
          }
      }
    else
      {
        cd->next_state = FTP_SEND_PWD;
      }

    TRACEMSG(("Set server type to: %d\n",cd->cc->server_type));

    return(0);  /* good */

} 

PRIVATE int
net_send_mac_bin(FTPConData * cd)
{

    PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "MACB%c%c", CR, LF);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_MAC_BIN_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Tx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}


PRIVATE int
net_send_mac_bin_response(FTPConData * cd)
{
    if(cd->response_code == 2)
      {  /* succesfully set mac binary */
        if(cd->cc->server_type == FTP_UNIX_TYPE)
            cd->cc->server_type = FTP_NCSA_TYPE;  /* we were unsure here */
      }

	cd->next_state = FTP_FIGURE_OUT_WHAT_TO_DO;
    
    return(0); /* continue */
}

int
net_figure_out_what_to_do(ActiveEntry *ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

	switch(ce->URL_s->method)
	  {
		case URL_MKDIR_METHOD:
			cd->next_state = FTP_SEND_MKDIR;
			break;

		case URL_DELETE_METHOD:
			cd->next_state = FTP_SEND_DELETE_FILE;
			break;

		case URL_PUT_METHOD:
			/* trim the path to the last slash */
			{
				char * slash = XP_STRRCHR(cd->path, '/');
				if(slash)
					*slash = '\0';
			}
			/* fall through */
			goto get_method_case;
					
		case URL_POST_METHOD:
			/* we support POST if files_to_post
			 * is filed in
			 */
			XP_ASSERT(ce->URL_s->files_to_post);
			/* fall through */

		case URL_GET_METHOD:
get_method_case:  /* ack! goto alert */

			/* don't send PASV if the server cant do it */
			if(cd->cc->no_pasv || !net_use_pasv)
				cd->next_state = FTP_SEND_PORT;
			else
				cd->next_state = FTP_SEND_PASV;
			break;

		default:
			XP_ASSERT(0);
	  }

	return(0);
}

PRIVATE int
net_send_mkdir(ActiveEntry *ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    PR_snprintf(cd->output_buffer, 
				OUTPUT_BUFFER_SIZE, 
				"MKD %s"CRLF, 
				cd->path);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_MKDIR_RESPONSE;
    cd->pause_for_read = TRUE;

    TRACEMSG(("FTP Rx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock,
                             cd->output_buffer,
                             XP_STRLEN(cd->output_buffer)));
}

PRIVATE int
net_send_mkdir_response(ActiveEntry *ce)
{
	FTPConData * cd = (FTPConData *)ce->con_data;

	if (cd->response_code !=2) 
      {
		/* error */
		ce->URL_s->error_msg = NET_ExplainErrorDetails(
												MK_COULD_NOT_CREATE_DIRECTORY,
												cd->path);
		return(MK_COULD_NOT_CREATE_DIRECTORY);
	  }
	
	/* success */

	cd->next_state = FTP_DONE;

	return(0);
}


PRIVATE int
net_send_delete_file(ActiveEntry *ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    PR_snprintf(cd->output_buffer, 
				OUTPUT_BUFFER_SIZE, 
				"DELE %s"CRLF, 
				cd->path);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_DELETE_FILE_RESPONSE;
    cd->pause_for_read = TRUE;

    TRACEMSG(("FTP Rx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock,
                             cd->output_buffer,
                             XP_STRLEN(cd->output_buffer)));
}

PRIVATE int
net_send_delete_file_response(ActiveEntry *ce)
{
	FTPConData * cd = (FTPConData *)ce->con_data;

	if (cd->response_code !=2) 
      {
		/* error */
		/* try and delete it as a file */
		cd->next_state = FTP_SEND_DELETE_DIR;
		return(0);
	  }
	
	/* success */

	cd->next_state = FTP_DONE;

	return(0);
}

PRIVATE int
net_send_delete_dir(ActiveEntry *ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    PR_snprintf(cd->output_buffer, 
				OUTPUT_BUFFER_SIZE, 
				"RMD %s"CRLF, 
				cd->path);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_DELETE_DIR_RESPONSE;
    cd->pause_for_read = TRUE;

    TRACEMSG(("FTP Rx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock,
                             cd->output_buffer,
                             XP_STRLEN(cd->output_buffer)));
}

PRIVATE int
net_send_delete_dir_response(ActiveEntry *ce)
{
	FTPConData * cd = (FTPConData *)ce->con_data;

	if (cd->response_code !=2) 
      {
		/* error */
		ce->URL_s->error_msg = NET_ExplainErrorDetails(
												MK_UNABLE_TO_DELETE_DIRECTORY,
												cd->path);
		return(MK_UNABLE_TO_DELETE_DIRECTORY);
	  }
	
	/* success */

	cd->next_state = FTP_DONE;

	return(0);
}

PRIVATE int
net_send_pwd(FTPConData * cd)
{
    PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "PWD%c%c", CR, LF);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_PWD_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Rx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}

PRIVATE int
net_send_pwd_response(ActiveEntry *ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;
    char *cp = XP_STRCHR(cd->return_msg+5,'"');

    if(cp) 
        *cp = '\0';

    /* default next state */
    /* don't send PASV if the server cant do it */
    cd->next_state = FTP_FIGURE_OUT_WHAT_TO_DO;

    if (cd->cc->server_type == FTP_TCPC_TYPE)
      {
        cd->cc->server_type = (cd->return_msg[1] == '/' ? 
								FTP_NCSA_TYPE : FTP_TCPC_TYPE);
      }
	else if(cd->cc->server_type == FTP_GENERIC_TYPE)
	  {
    	if (cd->return_msg[1] == '/')
          {
            /* path names beginning with / imply Unix,
             * right?
             */
              cd->cc->server_type = FTP_UNIX_TYPE;
              cd->cc->use_list = TRUE;
              cd->next_state = FTP_SEND_MAC_BIN;  /* see if it's a mac */
           }
         else if (cd->return_msg[XP_STRLEN(cd->return_msg)-1] == ']')
           {
             /* path names ending with ] imply VMS, right? */
             cd->cc->server_type = FTP_VMS_TYPE;
             cd->cc->use_list = TRUE;
           }
	  }

	/* use the path returned by PWD if use_default_path is set
	 * don't do this for vms hosts
	 */
	if(cd->use_default_path && cd->cc->server_type != FTP_VMS_TYPE)
	  {
		char *path = XP_STRTOK(&cd->return_msg[1], "\"");
		char *ptr;
		char *existing_path = cd->path;

		/* NT can return a drive letter.  We can't
		 * deal with a drive letter right now
		 * so we need to just stript the drive letter
		 */
		if(*path != '/')
		  ptr = XP_STRCHR(path, '/');
		else
		  ptr = path;

		if(ptr)
		  {
			char * new_url;

			cd->path = NULL;
			StrAllocCopy(cd->path, ptr);

			if(existing_path && existing_path[0] != '\0')
		 	  {
			  	/* if the new path has a / on the
				 * end remove it since the
				 * existing path already has
				 * a slash at the beginning
				 */
				if(cd->path[0] != '\0')
				  {
					char *end = &cd->path[XP_STRLEN(cd->path)-1];
					if(*end == '/')
						*end = '\0';
				  }
				StrAllocCat(cd->path, existing_path);
		 	  }

			/* strip the path part from the URL
			 * and add the new path
			 */
			new_url = NET_ParseURL(ce->URL_s->address, 
								   GET_PROTOCOL_PART| GET_HOST_PART);

			StrAllocCat(new_url, cd->path);
			
			FREE(ce->URL_s->address);

			ce->URL_s->address = new_url;
		  }


		/* make sure we don't do this again for some reason */
		cd->use_default_path = FALSE;
	  }

     if (cd->cc->server_type == FTP_NCSA_TYPE ||
                        cd->cc->server_type == FTP_TCPC_TYPE ||
                        cd->cc->server_type == FTP_PETER_LEWIS_TYPE)
         cd->next_state = FTP_SEND_MAC_BIN;

     return(0); /* good */
}


PRIVATE int
net_send_pasv(FTPConData * cd)
{

    PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "PASV%c%c", CR, LF);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_PASV_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Rx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}


PRIVATE int
net_send_pasv_response(FTPConData * cd)
{
    int    h0, h1, h2, h3; 
    int32  p0, p1;
    int32  pasv_port;
    char  *cp;
    int    status;

    if (cd->response_code !=2) 
      {
        cd->next_state = FTP_SEND_PORT; /* default next state */
        cd->pasv_mode = FALSE;
        cd->cc->no_pasv = TRUE;  /* pasv doesn't work don't try it again */
        return(0); /* continue */
      }

    cd->cc->no_pasv = FALSE;  /* pasv does work */

    TRACEMSG(("PASV response: %s\n",cd->return_msg));

    /* find first comma or end of line */
    for(cp=cd->return_msg; *cp && *cp != ','; cp++)
        ; /* null body */

    /* find the begining of digits */
    while (--cp > cd->return_msg && '0' <= *cp && *cp <= '9')
        ; /* null body */   

	/* go past the '(' when the return is of the
	 * form (128,234,563,356,356)
	 */
	if(*cp < '0' || *cp > '9')
		cp++;

    status = sscanf(cp,
#ifdef __alpha
		"%d,%d,%d,%d,%d,%d",
#else
		"%d,%d,%d,%d,%ld,%ld",
#endif
		&h0, &h1, &h2, &h3, &p0, &p1);
    if (status<6)
      {
        TRACEMSG(("FTP: PASV reply has no inet address!\n"));
        cd->next_state = FTP_SEND_PORT; /* default next state */
        cd->pasv_mode = FALSE;
        return(0); /* continue */
      }

    pasv_port = ((int32) (p0<<8)) + p1;
    TRACEMSG(("FTP: Server is listening on port %d\n", pasv_port));

    /* Open connection for data:
     */
    PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "%d.%d.%d.%d:%ld",h0,h1,h2,h3,pasv_port);
    StrAllocCopy(cd->data_con_address, cd->output_buffer);

    cd->next_state = FTP_PASV_DATA_CONNECT;

    return(0); /* OK */
}


PRIVATE int
net_send_port(ActiveEntry * ce)
{
	FTPConData * cd = (FTPConData *) ce->con_data;
    struct sockaddr_in soc_address; /* Binary network address */
    struct sockaddr_in* sin_struct = &soc_address;
    int    address_length;
    int    status;
    
    TRACEMSG(("Entering get_listen_socket with control socket: %d\n", cd->cc->csock));

	cd->pasv_mode = FALSE;  /* not using pasv mode if we get here */

    /*  Create internet socket
    */
    cd->listen_sock = NETSOCKET(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	
#ifdef XP_WIN
	{
      struct linger {
       	  int l_onoff;
       	  int l_linger;
      } lingerstruct;

      FE_SetupAsyncSelect(cd->listen_sock, ce->window_id);
      lingerstruct.l_onoff = 1;
      lingerstruct.l_linger = 0;
      SSL_SetSockOpt(cd->listen_sock, SOL_SOCKET, SO_LINGER,
                     &lingerstruct, sizeof(lingerstruct));
	}
#else
	{
      /* lets try to set the socket non blocking
       * so that we can do multi threading :)
       */
      int opt = 1;
      int sock_status = SOCKET_IOCTL(cd->listen_sock, FIONBIO, &opt);
      if (sock_status == SOCKET_INVALID)
          NET_Progress(ce->window_id, 
				XP_GetString( XP_ERROR_COULD_NOT_MAKE_CONNECTION_NON_BLOCKING ) );
	}
#endif

    
    if(cd->listen_sock == SOCKET_INVALID)
	  {
		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CREATE_SOCKET);
        return MK_UNABLE_TO_CREATE_SOCKET;
	  }

#ifndef NO_SSL
   if(NET_SocksHost)
      {
        TRACEMSG(("Using socks for this connection"));

        SSL_Enable(cd->listen_sock, SSL_SOCKS, 1);

        SSL_ConfigSockd(cd->listen_sock, NET_SocksHost, NET_SocksPort);

      }
#endif /* NO_SSL */

    /*  Search for a free port.
    */
    sin_struct->sin_family = AF_INET;      /* Family = internet, host order  */
    sin_struct->sin_addr.s_addr = INADDR_ANY; /* Any peer address */

    address_length = sizeof(soc_address);
    status = NETGETSOCKNAME(cd->cc->csock, (struct sockaddr *)&soc_address, &address_length);
    if (status<0) 
      {
        TRACEMSG(("Failure in getsockname for control connection: %d\n", 
                                cd->cc->csock));
        return status;
      }
    
    soc_address.sin_port = 0;   /* let bind allocate it */
    status=NETBIND(cd->listen_sock, (struct sockaddr*)&soc_address,
                    /* Cast to generic sockaddr */ sizeof(soc_address));
    	if (status<0) 
      	  {
        TRACEMSG(("Failure in bind for listen socket: %d\n", cd->listen_sock));
        	return status;
      	  }

    TRACEMSG(("FTP: This host is %s\n", NET_IpString(sin_struct)));
    
    address_length = sizeof(soc_address);
    status = NETGETSOCKNAME(cd->listen_sock, (struct sockaddr*)&soc_address, &address_length);
    if (status<0)   
        return status;

    TRACEMSG(("FTP: bound to port %d on %s\n",(int)ntohs(sin_struct->sin_port),NET_IpString(sin_struct)));

    /*  Now we must tell the other side where to connect
     */
    PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "PORT %d,%d,%d,%d,%d,%d%c%c",
            (int)* ((unsigned char*) (&sin_struct->sin_addr)+0),
            (int)* ((unsigned char*) (&sin_struct->sin_addr)+1),
            (int)* ((unsigned char*) (&sin_struct->sin_addr)+2),
            (int)* ((unsigned char*) (&sin_struct->sin_addr)+3),
            (int)* ((unsigned char*) (&sin_struct->sin_port)+0),
            (int)* ((unsigned char*) (&sin_struct->sin_port)+1),
            CR, LF);

    /*  Inform TCP that we will accept connections
     */
    if (NETLISTEN(cd->listen_sock, 1) < 0) 
      {
		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_LISTEN_ON_SOCKET);
        return MK_UNABLE_TO_LISTEN_ON_SOCKET;
      }

    TRACEMSG(("TCP: listen socket(), bind() and listen() all OK.\n"));

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_PORT_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Tx: %s", cd->output_buffer));

    /* Inform the server of the port number we will listen on
     * The port_command came from above
     */
    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));

} /* get_listen */

PRIVATE int
net_send_port_response(ActiveEntry * ce)
{
	FTPConData * cd = (FTPConData *) ce->con_data;

    if (cd->response_code !=2)
      {
        TRACEMSG(("FTP: Bad response (port_command: %s)\n",cd->return_msg));
		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_SEND_PORT_COMMAND);

        return MK_UNABLE_TO_SEND_PORT_COMMAND;
      }

    /* else */

    cd->next_state = FTP_GET_FILE_TYPE;
    return(0); /* good */
}

/* figure out what file transfer type to use
 * and set the next state to set it if it needs
 * set or else begin the transfer if it's already
 * set correctly
 */
PRIVATE int
net_get_ftp_file_type(ActiveEntry *ce)
{
	FTPConData * cd = (FTPConData *)ce->con_data;

    NET_cinfo * cinfo_struct;

	/* figure out if we are puttting a file
	 * and if we are figure out which
	 * file we are doing now 
	 */
	if(ce->URL_s->files_to_post &&
		ce->URL_s->files_to_post[0] != 0)
	  {
		/* files_to_post is an array of filename pointers
		 * find the last one in the list and
		 * remove it once done.
		 */
		char **array = ce->URL_s->files_to_post;
		int i;
		
		if(!cd->total_size_of_files_to_post)
		  {
		  	/* If we don't know the total size
			 * of all the files we are posting
			 * figure it out now
			 */
			XP_StatStruct stat_entry; 
			
			for(i=0; array[i]; i++)
			  {
				if(XP_Stat(array[i], 
						   &stat_entry, xpFileToPost) != -1)
					cd->total_size_of_files_to_post += stat_entry.st_size;
			  }

			/* start the graph progress now since we know the size
		 	 */
			ce->URL_s->content_length = cd->total_size_of_files_to_post; 
	    	FE_GraphProgressInit(ce->window_id, ce->URL_s, ce->URL_s->content_length);
			cd->destroy_graph_progress = TRUE;
			cd->original_content_length = ce->URL_s->content_length;

#ifdef EDITOR
			/* Don't show the dialog if the data is being delivered to a plug-in */
			if (CLEAR_CACHE_BIT(ce->format_out) != FO_PLUGIN)
			{
	            FE_SaveDialogCreate(ce->window_id, i, TRUE );
	            cd->destroy_file_upload_progress_dialog = TRUE;
	        }
#endif /* EDITOR */
		  }  
		   		      
		/* posting files is started as a POST METHOD
		 * eventually though, after posting the files
		 * we end up with a directory listing which
		 * should be treated as a GET METHOD.  Change
		 * the method now so that the cache doesn't
		 * get confused about the method as it caches
		 * the directory listing at the end
		 */
		if(ce->URL_s->method == URL_POST_METHOD)
			ce->URL_s->method = URL_GET_METHOD;

		for(i=0; array[i] != 0; i++)
		    ; /* null body */
		
	    if(i < 1)
		  {
			/* no files left to post
			 * quit
			 */
			cd->next_state = FTP_DONE;
			return(0);
		  }

		FREEIF(cd->file_to_post);
		cd->file_to_post = array[i-1];
		array[i-1] = 0;

        cinfo_struct = NET_cinfo_find_type(cd->file_to_post);

        /* use binary mode always except for "text" types
		 * don't use ASCII for default types
         */
        if(!cinfo_struct->is_default 
			&& !strncasecomp(cinfo_struct->type, "text",4))
          {
            if(cd->cc->cur_mode != FTP_MODE_ASCII)
                cd->next_state = FTP_SEND_ASCII;
            else
                cd->next_state = FTP_SEND_CWD;
          }
        else
          {
            if(cd->cc->cur_mode != FTP_MODE_BINARY)
                cd->next_state = FTP_SEND_BIN;
            else
                cd->next_state = FTP_SEND_CWD;
          }

        return(0);  /* continue */
	  }
	else if(ce->URL_s->method == URL_PUT_METHOD)
	  {
		cd->next_state = FTP_SEND_BIN;
		return(0);
	  }

    if(cd->type == FTP_MODE_UNKNOWN)
      {  /* use extensions to determine the type */

		NET_cinfo * enc = NET_cinfo_find_enc(cd->filename);
        cinfo_struct = NET_cinfo_find_type(cd->filename);

		TRACEMSG(("%s", cinfo_struct->is_default ?
							"Default cinfo type found" : ""));

		TRACEMSG(("Current FTP transfer mode is: %d", cd->cc->cur_mode));

        /* use binary mode always except for "text" types
		 * don't use ASCII for default types
		 *
		 * if it has an encoding value, transfer as binary
         */
        if(!cinfo_struct->is_default 
			&& !strncasecomp(cinfo_struct->type, "text",4)
			&& (!enc || !enc->encoding))
          {
            if(cd->cc->cur_mode != FTP_MODE_ASCII)
                cd->next_state = FTP_SEND_ASCII;
            else
                cd->next_state = FTP_GET_FILE_SIZE;
          }
        else
          {
            if(cd->cc->cur_mode != FTP_MODE_BINARY)
                cd->next_state = FTP_SEND_BIN;
            else
                cd->next_state = FTP_GET_FILE_SIZE;
          }
      }
    else
      {  /* use the specified type */
        if(cd->cc->cur_mode == cd->type)
          {
            cd->next_state = FTP_GET_FILE_SIZE;
          }
        else
          {
            if(cd->type == FTP_MODE_ASCII)
                cd->next_state = FTP_SEND_ASCII;
            else
                cd->next_state = FTP_SEND_BIN;
          }
      }
  

    return(0);  /* continue */
}

PRIVATE int
net_send_bin(FTPConData * cd)
{

    PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "TYPE I%c%c", CR, LF);
    cd->cc->cur_mode = FTP_MODE_BINARY;

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_BIN_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Tx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}


PRIVATE int
net_send_ascii(FTPConData * cd)
{

    PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "TYPE A%c%c", CR, LF);
    cd->cc->cur_mode = FTP_MODE_ASCII;

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_ASCII_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Rx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}

PRIVATE int
net_send_bin_or_ascii_response(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *) ce->con_data;

    if (cd->response_code != 2)
	  {
		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CHANGE_FTP_MODE);
        return MK_UNABLE_TO_CHANGE_FTP_MODE;
	  }

    /* else */
	if(cd->file_to_post || ce->URL_s->method == URL_PUT_METHOD)
    	cd->next_state = FTP_SEND_CWD;
	else
    	cd->next_state = FTP_GET_FILE_SIZE;

    return(0); /* good */
}

PRIVATE int
net_get_ftp_file_size(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *) ce->con_data;

    if (cd->cc->server_type == FTP_VMS_TYPE) 
	  {
		/* skip this since I'm not sure how it should work on VMS
		 */
		cd->next_state = FTP_GET_FILE;
		return(0);
	  }

    PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "SIZE %.1024s%c%c", cd->path, CR, LF);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_GET_FILE_SIZE_RESPONSE;
    cd->pause_for_read = TRUE;

    TRACEMSG(("FTP Rx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock,
                             cd->output_buffer,
                             XP_STRLEN(cd->output_buffer)));
}

PRIVATE int
net_get_ftp_file_size_response(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *) ce->con_data;

    if(cd->response_code == 2 && cd->return_msg)
	  {
		/* The response looks like  "160452"
		 * where "160452" is the size in bytes.
		 */
		ce->URL_s->content_length = atol(cd->return_msg);

		TRACEMSG(("Set content length: %ld",  ce->URL_s->content_length));
	  }

	cd->next_state = FTP_GET_FILE;
	
	return(0);
}

PRIVATE int
net_get_ftp_file(FTPConData * cd)
{
    
    /* if this is a VMS server then we need to 
     * do some special things to map a UNIX syntax
     * ftp url to a VMS type file syntax
     */
    if (cd->cc->server_type == FTP_VMS_TYPE) 
      {
        /* If we want the VMS account's top directory speed to it 
		 */
        if (cd->path[0] == '\0' || (cd->path[0] == '/' && cd->path[1]== '\0')) 
          {
        	cd->is_directory = YES;
        	cd->next_state = FTP_SEND_LIST_OR_NLST;
          }
		else if(!XP_STRCHR(cd->path, '/'))
		  {
			/* if we want a file out of the top directory skip the PWD
			 */
			cd->next_state = FTP_SEND_RETR;
		  }
		else
		  {
    	    cd->next_state = FTP_SEND_CWD;
		  }
    
      }
    else
      {  /* non VMS */
		/* if we have already done the RETR or
		 * if the path ends with a slash
		 */
		if(cd->retr_already_failed ||
			'/' == cd->path[XP_STRLEN(cd->path)-1])
        	cd->next_state = FTP_SEND_CWD;
		else
        	cd->next_state = FTP_SEND_RETR;
      }

    return(0); /* continue */
}

PRIVATE int
net_send_cwd(FTPConData * cd)
{

    if (cd->cc->server_type == FTP_VMS_TYPE) 
      {
        char *cp, *cp1;
        /** Try to go to the appropriate directory and doctor filename **/
        if ((cp=XP_STRRCHR(cd->path, '/'))!=NULL) 
          {
			*cp = '\0';
        	PR_snprintf(cd->output_buffer, 
					   OUTPUT_BUFFER_SIZE, 
					   "CWD [%.1024s]" CRLF, 
					   cd->path);
			*cp = '/';  /* set it back so it is left unmodified */

			/* turn slashes into '.' */
        	while ((cp1=XP_STRCHR(cd->output_buffer, '/')) != NULL)
            	*cp1 = '.';
          }
        else
          {
			PR_snprintf(cd->output_buffer, 
					   OUTPUT_BUFFER_SIZE, 
					   "CWD [.%.1024s]" CRLF, 
					   cd->path);
          }
      } /* if FTP_VMS_TYPE */
    else
      {  /* non VMS server */
        PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "CWD %.1024s" CRLF, cd->path);
      }

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_CWD_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Tx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}

PRIVATE int
net_send_cwd_response(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *) ce->con_data;

    if (cd->response_code == 2) /* Successed : let's NAME LIST it */
      {  

		if(cd->file_to_post)
		  {
            cd->next_state = FTP_BEGIN_PUT;
            return(0); /* good */
          }
		else if(ce->URL_s->method == URL_PUT_METHOD)
		  {
            cd->next_state = FTP_BEGIN_PUT;
            return(0); /* good */
		  }

        ce->URL_s->is_directory = TRUE;

        if (cd->cc->server_type == FTP_VMS_TYPE)
          {
			if(*cd->filename)
                cd->next_state = FTP_SEND_RETR;
			else
			 	cd->next_state = FTP_SEND_LIST_OR_NLST;
            return(0); /* good */
          }

        /* non VMS server */
        /* we are in the correct directory and we
         * already failed on the RETR so lets 
         * LIST or NLST it
         */
        cd->next_state = FTP_SEND_LIST_OR_NLST;
        return(0); /* good */
      }

    /* else */
	ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_LOCATE_FILE, 
													*cd->path ? cd->path : "/");

    return MK_UNABLE_TO_LOCATE_FILE;  /* error */
}

PRIVATE int
net_begin_put(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *) ce->con_data;
	char *path, *filename;

	if(cd->file_to_post)
	  {
		path = XP_STRDUP(cd->file_to_post);
		if(!path)
			return(MK_OUT_OF_MEMORY);
#ifdef XP_WIN
		filename = XP_STRRCHR(path, '\\');
#else
		filename = XP_STRRCHR(path, '/');
#endif
	  }
	else
	  {
		path = NET_ParseURL(ce->URL_s->address, GET_PATH_PART);
		if(!path)
			return(MK_OUT_OF_MEMORY);
		filename = XP_STRRCHR(path, '/');
	  }

	if(!filename)
		filename = path;
	else
		filename++; /* go past delimiter */

	PR_snprintf(cd->output_buffer, 
			   OUTPUT_BUFFER_SIZE, 
			   "STOR %.1024s" CRLF, 
			   filename);

#ifdef EDITOR
    if(cd->destroy_file_upload_progress_dialog)
    	FE_SaveDialogSetFilename(ce->window_id, filename);
#endif /* EDITOR */

	FREE(path);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_BEGIN_PUT_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Tx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}


PRIVATE int
net_begin_put_response(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *) ce->con_data;


    if (cd->response_code != 1) 
	  {
	    /* failed */
		ce->URL_s->error_msg = NET_ExplainErrorDetails(
								MK_COULD_NOT_PUT_FILE, 
								cd->file_to_post ? 
										cd->file_to_post : ce->URL_s->address,
								cd->return_msg ? cd->return_msg : "");
		return(MK_COULD_NOT_PUT_FILE);
	  }

    FE_ClearReadSelect(ce->window_id, ce->socket);
    ce->socket = cd->dsock;
    ce->con_sock = cd->dsock;

#ifdef XP_WIN
	{
		struct linger {
			int l_onoff;
			int l_linger;
		} lingerstruct;

        /* turn the linger off so that we don't
         * wait but we don't abort either
         * since aborting will lose quede data
         */
        lingerstruct.l_onoff = 0;
        lingerstruct.l_linger = 0;
        SSL_SetSockOpt(ce->socket, SOL_SOCKET, SO_LINGER,
                       &lingerstruct, sizeof(lingerstruct));
	}
#endif /* XP_WIN */

    /* set connect select because we are writting
     * not reading
     */
    FE_SetConnectSelect(ce->window_id, ce->socket);
#ifdef XP_WIN
	net_call_all_the_time_count++;
	cd->calling_netlib_all_the_time = TRUE;
	FE_SetCallNetlibAllTheTime(ce->window_id);
#endif

	PR_snprintf(cd->output_buffer,
			   OUTPUT_BUFFER_SIZE,
				XP_GetString( XP_POSTING_FILE ),
			   cd->file_to_post);

	NET_Progress(ce->window_id, cd->output_buffer);
		
    if(cd->pasv_mode)
      {
        cd->next_state = FTP_DO_PUT;
      }
    else
      { /* non PASV */
        cd->next_state = FTP_WAIT_FOR_ACCEPT;
      }

	return(0);

}

/* finally send the file
 */
PRIVATE int
net_do_put(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *) ce->con_data;

	if(!cd->write_post_data_data)
	  {
		/* first time through */

		/* if file_to_post is filled in then
		 * throw the name into the post data
		 * so that we can use the WritePostData
		 * function
		 */
		if(cd->file_to_post)
		  {
			ce->URL_s->post_data = cd->file_to_post;
			ce->URL_s->post_data_is_file = TRUE;
		  }
	  }

    /* returns 0 on done and negative on error
     * positive if it needs to continue.
     */
    ce->status = NET_WritePostData(ce->window_id, ce->URL_s,
                                  ce->socket,
                                  &cd->write_post_data_data,
								  (cd->cc->cur_mode == FTP_MODE_ASCII));

	cd->pause_for_read = TRUE;

    if(ce->status == 0)
      {
        /* normal done
         */
   		FE_ClearConnectSelect(ce->window_id, ce->socket);
#ifdef XP_WIN
		if(cd->calling_netlib_all_the_time)
		{
			net_call_all_the_time_count--;
			cd->calling_netlib_all_the_time = FALSE;
			if(net_call_all_the_time_count == 0)
				FE_ClearCallNetlibAllTheTime(ce->window_id);
		}
#endif
   		ce->socket = cd->cc->csock;
   		ce->con_sock = SOCKET_INVALID;
   		FE_SetReadSelect(ce->window_id, ce->socket);

        NETCLOSE(cd->dsock);
        cd->dsock = SOCKET_INVALID;

        if(cd->destroy_graph_progress)
		  {
            FE_GraphProgressDestroy(ce->window_id, 	
									ce->URL_s, 	
									cd->original_content_length,
									ce->bytes_received);
			cd->destroy_graph_progress = FALSE;
		  }

       	/* go read the response since we need to
       	 * get the "226 sucessful transfer" message
       	 * out of the read queue in order to cache
       	 * connections properly
       	 */
       	cd->next_state = FTP_WAIT_FOR_RESPONSE;

		if(cd->file_to_post)
		  {
			ce->URL_s->post_data = 0;
			ce->URL_s->post_data_is_file = FALSE;
			FREE_AND_CLEAR(cd->file_to_post);
		  }

		/* after getting the server response start
		 * over again if there are more
		 * files to send or to do a new directory
		 * listing...
		 */
      	cd->next_state_after_response = FTP_DONE;


		/* if this was the last file, check the history
		 * to see if the window is currently displaying an ftp
		 * listing.  If it is update the listing.  If it isnt
		 * quit.
		 */
		if(ce->URL_s->files_to_post)
		  {
			if(ce->URL_s->files_to_post[0])
			  {
				/* more files, keep going */
				cd->next_state_after_response = FTP_FIGURE_OUT_WHAT_TO_DO;
			  }
			else
		  	  {
				/* no more files, figure out if we should
				 * show an index
				 */
#ifdef MOZILLA_CLIENT
		  		History_entry * his = SHIST_GetCurrent(&ce->window_id->hist);
#else
            	History_entry * his = NULL;
#endif /* MOZILLA_CLIENT */

				if(his && !strncasecomp(his->address, "ftp:", 4))
			  	  {
					/* go get the index of the directory */
					cd->next_state_after_response = FTP_FIGURE_OUT_WHAT_TO_DO;
			  	  }
			  }
		  }
	  }
	else if(ce->status > 0)
	  {
    	ce->bytes_received += ce->status;
    	FE_GraphProgress(ce->window_id,
                     	ce->URL_s,
                     	ce->bytes_received,
                     	ce->status,
                     	ce->URL_s->content_length);
    	FE_SetProgressBarPercent(ce->window_id,
                             	(ce->bytes_received*100)
                               		/ce->URL_s->content_length);
	  }

	return(ce->status);
}

PRIVATE int
net_send_retr(FTPConData * cd)
{

    if(cd->cc->server_type == FTP_VMS_TYPE)
        PR_snprintf(cd->output_buffer, 
				   OUTPUT_BUFFER_SIZE,
				   "RETR %.1024s" CRLF, 
				   cd->filename);
    else
        PR_snprintf(cd->output_buffer, 
					OUTPUT_BUFFER_SIZE, 
					"RETR %.1024s" CRLF, 
					cd->path);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_RETR_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Tx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}


PRIVATE int
net_send_retr_response(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *) ce->con_data;

    if(cd->response_code != 1) 
      {
        if(cd->cc->server_type == FTP_VMS_TYPE) 
          {
	        ce->URL_s->error_msg = NET_ExplainErrorDetails(
												  MK_UNABLE_TO_LOCATE_FILE, 
												  *cd->path ? cd->path : "/");
            return(MK_UNABLE_TO_LOCATE_FILE);
          }
        else
          { /* non VMS */

#ifdef DOUBLE_PASV 
			/* this will use a new PASV connection
			 * for each command even if the one only
			 * failed and didn't transmit
			 */
			/* close the old pasv connection */
			NETCLOSE(cd->dsock);
			/* invalidate the old pasv connection */
			cd->dsock = SOCKET_INVALID;

			cd->next_state = FTP_SEND_PASV; /* send it again */
			cd->retr_already_failed = TRUE;
#else
			/* we need to use this to make the mac FTPD work
			 */
			/* old way */
            cd->next_state = FTP_SEND_CWD; 
#endif /* DOUBLE_PASV */
          }
      }
    else
      {   /* successful RETR */
        cd->next_state = FTP_SETUP_STREAM;
      }

    return(0); /* continue */
}

PRIVATE int
net_send_list_or_nlst(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    if(cd->cc->use_list && FE_UseFancyFTP(ce->window_id))
        XP_STRCPY(cd->output_buffer, "LIST" CRLF);
    else
        XP_STRCPY(cd->output_buffer, "NLST" CRLF);

    cd->next_state = FTP_WAIT_FOR_RESPONSE;
    cd->next_state_after_response = FTP_SEND_LIST_OR_NLST_RESPONSE;
    cd->pause_for_read = TRUE;

	TRACEMSG(("FTP Tx: %s", cd->output_buffer));

    return((int) NET_BlockingWrite(cd->cc->csock, 
							 cd->output_buffer, 
							 XP_STRLEN(cd->output_buffer)));
}

PRIVATE int
net_send_list_or_nlst_response(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    if(cd->response_code == 1)
      {  /* succesful list or nlst */
    	cd->is_directory = TRUE;
    	cd->next_state = FTP_SETUP_STREAM;

		/* add a slash to the end of the uRL if it doesnt' have one now
		 */
		if(ce->URL_s->address[XP_STRLEN(ce->URL_s->address)-1] != '/')
		  {
			/* update the global history before modification
			 */
#ifdef MOZILLA_CLIENT
			GH_UpdateGlobalHistory(ce->URL_s);
#endif /* MOZILLA_CLIENT */
			StrAllocCat(ce->URL_s->address, "/");
			ce->URL_s->address_modified = TRUE;
		  }

    	return(0); /* good */
      }
  
	ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_LOCATE_FILE, 
												  *cd->path ? cd->path : "/");
    return(MK_UNABLE_TO_LOCATE_FILE);
}

    
PRIVATE int
net_setup_ftp_stream(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    /* set up the data stream now
     */
    if(cd->is_directory)
      {
	 	StrAllocCopy(ce->URL_s->content_type, TEXT_HTML);
        cd->stream = net_ftp_make_stream(ce->format_out, ce->URL_s, ce->window_id);
      }
    else
      {
		NET_cinfo * cinfo_struct = NET_cinfo_find_type(cd->filename);
	 	StrAllocCopy(ce->URL_s->content_type, cinfo_struct->type);

		cinfo_struct = NET_cinfo_find_enc(cd->filename);
	 	StrAllocCopy(ce->URL_s->content_encoding, cinfo_struct->encoding);

        cd->stream = net_ftp_make_stream(ce->format_out, ce->URL_s, ce->window_id);
      }

    if (!cd->stream)
      {
        cd->next_state = FTP_ERROR_DONE;
	    ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CONVERT); 
        return(MK_UNABLE_TO_CONVERT);
      }

    if(cd->pasv_mode)
      {
        if(cd->is_directory)
           cd->next_state = FTP_START_READ_DIRECTORY;
        else
           cd->next_state = FTP_START_READ_FILE;
      }
    else
      { /* non PASV */
        cd->next_state = FTP_WAIT_FOR_ACCEPT;
      }

	/* start the graph progress indicator
     */
    FE_GraphProgressInit(ce->window_id, ce->URL_s, ce->URL_s->content_length);
	cd->destroy_graph_progress = TRUE;
	cd->original_content_length = ce->URL_s->content_length;

    return(0); /* continue */
}
    
PRIVATE int
net_wait_for_ftp_accept(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

/* make this non-blocking sometime */

    /* Wait for the connection
     */
    struct sockaddr_in soc_address;
    int    soc_addrlen = sizeof(soc_address);


    cd->dsock = NETACCEPT(cd->listen_sock, (struct sockaddr *)&soc_address, &soc_addrlen);
   
    if (cd->dsock == SOCKET_INVALID)
      {
		if(SOCKET_ERRNO == XP_ERRNO_EWOULDBLOCK)
		  {
			cd->pause_for_read = TRUE; /* pause */
    		FE_ClearReadSelect(ce->window_id, ce->socket);
    		ce->socket = cd->listen_sock;
    		FE_SetReadSelect(ce->window_id, ce->socket);
			return(0);
		  }
		else
		  {
	        ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_ACCEPT_SOCKET); 
            return MK_UNABLE_TO_ACCEPT_SOCKET;
		  }
      }

    TRACEMSG(("FTP: Accepted new socket %d\n", cd->dsock));

	if(cd->file_to_post || ce->URL_s->method == URL_POST_METHOD)
	  {
		cd->next_state = FTP_DO_PUT;
	  }
    else if (cd->is_directory)
      {
        cd->next_state = FTP_START_READ_DIRECTORY;
      }
    else
      {
        cd->next_state = FTP_START_READ_FILE;
      }

    return(0);
}

PRIVATE int
net_start_ftp_read_file(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    /* nothing much to be done */
    cd->next_state = FTP_READ_FILE;
    cd->pause_for_read = TRUE;

    FE_ClearReadSelect(ce->window_id, ce->socket);
    ce->socket = cd->dsock;
    FE_SetReadSelect(ce->window_id, ce->socket);

    NET_Progress (ce->window_id, XP_GetString(XP_PROGRESS_RECEIVE_FTPFILE));

    return(0); /* continue */
}

PRIVATE int
net_ftp_read_file(ActiveEntry * ce) 
{ 
	FTPConData * cd = (FTPConData *)ce->con_data; 
	int status; 
    unsigned int write_ready, read_size;

    /* check to see if the stream is ready for writing
     */
    write_ready = (*cd->stream->is_write_ready)(cd->stream->data_object);

    if(!write_ready)
      {
        cd->pause_for_read = TRUE;
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

	status = NETREAD(cd->dsock, NET_Socket_Buffer, read_size); 

	if(status > 0) 
	  { 
		ce->bytes_received += status; 
		FE_GraphProgress(ce->window_id, ce->URL_s, ce->bytes_received, status, ce->URL_s->content_length);
        status = PUTBLOCK(NET_Socket_Buffer, status);
        cd->pause_for_read = TRUE;
      }
    else if(status == 0)
      {
        /* go read the response since we need to
         * get the "226 sucessful transfer" message
         * out of the read queue in order to cache
         * connections properly
         */
        cd->next_state = FTP_WAIT_FOR_RESPONSE;
        cd->next_state_after_response = FTP_DONE;

		if(cd->dsock != SOCKET_INVALID)
	      {
		    FE_ClearReadSelect(ce->window_id, cd->dsock);
            NETCLOSE(cd->dsock);
            cd->dsock = SOCKET_INVALID;
		  }

		/* set select on the control socket */
    	ce->socket = cd->cc->csock;
    	FE_SetReadSelect(ce->window_id, ce->socket);
        /* try and read the response immediately,
         * so don't pause for read
         *
         * cd->pause_for_read = TRUE;
         */

        return(MK_DATA_LOADED);
      }
	else
	  {
		/* status less than zero
	     */
	      int rv = SOCKET_ERRNO;

		if (rv == XP_ERRNO_EWOULDBLOCK)
		  {
			cd->pause_for_read = TRUE;
			return 0;
		  }
	      status = (rv < 0) ? rv : (-rv);
	  }

   return(status);
}

PRIVATE int
net_start_ftp_read_dir(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    cd->sort_base = NET_SortInit();

    NET_Progress (ce->window_id, XP_GetString(XP_PROGRESS_RECEIVE_FTPDIR));

    if (*cd->path != '\0')  /* Not Empty path: */
      {
        int end;
        NET_UnEscape(cd->path);

        end = XP_STRLEN(cd->path)-1;
        /* if the path ends with a slash kill it.
         * that includes the path "/"
         */
        if(cd->path[end] == '/')
          {
             cd->path[end] = 0; /* kill the last slash */
          }
      }

#ifdef DEBUG_jwz /* mostly from Kartik Subbarao <subbarao@computer.org> */
    {
      char *path = XP_STRDUP(cd->path);
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
      PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE,
                  XP_GetString( XP_H1_DIRECTORY_LISTING ), hpath);

      FREEIF (path);
    }

#else /* !DEBUG_jwz */
	PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE,
				XP_GetString( XP_TITLE_DIRECTORY_OF_ETC ),
				*cd->path ? cd->path : "/", *cd->path ? cd->path : "/");
#endif /* DEBUG_jwz */



	if(ce->status > -1)
    	ce->status = PUTSTRING(cd->output_buffer);

    if(cd->login_message_buffer) /* Don't check !(*cd->path) here or this
									message never shows up.  It's supposed to
									be shown the first time you connect to
									this server, whether you started at the
									root or not. */
      {
		if(ce->status > -1)
		  {
			XP_STRCPY(cd->output_buffer, "<HR>");
			ce->status = PUTSTRING(cd->output_buffer);

			if(ce->status > -1)
			  {
#define HTML_STRING "<HTML>"
				if(!XP_STRNCMP(cd->login_message_buffer,
							  HTML_STRING,
							  XP_STRLEN(HTML_STRING)))
				  {
#ifndef DEBUG_jwz
					char p1[] = "<PRE>";
					char p2[] = "</PRE>";
					ce->status = PUTSTRING(p2);
#endif /* DEBUG_jwz */
					if (ce->status > -1)
					  ce->status = PUTSTRING(cd->login_message_buffer);
#ifndef DEBUG_jwz
					if (ce->status > -1)
					  ce->status = PUTSTRING(p1);
#endif /* DEBUG_jwz */
				  }
				else
				  {
#ifdef DEBUG_jwz
					char p1[] = "<PRE>";
					char p2[] = "</PRE>";
					ce->status = PUTSTRING(p1);
#endif /* DEBUG_jwz */
					ce->status =
					  NET_ScanForURLs (cd->login_message_buffer,
									   XP_STRLEN(cd->login_message_buffer),
									   cd->output_buffer, OUTPUT_BUFFER_SIZE,
									   TRUE);
					if (ce->status < 0)
					  return ce->status;
					ce->status = PUTSTRING(cd->output_buffer);
#ifdef DEBUG_jwz
					if (ce->status > -1)
					  ce->status = PUTSTRING(p2);
#endif /* DEBUG_jwz */
				  }
			  }
			if (!cd->cwd_message_buffer && ce->status > -1)
			  {
				XP_STRCPY(cd->output_buffer, "<HR>");
				ce->status = PUTSTRING(cd->output_buffer);
			  }
		  }
      }
    if(cd->cwd_message_buffer)
      {
		if(ce->status > -1)
		  {
			XP_STRCPY(cd->output_buffer, "<HR>");
			ce->status = PUTSTRING(cd->output_buffer);

			if(ce->status > -1)
			  {
#ifdef I_DONT_CARE_ABOUT_STANDARDS_DEFACTO_OR_OTHERWISE
				if(NET_ContainsHTML(cd->cwd_message_buffer,
									XP_STRLEN(cd->cwd_message_buffer)))
				  {
					/* See above. */
					char p1[] = "<PRE>";
					char p2[] = "</PRE>";
					ce->status = PUTSTRING(p2);
					if (ce->status > -1)
					  ce->status = PUTSTRING(cd->cwd_message_buffer);
					if (ce->status > -1)
					  ce->status = PUTSTRING(p1);
				  }
				else
#endif
				  {
                    XP_STRCPY (cd->output_buffer, "<PRE>");
					if (ce->status < 0) return ce->status;
					ce->status = PUTSTRING(cd->output_buffer);

					ce->status =
					  NET_ScanForURLs (cd->cwd_message_buffer,
									   XP_STRLEN(cd->cwd_message_buffer),
									   cd->output_buffer, OUTPUT_BUFFER_SIZE,
									   TRUE);
                    XP_STRCAT (cd->output_buffer, "</PRE>");
					if (ce->status < 0) return ce->status;
					ce->status = PUTSTRING(cd->output_buffer);
				  }
			  }
			if (ce->status > -1)
			  {
				XP_STRCPY(cd->output_buffer, "<HR>");
				ce->status = PUTSTRING(cd->output_buffer);
			  }
		  }
      }

#ifndef DEBUG_jwz
	/* allow the user to go up if cd->path is not empty
	 */	
	if(*cd->path != '\0')
	  {
		char *cp = XP_STRRCHR(cd->path, '/');
		PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "<A HREF=\"");
		if(ce->status > -1)
    		ce->status = PUTSTRING(cd->output_buffer);
		if(cp)
		  {
			*cp = '\0';
			if(ce->status > -1)
				ce->status = PUTSTRING(cd->path);
			*cp = '/';
			PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "/");
			if(ce->status > -1)
    			ce->status = PUTSTRING(cd->output_buffer);
		  }
		PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE,
		XP_GetString( XP_UPTO_HIGHER_LEVEL_DIRECTORY ) );
		if(ce->status > -1)
    		ce->status = PUTSTRING(cd->output_buffer);
	  }
#endif /* !DEBUG_jwz */

    /* clear the buffer so we can use it on the data sock */
    FREE_AND_CLEAR(cd->data_buf);
    cd->data_buf_size = 0;

    cd->next_state = FTP_READ_DIRECTORY;
    cd->pause_for_read = TRUE;

    /* clear the select on the control socket
     */
    FE_ClearReadSelect(ce->window_id, ce->socket);
    ce->socket = cd->dsock;
    FE_SetReadSelect(ce->window_id, ce->socket);

    return(0); /* continue */
}

PRIVATE int
net_ftp_read_dir(ActiveEntry * ce)
{
   
    char * line;
    NET_FileEntryInfo * entry_info;
    FTPConData * cd = (FTPConData *)ce->con_data;

    ce->status = NET_BufferedReadLine(cd->dsock, &line, &cd->data_buf, 
                    &cd->data_buf_size, &cd->pause_for_read);

    if(ce->status == 0)
      {
		ce->status = NET_PrintDirectory(&cd->sort_base, cd->stream, cd->path);

        /* go read the response since we need to
         * get the "226 sucessful transfer" message
         * out of the read queue in order to cache
         * connections properly
         */
        cd->next_state = FTP_WAIT_FOR_RESPONSE;
        cd->next_state_after_response = FTP_DONE;
    
        /* clear select on the data sock and set select on the control sock
         */
        if(cd->dsock != SOCKET_INVALID)
          {
            FE_ClearReadSelect(ce->window_id, cd->dsock);
            NETCLOSE(cd->dsock);
            cd->dsock = SOCKET_INVALID;
          }
    
        ce->socket = cd->cc->csock;
        FE_SetReadSelect(ce->window_id, cd->cc->csock);

        return(ce->status);
      }
    else if(ce->status < 0)
	  {
		NET_PrintDirectory(&cd->sort_base, cd->stream, cd->path);

		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, ce->status);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
	  }

    /* not exactly right but really close
	 */
    if(ce->status > 1)
      {
    	ce->bytes_received += ce->status;
		FE_GraphProgress(ce->window_id, ce->URL_s, ce->bytes_received, ce->status, ce->URL_s->content_length);
	  }

    if(!line)
        return(0); /* no line ready */

    TRACEMSG(("MKFTP: Line in %s is %s\n", cd->path, line));

    entry_info = net_parse_dir_entry(line, 
		 FE_UseFancyFTP(ce->window_id) ? cd->cc->server_type : FTP_GENERIC_TYPE);

    if(!entry_info)
	  {
	    ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY); 
    	return(MK_OUT_OF_MEMORY);
	  }

    if(entry_info->display)
      {
        TRACEMSG(("Adding file to sort list: %s\n", entry_info->filename));
        NET_SortAdd(cd->sort_base, (void *)entry_info);
      }
    else
      {
        NET_FreeEntryInfoStruct(entry_info);
      }

    return(ce->status);
}


/*
 * is_ls_date() --
 *      Return 1 if the ptr points to a string of the form:
 *              "Sep  1  1990 " or
 *              "Sep 11 11:59 " or
 *              "Dec 12 1989  " or
 *              "FCv 23 1990  " ...
 */
PRIVATE Bool 
net_is_ls_date (char *s)
{
    /* must start with three alpha characters */
    if (!isalpha(*s++) || !isalpha(*s++) || !isalpha(*s++))
        return FALSE;

    /* space */
    if (*s++ != ' ')
        return FALSE;

    /* space or digit */
    if ((*s != ' ') && !isdigit(*s))
        return FALSE;
    s++;

    /* digit */
    if (!isdigit(*s++))
        return FALSE;

    /* space */
    if (*s++ != ' ')
        return FALSE;

    /* space or digit */
    if ((*s != ' ') && !isdigit(*s))
        return FALSE;
    s++;

    /* digit */
    if (!isdigit(*s++))
        return FALSE;

    /* colon or digit */
    if ((*s != ':') && !isdigit(*s))
        return FALSE;
    s++;

    /* digit */
    if (!isdigit(*s++))
        return FALSE;

    /* space or digit */
    if ((*s != ' ') && !isdigit(*s))
        return FALSE;
    s++;

    /* space */
    if (*s++ != ' ')
        return FALSE;

    return TRUE;
} /* is_ls_date() */

/*       
 *  Converts a date string from 'ls -l' to a time_t number
 *
 *              "Sep  1  1990 " or
 *              "Sep 11 11:59 " or
 *              "Dec 12 1989  " or
 *              "FCv 23 1990  " ...
 *
 *  Returns 0 on error.
 */
PRIVATE time_t net_convert_unix_date (char * datestr)
{
    struct tm *time_info;         /* Points to static tm structure */
    char *bcol = datestr;         /* Column begin */
    char *ecol;                   /* Column end */
    long tval;
    int cnt;
    time_t curtime = time(NULL);

    if ((time_info = gmtime(&curtime)) == NULL) 
      {
    	return (time_t) 0;
      }

    time_info->tm_isdst = -1;                 /* Disable summer time */
    for (cnt=0; cnt<3; cnt++)                 /* Month */
    	*bcol++ = toupper(*bcol);

    if ((time_info->tm_mon = NET_MonthNo(datestr)) < 0)
    	return (time_t) 0;

    ecol = bcol;                        /* Day */
    while (*ecol++ == ' ') ;            /* Spool to other side of day */
    while (*ecol++ != ' ') ;
    *--ecol = '\0';

    time_info->tm_mday = atoi(bcol);
    time_info->tm_wday = 0;
    time_info->tm_yday = 0;
    bcol = ++ecol;                                   /* Year */
    if ((ecol = XP_STRCHR(bcol, ':')) == NULL) 
      {
    	time_info->tm_year = atoi(bcol)-1900;
    	time_info->tm_sec = 0;
    	time_info->tm_min = 0;
    	time_info->tm_hour = 0;
      } 
	else 
      {                                 /* Time */
    	/* If the time is given as hh:mm, then the file is less than 1 year
         *	old, but we might shift calandar year. This is avoided by checking
         *	if the date parsed is future or not. 
		 */
    	*ecol = '\0';
    	time_info->tm_sec = 0;
    	time_info->tm_min = atoi(++ecol);           /* Right side of ':' */
    	time_info->tm_hour = atoi(bcol);         /* Left side of ':' */
    	if (mktime(time_info) > curtime)
        	--time_info->tm_year;
      }
    return ((tval = mktime(time_info)) == -1 ? (time_t) 0 : tval);
}

/* a dos date/time string looks like this
 * 04-06-95  02:03PM 
 * 07-13-95  11:39AM 
 */
PRIVATE time_t 
net_parse_dos_date_time (char * datestr)
{
    struct tm *time_info;         /* Points to static tm structure */
    long tval;
    time_t curtime = time(NULL);

    if ((time_info = gmtime(&curtime)) == NULL) 
      {
    	return (time_t) 0;
      }

    time_info->tm_isdst = -1;                 /* Disable summer time */

    time_info->tm_mon = (datestr[1]-'0')-1;

    time_info->tm_mday = (((datestr[3]-'0')*10) + datestr[4]-'0');
    time_info->tm_year = (((datestr[6]-'0')*10) + datestr[7]-'0');
    time_info->tm_hour = (((datestr[10]-'0')*10) + datestr[11]-'0');
	if(datestr[15] == 'P')
    	time_info->tm_hour += 12;
    time_info->tm_min = (((datestr[13]-'0')*10) + datestr[14]-'0');

    time_info->tm_wday = 0;
    time_info->tm_yday = 0;
    time_info->tm_sec = 0;

    return ((tval = mktime(time_info)) == -1 ? (time_t) 0 : tval);
}

/*         
 *  Converts a date string from vms to a time_t number
 *  This is needed in order to put out the date using the same format
 *  for all directory listings.
 *
 *  Returns 0 on error
 */
PRIVATE time_t net_convert_vms_date (char * datestr)
{
    struct tm *time_info;           /* Points to static tm structure */
    char *col;
    long tval;
    time_t curtime = time(NULL);

    if ((time_info = gmtime(&curtime)) == NULL)
    	return (time_t) 0;

    time_info->tm_isdst = -1;                 /* Disable summer time */

    if ((col = XP_STRTOK(datestr, "-")) == NULL)
    	return (time_t) 0;

    time_info->tm_mday = atoi(col);                   /* Day */
    time_info->tm_wday = 0;
    time_info->tm_yday = 0;

    if ((col = XP_STRTOK(NULL, "-")) == NULL || (time_info->tm_mon = NET_MonthNo(col)) < 0)
    	return (time_t) 0;

    if ((col = XP_STRTOK(NULL, " ")) == NULL)               /* Year */
    	return (time_t) 0;

    time_info->tm_year = atoi(col)-1900;

    if ((col = XP_STRTOK(NULL, ":")) == NULL)               /* Hour */
    	return (time_t) 0;

    time_info->tm_hour = atoi(col);

    if ((col = XP_STRTOK(NULL, " ")) == NULL)               /* Mins */
    	return (time_t) 0;

    time_info->tm_min = atoi(col);
    time_info->tm_sec = 0;

    return ((tval = mktime(time_info)) < 0 ? (time_t) 0 : tval);
}


/*
 * parse_ls_line() --
 *      Extract the name, size, and date from an ls -l line.
 *
 * ls -l listing
 *
 * drwxr-xr-x    2 montulli eng          512 Nov  8 23:23 CVS
 * -rw-r--r--    1 montulli eng         2244 Nov  8 23:23 Imakefile
 * -rw-r--r--    1 montulli eng        14615 Nov  9 17:03 Makefile
 *
 * or a dl listing
 *
 * cs.news/              =  ICSI Computing Systems Newsletter
 * dash/                 -  DASH group documents and software
 * elisp/                -  Emacs lisp code
 * 
 */
PRIVATE void 
net_parse_ls_line (char *line, NET_FileEntryInfo *entry_info)
{
    int32   base=1;
	int32   size_num=0;
	char    save_char;
	char   *ptr;

    for (ptr = &line[XP_STRLEN(line) - 1];
            (ptr > line+13) && (!XP_IS_SPACE(*ptr) || !net_is_ls_date(ptr-12)); ptr--)
                ; /* null body */
	save_char = *ptr;
    *ptr = '\0';
    if (ptr > line+13) 
	  {
        entry_info->date = net_convert_unix_date(ptr-12);
	  }
	else
	  {
	    /* must be a dl listing
	     */
		/* unterminate the line */
		*ptr = save_char;
		/* find the first whitespace and  terminate
		 */
		for(ptr=line; *ptr != '\0'; ptr++)
			if(XP_IS_SPACE(*ptr))
			  {
				*ptr = '\0';
				break;
			  }
        entry_info->filename = NET_Escape(line, URL_PATH);
	
		return;
	  }

    /* escape and copy
     */
    entry_info->filename = NET_Escape(ptr+1, URL_PATH);

	/* parse size
	 */
	if(ptr > line+15)
	  {
        ptr -= 14;
        while (isdigit(*ptr))
          {
            size_num += ((int32) (*ptr - '0')) * base;
            base *= 10;
            ptr--;
          }
    
        entry_info->size = size_num;
	  }
    
} /* parse_ls_line() */

/*
 * net_parse_vms_dir_entry()
 *      Format the name, date, and size from a VMS LIST line
 *      into the EntryInfo structure
 */
PRIVATE void 
net_parse_vms_dir_entry (char *line, NET_FileEntryInfo *entry_info)
{
        int i, j;
        int32 ialloc;
        char *cp, *cpd, *cps, date[64], *sp = " ";
        time_t NowTime;
        static char ThisYear[32];
        static Bool HaveYear = FALSE; 

        /**  Get rid of blank lines, and information lines.  **/
        /**  Valid lines have the ';' version number token.  **/
        if (!XP_STRLEN(line) || (cp=XP_STRCHR(line, ';')) == NULL) 
		  {
            entry_info->display = FALSE;
            return;
          }

        /** Cut out file or directory name at VMS version number. **/
    	*cp++ ='\0';
		/* escape and copy
	 	 */
		entry_info->filename = NET_Escape(line, URL_PATH);

        /** Cast VMS file and directory names to lowercase. **/
    	for (i=0; entry_info->filename[i]; i++)
            entry_info->filename[i] = tolower(entry_info->filename[i]);

        /** Uppercase terminal .z's or _z's. **/
    	if ((--i > 2) && entry_info->filename[i] == 'z' &&
             	(entry_info->filename[i-1] == '.' || entry_info->filename[i-1] == '_'))
            entry_info->filename[i] = 'Z';

        /** Convert any tabs in rest of line to spaces. **/
    	cps = cp-1;
        while ((cps=XP_STRCHR(cps+1, '\t')) != NULL)
            *cps = ' ';

        /** Collapse serial spaces. **/
        i = 0; j = 1;
    	cps = cp;
        while (cps[j] != '\0') 
		  {
            if (cps[i] == ' ' && cps[j] == ' ')
                j++;
            else
                cps[++i] = cps[j++];
		  }
        cps[++i] = '\0';

        /* Save the current year.       
    	 * It could be wrong on New Year's Eve.
		 */
    	if (!HaveYear) 
	 	  {
        	NowTime = time(NULL);
        	XP_STRCPY(ThisYear, (char *)ctime(&NowTime)+20);
        	ThisYear[4] = '\0';
        	HaveYear = TRUE;
    	  }

        /* get the date. 
		 */
        if ((cpd=XP_STRCHR(cp, '-')) != NULL &&
                XP_STRLEN(cpd) > 9 && isdigit(*(cpd-1)) &&
                isalpha(*(cpd+1)) && *(cpd+4) == '-') 
		  {

            /* Month 
			 */
            *(cpd+4) = '\0';
            *(cpd+2) = tolower(*(cpd+2));
            *(cpd+3) = tolower(*(cpd+3));
            XP_SPRINTF(date, "%.32s ", cpd+1);
            *(cpd+4) = '-';

            /** Day **/
            *cpd = '\0';
            if (isdigit(*(cpd-2)))
                XP_SPRINTF(date+4, "%.32s ", cpd-2);
            else
                XP_SPRINTF(date+4, " %.32s ", cpd-1);
            *cpd = '-';

            /** Time or Year **/
            if (!XP_STRNCMP(ThisYear, cpd+5, 4) && XP_STRLEN(cpd) > 15 && *(cpd+12) == ':')
			  {
                *(cpd+15) = '\0';
                XP_SPRINTF(date+7, "%.32s", cpd+10);
                *(cpd+15) = ' ';
              } 
			else 
			  {
                *(cpd+9) = '\0';
                XP_SPRINTF(date+7, " %.32s", cpd+5);
                *(cpd+9) = ' ';
              }

            entry_info->date = net_convert_vms_date(date);
          }

        /* get the size 
		 */
        if ((cpd=XP_STRCHR(cp, '/')) != NULL) 
		  {
            /* Appears be in used/allocated format */
            cps = cpd;
            while (isdigit(*(cps-1)))
                cps--;
            if (cps < cpd)
                *cpd = '\0';
            entry_info->size = atol(cps);
            cps = cpd+1;
            while (isdigit(*cps))
                cps++;
            *cps = '\0';
            ialloc = atoi(cpd+1);
            /* Check if used is in blocks or bytes */
            if (entry_info->size <= ialloc)
                entry_info->size *= 512;
          }
        else if ((cps=XP_STRTOK(cp, sp)) != NULL) 
		  {
            /* We just initialized on the version number 
             * Now let's find a lone, size number   
			 */
            while ((cps=XP_STRTOK(NULL, sp)) != NULL) 
			  {
                 cpd = cps;
                 while (isdigit(*cpd))
                     cpd++;
                 if (*cpd == '\0') 
				   {
                     /* Assume it's blocks */
                     entry_info->size = atol(cps) * 512;
                     break;
                   }
               }
          }

        /** Wrap it up **/
        TRACEMSG(("MKFTP: VMS filename: %s  date: %d  size: %ld\n",
                         entry_info->filename, entry_info->date, entry_info->size));
        return;

} /* net_parse_vms_dir_entry() */

/*
 *     parse_dir_entry() 
 *      Given a line of LIST/NLST output in entry, return results 
 *      and a file/dir name in entry_info struct
 *
 */
PRIVATE NET_FileEntryInfo * 
net_parse_dir_entry (char *entry, int server_type)
{
        NET_FileEntryInfo *entry_info;
        int  i;
        int  len;
        Bool remove_size=FALSE;

        entry_info = NET_CreateFileEntryInfoStruct();
        if(!entry_info)
            return(NULL);

        entry_info->display = TRUE;

		/* do special case for ambiguous NT servers
		 *
		 * ftp.microsoft.com is an NT server with UNIX ls -l
		 * syntax.  But most NT servers use the DOS dir syntax.
		 * If there is a space at position 8 then it's a DOS dir syntax
		 */
		if(server_type == FTP_NT_TYPE && !XP_IS_SPACE(entry[8]))
			server_type = FTP_UNIX_TYPE;

        switch (server_type)
        {
            case FTP_UNIX_TYPE:
            case FTP_PETER_LEWIS_TYPE:
            case FTP_MACHTEN_TYPE:
				/* interpret and edit LIST output from Unix server */

                if(!XP_STRNCMP(entry, "total ", 6)  
						|| (XP_STRSTR(entry, "Permission denied") != NULL)
						    || 	(XP_STRSTR(entry, "not available") != NULL))
                  {
                    entry_info->display=FALSE;
                    return(entry_info);
                  }
 
                len = XP_STRLEN(entry);

                /* check first character of ls -l output */
                if (toupper(entry[0]) == 'D') 
                  {
                    /* it's a directory */
				    entry_info->special_type = NET_DIRECTORY;
                    remove_size=TRUE; /* size is not useful */
                  }
                else if (entry[0] == 'l')
                  {
                    /* it's a symbolic link, does the user care about
                     * knowing if it is symbolic?  I think so since
                     * it might be a directory
                     */
					entry_info->special_type = NET_SYM_LINK;
                    remove_size=TRUE; /* size is not useful */

                    /* strip off " -> pathname" */
                    for (i = len - 1; (i > 3) && (!XP_IS_SPACE(entry[i])
                    || (entry[i-1] != '>') 
                    || (entry[i-2] != '-')
                    || (entry[i-3] != ' ')); i--)
                             ; /* null body */
                    if (i > 3)
                      {
                        entry[i-3] = '\0';
                        len = i - 3;
                      }
                  } /* link */

                net_parse_ls_line(entry, entry_info); 

                if(!XP_STRCMP(entry_info->filename,"..") || !XP_STRCMP(entry_info->filename,"."))
            		entry_info->display=FALSE;

				/* add a trailing slash to all directories */
				if(entry_info->special_type == NET_DIRECTORY)
				    StrAllocCat(entry_info->filename, "/");

                /* goto the bottom and get real type */
                break;

            case FTP_VMS_TYPE:
                /* Interpret and edit LIST output from VMS server */
                /* and convert information lines to zero length.  */
                net_parse_vms_dir_entry(entry, entry_info);

                /* Get rid of any junk lines */
                if(!entry_info->display)
                    return(entry_info);

                /** Trim off VMS directory extensions **/
                len = XP_STRLEN(entry_info->filename);
                if ((len > 4) && !XP_STRCMP(&entry_info->filename[len-4], ".dir"))
                  {
                    entry_info->filename[len-4] = '\0';
				    entry_info->special_type = NET_DIRECTORY;
                    remove_size=TRUE; /* size is not useful */
					/* add trailing slash to directories
					 */
					StrAllocCat(entry_info->filename, "/");
                  }
                /* goto the bottom and get real type */
                break;

            case FTP_CMS_TYPE:
                /* can't be directory... */
                /*
                 * "entry" already equals the correct filename
                 */
		        /* escape and copy
	 	         */
		        entry_info->filename = NET_Escape(entry, URL_PATH);
                /* goto the bottom and get real type */
                break;

            case FTP_NCSA_TYPE:
            case FTP_TCPC_TYPE:
                /* directories identified by trailing "/" characters */
		        /* escape and copy
	 	         */
		        entry_info->filename = NET_Escape(entry, URL_PATH);
                len = XP_STRLEN(entry);
                if (entry[len-1] == '/')
                  {
				    entry_info->special_type = NET_DIRECTORY;
                    remove_size=TRUE; /* size is not useful */
                  }
                  /* goto the bottom and get real type */
                break;

			case FTP_NT_TYPE:
				/* windows NT DOS dir syntax.
				 * looks like:  
				 *            1         2         3         4         5
				 *  012345678901234567890123456789012345678901234567890
				 *  06-29-95  03:05PM       <DIR>          muntemp
				 *  05-02-95  10:03AM               961590 naxp11e.zip
				 *
				 *  The date time directory indicator and filename
				 *  are always in a fixed position.  The file
			 	 *  size always ends at position 37.
				 */
				  {
					char *date, *size_s, *name;
		
					if(XP_STRLEN(entry) > 37)
					  {
					    date = entry;
					    entry[17] = '\0';
					    size_s = &entry[18];
					    entry[38] = '\0';
					    name = &entry[39];
    
					    if(XP_STRSTR(size_s, "<DIR>"))
						    entry_info->special_type = NET_DIRECTORY;
					    else
						    entry_info->size = atol(XP_StripLine(size_s));
    
					    entry_info->date = net_parse_dos_date_time(date);

					    StrAllocCopy(entry_info->filename, name);
					  }
					else
					  {
					    StrAllocCopy(entry_info->filename, entry);
					  }
				  }
				break;
    
            default:
                /* we cant tell if it is a directory since we only
                 * did an NLST   
                 */
		        /* escape and copy
	 	         */
		        entry_info->filename = NET_Escape(entry, URL_PATH);
                return(entry_info); /* mostly empty info */
                /* break; Not needed */

        } /* switch (server_type) */


    if(remove_size && entry_info->size)
      {
        entry_info->size = 0;
      }

    /* get real types eventually */
    if(!entry_info->special_type) 
      {
        entry_info->cinfo = NET_cinfo_find_type(entry_info->filename);
      }

    return(entry_info);

} /* net_parse_dir_entry */

PRIVATE int
net_get_ftp_password(ActiveEntry *ce)
{
    FTPConData *cd = (FTPConData *) ce->con_data;

	if(!cd->password)
	  {
        char * host_string = NET_ParseURL(ce->URL_s->address, GET_HOST_PART);

#ifndef MCC_PROXY
		if(ftp_last_password 
				&& ftp_last_password_host 
					&& !XP_STRCMP(ftp_last_password_host, host_string)) 
		  {
			StrAllocCopy(cd->password, ftp_last_password);
		  }
		else
#endif
	      {
            PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE,
						XP_GetString(XP_PROMPT_ENTER_PASSWORD),
						host_string);
            cd->password = (char *)FE_PromptPassword(ce->window_id,
													 cd->output_buffer);

            if(!cd->password)
			  {
				FREE(host_string);
                return(MK_INTERRUPTED);  /* user canceled */
			  }

			StrAllocCopy(ftp_last_password_host, host_string);
			StrAllocCopy(ftp_last_password, cd->password);
		  }

		FREE(host_string);
	  }

	cd->next_state = FTP_SEND_PASSWORD;

    return 0;
}

PRIVATE int
net_get_ftp_password_response(FTPConData *cd)
{
	/* not used yet
	 */
    return 0; /* #### what should return value be? */
}

/* the load initializeation routine for FTP.
 *
 * this sets up all the data structures and begin's the
 * connection by calling processFTP
 */
MODULE_PRIVATE int
NET_FTPLoad (ActiveEntry * ce)
{
    char * host_string = NET_ParseURL(ce->URL_s->address, GET_HOST_PART);
    char * at_sign = XP_STRCHR(host_string, '@');    /* user? */
    char * filename;
    char * semi_colon;
    FTPConData * cd = XP_NEW(FTPConData);

    if(!cd)
	  {
	    ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY); 
		ce->status = MK_OUT_OF_MEMORY;
        return(MK_OUT_OF_MEMORY);  
	  }

    ce->con_data = (FTPConData *)cd;

    /* init the connection data struct 
     */
    XP_MEMSET(cd, 0, sizeof(FTPConData));
    cd->dsock         = SOCKET_INVALID;
    cd->listen_sock   = SOCKET_INVALID;
    cd->pasv_mode     = TRUE;
    cd->is_directory  = FALSE;
    cd->next_state_after_response = FTP_ERROR_DONE;
    cd->cont_response     = -1;

	/* @@@@ hope we never need a buffer larger than this 
	 */
	cd->output_buffer = (char *) XP_ALLOC(OUTPUT_BUFFER_SIZE);
	if(!cd->output_buffer)
	  {
		FREE(cd);
	    ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY); 
		ce->status = MK_OUT_OF_MEMORY;
        return(MK_OUT_OF_MEMORY);  
	  }

    /* check for username */
    if (at_sign) 
      {
        char *username = host_string;
        char *password;
        
        *at_sign=0;             /* terminate */

        password = XP_STRCHR(username, ':');
        if (password)
          {
            *password++ = 0; /* terminate username and adv pw */
            StrAllocCopy(cd->password, password);
          }

        /* copy the username */
        StrAllocCopy(cd->username, username);

		/* set it back so we can use it later
		 */
		*at_sign = '@';
		if(password)
			*(password-1) = ':';
      } 
	else 
	  { 
		const char * user = NULL;

		StrAllocCopy(cd->password, "mozilla@");

#ifdef MOZILLA_CLIENT
		if(net_send_email_address_as_password)
			user = FE_UsersMailAddress();
#endif

		if(user && *user)
		  {
		    StrAllocCopy(cd->password, user);

			/* make sure it has an @ sign in it or else the ftp
			 * server won't like it
			 */
			if(!XP_STRCHR(cd->password, '@'))
				StrAllocCat(cd->password, "@");
		  }
		else
	      {
			StrAllocCopy(cd->password, "mozilla@");
		  }
	  }
 

    /* get the path */
    cd->path = NET_ParseURL(ce->URL_s->address, GET_PATH_PART);

    if(!cd->path)
      {
        FREE(ce->con_data);
        FREE(cd->output_buffer);
	    ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY); 
		ce->status = MK_OUT_OF_MEMORY;
        return(MK_OUT_OF_MEMORY);
      }

	NET_UnEscape(cd->path);

	if(*cd->path == '\0')
	  {
		TRACEMSG(("Found ftp url with NO PATH"));
		
		cd->use_default_path = TRUE;
	  }

	/* if the path begins with /./ 
	 * use pwd to get the sub path
	 * and append the end to it
	 */
	if(!XP_STRNCMP(cd->path, "/./", 3) || !XP_STRCMP(cd->path, "/."))
	  {
		char * tmp = cd->path;

		cd->use_default_path = TRUE;

		/* skip the "/." and leave a slash at the beginning */
		cd->path = XP_STRDUP(cd->path+2);
		XP_FREE(tmp);

		if(!cd->path)
		  {
			FREE(ce->con_data);
        	FREE(cd->output_buffer);
	    	ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY); 
			ce->status = MK_OUT_OF_MEMORY;
			return(MK_OUT_OF_MEMORY);
		  }
	  }

    /* set the default type for use later
     */
    cd->type = FTP_MODE_UNKNOWN;

    /* look for the extra type information at the end per
     * the RFC 1630.  It will be delimited by a ';'
     */
    if((semi_colon = XP_STRRCHR(cd->path, ';')) != NULL)
      {
        /* just switch on the character at the end of this whole
         * thing since it must be the type to use
         */
        switch(semi_colon[XP_STRLEN(semi_colon)-1])
          {
            case 'a':
            case 'A':
                cd->type = FTP_MODE_ASCII;
				TRACEMSG(("Explicitly setting type ASCII"));
                break;

            case 'i':
            case 'I':
                cd->type = FTP_MODE_BINARY;
				TRACEMSG(("Explicitly setting type BINARY"));
                break;
          }

        /* chop off the bits after the semi colon
         */
        *semi_colon = '\0';
      }

    /* find the filename in the path */
    filename = XP_STRRCHR(cd->path, '/');
    if(!filename)
        filename = cd->path;
	else
		filename++;  /* go past slash */

    cd->filename = 0;
    StrAllocCopy(cd->filename, filename);

    /* Try and find an open connection to this 
     * server that is not currently being used
     */
    if(connection_list)
      {
        FTPConnection * tmp_con;
        XP_List * list_entry = connection_list;

        while((tmp_con = (FTPConnection *)XP_ListNextObject(list_entry)) != NULL)
          {
            /* if the hostnames match up exactly and the connection
             * is not busy at the moment then reuse this connection.
             */
            if(!tmp_con->busy && !XP_STRCMP(tmp_con->hostname, host_string))
              {
                cd->cc = tmp_con;
                ce->socket = cd->cc->csock;  /* set select on the control socket */
    			FE_SetReadSelect(ce->window_id, cd->cc->csock);
                cd->cc->prev_cache = TRUE;  /* this was from the cache */
                break;
              }
          }
      }
    else
      {
    /* initialize the connection list 
     */
        connection_list = XP_ListNew();
      }
    
    if(!cd->cc)
      {

		TRACEMSG(("cached connection not found making new FTP connection"));

        /* build a control connection structure so we
         * can store the data as we go along
         */
        cd->cc = XP_NEW(FTPConnection);
        if(!cd->cc)
          {
            XP_FREE(host_string);  /* free from way up above */
	        ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY); 
            return(MK_OUT_OF_MEMORY);
          }

        cd->cc->hostname = 0;
        StrAllocCopy(cd->cc->hostname, host_string);
        /* set the mode of the control connection unknown
         */
        cd->cc->cur_mode = FTP_MODE_UNKNOWN;
		cd->cc->csock = SOCKET_INVALID;
		cd->cc->server_type = FTP_GENERIC_TYPE;
		cd->cc->use_list = FALSE;
		cd->cc->no_pasv  = FALSE;

        cd->cc->prev_cache = FALSE;  /* this wasn't from the cache */

        /* add this structure to the connection list even
         * though it's not really valid yet.
         * we will fill it in as we go and if
         * an error occurs will will remove it from the
         * list.  No one else will be able to use it since
         * we will mark it busy.
         */
        XP_ListAddObjectToEnd(connection_list, cd->cc);

        /* set this connection busy so no one else can use it
         */
        cd->cc->busy = TRUE;

		/* gate the maximum number of cached connections
		 * to one
		 */
		if(XP_ListCount(connection_list) > 1)
		  {
            XP_List * list_ptr = connection_list->next;
            FTPConnection * con;

            while(list_ptr)
              {
                con = (FTPConnection *) list_ptr->object;
                list_ptr = list_ptr->next;
                if(!con->busy)
                  {
                    XP_ListRemoveObject(connection_list, con);
                    NETCLOSE(con->csock);
					/* dont reduce the number of
					 * open connections since cached ones
					 * dont count as active
					 */
                    FREEIF(con->hostname);
                    XP_FREE(con);
                    break;
                  }
              }
		  }

        cd->next_state = FTP_CONTROL_CONNECT;
      }
    else
      {
		TRACEMSG(("Found cached FTP connection! YES!!! "));

    	/* don't send PASV if the server cant do it */
    	if(cd->cc->no_pasv || !net_use_pasv)
            cd->next_state = FTP_SEND_PORT;
        else
            cd->next_state = FTP_SEND_PASV;

        /* set this connection busy so no one else can use it
         */
        cd->cc->busy = TRUE;
		/* add this connection to the number that is open
		 * since it is now active
		 */
		NET_TotalNumberOfOpenConnections++;
      }

    XP_FREE(host_string);  /* free from way up above */

    return(NET_ProcessFTP(ce));
} 


/* the main state machine control routine.  Calls
 * the individual state processors
 * 
 * returns -1 when all done or 0 or positive all other times
 */
MODULE_PRIVATE int
NET_ProcessFTP(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    cd->pause_for_read = FALSE;

    while(!cd->pause_for_read)
      {
        TRACEMSG(("In ProcessFTP switch loop with state: %d \n", cd->next_state));

        switch(cd->next_state) {

          case FTP_WAIT_FOR_RESPONSE:
            ce->status = net_ftp_response(ce);
            break;
    
          /* begin login states */
          case FTP_CONTROL_CONNECT:
            ce->status = NET_BeginConnect(ce->URL_s->address,
										  ce->URL_s->IPAddressString,
										 "FTP",
                               			 FTP_PORT,
										 &cd->cc->csock, 
										 FALSE, 
										 &cd->tcp_con_data, 
										 ce->window_id,
										 &ce->URL_s->error_msg,
										  ce->socks_host,
										  ce->socks_port);
            ce->socket = cd->cc->csock;

			if(cd->cc->csock != SOCKET_INVALID)
				NET_TotalNumberOfOpenConnections++;
    
            cd->pause_for_read = TRUE;
    
            if(ce->status == MK_CONNECTED)
              {
                cd->next_state = FTP_WAIT_FOR_RESPONSE;
                cd->next_state_after_response = FTP_SEND_USERNAME;
			    FE_SetReadSelect(ce->window_id, cd->cc->csock);
              }
            else if(ce->status > -1)
              {
                cd->next_state = FTP_CONTROL_CONNECT_WAIT;
                ce->con_sock = cd->cc->csock;  /* set con sock so we can select on it */
                FE_SetConnectSelect(ce->window_id, ce->con_sock);
    
              }
            break;
    
          case FTP_CONTROL_CONNECT_WAIT:
            ce->status = NET_FinishConnect(ce->URL_s->address,
										  "FTP",
                            	  		  FTP_PORT,
										  &cd->cc->csock, 
										  &cd->tcp_con_data, 
										  ce->window_id,
										  &ce->URL_s->error_msg);
    
            cd->pause_for_read = TRUE;
    
            if(ce->status == MK_CONNECTED)
              {
                cd->next_state = FTP_WAIT_FOR_RESPONSE;
                cd->next_state_after_response = FTP_SEND_USERNAME;
                FE_ClearConnectSelect(ce->window_id, ce->con_sock);
                ce->con_sock = SOCKET_INVALID;  /* reset con sock so we don't select on it */
			    FE_SetReadSelect(ce->window_id, ce->socket);
              }
			else
			  {
				/* the not yet connected state */

        		/* unregister the old CE_SOCK from the select list
         		 * and register the new value in the case that it changes
         		 */
				if(ce->con_sock != cd->cc->csock)
				  {
            		FE_ClearConnectSelect(ce->window_id, ce->con_sock);
					ce->con_sock = cd->cc->csock;
            		FE_SetConnectSelect(ce->window_id, ce->con_sock);
				  }
			  }
            break;

          case FTP_SEND_USERNAME:
            ce->status = net_send_username((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_USERNAME_RESPONSE:
            ce->status = net_send_username_response(ce);
            break;
    
		  case FTP_GET_PASSWORD:
            ce->status = net_get_ftp_password(ce);
            break;

		  case FTP_GET_PASSWORD_RESPONSE:
			/* currently skipped 
			 */
            ce->status = net_get_ftp_password_response((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_PASSWORD:
            ce->status = net_send_password((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_PASSWORD_RESPONSE:
            ce->status = net_send_password_response(ce);
            break;
        
          case FTP_SEND_ACCT:
            ce->status = net_send_acct((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_ACCT_RESPONSE:
            ce->status = net_send_acct_response(ce);
            break;
    
          case FTP_SEND_SYST:
            ce->status = net_send_syst((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_SYST_RESPONSE:
            ce->status = net_send_syst_response((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_MAC_BIN:
            ce->status = net_send_mac_bin((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_MAC_BIN_RESPONSE:
            ce->status = net_send_mac_bin_response((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_PWD:
            ce->status = net_send_pwd((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_PWD_RESPONSE:
            ce->status = net_send_pwd_response(ce);
            break;
        /* end login states */

		  case FTP_FIGURE_OUT_WHAT_TO_DO:
            ce->status = net_figure_out_what_to_do(ce);
			break;

		  case FTP_SEND_MKDIR:
            ce->status = net_send_mkdir(ce);
			break;

		  case FTP_SEND_MKDIR_RESPONSE:
            ce->status = net_send_mkdir_response(ce);
			break;
    
		  case FTP_SEND_DELETE_FILE:
            ce->status = net_send_delete_file(ce);
			break;

		  case FTP_SEND_DELETE_FILE_RESPONSE:
            ce->status = net_send_delete_file_response(ce);
			break;
    
		  case FTP_SEND_DELETE_DIR:
            ce->status = net_send_delete_dir(ce);
			break;

		  case FTP_SEND_DELETE_DIR_RESPONSE:
            ce->status = net_send_delete_dir_response(ce);
			break;

          case FTP_SEND_PASV:
            ce->status = net_send_pasv((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_PASV_RESPONSE:
            ce->status = net_send_pasv_response((FTPConData *)ce->con_data);
            break;
    
          case FTP_PASV_DATA_CONNECT:
            ce->status = NET_BeginConnect(cd->data_con_address, 
										  NULL,
										 "FTP Data Connection",
                       					FTP_PORT, 
										 &cd->dsock, 
										 FALSE, 
										 &cd->tcp_con_data, 
										 ce->window_id,
										 &ce->URL_s->error_msg,
										  ce->socks_host,
										  ce->socks_port);
    
            if(ce->status == MK_CONNECTED)
              {
                cd->next_state = FTP_GET_FILE_TYPE;
              }
            else if(ce->status > -1)
              {
                cd->pause_for_read = TRUE;
                cd->next_state = FTP_PASV_DATA_CONNECT_WAIT;
                ce->con_sock = cd->dsock;  /* set con sock so we can select on it */
                FE_SetConnectSelect(ce->window_id, ce->con_sock);
              }
            break;

          case FTP_PASV_DATA_CONNECT_WAIT:
                ce->status = NET_FinishConnect(cd->data_con_address, 
											  "FTP Data Connection",
                                			  FTP_PORT,
											  &cd->dsock,
											  &cd->tcp_con_data,
											  ce->window_id,
											  &ce->URL_s->error_msg);
                TRACEMSG(("FTP: got pasv data connection on port #%d\n",cd->cc->csock));
    
                if(ce->status == MK_CONNECTED)
                  {
                    cd->next_state = FTP_GET_FILE_TYPE;
                    FE_ClearConnectSelect(ce->window_id, ce->con_sock);
                    ce->con_sock = SOCKET_INVALID;  /* reset con sock so we don't select on it */
                  }
                else
                  {
					ce->con_sock = cd->dsock; /* it might change */
                    cd->pause_for_read = TRUE;
                  }
            break;
    
          case FTP_SEND_PORT:
            ce->status = net_send_port(ce);
            break;
    
          case FTP_SEND_PORT_RESPONSE:
            ce->status = net_send_port_response(ce);
            break;
    
		  /* This is the state that gets called after
		   * login and port/pasv are done
		   */
          case FTP_GET_FILE_TYPE:
            ce->status = net_get_ftp_file_type(ce);
            break;
    
          case FTP_SEND_BIN:
            ce->status = net_send_bin((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_ASCII:
            ce->status = net_send_ascii((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_BIN_RESPONSE:
          case FTP_SEND_ASCII_RESPONSE:
            ce->status = net_send_bin_or_ascii_response(ce);
            break;

		  case FTP_GET_FILE_SIZE:
            ce->status = net_get_ftp_file_size(ce);
            break;

		  case FTP_GET_FILE_SIZE_RESPONSE:
            ce->status = net_get_ftp_file_size_response(ce);
            break;
    
          case FTP_GET_FILE:
            ce->status = net_get_ftp_file((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_RETR:
            ce->status = net_send_retr((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_RETR_RESPONSE:
            ce->status = net_send_retr_response(ce);
            break;
    
          case FTP_SEND_CWD:
            ce->status = net_send_cwd((FTPConData *)ce->con_data);
            break;
    
          case FTP_SEND_CWD_RESPONSE:
            ce->status = net_send_cwd_response(ce);
            break;
    
          case FTP_BEGIN_PUT:
            ce->status = net_begin_put(ce);
            break;
    
          case FTP_BEGIN_PUT_RESPONSE:
            ce->status = net_begin_put_response(ce);
            break;
    
          case FTP_DO_PUT:
            ce->status = net_do_put(ce);
            break;
    
          case FTP_SEND_LIST_OR_NLST:
            ce->status = net_send_list_or_nlst(ce);
            break;
    
          case FTP_SEND_LIST_OR_NLST_RESPONSE:
            ce->status = net_send_list_or_nlst_response(ce);
            break;

          case FTP_SETUP_STREAM:
            ce->status = net_setup_ftp_stream(ce);
            break;
    
          case FTP_WAIT_FOR_ACCEPT:
            ce->status = net_wait_for_ftp_accept(ce);
            break;
    
          case FTP_START_READ_FILE:
            ce->status = net_start_ftp_read_file(ce);
            break;
    
          case FTP_READ_FILE:
            ce->status = net_ftp_read_file(ce);
            break;
    
          case FTP_START_READ_DIRECTORY:
            ce->status = net_start_ftp_read_dir(ce);
            break;
    
          case FTP_READ_DIRECTORY:
    
            ce->status = net_ftp_read_dir(ce);
            break;

          case FTP_DONE:
			if(cd->stream)
            	COMPLETE_STREAM;

    	    /* don't close the control sock since we need 
			 * it for connection caching
			 */
            if(cd->dsock != SOCKET_INVALID)
			  {
			    FE_ClearReadSelect(ce->window_id, cd->dsock);
				TRACEMSG(("Closing and clearing sock cd->dsock: %d", cd->dsock));
				NETCLOSE(cd->dsock);
				cd->dsock = SOCKET_INVALID;
			  }

            if(cd->listen_sock != SOCKET_INVALID)
              {
				TRACEMSG(("Closing and clearing sock cd->listen_sock: %d", cd->listen_sock));
			    FE_ClearReadSelect(ce->window_id, cd->listen_sock);
				NETCLOSE(cd->listen_sock);
				/* not counted as a connection */
			  }
				
            cd->next_state = FTP_FREE_DATA;
    
            cd->cc->busy = FALSE;  /* we are no longer using the connection */
			FE_ClearReadSelect(ce->window_id, cd->cc->csock);

			/* we can't reuse VMS control connections since they are screwed up
		   	 */
			if(cd->cc->server_type == FTP_VMS_TYPE)
			  {
	            /* remove the connection from the cache list
                 * and free the data
                 */
                XP_ListRemoveObject(connection_list, cd->cc);
                FREEIF(cd->cc->hostname);
                if(cd->cc->csock != SOCKET_INVALID)
                  {
				    TRACEMSG(("Closing and clearing sock cd->cc->csock: %d", cd->cc->csock));
                    NETCLOSE(cd->cc->csock);
					cd->cc->csock = SOCKET_INVALID;
                  }
                XP_FREE(cd->cc);
                cd->cc = 0;
			  }

			/* remove the cached connection from the total number
			 * counted, since it is no longer active
			 */
			NET_TotalNumberOfOpenConnections--;

            break;
    
          case FTP_ERROR_DONE:
            if(cd->sort_base)
              {
                 /* we have to do this to get it free'd
                  */
                 NET_PrintDirectory(&cd->sort_base, cd->stream, cd->path);
              }
    
            if(cd->stream)
                ABORT_STREAM(ce->status);
    
            if(cd->dsock != SOCKET_INVALID)
              {
			    FE_ClearConnectSelect(ce->window_id, cd->dsock);
			    FE_ClearReadSelect(ce->window_id, cd->dsock);
#if defined(XP_WIN) || defined(XP_UNIX)
                FE_ClearDNSSelect(ce->window_id, cd->dsock);
#endif /* XP_WIN || XP_UNIX */

#ifdef XP_WIN
				if(cd->calling_netlib_all_the_time)
				{
					net_call_all_the_time_count--;
					cd->calling_netlib_all_the_time = FALSE;
					if(net_call_all_the_time_count == 0)
						FE_ClearCallNetlibAllTheTime(ce->window_id);
				}
#endif /* XP_WIN */

				TRACEMSG(("Closing and clearing sock cd->dsock: %d", cd->dsock));
                NETCLOSE(cd->dsock);
                cd->dsock = SOCKET_INVALID;
              }

			if(cd->file_to_post_fp)
			  {
				XP_FileClose(cd->file_to_post_fp);
				cd->file_to_post_fp = 0;
			  }
    
			if(cd->cc->csock != SOCKET_INVALID)
			  {
			    FE_ClearConnectSelect(ce->window_id, cd->cc->csock);
			    FE_ClearReadSelect(ce->window_id, cd->cc->csock);
#if defined(XP_WIN) || defined(XP_UNIX)
                FE_ClearDNSSelect(ce->window_id, cd->cc->csock);
#endif /* XP_WIN || XP_UNIX */
				TRACEMSG(("Closing and clearing sock cd->cc->csock: %d", cd->cc->csock));
                NETCLOSE(cd->cc->csock);
				cd->cc->csock = SOCKET_INVALID;
				NET_TotalNumberOfOpenConnections--;
			  }

            if(cd->listen_sock != SOCKET_INVALID)
              {
				TRACEMSG(("Closing and clearing sock cd->listen_sock: %d", cd->listen_sock));
				FE_ClearReadSelect(ce->window_id, cd->listen_sock);
				NETCLOSE(cd->listen_sock);
				/* not counted as a connection */
			  }


            /* check if this connection came from the cache or if it was
             * a new connection.  If it was new lets start it over again
             */
            if(cd->cc->prev_cache 
				&& ce->status != MK_INTERRUPTED
				  && ce->status != MK_UNABLE_TO_CONVERT
					&& ce->status != MK_OUT_OF_MEMORY)
              {
                cd->cc->prev_cache = NO;
                cd->next_state = FTP_CONTROL_CONNECT;
				cd->pasv_mode = TRUE;  /* until we learn otherwise */
            	cd->pause_for_read = FALSE;
                ce->status = 0;  /* keep going */
            	cd->cc->cur_mode = FTP_MODE_UNKNOWN;
              }
            else
              {
                cd->next_state = FTP_FREE_DATA;
        
                /* remove the connection from the cache list
                 * and free the data
                 */
                XP_ListRemoveObject(connection_list, cd->cc);
                FREEIF(cd->cc->hostname);
                XP_FREE(cd->cc);
              }
    
            break;
    
          case FTP_FREE_DATA:
                if(cd->destroy_graph_progress)
                    FE_GraphProgressDestroy(ce->window_id, 	
											ce->URL_s, 	
											cd->original_content_length,
											ce->bytes_received);
#ifdef EDITOR
                if(cd->destroy_file_upload_progress_dialog)
                    FE_SaveDialogDestroy(ce->window_id, ce->status, cd->file_to_post);
#endif /* EDITOR */
                FREEIF(cd->cwd_message_buffer);
                FREEIF(cd->login_message_buffer);
                FREEIF(cd->data_buf)
                FREEIF(cd->return_msg)
                FREEIF(cd->data_con_address)
                FREEIF(cd->filename)
                FREEIF(cd->path)
                FREEIF(cd->username)
                FREEIF(cd->password)
                FREEIF(cd->stream);  /* don't forget the stream */
				FREEIF(cd->output_buffer);
            	if(cd->tcp_con_data)
                	NET_FreeTCPConData(cd->tcp_con_data);
				

                XP_FREE(cd);

                /* gate the maximum number of cached connections
			 	 * to one
                 */
                if(XP_ListCount(connection_list) > 1)
                  {
                    XP_List * list_ptr = connection_list->next;
                    FTPConnection * con;
        
                    while(list_ptr)
                      {
						con = (FTPConnection *) list_ptr->object;
						list_ptr = list_ptr->next;
                        if(!con->busy)
                          {
                            XP_ListRemoveObject(connection_list, con);

							if(con->csock != SOCKET_INVALID)
							  {
                			    NETCLOSE(con->csock);
								/* dont reduce the number of
								 * open connections since cached ones
							 	 * dont count as active
								 */
							  }
                            FREEIF(con->hostname);
                            XP_FREE(con);
                            break;
                          }
                      }
                  }

                return(-1);
    
              default:
                TRACEMSG(("Bad state in ProcessFTP\n"));
                return(-1);
         } /* end switch */
    
       if(ce->status < 0 && cd->next_state != FTP_FREE_DATA)
          {
            cd->pause_for_read = FALSE;
            cd->next_state = FTP_ERROR_DONE;
          }
    } /* end while */

    return(ce->status);
}
    
/* abort the connection in progress
 */
MODULE_PRIVATE int
NET_InterruptFTP(ActiveEntry * ce)
{
    FTPConData * cd = (FTPConData *)ce->con_data;

    cd->next_state = FTP_ERROR_DONE;
    ce->status = MK_INTERRUPTED;
	cd->cc->prev_cache = FALSE;  /* to keep it from reconnecting */
 
    return(NET_ProcessFTP(ce));
}

/* Free any connections or memory that are cached
 */
MODULE_PRIVATE void
NET_CleanupFTP(void)
{
    if(connection_list)
      {
        FTPConnection * tmp_con;

        while((tmp_con = (FTPConnection *)XP_ListRemoveTopObject(connection_list)) != NULL)
          {
			if(!tmp_con->busy)
			  {
            	FREE(tmp_con->hostname);       /* hostname string */
				if(tmp_con->csock != SOCKET_INVALID)
			      {
            	    NETCLOSE(tmp_con->csock);      /* control socket */
            	    tmp_con->csock = SOCKET_INVALID;
					/* don't count cached connections as open ones
					 */
			      }
		        FREE(tmp_con);
              }
		  }
      }

    return;
}
