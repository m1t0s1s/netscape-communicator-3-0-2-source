#ifndef _SECSERV_H_
#define _SECSERV_H_
/*
 * secserv.h - public data structures and prototypes for the server
 *	       specific security library
 *
 * $Id: secfort.h,v 1.1 1996/10/14 05:27:26 jsw Exp $
 */

#ifdef FORTEZZA
/*
** Fortezza Init functions
**
*/
typedef void *(*FortezzaPinFunc)(char **);
typedef SECStatus (*FortezzaAlert) (void *arg, PRBool onOpen, char *string,
					int value1, void *value2);
SECStatus FortezzaConfigureServer(FortezzaPinFunc func1, uint32 mask, 
		char *cert_name, FortezzaAlert func, void *arg, int *error);
#define FORTEZZA_OK        0
#define FORTEZZA_BADPASSWD 1
#define FORTEZZA_BADCARD   2
#endif

#endif /* _SECSERV_H_ */
