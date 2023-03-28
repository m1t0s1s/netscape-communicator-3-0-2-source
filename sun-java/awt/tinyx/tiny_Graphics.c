#include "tiny.h"
#include "image.h"
#include "java_awt_Rectangle.h"
#include "sun_awt_tiny_TinyGraphics.h"
#include "sun_awt_tiny_TinyWindow.h"

#define OX(this)		(unhand(this)->originX)
#define OY(this)		(unhand(this)->originY)
#define TX(this, x)		((x) + OX(this))
#define TY(this, y)		((y) + OY(this))


/**
 * There is no need to worry about the gc_owner being
 * garbage collected. If that happens, and if a new
 * one is created at exactly the same address it would
 * still be marked touched and the gc will be reinitialized.
 */
static TinyGraphics *gc_owner = 0;
static GC gc = 0;

static GC 
tiny_gc(TinyGraphics *this)
{
    Window win = (Window)unhand(this)->pDrawable;
    XGCValues atts;
    int mask = 0;

    if (win == 0) {
	return 0;
    }

    if ((gc == 0) || (this != gc_owner) || unhand(this)->touched) {
	if (gc == 0) {
	    gc = XCreateGC(awt_display, win, 0, 0);
	    if (gc == 0) {
		return 0;
	    }
	}
	gc_owner = this;

	if (unhand(this)->font) {
	    XFontStruct *f = awt_getFont(unhand(this)->font);
	    if (f) {
		atts.font = f->fid;
		mask |= GCFont;
	    }
	}
	if (unhand(this)->color) {
	    atts.foreground = awt_getColor(unhand(this)->color);
	    mask |= GCForeground;
	} else {
	    atts.foreground = 0;
	}
	if (unhand(this)->xorColor) {
	    atts.foreground ^= awt_getColor(unhand(this)->xorColor);
	    atts.function = GXxor;
	    mask |= GCFunction | GCForeground;
	} else {
	    atts.function = GXcopy;
	    mask |= GCFunction;
	}
	XChangeGC(awt_display, gc, mask, &atts);

	if (unhand(this)->clip) {
	    XRectangle rect;
	    int w = unhand(unhand(this)->clip)->width;
	    int h = unhand(unhand(this)->clip)->height;
	    rect.x = unhand(unhand(this)->clip)->x;
	    rect.y = unhand(unhand(this)->clip)->y;
	    rect.width = (w < 0) ? 0 : w;
	    rect.height = (h < 0) ? 0 : h;
	    XSetClipRectangles(awt_display, gc, 0, 0, &rect, 1, YXBanded);
	} else {
	    XSetClipMask(awt_display, gc, None);
	}
	unhand(this)->touched = 0;
    }

    tiny_flush();
    return gc;
}

void 
sun_awt_tiny_TinyGraphics_createFromGraphics(TinyGraphics *this, TinyGraphics *g)
{
    Window win;
    GC gc;

    AWT_LOCK();
    unhand(this)->pDrawable = unhand(g)->pDrawable;
    unhand(this)->touched = 1;
    AWT_UNLOCK();
}

void 
sun_awt_tiny_TinyGraphics_createFromWindow(TinyGraphics *this, TinyWindow *window)
{
    Window win;
    GC gc;

    AWT_LOCK();
    if (window == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "window");
	AWT_UNLOCK();
	return;
    }
    win = (Window)unhand(window)->xid;
    if (win == 0) {
	AWT_UNLOCK();
	return;
    }

    unhand(this)->pDrawable = (long)win;
    unhand(this)->touched = 1;
    AWT_UNLOCK();
}

void
sun_awt_tiny_TinyGraphics_imageCreate(TinyGraphics *this, struct Hsun_awt_image_ImageRepresentation *ir)
{
    extern Drawable image_getIRDrawable(struct Hsun_awt_image_ImageRepresentation *ir);
    Drawable win;
    GC gc;

    AWT_LOCK();
    if (ir == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "ir");
	AWT_UNLOCK();
	return;
    }

    win = image_getIRDrawable(ir);
    if (win == 0) {
	AWT_UNLOCK();
	return;
    }

    unhand(this)->pDrawable = (long)win;
    unhand(this)->touched = 1;
    AWT_UNLOCK();
}

void 
sun_awt_tiny_TinyGraphics_dispose(TinyGraphics *this)
{
    GC gc;

    AWT_LOCK();
    if (this == gc_owner) {
	gc_owner = 0;
    }
    unhand(this)->pDrawable = 0;
    AWT_UNLOCK();
}

