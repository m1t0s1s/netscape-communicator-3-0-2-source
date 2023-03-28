/* -*- Mode: C; tab-width: 4 -*-
 *  colormap.c
 *             
 *   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 *   $Id: colormap.c,v 1.28 1996/05/24 19:48:14 fur Exp $
 */

#include "xp.h"
#include "if.h"

#if defined(XP_UNIX) || defined(XP_WIN)
#    ifndef CUSTOM_COLORMAP
#        define CUSTOM_COLORMAP
#    endif
#endif

/* Determine allocation of desired colors to components, */
/* and fill in Ncolors[] array to indicate choice. */
/* Return value is total number of colors (product of Ncolors[] values). */

static int
select_ncolors(int Ncolors[],
               int out_color_components,
               int desired_number_of_colors)
{
	int nc = out_color_components; /* number of color components */
	int max_colors = desired_number_of_colors;
	int total_colors, iroot, i, j;
	long temp;

        /* XXX - fur .  Is this right ? */
        static const int RGB_order[3] = { 2, 1, 0 };

	/* We can allocate at least the nc'th root of max_colors per component. */
	/* Compute floor(nc'th root of max_colors). */
	iroot = 1;
	do {
		iroot++;
		temp = iroot;		/* set temp = iroot ** nc */
		for (i = 1; i < nc; i++)
			temp *= iroot;
	} while (temp <= (long) max_colors); /* repeat till iroot exceeds root */
	iroot--;			/* now iroot = floor(root) */

	/* Must have at least 2 color values per component */
	if (iroot < 2)
		return -1;

	/* Initialize to iroot color values for each component */
	total_colors = 1;
	for (i = 0; i < nc; i++)
    {
		Ncolors[i] = iroot;
		total_colors *= iroot;
	}

	/* We may be able to increment the count for one or more components without
	 * exceeding max_colors, though we know not all can be incremented.
	 * In RGB colorspace, try to increment G first, then R, then B.
	 */
	for (i = 0; i < nc; i++)
    {
		j = RGB_order[i];
		/* calculate new total_colors if Ncolors[j] is incremented */
		temp = total_colors / Ncolors[j];
		temp *= Ncolors[j]+1;	/* done in long arith to avoid oflo */
		if (temp > (long) max_colors)
			break;			/* won't fit, done */
		Ncolors[j]++;		/* OK, apply the increment */
		total_colors = (int) temp;
	}
	return total_colors;
}


/* Return j'th output value, where j will range from 0 to maxj */
/* The output values must fall in 0..255 in increasing order */

static int output_value(int j, int maxj)
{
	/* We always provide values 0 and 255 for each component;
	 * any additional values are equally spaced between these limits.
	 * (Forcing the upper and lower values to the limits ensures that
	 * dithering can't produce a color outside the selected gamut.)
	 */
	return (int) (((int32) j * 255 + maxj/2) / maxj);
}

/* Return largest input value that should map to j'th output value */
/* Must have largest(j=0) >= 0, and largest(j=maxj) >= 255 */

static int largest_input_value(int j, int maxj)
{
	/* Breakpoints are halfway between values returned by output_value */
	int32 tmpInt = (2*j + 1) * 255 + maxj;
	return (int) (tmpInt  / (2*maxj));
}


/*
 * Create the colormap and color index table.
 * Also creates the ordered-dither tables, if required.
 */

