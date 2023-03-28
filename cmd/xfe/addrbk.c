/* -*- Mode:C; tab-width: 8 -*-
   addrbk.c	- a hack at the new address book STUFF
   Copyright © 1996 Netscape Communications Corporation, 
	       all rights reserved.

	       Created:    Benjie Chen - 6/8/96
	       Copied lots of code from hot.c
	       Then deleted lots of code from hot.c
	       Then deleted lots of code from here
	       Then faked bunch of code here
*/


/* note to myself: any comments with Benjie in it means the code
 * below it still needs to be changed from BKMK to ADBK
 */


#include "mozilla.h"
#include "xfe.h"
#include "menu.h"
#include "outline.h"
#include "felocale.h"
#include "xlate.h"

/* Kludge around conflicts between Motif and xp_core.h... */
#undef Bool
#define Bool char

/* must be in front of addrbook.h */
#define no_c_plus_plus
#include "addrbook.h"  
#include "bkmks.h"


#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <XmL/Folder.h>

/* for XP_GetString() */
#include <xpgetstr.h>



/* Quote from UI spec: "We will not support multiple address books. 
 * The current design assumes that there is one personal address 
 * book per user." - 6/8/96. 
 * 
 * BUT...
 */

static ABook* AddrBook = NULL; 
static MWContext* ABfeContext = NULL;

/* just to have this here in case we decide to have multiple address
 * books l8er 
 */

static ABook*     fe_GetABook(MWContext *); 
static MWContext* fe_GetABookFEContext(MWContext *);

#define AB_DATA(context) (CONTEXT_DATA(context)->abdata) 

extern void
fe_attach_field_to_labels (Widget value_field, ...);

extern void
fe_DestroyWidgetTree(Widget *widgets, int n);


static void
fe_ab_deleteItem(MWContext *ab_context);

/* static void 
   fe_ab_find_setvalues(MWContext* ab_context, BM_FindInfo* findInfo); */

static void 
fe_ab_find_destroy_cb(Widget widget, XtPointer closure, XtPointer call_data);

static void 
fe_make_ab_popup(MWContext* ab_context);

static void 
fe_ab_popup_menu_action (Widget, XEvent *, String *, Cardinal *);

static void 
fe_ab_cut_action (Widget, XEvent *, String *, Cardinal *);

static void 
fe_ab_copy_action (Widget, XEvent *, String *, Cardinal *);

static void 
fe_ab_paste_action (Widget, XEvent *, String *, Cardinal *);

static void 
fe_ab_find_action (Widget, XEvent *, String *, Cardinal *);

static void 
fe_ab_findAgain_action (Widget, XEvent *, String *, Cardinal *);

static void 
fe_ab_undo_action (Widget, XEvent *, String *, Cardinal *);

static void 
fe_ab_redo_action (Widget, XEvent *, String *, Cardinal *);

static void 
fe_ab_deleteItem_action (Widget, XEvent *, String *, Cardinal *);

static void 
fe_ab_mailMessage_action (Widget, XEvent *, String *, Cardinal *);

static void
fe_ab_commit_list(MWContext *ab_context, XP_Bool newlist);

static void
fe_ab_commit_user(MWContext *ab_context, XP_Bool newuser);

static void 
fe_ab_saveAs_action (Widget, XEvent *, String *, Cardinal *);

static void 
fe_ab_close_action (Widget, XEvent *, String *, Cardinal *);

static void
fe_ab_make_edituser_per (MWContext *);

static void
fe_ab_make_edituser_sec (MWContext *);

static void
fe_ab_PopupUserPropertyWindow(MWContext *, ABID entry, XP_Bool newuser);

static void
fe_ab_PopupListPropertyWindow(MWContext *, ABID entry, XP_Bool newlist);

static void
fe_ab_PopupFindWindow(MWContext *, XP_Bool again);



typedef struct fe_ab_clipboard {
  void *block;
  int32 length;
} fe_ab_clipboard;


typedef struct fe_addrbk_data 
{

  ABPane *abpane;
  ABID editUID;
  ABID editLID;

  Widget outline;

  Widget edituser;              /* Edit user container widget */

  Widget edituser_p;            /* Personal Folder */
  Widget edituser_p_fn;
  Widget edituser_p_mi;
  Widget edituser_p_ln;
  Widget edituser_p_org;
  Widget edituser_p_desp;
  Widget edituser_p_locality;
  Widget edituser_p_region;
  Widget edituser_p_nickname;
  Widget edituser_p_em;

  Widget edituser_b;            /* Business Folder */
  Widget edituser_b_text;

  Widget editlist;              /* Edit list container widget */
  Widget editlist_m;
  Widget editlist_m_nickname;
  Widget editlist_m_name;
  Widget editlist_m_desp;
  Widget editlist_m_members;

  Widget title;
  Widget nickname;
  Widget name;
  Widget lname;
  Widget mname;
  Widget location;		/* email address */
  Widget locationlabel;
  Widget lastvisited;
  Widget lastvisitedlabel;
  Widget addedon;
  Widget aliaslabel;
  Widget aliasbutton;

  Widget editshell;

  /* tool bar stuff */
  Widget queryText;
  Widget helpBar;

  fe_ab_clipboard clip;	        /* Bookmarks own private clipboard */


  Widget findshell;           /* Find stuff */
  Widget findtext;
  Widget findnicknameT;
  Widget findnameT;
  Widget findlocationT;
  Widget finddescriptionT;
  Widget findcaseT;
  Widget findwordT;

  Widget popup;			/* Bookmark popup menu */
} fe_addrbk_data;


static char* addressBookHeaders[] = {
    "Type", "Name", "Nickname", "Email", "Company", "Locality"
};

static XtActionsRec fe_addressBookActions [] =
{
  { "ABPopup",		        fe_ab_popup_menu_action	},
  { "ABcut",			fe_ab_cut_action		},
  { "ABcopy",			fe_ab_copy_action	},
  { "ABpaste",			fe_ab_paste_action	},
  { "ABfind",			fe_ab_find_action	},
  { "ABfindAgain",		fe_ab_findAgain_action	},
  { "ABundo",			fe_ab_undo_action	},
  { "ABredo",			fe_ab_redo_action	},
  { "ABdeleteItem",		fe_ab_deleteItem_action	},
  { "ABmailMessage",		fe_ab_mailMessage_action},
  { "ABsaveAs",			fe_ab_saveAs_action	},
  { "ABclose",			fe_ab_close_action	},
};


/* Benjie */
extern int XFE_ESTIMATED_TIME_REMAINING;
extern int XFE_ESTIMATED_TIME_REMAINING_CHECKED;
extern int XFE_ESTIMATED_TIME_REMAINING_CHECKING;
extern int XFE_DONE_CHECKING_ETC;
extern int XFE_GG_EMPTY_LL;


/*****************************************************
 * Actual code begins here 
 */

void FE_InitAddrBook() {

  char tmp[1024];
  char* home = getenv("HOME");

  if (!home) home = "";

  /* database file is not html anymore ... */
   PR_snprintf(tmp, sizeof (tmp), "%.900s/.netscape/addrbook.db", home); 
      
   AB_InitAddressBook(tmp, &AddrBook);
}

void FE_CloseAddrBook() {
  AB_CloseAddressBook(&AddrBook);
}


/* Frontend version of GetAddressBook - deals with the mwcontext instead
 * of pane as specified in the backend. The backend version of GetAddressBook
 * calls this routine 
 */
MWContext*
fe_GetAddressBook(MWContext *mwcontext, XP_Bool viewnow)
{
  MWContext* context = mwcontext;
  ABPane *abpane = NULL;

  if (!ABfeContext && context) {
    XtAppAddActions (fe_XtAppContext, fe_addressBookActions,
			countof(fe_addressBookActions));


    ABfeContext = fe_MakeWindow(XtParent(CONTEXT_WIDGET(context)),
					NULL, NULL, NULL, MWContextAddressBook,
					TRUE);
  }

  if (ABfeContext) {  
    fe_addrbk_data* d = AB_DATA(ABfeContext);
    AB_InitAddressBookPane(&abpane, AddrBook, ABfeContext, 
			   NULL, AB_SortByFullNameCmd, True);
    d->abpane = abpane;
  }
  if (ABfeContext && viewnow)
    XtPopup (CONTEXT_WIDGET (ABfeContext), XtGrabNone);

  return ABfeContext;
}


void 
fe_ab_property_cb(Widget widget, XtPointer closure, XtPointer call_data) 
{
  MWContext *ab_context = (MWContext*) closure;
  int count;
  MsgViewIndex *rowPos;
  ABID entry;
  ABID type;

  count = XmLGridGetSelectedRowCount(AB_DATA(ab_context)->outline);
  if (count > 0) {
    rowPos = (MsgViewIndex *) malloc (sizeof(MsgViewIndex) * count);
    if (XmLGridGetSelectedRows
	(AB_DATA(ab_context)->outline, rowPos, count)!=-1) {
  	entry = AB_GetEntryIDAt(AB_DATA(ab_context)->abpane, (uint) rowPos[0]);
	AB_GetType(AddrBook, entry, &type);
	if (type == ABTypePerson)
	  fe_ab_PopupUserPropertyWindow(ab_context, entry, False);
	else if (type == ABTypeList)
	  fe_ab_PopupListPropertyWindow(ab_context, entry, False);
    }
  }
}


