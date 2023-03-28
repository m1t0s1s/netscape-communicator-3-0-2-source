/* -*- Mode: C; tab-width: 4 -*- */

/* Please leave outside of ifdef for windows precompiled headers */
#include "mkutils.h"

#ifdef MOZILLA_CLIENT

#include "mkcache.h"
#include "glhist.h"
#include "xp_hash.h"
#include "xp_mcom.h"
#include "client.h"
#include "mkgeturl.h"
#include "mkstream.h"
#include "extcache.h"
#include "mkmemcac.h"
#include "il.h"
#include "cert.h" /* for CERT_DupCertificate() */
#include "prclist.h"

/* exported error ints */
extern int MK_OUT_OF_MEMORY;

/* the size of the cache segment output size
 */
#define NET_MEM_CACHE_OUTPUT_SIZE 2048

/* MAX_MEMORY_ALLOC_SIZE is the size of the largest malloc
 * used in this module
 * 
 * the NET_Socket_Buffer can never be larger than the
 * MAX_MEMORY_ALLOC_SIZE or this will fail
 */
#ifdef XP_WIN16
#define MAX_MEMORY_ALLOC_SIZE 32767 /* MAXINT */
#elif defined(XP_MAC)
#define MAX_MEMORY_ALLOC_SIZE 12*1024
#else
#define MAX_MEMORY_ALLOC_SIZE (((unsigned) (~0) << 1) >> 1)  /* MAXINT */

/* @@@@@@@@@ there is currently a limit on XP_ALLOC to less than 32 K */
#undef  MAX_MEMORY_ALLOC_SIZE
#define MAX_MEMORY_ALLOC_SIZE 32767 /* MAXINT */
#endif

/* this is the minimum size of each of the memory segments used to hold
 * the cache object in memory
 */
#ifdef XP_MAC
#define MEMORY_CACHE_SEGMENT_SIZE 12*1024
#else
#define MEMORY_CACHE_SEGMENT_SIZE 2048
#endif

/* !!! structure is typedef'd to net_MemoryCacheObject
 * in net.h
 */
struct _net_MemoryCacheObject {
	XP_List         *list;
	net_CacheObject  cache_obj;
	int              external_locks;       /* locks set by other modules calling in */
	int              mem_read_lock;        /* the number of current readers */
    Bool             delete_me;            /* set this to delete the object 
									        * once all readers are done 
									        */
	Bool             current_page_only;    /* this is set when the document
											* requested not to be cached.
											* When set the document should
											* only be served if it's a
											* resize or print
											*/
};

/* structure to hold memory
 * segments
 */
typedef struct _net_MemorySegment {
	char      *segment;
	uint32     seg_size;
	uint32     in_use;
} net_MemorySegment;

/* the hash table pointer that holds all the memory cache
 * objects for quick lookup
 */
PRIVATE XP_HashList * net_MemoryCacheHashList = 0;

/* semaphore counter set when calling any of the list add functions
 */
PRIVATE int net_cache_adding_object=0;

/* a list of all the documents currently in
 * memory
 *
 * The net_MemoryCacheList is used as a delete queue
 * and is ordered by last-accessed time.
 */
PRIVATE XP_List   *net_MemoryCacheList=0;
PRIVATE uint32     net_MemoryCacheSize=0;
PRIVATE uint32     net_MaxMemoryCacheSize=0;

/* this object is used by the MemCacheConverter.
 * MemCacheConverter is a standard netlib stream and uses
 * this structure to hold data between invokations of
 * the write function and complete or abort.
 */
typedef struct _CacheDataObject {
	PRCList               links;
    NET_StreamClass       *next_stream;
    net_MemoryCacheObject *memory_copy;
	URL_Struct            *URL_s;
	uint32            	   computed_content_length;
} CacheDataObject;

PRIVATE PRCList active_cache_data_objects
	= PR_INIT_STATIC_CLIST(&active_cache_data_objects);

/* free a segmented memory copy of an object
 * and the included net_cacheObject struct
 */
PRIVATE void
net_FreeMemoryCopy(net_MemoryCacheObject * mem_copy)
{
    net_MemorySegment * mem_seg;

	XP_ASSERT(mem_copy);

	if(!mem_copy)
		return;

	/* if this object is currently being read DO NOT delete it.
	 * It will be deleted by another call to this function
	 * after it is finished being read.
	 */
	if(mem_copy->mem_read_lock > 0)
	  {
		TRACEMSG(("Seting delete_me flag on memory cache object"));
		mem_copy->delete_me = TRUE;

		/* remove it from the hash list so it won't be found
		 * by NET_GetURLInMemCache
		 */
		XP_HashListRemoveObject(net_MemoryCacheHashList, mem_copy);
		return;
	  }

	TRACEMSG(("Freeing memory cache copy"));

	/* free and delete the memory segment list
	 */
    while((mem_seg = (net_MemorySegment *)XP_ListRemoveTopObject(mem_copy->list)))
      {
		/* reduce the global memory cache size
		 */
		net_MemoryCacheSize -= mem_seg->seg_size;
        FREE(mem_seg->segment);
        FREE(mem_seg);
      }

    XP_ListDestroy(mem_copy->list);

	/* Remove it from the hash and delete lists,
	 * If the object isn't in these lists anymore
	 * the call will be ignored.
	 */
	XP_HashListRemoveObject(net_MemoryCacheHashList, mem_copy);
	XP_ListRemoveObject(net_MemoryCacheList, mem_copy);

    FREEIF(mem_copy->cache_obj.address);
    FREEIF(mem_copy->cache_obj.post_data);
    FREEIF(mem_copy->cache_obj.post_headers);
    FREEIF(mem_copy->cache_obj.content_type);
    FREEIF(mem_copy->cache_obj.charset);
    FREEIF(mem_copy->cache_obj.content_encoding);

	FREE(mem_copy);

}

/* removes the last mem object.  Returns negative on error,
 * otherwise returns the current size of the cache
 */
PRIVATE int32
net_remove_last_memory_cache_object(void)
{
	net_MemoryCacheObject * mem_cache_obj;
	
	/* safty valve in case there are no list items 
	 * or if we are in the process of adding an object
	 */
    if(net_cache_adding_object)
        return -1;

    mem_cache_obj = (net_MemoryCacheObject *) 
								XP_ListRemoveEndObject(net_MemoryCacheList);

    if(!mem_cache_obj)
        return -1;

	/* we can't remove it if we have external locks, try removing another one */
	if(mem_cache_obj->external_locks)
	  {
		int status = net_remove_last_memory_cache_object(); /* recurse */
		/* add the object back in */
		XP_ListAddObject(net_MemoryCacheList, mem_cache_obj);
		return status;
	  }

    net_FreeMemoryCopy(mem_cache_obj);

	return((int32) net_MemoryCacheSize);
}

/* this function free's objects to bring the cache size
 * below the size passed in
 */
PRIVATE void
net_ReduceMemoryCacheTo(uint32 size)
{
    /* safty valve in case we are in the process of adding an object
     */
    if(net_cache_adding_object)
        return;

	while(net_MemoryCacheSize > size)
	  {
		if(0 > net_remove_last_memory_cache_object())
			break;
	  }
}

