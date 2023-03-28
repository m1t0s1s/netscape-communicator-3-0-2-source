/* -*- Mode: C; tab-width: 4 -*- */
#include "xp.h"
#include "if.h"
#include "merrors.h"
/* for XP_GetString() */
#include "xpgetstr.h"
extern int XP_MSG_IMAGE_PIXELS;

static unsigned int
il_view_write_ready(void *data_object)
{
	il_container *ic = (il_container *)data_object;

    /* For some reason, the imagelib can't deliver the image.
       Trigger il_view_write(), which will abort the stream. */
    return (ic != 0);
}

/* Abort the stream if we get this far */
static int
il_view_write(void *dobj, const unsigned char *str, int32 len)
{
    /* If this assert fires, chances are that the provided URL was malformed.*/
    XP_ASSERT(dobj == (void*)1);
    
     /* Should be MK_DATA_LOADED, but netlib ignores that. */
	return MK_INTERRUPTED;
}

static void
il_view_complete(void *data_object)
{
}

static void 
il_view_abort(void *data_object, int status)
{
}


/* there can be only one, highlander */
static NET_StreamClass *unconnected_stream = 0;
static URL_Struct *unconnected_urls = 0;

void
il_reconnect(il_container *ic)
{
	if (unconnected_stream)
	{
		unconnected_stream->complete       = il_stream_complete;
		unconnected_stream->abort          = il_abort;
		unconnected_stream->is_write_ready = il_write_ready;
		unconnected_stream->data_object    = (void *)ic;
		unconnected_stream->put_block      = (MKStreamWriteFunc)il_first_write;
		ic->type = IL_UNKNOWN;
		ic->stream = unconnected_stream;
		ic->state = IC_STREAM;
		unconnected_urls->fe_data = ic;	/* ugh */
		ic->content_length = unconnected_urls->content_length;
		unconnected_stream = 0;
		unconnected_urls = 0;
	}
}

/* We aren't going to reconnect after all.  Cause the stream to abort */
void
il_abort_reconnect()
{
    if (unconnected_stream) {
        unconnected_stream->data_object = (void *)1;
		unconnected_stream = 0;
		unconnected_urls = 0;
	}
}

static char fakehtml[] = "<IMG SRC=\"internal-external-reconnect:%s\">";

NET_StreamClass *
IL_ViewStream(FO_Present_Types format_out, void *newshack, URL_Struct *urls, MWContext *cx)
{
    NET_StreamClass *stream = nil, *viewstream;
	il_container *ic = nil;
	char *org_content_type;
    
	/* multi-part reconnect hack */
	ic = (il_container*)urls->fe_data;
	if(ic && ic->multi)
	{
		return IL_NewStream(format_out, IL_UNKNOWN, urls, cx);
	}

	/* Create stream object */
    if (!(stream = XP_NEW_ZAP(NET_StreamClass))) {
		XP_TRACE(("il: IL_ViewStream memory lossage"));
		return 0;
	}
	
    stream->name           = "image view";
    stream->complete       = il_view_complete;
    stream->abort          = il_view_abort;
    stream->is_write_ready = il_view_write_ready;
    stream->data_object    = NULL;
    stream->window_id      = cx;
    stream->put_block      = (MKStreamWriteFunc)il_view_write;

	ILTRACE(0,("il: new view stream, %s", urls->address));

	XP_ASSERT(!unconnected_stream);
	unconnected_stream = stream;
	unconnected_urls = urls;

	if(!newshack)
	{
		org_content_type = urls->content_type; 
		urls->content_type = 0;
		StrAllocCopy(urls->content_type, TEXT_HTML);

		urls->is_binary = 1;	/* secret flag for mail-to save as */

		if((viewstream=NET_StreamBuilder(FO_PRESENT, urls, cx)) != 0)
		{
            char *buffer = (char*)
                XP_ALLOC(XP_STRLEN(fakehtml) + XP_STRLEN(urls->address) + 1);

            if (buffer)
            {
                XP_SPRINTF(buffer, fakehtml, urls->address);
                (*viewstream->put_block)(viewstream->data_object,
                                         buffer, XP_STRLEN(buffer));
                (*viewstream->complete)(viewstream->data_object);
                XP_FREE(buffer);
            }
		}

		/* XXX hack alert - this has to be set back for abort to work correctly */
		XP_FREE(urls->content_type);
		urls->content_type = org_content_type;
	}

    return stream;
}


/* Fabricate a title for window containing an image viewer. */
void
il_set_view_image_doc_title(il_container *ic)
{
    char buf[36], buf2[12];
    il_client *c;
    
    switch (ic->type)
    {
    case IL_GIF : XP_STRCPY(buf2, "GIF"); break;
    case IL_XBM : XP_STRCPY(buf2, "XBM"); break;
    case IL_JPEG : XP_STRCPY(buf2, "JPEG"); break;
    default : XP_STRCPY(buf2, "");
    }
    XP_SPRINTF(buf, XP_GetString(XP_MSG_IMAGE_PIXELS), buf2,
               ic->image->width, ic->image->height);

	for (c=ic->clients; c; c=c->next)
    {
        if (c->is_view_image)
            FE_SetDocTitle(c->cx, buf);
    }
}

