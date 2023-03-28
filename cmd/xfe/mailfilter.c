/* -*- Mode:C; tab-width: 4 -*-
   mailfilter.c - Mail Filter frontend
   Copyright © 1996 Netscape Communications Corporation, 
   all rights reserved.

   Created:    Benjie Chen - 6/24/96
   */


/****************************************************************************/
/* Just to go with everyone else's feelings:  MOTIF SUCKS SUCKS SUCKS SUCKS */
/* OPTION menu sucks. X sucks. GUI sucks. Navigator sucks. lynx rules.      */
/****************************************************************************/

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
#include "msg_filt.h"  
#include "msg_srch.h"  

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <XmL/Folder.h>
#include <XmL/Grid.h>
#include <Xm/ArrowBG.h>
#include "DtWidgets/ComboBox.h"

/* for XP_GetString() */
#include <xpgetstr.h>

static char* mailFilterHeaders[] = {
  "Type", "Order", "Name", "On"
};


static char* actionOptMenu[] = {
  "list of all user's mail folders", 0 
};

static MSG_PRIORITY priorityActionMenu[] = {
  MSG_HighPriority, MSG_NormalPriority, MSG_LowPriority, MSG_NoPriority, 99
};


typedef enum
{
  feFilterOn=11,
  feFilterOff
} feFilterStatus;



typedef struct fe_mailfilt_userData {
	int row;
	MSG_SearchAttribute attrib;
} fe_mailfilt_userData;

typedef struct fe_mailfilt_data {
  
  Widget filterDialog;
  Widget filterOutline;
  Widget editDialog;
  Widget deletebtn;
  Widget editbtn;
 
  Widget filterName; 
  Widget rulesRC;
  Widget content[5];	 /* rule content RC */
  Widget scopeOpt[5];  /* scope option */
  Widget whereOpt[5];  /* where option */
  Widget scopeOptPopup[5];
  Widget whereOptPopup[5];
  Widget scopeText[5]; /* scope text */
  Widget priLevel[5];  /* priority level */
  Widget priLevelPopup[5];
  Widget whereLabel[5];
  Widget rulesLabel[5];
  Widget thenTo;
  Widget thenToPopup;
  Widget thenClause;
  Widget thenClausePopup;
  Widget priAction;
  Widget priActionPopup;
  Widget commandGroup;
  Widget despField;
  Widget strip[5];
  Widget mainDespField;
  Widget filterOnBtn;
  Widget filterOffBtn;  
  Widget upBtn;
  Widget downBtn;

  /* these are just pointers to other widgets, they are not really
   * widgets themselves */
  /* priority buttons */
  Widget highBtn;
  Widget lowBtn;
  Widget noneBtn;
  /* selected btns */
  Widget scopeOptSelected[5];
  Widget whereOptSelected[5];
  Widget priLevelSelected[5];
  Widget thenToSelected;
  Widget thenClauseSelected;
  Widget priActionSelected;
  Widget editOkbtn;
 
  Dimension despwidth;
  MSG_FilterList *filterlist;

  int curPos;
  int stripCount;
  XP_Bool curFilterOn;
  
} fe_mailfilt_data;

#define FILTER_DATA(context) (CONTEXT_DATA(context)->filtdata)

static void 
fe_mailfilt_buildWhereOpt(MWContext *context, Dimension *width, 
					   	  Dimension *height, int num, 
						  MSG_SearchAttribute attrib);



/********************/
/* CODE STARTS HERE */
/********************/



/* this function get the rule data specified by the user */
static void
fe_mailfilt_getTerm(MWContext *context, 
					int num, 
					MSG_SearchAttribute *attrib,
					MSG_SearchOperator *op,
					MSG_SearchValue *value) 
{
  fe_mailfilt_data *d = FILTER_DATA(context);
  char *text;
  Widget w;
  fe_mailfilt_userData *userData;

  w=0;
  XtVaGetValues(d->scopeOpt[num], XmNmenuHistory, &w, NULL);
  XtVaGetValues(w, XmNuserData, &userData, 0);
  *attrib = (MSG_SearchAttribute) userData->attrib;
  w=0;
  XtVaGetValues(d->whereOpt[num], XmNmenuHistory, &w, NULL);
  XtVaGetValues(w, XmNuserData, &userData, 0);
  *op = (MSG_SearchOperator) userData->attrib;


  if (*attrib == attribPriority) {                      /* priority */
	value->attribute = *attrib;
  	w=0;
  	XtVaGetValues(d->priLevel[num], XmNmenuHistory, &w, NULL);
  	XtVaGetValues(w, XmNuserData, &userData, 0);
  	value->u.priority = (MSG_PRIORITY) userData->attrib;
  } else if (*attrib == attribDate) {                   /* Date */
  } else if (*attrib == attribMsgStatus) {              /* Status */
  } else {                                              /* everything else */
	value->attribute = *attrib;
    text = XmTextFieldGetString(d->scopeText[num]);
	strcpy(value->u.string, text);
  }
}


static void
fe_mailfilt_getAction(MWContext *context, 
					  MSG_RuleActionType *action, 
					  void **value) 
{
  fe_mailfilt_data *d = FILTER_DATA(context);
  char *text;
  Widget w;
  fe_mailfilt_userData *userData;

  w=0;
  XtVaGetValues(d->thenClause, XmNmenuHistory, &w, NULL);
  XtVaGetValues(w, XmNuserData, &userData, 0);
  *action = (MSG_RuleActionType) userData->attrib;
  switch (*action) {
	case acMoveToFolder:
		*value = NULL;
		break;
	case acChangePriority:
  		w=0;
  		XtVaGetValues(d->priAction, XmNmenuHistory, &w, NULL);
  		XtVaGetValues(w, XmNuserData, &userData, 0);
		*value = (void *) userData->attrib;
	default:
		*value = NULL;
  }
}




static void
fe_mailfilt_get_option_size ( Widget optionMenu, Widget btn,
							  Dimension *retWidth, Dimension *retHeight )
{
  Dimension width, height;
  Dimension mh, mw, ml, mr, mt, mb ; /*margin*/
  Dimension st, bw, ht;
  Dimension space;
  
  XtVaGetValues(btn, XmNwidth, &width,
				XmNheight, &height,
				XmNmarginHeight, &mh,
				XmNmarginWidth, &mw,
				XmNmarginLeft, &ml,
				XmNmarginRight, &mr,
				XmNmarginBottom, &mb,
				XmNmarginTop, &mt,
				XmNhighlightThickness, &ht,
				XmNshadowThickness, &st,
				XmNborderWidth, &bw,
				0);
  
  XtVaGetValues(optionMenu, XmNspacing, &space, 0);
  
  *retHeight = height + (mh+mt+mb+bw+st+ht + space ) * 2;
  *retWidth  = width + (mw+ml+mr+bw+st+ht + space) * 2;
}


static Widget
fe_mailfilt_make_opPopup(Widget popupW, MSG_SearchMenuItem menu[],
						 int itemNum, Dimension *width, Dimension *height,	
                          int row, XtCallbackProc cb, MWContext *context) 
{
  XmString xmStr;
  char *buttonLabel;
  Widget btn, returnBtn;
  Arg	av[30];
  Cardinal ac;
  int j = 0;
  fe_mailfilt_userData *userData;  

 
  while(j < itemNum) {
    j++;	
	buttonLabel = (char *) malloc(strlen(menu[j-1].name)+1);
	strcpy(buttonLabel, menu[j-1].name);
    xmStr = XmStringCreateLtoR(buttonLabel, XmSTRING_DEFAULT_CHARSET);

	userData = XP_NEW_ZAP(fe_mailfilt_userData);
    userData->row = row;
    userData->attrib = menu[j-1].attrib;
    ac = 0;
    XtSetArg(av[ac], XmNuserData, userData); ac++;
    XtSetArg(av[ac], XmNlabelString, xmStr); ac++;
    btn = XmCreatePushButtonGadget(popupW, "operatorBtn", av, ac);
    XtAddCallback(btn, XmNactivateCallback, cb, context);
	
    fe_mailfilt_get_option_size(popupW, btn, width, height);
    XtManageChild(btn);
    free(buttonLabel);
    XtFree(xmStr);
	if (j==1) returnBtn = btn;
  }
  return returnBtn;
}


