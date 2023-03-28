/*
 * @(#)canvas.c	1.55 96/04/06 Sami Shaio
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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
#include <X11/keysym.h>
#include <X11/IntrinsicP.h>
#include <ctype.h>
#include "java_awt_Event.h"
#include "java_awt_Frame.h"
#include "java_awt_Component.h"
#include "sun_awt_motif_MComponentPeer.h"
#include "color.h"
#include "canvas.h"

int awt_override_key = -1;

extern void awt_setWidgetGravity(Widget w, int gravity);
extern Boolean scrollBugWorkAround;

static Boolean
containsGadgets(Widget w) {
    if (XtIsSubclass(w, xmFileSelectionBoxWidgetClass)) {
        return True;
    }
    return False;
}

static void
eatAllExposures(Display *dpy, Window windowid, XRectangle *rect)
{
    XEvent ev;
    int x, y, x2, y2;

    x = rect->x;
    y = rect->y;
    x2 = x + rect->width;
    y2 = y + rect->height;

#ifndef MIN
#define MIN(a,b) (int)((int)(a) < (int)(b) ? (a) : (b))
#define MAX(a,b) (int)((int)(a) > (int)(b) ? (a) : (b))
#endif

    XSync(dpy, False);
    while (XCheckTypedEvent(dpy, Expose, &ev)) {
	/* we check for the window here rather than using 		*/
	/* XCheckTypedWindowEvent because we don't want to alter the 	*/
	/* order of the expose events (which would sometimes cause 	*/
	/* widgets not to get redisplayed). 				*/
	if (ev.xexpose.window != windowid) {
	    XPutBackEvent(dpy, &ev);
	    break;
	}
	if (x == -1) {
	    x = ev.xexpose.x;
	    y = ev.xexpose.y;
	    x2 = ev.xexpose.x + ev.xexpose.width;
	    y2 = ev.xexpose.y + ev.xexpose.height;
	} else {
	    x = MIN(x, ev.xexpose.x);
	    y = MIN(y, ev.xexpose.y);
	    x2 = MAX(x2, ev.xexpose.x + ev.xexpose.width);
	    y2 = MAX(y2, ev.xexpose.y + ev.xexpose.height);
	}
    }
    rect->x = x;
    rect->y = y;
    rect->width = x2 - rect->x;
    rect->height = y2 - rect->y;
}

void
callJavaExpose(void *javaObject, XRectangle *rect)
{
    JAVA_UPCALL((EE(), javaObject,
		"handleExpose","(IIII)V",
		rect->x, rect->y, rect->width, rect->height));
}

static void
HandleExposeEvent(Widget w,  void *javaObject, XEvent *event)
{
    switch (event->type) {
    case Expose:
    case GraphicsExpose:
	{
	    struct ComponentData *cdata;
	    XRectangle	rect;

	    rect.x = event->xexpose.x;
	    rect.y = event->xexpose.y;
	    rect.width = event->xexpose.width;
	    rect.height = event->xexpose.height;

	    if ((javaObject == 0) || ((cdata = PDATA(ComponentData, (struct Hsun_awt_motif_MComponentPeer *)javaObject)) == 0)) {
		return;
	    }

	    if (event->xexpose.send_event) {
		
		/* repaint event */
		if (cdata->repaintPending) {
		    cdata->repaintPending = 0;
		    JAVA_UPCALL((EE(), javaObject,
				 "handleRepaint","(IIII)V",
				 cdata->x1, cdata->y1, 
				 cdata->x2 - cdata->x1, 
				 cdata->y2 - cdata->y1));
		}
		return;
		
	    }

	    if (cdata->repaintPending) {
		cdata->repaintPending = 0;
		if (rect.x > cdata->x1) {
		    rect.width += rect.x - cdata->x1;
		    rect.x = cdata->x1;
		}
		if (rect.y > cdata->y1) {
		    rect.height += rect.y - cdata->y1;
		    rect.y = cdata->y1;
		}
		if ((int)(rect.x + rect.width) < cdata->x2) {
		    rect.width = cdata->x2 - rect.x;
		}
		if ((int)(rect.y + rect.height) < cdata->y2) {
		    rect.height = cdata->y2 - rect.y;
		}
	    }
	    if (!containsGadgets(w))
	        eatAllExposures(XtDisplay(w), XtWindow(w), &rect);
	    callJavaExpose(javaObject, &rect);
	}
	break;

    default:
	printf("Got event %d in HandleExposeEvent!\n", event->type);
    }
}

