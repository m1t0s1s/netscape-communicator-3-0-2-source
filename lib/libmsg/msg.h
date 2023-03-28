/* -*- Mode: C; tab-width: 4 -*-
   msg.h --- internal defs for the msg library
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 25-May-95.
 */

#ifndef _MSG_H_
#define _MSG_H_

#include "xp.h"
#include "msgcom.h"
#include "msgutils.h"

#undef MIN
#undef MAX
#define MAX(a,b)((a)>(b)?(a):(b))
#define MIN(a,b)((a)<(b)?(a):(b))


#define MANGLE_INTERNAL_ENVELOPE_LINES /* We always need to do this, for now */
#undef FIXED_SEPERATORS				   /* this doesn't work yet */
#define EMIT_CONTENT_LENGTH			   /* Experimental; and anyway, we only
										  emit it, we don't parse it, so this
										  is only the first step. */


/* #include "mailsum.h" */

/* hack - this string gets appended to the beginning of an attachment field
   during a forward quoted operation */

#define MSG_FORWARD_COOKIE  "$forward_quoted$"


#ifdef XP_WIN16
  /* Those winners who brought us the Win16 compiler seemed to be under
     the impression that C is a case-insensitive language.  How very. */
# define msg_OpenMessage msg_OpenMessage16
# define msg_ToggleFolderExpansion msg_ToggleFolderExpansion16
# define msg_ShowNewsgroups msg_ShowNewsgroups16
# define msg_ComposeMessage msg_ComposeMessage16
#endif


/* The PRINTF macro is for debugging messages of unusual events (summary */
/* files out of date or invalid or the like.  It's so that as I use the mail */
/* to actually read my e-mail, I can look at the shell output whenever */
/* something unusual happens so I can get some clues as to what's going on. */
/* Please don't remove any PRINTF calls you see, and be sparing of adding */
/* any additional ones.  Thanks.  - Terry */

#ifdef DEBUG
#define PRINTF(msg) XP_Trace msg
#else
#define PRINTF(msg)
#endif

#ifdef FREEIF
#undef FREEIF
#endif
#define FREEIF(obj) do { if (obj) { XP_FREE (obj); obj = 0; }} while (0)

/* The Netscape-specific header fields that we use for storing our
   various bits of state in mail folders.
 */
#define X_MOZILLA_STATUS        "X-Mozilla-Status"
#define X_MOZILLA_STATUS_LEN   /*1234567890123456*/    16

#define X_MOZILLA_NEWSHOST      "X-Mozilla-News-Host"
#define X_MOZILLA_NEWSHOST_LEN /*1234567890123456789*/ 19

#define X_UIDL                  "X-UIDL"
#define X_UIDL_LEN             /*123456*/               6

#define CONTENT_LENGTH          "Content-Length"
#define CONTENT_LENGTH_LEN     /*12345678901234*/      14


/* The three ways the list of newsgroups can be pruned.
 */
typedef enum
{
  MSG_ShowAll,
  MSG_ShowSubscribed,
  MSG_ShowSubscribedWithArticles
} MSG_NEWSGROUP_DISPLAY_STYLE;


/* The MSG_SORT_KEY shares the same space as MSG_CommandType, to avoid
   possible weird errors, but is restricted to the `sorting' commands
   (MSG_SortByDate through MSG_SortBySender.)
 */
typedef MSG_CommandType MSG_SORT_KEY;

/* The MSG_MOTION_TYPE shares the same space as MSG_CommandType, to avoid
   possible weird errors, but is restricted to the `motion' commands
   (MSG_NextMessage through MSG_LastMarkedMessage.)
 */
typedef MSG_CommandType MSG_MOTION_TYPE;

/* The MSG_REPLY_TYPE shares the same space as MSG_CommandType, to avoid
   possible weird errors, but is restricted to the `composition' commands
   (MSG_ReplyToSender through MSG_ForwardMessage.)
 */
typedef MSG_CommandType MSG_REPLY_TYPE;




/* The list of all message flags to not write to disk. */
#define MSG_FLAG_RUNTIME_ONLY   (MSG_FLAG_SELECTED |    \
                                 MSG_FLAG_ELIDED |      \
                                 MSG_FLAG_UPDATING |    \
                                 MSG_FLAG_DIRTY)

/* The list of all message flags that we allow undoing a change to. */
#define MSG_FLAG_UNDOABLE   (MSG_FLAG_READ |        \
                             MSG_FLAG_MARKED |      \
                             MSG_FLAG_EXPUNGED)



/* ===========================================================================
   Structures.
   ===========================================================================
 */


/* The msg_SortInfo structure describes one line of the sorting file. */

typedef struct msg_SortInfo {
  char* header;                 /* Name of the header to match. */
  char* match;                  /* String it must match. */
  char* pathname;               /* Name of folder to sort it into. */
  struct msg_SortInfo* next;    /* Next record to search (or NULL if
                                   last one.) */
} msg_SortInfo;




/* The MSG_Frame structure contains all of the mail and news specific
   information that is per-context.
 */