int
il_createColormap(il_colorspace *cs, int desired_colors, uint8 *colormap[3])
{
	int total_colors;		/* Number of distinct output colors */
	int Ncolors[3];		/* # of values alloced to each component */
	int i,j,k, nci, blksize, blkdist, ptr, val;
	uint8 *ci[3];
	uint8 *indexptr;

	/* Select number of colors for each component */
	total_colors = select_ncolors(Ncolors, 3, desired_colors);

	/* blksize is number of adjacent repeated entries for a component */
	/* blkdist is distance between groups of identical entries for a component */
	blkdist = total_colors;

	for (i = 0; i < 3; i++) 
	{
		ci[i] = (uint8*) XP_ALLOC(sizeof(uint8) * (255+1));
		if (!ci[i]) 
		{
			loser:
#ifdef DEBUG
			if (il_debug) XP_TRACE(("il: create_colormap lossage"));
#endif
			return -1;
		}

		colormap[i] = (uint8*) XP_ALLOC(sizeof(uint8) * total_colors);
		if (!colormap[i]) 
			goto loser;
	}

	for (i = 0; i < 3; i++)	{

		/* fill in colormap entries for i'th color component */
		/* # of distinct values for this color */
        nci = Ncolors[i];
		blksize = blkdist / nci;

		for (j = 0; j < nci; j++) {

			/* Compute j'th output value (out of nci) for component */
			val = output_value(j, nci-1);

			/* Fill in all colormap entries that have this value of this component */
			for (ptr = j * blksize; ptr < total_colors; ptr += blkdist) {
				/* fill in blksize entries beginning at ptr */
				for (k = 0; k < blksize; k++)
					(colormap[i])[ptr+k] = (uint8) val;
			}
		}
		blkdist = blksize;		/* blksize of this color is blkdist of next */

		/* Compute quantization for i'th color component */
		/* in loop, val = index of current output value, */
		/* and k = largest j that maps to current val */
		indexptr = ci[i];
		val = 0;
		k = largest_input_value(0, nci-1);
		for (j = 0; j <= 255; j++) 
		{
			while (j > k)		/* advance val if past boundary */
				k = largest_input_value(++val, nci-1);
			/* premultiply so that no multiplication needed in main processing */
			indexptr[j] = (uint8) (val * blksize);
		}
	}

	cs->rmap = ci[0];
	cs->gmap = ci[1];
	cs->bmap = ci[2];
	return total_colors;
}

static int
il_add_context_to_colorspace(MWContext *cx, il_colorspace *cs)
{
    if (cs->contexts == NULL)
        cs->contexts = XP_ListNew();
    
    if (!cs->contexts)
        return FALSE;

#ifdef DEBUG
    if (XP_ListFindObject(cs->contexts, cx))
        XP_ABORT(("il: Colorspace has already been initialized for this context"));
#endif

    XP_ListAddObject(cs->contexts, cx);
    cs->num_contexts++;
    return TRUE;
}

static il_process *
new_imageProcess(void)
{
    il_process *ip = XP_NEW_ZAP(il_process);
    if (!ip)
        return NULL;
    il_last_ip = ip;
    ip->progressive_display = TRUE;

    return ip;
}

il_colorspace *
IL_NewGreyScaleSpace(int depth)
{
    il_colorspace *cs = XP_NEW_ZAP(il_colorspace);
    if (!cs)
        return NULL;
	cs->mode = ilGrey;
	cs->depth = depth;
	cs->bytes_per_pixel = depth/8;
    return cs;
}

int
IL_GreyScale(MWContext *cx,
             int depth,
             IL_IRGB *transparent)
{
    il_colorspace *cs = cx->colorSpace;
	il_process *ip    = cx->imageProcess;

	if (!ip)
		ip = cx->imageProcess = new_imageProcess();
    if (!ip)
        return 0;

	if (!cs) {
		cs = cx->colorSpace = IL_NewGreyScaleSpace(depth);
        if (cs)
            il_add_context_to_colorspace(cx, cs);
    }
        
    if (!cs) {
        XP_FREE(ip);
        cx->imageProcess = NULL;
        return 0;
    }

#ifdef DEBUG
    if (!XP_ListFindObject(cs->contexts, cx))
        XP_ABORT(("IL_InitContext() was not called for the given context\n"));
#endif
            
	if (transparent)
		XP_MEMCPY(&ip->transparent, transparent, sizeof(IL_IRGB));

	return 0;
}

