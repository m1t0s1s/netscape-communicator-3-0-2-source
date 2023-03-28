#include <stdio.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include "GBarP.h"

/* Initialization of defaults */

#define offset(field) XtOffset(GBarWidget,gbarpart.field)
#define goffset(field) XtOffset(Widget,core.field)

/* ONLY percent is settable via SetValues! */

static XtResource resources[] = {
{XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
	goffset(background_pixel), XtRString, "black"},
{XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
	offset(fgpixel), XtRString, "red"},
{"padding", XtCMargin, XtRInt, sizeof(int),
	offset(padding), XtRString, "1"},
{"percent", XtCParameter, XtRInt, sizeof(int),
	offset(percent), XtRString, "0"},
{"zeroWidth", XtCParameter, XtRInt, sizeof(int),
	offset(zeroWidth), XtRString, "0"},
};

#undef offset
#undef goffset

static void	ClassInitialize();
static void	Initialize (Widget, Widget);
static void	Destroy ( Widget );
static void	Resize ( Widget );
static void	Redisplay ( Widget , XEvent *,Region );
static void	Realize ( Widget, XtValueMask *, XSetWindowAttributes *);
static Boolean	SetValues ( Widget , Widget , Widget );

GBarClassRec gbarClassRec = {
{   /* core fields */
    /* superclass		*/	&widgetClassRec,
    /* class_name		*/	"GBar",
    /* widget_size		*/	sizeof(GBarRec),
    /* class_initialize		*/	ClassInitialize,
    /* class_part_initialize	*/	NULL,
    /* class_inited		*/	FALSE,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	Realize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* resource_count		*/	XtNumber(resources),
    /* xrm_class		*/	NULL,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	Destroy,
    /* resize			*/	Resize,
    /* expose			*/	Redisplay,
    /* set_values		*/	NULL,
    /* set_values_hook		*/	SetValues,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	NULL,
    /* query_geometry           */	XtInheritQueryGeometry,
    /* display_accelerator      */	XtInheritDisplayAccelerator,
    /* extension                */	NULL
}
};

WidgetClass gbarWidgetClass = (WidgetClass) &gbarClassRec;

static void 
ClassInitialize()
{
	XtAddConverter( XtRString, XtRBackingStore, 
		XmuCvtStringToBackingStore, NULL, 0 );
	return;
}

/* ARGSUSED */
static void 
Initialize (request, new)
Widget request, new;
{
	GBarWidget	w = (GBarWidget)new;
	XGCValues	gcv;

	gcv.foreground = w->gbarpart.fgpixel;
	w->gbarpart.fgc = XtGetGC((Widget)w, GCForeground, &gcv);
	gcv.foreground = w->core.background_pixel;
	w->gbarpart.bgc = XtGetGC((Widget)w, GCForeground, &gcv);

	if (w->core.width == 0)  w->core.width = 300;
	if (w->core.height == 0) w->core.height = 100;

	return;
}

static void 
Realize (gw, valueMask, attrs)
Widget gw;
XtValueMask *valueMask;
XSetWindowAttributes *attrs;
{
	register CoreClassRec 	*superclass;

	*valueMask |= CWBitGravity;
	attrs->bit_gravity = ForgetGravity ;

	superclass = (CoreClassRec *)gbarWidgetClass->core_class.superclass;
	(*superclass->core_class.realize) (gw, valueMask, attrs);

	return;
}

static void 
Destroy (gw)
Widget gw;
{
	XtDestroyGC (((GBarWidget) gw)->gbarpart.fgc);
	XtDestroyGC (((GBarWidget) gw)->gbarpart.bgc);
	return;
}

static Boolean 
SetValues (gcurrent, grequest, gnew)
Widget gcurrent, grequest, gnew;
{
	GBarWidget 	w = (GBarWidget) gnew;
	GBarWidget 	ow = (GBarWidget) gcurrent;

	w->gbarpart.percent = ow->gbarpart.percent;

	return True;
}

static void 
Resize(Widget gw) 
{
	return ;
}

/* ARGSUSED */
static void 
Redisplay (gw, event, region)
Widget gw;
XEvent *event;		/* unused */
Region region;		/* unused */
{
	GBarWidget	w	= (GBarWidget) gw;
	Display		*dpy = XtDisplay(gw);

	int		tpos, tw, th;
	int		t1;

	tw = w->core.width - (w->gbarpart.padding <<1);
	th = w->core.height - (w->gbarpart.padding <<1);
	tpos = w->gbarpart.padding;

	t1 = w->gbarpart.zeroWidth + 
		(w->gbarpart.percent*(tw-w->gbarpart.zeroWidth))/100;

	XFillRectangle(dpy, XtWindow(gw), w->gbarpart.fgc,
		tpos, tpos, t1, th);
	XFillRectangle(dpy, XtWindow(gw), w->gbarpart.bgc,
		tpos+t1, tpos, tw-t1, th);
	XFlush(dpy);
	
	return ;
}