int
getModifiers(unsigned int state)
{
    int modifiers = 0;

    if (state & ShiftMask) {
	modifiers |= java_awt_Event_SHIFT_MASK;
    }
    if (state & ControlMask) {
	modifiers |= java_awt_Event_CTRL_MASK;
    }
    if (state & Mod1Mask) {
	modifiers |= java_awt_Event_META_MASK;
    }
    if (state & Mod4Mask) {
	modifiers |= java_awt_Event_ALT_MASK;
    }
    if ((state&Button2Mask) != 0) {
	modifiers |= java_awt_Event_ALT_MASK;
    }
    if (state & Button3Mask) {
	modifiers |= java_awt_Event_META_MASK;
    }

    return modifiers;
}

#ifdef NEW_EVENT_MODEL
void
awt_modify_Event(XEvent *xevent, Hjava_awt_Event *jevent)
{
    Classjava_awt_Event *jevent_ptr = unhand(jevent);
    KeySym keysym;

    switch (xevent->type) {
      case KeyPress:
      case KeyRelease:
	switch (jevent_ptr->key) {
	  case '\010':
	    keysym = XK_BackSpace;
	    break;
	  case '\177':
	    keysym = XK_Delete;
	    break;
	  case '\t':
	    keysym = XK_Tab;
	    break;
	  case '\n':
	    keysym = XK_Return;
	    break;
	  case java_awt_Event_HOME:
	    keysym = XK_Home;
	    break;
	  case java_awt_Event_F1:
	    keysym = XK_F1;
	    break;
	  case java_awt_Event_F2:
	    keysym = XK_F2;
	    break;
	  case java_awt_Event_F3:
	    keysym = XK_F3;
	    break;
	  case java_awt_Event_F4:
	    keysym = XK_F4;
	    break;
	  case java_awt_Event_F5:
	    keysym = XK_F5;
	    break;
	  case java_awt_Event_F6:
	    keysym = XK_F6;
	    break;
	  case java_awt_Event_F7:
	    keysym = XK_F7;
	    break;
	  case java_awt_Event_F8:
	    keysym = XK_F8;
	    break;
	  case java_awt_Event_F9:
	    keysym = XK_F9;
	    break;
	  case java_awt_Event_F10:
	    keysym = XK_F10;
	    break;
	  case java_awt_Event_F11:
	    keysym = XK_F11;
	    break;
	  case java_awt_Event_F12:
	    keysym = XK_F12;
	    break;
	  case java_awt_Event_END:
	    keysym = XK_End;
	    break;
	  case java_awt_Event_PGUP:
	    /* PgUp */
	    keysym = XK_F29;
	    break;
	  case java_awt_Event_PGDN:
	    keysym = XK_F35;
	    break;
	  case java_awt_Event_UP:
	    keysym = XK_Up;
	    break;
	  case java_awt_Event_DOWN:
	    keysym = XK_Down;
	    break;
	  case java_awt_Event_LEFT:
	    keysym = XK_Left;
	    break;
	  case java_awt_Event_RIGHT:
	    keysym = XK_Right;
	    break;
	  case 27:
	    keysym = XK_Escape;
	    break;
	  default:
	    if (jevent_ptr->key < 256) {
		keysym = jevent_ptr->key;
	    } else {
		keysym = 0;
	    }
	}
	if (keysym != 0) {
	    xevent->xkey.keycode = XKeysymToKeycode(awt_display,keysym);
	}
    }
    if (keysym >= 'A' && keysym <= 'Z') {
	xevent->xkey.state |= ShiftMask;
    } else {
	switch (keysym) {
	  case '!': case '@': case '#': case '$':
	  case '%': case '^': case '&': case '*': case '(':
	  case ')': case '_': case '+': case '|': case '~':
	  case '{': case '}': case ':': case '\"': case '<':
	  case '>': case '?':
	    xevent->xkey.state |= ShiftMask;
	}
    }
    
    if (jevent_ptr->modifiers & java_awt_Event_SHIFT_MASK) {
	xevent->xkey.state |= ShiftMask;
    }
    if (jevent_ptr->modifiers & java_awt_Event_CTRL_MASK) {
	xevent->xkey.state |= ControlMask;
    }
    if (jevent_ptr->modifiers & java_awt_Event_META_MASK) {
	xevent->xkey.state |= Mod1Mask;
    }
    if (jevent_ptr->modifiers & java_awt_Event_ALT_MASK) {
	xevent->xkey.state |= Mod4Mask;
    }
}
#endif				/* NEW_EVENT_MODEL */