void 
fe_ab_newuser_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *ab_context = (MWContext*) closure;
  fe_ab_PopupUserPropertyWindow(ab_context, -1, True);
}

void
fe_ab_newlist_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *ab_context = (MWContext*) closure;
  fe_ab_PopupListPropertyWindow(ab_context, -1, True);
}

void
fe_ab_delete_cb(Widget widget, XtPointer closure, XtPointer call_data) 
{
  MWContext *ab_context = (MWContext*) closure;
  fe_ab_deleteItem(ab_context);
}


void
fe_ab_destroy_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext *ab_context = (MWContext*) closure;
  fe_addrbk_data* d = AB_DATA(ab_context);
  d->findshell = 0;
  d->edituser = 0;
  d->editlist = 0;
  AB_CloseAddressBookPane(&(d->abpane));
  d->abpane = NULL;
}

static void
fe_ab_pulldown_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* ab_context = (MWContext*) closure;
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
      AB_CommandType cmd;
      XP_Bool enable, plural;
      MSG_COMMAND_CHECK_STATE sState;
      MsgViewIndex *indices;
      int count;

      XtVaGetValues(item, XmNuserData, &value, 0);
      cmd = (AB_CommandType) value;
      count = XmLGridGetSelectedRowCount(AB_DATA(ab_context)->outline);
      if (count != 0) {
	indices = (MsgViewIndex *) malloc (sizeof(MsgViewIndex) * count);
	XmLGridGetSelectedRows
	  (AB_DATA(ab_context)->outline, indices, count);
      }
      AB_CommandStatus(AB_DATA(ab_context)->abpane, cmd, indices, count, 
		       &enable, &sState, NULL, &plural);
      XtVaSetValues(item, XmNsensitive, enable, 0);
    }
  }
}



static Boolean
fe_ab_datafunc(Widget widget, void* closure, int row, fe_OutlineDesc* data)
{
  static char *separator_string = "--------------------";
  static char namestring[256];
  static char nicknamestring[256];
  static char emailstring[256];
  static char compstring[256];
  static char localitystring[256];
  ABID type, entry;
  int count;
  MWContext* ab_context = (MWContext*) closure;
  fe_addrbk_data* d = AB_DATA(ab_context);

  if (d->abpane == NULL) return False;

  AB_GetEntryCount (AddrBook, &count, ABTypeAll);

  entry = AB_GetEntryIDAt(d->abpane, row);

  if (entry <= 0 || entry > 9999) return False;


  /* we have list poping up in new windows now */
  data->depth = 0;

  data->tag = XmFONTLIST_DEFAULT_TAG;

  AB_GetType(AddrBook, entry, &type);

  if (type == ABTypeList) {
    data->flippy = FE_OUTLINE_Folded;
    data->icon = FE_OUTLINE_ClosedList;
  } 

  else {
    data->flippy = FE_OUTLINE_Leaf;
    data->icon = FE_OUTLINE_Address;
  }

  data->type[0] = FE_OUTLINE_String;

  {
      char name[1024];
      char nickname[1024];
      char company[1024];
      char locality[1024];
      char email[1024];
      unsigned char *namelocale;
      unsigned char *nicknamelocale;
      unsigned char *complocale;
      unsigned char *loclocale;
      unsigned char *emaillocale;

      AB_GetFullName(AddrBook, entry, name);
      namelocale = fe_ConvertToLocaleEncoding
	(INTL_DefaultWinCharSetID(NULL), name);
      PR_snprintf(namestring, sizeof(namestring), "%s", namelocale);
      if ((char *) namelocale != name)
	{
	  XP_FREE(namelocale);
	}
      data->str[0] = namestring;

      AB_GetNickname(AddrBook, entry, nickname);
      nicknamelocale = fe_ConvertToLocaleEncoding
	(INTL_DefaultWinCharSetID(NULL), nickname);
      PR_snprintf(nicknamestring, sizeof(nicknamestring), "%s", nicknamelocale);
      if ((char *) nicknamelocale != nickname)
	{
	  XP_FREE(nicknamelocale);
	}
      data->str[1] = nicknamestring;

      if (type == ABTypePerson) {
	AB_GetEmailAddress(AddrBook, entry, email);
	emaillocale = fe_ConvertToLocaleEncoding
	  (INTL_DefaultWinCharSetID(NULL), email);
	PR_snprintf(emailstring, sizeof(emailstring), "%s", emaillocale);
	if ((char *) emaillocale != email)
	  {
	    XP_FREE(emaillocale);
	  }
	data->str[2] = emailstring;
      
	AB_GetCompanyName(AddrBook, entry, company);
	complocale = fe_ConvertToLocaleEncoding
	  (INTL_DefaultWinCharSetID(NULL), company);
	PR_snprintf(compstring, sizeof(compstring), "%s", complocale);
	if ((char *) complocale != company)
	  {
	    XP_FREE(complocale);
	  }
	data->str[3] = compstring;
	
	AB_GetLocality(AddrBook, entry, locality);
	loclocale = fe_ConvertToLocaleEncoding
	  (INTL_DefaultWinCharSetID(NULL), locality);
	PR_snprintf(localitystring, sizeof(localitystring), "%s", loclocale);
	if ((char *) loclocale != locality)
	  {
	    XP_FREE(loclocale);
	  }
	data->str[4] = localitystring;
      } else {
	data->str[2] = NULL;
	data->str[3] = NULL;
	data->str[4] = NULL;
      }
  }

  return True;
}


/* Addressbook Drop Functions */
static void
fe_ab_dropfunc(Widget dropw, void* closure, fe_dnd_Event type,
	       fe_dnd_Source* source, XEvent* event)
{
  MWContext* ab_context = (MWContext*) closure;
  int row;
  XP_Bool under = FALSE;
  

  /* Benjie */

  if (source->type != FE_DND_BOOKMARK) return;

  /* Right now, we only handle drops that originated from us.  This should
     change eventually... We should add motif drag and drop */
     
  if (source->closure != AB_DATA(ab_context)->outline) {
    return;
  }

  fe_OutlineHandleDragEvent(AB_DATA(ab_context)->outline, event, type, source);

  row = fe_OutlineRootCoordsToRow(AB_DATA(ab_context)->outline,
				  event->xbutton.x_root,
				  event->xbutton.y_root,
				  &under);

  fe_OutlineSetDragFeedback(AB_DATA(ab_context)->outline, row,
			    BM_IsDragEffectBox(ab_context, row + 1, under));

  if (type == FE_DND_DROP && row >= 0) {
    /* BM_DoDrop(ab_context, row + 1, under); */
  }
  if (type == FE_DND_END) {
    fe_OutlineSetDragFeedback(AB_DATA(ab_context)->outline, -1, FALSE);
  }
  return;
}


static void
fe_ab_clickfunc(Widget widget, void* closure, int row, int column,
		char* header, int button, int clicks,
		Boolean shift, Boolean ctrl)
{
  MWContext* ab_context = (MWContext*) closure;
  fe_addrbk_data* d = AB_DATA(ab_context);
  ABID type;
  ABID entry;

  if (row < 0) {
    if (column == 0) 
      AB_Command(d->abpane, AB_SortByTypeCmd, NULL, 0);
    else if (column == 1)
      AB_Command(d->abpane, AB_SortByFullNameCmd, NULL, 0);
    else if (column == 2)
      AB_Command(d->abpane, AB_SortByNickname, NULL, 0);
    else if (column == 3)
      AB_Command(d->abpane, AB_SortByEmailAddress, NULL, 0);
    else if (column == 4)
      AB_Command(d->abpane, AB_SortByCompanyName, NULL, 0);
    else if (column == 5)
      AB_Command(d->abpane, AB_SortByLocality, NULL, 0);
    
    return;
  } else {
    entry = AB_GetEntryIDAt(d->abpane, row);
    
    if (entry<=0) return;
    
    AB_GetType(AddrBook, entry, &type);
    
    if (clicks == 1) {
      if ((d->edituser!=NULL && XtIsManaged(d->edituser))
	  || (d->editlist != NULL && XtIsManaged(d->editlist))) {
	if (type == ABTypePerson)
	  fe_ab_PopupUserPropertyWindow(ab_context, entry, False);
	else
	  fe_ab_PopupListPropertyWindow(ab_context, entry, False);
      }
    }
    
    if (clicks == 2) {
      if (type == ABTypePerson)
	fe_ab_PopupUserPropertyWindow(ab_context, entry, False);
      else
	fe_ab_PopupListPropertyWindow(ab_context, entry, False);
    }
  }
}


