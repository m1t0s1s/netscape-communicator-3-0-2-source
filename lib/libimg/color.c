/* -*- Mode: C; tab-width: 4 -*-
   color.c --- Responsible for conversion from image depth to screen depth.
               Includes dithering for B&W displays, but not dithering
               for PseudoColor displays which can be found in dither.c.
               
   Copyright © 1995 Netscape Communications Corporation, all rights reserved.

   $Id: color.c,v 1.57 1996/05/24 19:48:13 fur Exp $
*/

#include "xp.h"
#include "if.h"

#if defined(__sun)
# include <nspr/sunos4.h>
#endif /* __sun */

#ifdef PROFILE
#pragma profile on
#endif


static void
ConvertRGBToCI(il_container *ic,
               const uint8 *mask,
               const uint8 *sp,
               int x_offset,
               int num,
               void XP_HUGE *vout)
{
    uint r, g, b, pixel;
    il_colorspace *cs = ic->cs;
    uint8 XP_HUGE *out = (uint8 XP_HUGE *) vout + x_offset;
    const uint8 *end = sp + 3*num;
    uint8 cbase = cs->alloc_base;
    uint8 *rm = cs->rmap;
    uint8 *gm = cs->gmap;
    uint8 *bm = cs->bmap;
    uint8 *indirect_map = cs->current_indirect_map;

    XP_ASSERT(indirect_map);
    if (!indirect_map)
        return;

    if (!mask)
    {
        while (sp < end) 
        {
            r = sp[0];
            g = sp[1];
            b = sp[2];
            pixel = rm[r] + gm[g] + bm[b] + cbase;
            *out = indirect_map[pixel];
            out++;
            sp += 3;
        }
    }
    else
    {
        while (sp < end) 
        {
            if (*mask++) {
                r = sp[0];
                g = sp[1];
                b = sp[2];
                pixel = rm[r] + gm[g] + bm[b] + cbase;
                *out = indirect_map[pixel];
            }
            out++;
            sp += 3;
        }
    }
}

static void
DitherConvertRGBToCI(il_container *ic,
                     const uint8 *mask,
                     const uint8 *sp,
                     int x_offset,
                     int num,
                     void XP_HUGE *vout)
{
    il_colorspace *cs = ic->cs;
    uint8 XP_HUGE *out = (uint8 XP_HUGE *) vout + x_offset;
    const uint8 XP_HUGE *end = out + num;
    uint8 *indirect_map = cs->current_indirect_map;

    XP_ASSERT(indirect_map);
    if (!indirect_map)
        return;

    il_quantize_fs_dither(ic, mask, sp, x_offset, (uint8 XP_HUGE *) vout,
                          ic->cs->alloc_base, num);
    if (mask) {
        while (out < end) {
            if (*mask++)
                *out = indirect_map[*out];
            out++;
        }
    } else {
        while (out < end) {
            *out = indirect_map[*out];
            out++;
        }
    }
}

struct fs_data {
    long* err1;
    long* err2;
    long* err3;
    uint8 *greypixels;
    uint8 *bwpixels;
	int width;
    int direction;
    long threshval, sum;
};


static struct fs_data *
init_fs_dither(il_container *ic)
{
	struct fs_data *fs;
	fs = XP_NEW_ZAP(struct fs_data);
    if (! fs)
        return NULL;

	fs->width = ic->image->width;
	fs->direction = 1;
	fs->err1 = (long*) XP_CALLOC(fs->width+2, sizeof(long));
	fs->err2 = (long*) XP_CALLOC(fs->width+2, sizeof(long));
	fs->greypixels = (uint8 *)XP_CALLOC(fs->width+7, 1);
	fs->bwpixels = (uint8 *)XP_CALLOC(fs->width+7, 1);
#ifdef XP_UNIX
    {
        int i;
        /* XXX should not be unix only */
        for(i=0; i<fs->width+2; i++)
            {
                fs->err1[i] = (XP_RANDOM() % 1024 - 512)/4;
            }
    }
#endif
	fs->threshval = 512;
	ic->quantize = (void *)fs;
	return fs;
}

