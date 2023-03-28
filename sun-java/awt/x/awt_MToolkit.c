/*
 * @(#)awt_MToolkit.c	1.59 96/03/30 Sami Shaio
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
#include "canvas.h"
#include "interpreter.h"
#include <sys/time.h>
#include "sun_awt_motif_MToolkit.h"
#include "sun_awt_motif_InputThread.h"

#ifdef NEW_EVENT_MODEL
struct WidgetInfo {
    Widget widget;
    void   *peer;
    struct WidgetInfo *next;
};

static struct WidgetInfo *awt_winfo = (struct WidgetInfo *)0;
#endif

#if defined(AIX)
#include <sys/select.h>
#endif
#ifdef NETSCAPE
#include <signal.h>
extern int awt_init_xt;
#endif

#define bzero(a,b) memset(a, 0, b)

void		*awt_lock;
XtAppContext	awt_appContext;
Display		*awt_display;
Pixel		awt_defaultBg;
Pixel		awt_defaultFg;
int		awt_whitepixel;
int		awt_blackpixel;
Visual		*awt_visual;
GC		awt_maskgc;
Window		awt_root;
long		awt_screen;
int		awt_depth;
XVisualInfo	awt_visInfo;
Colormap	awt_cmap;
int		awt_multiclick_time;
PRThread	*awt_thread;

static Colormap GetDefaultColormap(void) {
    return DefaultColormap(awt_display, awt_screen);
}

Colormap (*awt_GetDefaultColormap)(void) = GetDefaultColormap;

#ifdef DEBUG_AWT_LOCK

int awt_locked = 0;
char *lastF = "";
int lastL = -1;

#endif

int inModalWait = 0;

/* list to associate a widget with a peer. It would have been
   preferable to use clientData but there seemed to be problems
   with that approach */
struct PeerList {
    Widget widget;
    void   *peer;
    struct PeerList *next;
};
struct PeerList *peers = NULL;

/*
 * error handlers
 */

static int
xerror_handler(Display *disp, XErrorEvent *err)
{
#ifdef DEBUG
    char msg[128];
    char buf[128];
    char *ev = getenv("NOISY_AWT");
    if (!ev || !ev[0]) return 0;
    XGetErrorText(disp, err->error_code, msg, sizeof(msg));
    fprintf(stderr, "Xerror %s\n", msg);
    jio_snprintf(buf, sizeof(buf), "%d", err->request_code);
    XGetErrorDatabaseText(disp, "XRequest", buf, "Unknown", msg, sizeof(msg));
    fprintf(stderr, "Major opcode %d (%s)\n", err->request_code, msg);
    if (err->request_code > 128) {
	fprintf(stderr, "Minor opcode %d\n", err->minor_code);
    }
    if (awt_lock) {
	/*SignalError(lockedee->lastpc, lockedee, "fp/ade/gui/GUIException", msg);*/
    }
    if (strcasecmp(ev, "abort") == 0) sysAbort();
#endif
    return 0;
}

static int
xtError() 
{
#ifdef DEBUG
    printf("Xt error\n");
    SignalError(0, JAVAPKG "NullPointerException", 0);
#endif
    return 0;
}

#if defined(NETSCAPE) && (defined(SCO) || defined(UNIXWARE))
/*
 * Prevent Motif warning messages from confusing Java apps
 */
static XtErrorMsgHandler
xtWarning(char *message)
{
#ifdef DEBUG
    extern FILE *real_stderr;

    if (message && *message)
       /* Is this safe to call? */
       (void)fprintf(real_stderr, "Java vs Motif: %s\n", message);
#endif
    return 0;
}
#endif

static int
xioerror_handler(Display *disp)
{
    if (awt_lock) {
	/*SignalError(lockedee->lastpc, lockedee, "fp/ade/gui/GUIException", "I/O error");*/
    }
    return 0;
}

Boolean scrollBugWorkAround;

