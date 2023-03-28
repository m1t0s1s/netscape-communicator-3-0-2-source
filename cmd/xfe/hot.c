/* -*- Mode:C; tab-width: 8 -*-
   hot.c --- bookmarks dialogs
   Copyright © 1996 Netscape Communications Corporation, all rights reserved.
   Created: Terry Weissman <terry@netscape.com>, 20-Jul-95.

   Modified: dpSuresh <dp@netscape.com>, 19 Sep 1995
		- Implemented addressbooks.
 */

#include "mozilla.h"
#include "xfe.h"
#include "menu.h"
#include "outline.h"
#include "felocale.h"
#ifdef EDITOR
#include "xeditor.h"
#endif

/* Kludge around conflicts between Motif and xp_core.h... */
#undef Bool
#define Bool char

#include "bkmks.h"

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>


/* for XP_GetString() */
#include <xpgetstr.h>
extern int XFE_BM_ALIAS;
extern int XFE_ESTIMATED_TIME_REMAINING;
extern int XFE_ESTIMATED_TIME_REMAINING_CHECKED;
extern int XFE_ESTIMATED_TIME_REMAINING_CHECKING;
extern int XFE_DONE_CHECKING_ETC;
extern int XFE_GG_EMPTY_LL;
extern int XFE_LOOK_FOR_DOCUMENTS_THAT_HAVE_CHANGED_ON;


extern void
fe_attach_field_to_labels (Widget value_field, ...);

/* Forward declaration */
void BMFE_FreeClipContents(MWContext *bm_context);
static void fe_find_setvalues(MWContext* bm_context, BM_FindInfo* findInfo);
static void fe_make_find_dialog(MWContext* bm_context, BM_FindInfo* findInfo);
static void fe_bmfind_destroy_cb(Widget widget, XtPointer closure,
					XtPointer call_data);
static void fe_make_bmpopup(MWContext* bm_context);
static void fe_bmpopup_menu_action (Widget, XEvent *, String *, Cardinal *);
static void fe_bmcut_action (Widget, XEvent *, String *, Cardinal *);
static void fe_bmcopy_action (Widget, XEvent *, String *, Cardinal *);
static void fe_bmpaste_action (Widget, XEvent *, String *, Cardinal *);
static void fe_bmfind_action (Widget, XEvent *, String *, Cardinal *);
static void fe_bmfindAgain_action (Widget, XEvent *, String *, Cardinal *);
static void fe_bmundo_action (Widget, XEvent *, String *, Cardinal *);
static void fe_bmredo_action (Widget, XEvent *, String *, Cardinal *);
static void fe_bmdeleteItem_action (Widget, XEvent *, String *, Cardinal *);
static void fe_bmmailMessage_action (Widget, XEvent *, String *, Cardinal *);
static void fe_bmsaveAs_action (Widget, XEvent *, String *, Cardinal *);
static void fe_bmclose_action (Widget, XEvent *, String *, Cardinal *);


typedef struct fe_bookmark_clipboard {
  void *block;
  int32 length;
} fe_bookmark_clipboard;

typedef struct fe_bookmark_data
{
  Widget outline;

  Widget editshell;		/* For the edititem window. */
  Widget title;
  Widget nickname;
  Widget name;
  Widget location;		/* doubles as email address for addressbook */
  Widget locationlabel;
  Widget description;
  Widget lastvisited;
  Widget lastvisitedlabel;
  Widget addedon;
  Widget aliaslabel;
  Widget aliasbutton;

  BM_Entry* entry;		/* Entry being edited. */

  fe_bookmark_clipboard clip;	/* Bookmarks own private clipboard */

  Widget findshell;           /* Find stuff */
  Widget findtext;
  Widget findnicknameT;
  Widget findnameT;
  Widget findlocationT;
  Widget finddescriptionT;
  Widget findcaseT;
  Widget findwordT;

  Widget whatshell;		/* What's Changed stuff */
  Widget whattext;		/* Progress message area. */
  Widget whatradiobox;		/* Box containing below radio buttons. */
  Widget whatdoall;		/* "All bookmarks" radio button. */
  Widget whatdoselected;	/* "Selected bookmarks" radio button. */
  Widget whatstart;		/* Start button. */
  Widget whatcancel;		/* Cancel button. */
  

  Widget popup;			/* Bookmark popup menu */
} fe_bookmark_data;


static MWContext* main_bm_context = NULL;
static MWContext* addressbook_context = NULL;

#define BM_DATA(context) (CONTEXT_DATA(context)->bmdata)

static XtActionsRec fe_bookmarkActions [] =
{
  { "BookmarkPopup",		fe_bmpopup_menu_action	},
  { "bmcut",			fe_bmcut_action		},
  { "bmcopy",			fe_bmcopy_action	},
  { "bmpaste",			fe_bmpaste_action	},
  { "bmfind",			fe_bmfind_action	},
  { "bmfindAgain",		fe_bmfindAgain_action	},
  { "bmundo",			fe_bmundo_action	},
  { "bmredo",			fe_bmredo_action	},
  { "bmdeleteItem",		fe_bmdeleteItem_action	},
  { "bmmailMessage",		fe_bmmailMessage_action	},
  { "bmsaveAs",			fe_bmsaveAs_action	},
  { "bmclose",			fe_bmclose_action	},
};


static void
fe_bm_pulldown_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  Widget menu = 0;
  Widget *buttons = 0;
  Cardinal nbuttons = 0;
  int i;
  XtVaGetValues(widget, XmNsubMenuId, &menu, 0);
  XtVaGetValues(menu, XmNchildren, &buttons, XmNnumChildren, &nbuttons, 0);
  for (i=0 ; i<nbuttons ; i++) {
    Widget item = buttons[i];
    if (XmIsPushButton(item) || XmIsPushButtonGadget(item)) {
      XtPointer value;
      BM_CommandType cmd;
      XtVaGetValues(item, XmNuserData, &value, 0);
      cmd = (BM_CommandType) value;
      if (cmd != BM_Cmd_Invalid) {
	XP_Bool enable = BM_FindCommandStatus(bm_context, cmd);
	XtVaSetValues(item, XmNsensitive, enable, 0);
      }
    }
  }
}


static Boolean
fe_bm_datafunc(Widget widget, void* closure, int row, fe_OutlineDesc* data)
{
  static char *separator_string = "--------------------";
  static char string[128];
  MWContext* bm_context = (MWContext*) closure;
  BM_Entry* entry = BM_AtIndex(bm_context, row + 1);
  if (!entry) return False;
  data->depth = BM_GetDepth(bm_context, entry);

  data->tag = BM_IsAlias(entry) ? "ITALIC" : XmFONTLIST_DEFAULT_TAG;

  if (BM_IsHeader(entry)) {
    data->flippy = BM_IsFolded(entry) ? MSG_Folded : MSG_Expanded;
    if (bm_context->type == MWContextAddressBook) {
      data->icon =
	BM_IsFolded(entry) ? FE_OUTLINE_ClosedList : FE_OUTLINE_OpenedList;
    } else {
      static fe_OutlineType list[] = {
	FE_OUTLINE_ClosedHeader,
	FE_OUTLINE_OpenedHeader,
	FE_OUTLINE_ClosedHeaderDest,
	FE_OUTLINE_OpenedHeaderDest,
	FE_OUTLINE_ClosedHeaderMenu,
	FE_OUTLINE_OpenedHeaderMenu,
	FE_OUTLINE_ClosedHeaderMenuDest,
	FE_OUTLINE_OpenedHeaderMenuDest
      };
      int key = 0;
      if (!BM_IsFolded(entry)) key++;
      if (entry == BM_GetAddHeader(bm_context)) key += 2;
      if (entry == BM_GetMenuHeader(bm_context)) key += 4;
      data->icon = list[key];
    }
  } else {
    data->flippy = MSG_Leaf;
    if (BM_IsAlias(entry) && BM_IsHeader(BM_GetAliasOriginal(entry))) {
      data->icon = (bm_context->type == MWContextBookmarks ?
		    FE_OUTLINE_ClosedHeader : FE_OUTLINE_ClosedList);
    } else {
      if (bm_context->type == MWContextAddressBook) {
	data->icon = FE_OUTLINE_Address;
      } else {
	switch (BM_GetChangedState(entry)) {
	case BM_CHANGED_UNKNOWN:
	  data->icon = FE_OUTLINE_UnknownBookmark;
	  break;
	case BM_CHANGED_YES:
	  data->icon = FE_OUTLINE_ChangedBookmark;
	  break;
	case BM_CHANGED_NO:
	  data->icon = FE_OUTLINE_Bookmark;
	  break;
	default:
	  XP_ASSERT(0);
	  break;
	}
      }
    }
  }

  /* Handle selections */
  if (BM_IsSelected(entry)) {
    fe_OutlineSelect(BM_DATA(bm_context)->outline, row, False);
  } else {
    fe_OutlineUnselect(BM_DATA(bm_context)->outline, row);
  }

  data->type[0] = FE_OUTLINE_String;

  if (BM_IsSeparator(entry))
    data->str[0] = separator_string;
  else
    {
      char *name;
      unsigned char *loc;

      name = BM_GetName(entry);
      loc = fe_ConvertToLocaleEncoding(INTL_DefaultWinCharSetID(NULL),
					(unsigned char *) name);
      PR_snprintf(string, sizeof(string), "%s", loc);
      if ((char *) loc != name)
	{
	  XP_FREE(loc);
	}
      data->str[0] = string;
    }
  return True;
}

