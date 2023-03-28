/* -*- Mode: C; tab-width: 4 -*-
 *   if.c --- Top-level image library routines
 *   Copyright © 1997 Netscape Communications Corporation, all rights reserved.
 *   $Id: if.c,v 1.304.4.1.10.1 1997/03/06 13:34:32 jwz Exp $
 */

#include "xp.h"
#include "merrors.h"
#include "shist.h"
#include "if.h"

#ifdef MOCHA
#    include "libmocha.h"
#endif

extern int MK_UNABLE_TO_LOCATE_FILE;
extern int MK_OUT_OF_MEMORY;

#define HOWMANY(x, r)     (((x) + ((r) - 1)) / (r))
#define ROUNDUP(x, r)     (HOWMANY(x, r) * (r))

#ifdef PROFILE
#pragma profile on
#endif

int il_debug=0;

il_process *il_last_ip = NULL;
il_colorspace *il_last_cs = NULL;


#ifdef DEBUG_jwz
XP_Bool IL_AnimationLoopsEnabled = TRUE;   /* xfe/commands.c changes this */
XP_Bool IL_AnimationsEnabled = TRUE;
#endif /* DEBUG_jwz */


static int
il_init_scaling(il_container *ic)
{
    int scaled_width = ic->image->width;
        
    /* Allocate temporary scale space */
    if (scaled_width != ic->unscaled_width)
	{
        if (ic->scalerow)
            XP_FREE(ic->scalerow);
        
        if (!(ic->scalerow = (unsigned char *)XP_ALLOC(scaled_width * 3)))
		{
            ILTRACE(0,("il: MEM scale row"));
            return MK_OUT_OF_MEMORY;
        }
    }
    return 0;
}
    
int
il_size(il_container *ic, uint32 unscaled_width, uint32 unscaled_height)
{
    float aspect;
	il_client *c;
    int status;
    int scaled_width, scaled_height;
    int32 image_bytes, old_image_bytes;
	IL_Image *im = ic->image;
    IL_ImageStatus size_mode;

	/* Ensure that the image has a sane area. */
	if (!unscaled_width || !unscaled_height    ||
        (unscaled_width    > MAX_IMAGE_WIDTH)  ||
        (unscaled_height   > MAX_IMAGE_HEIGHT) ||
        (ic->layout_width  > MAX_IMAGE_WIDTH)  ||
        (ic->layout_height > MAX_IMAGE_HEIGHT))
    {
		ILTRACE(1,("il: bad image size: %dx%d",
                   unscaled_width, unscaled_height));
		return MK_IMAGE_LOSSAGE;
	}

	ic->unscaled_height = unscaled_height;
	ic->unscaled_width  = unscaled_width;

	if (ic->state == IC_MULTI)
    {
        ic->state = IC_SIZED;
		return 0;
	}

	ic->state = IC_SIZED;

    /* For now, we don't allow an image to change output size on the
     * fly, but we do allow the source image size to change, and thus
     * we may need to change the scaling factor to fit it into the
     * same size container on the display.
     *
     * Macintoshes, however, do their own scaling so we need to reset the
     * MacFE to tell it that it's processing a different-sized image.
     */
    if (ic->sized) {
        status = il_init_scaling(ic);
#ifndef XP_MAC
        return status;
#else
        if (status)
            return status;
#endif
    }

    old_image_bytes = (int32)ic->image->widthBytes * ic->image->height;

    /* Scale the image according to the IMG tag attributes,
     * unless scaling is disabled.
     */
    aspect = (float)unscaled_width/(float)unscaled_height;
    if ((!ic->layout_width && !ic->layout_height) || ic->ip->dontscale)
    {
        scaled_width  = ic->unscaled_width;
        scaled_height = ic->unscaled_height;
    }
    else if (ic->layout_width && ic->layout_height)
    {
        scaled_width  = ic->layout_width;
        scaled_height = ic->layout_height;
    }
    else if (ic->layout_width)
    {
        scaled_width  = ic->layout_width;
        scaled_height = (int)((float)ic->layout_width / aspect + 0.5);
    }
    else
    {
        scaled_width  = (int)(aspect * (float)ic->layout_height + 0.5);
        scaled_height = ic->layout_height;
    }

    if (scaled_width == 0)
        scaled_width = 1;
    
    if (scaled_height == 0)
        scaled_height = 1;

    im->width  = scaled_width;
    im->height = scaled_height;

	ILTRACE(2,("il: size %dx%d, scaled from %dx%d, ic = 0x%08x\n",
               scaled_width, scaled_height, unscaled_width, unscaled_height,
               ic));

    if ((status = il_init_scaling(ic)) < 0)
        return status;

	/* set bytes per pixel based on real width */
	if (ic->cs->bytes_per_pixel && im->depth!=1)
    {

#ifdef XP_MAC
        /* don't expand 8-bit gifs to 16-bits if we're in "raw" mode */
		if (ic->type == IL_GIF)
        {
			im->bytesPerPixel = 1;
			im->depth = 8;
		}
        else
#endif /* XP_MAC */
		{
			im->bytesPerPixel = ic->cs->bytes_per_pixel;
			im->depth = ic->cs->depth;
		}
        
		im->widthBytes = im->width * ic->cs->bytes_per_pixel;
	}
	else
	{ /* mono */
		im->bytesPerPixel = 1;
		im->widthBytes = (im->width+7)/8;
		im->colors = 2;
		im->depth = 1;
	}

    /* Image data and mask must be quadlet aligned for optimizations */
	im->maskWidthBytes = ROUNDUP((im->width+7)/8, 4);
    im->widthBytes = ROUNDUP(im->widthBytes, 4);

    size_mode = ic->sized ? ilReset : ilSetSize;
    ic->sized = 1;
    image_bytes = (int32)im->widthBytes * im->height;
    if (image_bytes - old_image_bytes)
        il_adjust_cache_fullness(image_bytes - old_image_bytes);

    /* for debugging 
    if (ic->type == IL_GIF && !im->transparent)
        im->transparent = (void*)1;
    */

	XP_ASSERT(ic->clients);
	for (c=ic->clients; c; c=c->next)
    {
		int err = FE_ImageSize(c->cx, size_mode, ic->image, c->client);
		if (err < 0)
            return MK_OUT_OF_MEMORY;
	}

	if (!im->transparent && im->mask)
    {
        XP_ASSERT(0);
		im->mask = 0;
	}

/* for debugging
   if (im->transparent == (void*)1)
       im->transparent = NULL;
*/

	/* Get memory for quantizing.  (required amount depends on image width) */
	if (ic->cs->mode == ilCI)
    {
		if (!il_init_quantize(ic))
        {
			ILTRACE(0,("il: MEM il_init_quantize"));
			return MK_OUT_OF_MEMORY;
		}
	}

	/* If its an internal/external reconnect, fabricate a title */
    il_set_view_image_doc_title(ic);

	return 0;
}
	
