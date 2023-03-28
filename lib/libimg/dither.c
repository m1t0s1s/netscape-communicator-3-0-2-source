#include "xp.h"
#include "il.h"
#include "if.h"

#include "jinclude.h"
#include "jpeglib.h"
#include "jpegint.h"
#include "jerror.h"


/* BEGIN code adapted from jpeg library */

typedef INT32 FSERROR;		/* 16 bits should be enough */
typedef int LOCFSERROR;		/* use 'int' for calculation temps */

typedef FSERROR FAR *FSERRPTR;	/* pointer to error array (in FAR storage!) */

typedef struct my_cquantize_str {
	/* Variables for Floyd-Steinberg dithering */
	FSERRPTR fserrors[3];		/* accumulated errors */
	boolean on_odd_row;		/* flag to remember which row we are on */
	uint8 *colorindex[3];
	uint8 **colormap;
} my_cquantize;

typedef my_cquantize *my_cquantize_ptr;

static JSAMPLE *the_sample_range_limit = NULL;

/* allocate and fill in the sample_range_limit table */
int
il_setup_quantize(void)
{
	JSAMPLE *table;
	int i;

	if(the_sample_range_limit)
		return TRUE;

	/* lost for ever */
	table = (JSAMPLE *)XP_ALLOC((5 * (MAXJSAMPLE+1) + CENTERJSAMPLE) * SIZEOF(JSAMPLE));
	if (!table) 
	{
		XP_TRACE(("il: range limit table lossage"));
		return FALSE;
	}

	table += (MAXJSAMPLE+1);	/* allow negative subscripts of simple table */
	the_sample_range_limit = table;

	/* First segment of "simple" table: limit[x] = 0 for x < 0 */
	MEMZERO(table - (MAXJSAMPLE+1), (MAXJSAMPLE+1) * SIZEOF(JSAMPLE));

	/* Main part of "simple" table: limit[x] = x */
	for (i = 0; i <= MAXJSAMPLE; i++)
		table[i] = (JSAMPLE) i;

	table += CENTERJSAMPLE;	/* Point to where post-IDCT table starts */

	/* End of simple table, rest of first half of post-IDCT table */
	for (i = CENTERJSAMPLE; i < 2*(MAXJSAMPLE+1); i++)
		table[i] = MAXJSAMPLE;

	/* Second half of post-IDCT table */
	MEMZERO(table + (2 * (MAXJSAMPLE+1)),
			(2 * (MAXJSAMPLE+1) - CENTERJSAMPLE) * SIZEOF(JSAMPLE));
	MEMCOPY(table + (4 * (MAXJSAMPLE+1) - CENTERJSAMPLE),
			the_sample_range_limit, CENTERJSAMPLE * SIZEOF(JSAMPLE));

	return TRUE;
}


/* Must remain idempotent.  Also used to make sure that the ic->quantize has the same 
colorSpace info as the rest of ic. */
int
il_init_quantize(il_container *ic)
{
    size_t arraysize;
    int i, j;
    my_cquantize_ptr cquantize;

    if (ic->quantize)
        il_free_quantize(ic);

    ic->quantize = XP_NEW_ZAP(my_cquantize);
    if (!ic->quantize) 
	{
	loser:
		ILTRACE(0,("il: MEM il_init_quantize"));
		return FALSE;
    }

    cquantize = (my_cquantize_ptr) ic->quantize;
    arraysize = (size_t) ((ic->image->width + 2) * SIZEOF(FSERROR));
    for (i = 0; i < 3; i++) 
	{
		cquantize->fserrors[i] = (FSERRPTR) XP_CALLOC(1, arraysize);
		if (!cquantize->fserrors[i]) 
		{
			/* ran out of memory part way thru */
			for (j = 0; j < i; j++) 
			{
				if (cquantize->fserrors[j])
				{
					XP_FREE(cquantize->fserrors[j]);
					cquantize->fserrors[j]=0;
				}
			}
			if (cquantize)
			{
				XP_FREE(cquantize);
				ic->quantize = 0;
			}
			goto loser;
		}
    }
    cquantize->colorindex[0] = ic->cs->rmap;
    cquantize->colorindex[1] = ic->cs->gmap;
    cquantize->colorindex[2] = ic->cs->bmap;
    cquantize->colormap = ic->cs->cmap;

    return TRUE;
}

/*
** Free up quantizer information attached to ic. If this is the last
** quantizer then free up the sample range limit table.
*/
void
il_free_quantize(il_container *ic)
{
    my_cquantize_ptr cquantize = (my_cquantize_ptr) ic->quantize;
    int i;

    if (cquantize) 
    {
#ifdef DEBUG
		if (il_debug > 5) 
			XP_TRACE(("il: 0x%x: free quantize", ic));
#endif
		for (i = 0; i < 3; i++) 
		{
			if (cquantize->fserrors[i]) 
			{
				XP_FREE(cquantize->fserrors[i]);
				cquantize->fserrors[i] = 0;
			}
		}

		cquantize->colorindex[0] = 0;
		cquantize->colorindex[1] = 0;
		cquantize->colorindex[2] = 0;
		cquantize->colormap = 0;
		XP_FREE(cquantize);
		ic->quantize = 0;
    }
}


/* floyd-steinberg dithering */

#ifdef XP_MAC
#ifndef powerc
#pragma peephole on
#endif
#endif

