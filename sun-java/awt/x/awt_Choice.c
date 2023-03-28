/*
 * @(#)awt_Choice.c	1.27 96/02/21 Sami Shaio
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
#include "java_awt_Component.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "sun_awt_motif_MChoicePeer.h"


static void
Choice_callback(Widget menu_item,
		    Hsun_awt_motif_MChoicePeer *this,
		    XmAnyCallbackStruct *cbs)
{
    long				index;

    XtVaGetValues(menu_item,  XmNuserData, &index, NULL);
    /* index stored in user-data is 1-based instead of 0-based because */
    /* of a bug in XmNuserData */
    index--;
    JAVA_UPCALL((EE(), (void *)this, "action", "(I)V", index));
}

void
sun_awt_motif_MChoicePeer_create(struct Hsun_awt_motif_MChoicePeer *this,
				 struct Hsun_awt_motif_MComponentPeer *parent)
{
    struct ChoiceData	*odata;
    struct ComponentData	*wdata;
    Arg				args[30];
    int				argc;
    Pixel			bg;
    Widget			label;

    AWT_LOCK();
    if (parent == 0 || unhand(parent)->pData == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    odata = ZALLOC(ChoiceData);
    if (!odata) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    SET_PDATA(this, odata);
    odata->items = 0;
    odata->maxitems = 0;
    odata->n_items = 0;
    wdata = (struct ComponentData *)unhand(parent)->pData;
    XtVaGetValues(wdata->widget, XmNbackground, &bg, NULL);

    argc = 0;
    XtSetArg(args[argc], XmNx, 0); argc++;
    XtSetArg(args[argc], XmNy, 0); argc++;
    XtSetArg(args[argc], XmNvisual, awt_visual); argc++;
    XtSetArg(args[argc], XmNbackground, bg); argc++;
    odata->menu = XmCreatePulldownMenu(wdata->widget, "pulldown", args, argc);

    argc = 0;
    XtSetArg(args[argc], XmNx, 0); argc++;
    XtSetArg(args[argc], XmNy, 0); argc++;
    XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNrecomputeSize, False); argc++;
    XtSetArg(args[argc], XmNresizeHeight, False); argc++;
    XtSetArg(args[argc], XmNresizeWidth, False); argc++;
    XtSetArg(args[argc], XmNspacing, False); argc++;
    XtSetArg(args[argc], XmNborderWidth, 0); argc++;
    XtSetArg(args[argc], XmNtraversalOn, False); argc++;
    XtSetArg(args[argc], XmNorientation, XmVERTICAL); argc++;
    XtSetArg(args[argc], XmNadjustMargin, False); argc++;
    XtSetArg(args[argc], XmNbackground, bg); argc++;
    XtSetArg(args[argc], XmNsubMenuId, odata->menu); argc++;
    odata->comp.widget = XmCreateOptionMenu(wdata->widget, "", args, argc);

    label = XmOptionLabelGadget(odata->comp.widget);
    if (label != 0) {
	XtUnmanageChild(label);
    }
    XtSetMappedWhenManaged(odata->comp.widget, False);
    XtManageChild(odata->comp.widget);
    AWT_UNLOCK();
}

void
sun_awt_motif_MChoicePeer_addItem(struct Hsun_awt_motif_MChoicePeer *this,
                                  struct Hjava_lang_String *item,
                                  long index)
{
    XmString              xItem;
    struct ChoiceData     *odata;
    Widget                bw;
    Arg                   args[10];
    int                   argc;
    Pixel                 bg;

    if (item == 0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return;
    }
    
    AWT_LOCK();
    xItem = makeXmString( item );
    if( item == NULL ) {
        AWT_UNLOCK();
        return;
    }

    odata = PDATA(ChoiceData, this);
    if (odata == 0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        AWT_UNLOCK();
        return;
    }
    if (odata->maxitems == 0 || index > odata->maxitems - 1) {

        odata->maxitems += 20;
        if (odata->n_items > 0) {
            /* grow the list of items */
            odata->items = (Widget *)sysRealloc((void *)(odata->items),
                                                sizeof(Widget) * odata->maxitems);
        } else {
            odata->items = (Widget *)sysMalloc(sizeof(Widget) * odata->maxitems);
        }
        if (odata->items == 0) {
            SignalError(0, JAVAPKG "OutOfMemoryError", 0);
            AWT_UNLOCK();
            return;
        }
    }
    XtVaGetValues(odata->comp.widget, XmNbackground, &bg, NULL);

    argc = 0;
    XtSetArg(args[argc], XmNbackground, bg); argc++;
    XtSetArg(args[argc], XmNlabelString, xItem); argc++;
    /* XXX: XmNuserData doesn't seem to work when passing in zero */
    /* so we increment the index before passing it in. */
    XtSetArg(args[argc], XmNuserData, index+1); argc++;
    bw = XmCreatePushButton(odata->menu, "", args, argc);
    XtAddCallback(bw,
		  XmNactivateCallback,
		  (XtCallbackProc)Choice_callback,
		  (XtPointer)this);
    odata->items[index] = bw;
    odata->n_items++;
    XtManageChild(bw);
    
    XmStringFree( xItem );            
    
    AWT_UNLOCK();
}


