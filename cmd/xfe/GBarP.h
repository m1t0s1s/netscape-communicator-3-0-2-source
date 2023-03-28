#ifndef _GBarP_h
#define _GBarP_h

#include "GBar.h"
#include <X11/CoreP.h>

/* New fields for the gbar widget instance record */
typedef struct {
	int		percent;
	int		padding;
	int		zeroWidth;
	GC		fgc;
	GC		bgc;
	Pixel		fgpixel;
   } GBarPart;

/* Full instance record declaration */
typedef struct _GBarRec {
   CorePart core;
   GBarPart gbarpart;
   } GBarRec;

/* New fields for the Clock widget class record */
typedef struct {int dummy;} GBarClassPart;

/* Full class record declaration. */
typedef struct _GBarClassRec {
   CoreClassPart core_class;
   GBarClassPart gbar_class;
   } GBarClassRec;

/* Class pointer. */
extern GBarClassRec gbarClassRec;

#endif /* _GBarP_h */