typedef struct MSG_Frame
{
  /* Data common to mail and news frames.
   */
  MSG_PANE_CONFIG pane_config;

  /* Information about the displayed message and threads.
   */
  struct MSG_Thread *threads;

  XP_Bool rot13_p;
  XP_Bool inline_attachments_p;
  XP_Bool thread_p;
  MSG_SORT_KEY sort_key;
  XP_Bool sort_forward_p;
  XP_Bool all_headers_visible_p;
  XP_Bool micro_headers_p;
  XP_Bool all_messages_visible_p;
  MSG_FONT citation_font;
  MSG_CITATION_SIZE citation_size;
  XP_Bool fixed_width_p;
  XP_Bool wrap_long_lines_p;

  struct MSG_Folder *folders;
  struct MSG_Folder *opened_folder;

  struct mail_FolderData* msgs;

  struct MSG_ThreadEntry *displayed_message;

  int num_selected_folders;
  int num_selected_msgs;

  int max_depth;				/* Maximum indentation depth needed to
								   display the folders.  If zero, then
								   we don't know. */

  uint32 first_incorporated_offset; /* Place to store the offset of the
                                       first newly incorporated message.
                                       This doesn't really deserve a place
                                       in this structure.  Hackhackhack...###*/

  XP_File inc_fid;                      /* Stuff to store about a pending */
  void (*inc_donefunc)(void*, XP_Bool); /* IncorporateFromFile.  This stuff */
  void* inc_doneclosure;                /* also doesn't belong in this
                                           struct.  Hackhackhack ### */

  char* inc_uidl;				/* Special UIDL to inc from, if any. hack### */

  struct msg_FolderParseState* incparsestate; /* Parse state for messages
												 appended while incing. */

  int disable_updates_depth;        /* used by msg_DisableUpdates() for
                                       redisplay optimization. */

  XP_Bool safe_background_activity;	/* TRUE if we believe that the current
									   activity going on in netlib for this
									   context is completely "safe"; i.e., we
									   can do any other operation in the
									   foreground without worrying about the
									   background thread getting in our way.
									   One example of a "safe" activity is
									   scanning newsgroups for the number
									   of unread articles.  One example of
									   an "unsafe" activity is getting new
									   mail.*/

  struct MSG_Folder* update_folder; /* This folder's line needs updating in
                                       the FE. */
  int update_folder_line;
  int update_folder_depth;
  XP_Bool update_folder_visible;
  XP_Bool force_reparse_hack;

  struct MSG_ThreadEntry* update_first_message; /* Messages whose lines */
  struct MSG_ThreadEntry* update_last_message; /* need updating in the FE. */
  int update_num_messages;
  int update_message_line;
  int update_message_depth;
  struct MSG_ThreadEntry* update_message_visible;
  XP_Bool update_all_messages;	/* Screw it; the whole world changed;
								   just remember to update *everything*. */

  XP_Bool need_toolbar_update;

  /* The "Reply to Sender" and "Reply to All" commands and friends work
     by opening these URLs.  The URLs are computed at the time at which
     we are handed the headers of the message being displayed, so that
     we don't need to keep extensive header info around all the time. */
  char *mail_to_sender_url;
  char *mail_to_all_url;
  char *post_reply_url;
  char *post_and_mail_url;

  /* The "Forward Message" command needs this, only in the case where the
	 displayed message does not have a corresponding line in the summary
	 (as when it was a followed link.)  Note that `displayed_message' might
	 be NULL while a message is displayed.
   */
  char *displayed_message_subject;
  char *displayed_message_id;


  char* searchstring;			/* String from last search. */
  XP_Bool searchcasesensitive;	/* Whether last search was case sensitive. */

  struct UndoState* undo;

  struct MSG_Folder* lastSelectFolder; /* Remember a folder for shift-click
										  ranging. */
  struct MSG_Folder* focusFolder; /* Which folder should have focus (for
									 WinFE) */

  struct MSG_ThreadEntry* lastSelectMsg;/* Remember a message for shift-click
										  ranging. */
  struct MSG_ThreadEntry* focusMsg; /* Which message should have focus (for
									   WinFE) */


  /* Cached info about the current folder, used mostly to help implement
	 MSG_CommandStatus in an efficient manner.  Put into a substruct mostly
	 for namespace simplification; the MSG_Frame struct is getting awfully
	 big! */
  struct MSG_CacheInfo {
	XP_Bool valid;				/* If FALSE, then all this info needs to
								   be recomputed.*/
	XP_Bool selected_msgs_p;	/* Whether any messages are selected. */
	XP_Bool possibly_plural_p;	/* Whether multiple messages are selected. */
	XP_Bool any_marked_msgs;	/* Whether there are any messages with a
								   mark.*/
	XP_Bool any_marked_selected; /* Whether any messages that are selected
									are marked. */
	XP_Bool any_unmarked_selected; /* Whether any messages that are selected
									  are not marked. */
	XP_Bool any_read_selected;	/* Whether any messages that are selected have
								   been read. */
	XP_Bool any_unread_selected; /* Whether any messages that are selected
									have not been read. */
	XP_Bool any_unread_notdisplayed; /* Whether any messages besides the one
										being currently viewed have not been
										read. */
	XP_Bool any_marked_notdisplayed; /* Whether any messages besides the one
										being currently viewed have been
										marked. */
	XP_Bool future_unread;		/* Whether a later message is unread. */
	XP_Bool past_unread;		/* Whether a previous message is unread. */
	XP_Bool future_marked;		/* Whether a later message is marked. */
	XP_Bool past_marked;		/* Whether a previous message is unread. */
  } cacheinfo;

  /* Information specific to mail folders and news groups.
   */
  union
  {
    struct MSG_NewsFrame
    {
      struct MSG_NewsHost *hosts;           /* All news hosts known - there
                                               is a MSG_Folder for each. */
      struct MSG_NewsHost *current_host;    /* The selected member of the
                                               `hosts' list, or 0. */

      struct msg_NewsRCFile *newsrc_file;   /* The newsrc data for the selected
                                               host, or 0 if there is no host.
                                               If there is a host, there is
                                               always newsrc data. */
	  void* newsrc_changed_timer;			/* If not NULL, then the newsrc
											   file needs to be saved, and
											   this is a timer to go and save
											   it. */
      time_t newsrc_file_date;		        /* The write date of the file when
											   we last read or wrote it. */

      MSG_NEWSGROUP_DISPLAY_STYLE group_display_type;

      /* This is set when we get the headers for the currently-displayed
         message, based on whether it was written by the current user. */
      XP_Bool cancellation_allowed_p;

	  /* The below is all stuff that we remember for libnet about which
		 articles we've already seen in the current newsgroup. */
	  struct MSG_NewsKnown {
		char* host_and_port;
		XP_Bool secure_p;
		char* group_name;
		struct msg_NewsRCSet* set; /* Set of articles we've already gotten
									  from the newsserver (if it's marked
									  "read", then we've already gotten it).
									  If an article is marked "read", it
									  doesn't mean we're actually displaying
									  it; it may be an article that no longer
									  exists, or it may be one that we've
									  marked read and we're only viewing
									  unread messages. */

		int32 first_possible;	/* The oldest article in this group. */
		int32 last_possible;	/* The newest article in this group. */

		XP_Bool shouldGetOldest;
	  } knownarts;
    } news;

    struct MSG_MailFrame
    {
      void* save_timer;
      void* scan_timer;
      int scanline;

      XP_Bool dirty;            /* TRUE if there are changes to the current
                                   folder that have not been written out. */

      msg_SortInfo* sort;       /* Pointer to first record of auto-sort
                                   info. */
      time_t sortfile_date;     /* Date of the sort file, when last
                                   loaded. */
      long sortfile_size;       /* Size of the last-loaded sort file. */

	  XP_Bool offered_compress;	/* TRUE if we've already offered the user
								   the opportunity to compress the folders. */
	  XP_Bool want_compress;	/* TRUE if we've found a folder that looks
								   like it could benefit from compression. */
    } mail;
  } data;

} MSG_Frame;


