/* 
 * (c) Copyright 1989, 1990, 1991, 1992 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2
*/ 
/*   $RCSfile: HPaned.h,v $ $Revision: 1.1 $ $Date: 1995/07/25 00:06:20 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/****************************************************************
 *
 * Horizontal Paned Widget (SubClass of CompositeClass)
 *
 ****************************************************************/
#ifndef _XmHPanedW_h
#define _XmHPanedW_h

#include <Xm/Xm.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Class record constant */
externalref WidgetClass xmHPanedWindowWidgetClass;

#ifndef XmIsHPanedWindow
#define XmIsHPanedWindow(w)	XtIsSubclass(w, xmHPanedWindowWidgetClass)
#endif /* XmIsHPanedWindow */

typedef struct _XmHPanedWindowClassRec  *XmHPanedWindowWidgetClass;
typedef struct _XmHPanedWindowRec	*XmHPanedWindowWidget;


/********    Public Function Declarations    ********/
#ifdef _NO_PROTO

extern Widget XmCreateHPanedWindow() ;

#else

extern Widget XmCreateHPanedWindow( 
                        Widget parent,
                        char *name,
                        ArgList args,
                        Cardinal argCount) ;

#endif /* _NO_PROTO */
/********    End Public Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmHPanedWindow_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