static Widget
fe_mailfilt_make_option_menu( MWContext *context,
                              Widget parent, char* widgetName,
                              Widget *popup)
{
  Cardinal ac;
  Arg      av[10];
  Visual   *v = 0;
  Colormap cmap = 0;
  Cardinal depth =0;
  Widget option_menu;
  
  XtVaGetValues(CONTEXT_WIDGET(context),
				XtNvisual, &v, XtNcolormap, &cmap, XtNdepth, &depth, 0);
  
  ac = 0;
  XtSetArg(av[ac], XmNvisual, v); ac++;
  XtSetArg(av[ac], XmNdepth, depth); ac++;
  XtSetArg(av[ac], XmNcolormap, cmap); ac ++;
  *popup= XmCreatePulldownMenu(parent, widgetName, av, ac);
  
  ac = 0;
  XtSetArg(av[ac], XmNsubMenuId, *popup); ac++;
  XtSetArg(av[ac], XmNmarginWidth, 0); ac++;
  option_menu = XmCreateOptionMenu(parent, widgetName, av, ac);
  XtUnmanageChild(XmOptionLabelGadget(option_menu));
  
  return option_menu;
}

static void
fe_mailfilt_editclose_cb(Widget w, XtPointer closure, XtPointer calldata)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  int32 count;
  MSG_CloseFilterList(d->filterlist);
  MSG_OpenFilterList(filterInbox, &(d->filterlist));
  MSG_GetFilterCount(d->filterlist, &count);
  fe_OutlineChange(d->filterOutline, 0, count, count);
  XtUnmanageChild(d->editDialog);
}

static void
fe_mailfilt_edit_destroy_cb(Widget w, XtPointer closure, XtPointer calldata)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  fe_mailfilt_editclose_cb(w, closure, calldata);
  d->editDialog = NULL; 
}


static void
fe_mailfilt_getParams(MWContext *context, int position)
{
  fe_mailfilt_data *d = FILTER_DATA(context);
  int num;
  char *desptext;
  char *nametext;
  void *value2;
  MSG_Filter *filter;
  MSG_Rule *rule;
  MSG_RuleActionType action;
  MSG_SearchAttribute attrib;
  MSG_SearchOperator op;
  MSG_SearchValue value;
  int32 filterCount;

  nametext = XmTextFieldGetString(d->filterName);
  MSG_CreateFilter(filterInboxRule, nametext, &filter);
  desptext = XmTextGetString(d->despField);
  MSG_SetFilterDesc(filter, desptext);

  free(nametext);
  free(desptext);

  if (d->curFilterOn) {
	MSG_EnableFilter(filter, d->curFilterOn);
  }

  MSG_GetFilterRule(filter, &rule);

  for (num = 0; num < d->stripCount; num ++) {
	fe_mailfilt_getTerm(context, num, &attrib, &op, &value);
	MSG_RuleAddTerm(rule, attrib, op, &value);
  }

  fe_mailfilt_getAction(context, &action, &value2);
  MSG_RuleSetAction(rule, action, value2);

  if (position < 0) {
  	MSG_GetFilterCount(d->filterlist, &filterCount);
  	MSG_SetFilterAt(d->filterlist, filterCount, filter);
  } else {
  	MSG_SetFilterAt(d->filterlist, (MSG_FilterIndex) position, filter);
  }
}


static void
fe_mailfilt_editok_cb(Widget w, XtPointer closure, XtPointer calldata)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  MSG_Filter *f;

  MSG_GetFilterAt(d->filterlist,d->curPos,&f);
  MSG_RemoveFilterAt(d->filterlist,d->curPos);
  MSG_DestroyFilter(f);
  fe_mailfilt_getParams(context, d->curPos);
  fe_mailfilt_editclose_cb(w, closure, calldata); 
}


static void
fe_mailfilt_newok_cb(Widget w, XtPointer closure, XtPointer calldata)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_getParams(context, -1);
  fe_mailfilt_editclose_cb(w, closure, calldata); 
}


static void
fe_mailfilt_whereOpt_cb(Widget w, XtPointer closure, XtPointer calldata)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  int num;
  fe_mailfilt_userData *userData;
  MSG_SearchAttribute attrib;
  MSG_SearchOperator op;
  XtVaGetValues(w, XmNuserData, &userData, 0);
  num = userData->row - 1;
  op = (MSG_SearchOperator) userData->attrib;
  XtVaGetValues(d->scopeOptSelected[num], XmNuserData, &userData, 0);
  attrib = (MSG_SearchAttribute) userData->attrib;
  if (op == opIsEmpty || attrib == attribPriority) {
  	XtUnmanageChild(d->scopeText[num]);
  	XtVaSetValues(d->whereOpt[num],
				  XmNtopAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_FORM,
				  XmNleftAttachment, XmATTACH_WIDGET,
				  XmNleftWidget, d->whereLabel[num],
				  XmNrightAttachment, XmATTACH_FORM,
				  0);
  } else if (!XtIsManaged(d->scopeText[num])) {
  	XtVaSetValues(d->whereOpt[num],
				  XmNtopAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_FORM,
				  XmNleftAttachment, XmATTACH_WIDGET,
				  XmNleftWidget, d->whereLabel[num],
				  XmNrightAttachment, XmATTACH_NONE,
				  0);
  	XtVaSetValues(d->scopeText[num],
				  XmNtopAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_NONE,
				  XmNleftAttachment, XmATTACH_WIDGET,
				  XmNleftWidget, d->whereOpt[num],
				  0);
  	XtManageChild(d->scopeText[num]);
  }
  d->whereOptSelected[num] = w;


  if (op == opIsHigherThan && attrib == attribPriority) {
	XtManageChild(d->lowBtn);	
	XtUnmanageChild(d->highBtn);	
	XtUnmanageChild(d->noneBtn);	
  } else if (op == opIsLowerThan && attrib == attribPriority) {
	XtManageChild(d->highBtn);	
	XtUnmanageChild(d->lowBtn);	
	XtUnmanageChild(d->noneBtn);	
  }

}


static void
fe_mailfilt_scopeOpt_cb(Widget w, XtPointer closure, XtPointer calldata)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  fe_mailfilt_userData *userData;
  int num;
  MSG_SearchAttribute attrib;
  Dimension width, height;
  Widget popupW;
  
  XtVaGetValues(w, XmNuserData, &userData, 0);
  num = userData->row - 1;
  attrib = userData->attrib;

  /* MOTIF SUCKS */
  if (attrib==attribPriority) {
  	XtUnmanageChild(d->whereOpt[num]);
  	XtUnmanageChild(d->scopeText[num]);
 	fe_mailfilt_buildWhereOpt(context, &width, &height, num, attrib);

  	XtVaSetValues(d->whereOpt[num],
				  XmNwidth, width,
				  XmNtopAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_FORM,
				  XmNleftAttachment, XmATTACH_WIDGET,
				  XmNleftWidget, d->whereLabel[num],
				  XmNrightAttachment, XmATTACH_NONE,
				  0);
  	XtVaSetValues(d->priLevel[num],
				  XmNtopAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_NONE,
				  XmNleftAttachment, XmATTACH_WIDGET,
				  XmNleftWidget, d->whereOpt[num],
				  XmNrightAttachment, XmATTACH_FORM,
				  0);
  	XtManageChild(d->whereOpt[num]);
  	XtManageChild(d->priLevel[num]);
  } else {
  	XtUnmanageChild(d->whereOpt[num]);
  	XtUnmanageChild(d->priLevel[num]);

 	fe_mailfilt_buildWhereOpt(context, &width, &height, num, attrib);

  	XtVaSetValues(d->whereOpt[num],
				  XmNwidth, width,
				  XmNtopAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_FORM,
				  XmNleftAttachment, XmATTACH_WIDGET,
				  XmNleftWidget, d->whereLabel[num],
				  XmNrightAttachment, XmATTACH_NONE,
				  0);
  	XtVaSetValues(d->scopeText[num],
				  XmNtopAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_NONE,
				  XmNleftAttachment, XmATTACH_WIDGET,
				  XmNleftWidget, d->whereOpt[num],
				  XmNrightAttachment, XmATTACH_FORM,
				  0);
  	XtManageChild(d->whereOpt[num]);
  	XtManageChild(d->scopeText[num]);
  }
  d->scopeOptSelected[num] = w;
}


static void
fe_mailfilt_thenClause_cb(Widget w, XtPointer closure, XtPointer calldata)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  fe_mailfilt_userData *userData;
  int num; 
  MSG_SearchAttribute attrib;
  XtVaGetValues(w, XmNuserData, &userData, 0);
  num = userData->row - 1;
  attrib = userData->attrib;
  if (attrib==acChangePriority) {
	  XtUnmanageChild(d->thenTo);
	  XtVaSetValues(d->priAction,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, d->commandGroup,
					XmNbottomAttachment, XmATTACH_NONE,
					XmNleftAttachment, XmATTACH_WIDGET,
					XmNleftWidget, d->thenClause,
					XmNrightAttachment, XmATTACH_NONE,
					0);
	  XtManageChild(d->priAction);
  } else if (attrib==acMoveToFolder) {
	  XtUnmanageChild(d->priAction);
	  XtVaSetValues(d->thenTo,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, d->commandGroup,
					XmNbottomAttachment, XmATTACH_NONE,
					XmNleftAttachment, XmATTACH_WIDGET,
					XmNleftWidget, d->thenClause,
					XmNrightAttachment, XmATTACH_NONE,
					0);
      XtManageChild(d->thenTo);
  } else {
	  XtUnmanageChild(d->priAction);
	  XtUnmanageChild(d->thenTo);
	  XtVaSetValues(d->thenClause, XmNrightAttachment, XmATTACH_NONE, 0);
  }
  d->thenClauseSelected = w;
}