static XEvent *
awt_copyXEvent(XEvent *xev) 
{
    XEvent *xev_copy = (XEvent *)sysMalloc(sizeof(XEvent));

    if (xev_copy != NULL) {
	memcpy(xev_copy, xev, sizeof(XEvent));
	return xev_copy;
    } else {
	return NULL;
    }
}


static void
handleKeyEvent(char *javaMethod,
	       char *actionKeyMethod,
	       XEvent *event,
	       void *client_data,
	       Boolean *cont,
	       Boolean passEvent)
{
    char buf[64];
    int len = sizeof(buf);
    int modifiers = getModifiers(event->xkey.state);
    KeySym keysym = 0;
    int key = -1;

    len = XLookupString(&event->xkey, buf, len - 1, &keysym, NULL);
    buf[len - 1] = '\0';

    if (keysym < 256) {
	if ((event->xkey.state & ControlMask)) {
	    switch (keysym) {
	      case '[':
	      case ']':
	      case '\\':
	      case '_':
		keysym -= 64;
		break;
	      default:
		if (isalpha(keysym)) {
		    keysym = tolower(keysym) - 'a' + 1;
		}
		break;
	    }
	}
	JAVA_UPCALL((EE(),
		     (void *)client_data,
		     javaMethod,"(JIIIII)V",
		     int2ll(event->xkey.time),
		     (passEvent==TRUE) ? (long)awt_copyXEvent(event) : (long)0,
		     event->xkey.x, event->xkey.y,
		     keysym, modifiers));
    } else {
	switch (keysym) {
	  case XK_BackSpace:
	    key = '\010';
	    break;
	  case XK_Delete:
#ifdef XK_KP_Delete
	  case XK_KP_Delete:
#endif
	    key = '\177';
	    break;
	  case XK_Tab:
	    key = '\t';
	    break;
	  case XK_Return:
	  case XK_Linefeed:
	  case XK_KP_Enter:
	    key = '\n';
	    break;
	  case XK_F27:
	  case XK_Home:
	    /* Home */
	    key = java_awt_Event_HOME;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F1:
	    key = java_awt_Event_F1;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F2:
	    key = java_awt_Event_F2;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F3:
	    key = java_awt_Event_F3;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F4:
	    key = java_awt_Event_F4;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F5:
	    key = java_awt_Event_F5;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F6:
	    key = java_awt_Event_F6;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F7:
	    key = java_awt_Event_F7;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F8:
	    key = java_awt_Event_F8;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F9:
	    key = java_awt_Event_F9;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F10:
	    key = java_awt_Event_F10;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F11:
	    key = java_awt_Event_F11;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F12:
	    key = java_awt_Event_F12;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_R13:
	  case XK_End:
	    /* End */
	    key = java_awt_Event_END;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F29:
#ifdef XK_Prior
	  case XK_Prior:
#endif
	    /* PgUp */
	    key = java_awt_Event_PGUP;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F35:
#ifdef XK_Next
	  case XK_Next:
#endif
	    /* PgDn */
	    key = java_awt_Event_PGDN;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_Up:
	    /* Up */
	    key = java_awt_Event_UP;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_Down:
	    /* Down */
	    key = java_awt_Event_DOWN;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_Left:
	    /* Left */
	    key = java_awt_Event_LEFT;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_Right:
	    /* Right */
	    key = java_awt_Event_RIGHT;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_Escape:
	    key = 27;
	    break;
	  case XK_KP_Decimal:
	    key = '.';
	    break;
	  case XK_KP_Add:
	    key = '+';
	    break;
	  case XK_KP_Subtract:
	    key = '-';
	    break;
	  case XK_KP_Divide:
	    key = '/';
	    break;
	  case XK_KP_Multiply:
	    key = '*';
	    break;
	  case XK_KP_0:
	    key = '0';
	    break;
	  case XK_KP_1:
	    key = '1';
	    break;
	  case XK_KP_2:
	    key = '2';
	    break;
	  case XK_KP_3:
	    key = '3';
	    break;
	  case XK_KP_4:
	    key = '4';
	    break;
	  case XK_KP_5:
	    key = '5';
	    break;
	  case XK_KP_6:
	    key = '6';
	    break;
	  case XK_KP_7:
	    key = '7';
	    break;
	  case XK_KP_8:
	    key = '8';
	    break;
	  case XK_KP_9:
	    key = '9';
	    break;
	  default:
	    *cont = TRUE;
	    return;
	}
	if (key != -1) {
	    JAVA_UPCALL((EE(),
			 (void *)client_data,
			 javaMethod,"(JIIIII)V",
			 int2ll(event->xkey.time),
			 (passEvent==TRUE) ? (long)awt_copyXEvent(event) : (long)0,
			 event->xkey.x,
			 event->xkey.y,
			 key,
			 modifiers));
	}

    }
}

