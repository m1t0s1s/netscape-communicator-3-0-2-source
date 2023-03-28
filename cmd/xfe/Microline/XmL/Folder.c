/*
(c) Copyright 1994, 1995, Microline Software, Inc.  ALL RIGHTS RESERVED
  
THIS PROGRAM BELONGS TO MICROLINE SOFTWARE.  IT IS CONSIDERED A TRADE
SECRET AND IS NOT TO BE DIVULGED OR USED BY PARTIES WHO HAVE NOT
RECEIVED WRITTEN AUTHORIZATION FROM THE OWNER.

THIS SOFTWARE IS FURNISHED UNDER A LICENSE AND MAY BE COPIED AND USED 
ONLY IN ACCORDANCE WITH THE TERMS OF THAT LICENSE AND WITH THE INCLUSION
OF THE ABOVE COPYRIGHT NOTICE.  THIS SOFTWARE AND DOCUMENTATION, AND ITS 
COPYRIGHTS ARE OWNED BY MICROLINE SOFTWARE AND ARE PROTECTED BY UNITED
STATES COPYRIGHT LAWS AND INTERNATIONAL TREATY PROVISIONS.
 
THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE
AND SHOULD NOT BE CONSTRUED AS A COMMITMENT BY MICROLINE SOFTWARE.

THIS SOFTWARE AND REFERENCE MATERIALS ARE PROVIDED "AS IS" WITHOUT
WARRANTY AS TO THEIR PERFORMANCE, MERCHANTABILITY, FITNESS FOR ANY 
PARTICULAR PURPOSE, OR AGAINST INFRINGEMENT.  MICROLINE SOFTWARE
ASSUMES NO RESPONSIBILITY FOR THE USE OR INABILITY TO USE THIS 
SOFTWARE.

MICROLINE SOFTWARE SHALL NOT BE LIABLE FOR INDIRECT, SPECIAL OR
CONSEQUENTIAL DAMAGES RESULTING FROM THE USE OF THIS PRODUCT. SOME 
STATES DO NOT ALLOW THE EXCLUSION OR LIMITATION OF INCIDENTAL OR
CONSEQUENTIAL DAMAGES, SO THE ABOVE LIMITATIONS MIGHT NOT APPLY TO
YOU.

MICROLINE SOFTWARE SHALL HAVE NO LIABILITY OR RESPONSIBILITY FOR SOFTWARE
ALTERED, MODIFIED, OR CONVERTED BY YOU OR A THIRD PARTY, DAMAGES
RESULTING FROM ACCIDENT, ABUSE OR MISAPPLICATION, OR FOR PROBLEMS DUE
TO THE MALFUNCTION OF YOUR EQUIPMENT OR SOFTWARE NOT SUPPLIED BY
MICROLINE SOFTWARE.
  
U.S. GOVERNMENT RESTRICTED RIGHTS
This Software and documentation are provided with RESTRICTED RIGHTS.
Use, duplication or disclosure by the Government is subject to
restrictions as set forth in subparagraph (c)(1) of the Rights in
Technical Data and Computer Software Clause at DFARS 252.227-7013 or
subparagraphs (c)(1)(ii) and (2) of Commercial Computer Software -
Restricted Rights at 48 CFR 52.227-19, as applicable, supplier is
Microline Software, 41 Sutter St Suite 1374, San Francisco, CA 94104.
*/

#include "FolderP.h"
#include <X11/StringDefs.h>
#include <Xm/DrawnB.h>
#include <Xm/Label.h>
#include <Xm/Form.h>

#include <stdio.h>
#include <stdlib.h>

#if defined(_STDC_) || defined(__cplusplus) || defined(c_plusplus)

/* Create and Destroy */
static void ClassInitialize();
static void Initialize(Widget req, Widget newW, 
	ArgList args, Cardinal *nargs);
static void Destroy(Widget w);

/* Geometry, Drawing, Entry and Picking */
static void Realize(Widget w, XtValueMask *valueMask,
	XSetWindowAttributes *attr);
static void Redisplay(Widget w, XExposeEvent *event, Region region);
static void Layout(XmLFolderWidget f);
static void LayoutTopBottom(XmLFolderWidget f);
static void LayoutLeftRight(XmLFolderWidget f);
static void Resize(Widget w);
static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry *request,
	XtWidgetGeometry *);
static void ChangeManaged(Widget w);
static void BuildTabList(XmLFolderWidget f);
static void ConstraintInitialize(Widget, Widget w,
	ArgList args, Cardinal *nargs);
static void ConstraintDestroy(Widget w);
static void SetActiveTab(XmLFolderWidget f, Widget w, XEvent *event,
	Boolean notify);
static void DrawTabPixmap(XmLFolderWidget f, Widget tab, int active);
static void DrawManagerShadowLeftRight(XmLFolderWidget f, XRectangle *rect);
static void DrawManagerShadowTopBottom(XmLFolderWidget f, XRectangle *rect);
static void DrawTabHighlight(XmLFolderWidget f, Widget w);
static void SetTabPlacement(XmLFolderWidget f, Widget tab);
static void GetTabRect(XmLFolderWidget f, Widget tab, XRectangle *rect,
	int includeShadow);
static void DrawTabShadowArcTopBottom(XmLFolderWidget f, Widget w);
static void DrawTabShadowArcLeftRight(XmLFolderWidget f, Widget w);
static void DrawTabShadowLineTopBottom(XmLFolderWidget f, Widget w);
static void DrawTabShadowLineLeftRight(XmLFolderWidget f, Widget w);
static void DrawTabShadowNoneTopBottom(XmLFolderWidget f, Widget w);
static void DrawTabShadowNoneLeftRight(XmLFolderWidget f, Widget w);
static void SetGCShadow(XmLFolderWidget f, int type);

/* Getting and Setting Values */
static Boolean SetValues(Widget curW, Widget reqW, Widget newW, 
	ArgList args, Cardinal *nargs);
static Boolean ConstraintSetValues(Widget curW, Widget, Widget newW,
	ArgList, Cardinal *); 
static void CopyFontList(XmLFolderWidget f);
static Boolean CvtStringToCornerStyle(Display *dpy, XrmValuePtr args,
	Cardinal *numArgs, XrmValuePtr fromVal, XrmValuePtr toVal,
	XtPointer *data);
static Boolean CvtStringToTabPlacement(Display *dpy, XrmValuePtr args,
	Cardinal *numArgs, XrmValuePtr fromVal, XrmValuePtr toVal,
	XtPointer *data);

/* Utility */
static void GetCoreBackground(Widget w, int, XrmValue *value);
static void GetManagerForeground(Widget w, int, XrmValue *value);

/* Actions, Callbacks and Handlers */
static void Activate(Widget w, XEvent *event, String *, Cardinal *);
static void PrimActivate(Widget w, XtPointer, XtPointer);
static void PrimFocusIn(Widget w, XEvent *event, String *, Cardinal *);
static void PrimFocusOut(Widget w, XEvent *event, String *, Cardinal *);

#else

static void ClassInitialize();
static void Initialize();
static void Destroy();

static void Realize();
static void Redisplay();
static void Layout();
static void LayoutTopBottom();
static void LayoutLeftRight();
static void Resize();
static XtGeometryResult GeometryManager();
static void ChangeManaged();
static void BuildTabList();
static void ConstraintInitialize();
static void ConstraintDestroy();
static void SetActiveTab();
static void DrawTabHighlight();
static void SetTabPlacement();
static void GetTabRect();
static void DrawTabPixmap();
static void DrawManagerShadowLeftRight();
static void DrawManagerShadowTopBottom();
static void DrawTabShadowArcTopBottom();
static void DrawTabShadowArcLeftRight();
static void DrawTabShadowLineTopBottom();
static void DrawTabShadowLineLeftRight();
static void DrawTabShadowNoneTopBottom();
static void DrawTabShadowNoneLeftRight();
static void SetGCShadow();

static Boolean SetValues();
static Boolean ConstraintSetValues();
static void CopyFontList();
static Boolean CvtStringToCornerStyle();
static Boolean CvtStringToTabPlacement();

static void GetCoreBackground();
static void GetManagerForeground();

static void Activate();
static void PrimActivate();
static void PrimFocusIn();
static void PrimFocusOut();

#endif

static XtActionsRec actions[] =
	{
	{ "XmLFolderActivate",      Activate     },
	{ "XmLFolderPrimFocusIn",   PrimFocusIn  },
	{ "XmLFolderPrimFocusOut",  PrimFocusOut },
	};

/* Folder Translations */

static char translations[] =
"<Btn1Down>: XmLFolderActivate()\n\
<EnterWindow>:   ManagerEnter()\n\
<LeaveWindow>:   ManagerLeave()\n\
<FocusOut>:      ManagerFocusOut()\n\
<FocusIn>:       ManagerFocusIn()";

/* Primitive Child Translations */

static char primTranslations[] =
"<FocusIn>: XmLFolderPrimFocusIn() PrimitiveFocusIn()\n\
<FocusOut>: XmLFolderPrimFocusOut() PrimitiveFocusOut()";

static XtResource resources[] =
	{
		/* Folder Resources */
		{
		XmNactivateCallback, XmCCallback,
		XmRCallback, sizeof(XtCallbackList),
		XtOffset(XmLFolderWidget, folder.activateCallback),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNactiveTab, XmCActiveTab,
		XmRInt, sizeof(int),
		XtOffset(XmLFolderWidget, folder.activeTab),
		XmRImmediate, (XtPointer)-1,
		},
		{
		XmNblankBackground, XmCBlankBackground,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLFolderWidget, folder.blankBg),
#ifdef MOTIF11
		XmRCallProc, (XtPointer)GetCoreBackground,
#else
		XmRCallProc, GetCoreBackground,
#endif
		},
		{
		XmNcornerDimension, XmCCornerDimension,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLFolderWidget, folder.cornerDimension),
		XmRImmediate, (XtPointer)3,
		},
		{
		XmNcornerStyle, XmCCornerStyle,
		XmRCornerStyle, sizeof(unsigned char),
		XtOffset(XmLFolderWidget, folder.cornerStyle),
		XmRImmediate, (XtPointer)XmCORNER_ARC,
		},
		{
		XmNfontList, XmCFontList,
		XmRFontList, sizeof(XmFontList),
		XtOffset(XmLFolderWidget, folder.fontList),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNhighlightThickness, XmCHighlightThickness,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLFolderWidget, folder.highlightThickness),
		XmRImmediate, (XtPointer)2,
		},
		{
		XmNinactiveBackground, XmCInactiveBackground,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLFolderWidget, folder.inactiveBg),
#ifdef MOTIF11
		XmRCallProc, (XtPointer)GetCoreBackground,
#else
		XmRCallProc, GetCoreBackground,
#endif
		},
		{
		XmNinactiveForeground, XmCInactiveForeground,
		XmRPixel, sizeof(Pixel),
		XtOffset(XmLFolderWidget, folder.inactiveFg),
#ifdef MOTIF11
		XmRCallProc, (XtPointer)GetManagerForeground,
#else
		XmRCallProc, GetManagerForeground,
#endif
		},
		{
		XmNmarginHeight, XmCMarginHeight,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLFolderWidget, folder.marginHeight),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNmarginWidth, XmCMarginWidth,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLFolderWidget, folder.marginWidth),
		XmRImmediate, (XtPointer)2,
		},
		{
		XmNpixmapMargin, XmCPixmapMargin,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLFolderWidget, folder.pixmapMargin),
		XmRImmediate, (XtPointer)2,
		},
		{
		XmNrotateWhenLeftRight, XmCRotateWhenLeftRight,
		XmRBoolean, sizeof(Boolean),
		XtOffset(XmLFolderWidget, folder.allowRotate),
		XmRImmediate, (XtPointer)True,
		},
		{
		XmNspacing, XmCSpacing,
		XmRDimension, sizeof(Dimension),
		XtOffset(XmLFolderWidget, folder.spacing),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNtabCount, XmCTabCount,
		XmRInt, sizeof(int),
		XtOffset(XmLFolderWidget, folder.tabCount),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNtabPlacement, XmCTabPlacement,
		XmRTabPlacement, sizeof(unsigned char),
		XtOffset(XmLFolderWidget, folder.tabPlacement),
		XmRImmediate, (XtPointer)XmFOLDER_TOP,
		},
		{
		XmNtabWidgetList, XmCReadOnly,
		XmRPointer, sizeof(XtPointer),
		XtOffset(XmLFolderWidget, folder.tabs),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNtabTranslations, XmCTranslations,
		XmRTranslationTable, sizeof(XtTranslations),
		XtOffset(XmLFolderWidget, folder.primTrans),
		XmRString, (XtPointer)primTranslations,
		},
		{
		XmNdebugLevel, XmCDebugLevel,
		XmRChar, sizeof(char),
		XtOffset(XmLFolderWidget, folder.debugLevel),
		XmRImmediate, (XtPointer)0,
		},
		/* Overridden inherited resources */
		{
		XmNshadowThickness, XmCShadowThickness,
		XmRHorizontalDimension, sizeof(Dimension),
		XtOffset(XmLFolderWidget, manager.shadow_thickness),
		XmRImmediate, (XtPointer)2,
		},
    };

static XtResource constraint_resources[] =
	{
		/* Folder Constraint Resources */
		{
		XmNtabFreePixmaps, XmCTabFreePixmaps,
		XmRBoolean, sizeof(Boolean),
		XtOffset(XmLFolderConstraintPtr, folder.freePix),
		XmRImmediate, (XtPointer)False,
		},
		{
		XmNtabInactivePixmap, XmCTabInactivePixmap,
		XmRPixmap, sizeof(Pixmap),
		XtOffset(XmLFolderConstraintPtr, folder.inactPix),
		XmRImmediate, (XtPointer)XmUNSPECIFIED_PIXMAP,
		},
		{
		XmNtabManagedWidget, XmCTabManagedWidget,
		XmRWidget, sizeof(Widget),
		XtOffset(XmLFolderConstraintPtr, folder.managedW),
		XmRImmediate, (XtPointer)0,
		},
		{
		XmNtabPixmap, XmCTabPixmap,
		XmRPixmap, sizeof(Pixmap),
		XtOffset(XmLFolderConstraintPtr, folder.pix),
		XmRImmediate, (XtPointer)XmUNSPECIFIED_PIXMAP,
		},
	};

