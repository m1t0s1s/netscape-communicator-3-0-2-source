/* -*- Mode: C; tab-width: 8 -*-
   mailnews.c --- mail and news windows
   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 19-Jun-95.
 */

#include "mozilla.h"
#include "xfe.h"
#include <msgcom.h>
#include "outline.h"
#include "mailnews.h"
#include "menu.h"
#include "felocale.h"
#include "xpgetstr.h"
#include "libi18n.h"

#ifdef DEBUG_jwz
# define USE_MOVEMAIL_AND_POP3
#endif


#include <X11/IntrinsicP.h>     /* for w->core.height */

#include <dirent.h>
#include <signal.h>
#include <sys/wait.h>		/* for all the movemail crud */

#ifdef SCO_SV
#include "mcom_db.h"
#define _SVID3		/* for statvfs.h */
#endif
#if defined(IRIX) || defined(OSF1) || defined(SOLARIS) || defined(SCO_SV) || defined(UNIXWARE)
#include <sys/statvfs.h> /* for statvfs() */
#define STATFS statvfs
#elif defined(HPUX) || defined(LINUX) || defined(SUNOS4)
#include <sys/vfs.h>      /* for statfs() */
#define STATFS statfs
#elif defined(BSDI)
#include <sys/mount.h>    /* for statfs() */
#define STATFS statfs
#else
#include <sys/statfs.h>  /* for statfs() */
#define STATFS statfs
#endif

#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>

/* for XP_GetString() */
extern int XFE_COULDNT_FORK_FOR_MOVEMAIL;
extern int XFE_PROBLEMS_EXECUTING;
extern int XFE_TERMINATED_ABNORMALLY;
extern int XFE_UNABLE_TO_OPEN;
extern int XFE_PLEASE_ENTER_NEWS_HOST;
extern int XFE_APP_EXITED_WITH_STATUS;
extern int XFE_RE;
extern int XFE_ERROR_SAVING_PASSWORD;
extern int XFE_UNIMPLEMENTED;
extern int XFE_NO_KEEP_ON_SERVER_WITH_MOVEMAIL;
extern int XFE_MAIL_SPOOL_UNKNOWN;
extern int XFE_COULDNT_FORK_FOR_DELIVERY;

void
FE_UpdateFolderList(MWContext *context, int index, int length, int total,
		    int visible, int* selected, int numselected)
{
  int i;
  Widget outline = CONTEXT_DATA(context)->folderlist;
  fe_OutlineChange(outline, index, length, total);

  fe_OutlineChange(outline, index, length, total);
  fe_OutlineUnselectAll(outline);
  for (i=0 ; i<numselected ; i++) {
    fe_OutlineSelect(outline, selected[i], False);
  }
  if (visible >= 0) {
    fe_OutlineMakeVisible(outline, visible);
  }
  fe_OutlineSetMaxDepth(outline, MSG_GetFolderMaxDepth(context));
}



void
FE_UpdateThreadList(MWContext *context, int index, int length, int total,
		    int visible, int* selected, int numselected)
{
  int i;
  Widget outline = CONTEXT_DATA(context)->messagelist;
  
  fe_OutlineChange(outline, index, length, total);
  fe_OutlineUnselectAll(outline);
  for (i=0 ; i<numselected ; i++) {
    fe_OutlineSelect(outline, selected[i], False);
  }
  fe_OutlineMakeVisible(outline, visible);
}

/* We use an index to find out which folder was selected for
 * move/copy to folder. We use certain pre-defined folders like
 * 'file' (which would save/copy to file) This offset is used
 * for defining these.
 */
#define FOLDER_INDEX_OFFSET	2
#define FOLDER_NEW_INDEX	0
#define FOLDER_FILE_INDEX	1

static void
fe_MoveCopy_to_folder_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext *context = (MWContext *) closure;
    char *foldername;
    int index;
    Boolean doingMove = CONTEXT_DATA (context)->doingMove;

    XtVaGetValues(widget, XmNuserData, &index, 0);
    switch (index) {
	case FOLDER_FILE_INDEX :	/* to file */
	    if (doingMove)
		MSG_Command(context, MSG_SaveMessagesAsAndDelete);
	    else
		MSG_Command(context, MSG_SaveMessagesAs);
	    break;
	case FOLDER_NEW_INDEX :		/* to New folder */
	    MSG_Command(context, MSG_NewFolder);
	    /* XXX We need to do something here. Get the foldername
	     * XXX and move/copy this message into that folder. Maybe
	     * XXX backend should give a Move/Copy-selected-to-new-folder
	     * XXX command
	     */
	    break;
	default:			/* Move to folder */
	    foldername = MSG_GetFolderNameUnfolded(context,
						index-FOLDER_INDEX_OFFSET);
	    if (!foldername) break;
	    if (doingMove)
		MSG_MoveSelectedMessagesInto(context, foldername);
	    else
		MSG_CopySelectedMessagesInto(context, foldername);
    }
}

static void
fe_decipher_MoveCopy_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
    MWContext *context = (MWContext *) closure;

    if (widget == CONTEXT_DATA (context)->move_selected_to_folder)
	CONTEXT_DATA (context)->doingMove = True;
    else
	CONTEXT_DATA (context)->doingMove = False;
}


static int
fe_addto_folder_menu (MWContext *context, Widget menu,
			XtCallbackProc fe_folder_menu_cb,
			MSG_FolderLine *lines, int numlines, int index)
{
    Widget button;
    Arg av [20];
    int ac;
    Visual *v = 0;
    Colormap cmap = 0;
    Cardinal depth = 0;
    Boolean done = False;
    int this_index, next_index;
    int level = lines[index].depth;
    XmString xmstr;

    XtVaGetValues (CONTEXT_WIDGET (context), XtNvisual, &v,
			XtNcolormap, &cmap, XtNdepth, &depth, 0);


    while (!done) {
	if ( (index >= numlines) || (level != lines[index].depth)) {
	    done = True;
	    continue;
	}

	if (lines[index].flags & MSG_FOLDER_FLAG_DIRECTORY) {
	    Widget submenu = 0;
	    this_index = index;
	    next_index = index + 1;
	    if ((next_index < numlines) && (lines[next_index].depth > level)) {
		ac = 0;
		XtSetArg (av[ac], XmNvisual, v); ac++;
		XtSetArg (av[ac], XmNdepth, depth); ac++;
		XtSetArg (av[ac], XmNcolormap, cmap); ac++;
		submenu = XmCreatePulldownMenu (menu, "popup", av, ac);
		index = fe_addto_folder_menu (context, submenu,
						fe_folder_menu_cb,
						lines, numlines,
						next_index);
	    }
	    ac = 0;
	    xmstr = XmStringCreate((char *)lines[this_index].name,
					XmFONTLIST_DEFAULT_TAG);
	    XtSetArg (av[ac], XmNlabelString, xmstr); ac++;
	    XtSetArg (av[ac], XmNsubMenuId, submenu); ac++;
	    button = XmCreateCascadeButtonGadget (menu, "folderCascadeButton",
						av, ac);
	    XtManageChild (button);
	    if (submenu == 0) index++;
	    XmStringFree(xmstr);
	}
	else if (lines[index].flags & MSG_FOLDER_FLAG_MAIL) {
	    ac = 0;
	    xmstr = XmStringCreate((char *)lines[index].name,
					XmFONTLIST_DEFAULT_TAG);
	    XtSetArg (av[ac], XmNlabelString, xmstr); ac++;
	    XtSetArg (av[ac], XmNuserData, index+FOLDER_INDEX_OFFSET); ac++;
	    button = XmCreatePushButtonGadget (menu, "folderMenuItem",
						av, ac);
	    XtAddCallback (button, XmNactivateCallback, fe_folder_menu_cb,
				context);
	    XtManageChild (button);
	    index++;
	    XmStringFree(xmstr);
	}
	else {
	    /* Invalid lines entry. Ignore this. */
	    index++;
	}
    }
    return(index);
}

static void
fe_make_folder_menu(MWContext *context, Widget menu,
			XtCallbackProc fe_folder_menu_cb,
			MSG_FolderLine *lines, int numlines)
{
  Widget foldermenu;
  Widget *kids = 0;
  Cardinal nkids = 0;
  Arg av [10];
  int ac;

  Widget parent;
  Colormap cmap;
  Cardinal depth;
  Visual *v;

  XtVaGetValues (menu, XmNsubMenuId, &foldermenu, 0);

  if (!foldermenu) {
    /* First time we are using the folder menu. Create it. */
    parent = CONTEXT_DATA(context)->widget;
    XtVaGetValues(parent, XtNvisual, &v, XtNcolormap, &cmap,
			XtNdepth, &depth, 0);
    ac = 0;
    XtSetArg(av[ac], XmNvisual, v); ac++;
    XtSetArg(av[ac], XmNcolormap, cmap); ac++;
    XtSetArg(av[ac], XmNdepth, depth); ac++;
    foldermenu = XmCreatePulldownMenu(CONTEXT_DATA(context)->widget,
					"popup", av, ac);

    /* Connect the popup menu and the cascade */
    XtVaSetValues (menu, XmNsubMenuId, foldermenu, 0);
  }

  XtVaGetValues (foldermenu, XmNchildren, &kids, XmNnumChildren, &nkids, 0);
  /* Destroy all children */
  if (nkids > 0) {
    XtUnmanageChildren (kids, nkids);
    fe_DestroyWidgetTree(kids, nkids);
  }

  fe_addto_folder_menu (context, foldermenu, fe_folder_menu_cb,
			lines, numlines, 0);
}

void
FE_UpdateFolderMenus(MWContext *context, MSG_FolderLine *lines, int numlines)
{
    Widget menu;
    Widget submenu;

    if (!lines) return;

    /* Move to folder */
    menu = CONTEXT_DATA (context)->move_selected_to_folder;

    if (menu == NULL) return;

    XtVaGetValues (menu, XmNsubMenuId, &submenu, 0);

    fe_make_folder_menu(context, menu, fe_MoveCopy_to_folder_cb,
				lines, numlines);

    if (submenu == 0) {
	/* First time. Setup the copy selected menu item too. */
	XtVaGetValues (menu, XmNsubMenuId, &submenu, 0);
	XtAddCallback (menu, XmNcascadingCallback, fe_decipher_MoveCopy_cb,
				context);
	CONTEXT_DATA (context)->foldermenu = submenu;
	menu = CONTEXT_DATA (context)->copy_selected_to_folder;
	XtVaSetValues (menu, XmNsubMenuId, submenu, 0);
	XtAddCallback (menu, XmNcascadingCallback, fe_decipher_MoveCopy_cb,
				context);
    }
}


const char*
FE_GetFolderDirectory(MWContext* context)
{
  XP_ASSERT (fe_globalPrefs.mail_directory);
  return fe_globalPrefs.mail_directory;
}

/* XFE_DiskSpaceAvailable - returns the number of free bytes for the
   disk on which the given file resides. If something fails, we just
   return a suitably large number and hope for the best.
   */
uint32
fe_DiskSpaceAvailable (MWContext *context, char *filename)
{
  char curdir [MAXPATHLEN];
  struct STATFS fs_buf;

  if (!filename || !*filename) {
    (void) getcwd (curdir, MAXPATHLEN);
    if (! curdir) return 1L << 30;  /* hope for the best as we did in cheddar */
  } else {
    PR_snprintf (curdir, MAXPATHLEN, "%.200s", filename);
  }

  if (STATFS (curdir, &fs_buf) < 0) {
    /*    fe_perror (context, "Cannot stat current directory\n"); */
    return 1L << 30; /* hope for the best as we did in cheddar */
  }

#ifdef DEBUG_DISK_SPACE
  printf ("DiskSpaceAvailable: %d bytes\n", 
	  fs_buf.f_bsize * (fs_buf.f_bavail - 1));
#endif
  return (fs_buf.f_bsize * (fs_buf.f_bavail - 1));
}


uint32
FE_DiskSpaceAvailable (MWContext *context)
{
  return fe_DiskSpaceAvailable (context, NULL);
}


char*
FE_GetTempFileFor(MWContext* context, const char* fname, XP_FileType ftype,
		  XP_FileType* rettype)
{
  char* actual = WH_FileName(fname, ftype);
  int len;
  char* result;
  if (!actual) return NULL;
  len = strlen(actual) + 10;
  result = XP_ALLOC(len);
  if (!result) return NULL;
  PR_snprintf(result, len, "%s-XXXXXX", actual);
  XP_FREE(actual);
  mktemp(result);
  *rettype = xpMailFolder;	/* Ought to be harmless enough. */
  return result;
}


