/* -*- Mode: C; tab-width: 4 -*-
 *   gif.c --- GIF87 & GIF89 Image decoder
 *   Copyright � 1995 Netscape Communications Corporation, all rights reserved.
 *   $Id: gif.c,v 1.148.8.1 1996/09/27 23:53:09 blythe Exp $
 */

#include "xp.h"
#include "merrors.h"
#include "xp_core.h"
#include "il.h"
#include "if.h"


#ifndef MAX
#    define MAX(x, y)	(((x) > (y)) ? (x) : (y))
#endif

extern int MK_OUT_OF_MEMORY;

#ifdef DEBUG_jwz
extern XP_Bool IL_AnimationLoopsEnabled;
extern XP_Bool IL_AnimationsEnabled;
#endif /* DEBUG_jwz */


#ifdef PROFILE
#    pragma profile on
#endif

/* List of possible parsing states */
typedef enum {
	gif_gather,
	gif_init,					/*1*/
	gif_type,
	gif_version,
	gif_global_header,
	gif_global_colormap,
	gif_image_start,			/*6*/
	gif_image_header,
	gif_image_colormap,
	gif_image_body,
	gif_lzw_start,
	gif_lzw,					/*11*/
	gif_sub_block,
	gif_extension,
	gif_control_extension,
	gif_consume_block,
	gif_skip_block,
	gif_done,					/*17*/
	gif_oom,
	gif_error,
    gif_comment_extension,
    gif_application_extension,
    gif_netscape_extension_block,
    gif_consume_netscape_extension,
    gif_consume_comment,
    gif_delay,
    gif_wait_for_buffer_full
} gstate;

/* "Disposal" method indicates how the image should be handled in the
   framebuffer before the subsequent image is displayed. */
typedef enum 
{
    DISPOSE_NOT_SPECIFIED      = 0,
    DISPOSE_KEEP               = 1, /* Leave it in the framebuffer */
    DISPOSE_OVERWRITE_BGCOLOR  = 2, /* Overwrite with background color */
    DISPOSE_OVERWRITE_PREVIOUS = 4  /* Save-under */
} gdispose;

#define MAX_HOLD 768		/* for now must be big enough for a cmap */

#define	MAX_LZW_BITS	      12
#define MAX_BITS            4097 /* 2^MAX_LZW_BITS+1 */
#define MINIMUM_DELAY_TIME    10

/* A GIF decoder */
typedef struct
gif_struct {
	/* Parsing state machine */
	gstate state;               /* Curent decoder master state */
    uint8 *hold;                /* Accumulation buffer */
    int32 hold_size;            /* Capacity, in bytes, of accumulation buffer */
    uint8 *gather_head;         /* Next byte to read in accumulation buffer */
    int32 gather_request_size;  /* Number of bytes to accumulate */
	int32 gathered;             /* bytes accumulated so far*/
	gstate post_gather_state;   /* State after requested bytes accumulated */
    int32 requested_buffer_fullness; /* For netscape application extension */

	/* LZW decoder state machine */
    uint8 *stack;               /* Base of decoder stack */
	uint8 *stackp;              /* Current stack pointer */
	uint16 *prefix;
	uint8 *suffix;
	int datasize;
    int codesize;
    int codemask;
	int clear_code;             /* Codeword used to trigger dictionary reset */
    int avail;                  /* Index of next available slot in dictionary */
    int oldcode;
	uint8 firstchar;
	int count;                  /* Remaining # bytes in sub-block */
    int bits;                   /* Number of unread bits in "datum" */
	int32 datum;                /* 32-bit input buffer */

    /* Output state machine */
	int ipass;                  /* Interlace pass; Ranges 1-4 if interlaced. */
    uint rows_remaining;        /* Rows remaining to be output */
	uint irow;                  /* Current output row, starting at zero */
	uint8 *rgbrow;              /* Temporary storage for dithering/mapping */
	uint8 *rowbuf;              /* Single scanline temporary buffer */
	uint8 *rowend;              /* Pointer to end of rowbuf */
	uint8 *rowp;                /* Current output pointer */

    /* Image parameters */
    uint x_offset, y_offset;    /* With respect to "screen" origin */
	uint height, width;
    uint last_x_offset, last_y_offset; /* With respect to "screen" origin */
	uint last_height, last_width;
	int interlaced;             /* TRUE, if scanlines arrive interlaced order */
	int tpixel;                 /* Index of transparent pixel */
    int is_transparent;         /* TRUE, if tpixel is valid */
    int control_extension;      /* TRUE, if image control extension present */
    int is_local_colormap_defined;
    gdispose disposal_method;   /* Restore to background, leave in place, etc.*/
    gdispose last_disposal_method;
    IL_RGB *local_colormap;     /* Per-image colormap */
    int progressive_display;    /* If TRUE, do Haeberli interlace hack */
    uint32 delay_time;          /* Display time, in milliseconds,
                                   for this image in a multi-image GIF */

    /* Global (multi-image) state */
    int screen_bgcolor;         /* Logical screen background color */
	int version;                /* Either 89 for GIF89 or 87 for GIF87 */
    uint screen_width;          /* Logical screen width & height */
    uint screen_height;
    IL_RGB *global_colormap;    /* Default colormap if local not supplied  */
    int images_decoded;         /* Counts images for multi-part GIFs */
	il_container *ic;           /* Back pointer to requesting image_container */
    void *delay_timeout;        /* Used to delay after displaying picture */
    int destroy_pending;        /* Stream has ended */
} gif_struct;

