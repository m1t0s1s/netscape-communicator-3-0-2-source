/* -*- Mode: C; tab-width: 4 -*-
 * state machine to speak pop3
 *
 * Lou Montulli
 */

/* Please leave outside of ifdef for windows precompiled headers */
#include "mkutils.h"

#ifdef MOZILLA_CLIENT

#include "mkgeturl.h"
#include "mkpop3.h"
#include "xp_hash.h"
#include "merrors.h"
#include "msgcom.h"
#include "secnav.h"
#include "ssl.h"

#include "xp_error.h"
#include "xpgetstr.h"

#include "prefapi.h"

#ifdef AUTH_SKEY_DEFINED
extern int btoa8(char *out, char*in);
#endif

extern int MK_OUT_OF_MEMORY;
extern int MK_POP3_DELE_FAILURE;
extern int MK_POP3_LIST_FAILURE;
extern int MK_POP3_MESSAGE_WRITE_ERROR;
extern int MK_POP3_NO_MESSAGES;
extern int MK_POP3_OUT_OF_DISK_SPACE;
extern int MK_POP3_PASSWORD_FAILURE;
extern int MK_POP3_PASSWORD_UNDEFINED;
extern int MK_POP3_RETR_FAILURE;
extern int MK_POP3_SERVER_ERROR;
extern int MK_POP3_USERNAME_FAILURE;
extern int MK_POP3_USERNAME_UNDEFINED;
extern int MK_TCP_READ_ERROR;
extern int MK_TCP_WRITE_ERROR;
extern int XP_ERRNO_EWOULDBLOCK;
extern int XP_NO_ANSWER;
extern int XP_THE_POP3_SERVER_DOES_NOT_SUPPORT_UIDL_ETC;
extern int XP_RECEIVING_MESSAGE_OF;
extern int XP_THE_POP3_SERVER_DOES_NOT_SUPPORT_THE_TOP_COMMAND;
extern int XP_THE_PREVIOUSLY_ENTERED_PASSWORD_IS_INVALID_ETC;
extern int XP_CONNECT_HOST_CONTACTED_SENDING_LOGIN_INFORMATION;
extern int XP_PASSWORD_FOR_POP3_USER;


#define POP3_PORT 110  /* the iana port for pop3 */
#define OUTPUT_BUFFER_SIZE 512  /* max size of command string */

/* Globals */
PRIVATE XP_Bool net_leave_mail_on_server=FALSE;
PRIVATE char *net_pop3_username=0;
PRIVATE char *net_pop3_password=0; /* obfuscated pop3 password */
PRIVATE XP_Bool net_pop3_password_pending=FALSE;
PRIVATE int32 net_pop3_size_limit=-1;  /* negative means infinite */

#ifdef AUTH_SKEY_DEFINED
/* SKEY Authentication */
PRIVATE XP_Bool net_pop3_auth_skey_enabled=FALSE;
PRIVATE int net_pop3_auth_skey_sequence=0;
PRIVATE char *net_pop3_auth_skey_seed=NULL;
#endif

/* We set this if we find that the UIDL command doesn't work.
   It is reset to FALSE if the user name is re-set (a cheap way
   of seeing if the server has changed...)
 */
PRIVATE XP_Bool net_uidl_command_unimplemented = FALSE;
PRIVATE XP_Bool net_xtnd_xlst_command_unimplemented = FALSE;
PRIVATE XP_Bool net_top_command_unimplemented = FALSE;



/* definitions of state for the state machine design
 */
typedef enum {
    POP3_READ_PASSWORD,
    POP3_START_CONNECT,
    POP3_FINISH_CONNECT,
	POP3_WAIT_FOR_RESPONSE,
	POP3_WAIT_FOR_START_OF_CONNECTION_RESPONSE,
	POP3_SEND_USERNAME,
	POP3_SEND_PASSWORD,
	POP3_SEND_STAT,
	POP3_GET_STAT,
	POP3_SEND_LIST,
	POP3_GET_LIST,
	POP3_SEND_UIDL_LIST,
	POP3_GET_UIDL_LIST,
	POP3_SEND_XTND_XLST_MSGID,
	POP3_GET_XTND_XLST_MSGID,
	POP3_GET_MSG,
	POP3_SEND_TOP,
	POP3_TOP_RESPONSE,
	POP3_SEND_RETR,
	POP3_RETR_RESPONSE,
	POP3_SEND_DELE,
	POP3_DELE_RESPONSE,
	POP3_SEND_QUIT,
    POP3_DONE,
    POP3_ERROR_DONE,
    POP3_FREE,
    /* The following 3 states support the use of the 'TOP' command instead of UIDL
       for leaving mail on the pop server -km */
    POP3_START_USE_TOP_FOR_FAKE_UIDL, 
    POP3_SEND_FAKE_UIDL_TOP, 
    POP3_GET_FAKE_UIDL_TOP,
#ifdef AUTH_SKEY_DEFINED
	/* SKEY Authentication */
	POP3_AUTH_SKEY,
	POP3_AUTH_SKEY_RESPONSE,
	POP3_AUTH_SKEY_SEQUENCE_N_SEED,
#endif
	POP3_SEND_AUTH,
	POP3_AUTH_RESPONSE,
	POP3_AUTH_LOGIN,
	POP3_AUTH_LOGIN_RESPONSE,
	POP3_SEND_XSENDER,
	POP3_XSENDER_RESPONSE
} Pop3StatesEnum;

/* structure to hold data pertaining to the active state of
 * a transfer in progress.
 *
 */


#define KEEP		'k'			/* If we want to keep this item on server. */
#define DELETE_CHAR	'd'			/* If we want to delete this item. */

typedef struct Pop3AllocedString { /* Need this silliness as a place to
									  keep the strings that are allocated
									  for the keys in the hash table. ### */
  char* str;
  struct Pop3AllocedString* next;
} Pop3AllocedString;

typedef struct Pop3UidlHost {
  char* host;
  char* user;
  XP_HashTable hash;
  Pop3AllocedString* strings;
  struct Pop3UidlHost* next;
} Pop3UidlHost;


typedef struct Pop3MsgInfo {
  int32 size;
  char* uidl;
} Pop3MsgInfo;

typedef struct _Pop3ConData {
    Pop3StatesEnum  next_state;       		/* the next state or action to be taken */
    Pop3StatesEnum  next_state_after_response;  
    Bool     	pause_for_read;   		/* Pause now for next read? */
	
	XP_Bool         command_succeeded;      /* did the last command succeed? */
	char           *command_response;		/* text of last response */
	int32           first_msg;
	TCP_ConData *tcp_con_data;  /* Data pointer for tcp connect state machine */
	char  		   *data_buffer;
	int32		    data_buffer_size;

    char *obuffer;		/* line buffer for output to msglib */
    uint32 obuffer_size;
    uint32 obuffer_fp;

	int32           number_of_messages;
	Pop3MsgInfo	   *msg_info;	/* Message sizes and uidls (used only if we
								   are playing games that involve leaving
								   messages on the server). */
	int32           last_accessed_msg;
	int32           cur_msg_size;
	char           *output_buffer;
	XP_Bool         truncating_cur_msg;     /* are we using top and uidl? */
	XP_Bool         msg_del_started;        /* True if MSG_BeginMailDel...
											 * called
											 */
	XP_Bool         only_check_for_new_mail;
  	MSG_BIFF_STATE	biffstate;	/* If just checking for, what the answer is. */

	XP_Bool         password_failed;        /* flag for password querying */
	void 		   *msg_closure;
	int32			bytes_received_in_message; 
	int32			total_folder_size;

  	int32			total_download_size; /* Number of bytes we're going to
											download.  Might be much less
											than the total_folder_size. */

    XP_Bool  graph_progress_bytes_p;	/* whether we should display info about
										   the bytes transferred (usually we
										   display info about the number of
										   messages instead.) */

    Pop3UidlHost   *uidlinfo;
    XP_HashTable	newuidl;
	char           *only_uidl;	/* If non-NULL, then load only this UIDL. */
	
								/* the following three fields support the 
								   use of the 'TOP' command instead of UIDL
       							   for leaving mail on the pop server -km */
       
								/* the current message that we are retrieving 
								   with the TOP command */
   int32 current_msg_to_top;	
   	
   												/* we will download this many in 
   												   POP3_GET_MSG */							   
   int32 number_of_messages_not_seen_before;
    											/* reached when we have TOPped 
    											   all of the new messages */
   XP_Bool found_new_message_boundary; 
   
   												/* if this is true then we don't stop 
   												   at new message boundary, we have to 
   												   TOP em all to delete some */
   XP_Bool delete_server_message_during_top_traversal;
   XP_Bool sender_authed;				/* reset per message retrieved */
   XP_Bool auth_login_enabled;
} Pop3ConData;





PUBLIC void
NET_LeavePopMailOnServer(Bool do_it)
{
	net_leave_mail_on_server = do_it;
}

PUBLIC void
NET_SetPopUsername(const char *username)
{
	char *at = NULL;

	StrAllocCopy(net_pop3_username, username);
	net_uidl_command_unimplemented = FALSE;
	net_top_command_unimplemented = FALSE;
	net_xtnd_xlst_command_unimplemented = FALSE;
	/*
	** If we are called with data like "fred@bedrock.com", then we will
	** help the user by ignoring the stuff after the "@".  People with
	** @ signs in their user names will be hosed.  They also can't possibly
	** be current happy internet users.  This will waste a few bytes,
	** but was the minimal change to make in order to ship cheddar.
	** (it might even save bytes by avoiding the code which does the
	** right thing)
	*/
	if (net_pop3_username != NULL) at = XP_STRCHR(net_pop3_username, '@');
	if (at != NULL) *at = '\0';
}

PUBLIC const char*
NET_GetPopUsername(void)
{
  return net_pop3_username;
}

#ifndef HAVE_SECURITY /* added by jwz */
static unsigned char rot13[256] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
  21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
  40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58,
  59, 60, 61, 62, 63, 64, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
  65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 91, 92, 93, 94, 95, 96,
  110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 97, 98, 99,
  100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 123, 124, 125, 126, 127,
  128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142,
  143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157,
  158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172,
  173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187,
  188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 202,
  203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217,
  218, 219, 220, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232,
  233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247,
  248, 249, 250, 251, 252, 253, 254, 255 
};

#define SECNAV_MungeString rotate_string
#define SECNAV_UnMungeString unrotate_string

static unsigned char * SECNAV_MungeString(unsigned char *s)
{
  unsigned char *ss = strdup(s);
  s = ss;
  while (*s) {
	  *s = rot13[*s];
	  s++;
  }
  return ss;
}

static unsigned char * SECNAV_UnMungeString(unsigned char *s)
{
  unsigned char *ss = strdup(s);
  s = ss;
  while (*s) {
	  *s = rot13[*s];
	  s++;
  }
  return ss;
}
#endif /* !HAVE_SECURITY -- added by jwz */


PRIVATE void
net_set_pop3_password(const char *password)
{
    FREEIF(net_pop3_password);
    net_pop3_password = (char *) SECNAV_MungeString((unsigned char *) password);
}