/* set the size of the Memory cache.
 * Set it to 0 if you want cacheing turned off
 * completely
 */
#define HTML_CACHE_MAX_BASE_SIZE 1024l*1024l
#define HTML_CACHE_VARIABLE_GROWTH .05   /* the percentage to add */
PUBLIC void
NET_SetMemoryCacheSize(int32 new_size)
{
	int32 image_cache_size;
	int32 html_cache_size;

	if(new_size <= 0)
	  {
		IL_ReduceImageCacheSizeTo(0);
    	IL_SetImageCacheSize(0);
    	net_ReduceMemoryCacheTo(0);
		net_MaxMemoryCacheSize = 0;
		return;
	  }

	/* the netlib mem cache is set to .3*new_size 
	 * or 200K + .05 * new_size, whichever is less.
	 * and imagelb uses the rest 
	 */
	if(new_size * .3 < HTML_CACHE_MAX_BASE_SIZE)
	  {
		/* use .3 of new_size */
		html_cache_size = .3 * new_size;
	  }
	else
	  {
		/* use HTML_CACHE_MAX_BASE_SIZE + .05*new_size */
		html_cache_size = HTML_CACHE_MAX_BASE_SIZE + 
						  (HTML_CACHE_VARIABLE_GROWTH*
						   (new_size - HTML_CACHE_MAX_BASE_SIZE));
	  }
	
	image_cache_size = new_size - html_cache_size;

	net_MaxMemoryCacheSize = html_cache_size;
	
    net_ReduceMemoryCacheTo((uint32) html_cache_size);

	/* set the image cache to be the rest
	 */
    IL_SetImageCacheSize(image_cache_size);
	IL_ReduceImageCacheSizeTo(image_cache_size);

	return;
}

/* Remove the last memory cache object if one exists
 * Returns the total size of the memory cache in bytes
 * after performing the operation
 */
PUBLIC int32
NET_RemoveLastMemoryCacheObject()
{
	net_remove_last_memory_cache_object();

	return(net_MemoryCacheSize);
}

/* returns the number of bytes currently in use by the Memory cache
 */
PUBLIC int32
NET_GetMemoryCacheSize()
{
    return(net_MemoryCacheSize);
}

PUBLIC int32
NET_GetMaxMemoryCacheSize()
{
    return net_MaxMemoryCacheSize;
}

/* compare entries for the hashing functions
 *
 * return 0 if match or positive or negative on
 * non match
 */
PRIVATE int net_CacheHashComp(net_MemoryCacheObject * obj1,
                                 net_MemoryCacheObject * obj2)
{
    int result;
	char *hash1;
	char *hash2;
	char *ques1=0;
	char *ques2=0;

    if(obj1->cache_obj.method != obj2->cache_obj.method)
        return(obj1->cache_obj.method - obj2->cache_obj.method);

	/* If this is a "news:" or "snews:" URL, then any search data in the URL
	 * (the stuff after '?') should be ignored for cache-comparison purposes,
	 * so that "news:MSGID" and "news:MSGID?headers=all" share the same cache
	 * file.
	 */
	if((XP_TO_UPPER(obj1->cache_obj.address[0]) == 'N'
		|| XP_TO_UPPER(obj1->cache_obj.address[0]) == 'S')
	    &&  NET_URL_Type(obj1->cache_obj.address) == NEWS_TYPE_URL)
	  {
		ques1 = XP_STRCHR(obj1->cache_obj.address, '?');
		if(ques1) 
			*ques1 = '\0';
	  }

    if((XP_TO_UPPER(obj2->cache_obj.address[0]) == 'N'
        || XP_TO_UPPER(obj2->cache_obj.address[0]) == 'S')
        &&  NET_URL_Type(obj2->cache_obj.address) == NEWS_TYPE_URL)
      {
        ques2 = XP_STRCHR(obj2->cache_obj.address, '?');
        if(ques2)
            *ques2 = '\0';
      }

	/* strip hash symbols because they
	 * really represent the same
	 * document
	 */
	hash1 = XP_STRCHR(obj1->cache_obj.address, '#');
	hash2 = XP_STRCHR(obj2->cache_obj.address, '#');

	if(hash1)
		*hash1 = '\0';
	if(hash2)
		*hash2 = '\0';

    result = XP_STRCMP(obj1->cache_obj.address, obj2->cache_obj.address);

	/* set them back to previous values
	 */
	if(hash1)
		*hash1 = '#';
	if(hash2)
		*hash2 = '#';
	if(ques1)
		*ques1 = '?';
	if(ques2)
		*ques2 = '?';


    if(result != 0)
       return(result);

    if(!obj1->cache_obj.post_data && !obj2->cache_obj.post_data)
       return(0);  /* match; no post data on either */

    if(obj1->cache_obj.post_data && obj2->cache_obj.post_data)
      {
        result = XP_STRCMP(obj1->cache_obj.post_data, 
						   obj2->cache_obj.post_data);
        return(result);
      }
    else
      {  /* one but not the other */
        if(obj1->cache_obj.post_data)
           return(1);
        else
           return(-1);
      }

    return(0);  /* match with post data */
}

/* hashing function for url's
 *
 * This is some magic Jamie gave me...
 */
PRIVATE uint32 net_CacheHashFunc(net_MemoryCacheObject * obj1)
{
    unsigned const char *x = (unsigned const char *) obj1->cache_obj.address;
    uint32 h = 0;
    uint32 g;
	XP_Bool news_type_url = FALSE;
 
	/* figure out if it's a news type URL */
    if((XP_TO_UPPER(obj1->cache_obj.address[0]) == 'N'
        || XP_TO_UPPER(obj1->cache_obj.address[0]) == 'S')
        &&  NET_URL_Type(obj1->cache_obj.address) == NEWS_TYPE_URL)
		news_type_url = TRUE;

    /* modify the default String hash function
     * to work with URL's
     */
    assert(x);

    if (!x) return 0;

	/* ignore '#' data for all URL's.
 	 * ignore '?' data for news URL's
	 */
    while (*x != 0 && *x != '#' && (!news_type_url || *x != '?'))
      {
        h = (h << 4) + *x++;
        if ((g = h & 0xf0000000) != 0)
          h = (h ^ (g >> 24)) ^ g;
      }
 
    return h;
}

/******************************************************************
 * Cache Converter Stream input routines
 */

/* is the stream ready for writing?
 */
PRIVATE unsigned int net_MemCacheWriteReady (CacheDataObject * obj)
{
   if(obj->next_stream)
       return((*obj->next_stream->is_write_ready)(obj->next_stream->data_object));
   else
	   return(MAX_WRITE_READY);
}

/*  stream write function
 *
 * this function accepts a stream and writes the data
 * into memory segments on a segment list.
 */