static void
ConvertRGBToBW(il_container *ic,
               const uint8 *mask,
               const uint8 *sp,
               int x_offset,
               int num,
               void XP_HUGE *vout)
{
    uint32 fgmask32, bgmask32;
    uint32 *m;
    int mask_bit;
	struct fs_data *fs = (struct fs_data *)ic->quantize;
    uint8 XP_HUGE *out = (uint8 XP_HUGE *)vout;
	uint8 *gp, *bp;
	int col, limitcol;
	long grey;
	long sum;

	if(!fs)
		fs = init_fs_dither(ic);

    /* Silently fail if memory exhausted */
    if (! fs)
        return;

	gp = fs->greypixels;
	for(col=0; col<fs->width; col++)
	{
        /* CCIR 709 */
		uint8 r = *sp++;
		uint8 g = *sp++;
		uint8 b = *sp++;
        grey = ((uint)(0.299 * 4096) * r +
                (uint)(0.587 * 4096) * g +
                (uint)(0.114 * 4096) * b) / 4096;

		*gp++ = (uint8)grey;
	}

#if 0
	/* thresholding */
	gp = fs->greypixels;
	bp = fs->bwpixels;
	for(col=0; col<fs->width; col++)
	{
		*bp++ = (*gp++<128);
	}

#else

	for(col=0; col<fs->width+2; col++)
	{
		fs->err2[col] =0;
	}

	if (fs->direction)
	{
		col = 0;
		limitcol = fs->width;
		gp = fs->greypixels;
		bp = fs->bwpixels;
	}
	else
	{
		col = fs->width - 1;
		limitcol = -1;
		gp = &(fs->greypixels[col]);
		bp = &(fs->bwpixels[col]);
	}

	do
	{
		sum = ( (long) *gp * 1024 ) / 256 + fs->err1[col + 1];
        if ( sum >= 512)
        {
            *bp = 0;
            sum = sum - 1024;
        }
        else
            *bp = 1;
    
		if ( fs->direction )
		{
			fs->err1[col + 2] += ( sum * 7 ) / 16;
			fs->err2[col    ] += ( sum * 3 ) / 16;
			fs->err2[col + 1] += ( sum * 5 ) / 16;
			fs->err2[col + 2] += ( sum     ) / 16;
			col++; gp++; bp++;
		}
		else
		{
			fs->err1[col    ] += ( sum * 7 ) / 16;
			fs->err2[col + 2] += ( sum * 3 ) / 16;
			fs->err2[col + 1] += ( sum * 5 ) / 16;
			fs->err2[col    ] += ( sum     ) / 16;
			col--; gp--; bp--;
		}
	}
	while ( col != limitcol );

	fs->err3 = fs->err1;
	fs->err1 = fs->err2;
	fs->err2 = fs->err3;

	fs->direction = !fs->direction;

#endif
    
	bp = fs->bwpixels;
    bgmask32 = 0;        /* 32-bit temporary mask accumulators */
    fgmask32 = 0;

    m = ((uint32*)out) + (x_offset >> 5);
    mask_bit = ~x_offset & 0x1f; /* next bit to write in 32-bit mask */

/* Add a bit to the row of mask bits.  Flush accumulator to memory if full. */
#define SHIFT_IMAGE_MASK(opaqueness, pixel)					  		          \
    {																		  \
        fgmask32 |=  ((uint32)pixel & opaqueness) << M32(mask_bit);           \
        bgmask32 |=  ((uint32)((pixel ^ 1) & opaqueness)) << M32(mask_bit);   \
																			  \
        /* Filled up 32-bit mask word.  Write it to memory. */				  \
        if (mask_bit-- == 0) {                                                \
            uint32 mtmp = *m;                                                 \
            mtmp |= fgmask32;                                                 \
            mtmp &= ~bgmask32;                                                \
            *m++ = mtmp;                                                      \
            mask_bit = 31;													  \
            bgmask32 = 0;                                                     \
            fgmask32 = 0;                                                     \
        }																	  \
    }

    for(col=0; col < num; col++)
    {
        int opaqueness = 1;
        int pixel = *bp++;

        if (mask)
            opaqueness = *mask++;
        SHIFT_IMAGE_MASK(opaqueness, pixel);
    }

    /* End of scan line. Write out any remaining mask bits. */ 
    if (mask_bit < 31) {
        uint32 mtmp = *m;
        mtmp |= fgmask32;
        mtmp &= ~bgmask32; 
        *m = mtmp; 
    }
}

