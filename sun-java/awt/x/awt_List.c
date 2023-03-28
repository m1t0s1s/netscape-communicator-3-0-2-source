/*
 * @(#)awt_List.c	1.22 95/12/02 Sami Shaio
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
#include "awt_p.h"
#include "java_awt_List.h"
#include "sun_awt_motif_MListPeer.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "ustring.h"

static void
Slist_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XmListCallbackStruct *cbs = (XmListCallbackStruct *)call_data;

    switch (cbs->reason) {
      case XmCR_DEFAULT_ACTION:
	JAVA_UPCALL((EE(), (void *)client_data, "action","(I)V",
		     (cbs->item_position - 1)));
	break;
      case XmCR_SINGLE_SELECT:
	JAVA_UPCALL((EE(), (void *)client_data,
		     "handleListChanged","(I)V", (cbs->item_position - 1)));
	break;
      case XmCR_MULTIPLE_SELECT:
	JAVA_UPCALL((EE(), (void *)client_data,
		     "handleListChanged","(I)V", (cbs->item_position - 1)));
	break;
      default:
	break;
    }
}

void
sun_awt_motif_MListPeer_create(struct Hsun_awt_motif_MListPeer *this,
			       struct Hsun_awt_motif_MComponentPeer *parent)
{
    int				argc;
    Arg				args[40];
    struct ComponentData	*wdata;
    struct ListData		*sdata;
    Pixel			bg;

    AWT_LOCK();
    if (parent == 0 || unhand(parent)->pData == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    wdata = (struct ComponentData *)unhand(parent)->pData;
    sdata = (struct ListData *)sysMalloc(sizeof(struct ListData));
    SET_PDATA(this, sdata);
    if (sdata == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaGetValues(wdata->widget, XmNbackground, &bg, NULL);
    argc = 0;
    XtSetArg(args[argc], XmNrecomputeSize, False); argc++;
    XtSetArg(args[argc], XmNbackground, bg); argc++;
    XtSetArg(args[argc], XmNlistSizePolicy, XmCONSTANT); argc++;
    XtSetArg(args[argc], XmNx, 0); argc++;
    XtSetArg(args[argc], XmNy, 0); argc++;
    XtSetArg(args[argc], XmNmarginTop, 0); argc++;
    XtSetArg(args[argc], XmNmarginBottom, 0); argc++;
    XtSetArg(args[argc], XmNmarginLeft, 0); argc++;
    XtSetArg(args[argc], XmNmarginRight, 0); argc++;
    XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNlistMarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNlistMarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNscrolledWindowMarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNscrolledWindowMarginHeight, 0); argc++;
    sdata->list = XmCreateScrolledList(wdata->widget,
				       "slist",
				       args,
				       argc);
    sdata->comp.widget = XtParent(sdata->list);
    XtSetMappedWhenManaged(sdata->comp.widget, False);
    XtAddCallback(sdata->list,
		  XmNdefaultActionCallback,
		  Slist_callback,
		  (XtPointer)this);
    XtManageChild(sdata->list);
    XtManageChild(sdata->comp.widget);
    AWT_UNLOCK();
}


long
sun_awt_motif_MListPeer_isSelected(struct Hsun_awt_motif_MListPeer *this,
				   long pos)
{
    struct ListData *sdata;

    AWT_LOCK();
    sdata = PDATA(ListData,this);
    if (sdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return 0;
    }
    pos++;
    if (XmListPosSelected(sdata->list, pos) == True) {
	AWT_UNLOCK();
	return 1;
    } else {
	AWT_UNLOCK();
	return 0;
    }
}

void
sun_awt_motif_MListPeer_addItem(struct Hsun_awt_motif_MListPeer *this,
				Hjava_lang_String *item,
				long index)
{
    XmString		im;
    struct ListData	*sdata;

    AWT_LOCK();
    if (item == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    sdata = PDATA(ListData,this);
    if (sdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    
    im = makeXmString(item);
    if( im == NULL ) {
        AWT_UNLOCK();
        return;
    }
    
    /* motif uses 1-based indeces for the list operations with 0 */
    /* referring to the last item on the list. Thus if index is -1 */
    /* then we'll get the right effect of adding to the end of the */
    /* list. */
    index++;
    XmListAddItemUnselected(sdata->list, im, index);
    XmStringFree(im);
    AWT_UNLOCK();
}

void
sun_awt_motif_MListPeer_makeVisible(struct Hsun_awt_motif_MListPeer *this,
					long pos)
{
    int top, visible;
    struct ListData	*sdata;

    AWT_LOCK();
    sdata = PDATA(ListData,this);
    if (sdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaGetValues(sdata->list,
		  XmNtopItemPosition, &top,
		  XmNvisibleItemCount, &visible,
		  NULL);
    pos++;
    if (pos < top) {
	XmListSetPos(sdata->list, pos);
    } else {
	XmListSetBottomPos(sdata->list, pos);
    }

    AWT_UNLOCK();
}

void
sun_awt_motif_MListPeer_select(struct Hsun_awt_motif_MListPeer *this,
			       long pos)
{
    struct ListData *sdata;

    AWT_LOCK();
    sdata = PDATA(ListData,this);
    if (sdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    pos++;
    XmListSelectPos(sdata->list, pos, 0);
    AWT_UNLOCK();
}

void
sun_awt_motif_MListPeer_deselect(struct Hsun_awt_motif_MListPeer *this,
				 long pos)
{
    struct ListData *sdata;

    AWT_LOCK();
    sdata = PDATA(ListData,this);
    if (sdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    pos++;
    XmListDeselectPos(sdata->list, pos);
    AWT_UNLOCK();
}

void
sun_awt_motif_MListPeer_delItems(struct Hsun_awt_motif_MListPeer *this,
				 long start, long end)
{
    struct ListData *sdata;

    AWT_LOCK();
    sdata = PDATA(ListData,this);
    if (sdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    start++;
    end++;
    if (start == end) {
	XmListDeletePos(sdata->list, start);
    } else {
	XmListDeleteItemsPos(sdata->list, end - start + 1, start);
    }
    AWT_UNLOCK();
}	


void
sun_awt_motif_MListPeer_setMultipleSelections(struct Hsun_awt_motif_MListPeer *this,
					      long v)
{
    struct ListData *sdata;

    AWT_LOCK();
    sdata = PDATA(ListData,this);
    if (sdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if (v == 0) {
	XtVaSetValues(sdata->list,
		      XmNselectionPolicy, XmSINGLE_SELECT,
		      NULL);
	XtRemoveCallback(sdata->list,
			 XmNmultipleSelectionCallback,
			 Slist_callback,
			 (XtPointer)this);
	XtAddCallback(sdata->list,
		      XmNsingleSelectionCallback,
		      Slist_callback,
		      (XtPointer)this);
    } else {
	XtVaSetValues(sdata->list,
		      XmNselectionPolicy, XmMULTIPLE_SELECT,
		      NULL);
	XtRemoveCallback(sdata->list,
			 XmNsingleSelectionCallback,
			 Slist_callback,
			 (XtPointer)this);
	XtAddCallback(sdata->list,
		      XmNmultipleSelectionCallback,
		      Slist_callback,
		      (XtPointer)this);
    }
    AWT_UNLOCK();
}
