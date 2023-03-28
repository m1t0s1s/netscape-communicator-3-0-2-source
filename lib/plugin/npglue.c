/* -*- Mode: C; tab-width: 4; -*- */
/*
 *  npglue.c $Revision: 1.207.6.1.10.1 $
 *
 *  Function naming conventions:
 *      NPL_   Functions exported to other libs or FEs (prototyped in np.h)
 *		NPN_   Function prototypes exported to plug-ins via our function table (prototyped in npapi.h)
 *      npn_   Function definitions whose addresses are placed in the function table 
 *      np_    Miscellaneous functions internal to libplugin (this file)
 *
 */

#include "client.h"
#include "xpassert.h" 
#include "ntypes.h"
#include "net.h"
#include "fe_proto.h"
#include "cvactive.h"
#include "gui.h"			/* For XP_AppCodeName */
#include "np.h"
#include "nppg.h"
#include "merrors.h"
#include "xpgetstr.h"
#include "java.h"
#include "nppriv.h"

#ifdef MOCHA
#include "libmocha.h"
#include "layout.h"         /* XXX From ../layout */
#endif

extern int XP_PLUGIN_LOADING_PLUGIN;
extern int MK_BAD_CONNECT;
extern int XP_PLUGIN_NOT_FOUND;
extern int XP_PLUGIN_CANT_LOAD_PLUGIN;

int np_debug = 0;

#define LOCK    1
#define UNLOCK  0

#define NPTRACE(n, msg)	TRACEMSG(msg)

#define RANGE_EQUALS  "bytes="


#ifdef XP_WIN
/* Can't include FEEMBED.H because it's full of C++ */
extern NET_StreamClass *EmbedStream(int iFormatOut, void *pDataObj, URL_Struct *pUrlData, MWContext *pContext);
extern void EmbedUrlExit(URL_Struct *pUrl, int iStatus, MWContext *pContext);
#endif

extern void NET_RegisterAllEncodingConverters(char* format_in, FO_Present_Types format_out);

void NPL_EmbedURLExit(URL_Struct *urls, int status, MWContext *cx);
void NPL_URLExit(URL_Struct *urls, int status, MWContext *cx);


/* list of all plugins */
static np_handle *np_plist = 0;


/* this is a hack for now */
#define NP_MAXBUF (0xE000)

NPNetscapeFuncs npp_funcs;


/* Find a mimetype in the handle */
static np_mimetype *
np_getmimetype(np_handle *handle, char *mimeStr, XP_Bool wildCard)
{
	np_mimetype *mimetype;

	for (mimetype = handle->mimetypes; mimetype; mimetype = mimetype->next)
	{
		if (mimetype->enabled)
		{
			if ((wildCard && !strcmp(mimetype->type, "*")) ||
				!strcasecomp(mimetype->type, mimeStr))
				return (mimetype);
		}
	}
	return (NULL);
}
    

/*
 * Standard conversion from NetLib status
 * code to plug-in reason code.
 */
static NPReason
np_statusToReason(int status)
{
	if (status == 0)
		return NPRES_DONE;
	else if (status == MK_INTERRUPTED)
		return NPRES_USER_BREAK;
	else
		return NPRES_NETWORK_ERR;
}


/*
 * Add a node to the list of URLs for this
 * plug-in instance.  The caller must fill
 * out the fields of the node, which is returned.
 */
static np_urlsnode*
np_addURLtoList(np_instance* instance)
{
	np_urlsnode* node;
	
	if (!instance)
		return NULL;
		
	if (!instance->url_list)
		instance->url_list = XP_ListNew();
	if (!instance->url_list)
		return NULL;
		
	node = XP_NEW_ZAP(np_urlsnode);
	if (!node)
		return NULL;
		
	XP_ListAddObject(instance->url_list, node);
	
	return node;
}


/*
 * Deal with one URL from the list of URLs for the instance
 * before the URL is removed from the list.  If the URL was
 * locked in the cache, we must unlock it and delete the URL.
 * Otherwise, notify the plug-in that the URL is done (if 
 * necessary) and break the reference from the URL to the 
 * instance. 
 */
static void
np_processURLNode(np_urlsnode* node, np_instance* instance, int status)
{
	if (node->cached)
	{
		/* Unlock the cache file */
		XP_ASSERT(!node->notify);
		if (node->urls)
		{
        	NET_ChangeCacheFileLock(node->urls, UNLOCK);
        	NET_FreeURLStruct(node->urls);
        	node->urls = NULL;
        }
	    
	    return;
	}
	
	if (node->notify)
	{
		/* Notify the plug-in */
		XP_ASSERT(!node->cached);
        if (instance && ISFUNCPTR(instance->handle->f->urlnotify))
        {
            CallNPP_URLNotifyProc(instance->handle->f->urlnotify,
                                  instance->npp,
                                  node->urls->address,
                                  np_statusToReason(status),
                                  node->notifyData);
		}
	}
	
	/* Break the reference from the URL to the instance */
	if (node->urls)
		node->urls->fe_data = NULL;
}


/*
 * Remove an individual URL from the list of URLs for this instance.
 */
static void
np_removeURLfromList(np_instance* instance, URL_Struct* urls, int status)
{
    XP_List* url_list;
    np_urlsnode* node;

    if (!instance || !urls || !instance->url_list)
        return;

    /* Look for the URL in the list */
    url_list = instance->url_list;
    while ((node = XP_ListNextObject(url_list)) != NULL)
    {
        if (node->urls == urls)
        {
        	XP_ASSERT(!node->cached);
        	np_processURLNode(node, instance, status);

	        XP_ListRemoveObject(instance->url_list, node);
			XP_FREE(node);
			
	        /* If the list is now empty, delete it */
	        if (XP_ListCount(instance->url_list) == 0)
	        {
	            XP_ListDestroy(instance->url_list);
	            instance->url_list = NULL;
	        }

	        return;
        }
    }
}


/* 
 * Remove all URLs from the list of URLs for this instance.
 */
static void
np_removeAllURLsFromList(np_instance* instance)
{
	XP_List* url_list;
    np_urlsnode* node;

    if (!instance || !instance->url_list)
        return;
        
	url_list = instance->url_list;
	while ((node = XP_ListNextObject(url_list)) != NULL)
	{
		/* No notification of URLs now: the instance is going away */
		node->notify = FALSE;
		np_processURLNode(node, instance, 0);
	}

	/* Remove all elements from the list */
	url_list = instance->url_list;
    while ((node = XP_ListRemoveTopObject(url_list)) != NULL)
    	XP_FREE(node); 
        
	/* The list should now be empty, so delete it */
	XP_ASSERT(XP_ListCount(instance->url_list) == 0);
	XP_ListDestroy(instance->url_list);
	instance->url_list = NULL;
}



/* maps from a urls to the corresponding np_stream.  
   you might well ask why not just store the urls in the stream and
   put the stream in the netlib obj.  the reason is that there can be
   more than one active netlib stream for each plugin stream and
   netlib specific data for seekable streams (current position and
   so-on is stored in the urls and not in the stream.  what a travesty.
*/
static np_stream *
np_get_stream(URL_Struct *urls)
{
    if(urls)
    {
        NPEmbeddedApp *pEmbeddedApp = (NPEmbeddedApp*)urls->fe_data;
        if(pEmbeddedApp)
        {
            np_data *ndata = (np_data *)pEmbeddedApp->np_data;
            if(ndata && ndata->instance)
            {
                np_stream *stream;
                for (stream=ndata->instance->streams; stream; stream=stream->next)
                {
                	/*
                	 * Matching algorithm: Either this URL is the inital URL for
                	 * the stream, or it's a URL generated by a subsequent byte-
                	 * range request.  We don't bother to keep track of all the 
                	 * URLs for byte-range requests, but we can still detect if 
                	 * the URL matches this stream: since we know that this URL
                	 * belongs to one of the streams for this instance and that
                	 * there can only be one seekable stream for an instance
                	 * active at a time, then we know if this stream is seekable
                	 * and the URL is a byte-range URL then they must match.
                	 * NOTE: We check both urls->high_range and urls->range_header
                	 * because we could have been called before the range_header
                	 * has been parsed and the high_range set.
                	 */
                	if ((stream->initial_urls == urls) ||
                		(stream->seek && (urls->high_range || urls->range_header)))
                		return stream;
                }
            }
        }
    }
    return NULL;
}


/*
 * Create the two plug-in data structures we need to go along
 * with a netlib stream for a plugin: the np_stream, which is
 * private to libplugin, and the NPStream, which is exported
 * to the plug-in.  The NPStream has an opaque reference (ndata)
 * to the associated np_stream.  The np_stream as a reference
 * (pstream) to the NPStream, and another reference (nstream)
 * to the netlib stream.
 */
static np_stream*
np_makestreamobjects(np_instance* instance, NET_StreamClass* netstream, URL_Struct* urls)
{
	np_stream* stream;
	NPStream* pstream;
    XP_List* url_list;
    np_urlsnode* node;
    void*  notifyData;
	
	/* check params */
	if (!instance || !netstream || !urls)
		return NULL;
		
    /* make a npglue stream */
    stream = XP_NEW_ZAP(np_stream);
    if (!stream)
        return NULL;
    
    stream->instance = instance;
    stream->handle = instance->handle;
    stream->nstream = netstream;
    stream->initial_urls = urls;
    
    XP_ASSERT(urls->address);
    StrAllocCopy(stream->url, urls->address);
    stream->len = urls->content_length;

	/* Look for notification data for this URL */
	notifyData = NULL;
    url_list = instance->url_list;
    while ((node = XP_ListNextObject(url_list)) != NULL)
    {
        if (node->urls == urls && node->notify)
        {
        	notifyData = node->notifyData;
        	break;
        }
    }

    /* make a plugin stream (the thing the plugin sees) */
    pstream = XP_NEW_ZAP(NPStream);
    if (!pstream) 
    {
        XP_FREE(stream);
        return NULL;
    }
    pstream->ndata = stream;    /* confused yet? */
    pstream->url = stream->url;
    pstream->end = urls->content_length;
    pstream->lastmodified = (uint32) urls->last_modified;
	pstream->notifyData = notifyData;

    /* make a record of it */
    stream->pstream = pstream;
    stream->next = instance->streams;
    instance->streams = stream;
 
    NPTRACE(0,("np: new stream, %s, %s", instance->mimetype->type, urls->address));

	return stream;
}

/*
 * Do the reverse of np_makestreamobjects: delete the two plug-in
 * stream data structures (the NPStream and the np_stream).
 * We also need to remove the np_stream from the list of streams
 * associated with its instance.
 */
static void
np_deletestreamobjects(np_stream* stream)
{
	np_instance* instance = stream->instance;
	
    /* remove it from the instance list */
    if (stream == instance->streams)
        instance->streams = stream->next;
    else
    {
        np_stream* sx;
        for (sx = instance->streams; sx; sx = sx->next)
            if (sx->next == stream)
            {
                sx->next = sx->next->next;
                break;
            }
    }

    /* and nuke the stream */
    if (stream->pstream) 
    {
        XP_FREE(stream->pstream);
        stream->pstream = 0;
    }
    stream->handle = 0;
    XP_FREE(stream);
}           



/*
 * bing: This is wrong!
 *
 * (a) There should be a delayed load LIST (multiple requests get lost currently!)
 * (b) We should call NET_GetURL with the app in the fe_data (notification uses it!)
 * (c) We need to store the target context (may be different than instance context!)
 */
static void
np_dofetch(URL_Struct *urls, int status, MWContext *window_id)
{
    np_stream *stream = np_get_stream(urls);
    if(stream && stream->instance)
    {
        FE_GetURL(stream->instance->cx, stream->instance->delayedload);
    }
}

unsigned int
NPL_WriteReady(void *obj)
{
    URL_Struct *urls = (URL_Struct *)obj;
    np_stream *stream = nil;
    int ret = 0;

    if(!(stream = np_get_stream(urls)))
        return 0;

	if (stream->asfile == NP_ASFILEONLY)
		return NP_MAXREADY;
		
    XP_ASSERT(stream->instance);
    stream->instance->reentrant = 1;

    if(ISFUNCPTR(stream->handle->f->writeready) && (stream->seek >= 0))
        ret = CallNPP_WriteReadyProc(stream->handle->f->writeready, stream->instance->npp, stream->pstream);
    else
        ret = NP_MAXREADY;

    if(!stream->instance->reentrant)
    {
        urls->pre_exit_fn = np_dofetch;
        return (unsigned int)-1;
    }
    stream->instance->reentrant = 0;

    return ret;
}

int
NPL_Write(void *obj, const unsigned char *str, int32 len)
{
    int ret;
    URL_Struct *urls = (URL_Struct *)obj;
    np_stream *stream = np_get_stream(urls);

    if(!stream || !ISFUNCPTR(stream->handle->f->write))
        return -1;

	if (stream->asfile == NP_ASFILEONLY)
		return len;

    /* if this is the first non seek write after we turned the
       stream, then abort this (netlib) stream */

    if(!urls->high_range && (stream->seek == -1))
        return MK_UNABLE_TO_CONVERT; /* will cause an abort */

    XP_ASSERT(stream->instance);
    stream->instance->reentrant = 1;

    stream->pstream->end = urls->content_length;

    if(stream->seek)
    {
        /* NPTRACE(0,("seek stream gets %d bytes with high %d low %d pos %d\n", len, 
           urls->high_range, urls->low_range, urls->position)); */
        /* since we get one range per urls, position will only be non-zero
           if we are getting additional writes */
        if(urls->position == 0)
            urls->position = urls->low_range;
    }
    ret = CallNPP_WriteProc(stream->handle->f->write, stream->instance->npp, stream->pstream, 
                            urls->position, len, (void *)str);

    urls->position += len;

    if(!stream->instance->reentrant)
    {
        urls->pre_exit_fn = np_dofetch;
        return -1;
    }
    stream->instance->reentrant = 0;

    return ret;
}


