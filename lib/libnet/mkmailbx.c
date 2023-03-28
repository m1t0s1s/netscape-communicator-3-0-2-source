/* -*- Mode: C; tab-width: 4 -*-
 * state machine to handle mailbox URL 
 */

#include "mkutils.h"
#include "mkgeturl.h"
#include "mktcp.h"
#include "mkparse.h"
#include "mkmailbx.h"
#include "msgcom.h"
#include "libmime.h"
#include "merrors.h"


/* for XP_GetString() */
#include "xpgetstr.h"
extern int XP_MAIL_READING_FOLDER;
extern int XP_MAIL_READING_MESSAGE;
extern int XP_MAIL_EMPTYING_TRASH;
extern int XP_COMPRESSING_FOLDER;
extern int XP_MAIL_DELIVERING_QUEUED_MESSAGES;
extern int XP_READING_MESSAGE_DONE;
extern int XP_MAIL_READING_FOLDER_DONE;
extern int XP_MAIL_EMPTYING_TRASH_DONE;
extern int XP_MAIL_COMPRESSING_FOLDER_DONE;
extern int XP_MAIL_DELIVERING_QUEUED_MESSAGES_DONE;


extern int MK_OUT_OF_MEMORY;

#define OUTPUT_BUFFER_SIZE 512

/* definitions of states for the state machine design
 */
typedef enum _MailboxStates {
MAILBOX_OPEN_FOLDER,
MAILBOX_FINISH_OPEN_FOLDER,
MAILBOX_OPEN_MESSAGE,
MAILBOX_OPEN_STREAM,
MAILBOX_READ_MESSAGE,
MAILBOX_EMPTY_TRASH,
MAILBOX_FINISH_EMPTY_TRASH,
MAILBOX_COMPRESS_FOLDER,
MAILBOX_FINISH_COMPRESS_FOLDER,
MAILBOX_DELIVER_QUEUED,
MAILBOX_FINISH_DELIVER_QUEUED,
MAILBOX_DONE,
MAILBOX_ERROR_DONE,
MAILBOX_FREE
} MailboxStates;

/* structure to hold data pertaining to the active state of
 * a transfer in progress.
 *
 */
typedef struct _MailboxConData {
    int     		 next_state;     /* the next state or action to be taken */
    int     		 initial_state;  /* why we are here */
    Bool    		 pause_for_read; /* Pause now for next read? */
	void            *folder_ptr;
	void            *msg_ptr;
	void            *empty_trash_ptr;
	void            *compress_folder_ptr;
	void            *deliver_queued_ptr;
	char            *folder_name;
	char            *msg_id;
    int				 msgnum;	/* -1 if none specified. */
	NET_StreamClass *stream;

    XP_Bool  destroy_graph_progress;       /* do we need to destroy 
											* graph progress? */    
	int32  original_content_length; /* the content length at the time of
                                     * calling graph progress */

} MailboxConData;

#define COMPLETE_STREAM   (*cd->stream->complete)(cd->stream->data_object)
#define ABORT_STREAM(s)   (*cd->stream->abort)(cd->stream->data_object, s)
#define PUT_STREAM(buf, size)   (*cd->stream->put_block) \
						  (cd->stream->data_object, buf, size)



