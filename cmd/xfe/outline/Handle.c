/* Handle.c: Methods for the Handle widget. */
/* Copyright © 1994 Torgeir Veimo. */
/* See the README file for copyright details. */

#include <Xm/XmP.h>
#include <Xm/ArrowB.h>
#include <Xm/Label.h>
#include "HandleP.h"

static void Initialize();
static void Resize();
static void ChangeManaged();
static void Redisplay();
static Boolean SetValues();
static XtGeometryResult GeometryManager();
static XtGeometryResult QueryGeometry();

static void PreferredSize(XmHandleWidget, Dimension *, Dimension *);
static void DoLayout(XmHandleWidget);

static XtResource resources[] = {
  { XmNsubWidget, XmCSubWidget, 
      XmRPointer, sizeof(Widget),
      XtOffset(XmHandleWidget, handle.child), 
      XtRPointer, NULL },
  
  { XmNmarginWidth, XmCMarginWidth, 
      XmRVerticalDimension, sizeof(Dimension),
      XtOffset(XmHandleWidget, handle.marginWidth), 
      XtRImmediate, (caddr_t) 2 },

  { XmNmarginHeight, XmCMarginHeight, 
      XmRHorizontalDimension, sizeof(Dimension),
      XtOffset(XmHandleWidget, handle.marginHeight), 
      XtRImmediate, (caddr_t) 2 },

  { XmNspacing, XmCSpacing, 
      XmRDimension, sizeof(Dimension),
      XtOffset(XmHandleWidget, handle.spacing), 
      XtRImmediate, (caddr_t) 2 },
};



XmHandleClassRec xmHandleClassRec = {
{
  /* core_class members */
  (WidgetClass) &xmManagerClassRec, /* superclass         */
  "XmHandle",                       /* class_name         */
  sizeof (XmHandleRec),             /* widget_size        */
  NULL,                             /* class_initialize   */
  NULL,                             /* class_part_init    */  
  FALSE,                            /* class_inited       */  
  Initialize,                       /* initialize         */
  NULL,                             /* initialize_hook    */  
  XtInheritRealize,                 /* realize            */
  NULL,                             /* actions            */
  0,                                /* num_actions        */  
  resources,                        /* resources          */
  XtNumber(resources),              /* num_resources      */
  NULLQUARK,                        /* xrm_class          */
  TRUE,                             /* compress_motion    */  
  XtExposeCompressMaximal,          /* compress_exposure  */  
  TRUE,                             /* compress_enterleave*/  
  FALSE,                            /* visible_interest   */
  NULL,                             /* destroy            */
  Resize,                           /* resize             */
  Redisplay,                        /* expose             */
  SetValues,                        /* set_values         */
  NULL,                             /* set_values_hook    */
  XtInheritSetValuesAlmost,         /* set_values_almost  */
  NULL,                             /* get_values_hook    */  
  NULL,                             /* accept_focus       */
  XtVersion,                        /* version            */  
  NULL,                             /* callback_private   */
  NULL,                             /* tm_table           */
  QueryGeometry,                    /* query_geometry     */  
  NULL,                             /* display_accelerator*/
  NULL,                             /* extension          */
},
{
  /* composite_class members */
  GeometryManager,                  /* geometry_manager   */
  ChangeManaged,                    /* change_managed     */
  XtInheritInsertChild,             /* insert_child       */  
  XtInheritDeleteChild,             /* delete_child       */  
  NULL,                             /* extension          */
},
{      
  /* constraint_class fields */
  NULL,                             /* resource list        */   
  0,                                /* num resources        */   
  0,                                /* constraint size      */   
  NULL,                             /* init proc            */   
  NULL,                             /* destroy proc         */   
  NULL,                             /* set values proc      */   
  NULL,                             /* extension            */
},
{ 
  /* XmManager class */
  XtInheritTranslations,            /* translations         */
  NULL,                             /* syn resources        */
  0,                                /* num syn_resources    */
  NULL,                             /* syn_cont_resources     */
  0,                                /* num_syn_cont_resources */
  XmInheritParentProcess,           /* parent_process       */
  NULL,                             /* extension            */    
},
{
  /* Handle class members */
  0,                                /* empty              */  
}
};

