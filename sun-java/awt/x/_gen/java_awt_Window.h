/* DO NOT EDIT THIS FILE - it is machine generated */
#include "native.h"
/* Header for class java_awt_Window */

#ifndef _Included_java_awt_Window
#define _Included_java_awt_Window
struct Hjava_awt_peer_ComponentPeer;
struct Hjava_awt_Container;
struct Hjava_awt_Color;
struct Hjava_awt_Font;
struct Hjava_awt_Component;
struct Hjava_awt_LayoutManager;
struct Hjava_lang_String;

typedef struct Classjava_awt_Window {
    struct Hjava_awt_peer_ComponentPeer *peer;
    struct Hjava_awt_Container *parent;
    long x;
    long y;
    long width;
    long height;
    struct Hjava_awt_Color *foreground;
    struct Hjava_awt_Color *background;
    struct Hjava_awt_Font *font;
    /*boolean*/ long visible;
    /*boolean*/ long enabled;
    /*boolean*/ long valid;
    long ncomponents;
    struct HArrayOfObject *component;
    struct Hjava_awt_LayoutManager *layoutMgr;
    struct Hjava_lang_String *warningString;
} Classjava_awt_Window;
HandleTo(java_awt_Window);

#endif