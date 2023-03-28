/* -*- Mode: C; tab-width: 4 -*-
 *   ilclient.c --- Management of imagelib client data structures,
 *                  including image cache.
 *
 *   Copyright © 1995 Netscape Communications Corporation, all rights reserved.
 *   $Id: ilclient.c,v 1.55.4.1 1996/09/24 21:16:49 lwei Exp $
 */

#include "xp.h"
#include "if.h"


extern int MK_OUT_OF_MEMORY;
/* for XP_GetString() */
#include "xpgetstr.h"
extern int XP_MSG_IMAGE_NOT_FOUND;
extern int XP_MSG_XBIT_COLOR;	
extern int XP_MSG_1BIT_MONO;
extern int XP_MSG_XBIT_GREYSCALE;
extern int XP_MSG_XBIT_RGB;
extern int XP_MSG_DECODED_SIZE;	
extern int XP_MSG_WIDTH_HEIGHT;	
extern int XP_MSG_SCALED_FROM;	
extern int XP_MSG_IMAGE_DIM;	
extern int XP_MSG_COLOR;	
extern int XP_MSG_NB_COLORS;	
extern int XP_MSG_NONE;	
extern int XP_MSG_COLORMAP;	
extern int XP_MSG_BCKDRP_VISIBLE;	
extern int XP_MSG_SOLID_BKGND;	
extern int XP_MSG_JUST_NO;	
extern int XP_MSG_TRANSPARENCY;	
extern int XP_MSG_COMMENT;	
extern int XP_MSG_UNKNOWN;	
extern int XP_MSG_COMPRESS_REMOVE;	

static uint32 image_cache_size;

static int il_cache_trace = FALSE;

/* simple list, in use order */
struct il_cache_struct {
	il_container *head;
	il_container *tail;
	int32 bytes;
    int32 max_bytes;
	int items;
};

struct il_cache_struct il_cache;

void
IL_SetImageCacheSize(uint32 new_size)
{
	image_cache_size = new_size;
    IL_ReduceImageCacheSizeTo(new_size);
}

Bool
il_disconnect_container_from_context(MWContext *cx,
                                     il_container *ic_to_be_deleted)
{
	il_container_list *ic_list = cx->imageList;

    if (!ic_list)
        return FALSE;
    
	if (ic_list->ic == ic_to_be_deleted)
    {
		cx->imageList = ic_list->next;
        XP_FREE(ic_list);
    }
	else
	{
		for (; ic_list; ic_list = ic_list->next)
        {
			if (ic_list->next && (ic_list->next->ic == ic_to_be_deleted))
			{
                il_container_list *to_be_freed = ic_list->next;
				ic_list->next = ic_list->next->next;
                XP_FREE(to_be_freed);
                break;
			}
        }
	}

    if (ic_list)
    {
        cx->images--;
        cx->colorSpace->unique_images--;
        return TRUE;
    }

    return FALSE;
}

/* Returns TRUE if image container appears to match search parameters */
static int
il_image_match(il_container *ic, /* candidate for match */
               const char *image_url,
               IL_IRGB *background_color,
               Bool mask_required,
               int depth,       /* Color depth */
               int width,       /* *Displayed* image width and height */
               int height)
{

    /* Width and height both zero indicate usage of image's
       natural size.  If only one is zero, it indicates scaling
       maintaining the image's aspect ratio. */
    if (width != ic->layout_width) {
        if (! width) return FALSE;
        if (width != ic->image->width) return FALSE;
    }
    if (height != ic->layout_height) {
        if (! height) return FALSE;
        if (height != ic->image->height) return FALSE;
    }

    /* Now check background color (in case we remap transparent pixels) */
    if (ic->image->transparent) {
        if (mask_required) {
            if (!ic->image->has_mask)
                return FALSE;
        } else {
            if (ic->image->has_mask)
                return FALSE;
            if (!background_color)
                return FALSE;
            if (background_color->red != ic->image->transparent->red)
                return FALSE;
            if (background_color->green != ic->image->transparent->green)
                return FALSE;
            if (background_color->blue != ic->image->transparent->blue)
                return FALSE;
        }
    }

    /* We use the il_process's depth instead of image container's
     * depth, because the latter might not be initialized until
     * il_size() has been called.
     */
    if (ic->cs->depth != depth)
        return FALSE;

    if (strcmp(image_url, ic->url_address))
        return FALSE;

    /* XXX - temporary */
    if (ic->rendered_with_custom_palette)
        return FALSE;
                
    return TRUE;
}

