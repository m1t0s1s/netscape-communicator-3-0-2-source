/**********************************************************************
 sco_license.c
 By Daniel Malmer
 9/26/96

 Code for SCO product licensing.

 SCO has this whacked-out licensing library that's more trouble than
 it's worth, but the customer's always right, right?

 It's in nspr since all the SCO client & server products have to use it.

 It's in its own file so that everyone who links with nspr but doesn't
 care about licensing (eg, javah) won't pull in unresolved externals.
 
 Copyright \051 1996 Netscape Communications Corporation, all rights reserved.
**********************************************************************/

#if defined(SCO_PM)
 
#include <annot.h>
#include <licenseIDs.h>
#include <stdio.h>
#include <stdlib.h>
 
#define NELEM(array)    ((sizeof(array))/(sizeof(array[0])))
 

/*
 * license_error
 */
static void
license_error(char* msg)
{
#if defined(SERVER_BUILD)
    syslog(LOG_ERR, msg);
#endif
    fprintf(stderr, "%s\n", msg);
}


/*
 * sco_license
 */
void
sco_license(char* product_name, struct sco_license licenses[], int nelem)
{
    char    *message;
 
    if ( check_any_license(licenses, nelem, product_name, &message) != 1 ) {
        license_error(message);
        exit (1);
    }
 
    if ( SSL_IsDomestic() ) {
        char buf[1024];
        struct sco_license encryption[] = {
            { VENDOR_SCO, LIC_ZID_SCO_ENCRYPTION, NULL, },
            { -1, LIC_ID_STRONG_ENCRYPTION, NULL, },
        };

        sprintf(buf, "%s (encryption)", product_name);

        if ( check_any_license(encryption, NELEM(encryption),
                               buf, &message) != 1 ) {
            license_error(message);
            exit (1);
        }
    }
}
#endif /* SCO_PM */