void
awt_canvas_handleEvent(Widget w, XtPointer client_data,
		       XEvent *event, Boolean *cont, Boolean passEvent)
{
    static int  	clickCount = 1;
    static XtPointer	peer = 0;
    static Time		lastTime = 0;

    int 	modifiers = 0;
    extern int inModalWait;

    if (inModalWait) {
	return;
    }

    *cont = FALSE;

    switch (event->type) {
      case SelectionClear:
      case SelectionNotify:
      case SelectionRequest:
	*cont = TRUE;
  	break;
      case GraphicsExpose:
      case Expose:
	HandleExposeEvent(w, client_data, event);
	break;
      case FocusIn:
	if (((XFocusChangeEvent*)event)->detail != NotifyPointer) {
	    JAVA_UPCALL((EE(),(void *)client_data, "gotFocus","()V"));
	    *cont = TRUE;
	}
	break;
      case FocusOut:
	if (((XFocusChangeEvent*)event)->detail != NotifyPointer) {
	    JAVA_UPCALL((EE(), (void *)client_data,  "lostFocus","()V"));
	    *cont = TRUE;
	}
	break;
      case ButtonPress:
	if (peer == 0) {
	    peer = client_data;
	    lastTime = event->xbutton.time;
	    clickCount = 1;
	} else if (peer == client_data) {
	    /* check for multiple clicks */
	    if ((event->xbutton.time - lastTime) <= awt_multiclick_time) {
		clickCount++;
	    } else {
		clickCount = 1;
	    }
	    lastTime = event->xbutton.time;
	} else {
	    clickCount = 1;
	    peer = client_data;
	    lastTime = event->xbutton.time;
	}
	/* XXX: Button2Mask and Button3Mask don't get set on ButtonPress */
	/* events properly so we set them here so getModifiers will do */
	/* the right thing. */
	switch (event->xbutton.button) {
	  case 2:
	    event->xbutton.state |= Button2Mask;
	    break;
	  case 3:
	    event->xbutton.state |= Button3Mask;
	    break;
	  default:
	    break;
	}
#ifdef KEYBOARD_ONLY_EVENTS
#define PASS_EVENT (long)0
#else
#define PASS_EVENT (passEvent==TRUE) ? (long)awt_copyXEvent(event) : (long)0
#endif
	modifiers = getModifiers(event->xbutton.state);
	JAVA_UPCALL((EE(),
		    (void *)client_data,
		    "handleMouseDown","(JIIIIIII)V",
		     int2ll(event->xbutton.time),
		     PASS_EVENT,
		     event->xbutton.x,
		     event->xbutton.y,
		     event->xbutton.x_root,
		     event->xbutton.y_root,
		     clickCount,
		     modifiers));
	break;
      case ButtonRelease:
	modifiers = getModifiers(event->xbutton.state);
	JAVA_UPCALL((EE(),
		    (void *)client_data,
		    "handleMouseUp","(JIIIIII)V",
		     int2ll(event->xbutton.time),
		     PASS_EVENT,
		     event->xbutton.x,
		     event->xbutton.y,
		     event->xbutton.x_root,
		     event->xbutton.y_root,
		     modifiers));
	break;
      case MotionNotify:
	clickCount = 0;
	lastTime = 0;
	peer = 0;
	modifiers = getModifiers(event->xmotion.state);
	if (event->xmotion.state & (Button1Mask | Button2Mask | Button3Mask)) {
	    JAVA_UPCALL((EE(),
			 (void *)client_data,
			 "handleMouseDrag","(JIIIIII)V",
			 int2ll(event->xmotion.time),
			 PASS_EVENT,
			 event->xmotion.x,
			 event->xmotion.y,
			 event->xmotion.x_root,
			 event->xmotion.y_root,
			 modifiers));
	} else {
	    JAVA_UPCALL((EE(),
			 (void *)client_data,
			 "handleMouseMoved","(JIIIIII)V",
			 int2ll(event->xmotion.time),
			 PASS_EVENT,
			 event->xmotion.x,
			 event->xmotion.y,
			 event->xmotion.x_root,
			 event->xmotion.y_root,
			 modifiers));
	}
	break;
      case KeyPress:
	clickCount = 0;
	lastTime = 0;
	peer = 0;
	handleKeyEvent("handleKeyPress",
		       "handleActionKey",
		       event, client_data, cont, passEvent);
	break;
      case KeyRelease:
	clickCount = 0;
	lastTime = 0;
	peer = 0;
	handleKeyEvent("handleKeyRelease",
		       "handleActionKeyRelease",
		       event, client_data, cont, passEvent);
	break;
      case EnterNotify:
      case LeaveNotify:
        if (((XCrossingEvent*)event)->detail == NotifyInferior)
	    return;
	clickCount = 0;
	lastTime = 0;
	peer = 0;
	switch (event->type) {
	  case EnterNotify:
	    JAVA_UPCALL((EE(),
			(void *)client_data,
			"handleMouseEnter","(JII)V",
			 int2ll(event->xcrossing.time),
			event->xcrossing.x,
			event->xcrossing.y));
	    break;
	  case LeaveNotify:
	    JAVA_UPCALL((EE(),
			(void *)client_data,
			"handleMouseExit","(JII)V",
			 int2ll(event->xcrossing.time),
			event->xcrossing.x,
			event->xcrossing.y));
	    break;
	}
	break;

      default:
	break;
    }
}