static int
il_images_match(il_container *ic1, il_container *ic2)
{
    return il_image_match(ic1,
                          ic2->url_address, ic2->image->transparent,
                          ic2->image->has_mask != 0,
                          ic2->cs->depth,
                          ic2->layout_width, ic2->layout_height);
}

static il_container *
il_find_in_cache(uint32 hash,
                 const char *image_url,
                 IL_IRGB* background_color,
                 Bool mask_required,
                 int depth,
                 int width,
                 int height)
{
	il_container *ic=0;
	XP_ASSERT(hash);
	for (ic=il_cache.head; ic; ic=ic->next)
	{
		if (ic->hash != hash)
            continue;
        if (il_image_match(ic, image_url,background_color, mask_required,
                           depth, width, height))
			break;
	}
	if (ic)
	{
		ILTRACE(2,("il:  found ic=0x%08x in cache\n", ic));
        return ic;
	}

    if (il_cache_trace) {
        char buffer[1024];
        MWContext *alert_context =
            XP_FindContextOfType(NULL, MWContextBrowser);

        XP_SPRINTF(buffer,
        			XP_GetString(XP_MSG_IMAGE_NOT_FOUND),
        			image_url);


        if (alert_context)
            FE_Alert(alert_context, buffer);
    }
    
    return NULL;
}

static void
il_addtocache(il_container *ic);

il_container *
il_get_container(MWContext *cx,
                 NET_ReloadMethod cache_reload_policy,
                 const char *image_url,
                 const char *lowres_image_url,
                 IL_IRGB *background_color,
                 IL_DitherMode dither_mode,
                 Bool mask_required,
                 int depth,
                 int layout_width, /* Layout-defined image width and height */
                 int layout_height)
{
    uint32 urlhash, hash;
    il_container *ic;

    urlhash = hash = il_hash(image_url);

    /* and lowsrc */
    if (lowres_image_url)
        hash += il_hash(lowres_image_url);

    /*
     * Take into account whether color rendering preferences have
     * changed since the image was cached.
     */
    hash += 21117 * (int)dither_mode;
           
    /* Check the cache */
    ic = il_find_in_cache(hash, image_url, background_color, mask_required,
                          depth, layout_width, layout_height);

    if (ic) {
        /* We already started to discard this image container. Make a new one.*/
        if ((ic->state == IC_ABORT_PENDING))
            ic = NULL;

		/* Check if we have to reload or if there's an expiration date   */
		/* and we've expired or if this is a JavaScript generated image. */
		/* We don't want to cache JavaScript generated images because:   */
		/* 1) The assumption is that they are generated and might change */
		/*    if the page is reloaded.                                   */
		/* 2) Their namespace crosses document boundaries, so caching    */
		/*    could result in incorrect behavior.                        */
        else if ((FORCE_RELOAD(cache_reload_policy) && !ic->forced) ||
				 (cache_reload_policy != NET_CACHE_ONLY_RELOAD &&
				  ic->expires && (time(NULL) > ic->expires)) ||
				 (NET_URL_Type(ic->url_address) == MOCHA_TYPE_URL))
        {
            /* Get rid of the old copy of the image that we're replacing. */
            if (!ic->is_in_use) 
            {
                il_removefromcache(ic);
                il_delete_container(ic, TRUE);
            }
            ic = NULL;
        }

        /* Printer contexts and window contexts may have different
           native image formats, so don't reuse an image cache entry
           created for an onscreen context in a printer context or
           vice-versa. */
        else if (((cx->type == MWContextPrint) ||
            (cx->type ==MWContextPostScript)) &&
            (ic->context_type != cx->type))
            ic = NULL;
    }

    /* Reorder the cache list so it remains in LRU order*/
    if (ic)
        il_removefromcache(ic);

    /* There's no existing matching container.  Make a new one. */
    if (!ic) {
		ic = XP_NEW_ZAP(il_container);
        if (!ic)
            return NULL;
		if (!(ic->image = XP_NEW_ZAP(IL_Image))) {
            XP_FREE(ic);
            return NULL;
        }

        ILTRACE(2, ("il: create ic=0x%08x\n", ic));

        ic->hash = hash;
        ic->urlhash = urlhash;
        ic->url_address = XP_STRDUP(image_url);
        ic->ip = cx->imageProcess;
        ic->layout_width  = layout_width;
        ic->layout_height = layout_height;
        ic->image->has_mask = mask_required;
        ic->context_type = cx->type;
    }

	il_addtocache(ic);
    ic->is_in_use = TRUE;
    
    return ic;
}