void
sun_awt_motif_MChoicePeer_select(struct Hsun_awt_motif_MChoicePeer *this,
				      long index)
{
    struct ChoiceData  *odata;

    AWT_LOCK();
    if ((odata = PDATA(ChoiceData, this)) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if (index > odata->n_items || index < 0) {
	SignalError(0, JAVAPKG "IllegalArgumentException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaSetValues(odata->comp.widget,
		  XmNmenuHistory, odata->items[index],
		  NULL);
    AWT_UNLOCK();
}

void 
sun_awt_motif_MChoicePeer_setFont(struct Hsun_awt_motif_MChoicePeer *this,
                                  struct Hjava_awt_Font *f)
{
    struct ChoiceData *cdata;
    struct FontData   *fdata;
    XmFontList        fontlist;
    char              *err;

    if (f == 0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return;
    }
    AWT_LOCK();
    fdata = awt_GetFontData(f, &err);
    if (fdata == 0) {
        SignalError(0, err, 0);
        AWT_UNLOCK();
        return;
    }
    cdata = PDATA(ChoiceData,this);
    if (cdata == 0 || cdata->comp.widget==0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        AWT_UNLOCK();
        return;
    }
    fontlist = fdata->xmfontlist;
    if (fontlist != NULL) {
        int i;

        XtVaSetValues(cdata->comp.widget,
                      XmNfontList, fontlist,
                      NULL);
        XtVaSetValues(cdata->menu,
                      XmNfontList, fontlist,
                      NULL);    
        for (i=0; i < cdata->n_items; i++) {
            XtVaSetValues(cdata->items[i],
                          XmNfontList, fontlist,
                          NULL);
        }
    } else {
        SignalError(0, JAVAPKG "NullPointerException", 0);
    }
    AWT_UNLOCK();
}


void 
sun_awt_motif_MChoicePeer_setBackground(struct Hsun_awt_motif_MChoicePeer *this,
				     struct Hjava_awt_Color *c)
{
    struct ChoiceData *bdata;
    Pixel		color;
    int			i;

    if (c==0) {
	SignalError(0,JAVAPKG "NullPointerException","null color");
	return;
    }
    AWT_LOCK();
    if ((bdata = PDATA(ChoiceData,this)) == 0 || bdata->comp.widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    color = awt_getColor(c);
    XtVaSetValues(bdata->comp.widget, XmNbackground, color, NULL);
    XmChangeColor(bdata->comp.widget, color);
    XtVaSetValues(bdata->menu, XmNbackground, color, NULL);
    XmChangeColor(bdata->menu, color);
    for (i=0; i < bdata->n_items; i++) {
	XtVaSetValues(bdata->items[i], XmNbackground, color, NULL);
	XmChangeColor(bdata->items[i], color);
    }
    AWT_FLUSH_UNLOCK();
}

void 
sun_awt_motif_MChoicePeer_pReshape(struct Hsun_awt_motif_MChoicePeer *this,
				   long x,
				   long y,
				   long w,
				   long h)
{
    struct ComponentData	*cdata;
    Widget			button;

    AWT_LOCK();
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    button = XmOptionButtonGadget(cdata->widget);
    awt_util_reshape(cdata->widget, x, y, w, h);
    awt_util_reshape(button, x, y, w, h);

    AWT_FLUSH_UNLOCK();
}

void 
sun_awt_motif_MChoicePeer_remove(struct Hsun_awt_motif_MChoicePeer *this,
				 long index)
{
    struct ChoiceData		*cdata;
    Widget			selected;
    int				i;

    AWT_LOCK();
    cdata = PDATA(ChoiceData,this);
    if (cdata == 0 || cdata->comp.widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if (index < 0 || index > cdata->n_items) {
	SignalError(0, JAVAPKG "IllegalArgumentException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaGetValues(cdata->comp.widget,
		  XmNmenuHistory, &selected,
		  NULL);
    if (selected == cdata->items[index]) {
	long nextSelected;
	/* we're removing the selected item so select another */
	if (index < (cdata->n_items - 1)) {
	    nextSelected = index+1;
	} else {
	    nextSelected = index-1;
	}
	if (nextSelected > 0) {
	    XtVaGetValues(cdata->comp.widget,
			  XmNmenuHistory, cdata->items[nextSelected],
			  NULL);
	}
    }
    XtUnmanageChild(cdata->items[index]);
    XtDestroyWidget(cdata->items[index]);
    for (i=index+1; i < cdata->n_items; i++) {
	cdata->items[index-1] = cdata->items[index];
    }
    cdata->n_items--;
    /* XXX: what if it's the selected item? */
    AWT_UNLOCK();
}
