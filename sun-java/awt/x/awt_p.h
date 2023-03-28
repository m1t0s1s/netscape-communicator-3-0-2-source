/*
 * @(#)awt_p.h	1.30 95/12/08 Sami Shaio
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

/*
 * Motif-specific data structures for AWT Java objects.
 *
 */
#ifndef _AWT_P_H_
#define _AWT_P_H_

/* turn on to do event filtering */
#define NEW_EVENT_MODEL
/* turn on to only filter keyboard events */
#define KEYBOARD_ONLY_EVENTS

#include <native.h>
#include <monitor.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <Xm/CascadeB.h>
#include <Xm/DrawingA.h>
#include <Xm/FileSB.h>
#include <Xm/BulletinB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleB.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include "timeval.h"
#include "awt.h"
#include "awt_util.h"
#include "sys_api.h"

struct Hjava_awt_Font;
struct Hjava_awt_Color;
struct Hjava_awt_Component;
struct Hsun_awt_ChoiceHandler;

extern Pixel awt_pixel_by_name(Display *dpy, char *color, char *defaultColor);


struct ComponentData {
    Widget	widget;
    int 	repaintPending;
    int		x1, y1, x2, y2;
};

struct MessageDialogData {
    struct ComponentData	comp;
    long			isModal;
};

struct GraphicsData {
    Widget	win;
    Drawable	drawable;
    GC		gc;
    XRectangle	cliprect;
    Pixel	fgpixel;
    Pixel	xorpixel;
    char	clipset;
    char	xormode;
    char	off_screen;
};

struct MenuItemData {
    struct ComponentData	comp;
    int				index;
};

struct MenuData {
    struct ComponentData	comp;
    struct MenuItemData		itemData;
};

struct CanvasData {
    struct ComponentData	comp;
    Widget			shell;
    int				flags;
};

#define W_GRAVITY_INITIALIZED 1
#define W_IS_EMBEDDED 2

struct FrameData {
    struct CanvasData	winData;
    long		isModal;
    long		mappedOnce;
    Widget		mainWindow;
    Widget		contentWindow;
    Widget		menuBar;
    Widget		warningWindow;
    long		top;
    long		bottom;
    long		left;
    long		right;
    Cursor		cursor;
    int			cursorSet;
};

struct ListData {
    struct ComponentData comp;
    Widget		 list;
};

struct TextAreaData {
    struct ComponentData comp;
    Widget 		 txt;
};

struct FileDialogData {
    struct ComponentData comp;
    char	*file;
};

struct FontData {
    XFontStruct	*xfont;
    XmFontList 	xmfontlist;
    long        refcount;
};

struct ChoiceData {
    struct ComponentData comp;
    Widget		 menu;
    Widget		 *items;
    int			 maxitems;
    int			 n_items;
};

struct ImageData {
    Pixmap	xpixmap;
    Pixmap	xmask;
};

extern int awt_init_gc(Display *display, struct GraphicsData *gdata);
extern struct FontData *awt_GetFontData(struct Hjava_awt_Font *font, char **msg);
extern GC awt_getImageGC(Pixmap pixmap);

extern XtAppContext	awt_appContext;
extern Pixel		awt_defaultBg;
extern Pixel		awt_defaultFg;

/* allocated and initialize a structure */
#define ZALLOC(T)	((struct T *)sysCalloc(1, sizeof(struct T)))

#define PDATA(T, x) ((struct T *)(unhand(x)->pData))
#define PEER_PDATA(T, T2, x) ((struct T *)(unhand((struct T2 *)unhand(x)->peer)->pData))
#define SET_PDATA(x, d) (unhand(x)->pData = (int)d)
#define XDISPLAY awt_display;
#define XVISUAL  awt_visInfo;
#define XFOREGROUND awt_defaultFg;
#define XBACKGROUND awt_defaultBg;
#define XCOLORMAP   awt_cmap;

#define FRAME_PEER(frame) ((Hsun_awt_motif_MFramePeer *)unhand(frame)->peer)

extern void awt_MToolkit_modalWait(int (*terminateFn)(void *data), void *data);
extern void awt_output_flush(void);

#ifdef NEW_EVENT_MODEL
extern void awt_addWidget(Widget w, void *peer);
extern void awt_delWidget(Widget w);
#endif

#endif           /* _AWT_P_H_ */
