/*
 * @(#)awt.h	1.6 95/11/27 Arthur van Hoff
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

/*
 * Common AWT definitions
 */

#ifndef _AWT_
#define _AWT_

#include "oobj.h"
#include "interpreter.h"
#include "exceptions.h"
#include "monitor.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifdef NETSCAPE
#include "prmon.h"

extern PRMonitor *_pr_rusty_lock;
extern void PR_XWait(int ms);
extern void PR_XNotify(void);
extern void PR_XNotifyAll(void);

#define AWT_LOCK() PR_XLock()

#define AWT_UNLOCK() PR_XUnlock()

#define AWT_FLUSH_UNLOCK() XFlush(awt_display); PR_XUnlock()

#define AWT_MAYBE_FLUSH_UNLOCK(f) if (f) XFlush(awt_display); PR_XUnlock()

#define AWT_WAIT(tm) PR_XWait(tm)

#define AWT_NOTIFY() PR_XNotify()

#define AWT_NOTIFY_ALL() PR_XNotifyAll()

#else

#ifdef DEBUG
#define DEBUG_AWT_LOCK
#endif

#ifdef DEBUG_AWT_LOCK

extern int awt_locked;
extern char *lastF;
extern int lastL;
#define AWT_LOCK()\
if (awt_lock == 0) {\
printf("AWT lock error, awt_lock is null\n");\
}\
monitorEnter(obj_monitor(awt_lock));\
if (awt_locked != 0) {\
printf("AWT lock error (%s,%d) (last held by %s,%d) %d\n",\
__FILE__, __LINE__,lastF,lastL,awt_locked);\
}\
lastF=__FILE__;\
lastL=__LINE__;\
awt_locked++

#define AWT_UNLOCK()\
lastF=""; lastL=-1; awt_locked--;\
if (awt_locked != 0) {\
printf("AWT unlock error (%s,%d,%d)\n", __FILE__,__LINE__,awt_locked);\
}\
monitorExit(obj_monitor(awt_lock))

#define AWT_MAYBE_FLUSH_UNLOCK(f) AWT_FLUSH_UNLOCK()

#define AWT_FLUSH_UNLOCK()\
awt_output_flush(); lastF=""; lastL=-1; awt_locked--;\
if (awt_locked != 0) {\
printf("AWT unlock error (%s,%d,%d)\n", __FILE__,__LINE__,awt_locked);\
}\
monitorExit(obj_monitor(awt_lock))

#define AWT_WAIT(tm)\
if (awt_locked != 1) {\
printf("AWT wait error (%s,%d,%d)\n", __FILE__,__LINE__,awt_locked);\
}\
awt_locked--; monitorWait(obj_monitor(awt_lock), (tm)); awt_locked++

#define AWT_NOTIFY()\
if (awt_locked != 1) {\
printf("AWT notify error (%s,%d,%d)\n", __FILE__,__LINE__,awt_locked);\
}\
monitorNotify(obj_monitor(awt_lock))

#define AWT_NOTIFY_ALL()\
if (awt_locked != 1) {\
printf("AWT notify all error (%s,%d,%d)\n", __FILE__,__LINE__,awt_locked);\
}\
monitorNotifyAll(obj_monitor(awt_lock))

#else

#define AWT_LOCK()\
monitorEnter(obj_monitor(awt_lock))

#define AWT_UNLOCK()\
monitorExit(obj_monitor(awt_lock))

#define AWT_FLUSH_UNLOCK()\
awt_output_flush(); monitorExit(obj_monitor(awt_lock))

#define AWT_WAIT(tm)\
monitorWait(obj_monitor(awt_lock), (tm))

#define AWT_NOTIFY()\
monitorNotify(obj_monitor(awt_lock))

#define AWT_NOTIFY_ALL()\
monitorNotifyAll(obj_monitor(awt_lock))

#endif				/* DEBUG_AWT_LOCK */

#endif /* NETSCAPE */

#define JAVA_UPCALL(args)\
AWT_UNLOCK();\
execute_java_dynamic_method args;\
if (exceptionOccurred(EE())) {\
exceptionDescribe(EE());\
exceptionClear(EE());\
}\
AWT_LOCK();

/* Call a Java dynamic method and expect a return value.  Rettype should be an
 * expression to cast the long returned by execute_java_dynamic_method() to the
 * type of retval.
 */
#define JAVA_UPCALL_WITH_RETURN(args,retval,rettype)\
AWT_UNLOCK();\
retval = rettype execute_java_dynamic_method args;\
if (exceptionOccurred(EE())) {\
exceptionDescribe(EE());\
exceptionClear(EE());\
}\
AWT_LOCK();

#define JAVA_UPCALL_UNLOCKED(args)\
execute_java_dynamic_method args;\
if (exceptionOccurred(EE())) {\
exceptionDescribe(EE());\
exceptionClear(EE());\
}

extern void		*awt_lock;
extern Display		*awt_display;
extern Visual		*awt_visual;
extern GC		awt_maskgc;
extern Window		awt_root;
extern long		awt_screen;
extern int		awt_depth;
extern Colormap		awt_cmap;
extern XVisualInfo	awt_visInfo;
extern int		awt_whitepixel;
extern int		awt_blackpixel;
extern int		awt_multiclick_time;

struct Hjava_awt_Color;
struct Hjava_awt_Font;

extern int awt_getColor(struct Hjava_awt_Color *this);
extern XFontStruct *awt_getFont(struct Hjava_awt_Font *this);
extern int awt_allocate_colors(void);
extern struct Hjava_awt_image_ColorModel *awt_getColorModel(void);

#endif