PRIVATE char *
net_get_pop3_password(void)
{
	return (char *) SECNAV_UnMungeString((unsigned char *) net_pop3_password);
}

PUBLIC void
NET_SetPopPassword(const char *password)
{
	StrAllocCopy(net_pop3_password, password);
}

PUBLIC const char *
NET_GetPopPassword(void)
{
  return net_get_pop3_password();
}

PUBLIC void
NET_SetPopPassword2(const char *password)
{
	net_set_pop3_password(password);
}

/* sets the size limit for pop3 messages
 *
 * set the size negative to make it infinite length
 */
PUBLIC void
NET_SetPopMsgSizeLimit(int32 size)
{
  if (size < 0) size = -1;
  else if (size < 2048) size = 2048;

  net_pop3_size_limit = size;
}

PUBLIC int32
NET_GetPopMsgSizeLimit(void)
{
  return net_pop3_size_limit;
}


static int
uidl_cmp (const void *obj1, const void *obj2)
{
  XP_ASSERT (obj1 && obj2);
  return XP_STRCMP ((char *) obj1, (char *) obj2);
}


static void
put_hash(Pop3UidlHost* host, XP_HashTable table, const char* key, char value)
{
  Pop3AllocedString* tmp;
  int v = value;
  tmp = XP_NEW_ZAP(Pop3AllocedString);
  if (tmp) {
	tmp->str = XP_STRDUP(key);
	if (tmp->str) {
	  tmp->next = host->strings;
	  host->strings = tmp;
	  XP_Puthash(table, tmp->str, (void*) v);
	} else {
	  XP_FREE(tmp);
	}
  }
}

PRIVATE Pop3UidlHost* net_pop3_load_state(const char* searchhost,
										  const char* searchuser)
{
  XP_File file;
  char* buf;
  char* host;
  char* user;
  char* uidl;
  char* flags;
  Pop3UidlHost* result = NULL;
  Pop3UidlHost* current = NULL;
  Pop3UidlHost* tmp;
  result = XP_NEW_ZAP(Pop3UidlHost);
  if (!result) return NULL;
  result->host = XP_STRDUP(searchhost);
  result->user = XP_STRDUP(searchuser);
  result->hash = XP_HashTableNew(20, XP_StringHash, uidl_cmp);
  if (!result->host || !result->user || !result->hash) {
	FREEIF(result->host);
	FREEIF(result->user);
	if (result->hash) XP_HashTableDestroy(result->hash);
	XP_FREE(result);
	return NULL;
  }
  file = XP_FileOpen("", xpMailPopState, XP_FILE_READ);
  if (!file) return result;
  buf = (char*)XP_ALLOC(512);
  if (buf) {
	while (XP_FileReadLine(buf, 512, file)) {
	  if (*buf == '#' || *buf == CR || *buf == LF || *buf == 0)
		continue;
	  if (buf[0] == '*') {
		/* It's a host&user line. */
		current = NULL;
		host = XP_STRTOK(buf + 1, " \t" LINEBREAK);
		user = XP_STRTOK(NULL, " \t" LINEBREAK);
		if (host == NULL || user == NULL) continue;
		for (tmp = result ; tmp ; tmp = tmp->next) {
		  if (XP_STRCMP(host, tmp->host) == 0 &&
			  XP_STRCMP(user, tmp->user) == 0) {
			current = tmp;
			break;
		  }
		}
		if (!current) {
		  current = XP_NEW_ZAP(Pop3UidlHost);
		  if (current) {
			current->host = XP_STRDUP(host);
			current->user = XP_STRDUP(user);
			current->hash = XP_HashTableNew(20, XP_StringHash, uidl_cmp);
			if (!current->host || !current->user || !current->hash) {
			  FREEIF(current->host);
			  FREEIF(current->user);
			  if (current->hash) XP_HashTableDestroy(current->hash);
			  XP_FREE(current);
			} else {
			  current->next = result->next;
			  result->next = current;
			}
		  }
		}
	  } else {
		/* It's a line with a UIDL on it. */
		if (current) {
		  flags = XP_STRTOK(buf, " \t" LINEBREAK);
		  uidl = XP_STRTOK(NULL, " \t" LINEBREAK);
		  if (flags && uidl) {
			XP_ASSERT(flags[0] == KEEP || flags[0] == DELETE_CHAR);
			if (flags[0] == KEEP || flags[0] == DELETE_CHAR) {
			  put_hash(current, current->hash, uidl, flags[0]);
			}
		  }
		}
	  }
	}
	XP_FREE(buf);
  }
  XP_FileClose(file);
  return result;
}



static XP_Bool
hash_empty_mapper(XP_HashTable hash, const void* key, void* value,
				  void* closure)
{
  *((XP_Bool*) closure) = FALSE;
  return FALSE;
}

static XP_Bool
hash_empty(XP_HashTable hash)
{
  XP_Bool result = TRUE;
  XP_Maphash(hash, hash_empty_mapper, &result);
  return result;
}


PRIVATE XP_Bool
net_pop3_write_mapper(XP_HashTable hash, const void* key, void* value,
					  void* closure)
{
  XP_File file = (XP_File) closure;
  XP_ASSERT(value == ((void *) (int) KEEP) ||
			value == ((void *) (int) DELETE_CHAR));
  XP_FilePrintf(file, "%c %s" LINEBREAK, (char)(long)value, (char*) key);
  return TRUE;
}


PRIVATE void
net_pop3_write_state(Pop3UidlHost* host)
{
  XP_File file;
  file = XP_FileOpen("", xpMailPopState, XP_FILE_WRITE_BIN);
  if (!file) return;
  XP_FileWrite("# Netscape POP3 State File" LINEBREAK
			   "# This is a generated file!  Do not edit." LINEBREAK LINEBREAK,
			   -1, file);
  for (; host ; host = host->next) {
	if (!hash_empty(host->hash)) {
	  XP_FileWrite("*", 1, file);
	  XP_FileWrite(host->host, -1, file);
	  XP_FileWrite(" ", 1, file);
	  XP_FileWrite(host->user, -1, file);
	  XP_FileWrite(LINEBREAK, LINEBREAK_LEN, file);
	  XP_Maphash(host->hash, net_pop3_write_mapper, file);
	}
  }
  XP_FileClose(file);
}


PRIVATE void
net_pop3_free_state(Pop3UidlHost* host) 
{
  Pop3UidlHost* h;
  Pop3AllocedString* tmp;
  Pop3AllocedString* next;
  while (host) {
	h = host->next;
	XP_FREE(host->host);
	XP_FREE(host->user);
	XP_HashTableDestroy(host->hash);
	tmp = host->strings;
	while (tmp) {
	  next = tmp->next;
	  XP_FREE(tmp->str);
	  XP_FREE(tmp);
	  tmp = next;
	}
	XP_FREE(host);
	host = h;
  }
}



PRIVATE void
net_pop3_free_msg_info(ActiveEntry* ce)
{
  Pop3ConData * cd = (Pop3ConData *)ce->con_data;
  int i;
  if (cd->msg_info) {
	for (i=0 ; i<cd->number_of_messages ; i++) {
	  if (cd->msg_info[i].uidl) XP_FREE(cd->msg_info[i].uidl);
	}
	XP_FREE(cd->msg_info);
	cd->msg_info = NULL;
  }
}	

/* km
	This function will read one line and only go on to the next state
	if the first character of that line is a '+' character.  This will
	accomodate pop mail servers that print a banner when the client 
	initiates a connection.  Bug #7165
*/
PRIVATE int
net_pop3_wait_for_start_of_connection_response(ActiveEntry * ce)
{
	Pop3ConData * cd = (Pop3ConData *)ce->con_data;
    char * line;

    ce->status = NET_BufferedReadLine(ce->socket, &line, &cd->data_buffer,
                        &cd->data_buffer_size, &cd->pause_for_read);

    if(ce->status == 0)
      {
        /* this shouldn't really happen, but...
         */
        cd->next_state = cd->next_state_after_response;
        cd->pause_for_read = FALSE; /* don't pause */
        return(0);
      }
    else if(ce->status < 0)
      {
        int rv = SOCKET_ERRNO;

        if (rv == XP_ERRNO_EWOULDBLOCK)
          {
            cd->pause_for_read = TRUE;
            return(0);
          }

        TRACEMSG(("TCP Error: %d", rv));

        ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, rv);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
      }
    else if(!line)
      {
         return ce->status; /* wait for line */
      }

    TRACEMSG(("    Rx: %s", line));

	if(*line == '+')
	  {
		cd->command_succeeded = TRUE;
		if(XP_STRLEN(line) > 4)
			StrAllocCopy(cd->command_response, line+4);
		else
			StrAllocCopy(cd->command_response, line);
			
    	cd->next_state = cd->next_state_after_response;
   		cd->pause_for_read = FALSE; /* don't pause */
	  }
	 

    return(1);  /* everything ok */
}

PRIVATE int
net_pop3_wait_for_response(ActiveEntry * ce)
{
	Pop3ConData * cd = (Pop3ConData *)ce->con_data;
    char * line;

    ce->status = NET_BufferedReadLine(ce->socket, &line, &cd->data_buffer,
                        &cd->data_buffer_size, &cd->pause_for_read);

    if(ce->status == 0)
      {
        /* this shouldn't really happen, but...
         */
        cd->next_state = cd->next_state_after_response;
        cd->pause_for_read = FALSE; /* don't pause */
        return(0);
      }
    else if(ce->status < 0)
      {
        int rv = SOCKET_ERRNO;

        if (rv == XP_ERRNO_EWOULDBLOCK)
          {
            cd->pause_for_read = TRUE;
            return(0);
          }

        TRACEMSG(("TCP Error: %d", rv));

        ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, rv);

        /* return TCP error
         */
        return MK_TCP_READ_ERROR;
      }
    else if(!line)
      {
         return ce->status; /* wait for line */
      }

    TRACEMSG(("    Rx: %s", line));

	if(*line == '+')
	  {
		cd->command_succeeded = TRUE;
#ifdef AUTH_SKEY_DEFINED
		if (net_pop3_auth_skey_enabled &&
			cd->next_state_after_response == POP3_AUTH_SKEY_SEQUENCE_N_SEED && 
			XP_STRLEN(line+2))
			StrAllocCopy(cd->command_response, line+2);
		else if (XP_STRLEN(line) > 4)
#else
		if(XP_STRLEN(line) > 4)
#endif
			StrAllocCopy(cd->command_response, line+4);
		else
			StrAllocCopy(cd->command_response, line);
	  }
	else
	  {
		cd->command_succeeded = FALSE;
		if(XP_STRLEN(line) > 5)
			StrAllocCopy(cd->command_response, line+5);
		else
			StrAllocCopy(cd->command_response, line);
	  }

    cd->next_state = cd->next_state_after_response;
    cd->pause_for_read = FALSE; /* don't pause */

    return(1);  /* everything ok */
}

PRIVATE int
net_pop3_error(ActiveEntry *ce, int err_code)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;