Boolean 
fe_folder_datafunc(Widget widget, void* closure, int row, fe_OutlineDesc* data)
{
  MWContext* context = (MWContext*) closure;
  MSG_FolderLine folder;
  static char unseen[10];
  static char total[10];
  int c = 0;
  if (!MSG_GetFolderLine(context, row, &folder, NULL)) return False;
  data->tag = XmFONTLIST_DEFAULT_TAG;
  data->flippy = folder.flippy_type;
  data->depth = folder.depth;
  data->type[c] = (folder.flags & MSG_FOLDER_FLAG_NEWSGROUP) ?
    FE_OUTLINE_ChoppedString : FE_OUTLINE_String;
  data->str[c++] = folder.name;
  if (context->type == MWContextNews) {
    /*
     * 4-29-96 jefft -- fixed bug# 17295
     *  Added more checkings before putting up the
     *  check mark for each folder line.
     */
    if (folder.flags & MSG_FOLDER_FLAG_NEWSGROUP &&
        !(folder.flags & (MSG_FOLDER_FLAG_NEWS_HOST | 
			MSG_FOLDER_FLAG_MAIL |
			MSG_FOLDER_FLAG_DIRECTORY))) {
      data->icon = FE_OUTLINE_Newsgroup;
      data->type[c++] =
	(folder.flags & MSG_FOLDER_FLAG_SUBSCRIBED) ? FE_OUTLINE_Subscribed :
	FE_OUTLINE_Unsubscribed;
    } else {
      data->icon = (folder.flags & MSG_FOLDER_FLAG_NEWS_HOST) ?
	FE_OUTLINE_Newshost : FE_OUTLINE_Newsgroup;
      data->type[c] = FE_OUTLINE_String;
      data->str[c++] = "";
    }
  } else {
    data->icon = FE_OUTLINE_Folder;
  }
  if (!(folder.flags & MSG_FOLDER_FLAG_DIRECTORY)) {
    if (folder.unseen >= 0) {
      PR_snprintf(unseen, sizeof (unseen), "%d", folder.unseen);
      PR_snprintf(total, sizeof (total), "%d", folder.total);
      data->type[c] = FE_OUTLINE_String;
      data->str[c++] = unseen;
      data->type[c] = FE_OUTLINE_String;
      data->str[c++] = total;
      if (folder.unseen > 0) data->tag = "BOLD";
    } else {
      data->type[c] = FE_OUTLINE_String;
      data->str[c++] = "???";
      data->type[c] = FE_OUTLINE_String;
      data->str[c++] = "";
    }
  } else {
    data->type[c] = FE_OUTLINE_String;
    data->str[c++] = "---";
    data->type[c] = FE_OUTLINE_String;
    data->str[c++] = "---";
  }      
  return True;
}

void
fe_folder_clickfunc(Widget widget, void* closure, int row, int column,
		    char* header, int button, int clicks,
		    Boolean shift, Boolean ctrl)
{
  MWContext* context = (MWContext*) closure;
  MSG_FolderLine data;
  if (row < 0) return;
  if (button == 3) return;
  if (header == NULL) header = "";
  if (!MSG_GetFolderLine(context, row, &data, NULL)) return;
  if (context->type == MWContextNews && strcmp(header, "Sub") == 0) {
    /*
     * 4-29-96 jefft -- fixed bug# 17295
     *  Added more checkings before allowing users
     *  subscribe newsgroup.
     */
    if (data.flags & MSG_FOLDER_FLAG_NEWSGROUP &&
        !(data.flags & (MSG_FOLDER_FLAG_NEWS_HOST | 
			MSG_FOLDER_FLAG_MAIL |
			MSG_FOLDER_FLAG_DIRECTORY))) {
      MSG_ToggleSubscribed(context, row);
    }
  } else {
    if (clicks == 1) {
      if (ctrl) {
	MSG_ToggleSelectFolder(context, row);
      } else if (shift) {
	MSG_SelectFolderRangeTo(context, row);
      } else {
	MSG_SelectFolder(context, row, True);
      }
    }
  }
  fe_NeutralizeFocus(context);
  FE_UpdateToolbar(context);
}

void
fe_folder_dropfunc(Widget dropw, void* closure, fe_dnd_Event type,
		   fe_dnd_Source* source, XEvent* event)
{
  MWContext* context = (MWContext*) closure;
  int row;

  /* Only do any of this if we're dropping a column;
     or if we're dropping a mail or news message into a mail window.
   */
  if (source->type != FE_DND_COLUMN &&
      (context->type != MWContextMail ||
       (source->type != FE_DND_MAIL_MESSAGE &&
	source->type != FE_DND_NEWS_MESSAGE))) {
    return;
  }

  fe_OutlineHandleDragEvent(CONTEXT_DATA(context)->folderlist, event, type,
			    source);

  /* Only do the rest of this if we're dropping a mail or news message. */
  if (source->type != FE_DND_MAIL_MESSAGE &&
      source->type != FE_DND_NEWS_MESSAGE)
    return;

  row = fe_OutlineRootCoordsToRow(CONTEXT_DATA(context)->folderlist,
				  event->xbutton.x_root,
				  event->xbutton.y_root,
				  NULL);
  if (row >= 0) {
    MSG_FolderLine folder;
    if (!MSG_GetFolderLine(context, row, &folder, NULL) ||
	folder.flags & MSG_FOLDER_FLAG_DIRECTORY) row = -1;
  }
  switch (type) {
  case FE_DND_START:
    break;
  case FE_DND_DROP:
    if (row >= 0) {
      MWContext *drop_context = (MWContext*) closure;
      MWContext *drag_context = fe_WidgetToMWContext((Widget) source->closure);
      char *drop_file = 0;

      if (!drag_context || !drop_context)
	break;

      /* If we're dropping a mail message, the drag and drop contexts ought
	 to be the same, because there is only ever one mail context. */
      XP_ASSERT(source->type != FE_DND_MAIL_MESSAGE ||
		drag_context == drop_context);

      /* If we're dropping a news message, the drag and drop contexts ought
	 to be different, because there is nowhere to drop a news message
	 in a news context. */
      XP_ASSERT(source->type != FE_DND_NEWS_MESSAGE ||
		drag_context != drop_context);

      /* Mail contexts are the only drop site for either mail or news messages.
       */
      XP_ASSERT(drop_context->type == MWContextMail);

      drop_file = MSG_GetFolderName(drop_context, row);
      if (drop_file)
	{
	  XP_Bool copy_p = (drag_context->type == MWContextNews);
	  if (copy_p)
	    MSG_CopySelectedMessagesInto(drag_context, drop_file);
	  else
	    MSG_MoveSelectedMessagesInto(drag_context, drop_file);
	}
    }
    break;
  case FE_DND_DRAG:
    fe_OutlineSetDragFeedback(CONTEXT_DATA(context)->folderlist, row, TRUE);
    break;
  case FE_DND_END:
    fe_OutlineSetDragFeedback(CONTEXT_DATA(context)->folderlist, -1, TRUE);
    fe_NeutralizeFocus(context);
    break;
  default:
    XP_ASSERT (0);
    break;
  }
}


void
fe_folder_iconfunc(Widget widget, void* closure, int row)
{
  MWContext* context = (MWContext*) closure;
  MSG_FolderLine folder;
  if (!MSG_GetFolderLine(context, row, &folder, NULL)) return;
  if (folder.flippy_type != MSG_Leaf) {
    MSG_ToggleFolderExpansion(context, row);
  } else {
    MSG_SelectFolder(context, row, TRUE);
  }
  fe_NeutralizeFocus(context);
  FE_UpdateToolbar (context);
}


Boolean
fe_message_datafunc(Widget widget, void* closure, int row,
		    fe_OutlineDesc* data)
{
  MWContext* context = (MWContext*) closure;
  static MSG_MessageLine msg;
  static char buf[1024];	/* ## Sigh... */
  static char from_buf[1024];	/* ## Sigh... */
  int c = 0;
  char *tmp;
  if (!MSG_GetThreadLine(context, row, &msg, NULL)) return False;
  data->depth = msg.depth;
  data->icon = (context->type == MWContextNews ? FE_OUTLINE_Article :
		FE_OUTLINE_MailMessage);
  data->flippy = msg.flippy_type;
  data->type[c] = FE_OUTLINE_String;
  tmp = IntlDecodeMimePartIIStr(msg.from, fe_LocaleCharSetID, FALSE);
  if (tmp) {
    strncpy(from_buf, tmp, 1000);
    from_buf[1000] = 0;
    XP_FREE(tmp);
    data->str[c++] = from_buf;
  } else {
    data->str[c++] = msg.from;
  }
  if (msg.flags & MSG_FLAG_EXPIRED) {
    data->tag = XmFONTLIST_DEFAULT_TAG ; /* Don't boldface dummy messages. */
    data->type[c] = FE_OUTLINE_String;
    data->str[c++] = "";
    data->type[c] = FE_OUTLINE_String;
    data->str[c++] = "";
  } else {
    data->tag = (msg.flags & MSG_FLAG_READ) ? XmFONTLIST_DEFAULT_TAG : "BOLD";
    data->type[c++] =
      (msg.flags & MSG_FLAG_MARKED) ? FE_OUTLINE_Marked : FE_OUTLINE_Unmarked;
    data->type[c++] =
      (msg.flags & MSG_FLAG_READ) ? FE_OUTLINE_Read : FE_OUTLINE_Unread;
  }
  data->type[c] = FE_OUTLINE_String;
  tmp = IntlDecodeMimePartIIStr(msg.subject, fe_LocaleCharSetID, FALSE);
  if (msg.flags & MSG_FLAG_HAS_RE) {
    strcpy(buf, XP_GetString( XFE_RE  ) ); 
    strncpy(buf + XP_STRLEN( buf ), (tmp ? tmp : msg.subject), 1000);
    buf[1023] = '\0';
    data->str[c++] = buf;
  } else {
    if (tmp) {
      strncpy(buf, tmp, 1000);
      buf[1000] = 0;
      data->str[c++] = buf;
    } else {
      data->str[c++] = msg.subject;
    }
  }
  if (tmp) {
    XP_FREE(tmp);
  }
  data->type[c] = FE_OUTLINE_String;
  if (msg.date == 0) {
    data->str[c++] = "";
  } else {
    data->str[c++] = MSG_FormatDate(context, msg.date);
  }
  return True;
}


void
fe_message_clickfunc(Widget widget, void* closure, int row, int column,
		     char* header, int button, int clicks,
		     Boolean shift, Boolean ctrl)
{
  MWContext *context = (MWContext *) closure;
  if (header == NULL) header = "";
  if (row < 0) {
    if (strcmp(header, "Sender") == 0) {
      MSG_Command(context, MSG_SortBySender);
    } else if (strcmp(header, "Date") == 0) {
      MSG_Command(context, MSG_SortByDate);
    } else if (strcmp(header, "Subject") == 0) {
      MSG_Command(context, MSG_SortBySubject);
    } else if (strcmp(header, "Thread") == 0) {
      MSG_Command(context, MSG_ThreadMessages);
    }
    return;
  }
  if (strcmp(header, "Flag") == 0) {
    MSG_ToggleMark(context, row);
  } else if (strcmp(header, "UnreadMsg") == 0) {
    MSG_ToggleRead(context, row);
  } else {
    if (clicks < 2) {
      if (ctrl) {
	MSG_ToggleSelectMessage(context, row);
      } else if (shift) {
	MSG_SelectRangeTo(context, row);
      } else {
	MSG_SelectMessage(context, row, True);
      }
    }
  }
  fe_NeutralizeFocus(context);
}


void
fe_message_dropfunc(Widget dropw, void* closure, fe_dnd_Event type,
		    fe_dnd_Source* source, XEvent* event)
{
  MWContext* context = (MWContext*) closure;
  if (source->type == FE_DND_COLUMN) {
    fe_OutlineHandleDragEvent(CONTEXT_DATA(context)->messagelist, event, type,
			      source);
  }
  /* Right now, dropping on a message area does nothing.  ### */
}


void
fe_message_iconfunc(Widget widget, void* closure, int row)
{
  MWContext *context = (MWContext *) closure;
  MSG_ToggleThreadExpansion(context, row);
  fe_NeutralizeFocus(context);
}