#define IL_SIZE_CHUNK	128
#ifdef XP_MAC
#    if GENERATINGPOWERPC
#        define IL_PREFERRED_CHUNK 8048 
#        define IL_OFFSCREEN_CHUNK 2048
#    else	/* normal mac */
#        define IL_PREFERRED_CHUNK 4048 
#        define IL_OFFSCREEN_CHUNK 512
#    endif
#else /* !XP_MAC */
#    define IL_PREFERRED_CHUNK 2048 
#    define IL_OFFSCREEN_CHUNK 512 
#endif

unsigned int
il_write_ready(void *data_object)
{
    uint request_size = 1;
	il_container *ic = (il_container *)data_object;

    if (ic->write_ready)
        request_size = (*ic->write_ready)(ic);

    if (!request_size)
        return 0;

	/*
     * It could be that layout aborted image loading by calling IL_FreeImage
     * before the netlib finished transferring data.  Don't do anything.
     */
	if (ic->state == IC_ABORT_PENDING)
        return IL_OFFSCREEN_CHUNK;

    /* Layout is blocked until the backdrop is loaded, so load that ASAP. */
	if (ic->is_backdrop)
		return IL_PREFERRED_CHUNK;

    /* This is a non-backdrop image, but the backdrop hasn't loaded yet. */
    if (ic->ip->backdrop_loading)
        return IL_OFFSCREEN_CHUNK;

	if (! ic->sized)
		/* A (small) default initial chunk */
		return IL_SIZE_CHUNK;

    if (ic->is_visible_on_screen)
        return IL_PREFERRED_CHUNK;
    
    /* Re-check to see if we are on-screen. */
    {
        il_client *c;
        for (c = ic->clients; c; c = c->next)
        {
            il_process *ip = c->cx->imageProcess; 
            if (FE_ImageOnScreen(c->cx, (LO_ImageStruct*)c->client))
            {
                ic->is_visible_on_screen = TRUE;
                ip->active++;
                ILTRACE(0,("il: %d active", ip->active));
                return IL_PREFERRED_CHUNK;
            }
		}

		/* This active stream is not currently on-screen;
         * Are other images now being loaded on-screen ?
         */
        for (c = ic->clients; c; c = c->next)
            if (c->cx->imageProcess->active)
                return IL_OFFSCREEN_CHUNK;

        return IL_PREFERRED_CHUNK;
	}
}

/* Given the first few bytes of a stream, identify the image format */
static int
il_type(int suspected_type, const char *buf, int32 len)
{
	int i;

	if (len >= 4 && !strncmp(buf, "GIF8", 4)) 
	{
		return IL_GIF;
	}

	/* JFIF files start with SOI APP0 but older files can start with SOI DQT
	 * so we test for SOI followed by any marker, i.e. FF D8 FF
	 * this will also work for SPIFF JPEG files if they appear in the future.
	 *
	 * (JFIF is 0XFF 0XD8 0XFF 0XE0 <skip 2> 0X4A 0X46 0X49 0X46 0X00)
     */
	if (len >= 3 &&
	   ((unsigned char)buf[0])==0xFF &&
	   ((unsigned char)buf[1])==0xD8 &&
	   ((unsigned char)buf[2])==0xFF)
	{
		return IL_JPEG;
	}

	/* no simple test for XBM vs, say, XPM so punt for now */
	if (len >= 8 && !strncmp(buf, "#define ", 8) ) 
	{
        /* Don't contradict the given type, since this ID isn't definitive */
        if ((suspected_type == IL_UNKNOWN) || (suspected_type == IL_XBM))
            return IL_XBM;
	}

	if (len < 35) 
	{
		ILTRACE(1,("il: too few bytes to determine type"));
		return suspected_type;
	}

	/* all the servers return different formats so root around */
	for (i=0; i<28; i++)
	{
		if (!strncmp(&buf[i], "Not Fou", 7))
			return IL_NOTFOUND;
	}
	
	return suspected_type;
}

/*
 *	determine what kind of image data we are dealing with
 */
int
IL_Type(const char *buf, int32 len)
{
    return il_type(IL_UNKNOWN, buf, len);
}

static int 
il_write(void *data_object, const unsigned char *str, int32 len)
{
	int err = 0;
    il_container *ic = (il_container *)data_object;

    ILTRACE(4, ("il: write with %5d bytes for %s\n", len, ic->url_address));

    /*
     * Layout may have decided to abort this image in mid-stream,
     * but netlib doesn't know about it yet and keeps sending
     * us data.  Force the netlib to abort.
     */
    if (ic->state == IC_ABORT_PENDING)
        return -1;

    /* Has user hit the stop button ? */
    if (il_image_stopped(ic))
        return -1;

    ic->bytes_consumed += len;
    
	if (len)
		err = (*ic->write)(ic, (unsigned char *)str, len);

    il_update_thermometer(ic);

	if (err < 0)
		return err;
	else
		return len;
}


int
il_first_write(void *dobj, const unsigned char *str, int32 len)
{
	il_container *ic = (il_container *)dobj;
	int (*init)(il_container *);

	XP_ASSERT(dobj == ic->stream->data_object);

	XP_ASSERT(ic);
	XP_ASSERT(ic->ip);
	XP_ASSERT(ic->image);

    /* Figure out the image type, possibly overriding the given MIME type */
    ic->type = il_type(ic->type, (const char*) str, len);
    ic->write_ready = NULL;

	/* Grab the URL's expiration date */
	if (ic->url)
	  ic->expires = ic->url->expires;

	switch (ic->type) 
	{
		case IL_GIF:
			init = il_gif_init;
			ic->write = il_gif_write;
			ic->complete = il_gif_complete;
            ic->write_ready = il_gif_write_ready;
            ic->abort = il_gif_abort;
			break;

		case IL_XBM:
			init = il_xbm_init;
			ic->write = il_xbm_write;
			ic->abort = il_xbm_abort;
            ic->complete = il_xbm_complete;
			break;

		case IL_JPEG:
			init = il_jpeg_init;
			ic->write = il_jpeg_write;
			ic->abort = il_jpeg_abort;
            ic->complete = il_jpeg_complete;
			break;

		case IL_NOTFOUND:
			ILTRACE(1,("il: html image"));
			return MK_IMAGE_LOSSAGE;

		default: 
			ILTRACE(1,("il: ignoring unknown image type (%d)", ic->type));
			return MK_IMAGE_LOSSAGE;
	}

	if (!(*init)(ic))
	{
		ILTRACE(0,("il: image init failed"));
		return MK_OUT_OF_MEMORY;
	}

	ic->stream->put_block = (MKStreamWriteFunc)il_write;

	/* do first write */
	return ic->stream->put_block(dobj, (const char*) str, len);
}

