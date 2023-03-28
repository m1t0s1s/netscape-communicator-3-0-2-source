/* -*- Mode: C; tab-width: 8 -*-
   quickfile.h --- structures and prototypes exported for the quickfile button/menu.
   Copyright © 1994 Netscape Communications Corporation, all rights reserved.
   Created: Chris Toshok <toshok@netscape.com>, 10-Jun-1996
 */
#ifndef QUICKFILE_H
#define QUICKFILE_H

#include <X11/Intrinsic.h>

typedef struct fe_qf_struct *fe_QuickFile;

extern fe_QuickFile fe_QuickFile_Create(MWContext *context, Widget parent);
extern void fe_QuickFile_Destroy(fe_QuickFile quickfile);
extern void fe_QuickFile_Invalidate(fe_QuickFile quickfile);

#endif /* QUICKFILE_H */