void
il_scour_container(il_container *ic)
{
	/* scour quantize, ds, scalerow etc */
	if (ic->cs->mode == ilCI)
		il_free_quantize(ic);
    FREE_IF_NOT_NULL(ic->hiurl);
    FREE_IF_NOT_NULL(ic->scalerow);
    FREE_IF_NOT_NULL(ic->indirect_map);

	/* reset a bunch of stuff */
	ic->url    = NULL;
	ic->stream = NULL;
	ic->cx     = NULL;

	ic->forced                  = FALSE;
	ic->is_visible_on_screen    = FALSE;
	ic->is_backdrop             = FALSE;
	ic->is_alone                = FALSE;
}

/*
 * destroy an il_container, freeing it and the image
 */
void
il_delete_container(il_container *ic, int explicitly)
{
	XP_ASSERT(ic);
	if (ic)
	{
		ILTRACE(2,("il: delete ic=0x%08x", ic));

        /*
         * We can't throw away this container until the netlib
         * stops sending us stuff.  We defer the actual deletion
         * of the container until then.
         */
        if (IMAGE_CONTAINER_LOADING(ic)) {
            ic->state = IC_ABORT_PENDING;
            return;
        }
        
        XP_ASSERT(ic->clients == NULL);
		il_scour_container(ic);

		/* delete the image */
		XP_ASSERT(ic->image);
        if (!ic->image)
            return;

        FREE_IF_NOT_NULL(ic->image->map);
		FREE_IF_NOT_NULL(ic->image->transparent);
        FREE_IF_NOT_NULL(ic->comment);

		/* bits & mask belongs to the FE */
		if (explicitly)
			FE_ImageDelete(ic->image);

        FREE_IF_NOT_NULL(ic->url_address);
		
		XP_FREE(ic->image);
		ic->image = 0;
		XP_FREE(ic);
	}
}

static char *
il_visual_info(il_container *ic)
{
	char *msg = (char *)XP_CALLOC(1, 50);
    il_colorspace *cs = ic->cs;

    if (!msg)
        return NULL;
    
    switch (cs->mode)
        {
        case ilCI:
            XP_SPRINTF(msg, XP_GetString(XP_MSG_XBIT_COLOR), cs->depth); /* #### i18n */
            break;

        case ilGrey:
            if (cs->depth == 1)
               XP_SPRINTF(msg, XP_GetString(XP_MSG_1BIT_MONO)); /* #### i18n */
            else
               XP_SPRINTF(msg, XP_GetString(XP_MSG_XBIT_GREYSCALE), cs->depth); /* #### i18n */
             break;

        case ilRGB:
            XP_SPRINTF(msg, XP_GetString(XP_MSG_XBIT_RGB), cs->depth); /* #### i18n */
            break;

        default:
            XP_ASSERT(0);
            *msg=0;
            break;
		}

    return msg;
}

/* Define some macros to help us output HTML */
#define CELL_TOP 							\
	StrAllocCat(output, 					\
				"<TR><TD VALIGN=BASELINE ALIGN=RIGHT><B>");	
#define CELL_TITLE(title)					\
	StrAllocCat(output, title);
#define CELL_MIDDLE							\
	StrAllocCat(output, 					\
				"</B></TD>"					\
				"<TD>");
#define CELL_BODY(body)						\
	StrAllocCat(output, body);
#define CELL_END							\
	StrAllocCat(output, 					\
				"</TD></TR>");
#define ADD_CELL(c_title, c_body)			\
	CELL_TOP;								\
	CELL_TITLE(c_title);					\
	CELL_MIDDLE;							\
	CELL_BODY(c_body);						\
	CELL_END;

