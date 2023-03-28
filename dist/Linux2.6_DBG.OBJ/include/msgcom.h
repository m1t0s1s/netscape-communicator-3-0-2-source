/* -*- Mode: C; tab-width: 4 -*-
   msgcom.h --- prototypes for the mail/news reader module.
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 10-May-95.
 */

#ifndef _MSGCOM_H_
#define _MSGCOM_H_

/* Make sure we have a definition for MSG_AttachmentData (otherwise, some
   compilers will whine at us.)*/
#ifndef MIME_H
#include "mime.h"
#ifndef MIME_H
Error! mime.h no longer seems to define MIME_H
#endif
#endif

#ifndef OLD_MIME
# include "libmime.h"
#endif /* OLD_MIME */


/* This file defines all of the types and prototypes for communication
   between the msg library, which implements the mail and news applications,
   and the various front ends.

   Functions beginning with MSG_ are defined in the library, and are invoked
   by the front ends in response to user activity.

   Functions beginning with FE_ are defined by the front end, and are invoked
   by the message library to get things to happen on the screen.

   Many of these functions work on a set of messages.  Usually, the
   messages to use are whatever messages are currently selected.  If a
   message is explicitely specified, it is passed as an index into the
   displayed list, with the first line being number 0.

   There should always be at least one message selected.

 */


#include "xp_mcom.h"
#include "xp_core.h"
#include "ntypes.h"

/* Several things want to know this (including libnet/mknews.c) */
#define NEWS_PORT 119
#define SECURE_NEWS_PORT 563   


/* The types of pane configuration which can be used in the mail and
   news applications. */
typedef enum
{
  MSG_Vertical,
  MSG_Horizontal
} MSG_PANE_CONFIG;


/* The various kinds of icons which can be displayed in one of the outline
   list windows.
 */


typedef enum
{
  MSG_Folded,					/* This item is a container, and its contents
								   are hidden. */
  MSG_Expanded,					/* This item is a container, and its contents
								   are exposed. */
  MSG_Leaf						/* This item is not a container. */
} MSG_FLIPPY_TYPE;

/* The font type which should be used for presentation of cited text.
 */
typedef enum
{
  MSG_PlainFont,
  MSG_BoldFont,
  MSG_ItalicFont,
  MSG_BoldItalicFont
} MSG_FONT;

/* The font size which should be used for presentation of cited text.
 */
typedef enum
{
  MSG_NormalSize,
  MSG_Bigger,
  MSG_Smaller
} MSG_CITATION_SIZE;

typedef enum
{
  MSG_NotUsed,
  MSG_Checked,
  MSG_Unchecked
} MSG_COMMAND_CHECK_STATE;



/* Flags about a single message.  These values are used in the MSG_MessageLine
   struct, and are also used internally by libmsg in the `flags' slot in
   MSG_ThreadEntry and in a folder's summary cache file. */

#define MSG_FLAG_READ     0x0001    /* has been read */
#define MSG_FLAG_REPLIED  0x0002    /* a reply has been successfully sent */
#define MSG_FLAG_MARKED   0x0004    /* the user-provided mark */
#define MSG_FLAG_EXPUNGED 0x0008    /* already gone (when folder not
                                       compacted.)  Since actually
                                       removing a message from a
                                       folder is a semi-expensive
                                       operation, we tend to delay it;
                                       messages with this bit set will
                                       be removed the next time folder
                                       compaction is done.  Once this
                                       bit is set, it never gets
                                       un-set.  */
#define MSG_FLAG_HAS_RE   0x0010    /* whether subject has "Re:" on
                                       the front.  The folder summary
                                       uniquifies all of the strings
                                       in it, and to help this, any
                                       string which begins with "Re:"
                                       has that stripped first.  This
                                       bit is then set, so that when
                                       presenting the message, we know
                                       to put it back (since the "Re:"
                                       is not itself stored in the
                                       file.)  */
#define MSG_FLAG_ELIDED   0x0020    /* Whether the children of this
                                       sub-thread are folded in the
                                       display.  */
#define MSG_FLAG_EXPIRED  0x0040    /* If this flag is set, then this
                                       is not a "real" message, but is
                                       a dummy container representing
                                       an expired parent in a thread.  */
#define MSG_FLAG_SELECTED 0x0080    /* Whether the message is currently
                                       selected by the user.  This flag
                                       should never be saved out to disk;
                                       it is strictly run-time only. */
#define MSG_FLAG_UPDATING 0x0100    /* If set, then we have this message
                                       on the list of messages whose
                                       display needs refreshing in the FE. */
#define MSG_FLAG_DIRTY    0x0200    /* If set, then this message's flags
                                       need to be changed on disk. */
#define MSG_FLAG_PARTIAL  0x0400    /* If set, then this message's body is
                                       only the first ten lines or so of the
                                       message, and we need to add a link to
                                       let the user download the rest of it
                                       from the POP server. */
#define MSG_FLAG_QUEUED   0x0800    /* If set, this message is queued for
                                       delivery.  This only ever gets set on
                                       messages in the queue folder, but is
                                       used to protect against the case of
                                       other messages having made their way
                                       in there somehow -- if some other
                                       program put a message in the queue, we
                                       don't want to later deliver it! */



/* More flags about a message or folder.  These are flags generated on the fly
   by MSG_GetThreadLine() or MSG_GetFolderLine(), and put in the "moreflags"
   field, and aren't really stored internally anywhere.  */

#define MSG_MOREFLAG_LAST_SELECTED	0x0001	/* This is the last message that
											   was selected.  WinFE likes to
											   use this to remember which
											   message should have the
											   focus. */



/* Flags about a folder or a newsgroup.  Used in the MSG_FolderLine struct;
   also used internally in libmsg (the `flags' slot in MSG_Folder).  Note that
   these don't have anything to do with the above MSG_FLAG flags; they belong
   to different objects entirely.  */

    /* These flags say what kind of folder this is:
       mail or news, directory or leaf.
     */
#define MSG_FOLDER_FLAG_NEWSGROUP   0x0001  /* The type of this folder. */
#define MSG_FOLDER_FLAG_NEWS_HOST   0x0002  /* Exactly one of these three */
#define MSG_FOLDER_FLAG_MAIL        0x0004  /* flags will be set. */

#define MSG_FOLDER_FLAG_DIRECTORY   0x0008  /* Whether this is a directory:
                                               NEWS_HOSTs are always
                                               directories; NEWS_GROUPs can be
                                               directories if we are in ``show
                                               all groups'' mode; MAIL folders
                                               will have this bit if they are
                                               really directories, not files.
                                               (Note that directories may have
                                               zero children.) */

#define MSG_FOLDER_FLAG_ELIDED      0x0010  /* Whether the children of this
                                               folder are currently hidden in
                                               the listing.  This will only
                                               be present if the DIRECTORY
                                               bit is on. */

    /* These flags only occur in folders which have
       the MSG_FOLDER_FLAG_NEWSGROUP bit set, and do
       not have the MSG_FOLDER_FLAG_DIRECTORY or
       MSG_FOLDER_FLAG_ELIDED bits set.
     */

