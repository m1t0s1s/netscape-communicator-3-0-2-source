#include <X11/keysym.h>
#include <ctype.h>
/*#include "thread.h"*/
#include "tiny.h"
#include "java_awt_Event.h"
#include "sun_awt_tiny_TinyToolkit.h"
#include "sun_awt_tiny_TinyWindow.h"
#include "sun_awt_tiny_TinyEventThread.h"
#include "sun_awt_tiny_TinyInputThread.h"

#define bzero(a,b) memset(a, 0, b)

void		*awt_lock;
Display 	*awt_display;
long 		awt_screen;
Window 		awt_root;
int		awt_depth;
XVisualInfo	awt_visInfo;
Colormap	awt_cmap;
Visual		*awt_visual;
int		awt_whitepixel;
int		awt_blackpixel;
GC		awt_maskgc;
static int	awt_flush;

#ifdef DEBUG_AWT_LOCK

int awt_locked = 0;
char *lastF = "";
int lastL = -1;

#endif

#define MAXWINDOWS 512
TinyWindow *windows[MAXWINDOWS];

void
tiny_register(TinyWindow *win)
{
    int i;

    for (i = 0 ; i < MAXWINDOWS ; i++) {
	if (!windows[i]) {
	    windows[i] = win;
	    return;
	}
    }
}

void
tiny_unregister(TinyWindow *win)
{
    int i;

    for (i = 0 ; i < MAXWINDOWS ; i++) {
	if (windows[i] == win) {
	    windows[i] = 0;
	    return;
	}
    }
}

static TinyWindow *
tiny_find(Window win)
{
    long xid = (long)win;
    int i;

    for (i = 0 ; i < MAXWINDOWS ; i++) {
	if (windows[i] && (unhand(windows[i])->xid == xid)) {
	    return windows[i];
	}
    }
    return 0;
}

static int
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

static int
xerror_handler(Display *disp, XErrorEvent *err)
{
    char msg[128];
    char buf[128];
    XGetErrorText(disp, err->error_code, msg, sizeof(msg));
    fprintf(stderr, "Xerror %s\n", msg);
    jio_snprintf(buf, sizeof(buf), "%d", err->request_code);
    XGetErrorDatabaseText(disp, "XRequest", buf, "Unknown", msg, sizeof(msg));
    fprintf(stderr, "Major opcode %d (%s)\n", err->request_code, msg);
    if (err->request_code > 128) {
	fprintf(stderr, "Minor opcode %d\n", err->minor_code);
    }
    return 0;
}

static int
xioerror_handler(Display *disp)
{
    fprintf(stderr, "X I/O Error\n");
    return 0;
}

static void
handleKeyEvent(char *javaMethod,
	       char *actionKeyMethod,
	       XEvent *event,
	       TinyWindow *win)
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
		     (HObject*)win,
		     javaMethod,"(JIIII)V",
		     int2ll(event->xkey.time),
		     event->xkey.x, event->xkey.y,
		     keysym, modifiers));
    } else {
	switch (keysym) {
	  case XK_BackSpace:
	    key = '\010';
	    break;
	  case XK_Delete:
	  case XK_KP_Delete:
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
	  case XK_Page_Up:
	    /* PgUp */
	    key = java_awt_Event_PGUP;
	    javaMethod = actionKeyMethod;
	    break;
	  case XK_F35:
	  case XK_Page_Down:
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
	    break;
	}
	if (key != -1) {
	    JAVA_UPCALL((EE(),
			 (HObject*)win,
			 javaMethod,"(JIIII)V",
			 int2ll(event->xkey.time),
			 event->xkey.x,
			 event->xkey.y,
			 key,
			 modifiers));
	}

    }
}


