#ifndef XPSTRING_H
#define XPSTRING_H

#include <string.h>

#define XP_STRCASECMP  strcasecomp
#define XP_STRNCASECMP strncasecomp

XP_BEGIN_PROTOS

/*
** Some basic string things
*/

/*
** Concatenate s1 to s2. If s1 is NULL then a copy of s2 is
** returned. Otherwise, s1 is realloc'd and s2 is concatenated, with the
** new value of s1 being returned.
*/
extern char *XP_AppendStr(char *s1, char *s2);


/*
** Concatenate a bunch of strings. The variable length argument list must
** be terminated with a NULL. This result is a DS_Alloc'd string.
*/
extern char *XP_Cat(char *s1, ...);

/*
 * Case-insensitive string comparison
 */
extern int strcasecomp  (const char *a, const char *b);
extern int strncasecomp (const char *a, const char *b, int n);

/* find a substring within a string with a case insensitive search
 */
extern char * strcasestr (const char * str, const char * substr);

/* find a substring within a specified length string with a case 
 * insensitive search
 */
extern char * strncasestr (const char * str, const char * substr, int32 len);

/*
 * Malloc'd string manipulation
 *
 * notice that they are dereferenced by the define!
 */
#define StrAllocCopy(dest, src) NET_SACopy (&(dest), src)
#define StrAllocCat(dest, src)  NET_SACat  (&(dest), src)
extern char * NET_SACopy (char **dest, const char *src);
extern char * NET_SACat  (char **dest, const char *src);

/*
 * Malloc'd block manipulation
 *
 * Lengths are necessary here :(
 *
 * notice that they are dereferenced by the define!
 */
#define BlockAllocCopy(dest, src, src_length) NET_BACopy((char**)&(dest), src, src_length)
#define BlockAllocCat(dest, dest_length, src, src_length)  NET_BACat(&(dest), dest_length, src, src_length)
extern char * NET_BACopy (char **dest, const char *src, size_t src_length);
extern char * NET_BACat  (char **dest, size_t dest_length, const char *src, size_t src_length);

extern char * XP_StripLine (char *s);

/* Match = 0, NoMatch = 1, Abort = -1 */
/* Based loosely on sections of wildmat.c by Rich Salz */
extern int xp_RegExpSearch(char *str, char *exp);

XP_END_PROTOS

#endif /* XPSTRING_H */