/* Gather n characters from the input stream and then enter state s. */
#define GETN(n,s)                                                             \
do {gs->state=gif_gather; gs->gather_request_size = (n);                      \
    gs->post_gather_state = s;} while (0)

/* Get a 16-bit value stored in little-endian format */
#define GETINT16(p)   ((p)[1]<<8|(p)[0])

/* Get a 32-bit value stored in little-endian format */
#define GETINT32(p)   (((p)[3]<<24) | ((p)[2]<<16) | ((p)[1]<<8) | ((p)[0]))

/* Send the data to the display front-end. */
static void
output_row(gif_struct *gs)
{
    int width, drow_start, drow_end;
    
    drow_start = drow_end = gs->irow;

    /*
     * Haeberli-inspired hack for interlaced GIFs: Replicate lines while
     * displaying to diminish the "venetian-blind" effect as the image is
     * loaded. Adjust pixel vertical positions to avoid the appearance of the
     * image crawling up the screen as successive passes are drawn.
     */
	if (gs->progressive_display && gs->interlaced && (gs->ipass < 4))
	{
        uint row_dup, row_shift;

        switch (gs->ipass) {
        case 1:
            row_dup = 7;
            row_shift = 3;
            break;
        case 2:
            row_dup = 3;
            row_shift = 1;
            break;
        case 3:            
            row_dup = 1;
            row_shift = 0;
            break;
        default:
            XP_ABORT(("Illegal interlace pass"));
            break;
        }

		drow_start -= row_shift;
		drow_end = drow_start + row_dup;

		/* Extend if bottom edge isn't covered because of the shift upward. */
		if (((gs->height - 1) - drow_end) <= row_shift)
            drow_end = gs->height - 1;

        /* Clamp first and last rows to upper and lower edge of image. */
        if (drow_start < 0)
            drow_start = 0;
        if ((uint)drow_end >= gs->height)
            drow_end = gs->height - 1;
    }

    /* Check for scanline below edge of logical screen */
    if ((gs->y_offset + gs->irow) < gs->screen_height) {
        il_draw_mode draw_mode;

        if (gs->images_decoded >= 1)
            draw_mode = ilOverlay;
        else
            draw_mode = ilErase;

        /* Clip if right edge of image exceeds limits */
        if ((gs->x_offset + gs->width) > gs->screen_width)
            width = gs->screen_width - gs->x_offset;
        else
            width = gs->width;
/* for testing
        if ((gs->images_decoded >= 3) || (gs->ic->is_backdrop))
*/
        if (width > 0)
            il_emit_row(gs->ic, gs->rowbuf, gs->rgbrow, gs->x_offset, width,
                        gs->y_offset + drow_start, drow_end - drow_start + 1,
                        draw_mode, gs->ipass);
    }

	gs->rowp = gs->rowbuf;

	if(!gs->interlaced)
	{
		gs->irow++;
	}
	else
	{
		switch(gs->ipass)
		{
			case 1:
				gs->irow += 8;
				if(gs->irow >= gs->height)
				{
					gs->ipass++;
					gs->irow = 4;
				}
				break;

			case 2:
				gs->irow += 8;
				if(gs->irow >= gs->height)
				{
					gs->ipass++;
					gs->irow = 2;
				}
				break;

			case 3:
				gs->irow += 4;
				if(gs->irow >= gs->height)
				{
					gs->ipass++;
					gs->irow = 1;
				}
				break;

			case 4:
				gs->irow += 2;
				if(gs->irow >= gs->height)
					gs->ipass++;
				break;

			default:
				XP_ASSERT(0);
		}
	}
}

