/* 
 * (c) Copyright 1989, 1990, 1991, 1992, 1993 OPEN SOFTWARE FOUNDATION, INC. 
 * ALL RIGHTS RESERVED 
*/ 
/* 
 * Motif Release 1.2.2
*/ 
/*   $RCSfile: HPanedP.h,v $ $Revision: 1.1 $ $Date: 1995/07/25 00:06:20 $ */
/*
*  (c) Copyright 1989, DIGITAL EQUIPMENT CORPORATION, MAYNARD, MASS. */
/*
*  (c) Copyright 1987, 1988, 1989, 1990, 1991, 1992 HEWLETT-PACKARD COMPANY */
/*
*  (c) Copyright 1988 MASSACHUSETTS INSTITUTE OF TECHNOLOGY  */
/*********************************************************************
 *
 * XmHPanedWindowWidget Private Data
 *
 *********************************************************************/

#ifndef _XmHPanedWP_h
#define _XmHPanedWP_h

#include "HPaned.h"
#include <Xm/ManagerP.h>

#ifdef __cplusplus
extern "C" {
#endif

/* New fields for the HPanedWindow widget class record */

typedef struct _XmHPanedWindowClassPart
{
    XtPointer extension;
} XmHPanedWindowClassPart;


/* Full Class record declaration */

typedef struct _XmHPanedWindowClassRec {
    CoreClassPart       core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    XmManagerClassPart  manager_class;
    XmHPanedWindowClassPart     paned_window_class;
} XmHPanedWindowClassRec;

externalref XmHPanedWindowClassRec xmHPanedWindowClassRec;


/* HPanedWindow constraint record */

typedef struct _XmHPanedWindowConstraintPart {
    int		position;	/* position location in HPanedWindow */
    int		dheight;	/* Desired size */
    Position	dy;		/* Desired Location */
    Position	olddy;		/* The last value of dy. */
    Dimension	min;		/* Minimum height */
    Dimension	max;		/* Maximum height */
    Boolean     isPane;         /* true if constraint of pane, false if
				   constraint of sash */
    Boolean	allow_resize;	/* TRUE iff child resize requests are ok */
    Boolean	skip_adjust;	/* TRUE iff child's height should not be */
				/* changed without explicit user action. */
    Widget	sash;		/* The sash for this child */
    Widget	separator;	/* The separator for this child */
    short       position_index; /* new 1.2 positionIndex resource */
 
} XmHPanedWindowConstraintPart;

typedef struct _XmHPanedWindowConstraintRec 
{
    XmManagerConstraintPart manager;
    XmHPanedWindowConstraintPart  panedw;
} XmHPanedWindowConstraintRec, * XmHPanedWindowConstraintPtr;


/* New Fields for the HPanedWindow widget record */

typedef struct {
    /* resources */
    Boolean     refiguremode;        /* Whether to refigure changes right now */
    Boolean	separator_on;	     /* make separator visible */

    Dimension  	margin_width;	     /* space between right and left edges of
					HPanedWindow window and it's children */
    Dimension  	margin_height;	     /* space between top and bottom edges of
					HPanedWindow window and it's children */
    Dimension   spacing;             /* whitespace between panes
				        around window, else leave none */
    /* sash modifying resources */
    Dimension	sash_width;	       /* Modify sash width */
    Dimension	sash_height;	       /* Modify sash height */
    Dimension   sash_shadow_thickness; /* Modify sash shadow_thickness */

    Position    sash_indent;           /* Location of sashs (offset
                                          from right margin)	*/
    /* private */
    int         starty;               /* mouse origin when adjusting */

    short	increment_count;      /* Sash increment count */
    short       pane_count;           /* number of managed panes */
    short       num_slots;	      /* number of avail. slots for children*/
    short       num_managed_children; /* holds number of managed children */

    Boolean     recursively_called;    /* For change_managed AND creation of
					* private sash and separator
					* children
					*/
    Boolean     resize_at_realize;    /* For realize if GeometryNo condition */

    XmHPanedWindowConstraintPtr top_pane;    /* pane closest to 0 index */
    XmHPanedWindowConstraintPtr bottom_pane; /* pane farthest away from 0 index*/

    GC          flipgc;               /* GC to use when animating borders */
    WidgetList  managed_children;     /* keep track of managed children */
#ifdef ORIENTED_PANEDW
    unsigned char orientation ; /* new pane orientation support */
#endif

} XmHPanedWindowPart;

/**************************************************************************
 *
 * Full instance record declaration
 *
 **************************************************************************/

typedef struct _XmHPanedWindowRec {
    CorePart       core;
    CompositePart  composite;
    ConstraintPart constraint;
    XmManagerPart  manager;
    XmHPanedWindowPart   paned_window;
} XmHPanedWindowRec;


/********    Private Function Declarations    ********/
#ifdef _NO_PROTO


#else


#endif /* _NO_PROTO */
/********    End Private Function Declarations    ********/


#ifdef __cplusplus
}  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif /* _XmHPanedWP_h */
/* DON'T ADD ANYTHING AFTER THIS #endif */