static XP_Bool
fe_run_movemail (MWContext *context, const char *from, const char *to)
{
  struct sigaction newact, oldact;
  pid_t forked;
  int ac = 0;
  char **av = (char **) malloc (10 * sizeof (char *));
  av [ac++] = strdup (fe_globalPrefs.movemail_program);
  av [ac++] = strdup (from);
  av [ac++] = strdup (to);
  av [ac++] = 0;

  /* Setup signals so that SIGCHLD is ignored as we want to do waitpid */
  sigaction(SIGCHLD, NULL, &oldact);
  newact.sa_handler = SIG_DFL;
  newact.sa_flags = 0;
  sigfillset(&newact.sa_mask);
  sigaction (SIGCHLD, &newact, NULL);

  errno = 0;
  switch (forked = fork ())
    {
    case -1:
      while (--ac >= 0)
	free (av [ac]);
      free (av);
      /* fork() failed (errno is meaningful.) */
      fe_perror (context, XP_GetString( XFE_COULDNT_FORK_FOR_MOVEMAIL ) );
      /* Reset SIGCHLD handler before returning */
      sigaction (SIGCHLD, &oldact, NULL);
      return FALSE;
      break;
    case 0:
      {
	execvp (av[0], av);
	exit (-1);	/* execvp() failed (this exits the child fork.) */
	break;
      }
    default:
      {
	/* This is the "old" process (subproc pid is in `forked'.) */
	char buf [2048];
	int status = 0;
	pid_t waited_pid;

	while (--ac >= 0)
	  free (av [ac]);
	free (av);

	/* wait for the other process (and movemail) to terminate. */
	waited_pid = waitpid (forked, &status, 0);

	/* Reset the SIG CHILD signal handler */
	sigaction(SIGCHLD, &oldact, NULL);

	if (waited_pid <= 0 || waited_pid != forked)
	  {
	    /* We didn't fork properly, or something.  Let's hope errno
	       has a meaningful value... (can it?) */
	    PR_snprintf (buf, sizeof (buf), XP_GetString( XFE_PROBLEMS_EXECUTING ),
		     fe_globalPrefs.movemail_program);
	    fe_perror (context, buf);
	    return FALSE;
	  }

	if (!WIFEXITED (status))
	  {
	    /* Dumped core or was killed or something.  Let's hope errno
	       has a meaningful value... (can it?) */
	    PR_snprintf (buf, sizeof (buf),
		 XP_GetString( XFE_TERMINATED_ABNORMALLY ),
		 fe_globalPrefs.movemail_program);
	    fe_perror (context, buf);
	    return FALSE;
	  }

	if (WEXITSTATUS (status) != 0)
	  {
	    PR_snprintf (buf, sizeof (buf), XP_GetString( XFE_APP_EXITED_WITH_STATUS ),
		     fe_globalPrefs.movemail_program,
		     WEXITSTATUS (status));
	    FE_Alert (context, buf);
	    return FALSE;
	  }
	/* Else, exited with code 0 */
	return TRUE;
      }
    }
}


typedef struct fe_mn_incstate {
  MWContext* context;
  char filename[512];
  XP_Bool wascrashbox;
  XP_File file;
} fe_mn_incstate;


static void
fe_incdone(void* closure, XP_Bool result)
{
  fe_mn_incstate* state = (fe_mn_incstate*) closure;
  if (result) {
    unlink(state->filename);
    if (state->file) {
      fclose(state->file);
      state->file = NULL;
    }
    if (state->wascrashbox) {
      fe_mn_getnewmail(NULL, (XtPointer) state->context, NULL);
    } else {
      FE_UpdateBiff(MSG_BIFF_NoMail);
    }
  }
  XP_FREE(state);
}




char*
fe_mn_getmailbox(void)
{
  static char* mailbox = NULL;
  if (mailbox == NULL) {
    mailbox = getenv("MAIL");
    if (mailbox && *mailbox)
      {
	/* Bug out if $MAIL is set to something silly: obscenely long,
	   or not an absolute path.  Returning 0 will cause the error
	   message about "please set $MAIL ..." to be shown.
	 */
	if (strlen(mailbox) >= 1024) /* NAME_LEN */
	  mailbox = 0;
	else if (mailbox[0] != '/')
	  mailbox = 0;
	else
	  mailbox = strdup(mailbox);   /* copy result of getenv */
      }
    else
      {
	const char *heads[] = {
	  "/var/spool/mail",
	  "/usr/spool/mail",
	  "/var/mail",
	  "/usr/mail",
	  0 };
	const char **head = heads;
	while (*head)
	  {
	    struct stat st;
	    if (!stat (*head, &st) && S_ISDIR(st.st_mode))
	      break;
	    head++;
	  }
	if (*head)
	  {
	    char *uid = 0, *name = 0;
	    fe_DefaultUserInfo (&uid, &name, True);
	    if(uid && *uid)
	      mailbox = PR_smprintf("%s/%s", *head, uid);
	    if(uid) XP_FREE(uid);
	    if(name) XP_FREE(name);
	  }
      }
  }
  return mailbox;
}



#ifdef USE_MOVEMAIL_AND_POP3
static void fe_mn_getnewmail_1(Widget, XtPointer, XtPointer);

void
fe_mn_getnewmail(Widget widget, XtPointer closure, XtPointer call_data) 
{
  XP_Bool old_ump  = fe_globalPrefs.use_movemail_p;
  XP_Bool old_lmos = fe_globalPrefs.pop3_leave_mail_on_server;

  if (!old_ump)		/* that is, if "use pop3" is checked. */
    {                   /* The semantics caused by this ifdef are:
                           "use pop3" ==> "use pop3 and movemail"
                           "use movemail" ==> "use movemail only"
                         */
      fe_globalPrefs.use_movemail_p = TRUE;
      fe_globalPrefs.pop3_leave_mail_on_server = FALSE;
      fe_mn_getnewmail_1(widget, closure, call_data);
      fe_globalPrefs.pop3_leave_mail_on_server = old_lmos;
      fe_globalPrefs.use_movemail_p = old_ump;
    }

  fe_mn_getnewmail_1(widget, closure, call_data);
}

static void
fe_mn_getnewmail_1(Widget widget, XtPointer closure, XtPointer call_data) 
#else  /* !USE_MOVEMAIL_AND_POP3 */
void
fe_mn_getnewmail(Widget widget, XtPointer closure, XtPointer call_data) 
#endif /* !USE_MOVEMAIL_AND_POP3 */
{
  MWContext *context = (MWContext *) closure;
  int status;
  const char *string = 0;
  XP_Bool selectable_p = FALSE;
  MSG_COMMAND_CHECK_STATE selected_p = MSG_NotUsed;

  if ( fe_globalPrefs.use_movemail_p &&
       fe_globalPrefs.movemail_warn &&
       !fe_MovemailWarning(context) ) return;

  status = MSG_CommandStatus (context, MSG_GetNewMail,
			      &selectable_p, &selected_p,
			      &string, 0);
  if (status < 0 || !selectable_p)
    return;

  if (!fe_globalPrefs.use_movemail_p)
    {
      /* use POP */
      MSG_Command (context, MSG_GetNewMail);
    }
  else
    {
      /* Use movemail to get the mail out of the spool area and into
	 our home directory, and then read out of the resultant file
	 (without locking.)
       */
      char *mailbox;
      char *s;
      XP_File file;
      XP_StatStruct st;
      fe_mn_incstate* state = 0;

      /* First, prevent the user from assuming that `Leave on Server' will
	 work with movemail.  A better way to do this might be to change the
	 selection/sensitivity of the dialog buttons, but this will do.
	 (bug 11561)
       */
      if (fe_globalPrefs.pop3_leave_mail_on_server)
	{
	  FE_Alert(context, XP_GetString(XFE_NO_KEEP_ON_SERVER_WITH_MOVEMAIL));
	  return;
	}

      state = XP_NEW_ZAP(fe_mn_incstate);
      if (!state) return;
      state->context = context;

      /* If this doesn't exist, there's something seriously wrong */
      XP_ASSERT (fe_globalPrefs.movemail_program);

      mailbox = fe_mn_getmailbox();
      if(!mailbox)
	{
	  FE_Alert(context, XP_GetString(XFE_MAIL_SPOOL_UNKNOWN));
	  return;
	}

      s = (char *) FE_GetFolderDirectory(context);
      PR_snprintf (state->filename, sizeof (state->filename),
		    "%s/.netscape.mail-recovery", s);

      if (!stat (state->filename, &st))
	{
	  if (st.st_size == 0)
	    /* zero length - a mystery. */
	    unlink (state->filename);
	  else
	    {
	      /* The crashbox already exists - that means we didn't delete it
		 last time, possibly because we crashed.  So, read it now.
	       */
	      file = fopen (state->filename, "r");
	      if (!file)
		{
		  char buf[1024];
		  PR_snprintf (buf, sizeof (buf), XP_GetString( XFE_UNABLE_TO_OPEN ),
				state->filename);
		  fe_perror (context, buf);
		  return;
		}
	      state->file = file;
	      state->wascrashbox = TRUE;
	      MSG_IncorporateFromFile (context, file, fe_incdone, state);
	      return;
	    }
	}
      
      /* Now move mail into the tmp file.
       */
      if (fe_globalPrefs.builtin_movemail_p)
	{
          if (!fe_MoveMail (context, mailbox, state->filename))
	    {
	      return;
	    }
	}
      else
	{
	  if (!fe_run_movemail (context, mailbox, state->filename))
	    return;
	}

      /* Now move the mail from the crashbox to the folder.
       */
      file = fopen (state->filename, "r");
      if (file)
	{
	  state->file = file;
	  MSG_IncorporateFromFile(context, file, fe_incdone, state);
	}
    }
}



/* If we're set up to deliver mail/news by running a program rather
   than by talking to SMTP/NNTP, this does it.

   Returns positive if delivery via program was successful;
   Returns negative if delivery failed;
   Returns 0 if delivery was not attempted (in which case we
   should use SMTP/NNTP instead.)

   $NS_MSG_DELIVERY_HOOK names a program which is invoked with one argument,
   a tmp file containing a message.  (Lines are terminated with CRLF.)
   This program is expected to parse the To, CC, BCC, and Newsgroups headers,
   and effect delivery to mail and/or news.  It should exit with status 0
   iff successful.

   #### This really wants to be defined in libmsg, but it wants to
   be able to use fe_perror, so...
 */
int
msg_DeliverMessageExternally(MWContext *context, const char *msg_file)
{
  struct sigaction newact, oldact;
  static char *cmd = 0;
  if (!cmd)
    {
      /* The first time we're invoked, look up the command in the
	 environment.  Use "" as the `no command' tag. */
      cmd = getenv("NS_MSG_DELIVERY_HOOK");
      if (!cmd)
	cmd = "";
      else
	cmd = XP_STRDUP(cmd);
    }

  if (!cmd || !*cmd)
    /* No external command -- proceed with "normal" delivery. */
    return 0;
  else
    {
      pid_t forked;
      int ac = 0;
      char **av = (char **) malloc (10 * sizeof (char *));
      av [ac++] = XP_STRDUP (cmd);
      av [ac++] = XP_STRDUP (msg_file);
      av [ac++] = 0;

      /* Setup signals so that SIGCHLD is ignored as we want to do waitpid */
      sigaction(SIGCHLD, NULL, &oldact);
      newact.sa_handler = SIG_DFL;
      newact.sa_flags = 0;
      sigfillset(&newact.sa_mask);
      sigaction (SIGCHLD, &newact, NULL);

      errno = 0;
      switch (forked = fork ())
	{
	case -1:
	  {
	    while (--ac >= 0)
	      XP_FREE (av [ac]);
	    XP_FREE (av);
	    /* fork() failed (errno is meaningful.) */
	    fe_perror (context, XP_GetString( XFE_COULDNT_FORK_FOR_DELIVERY ));
	    /* Reset SIGCHLD signal hander before returning */
	    sigaction(SIGCHLD, &oldact, NULL);
	    return -1;
	    break;
	  }
	case 0:
	  {
	    execvp (av[0], av);
	    exit (-1);	/* execvp() failed (this exits the child fork.) */
	    break;
	  }
	default:
	  {
	    /* This is the "old" process (subproc pid is in `forked'.) */
	    char buf [2048];
	    int status = 0;
	    pid_t waited_pid;
	    
	    while (--ac >= 0)
	      XP_FREE (av [ac]);
	    XP_FREE (av);

	    /* wait for the other process to terminate. */
	    waited_pid = waitpid (forked, &status, 0);

	    /* Reset SIG CHILD signal handler */
	    sigaction(SIGCHLD, &oldact, NULL);

	    if (waited_pid <= 0 || waited_pid != forked)
	      {
		/* We didn't fork properly, or something.  Let's hope errno
		   has a meaningful value... (can it?) */
		PR_snprintf (buf, sizeof (buf),
			     XP_GetString( XFE_PROBLEMS_EXECUTING ),
			     cmd);
		fe_perror (context, buf);
		return -1;
	      }

	    if (!WIFEXITED (status))
	      {
		/* Dumped core or was killed or something.  Let's hope errno
		   has a meaningful value... (can it?) */
		PR_snprintf (buf, sizeof (buf),
			     XP_GetString( XFE_TERMINATED_ABNORMALLY ),
			     cmd);
		fe_perror (context, buf);
		return -1;
	      }

	    if (WEXITSTATUS (status) != 0)
	      {
		PR_snprintf (buf, sizeof (buf),
			     XP_GetString( XFE_APP_EXITED_WITH_STATUS ),
			     cmd,
			     WEXITSTATUS (status));
		FE_Alert (context, buf);
		return -1;
	      }
	    /* Else, exited with code 0 */
	    return 1;
	  }
	}
    }
}

