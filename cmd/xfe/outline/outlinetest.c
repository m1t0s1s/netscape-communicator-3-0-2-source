/* outlinetest.c: Demo for the Outline widget class. */
/* Copyright © 1994 Torgeir Veimo. */
/* See the README file for copyright details. */

#include <Xm/Xm.h>
#include <Xm/ArrowB.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h> 
#include <Xm/ScrolledW.h>
#include <X11/Xmu/Editres.h>
#include "Outline.h"
#include "Handle.h"

static void  GrowCB(Widget, XtPointer, XtPointer); 
static void  ToggleButtonHandleCB(Widget, XtPointer, XtPointer); 
static void  UnmanageCB(Widget, XtPointer, XtPointer); 
static void  ManageCB(Widget, XtPointer, XtPointer); 

void main (int argc, char **argv)
{
  Widget shell, scroller, outline, suboutline;
  Widget button1, button2, button3, button4;
  Widget left, right, handle, subhandle;

  XtAppContext app;

  void _XEditResCheckMessages();
  
  /* Initialize Xt. */
  
  shell = XtAppInitialize(&app, "RowTest", NULL, 0, 
			  &argc, argv, NULL, NULL, 0);    
  
  /* Initialize editres protocol */
  XtAddEventHandler(shell, (EventMask) 0, True, _XEditResCheckMessages, NULL);
  
  
  scroller = XtVaCreateManagedWidget("scroller", xmScrolledWindowWidgetClass,
				     shell,
				     XmNscrollingPolicy, XmAUTOMATIC,
				     NULL);
				     
  
  handle = XtVaCreateManagedWidget("roothandle", xmHandleWidgetClass,
				   scroller,
				   NULL);

  left = XtVaCreateManagedWidget("left", xmArrowButtonWidgetClass,
				 handle,
				 XmNarrowDirection, XmARROW_DOWN,
				 XmNshadowThickness, 0,
				 NULL);
  right = XtVaCreateManagedWidget("right", xmLabelWidgetClass,
				  handle,
				  NULL);

  right = XtVaCreateManagedWidget("2", xmLabelWidgetClass,
				  handle,
				  NULL);

  right = XtVaCreateManagedWidget("3", xmLabelWidgetClass,
				  handle,
				  NULL);

  right = XtVaCreateManagedWidget("4", xmLabelWidgetClass,
				  handle,
				  NULL);

  outline = XtVaCreateManagedWidget("outline", xmOutlineWidgetClass,
				    handle,
				    XmNindentation, 20,
				    XmNoutline, TRUE,
				    NULL);
  XtAddCallback(left, 
		XmNactivateCallback, ToggleButtonHandleCB, 
		(XtPointer) outline);
  
  XtVaSetValues(handle, 
		XmNsubWidget, outline,
		NULL);

  /* Add children to the Outline widget. */
  
  button1 = XtCreateManagedWidget("gadgetbutton",
				   xmPushButtonWidgetClass,
				   outline, NULL, 0);

  button2 = XtCreateManagedWidget("unmanage",
				   xmPushButtonWidgetClass,
				   outline, NULL, 0);
  
  button3 = XtCreateManagedWidget("manage",
				   xmPushButtonWidgetClass,
				   outline, NULL, 0);
  
  button4 = XtCreateManagedWidget("growbutton",
				  xmPushButtonWidgetClass,
				  outline, NULL, 0);

  XtAddCallback(button1, XmNactivateCallback, GrowCB, NULL);
  XtAddCallback(button2, XmNactivateCallback, UnmanageCB, (XtPointer) button2);
  XtAddCallback(button3, XmNactivateCallback, ManageCB, (XtPointer) button2);
  XtAddCallback(button4, XmNactivateCallback, GrowCB, NULL);

  subhandle = XtVaCreateManagedWidget("subhandle",
				      xmHandleWidgetClass,
				      outline,
				      NULL);
  left = XtVaCreateManagedWidget("left", xmArrowButtonWidgetClass,
				 subhandle,
				 XmNarrowDirection, XmARROW_DOWN,
				 XmNshadowThickness, 0,
				 NULL);
  right = XtVaCreateManagedWidget("right", xmLabelWidgetClass,
				  subhandle,
				  NULL);


  suboutline = XtVaCreateManagedWidget("suboutline",
				       xmOutlineWidgetClass,
				       subhandle, 
				       XmNindentation, 20, 
				       XmNoutline, TRUE,
				       NULL);
  XtAddCallback(left, 
		XmNactivateCallback, ToggleButtonHandleCB, 
		(XtPointer) suboutline);

  XtVaSetValues(subhandle, 
		XmNsubWidget, suboutline, 
		NULL);

  button1 = XtCreateManagedWidget("subbutton1",
				   xmPushButtonWidgetClass,
				   suboutline, NULL, 0);

  button2 = XtCreateManagedWidget("subbutton2",
				   xmPushButtonWidgetClass,
				   suboutline, NULL, 0);
  
  button3 = XtCreateManagedWidget("subbutton3",
				   xmPushButtonWidgetClass,
				   suboutline, NULL, 0);
  
  button4 = XtCreateManagedWidget("subbutton4",
				  xmPushButtonWidgetClass,
				  suboutline, NULL, 0);

  XtAddCallback(button1, XmNactivateCallback, GrowCB, NULL);
  XtAddCallback(button2, XmNactivateCallback, UnmanageCB, (XtPointer) button2);
  XtAddCallback(button3, XmNactivateCallback, ManageCB, (XtPointer) button2);
  XtAddCallback(button4, XmNactivateCallback, GrowCB, NULL);
  
  subhandle = XtVaCreateManagedWidget("subhandle",
				      xmHandleWidgetClass,
				      outline,
				      NULL);
  left = XtVaCreateManagedWidget("left", xmArrowButtonWidgetClass,
				 subhandle,
				 XmNarrowDirection, XmARROW_DOWN,
				 XmNshadowThickness, 0,
				 NULL);
  right = XtVaCreateManagedWidget("right", xmLabelWidgetClass,
				  subhandle,
				  NULL);


  suboutline = XtVaCreateManagedWidget("suboutline",
				       xmOutlineWidgetClass,
				       subhandle, 
				       XmNindentation, 20, 
				       XmNoutline, TRUE,
				       NULL);
  XtAddCallback(left, 
		XmNactivateCallback, ToggleButtonHandleCB, 
		(XtPointer) suboutline);

  XtVaSetValues(subhandle, XmNsubWidget, suboutline, NULL);

  button1 = XtCreateManagedWidget("even",
				   xmPushButtonGadgetClass,
				   suboutline, NULL, 0);

  button2 = XtCreateManagedWidget("more",
				   xmPushButtonWidgetClass,
				   suboutline, NULL, 0);
  
  button3 = XtCreateManagedWidget("widgets",
				   xmPushButtonWidgetClass,
				   suboutline, NULL, 0);
  
  button4 = XtCreateManagedWidget("here",
				  xmPushButtonWidgetClass,
				  suboutline, NULL, 0);

  XtRealizeWidget(shell);
  
  XtAppMainLoop(app); 
}