PRIVATE int net_MemCacheWrite (CacheDataObject * obj, CONST char* buffer, int32 len)
{
	TRACEMSG(("net_MemCacheWrite called with %ld buffer size", len));

	if(obj->memory_copy)
	  {
		net_MemorySegment * mem_seg;
		char * cur_mem_seg_ptr;

		/* compute a content_length as we read data 
		 * to use as a comparison and for URL's that
		 * don't send a content type
		 */
		obj->computed_content_length += len;

		/* we are always adding to the end one in the list so 
		 * lets get it
		 */
		mem_seg = (net_MemorySegment *)
								XP_ListGetEndObject(obj->memory_copy->list);
        XP_ASSERT(mem_seg);
	
		if(mem_seg->in_use+len > mem_seg->seg_size)
		  {
			/* we need a new segment
			 */
		    net_MemorySegment * new_mem_seg = XP_NEW(net_MemorySegment);
		    uint32 size_left_in_old_buffer = mem_seg->seg_size - mem_seg->in_use;
			uint32 size_for_new_buffer = len - size_left_in_old_buffer;
			char * new_mem_seg_ptr;
		
			if(!new_mem_seg)
			  {
				net_FreeMemoryCopy(obj->memory_copy);
				obj->memory_copy = 0;
				goto EndOfMemWrite;
			  }

			/* @@@ the socket buffer can never be larger
			 * than the MAX_MEMORY_ALLOC_SIZE
			 */
			if(size_for_new_buffer > MEMORY_CACHE_SEGMENT_SIZE)
			  {
			    new_mem_seg->segment = (char*)XP_ALLOC(size_for_new_buffer);
			    new_mem_seg->seg_size = size_for_new_buffer;
			  }
			else
			  {
			    new_mem_seg->segment = (char*)XP_ALLOC(
													MEMORY_CACHE_SEGMENT_SIZE);
			    new_mem_seg->seg_size = MEMORY_CACHE_SEGMENT_SIZE;
			  }

			if(!new_mem_seg->segment)
			  {
				FREE(new_mem_seg);
				net_FreeMemoryCopy(obj->memory_copy);
				obj->memory_copy = 0;
				goto EndOfMemWrite;
			  }

			/* increase the global cache size counter
			 */
		    net_MemoryCacheSize += new_mem_seg->seg_size;

			TRACEMSG(("Cache size now: %d", net_MemoryCacheSize));

			cur_mem_seg_ptr = mem_seg->segment;
			new_mem_seg_ptr = new_mem_seg->segment;

			/* fill in what we can to the old buffer
			 */
			if(size_left_in_old_buffer)
			  {
			    XP_MEMCPY(cur_mem_seg_ptr+mem_seg->in_use, 
					      buffer, 
					      (size_t) size_left_in_old_buffer);
			    mem_seg->in_use = mem_seg->seg_size;  /* seg new size */
			  }

			/* fill in the new buffer
			 */
			XP_MEMCPY(new_mem_seg_ptr, 
					  buffer+size_left_in_old_buffer, 
					  (size_t) size_for_new_buffer);
			new_mem_seg->in_use = size_for_new_buffer;

			net_cache_adding_object++; /* semaphore */
			XP_ListAddObjectToEnd(obj->memory_copy->list, new_mem_seg);
			net_cache_adding_object--; /* semaphore */

			TRACEMSG(("Adding %d to New memory segment %p", len, new_mem_seg));
		  }
		else
		  {
		  	assert (mem_seg->segment);
			cur_mem_seg_ptr = mem_seg->segment;

			/* fill in some more into the existing segment
			 */
			XP_MEMCPY(cur_mem_seg_ptr + mem_seg->in_use, buffer, (size_t) len);
			mem_seg->in_use += len;

			TRACEMSG(("Adding %d to existing memory segment %p", len, mem_seg));
		  }

	  }

EndOfMemWrite:  /* target of a goto from an error above */

    if(obj->next_stream)
	  {
    	int status=0;
	
		XP_ASSERT (buffer);
		XP_ASSERT (len > -1);
        status = (*obj->next_stream->put_block)
									(obj->next_stream->data_object, buffer, len);

		/* abort */
	    if(status < 0)
			return(status);
	  }

    return(1);
}

/*  complete the stream
*/
PRIVATE void net_MemCacheComplete (CacheDataObject * obj)
{
    int status;

	/* refresh these entries in case meta changes them */
	if(obj->URL_s && obj->memory_copy)
	  {
		obj->memory_copy->cache_obj.expires       = obj->URL_s->expires;
    	obj->memory_copy->cache_obj.last_modified = obj->URL_s->last_modified;
    	StrAllocCopy(obj->memory_copy->cache_obj.charset, obj->URL_s->charset);
	  }

	/* if the hash table doesn't exist yet, initialize it now 
	 */
    if(!net_MemoryCacheHashList)
      {
 
        net_MemoryCacheHashList = XP_HashListNew(499, 
									(XP_HashingFunction) net_CacheHashFunc,
                                    (XP_HashCompFunction) net_CacheHashComp); 
        if(!net_MemoryCacheHashList)
          {
			net_FreeMemoryCopy(obj->memory_copy);
			goto loser;
            return;
          }

      }

	/* if the delete list doesn't exist yet, initialize it now 
	 */
	if(!net_MemoryCacheList)
	  {
		net_MemoryCacheList = XP_ListNew();
		if(!net_MemoryCacheList)
		  {
			net_FreeMemoryCopy(obj->memory_copy);
			goto loser;
			return;
		  }
	  }

	/* memory copy could have been free'd and clear if an
 	 * error occured in the write, so check to make sure
	 * it's still around.
	 */
    if(obj->memory_copy)
      {
		/* if the object is zero size or 
	 	 * if the computed size is different that a given content type,
	 	 * zero the last modified date so that it wont get reused
	 	 * on a recheck or reload
	 	 */
		if(!obj->computed_content_length
		   ||  (obj->memory_copy->cache_obj.content_length
				&& obj->memory_copy->cache_obj.content_length 
				   != obj->computed_content_length
			   ) 
		  )
	  	  {
			obj->memory_copy->cache_obj.last_modified = 0;
	  	  }

		/* override the given content_length with the correct
		 * content length
		 */
		obj->memory_copy->cache_obj.content_length = 
												obj->computed_content_length;
	
 		/* set current time */
		obj->memory_copy->cache_obj.last_accessed = time(NULL); 

		/* if the document requested not to be cached, 
		 * set the flag to only serve it for reloads and such
		 */
		if(obj->URL_s && obj->URL_s->dont_cache)
			obj->memory_copy->current_page_only = TRUE;
			
		/* add the struct to the delete list */
		net_cache_adding_object++; /* semaphore */
        XP_ListAddObject(net_MemoryCacheList, obj->memory_copy);
		net_cache_adding_object--; /* semaphore */

		/* add the struct to the hash list */
		net_cache_adding_object++; /* semaphore */
        status = XP_HashListAddObject(net_MemoryCacheHashList, 
									  obj->memory_copy);
		net_cache_adding_object--; /* semaphore */

		/* check for hash collision */
		if(status == XP_HASH_DUPLICATE_OBJECT)
		  {
			net_MemoryCacheObject * tmp_obj;

            tmp_obj = (net_MemoryCacheObject *)
                          		XP_HashListFindObject(net_MemoryCacheHashList, 
													  obj->memory_copy);

			/* this will remove it from the hash list too
			 */
			net_FreeMemoryCopy(tmp_obj);
   
		    TRACEMSG(("Found duplicate object in cache.  Removing old object"));

		    /* re add the object 
			 */
			net_cache_adding_object++; /* semaphore */
		    XP_HashListAddObject(net_MemoryCacheHashList, 
								 obj->memory_copy);
			net_cache_adding_object--; /* semaphore */
		  }
      }

loser:

    /* complete the next stream
     */
    if(obj->next_stream)
	  {
	    (*obj->next_stream->complete)(obj->next_stream->data_object);
        FREE(obj->next_stream);
	  }

	/* don't need the data obj any more since this is the end 
	 * of the stream... */
	PR_REMOVE_LINK(&obj->links);
    FREE(obj);

	/* this is what keeps the cache from growing without end */
	net_ReduceMemoryCacheTo(net_MaxMemoryCacheSize);

    return;
}