static void
fe_ab_iconfunc(Widget widget, void* closure, int row)
{
  MWContext* ab_context = (MWContext*) closure;
}



/* This is for dragging from addressbook and dropping onto compose window */
#define XFE_CHUNK_SIZE 1024
static void
fe_collect_addresses(MWContext *context, BM_Entry *entry, void *closure)
{
  char **newstrp = (char **)closure;
  int oldlen = XP_STRLEN(*newstrp), nchunk, newlen;
  char *str;

  /* Benjie */
  /* str = BM_GetFullAddress(context, entry); */
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
  MWContext* ab_context;

  /* Benjie */
  if (source->type == FE_DND_MAIL_MESSAGE ||
      source->type == FE_DND_NEWS_MESSAGE)
    {
      fe_attach_dropfunc (dropw, closure, type, source, event);
      return;
    }

  ab_context = fe_GetAddressBook(NULL, False);

  /* Not our drag ask an addressbook context doesn't even exist */
  if (!ab_context) return;

  if ((source->type != FE_DND_BOOKMARK)
      || (source->closure != AB_DATA(ab_context)->outline)) return;

  if (type == FE_DND_DROP) {
    char *oldstr = XmTextGetString(dropw);
    int oldlen = oldstr ? XP_STRLEN(oldstr) : 0;
    char *newstr = (char *)
      XP_ALLOC(XFE_CHUNK_SIZE * ((oldlen / XFE_CHUNK_SIZE) + 1));
    newstr[0] = '\0';
    if (oldstr && *oldstr) {
      XP_STRCPY(newstr, oldstr);
    }
    /* Benjie */
    /* BM_EachProperSelectedEntryDo(ab_context, fe_collect_addresses, &newstr,
				 NULL); */
    XmTextSetString(dropw, newstr);
    /* Expand the nickname instantly */
    XtCallCallbacks(dropw, XmNlosingFocusCallback, NULL);
    if (oldstr) XtFree(oldstr);
    XP_FREE(newstr);
  }

  return;
}

static void
fe_ab_edituser_destroy(Widget widget, 
		   XtPointer closure, 
		   XtPointer call_data)
{
  MWContext* ab_context = (MWContext*) closure;
  fe_addrbk_data* d = AB_DATA(ab_context);
  if (d && d->edituser) {
    d->edituser = NULL;
  }
}

static void
fe_ab_editlist_destroy(Widget widget, 
		   XtPointer closure, 
		   XtPointer call_data)
{
  MWContext* ab_context = (MWContext*) closure;
  fe_addrbk_data* d = AB_DATA(ab_context);
  if (d && d->editlist) {
    d->editlist = NULL;
  }
}


static void
fe_ab_clean_text(MWContext* ab_context, char *str, Boolean new_lines_too_p)
{
  fe_forms_clean_text(ab_context, ab_context->win_csid, str, new_lines_too_p);
}


static void
fe_ab_editlist_close(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* ab_context = (MWContext*) closure;
  int count;
  AB_DATA(ab_context)->editLID = -1;
  XtUnmanageChild(AB_DATA(ab_context)->editlist);
  AB_GetEntryCount (AddrBook, &count, ABTypeAll);
  fe_OutlineChange(AB_DATA(ab_context)->outline, 0, count, count);
}

static void
fe_ab_edituser_close(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* ab_context = (MWContext*) closure;
  int count;
  AB_DATA(ab_context)->editUID = -1;
  XtUnmanageChild(AB_DATA(ab_context)->edituser);
  AB_GetEntryCount (AddrBook, &count, ABTypeAll);
  fe_OutlineChange(AB_DATA(ab_context)->outline, 0, count, count);
}

static void
fe_ab_editlist_ok(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* ab_context = (MWContext*) closure;
  fe_ab_commit_list(ab_context, False);
  fe_ab_editlist_close(widget, closure, call_data);
}

static void
fe_ab_edituser_ok(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* ab_context = (MWContext*) closure;
  fe_ab_commit_user(ab_context, False);
  fe_ab_edituser_close(widget, closure, call_data);
}

static void
fe_ab_newlist_ok(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* ab_context = (MWContext *) closure;
  fe_ab_commit_list(ab_context, True);
  fe_ab_editlist_close(widget, closure, call_data); 
}

static void
fe_ab_commit_list(MWContext *ab_context, XP_Bool newlist)
{
  fe_addrbk_data* d = AB_DATA(ab_context);
  MailingListEntry entry;
  ABID eID;

  entry.pNickName = NULL;
  entry.pFullName = NULL;
  entry.pInfo = NULL;
  entry.pNickName = XmTextFieldGetString(d->editlist_m_nickname);
  entry.pFullName = XmTextFieldGetString(d->editlist_m_name);
  entry.pInfo = XmTextFieldGetString(d->editlist_m_desp);

  if (newlist)
    AB_AddMailingList(AddrBook, &entry, &eID);
  else {
    eID = d->editLID;
    AB_ModifyMailingList(AddrBook, eID, &entry);
  }

  XP_FREE (entry.pNickName);
  XP_FREE (entry.pFullName);
  XP_FREE (entry.pInfo);
}

static void
fe_ab_newuser_ok(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* ab_context = (MWContext *) closure;
  fe_ab_commit_user(ab_context, True);
  fe_ab_edituser_close(widget, closure, call_data); 
}

static void
fe_ab_commit_user(MWContext *ab_context, XP_Bool newuser)
{
  fe_addrbk_data* d = AB_DATA(ab_context);
  PersonEntry entry;
  ABID eID;

  /* setting up the defaults */
  entry.pCompanyName = NULL;
  entry.pLocality = NULL;
  entry.pRegion = NULL;
  entry.pInfo = NULL;
  entry.HTMLmail = FALSE;
  entry.pNickName = NULL;
  entry.pGivenName = NULL;
  entry.pFamilyName = NULL;
  entry.pMiddleName = NULL;
  entry.pEmailAddress = NULL;

  entry.pNickName = XmTextFieldGetString(d->edituser_p_nickname);
  entry.pGivenName = XmTextFieldGetString(d->edituser_p_fn);
  entry.pFamilyName = XmTextFieldGetString(d->edituser_p_ln);
  entry.pMiddleName = XmTextFieldGetString(d->edituser_p_mi);
  entry.pEmailAddress = XmTextFieldGetString(d->edituser_p_em);
  entry.pCompanyName = XmTextFieldGetString(d->edituser_p_org);
  entry.pLocality = XmTextFieldGetString(d->edituser_p_locality);
  entry.pRegion = XmTextFieldGetString(d->edituser_p_region);
  entry.pInfo = XmTextGetString(d->edituser_p_desp);

  if (newuser)
    AB_AddUser(AddrBook, &entry, &eID);
  else {
    eID = d->editUID;
    AB_ModifyUser(AddrBook, eID, &entry);
  }

  XP_FREE (entry.pNickName);
  XP_FREE (entry.pGivenName);
  XP_FREE (entry.pFamilyName);
  XP_FREE (entry.pMiddleName);
  XP_FREE (entry.pEmailAddress);
  XP_FREE (entry.pLocality);
  XP_FREE (entry.pRegion);
  XP_FREE (entry.pCompanyName);
  XP_FREE (entry.pInfo);
}


static void
fe_ab_close(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* ab_context = (MWContext*) closure;

  XtPopdown(CONTEXT_WIDGET(ab_context));
}


static void
fe_ab_popdown(Widget widget, XtPointer closure, XtPointer call_data)
{
  uint count;
  MWContext* ab_context = (MWContext*) closure;
  fe_addrbk_data* d = AB_DATA(ab_context);

  /* close the pane */
  AB_GetEntryCount (AddrBook, &count, ABTypePerson);
  AB_CloseAddressBookPane(&(d->abpane));
  d->abpane = NULL;
  fprintf(stderr,"%d users read in addrbook", count);
  
  if (AB_DATA(ab_context)->edituser)
    fe_ab_edituser_close(widget, closure, call_data);
  if (AB_DATA(ab_context)->editlist)
    fe_ab_editlist_close(widget, closure, call_data);
}


