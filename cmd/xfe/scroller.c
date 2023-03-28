/* -*- Mode: C; tab-width: 8 -*-
   scroller.c --- defines a subclass of XmScrolledWindow
   Copyright © 1994 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 23-Jul-94.
 */

#include <stdio.h>
#include <stdlib.h>
#include "mozilla.h"
#include "scroller.h"
#include "scrollerP.h"

static void scroller_resize (Widget);

ScrollerClassRec scrollerClassRec =
{
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &xmScrolledWindowClassRec,
    /* class_name         */    "Scroller",
    /* widget_size        */    sizeof(ScrollerRec),
    /* class_initialize   */    NULL,
    /* class_partinit     */    NULL /* ClassPartInitialize */,
    /* class_inited       */	FALSE,
    /* initialize         */    NULL /* Initialize */,
    /* Init hook	  */    NULL,
    /* realize            */    XtInheritRealize /* Realize */,
    /* actions		  */	NULL /* ScrolledWActions */,
    /* num_actions	  */	0 /* XtNumber(ScrolledWActions) */,
    /* resources          */    NULL /* resources */,
    /* num_resources      */    0 /* XtNumber(resources) */,
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterleave*/	TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    NULL,
    /* resize             */    scroller_resize,
    /* expose             */    XtInheritExpose /* (XtExposeProc) Redisplay */,
    /* set_values         */    NULL /* (XtSetValuesFunc )SetValues */,
    /* set values hook    */    NULL,
    /* set values almost  */    XtInheritSetValuesAlmost,
    /* get values hook    */    NULL,
    /* accept_focus       */    NULL,
    /* Version            */    XtVersion,
    /* PRIVATE cb list    */    NULL,
    /* tm_table		  */    XtInheritTranslations,
    /* query_geometry     */    XtInheritQueryGeometry /* QueryProc */,
    /* display_accelerator*/    NULL,
    /* extension          */    NULL,
  },
  {
/* composite_class fields */
    /* geometry_manager   */    XtInheritGeometryManager /*(XtGeometryHandler )GeometryManager*/,
    /* change_managed     */    XtInheritChangeManaged /*(XtWidgetProc) ChangeManaged*/,
    /* insert_child	  */	XtInheritInsertChild /*(XtArgsProc) InsertChild*/,	
    /* delete_child	  */	XtInheritDeleteChild,
    /* Extension          */    NULL,
  },{
/* Constraint class Init */
    NULL,
    0,
    0,
    NULL,
    NULL,
    NULL,
    NULL
      
  },
/* Manager Class */
   {		
      XmInheritTranslations/*ScrolledWindowXlations*/,     /* translations        */    
      NULL /*get_resources*/,			/* get resources      	  */
      0 /*XtNumber(get_resources)*/,		/* num get_resources 	  */
      NULL,					/* get_cont_resources     */
      0,					/* num_get_cont_resources */
      XmInheritParentProcess,                   /* parent_process         */
      NULL,					/* extension           */    
   },

 {
/* Scrolled Window class - none */     
     /* mumble */               0
 },

 {
/* Scroller class - none */     
     /* mumble */               0
 }
};

WidgetClass scrollerClass = (WidgetClass)&scrollerClassRec;


static void scroller_resize (Widget widget)
{
  Scroller scroller = (Scroller) widget;

  /* Invoke the resize procedure of the superclass.
     Probably there's some nominally more portable way to do this
     (yeah right, like any of these slot names could possibly change
     and have any existing code still work.)
   */
  widget->core.widget_class->core_class.superclass->core_class.resize (widget);

  /* Now run our callback (yeah, I should use a real callback, so sue me.) */
  scroller->scroller.resize_hook (widget, scroller->scroller.resize_arg);
}