static void 
fe_mailfilt_buildWhereOpt(MWContext *context, Dimension *width, 
					   	  Dimension *height, int num, 
						  MSG_SearchAttribute attrib)
{
  fe_mailfilt_data *d = FILTER_DATA(context);
  Cardinal ac;
  Arg      av[10];
  Visual   *v = 0;
  Colormap cmap = 0;
  Cardinal depth =0;
  Widget dummy;
  Widget popupW;
  MSG_SearchMenuItem menu[20];
  uint16 maxItems = 20;
  int i, numChildren;
  WidgetList childrenList;

  /** MOTIF SUCKS **/

  XtVaGetValues(CONTEXT_WIDGET(context),
				XtNvisual, &v, XtNcolormap, &cmap, XtNdepth, &depth, 0);

  if (d->whereOptPopup[num]) {
	XtVaGetValues(d->whereOptPopup[num], XmNchildren, &childrenList,
                  XmNnumChildren, &numChildren, 0);
  	for ( i = 0; i < numChildren; i++ ) {
		XtDestroyWidget(childrenList[i]);
  	}
  }

  popupW = d->whereOptPopup[num];

  /* make the option menu for where */
  /* get what's needed in the option menu from backend */
  MSG_GetOperatorsForAttribute(scopeMailFolder, attrib, menu, &maxItems);

  /* we save the first widget returned always */
  dummy = fe_mailfilt_make_opPopup(popupW, menu, maxItems, width,
                                   height, num+1, fe_mailfilt_whereOpt_cb,
                                   context);
  XtVaSetValues(d->whereOpt[num], XmNmenuHistory, dummy, NULL);
  d->whereOptPopup[num] = popupW;
  d->whereOptSelected[num] = dummy;
}




static void
fe_mailfilt_setRulesParams(MWContext *context, int num,
  					       MSG_SearchAttribute attrib,
  					       MSG_SearchOperator op,
  					       MSG_SearchValue value)
{
  fe_mailfilt_data *d = FILTER_DATA(context);
  fe_mailfilt_userData *userData;
  int numChildren, i;
  WidgetList childrenList;

  XtVaGetValues(d->scopeOptPopup[num], XmNchildren, &childrenList,
                XmNnumChildren, &numChildren, 0);
  for ( i = 0; i < numChildren; i++ ) {
	XtVaGetValues(childrenList[i], XmNuserData, &userData, 0);
	if (attrib == (MSG_SearchAttribute) userData->attrib) {
	  	XtVaSetValues(d->scopeOpt[num], XmNmenuHistory, childrenList[i], NULL);
		break;
    }
  }
  fe_mailfilt_scopeOpt_cb(childrenList[i], (XtPointer) context, NULL);

  XtVaGetValues(d->whereOptPopup[num], XmNchildren, &childrenList,
                XmNnumChildren, &numChildren, 0);
  for ( i = 0; i < numChildren; i++ ) {
	XtVaGetValues(childrenList[i], XmNuserData, &userData, 0);
	if (op == (MSG_SearchOperator) userData->attrib) {
	  	XtVaSetValues(d->whereOpt[num], XmNmenuHistory, childrenList[i], NULL);
		break;
    }
  }
  fe_mailfilt_whereOpt_cb(childrenList[i], (XtPointer) context, NULL);
  
  switch(attrib) {
	case attribDate:
		break;
	case attribMsgStatus:
		break;
	case attribPriority:
  		XtVaGetValues(d->priLevelPopup[num], XmNchildren, &childrenList,
                      XmNnumChildren, &numChildren, 0);
  		for ( i = 0; i < numChildren; i++ ) {
			XtVaGetValues(childrenList[i], XmNuserData, &userData, 0);
			if (value.u.priority == (MSG_PRIORITY) userData->attrib) {
	  			XtVaSetValues(d->priLevel[num], 
							  XmNmenuHistory, childrenList[i], NULL);
				break;
			}
		}
		break;
	default:
		XtVaSetValues(d->scopeText[num], XmNvalue, value.u.string, NULL);
		break;
  }
}


static void
fe_mailfilt_setActionParams(MWContext *context,
							MSG_RuleActionType type,
						    void *value)
{
  fe_mailfilt_data *d = FILTER_DATA(context);
  fe_mailfilt_userData *userData;
  int numChildren, i;
  WidgetList childrenList;

  XtVaGetValues(d->thenClausePopup, XmNchildren, &childrenList,
                XmNnumChildren, &numChildren, 0);
  for ( i = 0; i < numChildren; i++ ) {
	XtVaGetValues(childrenList[i], XmNuserData, &userData, 0);
	if (type == (MSG_RuleActionType) userData->attrib) {
	  	XtVaSetValues(d->thenClause, XmNmenuHistory, childrenList[i], NULL);
		break;
    }
  }

  fe_mailfilt_thenClause_cb(childrenList[i], (XtPointer) context, NULL);

  if (type == acMoveToFolder) {
  } else if (type == acChangePriority) {
  	XtVaGetValues(d->priActionPopup, XmNchildren, &childrenList,
                  XmNnumChildren, &numChildren, 0);
  	for ( i = 0; i < numChildren; i++ ) {
		XtVaGetValues(childrenList[i], XmNuserData, &userData, 0);
		if ((MSG_PRIORITY) value == (MSG_PRIORITY) userData->attrib) {
	  		XtVaSetValues(d->priAction, XmNmenuHistory, childrenList[i], NULL);
			break;
    	}
  	}
  }

}



static void
fe_mailfilt_priLevel_cb(Widget w, XtPointer closure, XtPointer calldata)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  fe_mailfilt_userData *userData;
  int num;
  XtVaGetValues(w, XmNuserData, &userData, 0);
  num = userData->row - 1;
  d->priLevelSelected[num] = w;
}


static void
fe_mailfilt_thenTo_cb(Widget w, XtPointer closure, XtPointer calldata)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  d->thenToSelected = w;
}




static Widget
fe_mailfilt_make_actPopup(Widget popupW, Dimension *width, Dimension *height,	
                          int row, XtCallbackProc cb, MWContext *context) 
{
  XmString xmStr;
  char *buttonLabel;
  Widget btn, returnBtn;
  Arg	av[30];
  Cardinal ac;
  int j = 0;
  uint16 maxItems;
  fe_mailfilt_userData *userData;  
  MSG_RuleMenuItem menu[20];

  maxItems = 20;
  MSG_GetRuleActionMenuItems(filterInbox, menu, &maxItems);
 
  while(j < maxItems) {
    j++;	
	buttonLabel = (char *) malloc(strlen(menu[j-1].name)+1);
	strcpy(buttonLabel, menu[j-1].name);
    xmStr = XmStringCreateLtoR(buttonLabel, XmSTRING_DEFAULT_CHARSET);

	userData = XP_NEW_ZAP(fe_mailfilt_userData);
    userData->row = row;
    userData->attrib = menu[j-1].attrib;
    ac = 0;
    XtSetArg(av[ac], XmNuserData, userData); ac++;
    XtSetArg(av[ac], XmNlabelString, xmStr); ac++;
    btn = XmCreatePushButtonGadget(popupW, "operatorBtn", av, ac);
    XtAddCallback(btn, XmNactivateCallback, cb, context);
	
    fe_mailfilt_get_option_size(popupW, btn, width, height);
    XtManageChild(btn);
    free(buttonLabel);
    XtFree(xmStr);
	if (j==1) returnBtn = btn;
  }
  return returnBtn;
}



static Widget
fe_mailfilt_make_folderlist(Widget popupW, char **menudesc, 
                          Dimension *width, Dimension *height,	
                          int row, XtCallbackProc cb, MWContext *context) 
{}



/* this function returns the first button that is created which
 * by default shows on the option menu */