/* Abort the stream 
 * 
 */
PRIVATE void net_MemCacheAbort (CacheDataObject * obj, int status)
{

    /* abort the next stream
     */
    if(obj->next_stream)
	  {
	    (*obj->next_stream->abort)(obj->next_stream->data_object, status);
        FREE(obj->next_stream);
	  }

	net_FreeMemoryCopy(obj->memory_copy);

	PR_REMOVE_LINK(&obj->links);
    FREE(obj);

    return;
}

/* setup the stream
 */
MODULE_PRIVATE NET_StreamClass * 
NET_MemCacheConverter (FO_Present_Types format_out,
                    void       *converter_obj,
                    URL_Struct *URL_s,
                    MWContext  *window_id)
{
    CacheDataObject * data_object=0;
    NET_StreamClass * stream=0;
	net_MemorySegment * mem_seg=0;
	net_MemoryCacheObject *memory_copy=0;
	net_MemCacheConverterObject *mem_conv_obj = converter_obj;
    
    TRACEMSG(("Setting up cache stream. Have URL: %s\n", URL_s->address));

	TRACEMSG(("Setting up memory cache"));

	/* check to see if the object is larger than the
	 * size of the max memory cache 
	 *
	 * skip the size check if the must_cache flag is set.
	 */
	if(!URL_s->must_cache && URL_s->content_length >= net_MaxMemoryCacheSize)
		goto malloc_failure; /* dont cache this URL */

	/* set up the memory caching struct.
 	 *
	 * malloc the segment first since it's the most likely malloc to fail
	 */
	mem_seg = XP_NEW(net_MemorySegment);

	if(!mem_seg)
		return(NULL);

	mem_seg->in_use = 0;

	if(URL_s->content_length > 0)
	  {
		/* use the content_length if we can since that would be
		 * the most efficient 
		 */
		if(URL_s->content_length > MAX_MEMORY_ALLOC_SIZE)
		  {
		    mem_seg->segment =  (char*)XP_ALLOC(MAX_MEMORY_ALLOC_SIZE);
	        mem_seg->seg_size = MAX_MEMORY_ALLOC_SIZE;
		  }
		else
		  {
		    /* add 10 just in case it's needed due to a bad content type :) */
		    mem_seg->segment =  (char*)XP_ALLOC(URL_s->content_length+10);
	        mem_seg->seg_size = URL_s->content_length+10;
		  }
	  }
	else
	  {
		/* this is a case of no content length, use standard size segments */
		mem_seg->segment  = (char*)XP_ALLOC(MEMORY_CACHE_SEGMENT_SIZE);
	    mem_seg->seg_size = MEMORY_CACHE_SEGMENT_SIZE;
	  }

	if(!mem_seg->segment)
	  {
		FREE(mem_seg);
		goto malloc_failure;  /* skip memory cacheing */
	  }

	/* malloc the main cache holding structure */
	memory_copy = XP_NEW(net_MemoryCacheObject);
	if(!memory_copy)
      {
		FREE(mem_seg);
		FREE(mem_seg->segment);
		goto malloc_failure;  /* skip memory cacheing */
      }
	memset(memory_copy, 0, sizeof(net_MemoryCacheObject));

	memory_copy->list = XP_ListNew();
	if(!memory_copy->list)
	  {
		FREE(mem_seg);
		FREE(mem_seg->segment);
		FREE(memory_copy);
		goto malloc_failure;  /* skip memory cacheing */
	  }

	/* if we get this far add the object to the size count
	 * since if we fail anywhere below here it will get
	 * subtracted by net_FreeMemoryCopy(memory_copy);
	 */
    net_MemoryCacheSize += mem_seg->seg_size;

	/* add the segment malloced above to the segment list */
	net_cache_adding_object++; /* semaphore */
	XP_ListAddObject(memory_copy->list, mem_seg);
	net_cache_adding_object--; /* semaphore */

    StrAllocCopy(memory_copy->cache_obj.address, URL_s->address);
    memory_copy->cache_obj.method = URL_s->method;
    if(URL_s->post_data)
      {
		memory_copy->cache_obj.post_data_size = URL_s->post_data_size;
        StrAllocCopy(memory_copy->cache_obj.post_data, URL_s->post_data);
        StrAllocCopy(memory_copy->cache_obj.post_headers, URL_s->post_headers);
      }

    memory_copy->cache_obj.expires        = URL_s->expires;
    memory_copy->cache_obj.last_modified  = URL_s->last_modified;
   
    memory_copy->cache_obj.content_length      = URL_s->content_length;
    memory_copy->cache_obj.real_content_length = URL_s->content_length;
    memory_copy->cache_obj.is_netsite     = URL_s->is_netsite;

    StrAllocCopy(memory_copy->cache_obj.content_type, URL_s->content_type);
    StrAllocCopy(memory_copy->cache_obj.charset, URL_s->charset);
    StrAllocCopy(memory_copy->cache_obj.content_encoding, 
				 URL_s->content_encoding);

	/* check for malloc failure on critical fields above */
	if(!memory_copy->cache_obj.address || !memory_copy->cache_obj.content_type)
	  {
		net_FreeMemoryCopy(memory_copy);
		goto malloc_failure;  /* skip memory cacheing */
	  }
    
	/* copy security info */
	memory_copy->cache_obj.security_on             = URL_s->security_on;
    StrAllocCopy(memory_copy->cache_obj.key_cipher,  URL_s->key_cipher);
	memory_copy->cache_obj.key_size                = URL_s->key_size;
	memory_copy->cache_obj.key_secret_size         = URL_s->key_secret_size;
#ifdef HAVE_SECURITY /* added by jwz */
	if (URL_s->certificate)
		memory_copy->cache_obj.certificate = CERT_DupCertificate(URL_s->certificate);
#endif /* !HAVE_SECURITY -- added by jwz */

	/* build the stream object */
    stream = XP_NEW(NET_StreamClass);
    if(!stream)
	  {
		net_FreeMemoryCopy(memory_copy);
		goto malloc_failure;  /* skip memory cacheing */
	  }

	/* this structure gets passed back into the write, complete
	 * and abort stream functions
	 */
    data_object = XP_NEW(CacheDataObject);
    if (!data_object)
      {
        FREE(stream);
		net_FreeMemoryCopy(memory_copy);
		goto malloc_failure;  /* skip memory cacheing */
      }

    /* init the object */
    XP_MEMSET(data_object, 0, sizeof(CacheDataObject));
    
	/* assign the cache object to the stream data object
	 */
    data_object->memory_copy = memory_copy;

	if (mem_conv_obj->dont_hold_URL_s == FALSE)
		data_object->URL_s = URL_s;

    TRACEMSG(("Returning stream from NET_CacheConverter\n"));

    stream->name           = "Cache stream";
    stream->complete       = (MKStreamCompleteFunc) net_MemCacheComplete;
    stream->abort          = (MKStreamAbortFunc) net_MemCacheAbort;
    stream->put_block      = (MKStreamWriteFunc) net_MemCacheWrite;
    stream->is_write_ready = (MKStreamWriteReadyFunc) net_MemCacheWriteReady;
    stream->data_object    = data_object;  /* document info object */
    stream->window_id      = window_id;

	/* next stream is passed in by the caller but can be null */
    data_object->next_stream = mem_conv_obj->next_stream;

	PR_APPEND_LINK(&data_object->links, &active_cache_data_objects);
    return stream;

malloc_failure: /* target of malloc failure */

	TRACEMSG(("NOT Caching this URL"));

	/* bypass the whole cache mechanism
	 */
	if(format_out != FO_CACHE_ONLY)
      {
        format_out = CLEAR_CACHE_BIT(format_out);
        return(mem_conv_obj->next_stream);
	  }

	return(NULL);
}