void 
sun_awt_tiny_TinyToolkit_init(TinyToolkit *this)
{
    XColor color;
    XVisualInfo viTmp, *pVI;
    int numvi;
    char *dstr = getenv("DISPLAY");

    awt_lock = this;

    AWT_LOCK();
    awt_display = XOpenDisplay(dstr ? dstr : ":0.0");

    if (awt_display == 0) {
	SignalError(0, JAVAPKG "InternalError", "Can't connect to X server");
	AWT_UNLOCK();
	return;
    }

    XSetErrorHandler(xerror_handler);
    XSetIOErrorHandler(xioerror_handler);

    awt_screen = DefaultScreen(awt_display);
    awt_root = RootWindow(awt_display, awt_screen);
    if (!getenv("FORCEDEFVIS") && XMatchVisualInfo(awt_display, awt_screen,
						   24, TrueColor,
						   &awt_visInfo))
    {
	awt_visual = awt_visInfo.visual;
	awt_depth = awt_visInfo.depth;
	if (awt_visual == DefaultVisual(awt_display, awt_screen)) {
	    awt_cmap = DefaultColormap(awt_display, awt_screen);
	} else {
	    awt_cmap = XCreateColormap(awt_display, awt_root, awt_visual,
					  AllocNone);
	}
	color.flags = DoRed | DoGreen | DoBlue;
	color.red = color.green = color.blue = 0x0000;
	XAllocColor(awt_display, awt_cmap, &color);
	awt_blackpixel = color.pixel;
	color.flags = DoRed | DoGreen | DoBlue;
	color.red = color.green = color.blue = 0xffff;
	XAllocColor(awt_display, awt_cmap, &color);
	awt_whitepixel = color.pixel;
    } else {
	awt_visual = DefaultVisual(awt_display, awt_screen);
	awt_depth = DefaultDepth(awt_display, awt_screen);
	awt_cmap = DefaultColormap(awt_display, awt_screen);
	viTmp.visualid = XVisualIDFromVisual(awt_visual);
	viTmp.depth = awt_depth;
	pVI = XGetVisualInfo(awt_display, VisualIDMask | VisualDepthMask,
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
    awt_allocate_colors();

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

void
tiny_flush() 
{
    if (!awt_flush++) {
	AWT_NOTIFY();
    }
}

void 
sun_awt_tiny_TinyInputThread_run(TinyInputThread *this)
{
    AWT_LOCK();

    while (1) {
	fd_set rdset;

	FD_ZERO(&rdset);
	FD_SET(ConnectionNumber(awt_display), &rdset);
	AWT_UNLOCK();
	select(ConnectionNumber(awt_display)+1, &rdset, 0,  0, 0);
	AWT_LOCK();

	tiny_flush();
	while (awt_flush > 0) {
	    AWT_WAIT(TIMEOUT_INFINITY);
	}
    }
    /* NOT REACHED */
}

static void FlushMotionEvent(XEvent *evt) {
    TinyWindow *win;
    int modifiers;

    if ((win = tiny_find(evt->xexpose.window)) == 0) {
	return;
    }

    modifiers = getModifiers(evt->xmotion.state);
    if (evt->xmotion.state & (Button1Mask | Button2Mask | Button3Mask)) {
	INVOKE((EE(), (HObject *)win,
		"handleMouseDrag","(JIII)V",
		int2ll(evt->xmotion.time),
		evt->xmotion.x,
		evt->xmotion.y,
		modifiers));
    } else {
	INVOKE((EE(), (HObject *)win,
		"handleMouseMove","(JIII)V",
		int2ll(evt->xmotion.time),
		evt->xmotion.x,
		evt->xmotion.y,
		modifiers));
    }
}

void 
sun_awt_tiny_TinyEventThread_run(TinyEventThread *this)
{
    TinyWindow *win;
    int modifiers;
    XEvent evt;
    TinyWindow *exposeWin = 0;
    int x1, y1, x2, y2;
#ifdef MOTION_COMPRESSION
    XEvent lastMotionEvent;
    int haveMotionEvent;
#endif

    AWT_LOCK();
    
    while (1) {
	/* flush */
	if (--awt_flush > 0) {
	    XFlush(awt_display);
	    awt_flush = 0;
	    AWT_NOTIFY();
	}

	AWT_WAIT(TIMEOUT_INFINITY);
	awt_flush++;
#ifdef MOTION_COMPRESSION
	haveMotionEvent = 0;
#endif

	/* process events */
	while (XPending(awt_display)) {
	    XNextEvent(awt_display, &evt);
#ifdef MOTION_COMPRESSION
	    if (haveMotionEvent &&
		((evt.type != MotionNotify) ||
		 (evt.xmotion.window != lastMotionEvent.xmotion.window) ||
		 (evt.xmotion.state != lastMotionEvent.xmotion.state))) {
		FlushMotionEvent(&lastMotionEvent);
	    }
#endif

	    switch (evt.type) {
	    case Expose:
		if ((win = tiny_find(evt.xexpose.window)) == 0) {
		    break;
		}
		if (exposeWin == win) {
		    x1 = MIN(x1, evt.xexpose.x);
		    y1 = MIN(y1, evt.xexpose.y);
		    x2 = MAX(x2, evt.xexpose.x + evt.xexpose.width);
		    y2 = MAX(y2, evt.xexpose.y + evt.xexpose.height);
		} else {
		    if (exposeWin != 0) {
			INVOKE((EE(), (HObject *)exposeWin, "handleExpose","(IIII)V",
				x1, y1, x2 - x1, y2 - y1));
		    }
		    exposeWin = win;
		    x1 = evt.xexpose.x;
		    y1 = evt.xexpose.y;
		    x2 = x1 + evt.xexpose.width;
		    y2 = y1 + evt.xexpose.height;
		}
		if (evt.xexpose.count == 0) {
		    INVOKE((EE(), (HObject *)exposeWin, "handleExpose","(IIII)V",
			    x1, y1, x2 - x1, y2 - y1));
		    exposeWin = 0;
		}
		break;

	    case GraphicsExpose:
		if ((win = tiny_find(evt.xexpose.window)) == 0) {
		    break;
		}
		INVOKE((EE(), (HObject *)win, "handleExpose","(IIII)V",
			evt.xexpose.x, evt.xexpose.y, evt.xexpose.width, evt.xexpose.height));
		break;

	    case ButtonPress:
		if ((win = tiny_find(evt.xexpose.window)) == 0) {
		    break;
		}
		switch (evt.xbutton.button) {
		case 2:
		    evt.xbutton.state |= Button2Mask;
		    break;
		case 3:
		    evt.xbutton.state |= Button3Mask;
		    break;
		}
		modifiers = getModifiers(evt.xbutton.state);
		INVOKE((EE(), (HObject *)win,
			"handleMouseDown","(JIII)V",
			int2ll(evt.xbutton.time),
			evt.xbutton.x, evt.xbutton.y,
			modifiers));
		    break;

	    case ButtonRelease:
		if ((win = tiny_find(evt.xexpose.window)) == 0) {
		    break;
		}
		modifiers = getModifiers(evt.xbutton.state);
		INVOKE((EE(), (HObject *)win,
			"handleMouseUp","(JIII)V",
			int2ll(evt.xbutton.time),
			evt.xbutton.x,
			evt.xbutton.y,
			modifiers));
		break;

	    case MotionNotify:
#ifdef MOTION_COMPRESSION
	        lastMotionEvent = evt;
		haveMotionEvent = 1;
#else
		FlushMotionEvent(&evt);
#endif
		break;

	    case EnterNotify:
	    case LeaveNotify:
		if ((win = tiny_find(evt.xexpose.window)) == 0) {
		    break;
		}

		switch (evt.type) {
		case EnterNotify:
		    INVOKE((EE(), (HObject *)win,
				 "handleMouseEnter","(JII)V",
				 int2ll(evt.xcrossing.time),
				 evt.xcrossing.x,
				 evt.xcrossing.y));
		    break;
		case LeaveNotify:
		    INVOKE((EE(), (HObject *)win,
			    "handleMouseExit","(JII)V",
			    int2ll(evt.xcrossing.time),
			    evt.xcrossing.x,
			    evt.xcrossing.y));
		    break;
		}
		break;

	    case KeyPress:
		if ((win = tiny_find(evt.xexpose.window)) == 0) {
		    break;
		}
		handleKeyEvent("handleKeyPress",
			       "handleActionKey",
			       &evt, win);
		break;
	    case KeyRelease:
		if ((win = tiny_find(evt.xexpose.window)) == 0) {
		    break;
		}
		handleKeyEvent("handleKeyRelease",
			       "handleActionKeyRelease",
			       &evt, win);
		break;

	    case FocusIn:
		if ((win = tiny_find(evt.xexpose.window)) == 0) {
		    break;
		}
		INVOKE((EE(), (HObject *)win, "handleFocusIn","()V"));
		break;

	    case FocusOut:
		if ((win = tiny_find(evt.xexpose.window)) == 0) {
		    break;
		}
		INVOKE((EE(), (HObject *)win, "handleFocusOut","()V"));
		break;

	    default:
		/*printf("got event 0x%d\n", evt.type);*/
		break;
	    }
	}
#ifdef MOTION_COMPRESSION
	if (haveMotionEvent) {
	    FlushMotionEvent(&lastMotionEvent);
	}
#endif
    }

    /* NOT REACHED */
}

long
sun_awt_tiny_TinyToolkit_getScreenWidth(TinyToolkit *this)
{
    return DisplayWidth(awt_display, awt_screen);
}

long
sun_awt_tiny_TinyToolkit_getScreenHeight(TinyToolkit *this)
{
    return DisplayHeight(awt_display, awt_screen);
}

long
sun_awt_tiny_TinyToolkit_getScreenResolution(TinyToolkit *this)
{
    return (int)((DisplayWidth(awt_display, awt_screen) * 25.4) / 
		 DisplayWidthMM(awt_display, awt_screen));
}

struct Hjava_awt_image_ColorModel *
sun_awt_tiny_TinyToolkit_makeColorModel(TinyToolkit *this)
{
    return awt_getColorModel();
}

void
sun_awt_tiny_TinyToolkit_sync(TinyToolkit *this)
{
    XSync(awt_display, False);
}