static Widget
fe_mailfilt_make_attribPopup(Widget popupW, Dimension *width, 
							 Dimension *height,	int row, 
							 XtCallbackProc cb, MWContext *context) 
{
  XmString xmStr;
  char *buttonLabel;
  Widget btn, returnBtn;
  Arg	av[30];
  Cardinal ac;
  int j = 0;
  uint16 i = 0;
  MSG_SearchMenuItem menu[20]; 
  fe_mailfilt_userData *userData;  

  i = 20; 
  MSG_GetAttributesForScope(scopeMailFolder, menu, &i);
  while(j < i) {
    j++;	
	buttonLabel = (char *) malloc(strlen(menu[j-1].name)+1);
	strcpy(buttonLabel, menu[j-1].name);
    xmStr = XmStringCreateLtoR(buttonLabel, XmSTRING_DEFAULT_CHARSET);

	userData = XP_NEW_ZAP(fe_mailfilt_userData);
    userData->row = row;
    userData->attrib = menu[j-1].attrib;
    ac = 0;
    XtSetArg(av[ac], XmNuserData, userData); ac++;
    XtSetArg(av[ac], XmNlabelString, xmStr); ac++;
    btn = XmCreatePushButtonGadget(popupW, "operatorBtn", av, ac);
    XtAddCallback(btn, XmNactivateCallback, cb, context);
	
    fe_mailfilt_get_option_size(popupW, btn, width, height);
    XtManageChild(btn);
    free(buttonLabel);
    XtFree(xmStr);
	if (j==1) returnBtn = btn;
  }
  return returnBtn;
}


/* this function returns the first button that is created which
 * by default shows on the option menu */

static Widget
fe_mailfilt_makeopt_priority(Widget popupW, MSG_PRIORITY *menudesc, 
                             Dimension *width, Dimension *height,	
                             int row, XtCallbackProc cb, MWContext *context) 
{
  XmString xmStr;
  char *buttonLabel;
  Widget btn;
  Arg	av[30];
  Cardinal ac;
  int j = 0;
  uint16 i = 0;
  MSG_RuleMenuItem menu[20]; 
  Widget returnBtn;
  fe_mailfilt_data *d = FILTER_DATA(context);
  fe_mailfilt_userData *userData;  

 
  while(*menudesc != 99) {
	menu[i].attrib = *menudesc;
	i++;
	menudesc++;
  }

  while(j < i) {
	j++;
	MSG_GetPriorityName((MSG_PRIORITY)menu[j-1].attrib,
						menu[j-1].name,32);
	buttonLabel = (char *) malloc(strlen(menu[j-1].name)+1);
	strcpy(buttonLabel, menu[j-1].name);
    xmStr = XmStringCreateLtoR(buttonLabel, XmSTRING_DEFAULT_CHARSET);

	
	userData = XP_NEW_ZAP(fe_mailfilt_userData);
    userData->row = row;
    userData->attrib = menu[j-1].attrib;
    ac = 0;
    XtSetArg(av[ac], XmNuserData, userData); ac++;
    XtSetArg(av[ac], XmNlabelString, xmStr); ac++;
    btn = XmCreatePushButtonGadget(popupW, "operatorBtn", av, ac);
    XtAddCallback(btn, XmNactivateCallback, cb, context);
	if (menu[j-1].attrib == MSG_HighPriority) d->highBtn = btn;
	else if (menu[j-1].attrib == MSG_LowPriority) d->lowBtn = btn;
	else if (menu[j-1].attrib == MSG_NoPriority) d->noneBtn = btn;
	
    fe_mailfilt_get_option_size(popupW, btn, width, height);
    XtManageChild(btn);
    free(buttonLabel);
    XtFree(xmStr);
	if (j==1) returnBtn = btn;
  }
  return returnBtn;
}



static
void fe_mailfilt_makeRules(Widget rowcol, int fake_num, 
                           fe_mailfilt_data *d, MWContext *context) 
{
  int num = fake_num-1;
  Arg	av[30];
  Cardinal ac;
  Widget popupW;
  Widget dummy;
  char name[128];
  Dimension width, height;
  fe_mailfilt_userData *userData;
  MSG_SearchAttribute attrib;
  uint16 maxItems=16;
  MSG_SearchMenuItem menu[16]; 
 
  sprintf(name,"formContainer%d",num);
  ac = 0;
  d->content[num] = XmCreateForm(rowcol, name, av, ac);
  ac = 0;
  d->strip[num] = XmCreateForm(d->content[num], "rulesForm", av, ac );

  /* build the label */
  if (num == 0)
  	d->rulesLabel[num] = XmCreateLabelGadget(d->content[num], 
						 "rulesLabel1", av, ac);
  else
  	d->rulesLabel[num] = XmCreateLabelGadget(d->content[num], 
	 					 "rulesLabel", av, ac);
 
  /* make the option menu for scope */ 
  d->scopeOpt[num] = fe_mailfilt_make_option_menu(context, d->strip[num],
                                                  "rulesScopeOpt", &popupW);
  dummy = fe_mailfilt_make_attribPopup(popupW, &width, &height, num+1, 
									   fe_mailfilt_scopeOpt_cb, context);
  d->scopeOptPopup[num] = popupW;
  /* we save the first widget returned always */
  /* later we use this widget to get the correct attribute */
  d->scopeOptSelected[num] = dummy;

  /* another label */ 
  ac = 0;
  d->whereLabel[num] = XmCreateLabelGadget(d->strip[num], "whereLabel", av, ac);

  /* get what's needed in the option menu from backend */
  XtVaGetValues(d->scopeOptSelected[num], XmNuserData, &userData, 0);
  attrib = userData->attrib;
  popupW = 0;
  d->whereOpt[num] = fe_mailfilt_make_option_menu(context, d->strip[num],
                                                  "rulesWhereOpt", &popupW);
  d->whereOptPopup[num] = popupW;
  fe_mailfilt_buildWhereOpt(context, &width, &height, num, attrib);

  ac = 0;
  XtSetArg (av[ac], XmNwidth, 120); ac++;
  d->scopeText[num] = XmCreateTextField(d->strip[num], "rulesText", av, ac);

  /* here is the priority level option menu */  
  popupW=0;
  d->priLevel[num] = fe_mailfilt_make_option_menu(context, d->strip[num],
                                                  "priorityLevel", &popupW);
  dummy = fe_mailfilt_makeopt_priority(popupW, priorityActionMenu, &width, 
									   &height, num+1, fe_mailfilt_priLevel_cb,
									   context);
  d->priLevelPopup[num] = popupW;
  d->priLevelSelected[num] = dummy;

  /* geometry */  
  XtVaSetValues(d->rulesLabel[num],
                XmNalignment, XmALIGNMENT_END,
                XmNtopAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                XmNbottomWidget, d->strip[num],
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_NONE,
                0);
  
  XtVaSetValues(d->scopeOpt[num],
                XmNtopAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                0);
  
  XtVaSetValues(d->whereLabel[num],
                XmNalignment, XmALIGNMENT_BEGINNING,
                XmNtopAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                XmNbottomWidget, d->scopeOpt[num],
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, d->scopeOpt[num],
                XmNheight, (d->scopeOpt[num])->core.height,
                0);

  XtVaSetValues(d->whereOpt[num],
                XmNtopAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, d->whereLabel[num],
								0);

  XtVaSetValues(d->scopeText[num],
                XmNtopAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_NONE,
                XmNleftAttachment, XmATTACH_WIDGET,
                XmNleftWidget, d->whereOpt[num],
                XmNrightAttachment, XmATTACH_FORM,
				0);
  
  XtVaSetValues(d->strip[num],
                XmNtopAttachment, XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNleftAttachment, XmATTACH_FORM,
                XmNrightAttachment, XmATTACH_FORM,
                0);

  XtManageChild(d->rulesLabel[num]);
  XtManageChild(d->scopeOpt[num]);
  XtManageChild(d->whereLabel[num]);
  XtManageChild(d->whereOpt[num]);
  XtManageChild(d->scopeText[num]);
  XtManageChild(d->strip[num]);
  XtManageChild(d->content[num]);
}


static void
fe_mailfilt_setRulesGeo(Dimension width, fe_mailfilt_data *d)
{
  int i;
  for (i=0; i<d->stripCount; i++) {
  	XtVaSetValues(d->rulesLabel[i], XmNwidth, width, 0);
	XtVaSetValues(d->strip[i], XmNleftOffset, width, 0);
  }
}




static void
fe_mailfilt_addRow_cb(Widget w, XtPointer closure, XtPointer call_data)
{
  MWContext* context = (MWContext *) closure;
  Dimension width;
  fe_mailfilt_data *d = FILTER_DATA(context);
  if (d->stripCount < 5) {
	d->stripCount++;
	fe_mailfilt_makeRules(d->rulesRC, d->stripCount, d, context);
    /* set the rules geometry to be uniform */
	fe_mailfilt_setRulesGeo(d->despwidth, d);
  }
}


static void
fe_mailfilt_delRow_cb(Widget w, XtPointer closure, XtPointer call_data)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  if (d->stripCount>1) {
	int num = d->stripCount-1;
	XtUnmanageChild(d->content[num]);
	XtDestroyWidget(d->content[num]);
	XtDestroyWidget(d->strip[num]);
	XtDestroyWidget(d->scopeText[num]);
	XtDestroyWidget(d->rulesLabel[num]);
	XtDestroyWidget(d->scopeOpt[num]);
	XtDestroyWidget(d->whereOpt[num]);
	XtDestroyWidget(d->whereLabel[num]);
	d->stripCount--;
  }
}