PRIVATE net_MemoryCacheObject *
net_FindObjectInMemoryCache(URL_Struct *URL_s)
{
	net_MemoryCacheObject tmp_cache_obj;

	/* fill in the temporary cache object so we can
	 * use it for searching
	 */
	XP_MEMSET(&tmp_cache_obj, 0, sizeof(net_CacheObject));
	tmp_cache_obj.cache_obj.method    = URL_s->method;
	tmp_cache_obj.cache_obj.address   = URL_s->address;
	tmp_cache_obj.cache_obj.post_data = URL_s->post_data;
	tmp_cache_obj.cache_obj.post_data_size = URL_s->post_data_size;

	return((net_MemoryCacheObject *)
           XP_HashListFindObject(net_MemoryCacheHashList, &tmp_cache_obj));
}

/* set or unset a lock on a memory cache object
 */
MODULE_PRIVATE void
NET_ChangeMemCacheLock(URL_Struct *URL_s, Bool set)
{
	net_MemoryCacheObject * found_cache_obj;

	/* look up cache struct */
	found_cache_obj = net_FindObjectInMemoryCache(URL_s);

	if(found_cache_obj)
	  {
		XP_ASSERT(found_cache_obj->external_locks >= 0);

		if(set)
		  {
			/* increment lock counter */
			found_cache_obj->external_locks++;
		  }
		else
		  {
			/* decrement lock counter */
			found_cache_obj->external_locks--;

			XP_ASSERT(found_cache_obj->external_locks >= 0);

			if(found_cache_obj->external_locks < 0)
				found_cache_obj->external_locks = 0;
		  }
	  }
}

/* remove a URL from the memory cache
 */
MODULE_PRIVATE void
NET_RemoveURLFromMemCache(URL_Struct *URL_s)
{
	net_MemoryCacheObject * found_cache_obj;

	found_cache_obj = net_FindObjectInMemoryCache(URL_s);

	if(found_cache_obj)
		net_FreeMemoryCopy(found_cache_obj);
}

/* Returns TRUE if the URL is currently in the
 * memory cache and false otherwise.
 */
PUBLIC Bool
NET_IsURLInMemCache(URL_Struct *URL_s)
{
	if(net_FindObjectInMemoryCache(URL_s))
		return(TRUE);
	else
		return(FALSE);
}

/* this function looks up a URL in the hash table and
 * returns MEMORY_CACHE_TYPE_URL if it's found and
 * should be loaded via the memory cache.  
 *
 * It also sets several entries in the passed in URL
 * struct including "memory_copy" which is a pointer
 * to the found net_MemoryCacheObject struct.
 */
MODULE_PRIVATE int
NET_FindURLInMemCache(URL_Struct * URL_s, MWContext *ctxt)
{
	net_MemoryCacheObject *found_cache_obj;

	TRACEMSG(("Checking for URL in cache"));

    if(!net_MemoryCacheHashList)
		return(0);

	found_cache_obj = net_FindObjectInMemoryCache(URL_s);

    if(found_cache_obj)
      {
        TRACEMSG(("mkcache: found URL in memory cache!"));

		if(found_cache_obj->current_page_only)
		  {
			History_entry * he = SHIST_GetCurrent(&ctxt->hist);
			int hist_num;

			/* if the current_page_only flag is set 
			 * then the page should only be
			 * served if it's the current document loaded
			 * (i.e. reloads)
			 *
			 * @@@ if !he then let it through, this allows the
			 *     doc info window to work
			 */
			if(he)
			  {
				hist_num = SHIST_GetIndex(&ctxt->hist, he);

				/* we can tell if the document being loaded is the
			 	 * same as the current document by looking up it's
			 	 * index in the history.  If they don't match
			 	 * then it isn't the current document
			 	 */
				if(URL_s->history_num != hist_num)
					return 0;
			  }
		  }

		/* set a pointer to the structure in the URL struct
		 */
		URL_s->memory_copy = found_cache_obj;

        /* copy the contents of the URL struct so that the content type
         * and other stuff gets recorded
         */
        StrAllocCopy(URL_s->content_type,     
					 found_cache_obj->cache_obj.content_type);
        StrAllocCopy(URL_s->charset,          
					 found_cache_obj->cache_obj.charset);
        StrAllocCopy(URL_s->content_encoding, 
					 found_cache_obj->cache_obj.content_encoding);
        URL_s->content_length      = found_cache_obj->cache_obj.content_length;
        URL_s->real_content_length = found_cache_obj->cache_obj.content_length;
 		URL_s->expires             = found_cache_obj->cache_obj.expires;
 		URL_s->last_modified       = found_cache_obj->cache_obj.last_modified;
 		URL_s->is_netsite          = found_cache_obj->cache_obj.is_netsite;

		/* copy security info */
		URL_s->security_on         = found_cache_obj->cache_obj.security_on;
		URL_s->key_size            = found_cache_obj->cache_obj.key_size;
		URL_s->key_secret_size     = found_cache_obj->cache_obj.key_secret_size;
		StrAllocCopy(URL_s->key_cipher, found_cache_obj->cache_obj.key_cipher);

		/* remove any existing certificate */
#ifdef HAVE_SECURITY /* added by jwz */
		CERT_DestroyCertificate(URL_s->certificate);
#endif /* !HAVE_SECURITY -- added by jwz */
		URL_s->certificate = NULL;
		
		if (found_cache_obj->cache_obj.certificate)
#ifdef HAVE_SECURITY /* added by jwz */
		  URL_s->certificate =
			CERT_DupCertificate(found_cache_obj->cache_obj.certificate);
#endif /* !HAVE_SECURITY -- added by jwz */

		net_cache_adding_object++; /* semaphore */
		/* reorder objects so that the list is in last accessed order */
		XP_ListRemoveObject(net_MemoryCacheList, found_cache_obj);
		XP_ListAddObject(net_MemoryCacheList, found_cache_obj);
		net_cache_adding_object--; /* semaphore */

		TRACEMSG(("Cached copy is valid. returning method"));

		return(MEMORY_CACHE_TYPE_URL);
      }

	TRACEMSG(("URL not found in cache"));

    return(0);
}