static char *
np_make_byterange_string(NPByteRange *ranges)
{
    char *burl;
    NPByteRange *br;
    int count, maxlen;

    for(count=0, br=ranges; br; br=br->next)
        count++;
    maxlen = count*22 + 64; /* two 32-bit numbers, plus slop */

    burl = (char*)XP_ALLOC(maxlen);
    if(burl)
    {    
        char range[64];
        int len=0;
        int iBytesEqualsLen;

        iBytesEqualsLen = strlen(RANGE_EQUALS);

	    /* the range must begin with bytes=
	    * a final range string looks like:
	    *  bytes=5-8,12-56
	    */
	    XP_STRCPY(burl, RANGE_EQUALS);

        for(br=ranges; br; br=br->next)
        {
            int32 brlen = br->length;
            if(len)
                XP_STRCAT(burl, ",");
            if(br->offset<0)
                sprintf(range, "%ld", -((long)(br->length)));
            else
                sprintf(range, "%ld-%ld", br->offset, (br->offset+(brlen-1)));

            len += XP_STRLEN(range);
            if((len + iBytesEqualsLen) >= maxlen)
                break;
            XP_STRCAT(burl, range);
        }

        if(len == 0) /* no ranges added */
            *burl = 0;
    }
    return burl;
}

static NPByteRange *
np_copy_rangelist(NPByteRange *rangeList)
{
    NPByteRange *r, *rn, *rl=0, *rh=0;

    for(r=rangeList; r; r=r->next)
    {
        if(!(rn = XP_NEW_ZAP(NPByteRange)))
            break;
        rn->offset = r->offset;
        rn->length = r->length;

        if(!rh) 
            rh = rn;
        if(rl) 
            rl->next = rn;
        rl = rn;
    }
    return rh;
}


static void
np_lock(np_stream *stream)
{
    if(!stream->islocked)
    {
        NET_ChangeCacheFileLock(stream->initial_urls, LOCK);
        stream->islocked = 1;
    }
}

static void
np_unlock(np_stream *stream)
{
    if(stream->islocked)
    {
        NET_ChangeCacheFileLock(stream->initial_urls, UNLOCK);
        stream->islocked = 0;
    }
}


NPError
#ifdef XP_WIN16
__export
#endif
npn_requestread(NPStream *pstream, NPByteRange *rangeList)
{
    np_stream *stream;

    if (pstream == NULL || rangeList == NULL)
        return NPERR_INVALID_PARAM;

    stream = (np_stream *)pstream->ndata;

    if (stream == NULL)
        return NPERR_INVALID_PARAM;

    /* requestread may be called before newstream has returned */
    if (stream)
    {
        if (!stream->seekable)
        {
	    	/*
	    	 * If the stream is not seekable, we can only fulfill
	    	 * the request if we're going to cache it (seek == 2);
	    	 * otherwise it's an error.   If we are caching it,
	    	 * the request must still wait until the entire file
	    	 * is cached (when NPL_Complete is finally called).
	    	 */
        	if (stream->seek == 2)
        	{
				/* defer the work */
	            NPTRACE(0,("\tdeferred the request"));

	            if(!stream->deferred)
	                stream->deferred = np_copy_rangelist(rangeList);
	            else
	            {
	                NPByteRange *r;
	                /* append */
	                for(r=stream->deferred;r && r->next;r++)
	                    ;
	                if(r)
	                {
	                    XP_ASSERT(!r->next);
	                    r->next = np_copy_rangelist(rangeList);
	                }
	            }
	    	}
	    	else
	    		return NPERR_STREAM_NOT_SEEKABLE;
        }
        else
        {
            char *ranges;

            /* if seeking is ok, we delay this abort until now to get
               the most out of the existing connection */
            if(!stream->seek)
                stream->seek = -1; /* next write aborts */

            if ((ranges = np_make_byterange_string(rangeList)) != NULL)
            {
                URL_Struct *urls;
                urls = NET_CreateURLStruct(stream->url, NET_DONT_RELOAD);
                urls->range_header = ranges;
                XP_ASSERT(stream->instance);
                if(stream->instance)
                {
                    urls->fe_data = (void *)stream->instance->app;
                    (void) NET_GetURL(urls, FO_CACHE_AND_BYTERANGE, stream->instance->cx, NPL_URLExit);
                }
            }
        }
    }
    
    return NPERR_NO_ERROR;
}


static void
np_destroystream(np_stream *stream, NPError reason)
{
    if (stream)
    {
    	/* Tell the plug-in that the stream is going away, and why */
        np_instance *instance = stream->instance;
        if(ISFUNCPTR(stream->handle->f->destroystream))
        {
            CallNPP_DestroyStreamProc(stream->handle->f->destroystream, instance->npp, stream->pstream, reason);
        }

		/* Delete the np_stream and associated NPStream objects */
		np_deletestreamobjects(stream);
    }           
}


void
np_streamAsFile(np_stream* stream)
{
    char* fname = NULL;
	XP_ASSERT(stream->asfile);

    if (stream->initial_urls)
    {
		if (NET_IsLocalFileURL(stream->initial_urls->address))
		{
			char* pathPart = NET_ParseURL(stream->initial_urls->address, GET_PATH_PART);
            fname = WH_FileName(pathPart, xpURL);
			XP_FREE(pathPart);
		}
		else if (stream->initial_urls->cache_file)
        {
        	np_urlsnode* node;
        	
            fname = WH_FileName(stream->initial_urls->cache_file, xpCache);

			/* Lock the file in the cache until we're done with it */
			np_lock(stream);
			node = np_addURLtoList(stream->instance);
            if (node)
            {
                /* make a copy of the urls */
                URL_Struct* newurls = NET_CreateURLStruct(stream->initial_urls->address, NET_DONT_RELOAD);

                /* add the struct to the node */
                node->urls = newurls;
                node->cached = TRUE;
            }
       }
	}
	
    /* fname can be NULL if something went wrong */
    if (ISFUNCPTR(stream->handle->f->asfile))
	    CallNPP_StreamAsFileProc(stream->handle->f->asfile, stream->instance->npp, stream->pstream, fname);

	if (fname) XP_FREE(fname);
}


void
NPL_Complete(void *obj)
{
    URL_Struct *urls = (URL_Struct *)obj;
    np_stream *stream = nil;

    if(!(stream = np_get_stream(urls)))
        return;

    if(stream->seek)
    {
        if(stream->seek == 2)
        {
            /* request all the outstanding reads that had been waiting */
            stream->seekable = 1; /* for cgi hack */
            stream->seek = 1;
            np_lock(stream);
            npn_requestread(stream->pstream, stream->deferred);
            /* and delete the copies we made */
            {
                NPByteRange *r, *rl=0;
                for(r=stream->deferred; r; rl=r, r=r->next)
                    if(rl) XP_FREE(rl);
                if(rl) XP_FREE(rl);
                stream->deferred = 0;
            }
            np_unlock(stream);
        }
    }

    if (stream->asfile)
		np_streamAsFile(stream);
		
    stream->nstream = NULL;		/* Remove reference to netlib stream */

    if (!stream->dontclose)
        np_destroystream(stream, NPRES_DONE);
}


void 
NPL_Abort(void *obj, int status)
{
    URL_Struct *urls = (URL_Struct *)obj;
    np_stream *stream = nil;

    if(!(stream = np_get_stream(urls)))
        return;

    if(stream->seek == -1)
    {
        /* this stream is being turned around */
        stream->seek = 1;
    }

    stream->nstream = NULL;		/* Remove reference to netlib stream */

	/*
	 * MK_UNABLE_TO_CONVERT is the special status code we
	 * return from NPL_Write to cancel the original netlib
	 * stream when we get a byte-range request, so we don't
	 * want to destroy the plug-in stream in this case (we
	 * shouldn't get this status code any other time here).
	 */
    if (!stream->dontclose || (status < 0 && status != MK_UNABLE_TO_CONVERT))
        np_destroystream(stream, np_statusToReason(status));
}

extern XP_Bool
NPL_HandleURL(MWContext *cx, FO_Present_Types iFormatOut, URL_Struct *pURL, Net_GetUrlExitFunc *pExitFunc)
{
    /* check the cx for takers */
    return FALSE;
}


/*
 * This exit routine is called for embed streams (the
 * initial stream created when the plug-in is instantiated).
 * We use a special exit routine in this case because FE's
 * may want to take additional action when a plug-in stream
 * finishes (e.g. show special error status indicating why
 * the stream failed).
 */
/* bing: This needs to have all the code in NPL_URLExit in it too! (notification) */
void
NPL_EmbedURLExit(URL_Struct *urls, int status, MWContext *cx)
{
	if (urls)
    {
#ifdef XP_WIN	 
		/* WinFE is responsible for deleting the URL_Struct */   
	    FE_EmbedURLExit(urls, status, cx);
#else
        NET_FreeURLStruct (urls);
#endif
    }
}

/* 
 * This exit routine is used for all streams requested by the
 * plug-in: byterange request streams, NPN_GetURL streams, and 
 * NPN_PostURL streams.  NOTE: If the exit routine gets called
 * in the course of a context switch, we must NOT delete the
 * URL_Struct.  Example: FTP post with result from server
 * displayed in new window -- the exit routine will be called
 * when the upload completes, but before the new context to
 * display the result is created, since the display of the
 * results in the new context gets its own completion routine.
 */
void
NPL_URLExit(URL_Struct *urls, int status, MWContext *cx)
{
    if (urls && status != MK_CHANGING_CONTEXT)
    {
        NPEmbeddedApp* app = (NPEmbeddedApp*) urls->fe_data;
    	np_stream* pstream = np_get_stream(urls);

    	if (pstream)
    	{
			/*
			 * MK_UNABLE_TO_CONVERT is the special status code we
			 * return from NPL_Write to cancel the original netlib
			 * stream when we get a byte-range request, so we don't
			 * want to destroy the plug-in stream in this case (we
			 * shouldn't get this status code any other time here).
			 */
    		if (!pstream->dontclose || (status < 0 && status != MK_UNABLE_TO_CONVERT))
	    		np_destroystream(pstream, np_statusToReason(status));
	    		
	    	/*
	    	 * If the initial URL_Struct is being deleted, break our
	    	 * reference to it (we might need to unlock it, too).
	    	 */
	    	if (pstream->initial_urls == urls)
	    	{
	    		np_unlock(pstream);
	    		pstream->initial_urls = NULL;
	    	}
	    }

		/*
		 * Check to see if the instance wants
		 * to be notified of the URL completion.
		 */
        if (app)
        {
            np_data* ndata = (np_data*) app->np_data;
            if (ndata && ndata->instance)
            	np_removeURLfromList(ndata->instance, urls, status);
		}
		
        NET_FreeURLStruct(urls);
    }
}
 
             

static URL_Struct*
np_makeurlstruct(np_instance* instance, const char* relativeURL)
{
	History_entry* history;   
	URL_Struct* temp_urls = NULL;
    char* absoluteURL = NULL;
    URL_Struct* urls = NULL;   
	
	if (!instance || !relativeURL)
		return NULL;
		
    /*
     * Convert the (possibly) relative URL passed in by the plug-in
     * to a guaranteed absolute URL.  To do this we need the base
     * URL of the page we're on, which we can get from the history
     * info in the plug-in's context.
     */
    XP_ASSERT(instance->cx);
   	history = SHIST_GetCurrent(&instance->cx->hist);
   	if (history)
   	    temp_urls = SHIST_CreateURLStructFromHistoryEntry(instance->cx, history);
   	if (temp_urls)
   	{
   		absoluteURL = NET_MakeAbsoluteURL(temp_urls->address, (char*) relativeURL);
		NET_FreeURLStruct(temp_urls);
	}


	/*
	 * Now that we've got the absolute URL string, make a NetLib struct
	 * for it. If something went wrong making the absolute URL, fall back
	 * on the relative one.
	 */
	XP_ASSERT(absoluteURL);
   	if (absoluteURL)
   	{
    	urls = NET_CreateURLStruct(absoluteURL, NET_NORMAL_RELOAD);
	    XP_FREE(absoluteURL);
	}
    else
        urls = NET_CreateURLStruct(relativeURL, NET_NORMAL_RELOAD);

	return urls;
}


static MWContext*
np_makecontext(np_instance* instance, const char* window)
{
	MWContext* cx;
	
    /* Figure out which context to do this on */
    if ((!strcmp(window, "_self")) || (!strcmp(window, "_current")))
        cx = instance->cx;
    else
        cx = XP_FindNamedContextInList(instance->cx, (char*) window);

    /* If we didn't find a context, make a new one */
    if (!cx)
        cx = FE_MakeNewWindow(instance->cx, NULL, (char*) window, NULL);
        
	return cx;
}



