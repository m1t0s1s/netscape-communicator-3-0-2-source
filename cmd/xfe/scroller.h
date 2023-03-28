/* -*- Mode: C; tab-width: 8 -*-
   scroller.c --- defines a subclass of XmScrolledWindow
   Copyright © 1994 Netscape Communications Corporation, all rights reserved.
   Created: Jamie Zawinski <jwz@netscape.com>, 23-Jul-94.
 */

#ifndef _FE_SCROLLER_H_
#define _FE_SCROLLER_H_

#include <Xm/Xm.h>
#include <Xm/ScrolledW.h>

extern WidgetClass scrollerClass;
typedef struct _ScrollerClassRec *ScrollerClass;
typedef struct _ScrollerRec      *Scroller;

#endif /* _FE_SCROLLER_H_ */