static void
fe_ab_cmd(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* ab_context = (MWContext*) closure;
  struct fe_addrbk_data *d = AB_DATA(ab_context);
  XtPointer userdata = 0;
  AB_CommandType abcmd;
  MsgViewIndex *indices;
  int count;

  XtVaGetValues(widget, XmNuserData, &userdata, 0);
  abcmd = (AB_CommandType) userdata;
  switch (abcmd) {
  case AB_NewMessageCmd:
    count = XmLGridGetSelectedRowCount(AB_DATA(ab_context)->outline);
    indices = (MsgViewIndex *) malloc (sizeof(MsgViewIndex) * count);
    XmLGridGetSelectedRows (d->outline, indices, count);
    AB_Command(d->abpane, abcmd, indices, count);
    break;
  case AB_FindCmd:
    fe_ab_PopupFindWindow(ab_context, False);
    break;
  case AB_FindAgainCmd:
    fe_ab_PopupFindWindow(ab_context, True);
    break;
  case AB_AddUserCmd:
    fe_ab_PopupUserPropertyWindow(ab_context,-1, True);
    break;
  case AB_AddMailingListCmd:
    fe_ab_PopupListPropertyWindow(ab_context, -1, True);
    break;
  case AB_PropertiesCmd:
    fe_ab_property_cb(widget, closure, call_data);
    break;
  case AB_DeleteCmd:
    fe_ab_deleteItem(ab_context);
    break;
  case AB_SortByTypeCmd:
  case AB_SortByFullNameCmd:
  case AB_SortByLocality:
  case AB_SortByNickname:
  case AB_SortByEmailAddress:
  case AB_SortByCompanyName:
  case AB_SortAscending:
  case AB_SortDescending:
    AB_Command(d->abpane, abcmd, indices, 0);
    break;
  default:
    break;
  }

}


static void
XtUnmanageChild_safe (Widget w)
{
  if (w) XtUnmanageChild (w);
}

/* Benjie */
static struct fe_button addressbookPopupMenudesc[] = {
  { "mailMessage",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_NewMessageCmd},
  { "properties",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_PropertiesCmd},
  { 0 },
};

static struct fe_button addressbookMenudesc[] = {
  { "file",		0,			SELECTABLE,	0 },
  { "mailMessage",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_NewMessageCmd},
  { "import",	fe_ab_cmd,			SELECTABLE,	0, 0,
    (void*) AB_ImportCmd},/**/
  { "saveAs",		fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_SaveCmd},
  { "",			0,			UNSELECTABLE,	0 },
  { "delete",		fe_ab_close,		SELECTABLE,	0, 0,
    (void*) AB_CloseCmd},
  { 0 },
  { "edit",		0,			SELECTABLE,	0 },
  { "undo",		fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_UndoCmd},
  { "redo",		fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_RedoCmd},
  { "",			0,			UNSELECTABLE,	0 },
  { "deleteItem",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_DeleteCmd},
  { "",			0,			UNSELECTABLE,	0 },
  { "find",		fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_FindCmd},
  { "findAgain",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_FindAgainCmd},
  { 0 },
  { "View",		0,			SELECTABLE,	0 },
  { "By Type",		fe_ab_cmd, 	        SELECTABLE,	0, 0,
    (void*) AB_SortByTypeCmd},
  { "By Name",		fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_SortByFullNameCmd},
  { "By Email Address",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_SortByLocality},
  { "By Company",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_SortByNickname},
  { "By Locality",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_SortByEmailAddress},
  { "By Nickname",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_SortByCompanyName},
  { "",			0,			UNSELECTABLE,	0 },
  { "Sort Ascending",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_SortAscending},
  { "Sort Descending",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_SortDescending},
  { 0 },
  { "Item",		0,			SELECTABLE,	0 },
  { "addUser",		fe_ab_cmd, 	        SELECTABLE,	0, 0,
    (void*) AB_AddUserCmd},
  { "addList",		fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_AddMailingListCmd},
  { "",			0,			UNSELECTABLE,	0 },
  { "properties",	fe_ab_cmd,		SELECTABLE,	0, 0,
    (void*) AB_PropertiesCmd},
  { 0 },
  { 0 }
};



/*
 * fe_make_ab_popup
 *
 * Makes addressbook popup 
 */
static void
fe_make_ab_popup(MWContext* ab_context)
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

  parent = CONTEXT_WIDGET(ab_context);
  popup = AB_DATA(ab_context)->popup;
  if (popup) return;

  menudesc = addressbookPopupMenudesc;
  nummenu = countof(addressbookPopupMenudesc);

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
	XtAddCallback (item, XmNactivateCallback, button.callback, ab_context);
    }
    XtManageChild(item);
  }
  AB_DATA(ab_context)->popup = popup;
}


void
fe_MakeAddressBookWidgets(Widget shell, MWContext *context)
{
  static char* columnStupidity = NULL; /* Sillyness to give the outline
					  code a place to write down column
					  width stuff that will never change
					  and that we don't care about. */

  Arg av [20];
  int ac = 0;
  Pixel pix = 0;

  Widget mainw;
  Widget menubar = 0;
  Widget toparea = 0;
  Widget toolbar = 0;
  
  Widget queryFrame = 0;
  Widget queryLabel = 0;
  Widget queryText = 0;

  /* Used as a place holder */
  Widget dummy = 0;

  static int columnwidths[] = {25, 20, 20, 20, 20};
  struct fe_button *menudesc;


  fe_ContextData* data = CONTEXT_DATA(context);
  fe_addrbk_data* d;

  uint count;

  menudesc = addressbookMenudesc;


  /* Allocate fontend data */
  d = AB_DATA(context) = XP_NEW_ZAP(fe_addrbk_data);

  data->widget = shell;

  /* setting stuff to null, don't remove these */
  d->editlist = NULL;
  d->edituser = NULL;
  d->findshell = NULL;

  /* When addrbk context get pops down, we need to save them */
  XtAddCallback(shell, XmNpopdownCallback, fe_ab_popdown, context);

  XmGetColors (XtScreen (shell),
               fe_cmap(context),
               data->default_bg_pixel,
               &data->fg_pixel,
               &data->top_shadow_pixel,
               &data->bottom_shadow_pixel,
               &pix);

  /* "mainw" is the parent of the top level of vertically stacked areas. */
  ac = 0;
  XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
  mainw = XmCreateForm (shell, "mainform", av, ac);

  menubar = fe_PopulateMenubar(mainw, menudesc, context, context,
			       NULL, fe_ab_pulldown_cb);

  ac = 0;
  XtSetArg (av[ac], XmNshadowThickness, 0); ac++;
  toparea = XmCreateFrame(mainw, "toolbarFrame", av, ac);
  toolbar = fe_MakeToolbar(toparea, context, False);
  XtManageChild(toolbar);

  ac = 0;
  XtSetArg (av[ac], XmNshadowThickness, 0); ac++;
  queryFrame = XmCreateFrame(mainw, "queryFrame", av, ac);
 
  ac = 0;
  XtSetArg (av[ac], XmNorientation, XmVERTICAL); ac++;
  dummy = XmCreateRowColumn(queryFrame, "queryRowCol", av, ac);
  XtManageChild(dummy);
  
  ac = 0;
  queryLabel = XmCreateLabelGadget(dummy, "queryLabel", av, ac);
  XtManageChild(queryLabel);

  ac = 0;
  /* save ourselves, and hopefully no one's name is greater than 100 */
  XtSetArg (av[ac], XmNmaxLength, 100); ac++;
  /* should we specify this? */ 
  XtSetArg (av[ac], XmNwidth, 340); ac++;
  queryText = XmCreateText(dummy, "queryText", av, ac);
  XtManageChild(queryText);
  d->queryText = queryText;

  ac = 0;
  if (columnStupidity) {
    XP_FREE(columnStupidity);
    columnStupidity = NULL;
  }
  d->outline = fe_OutlineCreate(context, mainw, "ablist", av, ac,
				0, 5, columnwidths, fe_ab_datafunc,
				fe_ab_clickfunc, fe_ab_iconfunc, context,
				/* Benjie */
				FE_DND_BOOKMARK, &columnStupidity);

  fe_dnd_CreateDrop(d->outline, fe_ab_dropfunc, context);
  fe_OutlineSetHeaders(d->outline, addressBookHeaders);  

  AB_GetEntryCount (AddrBook, &count, ABTypeAll);
  fe_OutlineChange(d->outline, 0, count, count);

  XtVaSetValues(menubar,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		0);

  XtVaSetValues(toparea,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, menubar,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		0);

  XtVaSetValues(queryFrame,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, toparea,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		0);

  XtVaSetValues(d->outline,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, queryFrame,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		0);
  
  XtManageChild (toparea);
  XtManageChild (d->outline);
  XtManageChild (menubar);
  XtManageChild (queryFrame);
  XtManageChild (mainw);

  data->menubar = menubar;
  XtVaSetValues (shell, XmNinitialFocus, mainw, 0);
  XtVaSetValues (mainw, XmNinitialFocus, d->outline, 0);

  fe_HackTranslations (context, shell);

  fe_make_ab_popup(context);
}


/* Find */

