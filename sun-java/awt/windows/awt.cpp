/*
 * @(#)awt.cpp	1.76 95/12/09 Thomas Ball
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

#include "stdafx.h" 
#include "awt.h"
#include "awtapp.h"
#include "awt_comp.h"
#include "awt_mainfrm.h"
#include "awt_edit.h"
#include "awt_font.h"
#include "awt_fontmetrics.h"
#include "awt_graphics.h"
#include "awt_image.h"
#include "awt_list.h"
#include "awt_optmenu.h"
#include "awt_window.h"
#include "awt_button.h"
#include "awt_menu.h"
#include "awt_label.h"
#include "resource.h"
#include "awt_filedlg.h"
#include "awt_sbar.h"
#include "awt_event.h"

#if defined(NETSCAPE) && defined(DEBUG)
AwtCList __AllAwtObjects;
#endif /* NETSCAPE */

extern "C" {

#define AWT_LOCK() GetApp()->MonitorEnter()
#define AWT_UNLOCK() GetApp()->MonitorExit()

PR_LOG_DEFINE(AWT);

#if defined(NETSCAPE) && defined(DEBUG)
int DebugPrintf(const char *fmt, ...)
{
    char buffer[512];
    int ret = 0;

    va_list args;
    va_start(args, fmt);

    PR_vsnprintf(buffer, sizeof(buffer), fmt, args);
    PR_LOG(AWT, out, (buffer));

    va_end(args);
    return ret;
}
#endif  // NETSCAPE && DEBUG

// Returns a copy of pStr where \n have been converted to \r\n.
// If the result != 'pStr', 'pStr' has been freed.
char *AddCR(char *pStr, int nStrLen) {
	int nNewlines = 0;
	int i, nResultIx, nLen;
	char *result;

	// count newlines
	for (i=0; pStr[i] != 0; i++) {
		if (pStr[i] == '\n') {
			nNewlines++;
		}
	}
	nLen = i;

	if (nLen + nNewlines + 1 > nStrLen) {
		result = (char *)malloc(nLen + nNewlines + 1);
	} else {
		result = pStr;
	}
	nResultIx = nLen + nNewlines;
	for (i=nLen; i>=0; i--) {
		result[nResultIx--] = pStr[i];
		if (pStr[i] == '\n') {
			result[nResultIx--] = '\r';
		}
	}
	ASSERT(nResultIx == -1);
	if (result != pStr) {
		free(pStr);
	}
	return result;
}

// Converts \r\n pairs to \n.
int RemoveCR(char *pStr) {
	int i, nLen = 0;

	if (pStr) {
		for (i=0; pStr[i] != 0; i++) {
			if (pStr[i] == '\r' && pStr[i + 1] == '\n') {
				i++;
			}
			pStr[nLen++] = pStr[i];
		}
		pStr[nLen] = 0;
	}
	return nLen;
}

// Image related stuff
#include "java_awt_image_ImageObserver.h"

typedef struct Hjava_awt_image_ColorModel *CMHandle;
typedef Classjava_awt_image_ColorModel *CMPtr;
typedef struct Hsun_awt_win32_Win32Image *IHandle;
typedef Classsun_awt_win32_Win32Image *IPtr;
typedef struct Hsun_awt_image_ImageRepresentation *IRHandle;
typedef Classsun_awt_image_ImageRepresentation *IRPtr;
typedef struct Hsun_awt_image_OffScreenImageSource *OSISHandle;
typedef Classsun_awt_image_OffScreenImageSource *OSISPtr;
typedef struct Hjava_awt_Graphics *GHandle;
typedef Classjava_awt_Graphics *GPtr;
typedef struct HArrayOfByte *ByteHandle;
typedef unsigned char *BytePtr;
typedef struct HArrayOfInt *IntHandle;
typedef unsigned int *IntPtr;

#define IMAGE_SIZEINFO (java_awt_image_ImageObserver_WIDTH |	\
			java_awt_image_ImageObserver_HEIGHT)

#define IMAGE_DRAWINFO (java_awt_image_ImageObserver_WIDTH |	\
			java_awt_image_ImageObserver_SOMEBITS |	\
			java_awt_image_ImageObserver_HEIGHT)

#define IMAGE_OFFSCRNINFO (java_awt_image_ImageObserver_WIDTH |		\
			   java_awt_image_ImageObserver_HEIGHT |	\
			   java_awt_image_ImageObserver_SOMEBITS |	\
			   java_awt_image_ImageObserver_ALLBITS)

AwtImage *
image_getpImage(IRHandle irh) {
    AwtImage *pImage = (AwtImage *) irh->obj->pData;
    if (pImage == NULL &&
	(irh->obj->availinfo & IMAGE_SIZEINFO) == IMAGE_SIZEINFO)
    {
	pImage = new AwtImage;
	pImage->Init(irh);
	irh->obj->pData = (long) pImage;
    }
    return pImage;
}

} //extern "C"

/** BUTTON methods **/

void
sun_awt_win32_MButtonPeer_create(struct Hsun_awt_win32_MButtonPeer *self,
				 struct Hsun_awt_win32_MComponentPeer *parent)
{
	CHECKHEAP;
    AWT_LOCK();
    Hjava_awt_Button *selfT = (Hjava_awt_Button*)unhand(self)->target;
    if (parent == 0 || unhand(parent)->pData == 0 || selfT == NULL) {
		SignalError(0, JAVAPKG "NullPointerException", "null parent");
		AWT_UNLOCK();
		return;
    }
    Classjava_awt_Button *selfTC = (Classjava_awt_Button *)unhand(selfT);
	AwtWindow *pParent = (AwtWindow*)unhand(parent)->pData;
	char stackBuffer[64];
	char *buffer = &stackBuffer[0];
	int len = javaStringLength(selfTC->label);

	if (len >= sizeof(stackBuffer)) {
		buffer = (char *)malloc(len + 1);
	}
	javaString2CString(selfTC->label, buffer, len + 1);
	AWT_TRACE(("%x:buttonCreate(%x, %s)\n", self, parent, buffer));
	AwtButton *pButton = AwtButton::CreateButton(selfT, pParent, buffer);
	if (len >= sizeof(stackBuffer)) {
		free(buffer);
	}
	unhand(self)->pData = (long)pButton;

	CHECKHEAP;
    AWT_UNLOCK();
}

void 
sun_awt_win32_MButtonPeer_setColor(struct Hsun_awt_win32_MButtonPeer *self,
					   				   long fg,
					   				   struct Hjava_awt_Color *c)
{
	CHECKHEAP;
    AWT_LOCK();
    if (c == NULL) {
		SignalError(0,JAVAPKG "NullPointerException", "null color");
		AWT_UNLOCK();
		return;
    }
	Classjava_awt_Color *cC = (Classjava_awt_Color *)c->obj;
	COLORREF color = PALETTERGB((cC->value>>16)&0xff, (cC->value>>8)&0xff, (cC->value)&0xff);
	AwtButton *pButton = (AwtButton*)unhand(self);

	AWT_TRACE(("%x:buttonSetColor(%d, %x)\n", self, fg, color));
	pButton->PostMsg(AwtButton::SETCOLOR, unhand(self)->pData, fg, color);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MButtonPeer_setLabel(struct Hsun_awt_win32_MButtonPeer *self,
				   struct Hjava_lang_String *label)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtButton *pButton = (AwtButton*)unhand(self)->pData;
	int len = javaStringLength(label);
	char *buffer;

	buffer = (char *)malloc(len + 1);
	javaString2CString(label, buffer, len + 1);
	AWT_TRACE(("%x:buttonSetLabel(%s)", self, buffer));
	pButton->PostMsg(AwtButton::SETTEXT, (long)buffer);

    AWT_UNLOCK();
	CHECKHEAP;
}


/** CANVAS methods **/

void
sun_awt_win32_MCanvasPeer_create(struct Hsun_awt_win32_MCanvasPeer *self,
				 struct Hsun_awt_win32_MComponentPeer *parent)
{
	CHECKHEAP;
    AWT_LOCK();
    if (parent == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null parent");
		AWT_UNLOCK();
		return;
    }
	AwtWindow *pParent = (AwtWindow *)unhand(parent)->pData;
	Hjava_awt_Canvas *selfT = (Hjava_awt_Canvas*)unhand(self)->target;
	if (selfT == 0) {
	    SignalError(0, JAVAPKG "NullPointerException", "null target");
	    AWT_UNLOCK();
	    return;
	}

	AWT_TRACE(("%x:canvasCreate(%x)\n", self, parent));
	AwtWindow *pWindow = AwtWindow::Create(selfT, pParent,
					       unhand(selfT)->width,
					       unhand(selfT)->height,
					       RGB(192, 192, 192));
	unhand(self)->pData = (long)pWindow;

    AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MCanvasPeer_setColor(struct Hsun_awt_win32_MCanvasPeer *self,
					   				   long fg,
					   				   struct Hjava_awt_Color *c)
{
	CHECKHEAP;
    AWT_LOCK();
    if (c == NULL) {
		SignalError(0,JAVAPKG "NullPointerException", "null color");
		AWT_UNLOCK();
		return;
    }
	Classjava_awt_Color *cC = (Classjava_awt_Color *)c->obj;
	COLORREF color = PALETTERGB((cC->value>>16)&0xff, (cC->value>>8)&0xff, (cC->value)&0xff);
	AwtWindow *pWindow = (AwtWindow *)unhand(self)->pData;

	AWT_TRACE(("%x:canvasSetColor(%d, %x)\n", self, fg, color));
	pWindow->PostMsg(AwtWindow::SETCOLOR, unhand(self)->pData, fg, color);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MCanvasPeer_scroll(struct Hsun_awt_win32_MCanvasPeer *self,
				 long dx, long dy)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtWindow *pWindow = (AwtWindow *)unhand(self)->pData;

	AWT_TRACE(("%x:canvasScroll(%d, %d)\n", self, dx, dy));
	pWindow->PostMsg(AwtWindow::SCROLLWND, dx, -dy);

    AWT_UNLOCK();
	CHECKHEAP;
}

/** CHECKBOX methods **/

void
sun_awt_win32_MCheckboxPeer_create(struct Hsun_awt_win32_MCheckboxPeer *self,
				   struct Hsun_awt_win32_MComponentPeer *parent)
{
 	CHECKHEAP;
    AWT_LOCK();
    Hjava_awt_Checkbox *selfT = (Hjava_awt_Checkbox *)unhand(self)->target;
    if (parent == 0 || unhand(parent)->pData == 0 || selfT == NULL) {
		SignalError(0,JAVAPKG "NullPointerException", "null parent");
		AWT_UNLOCK();
		return;
    }
    Classjava_awt_Checkbox *selfTC = (Classjava_awt_Checkbox *)unhand(selfT);
	AwtWindow *pParent = (AwtWindow*)unhand(parent)->pData;
	AwtButton *pButton;
	char stackBuffer[64];
	char *buffer = &stackBuffer[0];
	int len = javaStringLength(selfTC->label);

	if (len >= sizeof(stackBuffer)) {
		buffer = (char *)malloc(len + 1);
	}
	javaString2CString(selfTC->label, buffer, len + 1);
	AWT_TRACE(("%x:checkboxCreate(%s)->", self, buffer));
	if (selfTC->group != NULL) {
		pButton = AwtButton::CreateRadio(selfT, pParent, buffer);
	} else {
		pButton = AwtButton::CreateToggle(selfT, pParent, buffer);
	}
	if (len >= sizeof(stackBuffer)) {
		free(buffer);
	}
	unhand(self)->pData = (long)pButton;

	CHECKHEAP;
    AWT_UNLOCK();
}

void 
sun_awt_win32_MCheckboxPeer_setColor(struct Hsun_awt_win32_MCheckboxPeer *self,
					   				   long fg,
					   				   struct Hjava_awt_Color *c)
{
	CHECKHEAP;
    AWT_LOCK();
    if (c == NULL) {
		SignalError(0,JAVAPKG "NullPointerException", "null color");
		AWT_UNLOCK();
		return;
    }
	Classjava_awt_Color *cC = (Classjava_awt_Color *)c->obj;
	COLORREF color = PALETTERGB((cC->value>>16)&0xff, (cC->value>>8)&0xff, (cC->value)&0xff);
	AwtButton *pButton = (AwtButton*)unhand(self);

	AWT_TRACE(("%x:checkboxSetColor(%d, %x)\n", self, fg, color));
	pButton->PostMsg(AwtButton::SETCOLOR, unhand(self)->pData, fg, color);

    AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MCheckboxPeer_setCheckboxGroup(struct Hsun_awt_win32_MCheckboxPeer *self,
					     struct Hjava_awt_CheckboxGroup *group)
{
}


void 
sun_awt_win32_MCheckboxPeer_setState(struct Hsun_awt_win32_MCheckboxPeer *self,
				     long state)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtButton *pButton = (AwtButton*)unhand(self)->pData;

	AWT_TRACE(("%x:checkboxSetState(%d)", self, state));
	pButton->PostMsg(AwtButton::SETSTATE, state);

    AWT_UNLOCK();
	CHECKHEAP;
}

long
sun_awt_win32_MCheckboxPeer_getState(struct Hsun_awt_win32_MCheckboxPeer *self)
{
	CHECKHEAP;
    AWT_LOCK();
	long result;
	AwtButton *pButton = (AwtButton*)unhand(self)->pData;

	AWT_TRACE(("%x:checkboxGetState()->", self));
	result = pButton->SendMsg(AwtButton::GETSTATE);
	AWT_TRACE(("%d\n", result));

    AWT_UNLOCK();
	CHECKHEAP;
    return result;
}

void
sun_awt_win32_MCheckboxPeer_setLabel(struct Hsun_awt_win32_MCheckboxPeer *self,
				     struct Hjava_lang_String *label)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtButton *pButton = (AwtButton*)unhand(self)->pData;
	int len = javaStringLength(label);
	char *buffer;