/* The MSG_NewsHost structure contains all of the data specific to a news
   host.  These come in pairs with MSG_Folder structures, but this struct
   is used to hold all of the data specific to a MSG_Folder which has
   MSG_FOLDER_FLAG_NEWS_HOST set in its `flags' slot.
 */
typedef struct MSG_NewsHost
{
  char *name;                   /* The host name, and nothing else */
  uint32 port;                  /* The port number; never 0. */
  XP_Bool secure_p;             /* The protocol to use */

  char *newsrc_file_name;       /* The name of the newsrc file associated with
                                   this host.  This will be of the form:

                                   ""		  meaning .newsrc or .snewsrc
                                   HOST       meaning .newsrc-HOST
                                   HOST:PORT  meaning .newsrc-HOST:PORT

								   Whether it begins with .newsrc or .snewsrc
								   depends on the secure_p slot (we pass one of
								   the types xpNewsRC or xpSNewsRC to xp_file.)

                                   The reason this is not simply derived from
                                   the host_name and port slots is that it's
                                   not a 1:1 mapping; if the user has a file
                                   called "newsrc", we will use that for the
                                   default host (the "" name.)  Likewise,
								   ".newsrc-H" and ".newsrc-H:119" are
								   ambiguous.
                                 */

  struct MSG_NewsHost *next;

} MSG_NewsHost;


/* The MSG_Folder structure contains all of the data specific to a folder,
   be it a newsgroup, news host, mail file, or mail directory.

   Because of this overloading, the MSG_Folder stucture really comes in three
   varieties (the differences held in the `data' union):

   Mail files:
      These have the MSG_FOLDER_FLAG_MAIL bit set, and don't have the
      MSG_FOLDER_FLAG_DIRECTORY bit set.  The `data.mail' part of the
      union is used.

   Mail directories:
      These have the MSG_FOLDER_FLAG_MAIL and MSG_FOLDER_FLAG_DIRECTORY
      bits set.  The `data.mail' part of the union is used.

   News Hosts:
      These have the MSG_FOLDER_FLAG_NEWS_HOST and MSG_FOLDER_FLAG_DIRECTORY
      bits set.  The `data.newshost' part of the union is used.

   News Groups:
      These have the MSG_FOLDER_FLAG_NEWSGROUP bit set, and might have
      the MSG_FOLDER_FLAG_DIRECTORY bit set, depending.  The `data.newsgroup'
      part of the union is used.

   Don't assume that News contexts will never see mail folders: they will,
   when mail messages are copied to files, and similar actions.  (However,
   a news folder will never display a mail folder in the folder list, and
   vice versa.)

   So when deciding what kind of folder you're dealing with, you should look
   at the flags on the folder, not the type of the MWContext being used.
 */
typedef struct MSG_Folder
{
  char *name;                   /* Name to display. */
  struct MSG_Folder* children;  /* Pointer to first child, if any. */
  struct MSG_Folder* next;      /* Pointer to next sibling. */
  uint16 flags;                 /* The MSG_FOLDER_FLAG_ values. */
  int32 unread_messages;        /* count of unread messages   (-1 means
                                   unknown; -2 means unknown but we already
                                   tried to find out.) */
  int32 total_messages;         /* count of existing messages. */

  union
  {
    struct
    {
      MSG_NewsHost *host;       /* Pointer into context->msgframe->hosts. */
    } newshost;

    struct
    {
      char *group_name;         /* Full newsgroup name. */
      int32 last_message;       /* id of the youngest message */
    } newsgroup;

    struct
    {
      char *pathname;           /* File name in URL (aka UNIX) syntax. */
	  int32 wasted_bytes;		/* How many bytes we could get back by
								   compressing this folder. */
    } mail;
  } data;
} MSG_Folder;


/* The MSG_ThreadEntry structure contains data about a message for a few
   related purposes:

     - there must be enough information here to generate the text of an
       entry in the thread list window;

     - there must be enough information to decide where it goes in that
       window, that is, how to sort and thread it;

     - and there must be enough information to get at the actual message,
       be it in a file or on a news server.
   
   Since there get to be a great number of these structures, it's important
   to keep it small.  Most of the time, when we are interested in some
   specific header field of a message, we are only interested in the headers
   of the visible message, not of all messages.  So this structure doesn't
   contain that kind of information.

   Some evil games are played with allocation of these structures, since
   the `mail' version needs less bytes than the `news' version.  Beware.
   Also, keep an eye on the word-alignment of the various slots to avoid
   causing the compiler to open up `holes' in it.

   Manipulators of MSG_ThreadEntry structures should always be passed a
   corresponding `mail_FolderData' structure, which is where the string
   tables and related data reside.  In particular, **DO NOT** assume that
   the MSG_ThreadEntry you have been passed came from the `msgs' slot
   of the MSG_Frame in the current MWContext.  That is often the case,
   but not always.

   Also, do not make assumptions about the type of the ThreadEntry (mail
   message or news message) based on the type of the MWContext that is
   manipulating it.  News and mail composition contexts will manipulate
   mail folders occasionally.
 */