static char *
il_HTML_image_info(il_container *ic, int long_form, int show_comment)
{
    char tmpbuf[512];           /* Temporary consing area */
    char *output = NULL;
    IL_Image *image = ic->image;

    XP_SPRINTF(tmpbuf, "%lu", (long)image->widthBytes * image->height +
               sizeof(il_container));
    ADD_CELL(XP_GetString(XP_MSG_DECODED_SIZE), tmpbuf); /* #### i18n */

    /* #### i18n */
    XP_SPRINTF(tmpbuf, XP_GetString(XP_MSG_WIDTH_HEIGHT), image->width, image->height);
    if ((image->width  != ic->unscaled_width) ||
        (image->height != ic->unscaled_height))
    {
        /* #### i18n */
        XP_SPRINTF(tmpbuf + strlen(tmpbuf),  XP_GetString(XP_MSG_SCALED_FROM),
                   ic->unscaled_width, ic->unscaled_height);
    }
    /* #### i18n */
    ADD_CELL(XP_GetString(XP_MSG_IMAGE_DIM), tmpbuf);

    if (long_form) {

        char *visual_info = il_visual_info(ic);
        if (visual_info) {
            ADD_CELL(XP_GetString(XP_MSG_COLOR), visual_info);    /* #### i18n */
            XP_FREE(visual_info);
        }
        
        if (image->map)
            XP_SPRINTF(tmpbuf, XP_GetString(XP_MSG_NB_COLORS), image->unique_colors);    /* #### i18n */
        else
            XP_SPRINTF(tmpbuf, XP_GetString(XP_MSG_NONE));    /* #### i18n */
        ADD_CELL(XP_GetString(XP_MSG_COLORMAP), tmpbuf);
                
        if (image->transparent) {
            if (image->has_mask)
                XP_SPRINTF(tmpbuf,
                           /* #### i18n */
                           XP_GetString(XP_MSG_BCKDRP_VISIBLE));
            else
                XP_SPRINTF(tmpbuf,
                           /* #### i18n */
                           XP_GetString(XP_MSG_SOLID_BKGND),
                           image->transparent->red,
                           image->transparent->green,
                           image->transparent->blue);
        } else {
            XP_SPRINTF(tmpbuf, XP_GetString(XP_MSG_JUST_NO));    /* #### i18n */
        }
        ADD_CELL(XP_GetString(XP_MSG_TRANSPARENCY), tmpbuf);    /* #### i18n */
    }

    if (show_comment && ic->comment) {
        XP_SPRINTF(tmpbuf, "%.500s", ic->comment);
        ADD_CELL(XP_GetString(XP_MSG_COMMENT), tmpbuf);    /* #### i18n */
    }

    return output;
}

char *
IL_HTMLImageInfo(char *url_address)
{
    il_container *ic;
    char *output = NULL;
    char *il_msg;
    
	for (ic=il_cache.head; ic; ic=ic->next)
	{
		if (!strcmp(ic->url_address, url_address))
			break;
	}

    if ((ic == NULL) || (ic->state != IC_COMPLETE))
        return NULL;

    il_msg = il_HTML_image_info(ic, TRUE, TRUE);
    if (il_msg == NULL)
        return NULL;

    StrAllocCat(output,
                "<TABLE CELLSPACING=0 CELLPADDING=1 "
                "BORDER=0 ALIGN=LEFT WIDTH=66%>");
    StrAllocCat(output, il_msg);
    StrAllocCat(output, "</TABLE> <A HREF=\"");
    StrAllocCat(output, url_address);
    StrAllocCat(output, "\"> <IMG WIDTH=90% ALIGN=CENTER SRC=\"");
    StrAllocCat(output, url_address);
    StrAllocCat(output, "\"></A>\n");

    XP_FREE(il_msg);

    return output;
}

/*
 * Create an HTML stream and generate HTML describing
 * the image cache.  Use "about:memory-cache" URL to acess.
 */