static void
fe_newshost_close(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* context = (MWContext*) closure;
  fe_NewsContextData *newsdata = NEWS_CONTEXT_DATA(context);

  newsdata->openNewsHost_shell     = 0;
  newsdata->openNewsHost_host      = 0;
  newsdata->openNewsHost_port      = 0;
  newsdata->openNewsHost_secure    = 0;
  newsdata->openNewsHost_fn        = 0;
  newsdata->openNewsHost_fnClosure = 0;
}

static void
fe_newshost_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  MWContext* context = (MWContext*) closure;
  fe_NewsContextData *newsdata = NEWS_CONTEXT_DATA(context);
  char *host_and_port = NULL;

  switch (cb->reason) {
    case XmCR_OK: {
	char *host, *portStr;
	int port = -1;
	Boolean secure_p;
	host = XmTextFieldGetString(newsdata->openNewsHost_host);
	portStr = XmTextFieldGetString(newsdata->openNewsHost_port);
	host = fe_StringTrim(host);
	portStr = fe_StringTrim(portStr);
	if (portStr && *portStr)
	  port = atoi(portStr);
	XtVaGetValues(newsdata->openNewsHost_secure, XmNset, &secure_p, 0);
	if (*host)
	  host_and_port = PR_smprintf( "%s://%s%s%s",
					(secure_p?"snews":"news"),
					host,
					(port > 0)?":":"",
					(port > 0)?portStr:""
					);
	else
	  host_and_port = PR_smprintf( "%s:",
					(secure_p?"snews":"news")
					);
	XtFree(host);
	XtFree(portStr);
      }
      break;
	
    case XmCR_CANCEL:
      break;
  }

  /* fn will free host_and_port */
  (*(newsdata->openNewsHost_fn))(context, host_and_port,
				newsdata->openNewsHost_fnClosure);
  XtUnmanageChild(newsdata->openNewsHost_shell);
}

static void
fe_make_openNewsHost_dialog (MWContext *context)
{
  fe_NewsContextData *newsdata = NEWS_CONTEXT_DATA(context);

  Widget shell = newsdata->openNewsHost_shell;
  Widget   form;
  Widget    hostLabel, hostText;
  Widget    portLabel, portText;
  Widget    secure;

  int ac;
  Arg av[20];
  int i = 0;
  Widget kids[20];

  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  XtVaGetValues(CONTEXT_WIDGET(context), XtNvisual, &v,
		XtNcolormap, &cmap, XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmUNMAP); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNresizePolicy, XmRESIZE_GROW); ac++;
  XtSetArg (av[ac], XmNchildPlacement, XmPLACE_BELOW_SELECTION); ac++;

  shell = XmCreatePromptDialog(CONTEXT_WIDGET(context),
					"openNewsHost", av, ac);
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell, XmDIALOG_TEXT));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell, XmDIALOG_APPLY_BUTTON));
  fe_UnmanageChild_safe (XmSelectionBoxGetChild (shell, XmDIALOG_HELP_BUTTON));

  XtAddCallback(shell, XmNdestroyCallback, fe_newshost_close, context);
  XtAddCallback(shell, XmNokCallback, fe_newshost_cb, context);
  XtAddCallback(shell, XmNcancelCallback, fe_newshost_cb, context);

  ac = 0;
  form = XmCreateForm( shell, "openNewsHostForm", av, ac);

  ac = 0;
  kids[i++] = hostLabel = XmCreateLabelGadget(form, "hostLabel", av, ac);
  kids[i++] = hostText = XmCreateTextField(form, "hostText", av, ac);
  kids[i++] = portLabel = XmCreateLabelGadget(form, "portLabel", av, ac);
  kids[i++] = portText = XmCreateTextField(form, "portText", av, ac);
  kids[i++] = secure = XmCreateToggleButtonGadget(form, "secure", av, ac);

  /* Attachments */
  XtVaSetValues(hostLabel,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_NONE,
		0);
  XtVaSetValues(hostText,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, hostLabel,
		XmNrightAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_NONE,
		0);
  XtVaSetValues(portLabel,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, hostText,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, hostText,
		XmNrightAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, hostText,
		0);
  XtVaSetValues(portText,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, portLabel,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		0);
  XtVaSetValues(secure,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, hostLabel,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, hostLabel,
		XmNrightAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_NONE,
		0);

  if (hostLabel->core.height < hostText->core.height)
    XtVaSetValues(hostLabel, XmNheight, hostText->core.height, 0);

  newsdata->openNewsHost_shell = shell;
  newsdata->openNewsHost_host = hostText;
  newsdata->openNewsHost_port = portText;
  newsdata->openNewsHost_secure = secure;

  XtManageChildren(kids, i);
  XtManageChild(form);

  return;
}



int
FE_PromptForNewsHost (MWContext *context,
		      const char *prompt_string,
		      ReadFileNameCallbackFunction fn,
		      void *closure)
{
  fe_NewsContextData *newsdata = NEWS_CONTEXT_DATA(context);

  newsdata->openNewsHost_fn = fn;
  newsdata->openNewsHost_fnClosure = closure;

  if (!newsdata->openNewsHost_shell) {
    fe_make_openNewsHost_dialog(context);
  }

  XtManageChild(newsdata->openNewsHost_shell);
  XMapRaised(XtDisplay(newsdata->openNewsHost_shell),
		XtWindow(newsdata->openNewsHost_shell));

  return 0;
}

int
FE_PromptForFileName (MWContext *context,
		      const char *prompt_string,
		      const char *default_path,
		      XP_Bool file_must_exist_p,
		      XP_Bool directories_allowed_p,
		      ReadFileNameCallbackFunction fn,
		      void *closure)
{
  char *file = fe_ReadFileName (context, prompt_string, default_path,
				file_must_exist_p, 0);
  if (!file)
    return -1;

  fn (context, file, closure);
  return 0;
}