XmLFolderClassRec xmlFolderClassRec =
    {
        { /* core_class */
        (WidgetClass)&xmManagerClassRec,          /* superclass         */
        "XmLFolder",                              /* class_name         */
        sizeof(XmLFolderRec),                     /* widget_size        */
        ClassInitialize,                          /* class_init         */
        0,                                        /* class_part_init    */
        FALSE,                                    /* class_inited       */
        (XtInitProc)Initialize,                   /* initialize         */
        0,                                        /* initialize_hook    */
        (XtRealizeProc)Realize,                   /* realize            */
        (XtActionList)actions,                    /* actions            */
        (Cardinal)XtNumber(actions),              /* num_actions        */
        resources,                                /* resources          */
        XtNumber(resources),                      /* num_resources      */
        NULLQUARK,                                /* xrm_class          */
        TRUE,                                     /* compress_motion    */
        XtExposeCompressMaximal,                  /* compress_exposure  */
        TRUE,                                     /* compress_enterleave*/
        TRUE,                                     /* visible_interest   */
        (XtWidgetProc)Destroy,                    /* destroy            */
        (XtWidgetProc)Resize,                     /* resize             */
        (XtExposeProc)Redisplay,                  /* expose             */
        (XtSetValuesFunc)SetValues,               /* set_values         */
        0,                                        /* set_values_hook    */
        XtInheritSetValuesAlmost,                 /* set_values_almost  */
        0,                                        /* get_values_hook    */
        0,                                        /* accept_focus       */
        XtVersion,                                /* version            */
        0,                                        /* callback_private   */
        translations,                             /* tm_table           */
        0,                                        /* query_geometry     */
        0,                                        /* display_accelerator*/
        0,                                        /* extension          */
        },
        { /* composite_class */
        (XtGeometryHandler)GeometryManager,       /* geometry_manager   */
        (XtWidgetProc)ChangeManaged,              /* change_managed     */
        XtInheritInsertChild,                     /* insert_child       */
        XtInheritDeleteChild,                     /* delete_child       */
        0,                                        /* extension          */
        },
        { /* constraint_class */
        constraint_resources,	                  /* subresources       */
        XtNumber(constraint_resources),           /* subresource_count  */
        sizeof(XmLFolderConstraintRec),           /* constraint_size    */
        (XtInitProc)ConstraintInitialize,         /* initialize         */
        (XtWidgetProc)ConstraintDestroy,          /* destroy            */
        (XtSetValuesFunc)ConstraintSetValues,     /* set_values         */
        0,                                        /* extension          */
        },
        { /* manager_class */
        XtInheritTranslations,                    /* translations          */
        0,                                        /* syn resources         */
        0,                                        /* num syn_resources     */
        0,                                        /* get_cont_resources    */
        0,                                        /* num_get_cont_resources*/
        XmInheritParentProcess,                   /* parent_process        */
        0,                                        /* extension             */
        },
        { /* folder_class */
        0,                                        /* unused                */
        }
    };

WidgetClass xmlFolderWidgetClass = (WidgetClass)&xmlFolderClassRec;

/*
   Create and Destroy
*/

static void ClassInitialize()
	{
	XtSetTypeConverter(XmRString, XmRCornerStyle, CvtStringToCornerStyle,
		0, 0, XtCacheNone, 0);
	XtSetTypeConverter(XmRString, XmRTabPlacement, CvtStringToTabPlacement,
		0, 0, XtCacheNone, 0);
	}

static void Initialize(req, newW, args, narg) 
Widget req, newW;
ArgList args;
Cardinal *narg;
	{
	Display *dpy;
	Window root;
	XmLFolderWidget f, request;
	XtGCMask mask;
	XGCValues values;
 
	f = (XmLFolderWidget)newW;
	dpy = XtDisplay((Widget)f);
	request = (XmLFolderWidget)req;
	root = RootWindowOfScreen(XtScreen(newW));

	if (f->core.width <= 0) 
		f->core.width = 10;
	if (f->core.height <= 0) 
		f->core.height = 10;

	values.foreground = f->manager.foreground;
	mask = GCForeground;
#if MicrolineDidItAgain
	f->folder.gc = XCreateGC(dpy, root, mask, &values);
#endif /* MicrolineDidItAgain */
	f->folder.tabs = 0;
	f->folder.tabHeight = 0;
	f->folder.tabWidth = 0;
	f->folder.activeW = 0;
	f->folder.focusW = 0;
	f->folder.allowLayout = 1;
	CopyFontList(f);
}

static void Destroy(w)
Widget w;
	{
	XmLFolderWidget f;
	Display *dpy;

	f = (XmLFolderWidget)w;
	dpy = XtDisplay(w);
	if (f->folder.debugLevel)
		fprintf(stderr, "Folder destroy: \n"); 
	if (f->folder.tabs)
		free((char *)f->folder.tabs);
	XFreeGC(dpy, f->folder.gc);
	XmFontListFree(f->folder.fontList);
	}

static void Realize(w, valueMask, attr)
Widget w;
XtValueMask *valueMask;
XSetWindowAttributes *attr;
	{
	XtGCMask mask;
	XGCValues values;
	XmLFolderWidget f;
	Display *dpy;
	WidgetClass superClass;
	XtRealizeProc realize;

        superClass = xmlFolderWidgetClass->core_class.superclass;
        realize = superClass->core_class.realize;
        (*realize)(w, valueMask, attr);

	f = (XmLFolderWidget)w;
	values.foreground = f->manager.foreground;
	dpy = XtDisplay(w);
	mask = GCForeground;
	f->folder.gc = XCreateGC(dpy, XtWindow(w), mask, &values);
	}

/*
   Geometry, Drawing, Entry and Picking
*/

static void Redisplay(w, event, region)
Widget w;
XExposeEvent *event;
Region region;
	{
	Display *dpy;
	Window win;
	XmLFolderWidget f;
	XmLFolderConstraintRec *fc;
	XRectangle eRect, rRect;
	XSegment *topSeg, *botSeg;
	int tCount, bCount;
	Widget tab;
	int i, st, ht, x, y;

	f = (XmLFolderWidget)w;
	if (!XtIsRealized(w))
		return;
	if (!f->core.visible)
		return;
	dpy = XtDisplay(f);
	win = XtWindow(f);
	st = f->manager.shadow_thickness;
	ht = f->folder.highlightThickness;

	if (event)
		{
		eRect.x = event->x;
		eRect.y = event->y;
		eRect.width = event->width;
		eRect.height = event->height;
		if (f->folder.debugLevel)
			fprintf(stderr, "XmLFolder: Redisplay x %d y %d w %d h %d\n",
				event->x, event->y, event->width, event->height);
		}
	else
		{
		eRect.x = 0;
		eRect.y = 0;
		eRect.width = f->core.width;
		eRect.height = f->core.height;
		}
	if (!eRect.width || !eRect.height)
		return;

	if (f->folder.tabPlacement == XmFOLDER_TOP)
		{
		rRect.x = 0;
		rRect.y = f->folder.tabHeight;
		rRect.width = f->core.width;
		rRect.height = f->core.height - f->folder.tabHeight;
		}
	else if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		{
		rRect.x = 0;
		rRect.y = 0;
		rRect.width = f->core.width;
		rRect.height = f->core.height - f->folder.tabHeight;
		}
	if (f->folder.tabPlacement == XmFOLDER_LEFT)
		{
		rRect.x = f->folder.tabWidth;
		rRect.y = 0;
		rRect.width = f->core.width - f->folder.tabWidth;
		rRect.height = f->core.height;
		}
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		{
		rRect.x = 0;
		rRect.y = 0;
		rRect.width = f->core.width - f->folder.tabWidth;
		rRect.height = f->core.height;
		}
	if (XmLRectIntersect(&eRect, &rRect) != XmLRectOutside)
		{
		if (f->folder.tabPlacement == XmFOLDER_TOP ||
			f->folder.tabPlacement == XmFOLDER_BOTTOM)
			DrawManagerShadowTopBottom(f, &rRect);
		else
			DrawManagerShadowLeftRight(f, &rRect);
		}

	if (!f->folder.tabCount)
		return;

	/* Draw tabs */
	for (i = 0; i < f->folder.tabCount; i++)
		{
		tab = f->folder.tabs[i];
		if (!XtIsManaged(tab))
			continue;
		GetTabRect(f, tab, &rRect, 0);
		if (XmLRectIntersect(&eRect, &rRect) == XmLRectOutside)
			continue;
		fc = (XmLFolderConstraintRec *)(tab->core.constraints);

		if (f->folder.debugLevel)
			fprintf(stderr, "XmLFolder: Redisplay tab for widget %d\n", i);
		if (tab == f->folder.activeW)
			{
			XtVaSetValues(tab,
				XmNbackground, f->core.background_pixel,
				XmNtopShadowPixmap, XmUNSPECIFIED_PIXMAP,
				XmNtopShadowColor, f->core.background_pixel,
				XmNbottomShadowPixmap, XmUNSPECIFIED_PIXMAP,
				XmNbottomShadowColor, f->core.background_pixel,
				XmNforeground, f->manager.foreground,
				NULL);
			}
		else
			{
			XSetForeground(dpy, f->folder.gc, f->folder.inactiveBg);
			XFillRectangle(dpy, win, f->folder.gc,
				rRect.x, rRect.y, rRect.width, rRect.height);
			XtVaSetValues(tab,
				XmNbackground, f->folder.inactiveBg,
				XmNtopShadowPixmap, XmUNSPECIFIED_PIXMAP,
				XmNtopShadowColor, f->folder.inactiveBg,
				XmNbottomShadowPixmap, XmUNSPECIFIED_PIXMAP,
				XmNbottomShadowColor, f->folder.inactiveBg,
				XmNforeground, f->folder.inactiveFg,
				NULL);
			}

		if (f->folder.tabPlacement == XmFOLDER_TOP ||
			f->folder.tabPlacement == XmFOLDER_BOTTOM)
			{
			if (f->folder.cornerStyle == XmCORNER_LINE)
				DrawTabShadowLineTopBottom(f, tab);
			else if (f->folder.cornerStyle == XmCORNER_ARC)
				DrawTabShadowArcTopBottom(f, tab);
			else
				DrawTabShadowNoneTopBottom(f, tab);
			}
		else
			{
			if (f->folder.cornerStyle == XmCORNER_LINE)
				DrawTabShadowLineLeftRight(f, tab);
			else if (f->folder.cornerStyle == XmCORNER_ARC)
				DrawTabShadowArcLeftRight(f, tab);
			else
				DrawTabShadowNoneLeftRight(f, tab);
			}

		if (f->folder.focusW == tab)
			DrawTabHighlight(f, tab);

		if (tab == f->folder.activeW &&
			fc->folder.pix != XmUNSPECIFIED_PIXMAP &&
			(fc->folder.maxPixWidth || fc->folder.maxPixHeight))
				DrawTabPixmap(f, tab, 1);
		else if (tab != f->folder.activeW &&
			fc->folder.inactPix != XmUNSPECIFIED_PIXMAP &&
			(fc->folder.maxPixWidth || fc->folder.maxPixHeight))
				DrawTabPixmap(f, tab, 0);

		if (f->folder.spacing)
			{
			XSetForeground(dpy, f->folder.gc, f->folder.blankBg);
			if (f->folder.tabPlacement == XmFOLDER_TOP ||
				f->folder.tabPlacement == XmFOLDER_BOTTOM)
				XFillRectangle(dpy, win, f->folder.gc, rRect.x + rRect.width,
					rRect.y, f->folder.spacing, rRect.height);
			else
				XFillRectangle(dpy, win, f->folder.gc, rRect.x,
					rRect.y + rRect.height, rRect.width, f->folder.spacing);
			}
		}

	/* Draw empty area */
	if (f->folder.tabPlacement == XmFOLDER_TOP ||
		f->folder.tabPlacement == XmFOLDER_BOTTOM)
		{
		rRect.x += rRect.width + f->folder.spacing;
		if ((int)f->core.width > rRect.x)
			{
			if (f->folder.tabPlacement == XmFOLDER_TOP)
				rRect.y = 0;
			else
				rRect.y = f->core.height - f->folder.tabHeight;
			rRect.width = f->core.width - rRect.x;
			rRect.height = f->folder.tabHeight;
			XSetForeground(dpy, f->folder.gc, f->folder.blankBg);
			XFillRectangle(dpy, win, f->folder.gc,
				rRect.x, rRect.y, rRect.width, rRect.height);
			}
		}
	else
		{
		rRect.y += rRect.height + f->folder.spacing;
		if ((int)f->core.height > rRect.y)
			{
			if (f->folder.tabPlacement == XmFOLDER_LEFT)
				rRect.x = 0;
			else
				rRect.x = f->core.width - f->folder.tabWidth;
			rRect.width = f->folder.tabWidth;
			rRect.height = f->core.height - rRect.y;
			XSetForeground(dpy, f->folder.gc, f->folder.blankBg);
			XFillRectangle(dpy, win, f->folder.gc,
				rRect.x, rRect.y, rRect.width, rRect.height);
			}
		}
	}

static void Layout(f)
XmLFolderWidget f;
	{
	Window win;

	if (!f->folder.allowLayout)
		return;
	f->folder.allowLayout = 0;
	if (f->folder.tabPlacement == XmFOLDER_LEFT ||
		f->folder.tabPlacement == XmFOLDER_RIGHT)
		LayoutLeftRight(f);
	else
		LayoutTopBottom(f);
	if (XtIsRealized((Widget)f) && f->core.visible)
		XClearArea(XtDisplay(f), XtWindow(f), 0, 0, 0, 0, True);
	f->folder.allowLayout = 1;
	}