int 
IL_DisplayMemCacheInfoAsHTML(FO_Present_Types format_out,
                             URL_Struct *urls,
                             MWContext *cx)
{
	char buffer[2048];
    char *output = NULL;
    char *img_info;
	char *address;
	char *escaped;
   	NET_StreamClass * stream;
	Bool long_form = FALSE;
	int status;

	if(strcasestr(urls->address, "?long"))
		long_form = TRUE;
	else if(strcasestr(urls->address, "?traceon"))
		il_cache_trace = TRUE;
	else if(strcasestr(urls->address, "?traceoff"))
		il_cache_trace = FALSE;

	StrAllocCopy(urls->content_type, TEXT_HTML);

	format_out = CLEAR_CACHE_BIT(format_out);
	stream = NET_StreamBuilder(format_out, urls, cx);

	if (!stream)
		return 0;

	/*
     * Define a macro to push a string up the stream
	 * and handle errors
	 */
#define PUT_PART(part)														\
{                                                                           \
    status = (*stream->put_block)(stream->data_object,						\
                                  part ? part : "Unknown",					\
                                  part ? XP_STRLEN(part) : 7);				\
    if (status < 0)															\
        goto END;                                                           \
}
                                                                            
                                  

    /* Send something to the screen immediately.  This will force all
     * the images on the page to enter the cache now. Otherwise,
     * layout will be causing cache state to be modified even as we're
     * trying to display it.
     */
    XP_STRCPY(buffer,
              "<TITLE>Information about the Netscape image cache</TITLE>\n");
    PUT_PART(buffer);
                                  
 	XP_SPRINTF(buffer, 
			   "<h2>Image Cache statistics</h2>\n"
			   "<TABLE CELLSPACING=0 CELLPADDING=1>\n"
			   "<TR>\n"
			   "<TD ALIGN=RIGHT><b>Maximum size:</b></TD>\n"
			   "<TD>%ld</TD>\n"
			   "</TR>\n"
			   "<TR>\n"
			   "<TD ALIGN=RIGHT><b>Current size:</b></TD>\n"
			   "<TD>%ld</TD>\n"
			   "</TR>\n"
			   "<TR>\n"
			   "<TD ALIGN=RIGHT><b>Number of images in cache:</b></TD>\n"
			   "<TD>%d</TD>\n"
			   "</TR>\n"
			   "<TR>\n"
			   "<TD ALIGN=RIGHT><b>Average cached image size:</b></TD>\n"
			   "<TD>%ld</TD>\n"
			   "</TR>\n"
			   "</TABLE>\n"
			   "<HR>",
			   (long)il_cache.max_bytes,
			   (long)il_cache.bytes,
			   il_cache.items,
			   (il_cache.bytes ?
                (long)il_cache.bytes / il_cache.items : 0L));

	PUT_PART(buffer);

    {
        il_container *ic, *l = NULL;
        IL_Image *image;
        
        for (ic=il_cache.head; ic; ic=ic->next) {
            image = ic->image;
            
            /* Sanity check */
            if (l)
                XP_ASSERT(ic->prev == l);
            l = ic;

            /* Don't display uninteresting images */
            if ((ic->state != IC_COMPLETE) &&
                (ic->state != IC_SIZED)    &&
                (ic->state != IC_HIRES)    &&
                (ic->state != IC_MULTI))
                continue;
            
            StrAllocCat(output, "<TABLE>\n");

            /* Emit DocInfo link to URL */
            address = ic->url_address;
            XP_STRCPY(buffer, "<A TARGET=Internal_URL_Info HREF=about:");
            XP_STRCAT(buffer, address);
            XP_STRCAT(buffer, ">");
            escaped = NET_EscapeHTML(address);
            XP_STRCAT(buffer, escaped);
            XP_FREE(escaped);
            XP_STRCAT(buffer, "</A>");
            ADD_CELL("URL:", buffer);
            
            /* Rest of image info (size, colors, depth, etc.) */
            img_info = il_HTML_image_info(ic, long_form, FALSE);
            if (img_info) {
                StrAllocCat(output, img_info);
                XP_FREE(img_info);
            }
            
            StrAllocCat(output, "</TABLE><P>\n");
        }

        if (output)
            PUT_PART(output);
    }
        
  END:
     
    if (output)
        XP_FREE(output);
     
	if(status < 0)
		(*stream->abort)(stream->data_object, status);
	else
		(*stream->complete)(stream->data_object);

	return 1;
}