#ifdef DEBUG_jwz
	if (err_code != MK_POP3_NO_MESSAGES)
#endif
	ce->URL_s->error_msg = NET_ExplainErrorDetails(err_code, 
							cd->command_response ? cd->command_response :
							XP_GetString( XP_NO_ANSWER ) );

	cd->next_state = POP3_ERROR_DONE;
	cd->pause_for_read = FALSE;

	return(err_code);
}

PRIVATE int
net_pop3_send_command(ActiveEntry *ce, const char * command)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;
	int status;

    status = (int) NET_BlockingWrite(ce->socket,
										command,
										XP_STRLEN(command));

    TRACEMSG(("Pop3 Tx: %s", cd->output_buffer));

	if(status < 0)
	  {
		ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_TCP_WRITE_ERROR, SOCKET_ERRNO);
		cd->next_state = POP3_ERROR_DONE;
		return(MK_TCP_WRITE_ERROR);
	  }

	cd->pause_for_read = TRUE;
	cd->next_state = POP3_WAIT_FOR_RESPONSE;

    return(status);
}

/*
 * POP3 AUTH LOGIN extention
 */

PRIVATE int
net_pop3_send_auth(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	/* check login response */
	if(!cd->command_succeeded)
		return(net_pop3_error(ce, MK_POP3_SERVER_ERROR));

	PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "AUTH" CRLF);

	cd->next_state_after_response = POP3_AUTH_RESPONSE;

	return net_pop3_send_command(ce, cd->output_buffer);
}

PRIVATE	int
net_pop3_auth_response(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;
    char * line;

	cd->next_state = POP3_SEND_USERNAME;

	if (!cd->command_succeeded) {
		/* AUTH command not implemented 
		 * no base64 encoded username/password
		 */
		cd->command_succeeded = TRUE;
		cd->auth_login_enabled = FALSE;
		return 0;
	}

	while (1) {
		ce->status = NET_BufferedReadLine(ce->socket, &line, &cd->data_buffer,
                        &cd->data_buffer_size, &cd->pause_for_read);

		if(ce->status == 0)
		  {
			/* this shouldn't really happen, but...
			 */
			cd->next_state = cd->next_state_after_response;
			cd->pause_for_read = FALSE; /* don't pause */
			return(0);
		  }
		else if(ce->status < 0)
		  {
			int rv = SOCKET_ERRNO;

			if (rv == XP_ERRNO_EWOULDBLOCK)
			  {
				cd->pause_for_read = TRUE;
				return(0);
			  }

			TRACEMSG(("TCP Error: %d", rv));

			ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, rv);

			/* return TCP error
			 */
			return MK_TCP_READ_ERROR;
		  }
		else if(!line)
		  {
			 /* ????? Is this right?? */
			 return ce->status; /* wait for line */
		  }

		TRACEMSG(("    Rx: %s", line));

		if(*line == '.') {
			return 0;
		}
		else {
			if (!XP_STRCASECMP (line, "LOGIN")) {
				cd->next_state = POP3_AUTH_LOGIN;
			}
		}
	}
}

PRIVATE int
net_pop3_auth_login(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	/* check login response */
	if(!cd->command_succeeded)
		return(net_pop3_error(ce, MK_POP3_SERVER_ERROR));

	PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "AUTH LOGIN" CRLF);

	cd->next_state_after_response = POP3_AUTH_LOGIN_RESPONSE;

	return net_pop3_send_command(ce, cd->output_buffer);

}

PRIVATE int
net_pop3_auth_login_response(ActiveEntry *ce)
{
  Pop3ConData * cd = (Pop3ConData *)ce->con_data;

  if (!cd->command_succeeded) 
	{
	  /* sounds like server does not support auth-skey extension
		 resume regular logon process */
	  /* reset command_succeeded to true */
	  cd->command_succeeded = TRUE;
	  /* reset auth login state */
	  cd->auth_login_enabled = FALSE;
	}
  else
	{
	  cd->auth_login_enabled = TRUE;
	}
  cd->next_state = POP3_SEND_USERNAME;
  return 0;
}

#ifdef AUTH_SKEY_DEFINED
PRIVATE int
net_pop3_auth_skey(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	/* check login response */
	if(!cd->command_succeeded)
		return(net_pop3_error(ce, MK_POP3_SERVER_ERROR));

	PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "AUTH SKEY" CRLF);

	cd->next_state_after_response = POP3_AUTH_SKEY_RESPONSE;

	return net_pop3_send_command(ce, cd->output_buffer);
}

PRIVATE int
net_pop3_auth_skey_response(ActiveEntry *ce)
{
  Pop3ConData * cd = (Pop3ConData *)ce->con_data;

  if (!cd->command_succeeded) 
	{
	  /* sounds like server does not support auth-skey extension
		 resume regular logon process */
	  /* reset command_succeeded to true */
	  cd->command_succeeded = TRUE;
	  /* reset auth skey state */
	  net_pop3_auth_skey_enabled = FALSE;
	  net_pop3_auth_skey_sequence = 0;
	  XP_FREEIF (net_pop3_auth_skey_seed);
	}
  else
	{
	  net_pop3_auth_skey_enabled = TRUE;
	}
  cd->next_state = POP3_SEND_USERNAME;
  return 0;
}

PRIVATE int
net_pop3_auth_skey_sequence_n_seed(ActiveEntry *ce)
{
  Pop3ConData * cd = (Pop3ConData *) ce->con_data;

  if (cd->command_succeeded)
	{
	  char * str = 
		NET_Base64Decode (cd->command_response,
						  XP_STRLEN(cd->command_response));
	  if (str)
		{
		  char seed[256];
		  XP_MEMSET(seed, 0, 256);
		  sscanf (str, "%d %s", &net_pop3_auth_skey_sequence, seed);
		  if (*seed)
			  StrAllocCopy (net_pop3_auth_skey_seed, seed);
		  XP_FREEIF(str);
		  cd->next_state = POP3_SEND_PASSWORD;
		  return 0;
		}
	}
  /* auth failed for some odd reason
	 reset the state and then start over again
   */
  net_pop3_auth_skey_enabled = FALSE;
  XP_STRCPY(cd->output_buffer, "*" CRLF);
  cd->next_state_after_response = POP3_SEND_USERNAME;
  return net_pop3_send_command(ce, cd->output_buffer);
}

PRIVATE int
net_pop3_crunch_skey_password(ActiveEntry *ce, char *password)
{
  int n;
  char key[8];
  char hexKey[16];
  char *b64Str = NULL;

  if (!net_pop3_auth_skey_seed || !password)
	  return -1;
  if (keycrunch(key, net_pop3_auth_skey_seed, password) != 0)
	/* ***NOTE*** may need a real error code */
	return -1;
  n = net_pop3_auth_skey_sequence;
  if (n <= 0) 
	/* ***NOTE*** may need a real error code */
	return -1;
  while(n-- != 0)
	f(key);

  btoa8(hexKey, key);

  b64Str = NET_Base64Encode(hexKey, 16);
  if (b64Str)
	{
	  Pop3ConData *cd = (Pop3ConData *)ce->con_data;
	  PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "%.256s" CRLF,
				  b64Str);
	  XP_FREEIF(b64Str);
	  return 0;
	}
  return -1;
}
#endif

PRIVATE int
net_pop3_send_username(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	/* check login response */
	if(!cd->command_succeeded)
		return(net_pop3_error(ce, MK_POP3_SERVER_ERROR));

	if(!net_pop3_username || !*net_pop3_username)
		return(net_pop3_error(ce, MK_POP3_USERNAME_UNDEFINED));

#ifdef AUTH_SKEY_DEFINED
	if (net_pop3_auth_skey_enabled)
	  {
		char * str = 
		  NET_Base64Encode(net_pop3_username, 
						   XP_STRLEN(net_pop3_username));
		if (str)
		  {
			PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "%.256s" CRLF,
						str);
			XP_FREEIF(str);

			cd->next_state_after_response = POP3_AUTH_SKEY_SEQUENCE_N_SEED;
		  }
		else
		  {
			return (net_pop3_error(ce, MK_POP3_USERNAME_UNDEFINED));
		  }
	  }
	else 
	  {
		PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, 
					"USER %.256s" CRLF, net_pop3_username);

		cd->next_state_after_response = POP3_SEND_PASSWORD;
	  }
#else
/*	if (cd->auth_login_enabled) {
		char * str = 
		  NET_Base64Encode(net_pop3_username, 
						   XP_STRLEN(net_pop3_username));
		if (str)
		  {
			PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "%.256s" CRLF,
						str);
			XP_FREEIF(str);
		  }
		else
		  {
			return (net_pop3_error(ce, MK_POP3_USERNAME_UNDEFINED));
		  }
	}
	else*/ {
		PR_snprintf(cd->output_buffer, 
					OUTPUT_BUFFER_SIZE, 
					"USER %.256s" CRLF, net_pop3_username);
	}

	cd->next_state_after_response = POP3_SEND_PASSWORD;
#endif

	return(net_pop3_send_command(ce, cd->output_buffer));
}

PRIVATE int
net_pop3_send_password(ActiveEntry *ce)
{
	Pop3ConData * cd = (Pop3ConData *)ce->con_data;
	char *password;

	/* check username response */
	if (!cd->command_succeeded)
		return(net_pop3_error(ce, MK_POP3_USERNAME_FAILURE));

	password = net_get_pop3_password();

	if (password == NULL)
		return(net_pop3_error(ce, MK_POP3_PASSWORD_UNDEFINED));

#ifdef AUTH_SKEY_DEFINED
	if (net_pop3_auth_skey_enabled) {
	  if (net_pop3_crunch_skey_password(ce, password) != 0) {
		  /* failed to cruch one time password; abort the authentication
		   process
		 */
		  XP_STRCPY(cd->output_buffer, "*" CRLF);
		  cd->next_state_after_response = POP3_SEND_USERNAME;
	  }
	}
	else {
	  PR_snprintf(cd->output_buffer, OUTPUT_BUFFER_SIZE, "PASS %.256s" CRLF,
				  password);
	}
#else
/*	if (cd->auth_login_enabled) {
		char * str = 
		  NET_Base64Encode(password, XP_STRLEN(password));
		if (str)
		  {
			PR_snprintf(cd->output_buffer, 
						OUTPUT_BUFFER_SIZE, "%.256s" CRLF, str);
			XP_FREEIF(str);
		  }
		else
		  {
			return (net_pop3_error(ce, MK_POP3_PASSWORD_UNDEFINED));
		  }
	}
	else*/ {
		PR_snprintf(cd->output_buffer, 
					OUTPUT_BUFFER_SIZE, "PASS %.256s" CRLF, password);
	}
#endif
	XP_MEMSET(password, 0, XP_STRLEN(password));
	XP_FREE(password);

	cd->next_state_after_response = POP3_SEND_STAT;

	return(net_pop3_send_command(ce, cd->output_buffer));
}