void
sun_awt_motif_MToolkit_init(struct Hsun_awt_motif_MToolkit *this)
{
    int	argc = 0;
    char *argv[1];
    Display *dpy;
    XColor color;
    XVisualInfo viTmp, *pVI;
    int numvi;
#ifdef NETSCAPE
    sigset_t alarm_set, oldset;
#endif

    awt_lock = this;
    AWT_LOCK();

#ifdef NETSCAPE
    if (awt_init_xt) {
	XtToolkitInitialize();
    }
#else
    XtToolkitInitialize();
#endif

    argv[0] = 0;
#ifdef NETSCAPE
    /* Disable interrupts during XtOpenDisplay to avoid bugs in unix os select
       code: some unix systems don't implement SA_RESTART properly and
       because of this, select returns with EINTR. Most implementations of
       gethostbyname don't cope with EINTR properly and as a result we get
       stuck (forever) in the gethostbyname code
       */
    sigemptyset(&alarm_set);
    sigaddset(&alarm_set, SIGALRM);
    sigprocmask(SIG_BLOCK, &alarm_set, &oldset);
#endif
    awt_appContext = XtCreateApplicationContext();
#if defined(NETSCAPE) && (defined(SCO) || defined(UNIXWARE))
    XtAppSetWarningHandler(awt_appContext, &xtWarning);
#endif
    dpy = awt_display = XtOpenDisplay(awt_appContext,
				      (getenv("DISPLAY") == 0) ? ":0.0" : NULL,
				      "MToolkit app",
				      "XApplication",
				      NULL,
				      0,
				      &argc,
				      argv);
#ifdef NETSCAPE
    sigprocmask(SIG_SETMASK, &oldset, 0);
#endif

    XtAppSetErrorHandler(awt_appContext, (XtErrorHandler)xtError);
    if (!dpy) {
	char *errmsg = (char *)sysMalloc(1024);

	if (!errmsg) {
	    errmsg = "Can't connect to X11 window server";
	} else {
	    jio_snprintf(errmsg,
			 1024,
			 "Can't connect to X11 window server using '%s' as the value of the DISPLAY variable.",
			 (getenv("DISPLAY") == 0) ? ":0.0" : getenv("DISPLAY"));
	}
	SignalError(0, JAVAPKG "InternalError", errmsg);
	AWT_UNLOCK();
	return;
    }

    awt_multiclick_time = XtGetMultiClickTime(awt_display);
/*
    scrollBugWorkAround = 
	(strcmp(XServerVendor(dpy), "Sun Microsystems, Inc.") == 0
	 && XVendorRelease(dpy) == 3400);
*/
    scrollBugWorkAround = TRUE;

    XSetErrorHandler(xerror_handler);
    XSetIOErrorHandler(xioerror_handler);

    awt_screen = DefaultScreen(awt_display);
    awt_root = RootWindow(awt_display, awt_screen);
    if (!getenv("FORCEDEFVIS") && XMatchVisualInfo(dpy, awt_screen,
						   24, TrueColor,
						   &awt_visInfo))
    {
	awt_visual = awt_visInfo.visual;
	awt_depth = awt_visInfo.depth;
	if (awt_visual == DefaultVisual(awt_display, awt_screen)) {
	    awt_cmap = DefaultColormap(awt_display, awt_screen);
	} else {
	    awt_cmap = XCreateColormap(dpy, awt_root, awt_visual,
					  AllocNone);
	}
	color.flags = DoRed | DoGreen | DoBlue;
	color.red = color.green = color.blue = 0x0000;
	XAllocColor(dpy, awt_cmap, &color);
	awt_blackpixel = color.pixel;
	color.flags = DoRed | DoGreen | DoBlue;
	color.red = color.green = color.blue = 0xffff;
	XAllocColor(dpy, awt_cmap, &color);
	awt_whitepixel = color.pixel;
    } else {
	awt_visual = DefaultVisual(awt_display, awt_screen);
	awt_depth = DefaultDepth(awt_display, awt_screen);
#ifdef NETSCAPE
	awt_cmap = (*awt_GetDefaultColormap)();
#else
	awt_cmap = DefaultColormap(awt_display, awt_screen);
#endif /* NETSCAPE */
	viTmp.visualid = XVisualIDFromVisual(awt_visual);
	viTmp.depth = awt_depth;
	pVI = XGetVisualInfo(dpy, VisualIDMask | VisualDepthMask,
			     &viTmp, &numvi);
	if (!pVI) {
	    /* This should never happen. */
	    SignalError(0, JAVAPKG "InternalError",
			"Can't find default visual information");
	    AWT_UNLOCK();
	    return;
	}
	awt_visInfo = *pVI;
	XFree(pVI);
	awt_blackpixel = BlackPixel(awt_display, awt_screen);
	awt_whitepixel = WhitePixel(awt_display, awt_screen);
    }

    (void)awt_allocate_colors();
    awt_defaultBg = awtImage->ColorMatch(200, 200, 200);
    awt_defaultFg = awt_blackpixel;
    argc = 0;
    {
	Pixmap one_bit;
	XGCValues xgcv;
	xgcv.background = 0;
	xgcv.foreground = 1;
	one_bit = XCreatePixmap(awt_display, awt_root, 1, 1, 1);
	awt_maskgc = XCreateGC(awt_display, one_bit,
				  GCForeground | GCBackground, &xgcv);
	XFreePixmap(awt_display, one_bit);
    }
    AWT_UNLOCK();
}