/* Perform Lempel-Ziv-Welch decoding */
static int 
do_lzw(gif_struct *gs, const uint8 *q)
{
    int code;
    int incode;
    const uint8 *ch;

    /* Copy all the decoder state variables into locals so the compiler
     * won't worry about them being aliased.  The locals will be homed
     * back into the GIF decoder structure when we exit.
     */
    int avail       = gs->avail;
    int bits        = gs->bits;
    int codesize    = gs->codesize;
    int codemask    = gs->codemask;
    int count       = gs->count;
    int oldcode     = gs->oldcode;
    int clear_code  = gs->clear_code;
    uint8 firstchar = gs->firstchar;
    int32 datum     = gs->datum;
    uint16 *prefix  = gs->prefix;
    uint8 *stackp   = gs->stackp;
    uint8 *suffix   = gs->suffix;
    uint8 *stack    = gs->stack;
    uint8 *rowp     = gs->rowp;
    uint8 *rowend   = gs->rowend;
    uint rows_remaining = gs->rows_remaining;

#define OUTPUT_ROW(gs)    													  \
    {																		  \
        output_row(gs);														  \
        rows_remaining--;													  \
        rowp = gs->rowp;													  \
        if (!rows_remaining)												  \
            goto END;														  \
    }

    for (ch=q; count-- > 0; ch++) 
    {
        /* Feed the next byte into the decoder's 32-bit input buffer. */
		datum += ((int32) *ch) << bits;
		bits += 8;
        
        /* Check for underflow of decoder's 32-bit input buffer. */
		while (bits >= codesize) 
		{
            /* Get the leading variable-length symbol from the data stream */
			code = datum & codemask;
			datum >>= codesize;
			bits -= codesize;

            /* Reset the dictionary to its original state, if requested */
			if (code == clear_code) 
			{
				codesize = gs->datasize + 1;
				codemask = (1 << codesize) - 1;
				avail = clear_code + 2;
				oldcode = -1;
				continue;
			}

            /* Check for explicit end-of-stream code */
			if (code == (clear_code + 1))
                return 0;

			if (oldcode == -1) 
			{
				*rowp++ = suffix[code];
				if (rowp == rowend) {
					OUTPUT_ROW(gs);
                }
                
				firstchar = oldcode = code;
				continue;
			}

            /* Check for a code not defined in the dictionary yet. */
			if (code > avail) 
			{
				ILTRACE(3,("il:gif: code too large %d %d", code, avail));
				return -1;
			}

			incode = code;
			if (code == avail) 
			{	   
				/* the first code is always < avail */
				*stackp++ = firstchar;
				code = oldcode;
			}

			while (code > clear_code) 
			{
				*stackp++ = suffix[code];
				code = prefix[code];
			}

            /* Define a new codeword in the dictionary. */
			*stackp++ = firstchar = suffix[code];
			prefix[avail] = oldcode;
			suffix[avail] = firstchar;
			avail++;

            /* If we've used up all the codewords of a given length
             * increase the length of codewords by one bit, but don't
             * exceed the specified maximum codeword size of 12 bits.
             */
			if (((avail & codemask) == 0) && (avail < 4096)) 
			{
				codesize++;
				codemask += avail;
			}
			oldcode = incode;
            
            /* Copy the decoded data out to the scanline buffer. */
			do {
				*rowp++ = *--stackp;
				if (rowp == rowend) {
					OUTPUT_ROW(gs);
                }
			} while (stackp > stack);
		}
    }

  END:

    /* Home the local copies of the GIF decoder state variables */
    gs->avail = avail;
    gs->bits = bits;
    gs->codesize = codesize;
    gs->codemask = codemask;
    gs->count = count;
    gs->oldcode = oldcode;
    gs->firstchar = firstchar;
    gs->datum = datum;
    gs->stackp = stackp;
    gs->rowp = rowp;
    gs->rows_remaining = rows_remaining;

    return 0;
}

/*
 * setup an ic for gif decoding
 */
int
il_gif_init(il_container *ic)
{
	gif_struct *gs;
	gs = XP_NEW_ZAP(gif_struct);
	if (gs) 
	{
		ic->ds = gs;
		gs->state = gif_init;
		gs->post_gather_state = gif_error;
		gs->ic = ic;
	}
	return gs != 0;
}

static int
il_gif_init_transparency(il_container *ic)
{
    IL_Image *im = ic->image;
    
    if (!im->transparent)
    {
        if ((im->transparent = XP_NEW_ZAP(IL_IRGB)) == NULL)
            return FALSE;
        
        /* Get context background color, if defined */
        im->transparent->red   = ic->ip->transparent.red;
        im->transparent->green = ic->ip->transparent.green;
        im->transparent->blue  = ic->ip->transparent.blue;
    }
    return TRUE;
}

int
il_gif_compute_percentage_complete(int row, il_container *ic)
{
    uint percent_height;
    int percent_done = 0;
    
    percent_height = (uint)(row * (uint32)100 / ic->image->height);
    switch(ic->pass) {
    case 0: percent_done = percent_height; /* non-interlaced GIF */
        break;
    case 1: percent_done =      percent_height / 8;
        break;
    case 2: percent_done = 12 + percent_height / 8;
        break;
    case 3: percent_done = 25 + percent_height / 4;
        break;
    case 4: percent_done = 50 + percent_height / 2;
        break;
    default:
        XP_ABORT(("Illegal interlace pass"));
        break;
    }

    return percent_done;
}

/* Maximum # of bytes to read ahead while waiting for delay_time to expire.
   We limit this number to remain within WIN16 malloc limitations */

#define MAX_READ_AHEAD  (60000L)

unsigned int
il_gif_write_ready(il_container *ic)
{
	gif_struct *gs = (gif_struct *)ic->ds;
    int32 max;
    
    if (!gs)
        return 1;               /* Let imglib generic code decide */

    max = MAX(MAX_READ_AHEAD, gs->requested_buffer_fullness);
    if (gs->gathered < max)
        return 1;               /* Let imglib generic code decide */
    else
        return 0;               /* No more data until timeout expires */
}


static void
process_buffered_gif_input_data(gif_struct* gs)
{
    gstate state;
	il_container *ic = gs->ic;

    /* Force any data we've buffered up to be processed. */
    il_gif_write(ic, (uint8 *) "", 0);

    /* The stream has already finished delivering data and the stream
       completion routine has been called sometime in the past. Now that
       we're actually done handling all of that data, call the stream
       completion routine again, but this time for real. */
    state = gs->state;
    if (gs->destroy_pending &&
        ((state == gif_done) || (state == gif_error) || (state == gif_oom))) {
        il_gif_abort(ic);
        il_image_complete(ic);
    }
}

