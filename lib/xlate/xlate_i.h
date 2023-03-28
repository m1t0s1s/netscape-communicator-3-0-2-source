#include "xp.h"
#include "xlate.h"

#define N_FONTS 8

typedef struct {
    short llx, lly, urx, ury;
} PS_BBox;

typedef struct {
	short wx, wy;
	PS_BBox charBBox;
} PS_CharInfo;

typedef struct {
    char *name;
    PS_BBox fontBBox;
    short upos, uthick;
    PS_CharInfo chars[256];
} PS_FontInfo;

#define MAKE_FE_FUNCS_PREFIX(f) TXFE_##f
#define MAKE_FE_FUNCS_EXTERN
#include "mk_cx_fn.h"

extern PS_FontInfo *PSFE_MaskToFI[N_FONTS];

#define LINE_WIDTH 160

#define TEXT_WIDTH 8
#define TEXT_HEIGHT 16

#define MAKE_FE_FUNCS_PREFIX(f) PSFE_##f
#define MAKE_FE_FUNCS_EXTERN
#include "mk_cx_fn.h"

extern void xl_begin_document(MWContext*);
extern void xl_begin_page(MWContext*,int);
extern void xl_end_page(MWContext*,int);
extern void xl_end_document(MWContext*);
extern void xl_show(MWContext *cx, char* txt, int len, char*);
extern void xl_moveto(MWContext* cx, int x, int y);
extern void xl_circle(MWContext* cx, int w, int h);
extern void xl_box(MWContext* cx, int w, int h);
extern void xl_stroke(MWContext*);
extern void xl_fill(MWContext*);
extern void xl_colorimage(MWContext *, int x, int y, int w, int h, void* fe_data);
extern void xl_wait_for_image(MWContext*, IL_Image*);
extern void xl_begin_squished_text(MWContext*, float);
extern void xl_end_squished_text(MWContext*);
extern void xl_initialize_translation(MWContext*, PrintSetup*);
extern void xl_finalize_translation(MWContext*);
extern void xl_arrived(MWContext*, IL_Image*);
extern void xl_annotate_page(MWContext*, char*, int, int, int);
extern void xl_draw_border(MWContext *, int , int , int , int , int );
extern XP_Bool xl_item_span(MWContext* cx, int top, int bottom);

struct LineRecord_struct;
