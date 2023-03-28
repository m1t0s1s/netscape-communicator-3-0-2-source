/*
 * @(#)awt_MenuBar.c	1.12 95/12/08 Sami Shaio
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
#include "java_awt_MenuBar.h"
#include "sun_awt_motif_MMenuBarPeer.h"
#include "java_awt_Menu.h"
#include "java_awt_Frame.h"
#include "sun_awt_motif_MFramePeer.h"

void
sun_awt_motif_MMenuBarPeer_create(struct Hsun_awt_motif_MMenuBarPeer *this,
				  struct Hjava_awt_Frame *frame)
{
    Arg				args[20];
    int				argc;
    struct ComponentData	*mdata; 
    struct FrameData		*wdata;
    Pixel			bg;

    if (frame == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    wdata = PEER_PDATA(FrameData,Hsun_awt_motif_MFramePeer,frame);
    mdata = ZALLOC(ComponentData);

    if (wdata == 0 || mdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    SET_PDATA(this, mdata);
    XtVaGetValues(wdata->winData.comp.widget,
		  XmNbackground, &bg,
		  NULL);

    argc=0;
    XtSetArg(args[argc], XmNbackground, bg); argc++;
    XtSetArg(args[argc], XmNheight, 0); argc++;
    mdata->widget = XmCreateMenuBar(wdata->mainWindow, "menu_bar", args, argc);
    XtSetMappedWhenManaged(mdata->widget, False);
    XtManageChild(mdata->widget);
    AWT_UNLOCK();
}

void
sun_awt_motif_MMenuBarPeer_dispose(struct Hsun_awt_motif_MMenuBarPeer *this)
{
    struct ComponentData *mdata;

    AWT_LOCK();
    if ((mdata = PDATA(ComponentData,this)) == 0) {
	AWT_UNLOCK();
	return;
    }
    XtUnmanageChild(mdata->widget);
    XtDestroyWidget(mdata->widget);
    sysFree((void *)mdata);
    SET_PDATA(this, 0);
    AWT_UNLOCK();
}
