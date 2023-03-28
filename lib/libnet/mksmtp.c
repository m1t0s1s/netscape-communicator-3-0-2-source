/* -*- Mode: C; tab-width: 4 -*-
 * state machine to speak SMTP
 */

/* Please leave outside of ifdef for windows precompiled headers */
#include "mkutils.h"

#if defined(MOZILLA_CLIENT) || defined(LIBNET_SMTP)

#include "mkgeturl.h"
#include "mksmtp.h"
#include "mime.h"
#include "glhist.h"
#include "mktcp.h"
#include "mkparse.h"
#include "msgcom.h"
#include "xp_time.h"
#include "xp_thrmo.h"
#include "merrors.h"


#include "xp_error.h"


/* for XP_GetString() */
#include "xpgetstr.h"
extern int XP_PROGRESS_MAILSENT;
extern int MK_COULD_NOT_GET_USERS_MAIL_ADDRESS;
extern int MK_COULD_NOT_LOGIN_TO_SMTP_SERVER;
extern int MK_ERROR_SENDING_DATA_COMMAND;
extern int MK_ERROR_SENDING_FROM_COMMAND;
extern int MK_ERROR_SENDING_MESSAGE;
extern int MK_ERROR_SENDING_RCPT_COMMAND;
extern int MK_OUT_OF_MEMORY;
extern int MK_SMTP_SERVER_ERROR;
extern int MK_TCP_READ_ERROR;
extern int XP_MESSAGE_SENT_WAITING_MAIL_REPLY;
extern int MK_MSG_DELIV_MAIL;
extern int MK_MSG_NO_SMTP_HOST;
extern int MK_MIME_NO_RECIPIENTS;

#define SMTP_PORT 25

#define OUTPUT_BUFFER_SIZE 512

/* definitions of state for the state machine design
 */
#define SMTP_RESPONSE               0
#define SMTP_START_CONNECT          1
#define SMTP_FINISH_CONNECT         2
#define SMTP_LOGIN_RESPONSE         3
#define SMTP_SEND_HELO_RESPONSE     4
#define SMTP_SEND_VRFY_RESPONSE     5
#define SMTP_SEND_MAIL_RESPONSE     6
#define SMTP_SEND_RCPT_RESPONSE     7
#define SMTP_SEND_DATA_RESPONSE	    8
#define SMTP_SEND_POST_DATA	    	9
#define SMTP_SEND_MESSAGE_RESPONSE  10
#define SMTP_DONE                   11
#define SMTP_ERROR_DONE             12
#define SMTP_FREE                   13

/* structure to hold data pertaining to the active state of
 * a transfer in progress.
 *
 */
typedef struct _SMTPConData {
    int     next_state;       			/* the next state or action to be taken */
	int     next_state_after_response;  /* the state after the response one */
    Bool    pause_for_read;   			/* Pause now for next read? */
#ifdef XP_WIN
	Bool    calling_netlib_all_the_time;
#endif
	char   *response_text;
	int     response_code;
    char   *data_buffer;
    int32  data_buffer_size;
	char   *address_copy;
	char   *mail_to_address_ptr;
    int     mail_to_addresses_left;
	TCP_ConData * tcp_con_data;
	int     continuation_response;
	char   *output_buffer;
	char   *hostname;
	char   *verify_address;
	void   *write_post_data_data;      /* a data object for the 
										* WritePostData function
									 	*/
	int32   total_amt_written;
	uint32  total_message_size;
	unsigned long last_time;
} SMTPConData;

/* macro's to simplify variable names */
#define CD_NEXT_STATE        	      cd->next_state
#define CD_NEXT_STATE_AFTER_RESPONSE  cd->next_state_after_response
#define CD_PAUSE_FOR_READ    	      cd->pause_for_read
#define CD_RESPONSE_TXT			      cd->response_text
#define CD_RESPONSE_CODE              cd->response_code
#define CD_DATA_BUFFER				  cd->data_buffer
#define CD_DATA_BUFFER_SIZE			  cd->data_buffer_size
#define CD_ADDRESS_COPY			      cd->address_copy
#define CD_MAIL_TO_ADDRESS_PTR        cd->mail_to_address_ptr
#define CD_MAIL_TO_ADDRESSES_LEFT     cd->mail_to_addresses_left
#define CD_TCP_CON_DATA				  cd->tcp_con_data
#define CD_CONTINUATION_RESPONSE	  cd->continuation_response
#define CD_HOSTNAME	  				  cd->hostname
#define CD_VERIFY_ADDRESS			  cd->verify_address
#define CD_TOTAL_AMT_WRITTEN          cd->total_amt_written
#define CD_TOTAL_MESSAGE_SIZE         cd->total_message_size

