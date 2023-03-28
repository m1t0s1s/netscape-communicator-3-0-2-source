/* -*- Mode: C; tab-width: 8 -*-
   scroller.c --- defines a subclass of XmScrolledWindow
   Copyright © 1994 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 23-Jul-94.
 */

#ifndef _FE_SCROLLERP_H_
#define _FE_SCROLLERP_H_

#include "scroller.h"
#include <Xm/ScrolledWP.h>

typedef struct
{
  int bite_me;
} ScrollerClassPart;

typedef struct _ScrollerClassRec
{
  CoreClassPart	core_class;
  CompositeClassPart		composite_class;
  ConstraintClassPart		constraint_class;
  XmManagerClassPart		manager_class;
  XmScrolledWindowClassPart	swindow_class;
  ScrollerClassPart		scroller_class;
} ScrollerClassRec;

extern ScrollerClassRec scrollerClassRec;

typedef struct 
{
  void *resize_arg;
  void (*resize_hook) (Widget, void *arg);
} ScrollerPart;

typedef struct _ScrollerRec
{
    CorePart			core;
    CompositePart		composite;
    ConstraintPart		constraint;
    XmManagerPart		manager;
    XmScrolledWindowPart	swindow;
    ScrollerPart		scroller;
} ScrollerRec;

#endif /* _FE_SCROLLERP_H_ */