#define MSG_FOLDER_FLAG_MODERATED   0x0020  /* Whether this folder represents
                                               a moderated newsgroup. */
#define MSG_FOLDER_FLAG_SUBSCRIBED  0x0040  /* Whether this folder represents
                                               a subscribed newsgroup. */
#define MSG_FOLDER_FLAG_NEW_GROUP   0x0080  /* A newsgroup which has just
                                               been added by the `Check
                                               New Groups' command. */


    /* These flags only occur in folders which have
       the MSG_FOLDER_FLAG_MAIL bit set, and do
       not have the MSG_FOLDER_FLAG_DIRECTORY or
       MSG_FOLDER_FLAG_ELIDED bits set.

	   The numeric order of these flags is important;
	   folders with these flags on get displayed first,
	   in reverse numeric order, before folders that have
	   none of these flags on.  (Note that if a folder is,
	   say, *both* inbox and sentmail, then it's numeric value
	   will be even bigger, and so will bubble up to where the
	   inbox generally is.  What a hack!)
     */

#define MSG_FOLDER_FLAG_TRASH       0x0100  /* Whether this is the trash
                                               folder. */
#define MSG_FOLDER_FLAG_SENTMAIL	0x0200	/* Whether this is a folder that
											   sent mail gets delivered to.
											   This particular magic flag is
											   used only during sorting of
											   folders; we generally don't care
											   otherwise. */
#define MSG_FOLDER_FLAG_DRAFTS      0x0400	/* Whether this is the folder in
                                               which unfinised, unsent messages
                                               are saved for later editing. */
#define MSG_FOLDER_FLAG_QUEUE       0x0800  /* Whether this is the folder in
                                               which messages are queued for
                                               later delivery. */
#define MSG_FOLDER_FLAG_INBOX       0x1000  /* Whether this is the primary
                                               inbox folder. */



#define MSG_FOLDER_FLAG_SELECTED	0x2000 /* Whether this folder is currently
											  selected. */




/* This set enumerates the header fields which may be displayed in the
   message composition window.
 */
typedef uint32 MSG_HEADER_SET;
#define MSG_FROM_HEADER_MASK			0x0001
#define MSG_REPLY_TO_HEADER_MASK		0x0002
#define MSG_TO_HEADER_MASK				0x0004
#define MSG_CC_HEADER_MASK				0x0008
#define MSG_BCC_HEADER_MASK				0x0010
#define MSG_FCC_HEADER_MASK				0x0020
#define MSG_NEWSGROUPS_HEADER_MASK		0x0040
#define MSG_FOLLOWUP_TO_HEADER_MASK		0x0080
#define MSG_SUBJECT_HEADER_MASK			0x0100
#define MSG_ATTACHMENTS_HEADER_MASK		0x0200



/* This enumerates all of the mail/news-specific commands (those which are
   not shared with the web browsers.  The front ends should invoke each of
   these menu items through the MSG_Command() function like so:

      MSG_Command (context, MSG_PostReply);

   This interface is used for selections of normal menu items, toolbar
   buttons, and keyboard equivalents.  Clicks in the scrolling list windows,
   drag-and-drop, and the "folders" menus are handled differently.
 */
typedef enum
{
  /* FILE MENU
	 =========
   */
  MSG_GetNewMail,
  MSG_DeliverQueuedMessages,
  MSG_OpenFolder,
  MSG_NewFolder,
  MSG_CompressFolder,
  MSG_CompressAllFolders,
  MSG_OpenNewsHost,
  MSG_AddNewsGroup,
  MSG_EmptyTrash,
  MSG_Print,				/* not a command - used for selectability only */

  /* EDIT MENU
	 =========
   */
  MSG_Undo,
  MSG_Redo,
  MSG_DeleteMessage,
  MSG_DeleteFolder,
  MSG_CancelMessage,
  MSG_DeleteNewsHost,
  MSG_SubscribeSelectedGroups,		/* in the popup menu only */
  MSG_UnsubscribeSelectedGroups,	/* in the popup menu only */
  MSG_SelectThread,
  MSG_SelectMarkedMessages,
  MSG_SelectAllMessages,
  MSG_UnselectAllMessages,		/* not in the menu */
  MSG_MarkSelectedMessages,		/* not in the menu */
  MSG_UnmarkSelectedMessages,	/* not in the menu */

  MSG_FindAgain,

  /* VIEW/SORT MENUS
	 ===============
   */
  MSG_ReSort,
  MSG_ThreadMessages,
  MSG_SortBackward,
  MSG_SortByDate,
  MSG_SortBySubject,
  MSG_SortBySender,
  MSG_SortByMessageNumber,
  MSG_Rot13Message,

  MSG_AttachmentsInline,
  MSG_AttachmentsAsLinks,

  MSG_AddFromNewest,
  MSG_AddFromOldest,
  MSG_GetMoreMessages,
  MSG_GetAllMessages,

  MSG_WrapLongLines,

  /* MESSAGE MENU
	 ============
   */
  MSG_EditAddressBook,
  MSG_EditAddress,
  MSG_PostNew,
  MSG_PostReply,
  MSG_PostAndMailReply,
  MSG_MailNew,
  MSG_ReplyToSender,
  MSG_ReplyToAll,
  MSG_ForwardMessage,
  MSG_ForwardMessageQuoted,
  MSG_MarkSelectedRead,
  MSG_MarkSelectedUnread,
  MSG_MarkMessage,
  MSG_UnmarkMessage,
  MSG_UnmarkAllMessages,
  MSG_CopyMessagesInto,			/* Note: these are magic! */
  MSG_MoveMessagesInto,			/* Note: these are magic! */
  MSG_SaveMessagesAs,
  MSG_SaveMessagesAsAndDelete,

  /* GO MENU
	 =======
   */
  MSG_FirstMessage,			/* not in menu */
  MSG_NextMessage,
  MSG_PreviousMessage,
  MSG_LastMessage,			/* not in menu */
  MSG_FirstUnreadMessage,
  MSG_NextUnreadMessage,
  MSG_PreviousUnreadMessage,
  MSG_LastUnreadMessage,	/* not in menu */
  MSG_FirstMarkedMessage,
  MSG_NextMarkedMessage,
  MSG_PreviousMarkedMessage,
  MSG_LastMarkedMessage,	/* not in menu */
  MSG_MarkThreadRead,		/* toolbar only */
  MSG_MarkAllRead,			/* toolbar only */


  /* OPTIONS MENU
	 ============
   */
  MSG_ShowSubscribedNewsGroups,
  MSG_ShowActiveNewsGroups,
  MSG_ShowAllNewsGroups,
  MSG_CheckNewNewsGroups,
  MSG_ShowAllMessages,
  MSG_ShowOnlyUnreadMessages,
  MSG_ShowMicroHeaders,
  MSG_ShowSomeHeaders,
  MSG_ShowAllHeaders,


  /* COMPOSITION FILE MENU
	 =====================
   */
  MSG_SendMessage,
  MSG_SendMessageLater,
  MSG_Attach,
  MSG_QuoteMessage,
  /* save */

  /* COMPOSITION VIEW MENU
	 =====================
   */
  MSG_ShowFrom,
  MSG_ShowReplyTo,
  MSG_ShowTo,
  MSG_ShowCC,
  MSG_ShowBCC,
  MSG_ShowFCC,
  MSG_ShowPostTo,
  MSG_ShowFollowupTo,
  MSG_ShowSubject,
  MSG_ShowAttachments,

  /* COMPOSITION OPTIONS MENU
	 ========================
   */
  MSG_SendFormattedText,
  MSG_AttachAsText,
  MSG_SendEncrypted,
  MSG_SendSigned,
  MSG_SecurityAdvisor

} MSG_CommandType;


/* This structure represents a single line in the folder list window.
 */
typedef struct MSG_FolderLine
{
  MSG_FLIPPY_TYPE flippy_type;
  uint16 flags;					/* MSG_FOLDER_FLAG_* values */
  uint16 moreflags;				/* MSG_MOREFLAG_* values */
  const char *name;				/* The name of the folder to display */
  uint16 depth;					/* How far this line should be indented */

  /* The below are used only if the icon type is MSG_NewsgroupIcon or
	 MSG_FolderIcon. */
  int unseen;					/* Number of unseen articles. (If negative,
								   then we don't know yet; display question
								   marks or something.*/
  int total;					/* Total number of articles. */
} MSG_FolderLine;


/* This structure represents a single line in the message list window
   (either mail or news messages.)
 */
typedef struct MSG_MessageLine
{
  MSG_FLIPPY_TYPE flippy_type;
  uint16 flags;					/* MSG_FLAG_* values */
  uint16 moreflags;				/* MSG_MOREFLAG_* values */
  const char *from;				/* The name of the sender (or recipient) */
  const char *subject;			/* The subject of the message */
  uint16 depth;					/* How far this line should be indented */
  uint16 lines;					/* How many lines this message contains */
  time_t date;					/* The date to be displayed */
} MSG_MessageLine;

/* This structure represents one of the ancestors of a FolderLine or
   MessageLine.  An array of these structures is (optionally) returned when
   MSG_GetFolderLine() or MSG_GetThreadLine() is called.  Such an array will
   have the ancester at depth zero in entry zero, depth one in entry one, etc.
   The array must be freed with XP_FREE. The array will have an entry for every
   ancestor of the requested line, including one for that line itself. */

typedef struct MSG_AncestorInfo
{
  XP_Bool has_next;				/* Whether there is another folder or message
								   below this one that has the same parent that
								   this one has.  (Useful for drawing Windows
								   pipes stuff.)*/
  XP_Bool has_prev;				/* Whether there is another folder or message
								   above this one that has the same parent that
								   this one has. */
} MSG_AncestorInfo;

/* This structure is used as an annotation about the font and size information
   included in a text string passed to us by the front end after the user has
   edited their message.
 */
typedef struct MSG_FontCode
{
  uint32 pos;
  uint32 length;
  MSG_FONT font;
  uint8 font_size;
  XP_Bool fixed_width_p;
} MSG_FontCode;


XP_BEGIN_PROTOS

/* This is the front end's single interface to executing mail/news menu
   and toolbar commands.  See the comment above the definition of the
   MSG_CommandType enum.
 */
extern void MSG_Command (MWContext *context, MSG_CommandType command);


/* Before the front end displays any menu (each time), it should call this
   function for each command on that menu to determine how it should be
   displayed.

   selectable_p:	whether the user should be allowed to select this item;
					if this is FALSE, the item should be grayed out.

   selected_p:		if this command is a toggle or multiple-choice menu item,
					this will be MSG_Checked if the item should be `checked',
                    MSG_Unchecked if it should not be, and MSG_NotUsed if the
                    item is a `normal' menu item that never has a checkmark.

   display_string:	the string to put in the menu.  l10n handwave handwave.

   plural_p:		if it happens that you're not using display_string, then
					this tells you whether the string you display in the menu
					should be singular or plural ("Delete Message" versus
					"Delete Selected Messages".)  If you're using
					display_string, you don't need to look at this value.

   Returned value is negative if something went wrong.
 */
extern int MSG_CommandStatus (MWContext *context,
							  MSG_CommandType command,
							  XP_Bool *selectable_p,
							  MSG_COMMAND_CHECK_STATE *selected_p,
							  const char **display_string,
							  XP_Bool *plural_p);


/* Set the value of a toggle-style command to the given value.  Returns
   negative if something went wrong (like, you gave it a non-toggling
   command, or you passed in MSG_NotUsed for the check state.) */
extern int MSG_SetToggleStatus(MWContext* context, MSG_CommandType command,
							   MSG_COMMAND_CHECK_STATE value);

/* Queries whether the given toggle-style command is on or off. */
extern MSG_COMMAND_CHECK_STATE MSG_GetToggleStatus(MWContext* context,
												   MSG_CommandType command);


/* Determines whether we are currently actually showing the recipients
   in the "Sender" column of the display (because we are in the "sent"
   folder).  FE's should call this in their FE_UpdateToolbar to change
   the title of this column.*/

extern XP_Bool MSG_DisplayingRecipients(MWContext* context);



/* The MSG library will call this front end function when the sensitivity of
   items in the toolbar may have changed, and the FE should then call 
   MSG_CommandStatus() for each of the items in the toolbar, and adjust the
   display appropriately.  In a mail context, this is also a good time to call
   MSG_DisplayingRecipients(), and change the title of the appropriate column
   to be "Sender" or "Recipient".
 */
extern void FE_UpdateToolbar (MWContext *context);



/* The msg library calls the front end via these functions to tell it to
   replace part of the contents of the two outline lists.  The front end should
   invalidate the displayed data for "length" items starting at "index".  It
   should vertically grow the list so that it can display "totallines" entries.

   These calls are also called whenever the selection changes.  Multiple thread
   lines or folders may be selected; the list of selected lines is passed via
   an array.

   Finally, these calls take a "visibleline" parameter; if nonnegative, this
   indicates which line should be scrolled to be visible. */

extern void FE_UpdateFolderList(MWContext *cx, int index, int length,
								int totallines, int visibleline,
								int* select, int numselect);
extern void FE_UpdateThreadList(MWContext *cx, int index, int length,
								int totallines, int visibleline,
								int* select, int numselect);

/* The front end can use this call to determine what the indentation depth is
   needed to display all the icons in the folder list.  The XFE uses this to
   dynamically resize the icon column. In true C style, the number returned is
   actually one bigger than the biggest depth the FE will ever get. */
extern int MSG_GetFolderMaxDepth(MWContext* cx);


/* The front end uses these calls to determine what to display on a particular
   line in the outline lists.  They return TRUE on success, FALSE if the given
   line doesn't exist.  If ancestorinfo is not NULL, then it will be filled
   with info about all the ancestors of this message, including the message
   itself.  (See the comments by MSG_AncestorInfo.)  ancestorinfo will be
   set to NULL if memory is not available. */
extern XP_Bool MSG_GetFolderLine(MWContext* cx, int line,
								 MSG_FolderLine* data,
								 MSG_AncestorInfo** ancestorinfo);
extern XP_Bool MSG_GetThreadLine(MWContext* cx, int line,
								 MSG_MessageLine* data,
								 MSG_AncestorInfo** ancestorinfo);



/* Takes a time (most likely, the one returned in the MSG_MessageLine
   structure by MSG_GetThreadLine()) and formats it into a compact textual
   representation (like "Thursday" or "09:53:17" or "10/3 16:15"), based on
   how that date relates to the current time.  This is returned in a
   statically allocated buffer; the caller should copy it somewhere. */
extern const char* MSG_FormatDate(MWContext* cx, time_t date);


/* This is called to update the contents of the "Copy Messages Into"
   and "Move Messages Into" cascading menus.  It is called once at 
   startup, and any time the set of folders is known to have changed.
 */
extern void FE_UpdateFolderMenus (MWContext *cx, MSG_FolderLine *lines,
								  int numlines);


/*
 * The msg library calls this to get a directory name which it will run
 * through to generate a list of folders and their corresponding file
 * names
 */

extern const char* FE_GetFolderDirectory(MWContext* cx);


/*
 * The msg library calls this to get a temporary filename and type
 * (suitable for passing to XP_FileOpen, etc) to be used for storing a
 * temporary working copy of the given filename and type.  The returned
 * file will usually be renamed back onto the given filename, using
 * XP_FileRename().
 *
 * The returned filename will be freed using XP_FREE().
 */

/* #### this should be replaced with a backup-version-hacking XP_FileOpen */
extern char* FE_GetTempFileFor(MWContext* context, const char* fname,
							   XP_FileType ftype, XP_FileType* rettype);


/* Call these to initialize a new context to be ready for mail or news
   display.  The front end call this at a time where it will be ready for
   any of the above FE_ callbacks to happen.
 */
extern void MSG_InitializeMailContext(MWContext* context);
extern void MSG_InitializeNewsContext(MWContext* context);

/* Call these when about to blow away a context that is used for mail or news.
 */

extern void MSG_CleanupMailContext(MWContext*);
extern void MSG_CleanupNewsContext(MWContext*);
extern void MSG_CleanupMessageCompositionContext(MWContext*);


#ifdef XP_UNIX
/* This is how the XFE implements non-POP message delivery.  The given
 donefunc will be called when the incorporate actually finishes, which
 may be before or after this function returns. The boolean passed to the
 donefunc will be TRUE if the incorporate succeeds, and FALSE if it fails
 for any reason. */
extern void MSG_IncorporateFromFile(MWContext* context, XP_File infid,
									void (*donefunc)(void*, XP_Bool),
									void* doneclosure);
#endif /* XP_UNIX */


/* Start off a find operation.  To do "find again", use
   MSG_Command(context, MSG_FindAgain).  It's important to check the error
   code and report any error; that's how the "Not found" message will
   appear. */

extern int MSG_DoFind(MWContext* context, const char* searchstring,
					  XP_Bool casesensitive);



/* PREFERENCES
   ===========
 */

#ifndef OLD_MIME
/* Returns the window's current preferences state. */
extern void MSG_GetContextPrefs(MWContext *context,
								XP_Bool *all_headers_p,
								XP_Bool *micro_headers_p,
								XP_Bool *no_inline_p,
								XP_Bool *rot13_p,
								XP_Bool *wrap_long_lines_p);
#endif /* OLD_MIME */


extern void MSG_SetPaneConfiguration (MWContext *context,
									  MSG_PANE_CONFIG type);

extern void MSG_SetCitationStyle (MWContext *context,
								  MSG_FONT font,
								  MSG_CITATION_SIZE size,
								  const char *color);

extern void MSG_SetPlaintextFont (MWContext *context,
								  XP_Bool fixed_width_p);

extern void MSG_SetPopHost (const char *host);


extern void MSG_SetAutoShowFirstUnread(XP_Bool value);

/* Whether to initially show threads expanded or elided.  NYI. */
extern void MSG_SetExpandMessageThreads(XP_Bool isnews,/* False for mail,
														  True for news */
										XP_Bool value);


/* Whether to automatically quote original message when replying. */
extern void MSG_SetAutoQuoteReply(XP_Bool value);


/* CALLBACKS IN THE LISTS
   ======================
 */

/* The FE calls these in response to mouse gestures on the scrolling lists. */


/* Change the flippy state on this thread line, if possible. */
extern void MSG_ToggleThreadExpansion (MWContext *context, int line_number);

/* In news contexts only, this turns on or off the subscribed bit on the
   newsgroup displayed on the given line. */
extern void MSG_ToggleSubscribed(MWContext* context, int line_number);


/* Adds one message to the current selection.  If exclusive_p is true, then
   unselects all the other messages first (as if MSG_UnselectAllMessages
   was called, but more efficiently), and also causes the message to be opened.
 */
extern void MSG_SelectMessage(MWContext* context, int line_number,
							  XP_Bool exclusive_p);

/* Adds a range of messages to the current selection.  To be called by the FE
   when the user Shift-Clicks in the window.  This will unselect everything and
   then select a range of messages, from the last message that was selected
   with MSG_SelectMessage to the one given here. */
extern void MSG_SelectRangeTo(MWContext* context, int line_number);

/* Unselects a single message. */
extern void MSG_UnselectMessage(MWContext* context, int line_number);

/* Toggle the selection bit of the given message on or off. */
extern void MSG_ToggleSelectMessage(MWContext* context, int line_number);

/* Toggles the message mark on the selected message */
extern void MSG_ToggleMark(MWContext* context, int line_number);

/* Toggles the read bit on the selected message */
extern void MSG_ToggleRead(MWContext* context, int line_number);

/* Return how many messages are currently selected. */
extern int MSG_GetNumSelectedMessages(MWContext* context);


/* These routines are similar to the MSG_SelectMessage* routines above,
   but work in the folder list. */
extern void MSG_SelectFolder(MWContext *context, int line_number,
							 XP_Bool exclusive_p);
extern void MSG_SelectFolderRangeTo(MWContext* context, int line_number);
extern void MSG_UnselectFolder(MWContext* context, int line_number);
extern void MSG_ToggleSelectFolder(MWContext* context, int line_number);
extern void MSG_ToggleFolderExpansion (MWContext *context, int line_number);


/* These are invoked by the items on the `Copy/Move Message Into' cascading
   menus, and by drag-and-drop.  The folder name should be the full pathname
   of the folder in URL (Unix) file name syntax.  (You can get the name of a
   line with MSG_GetFolderName().)
 */
