/* DO NOT EDIT THIS FILE - it is machine generated */
#include "native.h"
/* Header for class s_a_i_OffScreenImageSource */

#ifndef _Included_sun_awt_image_OffScreenImageSource
#define _Included_sun_awt_image_OffScreenImageSource
struct Hsun_awt_image_ImageRepresentation;
struct Hjava_awt_image_ImageConsumer;

typedef struct Classsun_awt_image_OffScreenImageSource {
    long width;
    long height;
    struct Hsun_awt_image_ImageRepresentation *baseIR;
    struct Hjava_awt_image_ImageConsumer *theConsumer;
} Classsun_awt_image_OffScreenImageSource;
HandleTo(sun_awt_image_OffScreenImageSource);

extern void sun_awt_image_OffScreenImageSource_sendPixels(struct Hsun_awt_image_OffScreenImageSource* self);
#endif
