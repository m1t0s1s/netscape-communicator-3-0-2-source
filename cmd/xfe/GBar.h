#ifndef _GBar_h
#define _GBar_h

/***********************************************************************
 *
 * GBar Widget
 * Fill a rect (size of parent) with fg, at percent% from L to R
 *
 ***********************************************************************/

#include <X11/Xmu/Xmu.h>

typedef struct _GBarRec *GBarWidget;  
typedef struct _GBarClassRec *GBarWidgetClass;

extern WidgetClass gbarWidgetClass;

#define MIN(A,B) ( (A<B) ? A : B )
#define MAX(A,B) ( (A>B) ? A : B )
#define SIZE_DEFAULT 64

#endif /* _GBar_h */
/* DON'T ADD STUFF AFTER THIS #endif */