typedef struct MSG_ThreadEntry
{
  uint16 sender;                       /*  2 */  /* Indexes into the string- */
  uint16 recipient;                    /*  4 */  /* table in a corresponding */
  uint16 subject;                      /*  6 */  /* mail_FolderData struct. */
  uint16 lines;                        /*  8 */
  uint16 id;                           /* 10 */
  uint16 flags;                        /* 12 */  /* the MSG_FLAG_ values */
  uint16 *references;                  /* 16 */  /* need for threading */
  time_t date;                         /* 20 */
  struct MSG_ThreadEntry *first_child; /* 24 */  /* Followups in this thread */
  struct MSG_ThreadEntry *next;        /* 28 */  /* Siblings in this thread */

  union
  {
    struct msg_MailThreadData
    {
      uint32 file_index;               /* 32 */
      uint32 byte_length;              /* 36 */
      uint16 status_offset;            /* 38   REAL END OF ALLOCATED STORAGE:
                                               see mailsum.c */
      uint32 sleazy_end_of_data;       /* 42 */
    } mail;
    struct msg_NewsThreadData
    {
      uint32 article_number;           /* 32   REAL END OF ALLOCATED STORAGE:
                                               see mailsum.c */
      uint32 sleazy_end_of_data;       /* 36 */
    } news;
  } data;

  /* Put no new slots down here! */

} MSG_ThreadEntry;



/* Used for the various things that parse RFC822 headers...
 */
typedef struct message_header
{
  const char *value;  /* The contents of a header (after ": ") */
  short length;       /* The length of the data (it is not NULL-terminated.) */
  char *multi_header; /* The contents of mutliple instances of a header. */
					  /* Separated by \n\t. Null-terminated dynamic */
					  /* allocated buffer. */
} message_header;



/* This structure is the interface between compose.c and composew.c.
   When we have downloaded a URL to a tmp file for attaching, this
   represents everything we learned about it (and did to it) in the
   process.
 */
typedef struct MSG_AttachedFile
{
  char *orig_url;		/* Where it came from on the network (or even elsewhere
						   on the local disk.)
						 */
  char *file_name;		/* The tmp file in which the (possibly converted) data
						   now resides.
						 */
  char *type;			/* The type of the data in file_name (not necessarily
						   the same as the type of orig_url.)
						 */
  char *encoding;		/* Likewise, the encoding of the tmp file.
						   This will be set only if the original document had
						   an encoding already; we don't do base64 encoding and
						   so forth until it's time to assemble a full MIME
						   message of all parts.
						 */

  /* #### I'm not entirely sure where this data is going to come from...
   */
  char *description;					/* For Content-Description header */
  char *x_mac_type, *x_mac_creator;		/* Some kinda dumb-ass mac shit. */


  /* Some statistics about the data that was written to the file, so that when
	 it comes time to compose a MIME message, we can make an informed decision
	 about what Content-Transfer-Encoding would be best for this attachment.
	 (If it's encoded already, we ignore this information and ship it as-is.)
   */
  uint32 size;
  uint32 unprintable_count;
  uint32 highbit_count;
  uint32 ctl_count;
  uint32 null_count;
  uint32 max_line_length;
  
  XP_Bool decrypted_p;	/* S/MIME -- when attaching a message that was
						   encrypted, it's necessary to decrypt it first
						   (since nobody but the original recipient can read
						   it -- if you forward it to someone in the raw, it
						   will be useless to them.)  This flag indicates
						   whether decryption occurred, so that libmsg can
						   issue appropriate warnings about doing a cleartext
						   forward of a message that was originally encrypted.
						 */
} MSG_AttachedFile;




/* Argument to msg_MapNewsgroupNames() */
typedef int (*msg_NewsgroupNameMapper) (MWContext *context,
                                        const char *parent_name,
                                        const char *child_name,
                                        XP_Bool group_p,
                                        int32 descendant_group_count,
                                        void *closure);

/* Argument to msg_NewsgroupNameMapper() */
typedef int (*msg_SubscribedGroupNameMapper) (MWContext *context,
                                              const char *name,
                                              void *closure);


/* Preference variables */
extern XP_Bool msg_auto_expand_mail_threads;
extern XP_Bool msg_auto_expand_news_threads;


#define INBOX_FOLDER_NAME	"Inbox"
#define QUEUE_FOLDER_NAME	"Outbox"	/* #### Hagan wants "Outbox" */
#define TRASH_FOLDER_NAME	"Trash"


XP_BEGIN_PROTOS

/* ===========================================================================
   Routines called by MSG_Command() -- these are all the user-level commands.
   The context is either a mail or news context.
   ===========================================================================
 */
int msg_OpenFolder (MWContext *context);
int msg_NewFolder (MWContext* context);
int msg_DeleteFolder (MWContext* context);
int msg_OpenNewsHost (MWContext *context);
int msg_AddNewsGroup (MWContext* context);
int msg_Undo (MWContext *context);
int msg_Redo (MWContext *context);
int msg_QuoteMessage (MWContext* context);
int msg_ShowNewsGroups (MWContext *context, MSG_NEWSGROUP_DISPLAY_STYLE style);
int msg_CheckNewNewsGroups (MWContext *context);
int msg_Rot13Message (MWContext *context);
int msg_AttachmentsInline (MWContext *context, XP_Bool inline_p);
int msg_GetNewMail (MWContext *context);
int msg_DeliverQueuedMessages (MWContext *context);
int msg_ComposeMessage (MWContext *context, MSG_REPLY_TYPE command);
int msg_MarkMessage (MWContext *context, XP_Bool marked_p);
int msg_UnmarkAllMessages (MWContext *context);
int msg_DeleteMessages (MWContext *context);
int msg_CancelMessage (MWContext *context);
int msg_RemoveNewsHost (MWContext *context);
int msg_SubscribeSelectedGroups (MWContext *context, XP_Bool subscribe_p);
int msg_EmptyTrash (MWContext *context);
int msg_CompressFolder (MWContext *context);
int msg_CompressAllFolders (MWContext *context);
int msg_MarkSelectedRead (MWContext *context, XP_Bool read_p);
int msg_Find(MWContext* context, XP_Bool doOpenedFirst);
int msg_MarkThreadsRead (MWContext *context);
int msg_MarkAllMessagesRead (MWContext *context);
int msg_ShowMessages (MWContext *context, XP_Bool show_all_p);
int msg_GotoMessage (MWContext *context, MSG_MOTION_TYPE motion_type);
int msg_SetThreading (MWContext *context, XP_Bool thread_p);
int msg_SetSortKey (MWContext *context, MSG_SORT_KEY sort_key);
int msg_SetSortForward (MWContext *context, XP_Bool forward_p);
int msg_ReSort (MWContext *context);
int msg_ConfigureCompositionHeaders (MWContext *context, XP_Bool show_all_p);
int msg_ToggleCompositionHeader(MWContext* context, uint32 header);
int msg_SetHeaders (MWContext *context, MSG_CommandType which);
int msg_SetCompositionEncrypted (MWContext *context, XP_Bool encrypt_p);
int msg_SetCompositionSigned (MWContext *context, XP_Bool sign_p);
int msg_ToggleCompositionEncrypted (MWContext *context);
int msg_ToggleCompositionSigned (MWContext *context);
int msg_SetCompositionFormattingStyle (MWContext *context, XP_Bool markup_p);
XP_Bool msg_CompositionSendingEncrypted (MWContext *context);
XP_Bool msg_CompositionSendingSigned (MWContext *context);
int msg_ConfigureQueueing (MWContext *context, XP_Bool queue_for_later_p);
int msg_AttachAsText (MWContext *context, XP_Bool text_p);
XP_Bool msg_ShowingAllCompositionHeaders(MWContext* context);
XP_Bool msg_ShowingCompositionHeader(MWContext* context, uint32 mask);
int msg_DeliverMessage (MWContext *context, XP_Bool queue_p);