/* Benjie */
#if 0
static void
fe_ab_find_setvalues(MWContext* ab_context, BM_FindInfo* findInfo)
{
  fe_addrbk_data* d = AB_DATA(ab_context);
  XtVaSetValues(d->findtext, XmNvalue, findInfo->textToFind, 0);
  if (d->findnicknameT) {
    XtVaSetValues(d->findnicknameT, XmNset, findInfo->checkNickname, 0);
  }
  XtVaSetValues(d->findnameT, XmNset, findInfo->checkName, 0);
  XtVaSetValues(d->findlocationT, XmNset, findInfo->checkLocation, 0);
  XtVaSetValues(d->findcaseT, XmNset, findInfo->matchCase, 0);
  XtVaSetValues(d->findwordT, XmNset, findInfo->matchWholeWord, 0);
}
#endif

/* Benjie */
#if 0
static void
fe_ab_find_getvalues(MWContext* ab_context, AB_FindInfo* findInfo)
{
  fe_addrbk_data* d = AB_DATA(ab_context);
  /* Should we free findInfo->textToFind */
  XP_FREE(findInfo->textToFind);
  XtVaGetValues(d->findtext, XmNvalue, &findInfo->textToFind, 0);
  if (d->findnicknameT) {
    XtVaGetValues(d->findnicknameT, XmNset, &findInfo->checkNickname, 0);
  }
  XtVaGetValues(d->findnameT, XmNset, &findInfo->checkName, 0);
  XtVaGetValues(d->findlocationT, XmNset, &findInfo->checkLocation, 0);
  /* XtVaGetValues(d->finddescriptionT, XmNset, &findInfo->checkDescription, 0); */
  XtVaGetValues(d->findcaseT, XmNset, &findInfo->matchCase, 0);
  XtVaGetValues(d->findwordT, XmNset, &findInfo->matchWholeWord, 0);
}
#endif

static void
fe_ab_find_destroy_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* ab_context = (MWContext*) closure;
  fe_addrbk_data* d = AB_DATA(ab_context);
  if (d->findshell) {
    d->findshell = 0;
  }
}


static void
fe_ab_find_ok(Widget widget, XtPointer closure, XtPointer call_data)
{
  XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *) call_data;
  MWContext* ab_context = (MWContext*) closure;
  fe_addrbk_data* d = AB_DATA(ab_context);

  if (cb->reason == XmCR_OK) {
    /* Update FindInfo */
    /* BM_FindInfo* findinfo; */
    /* XtVaGetValues(d->findshell, XmNuserData, &findinfo, 0); */

    /* needs to get info from the dialog */
    /* fe_ab_find_getvalues(ab_context, findinfo); */
    XtUnmanageChild(d->findshell);
    /* BM_DoFindBookmark(ab_context, findinfo); */
  }
  else if (cb->reason == XmCR_APPLY) {
    /* Clear */
    /* BM_FindInfo* findinfo;
       XtVaGetValues(d->findshell, XmNuserData, &findinfo, 0);
       if (findinfo->textToFind)
       findinfo->textToFind[0] = '\0';
       XtVaSetValues(d->findtext, XmNvalue, findinfo->textToFind, 0);
       XmProcessTraversal (d->findtext, XmTRAVERSE_CURRENT); */
  }
  else if (cb->reason == XmCR_CANCEL)
    XtUnmanageChild(d->findshell);

  return;
}



MWContext*
FE_GetAddressBook(MSG_Pane *pane, XP_Bool viewnow)
{
  MWContext* context;
  if (!pane) return NULL;
  else {
    context = MSG_GetContext(pane);
    fe_GetAddressBook(context, viewnow);
  }
}




/*
 * Action routines
 */

static void
fe_ab_popup_menu_action (Widget widget, XEvent *event, String *cav,
				Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
  Widget popup = AB_DATA(ab_context)->popup;
  Widget *buttons;
  int nbuttons, i;

  XmMenuPosition (popup, (XButtonPressedEvent *) event);

  /* Sensitize the popup menu */
  XtVaGetValues(popup, XmNchildren, &buttons, XmNnumChildren, &nbuttons, 0);
  for (i=0 ; i<nbuttons ; i++) {
    Widget item = buttons[i];
    if (XmIsPushButton(item) || XmIsPushButtonGadget(item)) {
      XtPointer value;
      AB_CommandType cmd;
      XP_Bool enable, plural;
      MSG_COMMAND_CHECK_STATE sState;
      MsgViewIndex *indices;
      int count;

      cmd = (AB_CommandType) value;
      XtVaGetValues(item, XmNuserData, &value, 0);
      plural = FALSE;


      count = XmLGridGetSelectedRowCount(AB_DATA(ab_context)->outline);
      if (count != 0) {
	indices = (MsgViewIndex *) malloc (sizeof(MsgViewIndex) * count);
	XmLGridGetSelectedRows
	  (AB_DATA(ab_context)->outline, indices, count);
      }
      AB_CommandStatus(AB_DATA(ab_context)->abpane, cmd, indices, count, 
		       &enable, &sState, NULL, &plural);
      XtVaSetValues(item, XmNsensitive, enable, 0);
    }
  }
  XtManageChild (AB_DATA(ab_context)->popup);
}



static void
fe_ab_find_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
}



static void
fe_ab_findAgain_action (Widget widget, XEvent *event, 
			String *cav, Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
}


static void
fe_ab_undo_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
}


static void
fe_ab_redo_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
}


static void
fe_ab_deleteItem_action (Widget widget, XEvent *event, String *cav,
				Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
  fe_ab_deleteItem(ab_context);
}

static void
fe_ab_deleteItem(MWContext *ab_context)
{
  int count, i;
  MsgViewIndex *rowPos;

  count = XmLGridGetSelectedRowCount(AB_DATA(ab_context)->outline);
  if (count) {
    rowPos = (MsgViewIndex *) malloc (sizeof(MsgViewIndex) * count);
    if (XmLGridGetSelectedRows
	(AB_DATA(ab_context)->outline, rowPos, count)!=-1) {
      /* something in here */
    }
  }

  /* return 0 on err_successful */
  AB_Command(AB_DATA(ab_context)->abpane, AB_DeleteCmd, 
	     rowPos, count);
  free((char*)rowPos);
}

static void
fe_ab_mailMessage_action (Widget widget, XEvent *event, String *cav,
				Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
  AB_Command(AB_DATA(ab_context)->abpane, AB_NewMessageCmd, NULL, 0);
}


static void
fe_ab_saveAs_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
}


static void
fe_ab_close_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
  fe_ab_close(widget, (XtPointer) ab_context, NULL);
}


static void
fe_ab_cut_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
}


static void
fe_ab_copy_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
}

static void
fe_ab_paste_action (Widget widget, XEvent *event, String *cav, Cardinal *cac)
{
  MWContext *ab_context = fe_WidgetToMWContext(widget);
}