IL_IRGB *
IL_NewPseudoColorSpace(il_colorspace *cs,
                       IL_IRGB *in,
                       int in_count,
                       int free_count,
                       int *alloc_count)
{
	uint8 *rm, *gm, *bm;
	IL_IRGB *rmap, *imap, *mp;
	int i, total_used;
	uint8 **colormap;

	if (!il_setup_quantize())
		return NULL;

	if (cs->cmap)
    {
		/* this is an update */
		colormap = cs->cmap;
		if (colormap[0]) XP_FREE(colormap[0]);
		if (colormap[1]) XP_FREE(colormap[1]);
		if (colormap[2]) XP_FREE(colormap[2]);
        colormap[0] = colormap[1] = colormap[2] = NULL;

		/* these are going to be reused */
		if (cs->rmap) XP_FREE(cs->rmap);
		if (cs->gmap) XP_FREE(cs->gmap);
		if (cs->bmap) XP_FREE(cs->bmap);
        cs->rmap = cs->gmap = cs->bmap = NULL;

		if (cs->default_map) XP_FREE(cs->default_map);
        cs->default_map = NULL;
	}
	else
	{
		if (!(colormap = (uint8**) XP_CALLOC(3, sizeof(uint8*))))
        {
			ILTRACE(0,("il: MEM il_updatecolormap"));
			return 0;
		}
		cs->cmap = colormap;
	}

	if (!(imap = (IL_IRGB*)XP_ALLOC((in_count + free_count)*sizeof(IL_IRGB))))
    {
		ILTRACE(0,("il: MEM il_updatecolormap"));
		return 0;
	}

	XP_BCOPY((void *)in, (void *)imap, in_count * sizeof(IL_IRGB));

	ILTRACE(1,("il: in-colors=%d free=%d", in_count, free_count));

	if (free_count >= 8)
    {
		total_used = il_createColormap(cs, free_count, colormap);
		if (total_used < 0)
        {
			ILTRACE(0,("il: MEM il_createcolormap"));
			return 0;
		}

		if (!(rmap = (IL_IRGB*) XP_ALLOC((total_used)*sizeof(IL_IRGB))))
			return 0;

		ILTRACE(1,("il: using %d color cube", total_used));

		/* repack into different maps */
		rm = colormap[0];
		gm = colormap[1];
		bm = colormap[2];
		for (i=0, mp = (imap + in_count); i < total_used; i++, mp++) {
			uint8 r = *rm++;
			uint8 g = *gm++;
			uint8 b = *bm++;
			mp->red = r;
			mp->green = g;
			mp->blue = b;
			mp->index = i + in_count;
			mp->attr = 0;
		}

		*alloc_count = total_used;
		cs->alloc_base = in_count;

		/* make a private copy */
		XP_BCOPY((void *)(imap + in_count), (void *)rmap,
                 total_used * sizeof(IL_IRGB));
		cs->default_map_size = total_used;
		cs->current_map = cs->default_map = rmap;
	} 
	else 
	{
		XP_ASSERT(0);
	}

	cs->mode = ilCI;
	cs->depth = 8;
    cs->bytes_per_pixel = 1;

    cs->color_render_mode = ilInstallPaletteAllowed;
    cs->colormap_serial_num = 0;

    return imap;
}

IL_IRGB *
IL_UpdateColorMap(MWContext *cx,
                  IL_IRGB *in,
                  int in_count,
                  int free_count,
                  int *alloc_count,
                  IL_IRGB *transparent)
{
    IL_IRGB *imap;
    il_colorspace *cs = cx->colorSpace;
	il_process *ip    = cx->imageProcess;

	if (!ip)
		ip = cx->imageProcess = new_imageProcess();
    if (!ip)
        return NULL;

    if (!cs) {
        cs = cx->colorSpace = XP_NEW_ZAP(il_colorspace);
        if (cs)
            il_add_context_to_colorspace(cx, cs);
    }
    
    if (!cs) {
        XP_FREE(ip);
        cx->imageProcess = NULL;
        return 0;
    }
    
    imap = IL_NewPseudoColorSpace(cs, in, in_count, free_count, alloc_count);

	if(transparent) {
		XP_MEMCPY(&ip->transparent, transparent, sizeof(IL_IRGB));
	}

#ifdef XP_MAC
	IL_DisableScaling(cx);
#endif

	return imap;				/* caller deletes */
}

#ifdef CUSTOM_COLORMAP

/* Possible color-cube sizes */
static int
useful_allocations[] = { /* 252, */ 216, 180, 150, 125, 100, 80,
                                    64, 48, 36, 27, 18, 12, 8, 0};