/* ===========================================================================
   Redisplay-related stuff
   ===========================================================================
 */

/* This is an optimization to delay the updating of the front end display
   data until we're done with all changes in progress.  Calls to these
   two functions must be balanced; they may nest.
 */
extern void msg_DisableUpdates (MWContext* context);
extern void msg_EnableUpdates (MWContext* context);
extern void msg_FlushUpdateMessages (MWContext* context);


/* Tell the FE that the toolbar needs to be updated.  The FE won't be called
   until after any pending msg_DisableUpdates have been closed. */
void msg_UpdateToolbar(MWContext* context);

/* Tell the front end to remove all the folder lines, and reconstruct
   them all (skipping any folder that has the elided_p bit set.) */
extern void msg_RedisplayFolders (MWContext* context);

/* Tell the front end to redraw a single folder line.  If you don't know
   the line number or depth, you may pass in 0.  If force_visible_p is 
   TRUE, then the display will scroll to make this line visible if 
   necessary.  If FALSE, no scrolling will occur even if this line isn't
   currently visible. */
extern void
msg_RedisplayOneFolder (MWContext* context, MSG_Folder* folder,
                        int line_number, int depth, XP_Bool force_visible_p);


/* Tell the front end to recreate its folder pull-down menus. */
extern void msg_RebuildFolderMenus (MWContext* context);

/* Unselect every folder. */
void msg_UnselectAllFolders(MWContext* context);

/* Tell the front end to remove all the message thread lines, and reconstruct
   them all (skipping any folder that has the elided_p bit set.) */
extern void msg_RedisplayMessages (MWContext* context);

/* Tell the front end to redraw a single message line.  If you don't know
   the line number or depth, you may pass in 0.  If force_visible_p is 
   TRUE, then the display will scroll to make this line visible if 
   necessary.  If FALSE, no scrolling will occur even if this line isn't
   currently visible. */
extern void msg_RedisplayOneMessage (MWContext* context, MSG_ThreadEntry* msg,
                                     int line_number, int depth,
                                     XP_Bool force_visible_p);

/* #### what exactly does this do? */
extern void msg_RedisplayMessagesFrom (MWContext* context,
                                       MSG_ThreadEntry* msg,
                                       MSG_ThreadEntry* visible);

/* Clear out the message display (make Layout be displaying no document.) */
extern void msg_ClearMessageArea (MWContext *context);

/* Tell the FE to remove all lines in the message thread list; this also
   calls msg_ClearMessageArea(). */
extern void msg_ClearThreadList (MWContext *context);


/* Removes the folder from the list of folders, and redisplays the FE.
   The context must be a mail or news context; the folder must currently
   be listed.  The folder is not freed - it is merely unlinked from the
   tree.  Disposal of the folder is the caller's responsibility.
 */
extern void msg_RemoveFolderLine (MWContext *context, MSG_Folder *folder);


/* Causes all the toplevel threads to be elided. */
extern int msg_ElideAllToplevelThreads(MWContext* context);


/* ===========================================================================
   Manipulating MSG_ThreadEntry objects, and the flags thereof.
   ===========================================================================
 */

/* Make sure the given message is highlighted and visible.
   The MSG_FLAG_SELECTED flag will be set, and the thread list will be
   scrolled to display this message.  If the message is within an elided
   thread, that thread will be unelided. */
extern void msg_EnsureSelected (MWContext* context, MSG_ThreadEntry* msg);

/* These turn on or off the MSG_FLAG_SELECTED bit on every visible ThreadEntry,
   and redisplay.  The context may be a mail or news context. */
extern void msg_SelectAllMessages(MWContext *context);
extern void msg_UnselectAllMessages(MWContext *context);

extern int msg_SelectThread(MWContext* context);
extern int msg_SelectMarkedMessages(MWContext* context);
extern int msg_MarkSelectedMessages(MWContext* context, XP_Bool value);

/* This causes the given message to be displayed, by calling FE_GetURL
   on a URL which represents this message.

   WARNING: this assumes that the ThreadEntry belongs to the folder currently
   displayed, that is, it is a member of MWContext->msgdata->msgs.  The
   context may be either a mail or news context, but the ThreadEntry must be
   of the same type as the context.
 */
extern int msg_OpenMessage (MWContext *context, MSG_ThreadEntry *msg);

/* Reload the currently displayed message.  Note that msg_OpenMessage()
   is a no-op if given the current message; this function forces a reload. */

extern int msg_ReOpenMessage(MWContext* context);

extern void msg_ReCountSelectedMessages(MWContext* context);


/* ===========================================================================
   Searching for MSG_ThreadEntry objects which are displayed in the threads.
   ===========================================================================
 */


/* This returns the number of lines in the thread list - that is, the number
   of messages in the folder or newsgroup, minus those which are elided, and
   plus the dummy `expired' lines.
 */
extern int msg_CountVisibleMessages (MSG_ThreadEntry* msg);

/* Executes the given function on every message.
   Stops if the function ever returns FALSE.
 */
extern void
msg_IterateOverAllMessages (MWContext* context,
                            XP_Bool (*func)(MWContext* context,
                                            MSG_ThreadEntry* msg,
                                            void* closure),
                            void* closure);