extern void MSG_CopySelectedMessagesInto (MWContext *, const char *folder);
extern void MSG_MoveSelectedMessagesInto (MWContext *, const char *folder);


/* QUERIES INTO THE MSG LIB
   ========================
 */

/*
 * Get the filename for the folder on the given line.  The resulting
 * pointer should *NOT* be freed with XP_FREE().  This string is perfect for
 * passing to calls to MSG_MoveMessages() and friends.
 */
extern char* MSG_GetFolderName(MWContext* context, int line_number);


/*
 * Get the filename for the nth folder, counting all folders including
 * elided ones.
 */

extern char* MSG_GetFolderNameUnfolded(MWContext* context, int line_number);



/* BIFF
   ====
 */

/* biff is the unixy name for "Check for New Mail".  It comes from the unix
   command of the same name; the author of that command named it after his dog,
   who would bark at him whenever he had new mail...  */

/* The different biff states we can be in:  */

typedef enum
{
  MSG_BIFF_NewMail,				/* User has new mail waiting. */
  MSG_BIFF_NoMail,				/* No new mail is waiting. */
  MSG_BIFF_Unknown				/* We dunno whether there is new mail. */
} MSG_BIFF_STATE;


/* Set the preference of how often to run biff.  If zero is passed in, then
   never check. */