static NPError
np_geturlinternal(NPP npp, const char* relativeURL, const char* target, NPBool notify, void* notifyData)
{
    URL_Struct* urls = NULL;   
    MWContext* cx;
    np_instance* instance;
    np_urlsnode* node = NULL;
    NPError err = NPERR_NO_ERROR;
#ifdef XP_WIN32
	void*	pPrevState;
#endif
    
    if (!npp || !relativeURL)		/* OK for window to be NULL */
    	return NPERR_INVALID_PARAM;
    	
    instance = (np_instance*) npp->ndata;
    if (!instance)
        return NPERR_INVALID_PARAM;
    
    /* Make an abolute URL struct from the (possibly) relative URL passed in */
    urls = np_makeurlstruct(instance, relativeURL);
    if (!urls)
    {
    	err = NPERR_OUT_OF_MEMORY_ERROR;
    	goto error;
 	}
 
 	/*
 	 * Add this URL to the list of URLs for this instance,
 	 * and remember if the instance would like notification.
 	 */
	node = np_addURLtoList(instance);
	if (node)
	{
 	 	node->urls = urls;
 	 	if (notify)
 	 	{
			node->notify = TRUE;
			node->notifyData = notifyData;
		}
	}
	else
	{
    	err = NPERR_OUT_OF_MEMORY_ERROR;
    	goto error;
	}

	urls->fe_data = (void*) instance->app;
 	
    /*
     * If the plug-in passed NULL for the target, load the URL with a special stream
     * that will deliver the data to the plug-in; otherwise, convert the target name
     * they passed in to a context and load the URL into that context (possibly unloading
     * the plug-in in the process, if the target context is the plug-in's context).
     */
    if (!target)
	{
#ifdef XP_WIN32
		pPrevState = WFE_BeginSetModuleState();
#endif
        (void) NET_GetURL(urls, FO_CACHE_AND_PLUGIN, instance->cx, NPL_URLExit);
#ifdef XP_WIN32
		WFE_EndSetModuleState(pPrevState);
#endif
    }
	else
    {
    	cx = np_makecontext(instance, target);
	    if (!cx)
	    {
	    	err = NPERR_OUT_OF_MEMORY_ERROR;
	    	goto error;
	    }

		/* 
		 * Prevent loading "about:" URLs into the plug-in's context: NET_GetURL
		 * for these URLs will complete immediately, and the new layout thus
		 * generated will blow away the plug-in and possibly unload its code,
		 * causing us to crash when we return from this function.
		 */
		if (cx == instance->cx && NET_URL_Type(urls->address) == ABOUT_TYPE_URL)
		{
			err = NPERR_INVALID_URL;
			goto error;
		}
		
#ifdef XP_MAC
		/*
		 * One day the code below should call FE_GetURL, and this call will be
		 * unnecessary since the FE will do the right thing.  Right now (3.0b6)
		 * starting to use FE_GetURL is not an option so we'll just create 
		 * (yet another) FE callback for our purposes: we need to ask the FE
		 * to reset any timer it might have (for META REFRESH) so that the 
		 * timer doesn't go off after leaving the original page via plug-in
		 * request.
		 */
		FE_ResetRefreshURLTimer(cx);
#endif

        /* reentrancy matters for this case because it will cause the current
           stream to be unloaded which netlib can't deal with */
        if (instance->reentrant && (cx == instance->cx))
        {
        	XP_ASSERT(instance->delayedload == NULL);	/* We lose queued requests if this is non-NULL! */
            if (instance->delayedload)
                NET_FreeURLStruct(instance->delayedload);
            instance->delayedload = urls;
            instance->reentrant = 0;
        }
        else
		{
#ifdef XP_WIN32
			pPrevState = WFE_BeginSetModuleState();
#endif
        	(void) NET_GetURL(urls, FO_CACHE_AND_PRESENT, cx, NPL_URLExit);
#ifdef XP_WIN32
			WFE_EndSetModuleState(pPrevState);
#endif
		}
    }

    return NPERR_NO_ERROR;
    
error:
	if (node)
	{
		node->notify = FALSE;		/* Remove node without notification */
		np_removeURLfromList(instance, urls, 0);
	}
	if (urls)
		NET_FreeURLStruct(urls);
	return err;
}


static NPError
np_parsepostbuffer(URL_Struct* urls, const char* buf, uint32 len)
{
	/*
	 * Search the buffer passed in for a /n/n. If we find it, break the
	 * buffer in half: the first part is the header, the rest is the body.
	 */
	uint32 index;
	for (index = 0; index < len; index++)
	{
		if (buf[index] == '\n' && ++index < len && buf[index] == '\n')
			break;
	}
	
	/*
	 * If we found '\n\n' somewhere in the middle of the string then we 
	 * have headers, so we need to allocate a new string for the headers,
	 * copy the header data from the plug-in's buffer into it, and put
	 * it in the appropriate place of the URL struct.
	 */
	if (index > 1 && index < len)
		{
		uint32 headerLength = index;
		char* headers = (char*) XP_ALLOC(headerLength + 1);
	    if (!headers)
	    	return NPERR_OUT_OF_MEMORY_ERROR;
		XP_MEMCPY(headers, buf, headerLength);
		headers[headerLength] = 0;
		urls->post_headers = headers;
		}
	
	/*
	 * If we didn't find '\n\n', then the body starts at the beginning;
	 * otherwise, it starts right after the second '\n'.  Make sure the
	 * body is non-emtpy, allocate a new string for it, copy the data
	 * from the plug-in's buffer, and put it in the URL struct.
	 */
	if (index >= len) 
		index = 0;								/* No '\n\n', start body from beginning */
	else
		index++;								/* Found '\n\n', start body after it */
		
	if (len - index > 0)						/* Non-empty body? */
	{
		uint32 bodyLength = len - index + 1;
		char* body = (char*) XP_ALLOC(bodyLength);
	    if (!body)
	    	return NPERR_OUT_OF_MEMORY_ERROR;
		XP_MEMCPY(body, &(buf[index]), bodyLength);
	    urls->post_data = body;
	    urls->post_data_size = bodyLength;
		urls->post_data_is_file = FALSE;
	}	
	else
	{
		/* Uh-oh, no data to post */
		return NPERR_NO_DATA;
	}
	
	return NPERR_NO_ERROR;
}


static NPError
np_posturlinternal(NPP npp, const char* relativeURL, const char *target, uint32 len, const char *buf, NPBool file, NPBool notify, void* notifyData)
{
    np_instance* instance;
    URL_Struct* urls = NULL; 
    char* filename = NULL; 
    XP_Bool ftp;
    np_urlsnode* node = NULL;
    NPError err = NPERR_NO_ERROR;
#ifdef XP_WIN32
	void*	pPrevState;
#endif
    
    /* Validate paramters */
    if (!npp || !relativeURL)
    	return NPERR_INVALID_PARAM;

    instance = (np_instance*) npp->ndata;
    if (!instance)
        return NPERR_INVALID_PARAM;
   
    /* Make an absolute URL struct from the (possibly) relative URL passed in */
    urls = np_makeurlstruct(instance, relativeURL);
    if (!urls)
    	return NPERR_INVALID_URL;


 	/*
 	 * Add this URL to the list of URLs for this instance,
 	 * and remember if the instance would like notification.
 	 */
	node = np_addURLtoList(instance);
	if (node)
	{
 	 	node->urls = urls;
 	 	if (notify)
 	 	{
			node->notify = TRUE;
			node->notifyData = notifyData;
		}
	}
	else
		return NPERR_OUT_OF_MEMORY_ERROR;
 	
	/* 
	 * FTP protocol requires that the data be in a file.
	 * If we really wanted to, we could write code to dump the buffer to
	 * a temporary file, give the temp file to netlib, and delete it when
	 * the exit routine fires.  bing: We need to do this!
	 */
	ftp = (strncasecomp(urls->address, "ftp:", 4) == 0);
	if (ftp && !file)
	{
		err = NPERR_INVALID_URL;		
		goto error;				
	}
	
	if (file)
	{
		XP_StatStruct stat;
		
		/* If the plug-in passed a file URL, strip the 'file://' */
		if (!strncasecomp(buf, "file://", 7))
			filename = XP_STRDUP((char*) buf + 7);
		else
			filename = XP_STRDUP((char*) buf);

		if (!filename)
		{
			err = NPERR_OUT_OF_MEMORY_ERROR;
			goto error;
		}
		
		/* If the file doesn't exist, return an error NOW before netlib get it */
		if (XP_Stat(filename, &stat, xpURL))
		{
			err = NPERR_FILE_NOT_FOUND;
			goto error;
		}
	}
	
	/*
	 * NET_GetURL handles FTP posts differently: the post_data fields are
	 * ignored; instead, files_to_post contains an array of the files.
	 */
	if (ftp)
	{
		XP_ASSERT(filename);
		urls->files_to_post = (char**) XP_ALLOC(sizeof(char*) + sizeof(char*));
		if (!(urls->files_to_post))
		{
			err = NPERR_OUT_OF_MEMORY_ERROR;
			goto error;
		}
		urls->files_to_post[0] = filename;
		urls->files_to_post[1] = NULL;
	    urls->post_data = NULL;
	    urls->post_data_size = 0;
	    urls->post_data_is_file = FALSE;
	}
	else if (file)
	{
		XP_ASSERT(filename);
	    urls->post_data = filename;
	    urls->post_data_size = XP_STRLEN(filename);
	    urls->post_data_is_file = TRUE;
	}
	else
	{
		/* 
		 * There are two different sets of buffer-parsing code.
		 * The new code is contained within np_parsepostbuffer,
		 * and is used when the plug-in calls NPN_PostURLNotify.
		 * The old code, below, is preserved for compatibility
		 * for when the plug-in calls NPN_PostURL.
		 */
		if (notify)
		{
			NPError err = np_parsepostbuffer(urls, buf, len);
			if (err != NPERR_NO_ERROR)
				goto error;
		}
		else
		{
			urls->post_data = XP_ALLOC(len);
			if (!urls->post_data)
			{
				err = NPERR_OUT_OF_MEMORY_ERROR;
				goto error;
			}
			XP_MEMCPY(urls->post_data, buf, len);
		    urls->post_data_size = len;
			urls->post_data_is_file = FALSE;
		}
	}
	
    urls->method = URL_POST_METHOD;

    if (!target)
    {
        urls->fe_data = (void*) instance->app;
#ifdef XP_WIN32
		pPrevState = WFE_BeginSetModuleState();
#endif
        (void) NET_GetURL(urls, FO_CACHE_AND_PLUGIN, instance->cx, NPL_URLExit);
#ifdef XP_WIN32
		WFE_EndSetModuleState(pPrevState);
#endif
    }
    else
    {
    	MWContext* cx = np_makecontext(instance, target);
	    if (!cx)
		{
			err = NPERR_OUT_OF_MEMORY_ERROR;
			goto error;
		}
        urls->fe_data = (void*) instance->app;

#ifdef XP_MAC
		/*
		 * One day the code below should call FE_GetURL, and this call will be
		 * unnecessary since the FE will do the right thing.  Right now (3.0b6)
		 * starting to use FE_GetURL is not an option so we'll just create 
		 * (yet another) FE callback for our purposes: we need to ask the FE
		 * to reset any timer it might have (for META REFRESH) so that the 
		 * timer doesn't go off after leaving the original page via plug-in
		 * request.
		 */
		FE_ResetRefreshURLTimer(cx);
#endif

#ifdef XP_WIN32
		pPrevState = WFE_BeginSetModuleState();
#endif
    	(void) NET_GetURL(urls, FO_CACHE_AND_PRESENT, cx, NPL_URLExit);
#ifdef XP_WIN32
		WFE_EndSetModuleState(pPrevState);
#endif
	}
	
    return NPERR_NO_ERROR;
    
error:
	if (node)
	{
		node->notify = FALSE;		/* Remove node without notification */
		np_removeURLfromList(instance, urls, 0);
	}
	if (urls)
		NET_FreeURLStruct(urls);
	return err;
}



NPError
#ifdef XP_WIN16
__export
#endif
npn_geturlnotify(NPP npp, const char* relativeURL, const char* target, void* notifyData)
{
	return np_geturlinternal(npp, relativeURL, target, TRUE, notifyData);
}

#ifdef XP_UNIX
NPError
#ifdef XP_WIN16
__export
#endif
npn_getvalue(NPP npp, NPNVariable variable, void *r_value)
{
    np_instance* instance;
	NPError ret = NPERR_NO_ERROR;

	/* Some of these variabled may be handled by backend. The rest is FE.
	 * So Handle all the backend variables and pass the rest over to FE.
	 */

	switch(variable) {
		default:
			instance = NULL;
			if (npp != NULL) {
	    		instance = (np_instance*) npp->ndata;
			}
			ret = FE_PluginGetValue(instance?instance->handle->pdesc:NULL,
									variable, r_value);
	}

	return(ret);
}
#endif /* XP_UNIX */


NPError
#ifdef XP_WIN16
__export
#endif
npn_geturl(NPP npp, const char* relativeURL, const char* target)
{
	return np_geturlinternal(npp, relativeURL, target, FALSE, NULL);
}


NPError
#ifdef XP_WIN16
__export
#endif
npn_posturlnotify(NPP npp, const char* relativeURL, const char *target, uint32 len, const char *buf, NPBool file, void* notifyData)
{
	return np_posturlinternal(npp, relativeURL, target, len, buf, file, TRUE, notifyData);
}


NPError
#ifdef XP_WIN16
__export
#endif
npn_posturl(NPP npp, const char* relativeURL, const char *target, uint32 len, const char *buf, NPBool file)
{
	return np_posturlinternal(npp, relativeURL, target, len, buf, file, FALSE, NULL);
}



