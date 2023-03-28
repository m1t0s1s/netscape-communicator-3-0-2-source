#ifndef lint
static char *RCSid = "$Header: /m/src/ns/security/cmd/rdist/src/strerror.c,v 1.1 1996/01/31 23:32:39 dkarlton Exp $";
#endif

/*
 * $Log: strerror.c,v $
 * Revision 1.1  1996/01/31 23:32:39  dkarlton
 * original rdist 6.1 (copied from sslref)
 *    ssl modifications to rdist.c, rshrcmd.c
 *
 * Revision 1.1  1995/04/05  02:24:26  jsw
 * Unchanged 6.1 distribution of rdist
 *
 * Revision 1.1  1992/03/21  02:48:11  mcooper
 * Initial revision
 *
 */

#include <stdio.h>
#include <sys/errno.h>

/*
 * Return string for system error number "Num".
 */
char *strerror(Num)
     int			Num;
{
    extern int 			sys_nerr;
    extern char 	       *sys_errlist[];
    static char 		Unknown[100];

    if (Num < 0 || Num > sys_nerr) {
	(void) sprintf(Unknown, "Error %d", Num);
	return(Unknown);
    } else
	return(sys_errlist[Num]);
}