static void
fe_bm_dropfunc(Widget dropw, void* closure, fe_dnd_Event type,
	       fe_dnd_Source* source, XEvent* event)
{
  MWContext* bm_context = (MWContext*) closure;
  int row;
  XP_Bool under = FALSE;
  

  if (source->type != FE_DND_BOOKMARK) return;

  if (source->closure != BM_DATA(bm_context)->outline) {
    /* Right now, we only handle drops that originated from us.  This should
       change eventually... ### */
    return;
  }

  fe_OutlineHandleDragEvent(BM_DATA(bm_context)->outline, event, type, source);

  row = fe_OutlineRootCoordsToRow(BM_DATA(bm_context)->outline,
				  event->xbutton.x_root,
				  event->xbutton.y_root,
				  &under);

  fe_OutlineSetDragFeedback(BM_DATA(bm_context)->outline, row,
			    BM_IsDragEffectBox(bm_context, row + 1, under));

  if (type == FE_DND_DROP && row >= 0) {
    BM_DoDrop(bm_context, row + 1, under);
  }
  if (type == FE_DND_END) {
    fe_OutlineSetDragFeedback(BM_DATA(bm_context)->outline, -1, FALSE);
  }
  return;
}


static void
fe_bm_clickfunc(Widget widget, void* closure, int row, int column,
		char* header, int button, int clicks,
		Boolean shift, Boolean ctrl)
{
  MWContext* bm_context = (MWContext*) closure;
  BM_Entry* entry = BM_AtIndex(bm_context, row + 1);
  if (!entry) return;
  if (ctrl) {
    BM_ToggleItem(bm_context, entry, TRUE, TRUE);
  } else if (shift) {
    BM_SelectRangeTo(bm_context, entry);
  } else {
    BM_SelectItem(bm_context, entry, TRUE, FALSE, TRUE);
  }
  if (clicks == 2) {
    if (bm_context->type == MWContextAddressBook) {
      URL_Struct *url;
      char *str, *str2, *str3;

      /* set the address string and launch composition window */
      str = BM_GetFullAddress (bm_context, entry);
      if (!str) return;
      str2 = NET_Escape(str, URL_XALPHAS);
      XP_FREE(str);
      if (!str2) return;
      str3 = PR_smprintf("mailto:%s", str2);
      XP_FREE(str2);
      if (!str3) return;
      url = NET_CreateURLStruct (str3, False);
      if (url)
	fe_GetURL (bm_context, url, FALSE);
      XP_FREE (str3);
    } else if (BM_IsAlias(entry) || BM_IsUrl(entry)) {
      BM_GotoBookmark(bm_context, entry);
    }
  }
}


static void
fe_bm_iconfunc(Widget widget, void* closure, int row)
{
  MWContext* bm_context = (MWContext*) closure;
  BM_Entry* entry = BM_AtIndex(bm_context, row + 1);
  if (entry && BM_IsHeader(entry)) {
    if (!BM_IsFolded(entry)) {
      /* While closing an open folder, take care of clearing all its
       * child selections
       */
      BM_ClearAllChildSelection(bm_context, entry, TRUE);
    }
    BM_FoldHeader(bm_context, entry, !BM_IsFolded(entry), TRUE, FALSE);
  }
}

/* This is for dragging from addressbook and dropping onto compose window */
#define XFE_CHUNK_SIZE 1024
static void
fe_collect_addresses(MWContext *context, BM_Entry *entry, void *closure)
{
  char **newstrp = (char **)closure;
  int oldlen = XP_STRLEN(*newstrp), nchunk, newlen;
  char *str = BM_GetFullAddress(context, entry);
  if (str) {
    nchunk = oldlen / XFE_CHUNK_SIZE;
    nchunk++;
    newlen = oldlen + XP_STRLEN(str) + 2;	/* 2 for ", " */
    if (newlen+1 > nchunk*XFE_CHUNK_SIZE) {
      char *s = (char *) XP_REALLOC(*newstrp, (nchunk+1)*XFE_CHUNK_SIZE);
      if (!s)	/* realloc failed */
	return;
      *newstrp = s;
    }
    if (oldlen > 0) XP_STRCAT(*newstrp, ", ");
    XP_STRCAT(*newstrp, str);
    XP_FREE(str);
  }
}

extern void
fe_attach_dropfunc(Widget dropw, void* closure, fe_dnd_Event type,
		   fe_dnd_Source* source, XEvent* event);


/* Mail composition drop function for addressbook drag */
void
fe_mc_dropfunc(Widget dropw, void* closure, fe_dnd_Event type,
		  fe_dnd_Source* source, XEvent* event)
{
  MWContext* bm_context;

  if (source->type == FE_DND_MAIL_MESSAGE ||
      source->type == FE_DND_NEWS_MESSAGE /*||  ##### fix me!
      source->type == FE_DND_BOOKMARK*/)
    {
      /* Wow, how's this for object fucking oriented?  -- jwz. */
      fe_attach_dropfunc (dropw, closure, type, source, event);
      return;
    }

  bm_context = FE_GetAddressBook(NULL, False);

  /* Not our drag as an addressbook context doesn't even exist */
  if (!bm_context) return;

  if ((source->type != FE_DND_BOOKMARK)
      || (source->closure != BM_DATA(bm_context)->outline)) return;

  if (type == FE_DND_DROP) {
    char *oldstr = XmTextGetString(dropw);
    int oldlen = oldstr ? XP_STRLEN(oldstr) : 0;
    char *newstr = (char *)
      XP_ALLOC(XFE_CHUNK_SIZE * ((oldlen / XFE_CHUNK_SIZE) + 1));
    newstr[0] = '\0';
    if (oldstr && *oldstr) {
      XP_STRCPY(newstr, oldstr);
    }
    BM_EachProperSelectedEntryDo(bm_context, fe_collect_addresses, &newstr,
				 NULL);
    XmTextSetString(dropw, newstr);
    /* Expand the nickname instantly */
    XtCallCallbacks(dropw, XmNlosingFocusCallback, NULL);
    if (oldstr) XtFree(oldstr);
    XP_FREE(newstr);
  }

  return;
}


static void
fe_bmedit_selectalias_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  fe_bookmark_data* d = BM_DATA(bm_context);
  if (d->entry)
    BM_SelectAliases(bm_context, d->entry);
}


static void
fe_bmedit_destroy(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  fe_bookmark_data* d = BM_DATA(bm_context);
  if (d && d->editshell) {
    d->editshell = NULL;
  }
}

void
BMFE_BookmarkMenuInvalid (MWContext* bm_context)
{
  struct fe_MWContext_cons *rest;
  for (rest = fe_all_MWContexts; rest; rest = rest->next) {
    CONTEXT_DATA (rest->context)->bookmark_menu_up_to_date_p = False;
  }
}

static char *
fe_bmedit_get_and_clean_text(MWContext *bm_context, Widget widget,
	Boolean new_lines_too_p)
{
  char	*str;

  str = fe_GetTextField(widget);
  if (!str) {
    return NULL;
  }
  fe_forms_clean_text(bm_context, INTL_DefaultWinCharSetID(NULL), str,
			new_lines_too_p);

  return str;
}

static XP_Bool
fe_bmedit_commit_changes(MWContext* bm_context)
{
  fe_bookmark_data* d = BM_DATA(bm_context);
  char* ptr = 0;

  if (!d->entry) return(True);
  if (bm_context->type == MWContextAddressBook) {
    ptr = fe_bmedit_get_and_clean_text(bm_context, d->nickname, True);
    if (!ptr) {
	return(False);
    }
    if (!BM_SetNickName(bm_context, d->entry, ptr))
	return(False);
    XP_FREE(ptr);
  }
  ptr = fe_bmedit_get_and_clean_text(bm_context, d->name, True);
  if (!ptr) {
    return(False);
  }
  BM_SetName(bm_context, d->entry, ptr);
  XP_FREE(ptr);

  ptr = fe_bmedit_get_and_clean_text(bm_context, d->location, True);
  if (!ptr) {
    return(False);
  }
  BM_SetAddress(bm_context, d->entry, ptr);
  XP_FREE(ptr);

  ptr = fe_bmedit_get_and_clean_text(bm_context, d->description, False);
  if (!ptr) {
    return(False);
  }
  BM_SetDescription(bm_context, d->entry, ptr);
  XP_FREE(ptr);

  return(True);
}



static void
fe_bmedit_close(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  BM_CancelEdit(bm_context, BM_DATA(bm_context)->entry);
  BM_DATA(bm_context)->entry = NULL;
  BM_SaveBookmarks(bm_context, NULL);
  XtUnmanageChild(BM_DATA(bm_context)->editshell);
}

static void
fe_bmedit_ok(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  if (fe_bmedit_commit_changes(bm_context))
    fe_bmedit_close(widget, closure, call_data);
}

static void
fe_bm_close(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  XtPopdown(CONTEXT_WIDGET(bm_context));
}

static void
fe_bm_popdown(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  BM_SaveBookmarks(bm_context, NULL);
  /* While closing the bookmarks window, close the properties. If not
     it never comes up again. */
  if (BM_DATA(bm_context)->editshell)
    fe_bmedit_close(widget, closure, call_data);
}

#if 0
static void
fe_bm_destroy(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  fe_bookmark_data* d = BM_DATA(bm_context);
  BM_CleanupBookmarksContext(bm_context);
  fe_bmedit_destroy(widget, closure, call_data);
  fe_bmfind_destroy_cb(widget, closure, call_data);
  XP_FREE(d);
}
#endif

static void
fe_bookmark_menu_cb (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  BM_Entry* entry = 0;
  URL_Struct *url;
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNuserData, &entry, 0);
  if (!entry || !(BM_IsAlias(entry) || BM_IsUrl(entry)))
    return;
  url = NET_CreateURLStruct (BM_GetAddress(entry), FALSE);
  /* item->last_visit = time ((time_t *) 0); ### */
  /* fe_BookmarkAccessTimeChanged (context, item); ### */
  fe_GetURL (context, url, FALSE);
}