NPError
#ifdef XP_WIN16
__export
#endif
npn_newstream(NPP npp, NPMIMEType type, const char* window, NPStream** pstream)
{
    np_instance* instance;
	np_stream* stream;
    NET_StreamClass* netstream;
    URL_Struct* urls;
    MWContext* cx;
	*pstream = NULL;
	
    if (!npp || !type)
    	return NPERR_INVALID_PARAM;
    instance = (np_instance*) npp->ndata;
    if (!instance)
        return NPERR_INVALID_PARAM;

	/* Convert the window name to a context */
	cx = np_makecontext(instance, window);
    if (!cx)
    	return NPERR_OUT_OF_MEMORY_ERROR;

	/*
	 * Make a bogus URL struct.  The URL doesn't point to
	 * anything, but we need it to build the stream.
	 */
    urls = NET_CreateURLStruct("", NET_DONT_RELOAD);
    if (!urls)
    	return NPERR_OUT_OF_MEMORY_ERROR;
    StrAllocCopy(urls->content_type, type);
    
	/* Make a netlib stream */
    netstream = NET_StreamBuilder(FO_PRESENT, urls, cx);
    if (!netstream)
    {
    	NET_FreeURLStruct(urls);
    	return NPERR_OUT_OF_MEMORY_ERROR;
    }
    
    /* Make the plug-in stream objects */
    stream = np_makestreamobjects(instance, netstream, urls);
    if (!stream)
    {
    	XP_FREE(netstream);
    	NET_FreeURLStruct(urls);
    	return NPERR_OUT_OF_MEMORY_ERROR;
    }

	*pstream = stream->pstream;
    return NPERR_NO_ERROR;
}


int32
#ifdef XP_WIN16
__export
#endif
npn_write(NPP npp, NPStream *pstream, int32 len, void *buffer)
{
    np_instance* instance;
    np_stream* stream;
    NET_StreamClass* netstream;
    
    if (!npp || !pstream || !buffer || len<0)
        return NPERR_INVALID_PARAM;

    instance = (np_instance*) npp->ndata;
    stream = (np_stream*) pstream->ndata;

    if (!instance || !stream)
        return NPERR_INVALID_PARAM;
    
    netstream = stream->nstream;
    if (!netstream)
    	return NPERR_INVALID_PARAM;
    	
    return (*netstream->put_block)(netstream->data_object, (const char*) buffer, len);
}

NPError
#ifdef XP_WIN16
__export
#endif
npn_destroystream(NPP npp, NPStream *pstream, NPError reason)
{
    np_instance* instance;
    np_stream* stream;
    NET_StreamClass* netstream;
    URL_Struct* urls = NULL;
    
    if (!npp || !pstream)
        return NPERR_INVALID_PARAM;

    instance = (np_instance*) npp->ndata;
    stream = (np_stream*) pstream->ndata;

    if (!instance || !stream)
        return NPERR_INVALID_PARAM;
    
    netstream = stream->nstream;
    if (netstream)
    	urls = (URL_Struct*) netstream->data_object;
    
    /*
     * If we still have a valid netlib stream, ask netlib
     * to destroy it (it will call us back to inform the
     * plug-in and delete the plug-in-specific objects).
     * If we don't have a netlib stream (possible if the
     * stream was in NP_SEEK mode: the netlib stream might
     * have been deleted but we would keep the plug-in 
     * stream around because stream->dontclose was TRUE),
     * just inform the plug-in and delete our objects.
     */
	stream->dontclose = FALSE;		/* Make sure we really delete */
    if (urls)
    {
		if (NET_InterruptStream(urls) < 0)
		{
			/* Netlib doesn't know about this stream; we must have made it */
			/*MWContext* cx = netstream->window_id;*/
		    switch (reason)
		    {
		    	case NPRES_DONE:
		    		(*netstream->complete)(netstream->data_object);
		    		break;
		    	case NPRES_USER_BREAK:
		    		(*netstream->abort)(netstream->data_object, MK_INTERRUPTED);
		    		break;
		    	case NPRES_NETWORK_ERR:
		    		(*netstream->abort)(netstream->data_object, MK_BAD_CONNECT);
		    		break;
		    	default:			/* Unknown reason code */
		    		(*netstream->abort)(netstream->data_object, -1);	
		    		break;
		    }
    		np_destroystream(stream, reason);
    		XP_FREE(netstream);
		}
	}
    else
    	np_destroystream(stream, reason);
	
/*
 * bing:
 * We still need a way to pass the right status code
 * through to NPL_Abort (NET_InterruptStream doesn't
 * take a status code, so the plug-in always gets
 * NPRES_USER_BREAK, not what they passed in here).
 */
    return NPERR_NO_ERROR;
}


void
#ifdef XP_WIN16
__export
#endif
npn_status(NPP npp, const char *message)
{
    if(npp)
    {
        np_instance *instance = (np_instance *)npp->ndata;
        if(instance && instance->cx)
#ifdef XP_MAC
			/* Special entry point so MacFE can save/restore port state */
			FE_PluginProgress(instance->cx, message);
#else
            FE_Progress(instance->cx, message);
#endif
    }
}

#ifdef XP_MAC
#ifndef powerc
#pragma pointers_in_D0
#endif
#endif
const char *
#ifdef XP_WIN16
__export
#endif
npn_useragent(NPP npp)
{
    static char *uabuf = 0;
    if(!uabuf)
        uabuf = PR_smprintf("%.100s/%.90s", XP_AppCodeName, XP_AppVersion);
    return (const char *)uabuf;
}
#ifdef XP_MAC
#ifndef powerc
#pragma pointers_in_A0
#endif
#endif

#ifdef XP_MAC
#ifndef powerc
#pragma pointers_in_D0
#endif
#endif
void *
#ifdef XP_WIN16
__export
#endif
npn_memalloc (uint32 size)
{
    return XP_ALLOC(size);
}
#ifdef XP_MAC
#ifndef powerc
#pragma pointers_in_A0
#endif
#endif

void
#ifdef XP_WIN16
__export
#endif
npn_memfree (void *ptr)
{
    (void)XP_FREE(ptr);
}

#ifdef XP_MAC
/* For the definition of CallCacheFlushers() */
#include "prmacos.h"
#endif

uint32
#ifdef XP_WIN16
__export
#endif
npn_memflush(uint32 size)
{
#ifdef XP_MAC
    /* Try to free some memory and return the amount we freed. */
    if (CallCacheFlushers(size))
        return size;
    else
#endif
    return 0;
}


void
#ifdef XP_WIN16
__export
#endif
npn_reloadplugins(NPBool reloadPages)
{
    /*
     * Ask the FE to throw away its old plugin handlers and
     * re-scan the plugins folder to find new ones.  This function
     * is intended for use by the null plugin to signal that
     * some new plugin has been installed and we should make a
     * note of it.  If "reloadPages" is true, we should also
     * reload all open pages with plugins on them (since plugin
     * handlers could have come or gone as a result of the re-
     * registration).
     */
    FE_RegisterPlugins();
     
    if (reloadPages)
    {
        short i;
        XP_List* cxList = XP_GetGlobalContextList();
		for (i = 1; i <= XP_ListCount(cxList); i++)
		{
			MWContext* cx = (MWContext*) XP_ListGetObjectNum(cxList, i);
            if (cx->pluginList != NULL)		/* Any plugins on this page? */
            {
            	History_entry* history = SHIST_GetCurrent(&cx->hist);
            	URL_Struct* urls = SHIST_CreateURLStructFromHistoryEntry(cx, history);
		        if (urls)
		        {
		            urls->force_reload = NET_NORMAL_RELOAD;
            		FE_GetURL(cx, urls);
            	}
            }
		}
    }
}


NPError
NPL_RefreshPluginList(XP_Bool reloadPages)
{
	npn_reloadplugins(reloadPages);
	return NPERR_NO_ERROR;		/* Always succeeds for now */
}



/******************************************************************************/

#ifdef JAVA
#define IMPLEMENT_netscape_plugin_Plugin
#include "netscape_plugin_Plugin.h"
#ifdef MOCHA
#include "libmocha.h"
#endif /* MOCHA */
#endif /* JAVA */

#ifdef XP_MAC
#ifndef powerc
#pragma pointers_in_D0
#endif
#endif
JRIEnv*
#ifdef XP_WIN16
__export
#endif
npn_getJavaEnv(void)
{
#ifdef JAVA
    int err;
    extern int lj_java_enabled;

    err = LJ_StartupJava();
    if (err != LJ_INIT_OK) {
		LJ_ReportJavaStartupError(XP_FindSomeContext(), err);
		return NULL;
	}
	if (lj_java_enabled)
		return JRI_GetCurrentEnv();
    else
		return NULL;
#else
    return NULL;
#endif
}
#ifdef XP_MAC
#ifndef powerc
#pragma pointers_in_A0
#endif
#endif


#define NPN_NO_JAVA_INSTANCE	((jglobal)-1)

jglobal classPlugin = NULL;

#ifdef XP_MAC
#ifndef powerc
#pragma pointers_in_D0
#endif
#endif
jref
#ifdef XP_WIN16
__export
#endif
npn_getJavaPeer(NPP npp)
{
    jref javaInstance = NULL;
#ifdef JAVA
    np_instance* instance;
 
    if (npp == NULL)
		return NULL;
    instance = (np_instance*) npp->ndata;
	if (instance == NULL) return NULL;

	if (instance->javaInstance == NPN_NO_JAVA_INSTANCE) {
		/* Been there, done that. */
		return NULL;
	}
    else if (instance->javaInstance != NULL) {
		/*
		** It's ok to get the JRIEnv here -- it won't initialize the
		** runtime because it would have already been initialized to
		** create the instance that we're just about to return.
		*/
		return JRI_GetGlobalRef(npn_getJavaEnv(), instance->javaInstance);
	}
    else {
		jref clazz;
		if (instance->handle
			&& instance->handle->f
			&& instance->handle->f->javaClass) {
			JRIEnv* env = npn_getJavaEnv();
			if (env == NULL) {
				instance->javaInstance = NPN_NO_JAVA_INSTANCE;		/* prevent trying this every time around */
				return NULL;
			}
			clazz = JRI_GetGlobalRef(env, instance->handle->f->javaClass);

			if (classPlugin == NULL) {
				/*
				** Make sure we never unload the Plugin class. Why? Because
				** the method and field IDs we're using below have the same
				** lifetime as the class (theoretically):
				*/
				classPlugin = JRI_NewGlobalRef(env, use_netscape_plugin_Plugin(env));
			}

			/* instantiate the plugin's class: */
			javaInstance = netscape_plugin_Plugin_new(env, clazz);
			if (javaInstance) {
				/* Store the JavaScript context as the window object: */
				set_netscape_plugin_Plugin_window(env, javaInstance, 
												  LJ_GetMochaWindow(instance->cx)); 

				/* Store the plugin as the peer: */
				set_netscape_plugin_Plugin_peer(env, javaInstance, (jint)instance->npp); 

				instance->javaInstance = JRI_NewGlobalRef(env, javaInstance);
				netscape_plugin_Plugin_init(env, javaInstance);
			}
		}
		else {
			instance->javaInstance = NPN_NO_JAVA_INSTANCE;		/* prevent trying this every time around */
			return NULL;
		}
	}
#endif
    return javaInstance;
}
#ifdef XP_MAC
#ifndef powerc
#pragma pointers_in_A0
#endif
#endif

/******************************************************************************/



static void
np_setwindow(np_instance *instance, NPWindow *appWin)
{
#ifndef XP_MAC
	/*
	 * On Windows and UNIX, we don't want to give a window
	 * to hidden plug-ins.  To determine if we're hidden,
	 * we can look at the flag bit of the LO_EmbedStruct.
	 */
	NPEmbeddedApp* app;
	np_data* ndata;
	LO_EmbedStruct* lo_struct;
	
	if (instance)
	{
		app = instance->app;
		if (app)
		{
			ndata = (np_data*) app->np_data;
			lo_struct = ndata->lo_struct;
			if (lo_struct && lo_struct->extra_attrmask & LO_ELE_HIDDEN)
				return;
		}
	}
#endif

    XP_ASSERT(instance);
    if (instance && appWin)
    {
        if(ISFUNCPTR(instance->handle->f->setwindow))
            CallNPP_SetWindowProc(instance->handle->f->setwindow, instance->npp, appWin);
    }
    else
    {
        NPTRACE(0,("setwindow before appWin was valid"));
    }
}