static MSG_CommandType
fe_widget_name_to_command (const char *name)
{
  /* This sucks a lot.  Sluuurp. */
  if (!name) abort ();

  /* FILE MENU
     =========
   */
  else if (!strcmp (name, "getNewMail")) return MSG_GetNewMail;
  else if (!strcmp (name, "deliverQueuedMessages"))
    return MSG_DeliverQueuedMessages;
  else if (!strcmp (name, "openFolder")) return MSG_OpenFolder;
  /* openSelectedFolder */
  else if (!strcmp (name, "newFolder")) return MSG_NewFolder;
  else if (!strcmp (name, "compressFolder")) return MSG_CompressFolder;
  else if (!strcmp (name, "compressAllFolders")) return MSG_CompressAllFolders;
  else if (!strcmp (name, "openNewsHost")) return MSG_OpenNewsHost;
  else if (!strcmp (name, "addNewsgroup")) return MSG_AddNewsGroup;
  else if (!strcmp (name, "emptyTrash")) return MSG_EmptyTrash;
  else if (!strcmp (name, "print")) return MSG_Print;

  /* EDIT MENU
     =========
   */
  else if (!strcmp (name, "undo")) return MSG_Undo;
  else if (!strcmp (name, "redo")) return MSG_Redo;
  else if (!strcmp (name, "deleteMessage")) return MSG_DeleteMessage;
  else if (!strcmp (name, "deleteFolder")) return MSG_DeleteFolder;
  else if (!strcmp (name, "cancelMessage")) return MSG_CancelMessage;
  else if (!strcmp (name, "deleteNewsHost")) return MSG_DeleteNewsHost;
  else if (!strcmp (name, "selectThread")) return MSG_SelectThread;
  else if (!strcmp (name, "selectMarkedMessages"))
    return MSG_SelectMarkedMessages;
  else if (!strcmp (name, "selectAllMessages")) return MSG_SelectAllMessages;
  else if (!strcmp (name, "unselectAllMessages"))
    return MSG_UnselectAllMessages;
  else if (!strcmp (name, "markSelectedMessages"))
    return MSG_MarkSelectedMessages;
  else if (!strcmp (name, "unmarkSelectedMessages"))
    return MSG_UnmarkSelectedMessages;
  else if (!strcmp (name, "findAgain")) return MSG_FindAgain;

  /* VIEW/SORT MENUS
     ===============
   */
  else if (!strcmp (name, "reSort")) return MSG_ReSort;
  else if (!strcmp (name, "threadMessages")) return MSG_ThreadMessages;
  else if (!strcmp (name, "sortBackward")) return MSG_SortBackward;
  else if (!strcmp (name, "sortByDate")) return MSG_SortByDate;
  else if (!strcmp (name, "sortBySubject")) return MSG_SortBySubject;
  else if (!strcmp (name, "sortBySender")) return MSG_SortBySender;
  else if (!strcmp (name, "sortByMessageNumber"))
    return MSG_SortByMessageNumber;
  else if (!strcmp (name, "rot13Message")) return MSG_Rot13Message;
  else if (!strcmp (name, "addFromNewest")) return MSG_AddFromNewest;
  else if (!strcmp (name, "addFromOldest")) return MSG_AddFromOldest;
  else if (!strcmp (name, "getMoreMessages")) return MSG_GetMoreMessages;
  else if (!strcmp (name, "getAllMessages")) return MSG_GetAllMessages;
  else if (!strcmp (name, "wrapLongLines")) return MSG_WrapLongLines;
  else if (!strcmp (name, "attachmentsInline")) return MSG_AttachmentsInline;
  else if (!strcmp (name, "attachmentsAsLinks")) return MSG_AttachmentsAsLinks;

  /* MESSAGE MENU
     ============
   */
  else if (!strcmp (name, "editAddressBook")) return MSG_EditAddressBook;
  else if (!strcmp (name, "editAddress")) return MSG_EditAddress;
  else if (!strcmp (name, "postNew")) return MSG_PostNew;
  else if (!strcmp (name, "postReply")) return MSG_PostReply;
  else if (!strcmp (name, "postAndMailReply")) return MSG_PostAndMailReply;
  else if (!strcmp (name, "mailNew")) return MSG_MailNew;
  else if (!strcmp (name, "replyToSender")) return MSG_ReplyToSender;
  else if (!strcmp (name, "replyToAll")) return MSG_ReplyToAll;
  else if (!strcmp (name, "forwardMessage")) return MSG_ForwardMessage;
  else if (!strcmp (name, "forwardMessageQuoted"))
    return MSG_ForwardMessageQuoted;
  else if (!strcmp (name, "markSelectedRead")) return MSG_MarkSelectedRead;
  else if (!strcmp (name, "markSelectedUnread")) return MSG_MarkSelectedUnread;
  /* markMessage */
  /* unmarkMessage */
  else if (!strcmp (name, "unmarkAllMessages")) return MSG_UnmarkAllMessages;
  else if (!strcmp (name, "copyMessagesInto")) return MSG_CopyMessagesInto;
  else if (!strcmp (name, "moveMessagesInto")) return MSG_MoveMessagesInto;
  else if (!strcmp (name, "saveMessageAs"))  return MSG_SaveMessagesAs;
  else if (!strcmp (name, "saveMessagesAs")) return MSG_SaveMessagesAs;
  /* saveMessagesAsAndDelete */

  /* GO MENU
     =======
   */
  else if (!strcmp (name, "firstMessage")) return MSG_FirstMessage;
  else if (!strcmp (name, "nextMessage")) return MSG_NextMessage;
  else if (!strcmp (name, "previousMessage")) return MSG_PreviousMessage;
  else if (!strcmp (name, "lastMessage")) return MSG_LastMessage;
  else if (!strcmp (name, "firstUnreadMessage")) return MSG_FirstUnreadMessage;
  else if (!strcmp (name, "nextUnreadMessage")) return MSG_NextUnreadMessage;
  else if (!strcmp (name, "previousUnreadMessage"))
    return MSG_PreviousUnreadMessage;
  else if (!strcmp (name, "lastUnreadMessage")) return MSG_LastUnreadMessage;
  else if (!strcmp (name, "firstMarkedMessage")) return MSG_FirstMarkedMessage;
  else if (!strcmp (name, "nextMarkedMessage")) return MSG_NextMarkedMessage;
  else if (!strcmp (name, "previousMarkedMessage"))
    return MSG_PreviousMarkedMessage;
  else if (!strcmp (name, "lastMarkedMessage")) return MSG_LastMarkedMessage;
  else if (!strcmp (name, "markThreadRead")) return MSG_MarkThreadRead;
  else if (!strcmp (name, "markAllRead")) return MSG_MarkAllRead;

  /* OPTIONS MENU
     ============
   */
  else if (!strcmp (name, "showSubscribedNewsGroups"))
    return MSG_ShowSubscribedNewsGroups;
  else if (!strcmp (name, "showActiveNewsGroups"))
    return MSG_ShowActiveNewsGroups;
  else if (!strcmp (name, "showAllNewsGroups")) return MSG_ShowAllNewsGroups;
  else if (!strcmp (name, "checkNewNewsGroups")) return MSG_CheckNewNewsGroups;
  else if (!strcmp (name, "showAllMessages")) return MSG_ShowAllMessages;
  else if (!strcmp (name, "showOnlyUnread")) return MSG_ShowOnlyUnreadMessages;
  else if (!strcmp (name, "showMicroHeaders")) return MSG_ShowMicroHeaders;
  else if (!strcmp (name, "showSomeHeaders")) return MSG_ShowSomeHeaders;
  else if (!strcmp (name, "showAllHeaders")) return MSG_ShowAllHeaders;

  /* COMPOSITION FILE MENU
	 =====================
   */
  else if (!strcmp (name, "send")) return MSG_SendMessage;
  else if (!strcmp (name, "sendLater")) return MSG_SendMessageLater;
  else if (!strcmp (name, "sendOrSendLater") || !strcmp (name, "sendMessage")) {
    if (fe_globalPrefs.queue_for_later_p)
      return MSG_SendMessageLater;
    else
      return MSG_SendMessage;
  }
  else if (!strcmp (name, "attachFile")) return MSG_Attach;
  else if (!strcmp (name, "quoteMessage")) return MSG_QuoteMessage;

  /* COMPOSITION VIEW MENU
     =====================
   */
  else if (!strcmp (name, "showFrom")) return MSG_ShowFrom;
  else if (!strcmp (name, "showReplyTo")) return MSG_ShowReplyTo;
  else if (!strcmp (name, "showTo")) return MSG_ShowTo;
  else if (!strcmp (name, "showCC")) return MSG_ShowCC;
  else if (!strcmp (name, "showBCC")) return MSG_ShowBCC;
  else if (!strcmp (name, "showFCC")) return MSG_ShowFCC;
  else if (!strcmp (name, "showPostTo")) return MSG_ShowPostTo;
  else if (!strcmp (name, "showFollowupTo")) return MSG_ShowFollowupTo;
  else if (!strcmp (name, "showSubject")) return MSG_ShowSubject;
  else if (!strcmp (name, "showAttachments")) return MSG_ShowAttachments;

  /* COMPOSITION OPTIONS MENU
     ========================
   */
  else if (!strcmp (name, "sendFormattedText")) return MSG_SendFormattedText;
  else if (!strcmp (name, "sendEncrypted")) return MSG_SendEncrypted;
  else if (!strcmp (name, "sendSigned")) return MSG_SendSigned;

  /* NEWS POPUP MENU
     ===============
   */
  else if (!strcmp (name, "subscribeGroup"))
    return MSG_SubscribeSelectedGroups;
  else if (!strcmp (name, "unsubscribeGroup"))
    return MSG_UnsubscribeSelectedGroups;

  else if (!strcmp (name, "securityAdvisor"))
    return MSG_SecurityAdvisor;

#ifdef DEBUG
  else if (!strcmp (name, "openBrowser") ||
	   !strcmp (name, "openMail") ||
	   !strcmp (name, "openNews") ||
	   !strcmp (name, "delete") ||
	   !strcmp (name, "quit") ||
	   !strcmp (name, "cut") ||
	   !strcmp (name, "copy") ||
	   !strcmp (name, "paste") ||
	   !strcmp (name, "clear") ||
	   !strcmp (name, "find") ||
	   !strcmp (name, "findAgain") ||
	   !strcmp (name, "reload") ||
	   !strcmp (name, "loadImages") ||
	   !strcmp (name, "refresh") ||
	   !strcmp (name, "source") ||
	   !strcmp (name, "abort") ||
	   !strcmp (name, "generalPrefs") ||
	   !strcmp (name, "mailNewsPrefs") ||
	   !strcmp (name, "networkPrefs") ||
	   !strcmp (name, "securityPrefs") ||
	   !strcmp (name, "showToolbar") ||
	   !strcmp (name, "autoLoadImages") ||
	   !strcmp (name, "saveOptions") ||
	   !strcmp (name, "about") ||
	   !strcmp (name, "aboutMailAndNews") ||
	   !strcmp (name, "aboutUsenet") ||
	   !strcmp (name, "registration") ||
	   !strcmp (name, "manual") ||
	   !strcmp (name, "relnotes") ||
	   !strcmp (name, "faq") ||
	   !strcmp (name, "aboutSecurity") ||
	   !strcmp (name, "feedback") ||
	   !strcmp (name, "support") ||
	   !strcmp (name, "howTo") ||
	   !strcmp (name, "spacer") ||
	   !strcmp (name, "openComposition") || /* ### */
	   !strcmp (name, "pasteQuote") ||
	   !strcmp (name, "window_button") ||
	   !strcmp (name, "rot13") ||
	   !strcmp (name, "viewBookmark") ||
	   !strcmp (name, "viewHistory") ||
	   !strcmp (name, "docEncoding") ||
	   !strcmp (name, "sort") ||
	   !strcmp (name, "upgrade") ||
	   !strcmp (name, "aboutplugins") ||
	   !strcmp (name, "attachFile") ||
	   !strcmp (name, "clearAllText") ||
	   !strcmp (name, "selectAllText") ||
	   !strcmp (name, "windowButton") ||
	   !strcmp (name, "deliverNow") ||
	   !strcmp (name, "deliverLater") ||
	   !strcmp (name, "openURL") ||
	   !strcmp (name, "addURLBookmark") ||
	   !strcmp (name, "openURLNewWindow") ||
	   !strcmp (name, "saveURL") ||
	   !strcmp (name, "copyURLToClip") ||
	   !strcmp (name, "openImage") ||
	   !strcmp (name, "saveImage") ||
	   !strcmp (name, "copyImageURLToClip") ||
	   !strcmp (name, "showHeaders") ||
	   !strcmp (name, "wrapLines") ||
	   !strcmp (name, "button")		/* what? */

#ifdef DEBUG_jwz
           || !strcmp (name, "toggleJS")
           || !strcmp (name, "toggleAnim")
           || !strcmp (name, "toggleLoops")
           || !strcmp (name, "toggleSizes")
#endif /* DEBUG_jwz */

	   )
    return ((MSG_CommandType) ~0);

#endif /* DEBUG */

  else
    {
#ifdef DEBUG
      printf("Need to add %s to fe_widget_name_to_command\n", name);
#endif
    return ((MSG_CommandType) ~0);
    }
}


void fe_msg_sensitize_widget(Widget widget, XtPointer closure,
			     XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  MSG_CommandType command;
  const char *name = XtName (widget);
  int status;
  const char *string = 0;
  XmString xmstring = 0;
  XP_Bool selectable_p = FALSE;
  MSG_COMMAND_CHECK_STATE selected_p = MSG_NotUsed;
  XP_Bool haveQueuedMail = False;
  if (!name)
    return;
  command = fe_widget_name_to_command (name);
  if (command == (MSG_CommandType) ~0)
    return;

  status = MSG_CommandStatus (context, command, &selectable_p, &selected_p,
			      &string, 0);
  XP_ASSERT (status >= 0);
  xmstring = XmStringCreateLtoR ((/* idiots */ char *) (string ? string : ""),
				 XmFONTLIST_DEFAULT_TAG);

  if (command == MSG_DeliverQueuedMessages) {
      MSG_CommandStatus (context, command, &haveQueuedMail, NULL, NULL, NULL);
      if (haveQueuedMail) selectable_p = True;
  }

  XtVaSetValues (widget,
		 XmNlabelString, xmstring,
		 XmNsensitive, selectable_p,
		 XmNset, (selected_p == MSG_Checked),
		 0);
  XmStringFree (xmstring);
}


void fe_MsgSensitizeSubmenu(Widget widget, XtPointer closure,
			    XtPointer call_data)
{
  Widget *buttons = 0, menu = 0;
  Cardinal nbuttons = 0;
  int i;
  XtVaGetValues (widget, XmNsubMenuId, &menu, 0);
  if (!menu) return;
  XtVaGetValues (menu, XmNchildren, &buttons, XmNnumChildren, &nbuttons, 0);
  for (i = 0; i < nbuttons; i++)
    {
      Widget item = buttons[i];
      if (XmIsToggleButton(item) || XmIsToggleButtonGadget(item) ||
	      XmIsPushButton(item) || XmIsPushButtonGadget(item) ||
	      XmIsCascadeButtonGadget(item))
	fe_msg_sensitize_widget(item, closure, call_data);
    }
}


void
fe_MsgSensitizeChildren(Widget widget, XtPointer closure, XtPointer call_data)
{
  Widget *buttons = 0, menu = 0;
  Cardinal nbuttons = 0;
  int i;
  XtVaGetValues (widget, XmNchildren, &buttons, XmNnumChildren, &nbuttons, 0);
  for (i = 0; i < nbuttons; i++)
    {
      Widget item = buttons[i];
      if (XmIsToggleButton(item) || XmIsToggleButtonGadget(item) ||
	      XmIsPushButton(item) || XmIsPushButtonGadget(item) ||
	      XmIsCascadeButtonGadget(item))
	fe_msg_sensitize_widget(item, closure, call_data);

      if (XmIsCascadeButtonGadget(item)) {
	XtVaGetValues (widget, XmNsubMenuId, &menu, 0);
	if (menu) fe_MsgSensitizeChildren(menu, closure, call_data);
      }
    }
}


void fe_MsgSensitizeMenubar(Widget widget, XtPointer closure,
			    XtPointer call_data)
{
  Widget *buttons = 0;
  Cardinal nbuttons = 0;
  int i;
  XtVaGetValues (widget, XmNchildren, &buttons, XmNnumChildren, &nbuttons, 0);
  for (i = 0; i < nbuttons; i++)
      fe_MsgSensitizeSubmenu(buttons[i], closure, call_data);
}


static void
fe_sensitize_all_text_widgets (Widget widget, XP_Bool sensitive_p)
{
  Widget *kids = 0;
  Cardinal nkids = 0;
  int i;
  XtVaGetValues (widget, XmNchildren, &kids, XmNnumChildren, &nkids, 0);
  for (i = 0; i < nkids; i++)
    {
      if (XmIsTextField(kids[i]) || XmIsText(kids[i]))
	XtVaSetValues(kids[i], XmNsensitive, sensitive_p, 0);
      else
	fe_sensitize_all_text_widgets (kids[i], sensitive_p);
    }
}


extern Pixmap fe_SecurityAdvisorButtonPixmap(MWContext *context,
					     XP_Bool encrypting,
					     XP_Bool signing);

static void
fe_hack_security_advisor_button(MWContext *context, Widget button)
{
  MSG_COMMAND_CHECK_STATE sending_encrypted_p = MSG_NotUsed;
  MSG_COMMAND_CHECK_STATE sending_signed_p = MSG_NotUsed;
  Pixmap icon = 0, old_icon = 0;
  MSG_CommandStatus (context, MSG_SendEncrypted, 0, &sending_encrypted_p,0,0);
  MSG_CommandStatus (context, MSG_SendSigned,    0, &sending_signed_p,   0,0);

  icon = fe_SecurityAdvisorButtonPixmap(context,
					(sending_encrypted_p == MSG_Checked),
					(sending_signed_p == MSG_Checked));

  XtVaGetValues (button, XmNlabelPixmap, &old_icon, 0);
  if (icon != old_icon)
    XtVaSetValues(button, XmNlabelPixmap, icon, 0);
}