MODULE_PRIVATE int 
NET_MailboxLoad (ActiveEntry * ce)
{
    /* get memory for Connection Data */
    MailboxConData * cd = XP_NEW(MailboxConData);
	char *path = NET_ParseURL(ce->URL_s->address, GET_PATH_PART);
	char *search = NET_ParseURL(ce->URL_s->address, GET_SEARCH_PART);
	char *part = search;

    ce->con_data = cd;
    if(!ce->con_data)
      {
		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
        ce->status = MK_OUT_OF_MEMORY;
        return (ce->status);
      }

    /* init */
    XP_MEMSET(cd, 0, sizeof(MailboxConData));
	cd->msgnum = -1;

	cd->folder_name = path;
	cd->next_state = MAILBOX_OPEN_FOLDER;

#ifndef XP_MAC /* #### Giant Evil Mac Pathname Hack */
	NET_UnEscape (path);
#endif /* !XP_MAC */

	if (part && *part == '?') part++;
	while (part) {
	  char* amp = XP_STRCHR(part, '&');
	  if (amp) *amp++ = '\0';
	  if (XP_STRNCMP(part, "id=", 3) == 0) {
		cd->msg_id = XP_STRDUP (NET_UnEscape (part+3));
	  } else if (XP_STRNCMP(part, "number=", 7) == 0) {
		cd->msgnum = XP_ATOI(part + 7);
		if (cd->msgnum == 0 && part[7] != '0') cd->msgnum = -1;
	  } else if (XP_STRNCMP(part, "uidl=", 5) == 0) {
		/* ### Vile hack time.  If a UIDL was specified, then tell libmsg about
		   it, giving it a chance to arrange so that when this URL is all done,
		   MSG_GetNewMail gets called. */
		MSG_PrepareToIncUIDL(ce->window_id, ce->URL_s, NET_UnEscape(part + 5));
	  } else if (ce->URL_s->internal_url &&
				 XP_STRNCMP(part, "empty-trash", 10) == 0) {
		cd->next_state = MAILBOX_EMPTY_TRASH;
	  } else if (ce->URL_s->internal_url &&
				 XP_STRNCMP(part, "compress-folder", 15) == 0) {
		cd->next_state = MAILBOX_COMPRESS_FOLDER;
	  } else if (ce->URL_s->internal_url &&
				 XP_STRNCMP(part, "deliver-queued", 14) == 0) {
		cd->next_state = MAILBOX_DELIVER_QUEUED;
	  }
	  part = amp;
	}
	FREEIF(search);

	cd->initial_state = cd->next_state;

	/* don't cache mailbox url's */
	ce->format_out = CLEAR_CACHE_BIT(ce->format_out);

   	ce->local_file = TRUE;
   	ce->socket = 1;
	if(net_call_all_the_time_count < 1)
		FE_SetCallNetlibAllTheTime(ce->window_id);
	net_call_all_the_time_count++;

	return(NET_ProcessMailbox(ce));
}


/* This doesn't actually generate HTML - but it is our hook into the 
   message display code, so that we can get some values out of it
   after the headers-to-be-displayed have been parsed.
 */
static char *
mail_generate_html_header_fn (const char *dest, void *closure,
							  MimeHeaders *headers)
{
  ActiveEntry *cur_entry = (ActiveEntry *) closure;
  char *id = (headers
			  ? MimeHeaders_get(headers, HEADER_MESSAGE_ID, FALSE, FALSE)
			  : 0);
  if (id)
	{
	  /* #### If there is no message ID, we must hope that the click which
		 selected this message also marked it read - because at this point,
		 we don't have access to the MSG_ThreadEntry corresponding to this
		 message, which is the only place where the generated MD5 message ID
		 is. */
	  char *xref = MimeHeaders_get(headers, HEADER_XREF, FALSE, FALSE);
	  MSG_MarkMessageIDRead (cur_entry->window_id, id, xref);
	  FREEIF(xref);
	  FREEIF(id);
	}
  MSG_ActivateReplyOptions (cur_entry->window_id, headers);
  return 0;
}


static char *
mail_generate_html_footer_fn (const char *dest, void *closure,
							  MimeHeaders *headers)
{
  ActiveEntry *cur_entry = (ActiveEntry *) closure;
  char *uidl = (headers
				? MimeHeaders_get(headers, HEADER_X_UIDL, FALSE, FALSE)
				: 0);
  if (uidl)
	{
	  XP_FREE(uidl);
	  return MSG_GeneratePartialMessageBlurb (cur_entry->window_id,
											  cur_entry->URL_s,
											  closure, headers);
	}
  return 0;
}


static char *
mail_generate_reference_url_fn (const char *dest, void *closure,
								MimeHeaders *headers)
{
  ActiveEntry *cur_entry = (ActiveEntry *) closure;
  char *addr = cur_entry->URL_s->address;
  char *search = (addr ? XP_STRCHR (addr, '?') : 0);
  char *id2;
  char *new_dest;
  char *result;

  if (!dest || !*dest) return 0;
  id2 = XP_STRDUP (dest);
  if (!id2) return 0;
  if (id2[XP_STRLEN (id2)-1] == '>')
	id2[XP_STRLEN (id2)-1] = 0;
  if (id2[0] == '<')
	new_dest = NET_Escape (id2+1, URL_PATH);
  else
	new_dest = NET_Escape (id2, URL_PATH);

  FREEIF (id2);
  result = (char *) XP_ALLOC ((search ? search - addr : 0) +
							  (new_dest ? XP_STRLEN (new_dest) : 0) +
							  40);
  if (result && new_dest)
	{
	  if (search)
		{
		  XP_MEMCPY (result, addr, search - addr);
		  result[search - addr] = 0;
		}
	  else if (addr)
		XP_STRCPY (result, addr);
	  else
		*result = 0;
	  XP_STRCAT (result, "?id=");
	  XP_STRCAT (result, new_dest);

	  if (search && XP_STRSTR (search, "&headers=all"))
		XP_STRCAT (result, "&headers=all");
	}
  FREEIF (new_dest);
  return result;
}