static np_instance*
np_newinstance(np_handle *handle, MWContext *cx, NPEmbeddedApp *app,
			   np_mimetype *mimetype, char *requestedType)
{
    NPError err = NPERR_GENERIC_ERROR;
    np_instance* instance = NULL;
    NPP npp = NULL;
    void* tmp;

	XP_ASSERT(handle && app);
    if (!handle || !app)
        return NULL;

    /* make sure the plugin is loaded */
    if (!handle->refs)
    {
#ifdef JAVA
		JRIEnv* env;
#endif
		FE_Progress(cx, XP_GetString(XP_PLUGIN_LOADING_PLUGIN));               
        if (!(handle->f = FE_LoadPlugin(handle->pdesc, &npp_funcs))) 
		{
			char* msg = PR_smprintf(XP_GetString(XP_PLUGIN_CANT_LOAD_PLUGIN), handle->name, mimetype->type);
			FE_Alert(cx, msg);
			XP_FREE(msg);
			return NULL;
		}
#ifdef JAVA
		/*
		** Don't use npn_getJavaEnv here. We don't want to start the
		** interpreter, just use env if it already exists.
		*/
		env = JRI_GetCurrentEnv();

		/*
		** An exception could have occurred when the plugin tried to load
		** it's class file. We'll print any exception to the console.
		*/
		if (env && JRI_ExceptionOccurred(env)) {
			JRI_ExceptionDescribe(env);
			JRI_ExceptionClear(env);
		}
#endif
    }

    /* make an instance */
    if (!(instance = XP_NEW_ZAP(np_instance)))
        goto error;

    instance->handle = handle;
    instance->cx = cx;
    instance->app = app;
	instance->mimetype = mimetype;
	instance->type = (app->pagePluginType == NP_FullPage) ? NP_FULL : NP_EMBED;
    app->type = NP_Plugin;
	
    /* make an NPP */
    if (!(tmp = XP_NEW_ZAP(NPP_t)))
        goto error;
    npp = (NPP) tmp;             /* some stupid pc compiler */
    npp->ndata = instance;
    instance->npp = npp;

    /* invite the plugin */
    if (ISFUNCPTR(handle->f->newp))
    {
        NPSavedData* savedData;
        np_data* ndata;
        
        ndata = (np_data*) app->np_data;
        XP_ASSERT(ndata);
        savedData = ndata->sdata;
        
        if (instance->type == NP_EMBED)
        {
        	/* Embedded plug-ins get their attributes passed in from layout */
            int16 argc = (int16) ndata->lo_struct->attribute_cnt;
            char** names = ndata->lo_struct->attribute_list;
            char** values = ndata->lo_struct->value_list;

	        err = CallNPP_NewProc(handle->f->newp, requestedType, npp, 
	        			instance->type, argc, names, values, savedData);
        }
        else
        {
            /* A full-page plugin must be told its palette may
               be realized as a foreground palette */ 
	        char name[] = "PALETTE";
	        char value[] = "foreground";
	        char* names[1];
	        char* values[1];
            int16 argc = 1;
            names[0] = name;
            values[0] = value;

	        err = CallNPP_NewProc(handle->f->newp, requestedType, npp, 
	        			instance->type, argc, names, values, savedData);
        }

		if (err != NPERR_NO_ERROR)
			goto error;
    }
	
    /* add this to the handle chain */
    instance->next = handle->instances;
    handle->instances = instance;
    handle->refs++;

	/*
	 * In the full-page case, FE_DisplayEmbed hasn't been called yet, 
	 * so the window hasn't been created and wdata is still NULL.
	 * We don't want to give the plug-in a NULL window.
	 * N.B.: Actually, on the Mac, the window HAS been created (we
	 * need it because even undisplayed/hidden plug-ins may need a
	 * window), so wdata is not NULL; that's why we check the plug-in
	 * type rather than wdata here.
	 */
	if (app->pagePluginType == NP_Embedded)
	{
		XP_ASSERT(app->wdata);
    	np_setwindow(instance, app->wdata);
	}

#ifdef MOCHA
	{
		/* only wait on applets if onload flag */
		lo_TopState *top_state = lo_FetchTopState(XP_DOCID(cx));
		if (top_state != NULL &&
			top_state->mocha_has_onload &&
			top_state->mocha_loading_embeds_count)
		{
			top_state->mocha_loading_embeds_count--;
			LM_SendLoadEvent(cx, LM_XFER_DONE);
		}
	}
#endif /* MOCHA */

    return instance;
    
error:
    /* Unload the plugin if there are no other instances */
    if (handle->refs == 0)
    {
        FE_UnloadPlugin(handle->pdesc);
        handle->f = NULL;
        XP_ASSERT(handle->instances == NULL);
        handle->instances = NULL;
    }

	if (instance)
    	XP_FREE(instance);
    if (npp)
    	XP_FREE(npp);
    return NULL;
}



NET_StreamClass *
np_newstream(URL_Struct *urls, np_handle *handle, np_instance *instance)
{
    NET_StreamClass *nstream = nil;
    NPStream *pstream = nil;
    np_stream *stream = nil;
    uint16 stype;
	XP_Bool alreadyLocal;
	
    /* make a netlib stream */
    if (!(nstream = XP_NEW_ZAP(NET_StreamClass))) 
        return 0;

	/* make the plugin stream data structures */
	stream = np_makestreamobjects(instance, nstream, urls);
	if (!stream)
	{
        XP_FREE(nstream);
        return 0;
	}
	pstream = stream->pstream;
	
	alreadyLocal = NET_IsURLInDiskCache(stream->initial_urls) ||
        		   NET_IsLocalFileURL(urls->address);
        
    /* determine if the stream is seekable */
    if (urls->server_can_do_byteranges || alreadyLocal)
    {
    	/*
    	 * Zero-length streams are never seekable.
    	 * This will force us to download the entire
    	 * stream if a byterange request is made.
    	 */
    	if (urls->content_length > 0)
        	stream->seekable = 1;
    }

    /* and call the plugin */
    instance->reentrant = 1;
    stype = NP_NORMAL;
    if(ISFUNCPTR(handle->f->newstream))
        /*XXX*/CallNPP_NewStreamProc(handle->f->newstream, instance->npp, urls->content_type, 
                                     pstream, stream->seekable, &stype);
    if(!instance->reentrant)
    {
        urls->pre_exit_fn = np_dofetch;
        XP_FREE(nstream);       /* will not call abort */
        return 0;
    }
    instance->reentrant = 0;

    /* see if its hard */
    if(stype == NP_SEEK)
    {
        if(!stream->seekable)
        {
            NPTRACE(0,("stream is dumb, force caching"));
            stream->seek = 2;
        }
        if (!alreadyLocal)
        	urls->must_cache = TRUE;
        stream->dontclose++;
    }
	else if (stype == NP_ASFILE || stype == NP_ASFILEONLY)
    {   
        NPTRACE(0,("stream as file"));
        if (!alreadyLocal)
        	urls->must_cache = TRUE;
        stream->asfile = stype;
    }
	
	/*
	 * If they want just the file, and the file is local, there's 
	 * no need to continue with the netlib stream: just give them
	 * the file and we're done.
	 */
	if (stype == NP_ASFILEONLY)	
	{
		if (urls->cache_file || NET_IsLocalFileURL(urls->address))	
		{
			np_streamAsFile(stream);
			np_destroystream(stream, NPRES_DONE);
			XP_FREE(nstream);
			return NULL;
		}
    }

    /* and populate the netlib stream */
    nstream->name           = "plug-in";
    nstream->complete       = NPL_Complete;
    nstream->abort          = NPL_Abort;
    nstream->is_write_ready = NPL_WriteReady;
    nstream->put_block      = (MKStreamWriteFunc)NPL_Write;
    nstream->data_object    = (void *)urls;
    nstream->window_id      = instance->cx;

    return nstream;
}


NET_StreamClass*
NPL_NewPresentStream(FO_Present_Types format_out, void* type, URL_Struct* urls, MWContext* cx)
{
    np_handle* handle = (np_handle*) type;
    np_instance* instance = NULL;
	np_data* ndata = NULL;
	np_mimetype* mimetype = NULL;
	np_reconnect* reconnect;
    NPEmbeddedApp *app = NULL;

    XP_ASSERT(type && urls && cx);
	if (!type || !urls || !cx)
		return NULL;

	/* fe_data is set by EmbedCreate, which hasn't happed yet for PRESENT streams */
	XP_ASSERT(urls->fe_data == NULL);	
				
	mimetype = np_getmimetype(handle, urls->content_type, TRUE);
	if (!mimetype)
		return NULL;
 
 	/*
 	 * HACK ALERT:
 	 * The following code special-cases the LiveAudio plug-in to open 
 	 * a new chromeless window.  A new window is only opened if there's
 	 * history information for the current context; that prevents us from
 	 * opening ANOTHER new window if the FE has already made one (for
 	 * example, if the user chose "New window for this link" from the
 	 * popup).
 	 */
	if (handle->name && (XP_STRCASECMP(handle->name, "LiveAudio") == 0))
	{
		History_entry* history = SHIST_GetCurrent(&cx->hist);
		if (history)
		{
			MWContext* oldContext = cx;
			Chrome* customChrome = XP_NEW_ZAP(Chrome);
			if (customChrome == NULL)
				return NULL;
			customChrome->w_hint = 144 + 1;
			customChrome->h_hint = 60 + 1;
			customChrome->allow_close = TRUE;
			
			/* Make a new window with no URL or window name, but special chrome */
			cx = FE_MakeNewWindow(oldContext, NULL, NULL, customChrome);
			if (cx == NULL)
			{
				XP_FREE(customChrome);
				return NULL;
			}
			
			/* Switch to the new context, but don't change the exit routine */
			NET_SetNewContext(urls, cx, NULL);
		}
	}

	 
 	/*
 	 * Set up the "reconnect" data, which is used to communicate between this
 	 * function and the call to EmbedCreate that will result from pushing the
 	 * data into the stream below.  EmbedCreate needs to know from us the 
 	 * np_mimetype and requestedtype for this stream, and we need to know from
 	 * it the NPEmbeddedApp that it created.
 	 */
    XP_ASSERT(cx->pluginReconnect == NULL);
	reconnect = XP_NEW_ZAP(np_reconnect);
	if (!reconnect)
		return NULL;
    cx->pluginReconnect = (void*) reconnect;
    reconnect->mimetype = mimetype;
    reconnect->requestedtype = XP_STRDUP(urls->content_type);

	/*
	 * To actually create the instance we need to create a stream of
	 * fake HTML to cause layout to create a new embedded object.
	 * EmbedCreate will be called, which will created the NPEmbeddedApp
	 * and put it into urls->fe_data, where we can retrieve it.
	 */
	{
		static char fakehtml[] = "<embed src=internal-external-plugin width=1 height=1>";
	    NET_StreamClass* viewstream;
	    char* org_content_type = urls->content_type; 
	    
	    urls->content_type = NULL;
	    StrAllocCopy(urls->content_type, TEXT_HTML);
	    urls->is_binary = 1;    						/* ugly flag for mailto and saveas */
	
	    if ((viewstream = NET_StreamBuilder(FO_PRESENT, urls, cx)) != 0)
	    {
	        (*viewstream->put_block)(viewstream->data_object, fakehtml, XP_STRLEN(fakehtml));
	        (*viewstream->complete)(viewstream->data_object);
	    }
	
	    XP_FREE(urls->content_type);
	    urls->content_type = org_content_type;
    }
    
    /*
     * Retrieve the app created by EmbedCreate and stashed in the reconnect data.
     * From the app we can get the np_data, which in turn holds the handle and
     * instance, which we need to create the streams.
     */
    app = reconnect->app;
    XP_FREE(reconnect);
    cx->pluginReconnect = NULL;
    
    if (!app)
    	return NULL;  /* will be NULL if the plugin failed to initialize */
    XP_ASSERT(app->pagePluginType == NP_FullPage);
    
	urls->fe_data = (void*) app;		/* fe_data of plug-in URLs always holds NPEmbeddedApp */
	
    ndata = (np_data*) app->np_data;
    XP_ASSERT(ndata);
    if (!ndata)
    	return NULL;
    	
    handle = ndata->handle;
    instance = ndata->instance;
    XP_ASSERT(handle && instance);
	if (!handle || !instance)
		return NULL;
		
    /* now actually make a plugin and netlib stream */
    return np_newstream(urls, handle, instance);
}



NET_StreamClass*
NPL_NewEmbedStream(FO_Present_Types format_out, void* type, URL_Struct* urls, MWContext* cx)
{
    np_handle* handle = (np_handle*) type;
 	np_data* ndata = NULL;
    NPEmbeddedApp* app = NULL;

    XP_ASSERT(type && urls && cx);
	if (!type || !urls || !cx)
		return NULL;
		
	/* fe_data is set by EmbedCreate, which has already happened for EMBED streams */
    app = (NPEmbeddedApp*) urls->fe_data;
    XP_ASSERT(app);
    if (!app)
    	return NULL;
    XP_ASSERT(app->pagePluginType == NP_Embedded);

    ndata = (np_data*) app->np_data;
	XP_ASSERT(ndata && ndata->lo_struct);
	if (!ndata)
		return NULL;
		
	if (ndata->instance == NULL)
	{
		np_instance* instance;
		np_mimetype* mimetype;

		/* Map the stream's MIME type to a np_mimetype object */
		mimetype = np_getmimetype(handle, urls->content_type, TRUE);
		if (!mimetype)
			return NULL;
	 
	    /* Now that we have the MIME type and the layout data, we can create an instance */
	    instance = np_newinstance(handle, cx, app, mimetype, urls->content_type);
	    if (!instance)
	        return NULL;

	    ndata->instance = instance;
	    ndata->handle = handle;
    }
    
    /* now actually make a plugin and netlib stream */
    return np_newstream(urls, ndata->instance->handle, ndata->instance);
}


static NET_StreamClass *
np_newbyterangestream(FO_Present_Types format_out, void *type, URL_Struct *urls, MWContext *cx)
{
    NET_StreamClass *nstream = nil;

    /* make a netlib stream */
    if (!(nstream = XP_NEW_ZAP(NET_StreamClass))) 
        return 0;

    urls->position = 0;         /* single threaded for now */

    /* populate netlib stream */
    nstream->name           = "plug-in byterange";
    nstream->complete       = NPL_Complete;
    nstream->abort          = NPL_Abort;
    nstream->is_write_ready = NPL_WriteReady;
    nstream->put_block      = (MKStreamWriteFunc)NPL_Write;
    nstream->data_object    = (void *)urls;
    nstream->window_id      = cx;

    return nstream;
}

static NET_StreamClass *
np_newpluginstream(FO_Present_Types format_out, void *type, URL_Struct *urls, MWContext *cx)
{
    NPEmbeddedApp* app = (NPEmbeddedApp*) urls->fe_data;

    if (app)
    {
        np_data *ndata = (np_data *)app->np_data;
        if(ndata && ndata->instance)
        {
            XP_ASSERT(ndata->instance->app == app);
            return np_newstream(urls, ndata->instance->handle, ndata->instance);
        }
    }
    return 0;
}