static void
ConvertRGBToGrey8(il_container *ic,
                  const uint8 *mask,
                  const uint8 *sp,
                  int x_offset,
                  int num,
                  void XP_HUGE *vout)
{
    uint r, g, b;
    uint8 XP_HUGE *out = (uint8 XP_HUGE *)vout + x_offset;
    const uint8 *end = sp + num*3;
    uint32 grey;

    if (!mask)
    {
        while (sp < end)
        {
            /* CCIR 709 */
            r = sp[0];
            g = sp[1];
            b = sp[2];
            grey = ((uint)(0.299 * 4096) * r +
                    (uint)(0.587 * 4096) * g +
                    (uint)(0.114 * 4096) * b) / 4096;

            *out = (uint8)grey;
            out++;
            sp += 3;
        }
    }
    else
    {
        while (sp < end)
        {
            if (*mask++)
            {
                
                /* CCIR 709 */
                r = sp[0];
                g = sp[1];
                b = sp[2];
                grey = ((uint)(0.299 * 4096) * r +
                        (uint)(0.587 * 4096) * g +
                        (uint)(0.114 * 4096) * b) / 4096;
                *out = (uint8)grey;
            }
            out++;
            sp += 3;
        }
    }
}

static void
ConvertRGBToRGB8(il_container *ic,
                 const uint8 *mask,
                 const uint8 *sp,
                 int x_offset,
                 int num,
                 void XP_HUGE *vout)
{
    uint r, g, b, pixel;
    il_colorspace *cs = ic->cs;
    uint8 XP_HUGE *out = (uint8 XP_HUGE *) vout + x_offset;
    const uint8 *end = sp + num*3;
    uint8 *rm = (uint8*)cs->rtom;
    uint8 *gm = (uint8*)cs->gtom;
    uint8 *bm = (uint8*)cs->btom;
    if (!mask)
    {
        while (sp < end) 
        {
            r = sp[0];
            g = sp[1];
            b = sp[2];
            pixel = rm[r] + gm[g] + bm[b];
            *out = pixel;
            out++;
            sp+=3;
        }
    }
    else
    {
        while (sp < end) 
        {
            if (*mask++) {
                r = sp[0];
                g = sp[1];
                b = sp[2];
                pixel = rm[r] + gm[g] + bm[b];
                *out = pixel;
            }
            out++;
            sp += 3;
        }
    }
}

static void
ConvertRGBToRGB16(il_container *ic,
                  const uint8 *mask,
                  const uint8 *sp,
                  int x_offset,
                  int num,
                  void XP_HUGE *vout)
{
    uint r, g, b, pixel;
    il_colorspace *cs = ic->cs;
    uint16 XP_HUGE *out = (uint16 XP_HUGE *) vout + x_offset;
    const uint8 *end = sp + num*3;
    uint16 *rm = (uint16*)cs->rtom;
    uint16 *gm = (uint16*)cs->gtom;
    uint16 *bm = (uint16*)cs->btom;
    if (!mask)
    {
        while (sp < end) 
        {
            r = sp[0];
            g = sp[1];
            b = sp[2];
            pixel = rm[r] + gm[g] + bm[b];
            *out = pixel;
            out++;
            sp+=3;
        }
    }
    else
    {
        while (sp < end) 
        {
            if (*mask++) {
                r = sp[0];
                g = sp[1];
                b = sp[2];
                pixel = rm[r] + gm[g] + bm[b];
                *out = pixel;
            }
            out++;
            sp += 3;
        }
    }
}

static void
ConvertRGBToRGB24(il_container *ic,
                  const uint8 *mask,
                  const uint8 *sp,
                  int x_offset,
                  int num,
                  void XP_HUGE *vout)
{
    uint8 XP_HUGE *out = (uint8 XP_HUGE *) vout + (3 * x_offset);
    const uint8 *end = sp + num*3;
	/* XXX this is a hack because it ignores the shifts */

    if (!mask)
    {
        while (sp < end) {
            out[2] = sp[0];
            out[1] = sp[1];
            out[0] = sp[2];
            out+=3;
            sp+=3;
        }
    } else {
        while (sp < end) {
            if (*mask++)
            {
                out[2] = sp[0];
                out[1] = sp[1];
                out[0] = sp[2];
            }
            out+=3;
            sp+=3;
        }
    }
}

static void
ConvertRGBToRGB32(il_container *ic,
                  const uint8 *mask,
                  const uint8 *sp,
                  int x_offset,
                  int num,
                  void XP_HUGE *vout)
{
    uint r, g, b, pixel;
    il_colorspace *cs = ic->cs;
    uint32 XP_HUGE *out = (uint32 XP_HUGE *) vout + x_offset;
    const uint8 *end = sp + num*3;
    uint32 *rm = (uint32*)cs->rtom;
    uint32 *gm = (uint32*)cs->gtom;
    uint32 *bm = (uint32*)cs->btom;
    if (!mask)
    {
        while (sp < end) 
        {
            r = sp[0];
            g = sp[1];
            b = sp[2];
            pixel = rm[r] + gm[g] + bm[b];
            *out = pixel;
            out++;
            sp+=3;
        }
    }
    else
    {
        while (sp < end) 
        {
            if (*mask++) {
                r = sp[0];
                g = sp[1];
                b = sp[2];
                pixel = rm[r] + gm[g] + bm[b];
                *out = pixel;
            }
            out++;
            sp += 3;
        }
    }
}