static void
fe_bookmark_menu_arm(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  BM_Entry* entry = 0;
  fe_UserActivity (context);
  XtVaGetValues (widget, XmNuserData, &entry, 0);
  if (!entry || !(BM_IsAlias(entry) || BM_IsUrl(entry)))
    return;
  XFE_Progress(context, BM_GetAddress(entry));
}

static void
fe_bookmark_menu_disarm(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  XFE_Progress(context, "");
}

#if 0
static void
fe_addheader_to_popup(MWContext*bm_context, BM_Entry* entry, void* closure )
{
  Widget* warr = (Widget *) closure;
  Widget tmpw;
  Arg av [5];
  int ac;
  char *s;
  XmString xmstr;

  if (!entry || !BM_IsHeader(entry)) return;

  s = BM_GetName(entry);
  if (!s) return;
  xmstr = XmStringCreateLtoR (s, XmFONTLIST_DEFAULT_TAG);

  ac = 0;
  XtSetArg (av[ac], XmNlabelString, xmstr); ac++;
  tmpw = XmCreatePushButtonGadget (warr[0], "addbm_headers", av, ac);
  XtManageChild(tmpw);
  tmpw = XmCreatePushButtonGadget (warr[1], "bmmenu_headers", av, ac);
  XtManageChild(tmpw);
}
#endif

static void
fe_bm_cmd(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  XtPointer userdata = 0;
  BM_CommandType cmd;
  XtVaGetValues(widget, XmNuserData, &userdata, 0);
  cmd = (BM_CommandType) userdata;
  BM_ObeyCommand(bm_context, cmd);
}


static void
XtUnmanageChild_safe (Widget w)
{
  if (w) XtUnmanageChild (w);
}

static void
fe_bmwhat_start(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  fe_bookmark_data* d = BM_DATA(bm_context);
  Boolean doselected = FALSE;
  XtVaGetValues(d->whatdoselected, XmNset, &doselected, NULL);
  BM_StartWhatsChanged(bm_context, doselected);
}

static void
fe_bmwhat_close(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  fe_bookmark_data* d = BM_DATA(bm_context);
  if (d->whatshell) {
    Widget shell = d->whatshell;
    d->whatshell = NULL;	/* Careful recursion prevention. */
    XtRemoveCallback (shell, XmNdestroyCallback, fe_bmwhat_close, bm_context);
    XtDestroyWidget(shell);
    BM_CancelWhatsChanged(bm_context);
  }
}

static void
fe_bmwhat_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;

  if (cb->reason == XmCR_OK)
    fe_bmwhat_start(widget, closure, call_data);
  else if (cb->reason == XmCR_CANCEL)
    fe_bmwhat_close(widget, closure, call_data);
}

static void
fe_make_what_dialog (MWContext *bm_context)
{
  fe_bookmark_data* d = BM_DATA(bm_context);

  int ac;
  Arg av[20];
  int nk;
  Widget* k;
  XmString str;

  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  str = XmStringCreate("", XmFONTLIST_DEFAULT_TAG);

  XtVaGetValues(CONTEXT_WIDGET(bm_context), XtNvisual, &v,
		XtNcolormap, &cmap, XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmDESTROY); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  XtSetArg (av[ac], XmNresizePolicy, XmRESIZE_GROW); ac++;
  XtSetArg (av[ac], XmNmessageString, str); ac++;
  XtSetArg (av[ac], XmNchildPlacement, XmPLACE_BELOW_SELECTION); ac++;

  d->whatshell = XmCreatePromptDialog(CONTEXT_WIDGET(bm_context),
					"bookmarksWhatsChanged", av, ac);
  XmStringFree(str);

  XtUnmanageChild_safe (XmSelectionBoxGetChild (d->whatshell,
						XmDIALOG_TEXT));
  XtUnmanageChild_safe (XmSelectionBoxGetChild (d->whatshell,
						XmDIALOG_APPLY_BUTTON));
  XtUnmanageChild_safe (XmSelectionBoxGetChild (d->whatshell,
						XmDIALOG_HELP_BUTTON));

  d->whattext = XmSelectionBoxGetChild (d->whatshell, XmDIALOG_SELECTION_LABEL);
  d->whatstart = XmSelectionBoxGetChild (d->whatshell, XmDIALOG_OK_BUTTON);
  d->whatcancel = XmSelectionBoxGetChild (d->whatshell, XmDIALOG_CANCEL_BUTTON);

  XtAddCallback(d->whatshell, XmNdestroyCallback, fe_bmwhat_close, bm_context);
  XtAddCallback(d->whatshell, XmNokCallback, fe_bmwhat_cb, bm_context);
  XtAddCallback(d->whatshell, XmNcancelCallback, fe_bmwhat_cb, bm_context);

  d->whatradiobox =
    XmVaCreateSimpleRadioBox(d->whatshell, "radiobox", 0, NULL,
			     XmVaRADIOBUTTON, NULL, NULL, NULL, NULL,
			     XmVaRADIOBUTTON, NULL, NULL, NULL, NULL,
			     NULL);
  XtVaGetValues(d->whatradiobox, XmNnumChildren, &nk, XmNchildren, &k, 0);
  XP_ASSERT(nk == 2);
  d->whatdoall = k[0];
  d->whatdoselected = k[1];

  XtManageChildren(k, nk);

  XtManageChild(d->whatradiobox);
}


void
BMFE_UpdateWhatsChanged(MWContext* bm_context, const char* url,
			int32 done, int32 total,
			const char* totaltime)
{
  fe_bookmark_data* d = BM_DATA(bm_context);
  char buf[1024];
  XmString str;

  if (!d->whatshell) return;

  if (d->whatradiobox) {
    XtUnmanageChild_safe(d->whatradiobox);
    XtUnmanageChild_safe(d->whatstart);
    d->whatradiobox = NULL;
  }

  if ( url )
  PR_snprintf(buf, sizeof(buf),
         XP_GetString( XFE_ESTIMATED_TIME_REMAINING_CHECKED ),
	      url,
	      total - done,
	      total > 0 ? done * 100 / total : 0,
	      totaltime);
  else
  PR_snprintf(buf, sizeof(buf),
         XP_GetString( XFE_ESTIMATED_TIME_REMAINING_CHECKING ),
	      total - done,
	      total > 0 ? done * 100 / total : 0,
	      totaltime);

  str = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
  XtVaSetValues(d->whattext, XmNlabelString, str, NULL);
  XmStringFree(str);
  
}

void BMFE_FinishedWhatsChanged(MWContext* bm_context, int32 totalchecked,
			       int32 numreached, int32 numchanged)
{
  fe_bookmark_data* d = BM_DATA(bm_context);
  char buf[1024];
  XmString str;
  XtUnmanageChild_safe(d->whatradiobox);
  XtUnmanageChild_safe(d->whatstart);
  PR_snprintf(buf, sizeof(buf), XP_GetString( XFE_DONE_CHECKING_ETC ),
	      totalchecked, numreached, numchanged);

  str = XmStringCreateLtoR(buf, XmFONTLIST_DEFAULT_TAG);
  XtVaSetValues(d->whattext, XmNlabelString, str, NULL);
  XmStringFree(str);
}


static void
fe_bm_whatschanged(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  fe_bookmark_data* d = BM_DATA(bm_context);
  XmString str;
  fe_bmwhat_close(widget, closure, call_data);
  XP_ASSERT(!d->whatshell);
  if (!d->whatshell) {
    fe_make_what_dialog(bm_context);

    str = XmStringCreate( XP_GetString( XFE_LOOK_FOR_DOCUMENTS_THAT_HAVE_CHANGED_ON ), 
			 XmFONTLIST_DEFAULT_TAG); 
    XtVaSetValues(d->whattext, XmNlabelString, str, NULL);
    XmStringFree(str);
  }

/* On Sun: This sucks!
 * PromptDialog will not appear as its desired size if some of its children
 * are unmanaged.
 * Need to Realize the widget first to readjust the geometry before
 * this widget was managed.
 */
#if defined(__sun)
  XtRealizeWidget(d->whatshell);
#endif

  XtManageChild(d->whatshell);
  XMapRaised(XtDisplay(d->whatshell), XtWindow(d->whatshell));
}



  
static struct fe_button bookmarkPopupMenudesc[] = {
  { "properties",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_BookmarkProps },
  { "gotoBookmark",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_GotoBookmark},
  { "",			0,			UNSELECTABLE,	0 },
  { "sortBookmarks",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_SortBookmarks},
  { "",			0,			UNSELECTABLE,	0 },
  { "insertBookmark",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_InsertBookmark},
  { "insertHeader",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_InsertHeader},
  { "insertSeparator",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_InsertSeparator},
  { "",			0,			UNSELECTABLE,	0 },
  { "makeAlias",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_MakeAlias},
  { 0 },
};

static struct fe_button bookmarkMenudesc[] = {
  { "file",		0,			SELECTABLE,	0 },
  { "open",	fe_bm_cmd,			SELECTABLE,	0, 0,
    (void*) BM_Cmd_Open},
  { "import",	fe_bm_cmd,			SELECTABLE,	0, 0,
    (void*) BM_Cmd_ImportBookmarks},/**/
  { "saveAs",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_SaveAs},
  { "",			0,			UNSELECTABLE,	0 },
  { "whatsChanged",	fe_bm_whatschanged,	SELECTABLE,	0 },
  { "",			0,			UNSELECTABLE,	0 },
  { "delete",		fe_bm_close,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Invalid },
  { 0 },

  { "edit",		0,			SELECTABLE,	0 },
  { "undo",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Undo},
  { "redo",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Redo},
  { "",			0,			UNSELECTABLE,	0 },
  { "cut",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Cut},
  { "copy",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Copy},
  { "paste",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Paste},
  { "",			0,			UNSELECTABLE,	0 },
  { "deleteItem",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Delete},
  { "",			0,			UNSELECTABLE,	0 },
  { "selectAll",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_SelectAllBookmarks},
  { "",			0,			UNSELECTABLE,	0 },
  { "find",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Find},
  { "findAgain",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_FindAgain},
  { 0 },

