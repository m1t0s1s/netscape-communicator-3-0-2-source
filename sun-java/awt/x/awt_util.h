/*
 * @(#)awt_util.h	1.9 95/11/29 Sami Shaio
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */
#ifndef _AWT_UTIL_H_
#define _AWT_UTIL_H_

Widget awt_util_createWarningWindow(Widget parent, char *warning);
void awt_util_show(Widget w);
void awt_util_hide(Widget w);
void awt_util_enable(Widget w);
void awt_util_disable(Widget w);
void awt_util_reshape(Widget w, long x, long y, long wd, long h);
void awt_util_mapChildren(Widget w, void (*func)(Widget,void *),
			  int applyToSelf, void *data);

struct DPos {
    long x;
    long y;
    int mapped;
    void *data;
    void *peer;
};

#endif           /* _AWT_UTIL_H_ */