PRIVATE int
net_pop3_send_stat(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	/* check password response */
	if(!cd->command_succeeded)
	  {
		/* The password failed.

		   Sever the connection and go back to the `read password' state,
		   which, upon success, will re-open the connection.  Set a flag
		   which causes the prompt to be different that time (to indicate
		   that the old password was bogus.)

		   But if we're just checking for new mail (biff) then don't bother
		   prompting the user for a password: just fail silently. */
		if (cd->only_check_for_new_mail)
		  return MK_POP3_PASSWORD_UNDEFINED;

		cd->password_failed = TRUE;
		cd->next_state = POP3_ERROR_DONE;	/* close */
		cd->pause_for_read = FALSE;		   /* try again right away */
/*		MSG_SetUserAuthenticated(MSG_GetMaster(cd->pane), FALSE); */
		return 0;
	  }
    else if (net_pop3_password_pending)
      {
		/*
		 * First time with this password.  Record it as a keeper.
		 * (The user's preference might say to save it.)
		 */
		FE_RememberPopPassword(ce->window_id, net_pop3_password);
		net_pop3_password_pending = FALSE;
/*		if (cd->pane)
			MSG_SetUserAuthenticated(MSG_GetMaster(cd->pane), TRUE); */
      }

    XP_STRCPY(cd->output_buffer, "STAT" CRLF);

	cd->next_state_after_response = POP3_GET_STAT;

	return(net_pop3_send_command(ce, cd->output_buffer));
}


PRIVATE int
net_pop3_get_stat(ActiveEntry *ce)
{
	Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	char *num;

	/* check stat response */
	if(!cd->command_succeeded)
		return(net_pop3_error(ce, MK_POP3_PASSWORD_FAILURE));

	/* stat response looks like:  %d %d
	 * The first number is the number of articles
	 * The second number is the number of bytes
	 *
	 *  grab the first and second arg of stat response
	 */
	num = XP_STRTOK(cd->command_response, " ");

	cd->number_of_messages = atol(num);

	num = XP_STRTOK(NULL, " ");

	cd->total_folder_size = (int32) atol(num);

	cd->total_download_size = -1; /* Means we need to calculate it, later. */

	if(cd->number_of_messages <= 0) {
	  /* We're all done.  We know we have no mail. */
	  cd->next_state = POP3_SEND_QUIT;
	  XP_Clrhash(cd->uidlinfo->hash);
	  return(0);
	}

	if (cd->only_check_for_new_mail && !net_leave_mail_on_server &&
		net_pop3_size_limit < 0) {
	  /* We're just checking for new mail, and we're not playing any games that
		 involve keeping messages on the server.  Therefore, we now know enough
		 to finish up.  If we had no messages, that would have been handled
		 above; therefore, we know we have some new messages. */
	  cd->biffstate = MSG_BIFF_NewMail;
	  cd->next_state = POP3_SEND_QUIT;
	  return(0);
	}


	if (!cd->only_check_for_new_mail) {
	  cd->msg_del_started = MSG_BeginMailDelivery(ce->window_id);

	  if(!cd->msg_del_started)
		{
		  return(net_pop3_error(ce, MK_POP3_MESSAGE_WRITE_ERROR));
		}
	}

	if ( (net_uidl_command_unimplemented && net_xtnd_xlst_command_unimplemented && net_top_command_unimplemented) ||
		(!cd->only_uidl && !net_leave_mail_on_server &&
		 (net_pop3_size_limit < 0 || net_top_command_unimplemented))) {
	  /* We don't need message size or uidl info; go directly to getting
		 messages. */
	  cd->next_state = POP3_GET_MSG;
	} else {
	  cd->next_state = POP3_SEND_LIST;
	}
	return 0;
}




PRIVATE int
net_pop3_send_list(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;
	cd->msg_info = (Pop3MsgInfo *) XP_ALLOC(sizeof(Pop3MsgInfo) *
											cd->number_of_messages);
	if (!cd->msg_info) return(MK_OUT_OF_MEMORY);
	XP_MEMSET(cd->msg_info, 0, sizeof(Pop3MsgInfo) * cd->number_of_messages);
    XP_STRCPY(cd->output_buffer, "LIST" CRLF);
	cd->next_state_after_response = POP3_GET_LIST;
	return(net_pop3_send_command(ce, cd->output_buffer));
}



PRIVATE int
net_pop3_get_list(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;
    char * line;
	int32 msg_num;

	/* check list response 
	 * This will get called multiple times
	 * but it's alright since command_succeeded
	 * will remain constant
	 */
	if(!cd->command_succeeded)
		return(net_pop3_error(ce, MK_POP3_LIST_FAILURE));


    ce->status = NET_BufferedReadLine(ce->socket, &line, &cd->data_buffer,
                        &cd->data_buffer_size, &cd->pause_for_read);

    if(ce->status == 0)
      {
        /* this shouldn't really happen, but...
         */
		return(net_pop3_error(ce, MK_POP3_SERVER_ERROR));
      }
    else if(ce->status < 0)
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
         return ce->status; /* wait for line */
      }

    TRACEMSG(("    Rx: %s", line));

	/* remove CRLF */
	XP_StripLine(line);

	/* parse the line returned from the list command 
	 * it looks like
	 * #msg_number #bytes
	 *
	 * list data is terminated by a ".CRLF" line
	 */
	if(!XP_STRCMP(line, "."))
	  {
		cd->next_state = POP3_SEND_UIDL_LIST;
		cd->pause_for_read = FALSE;
		return(0);
	  }

	msg_num = atol(XP_STRTOK(line, " "));

	if(msg_num <= cd->number_of_messages && msg_num > 0)
		cd->msg_info[msg_num-1].size = atol(XP_STRTOK(NULL, " "));

	return(0);
}
/* km
 *
 *
 *  Process state: POP3_START_USE_TOP_FOR_FAKE_UIDL
 *
 *	If we get here then UIDL and XTND are not supported by the mail server.
 * 
 *	Now we will walk backwards, from higher message numbers to lower, 
 *  using the TOP command. Along the way, keep track of messages to down load
 *  by POP3_GET_MSG.
 *
 *  Stop when we reach a msg that we knew about before or we run out of
 *  messages.
 *
 *	There is also conditional code to handle:
 *		BIFF
 *		Pop3ConData->only_uidl == true (fetching one message only)
 *      emptying message hash table if all messages are new
 *		Deleting messages that are marked deleted.
 *
 *
*/

/* this function gets called for each hash table entry.  If it finds a message
   marked delete, then we will have to traverse all of the server messages
   with TOP in order to delete those that are marked delete.


   If UIDL or XTND XLST are supported then this function will never get called.
   
   A pointer to this function is passed to XP_Maphash     -km */

PRIVATE XP_Bool
net_pop3_check_for_hash_messages_marked_delete(XP_HashTable table,
				       						   const void *key, 
				       						   void *value, 
				       						   void *closure)
{
	char valueChar = (char) (int) value;
	if (valueChar == DELETE_CHAR)
	{
		((Pop3ConData *) closure)->delete_server_message_during_top_traversal = TRUE;
		return FALSE;	/* XP_Maphash will stop traversing hash table now */
	}
	
	return TRUE;		/* XP_Maphash will continue traversing the hash */
}

PRIVATE int
send_fake_uidl_top(ActiveEntry *ce)
{
	Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	PR_snprintf(cd->output_buffer, 
		        OUTPUT_BUFFER_SIZE,  
		       "TOP %ld 0" CRLF,
		        cd->current_msg_to_top);

	cd->next_state_after_response = POP3_GET_FAKE_UIDL_TOP;
	cd->pause_for_read = TRUE;
	return(net_pop3_send_command(ce, cd->output_buffer));
}

PRIVATE int
start_use_top_for_fake_uidl(ActiveEntry *ce)
{
	Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	cd->current_msg_to_top = cd->number_of_messages;
	cd->number_of_messages_not_seen_before = 0;
	cd->found_new_message_boundary = FALSE;
	cd->delete_server_message_during_top_traversal = FALSE;
	
	/* may set delete_server_message_during_top_traversal to true */
	XP_Maphash(cd->uidlinfo->hash,
			   net_pop3_check_for_hash_messages_marked_delete, cd);
	
	return (send_fake_uidl_top(ce));
}


PRIVATE int
get_fake_uidl_top(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;
    char * line;

	/* check list response 
	 * This will get called multiple times
	 * but it's alright since command_succeeded
	 * will remain constant
	 */
	if(!cd->command_succeeded) {

	  /* UIDL, XTND and TOP are all unsupported for this mail server.
		 Tell the user to join the 20th century.

		 Tell the user this, and refuse to download any messages until they've
		 gone into preferences and turned off the `Keep Mail on Server' and
		 `Maximum Message Size' prefs.  Some people really get their panties
		 in a bunch if we download their mail anyway. (bug 11561)
	   */
	  char *fmt = XP_GetString(XP_THE_POP3_SERVER_DOES_NOT_SUPPORT_UIDL_ETC);
	  char *host = NET_ParseURL(ce->URL_s->address, GET_HOST_PART);
	  ce->URL_s->error_msg = PR_smprintf (fmt, (host ? host : "(null)"));
	  FREEIF(host);
	  cd->next_state = POP3_ERROR_DONE;
	  cd->pause_for_read = FALSE;
	  return -1;

	}


    ce->status = NET_BufferedReadLine(ce->socket, &line, &cd->data_buffer,
                        &cd->data_buffer_size, &cd->pause_for_read);

    if(ce->status == 0)
      {
        /* this shouldn't really happen, but...
         */
		return(net_pop3_error(ce, MK_POP3_SERVER_ERROR));
      }
    else if(ce->status < 0)
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
         return ce->status; /* wait for line */
      }

   	TRACEMSG(("    Rx: %s", line));

	/* remove CRLF */
	XP_StripLine(line);

	if(!XP_STRCMP(line, "."))
	{
		cd->current_msg_to_top--;
		if (!cd->current_msg_to_top || 
		    (cd->found_new_message_boundary &&
			 !cd->delete_server_message_during_top_traversal))
		{
			/* we either ran out of messages or reached the edge of new
			   messages and no messages are marked dele */
			cd->next_state = POP3_GET_MSG;
			cd->pause_for_read = FALSE;
			
			/* if all of the messages are new, toss all hash table entries */
			if (!cd->current_msg_to_top && !cd->found_new_message_boundary)
				XP_Clrhash(cd->uidlinfo->hash);
		}
		else
		{
			/* this message is done, go to the next */
			cd->next_state = POP3_SEND_FAKE_UIDL_TOP;
			cd->pause_for_read = FALSE;
		}
	}
	else
	{
		/* we are looking for a string of the form
		   Message-Id: <199602071806.KAA14787@neon.netscape.com> */
		char *firstToken = XP_STRTOK(line, " ");
		if (firstToken && !XP_STRCASECMP(firstToken, "MESSAGE-ID:") )
		{
			char *message_id_token = XP_STRTOK(NULL, " ");
			if ( !cd->only_uidl && message_id_token &&
			     (((int) XP_Gethash(cd->uidlinfo->hash, message_id_token, 0) ) == 0) )
			{	/* we have not seen this message before */
				
				/* if we are only doing a biff, stop here */
				if (cd->only_check_for_new_mail)
				{
					cd->biffstate = MSG_BIFF_NewMail;
	  				cd->next_state = POP3_SEND_QUIT;
					cd->pause_for_read = FALSE;
				}
				else	/* we will retrieve it and cache it in GET_MSG */
				{
				cd->number_of_messages_not_seen_before++;
				cd->msg_info[cd->current_msg_to_top-1].uidl = XP_STRDUP(message_id_token);
	  			if (!cd->msg_info[cd->current_msg_to_top-1].uidl)
					return MK_OUT_OF_MEMORY;
				}
			}
			else if (cd->only_uidl && message_id_token &&
					 !XP_STRCMP(cd->only_uidl, message_id_token))
			{
					cd->last_accessed_msg = cd->current_msg_to_top;
					cd->found_new_message_boundary = TRUE;
			}
			else if (!cd->only_uidl)
			{	/* we have seen this message and we care about the edge,
				   stop looking for new ones */
				if (cd->number_of_messages_not_seen_before != 0)
				{
					cd->last_accessed_msg = cd->current_msg_to_top;
					cd->found_new_message_boundary = TRUE;
					/* we stay in this state so we can process the rest of the
					   lines in the top message */
				}
				else
				{
	  				cd->next_state = POP3_SEND_QUIT;
					cd->pause_for_read = FALSE;
				}
			}
		}
	}
	return 0;
}