#ifdef NEW_EVENT_MODEL

void
awt_addWidget(Widget w, void *peer)
{
     if (!XtIsSubclass(w, xmFileSelectionBoxWidgetClass) &&
	 !XtIsSubclass(w, xmDrawingAreaWidgetClass)) {
	struct WidgetInfo *nw = (struct WidgetInfo *)sysMalloc(sizeof(struct WidgetInfo));
	if (nw) {
	    nw->widget = w;
	    nw->peer = peer;
	    nw->next = awt_winfo;
	    awt_winfo = nw;
	} else {
	    SignalError(0, JAVAPKG "OutOfMemoryError", 0);	    
	}
    }
}

void
awt_delWidget(Widget w)
{
    struct WidgetInfo	*cw;

    if (awt_winfo != NULL) {
	if (awt_winfo->widget == w) {
	    cw = awt_winfo;
	    awt_winfo = awt_winfo->next;
	    sysFree((void *)cw);
	} else {
	    struct WidgetInfo *pw;

	    for (pw = awt_winfo, cw = awt_winfo->next;
		 cw != NULL;
		 pw = cw, cw = cw->next) {
		if (cw->widget == w) {
		    pw->next = cw->next;
		    sysFree((void *)cw);
		    break;
		}
	    }
	}
    }
}

static void *
findPeer(Widget widget)
{
    struct WidgetInfo	*cw;

    for (cw = awt_winfo; cw != NULL; cw = cw->next) {
	if (cw->widget == widget) {
	    return cw->peer;
	}
    }
    return NULL;
}


#define KEYBOARD_ONLY_EVENTS

static int
dispatchToWidget(XEvent *xev)
{
    Window	win;
    Widget	widget = 0;
    void	*peer = NULL;
    Boolean	cont = FALSE;

#ifdef KEYBOARD_ONLY_EVENTS
    switch (xev->type) {
      case KeyPress:
      case KeyRelease:
	win = xev->xkey.window;
	break;
      default:
	return 0;
    }
#endif				/* KEYBOARD_ONLY_EVENTS */

    widget = XtWindowToWidget(awt_display, win);

    peer = NULL;

    /* If it's a keyboard event, we need to find the peer associated */
    /* with the widget that has the focus rather than the widget */
    /* associated with the window in the X event. */

    switch (xev->type) {
      case KeyPress:
      case KeyRelease:
	{
	    Widget focusWidget = XmGetFocusWidget(widget);

	    if (focusWidget != 0 && focusWidget != widget &&
		(peer = findPeer(focusWidget)) != NULL) {
		widget = focusWidget;
	    }
	}
	break;
      default:
	break;
    }

    /* If we found a widget and a suitable peer (either the focus
       peer above or the one associated with the widget then we
       dispatch to it. */
    if (widget != NULL &&
	(peer!=NULL || (peer = findPeer(widget)))) {
	awt_canvas_handleEvent(widget, peer, xev, &cont, TRUE);
	if (cont == TRUE) {
	    return 0;
	}
	return 1;
    } else {
	return 0;
    }
}

#endif				/* NEW_EVENT_MODEL */

#include "prmon.h"
#include "prclist.h"

typedef struct ModalWorkItemStr {
    PRCList links;
    int (*terminateFn)(void *data);
    void *data;
} ModalWorkItem;

static PRCList workQ = PR_INIT_STATIC_CLIST(&workQ);

