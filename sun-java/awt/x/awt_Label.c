/*
 * @(#)awt_Label.c      1.16 95/11/12 Sami Shaio
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
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Label.h"
#include "sun_awt_motif_MLabelPeer.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "ustring.h"


void
sun_awt_motif_MLabelPeer_create(struct Hsun_awt_motif_MLabelPeer *this,
                                struct Hsun_awt_motif_MComponentPeer *parent)
{
    struct ComponentData        *cdata;
    struct ComponentData        *wdata;

    if (parent == 0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return;
    }
    AWT_LOCK();
    wdata = (struct ComponentData *)unhand(parent)->pData;
    cdata = ZALLOC(ComponentData);
    if (cdata == 0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        AWT_UNLOCK();
        return;
    }
    SET_PDATA(this, cdata);
    cdata->widget = XtVaCreateManagedWidget("",
                                            xmLabelWidgetClass, wdata->widget,
                                            XmNhighlightThickness, 0,
                                            XmNalignment, XmALIGNMENT_BEGINNING,
                                            XmNrecomputeSize, False,
                                            NULL);
    XtSetMappedWhenManaged(cdata->widget, False);
    AWT_UNLOCK();
}

void 
sun_awt_motif_MLabelPeer_setText(struct Hsun_awt_motif_MLabelPeer *this,
                                 Hjava_lang_String *label)
{
    struct ComponentData        *cdata;
    XmString                    xLabel;

#if 0
    /* Need to replace this functionality in the new code. */
    char                        *clabel;
    char                        *clabelEnd;
    
    if (label == 0) {
        clabel = "";
    } else {
        clabel = allocCString(label);
        /* scan for any \n's and terminate the string at that point */
        clabelEnd = strchr(clabel,'\n');
        if (clabelEnd != 0) {
            *clabelEnd = 0;
        }
    }
#endif
    
    AWT_LOCK();
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        AWT_UNLOCK();
        return;
    }
    
    xLabel = makeXmString( label );
    if( xLabel == NULL ) {
        AWT_UNLOCK();
        return;
    }
    XtVaSetValues(cdata->widget,  XmNlabelString, xLabel,  NULL);
    XmStringFree(xLabel);
    
    AWT_FLUSH_UNLOCK();
}


void 
sun_awt_motif_MLabelPeer_setAlignment(struct Hsun_awt_motif_MLabelPeer *this,
                                      long alignment)
{
    struct ComponentData *cdata;
    AWT_LOCK();
    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        AWT_UNLOCK();
        return;
    }
    switch (alignment) {
      case java_awt_Label_LEFT:
        XtVaSetValues(cdata->widget,
                      XmNalignment, XmALIGNMENT_BEGINNING,
                      NULL);
        break;
      case java_awt_Label_CENTER:
        XtVaSetValues(cdata->widget,
                      XmNalignment, XmALIGNMENT_CENTER,
                      NULL);
        break;
      case java_awt_Label_RIGHT:
        XtVaSetValues(cdata->widget,
                      XmNalignment, XmALIGNMENT_END,
                      NULL);
        break;
      default:
        break;
    }
    AWT_FLUSH_UNLOCK();
}