NPError
NPL_RegisterPluginFile(const char* pluginname, const char* filename, const char* description, void *pdesc)
{
    np_handle* handle;

    NPTRACE(0,("np: register file %s", filename));

#ifdef DEBUG
	/* Ensure uniqueness of pdesc values! */
	for (handle = np_plist; handle; handle = handle->next)
		XP_ASSERT(handle->pdesc != pdesc);
#endif
	
	handle = XP_NEW_ZAP(np_handle);
	if (!handle)
		return NPERR_OUT_OF_MEMORY_ERROR;
	
	StrAllocCopy(handle->name, pluginname);
	StrAllocCopy(handle->filename, filename);
	StrAllocCopy(handle->description, description);
	
	handle->pdesc = pdesc;
	handle->next = np_plist;
   	np_plist = handle;
   	
   	return NPERR_NO_ERROR;
}

/*
 * Given a pluginName and a mimetype, this will enable the plugin for
 * the mimetype and disable anyother plugin that had been enabled for
 * this mimetype.
 *
 * pluginName and type cannot be NULL.
 *
 * WARNING: If enable is FALSE, this doesn't unregister the converters yet.
 */
NPError
NPL_EnablePlugin(NPMIMEType type, const char *pluginName, XP_Bool enabled)
{
    np_handle* handle;
    np_mimetype* mimetype;
    NPTRACE(0,("np: enable plugin %s for type %s", pluginName, type));

	if (!pluginName || !*pluginName || !type || !*type)
		return(NPERR_INVALID_PARAM);

	for (handle = np_plist; handle; handle = handle->next)
	{
		if (!strcmp(handle->name, pluginName))
			break;
	}

	if (!handle)
		/* Plugin with the specified name not found */
		return(NPERR_INVALID_INSTANCE_ERROR);
	
	/* Look for an existing MIME type object for the specified type */
	/* We can't use np_getmimetype, because it respects enabledness and
	   here we don't care */
	for (mimetype = handle->mimetypes; mimetype; mimetype = mimetype->next)
	{
		if (strcasecomp(mimetype->type, type) == 0)
			break;
	}

	if (!mimetype)
		/* This plugin cannot handler the specified mimetype */
		return(NPERR_INVALID_PLUGIN_ERROR);

	/* Find the plug-in that was previously enabled for this type and
	   disable it */
	if (enabled)
	{
		XP_Bool foundType = FALSE;
		np_handle* temphandle;
		np_mimetype* temptype;
		
		for (temphandle = np_plist; temphandle && !foundType; temphandle = temphandle->next)
		{
			for (temptype = temphandle->mimetypes; temptype && !foundType; temptype = temptype->next)
			{
				if (temptype->enabled && strcasecomp(temptype->type, type) == 0)
				{
					temptype->enabled = FALSE;
					foundType = TRUE;
				}
			}
		}
	}
	
	mimetype->enabled = enabled;
	
	if (mimetype->enabled)
	{
		/*
		 * Is this plugin the wildcard (a.k.a. null) plugin?
		 * If so, we don't want to register it for FO_PRESENT
		 * or it will interfere with our normal unknown-mime-
		 * type handling.
		 */
		XP_Bool wildtype = (strcmp(type, "*") == 0);
		
#ifdef XP_WIN
	    /* EmbedStream does some Windows FE work and then calls NPL_NewStream */
		if (!wildtype)
	        NET_RegisterContentTypeConverter(type, FO_PRESENT, handle, EmbedStream);
	    NET_RegisterContentTypeConverter(type, FO_EMBED, handle, EmbedStream); /* XXX I dont think this does anything useful */
#else
		if (!wildtype)
		  {
	        NET_RegisterContentTypeConverter(type, FO_PRESENT, handle, NPL_NewPresentStream);
#ifdef XP_UNIX
			NET_RegisterAllEncodingConverters(type, FO_PRESENT);
			NET_RegisterAllEncodingConverters(type, FO_EMBED);
			NET_RegisterAllEncodingConverters(type, FO_PLUGIN);

			/* While printing we use the FO_SAVE_AS_POSTSCRIPT format type. We want
			 * plugin to possibly handle that case too. Hence this.
			 */
	        NET_RegisterContentTypeConverter(type, FO_SAVE_AS_POSTSCRIPT, handle,
											 NPL_NewPresentStream);
#endif /* XP_UNIX */
		}
	    NET_RegisterContentTypeConverter(type, FO_EMBED, handle, NPL_NewEmbedStream);
#endif
	    NET_RegisterContentTypeConverter(type, FO_PLUGIN, handle, np_newpluginstream);
	    NET_RegisterContentTypeConverter(type, FO_BYTERANGE, handle, np_newbyterangestream);
	}

    return(NPERR_NO_ERROR);
}


/* 
 * Look up the handle and mimetype objects given
 * the pdesc value and the mime type string.
 * Return TRUE if found successfully.
 */
void
np_findPluginType(NPMIMEType type, void* pdesc, np_handle** outHandle, np_mimetype** outMimetype)
{
    np_handle* handle;
    np_mimetype* mimetype;

	*outHandle = NULL;
	*outMimetype = NULL;
	
	/* Look for an existing handle */
	for (handle = np_plist; handle; handle = handle->next)
	{
		if (handle->pdesc == pdesc)
			break;
	}

	if (!handle)
		return;
	*outHandle = handle;
		
	/* Look for an existing MIME type object for the specified type */
	/* We can't use np_getmimetype, because it respects enabledness and here we don't care */
	for (mimetype = handle->mimetypes; mimetype; mimetype = mimetype->next)
	{
		if (strcasecomp(mimetype->type, type) == 0)
			break;
	}
	
	if (!mimetype)
		return;
	*outMimetype = mimetype;
}


void 
np_enablePluginType(np_handle* handle, np_mimetype* mimetype, XP_Bool enabled)
{
	char* type = mimetype->type;
	
	/*
	 * Find the plug-in that was previously
	 * enabled for this type and disable it.
	 */
	if (enabled)
	{
		XP_Bool foundType = FALSE;
		np_handle* temphandle;
		np_mimetype* temptype;
		
		for (temphandle = np_plist; temphandle && !foundType; temphandle = temphandle->next)
		{
			for (temptype = temphandle->mimetypes; temptype && !foundType; temptype = temptype->next)
			{
				if (temptype->enabled && strcasecomp(temptype->type, type) == 0)
				{
					temptype->enabled = FALSE;
					foundType = TRUE;
				}
			}
		}
	}
	
	mimetype->enabled = enabled;

	if (enabled)
	{
		/*
		 * Is this plugin the wildcard (a.k.a. null) plugin?
		 * If so, we don't want to register it for FO_PRESENT
		 * or it will interfere with our normal unknown-mime-
		 * type handling.
		 */
		XP_Bool wildtype = (strcmp(type, "*") == 0);
		
#ifdef XP_WIN
	    /* EmbedStream does some Windows FE work and then calls NPL_NewStream */
		if (!wildtype)
	        NET_RegisterContentTypeConverter(type, FO_PRESENT, handle, EmbedStream);
	    NET_RegisterContentTypeConverter(type, FO_EMBED, handle, EmbedStream); /* XXX I dont think this does anything useful */
#else
		if (!wildtype)
		  {
	        NET_RegisterContentTypeConverter(type, FO_PRESENT, handle, NPL_NewPresentStream);
#ifdef XP_UNIX
			/* While printing we use the FO_SAVE_AS_POSTSCRIPT format type. We want
			 * plugin to possibly handle that case too. Hence this.
			 */
	        NET_RegisterContentTypeConverter(type, FO_SAVE_AS_POSTSCRIPT, handle,
											 NPL_NewPresentStream);
#endif /* XP_UNIX */
		  }
	    NET_RegisterContentTypeConverter(type, FO_EMBED, handle, NPL_NewEmbedStream);
#endif
	    NET_RegisterContentTypeConverter(type, FO_PLUGIN, handle, np_newpluginstream);
	    NET_RegisterContentTypeConverter(type, FO_BYTERANGE, handle, np_newbyterangestream);
	}
}


NPError
NPL_EnablePluginType(NPMIMEType type, void* pdesc, XP_Bool enabled)
{
    np_handle* handle;
    np_mimetype* mimetype;
	
	if (!type)
		return NPERR_INVALID_PARAM;
		
	np_findPluginType(type, pdesc, &handle, &mimetype);
	
	if (!handle || !mimetype)
		return NPERR_INVALID_PARAM;

	np_enablePluginType(handle, mimetype, enabled);
		return NPERR_NO_ERROR;
}


/* xxx currently there is no unregister */
NPError
NPL_RegisterPluginType(NPMIMEType type, const char *extensions, const char* description,
		void* fileType, void *pdesc, XP_Bool enabled)
{
    np_handle* handle = NULL;
    np_mimetype* mimetype = NULL;
    NPTRACE(0,("np: register type %s", type));

	np_findPluginType(type, pdesc, &handle, &mimetype);

	/* We have to find the handle to do anything */
	XP_ASSERT(handle);
	if (!handle)
		return NPERR_INVALID_PARAM;
		
	/*If no existing mime type, add a new type to this handle */
	if (!mimetype)
	{
		mimetype = XP_NEW_ZAP(np_mimetype);
		if (!mimetype)
			return NPERR_OUT_OF_MEMORY_ERROR;
		mimetype->next = handle->mimetypes;
		handle->mimetypes = mimetype;
		mimetype->handle = handle;
		StrAllocCopy(mimetype->type, type);
	}

	/* Enable this plug-in for this type and disable any others */
	np_enablePluginType(handle, mimetype, enabled);

	/* Get rid of old file association info, if any */
	if (mimetype->fassoc)
	{
		void* fileType;
		fileType = NPL_DeleteFileAssociation(mimetype->fassoc);
#if 0
		/* Any FE that needs to free this, implement FE_FreeNPFileType */
		if (fileType)
			FE_FreeNPFileType(fileType);
#endif
		mimetype->fassoc = NULL;
	}
			
	/* Make a new file association and register it with netlib if enabled */
	XP_ASSERT(extensions && description);
    mimetype->fassoc = NPL_NewFileAssociation(type, extensions, description, fileType);
    if (mimetype->fassoc && enabled)
        NPL_RegisterFileAssociation(mimetype->fassoc);
    
    return NPERR_NO_ERROR;
}


/*
 * Add a NPEmbeddedApp to the list of plugins for the specified context.
 * We need to append items, because the FEs depend on them appearing
 * in the list in the same order in which they were created.
 */
void
np_bindContext(NPEmbeddedApp* app, MWContext* cx)
{
	np_data* ndata;
	
	XP_ASSERT(app && cx);
	XP_ASSERT(app->next == NULL);
	
    if (cx->pluginList == NULL)  					/* no list yet, just insert item */
        cx->pluginList = app;
    else                  							/* the list has at least one item in it, append to it */
    {
        NPEmbeddedApp* pItem = cx->pluginList;  	/* the first element */
        while(pItem->next) pItem = pItem->next; 	/* find the last element */
        pItem->next = app;  						/* append */
    }
    
    /* If there's an instance, set the instance's context */
    ndata = (np_data*) app->np_data;
    if (ndata)
    {
    	np_instance* instance = (np_instance*) ndata->instance;
    	if (instance)
    		instance->cx = cx;
    }
    	
}

void
np_unbindContext(NPEmbeddedApp* app, MWContext* cx)
{
	np_data* ndata;

	XP_ASSERT(app && cx);

    if (app == cx->pluginList)
        cx->pluginList = app->next;
    else
    {
        NPEmbeddedApp *ax;
        for (ax=cx->pluginList; ax; ax=ax->next)
            if (ax->next == app)
            {
                ax->next = ax->next->next;
                break;
            }
    }

	app->next = NULL;
	
    /* If there's an instance, clear the instance's context */
    ndata = (np_data*) app->np_data;
    if (ndata)
    {
    	np_instance* instance = (np_instance*) ndata->instance;
    	if (instance)
    		instance->cx = NULL;
    }
}


static void
np_delete_instance(np_instance *instance)
{
    if(instance)
    {
        np_handle *handle = instance->handle;
        np_stream *stream;

#ifdef JAVA		
		{
			/*
			** Break any association we have made between this instance and
			** its corresponding Java object. That way other java objects
			** still referring to it will be able to detect that the plugin
			** went away (by calling isActive).
			*/
			if (instance->javaInstance != NULL &&
				instance->javaInstance != NPN_NO_JAVA_INSTANCE)
			{
				/* Don't get the environment unless there is a Java instance */
 				JRIEnv* env = npn_getJavaEnv();
				jref javaInstance = JRI_GetGlobalRef(env, instance->javaInstance);
				netscape_plugin_Plugin_destroy(env, javaInstance);
				set_netscape_plugin_Plugin_peer(env, javaInstance, 0);
				JRI_DisposeGlobalRef(env, instance->javaInstance);
				instance->javaInstance = NULL;
			}
			if (handle && handle->f && handle->f->javaClass != NULL) {
				/* Don't get the environment unless there is a Java class */
 				JRIEnv* env = npn_getJavaEnv();
				JRI_DisposeGlobalRef(env, handle->f->javaClass);
				handle->f->javaClass = NULL;
			}
		}
#endif /* JAVA */

        /* nuke all open streams */
        for(stream=instance->streams; stream;)
        {
	    	np_stream *next = stream->next;
            stream->dontclose = 0;
            if (stream->nstream)
            {
            	/* Make sure the urls doesn't still point to us */
            	URL_Struct* urls = (URL_Struct*) stream->nstream->data_object;
            	if (urls)
            		urls->fe_data = NULL;
            }
            np_destroystream(stream, NPRES_USER_BREAK);
	    	stream = next;
        }
        instance->streams = 0;

        if(handle && handle->f && ISFUNCPTR(handle->f->destroy))
        {
            NPSavedData *save = NULL;

            CallNPP_DestroyProc(handle->f->destroy, instance->npp, &save);
            if(instance->app)
                if(instance->app->np_data)
                {
                    np_data* pnp = (np_data*)instance->app->np_data;
                    pnp->sdata = save;
                }
#ifdef XP_MAC
            /* turn scrollbars back on */
            if(instance->type == NP_FULL)
                FE_ShowScrollBars(instance->cx, TRUE);
#endif                
            /* remove it from the handle list */
            if(instance == handle->instances)
                handle->instances = instance->next;
            else
            {
                np_instance *ix;
                for(ix=handle->instances; ix; ix=ix->next)
                    if(ix->next == instance)
                    {
                        ix->next = ix->next->next;
                        break;
                    }
            }

            handle->refs--;
            XP_ASSERT(handle->refs>=0);
            if(!handle->refs)
            {
                FE_UnloadPlugin(handle->pdesc);
                handle->f = 0;
                handle->instances = 0;
            }
        }

		np_removeAllURLsFromList(instance);
		
        XP_FREE(instance);
    }
}

