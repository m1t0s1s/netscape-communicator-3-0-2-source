/*
 * @(#)awt_TextField.c	1.28 95/11/30 Sami Shaio
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
#include "java_awt_Canvas.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "sun_awt_motif_MCanvasPeer.h"
#include "java_awt_TextField.h"
#include "sun_awt_motif_MTextFieldPeer.h"
#include "ustring.h"

#define ECHO_BUFFER_LEN 1024

static void
TextField_changed(Widget w, XtPointer client_data, XmAnyCallbackStruct *s)
{
    JAVA_UPCALL((EE(), (void *)client_data,"action","()V"));
}

static void
TextField_focusIn(Widget w, XtPointer client_data, XmAnyCallbackStruct *s)
{
    JAVA_UPCALL((EE(),(void *)client_data, "gotFocus","()V"));
}

static void
TextField_focusOut(Widget w, XtPointer client_data, XmAnyCallbackStruct *s)
{
    JAVA_UPCALL((EE(),(void *)client_data, "lostFocus","()V"));
}

void
sun_awt_motif_MTextFieldPeer_create(struct Hsun_awt_motif_MTextFieldPeer *this,
				    struct Hsun_awt_motif_MComponentPeer *parent)
{
    struct ComponentData *wdata;
    struct ComponentData *cdata;

    AWT_LOCK();
    if (parent==0) {
	SignalError(0, JAVAPKG "NullPointerException",0);
	AWT_UNLOCK();
	return;
    }
    wdata = (struct ComponentData *)unhand(parent)->pData;
    cdata = ZALLOC(ComponentData);
    SET_PDATA(this, cdata);
    if (cdata == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    
    cdata->widget = XtVaCreateManagedWidget("textfield",
					    xmTextFieldWidgetClass,
					    wdata->widget,
					    XmNrecomputeSize, False,
					    XmNhighlightThickness, 1,
					    XmNshadowThickness, 2,
                                            NULL);
    XtSetMappedWhenManaged(cdata->widget, False);
    XtAddCallback(cdata->widget,
 		  XmNactivateCallback,
 		  (XtCallbackProc)TextField_changed,
 		  (XtPointer)this);
    XtAddCallback(cdata->widget,
 		  XmNfocusCallback,
 		  (XtCallbackProc)TextField_focusIn,
 		  (XtPointer)this);
    XtAddCallback(cdata->widget,
 		  XmNlosingFocusCallback,
 		  (XtCallbackProc)TextField_focusOut,
 		  (XtPointer)this);
    AWT_UNLOCK();
}

struct Hjava_lang_String *
sun_awt_motif_MTextFieldPeer_getText(struct Hsun_awt_motif_MTextFieldPeer *this)
{
    struct ComponentData *cdata = PDATA(ComponentData,this);
    char                 *val;
    struct DPos          *dp;
    Hjava_awt_TextField  *target;

    if (cdata == 0 || cdata->widget==0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return 0;
    }
    AWT_LOCK();
    target = (struct Hjava_awt_TextField *)unhand(this)->target;
    if (unhand(target)->echoChar != 0) {
        XtVaGetValues(cdata->widget,  XmNuserData, &dp,  NULL);
        val = (char *)(dp->data);
    } else {
        XtVaGetValues(cdata->widget,  XmNvalue, &val,  NULL);
    }
    AWT_UNLOCK();
    return makeJavaStringFromLocalizedCString(val);
}

void 
sun_awt_motif_MTextFieldPeer_setText(struct Hsun_awt_motif_MTextFieldPeer *this,
                                     struct Hjava_lang_String *l)
{
    struct ComponentData	*cdata = PDATA(ComponentData,this);
    char			*cl;

    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    cl = makeLocalizedCString( l );
    if( cl == NULL )
        return;

    AWT_LOCK();
    XtVaSetValues(cdata->widget,
		  XmNvalue, cl,
		  NULL);
    XmTextSetCursorPosition(cdata->widget, (XmTextPosition)strlen(cl));
    AWT_FLUSH_UNLOCK();
}


void
sun_awt_motif_MTextFieldPeer_pSetEditable(struct Hsun_awt_motif_MTextFieldPeer *this, long editable)
{
    struct ComponentData *cdata = PDATA(ComponentData,this);

    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    XtVaSetValues(cdata->widget,
		  XmNeditable, (editable ? True : False),
		  XmNcursorPositionVisible, (editable ? True : False),
		  NULL);
    AWT_FLUSH_UNLOCK();
}


void
sun_awt_motif_MTextFieldPeer_dispose(struct Hsun_awt_motif_MTextFieldPeer *this)
{
    struct ComponentData		*cdata = PDATA(ComponentData,this);
    struct DPos *dp;
    Hjava_awt_TextField	*target;

    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    target = (struct Hjava_awt_TextField *)unhand(this)->target;
    if (unhand(target)->echoChar != 0) {
	XtVaGetValues(cdata->widget, XmNuserData, &dp, NULL);
	if (dp) {
	    if (dp->data) {
		sysFree(dp->data);
	    }
	    sysFree(dp);
	}
    }

    awt_util_hide(cdata->widget);    
    XtDestroyWidget(cdata->widget);
    sysFree((void *)cdata);
    SET_PDATA(this, 0);
    AWT_UNLOCK();
}

static void
echoChar(Widget text_w, XtPointer c, XmTextVerifyCallbackStruct *cbs)
{
    int len;
    char *val;
    struct DPos *dp;

    XtVaGetValues(text_w,
		  XmNuserData, &dp,
		  NULL);
    val = (char *)(dp->data);
    len = strlen(val);
    if (cbs->text->ptr == NULL) {
	if (cbs->text->length == 0 && cbs->startPos == 0) {
	    val[0] = 0;
	    return;
	} else if (cbs->startPos == (len - 1)) {
	    /* handle deletion */
	    cbs->endPos = strlen(val);
	    val[cbs->startPos] = 0;
	    return;
	} else {
	    /* disable deletes anywhere but at the end */
	    cbs->doit = False;
	    return;
	}
    }
    if (cbs->startPos != len) {
	/* disable "paste" or inserts into the middle */
	cbs->doit = False;
	return;
    }
    /* append the value typed in */
    if ((cbs->endPos + cbs->text->length) > ECHO_BUFFER_LEN) {
	val = sysRealloc(val, cbs->endPos + cbs->text->length + 10);
    }
    strncat(val, cbs->text->ptr, cbs->text->length);
    val[cbs->endPos + cbs->text->length] = 0;

    /* modify the output to be the echo character */
    for (len = 0; len < cbs->text->length; len++) {
	cbs->text->ptr[len] = (char)c;
    }
}