  { "Item",		0,			SELECTABLE,	0 },
  { "properties",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_BookmarkProps },
  { "gotoBookmark",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_GotoBookmark},
  { "",			0,			UNSELECTABLE,	0 },
  { "sortBookmarks",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_SortBookmarks},
  { "",			0,			UNSELECTABLE,	0 },
  { "insertBookmark",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_InsertBookmark},
  { "insertHeader",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_InsertHeader},
  { "insertSeparator",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_InsertSeparator},
  { "",			0,			UNSELECTABLE,	0 },
  { "makeAlias",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_MakeAlias},
/* Backend needs to implemente two commands */
  { "newBmFolder",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_SetAddHeader},
  { "bmMenuFolder",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_SetMenuHeader},
  { 0 },
  { 0 }
};

static struct fe_button addressbookPopupMenudesc[] = {
  { "mailMessage",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_GotoBookmark},
  { "properties",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_BookmarkProps },
  { 0 },
};

static struct fe_button addressbookMenudesc[] = {
  { "file",		0,			SELECTABLE,	0 },
  { "mailMessage",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_GotoBookmark},
  { "import",	fe_bm_cmd,			SELECTABLE,	0, 0,
    (void*) BM_Cmd_ImportBookmarks},/**/
  { "saveAs",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_SaveAs},
  { "",			0,			UNSELECTABLE,	0 },
  { "delete",		fe_bm_close,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Invalid },
  { 0 },
  { "edit",		0,			SELECTABLE,	0 },
  { "undo",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Undo},
  { "redo",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Redo},
  { "",			0,			UNSELECTABLE,	0 },
  { "deleteItem",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Delete},
  { "",			0,			UNSELECTABLE,	0 },
  { "selectAll",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_SelectAllBookmarks},
  { "",			0,			UNSELECTABLE,	0 },
  { "find",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_Find},
  { "findAgain",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_FindAgain},
  { 0 },
  { "Item",		0,			SELECTABLE,	0 },
  { "addUser",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_InsertBookmark},
  { "addList",		fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_InsertHeader},
  { "",			0,			UNSELECTABLE,	0 },
  { "properties",	fe_bm_cmd,		SELECTABLE,	0, 0,
    (void*) BM_Cmd_BookmarkProps },
  { 0 },
  { 0 }
};



/*
 * fe_make_bmpopup
 *
 * Makes both the bookmark and addressbook popup depending on context->type
 */
static void
fe_make_bmpopup(MWContext* bm_context)
{
  Widget parent, popup, item;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Arg av[10];
  int ac;
  Widget kids[10];
  struct fe_button *menudesc;
  int nummenu;

  fe_button button;
  int i;

  parent = CONTEXT_WIDGET(bm_context);
  popup = BM_DATA(bm_context)->popup;
  if (popup) return;

  if (bm_context->type == MWContextBookmarks) {
    menudesc = bookmarkPopupMenudesc;
    nummenu = countof(bookmarkPopupMenudesc);
  }
  else if (bm_context->type == MWContextAddressBook) {
    menudesc = addressbookPopupMenudesc;
    nummenu = countof(addressbookPopupMenudesc);
  }
  else
    XP_ASSERT((bm_context->type != MWContextBookmarks) ||
		(bm_context->type != MWContextAddressBook));

  XtVaGetValues (parent, XtNvisual, &v, XtNcolormap, &cmap,
                 XtNdepth, &depth, 0);

  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  popup = XmCreatePopupMenu (parent, "popup", av, ac);

  i = 0;
  ac = 0;
  kids [i++] = XmCreateLabelGadget (popup, "title", av, ac);
  kids [i++] = XmCreateSeparatorGadget (popup, "titleSeparator", av, ac);
  XtManageChildren(kids, i);

  for (i=0; i<nummenu; i++) {
    button = menudesc[i];
    ac = 0;
    if (!button.name) break;
    if (!*button.name)
      item = XmCreateSeparatorGadget (popup, "separator", av, ac);
    else {
      if (button.type == UNSELECTABLE)
	XtSetArg(av[ac], XtNsensitive, False), ac++;
      if (button.userdata)
	XtSetArg(av[ac], XmNuserData, button.userdata); ac++;

      XtSetArg(av[ac], XmNacceleratorText, NULL); ac++;

      item = XmCreatePushButtonGadget (popup, button.name, av, ac);
      if (button.callback)
	XtAddCallback (item, XmNactivateCallback, button.callback, bm_context);
    }
    XtManageChild(item);
  }
  BM_DATA(bm_context)->popup = popup;
}

/*
 * fe_MakeBookmarkWidgets
 *
 * Creates both the bookmark and addressbook widgets based on context->type
 */
void
fe_MakeBookmarkWidgets(Widget shell, MWContext *context)
{
  static char* columnStupidity = NULL; /* Sillyness to give the outline
					  code a place to write down column
					  width stuff that will never change
					  and that we don't care about. */

  Arg av [20];
  int ac = 0;
  Pixel pix = 0;

  Widget mainw;
  Widget   menubar = 0;

  static int columnwidths[] = {100};
  struct fe_button *menudesc;

  fe_ContextData* data = CONTEXT_DATA(context);
  fe_bookmark_data* d;

  if (context->type == MWContextBookmarks) {
    menudesc = bookmarkMenudesc;
  }
  else if (context->type == MWContextAddressBook) {
    menudesc = addressbookMenudesc;
  }
  else
    XP_ASSERT((context->type != MWContextBookmarks) ||
		(context->type != MWContextAddressBook));

  /* Allocate fontend data */
  d = BM_DATA(context) = XP_NEW_ZAP(fe_bookmark_data);

  data->widget = shell;

  /* When bookmark context get pops down, we need to save Bookmarks */
  XtAddCallback(shell, XmNpopdownCallback, fe_bm_popdown, context);

  XmGetColors (XtScreen (shell),
               fe_cmap(context),
               data->default_bg_pixel,
               &data->fg_pixel,
               &data->top_shadow_pixel,
               &data->bottom_shadow_pixel,
               &pix);

  /* "mainw" is the parent of the top level of vertically stacked areas.
   */
  ac = 0;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  mainw = XmCreateForm (shell, "mainform", av, ac);

  menubar = fe_PopulateMenubar(mainw, menudesc, context, context,
			       NULL, fe_bm_pulldown_cb);

  ac = 0;
  if (columnStupidity) {
    XP_FREE(columnStupidity);
    columnStupidity = NULL;
  }
  d->outline = fe_OutlineCreate(context, mainw, "bmlist", av, ac,
				0, 1, columnwidths, fe_bm_datafunc,
				fe_bm_clickfunc, fe_bm_iconfunc, context,
				FE_DND_BOOKMARK, &columnStupidity);

  fe_dnd_CreateDrop(d->outline, fe_bm_dropfunc, context);


  XtVaSetValues(menubar,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		0);


  XtVaSetValues(d->outline,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, menubar,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		0);

  XtManageChild(d->outline);
  XtManageChild (menubar);
  XtManageChild (mainw);

  data->menubar = menubar;
  XtVaSetValues (shell, XmNinitialFocus, mainw, 0);
  XtVaSetValues (mainw, XmNinitialFocus, d->outline, 0);

  fe_HackTranslations (context, shell);

  fe_make_bmpopup(context);
}

