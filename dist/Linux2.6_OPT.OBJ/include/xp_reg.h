/*
 * shexp.h: Defines and prototypes for shell exp. match routines
 * 
 *
 * This routine will match a string with a shell expression. The expressions
 * accepted are based loosely on the expressions accepted by zsh.
 * 
 * o * matches anything
 * o ? matches one character
 * o \ will escape a special character
 * o $ matches the end of the string
 * o [abc] matches one occurence of a, b, or c. The only character that needs
 *         to be escaped in this is ], all others are not special.
 * o [a-z] matches any character between a and z
 * o [^az] matches any character except a or z
 * o ~ followed by another shell expression will remove any pattern
 *     matching the shell expression from the match list
 * o (foo|bar) will match either the substring foo, or the substring bar.
 *             These can be shell expressions as well.
 * 
 * The public interface to these routines is documented below.
 * 
 * Rob McCool
 * 
 */
 
#ifndef SHEXP_H
#define SHEXP_H

#include "xp_core.h"

/*
 * Requires that the macro MALLOC be set to a "safe" malloc that will 
 * exit if no memory is available. If not under MCC httpd, define MALLOC
 * to be the real malloc and play with fire, or make your own function.
 */

#if 0
#ifdef MCC_HTTPD
#include "../mc-httpd.h"
#endif
#endif

#include <ctype.h>  /* isalnum */
#include <string.h> /* strlen */



/* --------------------------- Public routines ---------------------------- */


/*
 * shexp_valid takes a shell expression exp as input. It returns:
 * 
 *  NON_SXP      if exp is a standard string
 *  INVALID_SXP  if exp is a shell expression, but invalid
 *  VALID_SXP    if exp is a valid shell expression
 */

#define NON_SXP -1
#define INVALID_SXP -2
#define VALID_SXP 1

extern int XP_RegExpValid(char *exp);

/*
 * shexp_match 
 * 
 * Takes a prevalidated shell expression exp, and a string str.
 *
 * Returns 0 on match and 1 on non-match.
 */
extern int XP_RegExpMatch(char *str, char *exp, Bool case_insensitive);

/*
 * 
 * Same as above, but validates the exp first. 0 on match, 1 on non-match,
 * -1 on invalid exp.
 */

extern int XP_RegExpSearch(char *str, char *exp);

/* same as above but uses case insensitive search
 */
extern int XP_RegExpCaseSearch(char *str, char *exp);

#endif
