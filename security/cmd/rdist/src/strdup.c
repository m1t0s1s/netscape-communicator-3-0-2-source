#ifndef lint
static char *RCSid = "$Header: /m/src/ns/security/cmd/rdist/src/strdup.c,v 1.1 1996/01/31 23:32:38 dkarlton Exp $";
#endif

/*
 * $Log: strdup.c,v $
 * Revision 1.1  1996/01/31 23:32:38  dkarlton
 * original rdist 6.1 (copied from sslref)
 *    ssl modifications to rdist.c, rshrcmd.c
 *
 * Revision 1.1  1995/04/05  02:24:25  jsw
 * Unchanged 6.1 distribution of rdist
 *
 * Revision 1.2  1992/04/16  01:28:02  mcooper
 * Some de-linting.
 *
 * Revision 1.1  1992/03/21  02:48:11  mcooper
 * Initial revision
 *
 */


#include <stdio.h>

/*
 * Most systems don't have this (yet)
 */
char *strdup(str)
     char *str;
{
    char 		       *p;
    extern char		       *malloc();
    extern char		       *strcpy();

    if ((p = malloc(strlen(str)+1)) == NULL)
	return((char *) NULL);

    (void) strcpy(p, str);

    return(p);
}