#define CE_URL_S            cur_entry->URL_s
#define CE_SOCK             cur_entry->socket
#define CE_CON_SOCK         cur_entry->con_sock
#define CE_STATUS           cur_entry->status
#define CE_WINDOW_ID        cur_entry->window_id
#define CE_BYTES_RECEIVED   cur_entry->bytes_received
#define CE_FORMAT_OUT       cur_entry->format_out

PRIVATE char *net_smtp_relay_host=0;

MODULE_PRIVATE char *
NET_MailRelayHost(MWContext *context)
{
    if(net_smtp_relay_host)
		return(net_smtp_relay_host);
	else
		return("");	/* caller now checks for empty string and returns MK_MSG_NO_SMTP_HOST */
}

PUBLIC void
NET_SetMailRelayHost(char * host)
{
	char * at = NULL;

	/*
	** If we are called with data like "fred@bedrock.com", then we will
	** help the user by ignoring the stuff before the "@".  People with
	** @ signs in their host names will be hosed.  They also can't possibly
	** be current happy internet users.
	*/
	if (host) at = XP_STRCHR(host, '@');
	if (at != NULL) host = at + 1;
	StrAllocCopy(net_smtp_relay_host, host);
}

/*
 * gets the response code from the nntp server and the
 * response line
 *
 * returns the TCP return code from the read
 */
PRIVATE int
net_smtp_response (ActiveEntry * cur_entry)
{
    char * line;
	char cont_char;
    SMTPConData * cd = (SMTPConData *)cur_entry->con_data;

    CE_STATUS = NET_BufferedReadLine(CE_SOCK, &line, &CD_DATA_BUFFER,
                    						&CD_DATA_BUFFER_SIZE, &CD_PAUSE_FOR_READ);

    if(CE_STATUS == 0)
      {
        CD_NEXT_STATE = SMTP_ERROR_DONE;
        CD_PAUSE_FOR_READ = FALSE;
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_SMTP_SERVER_ERROR,
													  CD_RESPONSE_TXT);
        return(MK_SMTP_SERVER_ERROR);
      }

    /* if TCP error of if there is not a full line yet return
     */
    if(CE_STATUS < 0)
	  {
		CE_URL_S->error_msg =
		  NET_ExplainErrorDetails(MK_TCP_READ_ERROR, SOCKET_ERRNO);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
	  }
	else if(!line)
	  {
         return CE_STATUS;
	  }

    TRACEMSG(("SMTP Rx: %s\n", line));

	cont_char = ' '; /* default */
    sscanf(line, "%d%c", &CD_RESPONSE_CODE, &cont_char);

     if(CD_CONTINUATION_RESPONSE == -1)
       {
         if (cont_char == '-')  /* begin continuation */
             CD_CONTINUATION_RESPONSE = CD_RESPONSE_CODE;

         if(XP_STRLEN(line) > 3)
         	StrAllocCopy(CD_RESPONSE_TXT, line+4);
       }
     else
       {    /* have to continue */
         if (CD_CONTINUATION_RESPONSE == CD_RESPONSE_CODE && cont_char == ' ')
             CD_CONTINUATION_RESPONSE = -1;    /* ended */

         StrAllocCat(CD_RESPONSE_TXT, "\n");
         if(XP_STRLEN(line) > 3)
             StrAllocCat(CD_RESPONSE_TXT, line+4);
       }

     if(CD_CONTINUATION_RESPONSE == -1)  /* all done with this response? */
       {
         CD_NEXT_STATE = CD_NEXT_STATE_AFTER_RESPONSE;
         CD_PAUSE_FOR_READ = FALSE; /* don't pause */
       }

    return(0);  /* everything ok */
}

PRIVATE int
net_smtp_login_response(ActiveEntry *cur_entry)
{
    SMTPConData * cd = (SMTPConData *)cur_entry->con_data;
	char buffer[356];

    if(CD_RESPONSE_CODE != 220)
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_COULD_NOT_LOGIN_TO_SMTP_SERVER);
		return(MK_COULD_NOT_LOGIN_TO_SMTP_SERVER);
	  }

	PR_snprintf(buffer, sizeof(buffer), "HELO %.256s" CRLF, NET_HostName());
	TRACEMSG(("Tx: %s", buffer));

    CE_STATUS = (int) NET_BlockingWrite(CE_SOCK, buffer, XP_STRLEN(buffer));

    CD_NEXT_STATE = SMTP_RESPONSE;
    CD_NEXT_STATE_AFTER_RESPONSE = SMTP_SEND_HELO_RESPONSE;
    CD_PAUSE_FOR_READ = TRUE;

    return(CE_STATUS);
}
    