WidgetClass xmHandleWidgetClass =  ( WidgetClass )  &xmHandleClassRec;



static void Initialize(Widget request, Widget new,
		       ArgList args, Cardinal *numArgs) 
{
  /* Make shure the widget has nonzero size. */

  if (XtWidth(new) <= 0) 
    XtWidth(new) = 1;
  
  if (XtWidth(new) <= 0) 
    XtHeight(new) = 1;
} 
    
static void Resize(Widget w) 
{
  DoLayout((XmHandleWidget) w);
} 
    
static void DoLayout(XmHandleWidget hw) 
{
  Widget child;
  int width = 0, height = 0, i;
  
  width += hw->handle.marginWidth;

  /* If children are to be centered we need to calculate max height first. */

  for (i = 0; i < hw->composite.num_children; i++) {
    
    child = hw->composite.children[i];

    if (child != hw->handle.child && child->core.managed) {

      /* Add Athena compatible code here... */

      _XmConfigureObject(child, width, hw->handle.marginHeight, 
			 XtWidth(child), XtHeight(child), 
			 XtBorderWidth(child)); 
      
      if (XtHeight(child) > height)
	height = XtHeight(child);
      
      width += (XtWidth(child) + hw->handle.spacing);
    }
  }

  height += hw->handle.marginHeight + hw->handle.spacing;

  child = hw->handle.child;

  if (child && child->core.managed) {
  
    _XmConfigureObject(child, hw->handle.marginWidth, height,
		       XtWidth(child), XtHeight(child),
		       XtBorderWidth(child));
  }
    
  /* Is this really is necessary... */

  if (XtIsRealized(hw))
    XClearArea(XtDisplay(hw), XtWindow(hw), 0, 0, 0, 0, True);      
}
                    
static Boolean SetValues(Widget current, 
			 Widget request, 
			 Widget new,
			 ArgList args,
			 Cardinal numargs)
{
  Boolean redraw = FALSE;
  XmHandleWidget new_hw = (XmHandleWidget) new;
  XmHandleWidget cur_hw = (XmHandleWidget) current;
  
  if (new_hw->core.background_pixel != cur_hw->core.background_pixel)
    redraw = TRUE;
      
  if (new_hw->handle.child != cur_hw->handle.child ||
      new_hw->handle.marginWidth != cur_hw->handle.marginWidth ||
      new_hw->handle.marginHeight != cur_hw->handle.marginHeight ||
      new_hw->handle.spacing != cur_hw->handle.spacing) {

    if (XtParent(new_hw->handle.child) != new)
      _XmWarning(new, "specified sub widget not direct child!");

    DoLayout(new_hw);
    redraw = FALSE;
  }
  return (redraw);
}


static void Redisplay(Widget w, 
		      XEvent *event, 
		      Region region) 
{
  /* Redraw all gadgets. */
  
  /* Check wether we are visible or not, and return if not. */
  
  if (XmGetVisibility(w) == XmVISIBILITY_FULLY_OBSCURED)
    return;

  _XmRedisplayGadgets(w, event, region);
  
  /* The handle widget provides no screen decoration by itself. */
}
        