static void
fe_ab_PopupUserPropertyWindow(MWContext *ab_context, 
			      ABID entry, XP_Bool newuser)
{
  fe_addrbk_data* d = AB_DATA(ab_context);

  int ac;
  Arg av[20];
  Widget kids[20];
  int i;
  XmString str;
  Widget form = 0;
  Widget dialog = 0;
  Widget tmp = 0;
  char s[1024];

  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  
  if (! d->edituser) {
    XtVaGetValues(CONTEXT_WIDGET(ab_context), XtNvisual, &v,
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
    
    
    d->edituser = XmCreatePromptDialog(CONTEXT_WIDGET(ab_context),
				       "addressBProps", av, ac);
    
    dialog = d->edituser;   /* too much typing */
    
    fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
    fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
    fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
    fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog,
						  XmDIALOG_SELECTION_LABEL));
    
    /* needs to change these */
    XtAddCallback(dialog, XmNdestroyCallback, fe_ab_edituser_destroy, 
		  ab_context);
    XtAddCallback(dialog, XmNcancelCallback, fe_ab_edituser_close, 
		  ab_context);
    XtAddCallback(dialog, XmNokCallback, fe_ab_edituser_ok, ab_context);
    
    form = XtVaCreateManagedWidget("form",
				   xmlFolderWidgetClass, dialog,
				   XmNshadowThickness, 2,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNleftAttachment, XmATTACH_FORM,
				   XmNrightAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   XmNbottomOffset, 3,
				   XmNspacing, 1,
				   NULL);
    
#define ADD_TAB(VAR,STRING) \
    str = XmStringCreateLocalized (STRING); \
    tmp = XmLFolderAddTabForm (form, str); \
    XmStringFree (str); \
    d->VAR = tmp
			  
    ADD_TAB(edituser_p, "Properties");
    ADD_TAB(edituser_b, "Security");
#undef ADD_TAB
    
    fe_ab_make_edituser_per (ab_context);
    fe_ab_make_edituser_sec (ab_context);
    
    XtManageChild (form);
    fe_NukeBackingStore (dialog);   /* ??? Someone tell me what this does */
    XtManageChild (dialog);
    XmLFolderSetActiveTab (form, 0, True);
    
  }

  if (!newuser) {
    XtRemoveAllCallbacks(d->edituser, XmNokCallback);
    XtAddCallback(d->edituser, XmNokCallback, fe_ab_edituser_ok, ab_context);
    s[0] = '\0';
    AB_GetGivenName(AddrBook, entry, s);
    XtVaSetValues(d->edituser_p_fn, XmNvalue, s, 0);
    s[0] = '\0';
    AB_GetMiddleName(AddrBook, entry, s);
    XtVaSetValues(d->edituser_p_mi, XmNvalue, s, 0);
    s[0] = '\0';
    AB_GetFamilyName(AddrBook, entry, s);
    XtVaSetValues(d->edituser_p_ln, XmNvalue, s, 0);
    s[0] = '\0';
    AB_GetCompanyName(AddrBook, entry, s);
    XtVaSetValues(d->edituser_p_org, XmNvalue, s, 0);
    s[0] = '\0';
    AB_GetLocality(AddrBook, entry, s);
    XtVaSetValues(d->edituser_p_locality, XmNvalue, s, 0);
    s[0] = '\0';
    AB_GetRegion(AddrBook, entry, s);
    XtVaSetValues(d->edituser_p_region, XmNvalue, s, 0);
    s[0] = '\0';
    AB_GetEmailAddress(AddrBook, entry, s);
    XtVaSetValues(d->edituser_p_em, XmNvalue, s, 0);
    s[0] = '\0';
    AB_GetNickname(AddrBook, entry, s);
    XtVaSetValues(d->edituser_p_nickname, XmNvalue, s, 0); 
    s[0] = '\0';
    AB_GetInfo(AddrBook, entry, s);
    XtVaSetValues(d->edituser_p_desp, XmNvalue, s, 0); 
    d->editUID = entry;
  } else {
    XtRemoveAllCallbacks(d->edituser, XmNokCallback);
    XtAddCallback(d->edituser, XmNokCallback, fe_ab_newuser_ok, ab_context);
    s[0] = '\0';
    XtVaSetValues(d->edituser_p_fn, XmNvalue, s, 0);
    XtVaSetValues(d->edituser_p_mi, XmNvalue, s, 0);
    XtVaSetValues(d->edituser_p_ln, XmNvalue, s, 0);
    XtVaSetValues(d->edituser_p_org, XmNvalue, s, 0);
    XtVaSetValues(d->edituser_p_locality, XmNvalue, s, 0);
    XtVaSetValues(d->edituser_p_region, XmNvalue, s, 0);
    XtVaSetValues(d->edituser_p_em, XmNvalue, s, 0);
    XtVaSetValues(d->edituser_p_nickname, XmNvalue, s, 0); 
    XtVaSetValues(d->edituser_p_desp, XmNvalue, s, 0); 
  }
  XtManageChild (d->edituser);
  XMapRaised(XtDisplay(d->edituser), XtWindow(d->edituser));
}


static void
fe_ab_make_edituser_per(MWContext *ab_context)
{
  fe_addrbk_data* d = AB_DATA(ab_context);

  int ac;
  Arg av[20];

  /* this is a complicated window, 4 strips, each has its own members */
  Widget mainrowcol;
  Widget strip1;
  Widget strip2;
  Widget strip3;
  Widget strip4;
  Widget strip5;
  Widget strip6;

  Widget dummy;  /* my favorite widget: da place holda */
  Widget dummy1; /* my favorite widget: da place holda */
  Widget dummy2; /* my favorite widget: da place holda */
  Widget text;   /* the textlabel */

  ac = 0;
  XtSetArg (av[ac], XmNborderWidth, 0); ac++;
  dummy = XmCreateFrame(d->edituser_p, "edituserFrame", av, ac);
  XtManageChild(dummy);

  ac = 0;
  XtSetArg (av[ac], XmNwidth, 304); ac++;
  mainrowcol = XmCreateForm(dummy, "mainRowCol", av, ac);
  XtManageChild(mainrowcol);

  ac = 0;
  strip1 = XmCreateForm(mainrowcol, "strip1", av, ac);
  strip2 = XmCreateForm(mainrowcol, "strip2", av, ac);
  strip3 = XmCreateForm(mainrowcol, "strip3", av, ac);
  strip4 = XmCreateForm(mainrowcol, "strip4", av, ac);
  strip5 = XmCreateForm(mainrowcol, "strip5", av, ac);
  strip6 = XmCreateForm(mainrowcol, "strip6", av, ac);

  XtVaSetValues(strip1, XmNleftAttachment, XmATTACH_FORM,
		        XmNrightAttachment, XmATTACH_FORM,
		        XmNtopAttachment, XmATTACH_FORM,
		        0);
  XtVaSetValues(strip2, XmNleftAttachment, XmATTACH_FORM,
		        XmNrightAttachment, XmATTACH_FORM,
		        XmNtopAttachment, XmATTACH_WIDGET,
		        XmNtopWidget, strip1,
		        0);
  XtVaSetValues(strip3, XmNleftAttachment, XmATTACH_FORM,
		        XmNrightAttachment, XmATTACH_FORM,
		        XmNtopAttachment, XmATTACH_WIDGET,
		        XmNtopWidget, strip2,
		        0);
  XtVaSetValues(strip4, XmNleftAttachment, XmATTACH_FORM,
		        XmNrightAttachment, XmATTACH_FORM,
		        XmNtopAttachment, XmATTACH_WIDGET,
		        XmNtopWidget, strip3,
		        0);
  XtVaSetValues(strip5, XmNleftAttachment, XmATTACH_FORM,
		        XmNrightAttachment, XmATTACH_FORM,
		        XmNtopAttachment, XmATTACH_WIDGET,
		        XmNtopWidget, strip4,
		        0);
  XtVaSetValues(strip6, XmNleftAttachment, XmATTACH_FORM,
		        XmNrightAttachment, XmATTACH_FORM,
		        XmNtopAttachment, XmATTACH_WIDGET,
		        XmNtopWidget, strip5,
		        XmNbottomAttachment, XmATTACH_FORM,
		        0);
  XtManageChild(strip1);
  XtManageChild(strip2);
  XtManageChild(strip3);
  XtManageChild(strip4);
  XtManageChild(strip5);
  XtManageChild(strip6);

  /* now for each strip.... */
  /* when there is more time, I should investigate on the geometry
   * management, here it seems like Label Gadget is the dominating
   * factor and overrides the TextField in terms of width, the form
   * width correspond to that of the Label 
   */

  ac = 0;
  XtSetArg (av[ac], XmNwidth, 100); ac++;
  dummy = XmCreateForm(strip1, "fnbox", av, ac);
  XtManageChild(dummy);
  text = XtVaCreateManagedWidget ("First Name",
				  xmLabelGadgetClass, dummy,
				  XmNalignment, XmALIGNMENT_BEGINNING, 
				  NULL);
  ac = 0;
  d->edituser_p_fn = XmCreateTextField(dummy, "fnText", av, ac);
  XtManageChild(d->edituser_p_fn);
  XtVaSetValues(text, 
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		0);
  XtVaSetValues(d->edituser_p_fn, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, text,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		XmNbottomAttachment, XmATTACH_FORM, 
		0);


  ac = 0;
  XtSetArg (av[ac], XmNwidth, 100); ac++;
  dummy1 = XmCreateForm(strip1, "mibox", av, ac);
  XtManageChild(dummy1);
  text = XtVaCreateManagedWidget ("M.I.",
				  xmLabelGadgetClass, dummy1,
				  XmNalignment, XmALIGNMENT_BEGINNING, 
				  NULL);
  ac = 0;
  d->edituser_p_mi = XmCreateTextField(dummy1, "miText", av, ac);
  XtManageChild(d->edituser_p_mi);
  XtVaSetValues(text, 
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		0);
  XtVaSetValues(d->edituser_p_mi, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, text,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		XmNbottomAttachment, XmATTACH_FORM, 
		0);

  ac = 0;
  XtSetArg (av[ac], XmNwidth, 100); ac++;
  dummy2 = XmCreateForm(strip1, "lnbox", av, ac);
  XtManageChild(dummy2);
  text = XtVaCreateManagedWidget ("Last Name",
				  xmLabelGadgetClass, dummy2,
				  XmNalignment, XmALIGNMENT_BEGINNING, 
				  NULL);
  ac = 0;
  d->edituser_p_ln = XmCreateTextField(dummy2, "lnText", av, ac);
  XtManageChild(d->edituser_p_ln);
  XtVaSetValues(text, 
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		0);
  XtVaSetValues(d->edituser_p_ln, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, text,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		XmNbottomAttachment, XmATTACH_FORM, 
		0);

  XtVaSetValues(dummy, XmNleftAttachment, XmATTACH_FORM,
		        XmNbottomAttachment, XmATTACH_FORM,
		        XmNtopAttachment, XmATTACH_FORM,
		        0);
  XtVaSetValues(dummy1, XmNleftAttachment, XmATTACH_WIDGET,
		        XmNleftWidget, dummy,
		        XmNbottomAttachment, XmATTACH_FORM,
		        XmNtopAttachment, XmATTACH_FORM,
		        0);
  XtVaSetValues(dummy2, XmNleftAttachment, XmATTACH_WIDGET,
		        XmNleftWidget, dummy1,
		        XmNbottomAttachment, XmATTACH_FORM,
		        XmNtopAttachment, XmATTACH_FORM,
		        XmNrightAttachment, XmATTACH_FORM,
		        0);


  ac = 0;
  text = XtVaCreateManagedWidget ("Email Address:",
				  xmLabelGadgetClass, strip2,
				  XmNwidth, 125,
				  XmNalignment, XmALIGNMENT_BEGINNING, 
				  NULL);
  ac = 0;
  d->edituser_p_em = XmCreateTextField(strip2, "emailText", av, ac);
  XtManageChild(d->edituser_p_em);
  XtVaSetValues(text, 
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM, 
		0);
  XtVaSetValues(d->edituser_p_em, 
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, text,
		XmNbottomAttachment, XmATTACH_FORM, 
		0);

  ac = 0;
  text = XtVaCreateManagedWidget ("Nickname:",
				  xmLabelGadgetClass, strip3,
				  XmNwidth, 125,
				  XmNalignment, XmALIGNMENT_BEGINNING, 
				  NULL);
  ac = 0;
  d->edituser_p_nickname = XmCreateTextField(strip3, "nicknameText", av, ac);
  XtManageChild(d->edituser_p_nickname);
  XtVaSetValues(text, 
		XmNbottomAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM, 
		0);
  XtVaSetValues(d->edituser_p_nickname, 
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, text,
		XmNbottomAttachment, XmATTACH_FORM, 
		0);

  ac = 0;
  text = XtVaCreateManagedWidget ("Company:",
				  xmLabelGadgetClass, strip4,
				  XmNwidth, 125,
				  XmNalignment, XmALIGNMENT_BEGINNING, 
				  NULL);
  ac = 0;
  d->edituser_p_org = XmCreateTextField(strip4, "companyText", av, ac);
  XtManageChild(d->edituser_p_org);
  XtVaSetValues(text, 
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM, 
		0);
  XtVaSetValues(d->edituser_p_org, 
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		XmNleftAttachment, XmATTACH_WIDGET,
		XmNleftWidget, text,
		XmNbottomAttachment, XmATTACH_FORM, 
		0);

  ac = 0;
  dummy = XmCreateForm(strip5, "localityBox", av, ac);
  XtManageChild(dummy);
  text = XtVaCreateManagedWidget ("Locality",
				  xmLabelGadgetClass, dummy,
				  XmNalignment, XmALIGNMENT_BEGINNING, 
				  NULL);
  ac = 0;
  d->edituser_p_locality = XmCreateTextField(dummy, "localityText", av, ac);
  XtManageChild(d->edituser_p_locality);
  XtVaSetValues(text, 
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		0);
  XtVaSetValues(d->edituser_p_locality, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, text,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		XmNbottomAttachment, XmATTACH_FORM, 
		0);

  ac = 0;
  dummy1 = XmCreateForm(strip5, "regionBox", av, ac);
  XtManageChild(dummy1);
  text = XtVaCreateManagedWidget ("Region",
				  xmLabelGadgetClass, dummy1,
				  XmNwidth, 151,
				  XmNalignment, XmALIGNMENT_BEGINNING, 
				  NULL);
  ac = 0;
  d->edituser_p_region = XmCreateTextField(dummy1, "regionText", av, ac);
  XtManageChild(d->edituser_p_region);
  XtVaSetValues(text, 
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		0);
  XtVaSetValues(d->edituser_p_region, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, text,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		XmNbottomAttachment, XmATTACH_FORM, 
		0);

  XtVaSetValues(dummy, XmNleftAttachment, XmATTACH_FORM,
		        XmNbottomAttachment, XmATTACH_FORM,
		        XmNtopAttachment, XmATTACH_FORM,
		        0);
  XtVaSetValues(dummy1, XmNleftAttachment, XmATTACH_WIDGET,
		        XmNleftWidget, dummy,
		        XmNbottomAttachment, XmATTACH_FORM,
		        XmNtopAttachment, XmATTACH_FORM,
		        XmNrightAttachment, XmATTACH_FORM,
		        0);

  ac = 0;
  text = XtVaCreateManagedWidget ("Description",
				  xmLabelGadgetClass, strip6,
				  XmNwidth, 151,
				  XmNalignment, XmALIGNMENT_BEGINNING, 
				  NULL);
  ac = 0;
  XtSetArg (av[ac], XmNheight, 100); ac++;
  d->edituser_p_desp = XmCreateText(strip6, "despText", av, ac);
  XtManageChild(d->edituser_p_desp);
  XtVaSetValues(text, 
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		0);
  XtVaSetValues(d->edituser_p_desp, 
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, text,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		XmNbottomAttachment, XmATTACH_FORM, 
		0);
}



