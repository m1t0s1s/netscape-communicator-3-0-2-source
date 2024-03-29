/* DO NOT EDIT THIS FILE - it is machine generated */
#include "native.h"
/* Header for class sun_awt_motif_InputThread */

#ifndef _Included_sun_awt_motif_InputThread
#define _Included_sun_awt_motif_InputThread
struct Hjava_lang_Thread;
struct Hjava_lang_Runnable;
struct Hjava_lang_ThreadGroup;

typedef struct Classsun_awt_motif_InputThread {
    struct HArrayOfChar *name;
    long priority;
    struct Hjava_lang_Thread *threadQ;
    long PrivateInfo;
    long eetop;
    /*boolean*/ long single_step;
    /*boolean*/ long daemon;
    /*boolean*/ long stillborn;
    struct Hjava_lang_Runnable *target;
    /*boolean*/ long interruptRequested;
/* Inaccessible static: activeThreadQ */
    struct Hjava_lang_ThreadGroup *group;
/* Inaccessible static: threadInitNumber */
#define sun_awt_motif_InputThread_MIN_PRIORITY	1L
#define sun_awt_motif_InputThread_NORM_PRIORITY	5L
#define sun_awt_motif_InputThread_MAX_PRIORITY	10L
} Classsun_awt_motif_InputThread;
HandleTo(sun_awt_motif_InputThread);

extern void sun_awt_motif_InputThread_run(struct Hsun_awt_motif_InputThread* self);
#endif