/*****************************************************************
 *  Memory cache output module routine
 *
 * This set of routines pushes the document in memory up a stream
 * created by NET_StreamBuilder
 */

/* used to hold data between invokations of ProcessNet
 */
typedef struct _MemCacheConData {
	XP_List         *cur_list_ptr;
	uint32           bytes_written_in_segment;
	NET_StreamClass *stream;
} MemCacheConData;

#define CD_CUR_LIST_PTR  connection_data->cur_list_ptr
#define CD_BYTES_WRITTEN_IN_SEGMENT connection_data->bytes_written_in_segment
#define CD_STREAM        connection_data->stream

#define CE_URL_S          cur_entry->URL_s
#define CE_WINDOW_ID      cur_entry->window_id
#define CE_FORMAT_OUT     cur_entry->format_out
#define CE_STATUS         cur_entry->status
#define CE_BYTES_RECEIVED cur_entry->bytes_received

/* begin the load, This is called from NET_GetURL
 */
extern int
net_InitializeNewsFeData (ActiveEntry * cur_entry);

MODULE_PRIVATE int
NET_MemoryCacheLoad (ActiveEntry * cur_entry)
{
    /* get memory for Connection Data */
    MemCacheConData * connection_data = XP_NEW(MemCacheConData);
	net_MemorySegment * mem_seg;
	uint32  chunk_size;
	char   *mem_seg_ptr;
	char   *first_buffer;

	TRACEMSG(("Entering NET_MemoryCacheLoad!\n"));

	if(!connection_data)
	  {
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
		CE_STATUS = MK_OUT_OF_MEMORY;
		return (CE_STATUS);
	  }

    cur_entry->protocol = MEMORY_CACHE_TYPE_URL;
	cur_entry->memory_file = TRUE;

	/* point to the first list struct that contains data
	 */
	CD_CUR_LIST_PTR = CE_URL_S->memory_copy->list->next;
	CD_BYTES_WRITTEN_IN_SEGMENT = 0;

	/* put a read lock on the data
	 */
	CE_URL_S->memory_copy->mem_read_lock++;

	cur_entry->con_data = connection_data;

	cur_entry->local_file = TRUE;

	cur_entry->socket = 1;

	if(net_call_all_the_time_count < 1)
		FE_SetCallNetlibAllTheTime(CE_WINDOW_ID);
    net_call_all_the_time_count++;

    cur_entry->format_out = CLEAR_CACHE_BIT(cur_entry->format_out);
	
	FE_EnableClicking(CE_WINDOW_ID);

    if (cur_entry->format_out == FO_PRESENT &&
        NET_URL_Type(cur_entry->URL_s->address) == NEWS_TYPE_URL)
      {
        /* #### DISGUSTING KLUDGE to make cacheing work for news articles. */
        cur_entry->status = net_InitializeNewsFeData (cur_entry);
        if (cur_entry->status < 0)
          {
            /* #### what error message? */
            return cur_entry->status;
          }
      }

	/* open the outgoing stream
	 */
    CD_STREAM = NET_StreamBuilder(CE_FORMAT_OUT, CE_URL_S, CE_WINDOW_ID);
	if(!CD_STREAM)
	  {
        net_call_all_the_time_count--;
        if(net_call_all_the_time_count < 1)
            FE_ClearCallNetlibAllTheTime(CE_WINDOW_ID);

		FREE(connection_data);

		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CONVERT);
        CE_STATUS = MK_UNABLE_TO_CONVERT;
        return (CE_STATUS);
	  }

	if (!CE_URL_S->load_background)
        FE_GraphProgressInit(CE_WINDOW_ID, CE_URL_S, CE_URL_S->content_length);

	/* process one chunk of the
	 * cache file so that
	 * layout can continue
	 * when images are in the cache
	 */
#define FIRST_BUFF_SIZE 1024
    mem_seg = (net_MemorySegment *) CD_CUR_LIST_PTR->object;

	mem_seg_ptr = mem_seg->segment;

    chunk_size = MIN(FIRST_BUFF_SIZE, 	
					 mem_seg->in_use-CD_BYTES_WRITTEN_IN_SEGMENT);

	/* malloc this first buffer because we can't use
 	 * the NET_SocketBuffer in calls from NET_GetURL 
	 * because of reentrancy
	 */
	first_buffer = (char *) XP_ALLOC(chunk_size);

	if(!first_buffer)
	  {
		FREE(connection_data);
		CE_URL_S->error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
		return(MK_OUT_OF_MEMORY);
	  }

    /* copy the segment because the parser will muck with it
     */
    XP_MEMCPY(first_buffer,
              mem_seg_ptr+CD_BYTES_WRITTEN_IN_SEGMENT,
              (size_t) chunk_size);

    CD_BYTES_WRITTEN_IN_SEGMENT += chunk_size;

    CE_STATUS = (*CD_STREAM->put_block)(CD_STREAM->data_object,
                                        first_buffer,
                                        chunk_size);
    if(CE_STATUS < 0)
      {
        net_call_all_the_time_count--;
        if(net_call_all_the_time_count < 1)
            FE_ClearCallNetlibAllTheTime(CE_WINDOW_ID);

        if (!CE_URL_S->load_background)
            FE_GraphProgressDestroy(CE_WINDOW_ID,
                                    CE_URL_S,
                                    CE_URL_S->content_length,
                                    CE_BYTES_RECEIVED);

        FREE(connection_data);

        return (CE_STATUS);
      }

    CE_BYTES_RECEIVED += chunk_size;

    /* check to see if we need to advance the pointer yet.
	 * should hardly ever happen here since the first buffer
	 * is so small
     */
    if(CD_BYTES_WRITTEN_IN_SEGMENT >= mem_seg->in_use)
      {
        CD_CUR_LIST_PTR = CD_CUR_LIST_PTR->next;
        CD_BYTES_WRITTEN_IN_SEGMENT = 0;
      }

	FREE(first_buffer);

    return(CE_STATUS);
	
}

/* called repeatedly from NET_ProcessNet to push all the
 * data up the stream
 */