static void
gif_delay_time_callback(void *closure)
{
	gif_struct *gs = (gif_struct *)closure;

    XP_ASSERT(gs->state == gif_delay);

#ifdef XP_WIN
    /*  3.01 bug fix WFE idle loop bug
     *  Need to remove this in Dogbert.
     */
    XP_ASSERT(gs->ic->cx);
    if(gs->ic->cx)  {
        FE_SetCallNetlibAllTheTime(gs->ic->cx);
    }
#endif

    gs->delay_timeout = NULL;

    if (gs->ic->state == IC_ABORT_PENDING)
        return;                                        

    gs->delay_time = 0;         /* Reset for next image */

    if (gs->state == gif_delay) {
        GETN(1, gif_image_start);
        process_buffered_gif_input_data(gs);
    }
}

/*
 * For the first images in the sequence clear the logical
 * screen to the background color, unless the first image
 * completely covers the logical screen, in which case
 * it's unnecessary.  XXX - This can be optimized.
 */

static int
gif_clear_screen(gif_struct *gs)
{
    uint erase_width, erase_height, erase_x_offset, erase_y_offset;
    XP_Bool erase;
    il_container *ic = gs->ic;

    erase = FALSE;
    if (gs->images_decoded == 0)
    {
        if ((gs->width  != gs->screen_width) ||
            (gs->height != gs->screen_height))
        {
            erase = TRUE;
            erase_width  = gs->screen_width;
            erase_height = gs->screen_height;
            erase_x_offset = erase_y_offset = 0;
        }
    }
    else
    {
        if (gs->last_disposal_method == DISPOSE_OVERWRITE_BGCOLOR)
        {
            erase = TRUE;
            erase_width    = gs->last_width;
            erase_height   = gs->last_height;
            erase_x_offset = gs->last_x_offset;
            erase_y_offset = gs->last_y_offset;
        }
    }

    gs->last_disposal_method = gs->disposal_method;
    gs->last_width = gs->width;
    gs->last_height = gs->height;
    gs->last_x_offset = gs->x_offset;
    gs->last_y_offset = gs->y_offset;
            
    if (erase)
    {
        uint i, index, save;
        uint8 *rowbuf = gs->rowbuf;
        IL_Image *im = ic->image;

        /* We have to temporarily pretend the image is transparent
           so we can clear using the context's background color */
        IL_IRGB *saved_transparent = im->transparent;

        /* Catch images that fall outside the logical screen. */
        if ((erase_x_offset + erase_width) > gs->screen_width)
            erase_width = gs->screen_width - erase_x_offset;

/* 	tj & fur (4/26/96)
	
	GIFs are not dithered on the Mac, so the transparent index is
 	the index specified in the image, not the index supplied by the FE,
 	as on the other platforms 
*/
 
#ifdef XP_MAC
		index = saved_transparent->index;
#else
        im->transparent = NULL;
        if (!il_gif_init_transparency(ic))
            return MK_OUT_OF_MEMORY; 
        index = im->transparent->index = ic->ip->transparent.index;
      

        /* God, this is ugly: force the background pixel color
           for the case when we have a custom colormap. */
        if (ic->indirect_map) {
            save = ic->indirect_map[index];
            ic->indirect_map[index] = index;
        }
#endif          
                   
        for (i = 0; i < erase_width; i++)
            rowbuf[i] = index;
                    
        /* Note: We deliberately lie about the interlace
           pass number so that calls to il_flush_image_data()
           are done using a timer. */
        if (erase_width > 0)
            il_emit_row(gs->ic, gs->rowbuf, gs->rgbrow, erase_x_offset,
                        erase_width, erase_y_offset, erase_height,
                        ilErase, 2);

#ifdef XP_MAC
		/* nothing to restore */
#else
        /* Restore the transparent color. */
        if (ic->indirect_map)
            ic->indirect_map[index] = save;
        
        XP_FREE(im->transparent);
        im->transparent = saved_transparent;
#endif
        
    }
    return 0;
}


/*
 * process data arriving from the stream for the gif decoder
 */
 
