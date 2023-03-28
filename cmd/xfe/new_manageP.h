/* -*- Mode: C; tab-width: 8 -*-
   new_manage.c --- defines a subclass of XmScrolledWindow
   Copyright © 1994 Netscape Communications Corporation, all rights reserved.
   Created: Eric Bina <ebina@netscape.com>, 17-Aug-94.
 */

#ifndef _FE_NEWMANAGEP_H_
#define _FE_NEWMANAGEP_H_

#include "new_manage.h"
#include <Xm/ManagerP.h>

typedef struct
{
  int frogs;
} NewManageClassPart;

typedef struct _NewManageClassRec
{
  CoreClassPart	core_class;
  CompositeClassPart		composite_class;
  ConstraintClassPart		constraint_class;
  XmManagerClassPart		manager_class;
  NewManageClassPart		newManage_class;
} NewManageClassRec;

extern NewManageClassRec newManageClassRec;

typedef struct 
{
  void *why;
  XtCallbackList                input_callback;
} NewManagePart;

typedef struct _NewManageRec
{
    CorePart			core;
    CompositePart		composite;
    ConstraintPart		constraint;
    XmManagerPart		manager;
    NewManagePart		newManage;
} NewManageRec;

#endif /* _FE_NEWMANAGEP_H_ */