static void
fe_make_edititem_dialog(MWContext* bm_context)
{
  fe_bookmark_data* d = BM_DATA(bm_context);

  int ac;
  Arg av[20];
  Widget kids[20];
  int i;

  Widget form = 0;
  Widget nickname_label = 0;
  Widget name_label = 0;
  Widget description_label = 0;
  Widget addedon_label = 0;

  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  XtVaGetValues(CONTEXT_WIDGET(bm_context), XtNvisual, &v,
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


  d->editshell = XmCreatePromptDialog(CONTEXT_WIDGET(bm_context),
					"bookmarkProps", av, ac);

  XtUnmanageChild_safe (XmSelectionBoxGetChild (d->editshell,
						XmDIALOG_SELECTION_LABEL));
  XtUnmanageChild_safe (XmSelectionBoxGetChild (d->editshell,
						XmDIALOG_TEXT));
  XtUnmanageChild_safe (XmSelectionBoxGetChild (d->editshell,
						XmDIALOG_APPLY_BUTTON));
  XtUnmanageChild_safe (XmSelectionBoxGetChild (d->editshell,
						XmDIALOG_HELP_BUTTON));

  XtAddCallback(d->editshell, XmNdestroyCallback, fe_bmedit_destroy,
		bm_context);
  XtAddCallback(d->editshell, XmNokCallback, fe_bmedit_ok, bm_context);
  XtAddCallback(d->editshell, XmNcancelCallback, fe_bmedit_close, bm_context);

  ac = 0;
  form = XmCreateForm (d->editshell, "form", av, ac);
  d->title = XmCreateLabelGadget(form, "title", av, ac);
  d->aliaslabel = XmCreateLabelGadget(form, "aliasLabel", av, ac);
  d->aliasbutton = XmCreatePushButtonGadget (form, "aliasButton", av, ac);
  if (bm_context->type == MWContextAddressBook) {
    nickname_label = XmCreateLabelGadget(form, "nicknameLabel", av, ac);
  }
  name_label = XmCreateLabelGadget(form, "nameLabel", av, ac);
  d->locationlabel = XmCreateLabelGadget(form, "locationLabel", av, ac);
  description_label = XmCreateLabelGadget(form, "descriptionLabel", av, ac);
  if (bm_context->type == MWContextBookmarks) {
    d->lastvisitedlabel = XmCreateLabelGadget(form, "lastvisitedLabel", av, ac);
    addedon_label = XmCreateLabelGadget(form, "addedonLabel", av, ac);

  }

  XtAddCallback(d->aliasbutton, XmNactivateCallback, fe_bmedit_selectalias_cb,
				 bm_context);

  ac = 0;
  if (bm_context->type == MWContextAddressBook) {
    d->nickname = fe_CreateTextField(form, "nicknameText", av, ac);
  }
  d->name = fe_CreateTextField(form, "nameText", av, ac);
  d->location = fe_CreateTextField(form, "locationText", av, ac);

  ac = 0;
  XtSetArg (av[ac], XmNeditMode, XmMULTI_LINE_EDIT); ac++;
  d->description = fe_CreateText(form, "descriptionText", av, ac);
  
  if (bm_context->type != MWContextAddressBook) {
    ac = 0;
    d->lastvisited = XmCreateLabelGadget(form, "lastVisited", av, ac);
    d->addedon = XmCreateLabelGadget(form, "addedOn", av, ac);
  }
  
  XtVaSetValues(d->title,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		0);
  if (bm_context->type == MWContextAddressBook) {
    XtVaSetValues(nickname_label,
		  XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		  XmNtopWidget, d->nickname,
		  XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		  XmNbottomWidget, d->nickname,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_WIDGET,
		  XmNrightWidget, d->nickname,
		  0);
   }
  XtVaSetValues(name_label,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, d->name,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, d->name,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, d->name,
		0);
  XtVaSetValues(d->locationlabel,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, d->location,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, d->location,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, d->location,
		0);
  XtVaSetValues(description_label,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, d->description,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, d->description,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, d->description,
		0);
  if (bm_context->type != MWContextAddressBook) {
    XtVaSetValues(d->lastvisitedlabel,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, d->lastvisited,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, d->lastvisited,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, d->lastvisited,
		0);
    XtVaSetValues(addedon_label,
		XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNtopWidget, d->addedon,
		XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNbottomWidget, d->addedon,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, d->addedon,
		0);
  }
  ac = 0;
  kids[ac++] = d->title;
  if (bm_context->type == MWContextAddressBook) {
    kids[ac++] = d->nickname;
  }
  kids[ac++] = d->name;
  kids[ac++] = d->location;
  kids[ac++] = d->description;
  if (bm_context->type != MWContextAddressBook) {
    kids[ac++] = d->lastvisited;
    kids[ac++] = d->addedon;
  }
  XtVaSetValues(kids[1],
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, kids[0],
		XmNleftAttachment, XmATTACH_NONE,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		0);
  for (i=2 ; i<ac ; i++) {
    XtVaSetValues(kids[i],
		  XmNtopAttachment, XmATTACH_WIDGET,
		  XmNtopWidget, kids[i-1],
		  XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		  XmNleftWidget, kids[i-1],
		  XmNrightAttachment, XmATTACH_FORM,
		  XmNbottomAttachment, XmATTACH_NONE,
		  0);
  }
  XtVaSetValues(d->aliaslabel,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, kids[ac-1],
		XmNleftAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_WIDGET,
		XmNrightWidget, d->aliasbutton,
		0);
  XtVaSetValues(d->aliasbutton,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, kids[ac-1],
		XmNleftAttachment, XmATTACH_NONE,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		0);

  if (bm_context->type == MWContextAddressBook)
    fe_attach_field_to_labels(d->nickname, nickname_label, 
				name_label, d->locationlabel,
				description_label, 0);
  else
    fe_attach_field_to_labels(d->name, name_label, d->locationlabel,
				description_label, d->lastvisitedlabel,
				addedon_label, 0);

  kids[ac++] = d->aliaslabel;
  kids[ac++] = d->aliasbutton;
  if (bm_context->type == MWContextAddressBook) {
    kids[ac++] = nickname_label;
  }
  kids[ac++] = name_label;
  kids[ac++] = d->locationlabel;
  kids[ac++] = description_label;
  if (bm_context->type != MWContextAddressBook) {
    kids[ac++] = d->lastvisitedlabel;
    kids[ac++] = addedon_label;
  }

  XtManageChildren(kids, ac);
  XtManageChild(form);

  fe_HackDialogTranslations (d->editshell);
}


#if 0		/* OBSOLETE */
void
fe_GotoBookmarkCallback(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  const char* filename;
  char* name;
  filename = BM_GetFileName(main_bm_context);
  if (!filename) return;
  fe_SaveBookmarks(filename);
  name = XP_ALLOC(XP_STRLEN(filename) + 50);
  if (!name) return;
  XP_SPRINTF(name, "file:%s", filename);
  FE_GetURL(context, NET_CreateURLStruct(name, FALSE));
  XP_FREE(name);
}
#endif /* 0 */


Boolean
fe_ImportBookmarks (char *filename)
{
  char url[512];
  PR_snprintf(url, sizeof (url), "file:%s", filename);

  BM_ReadBookmarksFromDisk(main_bm_context, filename, url);
  return True;
}

Boolean
fe_LoadBookmarks (char *filename)
{
#if 0
  struct stat st;
#endif
  char url[512];
  if (!filename)
    filename = fe_globalPrefs.bookmark_file;
  PR_snprintf(url, sizeof (url), "file:%s", filename);

  BM_ReadBookmarksFromDisk(main_bm_context, filename, url);
  return True;
}

Boolean
fe_SaveBookmarks (void)
{
  if (addressbook_context) BM_SaveBookmarks(addressbook_context, NULL);

  if (main_bm_context) 
    if (BM_SaveBookmarks(main_bm_context, NULL) < 0)
      return False;
  return True;
}

void
fe_AddToBookmark (MWContext *context, const char *title, URL_Struct *url,
		  time_t time)
{
  BM_Entry *bm;
  const char *new_title;
  if (!title || !*title) new_title = url->address;
  else new_title = title;
  bm = (BM_Entry*) BM_NewUrl(new_title, url->address, NULL, time);
  BM_AppendToHeader (main_bm_context, BM_GetAddHeader(main_bm_context), bm);
}

/* early definition for the benefit of fe_AddBookmarkCallback */
static void
fe_add_bookmark_menu_items (MWContext *, Widget, MWContext*, BM_Entry*);

void
fe_AddBookmarkCallback (Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  History_entry *h = SHIST_GetCurrent (&context->hist);
  BM_Entry *bm;
  char *new_title;

  if (!h) return;

  if (!h->title || !*h->title) new_title = h->address;
  else new_title = h->title;
  bm = (BM_Entry*)BM_NewUrl( new_title, h->address, NULL, h->last_access);

  fe_UserActivity (context);

  BM_AppendToHeader (main_bm_context, BM_GetAddHeader(main_bm_context), bm);
}

void
fe_ViewBookmarkCallback (Widget widget, XtPointer closure, XtPointer call_data)
{
  XtPopup (CONTEXT_WIDGET (main_bm_context), XtGrabNone);
}

static XmString
fe_format_item (MWContext* bm_context, BM_Entry* entry, Boolean no_indent,
		int16 charset)
{
  XmString xmhead, xmtail, xmcombo;
  int depth = (no_indent ? 0 : BM_GetDepth(bm_context, entry));
  char head [255];
  char buf [1024];
  int j = 0;
  char *title = BM_GetName(entry);
  char *url = BM_GetAddress(entry);
  int indent = 2;
  int left_offset = 2;

  if (! no_indent)
    {
      head [j++] = (BM_IsHeader(entry)
		    ? (BM_IsFolded(entry) ? '+' : '-') :
		    BM_IsSeparator(entry) ? ' ' :
		    /* ### item->last_visit == 0*/ 0 ? '?' : ' ');
      while (j < ((depth * indent) + left_offset))
	head [j++] = ' ';
    }
  head [j] = 0;

  if (BM_IsSeparator(entry))
    {
      strcpy (buf, "-------------------------");
    }
  else if (title || url)
    {
      fe_FormatDocTitle (title, url, buf, 1024);
    }
  else if (BM_IsUrl(entry))
    {
      strcpy (buf, XP_GetString( XFE_GG_EMPTY_LL ) );
    }

  if (!*head)
    xmhead = 0;
  else
    xmhead = XmStringSegmentCreate (head, "ICON", XmSTRING_DIRECTION_L_TO_R,
				    False);

  if (!*buf)
    xmtail = 0;
  else if (BM_IsUrl(entry))
    xmtail = fe_ConvertToXmString (charset, (unsigned char *) buf);
  else
    {
      char *loc;

      loc = (char *) fe_ConvertToLocaleEncoding (charset,
							(unsigned char *) buf);
      xmtail = XmStringSegmentCreate (loc, "HEADING",
					XmSTRING_DIRECTION_L_TO_R, False);
      if (loc != buf)
	{
	  XP_FREE(loc);
	}
    }

  if (xmhead && xmtail)
    {
      int size = XmStringLength (xmtail);

      /* Motif sucks, as usual.
	 There is a bug down under XmStringNConcat() where, in the process of
	 appending the two strings, it will read up to four bytes past the end
	 of the second string.  It doesn't do anything with the data it reads
	 from there, but people have been regularly crashing as a result of it,
	 because sometimes these strings end up at the end of the page!!

	 I tried making the second string a bit longer (by adding some spaces
	 to the end of `buf') and passing in a correspondingly smaller length
	 to XmStringNConcat(), but that causes it not to append *any* of the
	 characters in the second string.  So XmStringNConcat() would seem to
	 simply not work at all when the length isn't exactly the same as the
	 length of the second string.

	 Plan B is to simply realloc() the XmString to make it a bit larger,
	 so that we know that it's safe to go off the end.

	 The NULLs at the beginning of the data we append probably aren't
	 necessary, but they are read and parsed as a length, even though the
	 data is not used (with the current structure of the XmStringNConcat()
	 code.)  (The initialization of this memory is only necessary at all
	 to keep Purify from (rightly) squawking about it.)

	 And oh, by the way, the strncpy() down there, which I have verified
	 is doing strncpy( <the-right-place> , <the-string-below>, 15 )
	 actually writes 15 \000 bytes instead of four \000 bytes plus the
	 random text below.  So I seem to have stumbled across a bug in
	 strncpy() as well, even though it doesn't hose me.  THIS TIME. -- jwz
       */
#define MOTIF_BUG_BUFFER "\000\000\000\000 MOTIF BUG"
#ifdef MOTIF_BUG_BUFFER
      xmtail = (XmString) realloc ((void *) xmtail,
				   size + sizeof (MOTIF_BUG_BUFFER));
      strncpy (((char *) xmtail) + size, MOTIF_BUG_BUFFER,
	       sizeof (MOTIF_BUG_BUFFER));
# undef MOTIF_BUG_BUFFER
#endif /* MOTIF_BUG_BUFFER */

      xmcombo = XmStringNConcat (xmhead, xmtail, size);
    }
  else if (xmhead)
    {
      xmcombo = xmhead;
      xmhead = 0;
    }
  else if (xmtail)
    {
      xmcombo = xmtail;
      xmtail = 0;
    }
  else
    {
      xmcombo = XmStringCreateLtoR ("", XmFONTLIST_DEFAULT_TAG);
    }

  if (xmhead) XmStringFree (xmhead);
  if (xmtail) XmStringFree (xmtail);
  return (xmcombo);
}


static void
fe_bm_fillin_menu(Widget widget, XtPointer closure, XtPointer call_data);

static void
fe_add_bookmark_menu_items (MWContext *context, Widget menu,
			    MWContext* bm_context, BM_Entry* entry)
{
  Widget button;
  Arg av [20];
  int ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;

  XtVaGetValues (CONTEXT_WIDGET (context), XtNvisual, &v,
		 XtNcolormap, &cmap, XtNdepth, &depth, 0);

  while (entry)
    {
      XmString xmname = fe_format_item(bm_context, entry, True,
				       INTL_DefaultWinCharSetID(NULL));
      char *s;

      /* Mid truncate the name */
      if (XmStringGetLtoR(xmname, XmSTRING_DEFAULT_CHARSET, &s)) {
	XmStringFree (xmname);
	fe_MidTruncateString (s, s, 40);
	xmname = XmStringCreateLtoR(s, XmSTRING_DEFAULT_CHARSET);
	XP_FREE(s);
      };

      if (BM_IsHeader(entry))
	{
	  Widget submenu;
	  ac = 0;
	  if (BM_GetChildren(entry))
	    {
	      XtSetArg (av[ac], XmNlabelString, xmname); ac++;
	      XtSetArg (av[ac], XmNvisual, v); ac++;
	      XtSetArg (av[ac], XmNdepth, depth); ac++;
	      XtSetArg (av[ac], XmNcolormap, cmap); ac++;
	      submenu = XmCreatePulldownMenu (menu, "bookmarkCascadeMenu",
					      av, ac);
	    }
	  else
	    {
	      submenu = 0;
	    }
	  ac = 0;
	  XtSetArg (av[ac], XmNlabelString, xmname); ac++;
	  XtSetArg (av[ac], XmNsubMenuId, submenu); ac++;
	  XtSetArg (av[ac], XmNuserData, entry); ac++;
	  button = XmCreateCascadeButtonGadget (menu, "bookmarkCascadeButton",
						av, ac);
	  XtAddCallback(button, XmNcascadingCallback, fe_bm_fillin_menu,
			context);
	  XtManageChild (button);
	}
      else if (BM_IsSeparator(entry))
	{
	  ac = 0;
	  button = XmCreateSeparatorGadget (menu, "separator", av, ac);
	  XtManageChild (button);
	}
      else
	{
	  ac = 0;
	  XtSetArg (av[ac], XmNlabelString, xmname); ac++;
	  XtSetArg (av[ac], XmNuserData, entry); ac++;
	  button = XmCreatePushButtonGadget (menu, "bookmarkMenuItem", av, ac);
	  XtAddCallback (button, XmNactivateCallback,
			 fe_bookmark_menu_cb, context);
	  XtAddCallback (button, XmNarmCallback,
			 fe_bookmark_menu_arm, context);
	  XtAddCallback (button, XmNdisarmCallback,
			 fe_bookmark_menu_disarm, context);
	  XtManageChild (button);
	}

      XmStringFree (xmname);
      entry = BM_GetNext(entry);
    }
}


static void
fe_bm_fillin_menu(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *context = (MWContext *) closure;
  MWContext* bm_context = main_bm_context; /* ### */
  BM_Entry* entry = 0;
  Widget menu = 0;
  XtVaGetValues(widget, XmNuserData, &entry, XmNsubMenuId, &menu, 0);
  if (entry) {
    XP_ASSERT(BM_IsHeader(entry));
    fe_add_bookmark_menu_items(context, menu, bm_context,
			       BM_GetChildren(entry));
    XtVaSetValues(widget, XmNuserData, NULL, 0);
  }
}


void
fe_DestroyWidgetTree(Widget *widgets, int n)
{
    int i;
    Widget *morekids = NULL;
    int nmorekids = 0;
    Widget submenu = 0;

    if (n <= 0) return;

    for (i = n-1; i >= 0; i--) {
	XtVaGetValues (widgets[i], XmNsubMenuId, &submenu, 0);
	XtDestroyWidget (widgets[i]);
	if (submenu) {
	    XtVaGetValues (widgets[i], XmNchildren, &morekids,
				XmNnumChildren, &nmorekids, 0);
	    fe_DestroyWidgetTree (morekids, nmorekids);
	}
    }
}


void
fe_GenerateBookmarkMenu (MWContext *context)
{
  Widget menu = CONTEXT_DATA (context)->bookmark_menu;
  Widget *kids = 0;
  Cardinal nkids = 0;
  Widget button;
  Arg av [20];
  int ac;
  MWContext* bm_context;
  BM_Entry* entry;

  bm_context = main_bm_context; /* ### */

  if (menu == NULL || CONTEXT_DATA (context)->bookmark_menu_up_to_date_p)
    return;

  XtUnrealizeWidget (menu);

  XtVaGetValues (menu, XmNchildren, &kids, XmNnumChildren, &nkids, 0);

  /* The first 2 kids are std: addBookmark & gotoBookmarkHtml
   *
   * Not any more: the FE's have just decided to do away with
   * the Goto menu option since we can just pop the bookmarks
   * window instead.
   */
  XP_ASSERT(nkids >= 1);
  nkids -= 1;
  kids = &kids[1];
  if (nkids)
    XtUnmanageChildren (kids, nkids);

  fe_DestroyWidgetTree(kids, nkids);

  entry = BM_GetMenuHeader(bm_context);
  if (entry)
    {
      /* this is a bit of a hack, but we don't want to create
       * a submenu for the top header, "Joe's Bookmarks". instead
       * we jump to the children.
       */
      if (BM_IsHeader(entry)) entry = BM_GetChildren(entry);
      if (entry)
	{
	  ac = 0;
	  button = XmCreateSeparatorGadget (menu, "separator", av, ac);
	  XtManageChild (button);
	  fe_add_bookmark_menu_items (context, menu, bm_context, entry);
	}
    }


  XtRealizeWidget (menu);

  CONTEXT_DATA (context)->bookmark_menu_up_to_date_p = True;
}



void
fe_BookmarkInit (MWContext *context)
{
  /* Now create the bookmarks context.  We put it off to now because our
     code is so fragile that the bookmarks context creation code breaks in
     zillions of ways if it is the first context created.  ### */
  if (main_bm_context == NULL) {
    XtAppAddActions (fe_XtAppContext, fe_bookmarkActions,
			countof(fe_bookmarkActions));

    main_bm_context = fe_MakeWindow(XtParent(CONTEXT_WIDGET(context)),
			 NULL, 0, NULL, MWContextBookmarks, TRUE);
    BM_InitializeBookmarksContext(main_bm_context);
    fe_LoadBookmarks (NULL);
  }
}

void
fe_BookmarkAccessTimeChanged (MWContext *context, BookmarkStruct *h)
{
  /* ### Write me! */
}


void
BMFE_RefreshCells(MWContext* bm_context, int32 first, int32 last,
		  XP_Bool now )
{
  if (BM_DATA(bm_context)) {
    fe_OutlineChange(BM_DATA(bm_context)->outline, first - 1, last - first + 1,
		     BM_GetVisibleCount(bm_context));
    fe_OutlineSetMaxDepth(BM_DATA(bm_context)->outline,
			  BM_GetMaxDepth(bm_context));
  }
}


void
BMFE_SyncDisplay(MWContext* bm_context)
{
/***
  if (BM_DATA(bm_context)->outline)
    BMFE_RefreshCells(bm_context, 1, BM_LAST_CELL, TRUE);
***/
}


void
BMFE_MeasureEntry(MWContext* bm_context, BM_Entry* entry,
		  uint32* width, uint32* height )
{
  *width = 1;			/* ### */
  *height = 1;
}


void
BMFE_FreeClipContents(MWContext* bm_context)
{
  struct fe_bookmark_data *data = BM_DATA(bm_context);
  if (data->clip.block) XP_FREE(data->clip.block);
  data->clip.block = NULL;
  data->clip.length = 0;
}

void *
BMFE_GetClipContents(MWContext* bm_context, int32* length)
{
  struct fe_bookmark_data *data = BM_DATA(bm_context);
  if (length)
    *length = data->clip.length;
  return (data->clip.block);
}

void
BMFE_SetClipContents(MWContext* bm_context, void *block, int32 length )
{
  struct fe_bookmark_data *data = BM_DATA(bm_context);
  BMFE_FreeClipContents(bm_context);
  
  data->clip.block = XP_ALLOC(length);
  XP_MEMCPY(data->clip.block, block, length);
  data->clip.length = length;
}

void
BMFE_OpenBookmarksWindow(MWContext* bm_context)
{
  fe_bookmark_data* d = BM_DATA(bm_context);
  if (!d->editshell) {
    fe_make_edititem_dialog(bm_context);
    d->entry = NULL;
  }
  XtManageChild(d->editshell);
  XMapRaised(XtDisplay(d->editshell), XtWindow(d->editshell));
}

void
BMFE_EditItem(MWContext* bm_context, BM_Entry* entry )
{
  fe_bookmark_data* d = BM_DATA(bm_context);
  const char* str;
  char* tmp;

  /* Dont edit item if editshell in not popped up already */
  if (!d->editshell || !XtIsManaged(d->editshell)) return;

  /* Raise the edit shell to view */
  if (XtIsManaged (CONTEXT_WIDGET (bm_context))) {
    XRaiseWindow(XtDisplay(CONTEXT_WIDGET (bm_context)),
		 XtWindow(CONTEXT_WIDGET (bm_context)));
  }

  fe_bmedit_commit_changes(bm_context);

  /* Alias stuff */
  if (BM_IsAlias(entry)) {
    fe_SetString(d->title, XmNlabelString, XP_GetString(XFE_BM_ALIAS));
    entry = BM_GetAliasOriginal(entry);
  }
  else
    fe_SetString(d->title, XmNlabelString, " ");

  d->entry = entry;

  if (bm_context->type == MWContextAddressBook) {
    str = BM_GetNickName(entry);
    fe_SetTextField(d->nickname, str ? str : "");
    if (BM_IsHeader(entry)) {
      XtVaSetValues(d->locationlabel, XmNsensitive, False, 0);
      XtVaSetValues(d->location, XmNsensitive, False, XmNvalue, "", 0);
    }
    else {
      XtVaSetValues(d->locationlabel, XmNsensitive, True, 0);
      str = BM_GetAddress(entry);
      XtVaSetValues(d->location, XmNsensitive, True, 0);
      fe_SetTextField(d->location, str?str:"");
    }
    str = BM_GetName(entry);
    fe_SetTextField(d->name, str ? str : "");
    str = BM_GetDescription(entry);
    fe_SetTextField(d->description, str?str:"");
    fe_SetString(d->aliaslabel, XmNlabelString,
			BM_PrettyAliasCount(bm_context, entry));
    if (BM_CountAliases(bm_context, entry) > 0)
      XtVaSetValues(d->aliasbutton, XmNsensitive, True, 0);
    else
      XtVaSetValues(d->aliasbutton, XmNsensitive, False, 0);
  }
  else  /* MWContextBookmarks */
  if (BM_IsUrl(entry)) {
    str = BM_GetName(entry);
    fe_SetTextField(d->name, str?str:"");
    str = BM_GetAddress(entry);
    fe_SetTextField(d->location, str?str:"");
    XtVaSetValues(d->location, XmNsensitive, True, 0);
    XtVaSetValues(d->locationlabel, XmNsensitive, True, 0);
    str =  BM_GetDescription(entry);
    fe_SetTextField(d->description, str?str:"");
    tmp = BM_PrettyLastVisitedDate(entry);
    if (!tmp || !*tmp) tmp = " ";
    fe_SetString(d->lastvisited, XmNlabelString, tmp);
    XtVaSetValues(d->lastvisited, XmNsensitive, True, 0);
    XtVaSetValues(d->lastvisitedlabel, XmNsensitive, True, 0);
    fe_SetString(d->addedon, XmNlabelString, BM_PrettyAddedOnDate(entry));
    fe_SetString(d->aliaslabel, XmNlabelString,
			BM_PrettyAliasCount(bm_context, entry));
    if (BM_CountAliases(bm_context, entry) > 0)
      XtVaSetValues(d->aliasbutton, XmNsensitive, True, 0);
    else
      XtVaSetValues(d->aliasbutton, XmNsensitive, False, 0);
  }
  else if (BM_IsHeader(entry)) {
    str = BM_GetName(entry);
    fe_SetTextField(d->name, str?str:"");
    XtVaSetValues(d->location, XmNvalue, "", 0);
    XtVaSetValues(d->location, XmNsensitive, False, 0);
    XtVaSetValues(d->locationlabel, XmNsensitive, False, 0);
    str =  BM_GetDescription(entry);
    fe_SetTextField(d->description, str?str:"");
    fe_SetString(d->lastvisited, XmNlabelString, " ");
    XtVaSetValues(d->lastvisited, XmNsensitive, False, 0);
    XtVaSetValues(d->lastvisitedlabel, XmNsensitive, False, 0);
    fe_SetString(d->addedon, XmNlabelString, BM_PrettyAddedOnDate(entry));
    fe_SetString(d->aliaslabel, XmNlabelString, "" );
    XtVaSetValues(d->aliasbutton, XmNsensitive, False, 0);
  }
}


void
BMFE_EntryGoingAway(MWContext* bm_context, BM_Entry* entry)
{
  fe_bookmark_data* d = BM_DATA(bm_context);
  if (d->entry == entry) {
    d->entry = NULL;
    if (d->editshell) {
      XtVaSetValues(d->name, XmNvalue, "", 0);
      XtVaSetValues(d->location, XmNvalue, "", 0);
      XtVaSetValues(d->description, XmNvalue, "", 0);
      if (d->lastvisited) fe_SetString(d->lastvisited, XmNlabelString, " ");
      if (d->addedon) fe_SetString(d->addedon, XmNlabelString, " ");
    }
  }
}


void
BMFE_CloseBookmarksWindow(MWContext* bm_context )
{
  fe_bmedit_close(NULL, bm_context, NULL);
}


void
BMFE_GotoBookmark(MWContext* bm_context, const char* url )
{
#ifdef EDITOR
  fe_BrowserGetURL(bm_context, (char *)url);
#else
  MWContext *context = fe_GridToTopContext (fe_all_MWContexts->context);
  FE_GetURL(context, NET_CreateURLStruct(url, FALSE));
#endif
}


void*
BMFE_OpenFindWindow(MWContext* bm_context, BM_FindInfo* findInfo )
{
  fe_bookmark_data* d = BM_DATA(bm_context);

  if (!d->findshell)
    fe_make_find_dialog (bm_context, findInfo);

  fe_find_setvalues(bm_context, findInfo);

  if (!XtIsManaged(d->findshell))
    XtManageChild(d->findshell);
  XMapRaised(XtDisplay(d->findshell), XtWindow(d->findshell));
  return 0;
}


void
BMFE_ScrollIntoView(MWContext* bm_context, BM_Entry* entry )
{
  int num = BM_GetIndex(bm_context, entry);
  if (num > 0) {
    fe_OutlineMakeVisible(BM_DATA(bm_context)->outline, num - 1);
  }
}

/* Find */

static void
fe_find_setvalues(MWContext* bm_context, BM_FindInfo* findInfo)
{
  fe_bookmark_data* d = BM_DATA(bm_context);
  fe_SetTextField(d->findtext, findInfo->textToFind);
  if (d->findnicknameT) {
    XtVaSetValues(d->findnicknameT, XmNset, findInfo->checkNickname, 0);
  }
  XtVaSetValues(d->findnameT, XmNset, findInfo->checkName, 0);
  XtVaSetValues(d->findlocationT, XmNset, findInfo->checkLocation, 0);
  XtVaSetValues(d->finddescriptionT, XmNset, findInfo->checkDescription, 0);
  XtVaSetValues(d->findcaseT, XmNset, findInfo->matchCase, 0);
  XtVaSetValues(d->findwordT, XmNset, findInfo->matchWholeWord, 0);
}

static void
fe_find_getvalues(MWContext* bm_context, BM_FindInfo* findInfo)
{
  fe_bookmark_data* d = BM_DATA(bm_context);
  /* Should we free findInfo->textToFind */
  XP_FREE(findInfo->textToFind);
  findInfo->textToFind = fe_GetTextField(d->findtext);
  if (d->findnicknameT) {
    XtVaGetValues(d->findnicknameT, XmNset, &findInfo->checkNickname, 0);
  }
  XtVaGetValues(d->findnameT, XmNset, &findInfo->checkName, 0);
  XtVaGetValues(d->findlocationT, XmNset, &findInfo->checkLocation, 0);
  XtVaGetValues(d->finddescriptionT, XmNset, &findInfo->checkDescription, 0);
  XtVaGetValues(d->findcaseT, XmNset, &findInfo->matchCase, 0);
  XtVaGetValues(d->findwordT, XmNset, &findInfo->matchWholeWord, 0);
}

static void
fe_bmfind_destroy_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* bm_context = (MWContext*) closure;
  fe_bookmark_data* d = BM_DATA(bm_context);
  if (d->findshell) {
    d->findshell = 0;
  }
}


