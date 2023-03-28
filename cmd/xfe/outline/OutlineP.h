/* OutlineP.h: Private header file for the Outline widget. */
/* Copyright © 1994 Torgeir Veimo. */
/* See the README file for copyright details. */

#ifndef OUTLINEP_H
#define OUTLINEP_H

#include "Outline.h"
#include <Xm/ManagerP.h>

typedef struct _XmOutlineClassPart {
    int    empty;
} XmOutlineClassPart;
        
typedef struct _XmOutlineClassRec {
    CoreClassPart        core_class;
    CompositeClassPart   composite_class;
    ConstraintClassPart  constraint_class;
    XmManagerClassPart   manager_class;
    XmOutlineClassPart   outline_class;
}  XmOutlineClassRec;
    
typedef struct {
    Bool draw_outline;     /* Draw outline? */
    Dimension indentation; /* Indentation for child widgets. */
    Dimension spacing;      /* Space between children. */
    Dimension marginWidth, marginHeight;
} XmOutlinePart;
    
typedef struct _XmOutlineRec {
    CorePart        core;
    CompositePart   composite;
    ConstraintPart  constraint;
    XmManagerPart   manager;
    XmOutlinePart   outline;
}  XmOutlineRec;

#endif /* OUTLINEP_H */