MODULE_PRIVATE int
NET_ProcessMemoryCache (ActiveEntry * cur_entry)
{
	MemCacheConData * connection_data = (MemCacheConData *)cur_entry->con_data;
	net_MemorySegment * mem_seg;
    uint32  chunk_size;
	uint32  buffer_size;
	char  *mem_seg_ptr;

	if(!CD_CUR_LIST_PTR)
	  {
		/* when CD_CUR_LIST_PTR turns NULL we are at the end
 		 * of the document.  Finish up the stream and exit
		 */

		/* complete the stream
		 */
	    (*CD_STREAM->complete)(CD_STREAM->data_object);

		/* free the stream
		 */
		FREE(CD_STREAM);

		/* remove the read lock
         */
        CE_URL_S->memory_copy->mem_read_lock--;

		/* check to see if we should delete this memory cached object
		 *
		 * delete_me is set by FreeMemoryCacheObject when it tried
		 * to free it but couldn't because of read locks on the
		 * object
         */
        if(CE_URL_S->memory_copy->delete_me && 
                    CE_URL_S->memory_copy->mem_read_lock == 0)
          {
            net_FreeMemoryCopy(CE_URL_S->memory_copy);
            CE_URL_S->memory_copy = 0;
          }

		/* FREE the structs used by this protocol module
	     */
		FREE(connection_data);

		/* set the status to success */
		CE_STATUS = MK_DATA_LOADED;

		/* clear the CallNetlibAllTheTime if there are no
		 * other readers
		 */
		net_call_all_the_time_count--;
		if(net_call_all_the_time_count < 1)
		    FE_ClearCallNetlibAllTheTime(CE_WINDOW_ID);

        if (!CE_URL_S->load_background)
            FE_GraphProgressDestroy(CE_WINDOW_ID,
                                    CE_URL_S,
                                    CE_URL_S->content_length,
                                    CE_BYTES_RECEIVED);

		/* Tell ProcessNet that we are all done */
		return(-1);
	  }

	/* CD_CUR_LIST_PTR is pointing to the most current
	 * memory segment list object
 	 */
    mem_seg = (net_MemorySegment *) CD_CUR_LIST_PTR->object;

	TRACEMSG(("ProcessMemoryCache: printing segment %p",mem_seg));

	mem_seg_ptr = mem_seg->segment;

	/* write out at least part of the buffer
	 */
	buffer_size = (*CD_STREAM->is_write_ready)(CD_STREAM->data_object);
    buffer_size = MIN(buffer_size, (unsigned int) NET_Socket_Buffer_Size);

	/* make it ??? at the most
	 * when coming out of the cache
	 */
	buffer_size = MIN(buffer_size, NET_MEM_CACHE_OUTPUT_SIZE);

    chunk_size = MIN(buffer_size, mem_seg->in_use-CD_BYTES_WRITTEN_IN_SEGMENT);

    /* copy the segment because the parser will muck with it
     */
    XP_MEMCPY(NET_Socket_Buffer, 
			  mem_seg_ptr+CD_BYTES_WRITTEN_IN_SEGMENT, 
			  (size_t) chunk_size);

	/* remember how much of this segment we have written */
    CD_BYTES_WRITTEN_IN_SEGMENT += chunk_size;

    CE_STATUS = (*CD_STREAM->put_block)(CD_STREAM->data_object,
                                        NET_Socket_Buffer,
                                        chunk_size);
	CE_BYTES_RECEIVED += chunk_size;

	/* check to see if we need to advance the pointer yet
	 */
	if(CD_BYTES_WRITTEN_IN_SEGMENT >= mem_seg->in_use)
	  {
	    CD_CUR_LIST_PTR = CD_CUR_LIST_PTR->next;
        CD_BYTES_WRITTEN_IN_SEGMENT = 0;
	  }

	if (!CE_URL_S->load_background)
        FE_GraphProgress(CE_WINDOW_ID, CE_URL_S, CE_BYTES_RECEIVED,
                         chunk_size, CE_URL_S->content_length);

	if(CE_STATUS < 0)
	  {
        net_call_all_the_time_count--;
        if(net_call_all_the_time_count < 1)
            FE_ClearCallNetlibAllTheTime(CE_WINDOW_ID);

	    (*CD_STREAM->abort)(CD_STREAM->data_object, CE_STATUS);
	  }

	return(CE_STATUS);

}

/* called by functions in mkgeturl to interrupt the loading of
 * an object.  (Usually a user interrupt) 
 */
MODULE_PRIVATE int
NET_InterruptMemoryCache (ActiveEntry * cur_entry)
{
	MemCacheConData * connection_data = (MemCacheConData *)cur_entry->con_data;

	/* abort and free the outgoing stream
	 */
	(*CD_STREAM->abort)(CD_STREAM->data_object, MK_INTERRUPTED);
	FREE(CD_STREAM);

    /* remove the read lock
     */
    CE_URL_S->memory_copy->mem_read_lock--;

    /* check to see if we should delete this memory cached object
     */
    if(CE_URL_S->memory_copy->delete_me && 
			CE_URL_S->memory_copy->mem_read_lock == 0)
      {
        net_FreeMemoryCopy(CE_URL_S->memory_copy);
        CE_URL_S->memory_copy = 0;
      }

	/* FREE the structs
	 */
	FREE(connection_data);
	CE_STATUS = MK_INTERRUPTED;

	net_call_all_the_time_count--;
	if(net_call_all_the_time_count < 1)
	    FE_ClearCallNetlibAllTheTime(CE_WINDOW_ID);

	return(CE_STATUS);
}

/* create an HTML stream and push a bunch of HTML about
 * the memory cache
 */