extern void MSG_SetBiffInterval(int seconds);



#ifdef XP_UNIX
/* Set the file to stat, instead of using pop3.  This is for the Unix movemail
   nonsense.  If the filename is NULL (the default), then use pop3. */
extern void MSG_SetBiffStatFile(const char* filename);
#endif



/* Initialize the biff context.  Note that biff contexts exist entirely
   independent of mail contexts; it's up to the FE to decide what order they
   get created and stuff. */

extern int MSG_BiffInit(MWContext* context);


/* The biff context is about to go away.  The FE must call this first to
   clean up. */

extern int MSG_BiffCleanupContext(MWContext* context);


/* Causes a biff check to occur immediately.  This gets caused
   automatically by MSG_SetBiffInterval or whenever libmsg gets new mail. */

extern void MSG_BiffCheckNow(MWContext* context);



/* Tell the FE to render in all the right places this latest knowledge as to
   whether we have new mail waiting. */
extern void FE_UpdateBiff(MSG_BIFF_STATE state);



/* OTHER INTERFACES
   ================
 */

/* Certain URLs must always be displayed in certain types of windows:
   for example, all "mailbox:" URLs must go in the mail window, and
   all "http:" URLs must go in a browser window.  These predicates
   look at the address and say which window type is required.
 */
extern XP_Bool MSG_RequiresMailWindow (const char *url);
extern XP_Bool MSG_RequiresNewsWindow (const char *url);
extern XP_Bool MSG_RequiresBrowserWindow (const char *url);
extern XP_Bool MSG_RequiresComposeWindow (const char *url);