/* Figure out how many colors we were able to match closely. */
static int
il_IRGB_colormap_close_matches(IL_IRGB *installed_map,
                               IL_IRGB *requested_map,
                               int request_count)
{
    int i, dist, close_matches = 0;

    for (i = 0; i < request_count; i++) {

        if (installed_map[i].attr == IL_ATTR_TRANSPARENT)
        {
            close_matches++;
            continue;
        }

        dist = IL_COLOR_DISTANCE(requested_map[i].red, installed_map[i].red,
                                 requested_map[i].green, installed_map[i].green,
                                 requested_map[i].blue, installed_map[i].blue);

        if (dist < IL_CLOSE_COLOR_THRESHOLD)
            close_matches++;
    }
    
    return close_matches;
}

static IL_IRGB*
il_copy_IRGB_map(IL_IRGB *imap, int num_entries)
{
    IL_IRGB *imap_copy = XP_ALLOC(sizeof(IL_IRGB) * num_entries);
    if (imap_copy == NULL)
        return NULL;
    
    XP_MEMCPY(imap_copy, imap, sizeof(IL_IRGB) * num_entries);
    return imap_copy;
}

int
IL_RealizeDefaultColormap(MWContext *cx, IL_IRGB *bgcolor, int max_colors)
{
    int i, request_count, alloc_count, dummy;
    IL_IRGB *imap_request, *imap_grant;
    uint8* indirect_map;
    int cube_count;
    il_colorspace *cs = cx->colorSpace;
    
    /* Don't reinitialize the same palette more than once, even if it's shared by
       several contexts. */
    if (cs && (cs->num_contexts > 1)) {
        XP_ASSERT(cs->default_map);
        /* XXX - really should return the number of colors allocated
           in a previous call for this colorspace, but no one looks at
           this anyway */
        return 255;
    }
    
    imap_request = NULL;
    request_count = 0;
    while((cube_count = useful_allocations[request_count]))
    {
        if (cube_count > max_colors) {
            request_count++;
            continue;
        }
        
        if (imap_request)
            XP_FREE(imap_request);

        /* Ask image library to pick a color-cube map */
        imap_request =
            IL_UpdateColorMap (cx, NULL, 0, cube_count, &dummy, bgcolor);
        imap_grant = il_copy_IRGB_map(imap_request, cube_count);
        if (imap_grant == NULL)
            return 0;
        alloc_count = FE_SetColormap(cx, imap_grant, cube_count);
        
        if (alloc_count == cube_count)
            break;
        
        alloc_count = il_IRGB_colormap_close_matches(imap_request,
                                                     imap_grant, cube_count);

        if (alloc_count == cube_count)
            break;

        request_count++;
    }
    if (imap_request)
        XP_FREE(imap_request);

    indirect_map = XP_CALLOC(1, 256);
    if (!indirect_map)
        return 0;

    for (i = 0; i < cube_count; i++) {
        indirect_map[i] = imap_grant[i].index;
        imap_grant[i].attr |= IL_ATTR_HONOR_INDEX;
    }

    cs = cx->colorSpace;
    cs->default_indirect_map = indirect_map;
    cs->current_indirect_map = indirect_map;

    cs->default_map = cs->current_map = imap_grant;

    XP_ASSERT(!cx->is_grid_cell ||
              (cx->grid_parent && cx->grid_parent->colorSpace == cs));
    return cube_count;
}
#endif

