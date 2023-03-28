/* -*- Mode: C; tab-width: 4 -*-
 *   xbm.c --- Decoding of X bit-map format images
 *   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *
 * $Id: xbm.c,v 1.61 1996/04/28 02:25:24 blythe Exp $
 */
#include "xp.h"
#include "merrors.h"
#include "il.h"
#include "if.h"


extern int MK_OUT_OF_MEMORY;


typedef enum {
    xbm_gather,
    xbm_init,
    xbm_width,
    xbm_height,
    xbm_start_data,
    xbm_data,
    xbm_hex,
    xbm_done,
	xbm_oom,
    xbm_error
} xstate;

#define MAX_HOLD 512

typedef struct xbm_struct {
    xstate state;
    unsigned char hold[MAX_HOLD];
    intn gathern;				/* gather n chars */
    unsigned char gatherc;		/* gather until char c */
    intn gathered;				/* bytes accumulated */
    xstate gathers;				/* post-gather state */
    int xpos, ypos;
    unsigned char *p;			/* raster position */
} xbm_struct;

#define MAX_LINE MAX_HOLD		/* XXX really bad xbms will hose us */


#define GETN(n,s) {xs->state=xbm_gather; xs->gathern=n; xs->gathers=s;}
#define GETC(c,s) {xs->state=xbm_gather; xs->gatherc=c; xs->gathers=s;}

char hex_table_initialized = FALSE;
static uint8 hex[256];

static void
il_init_hex_table(void)
{
    hex['0'] = 0; hex['1'] = 8;
    hex['2'] = 4; hex['3'] = 12;
    hex['4'] = 2; hex['5'] = 10;
    hex['6'] = 6; hex['7'] = 14;
    hex['8'] = 1; hex['9'] = 9;
    hex['A'] = hex['a'] = 5;
    hex['B'] = hex['b'] = 13;
    hex['C'] = hex['c'] = 3;
    hex['D'] = hex['d'] = 11;
    hex['E'] = hex['e'] = 7;
    hex['F'] = hex['f'] = 15;

    hex_table_initialized = TRUE;
}

int il_xbm_init(il_container *ic)
{
    xbm_struct *xs;

    if (!hex_table_initialized)
		il_init_hex_table();

    xs = XP_NEW_ZAP (xbm_struct);
    if (xs) 
	{
		xs->state = xbm_init;
		xs->gathers = xbm_error;
		ic->ds = xs;
    }
    return xs != 0;
}

static void
copyline(char *d, const unsigned char *s)
{
    int i=0;
    while( i++ < MAX_LINE && *s && *s != LF )
		*d++ = *s++;
    *d=0;
}