static void
il_bad_container(il_container *ic)
{
	il_client *c;

	ILTRACE(4,("il: bad container, sending icon"));
	for (c=ic->clients; c; c=c->next)
    {
        
		if (ic->type == IL_NOTFOUND)
        {
			ic->state = IC_MISSING;
			FE_ImageIcon(c->cx, IL_IMAGE_NOT_FOUND, c->client);
		}
		else
		{
			if (ic->state == IC_INCOMPLETE)
				FE_ImageIcon(c->cx, IL_IMAGE_DELAYED, c->client);
			else
				FE_ImageIcon(c->cx, IL_IMAGE_BAD_DATA, c->client);
		}
	}
	/* gets deleted later */
}


/*
 *	geturl completion callback
 */
static void
il_netgeturldone (URL_Struct *request, int status, MWContext *cx)
{
	il_container *ic = (il_container*)request->fe_data;
	XP_ASSERT(ic);

    /* CLM: Editor hack: 
     *      We want to load images while in a modal dialog (Apply button)
     *      we need a flag to allow front end to run the net until
     *      image is loaded.
     * TODO: This should not be needed when we convert to modeless dialogs in Dogbert
     */
    cx->edit_loading_url = FALSE;

	/*
     * It could be that layout aborted image loading by calling IL_FreeImage
     * before the netlib finished transferring data.  If so, really do the
     * freeing of the data that was deferred there.
     */
	if (ic->state == IC_ABORT_PENDING)
    {
		il_delete_container(ic, 1);
        NET_FreeURLStruct(request);
        return;
    }
    
	if (status < 0)
    {
		il_client *c;

		ILTRACE(2,("il:net done ic=0x%08x, status=%d, ic->state=0x%02x\n",
                   ic, status, ic->state));

        /* Netlib detected failure before a stream was even created. */
		if (ic->state < IC_BAD)
		{
			if (status == MK_OBJECT_NOT_IN_CACHE)
				ic->state = IC_NOCACHE;
			else if (status == MK_UNABLE_TO_LOCATE_FILE)
                ic->state = IC_MISSING;
            else {
                /* status is probably MK_INTERRUPTED */
				ic->state = IC_INCOMPLETE; /* try again on reload */
            }

#ifdef MOCHA
            {
                /* As far as JavaScript events go, we treat an image
                   that wasn't in the cache the same as if it was
                   actually loaded.  That way, a Javascript program
                   will run the same way with images turned on or off */
                LM_ImageEvent event =
                    (status == MK_INTERRUPTED) ? LM_IMGABORT :
                    (status == MK_OBJECT_NOT_IN_CACHE) ? LM_IMGLOAD :
                    LM_IMGERROR;
                    
                for (c = ic->clients; c; c = c->next)
                    LM_SendImageEvent(c->cx, c->client, event);
            }
#endif

			if (! ic->sized)
			{
				for (c=ic->clients; c; c=c->next)
				{
					if ((status == MK_INTERRUPTED) ||
					    (status == MK_OBJECT_NOT_IN_CACHE))
					{
						FE_ImageIcon(c->cx, IL_IMAGE_DELAYED, c->client);
					}
					else if (status == MK_UNABLE_TO_LOCATE_FILE)
					{
						FE_ImageIcon(c->cx, IL_IMAGE_NOT_FOUND, c->client);
					}
					else
					{
						FE_ImageIcon(c->cx, IL_IMAGE_BAD_DATA, c->client);
					}
				}
			}
		}

		/* for mac */
		if (status == MK_OUT_OF_MEMORY)
			NET_InterruptWindow(cx);
	}

	ic->forced = 0;

	NET_FreeURLStruct(request);
}

void
il_image_abort(il_container *ic)
{
    if (ic->abort)
        (*ic->abort)(ic); 

    /* Clear any pending timeouts */
    if (ic->row_output_timeout) {
        FE_ClearTimeout(ic->row_output_timeout);
        ic->row_output_timeout = NULL;
    }

	ic->is_looping = FALSE;
}

void
il_stream_complete(void *data_object)
{
	il_container *ic = (il_container *)data_object;

	XP_ASSERT(ic);

    XP_ASSERT(data_object == ic->stream->data_object);

    ILTRACE(1, ("il: complete: %d seconds for %s\n",
                XP_TIME() - ic->start_time, ic->url_address));

	if (ic->complete)
        (*ic->complete)(ic);
    else
        il_image_complete(ic);

    ic->stream = NULL;
}

