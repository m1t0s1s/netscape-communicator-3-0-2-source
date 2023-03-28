/*
 * @(#)awt_Graphics.c	1.61 95/12/04 Sami Shaio
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
#include <X11/keysym.h>
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Rectangle.h"

#include "sun_awt_motif_X11Graphics.h"
#include "java_awt_Canvas.h"
#include "sun_awt_motif_MCanvasPeer.h"
#include "sun_awt_motif_MComponentPeer.h"

#ifdef  NETSCAPE
#define UNICODE_FONT_LIST

#ifdef UNICODE_FONT_LIST 
#include "ustring.h"
#endif 
#endif

#include "image.h"

#define max(x, y) (((int)(x) > (int)(y)) ? (x) : (y))
#define min(x, y) (((int)(x) < (int)(y)) ? (x) : (y))

#define INIT_GC0(disp, gdata)\
if (gdata==0 || (gdata->gc == 0 && !awt_init_gc(disp,gdata))) {\
    return;\
}

#define INIT_GC(disp, gdata)\
if (gdata==0 || (gdata->gc == 0 && !awt_init_gc(disp,gdata))) {\
    AWT_UNLOCK();\
    return;\
}

#define INIT_GC2(disp, gdata, rval)\
if (gdata==0 || (gdata->gc == 0 && !awt_init_gc(disp,gdata))) {\
    AWT_UNLOCK();\
    return rval;\
}

#define ABS(n)			(((n) < 0) ? -(n) : (n))
#define OX(this)		(unhand(this)->originX)
#define OY(this)		(unhand(this)->originY)
#define TX(this, x)		((x) + OX(this))
#define TY(this, y)		((y) + OY(this))

int
awt_init_gc(Display *display, struct GraphicsData *gdata)
{
    if (gdata->drawable == 0) {
        if (gdata->win != 0)
	    gdata->drawable = XtWindow(gdata->win);
	if (gdata->drawable == 0) {
	    return 0;
	}
    }

    if ((gdata->gc = XCreateGC(display, gdata->drawable, 0, 0)) != 0) {
	XSetGraphicsExposures(display, gdata->gc, True);
	return 1;
    }

    return 0;
}

static void
awt_drawArc(struct Hsun_awt_motif_X11Graphics *this,
	    struct GraphicsData *gdata,
	    long x, long y, long w, long h,
	    long startAngle, long endAngle,
	    long filled)
{
    long	s, e;

    if (w < 0 || h < 0) {
	return;
    }
    if (gdata == 0) {
	gdata = PDATA(GraphicsData,this);
	INIT_GC0(awt_display, gdata);
    }
    if (endAngle >= 360 || endAngle <= -360) {
	s = 0;
	e = 360 * 64;
    } else {
	s = (startAngle % 360) * 64;
	e = endAngle * 64;
    }
    if (filled == 0) {
	XDrawArc(awt_display,
		 gdata->drawable,
		 gdata->gc,
		 TX(this, x),
		 TY(this, y),
		 w,
		 h,
		 s,
		 e);
    } else {
	XFillArc(awt_display,
		 gdata->drawable,
		 gdata->gc,
		 TX(this, x),
		 TY(this, y),
		 w,
		 h,
		 s,
		 e);
    }
}

void
sun_awt_motif_X11Graphics_createFromComponent(struct Hsun_awt_motif_X11Graphics *this,
				 struct Hsun_awt_motif_MComponentPeer *canvas)
{
    struct GraphicsData	*gdata;
    struct CanvasData *wdata;

    if (this==0 || canvas==0) {
	SignalError(0, JAVAPKG "NullPointerException", "foo");
	return;
    }
    AWT_LOCK();
    gdata = ZALLOC(GraphicsData);
    SET_PDATA(this, gdata);

    if (gdata == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    wdata= PDATA(CanvasData,canvas);
    if (wdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", "bar");
	AWT_UNLOCK();
	return;
    }
    gdata->gc = 0;
    gdata->win = wdata->comp.widget;
    gdata->drawable = 0;
    gdata->clipset = False;
    AWT_UNLOCK();
}

void
sun_awt_motif_X11Graphics_createFromGraphics(struct Hsun_awt_motif_X11Graphics *this,
				 struct Hsun_awt_motif_X11Graphics *g)
{
    struct GraphicsData *odata;
    struct GraphicsData	*gdata;

    AWT_LOCK();
    if (g==0 || (odata = PDATA(GraphicsData, g))==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    gdata = ZALLOC(GraphicsData);
    SET_PDATA(this, gdata);
    if (gdata==0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }

    INIT_GC(awt_display, odata);
    gdata->win = odata->win;
    gdata->drawable = odata->drawable;
    INIT_GC(awt_display, gdata);

    /* copy the gc */
    XCopyGC(awt_display, odata->gc,
	    GCForeground | GCBackground | GCFont | GCFunction,
	    gdata->gc);

    /* Set the clip rect */
    gdata->clipset = odata->clipset;
    if (gdata->clipset) {
	gdata->cliprect = odata->cliprect;
	XSetClipRectangles(awt_display, gdata->gc, 0, 0, &gdata->cliprect, 1, YXBanded);
    }
    gdata->off_screen = odata->off_screen;
    AWT_UNLOCK();
}


