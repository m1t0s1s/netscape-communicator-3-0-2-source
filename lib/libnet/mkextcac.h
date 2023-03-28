
#ifndef MKEXTCACHE_H
#define MKEXTCACHE_H

/* lookup routine
 *
 * builds a key and looks for it in
 * the database.  Returns an access
 * method and sets a filename in the
 * URL struct if found
 */
extern int NET_FindURLInExtCache(URL_Struct * URL_s, MWContext *ctxt);

extern void
NET_OpenExtCacheFAT(MWContext *ctxt, char * cache_name, char * instructions);

#endif /* MKEXTCACHE_H */
