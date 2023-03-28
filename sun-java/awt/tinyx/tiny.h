/*
 * @(#)tiny.h	1.3 95/11/04 Arthur van Hoff
 *
 * Copyright (c) 1995 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _TINY_
#define _TINY_

#include "awt.h"

#define Color 			struct Hjava_awt_Color
#define Graphics 			struct Hjava_awt_Graphics
#define Font	 		struct Hjava_awt_Font
#define TinyToolkit 		struct Hsun_awt_tiny_TinyToolkit
#define TinyGraphics 		struct Hsun_awt_tiny_TinyGraphics
#define TinyFontMetrics 	struct Hsun_awt_tiny_TinyFontMetrics
#define TinyWindow 		struct Hsun_awt_tiny_TinyWindow
#define TinyEventThread 	struct Hsun_awt_tiny_TinyEventThread
#define TinyInputThread 	struct Hsun_awt_tiny_TinyInputThread

#define INVOKE(args) JAVA_UPCALL(args)

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

struct Hjava_awt_Graphics;
struct Hjava_awt_Color;
struct Hjava_awt_Font;
struct Hsun_awt_tiny_TinyToolkit;
struct Hsun_awt_tiny_TinyGraphics;
struct Hsun_awt_tiny_TinyFontMetrics;
struct Hsun_awt_tiny_TinyWindow;
struct Hsun_awt_tiny_TinyEventThread;
struct Hsun_awt_tiny_TinyInputThread;

extern void tiny_register(TinyWindow *win);
extern void tiny_unregister(TinyWindow *win);
extern void tiny_flush(void);

#endif