int
il_gif_write(il_container *ic, const uint8 *buf, int32 len)
{
    int status;
	gif_struct *gs = (gif_struct *)ic->ds;
	IL_Image *im = ic->image;
	const uint8 *q, *p=buf,*ep=buf+len;

    /* If this assert fires, chances are the netlib screwed up and
       continued to send data after the image stream was closed. */
    XP_ASSERT(gs);
    if (!gs) {
#ifdef DEBUG
        XP_TRACE(("Netlib just took a shit on the imagelib\n"));
#endif
        return MK_IMAGE_LOSSAGE;
    }

    /* If this assert fires, some upstream data provider ignored the
       zero return value from il_gif_write_ready() which says not to
       send any more data to this stream until the delay timeout fires. */
    XP_ASSERT ((len == 0) || (gs->gathered < MAX_READ_AHEAD));
    if (!((len == 0) || (gs->gathered < MAX_READ_AHEAD)))
        return MK_INTERRUPTED;
    
    q = NULL;                   /* Initialize to shut up gcc warnings */
	
	while (p <= ep)
	{
		ILTRACE(9,("il:gif: state %d len %d buf %u p %u q %u ep %u",
                   gs->state,len,buf,p,q,ep));
		switch(gs->state)
		{
        case gif_lzw:
            if (do_lzw(gs, q) < 0)
            {
                gs->state=gif_error;
                break;
            }
            GETN(1,gif_sub_block);
            break;
		
        case gif_lzw_start:
        {
            int i, status;

            im->map = gs->is_local_colormap_defined ?
                gs->local_colormap : gs->global_colormap;

            XP_ASSERT(im->map);
            if (!im->map)
                return MK_IMAGE_LOSSAGE;

            /* Now we know how many colors are in our colormap. */
            if (gs->is_local_colormap_defined || (gs->images_decoded == 0))
                il_setup_color_space_converter(ic);

            status = gif_clear_screen(gs);
            if (status < 0)
                return status;

            /* Initialize LZW parser/decoder */
            gs->datasize = *q;
            if(gs->datasize > MAX_LZW_BITS)
            {
                gs->state=gif_error;
                break;
            }

            gs->clear_code = 1 << gs->datasize;
            gs->avail = gs->clear_code + 2;
            gs->oldcode = -1;
            gs->codesize = gs->datasize + 1;
            gs->codemask = (1 << gs->codesize) - 1;

            gs->datum = gs->bits = 0;

			if (!gs->prefix)		
                gs->prefix = (uint16 *)XP_CALLOC(sizeof(uint16), MAX_BITS);
            if (!gs->suffix)
                gs->suffix = ( uint8 *)XP_CALLOC(sizeof(uint8),  MAX_BITS);
            if (!gs->stack)
                gs->stack  = ( uint8 *)XP_CALLOC(sizeof(uint8),  MAX_BITS);
					
            if( !gs->prefix || !gs->suffix || !gs->stack)
            {
                /* complete from abort will free prefix & suffix */
                ILTRACE(0,("il:gif: MEM stack"));
                gs->state=gif_oom;
                break;
            }

            if(gs->clear_code >= MAX_BITS)
            {
                gs->state=gif_error;
                break;
            }

            /* init the tables */
            for (i=0; i < gs->clear_code; i++) 
                gs->suffix[i] = i;

            gs->stackp = gs->stack;

            GETN(1,gif_sub_block);
        }
        break;

        /* We're positioned at the very start of the file. */
        case gif_init:
        {
            GETN(3,gif_type);
            break;
        }

        /* All GIF files begin with "GIF87a" or "GIF89a" */
        case gif_type:
        {
            if (strncmp((char*)q,"GIF",3))
            {
                ILTRACE(2,("il:gif: not a GIF file"));
                gs->state=gif_error;
                break;
            }
            GETN(3,gif_version);
        }
        break;

        case gif_version:
        {
            if(!strncmp((char*)q,"89a",3))
            {
                gs->version=89;
            }
            else
            {
                if(!strncmp((char*)q,"87a",3))
                {
                    gs->version=87;
                }
                else
                {
                    ILTRACE(2,("il:gif: unrecognized GIF version number"));
                    gs->state=gif_error;
                    break;
                }
            }
            ILTRACE(2,("il:gif: %d gif", gs->version));
            GETN(7,gif_global_header);
        }
        break;

        case gif_global_header:
        {
            /* This is the height and width of the "screen" or
             * frame into which images are rendered.  The
             * individual images can be smaller than the
             * screen size and located with an origin anywhere
             * within the screen.
             */
                    
            gs->screen_width = GETINT16(q);	
            gs->screen_height = GETINT16(q+2);

            gs->screen_bgcolor = q[5];

            im->colors = 2<<(q[4]&0x07);

            im->map = NULL;

            if(q[6])
            {
                /* should assert gif89 */
                if(q[6] != 49)
                {
#ifdef DEBUG
                    float aspect = (float)((q[6] + 15) / 64.0);
#endif
                    ILTRACE(2, ("il:gif: %f aspect ratio", aspect));
                }
            }

            if( q[4] & 0x80 ) /* global map */
            {
                GETN(im->colors*3, gif_global_colormap);
            }
            else
            {
                GETN(1, gif_image_start);
            }
        }
        break;

        case gif_global_colormap:
        {
            IL_RGB* map;
            int i;

            if(!(map = (IL_RGB*)XP_CALLOC(im->colors, sizeof(IL_RGB))))
            {
                ILTRACE(0,("il:gif: MEM map"));
                gs->state=gif_oom;
                break;
            }
                    
            il_reset_palette(ic);
            gs->global_colormap = map;

#ifdef XP_MAC
            im->hasUniqueColormap = 1;
#endif

            for (i=0; i < im->colors; i++, map++) 
            {
                map->red = *q++;
                map->green = *q++;
                map->blue = *q++;
            }

            GETN(1,gif_image_start);
        }
        break;

        case gif_image_start:
        {
            if(*q==';') /* terminator */
            {
                gs->state = gif_done;
                break;
            }

            if(*q=='!') /* extension */
            {
                GETN(2,gif_extension);
                break;
            }
                        
            if(*q!=',') /* invalid start character */
            {
                ILTRACE(2,("il:gif: bogus start character 0x%02x",
                           (int)*q));
                gs->state=gif_error;
                break;
            }
            else
            {
                /* If this is a multi-part GIF, flush the last image */
                if (gs->images_decoded) {
                    ic->multi++;    /* Avoid progressive display */
                }
            
                GETN(9, gif_image_header);
            }
        }
        break;

        case gif_extension:
        {
            int len = gs->count = q[1];
            gstate es = gif_skip_block;

            ILTRACE(2,("il:gif: %d byte extension %x", len, (int)*q));
            switch(*q)
            {
            case 0xf9:
                es = gif_control_extension;
                break;

            case 0x01:
                ILTRACE(2,("il:gif: ignoring plain text extension"));
                break;
							
            case 0xff:
                es = gif_application_extension;
                break;

            case 0xfe:
                es = gif_consume_comment;
                break;
            }
            
            if (len)
                GETN(len, es);
            else
                GETN(1, gif_image_start);
        }
        break;

        case gif_consume_block:
        {
            if(!*q)
            {
                GETN(1, gif_image_start);
            }
            else
            {
                GETN(*q, gif_skip_block);
            }
        }
        break;
			
        case gif_skip_block:
            GETN(1, gif_consume_block);
            break;

        case gif_control_extension:
        {
            if(*q & 0x1)
            {
                gs->tpixel = *(q+3);
                ILTRACE(2,("il:gif: transparent pixel %d", gs->tpixel));
                if (!il_gif_init_transparency(ic))
                    return MK_OUT_OF_MEMORY;
                im->transparent->index = gs->tpixel;
                gs->is_transparent = TRUE;
            }
            else
            {
                ILTRACE(2,("il:gif: ignoring gfx control extension"));
            }
            gs->control_extension = TRUE;
            gs->disposal_method = (gdispose)(((*q) >> 2) & 0x7);
            gs->delay_time = GETINT16(q + 1) * 10;
            GETN(1,gif_consume_block);
        }
        break;

        case gif_comment_extension:
        {
            gs->count = *q;
            if (gs->count) 
                GETN(gs->count, gif_consume_comment);
            else
                GETN(1, gif_image_start);
        }
        break;

        case gif_consume_comment:
        {
            BlockAllocCat(ic->comment, ic->comment_length, (char*)q, gs->count + 1);
            ic->comment_length += gs->count;
            ic->comment[ic->comment_length] = 0;
            GETN(1, gif_comment_extension);
        }
        break;

        case gif_application_extension:
            /* Check for netscape application extension */
            if (!strncmp((char*)q, "NETSCAPE2.0", 11) ||
                !strncmp((char*)q, "ANIMEXTS1.0", 11))
                GETN(1, gif_netscape_extension_block);
            else
                GETN(1, gif_consume_block);
            break;

        /* Netscape-specific GIF extension: animation looping */
        case gif_netscape_extension_block:
            if (*q)
                GETN(*q, gif_consume_netscape_extension);
            else
                GETN(1, gif_image_start);
            break;

        /* Parse netscape-specific application extensions */
        case gif_consume_netscape_extension:
        {
            int netscape_extension = q[0] & 7;

            /* Loop entire animation specified # of times */
            if (netscape_extension == 1) {
                if (!ic->is_looping
#ifdef DEBUG_jwz
                    && (IL_AnimationsEnabled && IL_AnimationLoopsEnabled)
#endif /* DEBUG_jwz */
                    ) {
                    ic->loop_count = GETINT16(q + 1);

                    /* Zero loop count is infinite animation loop request */
                    if (ic->loop_count == 0)
                        ic->loop_count = -1;

					/* Tell the front end that the stop state might have changed */
					/* because of the looping GIF.                               */
					if (ic->cx)
					  FE_UpdateStopState(ic->cx);
					ic->is_looping = TRUE;
                }

                GETN(1, gif_netscape_extension_block);
            }

            /* Wait for specified # of bytes to enter buffer */
            else if (netscape_extension == 2)
            {
                gs->requested_buffer_fullness = GETINT32(q + 1);
                GETN(gs->requested_buffer_fullness, gif_wait_for_buffer_full);
            }

            break;
        }

        case gif_wait_for_buffer_full:
            gs->gathered = gs->requested_buffer_fullness;
            GETN(1, gif_netscape_extension_block);
            break;

        case gif_image_header:
        {
            uint height, width;
            
/*          For debugging
            if (ic->url_address && strstr(ic->url_address, "multigif"))
                ic->loop_count = -1;
*/
                    
            /* Get image offsets, with respect to the screen origin */
            gs->x_offset = GETINT16(q);
            gs->y_offset = GETINT16(q + 2);

            /* Get image width and height. */
            width  = GETINT16(q + 4);
            height = GETINT16(q + 6);

            ILTRACE(2,("il:gif: screen %dx%d, image %dx%d", 
                       gs->screen_width, gs->screen_height, width, height));

            /* Work around broken GIF files where the logical screen
             * size has weird width or height.  We assume that GIF87a
             * files don't contain animations.
             */
            if ((gs->images_decoded == 0) &&
                ((gs->screen_height < height) || (gs->screen_width < width) ||
                 (gs->version == 87)))
            {
                gs->screen_height = height;
                gs->screen_width = width;
                gs->x_offset = 0;
                gs->y_offset = 0;
            }

            /* Work around more broken GIF files that have zero image
               width or height */
            if (!height || !width) 
			{
                height = gs->screen_height;
                width = gs->screen_width;
            }

            gs->height = height;
            gs->width = width;

            /* This case will never be taken if this is the first image */
            /* being decoded. If any of the later images are larger     */
            /* than the screen size, we need to reallocate buffers.     */
            if (gs->screen_width < width) 
			{
                gs->rgbrow = (uint8*)XP_REALLOC(gs->rgbrow, 3 * width);
                gs->rowbuf = (uint8*)XP_REALLOC(gs->rowbuf, width);
			}
            else 
			{
				if (!gs->rgbrow)
					gs->rgbrow = (uint8*)XP_ALLOC(3 * gs->screen_width);

				if (!gs->rowbuf)
					gs->rowbuf = (uint8*)XP_ALLOC(gs->screen_width);
			}

            if (!gs->rowbuf || !gs->rgbrow)
            {
                ILTRACE(0,("il:gif: MEM row"));
                gs->state=gif_oom;
                break;
            }
                    
            /* Free transparency from earlier image in multi-image sequence. */
            if (!gs->is_transparent && im->transparent) { 
                FREE_IF_NOT_NULL(im->transparent);
            }

            if (gs->images_decoded == 0) {
                status = il_size(ic, gs->screen_width, gs->screen_height);
                if (status < 0)
                { 
                    if (status == MK_OUT_OF_MEMORY) {
                        ILTRACE(0,("il:gif: MEM il_size"));
                        gs->state = gif_oom;
                    }
                    else
                        gs->state = gif_error;
                    break;
                }
            }

            if ( *(q+8) & 0x40 )
            {
                ILTRACE(2,("il:gif: interlaced"));
                gs->interlaced = TRUE;
                gs->ipass = 1;
            } else {
                gs->interlaced = FALSE;
                gs->ipass = 0;
            }
			
            if (gs->images_decoded == 0)
            {
                gs->progressive_display = ic->ip->progressive_display;
            } else {

                /* Overlaying interlaced, transparent GIFs over
                   existing image data using the Haeberli display hack
                   requires saving the underlying image in order to
                   avoid jaggies at the transparency edges.  We are
                   unprepared to deal with that, so don't display such
                   images progressively */
                gs->progressive_display = ic->ip->progressive_display &&
                    !(gs->interlaced && gs->is_transparent);
            }

            /* Clear state from last image */
            gs->requested_buffer_fullness = 0;
            gs->control_extension = FALSE;
            gs->is_transparent = FALSE;
            gs->irow = 0;
            gs->rows_remaining = gs->height;
            gs->rowend = gs->rowbuf + gs->width;
            gs->rowp = gs->rowbuf;

            /* bits per pixel is 1<<((q[8]&0x07)+1); */

            if ( *(q+8) & 0x80 ) 
            {
                int num_colors = 2 << (*(q + 8) & 0x7);
                
                if ((num_colors > im->colors) && gs->local_colormap)
                {
                    XP_FREE(gs->local_colormap);
                    gs->local_colormap = NULL;
                }
                im->colors = num_colors;

                /* Switch to the new local palette after it loads */
                il_reset_palette(ic);

                gs->is_local_colormap_defined = TRUE;
                GETN(im->colors * 3, gif_image_colormap);
            }
            else
            {
                /* Switch back to the global palette */
                if (gs->is_local_colormap_defined)
                    il_reset_palette(ic);

                gs->is_local_colormap_defined = FALSE;
                GETN(1, gif_lzw_start);
            }
        }
        break;

        case gif_image_colormap:
        {
            IL_RGB* map;
            int i;

            ILTRACE(2,("il:gif: local colormap"));
                    
            map = gs->local_colormap;
            if (!map) 
            {
                map = gs->local_colormap =
                    (IL_RGB*)XP_CALLOC(im->colors, sizeof(IL_RGB));

                if(!map)
                {
                    ILTRACE(0,("il:gif: MEM map"));
                    gs->state=gif_oom;
                    break;
                }
            }

#ifdef XP_MAC
            im->hasUniqueColormap = 1;
#endif

            for (i=0; i < im->colors; i++, map++) 
            {
                map->red   = *q++;
                map->green = *q++;
                map->blue  = *q++;
            }

            GETN(1,gif_lzw_start);
        }
        break;

        case gif_sub_block:
        {
            if ((gs->count = *q) != 0)
            /* Still working on the same image: Process next LZW data block */
            {
                /* Make sure there are still rows left. If the GIF data */
                /* is corrupt, we may not get an explicit terminator.   */
                if (gs->rows_remaining == 0) {
                    ILTRACE(3,("il:gif: missing image terminator, continuing"));
                    /* This is an illegal GIF, but we remain tolerant. */
#ifdef DONT_TOLERATE_BROKEN_GIFS
                    gs->state=gif_error;
                    break;
#else
                    GETN(1,gif_sub_block);
#endif
                }
                GETN(gs->count, gif_lzw);
            }
            else
            /* See if there are any more images in this sequence. */                
            {
                /* If this is a multi-part GIF, flush the previous image */
                if (gs->images_decoded || gs->delay_time) {
                    il_flush_image_data(ic, ilPartialData);
                }
            
                gs->images_decoded++;

                /* An image can specify a delay time before which to display
                   subsequent images.  Block until the appointed time. */
                if (gs->delay_time) {

                    gs->delay_timeout =
                        FE_SetTimeout(gif_delay_time_callback, gs, gs->delay_time);

                    /* Essentially, tell the decoder state machine to wait
                       forever.  The timeout callback routine will wake up the
                       state machine and force it to decode the next image. */
                    GETN(1L<<30, gif_image_start);
                    gs->state = gif_delay;
                } else {
                    GETN(1, gif_image_start);
                }
            }
        }
        break;

        case gif_done:
            return 0;
            break;

        case gif_delay:
        case gif_gather:	
        {
            int32 gather_remaining;
            int32 request_size = gs->gather_request_size;
            
            {
                gather_remaining = request_size - gs->gathered;

                /* Do we already have enough data in the accumulation
                   buffer to satisfy the request ?  (This can happen
                   after we transition from the gif_delay state.) */
                if (gather_remaining <= 0)
                {
                    gs->gathered -= request_size;
                    q = gs->gather_head;
                    gs->gather_head += request_size;
                    gs->state = gs->post_gather_state;
                    break;
                }

                /* Shift remaining data to the head of the buffer */
                if (gs->gathered && (gs->gather_head != gs->hold)) {
                    XP_MEMMOVE(gs->hold, gs->gather_head, gs->gathered);
                    gs->gather_head = gs->hold;
                }
                 
                /* If we add the data just handed to us by the netlib
                   to what we've already gathered, is there enough to satisfy 
                   the current request ? */
                if ((ep - p) >= gather_remaining)
                {
                    if(gs->gathered)
                    { /* finish a prior gather */
                        char *hold = (char*)gs->hold;
                        BlockAllocCat(hold, gs->gathered, (char*)p, gather_remaining);
                        gs->hold = (uint8*)hold;
                        q = gs->gather_head = gs->hold;
                        gs->gathered = 0;
                    }
                    else
                    {
                        q = p;
                    }
                    p += gather_remaining;
                    gs->state = gs->post_gather_state;
                }
                else
                { 
                    char *hold = (char*)gs->hold;
                    BlockAllocCat(hold, gs->gathered, (char*)p, ep - p);
                    gs->hold = (uint8*)hold;
                    gs->gather_head = gs->hold;
                    gs->gathered += ep-p;
                    return 0;
                }
            }
        }
        break;

        case gif_oom: 
            ILTRACE(1,("il:gif: reached oom state"));
            return MK_OUT_OF_MEMORY;
            break;

        case gif_error: 
            ILTRACE(2,("il:gif: reached error state"));
            return MK_IMAGE_LOSSAGE;
            break;

        default: 
            ILTRACE(0,("il:gif: unknown state"));
            XP_ASSERT(0);
            break;
		}
	}

	return 0;
}