/* km
 *
 *	net_pop3_send_xtnd_xlst_msgid
 *
 *  Process state: POP3_SEND_XTND_XLST_MSGID
 *
 *	If we get here then UIDL is not supported by the mail server.
 *  Some mail servers support a similar command:
 *
 *		XTND XLST Message-Id
 *
 * 	Here is a sample transaction from a QUALCOMM server
 
 >>XTND XLST Message-Id
 <<+OK xlst command accepted; headers coming.
 <<1 Message-ID: <3117E4DC.2699@netscape.com>
 <<2 Message-Id: <199602062335.PAA19215@lemon.mcom.com>
 
 * This function will send the xtnd command and put us into the
 * POP3_GET_XTND_XLST_MSGID state
 *
*/
PRIVATE int
net_pop3_send_xtnd_xlst_msgid(ActiveEntry *ce)
{
  Pop3ConData * cd = (Pop3ConData *)ce->con_data;

  if (net_xtnd_xlst_command_unimplemented)
  	return(start_use_top_for_fake_uidl(ce));


  XP_STRCPY(cd->output_buffer, "XTND XLST Message-Id" CRLF);
  cd->next_state_after_response = POP3_GET_XTND_XLST_MSGID;
  cd->pause_for_read = TRUE;
  return(net_pop3_send_command(ce, cd->output_buffer));
}


/* km
 *
 *	net_pop3_get_xtnd_xlst_msgid
 *
 *  This code was created from the net_pop3_get_uidl_list boiler plate.
 *	The difference is that the XTND reply strings have one more token per
 *  string than the UIDL reply strings do.
 *
 */

PRIVATE int
net_pop3_get_xtnd_xlst_msgid(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;
    char * line;
	int32 msg_num;

	/* check list response 
	 * This will get called multiple times
	 * but it's alright since command_succeeded
	 * will remain constant
	 */
	if(!cd->command_succeeded) {
	    net_xtnd_xlst_command_unimplemented = TRUE;
		cd->next_state = POP3_START_USE_TOP_FOR_FAKE_UIDL;
		cd->pause_for_read = FALSE;
		return(0);
	}


    ce->status = NET_BufferedReadLine(ce->socket, &line, &cd->data_buffer,
                        &cd->data_buffer_size, &cd->pause_for_read);

    if(ce->status == 0)
      {
        /* this shouldn't really happen, but...
         */
		return(net_pop3_error(ce, MK_POP3_SERVER_ERROR));
      }
    else if(ce->status < 0)
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
         return ce->status; /* wait for line */
      }

    TRACEMSG(("    Rx: %s", line));

	/* remove CRLF */
	XP_StripLine(line);


	/* parse the line returned from the list command 
	 * it looks like
	 * 1 Message-ID: <3117E4DC.2699@netscape.com>
	 *
	 * list data is terminated by a ".CRLF" line
	 */
	if(!XP_STRCMP(line, "."))
	  {
		cd->next_state = POP3_GET_MSG;
		cd->pause_for_read = FALSE;
		return(0);
	  }

	msg_num = atol(XP_STRTOK(line, " "));

	if(msg_num <= cd->number_of_messages && msg_num > 0) {
/*	  char *eatMessageIdToken = XP_STRTOK(NULL, " ");	*/
	  char *uidl = XP_STRTOK(NULL, " ");	/* not really a uidl but a unique token -km */

	  if (!uidl)
		/* This is bad.  The server didn't give us a UIDL for this message.
		   I've seen this happen when somehow the mail spool has a message
		   that contains a header that reads "X-UIDL: \n".  But how that got
		   there, I have no idea; must be a server bug.  Or something. */
		uidl = "";

	  cd->msg_info[msg_num-1].uidl = XP_STRDUP(uidl);
	  if (!cd->msg_info[msg_num-1].uidl) return MK_OUT_OF_MEMORY;
	}

	return(0);
}


PRIVATE int
net_pop3_send_uidl_list(ActiveEntry *ce)
{
  Pop3ConData * cd = (Pop3ConData *)ce->con_data;

  if (net_uidl_command_unimplemented)
  	return(net_pop3_send_xtnd_xlst_msgid(ce));


  XP_STRCPY(cd->output_buffer, "UIDL" CRLF);
  cd->next_state_after_response = POP3_GET_UIDL_LIST;
  cd->pause_for_read = TRUE;
  return(net_pop3_send_command(ce, cd->output_buffer));
}


PRIVATE int
net_pop3_get_uidl_list(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;
    char * line;
	int32 msg_num;

	/* check list response 
	 * This will get called multiple times
	 * but it's alright since command_succeeded
	 * will remain constant
	 */
	if(!cd->command_succeeded) {
		cd->next_state = POP3_SEND_XTND_XLST_MSGID;
		cd->pause_for_read = FALSE;
		net_uidl_command_unimplemented = TRUE;
		return(0);

#if 0  /* this if block shows what UIDL used to do in this case */
	  /* UIDL doesn't work so we can't retrieve the message later, and we
	   * can't play games notating how to keep it on the server.  Therefore
	   * just go download the whole thing, and warn the user.
	   */
	  char *host, *fmt, *buf;

	  net_uidl_command_unimplemented = TRUE;

	  fmt = XP_GetString( XP_THE_POP3_SERVER_DOES_NOT_SUPPORT_UIDL_ETC );

	  host = NET_ParseURL(ce->URL_s->address, GET_HOST_PART);
	  XP_ASSERT(host);
	  if (!host) host = "(null)";
	  buf = PR_smprintf (fmt, host);
	  if (!buf) return MK_OUT_OF_MEMORY;
	  FE_Alert (ce->window_id, buf);
	  XP_FREE (buf);

	  /* Free up the msg_info structure, as we use its presence later to
		 decide if we can do UIDL-based games. */
	  net_pop3_free_msg_info(ce);

	  cd->next_state = POP3_GET_MSG;
	  return(0);

#endif /* 0 */
	}


    ce->status = NET_BufferedReadLine(ce->socket, &line, &cd->data_buffer,
                        &cd->data_buffer_size, &cd->pause_for_read);

    if(ce->status == 0)
      {
        /* this shouldn't really happen, but...
         */
		return(net_pop3_error(ce, MK_POP3_SERVER_ERROR));
      }
    else if(ce->status < 0)
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
         return ce->status; /* wait for line */
      }

    TRACEMSG(("    Rx: %s", line));

	/* remove CRLF */
	XP_StripLine(line);

	/* parse the line returned from the list command 
	 * it looks like
	 * #msg_number uidl
	 *
	 * list data is terminated by a ".CRLF" line
	 */
	if(!XP_STRCMP(line, "."))
	  {
		cd->next_state = POP3_GET_MSG;
		cd->pause_for_read = FALSE;
		return(0);
	  }

	msg_num = atol(XP_STRTOK(line, " "));

	if(msg_num <= cd->number_of_messages && msg_num > 0) {
	  char *uidl = XP_STRTOK(NULL, " ");

	  if (!uidl)
		/* This is bad.  The server didn't give us a UIDL for this message.
		   I've seen this happen when somehow the mail spool has a message
		   that contains a header that reads "X-UIDL: \n".  But how that got
		   there, I have no idea; must be a server bug.  Or something. */
		uidl = "";

	  cd->msg_info[msg_num-1].uidl = XP_STRDUP(uidl);
	  if (!cd->msg_info[msg_num-1].uidl) return MK_OUT_OF_MEMORY;
	}

	return(0);
}




/* this function decides if we are going to do a
 * normal RETR or a TOP.  The first time, it also decides the total number
 * of bytes we're probably going to get.
 */