il_colorspace *
IL_NewTrueColorSpace(int bpp,
                     int rs, int gs, int bs,
                     int rb, int gb, int bb,
                     int wild_wacky_rounding) /* flag for Windoze95 */
{
	int32 j, k;
    int	depth;

    il_colorspace *cs = XP_NEW_ZAP(il_colorspace);
    if (!cs)
        return NULL;

    depth = rb + gb + bb;
	if (depth <= 8) 
	{
		unsigned char *mp;
		cs->rtom = XP_ALLOC(256);
		cs->gtom = XP_ALLOC(256);
		cs->btom = XP_ALLOC(256);

		if(!(cs->rtom && cs->gtom && cs->btom)) {
			ILTRACE(0,("il: MEM il_enabletruecolor"));
			return 0;
		}

		mp = (unsigned char*) cs->rtom;
		for (j = 0; j < (1 << rb); j++) 
			for (k = 0; k < (1 << (8 - rb)); k++) 
				*mp++ = (uint8)(j << rs);

		mp = (unsigned char*) cs->gtom;
		for (j = 0; j < (1 << gb); j++)
			for (k = 0; k < (1 << (8 - gb)); k++)
				*mp++ = (uint8)(j << gs);

		mp = (unsigned char*) cs->btom;
		for (j = 0; j < (1 << bb); j++)
			for (k = 0; k < (1 << (8 - bb)); k++)
				*mp++ = (uint8)(j << bs);
	} 
	else 
	{
		if (depth <= 16)
        {
			unsigned short *mp;

			cs->rtom = XP_ALLOC(sizeof(unsigned short) * 256);
			cs->gtom = XP_ALLOC(sizeof(unsigned short) * 256);
			cs->btom = XP_ALLOC(sizeof(unsigned short) * 256);

			if(!(cs->rtom && cs->gtom && cs->btom))
			{
				ILTRACE(0,("il: MEM il_enabletruecolor"));
				return 0;
			}

/* Compensate for Win95's sometimes-weird color quantization. */
#define _W1(v, b)         ((v) - (1 << (7 - (b))))
#define WACKY(v, b)       ((_W1(v, b) < 0) ? 0 : _W1(v, b))

#define ROUND(v, b)       (wild_wacky_rounding ? WACKY(v, b) : (v))

            mp = (unsigned short*) cs->rtom;
            for (j = 0; j < 256; j++)
                *mp++ = (uint16)(ROUND(j, rb) >> (8 - rb) << rs);
            
            mp = (unsigned short*) cs->gtom;
            for (j = 0; j < 256; j++)
                *mp++ = (uint16)(ROUND(j, gb) >> (8 - gb) << gs);

            mp = (unsigned short*) cs->btom;
            for (j = 0; j < 256; j++)
                *mp++ = (uint16)(ROUND(j, bb) >> (8 - bb) << bs);

#undef _W1
#undef WACKY
#undef ROUND
		} 
		else 
		{
			if (depth <= 24) 
			{
				uint32 *mp;

				cs->rtom = XP_ALLOC(sizeof(uint32)*256);
				cs->gtom = XP_ALLOC(sizeof(uint32)*256);
				cs->btom = XP_ALLOC(sizeof(uint32)*256);

				if(!(cs->rtom && cs->gtom && cs->btom))
				{
					ILTRACE(0,("il: MEM il_enabletruecolor"));
					return 0;
				}

				mp = (uint32*) cs->rtom;
				for (j = 0; j < (1 << rb); j++)
					*mp++ = j << rs;

				mp = (uint32*) cs->gtom;
				for (j = 0; j < (1 << gb); j++)
					*mp++ = j << gs;

				mp = (uint32*) cs->btom;
				for (j = 0; j < (1 << bb); j++)
					*mp++ = j << bs;
			}
			else 
			{
				ILTRACE(0,("il: unsupported truecolor depth %d bpp %d",
                           depth, bpp));
				return NULL;
			}
		}
	}

	cs->mode = ilRGB;

	if (!bpp)
		bpp = depth;

	cs->depth = bpp;
	cs->bytes_per_pixel = bpp/8;
    return cs;
}


/*
 * FE enables true color mode.
 * used instead of IL_UpdateColorMap when FE can render RGB.
 */
int
IL_EnableTrueColor(MWContext *cx,
                   int bpp,
                   int rs, int gs, int bs,
                   int rb, int gb, int bb,
                   IL_IRGB *transparent,
                   int wild_wacky_rounding) /* For Windoze95 */
{
	il_process *ip = cx->imageProcess;
    il_colorspace *cs = cx->colorSpace;

	if (!ip)
		ip = cx->imageProcess = new_imageProcess();

	if (!cs) {
		cs = cx->colorSpace = IL_NewTrueColorSpace(bpp, rs, gs, bs, rb, gb, bb,
                                                   wild_wacky_rounding);
        if (cs)
            il_add_context_to_colorspace(cx, cs);
    }

    if (!cs) {
        XP_FREE(ip);
        cx->imageProcess = NULL;
        return 0;
    }

#ifdef DEBUG
    if (!XP_ListFindObject(cs->contexts, cx))
        XP_ABORT(("IL_InitContext() was not called for the given context\n"));
#endif
            
	if (transparent) {
		XP_MEMCPY(&ip->transparent, transparent, sizeof(IL_IRGB));
	}

#ifdef XP_MAC
	IL_DisableScaling(cx);
#endif

	return 0;
}