il_container *
il_removefromcache(il_container *ic)
{
    int32 image_bytes;

	XP_ASSERT(ic); 
	if (ic)
	{
		ILTRACE(2,("il: remove ic=0x%08x from cache\n", ic));
        XP_ASSERT(ic->next || ic->prev || (il_cache.head == il_cache.tail));

        /* Remove entry from doubly-linked list. */
		if (il_cache.head == ic)
            il_cache.head = ic->next;

        if (il_cache.tail == ic)
            il_cache.tail = ic->prev;

        if (ic->next)
            ic->next->prev = ic->prev;
        if (ic->prev)
            ic->prev->next = ic->next;

        ic->next = ic->prev = NULL;

        image_bytes = ((int32) ic->image->widthBytes) * ic->image->height;

        XP_ASSERT (il_cache.bytes >= (int32)image_bytes);
        if (il_cache.bytes <  (int32)image_bytes)
            il_cache.bytes = 0;
        else        
            il_cache.bytes -= image_bytes;
		il_cache.items--;
        XP_ASSERT(il_cache.items >= 0);
	}
	return ic;
}

void
il_adjust_cache_fullness(int32 bytes)
{
    il_cache.bytes += bytes;
    XP_ASSERT(il_cache.bytes >= 0);

    /* Safety net - This should never happen. */
    if (il_cache.bytes < 0)
        il_cache.bytes = 0;

	/* We get all of the memory cache.  (Only images are cached in memory.) */
    IL_ReduceImageCacheSizeTo(image_cache_size);

    /* Collect some debugging info. */
    if (il_cache.bytes > il_cache.max_bytes)
        il_cache.max_bytes = il_cache.bytes;
}

static void
il_addtocache(il_container *ic)
{
	ILTRACE(2,("il:    add ic=0x%08x to cache\n", ic));
    XP_ASSERT(!ic->prev && !ic->next);

    /* Thread onto doubly-linked list, kept in LRU order */
    ic->prev = NULL;
    ic->next = il_cache.head;
	if (il_cache.head)
		il_cache.head->prev = ic;
    else {
        XP_ASSERT(il_cache.tail == NULL);
        il_cache.tail = ic;
    }
    il_cache.head = ic;

    /* Image storage is added in when image is sized */
    if (ic->sized)
        il_cache.bytes += (uint32)ic->image->widthBytes * ic->image->height;
	il_cache.items++;
}

void
IL_ReduceImageCacheSizeTo(uint32 new_size)
{
    int32 last_size = 0;
    
	while ((il_cache.bytes > (int32)new_size) && (il_cache.bytes != last_size)) {
        last_size = il_cache.bytes;
        IL_ShrinkCache();
    }
}

uint32
IL_ShrinkCache(void)
{
	il_container *ic;
	ILTRACE(3,("shrink"));

    for (ic = il_cache.tail; ic; ic = ic->prev)
	{
		if (ic->is_user_backdrop || ic->is_in_use)
            continue;
        
        if (il_cache_trace) {
            char buffer[1024];
            MWContext *alert_context =
                XP_FindContextOfType(NULL, MWContextBrowser);
            
            sprintf(buffer,
                    XP_GetString(XP_MSG_COMPRESS_REMOVE), ic->url_address);

            if (alert_context)
                FE_Alert(alert_context, buffer);
        }
        il_removefromcache(ic);
        il_delete_container(ic, TRUE);
        break;
    }

    return il_cache.bytes;
}

uint32 IL_GetCacheSize()
{	
	return il_cache.bytes;
}

static int
il_replace_client(MWContext *cx, il_container *ic,
	void* new_client, void* old_client)
{
    int num_replacements = 0;

	il_client *c;

	if (!ic)
		return 0;

	c = ic->clients;
	while (c)
	{
		if (c->client == old_client)
		{
			c->client = new_client;
            num_replacements++;
		}
		c = c->next;
	}

	return num_replacements;
}

void
IL_UnCache(IL_Image *img)
{
	il_container *ic;
	if (img)
	{
		for (ic=il_cache.head; ic; ic=ic->next)
		{
			if ((ic->image == img) && !ic->is_in_use)
			{
				il_removefromcache(ic);
				il_delete_container(ic, TRUE);
				return;
			}
		}
	}
}

