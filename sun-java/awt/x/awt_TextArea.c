/*
 * @(#)awt_TextArea.c	1.15 95/11/27 Sami Shaio
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
#include "sun_awt_motif_MTextAreaPeer.h"
#include "java_awt_Component.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "ustring.h"

static void
TextArea_focusIn(Widget w, XtPointer client_data, XmAnyCallbackStruct *s)
{
    JAVA_UPCALL((EE(),(void *)client_data, "gotFocus","()V"));
}

static void
TextArea_focusOut(Widget w, XtPointer client_data, XmAnyCallbackStruct *s)
{
    JAVA_UPCALL((EE(),(void *)client_data, "lostFocus","()V"));
}


void
sun_awt_motif_MTextAreaPeer_create(struct Hsun_awt_motif_MTextAreaPeer *this,
				   struct Hsun_awt_motif_MComponentPeer *parent)
{
    struct TextAreaData	*tdata;
    Arg				args[30];
    int				argc;
    struct ComponentData	*wdata;
    Pixel			bg;

    AWT_LOCK();
    if (parent == 0 || unhand(parent)->pData == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    tdata = ZALLOC(TextAreaData);
    SET_PDATA(this, tdata);

    if (!tdata) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    wdata = (struct ComponentData *)unhand(parent)->pData;

    XtVaGetValues(wdata->widget, XmNbackground, &bg, NULL);
    argc = 0;
    XtSetArg(args[argc], XmNrecomputeSize, False); argc++;
    XtSetArg(args[argc], XmNx, 0); argc++;
    XtSetArg(args[argc], XmNy, 0); argc++;
    XtSetArg(args[argc], XmNbackground, bg); argc++;
    XtSetArg(args[argc], XmNeditMode, XmMULTI_LINE_EDIT); argc++;
    XtSetArg(args[argc], XmNwordWrap, True); argc++;
    XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
    tdata->txt = XmCreateScrolledText(wdata->widget, "textA",
					 args, argc);
    tdata->comp.widget = XtParent(tdata->txt);
    XtSetMappedWhenManaged(tdata->comp.widget, False);
    XtManageChild(tdata->txt);
    XtManageChild(tdata->comp.widget);

    XtAddCallback(tdata->txt,
		  XmNfocusCallback,
		  (XtCallbackProc)TextArea_focusIn,
		  (XtPointer)this);
    XtAddCallback(tdata->txt,
 		  XmNlosingFocusCallback,
 		  (XtCallbackProc)TextArea_focusOut,
 		  (XtPointer)this);
    AWT_UNLOCK();
}


void
sun_awt_motif_MTextAreaPeer_pSetEditable(struct Hsun_awt_motif_MTextAreaPeer *this,
				    /*boolean*/ long editable)
{
    struct TextAreaData	*tdata;

    AWT_LOCK();
    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaSetValues(tdata->txt, 
		  XmNeditable, (editable ? True : False),
		  XmNcursorPositionVisible, (editable ? True : False),
		  NULL);
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MTextAreaPeer_setTextBackground(struct Hsun_awt_motif_MTextAreaPeer *this,
					      struct Hjava_awt_Color *c)
{
    struct TextAreaData	*tdata;
    Pixel	color;

    AWT_LOCK();
    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0 || c==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    color = awt_getColor(c);
    XtVaSetValues(tdata->txt, 
		  XmNbackground, color,
		  NULL);
    AWT_FLUSH_UNLOCK();
}


void
sun_awt_motif_MTextAreaPeer_select(struct Hsun_awt_motif_MTextAreaPeer *this, long start, long end)
{
    struct TextAreaData	*tdata;

    AWT_LOCK();
    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    XmTextSetSelection(tdata->txt, (XmTextPosition)start, (XmTextPosition)end, 0);
    AWT_FLUSH_UNLOCK();
}

long
sun_awt_motif_MTextAreaPeer_getSelectionStart(struct Hsun_awt_motif_MTextAreaPeer *this)
{
    struct TextAreaData	*tdata;
    XmTextPosition	start, end, pos;

    AWT_LOCK();
    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return 0;
    }
    if (XmTextGetSelectionPosition(tdata->txt, &start, &end)) {
	pos = start;
    } else {
	pos = XmTextGetCursorPosition(tdata->txt);
    }
    AWT_UNLOCK();

    return pos;
}