PRIVATE int
net_smtp_send_helo_response(ActiveEntry *cur_entry)
{
    SMTPConData * cd = (SMTPConData *)cur_entry->con_data;
	char buffer[512];
	const char *mail_add = FE_UsersMailAddress();

	/* don't check for a HELO response because it can be bogus and
	 * we don't care
	 */

	if(!mail_add)
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_COULD_NOT_GET_USERS_MAIL_ADDRESS);
		return(MK_COULD_NOT_GET_USERS_MAIL_ADDRESS);
	  }

	if(CD_VERIFY_ADDRESS)
	  {
		PR_snprintf(buffer, sizeof(buffer), "VRFY %.256s" CRLF, CD_VERIFY_ADDRESS);
	  }
	else
	  {
		/* else send the MAIL FROM: command */
		char *s = MSG_MakeFullAddress (NULL, mail_add);
		if (!s)
		  {
			CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
			return(MK_OUT_OF_MEMORY);
		  }
		PR_snprintf(buffer, sizeof(buffer), "MAIL FROM:<%.256s>" CRLF, s);
		XP_FREE (s);
	  }

	TRACEMSG(("Tx: %s", buffer));
    CE_STATUS = (int) NET_BlockingWrite(CE_SOCK, buffer, XP_STRLEN(buffer));

    CD_NEXT_STATE = SMTP_RESPONSE;

	if(CD_VERIFY_ADDRESS)
    	CD_NEXT_STATE_AFTER_RESPONSE = SMTP_SEND_VRFY_RESPONSE;
	else
    	CD_NEXT_STATE_AFTER_RESPONSE = SMTP_SEND_MAIL_RESPONSE;
    CD_PAUSE_FOR_READ = TRUE;

    return(CE_STATUS);
}

PRIVATE int
net_smtp_send_vrfy_response(ActiveEntry *cur_entry)
{
#if 0
	SMTPConData * cd = (SMTPConData *)cur_entry->con_data;
    char buffer[512];

    if(CD_RESPONSE_CODE == 250 || CD_RESPONSE_CODE == 251)
		return(MK_USER_VERIFIED_BY_SMTP);
	else
		return(MK_USER_NOT_VERIFIED_BY_SMTP);
#else	
	XP_ASSERT(0);
	return(-1);
#endif
}

PRIVATE int
net_smtp_send_mail_response(ActiveEntry *cur_entry)
{
	SMTPConData * cd = (SMTPConData *)cur_entry->con_data;
    char buffer[512];

    if(CD_RESPONSE_CODE != 250)
	  {
		CE_URL_S->error_msg = 
		  NET_ExplainErrorDetails(MK_ERROR_SENDING_FROM_COMMAND,
								  CD_RESPONSE_TXT);
		return(MK_ERROR_SENDING_FROM_COMMAND);
	  }

    /* Send the RCPT TO: command */
    PR_snprintf(buffer, sizeof(buffer), "RCPT TO:<%.256s>" CRLF, CD_MAIL_TO_ADDRESS_PTR);
	/* take the address we sent off the list (move the pointer to just
	   past the terminating null.) */
	CD_MAIL_TO_ADDRESS_PTR += XP_STRLEN (CD_MAIL_TO_ADDRESS_PTR) + 1;
	CD_MAIL_TO_ADDRESSES_LEFT--;

	TRACEMSG(("Tx: %s", buffer));
    
    CE_STATUS = (int) NET_BlockingWrite(CE_SOCK, buffer, XP_STRLEN(buffer));

    CD_NEXT_STATE = SMTP_RESPONSE;
    CD_NEXT_STATE_AFTER_RESPONSE = SMTP_SEND_RCPT_RESPONSE;
    CD_PAUSE_FOR_READ = TRUE;

    return(CE_STATUS);
}

PRIVATE int
net_smtp_send_rcpt_response(ActiveEntry *cur_entry)
{
    SMTPConData * cd = (SMTPConData *)cur_entry->con_data;
    char buffer[16];

	if(CD_RESPONSE_CODE != 250 && CD_RESPONSE_CODE != 251)
	  {
		CE_URL_S->error_msg =
		  NET_ExplainErrorDetails(MK_ERROR_SENDING_RCPT_COMMAND,
								  CD_RESPONSE_TXT);
        return(MK_ERROR_SENDING_RCPT_COMMAND);
	  }

	if(CD_MAIL_TO_ADDRESSES_LEFT > 0)
	  {
		/* more senders to RCPT to 
		 */
        CD_NEXT_STATE = SMTP_SEND_MAIL_RESPONSE; 
		return(0);
	  }

    /* else send the RCPT TO: command */
    XP_STRCPY(buffer, "DATA" CRLF);

	TRACEMSG(("Tx: %s", buffer));
        
    CE_STATUS = (int) NET_BlockingWrite(CE_SOCK, buffer, XP_STRLEN(buffer));   

    CD_NEXT_STATE = SMTP_RESPONSE;  
    CD_NEXT_STATE_AFTER_RESPONSE = SMTP_SEND_DATA_RESPONSE; 
    CD_PAUSE_FOR_READ = TRUE;   

    return(CE_STATUS);  
}