/* Sorting predicate for qsort() */
static int
compare_uint32(const void *a, const void *b)
{
    uint32 a1 = *(uint32*)a;
    uint32 b1 = *(uint32*)b;
    
    return (a1 < b1) ? -1 : ((a1 > b1) ? 1 : 0);
}

/* Count colors in color map.  Remove duplicate colors. */
static int
unique_map_colors(IL_Image *image)
{
    uint i;
    uint32 map[256];
    uint max_colors = image->colors;
    uint unique_colors = 1;
    
    XP_ASSERT(max_colors <= 256);
    XP_ASSERT(image->map);

    if (image->unique_colors)
        return image->unique_colors;

    /* Slightly slimy way of converting packed RGB structs to uint32s */
    for (i = 0; i < max_colors; i++) {
        map[i] = *((uint32*)&image->map[i]);
    }

    /* Sort by color, so identical colors will be grouped together. */
    qsort(map, max_colors, sizeof(*map), compare_uint32);

    /* Look for adjacent colors with different values */
    for (i = 0; i < max_colors-1; i++)
        if (map[i] != map[i + 1])
            unique_colors++;

    image->unique_colors = unique_colors;
    return unique_colors;
}
        
/*
 * A greater number of colors than this causes dithering to be used
 * rather than closest-color rendering if dither_mode == ilAuto.
 */
#define AUTO_DITHER_COLOR_THRESHOLD    16

/* XXX - need to do this for every client of a container*/
void
il_setup_color_space_converter(il_container *ic)
{
    il_colorspace *cs = ic->cs;

    if (ic->type == IL_GIF) {
        
        /*
         * Colormap sizes are rounded up to a power of two for GIFS, so
         * figure out how many colors are *really* in the colormap.
         */
        unique_map_colors(ic->image);
    }
    
#ifdef XP_MAC
    if ((ic->type == IL_GIF) && (ic->image->depth != 1)) {
        ic->converter = NULL;

        /* Allow for custom color palettes */
        il_set_color_palette(ic->cx, ic);
        
        return;
    }
#endif

    ic->dither_mode = ilClosestColor;
    /* Is this a pseudo-color display ? ... */
	if (cs->mode == ilCI) {
        switch (ic->ip->dither_mode) {
        case ilAuto:
            if (ic->type == IL_GIF) {
                int unique_colors = ic->image->unique_colors;
                /* Use a simple heuristic to decide whether or not we dither. */
                if ((unique_colors <= AUTO_DITHER_COLOR_THRESHOLD) &&
                    (unique_colors <= (cs->default_map_size / 2))) {
                    ic->converter = ConvertRGBToCI;
                    ILTRACE(1, ("Dithering turned off; Image has %d colors: %s",
                              unique_colors, ic->url ? ic->url->address : ""));
                    break;
                }
            }

            /* Fall through ... */
            
        case ilDither:
            ic->dither_mode = ilDither;
            ic->converter = DitherConvertRGBToCI;
            break;

        case ilClosestColor:
            if (ic->type == IL_JPEG)
                ic->converter = DitherConvertRGBToCI;
            else
                ic->converter = ConvertRGBToCI;
            break;

        default:
            XP_ASSERT(0);
        }

        /* Allow for custom color palettes */
        il_set_color_palette(ic->cx, ic);
        
    } else {
		il_converter conv;

        /* Is this a true-color display ? ... */
		if (cs->mode == ilRGB) {
            /* no dithering */
			switch (cs->bytes_per_pixel) 
			{
				case 0:
	    			conv = ConvertRGBToBW;
					break;
				case 1:
					conv = ConvertRGBToRGB8;
					break;
				case 2:
					conv = ConvertRGBToRGB16;
					break;
				case 3:
					conv = ConvertRGBToRGB24;
					break;
				case 4:
					conv = ConvertRGBToRGB32;
					break;
			}
		} else {
			XP_ASSERT(cs->mode == ilGrey);
			switch (cs->bytes_per_pixel) 
			{
				case 0:
                    ic->dither_mode = ilDither;
	    			conv = ConvertRGBToBW;
					break;
				case 1:
					conv = ConvertRGBToGrey8;
					break;
			}
		}
		ic->converter = conv;
	}
}

#ifdef PROFILE
#pragma profile off
#endif