int
IL_InitContext(MWContext *cx)
{
	il_process *ip = cx->imageProcess;

	if (!ip)
		ip = cx->imageProcess = new_imageProcess();
    if (!ip)
        return 0;

    /* Color spaces, i.e. palettes are one-per-window, so they're
       inherited from the parent context. */
    if (!cx->colorSpace && cx->is_grid_cell) {
        XP_ASSERT(cx->grid_parent);
        cx->colorSpace = cx->grid_parent->colorSpace;
    }
    
    if (cx->colorSpace)
        il_add_context_to_colorspace(cx, cx->colorSpace);
#ifdef XP_MAC
	IL_DisableScaling(cx);
#endif

	return 1;
}

static void
il_free_colorspace(il_colorspace *cs)
{
    if (cs->current_map != cs->default_map)
        FREE_IF_NOT_NULL(cs->current_map);
    FREE_IF_NOT_NULL(cs->default_map);
    cs->current_map = NULL;

    FREE_IF_NOT_NULL(cs->default_indirect_map);

    if (cs->cmap) {
        FREE_IF_NOT_NULL(cs->cmap[0]);
        FREE_IF_NOT_NULL(cs->cmap[1]);
        FREE_IF_NOT_NULL(cs->cmap[2]);
        XP_FREE(cs->cmap);
        cs->cmap = NULL;
    }
        
    FREE_IF_NOT_NULL(cs->rmap);
    FREE_IF_NOT_NULL(cs->gmap);
    FREE_IF_NOT_NULL(cs->bmap);
    FREE_IF_NOT_NULL(cs->rtom);
    FREE_IF_NOT_NULL(cs->gtom);
    FREE_IF_NOT_NULL(cs->btom);

    /* We leak this for now, because of some dangling references inside
       image containers */
    /*    XP_FREE(cs); */
}

void
IL_ScourContext(MWContext *cx)
{
    il_colorspace *cs = cx->colorSpace;

    if (cs) {
        if (!XP_ListRemoveObject(cs->contexts, cx)) {
#ifdef DEBUG
            XP_ABORT(("IL_ScourContext(): context was never initialized.\n"));
#endif
            return;
        }

        cs->num_contexts--;
        if (cs->num_contexts == 0) {
            il_free_colorspace(cs);
            cx->colorSpace = NULL;
        }
    }

    FREE_IF_NOT_NULL(cx->imageProcess);
}

int
IL_ColormapTag(const char* image_url, MWContext* cx)
{
	return 0;
}

/* Force il_set_color_palette() to load a new colormap for an image */
void
il_reset_palette(il_container *ic)
{
    ic->colormap_serial_num = -1;
    ic->dont_use_custom_palette = FALSE;
    ic->rendered_with_custom_palette = FALSE;
    ic->image->unique_colors = 0;
}

#if 0
/* pick the closest color in a map */
static int 
il_closest_color(IL_IRGB *imap, int size, IL_RGB *c)
{
	int i, min;
	unsigned long d, md=~0;
	uint8 r = c->red;
	uint8 g = c->green;
	uint8 b = c->blue;
	int rd, gd, bd;
	for(i=0; i<size; i++)
	{
		rd = imap[i].red - r;
		gd = imap[i].green - g;
		bd = imap[i].blue - b;
		d = (rd*rd + gd*gd + bd*bd);	/* xxx euclidean */
		if(d<md)
		{
			md = d;
			min = i;
		}
	}
	return min;
}
#endif

#ifdef CUSTOM_COLORMAP

static IL_IRGB *
il_copy_map(il_container *ic, int *count, int attr)
{
	IL_IRGB *imap, *p;
	IL_RGB *q;
	int i;
	int num_image_colors = ic->image->colors; /* XXX - unique_colors ??? */
    IL_Image *image = ic->image;
    
	/* make an IMAP from the (contiguous) image map */
	if (!(imap = (IL_IRGB*) XP_ALLOC(num_image_colors * sizeof(IL_IRGB))))
    {
		*count = 0;
		return NULL;
	}

	for(i = 0, p = imap, q = image->map;
		i < num_image_colors; 
		i++, p++, q++)
	{
        p->attr  = attr;
		p->red   = q->red;
		p->green = q->green;
		p->blue  = q->blue;
		p->index = i;
	}
	*count = num_image_colors;

    /* Identify the transparent pixel, if any. */
    if (image->transparent)
        imap[image->transparent->index].attr |= IL_ATTR_TRANSPARENT;

	return imap;
}