PRIVATE int
net_smtp_send_data_response(ActiveEntry *cur_entry)
{
    SMTPConData * cd = (SMTPConData *)cur_entry->con_data;
    char buffer[512];
    char * command=0;   

    if(CD_RESPONSE_CODE != 354)
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(
									MK_ERROR_SENDING_DATA_COMMAND,
                        			CD_RESPONSE_TXT ? CD_RESPONSE_TXT : "");
        return(MK_ERROR_SENDING_DATA_COMMAND);
	  }

#ifdef XP_UNIX
	{
	  const char * FE_UsersRealMailAddress(void); /* definition */
	  const char *real_name = FE_UsersRealMailAddress();
	  char *s = (real_name ? MSG_MakeFullAddress (NULL, real_name) : 0);
	  if (real_name && !s)
		{
		  CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
		  return(MK_OUT_OF_MEMORY);
		}
	  if(real_name)
		{
		  PR_snprintf(buffer, sizeof(buffer), "Sender: %.256s" CRLF, real_name);
		  StrAllocCat(command, buffer);
	      if(!command)
	        {
		      CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
		      return(MK_OUT_OF_MEMORY);
	        }
		}
	}

    TRACEMSG(("sending extra unix header: %s", command));

    CE_STATUS = (int) NET_BlockingWrite(CE_SOCK, command, XP_STRLEN(command));   
	if(CE_STATUS < 0)
		TRACEMSG(("Error sending message"));
#endif /* XP_UNIX */

	/* set connect select since we need to select on
	 * writes
	 */
	FE_ClearReadSelect(CE_WINDOW_ID, CE_SOCK);
	FE_SetConnectSelect(CE_WINDOW_ID, CE_SOCK);
#ifdef XP_WIN
	cd->calling_netlib_all_the_time = TRUE;
	net_call_all_the_time_count++;
	FE_SetCallNetlibAllTheTime(CE_WINDOW_ID);
#endif
    CE_CON_SOCK = CE_SOCK;

	FREE(command);

    CD_NEXT_STATE = SMTP_SEND_POST_DATA;
    CD_PAUSE_FOR_READ = FALSE;   /* send data directly */

	NET_Progress(CE_WINDOW_ID, XP_GetString(MK_MSG_DELIV_MAIL));

	/* get the size of the message */
	if(CE_URL_S->post_data_is_file)
	  {
		XP_StatStruct stat_entry;

		if(-1 != XP_Stat(CE_URL_S->post_data,
                         &stat_entry,
                         xpFileToPost))
			CD_TOTAL_MESSAGE_SIZE = stat_entry.st_size;
	  }
	else
	  {
			CD_TOTAL_MESSAGE_SIZE = CE_URL_S->post_data_size;
	  }


    return(CE_STATUS);  
}

PRIVATE int
net_smtp_send_post_data(ActiveEntry *cur_entry)
{
    SMTPConData * cd = (SMTPConData *)cur_entry->con_data;
	unsigned long curtime;

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
        XP_STRCPY(cd->data_buffer, CRLF "." CRLF);
        TRACEMSG(("sending %s", cd->data_buffer));
        CE_STATUS = (int) NET_BlockingWrite(CE_SOCK,
                                            cd->data_buffer,
                                            XP_STRLEN(cd->data_buffer));

		NET_Progress(CE_WINDOW_ID,
					XP_GetString(XP_MESSAGE_SENT_WAITING_MAIL_REPLY));

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

        CD_NEXT_STATE = SMTP_RESPONSE;
        CD_NEXT_STATE_AFTER_RESPONSE = SMTP_SEND_MESSAGE_RESPONSE;
        return(0);
	  }

	CD_TOTAL_AMT_WRITTEN += CE_STATUS;

	/* Update the thermo and the status bar.  This is done by hand, rather
	   than using the FE_GraphProgress* functions, because there seems to be
	   no way to make FE_GraphProgress shut up and not display anything more
	   when all the data has arrived.  At the end, we want to show the
	   "message sent; waiting for reply" status; FE_GraphProgress gets in
	   the way of that.  See bug #23414. */

	curtime = XP_TIME();
	if (curtime != cd->last_time) {
		FE_Progress(CE_WINDOW_ID, XP_ProgressText(CD_TOTAL_MESSAGE_SIZE,
												  CD_TOTAL_AMT_WRITTEN,
												  0, 0));
		cd->last_time = curtime;
	}
	if (CD_TOTAL_MESSAGE_SIZE > 0)
	  FE_SetProgressBarPercent(CE_WINDOW_ID,
							   CD_TOTAL_AMT_WRITTEN*100/CD_TOTAL_MESSAGE_SIZE);

    return(CE_STATUS);
}