static void
fe_bmfind_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  MWContext* bm_context = (MWContext*) closure;
  fe_bookmark_data* d = BM_DATA(bm_context);

  if (cb->reason == XmCR_OK) {
    /* Update FindInfo */
    BM_FindInfo* findinfo;
    XtVaGetValues(d->findshell, XmNuserData, &findinfo, 0);
    fe_find_getvalues(bm_context, findinfo);
    XtUnmanageChild(d->findshell);
    BM_DoFindBookmark(bm_context, findinfo);
  }
  else if (cb->reason == XmCR_APPLY) {
    /* Clear */
    BM_FindInfo* findinfo;
    XtVaGetValues(d->findshell, XmNuserData, &findinfo, 0);
    if (findinfo->textToFind)
      findinfo->textToFind[0] = '\0';
    fe_SetTextField(d->findtext, findinfo->textToFind);
    XmProcessTraversal (d->findtext, XmTRAVERSE_CURRENT);
  }
  else if (cb->reason == XmCR_CANCEL)
    XtUnmanageChild(d->findshell);

  return;
}


static void
fe_make_find_dialog (MWContext *bm_context, BM_FindInfo* findInfo)
{
  Widget mainw = CONTEXT_WIDGET (bm_context);
  fe_bookmark_data* d = BM_DATA(bm_context);

  Widget dialog, form;
  Widget findlabel, findtext;
  Widget nicknameT = NULL;
  Widget lookinlabel, nameT, locationT, descriptionT, caseT, wordT;
  Widget helplabel;

  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Arg av [20];
  int ac;
  Widget kids[20];

  XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap,
		 XtNdepth, &depth, 0);
  ac = 0;
  XtSetArg (av[ac], XmNvisual, v); ac++;
  XtSetArg (av[ac], XmNdepth, depth); ac++;
  XtSetArg (av[ac], XmNcolormap, cmap); ac++;
  XtSetArg (av[ac], XmNallowShellResize, TRUE); ac++;
  XtSetArg (av[ac], XmNtransientFor, mainw); ac++;
  XtSetArg (av[ac], XmNdialogStyle, XmDIALOG_MODELESS); ac++;
  XtSetArg (av[ac], XmNdialogType, XmDIALOG_QUESTION); ac++;
  XtSetArg (av[ac], XmNdeleteResponse, XmUNMAP); ac++;
  XtSetArg(av[ac], XmNuserData, (XtPointer) findInfo); ac++;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  dialog = XmCreatePromptDialog (mainw, "findDialog", av, ac);

  /* XtUnmanageChild_safe (XmSelectionBoxGetChild (dialog,
						XmDIALOG_SEPARATOR)); */
  XtUnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  XtUnmanageChild_safe (XmSelectionBoxGetChild (dialog,
						XmDIALOG_SELECTION_LABEL));
  XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));