	buffer = (char *)malloc(len + 1);
	javaString2CString(label, buffer, len + 1);
	AWT_TRACE(("%x:checkboxSetLabel(%s)", self, buffer));
	pButton->PostMsg(AwtButton::SETTEXT, (long)buffer);

    AWT_UNLOCK();
	CHECKHEAP;
}

/** CHOICE methods */

void
sun_awt_win32_MChoicePeer_create(struct Hsun_awt_win32_MChoicePeer *self,
				 struct Hsun_awt_win32_MComponentPeer *parent)
{
	CHECKHEAP;
    AWT_LOCK();
    if (parent == 0 || unhand(parent)->pData == NULL) {
		SignalError(0, JAVAPKG "NullPointerException", "null parent");
		AWT_UNLOCK();
		return;
    }
 	AwtWindow *pParent = (AwtWindow *)unhand(parent)->pData;
	Hjava_awt_Choice *selfT = (Hjava_awt_Choice*)unhand(self)->target;
	
	AWT_TRACE(("%x:choiceCreate(%x)->", self, parent));
	AwtOptionMenu *pChoice = AwtOptionMenu::Create(selfT, pParent);
	unhand(self)->pData = (long)pChoice;

    AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MChoicePeer_setColor(struct Hsun_awt_win32_MChoicePeer *self,
					   				   long fg,
					   				   struct Hjava_awt_Color *c)
{
	CHECKHEAP;
    AWT_LOCK();
    if (c == NULL) {
		SignalError(0,JAVAPKG "NullPointerException", "null color");
		AWT_UNLOCK();
		return;
    }
	Classjava_awt_Color *cC = (Classjava_awt_Color *)c->obj;
	COLORREF color = PALETTERGB((cC->value>>16)&0xff, (cC->value>>8)&0xff, (cC->value)&0xff);
	AwtOptionMenu *pChoice = (AwtOptionMenu *)unhand(self)->pData;

