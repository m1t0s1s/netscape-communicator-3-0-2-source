/* DO NOT EDIT THIS FILE - it is machine generated */
#include "native.h"
/* Header for class sun_awt_motif_MLabelPeer */

#ifndef _Included_sun_awt_motif_MLabelPeer
#define _Included_sun_awt_motif_MLabelPeer
struct Hjava_awt_Component;

typedef struct Classsun_awt_motif_MLabelPeer {
    struct Hjava_awt_Component *target;
    long pData;
} Classsun_awt_motif_MLabelPeer;
HandleTo(sun_awt_motif_MLabelPeer);

struct Hsun_awt_motif_MComponentPeer;
extern void sun_awt_motif_MLabelPeer_create(struct Hsun_awt_motif_MLabelPeer* self,struct Hsun_awt_motif_MComponentPeer *);
struct Hjava_lang_String;
extern void sun_awt_motif_MLabelPeer_setText(struct Hsun_awt_motif_MLabelPeer* self,struct Hjava_lang_String *);
extern void sun_awt_motif_MLabelPeer_setAlignment(struct Hsun_awt_motif_MLabelPeer* self,long);
#endif