#ifdef NO_HELP
  XtUnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
#endif

  XtAddCallback (dialog, XmNdestroyCallback, fe_bmfind_destroy_cb, bm_context);
  XtAddCallback (dialog, XmNokCallback, fe_bmfind_cb, bm_context);
  XtAddCallback (dialog, XmNcancelCallback, fe_bmfind_cb, bm_context);
  XtAddCallback (dialog, XmNapplyCallback, fe_bmfind_cb, bm_context);

  ac = 0;
  form = XmCreateForm (dialog, "form", av, ac);

  ac = 0;
  findlabel = XmCreateLabelGadget(form, "findLabel", av, ac);
  ac = 0;
  findtext = fe_CreateTextField(form, "findText", av, ac);
  ac = 0;
  lookinlabel = XmCreateLabelGadget(form, "lookinLabel", av, ac);
  XP_ASSERT(findlabel->core.width > 0 && lookinlabel->core.width > 0);
  if (findlabel->core.width < lookinlabel->core.width)
    XtVaSetValues (findlabel, XmNwidth, lookinlabel->core.width, 0);
  else
    XtVaSetValues (lookinlabel, XmNwidth, findlabel->core.width, 0);


  ac = 0;
  XtSetArg(av[ac], XmNindicatorType, XmN_OF_MANY); ac++;
  if (bm_context->type == MWContextAddressBook) {
    nicknameT = XmCreateToggleButtonGadget (form, "nicknameToggle", av, ac);
  }
  nameT = XmCreateToggleButtonGadget (form, "nameToggle", av, ac);
  locationT = XmCreateToggleButtonGadget (form, "locationToggle", av, ac);
  descriptionT = XmCreateToggleButtonGadget (form, "descriptionToggle", av, ac);
  caseT = XmCreateToggleButtonGadget (form, "caseSensitive", av, ac);
  wordT = XmCreateToggleButtonGadget (form, "wordToggle", av, ac);

  ac = 0;
  XtSetArg(av[ac], XmNentryAlignment, XmALIGNMENT_CENTER); ac++;
  helplabel = XmCreateLabelGadget(form, "helptext", av ,ac);

  /* Attachments */
  XtVaSetValues(findlabel, XmNleftAttachment, XmATTACH_FORM,
  			XmNtopAttachment, XmATTACH_FORM, 0);

  XtVaSetValues(findtext, XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, findlabel,
  			XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNtopWidget, findlabel,
			XmNrightAttachment, XmATTACH_FORM, 0);

  XtVaSetValues(lookinlabel, XmNleftAttachment, XmATTACH_FORM,
  			XmNtopAttachment, XmATTACH_WIDGET,
  			XmNtopWidget, findtext, 0);

  if (nicknameT) {
    XtVaSetValues(nicknameT, XmNleftAttachment, XmATTACH_WIDGET,
		  XmNleftWidget, lookinlabel,
		  XmNtopAttachment, XmATTACH_WIDGET,
		  XmNtopWidget, findtext, 0);
  }    

  XtVaSetValues(nameT, XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, nicknameT ? nicknameT : lookinlabel,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, findtext, 0);

  XtVaSetValues(locationT, XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, nameT,
			XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNtopWidget, nameT, 0);

  XtVaSetValues(descriptionT, XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, locationT,
			XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
			XmNtopWidget, nicknameT ? nicknameT : nameT, 0);

  XtVaSetValues(caseT, XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, lookinlabel,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, nicknameT ? nicknameT : nameT, 0);

  XtVaSetValues(wordT, XmNleftAttachment, XmATTACH_WIDGET,
			XmNleftWidget, lookinlabel,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, caseT, 0);

  XtVaSetValues(helplabel, XmNleftAttachment, XmATTACH_FORM,
  			XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, wordT, 0);

  ac = 0;
  kids[ac++] = findlabel;
  kids[ac++] = findtext;
  kids[ac++] = lookinlabel;
  if (nicknameT) kids[ac++] = nicknameT;
  kids[ac++] = nameT;
  kids[ac++] = locationT;
  kids[ac++] = descriptionT;
  kids[ac++] = caseT;
  kids[ac++] = wordT;
  kids[ac++] = helplabel;
  XtManageChildren(kids, ac);

  XtManageChild(form);

  d->findshell = dialog;
  d->findtext = findtext;
  d->findnicknameT = nicknameT;
  d->findnameT = nameT;
  d->findlocationT = locationT;
  d->finddescriptionT = descriptionT;
  d->findcaseT = caseT;
  d->findwordT = wordT;

  XtVaSetValues (dialog, XmNinitialFocus, findtext, 0);
}


