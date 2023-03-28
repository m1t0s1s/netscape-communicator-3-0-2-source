/* -*- Mode: C; tab-width: 4 -*-
 *   if.h --- Top-level image library internal routines
 *   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: if.h,v 1.112 1996/06/07 06:07:01 fur Exp $
 */

#ifndef _if_h
#define _if_h

#include "xp.h"
#include "net.h"
#include "il.h"

#ifdef DEBUG
#ifndef XP_MAC
#define Debug 1
#else
extern int Debug;
#endif
#endif

#ifdef XP_WIN
#define _USD 1              /* scanlines upside-down */ 
#endif

#ifdef DEBUG
#define ILTRACE(l,t) { if(il_debug>l) {XP_TRACE(t);} }
#else
#define ILTRACE(l,t) {}
#endif

#define FREE_IF_NOT_NULL(x)    do {if (x) {XP_FREE(x); (x) = NULL;}} while (0)

#ifndef XP_MAC
#    include "nspr/prcpucfg.h"  /* for IS_LITTLE_ENDIAN / IS_BIG_ENDIAN */
#else
#    include "prcpucfg.h"
#endif

/* The imagelib labels bits in a 32-bit word from 31 on the left to 0 on the right.
   This macro performs the necessary conversion to make that definition work on
   little-endian platforms */
#if defined(IS_LITTLE_ENDIAN)
#    define M32(bit)  ((bit) ^ 0x18)
#elif defined(IS_BIG_ENDIAN)
#    define M32(bit)  (bit)
#else
     ENDIANNESS UNKNOWN!
#endif

/* Don't change these unless you know what you're doing or you will
   break 16-bit binaries. */
#define MAX_IMAGE_WIDTH  8000
#define MAX_IMAGE_HEIGHT 8000

/* Last output pass of an image */
#define IL_FINAL_PASS     -1

typedef void (*il_converter)(il_container *ic, const uint8 *mask, 
                             const uint8 *sp, int x_offset,
                             int num, void XP_HUGE *out);

enum icstate {
    IC_VIRGIN       = 0x00, /* Newly-created container */
    IC_START        = 0x01, /* Requested stream from netlib, but no data yet */
    IC_STREAM       = 0x02, /* Stream opened, but insufficient data
                               received to determine image size  */ 
    IC_SIZED        = 0x04, /* Image size determined - still loading */
    IC_HIRES        = 0x05, /* Same as IC_SIZED, but for second
                               image in hi/lo source */
    IC_MULTI        = 0x06, /* Same as IC_SIZED, but for second or
                               subsequent images in multipart MIME */
    IC_NOCACHE      = 0x11, /* Image deferred for loading later */
    IC_COMPLETE     = 0x20, /* Image loaded - no errors */
    IC_BAD          = 0x21, /* Corrupt or illegal image data */
    IC_INCOMPLETE   = 0x22, /* Partially loaded image data */
    IC_MISSING      = 0x23, /* No such file on server */
    IC_ABORT_PENDING= 0x24  /* Image download abort in progress */
};

/* Still receiving data from the netlib ? */
#define IMAGE_CONTAINER_LOADING(ic)  ((ic)->state <= IC_MULTI)

/* Force memory cache to be flushed ? */
#define FORCE_RELOAD(reload_method)                                           \
 (((reload_method)==NET_NORMAL_RELOAD) || ((reload_method)==NET_SUPER_RELOAD))

/* There's one il_client per copy of an image on screen. */
typedef struct il_client_struct il_client;

struct il_client_struct {
    MWContext     *cx;
    void          *client;
    PRPackedBool   stopped;     /* TRUE - if user hit "Stop" button */
    int            is_view_image; /* non-zero if client is
                                     internal-external-reconnect */
    il_client     *next;
};

/* Simple list of image containers */
struct il_container_list {
    il_container *ic;
    struct il_container_list *next;
};


/* There is one il_container per real image */
struct il_container_struct {
    il_container *next;         /* Cache bidirectional linked list */
    il_container *prev;