/* If this URL requires a particular kind of window, and this is not
   that kind of window, then we need to find or create one.
 */
extern XP_Bool MSG_NewWindowRequired (MWContext *context, const char *url);

/* If we're in a mail window, and clicking on a link which will itself
   require a mail window, then don't allow this to show up in a different
   window - since there can be only one mail window.
 */
extern XP_Bool MSG_NewWindowProhibited (MWContext *context, const char *url);


/* When the user changes their default news host in preferences, it calls
   NET_SetNewsHost(), which then calls this to ensure that that host is in
   the list.
 */
extern int MSG_NewsHostChanged (MWContext *context, const char *host_and_port);

/* When the user changes their mail directory in preferences, the fe calls
   MSG_MailDirectoryChanged() to rebuild the mail folder list
 */
extern void MSG_MailDirectoryChanged (MWContext *context);

/* When the user changes their news directory in preferences, the fe calls
   MSG_NewsDirectoryChanged() to rebuild the news folder list
 */
extern void MSG_NewsDirectoryChanged (MWContext *context);

/* The NNTP module of netlib calls these to feed XOVER data to the message
   library, in response to a news:group.name URL having been opened.
   If MSG_FinishXOVER() returns a message ID, that message will be loaded
   next (used for selecting the first unread message in a group after
   listing that group.)

   The "out" arguments are (if non-NULL) a file descriptor to write the XOVER
   line to, followed by a "\n".  This is used by the XOVER-caching code.
 */
extern int MSG_InitXOVER (MWContext *context,
						  const char *host_and_port, XP_Bool secure_p,
						  const char *group_name,
						  uint32 first_msg, uint32 last_msg,
						  uint32 oldest_msg, uint32 youngest_msg,
						  void **data);
extern int MSG_ProcessXOVER (MWContext *context, char *line, void **data);
extern int MSG_ProcessNonXOVER (MWContext *context, char *line, void **data);
extern int MSG_FinishXOVER (MWContext *context, void **data, int status);

/* In case of XOVER failed due to the authentication process, we need to
   do some clean up. So that we could have a fresh start once we pass the
   authentication check.
*/
extern int MSG_ResetXOVER (MWContext *context, void **data);


/* These calls are used by libnet to determine which articles it ought to
   get in a big newsgroup. */

extern int
MSG_GetRangeOfArtsToDownload(MWContext* context, const char* host_and_port,
							 XP_Bool secure_p, const char* group_name,
							 int32 first_possible, /* Oldest article available
													  from newsserver*/
							 int32 last_possible, /* Newest article available
													 from newsserver*/
							 int32 maxextra,
							 int32* first,
							 int32* last);


extern int
MSG_AddToKnownArticles(MWContext* context, const char* host_and_port,
					   XP_Bool secure_p, const char* groupname,
					   int32 first, int32 last);



/* After displaying a list of newsgroups, we need the NNTP module to go and
   run "GROUP" commands for the ones for which we don't know the unread
   article count.  This function returns the subset of the currently displayed
   groups whose counts we don't already have.
 */
extern char **MSG_GetNewsRCList (MWContext *context,
								 const char *host_and_port,
								 XP_Bool secure_p);


/* In response to a "news://host/" URL; this is called once for each group
   that was returned by MSG_GetNewsRCList(), after the NNTP GROUP command has
   been run.  It's also called whenever we actually visit the group (the user
   clicks on the newsgroup line), in case the data has changed since the
   initial passthrough.  The "nowvisiting" parameter is TRUE in the latter
   case, FALSE otherwise. */
extern int MSG_DisplaySubscribedGroup (MWContext *context, const char *group,
									   uint32 oldest_message,
									   uint32 youngest_message,
									   uint32 total_messages,
									   XP_Bool nowvisiting);

/* In response to a "news://host/?newgroups" URL, to ask the server for a 
   list of recently-added newsgroups.  Similar to MSG_DisplaySubscribedGroup,
   except that in this case, the group is not already in the list. */
extern int MSG_DisplayNewNewsGroup (MWContext *context,
									const char *host_and_port,
									XP_Bool secure_p, const char *group_name,
									int32 oldest_message,
									int32 youngest_message);


/* The NNTP module calls this with a message ID and "xrefs" field just after
   the headers of an article have been retrieved and displayed (but before
   the body has been fully retrieved) so that we may mark it as read in the
   newsrc.  The "mailbox" module does the same thing, but with no "xrefs".
 */
extern int MSG_MarkMessageIDRead (MWContext *context, const char *message_id,
								  const char *xref);


/* News servers work better if you ask for message numbers instead of IDs.
   So, the NNTP module asks us what the group and number of an ID is with
   this.  If we don't know, we return 0 for both.   The context may be any
   type.
 */