static void GrowCB(Widget w, 
		   XtPointer clientData, 
		   XtPointer callData)
{
  Dimension  width, height;
  
  XtVaGetValues(w, XmNwidth, &width, XmNheight, &height, NULL);
  
  width +=10; height +=10;
  
  XtVaSetValues(w, XmNwidth, width, XmNheight, height, NULL);
}
       
void ToggleButtonHandleCB(Widget w,
			  XtPointer closure,
			  XtPointer call_data)
{
  XmOutlineWidget outline = (XmOutlineWidget) closure;
  int direction;
  
  XtVaGetValues(w, XmNarrowDirection, &direction, NULL);
  
  switch (direction){
  case XmARROW_DOWN:
    XtUnmanageChild((Widget) outline);
    XtVaSetValues(w, XmNarrowDirection, XmARROW_RIGHT, NULL);
    break;
  case XmARROW_RIGHT:
    XtManageChild((Widget) outline);
    XtVaSetValues(w, XmNarrowDirection, XmARROW_DOWN, NULL);
    break;
  default:
    break;
  }
}
   
void UnmanageCB(Widget w,
		XtPointer closure,
		XtPointer call_data)
{
  Widget button = (Widget) closure;

  XtUnmanageChild(button);
}

void ManageCB(Widget w,
	      XtPointer closure,
	      XtPointer call_data )
{
  Widget button = (Widget) closure;

  XtManageChild(button);
}