void
il_image_complete(il_container *ic)
{
	/*
     * It could be that layout aborted image loading by calling IL_FreeImage()
     * before the netlib finished transferring data.  Don't do anything.
     */
	if (ic->state == IC_ABORT_PENDING)
        goto no_more_images_for_this_ic;

    /* If we didn't size the image, but the stream finished loading, the
       image must be corrupt or truncated. */
	if (ic->state < IC_SIZED)
	{
		ic->state = IC_BAD;
		il_bad_container(ic);

#ifdef MOCHA
        {
            il_client *c;
            for (c = ic->clients; c; c = c->next) {
                LM_SendImageEvent(c->cx, c->client, LM_IMGERROR);
            }
        }
#endif

        goto done;
	}
	else
	{
		Bool multi;

		XP_ASSERT(ic->state == IC_SIZED ||
                  ic->state == IC_HIRES ||
                  ic->state == IC_MULTI);
		XP_ASSERT(ic->image && ic->image->bits);

        /* Is there a next part in a multipart MIME message ? */
		multi = ic->stream && ic->stream->is_multipart;

		ILTRACE(1,("il: complete %d image width %d (%d) height %d,"
                   " depth %d, %d colors",
				   ic->multi,
				   ic->image->width, ic->image->widthBytes, ic->image->height,
                   ic->image->depth, 
				   ic->image->colors));

		/* 4 cases: simple, hi/lo, multipart MIME, multimage animation */
		if (ic->hiurl || multi || ic->loop_count)
		{
            il_client *c;
			URL_Struct *netRequest = NULL;

            /* Display the rest of the last image before starting a new one */
			il_flush_image_data(ic, ilPartialData);

            /* Force new colormap to be loaded in case its different from the LOSRC
             * or previous images in the multipart message.
             * XXX - fur - Shouldn't do this for COLORMAP case.
             */
            il_reset_palette(ic);

            FREE_IF_NOT_NULL(ic->image->map);
            FREE_IF_NOT_NULL(ic->image->transparent);
            FREE_IF_NOT_NULL(ic->comment);
            ic->comment_length = 0;

            /* Handle looping, which can be used to replay an animation. */
            if (ic->loop_count) {
                URL_Struct *netRequest;
                int is_infinite_loop = (ic->loop_count == -1);
                
                if (!is_infinite_loop)
                    ic->loop_count--;
                
                ILTRACE(1,("il: loop %s", ic->url_address));

                netRequest =
                    NET_CreateURLStruct (ic->url_address, NET_DONT_RELOAD);
                if (!netRequest)    /* OOM */
                    goto no_more_images_for_this_ic;

                /* Only loop if the image stream is available locally.
                   Also, if the user hit the "stop" button, don't
                   allow the animation to loop. */
                if ((NET_IsLocalFileURL(ic->url_address)   ||
                     NET_IsURLInMemCache(netRequest)       ||
                     NET_IsURLInDiskCache(netRequest))          &&
                    (! il_image_stopped(ic))                    &&
#ifdef DEBUG_jwz
                    (IL_AnimationsEnabled && IL_AnimationLoopsEnabled) &&
#endif /* DEBUG_jwz */
                    ((ic->cx)                              &&
                     (ic->cx->type != MWContextPrint)      &&
                     (ic->cx->type != MWContextPostScript) &&
                     (ic->cx->type != MWContextMetaFile)))
                {
                    /* Suppress thermo & progress bar */
                    netRequest->load_background = TRUE;
                    netRequest->fe_data = (void *)ic;
                    ic->url = netRequest;

                    ic->bytes_consumed = 0;
                    
                    ic->state = IC_START;
                    ic->is_looping = TRUE;
                    (void) NET_GetURL(ic->url, FO_CACHE_AND_INTERNAL_IMAGE,
                                      ic->cx, &il_netgeturldone);
                    goto done;
                } else {
                    ic->loop_count = 0;
					ic->is_looping = FALSE;
                    NET_FreeURLStruct(netRequest);
                }
            }
			else
                ic->is_looping = FALSE;			 

			if (multi)
            {
				ic->multi++;
				ic->state = IC_MULTI;
                goto done;
			}
            else if (ic->hiurl)
			{
				ILTRACE(1,("il: get hi %s", ic->hiurl));

				netRequest = NET_CreateURLStruct (ic->hiurl, NET_DONT_RELOAD);
				if (!netRequest)
                    goto no_more_images_for_this_ic;

				/* Add the referer to the URL. */
                for (c = ic->clients; c; c = c->next)
				{
					History_entry *he =	SHIST_GetCurrent (&c->cx->hist);
					if (netRequest->referer)
						XP_FREE (netRequest->referer);
					if (he && he->address)
						netRequest->referer = XP_STRDUP (he->address);
					else
						netRequest->referer = 0;
				}

				/* save away the container */
				netRequest->fe_data = (void *)ic;
                ic->is_looping = FALSE;
				(void) NET_GetURL(netRequest, FO_CACHE_AND_INTERNAL_IMAGE,
                                  ic->cx, &il_netgeturldone);
				ic->url = netRequest;

				ic->state = IC_HIRES;
				if (ic->hiurl) { XP_FREE(ic->hiurl); ic->hiurl=0; }
			}

            goto done;
		}
	}
    
  no_more_images_for_this_ic:
    
    ic->stream = NULL;

    /* normal case */
    if (ic->is_visible_on_screen)
        {
            il_client *c;
            for (c = ic->clients; c; c = c->next) {
                il_process *ip = c->cx->imageProcess;
                if (--ip->active < 0)
                    ip->active = 0;
            }
            ic->is_visible_on_screen = 0;
        }

    /* XXX - This should be done on a per-client basis */
    if (ic->is_backdrop)
        ic->ip->backdrop_loading = FALSE;
    
    if (ic->state != IC_ABORT_PENDING)
        il_flush_image_data(ic, ilComplete);
    il_scour_container(ic);
    if (!ic->multi)
        ic->state = IC_COMPLETE;

  done:
    
    /* Clear any pending timeouts */
    if (ic->row_output_timeout) {
        FE_ClearTimeout(ic->row_output_timeout);
        ic->row_output_timeout = NULL;
    }

#ifdef MOCHA
    {
        il_client *c;
        for (c = ic->clients; c; c = c->next) {
            LM_SendImageEvent(c->cx, c->client, LM_IMGLOAD);
        }
    }
#endif
}


void 
il_abort(void *data_object, int status)
{
    int old_state;
    il_client *c;
	il_container *ic = (il_container *)data_object;

	XP_ASSERT(ic);

	ILTRACE(4,("il: abort, status=%d ic->state=%d", status, ic->state))
    il_image_abort(ic);

	/* It's possible that the stream is zero
	   because this container is scoured already */
	if (ic->stream)
		ic->stream->data_object = 0;
    ic->stream = NULL;

	if (ic->is_visible_on_screen)
	{
        for (c = ic->clients; c; c = c->next) {
            il_process *ip = c->cx->imageProcess;
            if (--ip->active < 0)
                ip->active = 0;
        }
		ic->is_visible_on_screen = FALSE;
	}

#ifdef MOCHA
    for (c = ic->clients; c; c = c->next) {
        LM_SendImageEvent(c->cx, c->client,
                          status == MK_INTERRUPTED ? LM_IMGABORT : LM_IMGERROR);
    }
#endif

	/*
     * It could be that layout aborted image loading by calling IL_FreeImage()
     * before the netlib finished transferring data.  Don't do anything.
     */
	if (ic->state == IC_ABORT_PENDING)
		return;

    old_state = ic->state;
        
    if ((status == MK_INTERRUPTED) || (status == MK_OUT_OF_MEMORY))
        ic->state = IC_INCOMPLETE;
    else
        ic->state = IC_BAD;
    
	if (old_state < IC_SIZED)
		il_bad_container(ic);
}