int il_xbm_write(il_container *ic, const unsigned char *buf, int32 len)
{
    int status, input_exhausted;
    xbm_struct *xs = (xbm_struct *)ic->ds;
    const unsigned char *q, *p=buf, *ep=buf+len;
	IL_Image *im = ic->image;
    char lbuf[MAX_LINE+1];

    /* If this assert fires, chances are the netlib screwed up and
       continued to send data after the image stream was closed. */
    XP_ASSERT(xs);
    if (!xs) {
#ifdef DEBUG
        XP_TRACE(("Netlib just took a shit on the imagelib\n"));
#endif
        return MK_IMAGE_LOSSAGE;
    }

    q = NULL;                   /* Initialize to shut up gcc warnings */
    input_exhausted = FALSE;
	
    while(!input_exhausted)
    {
		ILTRACE(9,("il:xbm: state %d len %d buf %u p %u ep %u",xs->state,len,buf,p,ep));

		switch(xs->state)
       	{
			case xbm_data:
		    	GETN(2,xbm_hex);	
				break;
	
			case xbm_hex:
				{
					*xs->p = (hex[q[1]]<<4) + hex[q[0]];
					xs->p++;
					xs->xpos+=8;
                        
					if(xs->xpos >= im->width)
					{
						xs->xpos=0;
						xs->ypos++;
#ifdef _USD
						xs->p = (unsigned char *)im->bits +
                            (im->height-xs->ypos)*(im->widthBytes);
#else
						xs->p = (unsigned char *) im->bits + xs->ypos*(im->widthBytes);
#endif
					}

					if(xs->ypos==im->height)
					{
						xs->state = xbm_done;
					}
					else
					{
						GETC('x',xbm_data);
					}
				}
				break;
		
			case xbm_init:
				GETC(LF,xbm_width);
				break;

			case xbm_width:
				{
					copyline(lbuf, q);
					if(sscanf(lbuf, "#define %*s %d", (int *)&im->width)==1) 
					{
						GETC(LF,xbm_height);
					}
					else
					{
                        /* Accept any of CR/LF, CR, or LF.
                           Allow multiple instances of each. */
						GETC(LF,xbm_width);
					}
				}
				break;
		
			case xbm_height:
				{
					copyline(lbuf, q);
					if(sscanf(lbuf, "#define %*s %d", (int *)&im->height)==1)
					{
						IL_RGB* map;
                        im->colors = 2;
						im->depth = 1;

						if(!im->transparent)
						{
							if((im->transparent = XP_NEW_ZAP(IL_IRGB))!=0)
							{
								im->transparent->index = 1;
                                /* Get context background color, if defined */
								im->transparent->red   = ic->ip->transparent.red;
								im->transparent->green = ic->ip->transparent.green;
								im->transparent->blue  = ic->ip->transparent.blue;
							}
						}
						
						if ((status = il_size(ic, im->width, im->height)) < 0)
						{
							if (status == MK_OUT_OF_MEMORY)
                                xs->state = xbm_oom;
                            else
                                xs->state = xbm_error;
							break;
						}
#ifdef _USD
						xs->p = (char *)im->bits +
                            (im->height-xs->ypos-1)*(im->widthBytes);
#else
						xs->p = (unsigned char*) im->bits;
#endif
						if((map = (IL_RGB*)XP_CALLOC(2, sizeof(IL_RGB)))!=0)
						{
							im->map = map;
							map++; /* first index is black */
							map->red = ic->ip->transparent.red;
							map->green = ic->ip->transparent.green;
							map->blue = ic->ip->transparent.blue;
						}
						GETC('{',xbm_start_data);
                        il_setup_color_space_converter(ic);
					}
					else
					{
                        /* Accept any of CR/LF, CR, or LF.
                           Allow multiple instances of each. */
						GETC(LF,xbm_height);
					}
				}
				break;

			case xbm_start_data:
				GETC('x',xbm_data);
				break;
	
			case xbm_gather:	
				{
					if(xs->gatherc)
					{
						const unsigned char *s;
                        /* We may need to look ahead one character, so don't point
                           at the last character in the buffer. */
						for(s = p; s < ep; s++)
						{
                            if (xs->gatherc == LF) 
                            {
                                if ((s[0] == LF) || (s[0] == CR))
                                {
                                    xs->gatherc = 0;
                                    break;
                                }
                            }
                            else if (s[0] == xs->gatherc)
                            {
                                xs->gatherc = 0;
                                break;
                            }
						}
                            
						if(xs->gatherc)
						{
							if((xs->gathered+ep-p) > MAX_HOLD)
							{
								/* we will never find it */
								xs->state = xbm_error;
							}
							else
							{
								XP_MEMCPY(xs->hold+xs->gathered, p, ep-p);
								xs->gathered += ep-p;
								input_exhausted = TRUE;
							}
						}
						else
						{		/* found it */
							if(xs->gathered)
							{
								XP_MEMCPY(xs->hold+xs->gathered, p, s-p+1);
								q = xs->hold;
							}
							else
							{
								q = p;
							}
							p = s+1;
							xs->gathered=0;
							xs->state=xs->gathers;
						}
					}

					if(xs->gathern)
					{
						if( (ep - p) >= xs->gathern)
						{		/* there is enough */
							if(xs->gathered)
							{	/* finish a prior gather */
								XP_MEMCPY(xs->hold+xs->gathered, p, xs->gathern);
								q = xs->hold;
								xs->gathered=0;
							}
							else
							{
								q = p;
							}
							p += xs->gathern;
							xs->gathern=0;
							xs->state=xs->gathers;
						}
						else
						{ 
							XP_MEMCPY(xs->hold+xs->gathered, p, ep-p);
							xs->gathered += ep-p;
							xs->gathern -= ep-p;
							input_exhausted = TRUE;
						}
					}
				}
				break;

			case xbm_done:
                il_partial(ic, 0, im->height, 0);
                return 0;

			case xbm_oom: 
				ILTRACE(1,("il:xbm: reached oom state"));
				return MK_OUT_OF_MEMORY;

			case xbm_error: 
				ILTRACE(2,("il:xbm: reached error state"));
				return MK_IMAGE_LOSSAGE;

			default: 
				ILTRACE(0,("il:xbm: unknown state %d", xs->state));
				XP_ASSERT(0);
				break;
			}
    }
    return 0;
}

void
il_xbm_abort(il_container *ic)
{
    if (ic->ds) 
	{
		xbm_struct *xs = (xbm_struct*) ic->ds;
		XP_FREE(xs);
		ic->ds = 0;
    }
}

void
il_xbm_complete(il_container *ic)
{
    il_xbm_abort(ic);
    il_image_complete(ic);
}