/* Figure out how many colors we were able to match closely. */
static int
il_colormap_close_matches(IL_IRGB *installed_map,
                          IL_RGB *requested_map,
                          int request_count)
{
    int i, rdist, gdist, bdist, close_matches = 0;
    int32 dist2;

    for (i = 0; i < request_count; i++) {

        if (installed_map[i].attr == IL_ATTR_TRANSPARENT)
        {
            close_matches++;
            continue;
        }

        rdist = requested_map[i].red   - installed_map[i].red;
        gdist = requested_map[i].green - installed_map[i].green;
        bdist = requested_map[i].blue  - installed_map[i].blue;

        dist2 = (rdist * rdist) + (gdist * gdist) + (bdist * bdist);

#define CLOSE_COLOR_THRESHOLD 48
        if (dist2 < CLOSE_COLOR_THRESHOLD)
            close_matches++;
#undef CLOSE_COLOR_THRESHOLD
      
    }
    
    return close_matches;
}

static void
il_install_default_colormap(MWContext *cx)
{
	il_colorspace *cs = cx->colorSpace;

    if (cs->current_map == cs->default_map)
        return;
    
    (void)FE_SetColormap(cx, cs->default_map, cs->default_map_size);
    cs->colormap_serial_num = 0;
    
    if (cs->current_map)
        XP_FREE(cs->current_map);
    cs->current_map = cs->default_map;
    cs->current_indirect_map = cs->default_indirect_map;

    ILTRACE(1,("il: default map"));
}
#endif /* CUSTOM_COLORMAP */

static int colormap_serial_num = 0;

/* Returns non-zero if successful at installing custom colormap */
int
il_set_color_palette(MWContext *cx, il_container *ic)
{
	il_colorspace *cs = ic->cs;

#ifdef CUSTOM_COLORMAP
    int close_matches;
    int install_colormap;

    /* Obviously, we can't install a colormap without a pseudocolor display. */
    if ((cs->mode != ilCI) || (cs->color_render_mode != ilInstallPaletteAllowed))
        return 0;
    
    /*
     * Right now, we only install custom color maps when there is a
     * single GIF image on the page.
     */
    install_colormap =
        (ic->type == IL_GIF) && cs->install_colormap_allowed &&
        !cx->colorSpace->install_colormap_forbidden          &&
        !ic->dont_use_custom_palette;

    /* The color flashing in all the background windows during palette
       changes is distracting and also slows display on machines with
       palette managers, e.g. Mac and Windows, so don't do it for any
       image after the first one for multiGIFs or for any image in a
       multi-part MIME. */
    if ((ic->stream && ic->stream->is_multipart) || ic->multi)
        install_colormap = FALSE;

#ifdef XP_WIN
    /* The current windows build is suffering from excessive colormap
       flashing, so we're only going to install an image if the image
       viewer is invoked.

       An image container should always have clients, but we're being
       a little paranoid here. */
    if (ic->clients && !ic->clients->is_view_image)
        install_colormap = FALSE;
#endif

    if (install_colormap)
        {
        IL_IRGB *imap;
        int attr, cnt=0, installed=0;

        /* Don't install colormap for this image if we already have. */
        if (ic->colormap_serial_num == cs->colormap_serial_num)
            return 1;

        attr = ic->rendered_with_custom_palette ? IL_ATTR_HONOR_INDEX : 0;
    
        imap = il_copy_map(ic, &cnt, attr);
        if (imap)
        {
            installed =
                FE_SetColormap(cx, imap, cnt);
            cs->colormap_serial_num = colormap_serial_num++;
        }
        else
            return 0;

        close_matches = il_colormap_close_matches(imap, ic->image->map, cnt);

        ILTRACE(0,
                ("il: palette: Requested: %d, Unique: %d, Got: %d, Close: %d\n",
                 cnt, ic->image->unique_colors, installed, close_matches));

        /* Don't bother installing the new color palette unless we got
         * at least half the colors we asked for.
         */
        if (close_matches > (ic->image->unique_colors / 2))
        {
            int i;
            uint8 *indirect_map;
            
            ILTRACE(1,("il: set %d colors", cnt));
            XP_ASSERT(cs->current_map);
            if (cs->current_map != cs->default_map)
                XP_FREE(cs->current_map);

            indirect_map = ic->indirect_map;

            if (!indirect_map)
                indirect_map = ic->indirect_map = XP_ALLOC(256);
            if (!indirect_map)
                return 0;

            for (i = 0; i < cnt; i++) {
                indirect_map[i] = imap[i].index;
                imap[i].attr |= IL_ATTR_HONOR_INDEX;
            }

            cs->current_map = imap;
            cs->current_indirect_map = indirect_map;
            ic->converter = NULL; /* XXX - danger Will Robinson */
            ic->colormap_serial_num = cs->colormap_serial_num;
            ic->rendered_with_custom_palette = TRUE;
            return 1;
        }
        else
        {
            XP_FREE(imap);

            /* Don't bother trying in the future. */
            ic->dont_use_custom_palette = TRUE;
            cs->current_map = NULL;
        }
    }
        
    il_install_default_colormap(cx);
    ic->colormap_serial_num = 0;
    
#else  /* !CUSTOM_COLORMAP */
    if (!cs->default_indirect_map) {
        int i;
        uint8* indirect_map = (uint8 *)XP_ALLOC(256);
        for (i = 0; i < 256; i++)
            indirect_map[i] = i;
        cs->default_indirect_map = indirect_map;
    }
            
    cs->current_indirect_map = cs->default_indirect_map;
#endif /* CUSTOM_COLORMAP */
    return 0;
}