/* Executes the given function on each selected message.
   Stops if the function ever returns FALSE, and returns FALSE if that happens.
   Otherwise, returns TRUE.
 */
extern XP_Bool
msg_IterateOverSelectedMessages (MWContext* context,
                                 XP_Bool (*func)(MWContext* context,
                                                 MSG_ThreadEntry* msg,
                                                 void* closure),
                                 void* closure);


/* Returns a line suitable for using as the envelope line in a BSD
   mail folder. The returned string is stored in a static, and so
   should be used before msg_GetDummyEnvelope is called again. */
extern char * msg_GetDummyEnvelope(void);

/* Returns TRUE if the buffer looks like a valid envelope.
   This test is somewhat more restrictive than XP_STRNCMP(buf, "From ", 5).
 */
extern XP_Bool msg_IsEnvelopeLine(const char *buf, int32 buf_size);


/* Given a MSG_ThreadEntry object, this returns the line number and
   indentation depth, for redisplay purposes.
 */
extern int msg_FindMessage (MWContext* context, MSG_ThreadEntry *search,
                            int *depth);

/* This returns the MSG_ThreadEntry at the given line number,
   and incidentally returns its indentation depth.

   #### These two functions seem to have had their names transposed :-)
 */
extern MSG_ThreadEntry* msg_FindMessageLine (MWContext *context,
                                             int line_number, int *depth);


/* Find the message indicated by the motion type: this is how commands
   like `Next Unread Message' decide which message to select.
   The context may be either mail or news, and the message returned
   will be a member of the current folder, relative in some way to the
   displayed message.
 */
extern MSG_ThreadEntry *msg_FindNextMessage (MWContext *context,
                                             MSG_MOTION_TYPE type);


/* Find the first message that is after a selected message but is not
   currently selected.  This is the message to view when a delete/move
   operation is complete. */
extern MSG_ThreadEntry *msg_FindNextUnselectedMessage(MWContext* context);


/* Mark the given message as expunged, moving the message onto the right list
   in the mail_FolderData structure, and redisplays, and stuff.  This only does
   part of the job; you almost always want to use msg_ChangeFlag to delete a
   message. */
extern void msg_MarkExpunged(MWContext* context, struct mail_FolderData* msgs,
							 MSG_ThreadEntry* msg);



/* Change a flag in a message in any folder.  This takes care of side
   effects, including changing newsrc counts and logging in the undo
   log.  The "message_offset" is the byte-offset of this message in a
   mail folder, or the article number in a news folder.

   The context must be a mail or news context.

   #### I don't understand the lifetime or constraints of the `folder'
   argument, esp. w.r.t. undo.
 */
extern int msg_ChangeFlagAny (MWContext* context, MSG_Folder* folder,
                              uint32 message_offset,
                              uint16 flagson, uint16 flagsoff);

/* Change a flag in a message in the current folder.  This takes care
   of side effects, including changing the newsrc counts and logging
   in the undo log.  The context may be mail or news; the message
   must belong to the currently displayed folder of the context.
 */
extern int msg_ChangeFlag (MWContext* context, MSG_ThreadEntry* msg,
                           uint16 flagson, uint16 flagsoff);


/* Turns a flag on if it's off, and off if it's on.
   Simply calls msg_ChangeFlag() with the appropriate value. */
extern void msg_ToggleFlag(MWContext* context, MSG_ThreadEntry* msg,
                           uint16 flag);

/* This allocates a string which is the URL for the given message.
   This takes into account all the magic of both mailbox: and news:
   syntaxes.  The context may be either mail or news; the threadEntry
   need not belong to them. */
extern char *msg_GetMessageURL (MWContext *context, MSG_ThreadEntry *msg);


/* ===========================================================================
   Utilities for folders of either type (mail files, news hosts, newsgroups.)
   ===========================================================================
 */


/* Selects and opens the given folder.  This is done by determining the URL
   for the folder or newsgroup, and calling FE_GetURL().  If `func' is
   provided, it will be called from the exit routine of the URL (you can
   use this to open the next URL you need, presumably a message.)

   The context may be either a mail or news context, and the folder must
   belong to it.

   The mail summary and newsrc information will be flushed out by this.
   If this is called on the current folder, it will be reloaded.
 */
extern int msg_SelectAndOpenFolder(MWContext *context, MSG_Folder *folder,
								   Net_GetUrlExitFunc* func);


/* Opens or closes the given folder.  The context may be either a mail or
   news context, and the folder must belong to it.  The folder may be of
   any `DIRECTORY' type, and the appropriate right thing will happen.
   If it is a news host or newsgroup folder, and it is being opened,
   and `update_article_counts_p' is true, then after displaying the
   newly-exposed newsgroup lines, their article counts will be updated
   by calling FE_GetURL().
 */
extern int msg_ToggleFolderExpansion (MWContext *context, MSG_Folder *folder,
                                      XP_Bool update_article_counts_p);


/* Free up the storage of one folder. */
extern void msg_FreeOneFolder(MWContext* context, MSG_Folder* folder);

/* Sorts the folders by name - does not recurse. */
extern int msg_SortFolders(MSG_Folder **folder_list);


/* ===========================================================================
   Utilities specific to mail folders and their Folders and ThreadEntries.
   ===========================================================================
 */

/* This creates a mail folder object, and initializes the important slots.
   Either of prefix and suffix may be NULL, but not both.  If both are
   provided, they are appended, with a "/" between them if there isn't
   one at the end of the first or beginning of the second already.

   This initalizes folder->data.mail.pathname, and sets folder->name
   to an abbreviation based on the user's mail directory.  It also
   checks the name and initalizes the INBOX, QUEUE, etc flags as
   appropriate.

   The context may be of any type - it is not manipulated, and the folder
   is not stored into it.
 */
extern MSG_Folder *msg_MakeMailFolder (MWContext *context,
                                       const char *prefix, const char *suffix);


/* Given a folder name (file name in Unix syntax) returns a folder object
   for it, creating it if necessary (if create_p is true).  The context must
   be a mail context, and the returned folder will belong to it, and be
   available in the context's folder list.
 */
extern struct MSG_Folder *msg_FindMailFolder (MWContext *context,
                                              const char *folder_name,
                                              XP_Bool create_p);


