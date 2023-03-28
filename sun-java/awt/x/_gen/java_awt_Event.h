/* DO NOT EDIT THIS FILE - it is machine generated */
#include "native.h"
/* Header for class java_awt_Event */

#ifndef _Included_java_awt_Event
#define _Included_java_awt_Event
struct Hjava_lang_Object;
struct Hjava_awt_Event;

typedef struct Classjava_awt_Event {
    long data;
#define java_awt_Event_SHIFT_MASK	1L
#define java_awt_Event_CTRL_MASK	2L
#define java_awt_Event_META_MASK	4L
#define java_awt_Event_ALT_MASK	8L
#define java_awt_Event_HOME	1000L
#define java_awt_Event_END	1001L
#define java_awt_Event_PGUP	1002L
#define java_awt_Event_PGDN	1003L
#define java_awt_Event_UP	1004L
#define java_awt_Event_DOWN	1005L
#define java_awt_Event_LEFT	1006L
#define java_awt_Event_RIGHT	1007L
#define java_awt_Event_F1	1008L
#define java_awt_Event_F2	1009L
#define java_awt_Event_F3	1010L
#define java_awt_Event_F4	1011L
#define java_awt_Event_F5	1012L
#define java_awt_Event_F6	1013L
#define java_awt_Event_F7	1014L
#define java_awt_Event_F8	1015L
#define java_awt_Event_F9	1016L
#define java_awt_Event_F10	1017L
#define java_awt_Event_F11	1018L
#define java_awt_Event_F12	1019L
#define java_awt_Event_WINDOW_EVENT	200L
#define java_awt_Event_WINDOW_DESTROY	201L
#define java_awt_Event_WINDOW_EXPOSE	202L
#define java_awt_Event_WINDOW_ICONIFY	203L
#define java_awt_Event_WINDOW_DEICONIFY	204L
#define java_awt_Event_WINDOW_MOVED	205L
#define java_awt_Event_KEY_EVENT	400L
#define java_awt_Event_KEY_PRESS	401L
#define java_awt_Event_KEY_RELEASE	402L
#define java_awt_Event_KEY_ACTION	403L
#define java_awt_Event_KEY_ACTION_RELEASE	404L
#define java_awt_Event_MOUSE_EVENT	500L
#define java_awt_Event_MOUSE_DOWN	501L
#define java_awt_Event_MOUSE_UP	502L
#define java_awt_Event_MOUSE_MOVE	503L
#define java_awt_Event_MOUSE_ENTER	504L
#define java_awt_Event_MOUSE_EXIT	505L
#define java_awt_Event_MOUSE_DRAG	506L
#define java_awt_Event_SCROLL_EVENT	600L
#define java_awt_Event_SCROLL_LINE_UP	601L
#define java_awt_Event_SCROLL_LINE_DOWN	602L
#define java_awt_Event_SCROLL_PAGE_UP	603L
#define java_awt_Event_SCROLL_PAGE_DOWN	604L
#define java_awt_Event_SCROLL_ABSOLUTE	605L
#define java_awt_Event_LIST_EVENT	700L
#define java_awt_Event_LIST_SELECT	701L
#define java_awt_Event_LIST_DESELECT	702L
#define java_awt_Event_MISC_EVENT	1000L
#define java_awt_Event_ACTION_EVENT	1001L
#define java_awt_Event_LOAD_FILE	1002L
#define java_awt_Event_SAVE_FILE	1003L
#define java_awt_Event_GOT_FOCUS	1004L
#define java_awt_Event_LOST_FOCUS	1005L
    struct Hjava_lang_Object *target;
    int64_t when;
    long id;
    long x;
    long y;
    long key;
    long modifiers;
    long clickCount;
    struct Hjava_lang_Object *arg;
    struct Hjava_awt_Event *evt;
} Classjava_awt_Event;
HandleTo(java_awt_Event);

#endif