
#ifndef MKPARSE_H
#define MKPARSE_H

#ifndef MKUTILS_H
#include "mkutils.h"
#endif /* MKUTILS_H */

/*
 *  Returns the number of the month given or -1 on error.
 */
extern int NET_MonthNo (char * month);

/* accepts an RFC 850 or RFC 822 date string and returns a
 * time_t
 */
extern time_t NET_ParseDate(char *date_string);

/* remove front and back white space
 * modifies the original string
 */
extern char * NET_Strip (char * s);

/* find a character in a fixed length buffer */
extern char * strchr_in_buf(char *string, int32 string_len, char find_char);

#endif  /* MKPARSE_H */