static void
fe_mailfilt_displayData(MWContext *context)
{
  fe_mailfilt_data *d = FILTER_DATA(context);
  MSG_Filter *f;
  char *text;
  int32 numTerms;
  int i;
  XP_Bool enabled;
  MSG_SearchAttribute attrib;
  MSG_SearchOperator op;
  MSG_SearchValue value;
  MSG_RuleActionType type;
  MSG_Rule *rule;
  void *value2;

  MSG_GetFilterAt(d->filterlist, (MSG_FilterIndex) d->curPos, &f);
  MSG_GetFilterName(f, &text); 
  XtVaSetValues(d->filterName, XmNvalue, text, NULL);
  MSG_GetFilterDesc(f, &text); 
  XtVaSetValues(d->despField, XmNvalue, text, NULL);

  MSG_GetFilterRule(f, &rule);
  MSG_RuleGetNumTerms(rule, &numTerms);
  XP_ASSERT(numTerms > 0 && numTerms < 6);
  
  for (i = 0; i < numTerms; i++) {
	if (d->stripCount <= i) {
		d->stripCount++;
		fe_mailfilt_makeRules(d->rulesRC, d->stripCount, d, context);
    	/* set the rules geometry to be uniform */
		fe_mailfilt_setRulesGeo(d->despwidth, d);
	}
	MSG_RuleGetTerm(rule, i, &attrib, &op, &value);
	fe_mailfilt_setRulesParams(context, i, attrib, op, value);
  }

  if (i < d->stripCount) {
	for(i; i < d->stripCount; i++) {
		XtUnmanageChild(d->content[i]);
		XtDestroyWidget(d->content[i]);
		XtDestroyWidget(d->strip[i]);
		XtDestroyWidget(d->scopeText[i]);
		XtDestroyWidget(d->rulesLabel[i]);
		XtDestroyWidget(d->scopeOpt[i]);
		XtDestroyWidget(d->whereOpt[i]);
		XtDestroyWidget(d->whereLabel[i]);
		d->stripCount--;
	}
  }

  MSG_RuleGetAction(rule, &type, &value2);
  fe_mailfilt_setActionParams(context, type, value2);
 
  MSG_IsFilterEnabled(f, &enabled);  
  if (enabled) {
	XtVaSetValues(d->filterOnBtn, XmNset, True, NULL);
	XtVaSetValues(d->filterOffBtn, XmNset, False, NULL);
  }
  else {
	XtVaSetValues(d->filterOffBtn, XmNset, True, NULL);
	XtVaSetValues(d->filterOnBtn, XmNset, False, NULL);
  }

	
}


static void
fe_mailfilt_turnOnOff(Widget w, XtPointer closure, XtPointer calldata)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  fe_mailfilt_userData *userData;

  XtVaGetValues(w, XmNuserData, &userData, NULL);
  switch(userData->attrib) {
	case feFilterOn:
		d->curFilterOn = True;
		break;
	case feFilterOff:
		d->curFilterOn = False;
		break;
	default:
		XP_ASSERT(0);
		break;
  }	
}




/* this is a long function - lots of stuff for a fairly complicated widget */