long
sun_awt_motif_MTextAreaPeer_getSelectionEnd(struct Hsun_awt_motif_MTextAreaPeer *this)
{
    struct TextAreaData	*tdata;
    XmTextPosition	start, end, pos;

    AWT_LOCK();
    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return 0;
    }
    if (XmTextGetSelectionPosition(tdata->txt, &start, &end)) {
	pos = end;
    } else {
	pos = XmTextGetCursorPosition(tdata->txt);
    }
    AWT_UNLOCK();

    return pos;
}

long
sun_awt_motif_MTextAreaPeer_getCursorPos(struct Hsun_awt_motif_MTextAreaPeer *this)
{
    struct TextAreaData	*tdata;
    XmTextPosition	pos;

    AWT_LOCK();
    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return 0;
    }
    pos = XmTextGetCursorPosition(tdata->txt);
    AWT_UNLOCK();

    return pos;
}

void
sun_awt_motif_MTextAreaPeer_setCursorPos(struct Hsun_awt_motif_MTextAreaPeer *this,
				     long pos)
{
    struct TextAreaData	*tdata;

    AWT_LOCK();
    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XmTextSetCursorPosition(tdata->txt, (XmTextPosition)pos);
    AWT_FLUSH_UNLOCK();
}

long
sun_awt_motif_MTextAreaPeer_endPos(struct Hsun_awt_motif_MTextAreaPeer *this)
{
    struct TextAreaData	*tdata;
    XmTextPosition	pos;

    AWT_LOCK();
    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return 0;
    }
    pos = XmTextGetLastPosition(tdata->txt);
    AWT_UNLOCK();

    return pos;
}

void
sun_awt_motif_MTextAreaPeer_setText(struct Hsun_awt_motif_MTextAreaPeer *this,
				Hjava_lang_String *txt)
{
    struct TextAreaData	*tdata;
    char		*cTxt;

    if (txt == 0) {
	SignalError(0, JAVAPKG "NullPointerException",0);
	return;
    }
    AWT_LOCK();

    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    cTxt = makeLocalizedCString(txt);
    XtVaSetValues(tdata->txt, XmNvalue, cTxt, NULL);
    AWT_FLUSH_UNLOCK();
}

Hjava_lang_String *
sun_awt_motif_MTextAreaPeer_getText(struct Hsun_awt_motif_MTextAreaPeer *this)
{
    struct TextAreaData	*tdata;
    char			*cTxt;
    Hjava_lang_String		*rval;

    AWT_LOCK();
    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return 0;
    }
    cTxt = XmTextGetString(tdata->txt);
    rval = makeJavaStringFromLocalizedCString(cTxt);
    XtFree(cTxt);
    AWT_UNLOCK();

    return rval;
}

void
sun_awt_motif_MTextAreaPeer_insertText(struct Hsun_awt_motif_MTextAreaPeer *this,
				   Hjava_lang_String *txt,
				   long pos)
{
    struct TextAreaData	*tdata;
    char			*cTxt;

    if (txt == 0) {
	SignalError(0, JAVAPKG "NullPointerException",0);
	return;
    }
    AWT_LOCK();
    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    cTxt = makeLocalizedCString(txt);
    XmTextInsert(tdata->txt, (XmTextPosition)pos, cTxt);
    AWT_FLUSH_UNLOCK();
}

void
sun_awt_motif_MTextAreaPeer_replaceText(struct Hsun_awt_motif_MTextAreaPeer *this,
				    Hjava_lang_String *txt,
				    long start,
				    long end)
{
    struct TextAreaData	*tdata;
    char			*cTxt;

    if (txt == 0) {
	SignalError(0, JAVAPKG "NullPointerException",0);
	return;
    }
    
    AWT_LOCK();
    if ((tdata = PDATA(TextAreaData,this)) == 0 || tdata->txt==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    cTxt = makeLocalizedCString(txt);
    XmTextReplace(tdata->txt,
		  (XmTextPosition)start,
		  (XmTextPosition)end,
		  cTxt);
    AWT_FLUSH_UNLOCK();
}