NET_StreamClass *
IL_NewStream (FO_Present_Types format_out,
              void *type,
              URL_Struct *urls,
              MWContext *cx)
{
	NET_StreamClass *stream = nil;
	il_container *ic = nil;

	/* recover the container */
	ic = (il_container*)urls->fe_data;

	XP_ASSERT(ic);

	/*
     * It could be that layout aborted image loading by calling IL_FreeImage()
     * before the netlib finished transferring data.  Don't do anything.
     */
	if (ic->state == IC_ABORT_PENDING)
		return NULL;
	
	/* Create stream object */
	if (!(stream = XP_NEW_ZAP(NET_StreamClass))) 
	{
		ILTRACE(0,("il: MEM il_newstream"));
		return 0;
	}

	ic->type = (int)type;
	ic->stream = stream;
    XP_ASSERT(ic->cx == cx);
	ic->cx = cx;
	ic->state = IC_STREAM;
	ic->content_length = urls->content_length;

#ifdef XP_MAC
    ic->image->hasUniqueColormap = FALSE;
#endif
	
	ILTRACE(4,("il: new stream, type %d, %s", ic->type, urls->address));

	stream->name		   = "image decode";
	stream->complete	   = il_stream_complete;
	stream->abort		   = il_abort;
	stream->is_write_ready = il_write_ready;
	stream->data_object	   = (void *)ic;
	stream->window_id	   = cx;
	stream->put_block	   = (MKStreamWriteFunc) il_first_write;

	return stream;
}


/*	Phong's linear congruential hash  */
uint32
il_hash(const char *ubuf)
{
	unsigned char * buf = (unsigned char*) ubuf;
	uint32 h=1;
	while(*buf)
	{
		h = 0x63c63cd9*h + 0x9c39c33d + (int32)*buf;
		buf++;
	}
	return h;
}

#define IL_LAST_ICON 61
/* Extra factor of 7 is to account for duplications between
   mc-icons and ns-icons */
static uint32 il_icon_table[(IL_LAST_ICON + 7) * 2];