static void
fe_mailfilt_edit(MWContext *context, XP_Bool isNew)
{
  Widget dialog = NULL;
  Widget dummy;
  Arg	av[30];
  Cardinal ac;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  Dimension width, height;
  int i;
  fe_mailfilt_userData *userData;
  
  Widget mainw, content, cancelbtn, okbtn, btnform;
  Widget btn, popupW, thenClause, thenTo, thenLabel;
  Widget commandGroup, lessbtn, morebtn;
  Widget filterName, filterLabel, despLabel, despField, statusRadioBox;
  Widget statusOff, statusOn, statusLabel;
  
  fe_mailfilt_data *d = FILTER_DATA(context);

  if (d->editDialog == NULL) {
  	mainw = CONTEXT_WIDGET(context);	
  
  	XtVaGetValues (mainw, XtNvisual, &v, XtNcolormap, &cmap, 
				   XtNdepth, &depth, 0);
  
  	ac = 0;
  	XtSetArg(av[ac], XmNvisual, v); ac++;
  	XtSetArg(av[ac], XmNdepth, depth); ac++;
  	XtSetArg(av[ac], XmNcolormap, cmap); ac++;
  	XtSetArg(av[ac], XmNallowShellResize, True); ac++;
  	XtSetArg(av[ac], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
  	XtSetArg(av[ac], XmNautoUnmanage, False); ac++;
  	XtSetArg(av[ac], XmNdeleteResponse, XmUNMAP); ac++;
  	d->editDialog = dialog = 
		XmCreatePromptDialog(mainw,"editFilterDialog",av,ac);
  	XtUnmanageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
  	XtUnmanageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_SELECTION_LABEL));
  	XtUnmanageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
  	XtUnmanageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_APPLY_BUTTON));
  	XtUnmanageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_OK_BUTTON));
  	XtUnmanageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_CANCEL_BUTTON));
  	XtUnmanageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
    XtAddCallback(d->editDialog, XmNdestroyCallback, 
				  fe_mailfilt_edit_destroy_cb, context);
  	
  	ac = 0;
  	XtSetArg (av [ac], XmNtopAttachment, XmATTACH_FORM); ac++;
  	XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  	XtSetArg (av [ac], XmNleftAttachment, XmATTACH_FORM); ac++;
  	XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  	content= XmCreateForm (dialog, "editContainer", av, ac);
  	
  	ac = 0;
  	filterLabel = XmCreateLabelGadget(content, "filterLabel", av, ac);
  	
  	ac = 0;
  	XtSetArg (av [ac], XmNcolumns, 15); ac++;
  	d->filterName = filterName = XmCreateTextField(content,"filterName",av,ac);
  	
  	ac = 0;
  	XtSetArg (av [ac], XmNorientation, XmVERTICAL); ac++;
  	d->rulesRC = XmCreateRowColumn(content, "rulesRC", av, ac);
  	
  	ac = 0;
  	d->commandGroup = commandGroup = XmCreateForm(content, 
												  "commandGrp", av, ac );
  	
  	ac = 0;
  	morebtn = XmCreatePushButton(commandGroup, "more", av, ac);
  	XtAddCallback(morebtn, XmNactivateCallback, fe_mailfilt_addRow_cb, context);
  	
  	lessbtn = XmCreatePushButton(commandGroup, "fewer", av, ac);
  	XtAddCallback(lessbtn, XmNactivateCallback, fe_mailfilt_delRow_cb, context);
  	
  	XtVaSetValues(morebtn,
                	XmNtopAttachment, XmATTACH_FORM,
                	XmNbottomAttachment, XmATTACH_FORM,
                	XmNleftAttachment, XmATTACH_FORM,
                	XmNrightAttachment, XmATTACH_NONE,
                	0);
  	
  	XtVaSetValues(lessbtn,
                	XmNtopAttachment, XmATTACH_FORM,
                	XmNbottomAttachment, XmATTACH_FORM,
                	XmNleftAttachment, XmATTACH_WIDGET,
                	XmNleftWidget, morebtn,
                	XmNrightAttachment, XmATTACH_NONE,
                	0);
  	
  	XtManageChild(morebtn);
  	XtManageChild(lessbtn);
  	
  	XtVaSetValues(filterLabel,
                	XmNalignment, XmALIGNMENT_END,
                	XmNtopAttachment, XmATTACH_FORM,
                	XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                	XmNbottomWidget, filterName,
                	XmNleftAttachment, XmATTACH_FORM,
                	XmNrightAttachment, XmATTACH_NONE,
                	0);
  	
  	XtVaSetValues(filterName,
                	XmNtopAttachment, XmATTACH_FORM,
                	XmNbottomAttachment, XmATTACH_NONE,
                	XmNleftAttachment, XmATTACH_FORM,
                	XmNrightAttachment, XmATTACH_NONE,
                	0);
  	
  	XtVaSetValues(d->rulesRC,
                	XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, filterName,
                	XmNleftAttachment, XmATTACH_FORM,
                	XmNrightAttachment, XmATTACH_FORM,
                	0);
  	
  	XtVaSetValues(commandGroup,
                	XmNnoResize, True,
                	XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, d->rulesRC,
                	XmNtopOffset, 5,
                	XmNbottomAttachment, XmATTACH_NONE,
                	XmNleftAttachment, XmATTACH_FORM,
                	XmNrightAttachment, XmATTACH_FORM,
                	0);
  	
  	fe_mailfilt_makeRules(d->rulesRC, 1, d, context);
  	d->stripCount=1;
  	
  	XtManageChild(d->rulesRC);
  	XtManageChild(commandGroup);
  	XtManageChild(filterName);
  	XtManageChild(filterLabel);
  	
  	ac = 0;
  	thenLabel = XmCreateLabelGadget(content, "thenLabel", av, ac);


    /* create the clause option menu */  	
  	ac = 0;
  	popupW=0;
  	thenClause = d->thenClause = 
		fe_mailfilt_make_option_menu(context, content, "thenClause", &popupW);
  	dummy = fe_mailfilt_make_actPopup(popupW, &width, &height, 
									  0, fe_mailfilt_thenClause_cb, context);
	d->thenClausePopup = popupW;
    d->thenClauseSelected = dummy;


 	/* create the to option menu */  	
	/* which is a list of folders */
  	ac = 0;
  	popupW=0;
  	thenTo = d->thenTo = 
		fe_mailfilt_make_option_menu(context, content, "thenTo", &popupW);
  	dummy = fe_mailfilt_make_folderlist(popupW, actionOptMenu, &width, &height,
										0, fe_mailfilt_thenTo_cb, context);
	d->thenToPopup = popupW;
    d->thenToSelected = dummy;

	/* this is the priority action menu */
	/* list of priority destinations */
  	ac = 0;
  	popupW=0;
  	d->priAction = fe_mailfilt_make_option_menu(context, content, 
												"priAction", &popupW);
  	dummy = fe_mailfilt_makeopt_priority(popupW, priorityActionMenu, &width, 
									     &height, 0, 
										 fe_mailfilt_priLevel_cb, context);
	d->priActionPopup = popupW;
    d->priActionSelected = dummy;
  	
  	
  	XtVaSetValues(thenLabel,
                	XmNalignment, XmALIGNMENT_END,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, commandGroup,
                	XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                	XmNbottomWidget, thenClause,
                	XmNleftAttachment, XmATTACH_FORM,
                	0);
  	XtVaSetValues(thenClause,
                	XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, commandGroup,
                	XmNbottomAttachment, XmATTACH_NONE,
                	XmNleftAttachment, XmATTACH_FORM,
                	0);
  	XtVaSetValues(thenTo,
                	XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, commandGroup,
                	XmNbottomAttachment, XmATTACH_NONE,
                	XmNleftAttachment, XmATTACH_WIDGET,
					XmNleftWidget, thenClause,
                	XmNrightAttachment, XmATTACH_NONE,
                	0);
  	
  	XtManageChild(thenLabel);
  	XtManageChild(thenClause);
  	XtManageChild(thenTo);
  	
  	ac = 0;
  	despLabel = XmCreateLabelGadget(content, "despLabel", av, ac);
  	
  	ac = 0;
  	d->despField = despField = XmCreateText(content, "despField", av, ac);
  	
  	
  	XtVaSetValues(despLabel,
                	XmNalignment, XmALIGNMENT_END,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, thenLabel,
                	XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
                	XmNbottomWidget, despField,
                	XmNleftAttachment, XmATTACH_FORM,
                	XmNrightAttachment, XmATTACH_NONE,
                	0);
  	XtVaSetValues(despField,
                	XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, thenClause,
                	XmNbottomAttachment, XmATTACH_NONE,
                	XmNleftAttachment, XmATTACH_FORM,
                	XmNrightAttachment, XmATTACH_FORM,
                	0);
  	
  	XtManageChild(despField);
  	XtManageChild(despLabel);
  	
  	ac = 0;
  	XtSetArg (av [ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
  	XtSetArg (av [ac], XmNtopWidget, despField); ac++;
  	XtSetArg (av [ac], XmNleftAttachment, XmATTACH_NONE); ac++;
  	XtSetArg (av [ac], XmNrightAttachment, XmATTACH_FORM); ac++;
  	XtSetArg (av [ac], XmNbottomAttachment, XmATTACH_FORM); ac++;
  	btnform = XmCreateForm(content, "btnForm", av, ac);
  	ac = 0;
  	okbtn = XmCreatePushButton(btnform, "okbtn", av, ac);
  	cancelbtn = XmCreatePushButton(btnform, "cancelbtn", av, ac);
  	statusLabel = XmCreateLabelGadget(btnform, "statusLabel", av, ac);
 	
  	XtAddCallback(cancelbtn, XmNactivateCallback, 
					fe_mailfilt_editclose_cb, context);
 	
  	ac = 0;
  	XtSetArg (av [ac], XmNorientation, XmHORIZONTAL); ac++;
  	statusRadioBox = XmCreateRadioBox(btnform, "radioBox", av, ac);
  	

	userData = XP_NEW_ZAP(fe_mailfilt_userData);
    userData->attrib = feFilterOn;
  	d->filterOnBtn = statusOn = 
		XtVaCreateManagedWidget("on", xmToggleButtonGadgetClass, 
								statusRadioBox, NULL);
  	XtVaSetValues(statusOn, XmNuserData, userData, 0);
	XtAddCallback(statusOn, XmNvalueChangedCallback, fe_mailfilt_turnOnOff,
				  context);


	userData = XP_NEW_ZAP(fe_mailfilt_userData);
    userData->attrib = feFilterOff;
  	d->filterOffBtn = statusOff = 
		XtVaCreateManagedWidget("off", xmToggleButtonGadgetClass, 
                               	statusRadioBox, NULL);
  	XtVaSetValues(statusOff, XmNuserData, userData, 0);
	XtAddCallback(statusOff, XmNvalueChangedCallback, fe_mailfilt_turnOnOff,
				  context);



  	
  	XtVaSetValues(okbtn,
					XmNtopAttachment, XmATTACH_FORM,
                	XmNbottomAttachment, XmATTACH_FORM,
                	XmNleftAttachment, XmATTACH_WIDGET,
                	XmNleftWidget, statusRadioBox,
                	0);
  	XtVaSetValues(cancelbtn,
                	XmNtopAttachment, XmATTACH_FORM,
                	XmNbottomAttachment, XmATTACH_FORM,
                	XmNleftAttachment, XmATTACH_WIDGET,
					XmNleftWidget, okbtn,
                	XmNrightAttachment, XmATTACH_FORM,
                	0);
  	XtVaSetValues(statusRadioBox,
                	XmNtopAttachment, XmATTACH_FORM,
                	XmNbottomAttachment, XmATTACH_FORM,
                	XmNrightOffset, 20,
                	XmNleftAttachment, XmATTACH_WIDGET,
					XmNleftWidget, statusLabel,
                	0);
  	XtVaSetValues(statusLabel,
                	XmNtopAttachment, XmATTACH_FORM,
                	XmNbottomAttachment, XmATTACH_FORM,
                	XmNleftAttachment, XmATTACH_NONE,
                	0);
  	XtManageChild(okbtn);
  	XtManageChild(cancelbtn);
  	XtManageChild(statusLabel);
  	XtManageChild(statusRadioBox);
  	XtManageChild(btnform);
  	
  	XtVaGetValues(despLabel, XmNwidth, &width, 0);
  	XtVaSetValues(thenLabel, XmNwidth, width, 0 );
  	XtVaSetValues(despLabel, XmNwidth, width, 0 );
  	XtVaSetValues(filterName, XmNleftOffset, width, 0);
  	XtVaSetValues(commandGroup, XmNleftOffset, width, 0);
  	XtVaSetValues(thenClause, XmNleftOffset, width, 0);
  	XtVaSetValues(despField, XmNleftOffset, width, 0);
  	
  	fe_mailfilt_setRulesGeo(width, d);
	d->despwidth = width;
	d->editOkbtn = okbtn;
  	XtManageChild(content);
  }
  if (d->editDialog) {
	if (isNew) {
		XtRemoveAllCallbacks(d->editOkbtn, XmNactivateCallback);
  		XtAddCallback(d->editOkbtn, XmNactivateCallback, 
				  	  fe_mailfilt_newok_cb, context);
	} else {
		fe_mailfilt_displayData(context);
		XtRemoveAllCallbacks(d->editOkbtn, XmNactivateCallback);
  		XtAddCallback(d->editOkbtn, XmNactivateCallback, 
				  	  fe_mailfilt_editok_cb, context);
	}
	XtManageChild(d->editDialog);
 	XtPopup (XtParent(d->editDialog), XtGrabNone);
  }
} 
		
static void
fe_mailfilt_newfilter_cb(Widget w, XtPointer closure, XtPointer calldata)
{
	fe_mailfilt_edit((MWContext *) closure, True);
}

static void
fe_mailfilt_editfilter_cb(Widget w, XtPointer closure, XtPointer calldata)
{
	fe_mailfilt_edit((MWContext *) closure, False);
}		
	