void
sun_awt_motif_X11Graphics_imageCreate(struct Hsun_awt_motif_X11Graphics *this,
			 struct Hsun_awt_image_ImageRepresentation *ir)
{
    struct GraphicsData	*gdata;
    IRData *ird;

    AWT_LOCK();
    if (this==0 || ir==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    gdata = ZALLOC(GraphicsData);
    SET_PDATA(this, gdata);

    if (gdata == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	AWT_UNLOCK();
	return;
    }
    ird = image_getIRData(ir);
    if (ird == 0 || ird->pixmap == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    gdata->gc = 0;
    gdata->win = 0;
    gdata->drawable = ird->pixmap;
    gdata->clipset = False;
    gdata->off_screen = True;
    AWT_UNLOCK();
}


void
sun_awt_motif_X11Graphics_pSetFont(struct Hsun_awt_motif_X11Graphics *this,
		      struct Hjava_awt_Font *font)
{
    struct FontData	*fdata;
    struct GraphicsData	*gdata;
    char		*err;

    if (font == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    fdata = awt_GetFontData(font, &err);
    if (fdata == 0) {
	SignalError(0, err, 0);
	AWT_UNLOCK();
	return;
    }
    INIT_GC(awt_display, gdata);
    XSetFont(awt_display, gdata->gc, fdata->xfont->fid);
    AWT_UNLOCK();
}

void
sun_awt_motif_X11Graphics_pSetForeground(struct Hsun_awt_motif_X11Graphics *this,
			    struct Hjava_awt_Color *c)
{
    Pixel p1;
    struct GraphicsData		*gdata;

    if (c == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    INIT_GC(awt_display, gdata);
    p1 = awt_getColor(c);
    gdata->fgpixel = p1;
    if (gdata->xormode) {
	p1 ^= gdata->xorpixel;
    }
    XSetForeground(awt_display, gdata->gc, p1);
    AWT_UNLOCK();
}


void
sun_awt_motif_X11Graphics_dispose(struct Hsun_awt_motif_X11Graphics *this)
{
    struct GraphicsData		*gdata;

    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    if (gdata == 0) {
	AWT_UNLOCK();
	return;
    }
    if (gdata->gc) {
	XFreeGC(awt_display, gdata->gc);
    }
    sysFree(gdata);
    SET_PDATA(this,0);
    AWT_UNLOCK();
}


#ifdef NETSCAPE
#include "nspr/prcpucfg.h"
#endif

#ifndef UNICODE_FONT_LIST
static void
awt_XDrawString16(Display *display, Drawable draw,
				  GC gc, int x, int y,
				  XChar2b *string, int length)
{
#if defined(NETSCAPE) && defined(IS_LITTLE_ENDIAN)
	XChar2b *str = sysMalloc(sizeof(XChar2b) * length);
	if(str)	{
		XChar2b *s, *d, *limit;
		for(s=string, d=str, limit=str+length; d<limit; d++, s++) {
			d->byte1 = s->byte2;
			d->byte2 = s->byte1;
		}
		XDrawString16(display, draw, gc, x, y, str, length);
		sysFree(str);
	}
#else
	XDrawString16(display, draw, gc, x, y, string, length);
#endif
	return;
}
#endif /* !UNICODE_FONT_LIST */

void
sun_awt_motif_X11Graphics_drawString(struct Hsun_awt_motif_X11Graphics *this,
				     Hjava_lang_String *text,
				     long x,
				     long y)
{
    Classjava_lang_String	*tptr;
    struct GraphicsData		*gdata;
    struct FontData		*fdata;
    char			*err;

    if (text==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }
    AWT_LOCK();
    fdata = awt_GetFontData(unhand(this)->font, &err);
    if (fdata == 0) {
	SignalError(0, err, 0);
	AWT_UNLOCK();
	return ;
    }
    tptr = unhand(text);
    gdata = PDATA(GraphicsData,this);
    INIT_GC(awt_display, gdata);
   
#ifdef UNICODE_FONT_LIST
    {    
       XmString xmstr;
       Dimension width;
       Dimension base;
    
       xmstr =  makeXmString( text );

       width =XmStringWidth(fdata->xmfontlist, xmstr);
       base =XmStringBaseline(fdata->xmfontlist, xmstr);
       XmStringDraw(awt_display,
                    gdata->drawable,
                    fdata->xmfontlist,
                    xmstr,
                    gdata->gc,
                    TX(this,x),
                    TY(this,y) - base ,
                    width,
                    XmALIGNMENT_BEGINNING,      /* For now - change for bidi */
                    XmSTRING_DIRECTION_L_TO_R,  /* For now - change for bidi */
                    NULL );
       XmStringFree(xmstr);
       XSetFont(awt_display, gdata->gc, fdata->xfont->fid);
    }
#else
    {
       ushort	*data;
       int	length;
       
       data = unhand(tptr->value)->body + tptr->offset;
       length = javaStringLength(text);
       
       if (length > 1024) {
          length = 1024;
       }
       awt_XDrawString16(awt_display,
                         gdata->drawable,
                         gdata->gc,
                         TX(this,x),
                         TY(this,y),
                         (XChar2b*)data,
                         length);
    }
#endif
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}

long
sun_awt_motif_X11Graphics_drawStringWidth(struct Hsun_awt_motif_X11Graphics *this,
			     Hjava_lang_String *text,
			     long x,
			     long y)
{
    ushort			*data;
    int				length;
    Classjava_lang_String	*tptr;
    struct GraphicsData		*gdata;
    struct FontData		*fdata;
    char			*err;
    long			rval;

    if (text==0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return -1;
    }
    tptr = unhand(text);
    AWT_LOCK();
    fdata = awt_GetFontData(unhand(this)->font, &err);
    if (fdata == 0) {
	SignalError(0, err, 0);
	AWT_UNLOCK();
	return -1;
    }

    gdata = PDATA(GraphicsData,this);
    INIT_GC2(awt_display, gdata,-1);
    
    data = unhand(tptr->value)->body + tptr->offset;
    length = javaStringLength(text);

    if (length > 1024) {
	length = 1024;
    }
    
#ifdef UNICODE_FONT_LIST
	{    
    XmString xmstr;
    Dimension width;
    Dimension base;
    
    xmstr =  makeXmString( text );

    width = XmStringWidth(fdata->xmfontlist , xmstr);
    base =XmStringBaseline(fdata->xmfontlist , xmstr);
    XmStringDraw(awt_display,
    	gdata->drawable,
   		fdata->xmfontlist,
   		xmstr,
   		gdata->gc,
		TX(this,x),
		TY(this,y) - base ,
   		width,
   		XmALIGNMENT_BEGINNING,		/* Use this value for now- change for bidi */
   		XmSTRING_DIRECTION_L_TO_R,	/* Use this value for now- change for bidi */
   		NULL
    );
    rval = (long) width;
    XmStringFree(xmstr);
    XSetFont(awt_display, gdata->gc, fdata->xfont->fid);
   	}
#else

    awt_XDrawString16(awt_display,
		  gdata->drawable,
		  gdata->gc,
		  TX(this,x),
		  TY(this,y),
		  (XChar2b*)data,
		  length);
    rval = XTextWidth16(fdata->xfont, (XChar2b*)data, length);
#endif
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);

    return rval;
}

void
sun_awt_motif_X11Graphics_drawChars(struct Hsun_awt_motif_X11Graphics *this,
		       HArrayOfChar *text,
		       long offset,
		       long length,
		       long x,
		       long y)
{
    ushort			*data;
    struct GraphicsData		*gdata;
    struct FontData		*fdata;
    char			*err;

    if (text == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    if (offset < 0 || length < 0 || offset + length > obj_length(text)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return;
    }
    AWT_LOCK();
    fdata = awt_GetFontData(unhand(this)->font, &err);
    if (fdata == 0) {
	SignalError(0, err, 0);
	AWT_UNLOCK();
	return ;
    }
    gdata = PDATA(GraphicsData,this);
    INIT_GC(awt_display, gdata);
    data = unhand(text)->body + offset;
    if (length > 1024) {
	length = 1024;
    }
#ifdef UNICODE_FONT_LIST
	{    
    XmString xmstr;
    Dimension width;
    Dimension base;
    
    xmstr =  makeXmStringFromChars( data, length );

    width = XmStringWidth(fdata->xmfontlist , xmstr);
    base = XmStringBaseline(fdata->xmfontlist , xmstr);
    XmStringDraw(awt_display,
    	gdata->drawable,
   		fdata->xmfontlist,
   		xmstr,
   		gdata->gc,
		TX(this,x),
		TY(this,y) - base,
   		width,
   		XmALIGNMENT_BEGINNING,		/* Use this value for now- change for bidi */
   		XmSTRING_DIRECTION_L_TO_R,	/* Use this value for now- change for bidi */
   		NULL
    );
    XmStringFree(xmstr);
    XSetFont(awt_display, gdata->gc, fdata->xfont->fid);
   	}
#else
    awt_XDrawString16(awt_display,
		  gdata->drawable,
		  gdata->gc,
		  TX(this,x),
		  TY(this,y),
		  (XChar2b*)data,
		  length);
#endif

    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}

long
sun_awt_motif_X11Graphics_drawCharsWidth(struct Hsun_awt_motif_X11Graphics *this,
			    HArrayOfChar *text,
			    long offset,
			    long length,
			    long x,
			    long y)
{
    ushort			*data;
    struct GraphicsData		*gdata;
    struct FontData		*fdata;
    char			*err;
    long			rval;

    if (text == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return -1;
    }
    AWT_LOCK();
    fdata = awt_GetFontData(unhand(this)->font, &err);
    if (fdata == 0) {
	SignalError(0, err, 0);
	AWT_UNLOCK();
	return -1;
    }

    if (offset < 0 || length < 0 || offset + length > obj_length(text)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	AWT_UNLOCK();
	return -1;
    }
    gdata = PDATA(GraphicsData,this);
    INIT_GC2(awt_display, gdata,-1);
    data = unhand(text)->body + offset;
    if (length > 1024) {
	length = 1024;
    }

#ifdef UNICODE_FONT_LIST
	{    
    XmString xmstr;
    Dimension width;
    Dimension base;
    
    xmstr =  makeXmStringFromChars( data, length );

    width = XmStringWidth(fdata->xmfontlist , xmstr);
    base = XmStringBaseline(fdata->xmfontlist , xmstr);
    XmStringDraw(awt_display,
    	gdata->drawable,
   		fdata->xmfontlist,
   		xmstr,
   		gdata->gc,
		TX(this,x),
		TY(this,y) - base,
   		width,
   		XmALIGNMENT_BEGINNING,		/* Use this value for now- change for bidi */
   		XmSTRING_DIRECTION_L_TO_R,	/* Use this value for now- change for bidi */
   		NULL
    );
    rval = (long) width;
    XmStringFree(xmstr);
    XSetFont(awt_display, gdata->gc, fdata->xfont->fid);
   	}
#else
    awt_XDrawString16(awt_display,
		  gdata->drawable,
		  gdata->gc,
		  TX(this,x),
		  TY(this,y),
		  (XChar2b*)data,
		  length);

	/* xxx this needs to be fixed for LE machines */
    rval =  XTextWidth16(fdata->xfont, (XChar2b*)data, length);
#endif
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);

    return rval;
}

void
sun_awt_motif_X11Graphics_drawBytes(struct Hsun_awt_motif_X11Graphics *this,
		       HArrayOfByte *text,
		       long offset,
		       long length,
		       long x,
		       long y)
{
    char			*data;
    struct GraphicsData		*gdata;

    if (text == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    if (offset < 0 || length < 0 || offset + length > obj_length(text)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return;
    }
    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    INIT_GC(awt_display, gdata);
    if (length > 1024) {
	length = 1024;
    }
    data = unhand(text)->body + offset;
    XDrawString(awt_display,
		gdata->drawable,
		gdata->gc,
		TX(this,x),
		TY(this,y),
		data,
		length);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}

long
sun_awt_motif_X11Graphics_drawBytesWidth(struct Hsun_awt_motif_X11Graphics *this,
			    HArrayOfByte *text,
			    long offset,
			    long length,
			    long x,
			    long y)
{
    char			*data;
    struct GraphicsData		*gdata;
    struct FontData		*fdata;
    char			*err;
    long			rval;

    if (text == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return -1;
    }
    if (offset < 0 || length < 0 || offset + length > obj_length(text)) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return -1;
    }
    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    INIT_GC2(awt_display, gdata,-1);
    fdata = awt_GetFontData(unhand(this)->font, &err);
    if (fdata == 0) {
	SignalError(0, err, 0);
	AWT_UNLOCK();
	return -1;
    }
    if (length > 1024) {
	length = 1024;
    }
    data = unhand(text)->body + offset;
    XDrawString(awt_display,
		gdata->drawable,
		gdata->gc,
		TX(this,x),
		TY(this,y),
		data,
		length);
    rval = XTextWidth(fdata->xfont, data, length);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);

    return rval;
}


void
sun_awt_motif_X11Graphics_drawLine(struct Hsun_awt_motif_X11Graphics *this,
		      long x1, long y1,
		      long x2, long y2)
{
    struct GraphicsData	*gdata;

    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    INIT_GC(awt_display, gdata);
    XDrawLine(awt_display,
	      gdata->drawable,
	      gdata->gc,
	      TX(this,x1), TY(this,y1),
	      TX(this,x2), TY(this,y2));

    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}

void
sun_awt_motif_X11Graphics_clearRect(struct Hsun_awt_motif_X11Graphics *this,
				    long x,
				    long y,
				    long w,
				    long h)
{
    struct GraphicsData	*gdata;

    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    if (gdata == 0) {
	AWT_UNLOCK();
	return;
    }

    INIT_GC(awt_display, gdata);
    if (gdata->clipset) {
	long    cx, cy, cw, ch;

	cx = gdata->cliprect.x - OX(this);
	cy = gdata->cliprect.y - OY(this);
	cw = gdata->cliprect.width;
	ch = gdata->cliprect.height;
	if (x < cx){
	    cw -= (cx - x);
	    x = cx;
	}
	if (y < cy){
	    ch -= (cy - y);
	    y = cy;
	}
	if (x + w > cx + cw) {
	    w = cx + cw - x;
	}
	if (y + h > cy + ch) {
	    h = cy + ch - y;
	}
    }
    if (w <= 0 || h <= 0){
	AWT_UNLOCK();
	return;
    }

    if (gdata->win == 0) {
	/* this is an offscreen graphics object so we can't use */
	/* XClearArea. Instead we fill a rectangle with white since
	   graphics objects don't have a background color. */
	XFillRectangle(awt_display,
		       gdata->drawable,
		       awt_getImageGC(gdata->drawable),
		       TX(this,x), TY(this,y), w, h);
    } else {
	XClearArea(awt_display,
		   gdata->drawable,
		   TX(this,x), TY(this,y), w, h, False);
    }
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}

void
sun_awt_motif_X11Graphics_fillRect(struct Hsun_awt_motif_X11Graphics *this,
		      long x,
		      long y,
		      long w,
		      long h)
{
    struct GraphicsData	*gdata;

    if (w <= 0 || h <= 0) {
	return;
    }
    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    INIT_GC(awt_display, gdata);
    XFillRectangle(awt_display,
		   gdata->drawable,
		   gdata->gc,
		   TX(this,x), TY(this,y), w, h);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}

void
sun_awt_motif_X11Graphics_drawRect(struct Hsun_awt_motif_X11Graphics *this,
		      long x,
		      long y,
		      long w,
		      long h)
{
    struct GraphicsData	*gdata;

    if (w < 0 || h < 0) {
	return;
    }
    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);

    INIT_GC(awt_display,gdata);
    XDrawRectangle(awt_display,
		   gdata->drawable,
		   gdata->gc,
		   TX(this,x), TY(this,y), w, h);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}



void
sun_awt_motif_X11Graphics_drawRoundRect(struct Hsun_awt_motif_X11Graphics *this,
				long x, long y, long w,	long h,
				long arcWidth, long arcHeight)
{
    struct GraphicsData	*gdata;
    long tx, txw, ty, ty1, ty2, tyh, tx1, tx2;

    if (w <= 0 || h <= 0) {
	return;
    }
    arcWidth = ABS(arcWidth);
    arcHeight = ABS(arcHeight);
    if (arcWidth > w) arcWidth = w;
    if (arcHeight > h) arcHeight = h;
    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    INIT_GC(awt_display,gdata);
    tx = TX(this,x);
    txw = TX(this,x+w);
    ty = TY(this,y);
    tyh = TY(this, y+h);

    ty1 = TY(this,y + (arcHeight / 2));
    ty2 = TY(this,y + h - (arcHeight / 2));
    tx1 = TX(this,x + (arcWidth / 2));
    tx2 = TX(this,x + w - (arcWidth / 2));

    awt_drawArc(this, gdata,
		x, y, arcWidth, arcHeight,
		90, 90, 0);
    awt_drawArc(this, gdata,
		x + w - arcWidth, y, arcWidth, arcHeight,
		0, 90, 0);
    awt_drawArc(this, gdata,
		x, y + h - arcHeight, arcWidth, arcHeight,
		180, 90, 0);
    awt_drawArc(this, gdata,
		x + w - arcWidth, y + h - arcHeight, arcWidth, arcHeight,
		270, 90, 0);

    XDrawLine(awt_display,
	      gdata->drawable,
	      gdata->gc,
	      tx1+1, ty, tx2-1, ty);
    XDrawLine(awt_display,
	      gdata->drawable,
	      gdata->gc,
	      txw, ty1+1, txw, ty2-1);

    XDrawLine(awt_display,
	      gdata->drawable,
	      gdata->gc,
	      tx1+1, tyh, tx2-1, tyh);
    XDrawLine(awt_display,
	      gdata->drawable,
	      gdata->gc,
	      tx, ty1+1, tx, ty2-1);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}

void
sun_awt_motif_X11Graphics_fillRoundRect(struct Hsun_awt_motif_X11Graphics *this,
				long x, long y, long w,	long h,
				long arcWidth, long arcHeight)
{
    struct GraphicsData	*gdata;
    long tx, txw, ty, ty1, ty2, tyh, tx1, tx2;

    if (w <= 0 || h <= 0) {
	return;
    }
    arcWidth = ABS(arcWidth);
    arcHeight = ABS(arcHeight);
    if (arcWidth > w) arcWidth = w;
    if (arcHeight > h) arcHeight = h;
    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    INIT_GC(awt_display,gdata);
    tx = TX(this,x);
    txw = TX(this,x+w);
    ty = TY(this,y);
    ty1 = TY(this,y + (arcHeight / 2));
    ty2 = TY(this,y + h - (arcHeight / 2));
    tyh = TY(this, y+h);
    tx1 = TX(this,x + (arcWidth / 2));
    tx2 = TX(this,x + w - (arcWidth / 2));

    awt_drawArc(this, gdata,
		x, y, arcWidth, arcHeight,
		90, 90, 1);
    awt_drawArc(this, gdata,
		x + w - arcWidth, y, arcWidth, arcHeight,
		0, 90, 1);
    awt_drawArc(this, gdata,
		x, y + h - arcHeight, arcWidth, arcHeight,
		180, 90,1);
    awt_drawArc(this, gdata,
		x + w - arcWidth, y + h - arcHeight, arcWidth, arcHeight,
		270, 90, 1);

    XFillRectangle(awt_display,
		   gdata->drawable,
		   gdata->gc,
		   tx1, ty, tx2-tx1, tyh-ty);
    XFillRectangle(awt_display,
		   gdata->drawable,
		   gdata->gc,
		   tx, ty1, tx1-tx, ty2-ty1);
    XFillRectangle(awt_display,
		   gdata->drawable,
		   gdata->gc,
		   tx2, ty1, txw-tx2, ty2-ty1);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}


void
sun_awt_motif_X11Graphics_setPaintMode(struct Hsun_awt_motif_X11Graphics *this)
{
    struct GraphicsData	*gdata;

    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    INIT_GC(awt_display, gdata);
    gdata->xormode = False;
    XSetFunction(awt_display, gdata->gc, GXcopy);
    XSetForeground(awt_display, gdata->gc, gdata->fgpixel);
    AWT_UNLOCK();
}

void
sun_awt_motif_X11Graphics_setXORMode(struct Hsun_awt_motif_X11Graphics *this,
				     struct Hjava_awt_Color *c)
{
    Pixel p1;
    struct GraphicsData	*gdata;

    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    if (c == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    INIT_GC(awt_display, gdata);
    p1 = awt_getColor(c);
    gdata->xorpixel = p1;
    p1 ^= gdata->fgpixel;
    gdata->xormode = True;
    XSetFunction(awt_display, gdata->gc, GXxor);
    XSetForeground(awt_display, gdata->gc, p1);
    AWT_UNLOCK();
}


void
sun_awt_motif_X11Graphics_clipRect(struct Hsun_awt_motif_X11Graphics *this,
		      long x, long y,
		      long w, long h)
{
    struct GraphicsData	*gdata;
    int x1, y1, x2, y2;

    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    INIT_GC(awt_display, gdata);

    /* compute the union */
    x1 = TX(this,x);
    y1 = TY(this,y);
    if (w <= 0 || h <= 0) {
	x2 = x1;
	y2 = y1;
    } else {
	x2 = x1 + w;
	y2 = y1 + h;
    }

    if (gdata->clipset) {
	x1 = max(gdata->cliprect.x, x1);
	y1 = max(gdata->cliprect.y, y1);
	x2 = min(gdata->cliprect.x + gdata->cliprect.width, x2);
	y2 = min(gdata->cliprect.y + gdata->cliprect.height, y2);
	if (x1 > x2) {
	    x2 = x1;
	}
	if (y1 > y2) {
	    y2 = y1;
	}
    }

    gdata->cliprect.x = (Position)x1;
    gdata->cliprect.y = (Position)y1;
    gdata->cliprect.width = (Dimension)(x2 - x1);
    gdata->cliprect.height = (Dimension)(y2 - y1);
    gdata->clipset = True;

    XSetClipRectangles(awt_display,
		       gdata->gc,
		       0, 0,
		       &gdata->cliprect, 1,
		       YXBanded);
    AWT_UNLOCK();
}

struct Hjava_awt_Rectangle *
sun_awt_motif_X11Graphics_getClipRect(struct Hsun_awt_motif_X11Graphics *this)
{
    struct GraphicsData	*gdata = PDATA(GraphicsData,this);
    struct Hjava_awt_Rectangle *clip = 0;

    if (gdata == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    AWT_LOCK();
    if (gdata->clipset) {
	clip = (Hjava_awt_Rectangle *)execute_java_constructor(EE(), "java/awt/Rectangle", 0, "(IIII)",
							       (gdata->cliprect.x - OX(this)),
								(gdata->cliprect.y - OY(this)),
							       gdata->cliprect.width,
							       gdata->cliprect.height);
	if (clip == 0) {
	    SignalError(0, JAVAPKG "OutOfMemoryError",0);
	}
    }
    AWT_UNLOCK();
    return clip;
}


void
sun_awt_motif_X11Graphics_clearClip(struct Hsun_awt_motif_X11Graphics *this)
{
    struct GraphicsData	*gdata;

    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    INIT_GC(awt_display, gdata);
    gdata->clipset = False;
    XSetClipMask(awt_display, gdata->gc, None);
    AWT_UNLOCK();
}


void
sun_awt_motif_X11Graphics_copyArea(struct Hsun_awt_motif_X11Graphics *this,
		      long x, long y,
		      long width, long height,
		      long dx, long dy)
{
   struct GraphicsData	*gdata;

   if (width <= 0 || height <= 0) {
       return;
   }
   AWT_LOCK();
   gdata = PDATA(GraphicsData,this);
   INIT_GC(awt_display, gdata);
   dx += x;
   dy += y;

   XCopyArea(awt_display, gdata->drawable, gdata->drawable, gdata->gc,
	     TX(this,x), TY(this,y), width, height,
	     TX(this,dx), TY(this, dy));
   AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}


static XPoint *
transformPoints(struct Hsun_awt_motif_X11Graphics *this,
		HArrayOfInt *xpoints,
		HArrayOfInt *ypoints,
		long npoints)
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
    for (i=0; i < npoints; i++) {
	xp = (points + i);

	xp->x = TX(this, (xpoints_array->body)[i]);
	xp->y = TY(this, (ypoints_array->body)[i]);
    }

    return points;
}

void
sun_awt_motif_X11Graphics_drawPolygon(struct Hsun_awt_motif_X11Graphics *this,
			 HArrayOfInt *xpoints,
			 HArrayOfInt *ypoints,
			 long npoints)
{
    struct GraphicsData	*gdata;
    XPoint	*points;

    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    if (xpoints == 0 || ypoints == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    INIT_GC(awt_display, gdata);
    points = transformPoints(this, xpoints, ypoints, npoints);
    if (points == 0) {
	AWT_UNLOCK();
	return;
    }

    XDrawLines(awt_display,
	       gdata->drawable,
	       gdata->gc,
	       points,
	       npoints,
	       CoordModeOrigin);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}

void
sun_awt_motif_X11Graphics_fillPolygon(struct Hsun_awt_motif_X11Graphics *this,
			 HArrayOfInt *xpoints,
			 HArrayOfInt *ypoints,
			 long npoints)
{
    struct GraphicsData	*gdata;
    XPoint		*points;

    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    if (xpoints == 0 || ypoints == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    INIT_GC(awt_display, gdata);
    points = transformPoints(this, xpoints, ypoints, npoints);
    if (points == 0) {
	AWT_UNLOCK();
	return;
    }
    XFillPolygon(awt_display,
		 gdata->drawable,
		 gdata->gc,
		 points,
		 npoints,
		 Complex,
		 CoordModeOrigin);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}


void
sun_awt_motif_X11Graphics_drawOval(struct Hsun_awt_motif_X11Graphics *this, long x, long y, long w, long h)
{
    struct GraphicsData	*gdata;
    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    awt_drawArc(this, 0, x, y, w, h, 0, 360, 0);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}


void
sun_awt_motif_X11Graphics_fillOval(struct Hsun_awt_motif_X11Graphics *this, long x, long y, long w, long h)
{
    struct GraphicsData	*gdata;
    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    awt_drawArc(this, 0, x, y, w, h, 0, 360, 1);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}

void
sun_awt_motif_X11Graphics_drawArc(struct Hsun_awt_motif_X11Graphics *this, long x, long y, long w, long h,
		     long startAngle, long endAngle)
{
    struct GraphicsData	*gdata;
    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    awt_drawArc(this, 0, x, y, w, h, startAngle, endAngle, 0);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}

void
sun_awt_motif_X11Graphics_fillArc(struct Hsun_awt_motif_X11Graphics *this, long x, long y, long w, long h,
		     long startAngle, long endAngle)
{
    struct GraphicsData	*gdata;
    AWT_LOCK();
    gdata = PDATA(GraphicsData,this);
    awt_drawArc(this, 0, x, y, w, h, startAngle, endAngle, 1);
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}

void 
sun_awt_image_ImageRepresentation_imageDraw(struct Hsun_awt_image_ImageRepresentation *this,
					    struct Hjava_awt_Graphics *g,
					    long x, long y,
					    struct Hjava_awt_Color *c)
{
    struct Hsun_awt_motif_X11Graphics *gx = (struct Hsun_awt_motif_X11Graphics *)g;
    struct GraphicsData	*gdata;

    if (g == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    if (obj_classblock(gx) != FindClass(0, "sun/awt/motif/X11Graphics", TRUE)) {
	SignalError(0, JAVAPKG "IllegalArgumentException",0);
 	return;
    }

    AWT_LOCK();
    gdata = PDATA(GraphicsData, gx);

    INIT_GC(awt_display, gdata);

    if ((gdata->gc == 0) || (gdata->drawable == 0)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	AWT_UNLOCK();
	return;
    }
    if (gdata->clipset) {
	awt_imageDraw(gdata->drawable, gdata->gc, this,
		      gdata->xormode, gdata->xorpixel, gdata->fgpixel,
		      TX(gx, x), TY(gx, y), c, &gdata->cliprect);
    } else {
	awt_imageDraw(gdata->drawable, gdata->gc, this,
		      gdata->xormode, gdata->xorpixel, gdata->fgpixel,
		      TX(gx, x), TY(gx, y), c, 0);
    }
    AWT_MAYBE_FLUSH_UNLOCK(gdata->off_screen);
}