#ifdef XP_UNIX
static NET_StreamClass *
np_noembedfound (FO_Present_Types format_out, 
				 void *type, 
				 URL_Struct *urls, MWContext *cx)
{
	char *msg = PR_smprintf(XP_GetString(XP_PLUGIN_NOT_FOUND),
							urls->content_type);
	if(msg)
	  {
		FE_Alert(cx, msg);
		XP_FREE(msg);
	  }

	return(NULL);
}
#endif /* XP_UNIX */

void
NPL_RegisterDefaultConverters()
{
    /* get netlib to deal with our content streams */
    NET_RegisterContentTypeConverter("*", FO_CACHE_AND_EMBED, NULL, NET_CacheConverter);
    NET_RegisterContentTypeConverter("*", FO_CACHE_AND_PLUGIN, NULL, NET_CacheConverter);
    NET_RegisterContentTypeConverter("*", FO_CACHE_AND_BYTERANGE, NULL, NET_CacheConverter);

    NET_RegisterContentTypeConverter("multipart/x-byteranges", FO_CACHE_AND_BYTERANGE, NULL, CV_MakeMultipleDocumentStream);

    NET_RegisterContentTypeConverter("*", FO_PLUGIN, NULL, np_newpluginstream);
    NET_RegisterContentTypeConverter("*", FO_BYTERANGE, NULL, np_newbyterangestream);
#ifdef XP_UNIX
    NET_RegisterContentTypeConverter("*", FO_EMBED, NULL, np_noembedfound);
#endif /* XP_UNIX */
}

/* called from netscape main */
void
NPL_Init()
{

#if defined(XP_UNIX) && defined(DEBUG)
    {
        char *str;
        str = getenv("NPD");
        if(str)
            np_debug=atoi(str);
    }
#endif

    /* Register all default plugin converters. Do this before
	 * FE_RegisterPlugins() because this registers a not found converter
	 * for "*" and FE can override that with the nullplugin if one is
	 * available.
	 */
    NPL_RegisterDefaultConverters();

    /* call the platform specific FE code to enumerate and register plugins */
    FE_RegisterPlugins();

    /* construct the function table for calls back into netscape.
       no plugin sees this until its actually loaded */
    npp_funcs.size = sizeof(npp_funcs);
    npp_funcs.version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;

    npp_funcs.geturl = NewNPN_GetURLProc(npn_geturl);
    npp_funcs.posturl = NewNPN_PostURLProc(npn_posturl);
    npp_funcs.requestread = NewNPN_RequestReadProc(npn_requestread);
    npp_funcs.newstream = NewNPN_NewStreamProc(npn_newstream);
    npp_funcs.write = NewNPN_WriteProc(npn_write);
    npp_funcs.destroystream = NewNPN_DestroyStreamProc(npn_destroystream);
    npp_funcs.status = NewNPN_StatusProc(npn_status);
    npp_funcs.uagent = NewNPN_UserAgentProc(npn_useragent);
    npp_funcs.memalloc = NewNPN_MemAllocProc(npn_memalloc);
    npp_funcs.memfree = NewNPN_MemFreeProc(npn_memfree);
    npp_funcs.memflush = NewNPN_MemFlushProc(npn_memflush);
    npp_funcs.reloadplugins = NewNPN_ReloadPluginsProc(npn_reloadplugins);
    npp_funcs.getJavaEnv = NewNPN_GetJavaEnvProc(npn_getJavaEnv);
    npp_funcs.getJavaPeer = NewNPN_GetJavaPeerProc(npn_getJavaPeer);
    npp_funcs.geturlnotify = NewNPN_GetURLNotifyProc(npn_geturlnotify);
    npp_funcs.posturlnotify = NewNPN_PostURLNotifyProc(npn_posturlnotify);
#ifdef XP_UNIX
    npp_funcs.getvalue = NewNPN_GetValueProc(npn_getvalue);
#endif
}


void
NPL_Shutdown()
{
    np_handle *handle, *dh;
    np_instance *instance, *di;

    for(handle=np_plist; handle;)
    {
        dh=handle;
        handle = handle->next;

        /* delete handle */
        for(instance=dh->instances; instance;)
        {
            di = instance;
            instance=instance->next;
            np_delete_instance(di);
        }
    }
}



/*
 * Like NPL_SamePage, but for an individual element.
 * It is called by laytable.c when relaying out table cells. 
 */
void
NPL_SameElement(LO_EmbedStruct* embed_struct)
{
	if (embed_struct)
	{
		NPEmbeddedApp* app = (NPEmbeddedApp*) embed_struct->FE_Data;
		if (app)
		{
			np_data* ndata = (np_data*) app->np_data;
			XP_ASSERT(ndata);
			if (ndata && ndata->state != NPDataSaved)
				ndata->state = NPDataCache;
		}
	}
}



/*
 * This function is called by the FE's when they're resizing a page.
 * We take advantage of this information to mark all our instances
 * so we know not to delete them when the layout information is torn
 * down so we can keep using the same instances with the new, resized,
 * layout structures.  It probably goes without saying that this is
 * a sick hack.
 */
void
NPL_SamePage(MWContext* resizedContext)
{
	MWContext* cx;
	XP_List* children;
	NPEmbeddedApp* app;
	
	if (!resizedContext)
		return;
		
	/* Mark all plug-ins in this context */
	app = resizedContext->pluginList;
	while (app)
	{
		np_data* ndata = (np_data*) app->np_data;
		XP_ASSERT(ndata);
		if (ndata && ndata->state != NPDataSaved)
			ndata->state = NPDataCache;
		app = app->next;
	}
	
	/* Recursively traverse child contexts */
	children = resizedContext->grid_children;
	while ((cx = XP_ListNextObject(children)) != NULL)
		NPL_SamePage(cx);
}


int
NPL_HandleEvent(NPEmbeddedApp *app, void *event)
{
    if (app)
    {
        np_data *ndata = (np_data *)app->np_data;
        if(ndata && ndata->instance)
        {
            np_handle *handle = ndata->instance->handle;
            if(handle && handle->f && ISFUNCPTR(handle->f->event))
                return CallNPP_HandleEventProc(handle->f->event, ndata->instance->npp, event);
        }
    }
    return 0;
}

void
NPL_Print(NPEmbeddedApp *app, void *pdata)
{
    if (app)
    {
        np_data *ndata = (np_data *)app->np_data;
        if(ndata && ndata->instance)
        {
            np_handle *handle = ndata->instance->handle;
            if(handle && handle->f && ISFUNCPTR(handle->f->print))
                CallNPP_PrintProc(handle->f->print, ndata->instance->npp, pdata);
        }
    }
}


void
np_deleteapp(MWContext* cx, NPEmbeddedApp* app)
{
	if (app)
	{
		if (cx)
			np_unbindContext(app, cx);
		
    	XP_FREE(app);
	}
}


extern NPBool
NPL_EmbedDelete(MWContext* cx, LO_EmbedStruct* embed_struct)
{
	NPEmbeddedApp* app;
	np_data* ndata;
	
	if (!cx || !embed_struct || !embed_struct->FE_Data)
		return FALSE;
		
	app = (NPEmbeddedApp*) embed_struct->FE_Data;
    ndata = (np_data*) app->np_data;
	embed_struct->FE_Data = NULL;

    if (ndata)
    {
        embed_struct->session_data = (void*) ndata;
        	
        if (cx->type == MWContextPrint || cx->type == MWContextMetaFile ||
			cx->type == MWContextPostScript)
        {
        	/* When done printing, don't delete and don't save session data */
        	ndata->state = NPDataCached;
        	return FALSE;
   		}
   		else if (ndata->state == NPDataCache)
        {
        	XP_ASSERT(ndata->app);
            ndata->state = NPDataCached;
            ndata->lo_struct = NULL;     		/* Remove ref to layout structure since it's about to be deleted */
            np_unbindContext(app, cx);			/* Remove ref to context since it may change */
            return FALSE;
        }
        else if (ndata->instance)
        {
            np_delete_instance(ndata->instance);
            ndata->app = NULL;
            ndata->instance = NULL;
            ndata->lo_struct = NULL;
            ndata->streamStarted = FALSE;
            ndata->state = NPDataSaved; 		/* ndata gets freed later when history goes away */
        }
        else
        {
        	/* If there's no instance, there's no need to save session data */
        	embed_struct->session_data = NULL;
        	app->np_data = NULL;
        	XP_FREE(ndata);
        }
    }

	np_deleteapp(cx, app);						/* unlink app from context and delete app */
    return TRUE;
}



/*
 * Get all the embeds in this context to save themselves in the
 * designated saved data list so we can reuse them when printing.
 */
void
NPL_PreparePrint(MWContext* context, SHIST_SavedData* savedData)
{
	NPEmbeddedApp* app;
	
	XP_ASSERT(context && savedData);
	if (!context || !savedData)
		return;
	
	for (app = context->pluginList; app != NULL; app = app->next)
	{
		np_data* ndata = app->np_data;
		XP_ASSERT(ndata);
		if (ndata && ndata->lo_struct)
		{	
			XP_ASSERT(ndata->state == NPDataNormal);
			ndata->state = NPDataCached;
			LO_AddEmbedData(context, ndata->lo_struct, ndata);
		}	
	}
	
	LO_CopySavedEmbedData(context, savedData);
}



void
NPL_ReconnectPlugin(MWContext *context, NPEmbeddedApp *app)
{
	XP_ASSERT(context && app);
	if (context && app)
	{
		np_reconnect* reconnect = (np_reconnect*) context->pluginReconnect;
		if (reconnect)
			reconnect->app = app;
	}
}


/*
 * bing: This function should be moved to layembed.c so the TYPE
 * attribute is pulled out the same way as the SRC attribute.
 */
static char*
np_findTypeAttribute(LO_EmbedStruct* embed_struct)
{
	char* typeAttribute = NULL;
	int i;

	/* Look for the TYPE attribute */
	for (i = 0; i < embed_struct->attribute_cnt; i++)
	{
		if (XP_STRCASECMP(embed_struct->attribute_list[i], "TYPE") == 0)
		{
			typeAttribute = embed_struct->value_list[i];
			break;
		}
	}
	
	return typeAttribute;
}


NPEmbeddedApp*
NPL_EmbedCreate(MWContext* cx, LO_EmbedStruct* embed_struct)
{
    NPEmbeddedApp* app = NULL;
    np_data* ndata = NULL;
    
    XP_ASSERT(cx && embed_struct);
    if (!cx || !embed_struct)
    	goto error;
    	
    /*
     * Check the contents of the session data. If we have a cached
     * app in the session data, we can short-circuit this function
     * and just return the app we cached earlier.  If we have saved
     * data in the session data, keep that np_data object but 
     * attach it to a new app.  If there is nothing in the session
     * data, then we must create both a np_data object and an app.
     */
    if (embed_struct->session_data)
    {
        ndata = (np_data*) embed_struct->session_data;
        embed_struct->session_data = NULL;

        if (ndata->state == NPDataCached)			/* We cached this app, so don't create another */
        {
	        XP_ASSERT(ndata->app);
	        if (cx->type == MWContextPrint || cx->type == MWContextMetaFile ||
				cx->type == MWContextPostScript)
	        {
	        	if (ndata->app->pagePluginType == NP_FullPage)
	        	{
	        		np_reconnect* reconnect;
	        		if (!cx->pluginReconnect)
						cx->pluginReconnect = XP_NEW_ZAP(np_reconnect);
					reconnect = (np_reconnect*) cx->pluginReconnect;
					if (reconnect)
						reconnect->app = ndata->app;
	        	}
	        }
	        else
	        {
	   			ndata->lo_struct = embed_struct;		/* Set reference to new layout structure */
	   			np_bindContext(ndata->app, cx);			/* Set reference to (potentially) new context */
	            ndata->state = NPDataNormal;
            }
            
            /*
             * For full-page/frame plug-ins, make sure scroll
             * bars are off in the (potentially) new context.
             */
            if (ndata->app->pagePluginType == NP_FullPage)
            	FE_ShowScrollBars(cx, FALSE);
            	
	        return ndata->app;
	    }

	    XP_ASSERT(ndata->state == NPDataSaved);		/* If not cached, it's just saved data */
	    XP_ASSERT(ndata->app == NULL);
	    XP_ASSERT(ndata->instance == NULL);
	    XP_ASSERT(ndata->lo_struct == NULL);
	    XP_ASSERT(ndata->streamStarted == FALSE);
    }

	if (!ndata)
    {
        ndata = XP_NEW_ZAP(np_data);
        if (!ndata)
        	goto error;
    }
	ndata->state = NPDataNormal;
    ndata->lo_struct = embed_struct;


    /*
     * Create the NPEmbeddedApp and attach it to its context.
     */
    app = XP_NEW_ZAP(NPEmbeddedApp);
    if (!app)
    	goto error;
    app->np_data = (void*) ndata;
    app->type = NP_Untyped;
	ndata->app = app;
	np_bindContext(app, cx);
    
    return app;
	
error:	
	if (app)
		np_deleteapp(cx, app);			/* Unlink app from context and delete app */
	if (ndata)
		XP_FREE(ndata);
	return NULL;
}


