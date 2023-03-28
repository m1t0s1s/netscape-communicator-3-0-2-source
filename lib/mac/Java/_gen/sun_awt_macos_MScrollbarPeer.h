/* DO NOT EDIT THIS FILE - it is machine generated */
#include "native.h"
/* Header for class sun_awt_macos_MScrollbarPeer */

#ifndef _Included_sun_awt_macos_MScrollbarPeer
#define _Included_sun_awt_macos_MScrollbarPeer
struct Hjava_awt_Component;
struct Hjava_awt_Color;

typedef struct Classsun_awt_macos_MScrollbarPeer {
    struct Hjava_awt_Component *target;
    long pData;
    long pInternationalData;
    long mOwnerPane;
    long mFont;
    long mSize;
    long mStyle;
    struct Hjava_awt_Color *mForeColor;
    struct Hjava_awt_Color *mBackColor;
    /*boolean*/ long mRecalculateClip;
    /*boolean*/ long mIsContainer;
    /*boolean*/ long mIsButton;
    /*boolean*/ long mIsChoice;
    /*boolean*/ long mIsResizableFrame;
    long mObscuredRgn;
    long mClipRgn;
    long mLineIncrement;
    long mPageIncrement;
} Classsun_awt_macos_MScrollbarPeer;
HandleTo(sun_awt_macos_MScrollbarPeer);

struct Hsun_awt_macos_MComponentPeer;
extern void sun_awt_macos_MScrollbarPeer_create(struct Hsun_awt_macos_MScrollbarPeer* self,struct Hsun_awt_macos_MComponentPeer *);
extern void sun_awt_macos_MScrollbarPeer_setValue(struct Hsun_awt_macos_MScrollbarPeer* self,long);
extern void sun_awt_macos_MScrollbarPeer_setValues(struct Hsun_awt_macos_MScrollbarPeer* self,long,long,long,long);
extern void sun_awt_macos_MScrollbarPeer_setLineIncrement(struct Hsun_awt_macos_MScrollbarPeer* self,long);
extern void sun_awt_macos_MScrollbarPeer_setPageIncrement(struct Hsun_awt_macos_MScrollbarPeer* self,long);
#endif