static int
net_make_mail_msg_stream (ActiveEntry *ce)
{
  MailboxConData * cd = (MailboxConData *)ce->con_data;

  StrAllocCopy(ce->URL_s->content_type, MESSAGE_RFC822);

  if (ce->format_out == FO_PRESENT || ce->format_out == FO_CACHE_AND_PRESENT)
	{
	  MimeDisplayOptions *opt = XP_NEW (MimeDisplayOptions);
	  if (!opt) return MK_OUT_OF_MEMORY;
	  XP_MEMSET (opt, 0, sizeof(*opt));

	  opt->generate_reference_url_fn      = mail_generate_reference_url_fn;
	  opt->generate_header_html_fn		  = 0;
	  opt->generate_post_header_html_fn	  = mail_generate_html_header_fn;
	  opt->generate_footer_html_fn		  = mail_generate_html_footer_fn;
	  opt->html_closure					  = ce;
	  ce->URL_s->fe_data = opt;
	}
  cd->stream = NET_StreamBuilder(ce->format_out, ce->URL_s, ce->window_id);
  if (!cd->stream)
	return MK_UNABLE_TO_CONVERT;
  else
	return 0;
}


/*
 * returns negative if the transfer is finished or error'd out
 *
 * returns zero or more if the transfer needs to be continued.
 */