MWContext*
FE_GetAddressBook(MWContext* context, XP_Bool viewnow)
{
  if (!addressbook_context && context) {
    char buf[1024];
    char* home = getenv("HOME");
    if (!home) home = "";
    addressbook_context = fe_MakeWindow(XtParent(CONTEXT_WIDGET(context)),
					NULL, NULL, NULL, MWContextAddressBook,
					TRUE);
    BM_InitializeBookmarksContext(addressbook_context);
    PR_snprintf(buf, sizeof (buf),
	 "%.900s/.netscape/address-book.html", home); /* ### dp -- need
								 a real
								 preference
								 item here! */
    BM_ReadBookmarksFromDisk(addressbook_context, buf, NULL /* ### */);
  }
  if (addressbook_context && viewnow)
    XtPopup (CONTEXT_WIDGET (addressbook_context), XtGrabNone);

  return addressbook_context;
}


/*
 * Action routines
 */

static void
fe_bmpopup_menu_action (Widget widget, XEvent *event, String *cav,
				Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  Widget popup = BM_DATA(bm_context)->popup;
  Widget *buttons;
  int nbuttons, i;

  XmMenuPosition (popup, (XButtonPressedEvent *) event);

  /* Sensitize the popup menu */
  XtVaGetValues(popup, XmNchildren, &buttons, XmNnumChildren, &nbuttons, 0);
  for (i=0 ; i<nbuttons ; i++) {
    Widget item = buttons[i];
    if (XmIsPushButton(item) || XmIsPushButtonGadget(item)) {
      XtPointer value;
      BM_CommandType cmd;
      XtVaGetValues(item, XmNuserData, &value, 0);
      cmd = (BM_CommandType) value;
      if (cmd != BM_Cmd_Invalid) {
        XP_Bool enable = BM_FindCommandStatus(bm_context, cmd);
        XtVaSetValues(item, XmNsensitive, enable, 0);
      }
    }
  }

  XtManageChild (BM_DATA(bm_context)->popup);
}

static void
fe_bmfind_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  BM_ObeyCommand(bm_context, BM_Cmd_Find);
}

static void
fe_bmfindAgain_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  BM_ObeyCommand(bm_context, BM_Cmd_FindAgain);
}

static void
fe_bmundo_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  BM_ObeyCommand(bm_context, BM_Cmd_Undo);
}

static void
fe_bmredo_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  BM_ObeyCommand(bm_context, BM_Cmd_Redo);
}

static void
fe_bmdeleteItem_action (Widget widget, XEvent *event, String *cav,
				Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  BM_ObeyCommand(bm_context, BM_Cmd_Delete);
}

static void
fe_bmmailMessage_action (Widget widget, XEvent *event, String *cav,
				Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  if (bm_context != addressbook_context) return;
  BM_ObeyCommand(bm_context, BM_Cmd_GotoBookmark);
}

static void
fe_bmsaveAs_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  BM_ObeyCommand(bm_context, BM_Cmd_SaveAs);
}

static void
fe_bmclose_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  fe_bm_close(widget, (XtPointer) bm_context, NULL);
}

static void
fe_bmcut_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  BM_ObeyCommand(bm_context, BM_Cmd_Cut);
}

static void
fe_bmcopy_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  BM_ObeyCommand(bm_context, BM_Cmd_Copy);
}

static void
fe_bmpaste_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *bm_context = fe_WidgetToMWContext(widget);
  BM_ObeyCommand(bm_context, BM_Cmd_Paste);
}