/* Makes sure the "inbox" folder exists.  If it doesn't, creates it,
   place an introductory message in it, and selects it.  The context
   must be a mail context.
 */
extern void msg_EnsureInboxExists (MWContext *context);



/* Write out the summary file for the current mail folder.  This is a no-op
   unless the dirty bit is on (as set by msg_SummaryFileChanged.)
   This will clear the dirty bit, and kill any pending save timer.
   This routine should always be called before changing to a different
   mail folder.  The context should be a mail context.
 */
void msg_SaveMailSummaryChangesNow(MWContext* context);

/* Writes out a new summary file for the given mail folder.

   The MWContext may be any type of context, and the folder object
   need not be a member of the context.

   If the MSG_ThreadEntry is provided, it is a message that has had its
   flags changed, and should have its X-Mozilla-Status line updated (if any).
   This message must be a member of the provided mail_FolderData.

   This writes the summary file immediately.  WARNING: to write the summary
   file, we might have to re-parse the folder first, and this is done as a
   blocking operation!
 */
extern void msg_MailSummaryFileChanged(MWContext* context, MSG_Folder* folder,
                                       struct mail_FolderData* msgs,
                                       MSG_ThreadEntry* msg);


/* This is just like msg_MailSummaryFileChanged() except that:
   - it doesn't save right away: instead, it starts a timer, to delay the
     update until some time later.
   - it assumes that the ThreadEntry is a member of the current context,
     which must be a mail context.
 */
extern void msg_SelectedMailSummaryFileChanged(MWContext* context,
                                               MSG_ThreadEntry* msg);


/* helper function for MSG_MarkMessageIDRead() -- don't call this. */
extern int msg_MarkMailArticleRead (MWContext *context,
                                    const char *message_id);



/* Free the storage used for the mail-sorting stuff. */
extern void msg_FreeSortInfo(MWContext* context);


/* returns the name of the queue folder - returns a new string.
   Context may be any type. */
extern char *msg_QueueFolderName(MWContext *context);

/* Whether mail should be delivered now, or just appended to the queue folder.
   The context may be any type, but this will usually be called with a
   mail composition context.
 */
extern XP_Bool msg_DeliverMailNow (MWContext *context);

/* Whether the given MWContextComposition is in the process of delivering
   mail; this is used to make all commands except "stop" be insensitive
   while mail is being sent, so that one can't easily bonk on the send
   button multiple times.
 */
extern XP_Bool msg_DeliveryInProgress (MWContext *context);

/* Get the current fcc folder name, so we can sort them up at the top of the
   other folder lists with the other magic folders. */
const char* msg_GetDefaultFcc(XP_Bool news_p);


/* If a mail context exists, make sure it lists the queue folder.
   The passed-in context may be of any type. */
extern void msg_EnsureQueueFolderListed (MWContext* context);

/* Reads the first few bytes of the file and returns FALSE if it doesn't
   seem to be a mail folder.  (Empty and nonexistent files return TRUE.)
   If it doesn't seem to be one, the user is asked whether it should be
   written to anyway, and their answer is returned.
 */
extern XP_Bool msg_ConfirmMailFile (MWContext *context, const char *file_name);

/* returns a folder object of the given magic type, creating it if necessary.
   Context must be a mail window. */
extern MSG_Folder *msg_FindFolderOfType(MWContext* context, int type);


/* A callback to be used as a url pre_exit_fn just after message incorporation
   has happened; this notices which message was the first one incorporated,
   and selects it.  Some magic to do with replacement of `partial' messages
   happens as well.  There, is that specialized enough?  */
extern void msg_OpenFirstIncorporated(URL_Struct *url, int status,
									  MWContext *context);

/* A callback to be used as a url pre_exit_fn just after a folder is opened;
   this causes the display so that the first message that is unread (or the
   last message if all read) is visible to the user. */
extern void msg_ScrolltoFirstUnseen(URL_Struct *url, int status,
									MWContext *context);

/* Writes the selected mail messages to a mail folder.
   The context must be a mail context. If ismove is TRUE, then the messages
   are removed from the original mail folder. */
extern int msg_MoveOrCopySelectedMailMessages(MWContext *context,
											  const char *file_name,
											  XP_Bool ismove);



/* This returns a structure representing the given folder, which should not be
   the current folder. The context may be any kind of context - it is not
   altered. If there is no summary file, or if it is out of date, the folder
   will be parsed (which will block!  Danger!  Suckage! ###)

   The results of this must be free'd with msg_FreeFolderData.  */
extern int msg_GetFolderData (MWContext* context, MSG_Folder* folder,
							  struct mail_FolderData **folder_ret);


extern int msg_RescueOldFlags(MWContext* context,
							  struct mail_FolderData* flags_msgs,
							  struct mail_FolderData* msgs);

/* ===========================================================================
   Utilities specific to newsgroups and their Folders and ThreadEntries.
   ===========================================================================
 */

/* Constructs a canonical "news:" or "snews:" URL for the given host, being
   careful to let the host name and port be unspecified defaults when
   appropriate.   `suffix' may be 0, a group name, or a message ID.
   The context may be of any type; it is used only for error messages.
 */
extern char *msg_MakeNewsURL (MWContext *context,
                              struct MSG_NewsHost *host,
                              const char *suffix);

/* Populate the folder window with all known news hosts.
   The context must be a news context.
 */
extern int msg_ShowNewsHosts (MWContext *context);

/* Given a group name prefix, like "alt.fan", this will map the given
   function over each of the groups under that level ("addams", "authors",
   "blues-brothers", etc.)  If the prefix has no children, it fails, and
   never calls the mapper.  If the mapper returns negative, this function
   fails with that result.  The context may be of any type - it is not
   used for anything except alerts.
   (#### This is defined in libnet/mknewsgr.c which should probably be moved.)
 */
extern int msg_MapNewsgroupNames (MWContext *context,
                                  const char *news_host_and_port,
								  XP_Bool     is_secure,
                                  const char *name_prefix,
                                  msg_NewsgroupNameMapper mapper,
                                  void *closure);

/* Maps over names in the newsrc file rather than all extant groups.
   This requires a news context, and maps over the currently-loaded file,
   if any.
 */
extern int msg_MapNewsRCGroupNames (MWContext *context,
                                    msg_SubscribedGroupNameMapper mapper,
                                    void *closure);