extern void MSG_NewsGroupAndNumberOfID (MWContext *context,
										const char *message_id,
										const char **group_return,
										uint32 *message_number_return);


/* libnet calls this if it got an error 441 back from the newsserver.  That
   error almost certainly means that the newsserver already has a message
   with the same message id.  If this routine returns TRUE, then we were
   pretty much expecting that error code, because we know we tried twice to
   post the same message, and we can just ignore it. */
extern XP_Bool MSG_IsDuplicatePost(MWContext* context);


/* libnet uses this on an error condition to tell libmsg to generate a new
   message-id for the given composition. */
extern void MSG_ClearMessageID(MWContext* context);


/* libnet uses this to determine the message-id for the given composition (so
   it can test if this message was already posted.) */
extern const char* MSG_GetMessageID(MWContext* context);


/* The "news:" and "mailbox:" protocol handlers call this when a message is
   displayed, so that we can use the contents of the headers when composing
   replies.
 */
#ifdef OLD_MIME
struct MIME_HTMLGeneratorHeaders;
extern void MSG_ActivateReplyOptions (MWContext *context,
									  const struct MIME_HTMLGeneratorHeaders
									    *headers);
#else  /* !OLD_MIME */
extern void MSG_ActivateReplyOptions (MWContext *, MimeHeaders *);
#endif /* !OLD_MIME */


/* The "mailbox:" protocol module uses these routines to invoke the mailbox
   parser in libmsg.
 */
extern int MSG_BeginOpenFolderSock (MWContext *context, 
									const char *folder_name,
									const char *message_id, int msgnum,
									void **folder_ptr);
extern int MSG_FinishOpenFolderSock (MWContext *context, 
									 const char *folder_name,
									 const char *message_id, int msgnum,
									 void **folder_ptr);
extern void MSG_CloseFolderSock (MWContext *context, const char *folder_name,
								 const char *message_id, int msgnum,
								 void *folder_ptr);
extern int MSG_OpenMessageSock (MWContext *context, const char *folder_name,
								const char *msg_id, int msgnum,
								void *folder_ptr, void **message_ptr,
								int32 *content_length);
extern int MSG_ReadMessageSock (MWContext *context, const char *folder_name,
								void *message_ptr, const char *message_id,
								int msgnum, char *buffer, int32 buffer_size);
extern void MSG_CloseMessageSock (MWContext *context, const char *folder_name,
								  const char *message_id, int msgnum,
								  void *message_ptr);
extern void MSG_PrepareToIncUIDL(MWContext* context, URL_Struct* url,
								 const char* uidl);

/* This is how "mailbox:?empty-trash" works
 */
extern int MSG_BeginEmptyTrash(MWContext* context, URL_Struct* url,
							   void** closure);
extern int MSG_FinishEmptyTrash(MWContext* context, URL_Struct* url,
								void* closure);
extern int MSG_CloseEmptyTrashSock(MWContext* context, URL_Struct* url,
								   void* closure);

/* This is how "mailbox:?compress-folder" and
   "mailbox:/foo/baz/nsmail/inbox?compress-folder" works. */

extern int MSG_BeginCompressFolder(MWContext* context, URL_Struct* url,
								   const char* foldername, void** closure);
extern int MSG_FinishCompressFolder(MWContext* context, URL_Struct* url,
									const char* foldername, void* closure);
extern int MSG_CloseCompressFolderSock(MWContext* context, URL_Struct* url,
									   void* closure);
/* This is how "mailbox:?deliver-queued" works
 */
extern int MSG_BeginDeliverQueued(MWContext* context, URL_Struct* url,
								  void** closure);
extern int MSG_FinishDeliverQueued(MWContext* context, URL_Struct* url,
								   void* closure);
extern int MSG_CloseDeliverQueuedSock(MWContext* context, URL_Struct* url,
									  void* closure);


/* Returns the number of bytes available on the disk where the mail
   folders live - this is so we know whether there is room to incorporate
   new mail.  (In other words, it should return disk space for the file system
   that the directory FE_GetFolderDirectory() returns is on.)
 */
extern uint32 FE_DiskSpaceAvailable (MWContext *context);


/* The POP3 protocol module uses these routines to hand us new messages.
 */
extern XP_Bool MSG_BeginMailDelivery (MWContext *context);
extern void MSG_AbortMailDelivery (MWContext *context);
extern void MSG_EndMailDelivery (MWContext *context);
extern void *MSG_IncorporateBegin (FO_Present_Types format_out,
											   char *pop3_uidl,
											   URL_Struct *url,
											   MWContext *context);
extern int MSG_IncorporateWrite (void *closure,
								 const char *block, int32 length);
extern int MSG_IncorporateComplete (void *closure);
extern int MSG_IncorporateAbort (void *closure, int status);





/* This is how the netlib registers the converters relevant to MIME message
   display and composition.
 */
void MSG_RegisterConverters (void);


/* 
   SECURE MAIL RELATED
   =====================
 */


/* FEs call this any time the set of recipients on the compose window
   changes.  They should make the state (and sensitivity) of the "sign"
   and "encrypt" checkboxes, and the state of the "security" button,
   correspond to the returned boolean values.

   Maybe this function doesn't really belong here, but it's as good a
   place as any...
  */
/* (This is maybe not the most appropriate header file for this proto.) */
extern void SEC_GetMessageCryptoViability(const char *from,
										  const char *reply_to,
										  const char *to,
										  const char *cc,
										  const char *bcc,
										  const char *newsgroups,
										  const char *followup_to,
										  XP_Bool *signing_possible_return,
										  XP_Bool *encryption_possible_return);


/* Returns TRUE if the user has selected the preference that says that new
   mail messages should be encrypted by default. 
 */
extern XP_Bool MSG_GetMailEncryptionPreference(void);

/* Returns TRUE if the user has selected the preference that says that new
   mail messages should be cryptographically signed by default. 
 */
extern XP_Bool MSG_GetMailSigningPreference(void);

/* Returns TRUE if the user has selected the preference that says that new
   news messages should be cryptographically signed by default. 
 */
extern XP_Bool MSG_GetNewsSigningPreference(void);


/* ===========================================================================
   MESSAGE COMPOSITION WINDOW
   ===========================================================================
 */

/* This is how the `mailto' parser asks the message library to create a
   message composition window.  That window has its own context.  The
   `old_context' arg is the context from which the mailto: URL was loaded.
   It may be NULL.

   Any of the fields may be 0 or "".  Some of them (From, BCC, Organization,
   etc) will be given default values if none is provided.

   This returns the new context.
 */
extern MWContext* MSG_ComposeMessage(MWContext *old_context,
									 const char *from,
									 const char *reply_to,
									 const char *to,
									 const char *cc,
									 const char *bcc,
									 const char *fcc,
									 const char *newsgroups,
									 const char *followup_to,
									 const char *organization,
									 const char *subject,
									 const char *references,
									 const char *other_random_headers,
									 const char *attachment,
									 const char *newspost_url,
									 const char *body,
									 XP_Bool encrypt_p,
									 XP_Bool sign_p);

