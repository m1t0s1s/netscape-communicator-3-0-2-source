#ifndef MK_MEMORY_CACHE_H

/* A pointer to this struct is passed as the converter_obj argument to
 * NET_MemCacheConverter.
 */
typedef struct net_MemCacheConverterObject {
    NET_StreamClass *next_stream;
	XP_Bool         dont_hold_URL_s;
} net_MemCacheConverterObject;

extern NET_StreamClass * 
NET_MemCacheConverter (FO_Present_Types format_out,
                    void       *converter_obj,
                    URL_Struct *URL_s,
                    MWContext  *window_id);

extern int
NET_FindURLInMemCache(URL_Struct * URL_s, MWContext *ctxt);

extern int
NET_MemoryCacheLoad (ActiveEntry * cur_entry);

extern int
NET_ProcessMemoryCache (ActiveEntry * cur_entry);

extern int
NET_InterruptMemoryCache (ActiveEntry * cur_entry);

/* remove a URL from the memory cache
 */
extern void
NET_RemoveURLFromMemCache(URL_Struct *URL_s);

/* create an HTML stream and push a bunch of HTML about
 * the memory cache
 */
extern void
NET_DisplayMemCacheInfoAsHTML(ActiveEntry * cur_entry);

/* set or unset a lock on a memory cache object
 */
extern void
NET_ChangeMemCacheLock(URL_Struct *URL_s, Bool set);

/* Create a wysiwyg cache converter to a copy of the current entry for URL_s.
 */
extern NET_StreamClass *
net_CloneWysiwygMemCacheEntry(MWContext *window_id, URL_Struct *URL_s,
							  uint32 nbytes);

#endif /* MK_MEMORY_CACHE_H */