/*
 * Sometimes we cache an image with a custom colormap and then
 * we want to redisplay it later using the default, fixed colormap.
 * The whole image needs to be converted to use the new colormap.
 */

#if 0
void
il_convert_image_to_default_colormap(il_container *ic)
{
    int i;
    uint8 conversion_map[256];
    uint8 *pixelp = ic->image->bits;
    uint8 *last_pixelp = pixelp + (ic->image->width * ic->image->height);
    il_colorspace *cs = ic->cs;
    unsigned char cbase = cs->alloc_base;
    unsigned char *rm = cs->rmap;
    unsigned char *gm = cs->gmap;
    unsigned char *bm = cs->bmap;

    /* Generate a conversion map from the old colormap to the new one. */
    for (i = 0; i < 256; i++) {
        IL_RGB *color = &ic->image->map[i];
		conversion_map[i] =
            rm[color->red] + gm[color->green] + bm[color->blue] + cbase;
    }

    /* Convert the pixels into the new colorspace */
    while (pixelp < last_pixelp) {
        int pixel = *pixelp;
        *pixelp++ = conversion_map[pixel];
    }

    ic->rendered_with_custom_palette = FALSE;

    /* Sorry boys, this is a one-way trip. */
    ic->dont_use_custom_palette = TRUE;
}
#endif

void
IL_SetColorRenderMode(MWContext *cx, IL_ColorRenderMode color_render_mode)
{
    il_colorspace *cs = cx->colorSpace;

    /* This context might not be initialized yet. */
    if (!cs)
        return;

    cs->color_render_mode = color_render_mode;
}

void
IL_NoMoreImages(MWContext *cx)
{
    /* This window could have no imageProcess or colorSpace if no images
     * were ever displayed in it.
     */
    if (cx->colorSpace == NULL)
        return;

	/* If there is exactly one image and it is loaded in this context,
     * then it can have a custom colormap.  For now, we don't allow
     * framesets to have custom colormaps.
     */
	if ((cx->colorSpace->unique_images == 1) && cx->imageList &&
        (cx->colorSpace->num_contexts == 1))
    {
        /* XXX - This doesn't work right for frames. */
        /* XP_ASSERT(cx->images == 1); */
		cx->colorSpace->install_colormap_allowed = TRUE;
    }
}

void
IL_UseDefaultColormapThisPage(MWContext *cx)
{
    if (cx->colorSpace)
        cx->colorSpace->install_colormap_forbidden = TRUE;
}

void
IL_SamePage(MWContext *cx)
{
}

void
IL_EndPage(MWContext *cx)
{
    /* XP_TRACE(("# Images on page: %d\n", cx->images)); */
}

