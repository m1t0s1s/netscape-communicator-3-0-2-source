/*
 * @(#)awt_MenuItem.c	1.17 95/12/08 Sami Shaio
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
#include <Xm/Separator.h>
#include "java_awt_MenuItem.h"
#include "java_awt_Font.h"
#include "sun_awt_motif_MMenuItemPeer.h"
#include "sun_awt_motif_MCheckboxMenuItemPeer.h"
#include "java_awt_Menu.h"
#include "sun_awt_motif_MMenuPeer.h"
#include "ustring.h"

static void
MenuItem_selected(Widget w, XtPointer client_data, XmAnyCallbackStruct *s)
{
    extern int getModifiers(int state);
    int modifiers = getModifiers(s->event->xbutton.state);
    int64_t when = int2ll(s->event->xbutton.time);
    struct Hsun_awt_motif_MMenuPeer* this = (struct Hsun_awt_motif_MMenuPeer *)client_data;

    if (unhand(this)->isCheckbox) {
	Boolean state;
	struct MenuItemData *mdata;
	if ((mdata = PDATA(MenuItemData,this)) != NULL) {
	    XtVaGetValues(mdata->comp.widget, XmNset, &state, NULL);
	    JAVA_UPCALL((EE(), (void *)this, "action", "(JIZ)V", when, modifiers, state));
	}
    } else {
	JAVA_UPCALL((EE(), (void *)this, "action", "(JI)V", when, modifiers));
    }
}

static Boolean
MenuItem_isSeperator( Hjava_lang_String* javaStringHandle )
{
   if( javaStringHandle == NULL )
   {
      return FALSE;
   }
   else if( javaStringLength( javaStringHandle ) != 1 )
   {
      return FALSE;
   }
   else
   {
      unicode* uniChars;
      Classjava_lang_String* javaStringClass = unhand( javaStringHandle );
      HArrayOfChar* unicodeArrayHandle = (HArrayOfChar *)javaStringClass->value;
      uniChars =  unhand(unicodeArrayHandle)->body + javaStringClass->offset;
      if( uniChars[0] == '-' )
         return TRUE;
      else
         return FALSE;
   }   
}

void
sun_awt_motif_MMenuItemPeer_create(struct Hsun_awt_motif_MMenuItemPeer *this,
				   struct Hjava_awt_Menu *parent)
{
    int				argc;
    Arg				args[10];
    XmString			xLabel;
    struct MenuData		*menuData;
    struct MenuItemData 	*mdata;
    struct FontData		*fdata;
    Pixel			bg;
    Classjava_awt_MenuItem	*targetPtr;

    if (parent == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    AWT_LOCK();
    targetPtr = unhand(unhand(this)->target);
    menuData = PEER_PDATA(MenuData,Hsun_awt_motif_MMenuPeer,parent);
    
    xLabel = makeXmString(targetPtr->label);
    if( xLabel == NULL ) {
        AWT_UNLOCK();
        return;
    }

    mdata = ZALLOC(MenuItemData);
    SET_PDATA(this, mdata);

    argc = 0;
    XtSetArg(args[argc], XmNbackground, &bg); argc++;
    XtGetValues(menuData->itemData.comp.widget, args, argc);

    argc = 0;
    XtSetArg(args[argc], XmNbackground, bg); argc++;
    if (MenuItem_isSeperator(targetPtr->label)) {
	mdata->comp.widget =  XmCreateSeparator(menuData->itemData.comp.widget,
						"", args, argc);
    } else {
       /* It is not sufficient to get the target's font from the target's font data
        * field.  MenuComponent.getFont() will return the the parent's font if
        * the target's own font is nil.
        */
        struct Hjava_awt_Font* font = NULL; 
        JAVA_UPCALL_WITH_RETURN( (EE(), (void*)unhand(this)->target,
                                  "getFont", "()Ljava/awt/Font;"),
                                 font, (struct Hjava_awt_Font*) );
	if (font != 0 && (fdata = awt_GetFontData(font, 0)) != 0) {
	    XtSetArg(args[argc], XmNfontList, fdata->xmfontlist); argc++;
	}
        
	XtSetArg(args[argc], XmNlabelString, xLabel);  argc++;

	if (unhand(this)->isCheckbox != 0) {
	    XtSetArg(args[argc], XmNset, False); argc++;
	    XtSetArg(args[argc], XmNvisibleWhenOff, True); argc++;
	    mdata->comp.widget = XmCreateToggleButton(menuData->itemData.comp.widget,
						      "", 
						      args,
						      argc);
	} else {
	    mdata->comp.widget = XmCreatePushButton(menuData->itemData.comp.widget,
						    "",
						    args,
						    argc);
	}
	XtAddCallback(mdata->comp.widget,
		      ((unhand(this)->isCheckbox != 0) ?
		       XmNvalueChangedCallback : XmNactivateCallback),
		      (XtCallbackProc)MenuItem_selected,
		      (XtPointer)this);
	XtSetSensitive(mdata->comp.widget, targetPtr->enabled ? True : False);
    }
    XtManageChild(mdata->comp.widget);
    AWT_UNLOCK();
}

void
sun_awt_motif_MMenuItemPeer_enable(struct Hsun_awt_motif_MMenuItemPeer *this)
{
    struct MenuItemData *mdata;

    AWT_LOCK();
    if ((mdata = PDATA(MenuItemData,this)) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtSetSensitive(mdata->comp.widget, True);
    AWT_UNLOCK();
}

void
sun_awt_motif_MMenuItemPeer_disable(struct Hsun_awt_motif_MMenuItemPeer *this)
{
    struct MenuItemData *mdata;

    AWT_LOCK();
    if ((mdata = PDATA(MenuItemData,this)) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtSetSensitive(mdata->comp.widget, False);
    AWT_UNLOCK();
}

void
sun_awt_motif_MMenuItemPeer_dispose(struct Hsun_awt_motif_MMenuItemPeer *this)
{
    struct MenuItemData *mdata;

    AWT_LOCK();
    if ((mdata = PDATA(MenuItemData,this)) != 0) {
	XtUnmanageChild(mdata->comp.widget);
	XtDestroyWidget(mdata->comp.widget);
	sysFree((void *)mdata);
	SET_PDATA(this, 0);
    }
    AWT_UNLOCK();
}

void
sun_awt_motif_MMenuItemPeer_setLabel(struct Hsun_awt_motif_MMenuItemPeer *this,
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
    
    AWT_UNLOCK();
}


void
sun_awt_motif_MCheckboxMenuItemPeer_setState(struct Hsun_awt_motif_MCheckboxMenuItemPeer *this,
					 long state)
{
    struct MenuItemData *mdata;

    AWT_LOCK();
    if ((mdata = PDATA(MenuItemData,this)) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    XtVaSetValues(mdata->comp.widget, XmNset, (Boolean)state, NULL);
    AWT_UNLOCK();
}

long
sun_awt_motif_MCheckboxMenuItemPeer_getState(struct Hsun_awt_motif_MCheckboxMenuItemPeer *this)
{
    struct MenuItemData *mdata;
    Boolean		state;

    AWT_LOCK();
    if ((mdata = PDATA(MenuItemData,this)) == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return 0;
    }
    XtVaGetValues(mdata->comp.widget, XmNset, &state, NULL);
    AWT_UNLOCK();

    return (long)state;
}