void
il_quantize_fs_dither(il_container *ic, const uint8 *mask, const uint8 *input_buf,
                      int x_offset, uint8 XP_HUGE *output_buf, int cbase, int width)
{
	my_cquantize_ptr cquantize;
	register LOCFSERROR cur;	/* current error or pixel value */
	LOCFSERROR belowerr;		/* error for pixel below cur */
	LOCFSERROR bpreverr;		/* error for below/prev col */
	LOCFSERROR bnexterr;		/* error for below/next col */
	LOCFSERROR delta;
	FSERRPTR errorptr;	/* => fserrors[] at column before current */
	const JSAMPLE* input_ptr;
	JSAMPLE XP_HUGE * output_ptr;
	uint8 *colorindex_ci;
	uint8 *colormap_ci;
        const uint8 *maskp;
	int pixcode;
	int nc;
	int dir;			/* 1 for left-to-right, -1 for right-to-left */
	int dirnc;			/* dir * nc */
	int ci;
	JDIMENSION col;
	JSAMPLE *range_limit = the_sample_range_limit;
	SHIFT_TEMPS

	
#ifdef GOLD
    /* This should be done for all versions of the navigator.
     * However, it's only a critical bug for Gold, and the
     * fix was made very late in the Akbar cycle. So, for
     * reasons of paranoia, we only made the fix for the
     * Gold source code. In Dogbert, we should remove the
     * #ifdef, and always do this assignment.
     */
    
    /* Blatant hack.  Somehow the quantize portion of the il_container has a 
    different colormap than the colorSpace. */
    if (((my_cquantize_ptr)ic->quantize)->colormap != ic->cs->cmap) {
        XP_TRACE(("il_quantize_fs_dither: ic->quantize and ic->cs don't agree on colormap"));
        il_init_quantize(ic);
    } 
#endif

    cquantize = (my_cquantize_ptr) ic->quantize;

    
	nc = 3;

        output_buf += x_offset;

	/* Initialize output values to 0 so can process components separately */
        if (mask) {
            output_ptr = output_buf;
            maskp = mask;
            for (col = width; col > 0; col--)
                *output_ptr++ &= ~*maskp++;
        } else {
            XP_BZERO((void XP_HUGE *) output_buf, (size_t) (width * SIZEOF(JSAMPLE)));
        }

	for (ci = 0; ci < nc; ci++) 
	{
		input_ptr = input_buf + ci;
		output_ptr = output_buf;
                maskp = mask;
		if (cquantize->on_odd_row) 
		{
			/* work right to left in this row */
			input_ptr += (width-1) * nc; /* so point to rightmost pixel */
			output_ptr += width-1;
			dir = -1;
			dirnc = -nc;
                        /* => entry after last column */
			errorptr = cquantize->fserrors[ci] + x_offset + (width+1);
                        maskp += (width - 1);
		} 
		else 
		{
			/* work left to right in this row */
			dir = 1;
			dirnc = nc;
                        /* => entry before first column */
			errorptr = cquantize->fserrors[ci] + x_offset;
		}

		colorindex_ci = cquantize->colorindex[ci];
		colormap_ci = cquantize->colormap[ci];
		/* Preset error values: no error propagated to first pixel from left */
		cur = 0;
		/* and no error propagated to row below yet */
		belowerr = bpreverr = 0;

		for (col = width; col > 0; col--) 
		{
			/* cur holds the error propagated from the previous pixel on the
			 * current line.  Add the error propagated from the previous line
			 * to form the complete error correction term for this pixel, and
			 * round the error term (which is expressed * 16) to an integer.
			 * RIGHT_SHIFT rounds towards minus infinity, so adding 8 is correct
			 * for either sign of the error value.
			 * Note: errorptr points to *previous* column's array entry.
			 */
			cur = RIGHT_SHIFT(cur + errorptr[dir] + 8, 4);

			/* Form pixel value + error, and range-limit to 0..MAXJSAMPLE.
			 * The maximum error is +- MAXJSAMPLE; this sets the required size
			 * of the range_limit array.
			 */
			cur += GETJSAMPLE(*input_ptr);
			cur = GETJSAMPLE(range_limit[cur]);

			/* Select output value, accumulate into output code for this pixel */
			pixcode = GETJSAMPLE(colorindex_ci[cur]);
                        if (mask) {
                            if (*maskp)
                                *output_ptr += (JSAMPLE) pixcode + cbase;
                            maskp += dir;
                        } else {
                            *output_ptr += (JSAMPLE) pixcode + cbase;
                        }

			/* Compute actual representation error at this pixel */
			/* Note: we can do this even though we don't have the final */
			/* pixel code, because the colormap is orthogonal. */
			cur -= GETJSAMPLE(colormap_ci[pixcode]);

			/* Compute error fractions to be propagated to adjacent pixels.
			 * Add these into the running sums, and simultaneously shift the
			 * next-line error sums left by 1 column.
			 */
			bnexterr = cur;
			delta = cur * 2;
			cur += delta;		/* form error * 3 */
			errorptr[0] = (FSERROR) (bpreverr + cur);
			cur += delta;		/* form error * 5 */
			bpreverr = belowerr + cur;
			belowerr = bnexterr;
			cur += delta;		/* form error * 7 */

			/* At this point cur contains the 7/16 error value to be propagated
			 * to the next pixel on the current line, and all the errors for the
			 * next line have been shifted over. We are therefore ready to move on.
			 */
			input_ptr += dirnc;	/* advance input ptr to next column */
			output_ptr += dir;	/* advance output ptr to next column */
			errorptr += dir;	/* advance errorptr to current column */
		}

		/* Post-loop cleanup: we must unload the final error value into the
		 * final fserrors[] entry.  Note we need not unload belowerr because
		 * it is for the dummy column before or after the actual array.
		 */
		errorptr[0] = (FSERROR) bpreverr; /* unload prev err into array */
		cbase = 0;
    }
    cquantize->on_odd_row = (cquantize->on_odd_row ? FALSE : TRUE);
}

#ifdef XP_MAC
#ifndef powerc
#pragma peephole reset
#endif
#endif

/* END code adapted from jpeg library */