PRIVATE int 
net_pop3_get_msg(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;
	char c;
	int i;
/*	XP_Bool prefBool = FALSE;*/

	if(cd->last_accessed_msg >= cd->number_of_messages) {
	  /* Oh, gee, we're all done. */
	  cd->next_state = POP3_SEND_QUIT;
	  return 0;
	}

	if (cd->total_download_size < 0) {
	  /* First time.  Figure out how many bytes we're about to get.
		 If we didn't get any message info, then we are going to get
		 everything, and it's easy.  Otherwise, if we only want one
		 uidl, than that's the only one we'll get.  Otherwise, go
		 through each message info, decide if we're going to get that
		 message, and add the number of bytes for it. */
	  if (cd->msg_info) {
		cd->total_download_size = 0;
		for (i=0 ; i<cd->number_of_messages ; i++) {
		  c = 0;
		  if (cd->only_uidl) {
			if (cd->msg_info[i].uidl && XP_STRCMP(cd->msg_info[i].uidl,
												  cd->only_uidl) == 0) {
			  cd->total_download_size = cd->msg_info[i].size;
			  break;
			}
			continue;
		  }
		  if (cd->msg_info[i].uidl) {
			c = (char) (int) XP_Gethash(cd->uidlinfo->hash,
										cd->msg_info[i].uidl, 0);
		  }
		  if (c != KEEP && c != DELETE_CHAR) {
			cd->total_download_size += cd->msg_info[i].size;
		  }
		}
	  } else {
		cd->total_download_size = cd->total_folder_size;
	  }
	  if (cd->only_check_for_new_mail) {
		if (cd->total_download_size > 0) cd->biffstate = MSG_BIFF_NewMail;
		cd->next_state = POP3_SEND_QUIT;
		return(0);
	  }
	  /* get the amount of available space on the drive
	   * and make sure there is enough
	   */	
	  if(cd->total_download_size > FE_DiskSpaceAvailable(ce->window_id))
		{
		  return(net_pop3_error(ce, MK_POP3_OUT_OF_DISK_SPACE));
		}

	}


	/* Look at this message, and decide whether to ignore it, get it, just get
	   the TOP of it, or delete it. */

/*	PREF_GetBoolPref("mail.auth_login", &prefBool);
	if (prefBool) 
		cd->next_state = POP3_SEND_XSENDER;
	else */
		cd->next_state = POP3_SEND_RETR;

	cd->pause_for_read = FALSE;
	if (cd->msg_info) {
	  Pop3MsgInfo* info = cd->msg_info + 
	  					  cd->last_accessed_msg;
	  if (cd->only_uidl) {
		if (info->uidl == NULL || XP_STRCMP(info->uidl, cd->only_uidl) != 0) {
		  cd->next_state = POP3_GET_MSG;
		}
	  } else {
		c = 0;
		if (info->uidl) {
		  c = (char) (int) XP_Gethash(cd->uidlinfo->hash, info->uidl, 0);
		}
		if (c == DELETE_CHAR) {
		  cd->next_state = POP3_SEND_DELE;
		} else if (c == KEEP) {
		  cd->next_state = POP3_GET_MSG;
		} else if (net_pop3_size_limit >= 0 &&
				   info->size > net_pop3_size_limit &&
				   !net_top_command_unimplemented &&
				   cd->only_uidl == NULL) {
		  cd->next_state = POP3_SEND_TOP;
		}
	  }
	  if ((net_leave_mail_on_server && cd->next_state != POP3_SEND_DELE) ||
		  cd->next_state == POP3_GET_MSG ||
		  cd->next_state == POP3_SEND_TOP) {

		/* This is a message we have decided to keep on the server.  Notate
		   that now for the future.  (Don't change the popstate file at all
		   if only_uidl is set; in that case, there might be brand new messages
		   on the server that we *don't* want to mark KEEP; we just want to
		   leave them around until the user next does a GetNewMail.) */

		if (info->uidl && cd->only_uidl == NULL) {
		  if (cd->newuidl == NULL) {
			cd->newuidl = XP_HashTableNew(20, XP_StringHash, uidl_cmp);
			if (!cd->newuidl) return MK_OUT_OF_MEMORY;
		  }
		  put_hash(cd->uidlinfo, cd->newuidl, info->uidl, KEEP);
		}
	  }
	  if (cd->next_state == POP3_GET_MSG) {
		cd->last_accessed_msg++; /* Make sure we check the next message next
									time! */
	  }
	}
	return 0;
}


/* start retreiving just the first 20 lines
 */
PRIVATE int
net_pop3_send_top(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	XP_ASSERT(!(net_uidl_command_unimplemented && 
	            net_xtnd_xlst_command_unimplemented &&
	            net_top_command_unimplemented) );
	XP_ASSERT(!net_top_command_unimplemented);

    PR_snprintf(cd->output_buffer, 
			   OUTPUT_BUFFER_SIZE,  
			   "TOP %ld 20" CRLF,
			   cd->last_accessed_msg+1);

	cd->truncating_cur_msg = TRUE;
	cd->next_state_after_response = POP3_TOP_RESPONSE;

	cd->cur_msg_size = -1;

	/* zero the bytes received in message in preparation for
	 * the next
	 */
	cd->bytes_received_in_message = 0;

	return(net_pop3_send_command(ce, cd->output_buffer));
}

/* send the xsender command
 */
PRIVATE int
net_pop3_send_xsender(ActiveEntry *ce)
{
  Pop3ConData * cd = (Pop3ConData *) ce->con_data;
  
  PR_snprintf(cd->output_buffer,
			  OUTPUT_BUFFER_SIZE,
			  "XSENDER %ld" CRLF,
			  cd->last_accessed_msg+1);

  cd->next_state_after_response = POP3_XSENDER_RESPONSE;
  return net_pop3_send_command(ce, cd->output_buffer);
}

PRIVATE int
net_pop3_xsender_response(ActiveEntry *ce)
{
  Pop3ConData * cd = (Pop3ConData *) ce->con_data;

  cd->sender_authed = FALSE;

  if (cd->command_succeeded && XP_STRLEN (cd->command_response)) {
  /* record the xsender info */
  /* inform backend to turn on the out of band auth info flag */
	  cd->sender_authed = TRUE;
  }
  
  cd->next_state = POP3_SEND_RETR;
  return 0;
}

/* retreive the whole message
 */
PRIVATE int
net_pop3_send_retr(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;
	char buf[OUTPUT_BUFFER_SIZE];

	cd->truncating_cur_msg = FALSE;
    PR_snprintf(cd->output_buffer, 
			   OUTPUT_BUFFER_SIZE,  
			   "RETR %ld" CRLF,
			   cd->last_accessed_msg+1);

	cd->next_state_after_response = POP3_RETR_RESPONSE;

	cd->cur_msg_size = -1;

	/* zero the bytes received in message in preparation for
	 * the next
	 */
	cd->bytes_received_in_message = 0;

	if (cd->only_uidl)
	  {
		/* Display bytes if we're only downloading one message. */
		XP_ASSERT(!cd->graph_progress_bytes_p);
		if (!cd->graph_progress_bytes_p)
		  FE_GraphProgressInit(ce->window_id, ce->URL_s,
							   cd->total_download_size);
		cd->graph_progress_bytes_p = TRUE;
	  }
	else
	  {
		PR_snprintf(buf, OUTPUT_BUFFER_SIZE,
		XP_GetString( XP_RECEIVING_MESSAGE_OF ),
					cd->last_accessed_msg+1,
					cd->number_of_messages);
		NET_Progress(ce->window_id, buf);
	  }

	return(net_pop3_send_command(ce, cd->output_buffer));
}

/* #### should include msgutils.h instead... */
extern int msg_LineBuffer (const char *net_buffer, int32 net_buffer_size,
						   char **bufferP, uint32 *buffer_sizeP,
						   uint32 *buffer_fpP,
						   XP_Bool convert_newlines_p,
						   int32 (*per_line_fn) (char *line, uint32
												 line_length, void *closure),
						   void *closure);


PRIVATE int32 net_pop3_retr_handle_line(char *line, uint32 line_length,
										void *closure);

/* digest the message
 */
PRIVATE int
net_pop3_retr_response(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;
	char *buffer;
	int32 buffer_size;
	int32 old_bytes_received = ce->bytes_received;

	if(cd->cur_msg_size == -1)
	  {
		/* this is the beginning of a message
		 * get the response code and byte size
		 */
		if(!cd->command_succeeded)
		  {
			return net_pop3_error(ce, MK_POP3_RETR_FAILURE);
		  }

		/* a successful retr response looks like: #num_bytes Junk
		 */
		cd->cur_msg_size = atol(XP_STRTOK(cd->command_response, " "));

		if(cd->cur_msg_size < 0)
			cd->cur_msg_size = 0;

		TRACEMSG(("Opening message stream: MSG_IncorporateBegin"));
		/* open the message stream so we have someplace
		 * to put the data
		 */
		cd->msg_closure =
		  MSG_IncorporateBegin (ce->format_out,
								(cd->truncating_cur_msg ?
								 cd->msg_info[cd->last_accessed_msg].uidl :
								 NULL),
								ce->URL_s,
								ce->window_id);

		TRACEMSG(("Done opening message stream!"));
													
		if(!cd->msg_closure)
		  {
		    return(net_pop3_error(ce, MK_POP3_MESSAGE_WRITE_ERROR));
		  }
	  }

	if (cd->data_buffer_size > 0)
	  {
		XP_ASSERT(cd->obuffer_fp == 0); /* must be the first time */
		buffer = cd->data_buffer;
		buffer_size = cd->data_buffer_size;
		cd->data_buffer_size = 0;
	  }
	else
	  {
        ce->status = NETREAD(ce->socket, 
							 NET_Socket_Buffer, 
							 NET_Socket_Buffer_Size);
		buffer = NET_Socket_Buffer;
		buffer_size = ce->status;

		cd->pause_for_read = TRUE;

	    if(ce->status == 0)
          {
		    /* this shouldn't happen
             */
		    return(net_pop3_error(ce, MK_POP3_SERVER_ERROR));
          }
        else if(ce->status < 0) /* error */
          {
            int err = SOCKET_ERRNO;
    
            TRACEMSG(("TCP Error: %d", err));
    
            if (err == XP_ERRNO_EWOULDBLOCK)
              {
                cd->pause_for_read = TRUE;
                return (0);
              }
    
            ce->URL_s->error_msg = NET_ExplainErrorDetails(MK_TCP_READ_ERROR, err);
    
            /* return TCP error
             */
            return MK_TCP_READ_ERROR;
          }
	  }

	ce->status = msg_LineBuffer(buffer, buffer_size,
								&cd->obuffer, &cd->obuffer_size,
								&cd->obuffer_fp, FALSE,
								net_pop3_retr_handle_line, (void *) ce);
	if (ce->status < 0)
	  {
		if (cd->msg_closure) {
		  MSG_IncorporateAbort(cd->msg_closure, MK_POP3_MESSAGE_WRITE_ERROR);
		  cd->msg_closure = NULL;
		}
		MSG_AbortMailDelivery(ce->window_id);
		return(net_pop3_error(ce, MK_POP3_MESSAGE_WRITE_ERROR));
	  }

	/* normal read. Yay! */
	if (cd->bytes_received_in_message + buffer_size > cd->cur_msg_size) {
	  buffer_size = cd->cur_msg_size -  cd->bytes_received_in_message;
	}
    cd->bytes_received_in_message += buffer_size;
    ce->bytes_received            += buffer_size;

	if (!cd->msg_closure) /* meaning _handle_line read ".\r\n" at end-of-msg */
	  {
		cd->pause_for_read = FALSE;
		if (cd->truncating_cur_msg ||
			(net_leave_mail_on_server && !(net_uidl_command_unimplemented &&
									       net_xtnd_xlst_command_unimplemented &&
									       net_top_command_unimplemented) )) {
		  /* We've retreived all or part of this message, but we want to
			 keep it on the server.  Go on to the next message. */
		  cd->last_accessed_msg++;
		  cd->next_state = POP3_GET_MSG;
		} else {
		  cd->next_state = POP3_SEND_DELE;
		}

		/* if we didn't get the whole message add the bytes that we didn't get
		   to the bytes received part so that the progress percent stays sane.
		 */
		if(cd->bytes_received_in_message < cd->cur_msg_size)
		  ce->bytes_received += (cd->cur_msg_size
								 - cd->bytes_received_in_message);
	  }

	if (cd->graph_progress_bytes_p)
	  FE_GraphProgress(ce->window_id, ce->URL_s,
					   ce->bytes_received,
					   ce->bytes_received - old_bytes_received,
					   cd->cur_msg_size);

	/* set percent done to portion of total bytes of all messages
	   that we're going to download. */
	FE_SetProgressBarPercent(ce->window_id,
							 ((ce->bytes_received*100)
							  / cd->total_download_size));

	return(0);
}


