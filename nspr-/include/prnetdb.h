#ifndef prnetdb_h___
#define prnetdb_h___

#ifdef XP_PC
	#include <winsock.h>
#elif defined(XP_MAC)
	#include "macsock.h"
#else
	#include "netdb.h"
#endif
#include "prtypes.h"

#if 0
/*
** Structures returned by network data base library.  All addresses are
** supplied in host order, and returned in network order (suitable for
** use in system calls).
*/
typedef struct PRHostEnt {
    char *h_name;	/* official name of host */
    char **h_aliases;	/* alias list */
    int	h_addrtype;	/* host address type */
    int	h_length;	/* length of address */
    char **h_addr_list;	/* list of addresses from name server */
} PRHostEnt;
#else
#define PRHostEnt struct hostent
#endif

/* A safe size to use that will mostly work... */
#define PR_NETDB_BUF_SIZE	512

/*
** Lookup a host by name. If found, fill in "hp" and return hp. These routines
** are thread safe and re-entrancy safe.
**
** If the host is not found, return NULL.
**
** If there is not enough space to store the return data values in buf
** then a NULL is returned and *errorp will be set to the error value.
*/
extern PR_PUBLIC_API(PRHostEnt *)
	PR_gethostbyname(const char *name, PRHostEnt *hp,
			 char *buf, int bufsize, int *errorp);

/*
** Lookup a host by address. "addr" is an opaque address of "len" bytes
** long which is of type "type". If found, fill in "hp" and return hp.
**
** If the host is not found, return NULL.
**
** If there is not enough space to store the return data values in buf
** then a NULL is returned and *errorp will be set to the error value.
*/
extern PR_PUBLIC_API(PRHostEnt *)
	PR_gethostbyaddr(char *addrp, int addrsize, int addrtype,
			 PRHostEnt *hp,
			 char *buf, int bufsize, int *errorp);

/*
** These are the error codes that can be returned in errorp
*/
#define PR_NETDB_OK		0		/* it worked */
#define PR_NETDB_BUF_OVERFLOW	-1		/* bufsize is too small */

#endif /* prnetdb_h___ */
