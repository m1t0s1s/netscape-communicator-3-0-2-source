/*
 * @(#)awt_Menu.c	1.15 95/12/08 Sami Shaio
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
#include "color.h"
#include "java_awt_Menu.h"
#include "sun_awt_motif_MMenuPeer.h"
#include "java_awt_MenuBar.h"
#include "sun_awt_motif_MMenuBarPeer.h"
#include "java_awt_Font.h"
#include "ustring.h"

static void
awt_createMenu(struct Hsun_awt_motif_MMenuPeer *this, Widget menuParent)
{
    int			argc;
    Arg			args[10];
    XmString		xTitle;
    struct MenuData	*mdata;
    struct FontData	*fdata;
    Pixel		bg;
    Pixel		fg;
    Widget		tearOff;
    Classjava_awt_Menu	*targetPtr;
    struct Hjava_awt_Font* font = NULL; 

    if (unhand(this)->target == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    targetPtr = unhand((struct Hjava_awt_Menu *)unhand(this)->target);
    mdata = ZALLOC(MenuData);
    SET_PDATA(this, mdata);

    if (mdata == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return;
    }
    
    xTitle = makeXmString(targetPtr->label);
    if( xTitle == NULL ) {
        return;
    }

    XtVaGetValues(menuParent, XmNbackground, &bg, NULL);

    argc = 0;
    XtSetArg(args[argc], XmNbackground, bg); argc++;
    if (targetPtr->tearOff != 0) {
	XtSetArg(args[argc], XmNtearOffModel, XmTEAR_OFF_ENABLED); argc++;
    }
    XtSetArg(args[argc], XmNvisual, awt_visual); argc++;
    mdata->itemData.comp.widget = XmCreatePulldownMenu(menuParent,
						       "",
						       args,
						       argc);
    if (targetPtr->tearOff != 0) {
	tearOff = XmGetTearOffControl(mdata->itemData.comp.widget);
	fg = awtImage->ColorMatch(0,0,0);
	XtVaSetValues(tearOff,
		      XmNbackground, bg,
		      XmNforeground, fg,
		      XmNhighlightColor, fg,
		      NULL);
    }

    argc=0;
    XtSetArg(args[argc], XmNsubMenuId, mdata->itemData.comp.widget); argc++;
    XtSetArg(args[argc], XmNlabelString, xTitle); argc++;
    XtSetArg(args[argc], XmNbackground, bg); argc++;
    
    /* It is not sufficient to get the target's font from the target's font data
     * field.  MenuComponent.getFont() will return the the parent's font if
     * the target's own font is nil.
     */
    JAVA_UPCALL_WITH_RETURN( (EE(), (void*)unhand(this)->target,
                              "getFont", "()Ljava/awt/Font;"),
                             font, (struct Hjava_awt_Font*) );
    if (font != 0 && (fdata = awt_GetFontData(font, 0)) != 0) {
	XtSetArg(args[argc], XmNfontList, fdata->xmfontlist); argc++;
    }
    mdata->comp.widget = XmCreateCascadeButton(menuParent, "", args, argc);
    if (targetPtr->isHelpMenu != 0) {
	XtVaSetValues(menuParent,
		      XmNmenuHelpWidget, mdata->comp.widget,
		      NULL);
    }
		      
    XtManageChild(mdata->comp.widget);
    XtSetSensitive(mdata->comp.widget, targetPtr->enabled ? True : False);
}

void
sun_awt_motif_MMenuPeer_createMenu(struct Hsun_awt_motif_MMenuPeer *this,
				   struct Hjava_awt_MenuBar *parent)
{
    struct ComponentData	*mbdata;

    AWT_LOCK();
    if (parent == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }	

    mbdata = PEER_PDATA(ComponentData,Hsun_awt_motif_MMenuBarPeer,parent);
    if (mbdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    awt_createMenu(this, mbdata->widget);

    AWT_UNLOCK();
}

void
sun_awt_motif_MMenuPeer_createSubMenu(struct Hsun_awt_motif_MMenuPeer *this,
				      struct Hjava_awt_Menu *parent)
{
    struct MenuData	*mpdata;

    AWT_LOCK();
    if (parent == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }	
    mpdata = PEER_PDATA(MenuData,Hsun_awt_motif_MMenuPeer,parent);
    if (mpdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    awt_createMenu(this, mpdata->itemData.comp.widget);
    AWT_UNLOCK();
}

void
sun_awt_motif_MMenuPeer_dispose(struct Hsun_awt_motif_MMenuPeer *this)
{
    struct MenuData *mdata;

    AWT_LOCK();
    if ((mdata = PDATA(MenuData,this)) == 0) {
	AWT_UNLOCK();
	return;
    }
    XtUnmanageChild(mdata->comp.widget);
    XtDestroyWidget(mdata->itemData.comp.widget);
    XtDestroyWidget(mdata->comp.widget);
    sysFree((void *)mdata);
    AWT_UNLOCK();
}