MODULE_PRIVATE void 
NET_DisplayMemCacheInfoAsHTML(ActiveEntry * cur_entry)
{
	char *buffer = (char*)XP_ALLOC(2048);
	char *address;
	char *escaped;
   	NET_StreamClass * stream;
	net_CacheObject * cache_obj;
    net_MemoryCacheObject * mem_cache_obj;
	Bool long_form = FALSE;
	int32 number_in_memory_cache;
	XP_List *list_ptr;
	int i;

	if(!buffer)
	  {
		cur_entry->status = MK_UNABLE_TO_CONVERT;
		return;
	  }

	if(strcasestr(cur_entry->URL_s->address, "?long"))
		long_form = TRUE;
	else if(strcasestr(cur_entry->URL_s->address, "?traceon"))
		NET_CacheTraceOn = TRUE;
	else if(strcasestr(cur_entry->URL_s->address, "?traceoff"))
		NET_CacheTraceOn = FALSE;

	StrAllocCopy(cur_entry->URL_s->content_type, TEXT_HTML);

	cur_entry->format_out = CLEAR_CACHE_BIT(cur_entry->format_out);
	stream = NET_StreamBuilder(cur_entry->format_out, 
							   cur_entry->URL_s, 
							   cur_entry->window_id);

	if(!stream)
	  {
		cur_entry->status = MK_UNABLE_TO_CONVERT;
		FREE(buffer);
		return;
	  }

	/* define a macro to push a string up the stream
	 * and handle errors
	 */
#define PUT_PART(part)													\
cur_entry->status = (*stream->put_block)(stream->data_object,			\
										part ? part : "Unknown",		\
										part ? XP_STRLEN(part) : 7);	\
if(cur_entry->status < 0)												\
  goto END;

	if(!net_MemoryCacheList)
	  {
		XP_STRCPY(buffer, "There are no objects in the memory cache");
		PUT_PART(buffer);
		goto END;
	  }

	number_in_memory_cache = XP_ListCount(net_MemoryCacheList);

	/* add the header info */
	XP_SPRINTF(buffer, 
"<TITLE>Information about the Netscape memory cache</TITLE>\n"
"<h2>Memory Cache statistics</h2>\n"
"<TABLE>\n"
"<TR>\n"
"<TD ALIGN=RIGHT><b>Maximum size:</TD>\n"
"<TD>%ld</TD>\n"
"</TR>\n"
"<TR>\n"
"<TD ALIGN=RIGHT><b>Current size:</TD>\n"
"<TD>%ld</TD>\n"
"</TR>\n"
"<TR>\n"
"<TD ALIGN=RIGHT><b>Number of files in cache:</TD>\n"
"<TD>%ld</TD>\n"
"</TR>\n"
"<TR>\n"
"<TD ALIGN=RIGHT><b>Average cache file size:</TD>\n"
"<TD>%ld</TD>\n"
"</TR>\n"
"</TABLE>\n"
"<HR>",
net_MaxMemoryCacheSize,
net_MemoryCacheSize,
number_in_memory_cache,
number_in_memory_cache ? net_MemoryCacheSize/number_in_memory_cache : 0);

	PUT_PART(buffer);

	/* define some macros to help us output HTML tables
	 */
#if 0

#define TABLE_TOP(arg1)				\
	XP_SPRINTF(buffer, 				\
"<TR><TD ALIGN=RIGHT><b>%s</TD>\n"	\
"<TD>", arg1);						\
PUT_PART(buffer);

#define TABLE_BOTTOM				\
	XP_SPRINTF(buffer, 				\
"</TD></TR>");						\
PUT_PART(buffer);

#else

#define TABLE_TOP(arg1)					\
	XP_STRCPY(buffer, "<tt>");			\
	for(i=XP_STRLEN(arg1); i < 16; i++)	\
		XP_STRCAT(buffer, "&nbsp;");	\
	XP_STRCAT(buffer, arg1);			\
	XP_STRCAT(buffer, " </tt>");		\
	PUT_PART(buffer);

#define TABLE_BOTTOM					\
	XP_STRCPY(buffer, "<BR>\n");		\
	PUT_PART(buffer);

#endif

	list_ptr = net_MemoryCacheList;

    while((mem_cache_obj = (net_MemoryCacheObject *) XP_ListNextObject(list_ptr)))
      {

		cache_obj = &mem_cache_obj->cache_obj;
		address = XP_STRDUP(mem_cache_obj->cache_obj.address);

		/* put the URL out there */
		TABLE_TOP("URL:");
		XP_STRCPY(buffer, "<A TARGET=Internal_URL_Info HREF=about:");
		PUT_PART(buffer);
		PUT_PART(address);
		XP_STRCPY(buffer, ">");
		PUT_PART(buffer);
		escaped = NET_EscapeHTML(address);
		PUT_PART(escaped);
		FREE(address);
		FREE(escaped);
		XP_STRCPY(buffer, "</A>");
		PUT_PART(buffer);
		TABLE_BOTTOM;

		TABLE_TOP("Content Length:");
		XP_SPRINTF(buffer, "%lu", cache_obj->content_length);
		PUT_PART(buffer);
		TABLE_BOTTOM;

		TABLE_TOP("Content type:");
		PUT_PART(cache_obj->content_type);
		TABLE_BOTTOM;

		TABLE_TOP("Last Modified:");
		if(cache_obj->last_modified)
		  {
			PUT_PART(ctime(&cache_obj->last_modified));
		  }
		else
		  {
			XP_STRCPY(buffer, "No date sent");
			PUT_PART(buffer);
		  }
		TABLE_BOTTOM;

		TABLE_TOP("Expires:");
		if(cache_obj->expires)
		  {
			PUT_PART(ctime(&cache_obj->expires));
		  }
		else
		  {
			XP_STRCPY(buffer, "No expiration date sent");
			PUT_PART(buffer);
		  }
		TABLE_BOTTOM;

		TABLE_TOP("Last accessed:");
		PUT_PART(ctime(&cache_obj->last_accessed));
		TABLE_BOTTOM;

		TABLE_TOP("Character set:");
		if(cache_obj->charset)
		  {
			PUT_PART(cache_obj->charset);
		  }
		else
		  {
			XP_STRCPY(buffer, "iso-8859-1 (default)");
			PUT_PART(buffer);
		  }
		TABLE_BOTTOM;

		TABLE_TOP("Secure:");
		XP_SPRINTF(buffer, "%s", cache_obj->security_on ? "TRUE" : "FALSE");
		PUT_PART(buffer);
		TABLE_BOTTOM;

		XP_STRCPY(buffer, "\n<P>\n");
		PUT_PART(buffer);
	
	
      }

END:
	FREE(buffer);
	if(cur_entry->status < 0)
		(*stream->abort)(stream->data_object, cur_entry->status);
	else
		(*stream->complete)(stream->data_object);

	return;
}

#include "libmocha.h"

#ifdef MOCHA /* added by jwz */
NET_StreamClass *
net_CloneWysiwygMemCacheEntry(MWContext *window_id, URL_Struct *URL_s,
							  uint32 nbytes)
{
	net_MemoryCacheObject *memory_copy;
	PRCList *link;
	CacheDataObject *data_object;
	NET_StreamClass *stream;
	XP_List *list;
	net_MemorySegment *seg;
	uint32 len;

	if (!(memory_copy = URL_s->memory_copy))
	  {
		/* not hitting the cache -- check whether we're filling it */
		for (link = active_cache_data_objects.next;
			 link != &active_cache_data_objects;
			 link = link->next)
		  {
			data_object = (CacheDataObject *)link;
			if (data_object->URL_s == URL_s &&
				(memory_copy = data_object->memory_copy))
			  {
				goto found;
			  }
		  }
		return NULL;
	  }

found:
	stream = LM_WysiwygCacheConverter(window_id, URL_s, NULL);
	if (!stream)
		return 0;
	list = memory_copy->list;
	while (nbytes != 0 &&
		   (seg = (net_MemorySegment *) XP_ListNextObject(list)))
	  {
		len = seg->seg_size;
		if (len > nbytes)
			len = nbytes;
		if (stream->put_block(stream->data_object, seg->segment,
							  (int32)len) < 0)
			break;
		nbytes -= len;
	  }
	if (nbytes != 0)
	  {
		/* NB: Our caller must clear top_state->mocha_write_stream. */
		stream->abort(stream->data_object, MK_UNABLE_TO_CONVERT);
		XP_DELETE(stream);
		return 0;
	  }
	return stream;
}
#endif /* MOCHA -- added by jwz */

#endif /* MOZILLA_CLIENT */
