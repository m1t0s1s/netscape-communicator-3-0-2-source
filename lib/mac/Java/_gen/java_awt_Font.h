/* DO NOT EDIT THIS FILE - it is machine generated */
#include "native.h"
/* Header for class java_awt_Font */

#ifndef _Included_java_awt_Font
#define _Included_java_awt_Font
struct Hjava_lang_String;

typedef struct Classjava_awt_Font {
#define java_awt_Font_PLAIN	0L
#define java_awt_Font_BOLD	1L
#define java_awt_Font_ITALIC	2L
    long pData;
    struct Hjava_lang_String *family;
    struct Hjava_lang_String *name;
    long style;
    long size;
} Classjava_awt_Font;
HandleTo(java_awt_Font);

extern void java_awt_Font_dispose(struct Hjava_awt_Font* self);
#endif
