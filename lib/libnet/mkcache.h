
#ifndef MKCACHE_H
#define MKCACHE_H

#include "mkgeturl.h"

XP_BEGIN_PROTOS

extern void   NET_CleanupCache (char * filename);
extern int    NET_FindURLInCache(URL_Struct * URL_s, MWContext *ctxt);
extern void   NET_RefreshCacheFileExpiration(URL_Struct * URL_s);

/* read the Cache File allocation table.
 */
extern void NET_ReadCacheFAT(char * cachefatfile, Bool stat_files);

/* remove a URL from the cache
 */
extern void NET_RemoveURLFromCache(URL_Struct *URL_s);

/* create an HTML stream and push a bunch of HTML about
 * the cache
 */
extern void NET_DisplayCacheInfoAsHTML(ActiveEntry * cur_entry);

/* trace variable for cache testing */
extern XP_Bool NET_CacheTraceOn;

/* return TRUE if the URL is in the cache and
 * is a partial cache file
 */ 
extern Bool NET_IsPartialCacheFile(URL_Struct *URL_s);


XP_END_PROTOS

#endif /* MKCACHE_H */