PRIVATE int
net_pop3_top_response(ActiveEntry *ce)
{
  Pop3ConData * cd = (Pop3ConData *)ce->con_data;
  if(cd->cur_msg_size == -1 &&  /* first line after TOP command sent */
	 !cd->command_succeeded)	/* and TOP command failed */
	{
	  /* TOP doesn't work so we can't retrieve the first part of this msg.
		 So just go download the whole thing, and warn the user.

		 Note that the progress bar will get somewhat screwed up in this case.
		 Oops. #### */
	  char *host, *fmt, *buf;
	  int size;
/*	  XP_Bool prefBool = FALSE;*/

	  net_top_command_unimplemented = TRUE;
	  cd->truncating_cur_msg = FALSE;

	  fmt = XP_GetString( XP_THE_POP3_SERVER_DOES_NOT_SUPPORT_THE_TOP_COMMAND );

	  host = NET_ParseURL(ce->URL_s->address, GET_HOST_PART);
	  XP_ASSERT(host);
	  if (!host) host = "(null)";
	  size = XP_STRLEN(fmt) + XP_STRLEN(host) + 100;
	  buf = (char *) XP_ALLOC (size);
	  if (!buf) return MK_OUT_OF_MEMORY;
	  PR_snprintf (buf, size, fmt, host);
	  FE_Alert (ce->window_id, buf);
	  XP_FREE (buf);

	  /* PREF_GetBoolPref ("mail.auth_login", &prefBool);
	  if (prefBool)
		  cd->next_state = POP3_SEND_XSENDER;
	  else */
		  cd->next_state = POP3_SEND_RETR;
	  return(0);
	}

  /* If TOP works, we handle it in the same way as RETR. */
  return net_pop3_retr_response(ce);
}



PRIVATE int32
net_pop3_retr_handle_line(char *line, uint32 line_length, void *closure)
{
  ActiveEntry *ce = (ActiveEntry *) closure;
  Pop3ConData * cd = (Pop3ConData *)ce->con_data;
  int status;

  XP_ASSERT(cd->msg_closure);
  if (!cd->msg_closure) return -1;

  status = MSG_IncorporateWrite(cd->msg_closure, line, line_length);

  if (status >= 0 &&
	  line[0] == '.' &&
	  (line[1] == CR || line[1] == LF)) {
	status = MSG_IncorporateComplete(cd->msg_closure);
	cd->msg_closure = 0;
  }

  return status;
}


PRIVATE int
net_pop3_send_dele(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	/* increment the last accessed message since we have now read it
	 */
	cd->last_accessed_msg++;

	PR_snprintf(cd->output_buffer, 
				OUTPUT_BUFFER_SIZE,  
				"DELE %ld" CRLF, 
				cd->last_accessed_msg);
	
	cd->next_state_after_response = POP3_DELE_RESPONSE;
	
	return(net_pop3_send_command(ce, cd->output_buffer));
}

PRIVATE int
net_pop3_dele_response(ActiveEntry *ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	/* the return from the delete will come here
	 */
	if(!cd->command_succeeded)
		return(net_pop3_error(ce, MK_POP3_DELE_FAILURE));

	cd->next_state = POP3_GET_MSG;

	cd->pause_for_read = FALSE;

	return(0);
}


/* start a pop3 load
 */
MODULE_PRIVATE int
NET_Pop3Load (ActiveEntry * ce)
{
	Pop3ConData * cd = XP_NEW(Pop3ConData);
	char* host = NET_ParseURL(ce->URL_s->address, GET_HOST_PART);
	char* uidl;

	if(!cd || !host)
		return(MK_OUT_OF_MEMORY);

	XP_MEMSET(cd, 0, sizeof(Pop3ConData));

	if(!net_pop3_username || !*net_pop3_username)
	  {
		FREE(cd);
		ce->URL_s->error_msg = NET_ExplainErrorDetails(
												MK_POP3_USERNAME_UNDEFINED);
		ce->status = MK_POP3_USERNAME_UNDEFINED;
#ifdef XP_MAC
		/*FE_EditPreference(PREF_Pop3ID); someone broke it*/
#endif
		return(MK_POP3_USERNAME_UNDEFINED);
	  }

	cd->uidlinfo = net_pop3_load_state(host, net_pop3_username);
	XP_FREE(host);
	cd->output_buffer = (char*)XP_ALLOC(OUTPUT_BUFFER_SIZE);
	if(!cd->uidlinfo || !cd->output_buffer) goto FAIL;


	ce->con_data = cd;

	cd->biffstate = MSG_BIFF_NoMail;	/* Return "no mail" unless proven
										   otherwise. */
	if(strcasestr(ce->URL_s->address, "?check")) {
	   cd->only_check_for_new_mail = TRUE;
	}

	uidl = strcasestr(ce->URL_s->address, "?uidl=");
	if (uidl) {
	  uidl += 6;
	  cd->only_uidl = XP_STRDUP(uidl);
	  if (!cd->only_uidl) goto FAIL;
	}

	ce->socket = SOCKET_INVALID;

	cd->next_state = POP3_READ_PASSWORD;
	
	return (NET_ProcessPop3(ce));

FAIL:
	if (cd->uidlinfo) net_pop3_free_state(cd->uidlinfo);
	if (cd->output_buffer) XP_FREE(cd->output_buffer);
	FREE(cd);
	return(MK_OUT_OF_MEMORY);
}

/* NET_process_Pop3  will control the state machine that
 * loads messages from a pop3 server
 *
 * returns negative if the transfer is finished or error'd out
 *
 * returns zero or more if the transfer needs to be continued.
 */