static void
fe_ab_make_edituser_sec(MWContext *ab_context)
{
  fe_addrbk_data* d = AB_DATA(ab_context);

  int ac;
  Arg av[20];

  Widget form;
  Widget dummy;   /* place holder widget */
  Widget text;

  ac = 0;
  form = XmCreateForm(d->edituser_b, "mainForm", av, ac);

  dummy = XtVaCreateManagedWidget ("Security Memo",   /* I put this here 
							 because right now I
							 don't know how to do
							 this via resources w
							 folders. need to find
							 out how. - bc */
				   xmLabelGadgetClass, form,
				   XmNwidth, 304, 
				   XmNalignment, XmALIGNMENT_BEGINNING, 
				   NULL);
  ac = 0;
  XtSetArg (av[ac], XmNheight, 240); ac++;
  text = XmCreateText(form, "secMemo", av, ac);
  d->edituser_b_text = text;
  XtManageChild(text);

  XtVaSetValues(dummy, 
		XmNleftAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		0);
  XtVaSetValues(text,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, dummy,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM, 
		XmNbottomAttachment, XmATTACH_FORM, 
		0);

  XtManageChild(form);
}


void
fe_ab_OutlineChange(MWContext *context, int index, int32 num, int totallines) {
  Widget outline;
  outline = AB_DATA(context)->outline;
  fe_OutlineChange(outline, index, num, totallines);
}


static void
fe_ab_make_editlist_mail(MWContext *ab_context)
{
  fe_addrbk_data* d = AB_DATA(ab_context);
  int ac;
  Arg av[20];
  Widget kids[20];

  Widget dummy;  /* da place holda */
  Widget dummy1;  /* da place holda */
  
  Widget strip;
  Widget form;

  Widget lnamelabel;
  Widget lnicknamelabel;
  Widget desplabel;
  Widget addtobtn;
  Widget removebtn;

  /* again, most of widget here have resources (labels) hardcoded
   * this needs to change, and I need to figure out how to do the
   * resource stuff via resource file with Folder widgets 
   */
  ac = 0;
  dummy = XmCreateFrame(d->editlist_m, "editlistFrame", av, ac);
  XtVaCreateManagedWidget ("Mailing List Info",
                           xmLabelGadgetClass, dummy,
                           XmNchildType, XmFRAME_TITLE_CHILD,
                           XmNchildVerticalAlignment, XmALIGNMENT_CENTER,
                           NULL);
  XtManageChild(dummy);

  ac = 0;
  form = XmCreateForm(dummy, "editlistForm", av, ac);
  ac = 0;
  dummy = XmCreateForm(form, "editlistFormTop", av, ac);
  ac = 0;
  lnamelabel = XmCreateLabelGadget(dummy, "List Name:", av, ac);
  lnicknamelabel = XmCreateLabelGadget(dummy, "List Nickname:", av, ac);
  desplabel = XmCreateLabelGadget(dummy, "Description:", av, ac);

  ac = 0;
  d->editlist_m_name = XmCreateTextField(dummy, "nameText", av, ac);
  d->editlist_m_nickname = XmCreateTextField(dummy, "nicknameText", av, ac);
  d->editlist_m_desp = XmCreateTextField(dummy, "despText", av, ac);

  XtVaSetValues(lnamelabel,
                XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                XmNtopWidget, d->editlist_m_name,
                XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                XmNbottomWidget, d->editlist_m_name,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_WIDGET,
                XmNrightWidget, d->editlist_m_name,
                0);
  XtVaSetValues(lnicknamelabel,
                XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                XmNtopWidget, d->editlist_m_nickname,
                XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                XmNbottomWidget, d->editlist_m_nickname,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_WIDGET,
                XmNrightWidget, d->editlist_m_nickname,
                0);
  XtVaSetValues(desplabel,
                XmNtopAttachment, XmATTACH_OPPOSITE_WIDGET,
                XmNtopWidget, d->editlist_m_desp,
                XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                XmNbottomWidget, d->editlist_m_desp,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_WIDGET,
                XmNrightWidget, d->editlist_m_desp,
                0);
  XtManageChild(desplabel);
  XtManageChild(lnamelabel);
  XtManageChild(lnicknamelabel);

  XtVaSetValues(d->editlist_m_name,
                XmNtopAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_NONE,
                XmNrightAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_NONE,
                0);
  XtVaSetValues(d->editlist_m_nickname,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, d->editlist_m_name,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, d->editlist_m_name,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		0);
  XtVaSetValues(d->editlist_m_desp,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, d->editlist_m_nickname,
		XmNleftAttachment, XmATTACH_OPPOSITE_WIDGET,
		XmNleftWidget, d->editlist_m_nickname,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_NONE,
		0);
  
  
  fe_attach_field_to_labels(d->editlist_m_name, lnamelabel, lnicknamelabel,
			    desplabel, 0);
 
  XtManageChild(d->editlist_m_name);
  XtManageChild(d->editlist_m_nickname);
  XtManageChild(d->editlist_m_desp);

  ac = 0;
  dummy1 = XmCreateRowColumn(form, "editlistRowCol", av, ac);
  XtManageChild(dummy1);
  ac = 0;
  XtSetArg (av[ac], XmNfractionBase, 10); ac++;
  strip = XmCreateForm(dummy1, "rowcol", av, ac);
  addtobtn = XmCreatePushButton(strip, "Add to List", av, ac);
  removebtn = XmCreatePushButton(strip, "Remove From List", av, ac);
  XtVaSetValues(addtobtn,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNtopPosition, 0,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNbottomPosition, 10,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 1,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 4, 0);
  XtVaSetValues(removebtn,
		XmNtopAttachment, XmATTACH_POSITION,
		XmNtopPosition, 0,
		XmNbottomAttachment, XmATTACH_POSITION,
		XmNbottomPosition, 10,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 6,
		XmNrightAttachment, XmATTACH_POSITION,
		XmNrightPosition, 9, 0);

  XtManageChild(addtobtn);
  XtManageChild(removebtn);
  XtManageChild(strip);
  ac = 0;
  XtSetArg(av[ac], XmNheight, 100); ac++;
  d->editlist_m_members = XmCreateList(dummy1, "memberList", av, ac);
  XtManageChild(d->editlist_m_members);


  XtVaSetValues(dummy,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		0);
  XtVaSetValues(dummy1,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, dummy,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		0);
  
  XtManageChild(dummy);
  XtManageChild(dummy1);
  XtManageChild(form);
}
  