int
il_add_client(MWContext *cx,
              il_container *ic,
              void* client,
              int is_view_image)
{
	il_client *c;
    il_container_list *ic_list;
    
	if (!(c = XP_NEW_ZAP(il_client)))
	{
		ILTRACE(0,("il: MEM il_add_client"));
		return MK_OUT_OF_MEMORY;
	}
	c->client = client;
    c->cx = cx;
    c->is_view_image = is_view_image;

	/* added to the end */
	if (!ic->lclient)
		ic->clients = c;
	else
		ic->lclient->next = c;
	ic->lclient = c;

    ILTRACE(3, ("il: add_client ic=0x%08x, client =0x%08x\n",
                ic, client));

    /* Add to the list of images for this context,
     * but only one entry per unique image.
     */
    il_disconnect_container_from_context(cx, ic);
    ic_list = XP_NEW_ZAP(il_container_list);
	if (!ic_list) 
    {
		ILTRACE(0,("il: MEM il_add_client"));
		return MK_OUT_OF_MEMORY;
	}
    ic_list->ic = ic;
    ic_list->next = cx->imageList;
    cx->imageList = ic_list;
    cx->images++;
    cx->colorSpace->unique_images++;

    return 0;
}

/* Delete an LO_ImageStruct from the list of clients for an image container.
 * Return TRUE if successful, FALSE otherwise.
 */
static Bool
il_delete_client(il_container *ic, void *client)
{
    MWContext *cx;
    il_client *c;
    il_client *lastc = NULL;

    c = ic->clients;
    while (c) {
        if (c->client == client)
            break;
        lastc = c;
        c = c->next;
    }

    if (!c)
        return FALSE;

    if (c == ic->clients)
        ic->clients = c->next;
    else
        lastc->next = c->next;

    if (c == ic->lclient)
        ic->lclient = lastc;
    
    cx = c->cx;
    XP_FREE(c);

    /* Decrement the number of unique images for this client's
     * context, but be careful to do this only when there are no other
     * clients of this image with the same context.
     */
    c = ic->clients;
    while (c) {
        if (c->cx == cx)
            break;
        c = c->next;
    }
    if (!c) {
        il_disconnect_container_from_context(cx, ic);

        /* Images need "home" contexts on which to perform some operations, such as network activity */ 
        if (ic->clients) {
           if (ic->cx == cx)
                ic->cx = ic->clients->cx;
           ic->cs = ic->clients->cx->colorSpace;
#ifdef GOLD
           /* This should be done for all versions of the navigator.
            * However, it's only a critical bug for Gold, and the
            * fix was made very late in the Akbar cycle. So, for
            * reasons of paranoia, we only made the fix for the
            * Gold source code. In Dogbert, we should remove the
            * #ifdef, and always do this assignment.
            */
           ic->ip = ic->clients->cx->imageProcess;
#endif
        } 
    }

    ILTRACE(3, ("il: delete_client ic=0x%08x, client =0x%08x\n", ic, client));
 
    return TRUE;
}

static void
il_delete_all_clients(il_container *ic)
{
    il_client *c;
    il_client *nextc = NULL;

    for (c = ic->clients; c; c = nextc)
    {
        nextc = c->next;
        il_delete_client(ic, c->client);
    }
}

/* Used when exiting the client, this code frees all imagelib data structures.
 * This is done for two reasons:
 *   1) It makes leakage analysis of the heap easier, and
 *   2) It causes resources to be freed on Windows that would otherwise persist
 *      beyond the browser's lifetime.
 *
 * XXX - Actually, right now it just frees the cached images.  There are still
 *       a couple of other places that leaks need to be fixed.
 */
void
IL_Shutdown()
{
    il_container *ic, *ic_next;
    for (ic = il_cache.head; ic; ic = ic_next)
	{
        ic_next = ic->next;
        il_delete_all_clients(ic);
        il_removefromcache(ic);
        il_delete_container(ic, TRUE);
    }

    XP_ASSERT(il_cache.bytes == 0);
}

/* Disconnect a layout image client from the imagelib's image container
 * structure.  If the last client is removed, the container can be reclaimed,
 * either by storing it in the cache or throwing it away.
 * Always returns FALSE.
 */