static void LayoutTopBottom(f)
XmLFolderWidget f;
	{
	Display *dpy;
	Window pixRoot;
	int i, x, y;
	WidgetList children;
	Widget tab, child;
	XmLFolderConstraintRec *fc;
	XtGeometryResult result;
	unsigned int inactPixHeight, pixHeight;
	unsigned int inactPixWidth, pixWidth;
	unsigned int pixBW, pixDepth;
	Dimension height, minHeight;
	Dimension width, minWidth;
	Dimension co;
	int st, ht;
	Boolean map, isManaged;

	dpy = XtDisplay(f);
	children = f->composite.children;
	st = f->manager.shadow_thickness;
	ht = f->folder.highlightThickness;

	/* calculate corner offset */
	if (f->folder.cornerStyle == XmCORNER_LINE)
		co = (Dimension)((double)f->folder.cornerDimension * .5 + .99);
	else if (f->folder.cornerStyle == XmCORNER_ARC)
		co = (Dimension)((double)f->folder.cornerDimension * .3 + .99);
	else
		co = 0;

	/* calculate tab bar height and pixmap dimensions */
	minHeight = st * 2;
	for (i = 0; i < f->folder.tabCount; i++)
		{
		tab = f->folder.tabs[i];
		if (!XtIsManaged(tab))
			continue;
		fc = (XmLFolderConstraintRec *)(tab->core.constraints);

		/* check for maximum tab widget height */
		height = co + st + tab->core.height + tab->core.border_width * 2 +
			f->folder.marginHeight * 2 + ht * 2;
		if (height > minHeight)
			minHeight = height;

		/* check pixmap dimensions/maximum pixmap height */
		pixWidth = 0;
		pixHeight = 0;
		fc->folder.pixWidth = 0;
		fc->folder.pixHeight = 0;
		if (fc->folder.pix != XmUNSPECIFIED_PIXMAP)
			{
			XGetGeometry(dpy, fc->folder.pix, &pixRoot,
				&x, &y, &pixWidth, &pixHeight, &pixBW, &pixDepth);
			fc->folder.pixWidth = pixWidth;
			fc->folder.pixHeight = pixHeight;
			pixHeight += co + st + f->folder.marginHeight * 2;
			}
		inactPixWidth = 0;
		inactPixHeight = 0;
		fc->folder.inactPixWidth = 0;
		fc->folder.inactPixHeight = 0;
		if (fc->folder.inactPix != XmUNSPECIFIED_PIXMAP)
			{
			XGetGeometry(dpy, fc->folder.inactPix, &pixRoot, &x, &y,
				&inactPixWidth, &inactPixHeight, &pixBW, &pixDepth);
			fc->folder.inactPixWidth = inactPixWidth;
			fc->folder.inactPixHeight = inactPixHeight;
			inactPixHeight += co + st + f->folder.marginHeight * 2;
			}

		if (pixHeight && pixHeight + ht * 2 > minHeight)
			minHeight = pixHeight + ht * 2;
		if (inactPixHeight && inactPixHeight + ht * 2 > minHeight)
			minHeight = inactPixHeight + ht * 2;

		fc->folder.maxPixHeight = 0;
		fc->folder.maxPixWidth = pixWidth;
		if (inactPixWidth > fc->folder.maxPixWidth)
			fc->folder.maxPixWidth = inactPixWidth;
		}
	f->folder.tabHeight = minHeight;

	/* Calculate min width and height */
	x = 0;
	for (i = 0; i < f->folder.tabCount; i++)
		{
		tab = f->folder.tabs[i];
		if (!XtIsManaged(tab))
			continue;
		fc = (XmLFolderConstraintRec *)(tab->core.constraints);

		if (i)
			x += f->folder.spacing;
		x += st * 2 + co * 2 + f->folder.marginWidth * 2 + ht * 2 +
			XtWidth(tab) + tab->core.border_width * 2;
		if (fc->folder.maxPixWidth)
			x += fc->folder.maxPixWidth + f->folder.pixmapMargin;
		}
	minWidth = x;
	if ((int)minWidth < st * 2)
		minWidth = st;
	minHeight = f->folder.tabHeight;
	for (i = 0; i < f->composite.num_children; i++)
		{
		child = children[i];
		if (XtIsSubclass(child, xmPrimitiveWidgetClass))
			continue;

		height = XtHeight(child) + f->folder.tabHeight + st * 2;
		if (XtIsWidget(child))
			height += f->core.border_width * 2;
		if (height > minHeight)
			minHeight = height;
		width = XtWidth(child) + st * 2;
		if (XtIsWidget(child))
			width += f->core.border_width * 2;
		if (width > minWidth)
			minWidth = width;
		}

	/* Resize folder if needed */
	if (f->core.width < minWidth || f->core.height < minHeight)
		{
		if (f->core.width > minWidth)
			minWidth = f->core.width;
		if (f->core.height > minHeight)
			minHeight = f->core.height;
		if (minWidth <= 0)
			minWidth = 1;
		if (minHeight <= 0)
			minHeight = 1;
		result = XtMakeResizeRequest((Widget)f, minWidth, minHeight,
			&width, &height);
		if (result == XtGeometryAlmost)
			XtMakeResizeRequest((Widget)f, width, height, NULL, NULL);
		}

	/* Move tab primitives into place */
	x = 0;
	for (i = 0; i < f->folder.tabCount; i++)
		{
		tab = f->folder.tabs[i];
		if (!XtIsManaged(tab))
			continue;
		fc = (XmLFolderConstraintRec *)(tab->core.constraints);

		if (i)
			x += f->folder.spacing;
		fc->folder.x = x;
		x += st + co + f->folder.marginWidth + ht;
		if (fc->folder.maxPixWidth)
			x += fc->folder.maxPixWidth + f->folder.pixmapMargin;
		if (f->folder.tabPlacement == XmFOLDER_TOP)
			y = f->folder.tabHeight - f->folder.marginHeight - ht -
				XtHeight(tab) - tab->core.border_width * 2;
		else
			y = f->core.height + f->folder.marginHeight + ht -
				f->folder.tabHeight;
		XtMoveWidget(tab, x, y);
		x += XtWidth(tab) + tab->core.border_width * 2 + ht +
			f->folder.marginWidth + co + st;
		fc->folder.width = x - fc->folder.x; 
		}

	/* Move non-primitive widgets into place */
	for (i = 0; i < f->composite.num_children; i++)
		{
		child = children[i];
		if (XtIsSubclass(child, xmPrimitiveWidgetClass))
			continue;
		if ((int)f->core.width - st * 2 < 1 ||
			(int)f->core.height - f->folder.tabHeight - st * 2 < 1)
			continue;

		/* some manager widgets (like XmForm) dont resize correctly
		   if they are unmanaged, so we need to manage them during
		   the resize without displaying them */
		isManaged = True;
		if (!XtIsManaged(child))
			{
			XtVaGetValues(child,
				XmNmappedWhenManaged, &map,
				NULL);
			XtVaSetValues(child,
				XmNmappedWhenManaged, False,
				NULL);
			XtManageChild(child);
			isManaged = False;
		}
		x = st;
		if (f->folder.tabPlacement == XmFOLDER_TOP)
			y = f->folder.tabHeight + st;
		else
			y = st;
		width = f->core.width - st  * 2;
		height = f->core.height - f->folder.tabHeight - st * 2;
		if (child->core.x == x &&
			child->core.y == y &&
			child->core.width == width &&
			child->core.height == height)
			;
		else
			XtConfigureWidget(child, x, y, width, height,
				child->core.border_width);
		if (isManaged == False)
			{
			XtUnmanageChild(child);
			XtVaSetValues(child, XmNmappedWhenManaged, map, NULL);
			}
		}
	}

static void LayoutLeftRight(f)
XmLFolderWidget f;
	{
	Display *dpy;
	Window pixRoot;
	int i, x, y;
	WidgetList children;
	Widget tab, child;
	XmLFolderConstraintRec *fc;
	XtGeometryResult result;
	unsigned int inactPixHeight, pixHeight;
	unsigned int inactPixWidth, pixWidth;
	unsigned int pixBW, pixDepth;
	Dimension height, minHeight;
	Dimension width, minWidth;
	Dimension co;
	int st, ht;
	Boolean map, isManaged;

	dpy = XtDisplay(f);
	children = f->composite.children;
	st = f->manager.shadow_thickness;
	ht = f->folder.highlightThickness;

	/* calculate corner offset */
	if (f->folder.cornerStyle == XmCORNER_LINE)
		co = (Dimension)((double)f->folder.cornerDimension * .5 + .99);
	else if (f->folder.cornerStyle == XmCORNER_ARC)
		co = (Dimension)((double)f->folder.cornerDimension * .3 + .99);
	else
		co = 0;

	/* calculate tab bar height and pixmap dimensions */
	minWidth = st * 2;
	for (i = 0; i < f->folder.tabCount; i++)
		{
		tab = f->folder.tabs[i];
		if (!XtIsManaged(tab))
			continue;
		fc = (XmLFolderConstraintRec *)(tab->core.constraints);

		/* check for maximum tab widget width */
		width = co + st + tab->core.width + tab->core.border_width * 2 +
			f->folder.marginWidth * 2 + ht * 2;
		if (width > minWidth)
			minWidth = width;

		/* check pixmap dimensions/maximum pixmap height */
		pixWidth = 0;
		pixHeight = 0;
		fc->folder.pixWidth = 0;
		fc->folder.pixHeight = 0;
		if (fc->folder.pix != XmUNSPECIFIED_PIXMAP)
			{
			XGetGeometry(dpy, fc->folder.pix, &pixRoot,
				&x, &y, &pixWidth, &pixHeight, &pixBW, &pixDepth);
			fc->folder.pixWidth = pixWidth;
			fc->folder.pixHeight = pixHeight;
			pixWidth += co + st + f->folder.marginWidth * 2;
			}
		inactPixWidth = 0;
		inactPixHeight = 0;
		fc->folder.inactPixWidth = 0;
		fc->folder.inactPixHeight = 0;
		if (fc->folder.inactPix != XmUNSPECIFIED_PIXMAP)
			{
			XGetGeometry(dpy, fc->folder.inactPix, &pixRoot, &x, &y,
				&inactPixWidth, &inactPixHeight, &pixBW, &pixDepth);
			fc->folder.inactPixWidth = inactPixWidth;
			fc->folder.inactPixHeight = inactPixHeight;
			inactPixWidth += co + st + f->folder.marginWidth * 2;
			}

		if (pixWidth && pixWidth + ht * 2 > minWidth)
			minWidth = pixWidth + ht * 2;
		if (inactPixWidth && inactPixWidth + ht * 2 > minWidth)
			minWidth = inactPixWidth + ht * 2;

		fc->folder.maxPixWidth = 0;
		fc->folder.maxPixHeight = pixHeight;
		if (inactPixHeight > fc->folder.maxPixHeight)
			fc->folder.maxPixHeight = inactPixHeight;
		}
	f->folder.tabWidth = minWidth;

	/* Calculate min width and height */
	y = 0;
	for (i = 0; i < f->folder.tabCount; i++)
		{
		tab = f->folder.tabs[i];
		if (!XtIsManaged(tab))
			continue;
		fc = (XmLFolderConstraintRec *)(tab->core.constraints);

		if (i)
			y += f->folder.spacing;
		y += st * 2 + co * 2 + f->folder.marginHeight * 2 + ht * 2 +
			XtHeight(tab) + tab->core.border_width * 2;
		if (fc->folder.maxPixHeight)
			y += fc->folder.maxPixHeight + f->folder.pixmapMargin;
		}
	minHeight = y;
	if ((int)minHeight < st * 2)
		minHeight = st;
	minWidth = f->folder.tabWidth;

	for (i = 0; i < f->composite.num_children; i++)
		{
		child = children[i];
		if (XtIsSubclass(child, xmPrimitiveWidgetClass))
			continue;

		width = XtWidth(child) + f->folder.tabWidth + st * 2;
		if (XtIsWidget(child))
			width += f->core.border_width * 2;
		if (width > minWidth)
			minWidth = width;
		height = XtHeight(child) + st * 2;
		if (XtIsWidget(child))
			height += f->core.border_width * 2;
		if (height > minHeight)
			minHeight = height;
		}

	/* Resize folder if needed */
	if (f->core.width < minWidth || f->core.height < minHeight)
		{
		if (f->core.width > minWidth)
			minWidth = f->core.width;
		if (f->core.height > minHeight)
			minHeight = f->core.height;
		if (minWidth <= 0)
			minWidth = 1;
		if (minHeight <= 0)
			minHeight = 1;
		result = XtMakeResizeRequest((Widget)f, minWidth, minHeight,
			&width, &height);
		if (result == XtGeometryAlmost)
			XtMakeResizeRequest((Widget)f, width, height, NULL, NULL);
		}

	/* Move tab primitives into place */
	y = 0;
	for (i = 0; i < f->folder.tabCount; i++)
		{
		tab = f->folder.tabs[i];
		if (!XtIsManaged(tab))
			continue;
		fc = (XmLFolderConstraintRec *)(tab->core.constraints);

		if (i)
			y += f->folder.spacing;
		fc->folder.y = y;
		y += st + co + f->folder.marginHeight + ht;
		if (fc->folder.maxPixHeight)
			y += fc->folder.maxPixHeight + f->folder.pixmapMargin;
		if (f->folder.tabPlacement == XmFOLDER_LEFT)
			x = f->folder.tabWidth - f->folder.marginWidth - ht -
				XtWidth(tab) - tab->core.border_width * 2;
		else
			x = f->core.width - f->folder.tabWidth +
				f->folder.marginWidth + ht;
		XtMoveWidget(tab, x, y);
		y += XtHeight(tab) + tab->core.border_width * 2 + ht +
			f->folder.marginHeight + co + st;
		fc->folder.height = y - fc->folder.y; 
		}

	/* Move non-primitive widgets into place */
	for (i = 0; i < f->composite.num_children; i++)
		{
		child = children[i];
		if (XtIsSubclass(child, xmPrimitiveWidgetClass))
			continue;

		if ((int)f->core.width - f->folder.tabWidth - st * 2 < 1 ||
			(int)f->core.height - st * 2 < 1)
			continue;
		/* some manager widgets (like XmForm) dont resize correctly
		   if they are unmanaged, so we need to manage them during
		   the resize without displaying them */
		isManaged = True;
		if (!XtIsManaged(child))
			{
			XtVaGetValues(child,
				XmNmappedWhenManaged, &map,
				NULL);
			XtVaSetValues(child,
				XmNmappedWhenManaged, False,
				NULL);
			XtManageChild(child);
			isManaged = False;
		}
		if (f->folder.tabPlacement == XmFOLDER_LEFT)
			x = f->folder.tabWidth + st;
		else
			x = st;
		y = st;
		width = f->core.width - f->folder.tabWidth - st * 2;
		height = f->core.height - st * 2;
		if (child->core.x == x &&
			child->core.y == y &&
			child->core.width == width &&
			child->core.height == height)
			;
		else
			XtConfigureWidget(child, x, y, width, height,
				child->core.border_width);
		if (isManaged == False)
			{
			XtUnmanageChild(child);
			XtVaSetValues(child, XmNmappedWhenManaged, map, NULL);
			}
		}
	}