    URL_Struct *url;
    char *hiurl;
    char *url_address;          /* Same as url->address */

    uint32 hash;
    uint32 urlhash;
    MWContextType context_type; /* Differentiate window/printer contexts */

    enum icstate state;
    int sized;

    int is_backdrop;            /* Backdrop, user defined or not */
    int is_user_backdrop;       /* User-defined backdrop */
    int is_alone;               /* only image on a page */
    int is_visible_on_screen;   /* Duh, what do you think ? */
    int is_in_use;              /* Used by some context */
    int32 loop_count;           /* Remaining number of times to repeat image,
                                   -1 means loop infinitely */
    int is_looping;             /* TRUE, if anim displayed more than once */
    int multi;                  /* Known to be either multipart-MIME 
                                   or multi-image format */
    int new_data_for_fe;        /* Any Scanlines that FE doesn't know about ? */
    int update_start_row;       /* Scanline range to send to FE */
    int update_end_row;

    uint32 bytes_consumed;      /* Bytes read from the stream so far */

    IL_Image *image;

    intn type;
    void *ds;                   /* decoder's private data */

    il_converter converter;
    void *quantize;             /* quantizer's private data */

    int (*write)(il_container *ic, const uint8*, int32);
    void (*complete)(il_container *ic);
    void (*abort)(il_container *ic);

    unsigned int (*write_ready)(il_container *ic);

    void *row_output_timeout;
    uint8 *scalerow;
    int pass;                   /* pass (scan #) of a multi-pass image.
                                   Used for interlaced GIFs & p-JPEGs */

    il_process *ip;
    il_colorspace *cs;
    NET_StreamClass *stream;    /* back pointer to the stream */

    int forced;
    uint32 content_length;

    int unscaled_height, unscaled_width; /* Native, original image size */
    int layout_width, layout_height; /* Size requested by layout */
    char *comment;              /* Human-readable text stored in image */
    int comment_length;

    int colormap_serial_num;    /* serial number of last installed colormap */

    uint8 *indirect_map;
    int dont_use_custom_palette;
    int rendered_with_custom_palette;
    IL_DitherMode dither_mode;  /* ilDither or ilClosestColor */

    MWContext *cx;              /* Last context this image was displayed in */
    il_client *clients;         /* List of layout images */
    il_client *lclient;         /* Last in list of layout images */

	time_t expires;             /* Expiration date for the corresponding URL */
#ifdef DEBUG
    time_t start_time;
#endif
};


typedef enum { ilUndefined, ilCI, ilGrey, ilRGB } il_mode;

struct il_colorspace_struct
{
    /* Per-colormap state */
    int unique_images;
    int install_colormap_allowed;
    int install_colormap_forbidden;

    /* Configuration */
    il_mode mode;               /* PseudoColor, 24-bit RGB, etc. */
    int depth;                  /* Screen depth, in bits */
    int bytes_per_pixel;        /* Ranges from 0 to 4. (0 is B&W display) */
    IL_ColorRenderMode color_render_mode; /* color-cube, custom colormap, etc.*/

    /* Palette stuff */
    int colormap_serial_num;    /* serial number of installed colormap */
    IL_IRGB *current_map;
    uint8 *current_indirect_map;

    IL_IRGB *default_map;       /* default ci map */
    uint8 *default_indirect_map;
    int default_map_size;

    int alloc_base;
    uint8 *rmap, *gmap, *bmap;  /* Color quantization for pseudocolor displays */
    uint8 **cmap;
    
    /* RGB maps */
    void *rtom, *gtom, *btom;   /* Converting RGB from one depth to another */

    int num_contexts;           /* Number of contexts sharing this colorspace */
    XP_List *contexts;          /* List of contexts that use this colorspace */
};

/*
 *  il_process is global state per window
 */