/* This is how the message library asks the front end to create a mail
   composition window.  The FE should create a context (with type
   MWContextMessageComposition) and a window, but the window should
   not (yet) be placed on the screen.  The context may be NULL only if
   NULL was passed to MSG_ComposeMessage().
 */
extern MWContext *FE_MakeMessageCompositionContext (MWContext *old_context);


/* After the context has been created, and some libmsg-specific state has
   been set up on it, libmsg will call this to get the FE to fill in the
   default values of the text fields on the window it just created.
   The FE should then call *back* into the message library with
   MSG_SetHeaderContents() just as if the user had typed in the text
   the library just handed it (this simplifies things in the library
   a bit...)
 */
extern void FE_InitializeMailCompositionContext (MWContext *context,
												 const char *from,
												 const char *reply_to,
												 const char *to,
												 const char *cc,
												 const char *bcc,
												 const char *fcc,
												 const char *newsgroups,
												 const char *followup_to,
												 const char *subject,
												 const char *attachment);

/* The message library uses this to initialize the text of the message with
   things like the signature file or quoted text.  The text should get inserted
   at the place the insertion cursor is sitting at.  Afterwords, the insertion
   cursor should be at the end of the inserted text, unless
   leaveCursorAtBeginning is TRUE, in which case it should be at the beginning
   of the inserted text. */

extern void FE_InsertMessageCompositionText(MWContext* context,
											const char* text,
											XP_Bool leaveCursorAtBeginning);


/* This is how the library asks the FE to actually place the window on the
   screen.  Note that before this is called, FE_MsgShowHeaders() will have
   been called, telling the FE how the header fields should be configured
   (it's done this way so that the headers aren't changed after the window
   is already visible to the user.)
 */
extern void FE_RaiseMailCompositionWindow (MWContext *context);

/* When the mail has been delivered or cancelled, and the library wants to
   destroy the window, it calls this to free all the other FE context
   stuff.
 */
extern void FE_DestroyMailCompositionContext (MWContext *context);

/* Messages which are being posted to News are required to have a subject.
   Many news servers will reject such messages, and given the wider audience
   such messages have, it's antisocial of us to circumvent this. However,
   instead of simply refusing to send the message, we pop up a dialog box
   with the following text:

        You did not give a subject to this message.
        If you would like to provide one, please type it now.

           ___(no subject)___________________

        OK           Clear       Cancel          Help

   The text is selected by default, so the user can just start typing to
   replace it.  Or they can just hit return to really send a message with the
   subject "(no subject)".  But the assumption here is that sending a message
   with no subject to a wide audience is an oversight, not intentional, and
   we're doing them a service by giving them one last chance to correct their
   mistake.

   The context will be a mail composition context.

   NULL should be returned if the user hits "cancel", in which case we leave
   the composition window up and don't send the message (yet.)  Otherwise,
   a new string should be returned, which may be "" if that is what the user
   typed.  But the default text in the dialog box should be "(no subject)".
 */
extern char *FE_PromptMessageSubject(MWContext *context);


/* Get the URL associated with this context (the "X-Url" field.) */
extern const char* MSG_GetAssociatedURL(MWContext* context);


/* This is completely foul, but the FE needs to call this from within
   FE_MailCompositionAllConnectionsComplete() when the context->type is
   MWContextMessageComposition.
 */
extern void MSG_MailCompositionAllConnectionsComplete (MWContext *context);


/* Utility function that prefixes each line with "> " (for Paste As Quote). */
extern char *MSG_ConvertToQuotation (const char *string);


/* The user has just finished editing a given header (the field has lost focus
   or something like that.)  Find out if some magic change needs to be made
   (e.g., the user typed a nickname from the addressbook and we want to replace
   it with the specified address.)  If so, a new value to be displayed is
   returned.  The returned string, if any, must be freed using XP_FREE().  This
   should be called before calling MSG_SetHeaderContents(). */

extern char* MSG_UpdateHeaderContents(MWContext* context,
									  MSG_HEADER_SET header,
									  const char* value);

/* Inform the backend that the contents of some header field have been
   edited.  `header' must be a single header, not multiple ones.  The
   front end should call this whenever a change happens (or when the field
   is deselected, or whatever.)  In general, this should be called immediately
   after calling MSG_UpdateHeaderContents().  Usually, this call should not
   be made with the header MSG_ATTACHMENTS_HEADER_MASK; if so, it will treat
   it as a call to MSG_SetAttachment(context, value, NULL), which is probably
   not at all what you want.
 */
extern void MSG_SetHeaderContents (MWContext *context,
								   MSG_HEADER_SET header,
								   const char *value);


/* Inform the backend what is to be attached.  The MSG_AttachmentData structure
   is defined in mime.h.  The "list" parameter is an array of these fields,
   terminated by one which has a NULL url field.  In each entry, all that you
   have to fill in is "url" and probably "desired_type"; NULL is generally fine
   for all the other fields (but you can fill them in too).  See mime.h for all
   the spiffy details.

   Note that this call copies (sigh) all the data into libmsg; it is up to the
   caller to free any data it had to allocate to create the MSG_AttachmentData
   structure. */

extern void MSG_SetAttachmentList(MWContext* context,
								  struct MSG_AttachmentData* list);


/* Get what msglib's idea of what the current attachments are.  DO NOT MODIFY
   THE DATA RETURNED; you can look, but don't touch.  If you want to change
   the data, you have to copy out the whole structure somewhere, make your
   changes, call MSG_SetAttachmentList() with your new data, and then free
   your copy.  If there are no attachments, this routine always returns
   NULL. */
extern const struct MSG_AttachmentData* MSG_GetAttachmentList(MWContext*);


/* Return the string to display as the attachment header for this context.
   This should be called immediately after any call to MSG_SetAttachmentList,
   because the header string needs to change in that case.  Returns an
   allocated string; you must free the result with XP_FREE(). */
extern char* MSG_GetAttachmentString(MWContext* context);

#if 0
/* #### This is done via MSG_Command now */
/* Tell the back end to deliver (or queue) the message.  The MWContext is that
   of the mail composition window, and the back end has stored all the data it
   needs on there, as a result of the info gathered from FE calls to
   MSG_ComposeMessage() and MSG_SetHeaderContents() and all the others.
 */
extern void MSG_DeliverMessage (MWContext *context);
extern void MSG_CancelMessageDelivery (MWContext *context);
#endif /* 0 */


/* The back end uses this to tell the front end which header fields should
   be made visible in the msg composition window.   The MWContext is the
   one associated with the mail composition window.  This is done in response
   to a `Show All Headers' or `Show Interesting Headers' command.
 */
extern void FE_MsgShowHeaders (MWContext *context, MSG_HEADER_SET headers);