void 
sun_awt_motif_MTextFieldPeer_setEchoCharacter(struct Hsun_awt_motif_MTextFieldPeer *this, unicode c)
{
    char *val;
    char *cval;
    struct ComponentData *cdata;
    struct DPos *dp;
    int i, len;

    cdata = PDATA(ComponentData,this);
    if (cdata == 0 || cdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    XtVaGetValues(cdata->widget,
		  XmNvalue, &cval,
		  NULL);

    if ((int)strlen(cval) > ECHO_BUFFER_LEN) {
	val = (char *)sysMalloc(strlen(cval) + 1);
    } else {
	val = (char *)sysMalloc(ECHO_BUFFER_LEN+1);
    }
    if (val == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    if (cval) {
	strcpy(val, cval);
    } else {
	*val = 0;
    }
    dp = (struct DPos *)sysMalloc(sizeof(struct DPos));
    dp->x = -1;
    dp->data = (void *)val;
    len = strlen(cval);
    for (i = 0; i < len; i++) {
	cval[i] = (char)(c);
    }
    XtVaSetValues(cdata->widget,
		  XmNvalue, cval,
		  NULL);
    XtAddCallback(cdata->widget, XmNmodifyVerifyCallback,
		  (XtCallbackProc)echoChar, (XtPointer)c);
    XtVaSetValues(cdata->widget,
		  XmNuserData, (XtPointer)dp,
		  NULL);
    AWT_UNLOCK();
}

void
sun_awt_motif_MTextFieldPeer_select(struct Hsun_awt_motif_MTextFieldPeer *this, long start, long end)
{
    struct ComponentData	*tdata;

    AWT_LOCK();
    if ((tdata = PDATA(ComponentData,this)) == 0 || tdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    XmTextSetSelection(tdata->widget, (XmTextPosition)start, (XmTextPosition)end, 0);
    AWT_FLUSH_UNLOCK();
}

long
sun_awt_motif_MTextFieldPeer_getSelectionStart(struct Hsun_awt_motif_MTextFieldPeer *this)
{
    struct ComponentData	*tdata;
    XmTextPosition	start, end, pos;

    AWT_LOCK();
    if ((tdata = PDATA(ComponentData,this)) == 0 || tdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return 0;
    }
    if (XmTextGetSelectionPosition(tdata->widget, &start, &end)) {
	pos = start;
    } else {
	pos = XmTextGetCursorPosition(tdata->widget);
    }
    AWT_UNLOCK();

    return pos;
}

long
sun_awt_motif_MTextFieldPeer_getSelectionEnd(struct Hsun_awt_motif_MTextFieldPeer *this)
{
    struct ComponentData	*tdata;
    XmTextPosition	start, end, pos;

    AWT_LOCK();
    if ((tdata = PDATA(ComponentData,this)) == 0 || tdata->widget==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return 0;
    }
    if (XmTextGetSelectionPosition(tdata->widget, &start, &end)) {
	pos = end;
    } else {
	pos = XmTextGetCursorPosition(tdata->widget);
    }
    AWT_UNLOCK();

    return pos;

}