static void PreferredSize(XmHandleWidget hw,
			  Dimension *w,
			  Dimension *h) 
{
  XtWidgetGeometry preferred;
  Widget child;
  Dimension width = 0, height = 0;
  int nchildren = 0, i;

  for (i = 0; i < hw->composite.num_children; i++) {
    
    child = hw->composite.children[i];

    if (child != hw->handle.child && child->core.managed) {
      XtQueryGeometry(child, NULL, &preferred);
      
      width += preferred.width;
      
      if (preferred.height > height)
	height = preferred.height;
      
      ++nchildren;
    }
  }

  /* Add spacing between children. */
  width += (nchildren - 1) * hw->handle.spacing;

  child = hw->handle.child;

  if (child && child->core.managed) {
    XtQueryGeometry(child, NULL, &preferred);
    
    height += (hw->handle.spacing + preferred.height);
    
    if (preferred.width > width)
      width = preferred.width;
  }

  /* Add margin around widget. */
  width += hw->handle.marginWidth;
  height += hw->handle.marginHeight;

  *h = height; *w = width;
}


static XtGeometryResult QueryGeometry(Widget widget,
				      XtWidgetGeometry *intended,
				      XtWidgetGeometry *reply) 
{
  XmHandleWidget hw = (XmHandleWidget) widget;
  Dimension   w, h;
  
  if (intended->request_mode & (~(CWWidth | CWHeight))) 
    return (XtGeometryYes);
  
  PreferredSize(hw, &w, &h);
 
  if (intended->request_mode & CWWidth &&
      intended->width == w &&
      intended->request_mode & CWHeight &&
      intended->height == h)
    return (XtGeometryYes);

  if (w == XtWidth(widget) &&
      h == XtHeight(widget))
    return (XtGeometryNo);
  
  reply->request_mode = CWWidth | CWHeight;
  reply->width = w;
  reply->height = h;
  return XtGeometryAlmost;
}
                                        
static XtGeometryResult GeometryManager(Widget widget, 
					XtWidgetGeometry *request,
					XtWidgetGeometry *reply) 
{
  XmHandleWidget hw = (XmHandleWidget) XtParent(widget);
    
  if (request->request_mode & (CWX | CWY)) 
    return (XtGeometryNo);
  
  if (request->request_mode & (CWWidth | CWHeight)) {
    Dimension w = 0, h = 0;
    int i, nchildren = 0;
    
    /* Set the child's size to the requested size. */

    if (request->request_mode & CWWidth) 
      XtWidth(widget) = request->width;
    
    if (request->request_mode & CWHeight) 
      XtHeight(widget) = request->height;
    
    PreferredSize(hw, &w, &h);
    
    if (w != XtWidth(hw) || h != XtHeight(hw)) {
      XtGeometryResult result;
      
      Dimension replyWidth, replyHeight;
      
      result = XtMakeResizeRequest((Widget) hw, w, h,
				   &replyWidth,
				   &replyHeight);

      if (result == XtGeometryAlmost) 
	XtMakeResizeRequest((Widget) hw,
			    replyWidth, replyHeight, 
			    NULL, NULL);       
    }
    DoLayout(hw);
  }
  return (XtGeometryYes);
}
                                                    
static void ChangeManaged(Widget child) 
{
  XmHandleWidget hw = (XmHandleWidget) child; 
  Dimension w, h;
  
  PreferredSize(hw, &w, &h);
  
  if (w != XtWidth(hw) || h != XtHeight(hw)) {
    XtGeometryResult result;
    Dimension replyWidth, replyHeight;
    
    result = XtMakeResizeRequest((Widget) hw, w, h,
				 &replyWidth, &replyHeight);
    if (result == XtGeometryAlmost) 
      XtMakeResizeRequest((Widget) hw,
			  replyWidth, replyHeight, 
			  NULL, NULL);   
  }
  DoLayout(hw);

  _XmNavigChangeManaged(child);    
  
  if (XtIsRealized(child))
    XClearArea(XtDisplay(child), XtWindow(child), 0, 0, 0, 0, True);
}

Widget XmCreateHandle(Widget parent,
		      char *name,
		      ArgList arglist,
		      Cardinal argcount)  
{
  return (XtCreateWidget(name, xmHandleWidgetClass, 
			 parent, arglist, argcount));
}