static void
il_setup_icon_table(void)
{
    int inum = 0;

	/* gopher/ftp icons */
	il_icon_table[inum++] = il_hash("internal-gopher-text");
	il_icon_table[inum++] = IL_GOPHER_TEXT;
	il_icon_table[inum++] = il_hash("internal-gopher-image");
	il_icon_table[inum++] = IL_GOPHER_IMAGE;
	il_icon_table[inum++] = il_hash("internal-gopher-binary");
	il_icon_table[inum++] = IL_GOPHER_BINARY;
	il_icon_table[inum++] = il_hash("internal-gopher-sound");
	il_icon_table[inum++] = IL_GOPHER_SOUND;
	il_icon_table[inum++] = il_hash("internal-gopher-movie");
	il_icon_table[inum++] = IL_GOPHER_MOVIE;
	il_icon_table[inum++] = il_hash("internal-gopher-menu");
	il_icon_table[inum++] = IL_GOPHER_FOLDER;
	il_icon_table[inum++] = il_hash("internal-gopher-index");
	il_icon_table[inum++] = IL_GOPHER_SEARCHABLE;
	il_icon_table[inum++] = il_hash("internal-gopher-telnet");
	il_icon_table[inum++] = IL_GOPHER_TELNET;
	il_icon_table[inum++] = il_hash("internal-gopher-unknown");
	il_icon_table[inum++] = IL_GOPHER_UNKNOWN;

	/* news icons */
	il_icon_table[inum++] = il_hash("internal-news-catchup-group");
	il_icon_table[inum++] = IL_NEWS_CATCHUP;
	il_icon_table[inum++] = il_hash("internal-news-catchup-thread");
	il_icon_table[inum++] = IL_NEWS_CATCHUP_THREAD;
	il_icon_table[inum++] = il_hash("internal-news-followup");
	il_icon_table[inum++] = IL_NEWS_FOLLOWUP;
	il_icon_table[inum++] = il_hash("internal-news-go-to-newsrc");
	il_icon_table[inum++] = IL_NEWS_GOTO_NEWSRC;
	il_icon_table[inum++] = il_hash("internal-news-next-article");
	il_icon_table[inum++] = IL_NEWS_NEXT_ART;
	il_icon_table[inum++] = il_hash("internal-news-next-article-gray");
	il_icon_table[inum++] = IL_NEWS_NEXT_ART_GREY;
	il_icon_table[inum++] = il_hash("internal-news-next-thread");
	il_icon_table[inum++] = IL_NEWS_NEXT_THREAD;
	il_icon_table[inum++] = il_hash("internal-news-next-thread-gray");
	il_icon_table[inum++] = IL_NEWS_NEXT_THREAD_GREY;
	il_icon_table[inum++] = il_hash("internal-news-post");
	il_icon_table[inum++] = IL_NEWS_POST;
	il_icon_table[inum++] = il_hash("internal-news-prev-article");
	il_icon_table[inum++] = IL_NEWS_PREV_ART;
	il_icon_table[inum++] = il_hash("internal-news-prev-article-gray");
	il_icon_table[inum++] = IL_NEWS_PREV_ART_GREY;
	il_icon_table[inum++] = il_hash("internal-news-prev-thread");
	il_icon_table[inum++] = IL_NEWS_PREV_THREAD;
	il_icon_table[inum++] = il_hash("internal-news-prev-thread-gray");
	il_icon_table[inum++] = IL_NEWS_PREV_THREAD_GREY;
	il_icon_table[inum++] = il_hash("internal-news-reply");
	il_icon_table[inum++] = IL_NEWS_REPLY;
	il_icon_table[inum++] = il_hash("internal-news-rtn-to-group");
	il_icon_table[inum++] = IL_NEWS_RTN_TO_GROUP;
	il_icon_table[inum++] = il_hash("internal-news-show-all-articles");
	il_icon_table[inum++] = IL_NEWS_SHOW_ALL_ARTICLES;
	il_icon_table[inum++] = il_hash("internal-news-show-unread-articles");
	il_icon_table[inum++] = IL_NEWS_SHOW_UNREAD_ARTICLES;
	il_icon_table[inum++] = il_hash("internal-news-subscribe");
	il_icon_table[inum++] = IL_NEWS_SUBSCRIBE;
	il_icon_table[inum++] = il_hash("internal-news-unsubscribe");
	il_icon_table[inum++] = IL_NEWS_UNSUBSCRIBE;
	il_icon_table[inum++] = il_hash("internal-news-newsgroup");
	il_icon_table[inum++] = IL_NEWS_FILE;
	il_icon_table[inum++] = il_hash("internal-news-newsgroups");
	il_icon_table[inum++] = IL_NEWS_FOLDER;

	/* httpd file icons */
	il_icon_table[inum++] = il_hash("/mc-icons/menu.gif");
	il_icon_table[inum++] = IL_GOPHER_FOLDER;
	il_icon_table[inum++] = il_hash("/mc-icons/unknown.gif");  
	il_icon_table[inum++] = IL_GOPHER_UNKNOWN;
	il_icon_table[inum++] = il_hash("/mc-icons/text.gif");	
	il_icon_table[inum++] = IL_GOPHER_TEXT;
	il_icon_table[inum++] = il_hash("/mc-icons/image.gif"); 
	il_icon_table[inum++] = IL_GOPHER_IMAGE;
	il_icon_table[inum++] = il_hash("/mc-icons/sound.gif");	 
	il_icon_table[inum++] = IL_GOPHER_SOUND;
	il_icon_table[inum++] = il_hash("/mc-icons/movie.gif");	 
	il_icon_table[inum++] = IL_GOPHER_MOVIE;
	il_icon_table[inum++] = il_hash("/mc-icons/binary.gif"); 
	il_icon_table[inum++] = IL_GOPHER_BINARY;

    /* Duplicate httpd icons, but using new naming scheme. */
	il_icon_table[inum++] = il_hash("/ns-icons/menu.gif");
	il_icon_table[inum++] = IL_GOPHER_FOLDER;
	il_icon_table[inum++] = il_hash("/ns-icons/unknown.gif");  
	il_icon_table[inum++] = IL_GOPHER_UNKNOWN;
	il_icon_table[inum++] = il_hash("/ns-icons/text.gif");	
	il_icon_table[inum++] = IL_GOPHER_TEXT;
	il_icon_table[inum++] = il_hash("/ns-icons/image.gif"); 
	il_icon_table[inum++] = IL_GOPHER_IMAGE;
	il_icon_table[inum++] = il_hash("/ns-icons/sound.gif");	 
	il_icon_table[inum++] = IL_GOPHER_SOUND;
	il_icon_table[inum++] = il_hash("/ns-icons/movie.gif");	 
	il_icon_table[inum++] = IL_GOPHER_MOVIE;
	il_icon_table[inum++] = il_hash("/ns-icons/binary.gif"); 
	il_icon_table[inum++] = IL_GOPHER_BINARY;

	/* ... and names for all the image icons */
	il_icon_table[inum++] = il_hash("internal-icon-delayed"); 
	il_icon_table[inum++] = IL_IMAGE_DELAYED;
	il_icon_table[inum++] = il_hash("internal-icon-notfound"); 
	il_icon_table[inum++] = IL_IMAGE_NOT_FOUND;
	il_icon_table[inum++] = il_hash("internal-icon-baddata"); 
	il_icon_table[inum++] = IL_IMAGE_BAD_DATA;
	il_icon_table[inum++] = il_hash("internal-icon-insecure"); 
	il_icon_table[inum++] = IL_IMAGE_INSECURE;
	il_icon_table[inum++] = il_hash("internal-icon-embed"); 
	il_icon_table[inum++] = IL_IMAGE_EMBED;

	/* This belongs up in the `news icons' section */
	il_icon_table[inum++] = il_hash("internal-news-followup-and-reply");
	il_icon_table[inum++] = IL_NEWS_FOLLOWUP_AND_REPLY;

	/* editor icons. */
	il_icon_table[inum++] = il_hash("internal-edit-named-anchor");
	il_icon_table[inum++] = IL_EDIT_NAMED_ANCHOR;
	il_icon_table[inum++] = il_hash("internal-edit-form-element");
	il_icon_table[inum++] = IL_EDIT_FORM_ELEMENT;
	il_icon_table[inum++] = il_hash("internal-edit-unsupported-tag");
	il_icon_table[inum++] = IL_EDIT_UNSUPPORTED_TAG;
	il_icon_table[inum++] = il_hash("internal-edit-unsupported-end-tag");
	il_icon_table[inum++] = IL_EDIT_UNSUPPORTED_END_TAG;
	il_icon_table[inum++] = il_hash("internal-edit-java");
	il_icon_table[inum++] = IL_EDIT_JAVA;
	il_icon_table[inum++] = il_hash("internal-edit-PLUGIN");
	il_icon_table[inum++] = IL_EDIT_PLUGIN;

	/* Security Advisor and S/MIME icons */
	il_icon_table[inum++] = il_hash("internal-sa-signed");
    il_icon_table[inum++] = IL_SA_SIGNED;
	il_icon_table[inum++] = il_hash("internal-sa-encrypted");
    il_icon_table[inum++] = IL_SA_ENCRYPTED;
	il_icon_table[inum++] = il_hash("internal-sa-nonencrypted");
    il_icon_table[inum++] = IL_SA_NONENCRYPTED;
	il_icon_table[inum++] = il_hash("internal-sa-signed-bad");
    il_icon_table[inum++] = IL_SA_SIGNED_BAD;
	il_icon_table[inum++] = il_hash("internal-sa-encrypted-bad");
    il_icon_table[inum++] = IL_SA_ENCRYPTED_BAD;
	il_icon_table[inum++] = il_hash("internal-smime-attached");
    il_icon_table[inum++] = IL_SMIME_ATTACHED;
	il_icon_table[inum++] = il_hash("internal-smime-signed");
    il_icon_table[inum++] = IL_SMIME_SIGNED;
	il_icon_table[inum++] = il_hash("internal-smime-encrypted");
    il_icon_table[inum++] = IL_SMIME_ENCRYPTED;
	il_icon_table[inum++] = il_hash("internal-smime-encrypted-signed");
    il_icon_table[inum++] = IL_SMIME_ENC_SIGNED;
	il_icon_table[inum++] = il_hash("internal-smime-signed-bad");
    il_icon_table[inum++] = IL_SMIME_SIGNED_BAD;
	il_icon_table[inum++] = il_hash("internal-smime-encrypted-bad");
    il_icon_table[inum++] = IL_SMIME_ENCRYPTED_BAD;
	il_icon_table[inum++] = il_hash("internal-smime-encrypted-signed-bad");
    il_icon_table[inum++] = IL_SMIME_ENC_SIGNED_BAD;

    XP_ASSERT(inum <= (sizeof(il_icon_table) / sizeof(il_icon_table[0])));
}


static uint32
il_internal_image(const char *image_url)
{
	int i;
    uint32 hash = il_hash(image_url);
	if (il_icon_table[0]==0)
		il_setup_icon_table();

	for (i=0; i< (sizeof(il_icon_table) / sizeof(il_icon_table[0])); i++)
	{
		if (il_icon_table[i<<1] == hash)
		{
			return il_icon_table[(i<<1)+1];
		}
	}
	return 0;
}