static void
fe_ab_PopupListPropertyWindow(MWContext *ab_context, 
			      ABID entry, XP_Bool newuser)
{
  fe_addrbk_data* d = AB_DATA(ab_context);

  int ac;
  Arg av[20];
  Widget kids[20];
  int i;
  XmString str;
  Widget form = 0;
  Widget dialog = 0;
  Widget tmp = 0;
  char s[1024];

  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  
  if (! d->editlist) {
    XtVaGetValues(CONTEXT_WIDGET(ab_context), XtNvisual, &v,
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
    
    
    d->editlist = XmCreatePromptDialog(CONTEXT_WIDGET(ab_context),
				       "addrBookListProps", av, ac);
    
    dialog = d->editlist;   /* too much typing */
    
    fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
    fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
    fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
    fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog,
						  XmDIALOG_SELECTION_LABEL));
    
    /* needs to change these */
    XtAddCallback(dialog, XmNdestroyCallback, fe_ab_editlist_destroy, 
		  ab_context);
    XtAddCallback(dialog, XmNcancelCallback, fe_ab_editlist_close, 
		  ab_context);
    XtAddCallback(dialog, XmNokCallback, fe_ab_editlist_ok, 
		  ab_context);
    
    form = XtVaCreateManagedWidget("form",
				   xmlFolderWidgetClass, dialog,
				   XmNshadowThickness, 2,
				   XmNtopAttachment, XmATTACH_FORM,
				   XmNleftAttachment, XmATTACH_FORM,
				   XmNrightAttachment, XmATTACH_FORM,
				   XmNbottomAttachment, XmATTACH_FORM,
				   XmNbottomOffset, 3,
				   XmNspacing, 1,
				   NULL);
    
#define ADD_TAB(VAR,STRING) \
    str = XmStringCreateLocalized (STRING); \
    tmp = XmLFolderAddTabForm (form, str); \
    XmStringFree (str); \
    d->VAR = tmp
			  
    ADD_TAB(editlist_m, "Mailing List");
#undef ADD_TAB
    
    fe_ab_make_editlist_mail (ab_context);
    XtManageChild (form);
    fe_NukeBackingStore (dialog);   /* ??? Someone tell me what this does */
    XtManageChild (dialog);
    XmLFolderSetActiveTab (form, 0, True);
  }

  if (!newuser) {
    XtRemoveAllCallbacks(d->editlist, XmNokCallback);
    XtAddCallback(d->editlist, XmNokCallback, fe_ab_editlist_ok, ab_context);
    s[0] = '\0';
    AB_GetFullName(AddrBook, entry, s);
    XtVaSetValues(d->editlist_m_name, XmNvalue, s, 0);
    s[0] = '\0';
    AB_GetNickname(AddrBook, entry, s);
    XtVaSetValues(d->editlist_m_nickname, XmNvalue, s, 0);
    s[0] = '\0';
    AB_GetInfo(AddrBook, entry, s);
    XtVaSetValues(d->editlist_m_desp, XmNvalue, s, 0);
    d->editLID = entry;
  } else {
    XtRemoveAllCallbacks(d->editlist, XmNokCallback);
    XtAddCallback(d->editlist, XmNokCallback, fe_ab_newlist_ok, ab_context);
    s[0] = '\0';
    XtVaSetValues(d->editlist_m_name, XmNvalue, s, 0);
    XtVaSetValues(d->editlist_m_nickname, XmNvalue, s, 0);
    XtVaSetValues(d->editlist_m_desp, XmNvalue, s, 0);
  }
  XtManageChild (d->editlist);
  XMapRaised(XtDisplay(d->editlist), XtWindow(d->editlist));
}


static void
fe_ab_PopupFindWindow (MWContext *ab_context, XP_Bool again)
{
  Widget mainw = CONTEXT_WIDGET (ab_context);
  fe_addrbk_data* d = AB_DATA(ab_context);

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

  if (!d->findshell) {
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
    /* XtSetArg(av[ac], XmNuserData, (XtPointer) findInfo); ac++; */
    XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
    dialog = XmCreatePromptDialog (mainw, "findDialog", av, ac);
  
    d->findshell = dialog;
 
    XtUnmanageChild_safe (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
    XtUnmanageChild_safe (XmSelectionBoxGetChild (dialog,
						  XmDIALOG_SELECTION_LABEL));
    XtManageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));
    
    XtAddCallback (dialog, XmNdestroyCallback, 
		   fe_ab_find_destroy_cb, ab_context);
    XtAddCallback (dialog, XmNokCallback, 
		   fe_ab_find_ok, ab_context);
    XtAddCallback (dialog, XmNcancelCallback, 
		   fe_ab_find_ok, ab_context);
    XtAddCallback (dialog, XmNapplyCallback, 
		   fe_ab_find_ok, ab_context);
    
    ac = 0;
    form = XmCreateForm (dialog, "form", av, ac);
    
    ac = 0;
    findlabel = XmCreateLabelGadget(form, "findLabel", av, ac);
    ac = 0;
    findtext = fe_CreateTextField(form, "findText", av, ac);
    ac = 0;
    lookinlabel = XmCreateLabelGadget(form, "lookinLabel", av, ac);
    
    /* what does this do? why? */
    XP_ASSERT(findlabel->core.width > 0 && lookinlabel->core.width > 0);
    if (findlabel->core.width < lookinlabel->core.width)
      XtVaSetValues (findlabel, XmNwidth, lookinlabel->core.width, 0);
    else
      XtVaSetValues (lookinlabel, XmNwidth, findlabel->core.width, 0);
    
    ac = 0;
    XtSetArg(av[ac], XmNindicatorType, XmN_OF_MANY); ac++;
    nicknameT = XmCreateToggleButtonGadget (form, "nicknameToggle", av, ac);
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
    
    d->findtext = findtext;
    d->findnicknameT = nicknameT;
    d->findnameT = nameT;
    d->findlocationT = locationT;
    d->finddescriptionT = descriptionT;
    d->findcaseT = caseT;
    d->findwordT = wordT;
    
    XtVaSetValues (dialog, XmNinitialFocus, findtext, 0);
  }
  if (again) {
    /* should just do search */
  } else {

    /* put in search method info */
    /* fe_ab_find_setvalues(ab_context, findInfo); */

    /* put in previous info */

    /* popup window */
    if (!XtIsManaged(d->findshell))
      XtManageChild(d->findshell);
    XMapRaised(XtDisplay(d->findshell), XtWindow(d->findshell));
  }
}