static void Resize(w)
Widget w;
	{
	XmLFolderWidget f;

	f = (XmLFolderWidget)w;
	Layout(f);
	}

static XtGeometryResult GeometryManager(w, request, allow)
Widget w;
XtWidgetGeometry *request, *allow;
	{
	if (XtIsSubclass(w, xmPrimitiveWidgetClass))
		{
		if (request->request_mode & CWWidth)
			w->core.width = request->width;
		if (request->request_mode & CWHeight)
			w->core.height = request->height;
		if (request->request_mode & CWX)
			w->core.x = request->x;
		if (request->request_mode & CWY)
			w->core.y = request->y;
		if (request->request_mode & CWBorderWidth)
			w->core.border_width = request->border_width;
		Layout((XmLFolderWidget)XtParent(w));
		return XtGeometryYes;
		}
	return XtGeometryNo;
	}

static void ChangeManaged(w)
Widget w;
	{
	XmLFolderWidget f;

	f = (XmLFolderWidget)w;
	if (!f->folder.allowLayout)
		return;
	BuildTabList(f);
	Layout(f);
	_XmNavigChangeManaged(w);
	}

static void BuildTabList(f)
XmLFolderWidget f;
	{
	int activePos, i, nchildren, tabCount;
	WidgetList children;

	children = f->composite.children;
	nchildren = f->composite.num_children;
	if (f->folder.tabs)
		{
		free((char *)f->folder.tabs);
		f->folder.tabs = 0;
		}
	if (nchildren)
		f->folder.tabs = (Widget *)(malloc(sizeof(Widget) * nchildren));
	tabCount = 0;
	activePos = -1;
	for (i = 0; i < f->composite.num_children; i++)
		{
		if (!XtIsSubclass(children[i], xmPrimitiveWidgetClass))
			continue;
		if (children[i] == f->folder.activeW)
			activePos = tabCount;
		f->folder.tabs[tabCount] = children[i];
		tabCount++;
		}
	f->folder.tabCount = tabCount;
	f->folder.activeTab = activePos;
	if (activePos == -1)
		f->folder.activeW = 0;
	}

static void ConstraintInitialize(req, w, args, narg)
Widget req, w;
ArgList args;
Cardinal *narg;
	{
	XmLFolderWidget f;
	XmLFolderConstraintRec *fc;

	if (!XtIsRectObj(w))
		return;
	f = (XmLFolderWidget)XtParent(w);
	if (f->folder.debugLevel)
		fprintf(stderr, "XmLFolder: Constraint Init\n");
	if (XtIsSubclass(w, xmPrimitiveWidgetClass))
		{
		XtOverrideTranslations(w, f->folder.primTrans);
		XtAddCallback(w, XmNactivateCallback, PrimActivate, 0);
		XtVaSetValues(w,
        	XmNhighlightThickness, 0,
			XmNshadowThickness, 1,
			NULL);
		if (XtIsSubclass(w, xmLabelWidgetClass))
			XtVaSetValues(w, XmNfillOnArm, False, NULL);
		fc = (XmLFolderConstraintRec *)(w->core.constraints);
		fc->folder.x = 0;
		fc->folder.y = 0;
		fc->folder.width = 0;
		fc->folder.height = 0;
		fc->folder.maxPixWidth = 0;
		fc->folder.maxPixHeight = 0;
		BuildTabList(f);
		}
	if (XmIsDrawnButton(w))
		SetTabPlacement(f, w);
	}

static void ConstraintDestroy(w)
Widget w;
	{
	XmLFolderWidget f;
	XmLFolderConstraintRec *fc;
	
	if (!XtIsRectObj(w))
		return;
	f = (XmLFolderWidget)XtParent(w);
	if (f->folder.debugLevel)
		fprintf(stderr, "XmLFolder: Constraint Destroy\n");
	if (f->folder.focusW == w)
		f->folder.focusW = 0;
	if (XtIsSubclass(w, xmPrimitiveWidgetClass))
		{
		XtRemoveCallback(w, XmNactivateCallback, PrimActivate, 0);
		fc = (XmLFolderConstraintRec *)(w->core.constraints);
		if (fc->folder.freePix == True)
			{
			if (fc->folder.pix != XmUNSPECIFIED_PIXMAP)
				XFreePixmap(XtDisplay(w), fc->folder.pix);
			if (fc->folder.inactPix != XmUNSPECIFIED_PIXMAP)
				XFreePixmap(XtDisplay(w), fc->folder.inactPix);
			}
		/* We've got problems here. Microline is
		 * sending bug fixes, so we'll yank this
		 * in the meantime. Basically, the children
		 * of this folder are no longer valid when
		 * we get here.
		 */
		/* BuildTabList(f); */
		}
	}

static void DrawTabPixmap(f, tab, active)
XmLFolderWidget f;
Widget tab;
int active;
	{
	Display *dpy;
	Window win;
	int x, y;
	Pixmap pixmap;
	Dimension pixWidth, pixHeight, ht; 
	XmLFolderConstraintRec *fc;

	dpy = XtDisplay(f);
	win = XtWindow(f);
	fc = (XmLFolderConstraintRec *)(tab->core.constraints);
	ht = f->folder.highlightThickness;
	if (active)
		{
		pixWidth = fc->folder.pixWidth;
		pixHeight = fc->folder.pixHeight;
		pixmap = fc->folder.pix;
		}
	else
		{
		pixWidth = fc->folder.inactPixWidth;
		pixHeight = fc->folder.inactPixHeight;
		pixmap = fc->folder.inactPix;
		}
	if (f->folder.tabPlacement == XmFOLDER_TOP)
		{
		x = tab->core.x - pixWidth - ht - f->folder.pixmapMargin;
		y = tab->core.y + tab->core.height - pixHeight; 
		}
	else if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		{
		x = tab->core.x - fc->folder.pixWidth - ht - f->folder.pixmapMargin;
		y = tab->core.y;
		}
	else if (f->folder.tabPlacement == XmFOLDER_LEFT)
		{
		x = tab->core.x + tab->core.width - pixWidth;
		y = tab->core.y - pixHeight - f->folder.pixmapMargin - ht;
		}
	else if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		{
		x = tab->core.x;
		y = tab->core.y - pixHeight - f->folder.pixmapMargin - ht;
		}
	XCopyArea(dpy, pixmap, win, f->folder.gc, 0, 0, pixWidth, pixHeight, x, y);
	}

static void DrawManagerShadowTopBottom(f, rect)
XmLFolderWidget f;
XRectangle *rect;
	{
	Display *dpy;
	Window win;
	XmLFolderConstraintRec *fc;
	XSegment *topSeg, *botSeg;
	int i, bCount, tCount, st, botOff;

	dpy = XtDisplay(f);
	win = XtWindow(f);
	st = f->manager.shadow_thickness;
	if (!st)
		return;
	botOff = f->core.height - f->folder.tabHeight - 1;

	topSeg = (XSegment *)malloc(sizeof(XSegment) * st * 2);
	botSeg = (XSegment *)malloc(sizeof(XSegment) * st * 2);

	/* top shadow */
	fc = 0;
	if (f->folder.activeW)
		fc = (XmLFolderConstraintRec *)(f->folder.activeW->core.constraints);
	tCount = 0;
	if (fc)
		for (i = 0; i < st; i++)
			{
			topSeg[tCount].x1 = rect->x + i;
			topSeg[tCount].y1 = rect->y + i;
			topSeg[tCount].x2 = fc->folder.x + i;
			topSeg[tCount].y2 = rect->y + i;
			if (f->folder.activeTab > 0)
				tCount++;
			topSeg[tCount].x1 = rect->x + fc->folder.x +
				fc->folder.width - i - 1;
			topSeg[tCount].y1 = rect->y + i;
			topSeg[tCount].x2 = rect->x + rect->width - i - 1;
			topSeg[tCount].y2 = rect->y + i;
			tCount++;
			}
	else
		for (i = 0; i < st; i++)
			{
			topSeg[tCount].x1 = rect->x + i;
			topSeg[tCount].y1 = rect->y + i;
			topSeg[tCount].x2 = rect->x + rect->width - i - 1;
			topSeg[tCount].y2 = rect->y + i;
			tCount++;
			}
	if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		for (i = 0 ; i < tCount; i++)
			{
			topSeg[i].y1 = botOff - topSeg[i].y1;
			topSeg[i].y2 = botOff - topSeg[i].y2;
			}
	if (tCount)
		{
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			SetGCShadow(f, 0);
		else	
			SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}

	/* left shadow */
	tCount = 0;
	for (i = 0; i < st; i++)
		{
		topSeg[tCount].x1 = rect->x + i;
		topSeg[tCount].y1 = rect->y + i;
		topSeg[tCount].x2 = rect->x + i;
		topSeg[tCount].y2 = rect->y + rect->height - i - 1;
		tCount++;
		}
	if (tCount)
		{
		SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}

	/* right shadow */
	bCount = 0;
	for (i = 0; i < st; i++)
		{
		botSeg[bCount].x1 = rect->x + rect->width - i - 1;
		botSeg[bCount].y1 = rect->y + i;
		botSeg[bCount].x2 = rect->x + rect->width - i - 1;
		botSeg[bCount].y2 = rect->y + rect->height - i - 1;
		bCount++;
		}
	if (bCount)
		{
		SetGCShadow(f, 0);
		XDrawSegments(dpy, win, f->folder.gc, botSeg, bCount);
		SetGCShadow(f, 2);
		}

	/* bottom shadow */
	bCount = 0;
	for (i = 0; i < st; i++)
		{
		botSeg[bCount].x1 = rect->x + i;
		botSeg[bCount].y1 = rect->y + rect->height - i - 1;
		botSeg[bCount].x2 = rect->x + rect->width - i - 1;
		botSeg[bCount].y2 = rect->y + rect->height - i - 1;
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			{
			botSeg[bCount].y1 = botOff - botSeg[bCount].y1;
			botSeg[bCount].y2 = botOff - botSeg[bCount].y2;
			}
		bCount++;
		}
	if (bCount)
		{
		if (f->folder.tabPlacement == XmFOLDER_TOP)
			SetGCShadow(f, 0);
		else	
			SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, botSeg, bCount);
		SetGCShadow(f, 2);
		}
	free((char *)topSeg);
	free((char *)botSeg);
	}

static void DrawManagerShadowLeftRight(f, rect)
XmLFolderWidget f;
XRectangle *rect;
	{
	Display *dpy;
	Window win;
	XmLFolderConstraintRec *fc;
	XSegment *topSeg, *botSeg;
	int i, bCount, tCount, st, rightOff;

	dpy = XtDisplay(f);
	win = XtWindow(f);
	st = f->manager.shadow_thickness;
	if (!st)
		return;
	rightOff = f->core.width - f->folder.tabWidth - 1;

	topSeg = (XSegment *)malloc(sizeof(XSegment) * st * 2);
	botSeg = (XSegment *)malloc(sizeof(XSegment) * st * 2);

	/* left shadow */
	fc = 0;
	if (f->folder.activeW)
		fc = (XmLFolderConstraintRec *)(f->folder.activeW->core.constraints);
	tCount = 0;
	if (fc)
		for (i = 0; i < st; i++)
			{
			topSeg[tCount].x1 = rect->x + i;
			topSeg[tCount].y1 = rect->y + i;
			topSeg[tCount].x2 = rect->x + i;
			topSeg[tCount].y2 = fc->folder.y + i;
			if (f->folder.activeTab > 0)
				tCount++;
			topSeg[tCount].x1 = rect->x + i;
			topSeg[tCount].y1 = rect->y + fc->folder.y +
				fc->folder.height - i - 1;
			topSeg[tCount].x2 = rect->x + i;
			topSeg[tCount].y2 = rect->y + rect->height - i - 1;
			tCount++;
			}
	else
		for (i = 0; i < st; i++)
			{
			topSeg[tCount].x1 = rect->x + i;
			topSeg[tCount].y1 = rect->y + i;
			topSeg[tCount].x2 = rect->x + i;
			topSeg[tCount].y2 = rect->y + rect->height - i - 1;
			tCount++;
			}
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		for (i = 0 ; i < tCount; i++)
			{
			topSeg[i].x1 = rightOff - topSeg[i].x1;
			topSeg[i].x2 = rightOff - topSeg[i].x2;
			}
	if (tCount)
		{
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			SetGCShadow(f, 0);
		else	
			SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}

	/* top shadow */
	tCount = 0;
	for (i = 0; i < st; i++)
		{
		topSeg[tCount].x1 = rect->x + i;
		topSeg[tCount].y1 = rect->y + i;
		topSeg[tCount].x2 = rect->x + rect->width - i - 1;
		topSeg[tCount].y2 = rect->y + i;
		tCount++;
		}
	if (tCount)
		{
		SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}

	/* bottom shadow */
	bCount = 0;
	for (i = 0; i < st; i++)
		{
		botSeg[bCount].x1 = rect->x + i;
		botSeg[bCount].y1 = rect->y + rect->height - i - 1;
		botSeg[bCount].x2 = rect->x + rect->width - i - 1;
		botSeg[bCount].y2 = rect->y + rect->height - i - 1;
		bCount++;
		}
	if (bCount)
		{
		SetGCShadow(f, 0);
		XDrawSegments(dpy, win, f->folder.gc, botSeg, bCount);
		SetGCShadow(f, 2);
		}

	/* right shadow */
	bCount = 0;
	for (i = 0; i < st; i++)
		{
		botSeg[bCount].x1 = rect->x + rect->width - i - 1;
		botSeg[bCount].y1 = rect->y + i;
		botSeg[bCount].x2 = rect->x + rect->width - i - 1;
		botSeg[bCount].y2 = rect->y + rect->height - i - 1;
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			{
			botSeg[bCount].x1 = rightOff - botSeg[bCount].x1;
			botSeg[bCount].x2 = rightOff - botSeg[bCount].x2;
			}
		bCount++;
		}
	if (bCount)
		{
		if (f->folder.tabPlacement == XmFOLDER_LEFT)
			SetGCShadow(f, 0);
		else	
			SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, botSeg, bCount);
		SetGCShadow(f, 2);
		}
	free((char *)topSeg);
	free((char *)botSeg);
	}