Bool
IL_FreeImage(MWContext *cx,
             IL_Image *image,
             LO_ImageStruct* lo_image)
{
    Bool client_deleted;
    il_container *ic, *ic2, *ic2_next;
	il_container_list *ic_list = cx->imageList;

    while (ic_list) {
        ic = ic_list->ic;
        if (ic->image == image)
            break;
        ic_list = ic_list->next;
    }
    XP_ASSERT(ic_list);

    ILTRACE(1, ("il: IL_FreeImage: ic = 0x%08x\n", ic));

    if (!ic_list)
        return FALSE;

    client_deleted = il_delete_client(ic, lo_image);
    XP_ASSERT(client_deleted);

    /* The image container can't be deleted until all clients are done. */
    if (ic->clients)
        return FALSE;

    /* Don't allow any new streams to be created in
       il_image_complete() as a result of a looping animation.  (It's
       possible that this image is being freed before the stream
       completes/aborts.  This can happen in autoscroll mode and the
       editor apparently allows this to occur also.) */
    ic->loop_count = 0;

    il_image_abort(ic);

	XP_ASSERT(cx->images >= 0);
	if (!cx->images)
	{
		XP_ASSERT(!cx->imageList);
		cx->imageList = 0;
		if (cx->imageProcess)
		{
			cx->imageProcess->active = 0;
		}
	}

	if ((ic->state != IC_COMPLETE) || ic->multi || ic->rendered_with_custom_palette ||
		(NET_URL_Type(ic->url_address) == MOCHA_TYPE_URL))
    {
        il_removefromcache(ic);
        il_delete_container(ic, TRUE);
        return FALSE;
    }

    /* Don't allow duplicate entries in cache */
    for (ic2 = il_cache.head; ic2; ic2 = ic2_next)
    {
        ic2_next = ic2->next;
        if ((!ic2->is_in_use) && il_images_match(ic, ic2)) {
            il_removefromcache(ic2);
            il_delete_container(ic2, TRUE);
        }
    }

    ic->is_in_use = FALSE;

	return FALSE;
}

/*
 * Replace all occurances of this old image element with the
 * new element passed in.  The old element is going to be freed by
 * layout, but it wants to keep the image around, so it is hanging it
 * off of a new element.
 */
Bool
IL_ReplaceImage(MWContext *cx,
                LO_ImageStruct* new_lo_image,
                LO_ImageStruct* old_lo_image)
{
	int ret;
	il_container_list *ic_list;

	ret = 0;
	ic_list = cx->imageList;
	while (ic_list)
	{
		ret += il_replace_client(cx, ic_list->ic,
                                 (void*)new_lo_image,
                                 (void*)old_lo_image);
		ic_list = ic_list->next;
	}

    XP_ASSERT(ret == 1);
    
	if (!ret)
		return TRUE;
	else
		return FALSE;
}

/* This routine is a "safety net", in case layout didn't free up all the
 * images on a page.  It assumes layout made a mistake and frees them anyway.
 */
void
IL_StartPage(MWContext *cx)
{
    il_client *c, *nextc;
    il_container *ic;
	il_container_list *ic_list, *ic_list_next;
    
    if (cx->images > 0) {
        XP_ASSERT(cx->imageList);
        XP_TRACE(("Hey, layout failed to free all the images on this page!\n"
                  "There are still %d images unfreed.\n",
                  cx->images));

		for (ic_list = cx->imageList; ic_list; ic_list = ic_list_next)
        {
            ic_list_next = ic_list->next;
            ic = ic_list->ic;
            for (c = ic->clients; c; c = nextc)
            {
                nextc = c->next;
                if (c->cx == cx)
                    IL_FreeImage(c->cx, ic->image, (LO_ImageStruct*)c->client);
            }
        }
        XP_ASSERT(cx->images == 0);
        XP_ASSERT(cx->imageList == NULL);
    }

    if (cx->colorSpace) {
        cx->colorSpace->install_colormap_allowed = FALSE;
        cx->colorSpace->install_colormap_forbidden = FALSE;
    }

    ILTRACE(1, ("il: IL_StartPage\n"));
}