void
IL_DeleteImages(MWContext *cx)
{
}


Bool
IL_PreferredStream(URL_Struct *urls)
{
	il_container *ic = 0;
	il_client *c;

	XP_ASSERT(urls);
	if (urls)
	{
		/* xxx this MUST be an image stream */
		ic = (il_container*)urls->fe_data;		
		XP_ASSERT(ic);
		if (ic)
		{
            /*
             * It could be that layout aborted image loading by
             * calling IL_FreeImage before the netlib finished
             * transferring data.  Don't do anything.
             */
            if (ic->state == IC_ABORT_PENDING)
                return FALSE;

			/* discover if layout is blocked on this image */
			for (c=ic->clients; c; c=c->next)
			{
				if ((LO_BlockedOnImage(c->cx,
                                      (LO_ImageStruct*)c->client) == TRUE) ||
				   FE_ImageOnScreen(c->cx, (LO_ImageStruct*)c->client) )
					return TRUE;
			}
		}
	}
	return FALSE;
}

int
IL_GetImage (const char* image_url,
             MWContext* cx,
             LO_ImageStruct *lo_image,
             NET_ReloadMethod cache_reload_policy)
{
    int layout_width, layout_height;
    IL_IRGB *background_color;

    URL_Struct *urls = NULL;
	void *clientData = (void *)lo_image;

	il_container *ic = NULL;
    il_process *ip;
    il_colorspace *cs = NULL;
	int err, is_backdrop;
    int is_internal_external_reconnect = FALSE;
    int is_view_image;
    const char *lowres_image_url = NULL;

	/* xxx generate a default processor */
	if (!cx->imageProcess)
		cx->imageProcess = il_last_ip;

	if (!cx->colorSpace)
		cx->colorSpace = il_last_cs;

    cs = cx->colorSpace;
    ip = cx->imageProcess;

    ILTRACE(1, ("il: IL_GetImage, url=%s\n", image_url));

    /* XXX - fur - When can this happen ? */
	if (!image_url)
	{
		ILTRACE(0,("il: no url, sending delayed icon"));
		FE_ImageIcon(cx, IL_IMAGE_DELAYED, clientData);
        lo_image->il_image = NULL;
		return 0;
	}

    /* Check for any special internal-use URLs */
	if (*image_url == 'i'                  ||
        !XP_STRNCMP(image_url, "/mc-", 4)  ||
        !XP_STRNCMP(image_url, "/ns-", 4))
	{
		uint32 icon;

        /* A built-in icon ? */
		if ((icon = il_internal_image(image_url)))
		{
			ILTRACE(4,("il: internal icon %d", icon));
			FE_ImageIcon(cx, icon, clientData);
            lo_image->il_image = NULL;
			lo_image->image_attr->attrmask |= LO_ATTR_INTERNAL_IMAGE;
			return 0;
		}

        /* Image viewer URLs look like "internal-external-reconnect:REALURL.gif"
         * Strip off the prefix to get the real URL name.
         */
		if (!XP_STRNCMP(image_url, "internal-external-reconnect:", 28)) {
            image_url += 28;
			is_internal_external_reconnect = TRUE;
        }
	}

    if (!ip->nolowsrc)
        lowres_image_url = (const char *)lo_image->lowres_image_url;

    /* Locate image in cache or currently loading page */
    layout_width  = 
        ip->dontscale ? 0 : (int)(lo_image->width   / cx->convertPixX);
    layout_height =
        ip->dontscale ? 0 : (int)(lo_image->height  / cx->convertPixY);
    background_color = &ip->transparent;
     
    is_backdrop = (lo_image->image_attr->attrmask & LO_ATTR_BACKDROP) != 0;

    ic = il_get_container(cx, cache_reload_policy, image_url,
                          lowres_image_url, background_color,
                          ip->dither_mode, LO_HasBGImage(cx) && !is_backdrop,
                          cs->depth, layout_width, layout_height);
    if (!ic)
    {
        ILTRACE(0,("il: MEM il_container"));
        FE_ImageIcon(cx, IL_IMAGE_DELAYED, clientData);
        if (is_internal_external_reconnect)
            il_abort_reconnect();
        lo_image->il_image = NULL;
        return -1;
    }
     
    /* Give layout a handle into the imagelib world. */
    lo_image->il_image = ic->image;
    
    /* A user-specified default backdrop image, presumably a local file. */
    if (lo_image->image_attr->attrmask & LO_ATTR_STICKY)
        ic->is_user_backdrop = TRUE;

    /* Is this a non-default backdrop image ? */
    ic->is_backdrop = is_backdrop;

    /* Is this a call to the image viewer ? */
    is_view_image = is_internal_external_reconnect &&
        (cx->type != MWContextNews) && (cx->type != MWContextMail);
    if (il_add_client(cx, ic, clientData, is_view_image))
    {
        FE_ImageIcon(cx, IL_IMAGE_DELAYED, clientData);
        if (is_internal_external_reconnect)
            il_abort_reconnect();
        return -1;
    }

    /* Record the last context and imageProcess used against. */
    ic->ip = ip;
    XP_ASSERT(cx->colorSpace);
    ic->cs = cx->colorSpace;
    

    /* If the image is already in memory ... */
	if (ic->state != IC_VIRGIN) {
        switch (ic->state) {
        case IC_BAD:
#ifdef MOCHA
            LM_SendImageEvent(cx, clientData, LM_IMGERROR);
#endif
            FE_ImageIcon(cx, IL_IMAGE_BAD_DATA, clientData);
            break;

        case IC_MISSING:
#ifdef MOCHA
            LM_SendImageEvent(cx, clientData, LM_IMGERROR);
#endif
			FE_ImageIcon(cx, IL_IMAGE_NOT_FOUND, clientData);
            XP_ASSERT(! is_internal_external_reconnect);
            break;

        case IC_INCOMPLETE:
#ifdef MOCHA
            LM_SendImageEvent(cx, clientData, LM_IMGERROR);
#endif
            FE_ImageIcon(cx, IL_IMAGE_DELAYED, clientData);
            break;

        case IC_SIZED:
        case IC_HIRES:
        case IC_MULTI:
			FE_ImageSize(cx, ilSetSize, ic->image, clientData);
            break;

        case IC_COMPLETE:
            FE_ImageSize(cx, ilSetSize, ic->image, clientData);
            il_set_color_palette(cx, ic);
            FE_ImageData(cx, ilComplete, ic->image, clientData, 0, 0, FALSE);
#ifdef MOCHA
            LM_SendImageEvent(cx, clientData, LM_IMGLOAD);
#endif
            if (is_view_image)
            {
                /* Fabricate a title for the document window. */
                il_set_view_image_doc_title(ic);
            }
            break;

        case IC_START:
        case IC_STREAM:
        case IC_ABORT_PENDING:
        case IC_NOCACHE:
            break;
            
        default:
            XP_ASSERT(0);
            return -1;
        }

        if (is_internal_external_reconnect) {
            /* Since we found the image in the cache, we don't
             * need any of the data now streaming into the image viewer.
             */
            il_abort_reconnect();
        }

		/* NOCACHE falls through to be tried again */
        if (ic->state != IC_NOCACHE)
            return 0;
    }

    /* This is a virgin (never-used) image container. */
	else
    {
		ic->forced = FORCE_RELOAD(cache_reload_policy);

        /* Is this a non-default backdrop image ? */
		if (ic->is_backdrop)
            ic->ip->backdrop_loading = TRUE;
	}

	ic->state = IC_START;
    
#ifdef DEBUG
    ic->start_time = XP_TIME();
#endif

    /* Record the context that actually initiates and controls the transfer. */
    ic->cx = cx;

	if (is_internal_external_reconnect)
	{
        /* "Reconnect" to the stream feeding IL_ViewStream(), already created. */
        il_reconnect(ic);
        return 0;
	}
        
    /* need to make a net request */
    ILTRACE(1,("il: net request for %s, %s", image_url,
               ic->forced ? "force" : ""));

    urls = NET_CreateURLStruct(image_url, cache_reload_policy);

    if (!urls)
    {
        /* xxx clean up previously allocated objects */
        return MK_OUT_OF_MEMORY;
    }
    
    if (lowres_image_url && !NET_IsURLInDiskCache(urls))
    {
        /* do the lowsrc thing */
        ic->hiurl = (char*)XP_STRDUP(image_url);

        NET_FreeURLStruct(urls);
        urls=NULL;

        ILTRACE(1,("il: net request lowsrc %s",
                   (const char *)lo_image->lowres_image_url));

        /* lowres_image_url is really an XP_Block */
        urls = NET_CreateURLStruct (lowres_image_url, cache_reload_policy);
        
        if (!urls) 
            return MK_OUT_OF_MEMORY;
    }

    /* Add the referer to the URL. */
    {
        History_entry *he = SHIST_GetCurrent (&cx->hist);
        if (urls->referer)
			XP_FREE (urls->referer);
        if (he && he->address)
			urls->referer = XP_STRDUP (he->address);
        else
			urls->referer = 0;
    }

    /* save away the container */
    urls->fe_data = (void *)ic;
    ic->url = urls;

    /* CLM: Editor hack: 
     *      We want to load images while in a modal dialog (Apply button)
     *      we need a flag to allow front end to run the net until
     *      image is loaded.
     * TODO: This should not be needed when we convert to modeless dialogs in Dogbert
     */
    cx->edit_loading_url = TRUE;
    
    ic->is_looping = FALSE;
    err = NET_GetURL(urls, (cache_reload_policy == NET_CACHE_ONLY_RELOAD) ?
                     FO_ONLY_FROM_CACHE_AND_INTERNAL_IMAGE :
                     FO_CACHE_AND_INTERNAL_IMAGE, 
                     cx, &il_netgeturldone);
	return 0;
}

