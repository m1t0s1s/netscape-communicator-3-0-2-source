/* DO NOT EDIT THIS FILE - it is machine generated */
#include "native.h"
/* Header for class sun_awt_macos_MacFontMetrics */

#ifndef _Included_sun_awt_macos_MacFontMetrics
#define _Included_sun_awt_macos_MacFontMetrics
struct Hjava_awt_Font;

typedef struct Classsun_awt_macos_MacFontMetrics {
    struct Hjava_awt_Font *font;
    struct HArrayOfInt *widths;
    long ascent;
    long descent;
    long leading;
    long height;
    long maxAscent;
    long maxDescent;
    long maxHeight;
    long maxAdvance;
/* Inaccessible static: table */
} Classsun_awt_macos_MacFontMetrics;
HandleTo(sun_awt_macos_MacFontMetrics);

struct Hjava_lang_String;
extern long sun_awt_macos_MacFontMetrics_stringWidth(struct Hsun_awt_macos_MacFontMetrics* self,struct Hjava_lang_String *);
extern long sun_awt_macos_MacFontMetrics_charsWidth(struct Hsun_awt_macos_MacFontMetrics* self,HArrayOfChar *,long,long);
extern long sun_awt_macos_MacFontMetrics_bytesWidth(struct Hsun_awt_macos_MacFontMetrics* self,HArrayOfByte *,long,long);
extern void sun_awt_macos_MacFontMetrics_init(struct Hsun_awt_macos_MacFontMetrics* self);
#endif