MODULE_PRIVATE int 
NET_ProcessMailbox (ActiveEntry *ce)
{
    MailboxConData * cd = (MailboxConData *)ce->con_data;

    TRACEMSG(("Entering NET_ProcessMailbox"));

    cd->pause_for_read = FALSE; /* already paused; reset */

    while(!cd->pause_for_read)
      {

		TRACEMSG(("In NET_ProcessMailbox with state: %d", cd->next_state));

        switch(cd->next_state) {

        case MAILBOX_OPEN_FOLDER:
		    {
			  char *fmt = XP_GetString( XP_MAIL_READING_FOLDER );
			  char *folder = cd->folder_name;
			  char *s;
			  if ((s = XP_STRRCHR (folder, '/')))
				folder = s+1;
			  s = (char *) XP_ALLOC(XP_STRLEN(fmt) + XP_STRLEN(folder) + 20);
			  if (s)
				{
				  XP_SPRINTF (s, fmt, folder);
				  NET_Progress(ce->window_id, s);
				  XP_FREE(s);
				}
			}
            ce->status = MSG_BeginOpenFolderSock(ce->window_id,
												 cd->folder_name, 
												 cd->msg_id,
												 cd->msgnum,
												 &cd->folder_ptr);

            if(ce->status == MK_CONNECTED)
              {
                cd->next_state = MAILBOX_OPEN_MESSAGE;
              }
            else if(ce->status > -1)
              {
            	cd->pause_for_read = TRUE;
                cd->next_state = MAILBOX_FINISH_OPEN_FOLDER;
              }
            break;

		case MAILBOX_FINISH_OPEN_FOLDER:
            ce->status = MSG_FinishOpenFolderSock(ce->window_id,
												  cd->folder_name, 
												  cd->msg_id,
												  cd->msgnum,
												  &cd->folder_ptr);

            if(ce->status == MK_CONNECTED)
              {
                cd->next_state = MAILBOX_OPEN_MESSAGE;
              }
			else
              {
            	cd->pause_for_read = TRUE;
              }
            break;

       case MAILBOX_OPEN_MESSAGE:
			if(cd->msg_id == NULL && cd->msgnum < 0)
			  {
				/* only open the message if we are actuall
				 * asking for a message
			 	 */
            	cd->next_state = MAILBOX_DONE;
			  }
			else
			  {
				NET_Progress(ce->window_id,
							XP_GetString( XP_MAIL_READING_MESSAGE ) );
            	ce->status = MSG_OpenMessageSock(ce->window_id,
												 cd->folder_name,
												 cd->msg_id,
												 cd->msgnum,
												 cd->folder_ptr, 
												 &cd->msg_ptr,
												 &ce->URL_s->content_length);
            	cd->next_state = MAILBOX_OPEN_STREAM;
			  }
            break;

	   case MAILBOX_OPEN_STREAM:
		 {
		    int status = net_make_mail_msg_stream (ce);
			if (status < 0)
	          {
            	ce->URL_s->error_msg = NET_ExplainErrorDetails(status);
	            ce->status = status;
				return status;
          	  }
			else
			  {
				XP_ASSERT (cd->stream);
            	cd->next_state = MAILBOX_READ_MESSAGE;
			  }

			FE_GraphProgressInit(ce->window_id, ce->URL_s,
								 ce->URL_s->content_length);
			cd->destroy_graph_progress = TRUE;
			cd->original_content_length = ce->URL_s->content_length;
            /* CLM: Set error status so exit routines know about it*/
            ce->status = status;
		 }
		 break;

	   case MAILBOX_READ_MESSAGE:
			{
				int32 read_size;
	
        		cd->pause_for_read  = TRUE;
				read_size = (*cd->stream->is_write_ready)
											(cd->stream->data_object);

				if(!read_size)
      			  {
        			return(0);  /* wait until we are ready to write */
      			  }
    			else
      			  {
        			read_size = MIN(read_size, NET_Socket_Buffer_Size);
      			  }

            	ce->status = MSG_ReadMessageSock(ce->window_id,
												 cd->folder_name,
												 cd->msg_ptr,
												 cd->msg_id,
												 cd->msgnum,
												 NET_Socket_Buffer, 
												 read_size);

				if(ce->status > 0)
				  {
					ce->bytes_received += ce->status;
					FE_GraphProgress(ce->window_id, ce->URL_s,
									 ce->bytes_received, ce->status,
									 ce->URL_s->content_length);

					ce->status = PUT_STREAM(NET_Socket_Buffer, ce->status);
				  }
				else if(ce->status == 0)
					cd->next_state = MAILBOX_DONE;
		    }
            break;
			
		case MAILBOX_EMPTY_TRASH:
			NET_Progress(ce->window_id,
						 XP_GetString( XP_MAIL_EMPTYING_TRASH ) );
			ce->status = MSG_BeginEmptyTrash(ce->window_id, ce->URL_s,
											 &cd->empty_trash_ptr);
			if (ce->status == MK_CONNECTED) {
			  cd->next_state = MAILBOX_COMPRESS_FOLDER;
			} else {
			  cd->pause_for_read = TRUE;
			  cd->next_state = MAILBOX_FINISH_EMPTY_TRASH;
			}
			break;

		case MAILBOX_FINISH_EMPTY_TRASH:
			ce->status = MSG_FinishEmptyTrash(ce->window_id, ce->URL_s,
											  cd->empty_trash_ptr);
			if (ce->status == MK_CONNECTED) {
			  cd->next_state = MAILBOX_COMPRESS_FOLDER;
			} else {
			  cd->pause_for_read = TRUE;
			}
			break;

		case MAILBOX_COMPRESS_FOLDER:
			if (cd->initial_state == MAILBOX_COMPRESS_FOLDER)
			  {
				char *fmt= XP_GetString( XP_COMPRESSING_FOLDER );
				char *folder = cd->folder_name;
				char *s;
				if ((s = XP_STRRCHR (folder, '/')))
				  folder = s+1;
				s = (char *)XP_ALLOC (XP_STRLEN(fmt) + XP_STRLEN(folder) + 20);
				if (s)
				  {
					XP_SPRINTF (s, fmt, folder);
					NET_Progress(ce->window_id, s);
					XP_FREE(s);
				  }
			  }
			ce->status = MSG_BeginCompressFolder(ce->window_id, ce->URL_s,
												 cd->folder_name,
												 &cd->compress_folder_ptr);
			if (ce->status == MK_CONNECTED) {
			  cd->next_state = MAILBOX_DONE;
			} else {
			  cd->pause_for_read = TRUE;
			  cd->next_state = MAILBOX_FINISH_COMPRESS_FOLDER;
			}
			break;
		  
		case MAILBOX_FINISH_COMPRESS_FOLDER:
			ce->status = MSG_FinishCompressFolder(ce->window_id, ce->URL_s,
												  cd->folder_name,
												  cd->compress_folder_ptr);
			if (ce->status == MK_CONNECTED) {
			  cd->next_state = MAILBOX_DONE;
			} else {
			  cd->pause_for_read = TRUE;
			}
			break;

		case MAILBOX_DELIVER_QUEUED:
			NET_Progress(ce->window_id,
						XP_GetString( XP_MAIL_DELIVERING_QUEUED_MESSAGES ) );
			ce->status = MSG_BeginDeliverQueued(ce->window_id, ce->URL_s,
												&cd->deliver_queued_ptr);
			if (ce->status == MK_CONNECTED) {
			  cd->next_state = MAILBOX_DONE;
			} else {
			  cd->pause_for_read = TRUE;
			  cd->next_state = MAILBOX_FINISH_DELIVER_QUEUED;
			}
			break;

		case MAILBOX_FINISH_DELIVER_QUEUED:
			ce->status = MSG_FinishDeliverQueued(ce->window_id, ce->URL_s,
												 cd->deliver_queued_ptr);
			if (ce->status == MK_CONNECTED) {
			  cd->next_state = MAILBOX_DONE;
			} else {
			  cd->pause_for_read = TRUE;
			}
			break;

        case MAILBOX_DONE:
			if(cd->stream)
			  {
				COMPLETE_STREAM;
				FREE(cd->stream);
			  }
            cd->next_state = MAILBOX_FREE;
            break;
        
        case MAILBOX_ERROR_DONE:
			if(cd->stream)
			  {
				ABORT_STREAM(ce->status);
				FREE(cd->stream);
			  }
            cd->next_state = MAILBOX_FREE;

			if (ce->status < -1 && !ce->URL_s->error_msg)
			  ce->URL_s->error_msg = NET_ExplainErrorDetails(ce->status);

            break;
        
        case MAILBOX_FREE:
        	net_call_all_the_time_count--;
        	if(net_call_all_the_time_count < 1)
           		FE_ClearCallNetlibAllTheTime(ce->window_id);

			if(cd->destroy_graph_progress)
			  FE_GraphProgressDestroy(ce->window_id, 
									  ce->URL_s, 
									  cd->original_content_length,
									  ce->bytes_received);

			if(cd->msg_ptr)
			  {
            	MSG_CloseMessageSock(ce->window_id, cd->folder_name,
									 cd->msg_id, cd->msgnum, cd->msg_ptr);
				cd->msg_ptr = NULL;
				NET_Progress(ce->window_id,
							XP_GetString( XP_READING_MESSAGE_DONE ) );
			  }

			if(cd->folder_ptr)
			  {
            	MSG_CloseFolderSock(ce->window_id, cd->folder_name,
									cd->msg_id, cd->msgnum, cd->folder_ptr);
				if (cd->msg_id == NULL && cd->msgnum < 0)
				  /* If we read a message, don't hide the previous message
					 with this one. */
				  NET_Progress(ce->window_id,
							  XP_GetString( XP_MAIL_READING_FOLDER_DONE ) );
			  }

			if(cd->empty_trash_ptr)
			  {
            	MSG_CloseEmptyTrashSock(ce->window_id, ce->URL_s,
										cd->empty_trash_ptr);
				cd->empty_trash_ptr = NULL;
				NET_Progress(ce->window_id,
							XP_GetString( XP_MAIL_EMPTYING_TRASH_DONE ) );
			  }
			if(cd->compress_folder_ptr)
			  {
            	MSG_CloseCompressFolderSock(ce->window_id, ce->URL_s,
											cd->compress_folder_ptr);
				cd->compress_folder_ptr = NULL;
				if (cd->initial_state == MAILBOX_COMPRESS_FOLDER)
				  NET_Progress(ce->window_id,
							 XP_GetString( XP_MAIL_COMPRESSING_FOLDER_DONE ) );
			  }
			if(cd->deliver_queued_ptr)
			  {
            	MSG_CloseDeliverQueuedSock(ce->window_id, ce->URL_s,
										   cd->deliver_queued_ptr);
				cd->deliver_queued_ptr = NULL;
				NET_Progress(ce->window_id,
					XP_GetString( XP_MAIL_DELIVERING_QUEUED_MESSAGES_DONE ) );
			  }

			FREEIF(cd->folder_name);
			FREEIF(cd->msg_id);
            FREE(cd);

            return(-1); /* final end */
        
        default: /* should never happen !!! */
            TRACEMSG(("MAILBOX: BAD STATE!"));
			XP_ASSERT(0);
            cd->next_state = MAILBOX_ERROR_DONE;
            break;
        }

        /* check for errors during load and call error 
         * state if found
         */
        if(ce->status < 0 && cd->next_state != MAILBOX_FREE)
          {
            cd->next_state = MAILBOX_ERROR_DONE;
            /* don't exit! loop around again and do the free case */
            cd->pause_for_read = FALSE;
          }
      } /* while(!cd->pause_for_read) */
    
    return(ce->status);
}

/* abort the connection in progress
 */
MODULE_PRIVATE int
NET_InterruptMailbox(ActiveEntry * ce)
{
    MailboxConData * cd = (MailboxConData *)ce->con_data;

    cd->next_state = MAILBOX_ERROR_DONE;
    ce->status = MK_INTERRUPTED;
 
    return(NET_ProcessMailbox(ce));
}

/* Free any memory that might be used in caching etc.
 */
MODULE_PRIVATE void
NET_CleanupMailbox(void)
{
    /* nothing so far needs freeing */
    return;
}