int
IL_SetTransparent(MWContext *cx, IL_IRGB *transparent)
{
	il_process *ip = cx->imageProcess;
    il_colorspace *cs = cx->colorSpace;

	if (!ip)
		return -1;

	if ((cs->mode == ilGrey) && (cs->depth == 1)) {
        /* Override */
        ip->transparent.red = 0xff;
        ip->transparent.green = 0xff;
        ip->transparent.blue = 0xff;
        return 0;
    }
    


	if (transparent)
	{
		XP_MEMCPY(&ip->transparent, transparent, sizeof(IL_IRGB));
	}
	return 0;
}

/* XXX - remove this API function - It is now totally ignored. */

void
IL_SetByteOrder(MWContext *cx, Bool ls_bit_first, Bool ls_byte_first)
{
}

void
IL_DisableScaling(MWContext *cx)
{
	il_process *ip;

	if (!cx)
		return;

	if (!(ip=cx->imageProcess))
		return;

	ip->dontscale = 1;
}

void
IL_DisableLowSrc(MWContext *cx)
{
	il_process *ip;

	if (!cx)
		return;

	if (!(ip=cx->imageProcess))
		return;

	ip->nolowsrc = 1;
}

void
IL_SetPreferences(MWContext *cx, Bool incrementalDisplay,
                  IL_DitherMode dither_mode)
{
	il_process *ip;
    il_colorspace *cs;

	if (!cx)
		return;

	if (!(ip = cx->imageProcess))
		return;
	if (!(cs = cx->colorSpace))
		return;

    ip->progressive_display = incrementalDisplay;

    /* Dithering always on in monochrome mode. */
    if (cs->depth == 1)
        ip->dither_mode = ilDither;
    else
        ip->dither_mode = dither_mode;
}

/* Interrupts all image loading in a context.
   Specifically, stops all looping image animations.    */
void
IL_InterruptContext(MWContext *cx)
{
    il_container *ic;
    il_client *c;

    int stopped_count = 0;
	il_container_list *ic_list = cx->imageList;

	for (; ic_list; ic_list = ic_list->next) {
        ic = ic_list->ic;
        for (c = ic->clients; c; c = c->next) {
            if (c->cx == cx) {
                c->stopped = TRUE;
                stopped_count++;
            }
        }
    }

    if (stopped_count)
        FE_UpdateStopState(cx);
}

/* Has the user aborted the image load ? */
PRBool
il_image_stopped(il_container *ic)
{
    il_client *c;
    for (c = ic->clients; c; c = c->next) {
        if (! c->stopped)
            return FALSE;
    }

    /* All clients must be stopped for image container to become dormant. */
    return TRUE;
}

/* Checks to see if there are any looping images for the context */
Bool
IL_AreThereLoopingImagesForContext(MWContext *cx)
{
	il_container_list *ic_list = cx->imageList;

	for (; ic_list; ic_list = ic_list->next)
	  {
		/* If we're looping */
		if (ic_list->ic && IMAGE_CONTAINER_LOADING(ic_list->ic) &&
			ic_list->ic->is_looping)
		  {
			return TRUE;
		  }
	  }

	return FALSE;
}

#ifdef PROFILE
#pragma profile off
#endif