#define MODAL_WORK_ITEM_PTR(_qp) \
    ((ModalWorkItem*) ((char*) (_qp) - offsetof(ModalWorkItem,links)))

void awt_MToolkit_loop(void)
{
    XtInputMask	iMask;
    XEvent xev;
    PRCList *qp;
    fd_set rdset;

    /*
    ** Wait for something to show up for this connection. The awt-thread
    ** is the only thread that does this.
    */
    AWT_UNLOCK();
    FD_ZERO(&rdset);
    FD_SET(ConnectionNumber(awt_display), &rdset);
    select(ConnectionNumber(awt_display)+1, &rdset, 0,  0, 0);
    AWT_LOCK();

    for (;;) {
	iMask = XtAppPending(awt_appContext);
	if (iMask == 0)
	    break;
#ifdef NEW_EVENT_MODEL	
	if (iMask & XtIMXEvent) {
	    XtAppNextEvent(awt_appContext, &xev);
	    if (dispatchToWidget(&xev) == 0) {
		XtDispatchEvent(&xev);
	    }
	} else if (iMask & XtIMTimer) {
	    XtAppProcessEvent(awt_appContext, XtIMTimer);
	} else {
	    XtAppProcessEvent(awt_appContext, iMask);
	}
#else
	XtAppProcessEvent(awt_appContext, iMask);
#endif				/* NEW_EVENT_MODEL */
    }

    /* Scan work Q looking for modal operations that finished */
    qp = workQ.next;
    while (qp != &workQ) {
	ModalWorkItem *m = MODAL_WORK_ITEM_PTR(qp);
	if ((*m->terminateFn)(m->data)) {
	    /* All done */
	    PR_REMOVE_LINK(&m->links);
	    PR_CEnterMonitor(m);
	    PR_CNotify(m);
	    PR_CExitMonitor(m);
	}
	qp = qp->next;
    }
}

void
sun_awt_motif_MToolkit_run(struct Hsun_awt_motif_MToolkit *this)
{
    sysAssert(this == awt_lock);
    awt_thread = PR_CurrentThread();
    AWT_LOCK();
    for (;;) {
	(void) awt_MToolkit_loop();
    }
}

void
sun_awt_motif_MToolkit_sync(struct Hsun_awt_motif_MToolkit *this)
{
    AWT_LOCK();
    XSync(awt_display, False);
    AWT_UNLOCK();
}


void
awt_MToolkit_modalWait(int (*terminateFn)(void *data), void *data)
{
    ModalWorkItem e;

    if (PR_CurrentThread() == awt_thread) {
	/*
	** This is going to cause a deadlock. For some reason a modal
	** wait is being done by the awt thread (probably because
	** somebody's paint method brought up a modal dialog?).
	*/
	SignalError(0, "java/awt/AWTError", "modal deadlock");
	return;
    }

    /*
    ** Enter monitor before we append this modal wait to the queue so
    ** that we don't lose the notify
    */
    PR_CEnterMonitor(&e);

    /* Add this modal work item to the queue */
    AWT_LOCK();
    e.terminateFn = terminateFn;
    e.data = data;
    PR_APPEND_LINK(&e.links, &workQ);
    AWT_UNLOCK();

    /* Wait forever for modal operation to terminate */
    PR_CWait(&e, LL_MAXINT);
    PR_CExitMonitor(&e);
}

long
sun_awt_motif_MToolkit_getScreenWidth(struct Hsun_awt_motif_MToolkit *this)
{
    return DisplayWidth(awt_display, awt_screen);
}

long
sun_awt_motif_MToolkit_getScreenHeight(struct Hsun_awt_motif_MToolkit *this)
{
    return DisplayHeight(awt_display, awt_screen);
}

long
sun_awt_motif_MToolkit_getScreenResolution(struct Hsun_awt_motif_MToolkit *this)
{
    return (int)((DisplayWidth(awt_display, awt_screen) * 25.4) / 
		   DisplayWidthMM(awt_display, awt_screen));
}

struct Hjava_awt_image_ColorModel *
sun_awt_motif_MToolkit_makeColorModel(struct Hsun_awt_motif_MToolkit *this)
{
    extern struct Hjava_awt_image_ColorModel *awt_getColorModel(void);
    return awt_getColorModel();
}