void
FE_UpdateToolbar (MWContext *context)
{
  Widget toolbar;
  Widget *buttons = 0;
  Cardinal nbuttons = 0;
  int i;
  int text_sensitivity = 0;

#if 0
  /* This is causing core dumps. For now we will not do this
   * assert. We still need to figure out why this routine is
   * called for non-{mail/news} contexts.
   */
  XP_ASSERT (context->type == MWContextMail ||
	     context->type == MWContextNews ||
	     context->type == MWContextMessageComposition);
#endif
  if (context->type != MWContextMail && context->type != MWContextNews &&
	     context->type != MWContextMessageComposition)
    return;

  toolbar = CONTEXT_DATA (context)->toolbar;
  XtVaGetValues (toolbar, XmNchildren, &buttons, XmNnumChildren, &nbuttons, 0);
  for (i = 0; i < nbuttons; i++)
    {
      Widget button = buttons[i];
      const char *name = XtName (button);
      XP_Bool selectable_p = FALSE;
      XP_Bool was_selectable_p = FALSE;
      MSG_CommandType command;
      int status;
      if (!name)
	continue;
      command = fe_widget_name_to_command (name);
      if (command == (MSG_CommandType) ~0)
	continue;

      status = MSG_CommandStatus (context, command, &selectable_p, 0,0,0);
      if (status < 0)
	continue;

      if (command == MSG_SecurityAdvisor)
	fe_hack_security_advisor_button(context, button);

      XtVaGetValues (button, XmNsensitive, &was_selectable_p, 0);
      if (was_selectable_p != selectable_p)
	XtVaSetValues (button, XmNsensitive, selectable_p, 0);

      /* Make the sensitivity of the text widgets be the same as that of
	 the Send button. */
      if (command == MSG_SendMessage && was_selectable_p != selectable_p)
	text_sensitivity = (selectable_p ? 1 : -1);
    }
  if (context->type == MWContextMail || context->type == MWContextNews) {
    Widget outline = CONTEXT_DATA(context)->messagelist;
    fe_OutlineSetHeaderHighlight(outline, "Date",
				 MSG_GetToggleStatus(context, MSG_SortByDate)
				 == MSG_Checked);
    fe_OutlineSetHeaderHighlight(outline, "Sender",
				 MSG_GetToggleStatus(context, MSG_SortBySender)
				 == MSG_Checked);
    fe_OutlineSetHeaderHighlight(outline, "Subject",
				 MSG_GetToggleStatus(context,MSG_SortBySubject)
				 == MSG_Checked);
    fe_OutlineSetHeaderHighlight(outline, "Thread",
				MSG_GetToggleStatus(context,MSG_ThreadMessages)
				 == MSG_Checked);
    fe_OutlineChangeHeaderLabel(outline, "Sender",
				MSG_DisplayingRecipients(context) ? "Recipient"
				: NULL); /* Fix i18n ### */
  }
  else if (context->type == MWContextMessageComposition &&
	   text_sensitivity != 0)
    {
      fe_sensitize_all_text_widgets(CONTEXT_WIDGET(context),
				    text_sensitivity == 1);

      /* If we're making the text areas be insensitive, make sure we
	 make the abort button be sensitive. */
      if (text_sensitivity != 1 && CONTEXT_DATA(context)->abort_button)
	XtVaSetValues(CONTEXT_DATA(context)->abort_button,
		      XmNsensitive, True, 0);
      else
	/* Otherwise, set the abort button in the normal way (don't assume
	   it should be insensitive, though it probably will be.) */
	XFE_EnableClicking (context);
    }
}


void
FE_RememberPopPassword (MWContext *context, const char *password)
{
  /* Store password into preferences. */
  StrAllocCopy (fe_globalPrefs.pop3_password, password);

  /* If user has already requesting saving it, do that. */
  if (fe_globalPrefs.rememberPswd)
    {
      if (!XFE_SavePrefs ((char *) fe_globalData.user_prefs_file,
			  &fe_globalPrefs))
	fe_perror (context, XP_GetString( XFE_ERROR_SAVING_PASSWORD ) );
    }
}


void
fe_undo_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (context->type == MWContextMail ||
      context->type == MWContextNews) {
    MSG_Command (context, MSG_Undo);
  } else {
    FE_Alert (context, XP_GetString( XFE_UNIMPLEMENTED) );
  }
}


void
fe_redo_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  fe_UserActivity (context);
  if (context->type == MWContextMail ||
      context->type == MWContextNews) {
    MSG_Command (context, MSG_Redo);
  } else {
    FE_Alert (context, XP_GetString( XFE_UNIMPLEMENTED) );
  }
}


/********************
 * Mail Popup menus *
 ********************/


void fe_msg_command(Widget widget, XtPointer closure, XtPointer call_data);


/* Mail popup menu over Folder listing */
static struct fe_button fe_mailPopupFolderMenu[] =
{
    { "getNewMail",	fe_msg_command,		SELECTABLE,		0 },
    { "deliverQueuedMessages",fe_msg_command,	SELECTABLE,		0 },
    { "openFolder",	fe_msg_command,		SELECTABLE,		0 },
    { "newFolder",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "deleteFolder",	fe_msg_command,		SELECTABLE,		0 },
    { 0 },
};

