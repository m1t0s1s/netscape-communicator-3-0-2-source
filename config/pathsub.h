#ifndef pathsub_h___
#define pathsub_h___
/*
** Pathname subroutines.
**
** Brendan Eich, 8/29/95
*/
#include <limits.h>
#include <sys/types.h>

#if SUNOS4
#include "../nspr/include/sunos4.h"
#endif

#ifdef UNIXWARE
#undef NAME_MAX
#endif

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#ifndef NAME_MAX
#define NAME_MAX 256
#endif

extern char *program;

extern void fail(char *format, ...);
extern char *getcomponent(char *path, char *name);
extern char *ino2name(ino_t ino, char *dir);
extern void *xmalloc(size_t size);
extern char *xstrdup(char *s);
extern char *xbasename(char *path);
extern void xchdir(char *dir);

/* Relate absolute pathnames from and to returning the result in outpath. */
extern int relatepaths(char *from, char *to, char *outpath);

/* XXX changes current working directory -- caveat emptor */
extern void reversepath(char *inpath, char *name, int len, char *outpath);

#endif /* pathsub_h___ */