static void
fe_mailfilt_del_cb(Widget w, XtPointer closure, XtPointer calldata)
{
  MWContext* context = (MWContext *) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  MSG_Filter *f = 0;
  int32 count=0;
  int i;
  int *rowPos;
  
  count = XmLGridGetSelectedRowCount(d->filterOutline);
  if (count) {
	MSG_Filter *f;
	rowPos = malloc (sizeof(int) * count);
	if (XmLGridGetSelectedRows(d->filterOutline, rowPos, count)!=-1) {
	  MSG_GetFilterAt(d->filterlist,*(rowPos),&f);
	  MSG_RemoveFilterAt(d->filterlist,*(rowPos));
	  MSG_DestroyFilter(f);
	  MSG_GetFilterCount(d->filterlist, &count);
	  fe_OutlineChange(d->filterOutline, 0, count, count);
      if (count > 0) {
		XtVaSetValues(d->upBtn, XmNsensitive, True, 0);
		XtVaSetValues(d->downBtn, XmNsensitive, True, 0);
		XtVaSetValues(d->deletebtn, XmNsensitive, True, 0);
		XtVaSetValues(d->editbtn, XmNsensitive, True, 0);
  	  } else {
		XtVaSetValues(d->upBtn, XmNsensitive, False, 0);
		XtVaSetValues(d->downBtn, XmNsensitive, False, 0);
		XtVaSetValues(d->deletebtn, XmNsensitive, False, 0);
		XtVaSetValues(d->editbtn, XmNsensitive, False, 0);
  	  }
	
  	  if (count > 0) {
  		count = XmLGridGetSelectedRowCount(d->filterOutline);
  		rowPos = malloc (sizeof(int) * count);
  		if (XmLGridGetSelectedRows(d->filterOutline, rowPos, count)!=-1) {
  			d->curPos = *rowPos;
		}
  	  } else {
		d->curPos = 0;
  	  }
	}
  }
  free((char*)rowPos);
}


static Boolean
fe_mailfilt_datafunc(Widget widget, void* closure, int row, 
					 fe_OutlineDesc* data, int tag)
{
  MWContext* context = (MWContext*) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  MSG_Filter *f = 0;
  char *name = 0;
  char *status = 0;
  char *locale;
  char *order;
  XP_Bool enabled;

  XP_ASSERT(d != NULL);
  if (!d->filterlist) 
    return False;
  
  MSG_GetFilterAt(d->filterlist, (MSG_FilterIndex) row, &f);
  if (!f) 
	return False;
  data->flippy = FE_OUTLINE_Folded;
  data->icon = FE_OUTLINE_ClosedList;
  data->tag = XmFONTLIST_DEFAULT_TAG;
  
  data->type[0] = FE_OUTLINE_String;

  order = (char *) malloc(sizeof(char)*4);  
  sprintf(order, "%d", row+1);
  MSG_GetFilterName(f, &name);
  MSG_IsFilterEnabled(f, &enabled);  

  locale = fe_ConvertToLocaleEncoding
    (INTL_DefaultWinCharSetID(NULL), order);
  PR_snprintf(order, sizeof(order), "%s", locale);
  if ((char *) locale != order)
    {
      XP_FREE(locale);
    }
  data->str[0] = order;
  
  locale = fe_ConvertToLocaleEncoding
    (INTL_DefaultWinCharSetID(NULL), name);
  PR_snprintf(name, sizeof(name), "%s", locale);
  if ((char *) locale != name)
    {
      XP_FREE(locale);
    }
  data->str[1] = name;

  status = malloc(4*sizeof(char));
  if (enabled) {
	strcpy(status,"yes");	
  } else {
	strcpy(status,"no");	
  }
  locale = fe_ConvertToLocaleEncoding
    (INTL_DefaultWinCharSetID(NULL), status);
  PR_snprintf(status, sizeof(status), "%s", locale);
  if ((char *) locale != status)
    {
      XP_FREE(locale);
    }
  data->str[2] = status;

  return True;
}


static void
fe_mailfilt_clickfunc(Widget widget, void* closure, int row, int column,
					  char* header, int button, int clicks,
					  Boolean shift, Boolean ctrl, int tag)
{
  MWContext* context = (MWContext*) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  int32 count=0;
  int *rowPos;
  
  MSG_GetFilterCount(d->filterlist, &count);
  if (count > 0) {
	XtVaSetValues(d->upBtn, XmNsensitive, True, 0);
	XtVaSetValues(d->downBtn, XmNsensitive, True, 0);
	XtVaSetValues(d->deletebtn, XmNsensitive, True, 0);
	XtVaSetValues(d->editbtn, XmNsensitive, True, 0);
  } else {
	XtVaSetValues(d->upBtn, XmNsensitive, False, 0);
	XtVaSetValues(d->downBtn, XmNsensitive, False, 0);
	XtVaSetValues(d->deletebtn, XmNsensitive, False, 0);
	XtVaSetValues(d->editbtn, XmNsensitive, False, 0);
  }

  count = XmLGridGetSelectedRowCount(d->filterOutline);
  if (count > 0) {
  	rowPos = malloc (sizeof(int) * count);
  	if (XmLGridGetSelectedRows(d->filterOutline, rowPos, count)!=-1) {
		MSG_Filter *f;
		char *text;
  		d->curPos = *rowPos;
  		MSG_GetFilterAt(d->filterlist, (MSG_FilterIndex) d->curPos, &f);
		MSG_GetFilterDesc(f, &text);
		XtVaSetValues(d->mainDespField, XmNvalue, text, NULL);
	}
  } else {
	d->curPos = 0;
  }

  if (clicks == 2) {
	fe_mailfilt_edit(context, False);
  }
}


static void
fe_mailfilt_iconfunc(Widget widget, void* closure, int row, int tag)
{
  MWContext* context = (MWContext*) closure;
}


static void
fe_mailfilt_close_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* context = (MWContext*) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  XP_ASSERT(d != NULL);
  MSG_CloseFilterList(d->filterlist);
  d->filterlist = NULL;
  XtUnmanageChild(d->filterDialog);
}

static void
fe_mailfilt_destroy_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* context = (MWContext*) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  fe_mailfilt_close_cb(widget, closure, call_data);
  free(d);
  FILTER_DATA(context) = 0;
}


static void
fe_mailfilt_up_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* context = (MWContext*) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  MSG_Filter *f;
  int32 count;

  if (d->curPos > 0) {
  	MSG_MoveFilterAt(d->filterlist, (MSG_FilterIndex) d->curPos, filterUp);
	d->curPos--;
    XmLGridSelectRow(d->filterOutline, d->curPos, True);
	MSG_GetFilterCount(d->filterlist, &count);
  	fe_OutlineChange(d->filterOutline, 0, count, count);
  }	
}


static void
fe_mailfilt_down_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* context = (MWContext*) closure;
  fe_mailfilt_data *d = FILTER_DATA(context);
  MSG_Filter *f;
  int32 filterCount;

  MSG_GetFilterCount(d->filterlist, &filterCount);

  if (d->curPos < filterCount-1) {
  	MSG_MoveFilterAt(d->filterlist, (MSG_FilterIndex) d->curPos, filterDown);
	d->curPos++;
    XmLGridSelectRow(d->filterOutline, d->curPos, True);
	MSG_GetFilterCount(d->filterlist, &filterCount);
  	fe_OutlineChange(d->filterOutline, 0, filterCount, filterCount);
  }
}


static void
fe_mailfilt_ok_cb(Widget widget, XtPointer closure, XtPointer call_data)
{
  MWContext* context = (MWContext*) closure;
  
  fe_mailfilt_close_cb(widget, closure, call_data);
}


