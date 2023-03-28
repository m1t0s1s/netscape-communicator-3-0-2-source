/*
 * @(#)awt_Checkbox.c	1.14 95/11/13 Sami Shaio
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
#include "sun_awt_motif_MComponentPeer.h"
#include "sun_awt_motif_MCheckboxPeer.h"
#include "java_awt_Checkbox.h"
#include "java_awt_CheckboxGroup.h"
#include "ustring.h"

static void
Toggle_callback(Widget w,
		XtPointer client_data,
		XmAnyCallbackStruct *call_data)
{
    Boolean state;
    XtVaGetValues(w,  XmNset, &state,  NULL);
    JAVA_UPCALL((EE(), (void *)client_data, "action","(Z)V", state));
}

void
sun_awt_motif_MCheckboxPeer_create(struct Hsun_awt_motif_MCheckboxPeer *this,
                                   struct Hsun_awt_motif_MComponentPeer *parent)
{
    struct Hjava_awt_Checkbox 	*target;
    struct ComponentData	*bdata;
    struct ComponentData	*wdata;
    XmString			xLabel;
    Arg				args[10];
    int				argc;

    AWT_LOCK();
    if (parent == 0 || unhand(parent)->pData == 0 || unhand(this)->target == 0) {
	SignalError(0,JAVAPKG "NullPointerException","null parent");
	AWT_UNLOCK();
	return;
    }
    target = (struct Hjava_awt_Checkbox *)unhand(this)->target;
    wdata = (struct ComponentData *)unhand(parent)->pData;
    bdata = ZALLOC(ComponentData);
    SET_PDATA(this, bdata);
    if (bdata == 0) {
	SignalError(0,JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    
    xLabel = makeXmString( unhand(target)->label );
    if( xLabel == NULL ) {
       AWT_UNLOCK();
       return;
    }
    
    argc=0;
    XtSetArg(args[argc], XmNrecomputeSize, False); argc++;
    XtSetArg(args[argc], XmNvisibleWhenOff, True); argc++;
    XtSetArg(args[argc], XmNtraversalOn, False); argc++;
    XtSetArg(args[argc], XmNspacing, 0); argc++;
    XtSetArg(args[argc], XmNlabelString, xLabel); argc++;
    
    bdata->widget = XmCreateToggleButton(wdata->widget, "", args, argc);
    XtAddCallback(bdata->widget,
		  XmNvalueChangedCallback,
		  (XtCallbackProc)Toggle_callback,
		  (XtPointer)this);

    XmStringFree( xLabel );
    XtSetMappedWhenManaged(bdata->widget, False);
    XtManageChild(bdata->widget);
    AWT_UNLOCK();
}

void 
sun_awt_motif_MCheckboxPeer_setCheckboxGroup(struct Hsun_awt_motif_MCheckboxPeer *this,
					struct Hjava_awt_CheckboxGroup *group)
{
    struct ComponentData *bdata;

    AWT_LOCK();
    if ((bdata = PDATA(ComponentData,this)) == 0 || bdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if (group == 0) {
	XtVaSetValues(bdata->widget,
		      XmNindicatorType, XmN_OF_MANY,
		      NULL);
    } else {
	XtVaSetValues(bdata->widget,
		      XmNindicatorType, XmONE_OF_MANY,
		      NULL);
    }
    AWT_FLUSH_UNLOCK();
}


void 
sun_awt_motif_MCheckboxPeer_setState(struct Hsun_awt_motif_MCheckboxPeer *this,
				   long state)
{
    struct ComponentData *bdata;

    AWT_LOCK();
    if ((bdata = PDATA(ComponentData,this)) == 0 || bdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaSetValues(bdata->widget, XmNset, (Boolean)state, NULL);
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MCheckboxPeer_setLabel(struct Hsun_awt_motif_MCheckboxPeer *this,
                                     struct Hjava_lang_String *label)
{
    struct ComponentData	*wdata;
    XmString			xLabel;

    AWT_LOCK();
    wdata = PDATA(ComponentData, this);
    if (wdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    
    xLabel = makeXmString(label);
    if( xLabel == NULL ) {
        AWT_UNLOCK();
        return;
    }

    XtVaSetValues(wdata->widget, XmNlabelString, xLabel, NULL);
    XmStringFree(xLabel);
    
    AWT_FLUSH_UNLOCK();
}
