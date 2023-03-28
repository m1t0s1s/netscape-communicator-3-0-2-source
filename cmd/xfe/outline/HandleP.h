/* HANDLEP.h: Private header file for the Handle widget. */
/* Copyright © 1994 Torgeir Veimo. */
/* See the README file for copyright details. */

#ifndef HANDLEP_H
#define HANDLEP_H

#include "Handle.h"
#include <Xm/ManagerP.h>

typedef struct _XmHandleClassPart {
    int empty;
} XmHandleClassPart;
        
typedef struct _XmHandleClassRec {
  CoreClassPart        core_class;
  CompositeClassPart   composite_class;
  ConstraintClassPart  constraint_class;
  XmManagerClassPart   manager_class;
  XmHandleClassPart    handle_class;
} XmHandleClassRec;
    
typedef struct {
  Widget	child; /* child widget handeled by this widget */
  Dimension	marginWidth, marginHeight, spacing;
} XmHandlePart;
    
typedef struct _XmHandleRec {
  CorePart        core;
  CompositePart   composite;
  ConstraintPart  constraint;
  XmManagerPart   manager;
  XmHandlePart    handle;
} XmHandleRec;

#endif /* HANDLEP_H */