void fe_mailfilter_cb(Widget w, XtPointer closure, XtPointer call_data)
{
  fe_mailfilt_data *d;
  MWContext *context = (MWContext*) closure;
  static char* columnStupidity = NULL;
  static int columnwidths[] = {10, 10, 40, 5};
  int ac;
  Arg av[20];
  Widget mainw = 0;
  Widget dummy = 0; /* da place holda */
  Widget orderLabel = 0;
  Widget dialog = 0;
  Widget orderBox = 0;
  Widget btnbox = 0;
  Widget text = 0;
  Widget logbtn = 0;
  Widget newbtn = 0;
  Widget editbtn = 0;
  Widget deletebtn = 0;
  Widget javabtn = 0;
  Widget okbtn = 0;
  Widget cancelbtn = 0;
  Widget sep = 0;
  Visual *v = 0;
  Colormap cmap = 0;
  Cardinal depth = 0;
  int count;
  
  d = FILTER_DATA(context);
  if (!d) {
	
    d = FILTER_DATA(context) = XP_NEW_ZAP(fe_mailfilt_data);

    d->filterlist = NULL;
	d->editDialog = NULL;
	d->curPos = 0;
    XtVaGetValues(CONTEXT_WIDGET(context), XtNvisual, &v,
				  XtNcolormap, &cmap, XtNdepth, &depth, 0);
    
    ac = 0;
    XtSetArg (av[ac], XmNvisual, v); ac++;
    XtSetArg (av[ac], XmNdepth, depth); ac++;
    XtSetArg (av[ac], XmNcolormap, cmap); ac++;
    XtSetArg (av[ac], XmNallowShellResize, True); ac++;
    XtSetArg (av[ac], XmNdeleteResponse, XmUNMAP); ac++;
    XtSetArg (av[ac], XmNautoUnmanage, False); ac++;
    XtSetArg (av[ac], XmNdialogStyle, 
			  XmDIALOG_PRIMARY_APPLICATION_MODAL); ac++;
	d->filterDialog = XmCreatePromptDialog(CONTEXT_WIDGET(context), 
										   "filterDialog", av, ac);
	
	dialog = d->filterDialog;   /* too much typing */
	
	fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, XmDIALOG_SEPARATOR));
	fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT));
	fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, 
												  XmDIALOG_HELP_BUTTON));
	fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, 
												  XmDIALOG_OK_BUTTON));
	fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, 
												  XmDIALOG_CANCEL_BUTTON));
	fe_UnmanageChild_safe(XmSelectionBoxGetChild (dialog, 
												  XmDIALOG_SELECTION_LABEL));
			  
    XtAddCallback(d->filterDialog, XmNdestroyCallback, 
				  fe_mailfilt_destroy_cb, context);

    ac = 0;
    XtSetArg (av[ac], XmNwidth, 500); ac++;
    XtSetArg (av[ac], XmNheight, 300); ac++;
    mainw = XmCreateForm(dialog, "form", av, ac);
	
    ac = 0;
    XtSetArg (av[ac], XmNorientation, XmVERTICAL); ac++;
    orderBox = XmCreateRowColumn(mainw, "orderBox", av, ac);
	
    orderLabel = XtVaCreateManagedWidget ("orderLabel",
										  xmLabelGadgetClass, orderBox,
										  XmNalignment, XmALIGNMENT_BEGINNING, 
										  NULL);
    d->upBtn = XtVaCreateManagedWidget ("upArrow",
							 			xmArrowButtonGadgetClass, orderBox,
							 			XmNarrowDirection, XmARROW_UP,
							 			NULL);
	XtAddCallback(d->upBtn,XmNactivateCallback,fe_mailfilt_up_cb,context);

    d->downBtn = XtVaCreateManagedWidget ("downArrow",
							 			  xmArrowButtonGadgetClass, orderBox,
							 			  XmNarrowDirection, XmARROW_DOWN,
							 			  NULL);
	XtAddCallback(d->downBtn,XmNactivateCallback,fe_mailfilt_down_cb,context);
	
    ac = 0;
    d->filterOutline = fe_GridCreate(context, mainw, "list", av, ac,
									 0, 3, columnwidths, 
									 fe_mailfilt_datafunc,
									 fe_mailfilt_clickfunc, 
									 fe_mailfilt_iconfunc, 
									 context,
									 /* benjie - need to change this */
									 FE_DND_BOOKMARK, &columnStupidity,
									 False, False);
    fe_OutlineSetHeaders(d->filterOutline, mailFilterHeaders);  
    XtVaSetValues(d->filterOutline, 
				  XmNshowHideButton, True,
				  XmNselectionPolicy, XmSELECT_SINGLE_ROW,
				  0);
	
    ac = 0;
    dummy = XtVaCreateManagedWidget("btnrowcol", 
									xmRowColumnWidgetClass, mainw,
									XmNorientation, XmVERTICAL,
									NULL);
    newbtn = XtVaCreateWidget("newFilter", 
							  xmPushButtonGadgetClass, dummy,
							  NULL);
    editbtn = XtVaCreateWidget("editFilter",
							   xmPushButtonGadgetClass, dummy,
							   NULL);
	d->editbtn = editbtn;
    deletebtn = XtVaCreateWidget("delFilter",
								 xmPushButtonGadgetClass, dummy,
								 NULL);
	d->deletebtn = deletebtn;
    sep = XtVaCreateWidget("separator",
						   xmSeparatorGadgetClass, dummy,
						   NULL);
    javabtn = XtVaCreateWidget("javaScript",
							   xmPushButtonGadgetClass, dummy,
							   NULL);
    text = XtVaCreateWidget("text",
							xmTextWidgetClass, mainw,
							XmNheight, 100,
							XmNeditable, False,
							NULL);
    logbtn = XtVaCreateWidget("logbtn",
							  xmToggleButtonWidgetClass, mainw,
							  NULL);
    okbtn = XtVaCreateWidget("okbtn",
							 xmPushButtonGadgetClass, mainw,
							 NULL);
    cancelbtn = XtVaCreateWidget("cancelbtn",
								 xmPushButtonGadgetClass, mainw,
								 NULL);
	
    XtManageChild(text);
    XtManageChild(logbtn);
    XtManageChild(cancelbtn);
    XtManageChild(okbtn);
	
    XtAddCallback(cancelbtn, XmNactivateCallback, 
				  fe_mailfilt_close_cb, context);
    XtAddCallback(okbtn, XmNactivateCallback, 
				  fe_mailfilt_ok_cb, context);
    XtAddCallback(newbtn, XmNactivateCallback,
				  fe_mailfilt_newfilter_cb, context);
    XtAddCallback(editbtn, XmNactivateCallback,
				  fe_mailfilt_editfilter_cb, context);
    XtAddCallback(deletebtn, XmNactivateCallback,
				  fe_mailfilt_del_cb, context);
	
	
    XtManageChild(dummy);
    XtManageChild(newbtn);
    XtManageChild(editbtn);
    XtManageChild(deletebtn);
    XtManageChild(sep);
    XtManageChild(javabtn);
    XtManageChild(orderBox);
    XtManageChild (d->filterOutline);
	
    XtVaSetValues(logbtn,
				  XmNtopAttachment, XmATTACH_NONE,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_FORM,
				  0);
    XtVaSetValues(okbtn,
				  XmNtopAttachment, XmATTACH_NONE,
				  XmNleftAttachment, XmATTACH_NONE,
				  XmNrightAttachment, XmATTACH_WIDGET,
				  XmNrightWidget, cancelbtn,
				  XmNbottomAttachment, XmATTACH_FORM,
				  XmNwidth, cancelbtn->core.width,
				  0);
    XtVaSetValues(cancelbtn,
				  XmNtopAttachment, XmATTACH_NONE,
				  XmNrightAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_FORM,
				  0);
    XtVaSetValues(text,
				  XmNtopAttachment, XmATTACH_NONE,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNrightAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_WIDGET,
				  XmNbottomWidget, okbtn,
				  0);
    XtVaSetValues(dummy,
				  XmNtopAttachment, XmATTACH_FORM,
				  XmNleftAttachment, XmATTACH_NONE,
				  XmNrightAttachment, XmATTACH_FORM,
				  XmNbottomAttachment, XmATTACH_NONE,
				  0);
    XtVaSetValues(d->filterOutline,
				  XmNtopAttachment, XmATTACH_FORM,
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNleftOffset, orderLabel->core.width + 14,
				  XmNrightAttachment, XmATTACH_WIDGET,
				  /* XmNrightOffset, deletebtn->core.width + 14, */
				  XmNrightWidget, dummy,
				  XmNbottomAttachment, XmATTACH_WIDGET,
				  XmNbottomWidget, text,
				  0);
    XtVaSetValues(orderBox,
				  XmNtopAttachment, XmATTACH_NONE, 
				  XmNleftAttachment, XmATTACH_FORM,
				  XmNrightAttachment, XmATTACH_NONE,
				  XmNbottomAttachment, XmATTACH_WIDGET,
				  XmNbottomWidget, text,
				  0);
	d->mainDespField = text;	
	XtVaSetValues(d->upBtn, XmNsensitive, False, 0);
	XtVaSetValues(d->downBtn, XmNsensitive, False, 0);
	XtVaSetValues(d->deletebtn, XmNsensitive, False, 0);
	XtVaSetValues(d->editbtn, XmNsensitive, False, 0);
    XtManageChild (mainw);
    
    /* don't put translations for this because we don't want many of
     * the capabilities */
	
    /* fe_HackTranslations (context, dialog);  */
    
    /* need to have this here to add the action overriden by HackTranslations
       back to the hide/unhide btn for Grid widget */
	
    XmLGridInstallHideButtonTranslations(d->filterOutline); 
  }
  
  if (d->filterDialog) {
	
    MSG_Filter *f;
	int32 count=0;
	
    /* the first argument is an enum, don't know why it isn't capped or
       something to make it obvious...... backend stuff - bc */
    MSG_OpenFilterList(filterInbox, &(d->filterlist));
	MSG_GetFilterCount(d->filterlist, &count);
  	fe_OutlineChange(d->filterOutline, 0, count, count);
	
    XtManageChild(d->filterDialog);
    XtPopup (XtParent(d->filterDialog), XtGrabNone);
  }
}

void 
fe_mailfilt_detail_pop(MWContext *context)
{
}