struct il_process_struct {
    /* Preferences */
    int            dontscale;   /* Used for Macs, which do their own scaling */
    int            nolowsrc;    /* If TRUE, never display LOSRC images */
    int            progressive_display;    /* If TRUE, images displayed
                                              while loading */
    IL_DitherMode  dither_mode; /* On, off, automatic, etc. */

    /* Per-context state */
    IL_IRGB        transparent; /* Background color or transparent index */
    int            backdrop_loading; /* If TRUE, a backdrop is being decoded */
    int            active;      /* # of onscreen images decoding */
};

typedef enum il_draw_mode 
{
    ilErase,                    /* Transparent areas are reset to background */
    ilOverlay                   /* Transparent areas overlay existing data */
} il_draw_mode;
    
extern il_process *il_last_ip;
extern il_colorspace *il_last_cs;
extern int il_debug;



extern void il_delete_container(il_container *ic, int explicitly);
extern il_container *il_removefromcache(il_container *ic);
extern void il_image_abort(il_container *ic);
extern PRBool il_image_stopped(il_container *ic);

extern void il_image_complete(il_container *ic);
extern void il_stream_complete(void *data_object);
extern void il_abort(void *data_object, int status);
extern unsigned int il_write_ready(void *data_object);
extern int il_first_write(void *dobj, const unsigned char *str, int32 len);

extern int  il_gif_init(il_container *ic);
extern int  il_gif_write(il_container *, const uint8 *, int32);
extern void il_gif_complete(il_container *ic);
extern int  il_gif_compute_percentage_complete(int row, il_container *ic);
extern unsigned int il_gif_write_ready(il_container *ic);
extern void il_gif_abort(il_container *ic);

extern int  il_xbm_init(il_container *ic);
extern int  il_xbm_write(il_container *, const uint8 *, int32);
extern void il_xbm_complete(il_container *ic);
extern void il_xbm_abort(il_container *ic);

extern int  il_jpeg_init(il_container *ic);
extern int  il_jpeg_write(il_container *, const uint8 *, int32);
extern void il_jpeg_complete(il_container *ic);
extern void il_jpeg_abort(il_container *ic);

extern int  il_size(il_container *, uint32 w, uint32 h);

extern int  il_setup_quantize(void);
extern int  il_init_quantize(il_container *ic);
extern void il_free_quantize(il_container *ic);
extern void il_quantize_fs_dither(il_container * ic,
                                  const uint8* mask,
                                  const uint8 *samp_in,
                                  int x_offset,
                                  uint8 XP_HUGE *samp_out,
                                  int cbase, int width);

extern void il_emit_row(il_container *ic, uint8 *buf, uint8 *rgbbuf,
                        int start_column, int len, int row, int row_count,
                        il_draw_mode draw_mode, int ipass);

extern void il_flush_image_data(il_container *ic,
                                IL_ImageStatus new_status);
extern void il_setup_color_space_converter(il_container *ic);

extern void il_convert_image_to_default_colormap(il_container *ic);

extern int  il_set_color_palette(MWContext *cx, il_container *ic);
extern void il_reset_palette(il_container *ic);

extern void il_reverse_bits(uint8 *buf, int n);
extern void il_reconnect(il_container *cx);
extern void il_abort_reconnect(void);
extern void il_set_view_image_doc_title(il_container *ic);
extern il_container *il_get_container(MWContext *cx,
                                      NET_ReloadMethod reload_cache_policy,
                                      const char *image_url,
                                      const char *lowres_image_url,
                                      IL_IRGB *background_color,
                                      IL_DitherMode dither_mode,
                                      Bool mask_required,
                                      int depth,
                                      int display_width,
                                      int display_height);

extern uint32 il_hash(const char *ubuf);
extern void il_update_thermometer(il_container *ic);
extern void il_partial(il_container *ic, int row, int row_count, int pass);
extern void il_scour_container(il_container *ic);
extern void il_adjust_cache_fullness(int32 bytes);
extern int  il_add_client(MWContext *cx, il_container *ic,
                          void* client, int is_view_image);
#endif /* _if_h */