PRIVATE int
net_smtp_send_message_response(ActiveEntry *cur_entry)
{
    SMTPConData * cd = (SMTPConData *)cur_entry->con_data;

    if(CD_RESPONSE_CODE != 250)
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_ERROR_SENDING_MESSAGE,
													  CD_RESPONSE_TXT);
        return(MK_ERROR_SENDING_MESSAGE);
	  }

	NET_Progress(CE_WINDOW_ID, XP_GetString(XP_PROGRESS_MAILSENT));

	/* else */
	CD_NEXT_STATE = SMTP_DONE;
	return(MK_NO_DATA);
}


MODULE_PRIVATE int 
NET_MailtoLoad (ActiveEntry * cur_entry)
{
    /* get memory for Connection Data */
    SMTPConData * cd = XP_NEW(SMTPConData);

    cur_entry->con_data = cd;
    if(!cur_entry->con_data)
      {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
        CE_STATUS = MK_OUT_OF_MEMORY;
        return (CE_STATUS);
      }

/*	GH_UpdateGlobalHistory(cur_entry->URL_s); */

    /* init */
    XP_MEMSET(cd, 0, sizeof(SMTPConData));

	CD_CONTINUATION_RESPONSE = -1;  /* init */
	cd->output_buffer = (char *)XP_ALLOC(OUTPUT_BUFFER_SIZE);

	if(!cd->output_buffer)
	  {
		FREE(cd);
		return(MK_OUT_OF_MEMORY);
	  }
  
    CE_SOCK = SOCKET_INVALID;

	/* make a copy of the address
	 */
	if(CE_URL_S->method == URL_POST_METHOD)
	  {
		int status;
		char *addrs1 = 0;
		char *addrs2 = 0;
    	CD_NEXT_STATE = SMTP_START_CONNECT;

		/* Remove duplicates from the list, to prevent people from getting
		   more than one copy (the SMTP host may do this too, or it may not.)
		   This causes the address list to be parsed twice; this probably
		   doesn't matter.
		 */
		addrs1 = MSG_RemoveDuplicateAddresses (CE_URL_S->address+7, 0);

		/* Extract just the mailboxes from the full RFC822 address list.
		   This means that people can post to mailto: URLs which contain
		   full RFC822 address specs, and we will still send the right
		   thing in the SMTP RCPT command.
		 */
		status = MSG_ParseRFC822Addresses (addrs1, 0, &addrs2);
		FREEIF (addrs1);
		if (status < 0) return status;

		if (status == 0 || addrs2 == 0)
		  {
			CD_NEXT_STATE = SMTP_ERROR_DONE;
			CD_PAUSE_FOR_READ = FALSE;
			CE_STATUS = MK_MIME_NO_RECIPIENTS;
			CE_URL_S->error_msg = NET_ExplainErrorDetails(CE_STATUS);
			return(MK_SMTP_SERVER_ERROR);
			return CE_STATUS;
		  }

		CD_ADDRESS_COPY = addrs2;
		CD_MAIL_TO_ADDRESS_PTR = CD_ADDRESS_COPY;
		CD_MAIL_TO_ADDRESSES_LEFT = status;
        return(NET_ProcessMailto(cur_entry));
	  }
	else
	  {
		/* parse special headers and stuff from the search data in the
		   URL address.  This data is of the form

		    mailto:TO_FIELD?FIELD1=VALUE1&FIELD2=VALUE2

		   where TO_FIELD may be empty, VALUEn may (for now) only be
		   one of "cc", "bcc", "subject", "newsgroups", "references",
		   and "attachment".

		   "to" is allowed as a field/value pair as well, for consistency.

		   One additional parameter is allowed, which does not correspond
		   to a visible field: "newshost".  This is the NNTP host (and port)
		   to connect to if newsgroups are specified.  If the value of this
		   field ends in "/secure", then SSL will be used.

		   Each parameter may appear only once, but the order doesn't
		   matter.  All values must be URL-encoded.
		 */
		char *parms = NET_ParseURL (CE_URL_S->address, GET_SEARCH_PART);
		char *rest = parms;
		char *from = 0;						/* internal only */
		char *reply_to = 0;					/* internal only */
		char *to = 0;
		char *cc = 0;
		char *bcc = 0;
		char *fcc = 0;						/* internal only */
		char *newsgroups = 0;
		char *followup_to = 0;
		char *organization = 0;				/* internal only */
		char *subject = 0;
		char *references = 0;
		char *attachment = 0;				/* internal only */
		char *body = 0;
		char *other_random_headers = 0;		/* unused (for now) */
		char *newshost = 0;					/* internal only */
		XP_Bool encrypt_p = FALSE;
		XP_Bool sign_p = FALSE;				/* internal only */

		char *newspost_url = 0;

		to = NET_ParseURL (CE_URL_S->address, GET_PATH_PART);

		if (rest && *rest == '?')
		  {
 			/* start past the '?' */
			rest++;
			rest = XP_STRTOK (rest, "&");
			while (rest && *rest)
			  {
				char *token = rest;
				char *value = 0;
				char *eq = XP_STRCHR (token, '=');
				if (eq)
				  {
					value = eq+1;
					*eq = 0;
				  }
				switch (*token)
				  {
				  case 'A': case 'a':
					if (!strcasecomp (token, "attachment") &&
						CE_URL_S->internal_url)
					  StrAllocCopy (attachment, value);
					break;
				  case 'B': case 'b':
					if (!strcasecomp (token, "bcc"))
					  {
						if (bcc && *bcc)
						  {
							StrAllocCat (bcc, ", ");
							StrAllocCat (bcc, value);
						  }
						else
						  {
							StrAllocCopy (bcc, value);
						  }
					  }
					else if (!strcasecomp (token, "body"))
					  {
						if (body && *body)
						  {
							StrAllocCat (body, "\n");
							StrAllocCat (body, value);
						  }
						else
						  {
							StrAllocCopy (body, value);
						  }
					  }
					break;
				  case 'C': case 'c':
					if (!strcasecomp (token, "cc"))
					  {
						if (cc && *cc)
						  {
							StrAllocCat (cc, ", ");
							StrAllocCat (cc, value);
						  }
						else
						  {
							StrAllocCopy (cc, value);
						  }
					  }
					break;
				  case 'E': case 'e':
					if (!strcasecomp (token, "encrypt") ||
						!strcasecomp (token, "encrypted"))
					  encrypt_p = (!strcasecomp(value, "true") ||
								   !strcasecomp(value, "yes"));
					break;
				  case 'F': case 'f':
					if (!strcasecomp (token, "followup-to"))
					  StrAllocCopy (followup_to, value);
					else if (!strcasecomp (token, "from") &&
							 CE_URL_S->internal_url)
					  StrAllocCopy (from, value);
					break;
				  case 'N': case 'n':
					if (!strcasecomp (token, "newsgroups"))
					  StrAllocCopy (newsgroups, value);
					else if (!strcasecomp (token, "newshost") &&
							 CE_URL_S->internal_url)
					  StrAllocCopy (newshost, value);
					break;
				  case 'O': case 'o':
					if (!strcasecomp (token, "organization") &&
						CE_URL_S->internal_url)
					  StrAllocCopy (organization, value);
					break;
				  case 'R': case 'r':
					if (!strcasecomp (token, "references"))
					  StrAllocCopy (references, value);
					else if (!strcasecomp (token, "reply-to") &&
							 CE_URL_S->internal_url)
					  StrAllocCopy (reply_to, value);
					break;
				  case 'S': case 's':
					if(!strcasecomp (token, "subject"))
					  StrAllocCopy (subject, value);
					else if ((!strcasecomp (token, "sign") ||
							  !strcasecomp (token, "signed")) &&
							 CE_URL_S->internal_url)
					  sign_p = (!strcasecomp(value, "true") ||
								!strcasecomp(value, "yes"));
					break;
				  case 'T': case 't':
					if (!strcasecomp (token, "to"))
					  {
						if (to && *to)
						  {
							StrAllocCat (to, ", ");
							StrAllocCat (to, value);
						  }
						else
						  {
							StrAllocCopy (to, value);
						  }
					  }
					break;
				  }
				if (eq)
				  *eq = '='; /* put it back */
				rest = XP_STRTOK (0, "&");
			  }
		  }

		FREEIF (parms);
		if (to)
		  NET_UnEscape (to);
		if (cc)
		  NET_UnEscape (cc);
		if (subject)
		  NET_UnEscape (subject);
		if (newsgroups)
		  NET_UnEscape (newsgroups);
		if (references)
		  NET_UnEscape (references);
		if (attachment)
		  NET_UnEscape (attachment);
		if (body)
		  NET_UnEscape (body);
		if (newshost)
		  NET_UnEscape (newshost);

		if(newshost)
		  {
			char *prefix = "news://";
			char *slash = XP_STRRCHR (newshost, '/');
			if (slash && !strcasecomp (slash, "/secure"))
			  {
				*slash = 0;
				prefix = "snews://";
			  }
			newspost_url = (char *) XP_ALLOC (XP_STRLEN (prefix) +
											  XP_STRLEN (newshost) + 10);
			if (newspost_url)
			  {
				XP_STRCPY (newspost_url, prefix);
				XP_STRCAT (newspost_url, newshost);
				XP_STRCAT (newspost_url, "/");
			  }
		  }
		else
		  {
			newspost_url = XP_STRDUP ("news:");
		  }

		/* Tell the message library and front end to pop up an edit window.
		 */
		MSG_ComposeMessage (CE_WINDOW_ID,
							from, reply_to, to, cc, bcc, fcc, newsgroups,
							followup_to, organization, subject, references,
							other_random_headers, attachment, body,
							newspost_url, encrypt_p, sign_p);
		FREEIF(from);
		FREEIF(reply_to);
		FREEIF(to);
		FREEIF(cc);
		FREEIF(bcc);
		FREEIF(fcc);
		FREEIF(newsgroups);
		FREEIF(followup_to);
		FREEIF(organization);
		FREEIF(subject);
		FREEIF(references);
		FREEIF(attachment);
		FREEIF(body);
		FREEIF(other_random_headers);
		FREEIF(newshost);
		FREEIF(newspost_url);

		CE_STATUS = MK_NO_DATA;
		return(-1);
	  }
}