static void DrawTabShadowArcTopBottom(f, w)
XmLFolderWidget f;
Widget w;
	{
	XmLFolderConstraintRec *fc;
	Display *dpy;
	Window win;
	XSegment *topSeg, *botSeg;
	XPoint points[3];
	XArc arc;
	int tCount, bCount;
	int i, st, cd, botOff;

	dpy = XtDisplay(f);
	win = XtWindow(f);
	fc = (XmLFolderConstraintRec *)(w->core.constraints);
	botOff = f->core.height - 1;
	st = f->manager.shadow_thickness;
	if (!st)
		return;
	cd = f->folder.cornerDimension;

	tCount = 0;
	bCount = 0;
	topSeg = (XSegment *)malloc(sizeof(XSegment) * st);
	botSeg = (XSegment *)malloc(sizeof(XSegment) * st);
	for (i = 0; i < st; i++)
		{
		/* left tab shadow */
		topSeg[tCount].x1 = fc->folder.x + i;
		topSeg[tCount].y1 = cd + st;
		topSeg[tCount].x2 = fc->folder.x + i;
		if (w == f->folder.activeW)
			topSeg[tCount].y2 = f->folder.tabHeight + i - 1;
		else
			topSeg[tCount].y2 = f->folder.tabHeight - 1;
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			{
			topSeg[tCount].y1 = botOff - topSeg[tCount].y1;
			topSeg[tCount].y2 = botOff - topSeg[tCount].y2;
			}
		tCount++;

		/* right tab shadow */
		botSeg[bCount].x1 = fc->folder.x + fc->folder.width - i - 1;
		botSeg[bCount].y1 = cd + st;
		botSeg[bCount].x2 = fc->folder.x + fc->folder.width - i - 1;
		if (w == f->folder.activeW)
			botSeg[bCount].y2 = f->folder.tabHeight + i - 1;
		else
			botSeg[bCount].y2 = f->folder.tabHeight - 1;
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			{
			botSeg[bCount].y1 = botOff - botSeg[bCount].y1;
			botSeg[bCount].y2 = botOff - botSeg[bCount].y2;
			}
		bCount++;
		}
	if (tCount)
		{
		SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	if (bCount)
		{
		SetGCShadow(f, 0);
		XDrawSegments(dpy, win, f->folder.gc, botSeg, bCount);
		SetGCShadow(f, 2);
		}
	tCount = 0;
	for (i = 0; i < st; i++)
		{
		/* top tab shadow */
		topSeg[tCount].x1 = fc->folder.x + cd + st;
		topSeg[tCount].y1 = i;
		topSeg[tCount].x2 = fc->folder.x + fc->folder.width - cd - st - 1;
		topSeg[tCount].y2 = i;
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			{
			topSeg[tCount].y1 = botOff - topSeg[tCount].y1;
			topSeg[tCount].y2 = botOff - topSeg[tCount].y2;
			}
		tCount++;
		}
	if (tCount)
		{
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			SetGCShadow(f, 0);
		else
			SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	free((char *)topSeg);
	free((char *)botSeg);

	/* left arc */
	arc.x = fc->folder.x;
	arc.y = 0;
	arc.width = cd * 2 + st * 2;
	arc.height = cd * 2 + st * 2;
	arc.angle1 = 90 * 64;
	arc.angle2 = 90 * 64;
	if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		{
		arc.y = botOff - arc.height;
		arc.angle1 += 90 * 64;
		}
	XSetForeground(dpy, f->folder.gc, f->folder.blankBg);
	if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		XFillRectangle(dpy, win, f->folder.gc, arc.x,
			arc.y + arc.height / 2 + 1,
			arc.width / 2, arc.height / 2);
	else
		XFillRectangle(dpy, win, f->folder.gc, arc.x, arc.y,
			arc.width / 2, arc.height / 2);
	SetGCShadow(f, 1);
	XFillArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	XDrawArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	if (f->folder.tabPlacement == XmFOLDER_TOP)
		{
		points[0].x = arc.x + arc.width / 2;
		points[0].y = arc.y;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x;
		points[2].y = arc.y + arc.height / 2;
		}
	else
		{
		points[0].x = arc.x;
		points[0].y = arc.y + arc.height / 2;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x + arc.width / 2;
		points[2].y = arc.y + arc.height;
		}
	XDrawLines(dpy, win, f->folder.gc, points, 3, CoordModeOrigin);
	SetGCShadow(f, 2);
	if (w == f->folder.activeW)
		XSetForeground(dpy, f->folder.gc, f->core.background_pixel);
	else
		XSetForeground(dpy, f->folder.gc, f->folder.inactiveBg);
	arc.x += st;
	arc.y += st;
	arc.width -= st * 2;
	arc.height -= st * 2;
	XFillArc(dpy, win, f->folder.gc, arc.x, arc.y,
		arc.width, arc.height, arc.angle1, arc.angle2);
	XDrawArc(dpy, win, f->folder.gc,
		arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	if (f->folder.tabPlacement == XmFOLDER_TOP)
		{
		points[0].x = arc.x + arc.width / 2;
		points[0].y = arc.y;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x;
		points[2].y = arc.y + arc.height / 2;
		}
	else
		{
		points[0].x = arc.x;
		points[0].y = arc.y + arc.height / 2;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x + arc.width / 2;
		points[2].y = arc.y + arc.height;
		}
	XDrawLines(dpy, win, f->folder.gc, points, 3,
		CoordModeOrigin);

	/* right arc */
	arc.x = fc->folder.x + fc->folder.width - cd * 2 - st * 2;
	arc.y = 0;
	arc.width = cd * 2 + st * 2;
	arc.height = cd * 2 + st * 2;
	arc.angle1 = 0;
	arc.angle2 = 90 * 64;
	if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		{
		arc.y = botOff - arc.height;
		arc.angle1 += 270 * 64;
		}
	XSetForeground(dpy, f->folder.gc, f->folder.blankBg);
	if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		XFillRectangle(dpy, win, f->folder.gc, arc.x + arc.width / 2,
			arc.y + arc.height / 2 + 1, arc.width / 2, arc.height / 2);
	else
		XFillRectangle(dpy, win, f->folder.gc, arc.x + arc.width / 2,
			arc.y, arc.width / 2, arc.height / 2);
	arc.x -= 1;
	SetGCShadow(f, 0);
	XFillArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	XDrawArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	if (f->folder.tabPlacement == XmFOLDER_TOP)
		{
		points[0].x = arc.x + arc.width / 2;
		points[0].y = arc.y;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x + arc.width;
		points[2].y = arc.y + arc.height / 2;
		}
	else
		{
		points[0].x = arc.x + arc.width / 2;
		points[0].y = arc.y + arc.height;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2 + 1;
		points[2].x = arc.x + arc.width;
		points[2].y = arc.y + arc.height / 2 + 1;
		}
	XDrawLines(dpy, win, f->folder.gc, points, 3, CoordModeOrigin);
	SetGCShadow(f, 2);
	if (w == f->folder.activeW)
		XSetForeground(dpy, f->folder.gc, f->core.background_pixel);
	else
		XSetForeground(dpy, f->folder.gc, f->folder.inactiveBg);
	arc.x += st;
	arc.y += st;
	arc.width -= st * 2;
	arc.height -= st * 2;
	XFillArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	XDrawArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	if (f->folder.tabPlacement == XmFOLDER_TOP)
		{
		points[0].x = arc.x + arc.width / 2;
		points[0].y = arc.y;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x + arc.width;
		points[2].y = arc.y + arc.height / 2;
		}
	else
		{
		points[0].x = arc.x + arc.width / 2;
		points[0].y = arc.y + arc.height;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2 + 1;
		points[2].x = arc.x + arc.width;
		points[2].y = arc.y + arc.height / 2 + 1;
		}
	XDrawLines(dpy, win, f->folder.gc, points, 3, CoordModeOrigin);
	}

static void DrawTabShadowArcLeftRight(f, w)
XmLFolderWidget f;
Widget w;
	{
	XmLFolderConstraintRec *fc;
	Display *dpy;
	Window win;
	XSegment *topSeg, *botSeg;
	XPoint points[3];
	XArc arc;
	int tCount, bCount;
	int i, st, cd, rightOff;

	dpy = XtDisplay(f);
	win = XtWindow(f);
	fc = (XmLFolderConstraintRec *)(w->core.constraints);
	rightOff = f->core.width - 1;
	st = f->manager.shadow_thickness;
	if (!st)
		return;
	cd = f->folder.cornerDimension;

	tCount = 0;
	bCount = 0;
	topSeg = (XSegment *)malloc(sizeof(XSegment) * st);
	botSeg = (XSegment *)malloc(sizeof(XSegment) * st);
	for (i = 0; i < st; i++)
		{
		/* top tab shadow */
		topSeg[tCount].x1 = cd + st;
		topSeg[tCount].y1 = fc->folder.y + i;
		if (w == f->folder.activeW)
			topSeg[tCount].x2 = f->folder.tabWidth + i - 1;
		else
			topSeg[tCount].x2 = f->folder.tabWidth - 1;
		topSeg[tCount].y2 = fc->folder.y + i;
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			{
			topSeg[tCount].x1 = rightOff - topSeg[tCount].x1;
			topSeg[tCount].x2 = rightOff - topSeg[tCount].x2;
			}
		tCount++;

		/* bottom tab shadow */
		botSeg[bCount].x1 = cd + st;
		botSeg[bCount].y1 = fc->folder.y + fc->folder.height - i - 1;
		if (w == f->folder.activeW)
			botSeg[bCount].x2 = f->folder.tabWidth + i - 1;
		else
			botSeg[bCount].x2 = f->folder.tabWidth - 1;
		botSeg[bCount].y2 = fc->folder.y + fc->folder.height - i - 1;
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			{
			botSeg[bCount].x1 = rightOff - botSeg[bCount].x1;
			botSeg[bCount].x2 = rightOff - botSeg[bCount].x2;
			}
		bCount++;
		}
	if (tCount)
		{
		SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	if (bCount)
		{
		SetGCShadow(f, 0);
		XDrawSegments(dpy, win, f->folder.gc, botSeg, bCount);
		SetGCShadow(f, 2);
		}
	tCount = 0;
	for (i = 0; i < st; i++)
		{
		/* left tab shadow */
		topSeg[tCount].x1 = i;
		topSeg[tCount].y1 = fc->folder.y + cd + st;
		topSeg[tCount].x2 = i;
		topSeg[tCount].y2 = fc->folder.y + fc->folder.height - cd - st - 1;
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			{
			topSeg[tCount].x1 = rightOff - topSeg[tCount].x1;
			topSeg[tCount].x2 = rightOff - topSeg[tCount].x2;
			}
		tCount++;
		}
	if (tCount)
		{
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			SetGCShadow(f, 0);
		else
			SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	free((char *)topSeg);
	free((char *)botSeg);

	/* top arc */
	arc.x = 0;
	arc.y = fc->folder.y;
	arc.width = cd * 2 + st * 2;
	arc.height = cd * 2 + st * 2;
	arc.angle1 = 90 * 64;
	arc.angle2 = 90 * 64;
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		{
		arc.x = rightOff - arc.width;
		arc.angle1 = 0;
		}
	XSetForeground(dpy, f->folder.gc, f->folder.blankBg);
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		XFillRectangle(dpy, win, f->folder.gc,
			arc.x + arc.width / 2 + 1, arc.y,
			arc.width / 2, arc.height / 2);
	else
		XFillRectangle(dpy, win, f->folder.gc, arc.x, arc.y,
			arc.width / 2, arc.height / 2);
	SetGCShadow(f, 1);
	XFillArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	XDrawArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	if (f->folder.tabPlacement == XmFOLDER_LEFT)
		{
		points[0].x = arc.x;
		points[0].y = arc.y + arc.height / 2;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x + arc.width / 2;
		points[2].y = arc.y;
		}
	else
		{
		points[0].x = arc.x + arc.width / 2;
		points[0].y = arc.y;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x + arc.width;
		points[2].y = arc.y + arc.height / 2;
		}
	XDrawLines(dpy, win, f->folder.gc, points, 3, CoordModeOrigin);
	SetGCShadow(f, 2);
	if (w == f->folder.activeW)
		XSetForeground(dpy, f->folder.gc, f->core.background_pixel);
	else
		XSetForeground(dpy, f->folder.gc, f->folder.inactiveBg);
	arc.x += st;
	arc.y += st;
	arc.width -= st * 2;
	arc.height -= st * 2;
	XFillArc(dpy, win, f->folder.gc, arc.x, arc.y,
		arc.width, arc.height, arc.angle1, arc.angle2);
	XDrawArc(dpy, win, f->folder.gc,
		arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	if (f->folder.tabPlacement == XmFOLDER_LEFT)
		{
		points[0].x = arc.x;
		points[0].y = arc.y + arc.height / 2;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x + arc.width / 2;
		points[2].y = arc.y;
		}
	else
		{
		points[0].x = arc.x + arc.width / 2;
		points[0].y = arc.y;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x + arc.width;
		points[2].y = arc.y + arc.height / 2;
		}
	XDrawLines(dpy, win, f->folder.gc, points, 3,
		CoordModeOrigin);

	/* bottom arc */
	arc.x = 0;
	arc.y = fc->folder.y + fc->folder.height - cd * 2 - st * 2;
	arc.width = cd * 2 + st * 2;
	arc.height = cd * 2 + st * 2;
	arc.angle1 = 180 * 64;
	arc.angle2 = 90 * 64;
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		{
		arc.x = rightOff - arc.width;
		arc.angle1 = 270 * 64;
		}
	XSetForeground(dpy, f->folder.gc, f->folder.blankBg);
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		XFillRectangle(dpy, win, f->folder.gc,
			arc.x + arc.width / 2 + 1, arc.y + arc.height / 2,
			arc.width / 2, arc.height / 2);
	else
		XFillRectangle(dpy, win, f->folder.gc,
			arc.x, arc.y + arc.height / 2,
			arc.width / 2, arc.height / 2);
	arc.y -= 1;
	SetGCShadow(f, 0);
	XFillArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	XDrawArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	if (f->folder.tabPlacement == XmFOLDER_LEFT)
		{
		points[0].x = arc.x;
		points[0].y = arc.y + arc.height / 2;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x + arc.width / 2;
		points[2].y = arc.y + arc.height;
		}
	else
		{
		points[0].x = arc.x + arc.width;
		points[0].y = arc.y + arc.height / 2 + 1;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2 + 1;
		points[2].x = arc.x + arc.width / 2;
		points[2].y = arc.y + arc.height;
		}
	XDrawLines(dpy, win, f->folder.gc, points, 3, CoordModeOrigin);
	SetGCShadow(f, 2);
	if (w == f->folder.activeW)
		XSetForeground(dpy, f->folder.gc, f->core.background_pixel);
	else
		XSetForeground(dpy, f->folder.gc, f->folder.inactiveBg);
	arc.x += st;
	arc.y += st;
	arc.width -= st * 2;
	arc.height -= st * 2;
	XFillArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	XDrawArc(dpy, win, f->folder.gc, arc.x, arc.y, arc.width, arc.height,
		arc.angle1, arc.angle2);
	if (f->folder.tabPlacement == XmFOLDER_LEFT)
		{
		points[0].x = arc.x;
		points[0].y = arc.y + arc.height / 2;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2;
		points[2].x = arc.x + arc.width / 2;
		points[2].y = arc.y + arc.height;
		}
	else
		{
		points[0].x = arc.x + arc.width;
		points[0].y = arc.y + arc.height / 2 + 1;
		points[1].x = arc.x + arc.width / 2;
		points[1].y = arc.y + arc.height / 2 + 1;
		points[2].x = arc.x + arc.width / 2;
		points[2].y = arc.y + arc.height;
		}
	XDrawLines(dpy, win, f->folder.gc, points, 3, CoordModeOrigin);
	}

static void DrawTabShadowLineTopBottom(f, w)
XmLFolderWidget f;
Widget w;
	{
	XmLFolderConstraintRec *fc;
	Display *dpy;
	Window win;
	XSegment *topSeg, *botSeg;
	XPoint points[4];
	int tCount, bCount, botOff, i, st, cd, y;

	botOff = f->core.height - 1;
	dpy = XtDisplay(f);
	win = XtWindow(f);
	fc = (XmLFolderConstraintRec *)(w->core.constraints);
	st = f->manager.shadow_thickness;
	if (!st)
		return;
	cd = f->folder.cornerDimension;

	tCount = 0;
	bCount = 0;
	topSeg = (XSegment *)malloc(sizeof(XSegment) * st);
	botSeg = (XSegment *)malloc(sizeof(XSegment) * st);
	for (i = 0; i < st; i++)
		{
		/* left tab shadow */
		topSeg[tCount].x1 = fc->folder.x + i;
		topSeg[tCount].y1 = cd + st;
		topSeg[tCount].x2 = fc->folder.x + i;
		if (w == f->folder.activeW)
			topSeg[tCount].y2 = f->folder.tabHeight - 1 + i;
		else
			topSeg[tCount].y2 = f->folder.tabHeight - 1;
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			{
			topSeg[tCount].y1 = botOff - topSeg[tCount].y1;
			topSeg[tCount].y2 = botOff - topSeg[tCount].y2;
			}
		tCount++;
		/* right tab shadow */
		botSeg[bCount].x1 = fc->folder.x + fc->folder.width - i - 1;
		botSeg[bCount].y1 = cd + st;
		botSeg[bCount].x2 = fc->folder.x + fc->folder.width - i - 1;
		if (w == f->folder.activeW)
			botSeg[bCount].y2 = f->folder.tabHeight + i - 1;
		else
			botSeg[bCount].y2 = f->folder.tabHeight - 1;
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			{
			botSeg[bCount].y1 = botOff - botSeg[bCount].y1;
			botSeg[bCount].y2 = botOff - botSeg[bCount].y2;
			}
		bCount++;
		}
	if (tCount)
		{
		SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	if (bCount)
		{
		SetGCShadow(f, 0);
		XDrawSegments(dpy, win, f->folder.gc, botSeg, bCount);
		SetGCShadow(f, 2);
		}
	tCount = 0;
	for (i = 0; i < st; i++)
		{
		/* top tab shadow */
		topSeg[tCount].x1 = fc->folder.x + cd + st;
		topSeg[tCount].y1 = i;
		topSeg[tCount].x2 = fc->folder.x + fc->folder.width - cd - st - 1;
		topSeg[tCount].y2 = i;
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			{
			topSeg[tCount].y1 = botOff - topSeg[tCount].y1;
			topSeg[tCount].y2 = botOff - topSeg[tCount].y2;
			}
		tCount++;
		}
	if (tCount)
		{
		if (f->folder.tabPlacement == XmFOLDER_TOP)
			SetGCShadow(f, 1);
		else
			SetGCShadow(f, 0);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	free((char *)topSeg);
	free((char *)botSeg);

	/* left top line */
	points[0].x = fc->folder.x;
	points[0].y = cd + st - 1;
	points[1].x = fc->folder.x + cd + st - 1;
	points[1].y = 0;
	points[2].x = fc->folder.x + cd + st - 1;
	points[2].y = cd + st - 1;
	if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		{
		points[0].y = botOff - points[0].y;
		points[1].y = botOff - points[1].y;
		points[2].y = botOff - points[2].y;
		}
	XSetForeground(dpy, f->folder.gc, f->folder.blankBg);
	y = 0;
	if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		y = botOff - cd - st + 1;
	XFillRectangle(dpy, win, f->folder.gc, fc->folder.x, y, cd + st, cd + st);
	SetGCShadow(f, 1);
	XFillPolygon(dpy, win, f->folder.gc, points, 3, Nonconvex,
		CoordModeOrigin);
	points[3].x = points[0].x;
	points[3].y = points[0].y;
	XDrawLines(dpy, win, f->folder.gc, points, 4, CoordModeOrigin);
	SetGCShadow(f, 2);
	if (w == f->folder.activeW)
		XSetForeground(dpy, f->folder.gc, f->core.background_pixel);
	else
		XSetForeground(dpy, f->folder.gc, f->folder.inactiveBg);
	points[0].x += st;
	points[1].y += st;
	if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		points[1].y -= st * 2;
	XFillPolygon(dpy, win, f->folder.gc, points, 3,
		Nonconvex, CoordModeOrigin);
	points[3].x = points[0].x;
	points[3].y = points[0].y;
	XDrawLines(dpy, win, f->folder.gc, points, 4,
		CoordModeOrigin);

	/* right top line */
	points[0].x = fc->folder.x + fc->folder.width - 1;
	points[0].y = cd + st - 1;
	points[1].x = fc->folder.x + fc->folder.width - cd - st;
	points[1].y = 0;
	points[2].x = fc->folder.x + fc->folder.width - cd - st;
	points[2].y = cd + st - 1;
	if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		{
		points[0].y = botOff - points[0].y;
		points[1].y = botOff - points[1].y;
		points[2].y = botOff - points[2].y;
		}
	XSetForeground(dpy, f->folder.gc, f->folder.blankBg);
	y = 0;
	if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		y = botOff - cd - st + 1;
	XFillRectangle(dpy, win, f->folder.gc, fc->folder.x + fc->folder.width -
		cd - st, y, cd + st, cd + st);
	if (f->folder.tabPlacement == XmFOLDER_TOP)
		SetGCShadow(f, 1);
	else
		SetGCShadow(f, 0);
	XFillPolygon(dpy, win, f->folder.gc, points, 3, Nonconvex,
		CoordModeOrigin);
	points[3].x = points[0].x;
	points[3].y = points[0].y;
	XDrawLines(dpy, win, f->folder.gc, points, 4, CoordModeOrigin);
	SetGCShadow(f, 2);
	if (w == f->folder.activeW)
		XSetForeground(dpy, f->folder.gc, f->core.background_pixel);
	else
		XSetForeground(dpy, f->folder.gc, f->folder.inactiveBg);
	points[0].x -= st;
	points[1].y += st;
	if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		points[1].y -= st * 2;
	XFillPolygon(dpy, win, f->folder.gc, points, 3, Nonconvex,
		CoordModeOrigin);
	points[3].x = points[0].x;
	points[3].y = points[0].y;
	XDrawLines(dpy, win, f->folder.gc, points, 4, CoordModeOrigin);
	}

static void DrawTabShadowLineLeftRight(f, w)
XmLFolderWidget f;
Widget w;
	{
	XmLFolderConstraintRec *fc;
	Display *dpy;
	Window win;
	XSegment *topSeg, *botSeg;
	XPoint points[4];
	int tCount, bCount, rightOff, i, st, cd, x;

	rightOff = f->core.width - 1;
	dpy = XtDisplay(f);
	win = XtWindow(f);
	fc = (XmLFolderConstraintRec *)(w->core.constraints);
	st = f->manager.shadow_thickness;
	if (!st)
		return;
	cd = f->folder.cornerDimension;

	tCount = 0;
	bCount = 0;
	topSeg = (XSegment *)malloc(sizeof(XSegment) * st);
	botSeg = (XSegment *)malloc(sizeof(XSegment) * st);
	for (i = 0; i < st; i++)
		{
		/* top tab shadow */
		topSeg[tCount].x1 = cd + st;
		topSeg[tCount].y1 = fc->folder.y + i;
		if (w == f->folder.activeW)
			topSeg[tCount].x2 = f->folder.tabWidth - 1 + i;
		else
			topSeg[tCount].x2 = f->folder.tabWidth - 1;
		topSeg[tCount].y2 = fc->folder.y + i;
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			{
			topSeg[tCount].x1 = rightOff - topSeg[tCount].x1;
			topSeg[tCount].x2 = rightOff - topSeg[tCount].x2;
			}
		tCount++;
		/* bottom tab shadow */
		botSeg[bCount].x1 = cd + st;
		botSeg[bCount].y1 = fc->folder.y + fc->folder.height - i - 1;
		if (w == f->folder.activeW)
			botSeg[bCount].x2 = f->folder.tabWidth + i - 1;
		else
			botSeg[bCount].x2 = f->folder.tabWidth - 1;
		botSeg[bCount].y2 = fc->folder.y + fc->folder.height - i - 1;
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			{
			botSeg[bCount].x1 = rightOff - botSeg[bCount].x1;
			botSeg[bCount].x2 = rightOff - botSeg[bCount].x2;
			}
		bCount++;
		}
	if (tCount)
		{
		SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	if (bCount)
		{
		SetGCShadow(f, 0);
		XDrawSegments(dpy, win, f->folder.gc, botSeg, bCount);
		SetGCShadow(f, 2);
		}
	tCount = 0;
	for (i = 0; i < st; i++)
		{
		/* left tab shadow */
		topSeg[tCount].x1 = i;
		topSeg[tCount].y1 = fc->folder.y + cd + st;
		topSeg[tCount].x2 = i;
		topSeg[tCount].y2 = fc->folder.y + fc->folder.height - cd - st - 1;
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			{
			topSeg[tCount].x1 = rightOff - topSeg[tCount].x1;
			topSeg[tCount].x2 = rightOff - topSeg[tCount].x2;
			}
		tCount++;
		}
	if (tCount)
		{
		if (f->folder.tabPlacement == XmFOLDER_LEFT)
			SetGCShadow(f, 1);
		else
			SetGCShadow(f, 0);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	free((char *)topSeg);
	free((char *)botSeg);

	/* top line */
	points[0].x = cd + st - 1;
	points[0].y = fc->folder.y;
	points[1].x = 0;
	points[1].y = fc->folder.y + cd + st - 1;
	points[2].x = cd + st - 1;
	points[2].y = fc->folder.y + cd + st - 1;
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		{
		points[0].x = rightOff - points[0].x;
		points[1].x = rightOff - points[1].x;
		points[2].x = rightOff - points[2].x;
		}
	XSetForeground(dpy, f->folder.gc, f->folder.blankBg);
	x = 0;
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		x = rightOff - cd - st + 1;
	XFillRectangle(dpy, win, f->folder.gc, x, fc->folder.y, cd + st, cd + st);
	SetGCShadow(f, 1);
	XFillPolygon(dpy, win, f->folder.gc, points, 3, Nonconvex,
		CoordModeOrigin);
	points[3].x = points[0].x;
	points[3].y = points[0].y;
	XDrawLines(dpy, win, f->folder.gc, points, 4, CoordModeOrigin);
	SetGCShadow(f, 2);
	if (w == f->folder.activeW)
		XSetForeground(dpy, f->folder.gc, f->core.background_pixel);
	else
		XSetForeground(dpy, f->folder.gc, f->folder.inactiveBg);
	points[0].y += st;
	points[1].x += st;
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		points[1].x -= st * 2;
	XFillPolygon(dpy, win, f->folder.gc, points, 3,
		Nonconvex, CoordModeOrigin);
	points[3].x = points[0].x;
	points[3].y = points[0].y;
	XDrawLines(dpy, win, f->folder.gc, points, 4,
		CoordModeOrigin);

	/* bottom line */
	points[0].x = cd + st - 1;
	points[0].y = fc->folder.y + fc->folder.height - 1;
	points[1].x = 0;
	points[1].y = fc->folder.y + fc->folder.height - cd - st;
	points[2].x = cd + st - 1;
	points[2].y = fc->folder.y + fc->folder.height - cd - st;
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		{
		points[0].x = rightOff - points[0].x;
		points[1].x = rightOff - points[1].x;
		points[2].x = rightOff - points[2].x;
		}
	XSetForeground(dpy, f->folder.gc, f->folder.blankBg);
	x = 0;
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		x = rightOff - cd - st + 1;
	XFillRectangle(dpy, win, f->folder.gc, x, fc->folder.y +
		fc->folder.height - cd - st, cd + st, cd + st);
	SetGCShadow(f, 0);
	XFillPolygon(dpy, win, f->folder.gc, points, 3, Nonconvex,
		CoordModeOrigin);
	points[3].x = points[0].x;
	points[3].y = points[0].y;
	XDrawLines(dpy, win, f->folder.gc, points, 4, CoordModeOrigin);
	SetGCShadow(f, 2);
	if (w == f->folder.activeW)
		XSetForeground(dpy, f->folder.gc, f->core.background_pixel);
	else
		XSetForeground(dpy, f->folder.gc, f->folder.inactiveBg);
	points[0].y -= st;
	points[1].x += st;
	if (f->folder.tabPlacement == XmFOLDER_RIGHT)
		points[1].x -= st * 2;
	XFillPolygon(dpy, win, f->folder.gc, points, 3, Nonconvex,
		CoordModeOrigin);
	points[3].x = points[0].x;
	points[3].y = points[0].y;
	XDrawLines(dpy, win, f->folder.gc, points, 4, CoordModeOrigin);
	}

static void DrawTabShadowNoneTopBottom(f, w)
XmLFolderWidget f;
Widget w;
	{
	XmLFolderConstraintRec *fc;
	Display *dpy;
	Window win;
	XSegment *topSeg, *botSeg;
	int i, st, botOff, tCount, bCount;

	dpy = XtDisplay(f);
	win = XtWindow(f);
	fc = (XmLFolderConstraintRec *)(w->core.constraints);
	botOff = f->core.height - 1;
	st = f->manager.shadow_thickness;
	if (!st)
		return;

	tCount = 0;
	bCount = 0;
	topSeg = (XSegment *)malloc(sizeof(XSegment) * st * 2);
	botSeg = (XSegment *)malloc(sizeof(XSegment) * st * 2);
	for (i = 0; i < st; i++)
		{
		/* left tab shadow */
		topSeg[tCount].x1 = fc->folder.x + i;
		topSeg[tCount].y1 = i;
		topSeg[tCount].x2 = fc->folder.x + i;
		if (w == f->folder.activeW)
			topSeg[tCount].y2 = f->folder.tabHeight - 1 + i;
		else
			topSeg[tCount].y2 = f->folder.tabHeight - 1;
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			{
			topSeg[tCount].y1 = botOff - topSeg[tCount].y1;
			topSeg[tCount].y2 = botOff - topSeg[tCount].y2;
			}
		tCount++;

		/* right tab shadow */
		botSeg[bCount].x1 = fc->folder.x + fc->folder.width - 1 - i;
		botSeg[bCount].y1 = i;
		botSeg[bCount].x2 = fc->folder.x + fc->folder.width - 1 - i;
		if (w == f->folder.activeW)
			botSeg[bCount].y2 = f->folder.tabHeight - 1 + i;
		else
			botSeg[bCount].y2 = f->folder.tabHeight - 1;
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			{
			botSeg[bCount].y1 = botOff - botSeg[bCount].y1;
			botSeg[bCount].y2 = botOff - botSeg[bCount].y2;
			}
		bCount++;
		}
	if (tCount)
		{
		SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	if (bCount)
		{
		SetGCShadow(f, 0);
		XDrawSegments(dpy, win, f->folder.gc, botSeg, bCount);
		SetGCShadow(f, 2);
		}

	tCount = 0;
	for (i = 0; i < st; i++)
		{
		/* top tab shadow */
		topSeg[tCount].x1 = fc->folder.x + i + 1;
		topSeg[tCount].y1 = i;
		topSeg[tCount].x2 = fc->folder.x + fc->folder.width - i - 1;
		topSeg[tCount].y2 = i;
		if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
			{
			topSeg[tCount].y1 = botOff - topSeg[tCount].y1;
			topSeg[tCount].y2 = botOff - topSeg[tCount].y2;
			}
		tCount++;
		}
	if (tCount)
		{
		if (f->folder.tabPlacement == XmFOLDER_TOP)
			SetGCShadow(f, 1);
		else
			SetGCShadow(f, 0);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	free((char *)topSeg);
	free((char *)botSeg);
	}

static void DrawTabShadowNoneLeftRight(f, w)
XmLFolderWidget f;
Widget w;
	{
	XmLFolderConstraintRec *fc;
	Display *dpy;
	Window win;
	XSegment *topSeg, *botSeg;
	int i, st, rightOff, tCount, bCount;

	dpy = XtDisplay(f);
	win = XtWindow(f);
	fc = (XmLFolderConstraintRec *)(w->core.constraints);
	rightOff = f->core.width - 1;
	st = f->manager.shadow_thickness;
	if (!st)
		return;

	tCount = 0;
	bCount = 0;
	topSeg = (XSegment *)malloc(sizeof(XSegment) * st * 2);
	botSeg = (XSegment *)malloc(sizeof(XSegment) * st * 2);
	for (i = 0; i < st; i++)
		{
		/* bottom tab shadow */
		topSeg[tCount].x1 = i;
		topSeg[tCount].y1 = fc->folder.y + i;
		if (w == f->folder.activeW)
			topSeg[tCount].x2 = f->folder.tabWidth + i - 1;
		else
			topSeg[tCount].x2 = f->folder.tabWidth - 1;
		topSeg[tCount].y2 = fc->folder.y + i;
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			{
			topSeg[tCount].x1 = rightOff - topSeg[tCount].x1;
			topSeg[tCount].x2 = rightOff - topSeg[tCount].x2;
			}
		tCount++;

		/* top tab shadow */
		botSeg[bCount].x1 = i;
		botSeg[bCount].y1 = fc->folder.y + fc->folder.height - i - 1;
		if (w == f->folder.activeW)
			botSeg[bCount].x2 = f->folder.tabWidth + i - 1;
		else
			botSeg[bCount].x2 = f->folder.tabWidth - 1;
		botSeg[bCount].y2 = fc->folder.y + fc->folder.height - i - 1;
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			{
			botSeg[bCount].x1 = rightOff - botSeg[bCount].x1;
			botSeg[bCount].x2 = rightOff - botSeg[bCount].x2;
			}
		bCount++;
		}
	if (tCount)
		{
		SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	if (bCount)
		{
		SetGCShadow(f, 0);
		XDrawSegments(dpy, win, f->folder.gc, botSeg, bCount);
		SetGCShadow(f, 2);
		}

	tCount = 0;
	for (i = 0; i < st; i++)
		{
		/* left tab shadow */
		topSeg[tCount].x1 = i;
		topSeg[tCount].y1 = fc->folder.y + i + 1;
		topSeg[tCount].x2 = i;
		topSeg[tCount].y2 = fc->folder.y + fc->folder.height - i - 1;
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			{
			topSeg[tCount].x1 = rightOff - topSeg[tCount].x1;
			topSeg[tCount].x2 = rightOff - topSeg[tCount].x2;
			}
		tCount++;
		}
	if (tCount)
		{
		if (f->folder.tabPlacement == XmFOLDER_RIGHT)
			SetGCShadow(f, 0);
		else
			SetGCShadow(f, 1);
		XDrawSegments(dpy, win, f->folder.gc, topSeg, tCount);
		SetGCShadow(f, 2);
		}
	free((char *)topSeg);
	free((char *)botSeg);
	}

static void SetGCShadow(f, type)
XmLFolderWidget f;
int type;
	{
	Display *dpy;
	XGCValues values;
	XtGCMask mask;

	dpy = XtDisplay(f);
	if (type == 0)
		{
		mask = GCForeground;
		values.foreground = f->manager.bottom_shadow_color;
		if (f->manager.bottom_shadow_pixmap != XmUNSPECIFIED_PIXMAP)
			{
			mask |= GCFillStyle | GCTile;
			values.fill_style = FillTiled;
			values.tile = f->manager.bottom_shadow_pixmap;
			}
		XChangeGC(dpy, f->folder.gc, mask, &values);
		}
	else if (type == 1)
		{
		mask = GCForeground;
		values.foreground = f->manager.top_shadow_color;
		if (f->manager.top_shadow_pixmap != XmUNSPECIFIED_PIXMAP)
			{
			mask |= GCFillStyle | GCTile;
			values.fill_style = FillTiled;
			values.tile = f->manager.top_shadow_pixmap;
			}
		XChangeGC(dpy, f->folder.gc, mask, &values);
		}
	else
		{
		mask = GCFillStyle;
		values.fill_style = FillSolid;
		XChangeGC(dpy, f->folder.gc, mask, &values);
		}
	}

static void DrawTabHighlight(f, w)
XmLFolderWidget f;
Widget w;
	{
	XmLFolderConstraintRec *fc;
	Display *dpy;
	Window win;
	int ht;

	if (!XtIsRealized(w))
		return;
	if (!f->core.visible)
		return;
	dpy = XtDisplay(f);
	win = XtWindow(f);
	fc = (XmLFolderConstraintRec *)(w->core.constraints);
	ht = f->folder.highlightThickness;
	if (f->folder.focusW == w)
		XSetForeground(dpy, f->folder.gc, f->manager.highlight_color);
	else
		{
		if (f->folder.activeW == w)
			XSetForeground(dpy, f->folder.gc, f->core.background_pixel);
		else
			XSetForeground(dpy, f->folder.gc, f->folder.inactiveBg);
		}
	XFillRectangle(dpy, win, f->folder.gc,
		w->core.x - ht, w->core.y - ht,
		XtWidth(w) + w->core.border_width * 2 + ht * 2, 
		XtHeight(w) + w->core.border_width * 2 + ht * 2);
	}

static void SetTabPlacement(f, tab)
XmLFolderWidget f;
Widget tab;
	{
	if (!XmIsDrawnButton(tab))
		return;
	if (f->folder.allowRotate == True &&
		f->folder.tabPlacement == XmFOLDER_LEFT)
		XmLDrawnButtonSetType(tab, XmDRAWNB_STRING, XmDRAWNB_UP);
	else if (f->folder.allowRotate == True &&
		f->folder.tabPlacement == XmFOLDER_RIGHT)
		XmLDrawnButtonSetType(tab, XmDRAWNB_STRING, XmDRAWNB_DOWN);
	else
		XmLDrawnButtonSetType(tab, XmDRAWNB_STRING, XmDRAWNB_RIGHT);
	if (XtIsRealized(tab))
		XClearArea(XtDisplay(tab), XtWindow(tab), 0, 0, 0, 0, True);
	}

static void GetTabRect(f, tab, rect, includeShadow)
XmLFolderWidget f;
Widget tab;
XRectangle *rect;
int includeShadow;
	{
	XmLFolderConstraintRec *fc;
	int st;

	st = f->manager.shadow_thickness;
	fc = (XmLFolderConstraintRec *)(tab->core.constraints);
	if (f->folder.tabPlacement == XmFOLDER_TOP)
		{
		rect->x = fc->folder.x;
		rect->y = 0;
		rect->width = fc->folder.width;
		rect->height = f->folder.tabHeight;
		if (includeShadow)
			rect->height += st;
		}
	else if (f->folder.tabPlacement == XmFOLDER_BOTTOM)
		{
		rect->x = fc->folder.x;
		rect->y = f->core.height - f->folder.tabHeight;
		rect->width = fc->folder.width;
		rect->height = f->folder.tabHeight;
		if (includeShadow)
			{
			rect->y -= st;
			rect->height += st;
			}
		}
	else if (f->folder.tabPlacement == XmFOLDER_LEFT)
		{
		rect->x = 0;
		rect->y = fc->folder.y;
		rect->width = f->folder.tabWidth;
		rect->height = fc->folder.height;
		if (includeShadow)
			rect->width += st;
		}
	else
		{
		rect->x = f->core.width - f->folder.tabWidth;
		rect->y = fc->folder.y;
		rect->width = f->folder.tabWidth;
		rect->height = fc->folder.height;
		if (includeShadow)
			{
			rect->x -= st;
			rect->width += st;
			}
		}
	}

static void SetActiveTab(f, w, event, notify)
XmLFolderWidget f;
Widget w;
XEvent *event;
Boolean notify;
	{
	XmLFolderCallbackStruct cbs;
	XmLFolderConstraintRec *fc;
	int i, pos, allowActivate;
	Display *dpy;
	Window win;
	Widget activeW, child, managedW;
	WidgetList children;
	XRectangle rect;

	if (f->folder.activeW == w)
		return;
	children = f->composite.children;
	dpy = 0;
	if (XtIsRealized((Widget)f) && f->core.visible)
		{
		dpy = XtDisplay(f);
		win = XtWindow(f);
		}
	pos = -1;
	for (i = 0; i < f->folder.tabCount; i++)
		if (w == f->folder.tabs[i])
			pos = i;
	allowActivate = 1;
	if (notify == True)
		{
		if (f->folder.debugLevel)
			fprintf(stderr, "XmLFolder: activated %d\n", pos);
		cbs.reason = XmCR_ACTIVATE;
		cbs.event = event;
		cbs.pos = pos;
		cbs.allowActivate = 1;
		XtCallCallbackList((Widget)f, f->folder.activateCallback, &cbs);
		if (!cbs.allowActivate)
			allowActivate = 0;
		}
	if (allowActivate)
		{
		activeW = f->folder.activeW;
		if (activeW && dpy)
			{
			GetTabRect(f, activeW, &rect, 1);
      		XClearArea(dpy, win, rect.x, rect.y, rect.width,
				rect.height, True);
			}
		f->folder.activeTab = pos;
		f->folder.activeW = w;
		/* The active tab focus. this case applies
		 * when the folder is first mapped, and the
		 * first tab is not the active tab.
		 */
		PrimFocusIn(w, NULL, NULL, NULL);
		}
	if (allowActivate && w)
		{
		f->folder.allowLayout = 0;
		for (i = 0; i < f->composite.num_children; i++)
			{
			child = children[i];
			fc = (XmLFolderConstraintRec *)(child->core.constraints);
			managedW = fc->folder.managedW;
			if (managedW)
				{
				if (w == child && !XtIsManaged(managedW))
					XtManageChild(managedW);
				if (w != child && XtIsManaged(managedW))
					XtUnmanageChild(managedW);
				}
			}
		f->folder.allowLayout = 1;
		}
	if (allowActivate && w && dpy)
		{
		GetTabRect(f, w, &rect, 1);
      	XClearArea(dpy, win, rect.x, rect.y, rect.width,
			rect.height, True);
		XmProcessTraversal(w, XmTRAVERSE_CURRENT);
		}
	}

/*
   Utility
*/

static void GetCoreBackground(w, offset, value)
Widget w;
int offset;
XrmValue *value;
	{
	value->addr = (caddr_t)&w->core.background_pixel;
	}

static void GetManagerForeground(w, offset, value)
Widget w;
int offset;
XrmValue *value;
	{
	XmLFolderWidget f;

	f = (XmLFolderWidget)w;
	value->addr = (caddr_t)&f->manager.foreground;
	}

/*
   Getting and Setting Values
*/

static Boolean SetValues(curW, reqW, newW, args, narg)
Widget curW, reqW, newW;
ArgList args;
Cardinal *narg;
	{
	XmLFolderWidget f;
	XmLFolderWidget cur;
	int i;
	Boolean needsLayout, needsRedisplay;
 
	f = (XmLFolderWidget)newW;
	cur = (XmLFolderWidget)curW;
	needsLayout = False;
	needsRedisplay = False;

#define NE(value) (f->value != cur->value)
	if (NE(folder.tabCount))
		{
		XmLWarning(newW, "SetValues() - cannot set tabCount");
		f->folder.tabCount = cur->folder.tabCount;
		}
	if (NE(folder.activeTab))
		{
		XmLWarning(newW, "SetValues() - cannot set activeTab");
		f->folder.activeTab = cur->folder.activeTab;
		}
	if (NE(folder.tabPlacement) ||
		NE(folder.allowRotate))
		{
		f->folder.allowLayout = 0;
		for (i = 0; i < f->folder.tabCount; i++)
			SetTabPlacement(f, f->folder.tabs[i]);
		f->folder.allowLayout = 1;
		needsLayout = True;
		}
	if (NE(folder.cornerDimension))
		{
		if (f->folder.cornerDimension < 1)
			{
			XmLWarning(newW, "SetValues() - cornerDimension cannot be < 1");
			f->folder.cornerDimension = cur->folder.cornerDimension;
			}
		else
			needsLayout = True;
		}
	if (NE(folder.inactiveBg) ||
		NE(folder.inactiveFg))
		needsRedisplay = True;
	if (NE(folder.cornerDimension) ||
		NE(folder.cornerStyle) ||
		NE(folder.highlightThickness) ||
		NE(folder.marginHeight) ||
		NE(folder.marginWidth) ||
		NE(folder.pixmapMargin) ||
		NE(manager.shadow_thickness) ||
		NE(folder.spacing))
		needsLayout = True;
	if (NE(folder.fontList))
		{
		XmFontListFree(cur->folder.fontList);
		CopyFontList(f);
		}
#undef NE

	if (needsLayout == True)
		Layout(f);
	return needsRedisplay;
	}

static void CopyFontList(f)
XmLFolderWidget f;
	{
	if (!f->folder.fontList)
		f->folder.fontList = XmLFontListCopyDefault((Widget)f);
	else
		f->folder.fontList = XmFontListCopy(f->folder.fontList);
	if (!f->folder.fontList)
		XmLWarning((Widget)f, "- fatal error - font list NULL");
	}

static Boolean ConstraintSetValues(curW, reqW, newW, args, narg)
Widget curW, reqW, newW;
ArgList args;
Cardinal *narg;
	{
	XmLFolderConstraintRec *cons, *curCons;
	XmLFolderWidget f;
	int i, hasLabelChange;

	f = (XmLFolderWidget)XtParent(newW);
	if (!XtIsRectObj(newW))
		return False;
	if (!XtIsSubclass(newW, xmPrimitiveWidgetClass))
		return False;
	hasLabelChange = 0;
	for (i = 0; i < *narg; i++)
		if (args[i].name && !strcmp(args[i].name, XmNlabelString))
			hasLabelChange = 1;
	if (hasLabelChange &&
		(f->folder.tabPlacement == XmFOLDER_LEFT ||
		f->folder.tabPlacement == XmFOLDER_RIGHT))
		{
		f->folder.allowLayout = 0;
		for (i = 0; i < f->folder.tabCount; i++)
			SetTabPlacement(f, f->folder.tabs[i]);
		f->folder.allowLayout = 1;
		}
	curCons = (XmLFolderConstraintRec *)curW->core.constraints;
	cons = (XmLFolderConstraintRec *)newW->core.constraints;
	if (hasLabelChange ||
		curCons->folder.pix != cons->folder.pix ||
		curCons->folder.inactPix != cons->folder.inactPix)
		Layout((XmLFolderWidget)XtParent(curW));
	return False;
	}

static Boolean CvtStringToCornerStyle(dpy, args, narg, fromVal, toVal, data)
Display *dpy;
XrmValuePtr args;
Cardinal *narg;
XrmValuePtr fromVal, toVal;
XtPointer *data;
	{
	char *from;
	int i, num, valid;
	static struct styleStruct
		{ char *name; unsigned char val; } styles[] =
		{
		{ "CORNER_NONE", XmCORNER_NONE },
		{ "CORNER_LINE", XmCORNER_LINE },
		{ "CORNER_ARC", XmCORNER_ARC },
		};

	valid = 0;
	from = (char *)fromVal->addr;
	num = sizeof(styles) / sizeof(struct styleStruct);
	for (i = 0; i < num; i++)
		if (!strcmp(from, styles[i].name))
			{
			valid = 1;
			break;
			}
	if (!valid)
		{
		XtDisplayStringConversionWarning(dpy, from, "XmRCornerStyle");
		toVal->size = 0;
		toVal->addr = 0;
		return False;
		}
	toVal->size = sizeof(unsigned char);
	toVal->addr = (caddr_t)&styles[i].val;
	return True;
	}

static Boolean CvtStringToTabPlacement(dpy, args, narg, fromVal, toVal, data)
Display *dpy;
XrmValuePtr args;
Cardinal *narg;
XrmValuePtr fromVal, toVal;
XtPointer *data;
	{
	char *from;
	int i, num, valid;
	static struct placesStruct
		{ char *name; unsigned char val; } places[] =
		{
		{ "FOLDER_TOP", XmFOLDER_TOP },
		{ "FOLDER_LEFT", XmFOLDER_LEFT },
		{ "FOLDER_BOTTOM", XmFOLDER_BOTTOM },
		{ "FOLDER_RIGHT", XmFOLDER_RIGHT },
		};

	valid = 0;
	from = (char *)fromVal->addr;
	num = sizeof(places) / sizeof(struct placesStruct);
	for (i = 0; i < num; i++)
		if (!strcmp(from, places[i].name))
			{
			valid = 1;
			break;
			}
	if (!valid)
		{
		XtDisplayStringConversionWarning(dpy, from, "XmRTabPlacement");
		toVal->size = 0;
		toVal->addr = 0;
		return False;
		}
	toVal->size = sizeof(unsigned char);
	toVal->addr = (caddr_t)&places[i].val;
	return True;
	}

/*
   Actions, Callbacks and Handlers
*/

static void Activate(w, event, params, nparam)
Widget w;
XEvent *event;
String *params;
Cardinal *nparam;
	{
	XmLFolderWidget f;
	XButtonEvent *be;
	XRectangle rect;
	Widget tab;
	int i;

	f = (XmLFolderWidget)w;
	if (!event || event->type != ButtonPress)
		return;
	be = (XButtonEvent *)event;
	for (i = 0; i < f->folder.tabCount; i++)
		{
		tab = f->folder.tabs[i];
		if (!XtIsManaged(tab))
			continue;
		GetTabRect(f, tab, &rect, 0); 
		if (be->x > rect.x && be->x < rect.x + (int)rect.width &&
			be->y > rect.y && be->y < rect.y + (int)rect.height)
			{
			SetActiveTab(f, tab, event, True);
			return;
			}
		}
	}

static void PrimActivate(w, clientData, callData)
Widget w;
XtPointer clientData;
XtPointer callData;
	{
	XmLFolderWidget f;
	XmAnyCallbackStruct *cbs;

	f = (XmLFolderWidget)XtParent(w);
	cbs = (XmAnyCallbackStruct *)callData;
	SetActiveTab(f, w, cbs->event, True);
	}

static void PrimFocusIn(w, event, params, nparam)
Widget w;
XEvent *event;
String *params;
Cardinal *nparam;
	{
	XmLFolderWidget f;
	Widget prevW;

	f = (XmLFolderWidget)XtParent(w);
	prevW = f->folder.focusW;
	f->folder.focusW = w;
	DrawTabHighlight(f, w);
	if (prevW)
		DrawTabHighlight(f, prevW);
	XmProcessTraversal(w, XmTRAVERSE_CURRENT);
	}

static void PrimFocusOut(w, event, params, nparam)
Widget w;
XEvent *event;
String *params;
Cardinal *nparam;
	{
	XmLFolderWidget f;
	Widget prevW;

	f = (XmLFolderWidget)XtParent(w);
	prevW = f->folder.focusW;
	f->folder.focusW = 0;
	if (prevW)
		DrawTabHighlight(f, prevW);
	DrawTabHighlight(f, w);
	}

/*
   Public Functions
*/

Widget XmLCreateFolder(parent, name, arglist, argcount)
Widget parent;
char *name;
ArgList arglist;
Cardinal argcount;
	{
	return XtCreateWidget(name, xmlFolderWidgetClass, parent,
		arglist, argcount);
	}

Widget XmLFolderAddBitmapTab(w, string, bitmapBits, bitmapWidth, bitmapHeight)
Widget w;
XmString string;
char *bitmapBits;
int bitmapWidth, bitmapHeight;
	{
	XmLFolderWidget f;
	Widget tab;
	Pixmap pix, inactPix;
	Window root;
	Display *dpy;
	int depth;
	char name[20];

	if (!XmLIsFolder(w))
		{
		XmLWarning(w, "AddBitmapTab() - widget not a XmLFolder");
		return 0;
		}
	f = (XmLFolderWidget)w;
	dpy = XtDisplay(w);
	root = DefaultRootWindow(dpy);
	depth = DefaultDepthOfScreen(XtScreen(w));
	pix = XCreatePixmapFromBitmapData(dpy, root, bitmapBits,
		bitmapWidth, bitmapHeight, f->manager.foreground,
		f->core.background_pixel, depth);
	inactPix = XCreatePixmapFromBitmapData(dpy, root, bitmapBits,
		bitmapWidth, bitmapHeight, f->folder.inactiveFg,
		f->folder.inactiveBg, depth);
	sprintf(name, "tab%d", f->folder.tabCount);
	tab = XtVaCreateManagedWidget(name,
		xmDrawnButtonWidgetClass, w,
		XmNfontList, f->folder.fontList,
		XmNmarginWidth, 2,
		XmNmarginHeight, 0,
		XmNlabelString, string,
		XmNtabPixmap, pix,
		XmNtabInactivePixmap, inactPix,
		XmNtabFreePixmaps, True,
		NULL);
	return tab;
	}

Widget XmLFolderAddBitmapTabForm(w, string, bitmapBits,
bitmapWidth, bitmapHeight)
Widget w;
XmString string;
char *bitmapBits;
int bitmapWidth, bitmapHeight;
	{
	Widget form, tab;
	XmLFolderWidget f;
	char name[20];

	if (!XmLIsFolder(w))
		{
		XmLWarning(w, "AddBitmapTabForm() - widget not a XmLFolder");
		return 0;
		}
	f = (XmLFolderWidget)w;
	tab = XmLFolderAddBitmapTab(w, string, bitmapBits,
		bitmapWidth, bitmapHeight);
	sprintf(name, "form%d", f->folder.tabCount);
	form = XtVaCreateManagedWidget(name,
		xmFormWidgetClass, w,
		XmNbackground, f->core.background_pixel,
		NULL);
	XtVaSetValues(tab, XmNtabManagedWidget, form, NULL);
	return form;
	}

Widget XmLFolderAddTab(w, string)
Widget w;
XmString string;
	{
	Widget tab;
	XmLFolderWidget f;
	char name[20];

	if (!XmLIsFolder(w))
		{
		XmLWarning(w, "AddTab() - widget not a XmLFolder");
		return 0;
		}
	f = (XmLFolderWidget)w;
	sprintf(name, "tab%d", f->folder.tabCount);
	tab = XtVaCreateManagedWidget(name,
		xmDrawnButtonWidgetClass, w,
		XmNfontList, f->folder.fontList,
		XmNmarginWidth, 2,
		XmNmarginHeight, 0,
		XmNlabelString, string,
		NULL);
	return tab;
	}

Widget XmLFolderAddTabForm(w, string)
Widget w;
XmString string;
	{
	Widget form, tab;
	XmLFolderWidget f;
	char name[20];

	if (!XmLIsFolder(w))
		{
		XmLWarning(w, "AddBitmapTabForm() - widget not a XmLFolder");
		return 0;
		}
	f = (XmLFolderWidget)w;
	tab = XmLFolderAddTab(w, string);
	sprintf(name, "form%d", f->folder.tabCount);
	form = XtVaCreateManagedWidget(name,
		xmFormWidgetClass, w,
		XmNbackground, f->core.background_pixel,
		NULL);
	XtVaSetValues(tab, XmNtabManagedWidget, form, NULL);
	return form;
	}

void XmLFolderSetActiveTab(w, position, notify)
Widget w;
int position;
Boolean notify;
	{
	XmLFolderWidget f;

	if (!XmLIsFolder(w))
		{
		XmLWarning(w, "SetActiveTab() - widget not a XmLFolder");
		return;
		}
	f = (XmLFolderWidget)w;
	if (position < 0 || position >= f->folder.tabCount)
		{
		XmLWarning(w, "SetActiveTab() - invalid position");
		return;
		}
	SetActiveTab(f, f->folder.tabs[position], 0, notify);
	}