NPError
NPL_EmbedStart(MWContext* cx, LO_EmbedStruct* embed_struct, NPEmbeddedApp* app)
{
	np_handle* handle;
	np_mimetype* mimetype;
	char* typeAttribute;
    np_data* ndata = NULL;
    
    if (!cx || !embed_struct || !app)
    	goto error;
    	
    ndata = (np_data*) app->np_data;
    if (!ndata)
    	goto error;
    
    /*
     * Don't do all the work in this function multiple times for the
     * same NPEmbeddedApp.  For example, we could be reusing this 
     * app in a new context (when resizing or printing) or just re-
     * creating the app multiple times (when laying out complex tables),
     * so we don't want to be creating another stream, etc. each
     * time. 
     */
    if (ndata->streamStarted)
    	return NPERR_NO_ERROR;
    ndata->streamStarted = TRUE;			/* Remember that we've been here */
    
	/*
	 * First check for a TYPE attribute.  The type specified
	 * will override the MIME type of the SRC (if present).
	 */
	typeAttribute = np_findTypeAttribute(embed_struct);
	if (typeAttribute)
	{
		/* Only embedded plug-ins can have a TYPE attribute */
		app->pagePluginType = NP_Embedded;

		/* Found the TYPE attribute, so look for a matching handler */
		mimetype = NULL;
		for (handle = np_plist; handle; handle = handle->next)
		{
			mimetype = np_getmimetype(handle, typeAttribute, FALSE);
			if (mimetype) break;
		}

		/* No handler with an exactly-matching name, so check for a wildcard */ 
		if (!mimetype) 
		{
			for (handle = np_plist; handle; handle = handle->next)
			{
				mimetype = np_getmimetype(handle, typeAttribute, TRUE);
				if (mimetype) break;
			}
		}

		/*
		 * If we found a handler, now we can create an instance.
		 * If we didn't find a handler, we have to use the SRC
		 * to determine the MIME type later (so there better be
		 * SRC, or it's an error).
		 */
		if (mimetype)
		{
			ndata->instance = np_newinstance(handle, cx, app, mimetype, typeAttribute);
			if (ndata->instance == NULL)
		        goto error;
		}
	}
	 
	/*
	 * Now check for the SRC attribute.
	 * - If it's full-page, create a instance now since we already 
	 *	 know the MIME type (NPL_NewStream has already happened).
	 * - If it's embedded, create a stream for the URL (we'll create 
	 *	 the instance when we get the stream in NPL_NewStream).
	 */
    if (embed_struct->embed_src)
    {
		char* theURL;
	    PA_LOCK(theURL, char*, embed_struct->embed_src);
	    XP_ASSERT(theURL);
	    if (XP_STRCMP(theURL, "internal-external-plugin") == 0)
	    {
	    	/*
	    	 * Full-page case: Stream already exists, so now
	    	 * we can create the instance.
	    	 */
	    	np_reconnect* reconnect;
	    	np_mimetype* mimetype;
	    	np_handle* handle;
	    	np_instance* instance;
	    	char* requestedtype;
	    	
	        app->pagePluginType = NP_FullPage;

			reconnect = (np_reconnect*) cx->pluginReconnect;
	    	XP_ASSERT(reconnect); 
			if (!reconnect)
			{
				PA_UNLOCK(embed_struct->embed_src); 
		        goto error;
			}
			
	    	mimetype = reconnect->mimetype;
	    	requestedtype = reconnect->requestedtype;
			handle = mimetype->handle;
			
			/* Now we can create the instance */
			XP_ASSERT(ndata->instance == NULL);
		    instance = np_newinstance(handle, cx, app, mimetype, requestedtype);
		    if (!instance)
		    {
	    		PA_UNLOCK(embed_struct->embed_src); 
		        goto error;
		    }
	    
			reconnect->app = app;
	        ndata->instance =  instance;
	        ndata->handle = handle;
	        ndata->instance->app = app;
	        FE_ShowScrollBars(cx, FALSE);
	    }
	    else
	    {
	    	/*
	    	 * Embedded case: Stream doesn't exist yet, so
	    	 * we need to create it before we can make the
	    	 * instance (exception: if there was a TYPE tag,
	    	 * we already know the MIME type so the instance
	    	 * already exists).
	    	 */
	        URL_Struct* pURL;    
	        app->pagePluginType = NP_Embedded;
	        pURL = NET_CreateURLStruct(theURL, NET_DONT_RELOAD);
	        pURL->fe_data = (void*) app;
	
	        /* start a stream */
	        (void) NET_GetURL(pURL, FO_CACHE_AND_EMBED, cx, NPL_EmbedURLExit);
	    }
	    
	    PA_UNLOCK(embed_struct->embed_src); 
	}
	
	return NPERR_NO_ERROR;

error:
	if (cx && app)
		np_deleteapp(cx, app);			/* Unlink app from context and delete app */
	if (ndata)
		XP_FREE(ndata);
	return NPERR_GENERIC_ERROR;
}


void
NPL_EmbedSize(NPEmbeddedApp *app)
{
    if(app)
    {
        np_data *ndata = (np_data *)app->np_data;
        if(ndata && ndata->instance && app->wdata)
            np_setwindow(ndata->instance, app->wdata);
    }
}


void
NPL_DeleteSessionData(MWContext* context, void* sdata)
{
	/*
	 * Don't delete the data if we're printing, since
	 * data really belongs to the original context (a
	 * MWContextBrowser).  A more generic way to make
	 * this check might be 'if (context != ndata->
	 * instance->cx)'.
	 */
    np_data* ndata = (np_data*) sdata;
    if (ndata == NULL ||
		context->type == MWContextPrint ||
		context->type == MWContextMetaFile ||
		context->type == MWContextPostScript)
    	return;
    	
    if (ndata->state == NPDataCached)           /* Resize case */
    {
		XP_ASSERT(FALSE);

    	/*
    	 * Reset the state; otherwise NPL_DeleteInstance
    	 * will refuse to delete it AGAIN when the FE calls it
    	 */
        ndata->state = NPDataNormal;

#if 0
        if (ndata->instance)
        {
            np_instance* instance = ndata->instance;
            MWContext* cx = instance->cx;
            NPEmbeddedApp* app = instance->app;
            if (cx && app)
                FE_FreeEmbedSessionData(cx, app);
        }
#endif
        
        /* Shouldn't have instance data when resizing */
    	XP_ASSERT(ndata->sdata == NULL);
    }
    else                             			/* Saved instance data case */
    {
    	XP_ASSERT(ndata->state == NPDataSaved);
    	XP_ASSERT(ndata->app == NULL);
    	XP_ASSERT(ndata->streamStarted == FALSE);
        if (ndata->sdata)
        {
        	if (ndata->sdata->buf)
        		XP_FREE(ndata->sdata->buf);
            XP_FREE(ndata->sdata);
            ndata->sdata = 0;
        }
    }

    ndata->handle = NULL;
    XP_FREE(ndata);
}


NPBool
NPL_IteratePluginFiles(NPReference* ref, char** name, char** filename, char** description)
{
	np_handle* handle;
	
	if (*ref == NPRefFromStart)
		handle = np_plist;
	else
		handle = ((np_handle*) *ref)->next;

	if (handle)
	{
		if (name)
			*name = handle->name;
		if (filename)
			*filename = handle->filename;
		if (description)
			*description = handle->description;
	}
	
	*ref = handle;
	return (handle != NULL);
}


NPBool
NPL_IteratePluginTypes(NPReference* ref, NPReference plugin, NPMIMEType* type, char*** extents,
		 char** description, void** fileType)
{
	np_handle* handle = (np_handle*) plugin;
	np_mimetype* mimetype;
	
	if (*ref == NPRefFromStart)
		mimetype = handle->mimetypes;
	else	
		mimetype = ((np_mimetype*) *ref)->next;
		
	if (mimetype)
	{
		if (type)
			*type = mimetype->type;
		if (description)
			*description = mimetype->fassoc->description;
		if (extents)
			*extents = mimetype->fassoc->extentlist;
		if (fileType)
			*fileType = mimetype->fassoc->fileType;
	}
	
	*ref = mimetype;
	return (mimetype != NULL);
	
}




/*
 * Returns a null-terminated array of plug-in names that support the specified MIME type.
 * This function is called by the FEs to implement their MIME handler controls (they need
 * to know which plug-ins can handle a particular type to build their popup menu).
 * The caller is responsible for deleting the strings and the array itself. 
 */
char**
NPL_FindPluginsForType(const char* typeToFind)
{
	char** result;
	uint32 count = 0;

	/* First count plug-ins that support this type */
	{
		NPReference plugin = NPRefFromStart;
		while (NPL_IteratePluginFiles(&plugin, NULL, NULL, NULL))
		{
			char* type;
			NPReference mimetype = NPRefFromStart;
			while (NPL_IteratePluginTypes(&mimetype, plugin, &type, NULL, NULL, NULL))
			{
				if (strcmp(type, typeToFind) == 0)
					count++;
			}
		}
	}

	/* Bail if no plug-ins match this type */
	if (count == 0)
		return NULL;
		
	/* Make an array big enough to hold the plug-ins */
	result = (char**) XP_ALLOC((count + 1) * sizeof(char*));
	if (!result)
		return NULL;

	/* Look for plug-ins that support this type and put them in the array */
	count = 0;
	{
		char* name;
		NPReference plugin = NPRefFromStart;
		while (NPL_IteratePluginFiles(&plugin, &name, NULL, NULL))
		{
			char* type;
			NPReference mimetype = NPRefFromStart;
			while (NPL_IteratePluginTypes(&mimetype, plugin, &type, NULL, NULL, NULL))
			{
				if (strcmp(type, typeToFind) == 0)
					result[count++] = XP_STRDUP(name);
			}
		}
	}
	
	/* Null-terminate the array and return it */
	result[count] = NULL;
	return result;
}


/*
 * Returns the name of the plug-in enabled for the 
 * specified type, of NULL if no plug-in is enabled.
 * The caller is responsible for deleting the string. 
 */
char*
NPL_FindPluginEnabledForType(const char* typeToFind)
{
	np_handle* handle = np_plist;
	while (handle)
	{
		np_mimetype* mimetype = handle->mimetypes;
		while (mimetype)
		{
			if ((strcmp(mimetype->type, typeToFind) == 0) && mimetype->enabled)
				return XP_STRDUP(handle->name);
			mimetype = mimetype->next;
		}
		
		handle = handle->next;
	}
	
	return NULL;
}



#if !defined(XP_MAC) && !defined(XP_UNIX)		/* plugins change */

/* put_block modifies input buffer. We cannot pass static data to it.
 * And hence the strdup.
 */
#define PUT(string) if(string) { \
	char *s = XP_STRDUP(string); \
	int ret; \
	ret = (*stream->put_block)(stream->data_object,s, XP_STRLEN(s)); \
   	XP_FREE(s); \
	if (ret < 0) \
		return; \
}

void
NPL_DisplayPluginsAsHTML(FO_Present_Types format_out, URL_Struct *urls, MWContext *cx)
{
    NET_StreamClass * stream;
    np_handle *handle;

    StrAllocCopy(urls->content_type, TEXT_HTML);
    format_out = CLEAR_CACHE_BIT(format_out);

    stream = NET_StreamBuilder(format_out, urls, cx);
    if (!stream)
        return;

	if (np_plist)
	{
		PUT("<b><font size=+3>Installed plug-ins</font></b><br>");
	}
	else
	{
		PUT("<b><font size=+2>No plug-ins are installed.</font></b><br>");
	}
	
	PUT("For more information on plug-ins, <A HREF=http://home.netscape.com/comprod/products/navigator/version_2.0/plugins/index.html>click here</A>.<p><hr>");
	
    for (handle = np_plist; handle; handle = handle->next)
    {
    	np_mimetype* mimetype;
    	
    	PUT("<b><font size=+2>");
    	PUT(handle->name);
    	PUT("</font></b><br>");

		PUT("File name: ");
		PUT(handle->filename);
		PUT("<br>");
		
		PUT("MIME types: <br>");
		PUT("<ul>");
    	for (mimetype = handle->mimetypes; mimetype; mimetype = mimetype->next)
    	{
    		int index = 0;
    		char** extents = mimetype->fassoc->extentlist;
    		
    		PUT("Type: ");
	        PUT(mimetype->type);
	        if (!mimetype->enabled)
	        	PUT(" (disabled)");
	        PUT("<br>");
	        
	        PUT("File extensions: ");
	        while (extents[index])
	        {
	        	PUT(extents[index]);
	        	index++;
	        	if (extents[index])
	        		PUT(", ");
	        }
	        PUT("<br>");
	    }
		PUT("</ul>");
	    
        PUT("<p>");
        PUT("<hr>");
    }
    
    (*stream->complete)(stream->data_object);
}

#endif