void 
sun_awt_tiny_TinyGraphics_drawLine(TinyGraphics *this, long x1, long y1, long x2, long y2)
{
    Drawable win;
    GC gc;

    AWT_LOCK();
    win = (Drawable)unhand(this)->pDrawable;
    gc = tiny_gc(this);
    if ((gc == 0) || (win == 0)) {
	AWT_UNLOCK();
	return;
    }

    XDrawLine(awt_display, win, gc, 
	      TX(this, x1), TY(this, y1), TX(this, x2), TY(this, y2));
    AWT_UNLOCK();
}

void 
sun_awt_tiny_TinyGraphics_fillRect(TinyGraphics *this, long x, long y, long w, long h)
{
    Drawable win;
    GC gc;

    AWT_LOCK();
    win = (Drawable)unhand(this)->pDrawable;
    gc = tiny_gc(this);
    if ((gc == 0) || (win == 0)) {
	AWT_UNLOCK();
	return;
    }
    if (w < 0) {
	w = 0;
    }
    if (h < 0) {
	h = 0;
    }

    XFillRectangle(awt_display, win, gc, 
		   TX(this, x), TY(this, y), w, h);
    AWT_UNLOCK();
}

void 
sun_awt_tiny_TinyGraphics_drawString(TinyGraphics *this, struct Hjava_lang_String *str, long x, long y)
{
    Classjava_lang_String *strptr;
    ushort *dataptr;
    int	length;
    Drawable win;
    GC	gc;

    AWT_LOCK();
    win = (Drawable)unhand(this)->pDrawable;
    gc = tiny_gc(this);
    if (str == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if ((win == 0) || (gc == 0)) {
	AWT_UNLOCK();
	return;
    }

    strptr = unhand(str);
    dataptr = unhand(strptr->value)->body + strptr->offset;
    length = javaStringLength(str);

    if (length > 1024) {
	length = 1024;
    }

    XDrawString16(awt_display, win, gc, TX(this, x), TY(this, y), (XChar2b *)dataptr, length);
    AWT_UNLOCK();
}

void
sun_awt_tiny_TinyGraphics_drawChars(TinyGraphics *this, HArrayOfChar *data, long off, long len, long x, long y)
{
    ushort *dataptr;
    Drawable win;
    GC	gc;

    AWT_LOCK();
    win = (Drawable)unhand(this)->pDrawable;
    gc = tiny_gc(this);
    if (data == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if ((win == 0) || (gc == 0)) {
	AWT_UNLOCK();
	return;
    }

    if (off < 0 || len < 0 || off + len > obj_length(data)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	AWT_UNLOCK();
	return;
    }
    dataptr = unhand(data)->body;
    if (len > 1024) {
	len = 1024;
    }
    XDrawString16(awt_display, win, gc, TX(this,x), TY(this,y), 
		  (XChar2b*)dataptr + off, len);
    AWT_UNLOCK();
}

void
sun_awt_tiny_TinyGraphics_drawBytes(TinyGraphics *this, HArrayOfByte *data, long off, long len, long x, long y)
{
    char *dataptr;
    Drawable win;
    GC	gc;

    AWT_LOCK();
    win = (Drawable)unhand(this)->pDrawable;
    gc = tiny_gc(this);
    if (data == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if ((win == 0) || (gc == 0)) {
	AWT_UNLOCK();
	return;
    }

    if (off < 0 || len < 0 || off + len > obj_length(data)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	AWT_UNLOCK();
	return;
    }
    dataptr = unhand(data)->body;
    if (len > 1024) {
	len = 1024;
    }
    XDrawString(awt_display, win, gc, TX(this,x), TY(this,y), dataptr + off, len);
    AWT_UNLOCK();
}

void 
sun_awt_tiny_TinyGraphics_copyArea(TinyGraphics *this, long x, long y, long w, long h, long dx, long dy)
{
    Drawable win;
    GC gc;

    AWT_LOCK();
    win = (Drawable)unhand(this)->pDrawable;
    gc = tiny_gc(this);
    if ((gc == 0) || (win == 0)) {
	AWT_UNLOCK();
	return;
    }
    dx += x;
    dy += y;

    XCopyArea(awt_display, win, win, gc,
	      TX(this,x), TY(this,y), w, h,
	      TX(this,dx), TY(this, dy));
    AWT_UNLOCK();
}


static XPoint *
transformPoints(TinyGraphics *this, HArrayOfInt *xpoints, HArrayOfInt *ypoints,	long npoints)
{
    static XPoint	*points;
    static int		points_length = 0;
    XPoint		*xp;
    ArrayOfInt		*xpoints_array;
    ArrayOfInt		*ypoints_array;
    int			i;

    xpoints_array = unhand(xpoints);
    ypoints_array = unhand(ypoints);
    if (obj_length(ypoints) < npoints || obj_length(xpoints) < npoints) {
	SignalError(0, JAVAPKG "IllegalArgumentException",0);
	return 0;
    }
    if (points_length < npoints) {
	if (points_length != 0) {
	    sysFree((void *)points);
	}
	points = (XPoint *)sysMalloc(sizeof(XPoint) * npoints);
	if (points == 0) {
	    SignalError(0, JAVAPKG "OutOfMemoryError",0);
	    return 0;
	}
	points_length = npoints;
    }
    for (i=0; i < points_length; i++) {
	xp = (points + i);

	xp->x = TX(this, (xpoints_array->body)[i]);
	xp->y = TY(this, (ypoints_array->body)[i]);
    }

    return points;
}

void 
sun_awt_tiny_TinyGraphics_fillPolygon(TinyGraphics * this, HArrayOfInt *xp, HArrayOfInt *yp, long np)
{
    Drawable win;
    GC	gc;
    XPoint *points;

    AWT_LOCK();
    win = (Drawable)unhand(this)->pDrawable;
    gc = tiny_gc(this);
    if ((gc == 0) || (win == 0)) {
	AWT_UNLOCK();
	return;
    }

    if (points = transformPoints(this, xp, yp, np)) {
	XFillPolygon(awt_display, win, gc, points, np, Complex, CoordModeOrigin);
    }
    AWT_UNLOCK();
}

void 
sun_awt_tiny_TinyGraphics_drawArc(TinyGraphics *this, long x, long y, long w, long h, long startAngle, long endAngle)
{
    long	s, e;
    Drawable win;
    GC	gc;

    AWT_LOCK();
    win = (Drawable)unhand(this)->pDrawable;
    gc = tiny_gc(this);
    if ((gc == 0) || (win == 0)) {
	AWT_UNLOCK();
	return;
    }

    if (endAngle >= 360 || endAngle <= -360) {
	s = 0;
	e = 360 * 64;
    } else {
	s = (startAngle % 360) * 64;
	e = endAngle * 64;
    }
    XDrawArc(awt_display, win, gc, 
	     TX(this, x),
	     TY(this, y),
	     w,
	     h, 
	     s, e);
    AWT_UNLOCK();
}

void 
sun_awt_tiny_TinyGraphics_fillArc(TinyGraphics *this, long x, long y, long w, long h, long startAngle, long endAngle)
{
    long	s, e;
    Drawable win;
    GC	gc;

    AWT_LOCK();
    win = (Drawable)unhand(this)->pDrawable;
    gc = tiny_gc(this);
    if ((gc == 0) || (win == 0)) {
	AWT_UNLOCK();
	return;
    }

    if (endAngle >= 360 || endAngle <= -360) {
	s = 0;
	e = 360 * 64;
    } else {
	s = (startAngle % 360) * 64;
	e = endAngle * 64;
    }
    XFillArc(awt_display, win, gc, 
	     TX(this, x),
	     TY(this, y),
	     w,
	     h, 
	     s, e);
    AWT_UNLOCK();
}

void 
sun_awt_image_ImageRepresentation_imageDraw(struct Hsun_awt_image_ImageRepresentation *this,
					    Graphics *g, long x, long y,
					    struct Hjava_awt_Color *c)
{
    TinyGraphics *tg = (TinyGraphics *)g;
    Drawable win;
    GC	gc;
    int xormode;
    unsigned long fgpixel, xorpixel;

    if (tg == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    if (obj_classblock(tg) != FindClass(0, "sun/awt/tiny/TinyGraphics", TRUE)) {
	SignalError(0, JAVAPKG "IllegalArgumentException",0);
 	return;
    }

    AWT_LOCK();
    win = (Drawable)unhand(tg)->pDrawable;
    gc = tiny_gc(tg);
    if ((gc == 0) || (win == 0)) {
	AWT_UNLOCK();
	return;
    }
    xormode = (int) unhand(tg)->xorColor;
    if (xormode) {
	fgpixel = awt_getColor(unhand(tg)->color);
	xorpixel = awt_getColor(unhand(tg)->xorColor);
    }
    if (unhand(tg)->clip) {
	XRectangle clip;
	clip.x = unhand(unhand(tg)->clip)->x;
	clip.y = unhand(unhand(tg)->clip)->y;
	clip.width = unhand(unhand(tg)->clip)->width;
	clip.height = unhand(unhand(tg)->clip)->height;
	awt_imageDraw(win, gc, this,
		      xormode, xorpixel, fgpixel,
		      TX(tg, x), TY(tg, y), c, &clip);
    } else {
	awt_imageDraw(win, gc, this,
		      xormode, xorpixel, fgpixel,
		      TX(tg, x), TY(tg, y), c, 0);
    }
    AWT_UNLOCK();
}