void
awt_canvas_event_handler(Widget w, XtPointer client_data,
			 XEvent *event, Boolean *cont)
{
    awt_canvas_handleEvent(w, client_data, event, cont, FALSE);
}

void
awt_canvas_pointerMotionEvents(Widget w, long on, XtPointer this)
{
    if (on) {
	XtAddEventHandler(w,
			  PointerMotionMask,
			  False,
			  awt_canvas_event_handler,
			  this);
    } else {
	XtRemoveEventHandler(w,
			     PointerMotionMask,
			     False,
			     awt_canvas_event_handler,
			     this);
    }
}

void
awt_canvas_reconfigure(struct FrameData *wdata)
{
    Dimension		w, h;

    if (wdata->winData.comp.widget == 0 ||
	XtParent(wdata->winData.comp.widget) == 0) {
	return;
    }
    XtVaGetValues(XtParent(wdata->winData.comp.widget), XmNwidth, &w, XmNheight, &h, NULL);
    XtConfigureWidget(wdata->winData.comp.widget,
		      -(wdata->left),
		      -(wdata->top),
		      w + (wdata->left + wdata->right),
		      h + (wdata->top + wdata->bottom),
		      0);
}

static void
Wrap_event_handler(Widget widget,
		   XtPointer client_data,
		   XmDrawingAreaCallbackStruct *call_data)
{
    awt_canvas_reconfigure((struct FrameData *)client_data);
}


Widget
awt_canvas_create(XtPointer this,
		  Widget parent,
		  long width,
		  long height,
		  struct FrameData *wdata)
{
    Widget		   newCanvas;
    Widget		   wrap;
    Arg			   args[20];
    int			   argc;