/* This is how the backend interrogates the front end for the text of the
   message which the user has begun editing.  It does this just after
   MSG_DeliverMessage(), but may do it at other times as well, for
   auto-saving, etc.

   The front end returns a text block and a size, and may also return a set
   of annotations describing font information in that text block.

   The caller will make a copy of all information returned.  After it has done
   so, it will call FE_DoneWithMessageBody(), so that blighted platforms like
   the Mac that don't have a pointer to the text can free up the copy they had
   to cons up for this call.  Sigh.  (On other platforms,
   FE_DoneWithMessageBody() is probably a no-op.)  */
extern int FE_GetMessageBody (MWContext *context,
							  char **body,
							  uint32 *body_size,
							  MSG_FontCode **font_changes);

extern void FE_DoneWithMessageBody(MWContext* context, char* body,
								   uint32 body_size);


/* MSG_MessageBodyEdited() is called to indicate that the user made some text
   editing action that edited the message body.  Don't call this when the user
   selects "Quote Message", and don't call this when he farts around with his
   attachments.  But if he types into the message body, or cuts or pastes in
   there, then call this.  Don't worry if you call this from within a call to
   FE_InsertMessageCompositionText; the backend knows to ignore calls to
   MSG_MessageBodyEdited from within a call to
   FE_InsertMessageCompositionText. */

extern void MSG_MessageBodyEdited(MWContext* context);


/* This function creates a new mail message composition window.
 */

MWContext* MSG_Mail (MWContext *old_context);

/* Convenience function to start composing a mail message from a web
   browser window - the currently-loaded document will be the default
   attachment should the user choose to attach it.  The context may
   be of any type, or NULL.  Returns the new message composition context.
 */
extern MWContext* MSG_MailDocument (MWContext *context);


/* When a message which has the `partial' bit set, meaning we only downloaded
   the first 20 lines because it was huge, this function will be called to 
   return some HTML to tack onto the end of the message to explain that it
   is truncated, and provide a clickable link which will download the whole
   message.
 */
#ifdef OLD_MIME
extern char *
MSG_GeneratePartialMessageBlurb (MWContext *context,
								 URL_Struct *url, void *closure,
								 const struct MIME_HTMLGeneratorHeaders
								   *headers);
#else  /* !OLD_MIME */
extern char *MSG_GeneratePartialMessageBlurb (MWContext *context,
											  URL_Struct *url, void *closure,
											  MimeHeaders *headers);
#endif /* !OLD_MIME */


/* Returns the address book context, creating it if necessary.  A mail context
   (either a Mail, News, or MessageComposition context) is passed in, on the
   unlikely chance that it might be useful for the FE.  If "viewnow" is TRUE,
   then present the address book window to the user; otherwise, don't (unless
   it is already visible).*/

extern MWContext* FE_GetAddressBook(MWContext* context, XP_Bool viewnow);



/* MSG SENDING PREFERENCES
 */

/* Set the default values for certain headers.  `header' must contain only
   one header, not a list, and it may only be one of: FROM, REPLY_TO, BCC, FCC.
   This is called when the preferences change.  The MWContext is... whatever.
 */
extern void MSG_SetDefaultHeaderContents (MWContext *context,
										  MSG_HEADER_SET header,
										  const char *default_value,
										  XP_Bool news_p);

/* Set whether to BCC the user on all outgoing messages.  This is in
   addition to any BCC field that may have been set by
   MSG_SetDefaultHeaderContents(); if both are set, then libmsg will
   correctly concatenate the strings.  It will also correctly cope with
   any later changes to the user's e-mail address; whenever a new message
   is composed, we reexamine both this preference and the results of
   FE_UsersMailAddress(). */

extern void MSG_SetDefaultBccSelf(MWContext* context, XP_Bool includeself,
								  XP_Bool news_p);

/* #### more cruft from mknews */
extern const char *NET_GetNewsHost (MWContext *context,
									XP_Bool warnUserIfMissing);
extern time_t NET_NewsgroupsLastUpdatedTime(char * hostname, XP_Bool is_secure);

XP_END_PROTOS


#ifdef OLD_MIME

/* This is the interface into mimehtml.c; it's kinda crufty...
 */

struct MIME_HTMLGeneratorHeaders
{
  const char *from;
  const char *to;
  const char *cc;
  const char *newsgroups;
  const char *distribution;
  const char *message_id;
  const char *references;
  const char *mozilla_status;
  const char *uidl;
  const char *xref;
  const char *subject;
};

typedef char *
  (*MIME_HTMLGeneratorFunction) (MWContext *context, URL_Struct *url,
								 const char *data, void *closure,
								 const struct MIME_HTMLGeneratorHeaders
								   *headers);

typedef char *
  (*MIME_DecompositionResultsFunction) (MWContext *context, URL_Struct *url,
										void *closure,
										struct MIME_HTMLGeneratorHeaders
										  *headers,
										struct MSG_AttachmentData *parts);

struct mime_options
{
  XP_Bool all_headers_p;	/* Whether all headers should be shown
							   Set by "?headers=all" */
  XP_Bool micro_headers_p;	/* Whether headers should be shown as one line. */
  XP_Bool fancy_headers_p;	/* Whether to do clever formatting of headers. */
  XP_Bool rot13_p;			/* Whether text/plain parts should be rotated
							   Set by "?rot13=true" */
  XP_Bool no_inline_p;		/* Whether inline display of attachments should
							   be suppressed.  Set by "?inline=false" */
  char *part_to_load;		/* The particular part of the multipart which
							   we are extracting.  Set by "?part=3.2.4" */

  XP_Bool uudecode_p;		/* Special hack for saving inlined uue images. */

  XP_Bool parse_one_level_p;	/* Magic alternative mode of operation where
								   we return the byte offsets and sizes of the
								   top-level parts of the message to the URL's
								   fe_data.  This is used by the WFE Exchange
								   support. */

  XP_Bool decompose_to_files_p;	/* Magic alternative mode of operation where
								   we write each top-level part of the message
								   to a different tmp file, and return their
								   names to the caller (via a callback.)  This
								   is used by the XP edit-a-queued-message
								   code.  Set by "?edit", but can only be used
								   internally, not in arbitrary URLs (it also
								   depends on a callback in `options'.) */

  /* Callback for emitting some HTML before the start of the message itself. */
  MIME_HTMLGeneratorFunction generate_header_html_fn;
  void *generate_header_html_closure;

  /* Callback for emitting some HTML at the end. */
  MIME_HTMLGeneratorFunction generate_footer_html_fn;
  void *generate_footer_html_closure;

  /* Callback for turning a message ID into a loadable URL. */
  MIME_HTMLGeneratorFunction generate_reference_url_fn;
  void *generate_reference_url_closure;

  /* Callback for turning a mail address into a mailto URL. */
  MIME_HTMLGeneratorFunction generate_mailto_url_fn;
  void *generate_mailto_url_closure;

  /* Callback for turning a newsgroup name into a news URL. */
  MIME_HTMLGeneratorFunction generate_news_url_fn;
  void *generate_news_url_closure;

  /* Callback used when decomposing a MIME object into its component parts. */
  MIME_DecompositionResultsFunction decomposition_results_fn;
  void *decomposition_results_closure;
};

#endif /* OLD_MIME */


#endif /* _MSGCOM_H_ */