/*
 * returns negative if the transfer is finished or error'd out
 *
 * returns zero or more if the transfer needs to be continued.
 */
MODULE_PRIVATE int 
NET_ProcessMailto (ActiveEntry *cur_entry)
{
    SMTPConData * cd = (SMTPConData *)cur_entry->con_data;
	char	*mail_relay_host;

    TRACEMSG(("Entering NET_ProcessSMTP"));

    CD_PAUSE_FOR_READ = FALSE; /* already paused; reset */

    while(!CD_PAUSE_FOR_READ)
      {

		TRACEMSG(("In NET_ProcessSMTP with state: %d", CD_NEXT_STATE));

        switch(CD_NEXT_STATE) {

		case SMTP_RESPONSE:
			net_smtp_response (cur_entry);
			break;

        case SMTP_START_CONNECT:
			mail_relay_host = NET_MailRelayHost(CE_WINDOW_ID);
			if (XP_STRLEN(mail_relay_host) == 0)
			{
				CE_STATUS = MK_MSG_NO_SMTP_HOST;
				break;
			}
            CE_STATUS = NET_BeginConnect(NET_MailRelayHost(CE_WINDOW_ID), 
										NULL,
										"SMTP",
										SMTP_PORT, 
										&CE_SOCK, 
										FALSE, 
										&CD_TCP_CON_DATA, 
										CE_WINDOW_ID,
										&CE_URL_S->error_msg,
										 cur_entry->socks_host,
										 cur_entry->socks_port);
            CD_PAUSE_FOR_READ = TRUE;
            if(CE_STATUS == MK_CONNECTED)
              {
                CD_NEXT_STATE = SMTP_RESPONSE;
                CD_NEXT_STATE_AFTER_RESPONSE = SMTP_LOGIN_RESPONSE;
                FE_SetReadSelect(CE_WINDOW_ID, CE_SOCK);
              }
            else if(CE_STATUS > -1)
              {
                CE_CON_SOCK = CE_SOCK;  /* set con_sock so we can select on it */
                FE_SetConnectSelect(CE_WINDOW_ID, CE_CON_SOCK);
                CD_NEXT_STATE = SMTP_FINISH_CONNECT;
              }
            break;

		case SMTP_FINISH_CONNECT:
            CE_STATUS = NET_FinishConnect(NET_MailRelayHost(CE_WINDOW_ID), 
										  "SMTP", 
										  SMTP_PORT, 
										  &CE_SOCK, 
										  &CD_TCP_CON_DATA, 
										  CE_WINDOW_ID,
										  &CE_URL_S->error_msg);

            CD_PAUSE_FOR_READ = TRUE;
            if(CE_STATUS == MK_CONNECTED)
              {
                CD_NEXT_STATE = SMTP_RESPONSE;
                CD_NEXT_STATE_AFTER_RESPONSE = SMTP_LOGIN_RESPONSE;
                FE_ClearConnectSelect(CE_WINDOW_ID, CE_CON_SOCK);
                CE_CON_SOCK = SOCKET_INVALID;  /* reset con_sock so we don't select on it */
                FE_SetReadSelect(CE_WINDOW_ID, CE_SOCK);
              }
			else
              {
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

       case SMTP_LOGIN_RESPONSE:
            CE_STATUS = net_smtp_login_response(cur_entry);
            break;

	   case SMTP_SEND_HELO_RESPONSE:
            CE_STATUS = net_smtp_send_helo_response(cur_entry);
            break;
			
	   case SMTP_SEND_VRFY_RESPONSE:
			CE_STATUS = net_smtp_send_vrfy_response(cur_entry);
            break;
			
	   case SMTP_SEND_MAIL_RESPONSE:
            CE_STATUS = net_smtp_send_mail_response(cur_entry);
            break;
			
	   case SMTP_SEND_RCPT_RESPONSE:
            CE_STATUS = net_smtp_send_rcpt_response(cur_entry);
            break;
			
	   case SMTP_SEND_DATA_RESPONSE:
            CE_STATUS = net_smtp_send_data_response(cur_entry);
            break;
			
	   case SMTP_SEND_POST_DATA:
            CE_STATUS = net_smtp_send_post_data(cur_entry);
            break;
			
	   case SMTP_SEND_MESSAGE_RESPONSE:
            CE_STATUS = net_smtp_send_message_response(cur_entry);
            break;
        
        case SMTP_DONE:
            FE_ClearReadSelect(CE_WINDOW_ID, CE_SOCK);
            NETCLOSE(CE_SOCK);
            CD_NEXT_STATE = SMTP_FREE;
            break;
        
        case SMTP_ERROR_DONE:
            if(CE_SOCK != SOCKET_INVALID)
              {
                FE_ClearReadSelect(CE_WINDOW_ID, CE_SOCK);
                FE_ClearConnectSelect(CE_WINDOW_ID, CE_SOCK);
#ifdef XP_WIN
				if(cd->calling_netlib_all_the_time)
				{
					net_call_all_the_time_count--;
					cd->calling_netlib_all_the_time = FALSE;
					if(net_call_all_the_time_count == 0)
						FE_ClearCallNetlibAllTheTime(CE_WINDOW_ID);
				}
#endif /* XP_WIN */
#if defined(XP_WIN) || defined(XP_UNIX)
                FE_ClearDNSSelect(CE_WINDOW_ID, CE_SOCK);
#endif /* XP_WIN || XP_UNIX */
                NETCLOSE(CE_SOCK);
              }
            CD_NEXT_STATE = SMTP_FREE;
            break;
        
        case SMTP_FREE:
            FREEIF(CD_DATA_BUFFER);
            FREEIF(CD_ADDRESS_COPY);
			FREEIF(CD_RESPONSE_TXT);
            if(CD_TCP_CON_DATA)
                NET_FreeTCPConData(CD_TCP_CON_DATA);
            FREE(cd);

            return(-1); /* final end */
        
        default: /* should never happen !!! */
            TRACEMSG(("SMTP: BAD STATE!"));
            CD_NEXT_STATE = SMTP_ERROR_DONE;
            break;
        }

        /* check for errors during load and call error 
         * state if found
         */
        if(CE_STATUS < 0 && CD_NEXT_STATE != SMTP_FREE)
          {
            CD_NEXT_STATE = SMTP_ERROR_DONE;
            /* don't exit! loop around again and do the free case */
            CD_PAUSE_FOR_READ = FALSE;
          }
      } /* while(!CD_PAUSE_FOR_READ) */
    
    return(CE_STATUS);
}

/* abort the connection in progress
 */
MODULE_PRIVATE int
NET_InterruptMailto(ActiveEntry * cur_entry)
{
    SMTPConData * cd = (SMTPConData *)cur_entry->con_data;

    CD_NEXT_STATE = SMTP_ERROR_DONE;
    CE_STATUS = MK_INTERRUPTED;
 
    return(NET_ProcessMailto(cur_entry));
}

/* Free any memory that might be used in caching etc.
 */
MODULE_PRIVATE void
NET_CleanupMailto(void)
{
    /* nothing so far needs freeing */
    return;
}

#endif /* defined(MOZILLA_CLIENT) || defined(LIBNET_SMTP) */