    if (parent == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    if (width == 0) {
	width = 1;
    }
    if (height == 0) {
	height = 1;
    }

    if (wdata != 0) {
	argc = 0;
	XtSetArg(args[argc], XmNwidth, (XtArgVal)width); argc++;
	XtSetArg(args[argc], XmNheight, (XtArgVal)height); argc++;
	XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
	XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
	XtSetArg(args[argc], XmNspacing, 0); argc++;
	XtSetArg(args[argc], XmNresizePolicy, XmRESIZE_NONE); argc++;
	wrap = XmCreateDrawingArea(parent,"",args,argc);
	XtAddCallback(wrap,
		      XmNresizeCallback,
		      (XtCallbackProc) Wrap_event_handler,
		      wdata);
	XtManageChild(wrap);
    } else {
	wrap = parent;
    }

    argc=0;
    XtSetArg(args[argc], XmNspacing, 0); argc++;
    XtSetArg(args[argc], XmNwidth, (XtArgVal)width); argc++;    
    XtSetArg(args[argc], XmNheight, (XtArgVal)height); argc++;
    XtSetArg(args[argc], XmNmarginHeight, 0); argc++;
    XtSetArg(args[argc], XmNmarginWidth, 0); argc++;
    XtSetArg(args[argc], XmNancestorSensitive, True); argc++;
    XtSetArg(args[argc], XmNresizePolicy, XmRESIZE_NONE); argc++;
    newCanvas = XmCreateDrawingArea(wrap,"",args,argc);
    XtSetMappedWhenManaged(newCanvas, False);
    XtManageChild(newCanvas);
/*
  XXX: causes problems on 2.5
    if (!scrollBugWorkAround) {
	awt_setWidgetGravity(newCanvas, StaticGravity);
    }
*/
    
    XtOverrideTranslations(newCanvas,
			   XtParseTranslationTable("<KeyDown>:DrawingAreaInput()"));

    XtSetSensitive(newCanvas, True);
    XtAddEventHandler(newCanvas, ExposureMask |
		      KeyPressMask| KeyReleaseMask | ButtonPressMask | 
		      ButtonReleaseMask | FocusChangeMask |
		      EnterWindowMask | LeaveWindowMask,
		      True, awt_canvas_event_handler, this);

    awt_canvas_pointerMotionEvents(newCanvas, 1, this);

    return newCanvas;
}

#if 0
static void
messWithGravity(Widget w, int gravity)
{
    extern void awt_changeAttributes(Display *dpy, Widget w,
				     unsigned long mask,
				     XSetWindowAttributes *xattr);
    XSetWindowAttributes    xattr;

    xattr.bit_gravity = gravity;
    xattr.win_gravity = gravity;

    awt_changeAttributes(XtDisplay(w), w,
			 CWBitGravity|CWWinGravity, &xattr);
}
#endif

struct MoveRecord {
    long dx;
    long dy;
};

void 
moveWidget(Widget w, void *data)
{
    struct MoveRecord *rec = (struct MoveRecord *)data;

    if (XtIsRealized(w) && XmIsRowColumn(w)) {
	    w->core.x -= rec->dx;
	    w->core.y -= rec->dy;
    }
}

#if 0
/* Scroll entire contents of window by dx and dy.  Currently only
   dy is supported.  A negative dy means scroll backwards, i.e.,
   contents in window move down. */
void
awt_canvas_scroll(XtPointer this,
		  struct CanvasData *wdata,
		  long dx,
		  long dy)
{

    Window		    win;
    XWindowChanges	    xchgs;
    Window		    root;
    int			    x, y;
    unsigned int	    width, height, junk;
    Display		    *dpy;
    struct	MoveRecord  mrec;

    mrec.dx = dx;
    mrec.dy = dy;

    dpy = XtDisplay(wdata->comp.widget);
    win = XtWindow(wdata->comp.widget);

    /* REMIND: consider getting rid of this! */
    XGetGeometry(awt_display,
		 win,
		 &root,
		 &x,
		 &y,
		 &width,
		 &height,
		 &junk,
		 &junk);

    /* we need to actually update the coordinates for manager widgets, */
    /* otherwise the parent won't pass down events to them properly */
    /* after scrolling... */
    awt_util_mapChildren(wdata->comp.widget, moveWidget, 0, &mrec);

    if (dx < 0) {
	/* scrolling backward */

	if (scrollBugWorkAround) {
	    messWithGravity(wdata->comp.widget, NorthWestGravity);
	}

	xchgs.x = x + dx;
	xchgs.y = y;
	xchgs.width = width - dx;
	xchgs.height = height;
	XConfigureWindow(awt_display,
			 win,
			 CWX|CWY|CWWidth|CWHeight,
			 &xchgs);

	if (scrollBugWorkAround) {
	    messWithGravity(wdata->comp.widget, NorthWestGravity);
	}

	xchgs.x = x;
	xchgs.y = y;
	XConfigureWindow(awt_display,
			 win,
			 CWX|CWY,
			 &xchgs);

	xchgs.width = width;
	xchgs.height = height;
	XConfigureWindow(awt_display,
			 win,
			 CWWidth|CWHeight,
			 &xchgs);
    } else {
	/* forward scrolling */

	/* make window a little taller */
	xchgs.width = width + dx;
	xchgs.height = height;
	XConfigureWindow(awt_display,
			 win,
			 CWWidth|CWHeight,
			 &xchgs);

	if (scrollBugWorkAround) {
	    messWithGravity(wdata->comp.widget, NorthEastGravity);
	}

	/* move window by amount we're scrolling */
	xchgs.x = x - dx;
	xchgs.y = y;
	XConfigureWindow(awt_display,
			 win,
			 CWX|CWY,
			 &xchgs);

	if (scrollBugWorkAround) {
	    messWithGravity(wdata->comp.widget, NorthWestGravity);
	}

	/* resize to original size */
	xchgs.x = x;
	xchgs.y = y;
	xchgs.width = width;
	xchgs.height = height;
	XConfigureWindow(awt_display,
			 win,
			 CWX|CWY|CWWidth|CWHeight,
			 &xchgs);
    }
    /* Because of the weird way we're scrolling this window,
       we have to eat all the exposure events that result from
       scrolling forward, and translate them up by the amount we're
       scrolling by.

       Rather than just eating all the exposures and having the
       java code fill in what it knows is exposed, we do it this
       way.  The reason is that there might be some other exposure
       events caused by overlapping windows on top of us that we
       also need to deal with. */
    {
	XRectangle	rect;

	rect.x = -1;
	eatAllExposures(dpy, win, &rect);
	if (rect.x != -1) {	/* we got at least one expose event */
	    if (dx > 0) {
		rect.x -= dx;
		rect.width += dx;
	    }
/*
	    printf("EXPOSE (%d): %d, %d, %d, %d\n",
		   dy, rect.x, rect.y, rect.width, rect.height);
*/
	    callJavaExpose(this, &rect);
	    XSync(awt_display, False);
	}
    }
    if (dy < 0) {
	/* scrolling backward */

	if (scrollBugWorkAround) {
	    messWithGravity(wdata->comp.widget, SouthGravity);
	}

	xchgs.x = x;
	xchgs.y = y + dy;
	xchgs.width = width;
	xchgs.height = height - dy;
	XConfigureWindow(awt_display,
			 win,
			 CWX|CWY|CWWidth|CWHeight,
			 &xchgs);

	if (scrollBugWorkAround) {
	    messWithGravity(wdata->comp.widget, NorthWestGravity);
	}

	xchgs.x = x;
	xchgs.y = y;
	XConfigureWindow(awt_display,
			 win,
			 CWX|CWY,
			 &xchgs);

	xchgs.width = width;
	xchgs.height = height;
	XConfigureWindow(awt_display,
			 win,
			 CWWidth|CWHeight,
			 &xchgs);
    } else {
	/* forward scrolling */

	/* make window a little taller */
	xchgs.width = width;
	xchgs.height = height + dy;
	XConfigureWindow(awt_display,
			 win,
			 CWWidth|CWHeight,
			 &xchgs);

	/* move window by amount we're scrolling */
	xchgs.x = x;
	xchgs.y = y - dy;
	XConfigureWindow(awt_display,
			 win,
			 CWX|CWY,
			 &xchgs);

	if (scrollBugWorkAround) {
	    messWithGravity(wdata->comp.widget, SouthGravity);
	}

	/* resize to original size */
	xchgs.x = x;
	xchgs.y = y;
	xchgs.width = width;
	xchgs.height = height;
	XConfigureWindow(awt_display,
			 win,
			 CWX|CWY|CWWidth|CWHeight,
			 &xchgs);
	if (scrollBugWorkAround) {
	    messWithGravity(wdata->comp.widget, NorthWestGravity);
	}

    }
    /* Because of the weird way we're scrolling this window,
       we have to eat all the exposure events that result from
       scrolling forward, and translate them up by the amount we're
       scrolling by.

       Rather than just eating all the exposures and having the
       java code fill in what it knows is exposed, we do it this
       way.  The reason is that there might be some other exposure
       events caused by overlapping windows on top of us that we
       also need to deal with. */
    {
	XRectangle	rect;

	rect.x = -1;
	eatAllExposures(dpy, win, &rect);
	if (rect.x != -1) {	/* we got at least one expose event */
	    if (dy > 0) {
		rect.y -= dy;
		rect.height += dy;
	    }
	    if (dx > 0) {
		rect.x -= dx;
		rect.width += dx;
	    }
/*
	    printf("EXPOSE (%d): %d, %d, %d, %d\n",
		   dy, rect.x, rect.y, rect.width, rect.height);
*/
	    callJavaExpose(this, &rect);
	    XSync(awt_display, False);
	}
    }
}
#endif