void
il_gif_complete(il_container *ic)
{
    if (ic->ds)
    {
		gif_struct *gs = (gif_struct*) ic->ds;

        /* No more data in the stream, but we may still have work to do,
           so don't actually free any of the data structures. */
        if (gs->delay_timeout) {
            /* We will free the data structures when image display completes. */
            gs->destroy_pending = TRUE;
            return;
        } else if (gs->requested_buffer_fullness) {
            /* We will free the data structures when image display completes. */
            gs->destroy_pending = TRUE;
            process_buffered_gif_input_data(gs);
            return;
        }
        il_gif_abort(ic);
    }
    il_image_complete(ic);
}

/* Free up all the data structures associated with decoding a GIF */
void
il_gif_abort(il_container *ic)
{
	if (ic->ds) 
	{
		gif_struct *gs = (gif_struct*) ic->ds;

        /* Clear any pending timeouts */
        if (gs->delay_timeout) {
            FE_ClearTimeout(gs->delay_timeout);
            gs->delay_timeout = NULL;
        }
    
		if (gs->rowbuf) 
			XP_FREE(gs->rowbuf);
		if (gs->rgbrow) 
			XP_FREE(gs->rgbrow);
		if (gs->prefix)
			XP_FREE(gs->prefix);
		if (gs->suffix)
			XP_FREE(gs->suffix);
		if (gs->stack)
			XP_FREE(gs->stack);

        if (gs->hold)
            XP_FREE(gs->hold);

        /* Free the colormap that is not in use.  The other one, if
         * present, will be freed when the image container is
         * destroyed.
         */
        if (gs->is_local_colormap_defined) {
            if (gs->global_colormap) {
                XP_FREE(gs->global_colormap);
                gs->global_colormap = NULL;
            }
        } else {
            if (gs->local_colormap) {
                XP_FREE(gs->local_colormap);
                gs->local_colormap = NULL;
            }
        }

		XP_FREE(gs);

		ic->ds = 0;
	}
}

#ifdef PROFILE
#pragma profile off
#endif