MODULE_PRIVATE int
NET_ProcessPop3 (ActiveEntry *ce)
{

   Pop3ConData * cd = (Pop3ConData *)ce->con_data;

    TRACEMSG(("Entering NET_ProcessPop3"));

    cd->pause_for_read = FALSE; /* already paused; reset */

	if(!net_pop3_username || !*net_pop3_username)
	  return(net_pop3_error(ce, MK_POP3_USERNAME_UNDEFINED));

    while(!cd->pause_for_read)
      {

		TRACEMSG(("POP3: Entering state: %d", cd->next_state));

        switch(cd->next_state)
          {
		    case POP3_READ_PASSWORD:
			  /* This is a seperate state so that we're waiting for the
				 user to type in a password while we don't actually have
				 a connection to the pop server open; this saves us from
				 having to worry about the server timing out on us while
				 we wait for user input. */
			  {
				char *fmt1 = 0, *fmt2 = 0;

			    /* If we're just checking for new mail (biff) then don't
				   prompt the user for a password; just tell him we don't
				   know whether he has new mail. */
			    if (cd->only_check_for_new_mail &&
					(!net_pop3_password || !net_pop3_username))
				  {
					ce->status = MK_POP3_PASSWORD_UNDEFINED;
					cd->biffstate = MSG_BIFF_Unknown;
					FE_UpdateBiff(cd->biffstate);
					cd->next_state = POP3_FREE;
					cd->pause_for_read = FALSE;
					break;
				  }

				XP_ASSERT(net_pop3_username);

				if (cd->password_failed)
				  fmt2 =
				  XP_GetString( XP_THE_PREVIOUSLY_ENTERED_PASSWORD_IS_INVALID_ETC );
				else if (!net_pop3_password)
				  fmt1 = 
				   XP_GetString( XP_PASSWORD_FOR_POP3_USER );

				if (fmt1 || fmt2)	/* We need to read a password. */
				  {
					char *password;
					char *host = NET_ParseURL(ce->URL_s->address,
											  GET_HOST_PART);
					size_t len = (XP_STRLEN(fmt1 ? fmt1 : fmt2) +
                                 (host ? XP_STRLEN(host) : 0) + 300) 
								 * sizeof(char);
					char *prompt = (char *) XP_ALLOC (len);
					if (!prompt) return MK_OUT_OF_MEMORY;
					if (fmt1)
					  PR_snprintf (prompt, len, fmt1, net_pop3_username, host);
					else
					  PR_snprintf (prompt, len, fmt2,
								   (cd->command_response
									? cd->command_response
									: XP_GetString(XP_NO_ANSWER)),
								   net_pop3_username, host);
					FREEIF (host);

					cd->password_failed = FALSE;
					password = FE_PromptPassword(ce->window_id, prompt);
					XP_FREE(prompt);

					if (password == NULL)
					  return MK_POP3_PASSWORD_UNDEFINED;

					net_set_pop3_password(password);
					XP_MEMSET(password, 0, XP_STRLEN(password));
					XP_FREE(password);

					net_pop3_password_pending = TRUE;
				  }

				XP_ASSERT (net_pop3_username && net_pop3_password);
				if (!net_pop3_username || !net_pop3_password)
				  return -1;

				cd->next_state = POP3_START_CONNECT;
				cd->pause_for_read = FALSE;
				break;
			  }

        	case POP3_START_CONNECT:

			    /* Start the thermometer (in cylon mode.) */
			    FE_SetProgressBarPercent(ce->window_id, -1);

            	ce->status = NET_BeginConnect(ce->URL_s->address,
											ce->URL_s->IPAddressString,
                                         	"POP3",
                                         	POP3_PORT,
                                         	&ce->socket,
                                         	FALSE,
                                         	&cd->tcp_con_data,
                                         	ce->window_id,
                                         	&ce->URL_s->error_msg,
											 ce->socks_host,
											 ce->socks_port);
	
            	if(ce->socket != SOCKET_INVALID)
                	NET_TotalNumberOfOpenConnections++;
   
            	cd->pause_for_read = TRUE;
   
            	if(ce->status == MK_CONNECTED)
                  {
                	cd->next_state = POP3_WAIT_FOR_START_OF_CONNECTION_RESPONSE;
                	cd->next_state_after_response = POP3_SEND_USERNAME;
                	FE_SetReadSelect(ce->window_id, ce->socket);
                  }
            	else if(ce->status > -1)
                  {
                	cd->next_state = POP3_FINISH_CONNECT;
                	ce->con_sock = ce->socket;  /* set con sock so we can select on it */
                	FE_SetConnectSelect(ce->window_id, ce->con_sock);
                  }
				else if(ce->status < 0)
				  {
					/* close and clear the socket here
					 * so that we don't try and send a RSET
					 */
            		if(ce->socket != SOCKET_INVALID)
					  {
                		NET_TotalNumberOfOpenConnections--;
                		FE_ClearConnectSelect(ce->window_id, ce->socket);
                		TRACEMSG(("Closing and clearing socket ce->socket: %d", ce->socket));
                		NETCLOSE(ce->socket);
						ce->socket = SOCKET_INVALID;
						cd->next_state = POP3_ERROR_DONE;
					  }
				  }
            	break;

          case POP3_FINISH_CONNECT:
            	ce->status = NET_FinishConnect(ce->URL_s->address,
                                          	"POP3",
                                          	POP3_PORT,
                                          	&ce->socket,
                                          	&cd->tcp_con_data,
                                          	ce->window_id,
                                          	&ce->URL_s->error_msg);

            	cd->pause_for_read = TRUE;

            	if(ce->status == MK_CONNECTED)
                  {
#ifdef AUTH_SKEY_DEFINED
/*					XP_Bool prefBool = FALSE;*/
                	cd->next_state = POP3_WAIT_FOR_START_OF_CONNECTION_RESPONSE;
/*					PREF_GetBoolPref("mail.support_skey", &prefBool);
				if (prefBool) {
					  cd->next_state_after_response = POP3_AUTH_SKEY;
					}
					else*/ {
					  cd->next_state_after_response = POP3_SEND_USERNAME;
					  /* user might unchecked the skey preference */
					  /* reset the auth skey state */
					  net_pop3_auth_skey_enabled = FALSE;
					  net_pop3_auth_skey_sequence = 0;
					  XP_FREEIF (net_pop3_auth_skey_seed);
					}
#else
/*					XP_Bool prefBool = FALSE;*/

                	cd->next_state = POP3_WAIT_FOR_START_OF_CONNECTION_RESPONSE;
/*                	PREF_GetBoolPref ("mail.auth_login", &prefBool);
					if (prefBool)
						cd->next_state_after_response = POP3_SEND_AUTH;
					else */
                		cd->next_state_after_response = POP3_SEND_USERNAME;
#endif
                	FE_ClearConnectSelect(ce->window_id, ce->con_sock);
                	FE_SetReadSelect(ce->window_id, ce->socket);
                  }
            	else if(ce->status > -1)
                  {

                	/* unregister the old CE_SOCK from the select list
                 	 * and register the new value in the case that it changes
                 	 */
                	if(ce->con_sock != ce->socket)
                  	  {
                    	FE_ClearConnectSelect(ce->window_id, ce->con_sock);
                    	ce->con_sock = ce->socket;
                    	FE_SetConnectSelect(ce->window_id, ce->con_sock);
                  	  }
                  }
				else if(ce->status < 0)
				  {
					/* close and clear the socket here
					 * so that we don't try and send a RSET
					 */
                	NET_TotalNumberOfOpenConnections--;
                	FE_ClearConnectSelect(ce->window_id, ce->socket);
                	TRACEMSG(("Closing and clearing socket ce->socket: %d", ce->socket));
                	NETCLOSE(ce->socket);
					ce->socket = SOCKET_INVALID;
					cd->next_state = POP3_ERROR_DONE;
				  }
            	break;

			case POP3_WAIT_FOR_RESPONSE:
				ce->status = net_pop3_wait_for_response(ce);
				break;
				
			case POP3_WAIT_FOR_START_OF_CONNECTION_RESPONSE:
				net_pop3_wait_for_start_of_connection_response(ce);
				break;

#ifdef AUTH_SKEY_DEFINED
		    case POP3_AUTH_SKEY:
			    ce->status = net_pop3_auth_skey(ce);
				break;

		    case POP3_AUTH_SKEY_RESPONSE:
			    ce->status = net_pop3_auth_skey_response(ce);
				break;

		    case POP3_AUTH_SKEY_SEQUENCE_N_SEED:
			    ce->status = net_pop3_auth_skey_sequence_n_seed(ce);
			    break;
#endif

			case POP3_SEND_AUTH:
				ce->status = net_pop3_send_auth(ce);
				break;

			case POP3_AUTH_RESPONSE:
				ce->status = net_pop3_auth_response(ce);
				break;

			case POP3_AUTH_LOGIN:
				ce->status = net_pop3_auth_login(ce);
				break;

			case POP3_AUTH_LOGIN_RESPONSE:
				ce->status = net_pop3_auth_login_response(ce);
				break;

			case POP3_SEND_USERNAME:
				NET_Progress(ce->window_id,
				XP_GetString(XP_CONNECT_HOST_CONTACTED_SENDING_LOGIN_INFORMATION) );
				ce->status = net_pop3_send_username(ce);
				break;

			case POP3_SEND_PASSWORD:
				ce->status = net_pop3_send_password(ce);
				break;

			case POP3_SEND_STAT:
				ce->status = net_pop3_send_stat(ce);
				break;

			case POP3_GET_STAT:
				ce->status = net_pop3_get_stat(ce);
				break;

			case POP3_SEND_LIST:
				ce->status = net_pop3_send_list(ce);
				break;

			case POP3_GET_LIST:
				ce->status = net_pop3_get_list(ce);
				break;

			case POP3_SEND_UIDL_LIST:
				ce->status = net_pop3_send_uidl_list(ce);
				break;

			case POP3_GET_UIDL_LIST:
				ce->status = net_pop3_get_uidl_list(ce);
				break;
				
			case POP3_SEND_XTND_XLST_MSGID:
				ce->status = net_pop3_send_xtnd_xlst_msgid(ce);
				break;

			case POP3_GET_XTND_XLST_MSGID:
				ce->status = net_pop3_get_xtnd_xlst_msgid(ce);
				break;
			
			case POP3_START_USE_TOP_FOR_FAKE_UIDL:
				ce->status = start_use_top_for_fake_uidl(ce);
				break;

    		case POP3_SEND_FAKE_UIDL_TOP:
    			ce->status = send_fake_uidl_top(ce);
    			break;
    			
    		case POP3_GET_FAKE_UIDL_TOP:
    			ce->status = get_fake_uidl_top(ce);
    			break;
    
			case POP3_GET_MSG:
				ce->status = net_pop3_get_msg(ce);
				break;

			case POP3_SEND_TOP:
				ce->status = net_pop3_send_top(ce);
				break;

			case POP3_TOP_RESPONSE:
				ce->status = net_pop3_top_response(ce);
				break;

			case POP3_SEND_XSENDER:
				ce->status = net_pop3_send_xsender(ce);
				break;

			case POP3_XSENDER_RESPONSE:
				ce->status = net_pop3_xsender_response(ce);
				break;

			case POP3_SEND_RETR:
				ce->status = net_pop3_send_retr(ce);
				break;

			case POP3_RETR_RESPONSE:
				ce->status = net_pop3_retr_response(ce);
				break;

			case POP3_SEND_DELE:
				ce->status = net_pop3_send_dele(ce);
				break;

			case POP3_DELE_RESPONSE:
				ce->status = net_pop3_dele_response(ce);
				break;

			case POP3_SEND_QUIT:
				/* attempt to send a server quit command.  Since this means
				   everything went well, this is a good time to update the
				   status file and the FE's biff state.
				 */
			    if (cd->newuidl) {
				  XP_HashTableDestroy(cd->uidlinfo->hash);
				  cd->uidlinfo->hash = cd->newuidl;
				  cd->newuidl = NULL;
				}
				if (!cd->only_check_for_new_mail) {
				  net_pop3_write_state(cd->uidlinfo);
				}
				if (!cd->only_uidl) {
				  FE_UpdateBiff(cd->biffstate);
				  if (!cd->only_check_for_new_mail &&
					  cd->total_download_size <= 0) {
					ce->status = net_pop3_error(ce, MK_POP3_NO_MESSAGES);
					/* Even if there are no messages, send a QUIT so that
					   the server doesn't get an abnormal termination. */
					cd->next_state = POP3_SEND_QUIT;
					break;
				  }
				}
    			XP_STRCPY(cd->output_buffer, "QUIT" CRLF);
				ce->status = net_pop3_send_command(ce, cd->output_buffer);
				cd->next_state = POP3_WAIT_FOR_RESPONSE;
				cd->next_state_after_response = POP3_DONE;
				break;

			case POP3_DONE:
            	if(ce->socket != SOCKET_INVALID)
                  {
                	FE_ClearReadSelect(ce->window_id, ce->socket);
                	TRACEMSG(("Closing and clearing sock ce->socket: %d", 
							  ce->socket));
                	NETCLOSE(ce->socket);
                	NET_TotalNumberOfOpenConnections--;
                	ce->socket = SOCKET_INVALID;
                  }
				if(cd->msg_del_started)
					MSG_EndMailDelivery(ce->window_id);
				cd->next_state = POP3_FREE;
				break;

			case POP3_ERROR_DONE:
				
            	if(ce->socket != SOCKET_INVALID)
                  {
#if 0
				    /* attempt to send a server reset command
				     * so that messages will not be marked as read

					 #### turns out we never need to do this -- this causes
					 our DELE commands to not be honored, but that turns out
					 to never be the right thing, since we never issue a
					 DELE command until the messages are known to be safely
					 on disk.
				     */
					XP_STRCPY(cd->output_buffer, "RSET" CRLF);
					net_pop3_send_command(ce, cd->output_buffer);
#endif /* 0 */

                	FE_ClearReadSelect(ce->window_id, ce->socket);
                	FE_ClearConnectSelect(ce->window_id, ce->socket);
                	TRACEMSG(("Closing and clearing socket ce->socket: %d",
							  ce->socket));
                	NETCLOSE(ce->socket);
                	NET_TotalNumberOfOpenConnections--;
                	ce->socket = SOCKET_INVALID;
                  }

				if(cd->msg_closure)
				  {
					MSG_IncorporateAbort(cd->msg_closure,
										 MK_POP3_MESSAGE_WRITE_ERROR);
					cd->msg_closure = NULL;
					MSG_AbortMailDelivery(ce->window_id);
			 	  }

				if(cd->msg_del_started)
				  {
					XP_ASSERT (!cd->password_failed);
					MSG_AbortMailDelivery(ce->window_id);
				  }

				if (cd->password_failed)
				  /* We got here because the password was wrong, so go
					 read a new one and re-open the connection. */
				  cd->next_state = POP3_READ_PASSWORD;
				else
				  /* Else we got a "real" error, so finish up. */
				  cd->next_state = POP3_FREE;

				cd->pause_for_read = FALSE;
				break;

			case POP3_FREE:
			    if (cd->newuidl) XP_HashTableDestroy(cd->newuidl);
			    net_pop3_free_state(cd->uidlinfo);
				if (cd->graph_progress_bytes_p) {
				  /* Only destroy it if we have initialized it. */
				  FE_GraphProgressDestroy(ce->window_id, ce->URL_s,
										  cd->cur_msg_size,
										  ce->bytes_received);
				}
				net_pop3_free_msg_info(ce);
				FREEIF(cd->only_uidl);
				FREEIF(cd->output_buffer);
				FREEIF(cd->obuffer);
				FREEIF(cd->data_buffer);
				FREEIF(cd->command_response);
				FREE(ce->con_data);
				return(-1);
				break;

			default:
				XP_ASSERT(0);

	      }  /* end switch */

       if(ce->status < 0 && cd->next_state != POP3_FREE)
          {
            cd->pause_for_read = FALSE;
            cd->next_state = POP3_ERROR_DONE;
          }
 
	  }  /* end while */

	return(ce->status);

}

/* abort the connection in progress
 */
MODULE_PRIVATE int
NET_InterruptPop3(ActiveEntry * ce)
{
    Pop3ConData * cd = (Pop3ConData *)ce->con_data;

	TRACEMSG(("NET_InterruptPop3 called"));
	
	cd->next_state = POP3_ERROR_DONE;
	ce->status = MK_INTERRUPTED;

	return(NET_ProcessPop3(ce));
}

#endif /* MOZILLA_CLIENT */