/* Mail popup menu over Message listing */
static struct fe_button fe_mailPopupMessageMenu[] =
{
    { "replyToSender",	fe_msg_command,		SELECTABLE,		0 },
    { "replyToAll",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessageQuoted", fe_msg_command,	SELECTABLE,		0 },
    { "selectThread",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "deleteMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "reSort",	fe_msg_command,			SELECTABLE,	    	0 },
    { "selectMarkedMessages",fe_msg_command,	SELECTABLE,		0 },
    { "selectAllMessages",fe_msg_command,	SELECTABLE,		0 },
    { "markAllRead",	fe_msg_command,		SELECTABLE,		0 },
    { 0 },
};

/* Mail popup menu over Message Body */
static struct fe_button fe_mailPopupBodyMenu[] =
{
    { "replyToSender",	fe_msg_command,		SELECTABLE,		0 },
    { "replyToAll",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessageQuoted", fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "editAddress",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "deleteMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "rot13Message",	fe_msg_command,		SELECTABLE,		0 },
    { 0 },
};

/********************
 * News Popup menus *
 ********************/

/* News popup menu over Newsgroups listing */
static struct fe_button fe_newsPopupGroupsMenu[] =
{
    { "subscribeGroup",fe_msg_command,		SELECTABLE, 		0 },
    { "unsubscribeGroup",fe_msg_command,	SELECTABLE, 		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "showSubscribedNewsGroups",fe_msg_command,RADIO,	 		0 },
    { "showActiveNewsGroups",fe_msg_command,	RADIO, 			0 },
    { "showAllNewsGroups",   fe_msg_command,	RADIO, 			0 },
    { "checkNewNewsGroups",fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "openNewsHost",	fe_msg_command,		SELECTABLE,		0 },
    { "deleteNewsHost",	fe_msg_command,		SELECTABLE,		0 },
    { 0 },
};

/* News popup menu over Article listing */
static struct fe_button fe_newsPopupArticleMenu[] =
{
    { "postReply",	fe_msg_command,		SELECTABLE,		0 },
    { "postAndMailReply",fe_msg_command,	SELECTABLE,		0 },
    { "replyToSender",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessageQuoted", fe_msg_command,	SELECTABLE,		0 },
    { "selectThread",fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "reSort",	fe_msg_command,			SELECTABLE,	    	0 },
    { "selectMarkedMessages",fe_msg_command,	SELECTABLE,		0 },
    { "selectAllMessages",fe_msg_command,	SELECTABLE,		0 },
    { "markAllRead",	fe_msg_command,		SELECTABLE,		0 },
    { 0 },
};

/* News popup menu over Article Body */
static struct fe_button fe_newsPopupBodyMenu[] =
{
    { "postReply",	fe_msg_command,		SELECTABLE,		0 },
    { "postAndMailReply",fe_msg_command,	SELECTABLE,		0 },
    { "replyToSender",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessage",	fe_msg_command,		SELECTABLE,		0 },
    { "forwardMessageQuoted", fe_msg_command,	SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "editAddress",	fe_msg_command,		SELECTABLE,		0 },
    { "",		0,			UNSELECTABLE,		0 },
    { "rot13Message",	fe_msg_command,		SELECTABLE,		0 },
    { 0 },
};

/*********************
 * Mail/News actions *
 *********************/

/* This one popup action takes upon itself to display the correct
 * popup menu depending on whether we are in
 *	News
 *	Mail
 * and depending on whether the cursor is on
 *	{Folder/Group} listing,
 *	{Message/Article} Listing
 *	Body of {Message/Article}
 *
 * That is 6 popups all together.
*/

static int
fe_isxyin_widget (Widget widget, int x, int y)
{
    Position rootx;
    Position rooty;

    XtTranslateCoords(widget, 0, 0, &rootx, &rooty);
    x -= rootx;
    y -= rooty;
    if (x < 0 || y < 0 ||
	x >= widget->core.width || y >= widget->core.height)
	return -1;
    else
	return 0;
}

/* From commands.c */
extern void fe_remember_last_popup_item (Widget, XtPointer, XtPointer);
extern struct fe_button fe_popup_link[];
extern int nfe_popup_link;
extern struct fe_button fe_popup_image[];
extern int nfe_popup_image;
extern URL_Struct *fe_url_under_mouse;
extern URL_Struct *fe_image_under_mouse;
extern char *fe_last_popup_item;

static void
fe_mailNewsPopup_action (Widget widget, XEvent *event, String *iav, Cardinal *iac)
{
    MWContext *context = fe_WidgetToMWContext (widget);
    struct fe_button *which_popup = NULL;
    Widget *popup = NULL;

    /* These are used to findout which of the three areas the mouse is in */
    Widget folderlist, messagelist, messagebody;
    Widget area;

    Boolean isBodyPopup = False;
    Widget item;
    Widget kids [30];
    Arg av [20];
    int ac;
    int i, j;
    unsigned long x, y;
    LO_Element *le;
    char buf [255];
    char/* *s,*/ *image_url;
    XmString padding, xmstring, xmstring2, xmstring3;
    History_entry *h;
    Boolean image_delayed_p = False;
    struct fe_Pixmap *fep;
    Widget last = 0;

    if (!context) return;
    fe_UserActivity (context);
    if ((context->type != MWContextMail) && (context->type != MWContextNews))
      {
	XBell (XtDisplay (widget), 0);
	return;
      }

    /* Decide which popup to use. Mail has 3 popups depending on whether
       we are on Message Body, Message List, Folder List.

       To do this we need lookfor one of the three widgets that are
       roots of the three areas folderlist, messagelist, messagebody
       by traversing up the widget hierarchy.
    */
    folderlist = CONTEXT_DATA(context)->folderlist;
    messagelist = CONTEXT_DATA(context)->messagelist;
    messagebody = CONTEXT_DATA(context)->main_pane;
    area = widget;
    while (!XtIsWMShell (area) && (area != folderlist) && (area != messagelist)
		&& (area != messagebody))
	area = XtParent(area);

    if (fe_isxyin_widget(CONTEXT_DATA(context)->folderlist,
			event->xbutton.x_root, event->xbutton.y_root) >= 0) {
	popup = & CONTEXT_DATA(context) -> mailPopupFolder;
	if (context->type == MWContextMail)
	    which_popup = fe_mailPopupFolderMenu;
	else
	    which_popup = fe_newsPopupGroupsMenu;
	isBodyPopup = False;
    }
    else if (fe_isxyin_widget(CONTEXT_DATA(context)->messagelist,
			event->xbutton.x_root, event->xbutton.y_root) >= 0) {
	popup = & CONTEXT_DATA(context) -> mailPopupMessage;
	if (context->type == MWContextMail)
	    which_popup = fe_mailPopupMessageMenu;
	else
	    which_popup = fe_newsPopupArticleMenu;
	isBodyPopup = False;
    }
    else if (fe_isxyin_widget(CONTEXT_DATA(context)->main_pane,
			event->xbutton.x_root, event->xbutton.y_root) >= 0) {
	popup = & CONTEXT_DATA(context) -> mailPopupBody;
	if (context->type == MWContextMail)
	    which_popup = fe_mailPopupBodyMenu;
	else
	    which_popup = fe_newsPopupBodyMenu;
	isBodyPopup = True;
    }

    if (!which_popup) return;

    if (isBodyPopup && *popup) {
	/* We need to create the popup menu fresh */
	XtDestroyWidget(*popup);
	*popup = 0;
    }

    if (!*popup) {
	/* Create the popup */
	Widget parent;
	Visual *v = 0;
	Colormap cmap = 0;
	Cardinal depth = 0;
	int i;

	parent = CONTEXT_WIDGET(context);
	XtVaGetValues (parent, XtNvisual, &v, XtNcolormap, &cmap,
       		          XtNdepth, &depth, 0);

	ac = 0;
	XtSetArg (av[ac], XmNvisual, v); ac++;
	XtSetArg (av[ac], XmNcolormap, cmap); ac++;
	XtSetArg (av[ac], XmNdepth, depth); ac++;
	*popup = XmCreatePopupMenu (parent, "popup", av, ac);

	i = 0;
	ac = 0;
	kids [i++] = XmCreateLabelGadget (*popup, "title", av, ac);
	kids [i++] = XmCreateSeparatorGadget (*popup, "titleSeparator", av, ac);
	XtManageChildren(kids, i);

	fe_CreateSubmenu(*popup, which_popup, context, context,
				CONTEXT_DATA(context)); 
    }

    if (isBodyPopup) {
	/* The body popup needs more items depending on what element is
	   under the cursor */
	Widget menu = *popup;
	i = 0;
	h = SHIST_GetCurrent (&context->hist);

	padding = XmStringCreateLtoR ("", XmFONTLIST_DEFAULT_TAG);

	if (fe_url_under_mouse)   NET_FreeURLStruct (fe_url_under_mouse);
	if (fe_image_under_mouse) NET_FreeURLStruct (fe_image_under_mouse);
	fe_url_under_mouse   = 0;
	fe_image_under_mouse = 0;

	/* Find the URLs under the mouse. */
	fe_EventLOCoords (context, event, &x, &y);
	le = LO_XYToElement (context, x, y);
	if (le)
	switch (le->type) {
	    case LO_TEXT:
		if (le->lo_text.anchor_href)
		    fe_url_under_mouse = NET_CreateURLStruct ((char *) le->lo_text.anchor_href->anchor, FALSE);
	   	break;
	    case LO_IMAGE:
		if (le->lo_image.anchor_href)
		    fe_url_under_mouse = NET_CreateURLStruct ((char *) le->lo_image.anchor_href->anchor, FALSE);

		/* If this is an internal-external-reconnect image, then the *real* url
       		   follows the "internal-external-reconnect:" prefix. Gag gag gag. */
		image_url = (char*)le->lo_image.image_url;
		if (image_url) {
		    if (!XP_STRNCMP (image_url, "internal-external-reconnect:", 28))
			image_url += 28;
		    fe_image_under_mouse = NET_CreateURLStruct (image_url, NET_DONT_RELOAD);
		}

		fep = (struct fe_Pixmap *) le->lo_image.FE_Data;
		image_delayed_p = (fep &&
			   fep->type == fep_icon &&
			   fep->data.icon_number == IL_IMAGE_DELAYED);

		if (fe_url_under_mouse &&
		    strncmp (fe_url_under_mouse->address, "about:", 6) &&
		    le->lo_image.image_attr->attrmask & LO_ATTR_ISMAP) {
		    /* cribbed from fe_activate_link_action() */
		    long cx = event->xbutton.x + CONTEXT_DATA (context)->document_x;
		    long cy = event->xbutton.y + CONTEXT_DATA (context)->document_y;
		    long ix = le->lo_image.x + le->lo_image.x_offset;
		    long iy = le->lo_image.y + le->lo_image.y_offset;
		    long x = cx - ix - le->lo_image.border_width;
		    long y = cy - iy - le->lo_image.border_width;
		    NET_AddCoordinatesToURLStruct (fe_url_under_mouse,
						  ((x < 0) ? 0 : x),
						  ((y < 0) ? 0 : y));
	    }
	    break;
	}

	if (h && h->is_binary) {
	    if (fe_image_under_mouse)
		NET_FreeURLStruct (fe_image_under_mouse);
	    fe_image_under_mouse = NET_CreateURLStruct (h->address, FALSE);
	}

	if (fe_url_under_mouse &&
	    !strncmp ("mailto:", fe_url_under_mouse->address, 7)) {
	    NET_FreeURLStruct (fe_url_under_mouse);
	    fe_url_under_mouse = 0;
	}

	/* Add the referer to the URLs. */
	if (h && h->address) {
	    if (fe_url_under_mouse)
		fe_url_under_mouse->referer = strdup (h->address);
	    if (fe_image_under_mouse)
		fe_image_under_mouse->referer = strdup (h->address);
	}

  if (fe_url_under_mouse) {
      kids [i++] = XmCreateSeparatorGadget (menu, "separator2", av, ac);
      kids [i++] = XmCreateSeparatorGadget (menu, "separator3", av, ac);

      for (j = 0; j < nfe_popup_link; j++)
	{
	  item = XmCreatePushButtonGadget (menu, fe_popup_link[j].name,av,ac);
	  XtVaGetValues (item, XmNlabelString, &xmstring, 0);
	  if (j == 0)
	    {
	      if (fe_url_under_mouse)
		{
		  const char *url = fe_url_under_mouse->address;
		  char *sug = MimeGuessURLContentName(context, url);
		  const char *s;
		  if (sug) url = sug;
		  s = strrchr (url, '/');
		  if (s)
		    s++;
		  else
		    s = url;
		  PR_snprintf (buf, sizeof (buf), " (%.200s)", s);
		  if (sug) free(sug);

		  xmstring2 = XmStringCreateLtoR(buf,XmFONTLIST_DEFAULT_TAG);
		  xmstring3 = XmStringConcat (xmstring, xmstring2);
		  XtVaSetValues (item, XmNlabelString, xmstring3, 0);
		  XmStringFree (xmstring2);
		  XmStringFree (xmstring3);
		}
	    }
	  else
	    {
	      xmstring2 = XmStringConcat (padding, xmstring);
	      XtVaSetValues (item, XmNlabelString, xmstring2, 0);
	      XmStringFree (xmstring2);
	    }
	  XmStringFree (xmstring);
	  XtAddCallback (item, XmNactivateCallback,
			 fe_remember_last_popup_item, 0);
	  XtAddCallback (item, XmNactivateCallback, fe_popup_link[j].callback,
			 context);
	  if (! fe_url_under_mouse)
	    XtVaSetValues (item, XmNsensitive, False, 0);

	  if (fe_last_popup_item &&
	      !strcmp (fe_last_popup_item, fe_popup_link[j].name))
	    last = item;

	  kids [i++] = item;
	}
  }
  if (fe_image_under_mouse) {
      kids [i++] = XmCreateSeparatorGadget (menu, "separator2", av, ac);
      kids [i++] = XmCreateSeparatorGadget (menu, "separator3", av, ac);

      for (j = 0; j < nfe_popup_image; j++)
	{
	  item = XmCreatePushButtonGadget (menu, fe_popup_image[j].name,av,ac);
	  XtVaGetValues (item, XmNlabelString, &xmstring, 0);
	  if (j == 0)
	    {
	      if (fe_image_under_mouse)
		{
		  const char *url = fe_image_under_mouse->address;
		  char *sug = MimeGuessURLContentName(context, url);
		  const char *s;
		  if (sug) url = sug;
		  s = strrchr (url, '/');
		  if (s)
		    s++;
		  else
		    s = url;
		  PR_snprintf (buf, sizeof (buf), " (%.200s)", s);
		  if (sug) free(sug);

		  xmstring2 = XmStringCreateLtoR(buf,XmFONTLIST_DEFAULT_TAG);
		  xmstring3 = XmStringConcat (xmstring, xmstring2);
		  XtVaSetValues (item, XmNlabelString, xmstring3, 0);
		  XmStringFree (xmstring2);
		  XmStringFree (xmstring3);
		}
	    }
	  else
	    {
	      xmstring2 = XmStringConcat (padding, xmstring);
	      XtVaSetValues (item, XmNlabelString, xmstring2, 0);
	      XmStringFree (xmstring2);
	    }
	  XmStringFree (xmstring);
	  XtAddCallback (item, XmNactivateCallback,
			 fe_remember_last_popup_item, 0);
	  XtAddCallback (item, XmNactivateCallback, fe_popup_image[j].callback,
			 context);
	  if (! fe_image_under_mouse)
	    XtVaSetValues (item, XmNsensitive, False, 0);

	  if (fe_last_popup_item &&
	      !strcmp (fe_last_popup_item, fe_popup_image[j].name))
	    last = item;

	  kids [i++] = item;
	}
  }
 	XmStringFree(padding);
	if (last)
	    XtVaSetValues (menu, XmNmenuHistory, last, 0);
	XtManageChildren(kids, i);
    }

    XmMenuPosition (*popup, (XButtonPressedEvent *) event);
    fe_MsgSensitizeChildren(*popup, (XtPointer) context, NULL);
    XtManageChild(*popup);
}

/* fe_msgDoCommand()
 * Executes a commands on a Mail/News context after checking for
 *  validity of command at the instant of call.  Beeps if the
 *  command isn't valid, or if it's not a mail/news context.
 */

void
fe_msgDoCommand(MWContext *context, MSG_CommandType command)
{
    int status;
    const char *string = 0;
    XP_Bool selectable_p = FALSE;
    MSG_COMMAND_CHECK_STATE selected_p = MSG_NotUsed;

    if (!context) return;
    fe_UserActivity (context);
    if (context->type != MWContextMail &&
	context->type != MWContextNews &&
	context->type != MWContextMessageComposition)
      {
	/* It's illegal to call MSG_CommandStatus() on a context not of the
	   above types.  We can only get here when invoked by an accelerator:
	   otherwise, the menu item wouldn't have been present at all. */
	XBell (XtDisplay (CONTEXT_WIDGET(context)), 0);
	return;
      }

    if (command == (MSG_CommandType) ~0) return;
    status = MSG_CommandStatus (context, command, &selectable_p, &selected_p,
			      &string, 0);
    if ((status < 0) || !selectable_p)
      {
	/* We can only get here when invoked by an accelerator -- otherwise,
	   the menu item would have been unselectable. */
	XBell (XtDisplay (CONTEXT_WIDGET(context)), 0);
      }
    else
      {
	if (command == MSG_SendMessage && fe_globalPrefs.queue_for_later_p)
	  command = MSG_SendMessageLater;  /* special case 1 */

	if (command == MSG_GetNewMail)     /* special case 2 */
	  fe_mn_getnewmail (0, (XtPointer)context, NULL);
	else
	  MSG_Command (context, command);
      }
}


/* Called from menu items here and in compose.c. */
void fe_msg_command(Widget widget, XtPointer closure, XtPointer call_data)
{
  XP_ASSERT(widget);
  XP_ASSERT(closure);
  if (!widget || !closure) return;
  fe_msgDoCommand ((MWContext *) closure,
		   fe_widget_name_to_command (XtName (widget)));
}



void
fe_redo_action (Widget widget, XEvent *event, String *av, Cardinal *ac)
{
  MWContext *context = fe_WidgetToMWContext (widget);
  fe_redo_cb (widget, context, 0);
}


/* dialogs.c */
extern void fe_mail_text_modify_cb (Widget, XtPointer, XtPointer);

void
fe_set_compose_wrap_state(MWContext *context, XP_Bool wrap_p)
{
  Boolean old_wrap, new_wrap, old_scroll, new_scroll;
  Widget text, parent, gramp;
  XP_ASSERT(context->type == MWContextMessageComposition);
  XP_ASSERT(CONTEXT_DATA(context)->mcBodyText);

  text = CONTEXT_DATA(context)->mcBodyText;
  if (!text) return;

  parent = XtParent(text);
  gramp = XtParent(parent);

  XtVaGetValues(text,
		XmNwordWrap,	     &old_wrap,
		XmNscrollHorizontal, &old_scroll,
		0);

  new_wrap   = wrap_p;
  new_scroll = !wrap_p;
  CONTEXT_DATA (context)->compose_wrap_lines_p = wrap_p;

  if (new_wrap == old_wrap &&
      new_scroll == old_scroll)
    return;

  {
    String body = 0;
    XmTextPosition cursor = 0;
    unsigned char top = 0, bot = 0, left = 0, right = 0;
    Widget topw = 0, botw = 0, leftw = 0, rightw = 0;
    Arg av [20];
    int ac = 0;
    XmFontList font_list = 0;

    /* #### warning much of this is cloned from mozilla.c
       (fe_create_composition_widgets) */

    XtVaGetValues (text,
		   XmNvalue, &body,
		   XmNcursorPosition, &cursor,
		   XmNfontList, &font_list,
		   0);
    XtVaGetValues (parent,
		   XmNtopAttachment, &top,	XmNtopWidget, &topw,
		   XmNbottomAttachment, &bot,	XmNbottomWidget, &botw,
		   XmNleftAttachment, &left,	XmNleftWidget, &leftw,
		   XmNrightAttachment, &right,	XmNrightWidget, &rightw,
		   0);

    XtUnmanageChild(parent);
    XtDestroyWidget(parent);
    CONTEXT_DATA(context)->mcBodyText = text = parent = 0;

    XtSetArg (av[ac], XmNeditable, True); ac++;
    XtSetArg (av[ac], XmNcursorPositionVisible, True); ac++;
    XtSetArg (av[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;

    XtSetArg (av[ac], XmNscrollVertical,   TRUE); ac++;
    XtSetArg (av[ac], XmNscrollHorizontal, new_scroll); ac++;
    XtSetArg (av[ac], XmNwordWrap,	   new_wrap); ac++;

    XtSetArg (av[ac], XmNfontList, font_list); ac++;

    text = XmCreateScrolledText (gramp, "mailto_bodyText", av, ac);
    parent = XtParent(text);

    if (fe_LocaleCharSetID & MULTIBYTE) {
      Dimension columns;
      XtVaGetValues(text, XmNcolumns, &columns, NULL);
      XtVaSetValues(text, XmNcolumns, (columns + 1) / 2, NULL);
    }

    fe_HackDialogTranslations (text);

    /* Warning -- this assumes no widgets are attached to this one
       (which is the normal state of affairs, since this is the widget
       that should grow.) */
    XtVaSetValues (parent,
		   XmNtopAttachment,    top,   XmNtopWidget,    topw,
		   XmNbottomAttachment, bot,   XmNbottomWidget, botw,
		   XmNleftAttachment,   left,  XmNleftWidget,   leftw,
		   XmNrightAttachment,  right, XmNrightWidget,  rightw,
		   0);
    XtVaSetValues (text,
		   XmNfontList, font_list,
		   XmNvalue, (body ? body : ""),
		   XmNcursorPosition, cursor,
		   XmNwordWrap, new_wrap,
		   0);
    if (body) free(body);

    /* Put this callback back so that the "you haven't typed anything do
       you really want to send?" hack still works.
       #### cloned from dialogs.c (FE_InitializeMailCompositionContext)
     */
    XtAddCallback (text, XmNmodifyVerifyCallback,
		   fe_mail_text_modify_cb, context);

    CONTEXT_DATA(context)->mcBodyText = text;
    XtManageChild(text);

    /* just in case it didn't take... */
    XtVaSetValues (text, XmNcursorPosition, cursor, 0);

    /* Move focus back there. */
    XmProcessTraversal (text, XmTRAVERSE_CURRENT);
  }
}


/* I don't pretend to understand this. */
#undef cpp_stringify_noop_helper
#undef cpp_stringify
#define cpp_stringify_noop_helper(x)#x
#define cpp_stringify(x) cpp_stringify_noop_helper(x)


#define DEFMNACTION(NAME) \
static void \
fe_##NAME##_action (Widget widget, XEvent *event, String *av, Cardinal *ac) \
{  \
  MWContext *context = fe_WidgetToMWContext (widget); \
  XP_ASSERT (context); \
  if (!context) return; \
  fe_msgDoCommand(context, fe_widget_name_to_command(cpp_stringify(NAME))); \
}

DEFMNACTION(newFolder)
DEFMNACTION(compressAllFolders)
DEFMNACTION(openNewsHost)
DEFMNACTION(addNewsgroup)
DEFMNACTION(emptyTrash)
DEFMNACTION(deleteMessage)
DEFMNACTION(deleteFolder)
DEFMNACTION(cancelMessage)
DEFMNACTION(deleteNewsHost)
DEFMNACTION(selectMarkedMessages)
DEFMNACTION(unselectAllMessages)
DEFMNACTION(markSelectedMessages)
DEFMNACTION(unmarkSelectedMessages)
DEFMNACTION(reSort)
DEFMNACTION(threadMessages)
DEFMNACTION(sortBackward)
DEFMNACTION(sortByDate)
DEFMNACTION(sortBySubject)
DEFMNACTION(sortBySender)
DEFMNACTION(sortByMessageNumber)
DEFMNACTION(rot13Message)
DEFMNACTION(addFromNewest)
DEFMNACTION(addFromOldest)
DEFMNACTION(getMoreMessages)
DEFMNACTION(getAllMessages)
DEFMNACTION(attachmentsInline)
DEFMNACTION(attachmentsAsLinks)
DEFMNACTION(editAddressBook)
DEFMNACTION(editAddress)
DEFMNACTION(postNew)
DEFMNACTION(postReply)
DEFMNACTION(postAndMailReply)
DEFMNACTION(forwardMessageQuoted)
DEFMNACTION(markSelectedRead)
DEFMNACTION(markSelectedUnread)
DEFMNACTION(unmarkAllMessages)
DEFMNACTION(copyMessagesInto)
DEFMNACTION(moveMessagesInto)
DEFMNACTION(firstMessage)
DEFMNACTION(lastMessage)
DEFMNACTION(firstUnreadMessage)
DEFMNACTION(lastUnreadMessage)
DEFMNACTION(firstMarkedMessage)
DEFMNACTION(lastMarkedMessage)
DEFMNACTION(markThreadRead)
DEFMNACTION(markAllRead)
DEFMNACTION(showSubscribedNewsGroups)
DEFMNACTION(showActiveNewsGroups)
DEFMNACTION(showAllNewsGroups)
DEFMNACTION(checkNewNewsGroups)
DEFMNACTION(showAllMessages)
DEFMNACTION(showOnlyUnread)
DEFMNACTION(showMicroHeaders)
DEFMNACTION(showSomeHeaders)
DEFMNACTION(showAllHeaders)
DEFMNACTION(attachFile)
DEFMNACTION(quoteMessage)
DEFMNACTION(showFrom)
DEFMNACTION(showReplyTo)
DEFMNACTION(showTo)
DEFMNACTION(showCC)
DEFMNACTION(showBCC)
DEFMNACTION(showFCC)
DEFMNACTION(showPostTo)
DEFMNACTION(showFollowupTo)
DEFMNACTION(showSubject)
DEFMNACTION(showAttachments)
DEFMNACTION(subscribeGroup)
DEFMNACTION(unsubscribeGroup)
DEFMNACTION(compressFolder)
DEFMNACTION(getNewMail)
DEFMNACTION(deliverQueuedMessages)
DEFMNACTION(replyToSender)
DEFMNACTION(replyToAll)
DEFMNACTION(forwardMessage)
DEFMNACTION(selectAllMessages)
DEFMNACTION(selectThread)
DEFMNACTION(previousMessage)
DEFMNACTION(nextMessage)
DEFMNACTION(previousUnreadMessage)
DEFMNACTION(nextUnreadMessage)
DEFMNACTION(previousMarkedMessage)
DEFMNACTION(nextMarkedMessage)
DEFMNACTION(saveMessagesAs)
DEFMNACTION(openFolder)
DEFMNACTION(sendMessage)

XtActionsRec fe_MailNewsActions [] =
{
  { "mailNewsPopup", fe_mailNewsPopup_action },

  { "getNewMail", fe_getNewMail_action },
  { "deliverQueuedMessages", fe_deliverQueuedMessages_action },
  { "openFolder", fe_openFolder_action },
  { "newFolder", fe_newFolder_action },
  { "compressFolder", fe_compressFolder_action },
  { "compressAllFolders", fe_compressAllFolders_action },
  { "openNewsHost", fe_openNewsHost_action },
  { "addNewsgroup", fe_addNewsgroup_action },
  { "emptyTrash", fe_emptyTrash_action },
  { "deleteMessage", fe_deleteMessage_action },
  { "deleteFolder", fe_deleteFolder_action },
  { "cancelMessage", fe_cancelMessage_action },
  { "deleteNewsHost", fe_deleteNewsHost_action },
  { "selectThread", fe_selectThread_action },
  { "selectMarkedMessages", fe_selectMarkedMessages_action },
  { "selectAllMessages", fe_selectAllMessages_action },
  { "unselectAllMessages", fe_unselectAllMessages_action },
  { "markSelectedMessages", fe_markSelectedMessages_action },
  { "unmarkSelectedMessages", fe_unmarkSelectedMessages_action },
  { "reSort", fe_reSort_action },
  { "threadMessages", fe_threadMessages_action },
  { "sortBackward", fe_sortBackward_action },
  { "sortByDate", fe_sortByDate_action },
  { "sortBySubject", fe_sortBySubject_action },
  { "sortBySender", fe_sortBySender_action },
  { "sortByMessageNumber", fe_sortByMessageNumber_action },
  { "rot13Message", fe_rot13Message_action },
  { "addFromNewest", fe_addFromNewest_action },
  { "addFromOldest", fe_addFromOldest_action },
  { "getMoreMessages", fe_getMoreMessages_action },
  { "getAllMessages", fe_getAllMessages_action },
  { "attachmentsInline", fe_attachmentsInline_action },
  { "attachmentsAsLinks", fe_attachmentsAsLinks_action },
  { "editAddressBook", fe_editAddressBook_action },
  { "editAddress", fe_editAddress_action },
  { "postNew", fe_postNew_action },
  { "postReply", fe_postReply_action },
  { "postAndMailReply", fe_postAndMailReply_action },
  { "replyToSender", fe_replyToSender_action },
  { "replyToAll", fe_replyToAll_action },
  { "forwardMessage", fe_forwardMessage_action },
  { "forwardMessageQuoted", fe_forwardMessageQuoted_action },
  { "markSelectedRead", fe_markSelectedRead_action },
  { "markSelectedUnread", fe_markSelectedUnread_action },
  { "unmarkAllMessages", fe_unmarkAllMessages_action },
  { "copyMessagesInto", fe_copyMessagesInto_action },
  { "moveMessagesInto", fe_moveMessagesInto_action },
  { "saveMessagesAs", fe_saveMessagesAs_action },
  { "firstMessage", fe_firstMessage_action },
  { "nextMessage", fe_nextMessage_action },
  { "previousMessage", fe_previousMessage_action },
  { "lastMessage", fe_lastMessage_action },
  { "firstUnreadMessage", fe_firstUnreadMessage_action },
  { "nextUnreadMessage", fe_nextUnreadMessage_action },
  { "previousUnreadMessage", fe_previousUnreadMessage_action },
  { "lastUnreadMessage", fe_lastUnreadMessage_action },
  { "firstMarkedMessage", fe_firstMarkedMessage_action },
  { "nextMarkedMessage", fe_nextMarkedMessage_action },
  { "previousMarkedMessage", fe_previousMarkedMessage_action },
  { "lastMarkedMessage", fe_lastMarkedMessage_action },
  { "markThreadRead", fe_markThreadRead_action },
  { "markAllRead", fe_markAllRead_action },
  { "showSubscribedNewsGroups", fe_showSubscribedNewsGroups_action },
  { "showActiveNewsGroups", fe_showActiveNewsGroups_action },
  { "showAllNewsGroups", fe_showAllNewsGroups_action },
  { "checkNewNewsGroups", fe_checkNewNewsGroups_action },
  { "showAllMessages", fe_showAllMessages_action },
  { "showOnlyUnread", fe_showOnlyUnread_action },
  { "showMicroHeaders", fe_showMicroHeaders_action },
  { "showSomeHeaders", fe_showSomeHeaders_action },
  { "showAllHeaders", fe_showAllHeaders_action },
  { "sendMessage", fe_sendMessage_action },
  { "attachFile", fe_attachFile_action },
  { "quoteMessage", fe_quoteMessage_action },
  { "showFrom", fe_showFrom_action },
  { "showReplyTo", fe_showReplyTo_action },
  { "showTo", fe_showTo_action },
  { "showCC", fe_showCC_action },
  { "showBCC", fe_showBCC_action },
  { "showFCC", fe_showFCC_action },
  { "showPostTo", fe_showPostTo_action },
  { "showFollowupTo", fe_showFollowupTo_action },
  { "showSubject", fe_showSubject_action },
  { "showAttachments", fe_showAttachments_action },
  { "subscribeGroup", fe_subscribeGroup_action },
  { "unsubscribeGroup", fe_unsubscribeGroup_action },

  /* Gross aliases that must exist for not-very-good reasons... */
  { "reply", fe_replyToSender_action },
  { "replyAll", fe_replyToAll_action },
  { "selectAll", fe_selectAllMessages_action },
  { "selectPreviousMessage", fe_previousMessage_action },
  { "selectNextMessage", fe_nextMessage_action },
  { "selectPreviousUnreadMessage", fe_previousUnreadMessage_action },
  { "selectNextUnreadMessage", fe_nextUnreadMessage_action },
  { "selectPreviousMarkedMessage", fe_previousMarkedMessage_action },
  { "selectNextMarkedMessage", fe_nextMarkedMessage_action },
  { "saveMessageAs", fe_saveMessagesAs_action },
  { "sendMail", fe_sendMessage_action }
};
int fe_MailNewsActionsSize = countof (fe_MailNewsActions);
