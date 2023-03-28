/*
 * @(#)awt_Button.c     1.23 95/11/29 Sami Shaio
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
#include "ustring.h"
#include "java_awt_Button.h"
#include "sun_awt_motif_MButtonPeer.h"
#include "sun_awt_motif_MComponentPeer.h"


static void
Button_callback(Widget w,
                XtPointer client_data,
                XmPushButtonCallbackStruct *call_data)
{
    JAVA_UPCALL((EE(),
                (void *)client_data,
                "action", "()V",
                (void *)client_data));
}

void
sun_awt_motif_MButtonPeer_create(struct Hsun_awt_motif_MButtonPeer *this,
                                 struct Hsun_awt_motif_MComponentPeer *parent)
{
    struct Hjava_awt_Button     *target;
    struct ComponentData        *cdata;
    struct ComponentData        *wdata;
    Pixel                       bg;
    XmString                    xLabel;

    if (parent == 0 ||
        unhand(parent)->pData == 0 ||
        unhand(this)->target == 0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return;
    }

    AWT_LOCK();
    target = (struct Hjava_awt_Button *)unhand(this)->target;
    wdata = (struct ComponentData *)unhand(parent)->pData;
    cdata = ZALLOC(ComponentData);
    if (cdata == 0) {
        SignalError(0, JAVAPKG "OutOfMemoryError", 0);
        AWT_UNLOCK();
        return;
    }
    SET_PDATA(this, cdata);

    XtVaGetValues(wdata->widget, XmNbackground, &bg, NULL);

    xLabel = makeXmString( unhand(target)->label );
    if( xLabel == NULL ) {
       AWT_UNLOCK();
       return;
    }

    cdata->widget = XtVaCreateManagedWidget("", xmPushButtonWidgetClass,
                                            wdata->widget,
                                            XmNlabelString, xLabel,
                                            XmNrecomputeSize, False,
                                            XmNbackground, bg,
                                            XmNhighlightOnEnter, False,
                                            XmNshowAsDefault, 0,
                                            XmNdefaultButtonShadowThickness, 0,
                                            XmNmarginTop, 0,
                                            XmNmarginBottom, 0,
                                            XmNmarginLeft, 0,
                                            XmNmarginRight, 0,
                                            NULL);
    XmStringFree( xLabel );
    
    XtSetMappedWhenManaged(cdata->widget, False);
    XtAddCallback(cdata->widget,
                  XmNactivateCallback,
                  (XtCallbackProc)Button_callback,
                  (XtPointer)this);

    AWT_UNLOCK();
}


void
sun_awt_motif_MButtonPeer_setLabel(struct Hsun_awt_motif_MButtonPeer *this,
                                   struct Hjava_lang_String *label)
{
    struct ComponentData        *wdata;
    XmString                    xLabel;

    AWT_LOCK();
    wdata = PDATA(ComponentData, this);
    if (wdata == 0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        AWT_UNLOCK();
        return;
    }

    xLabel = makeXmString( label );
    if( xLabel == NULL ) {
        AWT_UNLOCK();
        return;
    }
    
    XtVaSetValues(wdata->widget, XmNlabelString, xLabel, NULL);
    XmStringFree( xLabel );

    AWT_FLUSH_UNLOCK();
}