	AWT_TRACE(("%x:choiceSetColor(%d, %x)\n", self, fg, color));
	pChoice->PostMsg(AwtOptionMenu::SETCOLOR, unhand(self)->pData, fg, color);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MChoicePeer_addItem(struct Hsun_awt_win32_MChoicePeer *self,
				  struct Hjava_lang_String *item,
				  long index)
{
	CHECKHEAP;
    AWT_LOCK();

    if (item == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null item");
		AWT_UNLOCK();
		return;
    }
	AwtOptionMenu *pChoice = (AwtOptionMenu *)unhand(self)->pData;
	int len = javaStringLength(item);
 	char *buffer;

	buffer = (char *)malloc(len + 1);
	javaString2CString(item, buffer, len + 1);
	AWT_TRACE(("%x:choiceAddItem(%s, %d)\n", self, buffer, index));
	pChoice->PostMsg(AwtOptionMenu::ADDITEM, (long)buffer, index);

    AWT_UNLOCK();
	CHECKHEAP;
}


void
sun_awt_win32_MChoicePeer_select(struct Hsun_awt_win32_MChoicePeer *self,
				 long index)
{
    CHECKHEAP;
    AWT_LOCK();

    AwtOptionMenu *pChoice = (AwtOptionMenu *)unhand(self)->pData;
    if (pChoice == NULL) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    // range checking is done by target.

    AWT_TRACE(("%x:choiceSelect(%d)\n", self, index));
    pChoice->PostMsg(AwtOptionMenu::SELECT, index);

    AWT_UNLOCK();
    CHECKHEAP;
}

void 
sun_awt_win32_MChoicePeer_remove(struct Hsun_awt_win32_MChoicePeer *self,
				 long index)
{
    CHECKHEAP;
    AWT_LOCK();

    AwtOptionMenu *pChoice = (AwtOptionMenu *)unhand(self)->pData;
    if (pChoice == NULL) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    AWT_TRACE(("%x:choiceRemove(%d)\n", self, index));
    pChoice->PostMsg(AwtOptionMenu::REMOVE, index);

    AWT_UNLOCK();
    CHECKHEAP;
}

/** COMPONENT methods **/

/*
 * Looking at the state variables in target, initialize the component properly.
 */
void
sun_awt_win32_MComponentPeer_pInitialize(struct Hsun_awt_win32_MComponentPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
    Classsun_awt_win32_MComponentPeer *selfC = unhand(self);
    if (selfC->target == 0) {
        SignalError(0, JAVAPKG "NullPointerException", "null target");
        AWT_UNLOCK();
        return;
    }
    Classjava_awt_Component *selfTC = unhand(selfC->target);

    AWT_TRACE(("%x:componentInitialize()\n", self));
    AwtComp::PostMsg(AwtComp::INITIALIZE, selfC->pData, selfTC->enabled, 
                     (long)selfC->target);

    AWT_UNLOCK();
    CHECKHEAP;
}

struct Hjava_awt_Event *
sun_awt_win32_MComponentPeer_setData(struct Hsun_awt_win32_MComponentPeer *self,
				     long data,
				     struct Hjava_awt_Event *event)
{
    Classjava_awt_Event	*eventPtr = unhand(event);
    eventPtr->data = data;
    return event;
}

long 
sun_awt_win32_MComponentPeer_handleEvent(struct Hsun_awt_win32_MComponentPeer *self,
					 struct Hjava_awt_Event *event)
{
#ifdef NEW_EVENT_MODEL
    AWT_LOCK();
    Classjava_awt_Event *eventPtr = unhand(event);
    if (eventPtr->data != 0) {
	DispatchInputEvent(eventPtr);
	AWT_UNLOCK();
	return 1;
    } else {
	AWT_UNLOCK();
	return 0;
    }
#else
    return 0;
#endif				/* NEW_EVENT_MODEL */
}

void 
sun_awt_win32_MComponentPeer_pEnable(struct Hsun_awt_win32_MComponentPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();

    AWT_TRACE(("%x:componentEnable()\n", self));
    AwtComp::PostMsg(AwtComp::ENABLE, unhand(self)->pData, TRUE);

    AWT_UNLOCK();
    CHECKHEAP;
}

void 
sun_awt_win32_MComponentPeer_pDisable(struct Hsun_awt_win32_MComponentPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();

    AWT_TRACE(("%x:componentDisable()\n", self));
    AwtComp::PostMsg(AwtComp::ENABLE, unhand(self)->pData, FALSE);

    AWT_UNLOCK();
    CHECKHEAP;
}

void 
sun_awt_win32_MComponentPeer_pShow(struct Hsun_awt_win32_MComponentPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();

    AWT_TRACE(("%x:componentShow()\n", self));
    AwtComp::PostMsg(AwtComp::SHOW, unhand(self)->pData, TRUE);

    AWT_UNLOCK();
    CHECKHEAP;
}

void 
sun_awt_win32_MComponentPeer_pHide(struct Hsun_awt_win32_MComponentPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();

    AWT_TRACE(("%x:componentHide()\n", self));
    AwtComp::PostMsg(AwtComp::SHOW, unhand(self)->pData, FALSE);

    AWT_UNLOCK();
    CHECKHEAP;
}

void 
sun_awt_win32_MComponentPeer_pReshape(struct Hsun_awt_win32_MComponentPeer *self,
				      long x,
				      long y,
				      long w,
				      long h)
{
 	CHECKHEAP;
    AWT_LOCK();

	AWT_TRACE(("%x:componentReshape(%d %d %d %d)\n", self, x, y, w, h));
	AwtComp::PostMsg(AwtComp::RESHAPE, unhand(self)->pData, x, y, w, h);

    AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MComponentPeer_pRepaint(struct Hsun_awt_win32_MComponentPeer *self,
				      long x,
				      long y,
				      long w,
				      long h)
{
 	CHECKHEAP;
    AWT_LOCK();
    Hjava_awt_Component *selfT = (Hjava_awt_Component*)unhand(self)->target;

	AWT_TRACE(("%x:componentRepaint(%d %d %d %d)\n", self, x, y, w, h));
    if (unhand(self)->pData != NULL) {
	AwtComp::PostMsg(AwtComp::REPAINT, unhand(self)->pData, (long)selfT, x, y, w, h);
    }

    AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MComponentPeer_pDispose(struct Hsun_awt_win32_MComponentPeer *self)
{
 	CHECKHEAP;
    AWT_LOCK();

	AWT_TRACE(("%x:componentDispose()\n", self));
 	if (unhand(self)->pData != NULL) {
		AwtComp::PostMsg(AwtComp::DISPOSE, unhand(self)->pData);
		unhand(self)->pData = NULL;
	}
    AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MComponentPeer_setFont(struct Hsun_awt_win32_MComponentPeer *self,
				     struct Hjava_awt_Font *f)
{
	CHECKHEAP;
    AWT_LOCK();
    if (f == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null font");
	    AWT_UNLOCK();
		return;
    }
	Classjava_awt_Font *fC = (Classjava_awt_Font*)unhand(f);
	AwtFont *pFont = (AwtFont*)fC->pData;
	char stackBuffer[64];
	char *buffer = &stackBuffer[0];
	int len = -1;
	
	if (pFont == NULL) {
		len = javaStringLength(fC->name);
		if (len >= sizeof(stackBuffer)) {
			buffer = (char *)malloc(len + 1);
		}
		javaString2CString(fC->name, buffer, len + 1);
		AWT_TRACE(("<%x:creatingFont(%s, %d, %d)>\n", f, buffer, fC->style, fC->size));
		pFont = AwtFont::Create(buffer, fC->style, fC->size);
	}
	AWT_TRACE(("%x:componentSetFont(%x)\n", self, f));
	AwtComp::PostMsg(AwtComp::SETFONT, unhand(self)->pData, (long)pFont);
	if (fC->pData != NULL && len >= 0) {
		free(buffer);
	}
	fC->pData = (long)pFont;

    AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MComponentPeer_setForeground(struct Hsun_awt_win32_MComponentPeer *self,
					   struct Hjava_awt_Color *c)
{
    CHECKHEAP;
    AWT_LOCK();
    if (c == NULL) {
        SignalError(0,JAVAPKG "NullPointerException", "null color");
        AWT_UNLOCK();
        return;
    }

    Classjava_awt_Color *cC = (Classjava_awt_Color *)c->obj;
    COLORREF color = PALETTERGB((cC->value>>16)&0xff, 
				(cC->value>>8)&0xff, 
				(cC->value)&0xff);
	
    AWT_TRACE(("%x:componentSetForeground(%x)\n", self, color));
    AwtComp::PostMsg(AwtComp::SETCOLOR, unhand(self)->pData, TRUE, color);

    AWT_UNLOCK();
    CHECKHEAP;
}

void 
sun_awt_win32_MComponentPeer_setBackground(struct Hsun_awt_win32_MComponentPeer *self,
					   struct Hjava_awt_Color *c)
{
    CHECKHEAP;
    AWT_LOCK();
    if (c == NULL) {
        SignalError(0,JAVAPKG "NullPointerException", "null color");
        AWT_UNLOCK();
        return;
    }

    Classjava_awt_Color *cC = (Classjava_awt_Color *)c->obj;
    COLORREF color = PALETTERGB((cC->value>>16)&0xff, 
				(cC->value>>8)&0xff, 
				(cC->value)&0xff);
	
    AWT_TRACE(("%x:componentSetBackground(%x)\n", self, color));
    AwtComp::PostMsg(AwtComp::SETCOLOR, unhand(self)->pData, FALSE, color);

    AWT_UNLOCK();
    CHECKHEAP;
}

void 
sun_awt_win32_MComponentPeer_requestFocus(struct Hsun_awt_win32_MComponentPeer *self)
{
 	CHECKHEAP;
    AWT_LOCK();

	AWT_TRACE(("%x:componentRequestFocus()\n", self));
	AwtComp::PostMsg(AwtComp::SETFOCUS, unhand(self)->pData);

    AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MComponentPeer_nextFocus(struct Hsun_awt_win32_MComponentPeer *self)
{
 	CHECKHEAP;
    AWT_LOCK();

	AWT_TRACE(("%x:componentNextFocus()\n", self));
	AwtComp::PostMsg(AwtComp::NEXTFOCUS, unhand(self)->pData);

    AWT_UNLOCK();
	CHECKHEAP;
}

/**  FILEDIALOG methods **/

void
sun_awt_win32_MFileDialogPeer_pCreate(struct Hsun_awt_win32_MFileDialogPeer *self,
				     struct Hsun_awt_win32_MComponentPeer *parent,
					 long mode,
					 struct Hjava_lang_String *title,
				     struct Hjava_lang_String *file)
{
	CHECKHEAP;
	AWT_LOCK();
	if (parent == NULL || unhand(self)->target == NULL) {
		SignalError(0, JAVAPKG "NullPointerException", "null parent");
		AWT_UNLOCK();
		return;
	}
	MainFrame *pParent = (MainFrame *)unhand(parent)->pData;
	Classsun_awt_win32_MFileDialogPeer *selfC = unhand(self);
	Hjava_awt_FileDialog *selfT = (Hjava_awt_FileDialog*)selfC->target;

	//Classjava_awt_FileDialog *selfTC = unhand(selfT);
	char *titleBuffer = NULL, *fileBuffer = NULL;
	BOOL bOpen;
	
	if (file != NULL) {
		int len = javaStringLength(file);
		fileBuffer = (char *)malloc(len + 1);
		javaString2CString(file, fileBuffer, len + 1);
	} else {
		fileBuffer = strdup("*.*");
	}

	// <<<Need to do something about title>>
	bOpen = mode == java_awt_FileDialog_LOAD;
	if (title == NULL) {
		titleBuffer = bOpen ? strdup("Open") : strdup("Save");
	} else {
		int len = javaStringLength(title);
		titleBuffer = (char *)malloc(len + 1);
		javaString2CString(title, titleBuffer, len + 1);
	}																  	
	AWT_TRACE(("%x:fileDialogCreate(%x, open=%d, title=%s, file=%s)\n", self, 
		parent, bOpen, titleBuffer, fileBuffer));
	AwtFileDialog *pFileDialog = AwtFileDialog::Create(selfT, pParent, bOpen, 
		titleBuffer, fileBuffer);
	unhand(self)->pData = (long)pFileDialog;
        free(fileBuffer);

	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MFileDialogPeer_setDirectory(struct Hsun_awt_win32_MFileDialogPeer *self,
					   struct Hjava_lang_String *dir)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtFileDialog *pFileDialog = (AwtFileDialog*)unhand(self)->pData;
	int len = javaStringLength(dir);
 	char *buffer;

	buffer = (char *)malloc(len + 1);
	javaString2CString(dir, buffer, len + 1);
	AWT_TRACE(("%x:fileDialogSetDirectory(%s)\n", self, buffer));
	pFileDialog->PostMsg(AwtFileDialog::SETDIRECTORY, (long)buffer);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MFileDialogPeer_setFile(struct Hsun_awt_win32_MFileDialogPeer *self,
                                      struct Hjava_lang_String *file)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtFileDialog *pFileDialog = (AwtFileDialog*)unhand(self)->pData;
	int len = javaStringLength(file);
 	char *buffer;

	buffer = (char *)malloc(len + 1);
	javaString2CString(file, buffer, len + 1);
	AWT_TRACE(("%x:fileDialogSetFile(%s)\n", self, buffer));
	pFileDialog->PostMsg(AwtFileDialog::SETFILE, (long)buffer);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MFileDialogPeer_pShow(struct Hsun_awt_win32_MFileDialogPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtFileDialog *pFileDialog = (AwtFileDialog *)unhand(self)->pData;

    AWT_TRACE(("%x:fileDialogShow\n", self));
    pFileDialog->SendMsg(AwtFileDialog::SHOW, TRUE);

    AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_MFileDialogPeer_pHide(struct Hsun_awt_win32_MFileDialogPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtFileDialog *pFileDialog = (AwtFileDialog *)unhand(self)->pData;

    AWT_TRACE(("%x:fileDialogHide\n", self));
    pFileDialog->SendMsg(AwtFileDialog::SHOW, FALSE);

    AWT_UNLOCK();
    CHECKHEAP;
}

/** FONT methods **/


void
sun_awt_win32_Win32FontMetrics_init(struct Hsun_awt_win32_Win32FontMetrics *self)
{
	CHECKHEAP;
    AWT_LOCK();
    Classsun_awt_win32_Win32FontMetrics	*selfC = (Classsun_awt_win32_Win32FontMetrics *)unhand(self);
    if (selfC == NULL || selfC->font == NULL) {
		SignalError(0, JAVAPKG "NullPointerException", 0);
		AWT_UNLOCK();
		return;
    }

	AWT_TRACE(("%x:fontMetricsInit()\n", self));
 	AwtFontMetrics::LoadMetrics(self);

    AWT_UNLOCK();
	CHECKHEAP;
}

long
sun_awt_win32_Win32FontMetrics_stringWidth(struct Hsun_awt_win32_Win32FontMetrics *self,
					   struct Hjava_lang_String *str)
{
	CHECKHEAP;
	AWT_LOCK();
    HArrayOfChar *data;
    if (str == 0 || (data = unhand(str)->value) == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null string");
		AWT_UNLOCK();
		return 0;
    }
	long result;
	char stackBuffer[64];
	char *buffer = &stackBuffer[0];
	int len = javaStringLength(str);

	if (len >= sizeof(stackBuffer)) {
		buffer = (char *)malloc(len + 1);
	}
	javaString2CString(str, buffer, len + 1);
	AWT_TRACE(("%x:fontStringWidth(%s)->", self, buffer));	
	result = AwtFontMetrics::StringWidth(self, buffer);
	AWT_TRACE(("%d\n", result));
	if (len >= sizeof(stackBuffer)) {
		free(buffer);
	}

	AWT_UNLOCK();
	CHECKHEAP;
    return result;
}

long
sun_awt_win32_Win32FontMetrics_charsWidth(struct Hsun_awt_win32_Win32FontMetrics *self,
					  HArrayOfChar *str, long off, long len)
{
	CHECKHEAP;
	AWT_LOCK();
    Classsun_awt_win32_Win32FontMetrics	*selfC = (Classsun_awt_win32_Win32FontMetrics *)unhand(self);
    if (str == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null chars");
		AWT_UNLOCK();
		return 0;
    }
    if ((len < 0) || (off < 0) || (len + off > (long)obj_length(str))) {
		SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
		AWT_UNLOCK();
		return 0;
    }
	WCHAR *pStr;
	long *widths = unhand(selfC->widths)->body;
	long result = 0;
	AWT_TRACE(("%x:fontCharsWidth(%s, %d, %d)->", self, "<some chars>", off, len));
	for (pStr = unhand(str)->body + off; len; len--) {
		if (*pStr < 256) {
			result += widths[*pStr++];
		}
	}
	AWT_TRACE(("%d\n", result));

	AWT_UNLOCK();
	CHECKHEAP;
    return result;
}

long
sun_awt_win32_Win32FontMetrics_bytesWidth(struct Hsun_awt_win32_Win32FontMetrics *self,
					  HArrayOfByte *str, long off, long len)
{
	CHECKHEAP;
	AWT_LOCK();

    Classsun_awt_win32_Win32FontMetrics	*selfC = (Classsun_awt_win32_Win32FontMetrics *)unhand(self);
    if (str == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null bytes");
		AWT_UNLOCK();
		return 0;
    }
	if ((len < 0) || (off < 0) || (len + off > (long)obj_length(str))) {
		SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
		AWT_UNLOCK();
		return 0;
    }
	char *pStr= unhand(str)->body + off;
	long *widths = unhand(unhand(self)->widths)->body;
	long result = 0;

	AWT_TRACE(("%x:fontBytesWidth(%s, %d, %d)->", self, pStr, off, len));
	for (; len; len--) {
		result += widths[*pStr++];
	}
	AWT_TRACE(("%d\n", result));

	AWT_UNLOCK();
	CHECKHEAP;
    return result;
}

/** FRAME methods */


void
sun_awt_win32_MFramePeer_create(struct Hsun_awt_win32_MFramePeer *self,
				struct Hsun_awt_win32_MComponentPeer *parent)
{
	CHECKHEAP;
    AWT_LOCK();

    if (unhand(self)->target == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null target");
                AWT_UNLOCK();
		return;
    }
    MainFrame *pParent = NULL;
	Classsun_awt_win32_MFramePeer *selfC = unhand(self);
	Hjava_awt_Frame *selfT = (Hjava_awt_Frame*)selfC->target;

    if (parent != NULL) {
		pParent = (MainFrame*)unhand(parent)->pData;
	}
	AWT_TRACE(("%x:frameCreate(%x)\n", self, parent));
	MainFrame *pFrame = MainFrame::Create(selfT, TRUE, pParent,
					      unhand(selfT)->width,
					      unhand(selfT)->height,
					      RGB(192, 192, 192));
    SET_PDATA(self, pFrame->GetMainWindow());

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MFramePeer_setInsets(struct Hsun_awt_win32_MFramePeer *self,
				   struct Hjava_awt_Insets *insets)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
    MainFrame *pFrame = pWindow->GetFrame();

    pFrame->SetInsets(insets);
    AWT_UNLOCK();
}


void
sun_awt_win32_MFramePeer_pSetIconImage(struct Hsun_awt_win32_MFramePeer *self,
				       struct Hsun_awt_image_ImageRepresentation *ir)
{
	CHECKHEAP;
    AWT_LOCK();
    if (ir == 0) {
		SignalError(0, JAVAPKG "NullPointerException", 0);
		AWT_UNLOCK();
		return;
    }
	AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
	MainFrame *pFrame = pWindow->GetFrame();

	AWT_TRACE(("%x:frameSetIcon(%x)\n", self, ir));
	pFrame->PostMsg(MainFrame::SETICON, (long)ir);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MFramePeer_setResizable(struct Hsun_awt_win32_MFramePeer *self,
				       long resizable)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
	MainFrame *pFrame = pWindow->GetFrame();

	AWT_TRACE(("%x:frameSetResizable(%d)\n", self, resizable));
	pFrame->PostMsg(MainFrame::SETRESIZABLE, resizable);

    AWT_UNLOCK();
	CHECKHEAP;
}

// This structure needs to be a CObject subclass, so it can be freed
// (if necessary) during cleanup.
class MenuBarData : public CObject {
public:
    DECLARE_DYNAMIC(MenuBarData)
    HMENU hMenuBar;
    MainFrame *pFrame;
};
IMPLEMENT_DYNAMIC(MenuBarData, CObject)

void
sun_awt_win32_MFramePeer_setMenuBar(struct Hsun_awt_win32_MFramePeer *self,
				    struct Hjava_awt_MenuBar *mb)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
    MainFrame *pFrame = pWindow->GetFrame();
    if (mb == 0) {
        AWT_UNLOCK();
        return;
    }
    if (unhand(self)->target == 0 || pFrame == 0) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        AWT_UNLOCK();
        return;
    }
    MenuBarData *pMenuBar = (MenuBarData*)
        unhand((Hsun_awt_win32_MMenuPeer*)unhand(mb)->peer)->pData;

    AWT_TRACE(("%x:frameSetMenuBar(%x)\n", self, mb));
    pFrame->PostMsg(MainFrame::DRAWMENUBAR, (long)pMenuBar->hMenuBar);

    AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_MFramePeer_pSetTitle(struct Hsun_awt_win32_MFramePeer *self,
				   struct Hjava_lang_String *title)
{
    CHECKHEAP;
    AWT_LOCK();
	AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
	MainFrame *pFrame = pWindow->GetFrame();
	int len = javaStringLength(title);
 	char *buffer = NULL;

	if (len > 0) {
		buffer = (char *)malloc(len + 1);
		javaString2CString(title, buffer, len + 1);
	}
	AWT_TRACE(("%x:frameSetTitle(%s)\n", self, buffer));
	pFrame->PostMsg(MainFrame::SETTEXT, (long)buffer);

    AWT_UNLOCK();
    CHECKHEAP;
}

void 
sun_awt_win32_MFramePeer_pShow(struct Hsun_awt_win32_MFramePeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
    MainFrame *pFrame = pWindow->GetFrame();

    AWT_TRACE(("%x:frameShow()\n", self));
    AwtComp::PostMsg(AwtComp::SHOW, (long)pFrame, TRUE);

    AWT_UNLOCK();
    CHECKHEAP;
}

void 
sun_awt_win32_MFramePeer_pHide(struct Hsun_awt_win32_MFramePeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
    MainFrame *pFrame = pWindow->GetFrame();

    AWT_TRACE(("%x:frameHide()\n", self));
    AwtComp::PostMsg(AwtComp::SHOW, (long)pFrame, FALSE);

    AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_MFramePeer_pReshape(struct Hsun_awt_win32_MFramePeer *self,
				  long x, long y,
				  long w, long h)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
	MainFrame *pFrame = pWindow->GetFrame();

	AWT_TRACE(("%x:frameReshape(%d %d %d %d)\n", self, x, y, w, h));
	pFrame->PostMsg(MainFrame::RESHAPE, x, y, w, h);

    AWT_UNLOCK();
	CHECKHEAP;
}


void
sun_awt_win32_MFramePeer_setCursor(struct Hsun_awt_win32_MFramePeer *self,
                                   long cursorType)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
    MainFrame *pFrame = pWindow->GetFrame();

    AWT_TRACE(("%x:frameSetCursor(%d)\n", self, cursorType));
    pFrame->PostMsg(MainFrame::SETCURSOR, cursorType);

    AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_MFramePeer_pDispose(struct Hsun_awt_win32_MFramePeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
	AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
	MainFrame *pFrame = pWindow->GetFrame();

	AWT_TRACE(("%x:frameDispose()\n", self));
	pFrame->PostMsg(MainFrame::DISPOSE);

    AWT_UNLOCK();
	CHECKHEAP;
}


/** GRAPHICS methods */


void
sun_awt_win32_Win32Graphics_create(struct Hsun_awt_win32_Win32Graphics *self,
				   struct Hsun_awt_win32_MComponentPeer *canvas)
{
    CHECKHEAP;
    AWT_LOCK();
    if (canvas == 0 || unhand(canvas)->pData == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null canvas");
		AWT_UNLOCK();
		return;
    }
 	AwtWindow *pWindow = (AwtWindow*)unhand(canvas)->pData;

	AWT_TRACE(("%x:graphicsCreate(%x)\n", self, canvas));
	AwtGraphics *pGraphics = AwtGraphics::Create(self, pWindow);
	unhand(self)->pData = (long)pGraphics;

	AWT_UNLOCK();
	CHECKHEAP;
}

void sun_awt_win32_Win32Graphics_createFromComponent(struct Hsun_awt_win32_Win32Graphics *self,
													 struct Hsun_awt_win32_MComponentPeer *canvas)
{
    CHECKHEAP;
    AWT_LOCK();
    if (canvas == NULL || unhand(canvas)->pData == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null canvas");
		AWT_UNLOCK();
		return;
    }
 	AwtWindow *pWindow = (AwtWindow*)unhand(canvas)->pData;

	AWT_TRACE(("%x:graphicsCreate(%x)\n", self, canvas));
	AwtGraphics *pGraphics = AwtGraphics::Create(self, pWindow);
	unhand(self)->pData = (long)pGraphics;

	AWT_UNLOCK();
	CHECKHEAP;
}

void sun_awt_win32_Win32Graphics_createFromGraphics(struct Hsun_awt_win32_Win32Graphics *self,
													struct Hsun_awt_win32_Win32Graphics *g)
{
    CHECKHEAP;
    AWT_LOCK();
	if (g == NULL || unhand(g)->pData == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null graphics");
		AWT_UNLOCK();
		return;
        }
	AwtGraphics *pG = (AwtGraphics*)unhand(g)->pData;

	AWT_TRACE(("%x:graphicsCreateFromGraphics(%x)\n", self, pG));
	AwtGraphics *pGraphics = AwtGraphics::CreateCopy(self, pG);
	unhand(self)->pData = (long)pGraphics;

	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_imageCreate(struct Hsun_awt_win32_Win32Graphics *self,
					struct Hsun_awt_image_ImageRepresentation *ir)
{
 	CHECKHEAP;
    AWT_LOCK();

   if (self==0 || ir==0) {
		SignalError(0, JAVAPKG "NullPointerException", "null image");
   		AWT_UNLOCK();
		return;
    }
    AwtImage *pImage = image_getpImage(ir);

	AWT_TRACE(("%x:graphicsImageCreate(%x)->", self, ir));
	AwtGraphics *pGraphics = AwtGraphics::Create(self, pImage);
	AWT_TRACE(("%x\n", pGraphics));

	unhand(self)->pData = (long)pGraphics;
    AWT_UNLOCK();
	CHECKHEAP;
}


void
sun_awt_win32_Win32Graphics_pSetFont(struct Hsun_awt_win32_Win32Graphics *self,
				     struct Hjava_awt_Font *font)
{
	CHECKHEAP;
    AWT_LOCK();

    if (font == 0) {
		SignalError(0, JAVAPKG "NullPointerException", 0);
		AWT_UNLOCK();
		return;
    }
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	AwtFont *pFont = AwtFont::GetFont(font);

	AWT_TRACE(("%x:graphicsSetFont(%x)\n", self, font));
	if (pGraphics != NULL) {
		pGraphics->SetFont(pFont);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_pSetForeground(struct Hsun_awt_win32_Win32Graphics *self,
					   struct Hjava_awt_Color *c)
{
	CHECKHEAP;
    AWT_LOCK();
    if (c == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null color");
	    AWT_UNLOCK();
		return;
    }
 	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	Classjava_awt_Color *pColor = (Classjava_awt_Color *)c->obj;
	COLORREF color = PALETTERGB((pColor->value>>16)&0xff, (pColor->value>>8)&0xff, (pColor->value)&0xff);

	AWT_TRACE(("%x:graphicsSetForeground(%x)\n", self, color));
	if (pGraphics != NULL) {
		pGraphics->SetColor(color);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_dispose(struct Hsun_awt_win32_Win32Graphics *self)
{
    CHECKHEAP;
    AWT_LOCK();

 	AwtGraphics *pGraphics = (AwtGraphics*)unhand(self)->pData;

	AWT_TRACE(("%x:graphicsDispose()\n", self));
	if (pGraphics != NULL) {
		pGraphics->Dispose();
		unhand(self)->pData = NULL;
	}

	AWT_UNLOCK();
	CHECKHEAP;
}


void
sun_awt_win32_Win32Graphics_drawString(struct Hsun_awt_win32_Win32Graphics *self,
				       Hjava_lang_String *text,
				       long x,
				       long y)
{
    CHECKHEAP;
    AWT_LOCK();

    if (text==0) {
		SignalError(0, JAVAPKG "NullPointerException", "null string");
		AWT_UNLOCK();
		return;
    }
 	AwtGraphics *pGraphics = (AwtGraphics*)unhand(self)->pData;
	char *buffer;
	int len = javaStringLength(text);

	buffer = (char *)malloc(len + 1);
	javaString2CString(text, buffer, len + 1);
	AWT_TRACE(("%x:graphicsDrawString(%s, %d, %d)\n", self, buffer, x, y));
	if (pGraphics != NULL) {
		pGraphics->DrawString(buffer, x, y);
	}

	AWT_UNLOCK();
	CHECKHEAP;
}

long
sun_awt_win32_Win32Graphics_drawStringWidth(struct Hsun_awt_win32_Win32Graphics *self,
					    Hjava_lang_String *text,
					    long x,
					    long y)
{
	CHECKHEAP;
	AWT_LOCK();

    long result = -1;
    if (text==0) {
		SignalError(0, JAVAPKG "NullPointerException", "null string");
		AWT_UNLOCK();
		return result;
    }
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	char *buffer;
	int len = javaStringLength(text);

	buffer = (char *)malloc(len + 1);
	javaString2CString(text, buffer, len + 1);
	AWT_TRACE(("%x:graphicsDrawStringWidth(%s, %d, %d)->", self, buffer, x, y));
	if (pGraphics != NULL) {
		result = pGraphics->DrawStringWidth(buffer, x, y);
	}
	AWT_TRACE(("%d\n", result));

	AWT_UNLOCK();
	CHECKHEAP;
	return result;
}

void
sun_awt_win32_Win32Graphics_drawChars(struct Hsun_awt_win32_Win32Graphics *self,
				      HArrayOfChar *text,
				      long offset,
				      long length,
				      long x,
				      long y)
{
	CHECKHEAP;
	AWT_LOCK();

    if (text == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null chars");
		AWT_UNLOCK();
		return;
    }
    if (offset < 0 || length < 0 || offset + length > (long)obj_length(text)) {
		SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
		AWT_UNLOCK();
		return;
    }
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	WCHAR *pSrc= unhand(text)->body + offset;
	char *buffer;
	int i;
	
	buffer = (char *)malloc(length + 1);
	for (i=0; i<length; i++) {
		buffer[i] = (char)pSrc[i];
	}
	buffer[length] = 0;
	AWT_TRACE(("%x:graphicsDrawChars(%s, %d, %d, %d, %d)\n", self, buffer, offset, length, x, y));
	if (pGraphics != NULL) {
		pGraphics->DrawString(buffer, x, y);
	}

	AWT_UNLOCK();
	CHECKHEAP;
}

long
sun_awt_win32_Win32Graphics_drawCharsWidth(struct Hsun_awt_win32_Win32Graphics *self,
					   HArrayOfChar *text,
					   long offset,
					   long length,
					   long x,
					   long y)
{
	CHECKHEAP;
	AWT_LOCK();

	long result = -1;
    if (text == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null chars");
		AWT_UNLOCK();
		return result;
    }
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	WCHAR *pSrc= unhand(text)->body + offset;
	char *buffer;
	int i;
	
	buffer = (char *)malloc(length + 1);
	for (i=0; i<length; i++) {
		buffer[i] = (char)pSrc[i];
	}
	buffer[length] = 0;
	AWT_TRACE(("%x:graphicsDrawCharsWidth(%s, %d, %d, %d, %d)->", self, buffer, offset, length, x, y));
	if (pGraphics != NULL) {
		result = pGraphics->DrawStringWidth(buffer, x, y);
	}
	AWT_TRACE(("%d\n", result));
	
	AWT_UNLOCK();
	CHECKHEAP;
	return result;
}

void
sun_awt_win32_Win32Graphics_drawBytes(struct Hsun_awt_win32_Win32Graphics *self,
				      HArrayOfByte *text,
				      long offset,
				      long length,
				      long x,
				      long y)
{
	CHECKHEAP;
	AWT_LOCK();

    if (text == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null bytes");
		AWT_UNLOCK();
		return;
    }
    if (offset < 0 || length < 0 || offset + length > (long)obj_length(text)) {
		SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
		AWT_UNLOCK();
		return;
    }
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	char *pSrc= unhand(text)->body + offset;
	char *buffer;
	
	buffer = (char *)malloc(length + 1);
	memcpy(buffer, pSrc, length);
	buffer[length] = 0;
	AWT_TRACE(("%x:graphicsDrawBytes(%s, %d, %d, %d, %d)\n", self, buffer, offset, length, x, y));
	if (pGraphics != NULL) {
		pGraphics->DrawString(buffer, x, y);
	}
	
	AWT_UNLOCK();
	CHECKHEAP;
}

long
sun_awt_win32_Win32Graphics_drawBytesWidth(struct Hsun_awt_win32_Win32Graphics *self,
					   HArrayOfByte *text,
					   long offset,
					   long length,
					   long x,
					   long y)
{
	CHECKHEAP;
	AWT_LOCK();

	long result = -1;
    if (text == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null bytes");
		AWT_UNLOCK();
		return result;
    }
    if (offset < 0 || length < 0 || offset + length > (long)obj_length(text)) {
		SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
		AWT_UNLOCK();
		return result;
    }
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	char *pSrc= unhand(text)->body + offset;
	char *buffer;
	
	buffer = (char *)malloc(length + 1);
	memcpy(buffer, pSrc, length);
	buffer[length] = 0;
	AWT_TRACE(("%x:graphicsDrawBytesWidth(%s, %d, %d, %d, %d)", self, buffer, offset, length, x, y));
	if (pGraphics != NULL) {
		result = pGraphics->DrawStringWidth(buffer, x, y);
	}
	AWT_TRACE(("%d\n", result));
	
	AWT_UNLOCK();
	CHECKHEAP;
	return result;
}

void
sun_awt_win32_Win32Graphics_drawLine(struct Hsun_awt_win32_Win32Graphics *self,
				     long x1, long y1,
				     long x2, long y2)
{
	CHECKHEAP;
    AWT_LOCK();

	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	AWT_TRACE(("%x:graphicsDrawLine(%d, %d, %d, %d)\n", self, x1, y1, x2, y2));
	if (pGraphics != NULL) {
		pGraphics->DrawLine(x1, y1, x2, y2);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_clearRect(struct Hsun_awt_win32_Win32Graphics *self,
				      long x,
				      long y,
				      long w,
				      long h)
{
	CHECKHEAP;
    AWT_LOCK();

	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	
	AWT_TRACE(("%x:graphicsClearRect(%d, %d, %d, %d)\n", 
		self, x, y, w, h));
	if (pGraphics != NULL) {
		pGraphics->ClearRect(x, y, w, h);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_fillRect(struct Hsun_awt_win32_Win32Graphics *self,
				     long x,
				     long y,
				     long w,
				     long h)
{
	CHECKHEAP;
    AWT_LOCK();

	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	
	AWT_TRACE(("%x:graphicsFillRect(%d, %d, %d, %d)\n", 
		self, x, y, w, h));
	if (pGraphics != NULL) {
		pGraphics->FillRect(x, y, w, h);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_drawRect(struct Hsun_awt_win32_Win32Graphics *self,
				     long x,
				     long y,
				     long w,
				     long h)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	
	AWT_TRACE(("%x:graphicsDrawRect(%d, %d, %d, %d)\n", 
		self, x, y, w, h));
	if (pGraphics != NULL) {
		pGraphics->DrawRect(x, y, w, h);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_drawRoundRect(struct Hsun_awt_win32_Win32Graphics *self,
					  long x, long y, long w, long h,
					  long arcWidth, long arcHeight)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	
	AWT_TRACE(("%x:graphicsDrawRoundRect(%d, %d, %d, %d, %d, %d)\n", 
		self, x, y, w, h, arcWidth, arcHeight));
	if (pGraphics != NULL) {
		pGraphics->RoundRect(x, y, w, h, arcWidth, arcHeight);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_fillRoundRect(struct Hsun_awt_win32_Win32Graphics *self,
					  long x, long y, long w, long h,
					  long arcWidth, long arcHeight)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	
	AWT_TRACE(("%x:graphicsFillRoundRect(%d, %d, %d, %d, %d, %d)\n", 
		self, x, y, w, h, arcWidth, arcHeight));
	if (pGraphics != NULL) {
		pGraphics->FillRoundRect(x, y, w, h, arcWidth, arcHeight);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}


void
sun_awt_win32_Win32Graphics_setPaintMode(struct Hsun_awt_win32_Win32Graphics *self)
{
 	CHECKHEAP;
    AWT_LOCK();
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	struct Hjava_awt_Color *fg = unhand(self)->foreground;
	if (fg == 0) {
	    SignalError(0, JAVAPKG "NullPointerException", "null color");
	    AWT_UNLOCK();
	    return;
	}
	Classjava_awt_Color *pColor = (Classjava_awt_Color *)fg->obj;
	COLORREF fgcolor = PALETTERGB((pColor->value>>16)&0xff,
				      (pColor->value>>8)&0xff,
				      (pColor->value)&0xff);

	AWT_TRACE(("%x:graphicsSetPaintMode()\n"));
	if (pGraphics != NULL) {
	    pGraphics->SetRop(R2_COPYPEN, fgcolor, RGB(0, 0, 0));
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_setXORMode(struct Hsun_awt_win32_Win32Graphics *self,
				       struct Hjava_awt_Color *c)
{
 	CHECKHEAP;
    AWT_LOCK();
	struct Hjava_awt_Color *fg = unhand(self)->foreground;
	if (c == 0 || fg == 0) {
	    SignalError(0, JAVAPKG "NullPointerException", "null color");
	    AWT_UNLOCK();
	    return;
	}
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	Classjava_awt_Color *pColor = (Classjava_awt_Color *)c->obj;
	COLORREF color = PALETTERGB((pColor->value>>16)&0xff,
				    (pColor->value>>8)&0xff,
				    (pColor->value)&0xff);
	pColor = (Classjava_awt_Color *)fg->obj;
	COLORREF fgcolor = PALETTERGB((pColor->value>>16)&0xff,
				      (pColor->value>>8)&0xff,
				      (pColor->value)&0xff);

	AWT_TRACE(("%x:graphicsSetXORMode()\n"));
	if (pGraphics != NULL) {
	    pGraphics->SetRop(R2_XORPEN, fgcolor, color);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}


void
sun_awt_win32_Win32Graphics_clipRect(struct Hsun_awt_win32_Win32Graphics *self,
				     long x, long y,
				     long w, long h)
{
 	CHECKHEAP;
    AWT_LOCK();

	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;

	AWT_TRACE(("%x:graphicsClipRect(%d, %d, %d, %d)\n", 
		self, x, y, w, h));
	if (pGraphics != NULL) {
		pGraphics->SetClipRect(x, y, w, h);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_getClipRect(struct Hsun_awt_win32_Win32Graphics *self,
					struct Hjava_awt_Rectangle *clip)
{
 	CHECKHEAP;
    AWT_LOCK();

	RECT rect;
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
    if (clip == 0 || pGraphics == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null clip");
		AWT_UNLOCK();
		return;
    }
	AWT_TRACE(("%x:graphicsGetClipRect()->"));
	if (pGraphics != NULL) {
		pGraphics->GetClipRect(&rect);
		unhand(clip)->x = rect.left;
		unhand(clip)->y = rect.top;
		unhand(clip)->width = rect.right - rect.left;
		unhand(clip)->height = rect.bottom - rect.top;
		AWT_TRACE(("(%d %d %d %d)\n", rect.left, rect.top, rect.right, rect.bottom));
	} else {
		AWT_TRACE(("** graphics disposed **\n"));
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_clearClip(struct Hsun_awt_win32_Win32Graphics *self)
{
 	CHECKHEAP;
    AWT_LOCK();

	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;

	AWT_TRACE(("%x:graphicsClearClip()\n", self));
	if (pGraphics != NULL) {
		pGraphics->ClearClipRect();
	}

    AWT_UNLOCK();
	CHECKHEAP;
}


void
sun_awt_win32_Win32Graphics_copyArea(struct Hsun_awt_win32_Win32Graphics *self,
				     long x, long y,
				     long width, long height,
				     long dx, long dy)
{
	CHECKHEAP;
    AWT_LOCK();

	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;

	AWT_TRACE(("%x:graphicsCopyArea(%d, %d, %d, %d, %d, %d)\n",
		self, x, y, width, height, dx, dy));
	if (pGraphics != NULL) {
		pGraphics->CopyArea(dx+x, dy+y, x, y, width, height);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}


static POINT *
transformPoints(struct Hsun_awt_win32_Win32Graphics *self,
		HArrayOfInt *xpoints,
		HArrayOfInt *ypoints,
		long npoints)
{
    static POINT *points;
    static int		points_length = 0;
    ArrayOfInt		*xpoints_array = unhand(xpoints);
    ArrayOfInt		*ypoints_array = unhand(ypoints);
    int			i;

    if ((int)obj_length(ypoints) < npoints || (int)obj_length(xpoints) < npoints) {
		SignalError(0, JAVAPKG "IllegalArgumentException", 0);
		return 0;
    }
    if (points_length < npoints) {
		if (points_length != 0) {
		    free((void *)points);
		}
		points = (POINT*)malloc(sizeof(POINT) * npoints);
		if (points == 0) {
		    SignalError(0, JAVAPKG "OutOfMemoryError",0);
		    return 0;
		}
		points_length = npoints;
    }
	for (i=0; i<npoints; ++i) {
		points[i].x = (xpoints_array->body)[i];
		points[i].y = (ypoints_array->body)[i];
	}

    return points;
}

void
sun_awt_win32_Win32Graphics_drawPolygon(struct Hsun_awt_win32_Win32Graphics *self,
					HArrayOfInt *xpoints,
					HArrayOfInt *ypoints,
					long npoints)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
    if (pGraphics == NULL || xpoints == NULL || ypoints == NULL) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    POINT *points = transformPoints(self, xpoints, ypoints, npoints);

    if (points == 0) {
	AWT_UNLOCK();
	return;
    }
    AWT_TRACE(("%x:graphicsPolygon(n=%d)\n", self, npoints));
    pGraphics->Polygon(points, npoints);

    AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_fillPolygon(struct Hsun_awt_win32_Win32Graphics *self,
					HArrayOfInt *xpoints,
					HArrayOfInt *ypoints,
					long npoints)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
    if (pGraphics == NULL || xpoints == NULL || ypoints == NULL) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }

    POINT *points = transformPoints(self, xpoints, ypoints, npoints);

    if (points == 0) {
	AWT_UNLOCK();
	return;
    }
    AWT_TRACE(("%x:graphicsFillPolygon(n=%d)\n", self, npoints));
    pGraphics->FillPolygon(points, npoints);

    AWT_UNLOCK();
    CHECKHEAP;
}


void
sun_awt_win32_Win32Graphics_drawOval(struct Hsun_awt_win32_Win32Graphics *self,
				     long x, long y, long w, long h)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	
	AWT_TRACE(("%x:graphicsOval(%d, %d, %d, %d)\n", 
		self, x, y, w, h));
	if (pGraphics != NULL) {
		pGraphics->Ellipse(x, y, w, h);
	} else 
		DebugBreak();

    AWT_UNLOCK();
	CHECKHEAP;
}


void
sun_awt_win32_Win32Graphics_fillOval(struct Hsun_awt_win32_Win32Graphics *self,
				     long x, long y, long w, long h)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	
	AWT_TRACE(("%x:graphicsFillOval(%d, %d, %d, %d)\n", 
		self, x, y, w, h));
	if (pGraphics != NULL) {
		pGraphics->FillEllipse(x, y, w, h);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_drawArc(struct Hsun_awt_win32_Win32Graphics *self,
				    long x, long y, long w, long h,
				    long startAngle, long endAngle)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	
	AWT_TRACE(("%x:graphicsDrawArc(%d, %d, %d, %d %d %d)\n", 
		self, x, y, w, h, startAngle, endAngle));
	if (pGraphics != NULL) {
		pGraphics->Arc(x, y, w, h, startAngle, endAngle);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_Win32Graphics_fillArc(struct Hsun_awt_win32_Win32Graphics *self,
				    long x, long y, long w, long h,
				    long startAngle, long endAngle)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	
	AWT_TRACE(("%x:graphicsFillArc(%d, %d, %d, %d %d %d)\n", 
		self, x, y, w, h, startAngle, endAngle));
	if (pGraphics != NULL) {
		pGraphics->FillArc(x, y, w, h, startAngle, endAngle);
	}

    AWT_UNLOCK();
	CHECKHEAP;
}

/** LABEL methods */

void
sun_awt_win32_MLabelPeer_create(struct Hsun_awt_win32_MLabelPeer *self,
				struct Hsun_awt_win32_MComponentPeer *parent)
{
	CHECKHEAP;
    AWT_LOCK();
    if (parent == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null parent");
	    AWT_UNLOCK();
		return;
    }
	Hjava_awt_Label *selfT = (Hjava_awt_Label*)unhand(self)->target;
	AwtWindow *pParent = (AwtWindow*)unhand(parent)->pData;

	AWT_TRACE(("%x:labelCreate(%x)\n", self, parent));
	AwtLabel *pLabel = AwtLabel::Create(selfT, pParent, NULL);
	unhand(self)->pData = (long)pLabel;

    AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MLabelPeer_setText(struct Hsun_awt_win32_MLabelPeer *self,
				 Hjava_lang_String *label)
{
    char *buffer;
	CHECKHEAP;
    AWT_LOCK();
    AwtLabel *pLabel = (AwtLabel *)unhand(self)->pData;
    if (label != 0) {
	int len = javaStringLength(label);
	buffer = (char *)malloc(len + 1);
	javaString2CString(label, buffer, len + 1);
    } else {
	buffer = (char *)malloc(1);
	buffer[0] = 0;
    }
	AWT_TRACE(("%x:labelSetText(%s)\n", self, (buffer ? buffer : "")));
	pLabel->PostMsg(AwtLabel::SETTEXT, (long)buffer);

    AWT_UNLOCK();
	CHECKHEAP;
}


void 
sun_awt_win32_MLabelPeer_setAlignment(struct Hsun_awt_win32_MLabelPeer *self,
				      long alignment)
{
    CHECKHEAP;
    AWT_TRACE(("%x:labelSetAlignment(%ld)\n", self, alignment));
    AwtLabel *pLabel = (AwtLabel *)unhand(self)->pData;
    pLabel->SetAlignment(alignment);
    CHECKHEAP;
}

/** LIST methods */


void
sun_awt_win32_MListPeer_create(struct Hsun_awt_win32_MListPeer *self,
			       struct Hsun_awt_win32_MComponentPeer *parent)
{
	CHECKHEAP;
    AWT_LOCK();
    if (parent == 0 || unhand(parent)->pData == 0) {
		SignalError(0, JAVAPKG "NullPointerException", 0);
		AWT_UNLOCK();
		return;
    }
	Hjava_awt_List *selfT = (Hjava_awt_List*)unhand(self)->target;
	AwtWindow *pParent = (AwtWindow *)unhand(parent)->pData;

	AWT_TRACE(("%x:listCreate(%x)\n", parent));
	AwtList *pList = AwtList::Create(selfT, pParent);
	unhand(self)->pData = (long)pList;

    AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MListPeer_setColor(struct Hsun_awt_win32_MListPeer *self,
					   				   long fg,
					   				   struct Hjava_awt_Color *c)
{
	CHECKHEAP;
    AWT_LOCK();
    if (c == NULL) {
		SignalError(0,JAVAPKG "NullPointerException", "null color");
		AWT_UNLOCK();
		return;
    }
	Classjava_awt_Color *cC = (Classjava_awt_Color *)c->obj;
	COLORREF color = PALETTERGB((cC->value>>16)&0xff, (cC->value>>8)&0xff, (cC->value)&0xff);
	AwtList *pList = (AwtList *)unhand(self)->pData;

	AWT_TRACE(("%x:listSetColor(%d, %x)\n", self, fg, color));
	pList->PostMsg(AwtList::SETCOLOR, unhand(self)->pData, fg, color);

    AWT_UNLOCK();
	CHECKHEAP;
}

long
sun_awt_win32_MListPeer_isSelected(struct Hsun_awt_win32_MListPeer *self,
				   long pos)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtList *pList = (AwtList *)unhand(self)->pData;
	long result;

	AWT_TRACE(("%x:listIsSelected(%d)->", self, pos));
	result = pList->SendMsg(AwtList::ISSELECTED, pos);
	AWT_TRACE(("%d\n", result));
	
	AWT_UNLOCK();
	CHECKHEAP;
	return result;
}

void
sun_awt_win32_MListPeer_addItem(struct Hsun_awt_win32_MListPeer *self,
				Hjava_lang_String *item,
				long index)
{
	CHECKHEAP;
    AWT_LOCK();

	if (item == NULL) {
		SignalError(0, JAVAPKG "NullPointerException", "null item");
		AWT_UNLOCK();
		return;
	}
	AwtList *pList = (AwtList *)unhand(self)->pData;
	int len = javaStringLength(item);
 	char *buffer;

	buffer = (char *)malloc(len + 1);
	javaString2CString(item, buffer, len + 1);
	AWT_TRACE(("%x:listAddItem(%s)\n", self, buffer));
	pList->PostMsg(AwtList::ADDITEM, (long)buffer, index);

	AWT_UNLOCK();
	CHECKHEAP;
}
    
void
sun_awt_win32_MListPeer_makeVisible(struct Hsun_awt_win32_MListPeer *self,
					long pos)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtList *pList = (AwtList *)unhand(self)->pData;

	AWT_TRACE(("%x:listMakeVisible(%d)\n", self, pos));
	pList->PostMsg(AwtList::MAKEVISIBLE, pos);

	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MListPeer_select(struct Hsun_awt_win32_MListPeer *self,
			       long pos)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtList *pList = (AwtList *)unhand(self)->pData;

	AWT_TRACE(("%x:listSelect(%d)\n", self, pos));
	pList->PostMsg(AwtList::SELECT, pos, TRUE);

	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MListPeer_deselect(struct Hsun_awt_win32_MListPeer *self,
				 long pos)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtList *pList = (AwtList *)unhand(self)->pData;

	AWT_TRACE(("%x:listDeselect(%d)\n", self, pos));
	pList->PostMsg(AwtList::SELECT, pos, FALSE);

	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MListPeer_delItems(struct Hsun_awt_win32_MListPeer *self,
				 long start, long end)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtList *pList = (AwtList *)unhand(self)->pData;

	AWT_TRACE(("%x:listDelItems(%d, %d)\n", self, start, end));
	pList->PostMsg(AwtList::DELETEITEMS, start, end);

	AWT_UNLOCK();
	CHECKHEAP;
}	
    
void
sun_awt_win32_MListPeer_setMultipleSelections(struct Hsun_awt_win32_MListPeer *self,
					      long v)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtList *pList = (AwtList *)unhand(self)->pData;

	AWT_TRACE(("%x:listSetMultipleSelections(%d)\n", self, v));
	pList->PostMsg(AwtList::SETMULTIPLESELECTIONS, v);

	AWT_UNLOCK();
	CHECKHEAP;
}

/** MTOOLKIT methods */

void
sun_awt_win32_MToolkit_init(struct Hsun_awt_win32_MToolkit *self,
			    struct Hjava_lang_Thread *parent)
{
 	CHECKHEAP;
#ifdef HEAPCHECKING_ENABLED
printf("Heap-checking enabled\n");
#endif
	AWT_TRACE(("pInit\n"));
	((CAwtApp *)AfxGetApp())->Init(self, parent);
	CHECKHEAP;
}

void
sun_awt_win32_MToolkit_eventLoop(struct Hsun_awt_win32_MToolkit *self)
{
	CHECKHEAP;
	AWT_TRACE(("eventLoop\n"));
	((CAwtApp *)AfxGetApp())->MainLoop(self);
	CHECKHEAP;
}

void
sun_awt_win32_MToolkit_callbackLoop(struct Hsun_awt_win32_MToolkit *self)
{
	CHECKHEAP;
	AWT_TRACE(("callbackLoop\n"));
	((CAwtApp *)AfxGetApp())->CallbackLoop(self);
	CHECKHEAP;
}

void
sun_awt_win32_MToolkit_sync(struct Hsun_awt_win32_MToolkit *self)
{
	CHECKHEAP;
	AWT_TRACE(("sync\n"));
	GdiFlush();
	CHECKHEAP;
}

long
sun_awt_win32_MToolkit_getScreenHeight(struct Hsun_awt_win32_MToolkit *self)
{
	CHECKHEAP;
	long result;
	HWND hWnd = ::GetDesktopWindow();
	HDC hDC = ::GetDC(hWnd);

	AWT_TRACE(("%x:getScreenHeight()->", self));
	result = ::GetDeviceCaps(hDC, VERTRES);
	AWT_TRACE(("%d\n", result));
	::ReleaseDC(hWnd, hDC);
	CHECKHEAP;
	return result;
}

long
sun_awt_win32_MToolkit_getScreenWidth(struct Hsun_awt_win32_MToolkit *self)
{
	CHECKHEAP;
	long result;
	HWND hWnd = ::GetDesktopWindow();
	HDC hDC = ::GetDC(hWnd);

	AWT_TRACE(("%x:getScreenWidth()->", self));
	result = ::GetDeviceCaps(hDC, HORZRES);
	AWT_TRACE(("%d\n", result));
	::ReleaseDC(hWnd, hDC);
	CHECKHEAP;
	return result;
}

long
sun_awt_win32_MToolkit_getScreenResolution(struct Hsun_awt_win32_MToolkit *self)
{
	CHECKHEAP;
	long result;
	HWND hWnd = ::GetDesktopWindow();
	HDC hDC = ::GetDC(hWnd);

	AWT_TRACE(("%x:getScreenResolution()->", self));
	result = ::GetDeviceCaps(hDC, LOGPIXELSX);
	AWT_TRACE(("%d\n", result));
	::ReleaseDC(hWnd, hDC);
	CHECKHEAP;
	return result;
}

struct Hjava_awt_image_ColorModel *
sun_awt_win32_MToolkit_makeColorModel(struct Hsun_awt_win32_MToolkit *self)
{
	struct Hjava_awt_image_ColorModel *result;

	AWT_TRACE(("%x:makeColorModel()->", self));
	result = AwtImage::getColorModel();
	AWT_TRACE(("%x\n", result));
	CHECKHEAP;
	return result;
}


/** MENU methods */

void
sun_awt_win32_MMenuPeer_createMenu(struct Hsun_awt_win32_MMenuPeer *self,
			       struct Hjava_awt_MenuBar *parent)
{
	CHECKHEAP;
    AWT_LOCK();
    Hjava_awt_Menu	*selfT = (Hjava_awt_Menu *)unhand(self)->target;
    Classjava_awt_Menu	*selfTC = unhand(selfT);
    if (parent == 0 || selfTC == 0) {
		SignalError(0, JAVAPKG "NullPointerException", 0);
		AWT_UNLOCK();
		return;
    }
	Hsun_awt_win32_MMenuBarPeer *parentP = (Hsun_awt_win32_MMenuBarPeer*)unhand(parent)->peer;
    MenuBarData *pMenuBarData = (MenuBarData*)unhand(parentP)->pData;
	char stackBuffer[64];
	char *buffer = &stackBuffer[0];
	int len = javaStringLength(selfTC->label);

	if (len >= sizeof(stackBuffer)) {
		buffer = (char *)malloc(len + 1);
	}
	javaString2CString(selfTC->label, buffer, len + 1);
	AWT_TRACE(("%x:menuCreate(%x, %s, font=%x, tearoff=%d, ishelp=%d, enabled=%d)\n", 
                   self, parentP, buffer, selfTC->font, selfTC->tearOff, 
                   selfTC->isHelpMenu, selfTC->enabled));
	AwtMenu *pMenu = AwtMenu::Create(selfT, pMenuBarData->pFrame, pMenuBarData->hMenuBar, buffer);
        if (!selfTC->enabled) {
            pMenu->PostMsg(AwtMenu::ENABLE, selfTC->enabled);
        }
	if (len >= sizeof(stackBuffer)) {
		free(buffer);
	}
	unhand(self)->pData = (long)pMenu;

	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MMenuPeer_createSubMenu(struct Hsun_awt_win32_MMenuPeer *self,
				      struct Hjava_awt_Menu *parent)
{
	CHECKHEAP;
    AWT_LOCK();
    Hjava_awt_Menu	*selfT = (Hjava_awt_Menu *)unhand(self)->target;
    Classjava_awt_Menu	*selfTC = unhand(selfT);
    if (parent == 0 || selfT == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null parent");
		AWT_UNLOCK();
		return;
    }
	Hsun_awt_win32_MMenuPeer *parentP = (Hsun_awt_win32_MMenuPeer*)unhand(parent)->peer;
	AwtMenu *pParent = (AwtMenu*)unhand(parentP)->pData;
	char stackBuffer[64];
	char *buffer = &stackBuffer[0];
	int len = javaStringLength(selfTC->label);

	if (len >= sizeof(stackBuffer)) {
		buffer = (char *)malloc(len + 1);
	}
	javaString2CString(selfTC->label, buffer, len + 1);
	AWT_TRACE(("%x:menuCreateSub(%x %d)\n", self, parentP, selfTC->enabled));
	AwtMenu *pSubMenu = AwtMenu::Create(selfT, pParent->GetFrame(), pParent->m_hMenu, buffer);
        if (!selfTC->enabled) {
            pSubMenu->PostMsg(AwtMenu::ENABLE, selfTC->enabled);
        }
	if (len >= sizeof(stackBuffer)) {
		free(buffer);
	}
	unhand(self)->pData = (long)pSubMenu;

	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MMenuPeer_pEnable(struct Hsun_awt_win32_MMenuPeer *self,
                                long enable)
{
    CHECKHEAP;
    AWT_LOCK();
    AWT_TRACE(("%x:menuEnable(%d)\n",  self, enable));
    AwtMenu *pMenu = (AwtMenu *)unhand(self)->pData;
    pMenu->PostMsg(AwtMenu::ENABLE, enable);
    AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_MMenuPeer_dispose(struct Hsun_awt_win32_MMenuPeer *self)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtMenu *pMenu = (AwtMenu *)unhand(self)->pData;

	AWT_TRACE(("%x:menuDispose()\n", self));
	if (pMenu != NULL) {
		unhand(self)->pData = NULL;
		pMenu->PostMsg(AwtMenu::DISPOSE);
	}
	AWT_UNLOCK();
	CHECKHEAP;
}

/** MENUBAR methods */

void
sun_awt_win32_MMenuBarPeer_create(struct Hsun_awt_win32_MMenuBarPeer *self,
				  struct Hjava_awt_Frame *frame)
{
	CHECKHEAP;
	AWT_LOCK();
    if (frame == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null frame");
		AWT_UNLOCK();
		return;
    }
 	MenuBarData *pData = new MenuBarData();
        ((CAwtApp*)AfxGetApp())->cleanupListAdd((CObject*)pData);
	Hsun_awt_win32_MFramePeer *frameP = (Hsun_awt_win32_MFramePeer*)unhand(frame)->peer;
	AwtWindow *pWindow = (AwtWindow*)unhand(frameP)->pData;
	MainFrame *pFrame = pWindow->GetFrame();
	
	pData->pFrame = pFrame;
	pData->hMenuBar = pFrame->CreateMenuBar();
	ASSERT(pData->hMenuBar != NULL);
	AWT_TRACE(("%x:menuBarCreate(%x)\n", self, frameP));
	pFrame->PostMsg(MainFrame::DRAWMENUBAR, (long)pData->hMenuBar);
	unhand(self)->pData = (long)pData;

	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MMenuBarPeer_dispose(struct Hsun_awt_win32_MMenuBarPeer *self)
{
	CHECKHEAP;
	AWT_LOCK();
	MenuBarData *pData = (MenuBarData *)unhand(self)->pData;

	AWT_TRACE(("%x:menuBarDispose()\n", self));
	if (pData != NULL) {
            ((CAwtApp*)AfxGetApp())->cleanupListRemove((CObject*)pData);
            unhand(self)->pData = NULL;
	    pData->pFrame->PostMsg(MainFrame::DISPOSEMENUBAR, (long)pData->hMenuBar);
            delete pData;
	}
	
	AWT_UNLOCK();
	CHECKHEAP;
}

/** MENUITEM methods */

// This structure needs to be a CObject subclass, so it can be freed
// (if necessary) during cleanup.
class MenuItemData : public CObject {
public:
    DECLARE_DYNAMIC(MenuItemData)
    // The menu that contains this item.
    AwtMenu *pMenu;
    // The menu item id.
    UINT nID;
};
IMPLEMENT_DYNAMIC(MenuItemData, CObject)

void
sun_awt_win32_MMenuItemPeer_create(struct Hsun_awt_win32_MMenuItemPeer *self,
				   struct Hjava_awt_Menu *parent)
{
	CHECKHEAP;
   	AWT_LOCK();
    if (parent == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "parent null");
		AWT_UNLOCK();
		return;
    }
	Hsun_awt_win32_MMenuPeer *parentP = (Hsun_awt_win32_MMenuPeer*)unhand(parent)->peer;
	Hjava_awt_MenuItem *selfT = unhand(self)->target;
   	Classjava_awt_MenuItem *selfTC = unhand(unhand(self)->target);
   	Classsun_awt_win32_MMenuItemPeer *selfC = unhand(self);
	MenuItemData *pMenuItem = new MenuItemData();
        ((CAwtApp*)AfxGetApp())->cleanupListAdd((CObject*)pMenuItem);
	AwtMenu *pMenu = (AwtMenu *)(unhand(parentP)->pData);
	char stackBuffer[64];
	char *buffer = &stackBuffer[0];
	int len = javaStringLength(selfTC->label);

	if (len >= sizeof(stackBuffer)) {
		buffer = (char *)malloc(len + 1);
	}
	javaString2CString(selfTC->label, buffer, len + 1);
	pMenuItem->pMenu = pMenu;
	AWT_TRACE(("%x:menuItemCreate(%x, %s, %d)\n", self, parentP, buffer, 
		selfTC->enabled));
	if (!strcmp(buffer, "-")) {
		pMenu->PostMsg(AwtMenu::APPENDSEPARATOR);
	} else {
		pMenuItem->nID = pMenu->SendMsg(AwtMenu::APPENDITEM, (long)selfT, (long)buffer);
		if (!selfTC->enabled) {
			pMenu->PostMsg(AwtMenu::ENABLEITEM, pMenuItem->nID, FALSE);
		}
		if (selfC->isCheckbox) {
			Classjava_awt_CheckboxMenuItem *pC = (Classjava_awt_CheckboxMenuItem*)selfTC;
			if (pC->state) {
				pMenu->PostMsg(AwtMenu::SETMARK, pMenuItem->nID, TRUE);
			}
		}
		if (len >= sizeof(stackBuffer)) {
			free(buffer);
		}
		unhand(self)->pData = (long)pMenuItem;
	}

	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MMenuItemPeer_pEnable(struct Hsun_awt_win32_MMenuItemPeer *self,
								   long enable)
{
    CHECKHEAP;
    AWT_LOCK();
   	Classsun_awt_win32_MMenuItemPeer *selfC = unhand(self);
	MenuItemData *pMenuItem = (MenuItemData*)selfC->pData;

	AWT_TRACE(("%x:menuItemEnable(%d)\n",  self, enable));
	pMenuItem->pMenu->PostMsg(AwtMenu::ENABLEITEM, pMenuItem->nID, enable);

	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MMenuItemPeer_dispose(struct Hsun_awt_win32_MMenuItemPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
   	Classsun_awt_win32_MMenuItemPeer *selfC = unhand(self);
	MenuItemData *pMenuItem = (MenuItemData*)selfC->pData;

	AWT_TRACE(("%x:menuItemDispose()\n",  self));
	if (pMenuItem != NULL) {
            ((CAwtApp*)AfxGetApp())->cleanupListRemove((CObject*)pMenuItem);
		pMenuItem->pMenu->PostMsg(AwtMenu::DISPOSEITEM, pMenuItem->nID);
		unhand(self)->pData = NULL;
		delete pMenuItem;
	}
	AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_MCheckboxMenuItemPeer_setState(struct Hsun_awt_win32_MCheckboxMenuItemPeer *self,
					 long state)
{
    CHECKHEAP;
    AWT_LOCK();
   	Classsun_awt_win32_MCheckboxMenuItemPeer *selfC = unhand(self);
	MenuItemData *pMenuItem = (MenuItemData*)selfC->pData;

	AWT_TRACE(("%x:menuItemSetState(%d)\n",  self, state));
	pMenuItem->pMenu->PostMsg(AwtMenu::SETMARK, pMenuItem->nID, state);

	AWT_UNLOCK();
    CHECKHEAP;
}

long
sun_awt_win32_MCheckboxMenuItemPeer_getState(struct Hsun_awt_win32_MCheckboxMenuItemPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
    long result;
   	Classsun_awt_win32_MCheckboxMenuItemPeer *selfC = unhand(self);
	MenuItemData *pMenuItem = (MenuItemData*)selfC->pData;

	AWT_TRACE(("%x:menuItemGetState()->",  self));
	result = pMenuItem->pMenu->SendMsg(AwtMenu::ISMARKED, pMenuItem->nID);
	AWT_TRACE(("%d\n", result));

	AWT_UNLOCK();
    CHECKHEAP;

    return result;
}


/** SCROLLBAR methods */

void
sun_awt_win32_MScrollbarPeer_pCreate(struct Hsun_awt_win32_MScrollbarPeer *self,
				    struct Hsun_awt_win32_MComponentPeer *parent)
{
	CHECKHEAP;
    AWT_LOCK();
    if (parent == 0 || unhand(self)->target == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null parent");
		AWT_UNLOCK();
		return;
    }
	Hjava_awt_Scrollbar *selfT = (Hjava_awt_Scrollbar*)unhand(self)->target;
	Classjava_awt_Scrollbar *selfTC = unhand(selfT);
	AwtWindow *pParent = (AwtWindow *)unhand(parent)->pData;

	AWT_TRACE(("%x:scrollbarCreate(%x, vert=%d)->", self, parent, 
		selfTC->orientation == java_awt_Scrollbar_VERTICAL));
	AwtScrollbar *pScrollbar = AwtScrollbar::Create(selfT, pParent, 
		selfTC->orientation == java_awt_Scrollbar_VERTICAL, FALSE);
	unhand(self)->pData = (long)pScrollbar; 

	AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MScrollbarPeer_setColor(struct Hsun_awt_win32_MScrollbarPeer *self,
					   				   long fg,
					   				   struct Hjava_awt_Color *c)
{
	CHECKHEAP;
    AWT_LOCK();
    if (c == NULL) {
		SignalError(0,JAVAPKG "NullPointerException", "null color");
		AWT_UNLOCK();
		return;
    }
	Classjava_awt_Color *cC = (Classjava_awt_Color *)c->obj;
	COLORREF color = PALETTERGB((cC->value>>16)&0xff, (cC->value>>8)&0xff, (cC->value)&0xff);
	AwtScrollbar *pScrollbar = (AwtScrollbar *)unhand(self)->pData;

	AWT_TRACE(("%x:scrollbarSetColor(%d, %x)\n", self, fg, color));
	pScrollbar->PostMsg(AwtScrollbar::SETCOLOR, unhand(self)->pData, fg, color);

    AWT_UNLOCK();
	CHECKHEAP;
}

long
sun_awt_win32_MScrollbarPeer_minimum(struct Hsun_awt_win32_MScrollbarPeer *self)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtScrollbar *pScrollbar = (AwtScrollbar *)unhand(self)->pData;
	long result;

	AWT_TRACE(("%x:scrollbarMinimum()->", self));
	result = pScrollbar->SendMsg(AwtScrollbar::MINIMUM);
	AWT_TRACE(("%d\n", result));
	
	AWT_UNLOCK();
	CHECKHEAP;
	return result;
}

long
sun_awt_win32_MScrollbarPeer_maximum(struct Hsun_awt_win32_MScrollbarPeer *self)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtScrollbar *pScrollbar = (AwtScrollbar *)unhand(self)->pData;
	long result;

	AWT_TRACE(("%x:scrollbarMaximum()->", self));
	result = pScrollbar->SendMsg(AwtScrollbar::MAXIMUM);
	AWT_TRACE(("%d\n", result));

	AWT_UNLOCK();
	CHECKHEAP;
	return result;
}


long
sun_awt_win32_MScrollbarPeer_value(struct Hsun_awt_win32_MScrollbarPeer *self)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtScrollbar *pScrollbar = (AwtScrollbar *)unhand(self)->pData;
	long result;

	AWT_TRACE(("%x:scrollbarValue()->", self));
	result = pScrollbar->SendMsg(AwtScrollbar::VALUE);
	AWT_TRACE(("%d\n", result));

	AWT_UNLOCK();
	CHECKHEAP;
	return result;
}

void
sun_awt_win32_MScrollbarPeer_setValue(struct Hsun_awt_win32_MScrollbarPeer *self, long value)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtScrollbar *pScrollbar = (AwtScrollbar *)unhand(self)->pData;

	AWT_TRACE(("%x:scrollbarSetValue(%d)\n", self, value));
	pScrollbar->PostMsg(AwtScrollbar::SETVALUE, value);

	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MScrollbarPeer_setValues(struct Hsun_awt_win32_MScrollbarPeer *self,
				       long value,
				       long visible,
				       long minimum,
				       long maximum)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtScrollbar *pScrollbar = (AwtScrollbar *)unhand(self)->pData;

	AWT_TRACE(("%x:scrollbarSetValues(%d, %d, %d, %d)\n", self, 
		value, visible, minimum, maximum));
	pScrollbar->PostMsg(AwtScrollbar::SETVALUES, minimum, value, max(0, maximum - visible));
	
	AWT_UNLOCK();
	CHECKHEAP;
}


/** TEXTAREA methods */

void
sun_awt_win32_MTextAreaPeer_create(struct Hsun_awt_win32_MTextAreaPeer *self,
				   struct Hsun_awt_win32_MComponentPeer *parent)
{
	CHECKHEAP;
    AWT_LOCK();
    if (parent == 0 || unhand(parent)->pData == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null parent");
		AWT_UNLOCK();
		return;
    }
	Hjava_awt_TextArea *selfT = (Hjava_awt_TextArea*)unhand(self)->target;
	Classjava_awt_TextArea *selfTC = (Classjava_awt_TextArea*)unhand(selfT);
	AwtWindow *pParent = (AwtWindow *)unhand(parent)->pData;

	AWT_TRACE(("%x:textAreaCreate(%x, %d, %d)\n", self, parent, selfTC->rows, selfTC->cols));
	AwtEdit *pEdit = AwtEdit::Create(selfT, pParent, NULL, NULL, TRUE, selfTC->rows, selfTC->cols);

	unhand(self)->pData = (long)pEdit;
	AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MTextAreaPeer_setColor(struct Hsun_awt_win32_MTextAreaPeer *self,
					   				   long fg,
					   				   struct Hjava_awt_Color *c)
{
	CHECKHEAP;
    AWT_LOCK();
    if (c == NULL) {
		SignalError(0,JAVAPKG "NullPointerException", "null color");
		AWT_UNLOCK();
		return;
    }
	Classjava_awt_Color *cC = (Classjava_awt_Color *)c->obj;
	COLORREF color = PALETTERGB((cC->value>>16)&0xff, (cC->value>>8)&0xff, (cC->value)&0xff);
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;

	AWT_TRACE(("%x:textareaSetColor(%d, %x)\n", self, fg, color));
	pEdit->PostMsg(AwtEdit::SETCOLOR, unhand(self)->pData, fg, color);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MTextAreaPeer_pSetEditable(struct Hsun_awt_win32_MTextAreaPeer *self,
					/*boolean*/ long e)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;

	AWT_TRACE(("%x:textAreaSetEditable(%d)\n", self, e));
	pEdit->PostMsg(AwtEdit::SETEDITABLE, e);
	
	AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MTextAreaPeer_select(struct Hsun_awt_win32_MTextAreaPeer *self,
				   long selStart, long selEnd)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;

	AWT_TRACE(("%x:textAreaSelect(%d, %d)\n", self, selStart, selEnd));
	pEdit->PostMsg(AwtEdit::SETSELECTION, selStart, selEnd);

    AWT_UNLOCK();
	CHECKHEAP;
}

long
sun_awt_win32_MTextAreaPeer_getSelectionStart(struct Hsun_awt_win32_MTextAreaPeer *self)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;
	long result;

	AWT_TRACE(("%x:textAreaGetSelectionStart()->", self));
	result = pEdit->SendMsg(AwtEdit::GETSELECTIONSTART);
	AWT_TRACE(("%d\n", result));

	AWT_UNLOCK();
	CHECKHEAP;
	return result;
}

long
sun_awt_win32_MTextAreaPeer_getSelectionEnd(struct Hsun_awt_win32_MTextAreaPeer *self)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;
	long result;

	AWT_TRACE(("%x:textAreaGetSelectionEnd()->", self));
	result = pEdit->SendMsg(AwtEdit::GETSELECTIONEND);
	AWT_TRACE(("%d\n", result));

	AWT_UNLOCK();
	CHECKHEAP;
	return result;
}

void
sun_awt_win32_MTextAreaPeer_setText(struct Hsun_awt_win32_MTextAreaPeer *self,
				    Hjava_lang_String *txt)
{
	CHECKHEAP;
    AWT_LOCK();
    if (txt == 0) {
		SignalError(0,JAVAPKG "NullPointerException", "null text");
		AWT_UNLOCK();
		return;
    }
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;
	int nLen = javaStringLength(txt);
 	char *pBuffer;
	// Let's allocate a more space which may prevent a another malloc.
	int nBufferLen = nLen + nLen/10 + 1;
	
	pBuffer = (char *)malloc(nBufferLen);
	javaString2CString(txt, pBuffer, nBufferLen);
	pBuffer = AddCR(pBuffer, nBufferLen);
	AWT_TRACE(("%x:textAreaSetText(%s)\n", self, pBuffer));
	pEdit->PostMsg(AwtEdit::SETTEXT, (long)pBuffer);

	AWT_UNLOCK();
	CHECKHEAP;
}

Hjava_lang_String *
sun_awt_win32_MTextAreaPeer_getText(struct Hsun_awt_win32_MTextAreaPeer *self)
{
	CHECKHEAP;
    AWT_LOCK();
	CString string;
	int nStringLen;
	AwtEdit *pEdit = (AwtEdit *)self->obj->pData;

	AWT_TRACE(("%x:textAreaGetText()->", self));
	pEdit->SendMsg(AwtEdit::GETTEXT, (long)&string);
	nStringLen = RemoveCR(string.GetBuffer(0));
	AWT_TRACE(("%d characters\n", nStringLen));

	AWT_UNLOCK();
	CHECKHEAP;
	return makeJavaString(string.GetBuffer(0), nStringLen);
}

void
sun_awt_win32_MTextAreaPeer_insertText(struct Hsun_awt_win32_MTextAreaPeer *self,
				       Hjava_lang_String *txt,
				       long pos)
{
	CHECKHEAP;
    AWT_LOCK();
    if (txt == 0) {
		SignalError(0,JAVAPKG "NullPointerException", "null text");
		AWT_UNLOCK();
		return;
    }
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;
	int nLen = javaStringLength(txt);
 	char *pBuffer;
	// Let's allocate a more space which may prevent a another malloc.
	int nBufferLen = nLen + nLen/10 + 1;
	
	pBuffer = (char *)malloc(nBufferLen);
	javaString2CString(txt, pBuffer, nBufferLen);
	pBuffer = AddCR(pBuffer, nBufferLen);
	AWT_TRACE(("%x:textAreaInsertText(%d, %s)\n", self, pos, pBuffer));
	pEdit->PostMsg(AwtEdit::INSERTTEXT, (long)pBuffer, pos);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MTextAreaPeer_replaceText(struct Hsun_awt_win32_MTextAreaPeer *self,
					Hjava_lang_String *txt,
					long start,
					long end)
{
	CHECKHEAP;
    AWT_LOCK();
    if (txt == 0) {
		SignalError(0,JAVAPKG "NullPointerException", "null text");
	    AWT_UNLOCK();
		return;
    }
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;
	int nLen = javaStringLength(txt);
 	char *pBuffer;
	// Let's allocate a more space which may prevent a another malloc.
	int nBufferLen = nLen + nLen/10 + 1;
	
	pBuffer = (char *)malloc(nBufferLen);
	javaString2CString(txt, pBuffer, nBufferLen);
	pBuffer = AddCR(pBuffer, nBufferLen);
	AWT_TRACE(("%x:textAreaReplaceText(%s, %d, %d)->\n", self, pBuffer, start, end));
	pEdit->PostMsg(AwtEdit::REPLACETEXT, (long)pBuffer, start, end);

	AWT_UNLOCK();
	CHECKHEAP;
}

/** TEXTFIELD methods */

void
sun_awt_win32_MTextFieldPeer_create(struct Hsun_awt_win32_MTextFieldPeer *self,
				    struct Hsun_awt_win32_MComponentPeer *parent)
{
	CHECKHEAP;
    AWT_LOCK();
    if (parent==0 ||
		unhand(parent)->pData == 0 ||
		unhand(self)->target == 0) {
		SignalError(0,JAVAPKG "NullPointerException", "null parent");
		AWT_UNLOCK();
		return;
    }
	Hjava_awt_TextField *selfT = (Hjava_awt_TextField*)unhand(self)->target;
	AwtWindow *pParent = (AwtWindow *)unhand(parent)->pData;

	AWT_TRACE(("%x:textFieldCreate(%x)\n", self, parent));
	AwtEdit *pEdit = AwtEdit::Create(selfT, pParent, NULL, NULL, TRUE);
	unhand(self)->pData = (long)pEdit;

	AWT_UNLOCK();
	CHECKHEAP;
}

void 
sun_awt_win32_MTextFieldPeer_setColor(struct Hsun_awt_win32_MTextFieldPeer *self,
					   				   long fg,
					   				   struct Hjava_awt_Color *c)
{
	CHECKHEAP;
    AWT_LOCK();
    if (c == NULL) {
		SignalError(0,JAVAPKG "NullPointerException", "null color");
		AWT_UNLOCK();
		return;
    }
	Classjava_awt_Color *cC = (Classjava_awt_Color *)c->obj;
	COLORREF color = PALETTERGB((cC->value>>16)&0xff, (cC->value>>8)&0xff, (cC->value)&0xff);
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;

	AWT_TRACE(("%x:textfieldSetColor(%d, %x)\n", self, fg, color));
	pEdit->PostMsg(AwtEdit::SETCOLOR, unhand(self)->pData, fg, color);

    AWT_UNLOCK();
	CHECKHEAP;
}

struct Hjava_lang_String *
sun_awt_win32_MTextFieldPeer_getText(struct Hsun_awt_win32_MTextFieldPeer *self)
{
	CHECKHEAP;
    AWT_LOCK();
	CString string;
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;

	AWT_TRACE(("%x:textFieldGetText()->", self));
	pEdit->SendMsg(AwtEdit::GETTEXT, (long)&string);
	AWT_TRACE(("%s\n", string));
	
	AWT_UNLOCK();
	CHECKHEAP;
	return makeJavaString(string.GetBuffer(0), string.GetLength());
}

void 
sun_awt_win32_MTextFieldPeer_setText(struct Hsun_awt_win32_MTextFieldPeer *self,
				 struct Hjava_lang_String *l)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;
	int len = javaStringLength(l);
 	char *buffer;

	buffer = (char *)malloc(len + 1);
	javaString2CString(l, buffer, len + 1);
	AWT_TRACE(("%x:textFieldSetText(%s)\n", self, buffer));
	pEdit->PostMsg(AwtEdit::SETTEXT, (long)buffer);

    AWT_UNLOCK();
	CHECKHEAP;
}


void
sun_awt_win32_MTextFieldPeer_pSetEditable(struct Hsun_awt_win32_MTextFieldPeer *self,
					  long editable)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;

	AWT_TRACE(("%x:textFieldSetEditable(%d)\n", self, editable));
	pEdit->PostMsg(AwtEdit::SETEDITABLE, editable);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MTextFieldPeer_dispose(struct Hsun_awt_win32_MTextFieldPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;

    AWT_TRACE(("%x:textFieldDispose()\n", self));
    pEdit->PostMsg(AwtEdit::DISPOSE);

    AWT_UNLOCK();
    CHECKHEAP;
}


void 
sun_awt_win32_MTextFieldPeer_setEchoCharacter(struct Hsun_awt_win32_MTextFieldPeer *self, unicode c)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;

	AWT_TRACE(("%x:textFieldSetEchoCharacter(%c)\n", self, c));
	pEdit->PostMsg(AwtEdit::SETECHOCHARACTER, c);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MTextFieldPeer_select(struct Hsun_awt_win32_MTextFieldPeer *self,
				   long selStart, long selEnd)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;

	AWT_TRACE(("%x:textFieldSelect(%d, %d)\n", self, selStart, selEnd));
	pEdit->PostMsg(AwtEdit::SETSELECTION, selStart, selEnd);

    AWT_UNLOCK();
	CHECKHEAP;
}

long
sun_awt_win32_MTextFieldPeer_getSelectionStart(struct Hsun_awt_win32_MTextFieldPeer *self)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;
	long result;

	AWT_TRACE(("%x:textFieldGetSelectionStart()->", self));
	result = pEdit->SendMsg(AwtEdit::GETSELECTIONSTART);
	AWT_TRACE(("%d\n", result));

	AWT_UNLOCK();
	CHECKHEAP;
	return result;
}

long
sun_awt_win32_MTextFieldPeer_getSelectionEnd(struct Hsun_awt_win32_MTextFieldPeer *self)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtEdit *pEdit = (AwtEdit *)unhand(self)->pData;
	long result;

	AWT_TRACE(("%x:textFieldGetSelectionEnd()->", self));
	result = pEdit->SendMsg(AwtEdit::GETSELECTIONEND);
	AWT_TRACE(("%d\n", result));

	AWT_UNLOCK();
	CHECKHEAP;
	return result;
}

/** IMAGE methods **/

void
sun_awt_image_ImageRepresentation_offscreenInit(IRHandle irh)
{
    if (irh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();

    IRPtr ir = unhand(irh);
    if (ir->width <= 0 || ir->height <= 0) {
	SignalError(0, JAVAPKG "IllegalArgumentException", 0);
	AWT_UNLOCK();
	return;
    }

    AwtImage *pImage = new AwtImage;
    if (pImage == NULL) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    AWT_TRACE(("%x:imageRepOffscreenInit(%d, %d)->", irh, ir->width, ir->height));
    if (!pImage->Create(irh, ir->width, ir->height)) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	delete pImage;
    } else {
	unhand(irh)->pData = (long)pImage;
	unhand(irh)->availinfo = IMAGE_OFFSCRNINFO;
	AWT_TRACE(("%x\n", pImage));
    }

    AWT_UNLOCK();
}

long
sun_awt_image_ImageRepresentation_setBytePixels(IRHandle irh,
						long x, long y,
						long w, long h,
						CMHandle cmh, ByteHandle arrh,
						long off, long scansize)
{
    if (irh == 0 || cmh == 0 || arrh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return -1;
    }
    if (w <= 0 || h <= 0) {
	return -1;
    }
    if ((long)obj_length(arrh) < (scansize * h)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return -1;
    }
    AWT_LOCK();
    AwtImage *pImage = image_getpImage(irh);
    if (pImage == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return -1;
    }
    long ret = pImage->GenericImageConvert(cmh, 0, x, y, w, h,
					   unhand(arrh), off, 8, scansize,
					   pImage->GetSrcWidth(),
					   pImage->GetSrcHeight(),
					   pImage->GetWidth(),
					   pImage->GetHeight(),
					   pImage);

    AWT_UNLOCK();
    return ret;
}

long
sun_awt_image_ImageRepresentation_setIntPixels(IRHandle irh,
					       long x, long y, long w, long h,
					       CMHandle cmh, IntHandle arrh,
					       long off, long scansize)
{
    if (irh == 0 || cmh == 0 || arrh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return -1;
    }
    if (w <= 0 || h <= 0) {
	return -1;
    }
    if ((long)obj_length(arrh) < (scansize * h)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return -1;
    }
    AWT_LOCK();
    AwtImage *pImage = image_getpImage(irh);
    if (pImage == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return -1;
    }
    long ret = pImage->GenericImageConvert(cmh, 0, x, y, w, h,
					   unhand(arrh), off, 32, scansize,
					   pImage->GetSrcWidth(),
					   pImage->GetSrcHeight(),
					   pImage->GetWidth(),
					   pImage->GetHeight(),
					   pImage);

    AWT_UNLOCK();
    return ret;
}

long
sun_awt_image_ImageRepresentation_finish(IRHandle irh, long force)
{
    int ret = 0;
    if (irh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    AWT_LOCK();
    AwtImage *pImage = image_getpImage(irh);
    if (pImage != 0) {
	ret = pImage->freeRenderData(force);
    }
    AWT_UNLOCK();
    return ret;
}

void
sun_awt_image_ImageRepresentation_imageDraw(IRHandle irh, GHandle gh,
					    long x, long y,
					    struct Hjava_awt_Color *c)
{
    struct Hsun_awt_win32_Win32Graphics *self;

    if (irh == 0 || gh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    self = (struct Hsun_awt_win32_Win32Graphics *) gh;
    IRPtr ir = unhand(irh);
    if ((ir->availinfo & IMAGE_DRAWINFO) != IMAGE_DRAWINFO) {
	return;
    }

    AWT_LOCK();
    AwtImage *pImage = (AwtImage *) ir->pData;
    AwtGraphics *pGraphics = (AwtGraphics *)unhand(self)->pData;
	if (pGraphics != NULL && pImage != NULL) {
		pGraphics->DrawImage(pImage, x, y, c);
	}
    AWT_UNLOCK();
}

void
sun_awt_image_ImageRepresentation_disposeImage(IRHandle irh)
{
    if (irh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    AwtImage *pImage = (AwtImage *) irh->obj->pData;
    AWT_TRACE(("%x:imageDestroy()\n", pImage));
    if (pImage != NULL) {
	delete pImage;
	irh->obj->pData = NULL;
    }

    AWT_UNLOCK();
}

/** OFFSCREENIMAGESOURCE methods */

void
sun_awt_image_OffScreenImageSource_sendPixels(OSISHandle osish)
{
    OSISPtr osis = unhand(osish);
    IRHandle irh = osis->baseIR;
    IRPtr ir;

    if (irh == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    ir = unhand(irh);
    if ((ir->availinfo & IMAGE_OFFSCRNINFO) != IMAGE_OFFSCRNINFO) {
	SignalError(0, JAVAPKG "IllegalAccessError", 0);
	return;
    }
    AwtImage *pImage = image_getpImage(irh);
    if (pImage == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return;
    }
    pImage->sendPixels(osis);
}

/** DIALOG methods **/

void
sun_awt_win32_MDialogPeer_create(struct Hsun_awt_win32_MDialogPeer *self,
				 struct Hsun_awt_win32_MComponentPeer *parent)
{
	CHECKHEAP;
    AWT_LOCK();

    if (unhand(self)->target == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null target");
                AWT_UNLOCK();
		return;
    }
    MainFrame *pParent = NULL;
	Classsun_awt_win32_MDialogPeer *selfC = unhand(self);
	Hjava_awt_Frame *selfT = (Hjava_awt_Frame*)selfC->target;

    if (parent != NULL) {
		pParent = (MainFrame*)unhand(parent)->pData;
	}
	AWT_TRACE(("%x:frameCreate(%x)\n", self, parent));
	MainFrame *pFrame = MainFrame::Create(selfT, TRUE, pParent,
					      unhand(selfT)->width,
					      unhand(selfT)->height,
					      RGB(192, 192, 192));
    SET_PDATA(self, pFrame->GetMainWindow());

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MDialogPeer_setInsets(struct Hsun_awt_win32_MDialogPeer *self,
				   struct Hjava_awt_Insets *insets)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
    MainFrame *pFrame = pWindow->GetFrame();

    pFrame->SetInsets(insets);
    AWT_UNLOCK();
}


void
sun_awt_win32_MDialogPeer_pShow(struct Hsun_awt_win32_MDialogPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
    MainFrame *pFrame = pWindow->GetFrame();

    AWT_TRACE(("%x:frameShow()\n", self));
    AwtComp::PostMsg(AwtComp::SHOW, (long)pFrame, TRUE);

    AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_MDialogPeer_pHide(struct Hsun_awt_win32_MDialogPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
    MainFrame *pFrame = pWindow->GetFrame();

    AWT_TRACE(("%x:frameHide()\n", self));
    AwtComp::PostMsg(AwtComp::SHOW, (long)pFrame, FALSE);

    AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_MDialogPeer_pSetTitle(struct Hsun_awt_win32_MDialogPeer *self,
				   struct Hjava_lang_String *title)
{
    CHECKHEAP;
    AWT_LOCK();
	AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
	MainFrame *pFrame = pWindow->GetFrame();
	int len = javaStringLength(title);
 	char *buffer = NULL;

	if (len > 0) {
		buffer = (char *)malloc(len + 1);
		javaString2CString(title, buffer, len + 1);
	}
	AWT_TRACE(("%x:frameSetTitle(%s)\n", self, buffer));
	pFrame->PostMsg(MainFrame::SETTEXT, (long)buffer);

    AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_MDialogPeer_pDispose(struct Hsun_awt_win32_MDialogPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
	AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
	MainFrame *pFrame = pWindow->GetFrame();

	AWT_TRACE(("%x:frameDispose()\n", self));
	pFrame->PostMsg(MainFrame::DISPOSE);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MDialogPeer_pReshape(struct Hsun_awt_win32_MDialogPeer *self,
				  long x, long y,
				  long w, long h)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
	MainFrame *pFrame = pWindow->GetFrame();

	AWT_TRACE(("%x:dialogReshape(%d %d %d %d)\n", self, x, y, w, h));
	pFrame->PostMsg(MainFrame::RESHAPE, x, y, w, h);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MDialogPeer_setResizable(struct Hsun_awt_win32_MDialogPeer *self,
				      long resizable)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
	MainFrame *pFrame = pWindow->GetFrame();

	AWT_TRACE(("%x:frameSetResizable(%d)\n", self, resizable));
	pFrame->PostMsg(MainFrame::SETRESIZABLE, resizable);

    AWT_UNLOCK();
	CHECKHEAP;
}


/** WINDOW methods **/

void
sun_awt_win32_MWindowPeer_create(struct Hsun_awt_win32_MWindowPeer *self,
				 struct Hsun_awt_win32_MComponentPeer *parent)
{
	CHECKHEAP;
    AWT_LOCK();

    if (unhand(self)->target == 0) {
		SignalError(0, JAVAPKG "NullPointerException", "null target");
                AWT_UNLOCK();
		return;
    }
    MainFrame *pParent = NULL;
	Classsun_awt_win32_MWindowPeer *selfC = unhand(self);
	Hjava_awt_Frame *selfT = (Hjava_awt_Frame*)selfC->target;

    if (parent != NULL) {
		pParent = (MainFrame*)unhand(parent)->pData;
	}
	AWT_TRACE(("%x:frameCreate(%x)\n", self, parent));
	MainFrame *pFrame = MainFrame::Create(selfT, FALSE, pParent,
					      unhand(selfT)->width,
					      unhand(selfT)->height,
					      RGB(192, 192, 192));
    SET_PDATA(self, pFrame->GetMainWindow());

    AWT_UNLOCK();
	CHECKHEAP;
}


void
sun_awt_win32_MWindowPeer_setInsets(struct Hsun_awt_win32_MWindowPeer *self,
	  			   struct Hjava_awt_Insets *insets)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
    MainFrame *pFrame = pWindow->GetFrame();

    pFrame->SetInsets(insets);
    AWT_UNLOCK();
}


void
sun_awt_win32_MWindowPeer_pShow(struct Hsun_awt_win32_MWindowPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
    MainFrame *pFrame = pWindow->GetFrame();

    AWT_TRACE(("%x:frameShow()\n", self));
    AwtComp::PostMsg(AwtComp::SHOW, (long)pFrame, TRUE);

    AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_MWindowPeer_pHide(struct Hsun_awt_win32_MWindowPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
    AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
    MainFrame *pFrame = pWindow->GetFrame();

    AWT_TRACE(("%x:frameHide()\n", self));
    AwtComp::PostMsg(AwtComp::SHOW, (long)pFrame, FALSE);

    AWT_UNLOCK();
    CHECKHEAP;
}

void
sun_awt_win32_MWindowPeer_pDispose(struct Hsun_awt_win32_MWindowPeer *self)
{
    CHECKHEAP;
    AWT_LOCK();
	AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
	MainFrame *pFrame = pWindow->GetFrame();

	AWT_TRACE(("%x:frameDispose()\n", self));
	pFrame->PostMsg(MainFrame::DISPOSE);

    AWT_UNLOCK();
	CHECKHEAP;
}

void
sun_awt_win32_MWindowPeer_pReshape(struct Hsun_awt_win32_MWindowPeer *self,
				  long x, long y,
				  long w, long h)
{
	CHECKHEAP;
    AWT_LOCK();
	AwtWindow *pWindow = (AwtWindow*)unhand(self)->pData;
	MainFrame *pFrame = pWindow->GetFrame();

	AWT_TRACE(("%x:dialogReshape(%d %d %d %d)\n", self, x, y, w, h));
	pFrame->PostMsg(MainFrame::RESHAPE, x, y, w, h);

    AWT_UNLOCK();
	CHECKHEAP;
}