/* If the given folder is a news host, lists the groups on that host
   (subscribed groups, or all groups, as appropriate.)  If the folder
   is a group-parent (meaning we are in "all groups" mode and this
   is a group like "alt.fan.*"), pulls in the next level of sub-groups.

   If update_article_counts_p is true, then after displaying the groups,
   the article counts will be filled in (by calling FE_GetURL()).
   Otherwise, they will be left in their un-updated state.

   The context must be a news context, and the folder must belong to it.
   Returns negative on error.
 */
extern int msg_ShowNewsgroupsOfFolder (MWContext *context, MSG_Folder *folder,
                                       XP_Bool update_article_counts_p);


/* Notes that some changes have been made to the currently-loaded newsrc file.
   A timer will go off after a while and save the changes if there is no more
   activity. */
extern void msg_NoteNewsrcChanged(MWContext* context);


/* This writes out the currently-loaded newsrc file if there are any
   unsaved changes.  The context must be a news context. */
extern int msg_SaveNewsRCFile (MWContext *context);


/* Given a group name, find the folder struct (if any) representing that
   group. */
extern struct MSG_Folder *msg_FindNewsgroupFolder(MWContext *context,
												  const char *group);


/* This ensures that things are currently set up to display the given group
   on the given host.  If the host is not listed, it will be added.  If the
   host is not open, it will be opened (closing any other open host, and
   populating the list with the subscribed groups on that host.)  If the
   passed-in group is not selected, it will be selected.

   group_name may be NULL, in which case only the host-related checks will
   be performed; if that host is selected already, this does nothing.
   Otherwise, it opens the given host and selects no group.

   The context must be a news context.
 */
extern int msg_EnsureNewsgroupSelected (MWContext *context,
                                        const char *host_and_port,
                                        const char *group_name,
                                        XP_Bool secure_p);

/* Call this when the number of unread messages changes in a newsgroup.
   The context must be a news context, and the folder must belong to it.
   If `remove_if_empty' is true, then the folder will be removed from
   the list and freed if the count is now 0.
 */
extern int msg_UpdateNewsFolderMessageCount (MWContext *context,
                                             struct MSG_Folder *folder,
                                             uint32 unread_messages,
                                             XP_Bool remove_if_empty);


/* Mapping news hosts to and from folders.  The contexts must be news
   contexts, and the folders/hosts must belong to those contexts. */
extern struct MSG_Folder *msg_NewsHostFolder (MWContext *context,
                                              MSG_NewsHost *host);
extern struct MSG_NewsHost *msg_NewsFolderHost (MWContext *context,
                                                MSG_Folder *folder);


/* Subscribe or unsubscribe to the given newsgroup.
   The context must be a news context, and the folder must belong to it.
   If there is new entry in the newsrc file for this group, one is added
   (but the file is not saved yet.)
 */
void msg_ToggleSubscribedGroup(MWContext* context, MSG_Folder* folder);

/* helper function for MSG_MarkMessageIDRead() -- don't call this. */
extern int msg_MarkNewsArticleRead (MWContext *context,
                                    const char *message_id,
                                    const char *xref);


/* Writes the selected news messages to a file (in mail folder format.)
   The context must be a news context.
 */
extern int msg_SaveSelectedNewsMessages (MWContext *, const char *file_name);

/* This nastiness is how msg_SaveSelectedNewsMessages() works. */
extern NET_StreamClass *msg_MakeAppendToFolderStream (int format_out,
                                                      void *closure,
                                                      URL_Struct *url,
                                                      MWContext *);

/* This returns a null-terminated array of the URLs of all selected
   messages.  Both the URL and the array must be freed by the caller.
   The context may be either a mail or news context. */
extern char **msg_GetSelectedMessageURLs (MWContext *context);


/* ===========================================================================
   The content-type converters for the MIME types.  These are provided by
   compose.c, but are registered by netlib rather than msglib, for some
   destined-to-be-mysterious reason.
   ===========================================================================
 */
extern NET_StreamClass *MIME_MessageConverter (int format_out, void *closure,
                                               URL_Struct *url,
                                               MWContext *context);

extern NET_StreamClass *MIME_RichtextConverter (int format_out, void *data_obj,
                                                URL_Struct *url,
                                                MWContext *context);

extern NET_StreamClass *MIME_EnrichedTextConverter (int format_out,
                                                    void *data_obj,
                                                    URL_Struct *url,
                                                    MWContext *context);


/* This probably should be in mime.h, except that mime.h should mostly be
   private to libmsg.  So it's here. */
extern void
msg_StartMessageDeliveryWithAttachments (MWContext *context,
						  void      *fe_data,
						  const char *from,
						  const char *reply_to,
						  const char *to,
						  const char *cc,
						  const char *bcc,
						  const char *fcc,
						  const char *newsgroups,
						  const char *followup_to,
						  const char *organization,
						  const char *message_id,
						  const char *news_url,
						  const char *subject,
						  const char *references,
						  const char *other_random_headers,
						  XP_Bool digest_p,
						  XP_Bool dont_deliver_p,
						  XP_Bool queue_for_later_p,
						  XP_Bool encrypt_p,
						  XP_Bool sign_p,
						  const char *attachment1_type,
						  const char *attachment1_body,
						  uint32 attachment1_body_length,
						  const struct MSG_AttachedFile *attachments,
						  void (*message_delivery_done_callback)
						       (MWContext *context, 
								void *fe_data,
								int status,
								const char *error_message));

extern void
msg_DownloadAttachments (MWContext *context,
						 void *fe_data,
						 const struct MSG_AttachmentData *attachments,
						 void (*attachments_done_callback)
						      (MWContext *context, 
							   void *fe_data,
							   int status, const char *error_message,
							   struct MSG_AttachedFile *attachments));


extern 
int msg_DoFCC (MWContext *context,
			   const char *input_file,  XP_FileType input_file_type,
			   const char *output_file, XP_FileType output_file_type,
			   const char *bcc_header_value,
			   const char *fcc_header_value);

extern char* msg_generate_message_id (void);


#ifdef XP_UNIX
extern int msg_DeliverMessageExternally(MWContext *, const char *msg_file);
#endif /* XP_UNIX */


XP_END_PROTOS

#endif /* !_MSG_H_ */
