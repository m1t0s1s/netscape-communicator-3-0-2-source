#include "prnetdb.h"
#include "prmon.h"
#include <string.h>
#ifndef XP_PC
	#ifndef XP_MAC
		#include <netinet/in.h>
	#endif
#endif

#ifdef XP_MAC
#include "macsock.h"
#endif

extern PRMonitor *_pr_dns_lock;

#define LockDNS() if (_pr_dns_lock) PR_EnterMonitor(_pr_dns_lock);
#define UnlockDNS() if (_pr_dns_lock) PR_ExitMonitor(_pr_dns_lock);

#ifdef XP_UNIX
#include <signal.h>
#include <sys/socket.h>

static sigset_t timer_set;
static int first_time = 1;

void DisableClock(sigset_t* oldset)
{
    if (first_time) {
	sigemptyset(&timer_set);
	sigaddset(&timer_set, SIGALRM);
	first_time = 0;
    }
    sigprocmask(SIG_BLOCK, &timer_set, oldset);
}

void EnableClock(sigset_t* oldset)
{
    sigprocmask(SIG_SETMASK, oldset, 0);
}
#endif

#ifdef XP_PC

#endif

/*
** Allocate space from the buffer, aligning it to "align" before doing
** the allocation. "align" must be a power of 2.
*/
static char *Alloc(int amount, char **bufp, int *buflenp, int align)
{
    char *buf = *bufp;
    int buflen = *buflenp;

    if (align && ((long)buf & (align - 1))) {
	int skip = align - (int)((long)buf & (align - 1));
	if (buflen < skip) {
	    return 0;
	}
	buf += skip;
	buflen -= skip;
    }
    if (buflen < amount) {
	return 0;
    }
    *bufp = buf + amount;
    *buflenp = buflen - amount;
    return buf;
}

/*
** Copy a hostent, and all of the memory that it refers to into
** (hopefully) stacked buffers.
*/
static int CopyHostent(PRHostEnt *to, struct hostent *from,
		       char *buf, int bufsize, int *errorp)
{
    int len, na;
    char **ap;

    /* Do the easy stuff */
    to->h_addrtype = from->h_addrtype;
    to->h_length = from->h_length;

    /* Copy the official name */
    if (!from->h_name) return PR_NETDB_BUF_OVERFLOW;
    len = strlen(from->h_name) + 1;
    to->h_name = Alloc(len, &buf, &bufsize, 0);
    if (!to->h_name) return PR_NETDB_BUF_OVERFLOW;
    memcpy(to->h_name, from->h_name, len);

    /* Count the aliases, then allocate storage for the pointers */
    if ((ap = from->h_aliases)) {
    for (na = 1, ap = from->h_aliases; *ap != 0; na++, ap++);
    to->h_aliases = (char**) Alloc(na * sizeof(char*), &buf, &bufsize,
				   sizeof(char**));
    if (!to->h_aliases) return PR_NETDB_BUF_OVERFLOW;

    /* Copy the aliases, one at a time */
    for (na = 0, ap = from->h_aliases; *ap != 0; na++, ap++) {
	if (*ap) {
	    len = strlen(*ap) + 1;
	    to->h_aliases[na] = Alloc(len, &buf, &bufsize, 0);
	    if (!to->h_aliases[na]) return PR_NETDB_BUF_OVERFLOW;
	    memcpy(to->h_aliases[na], *ap, len);
	} else {
	    to->h_aliases[na] = 0;
	}
    }
    to->h_aliases[na] = 0;
    }
    else
    	to->h_aliases = NULL;

    /* Count the addresses, then allocate storage for the pointers */
    for (na = 1, ap = from->h_addr_list; *ap != 0; na++, ap++);
    to->h_addr_list = (char**) Alloc(na * sizeof(char*), &buf, &bufsize,
				     sizeof(char**));
    if (!to->h_addr_list) return PR_NETDB_BUF_OVERFLOW;

    /* Copy the addresses, one at a time */
    for (na = 0, ap = from->h_addr_list; *ap != 0; na++, ap++) {
	if (*ap) {
	    to->h_addr_list[na] = Alloc(to->h_length, &buf, &bufsize, 0);
	    if (!to->h_addr_list[na]) return PR_NETDB_BUF_OVERFLOW;
	    memcpy(to->h_addr_list[na], *ap, to->h_length);
	} else {
	    to->h_addr_list[na] = 0;
	}
    }
    to->h_addr_list[na] = 0;
    return PR_NETDB_OK;
}

PR_PUBLIC_API(PRHostEnt *)
PR_gethostbyname(const char *name, PRHostEnt *hp,
		 char *buf, int bufsize, int *errorp)
{
    int rv;
    struct hostent *h;
#ifdef XP_UNIX
    sigset_t oldset;
    DisableClock(&oldset);
#endif
    LockDNS();
    h = gethostbyname(name);
    if (h && hp) {
	/*
	** Copy data from h into hp. Any subdata is copied into buf until
	** buf overflows.
	*/
	rv = CopyHostent(hp, h, buf, bufsize, errorp);
	if ((rv != PR_NETDB_OK)) {
	    h = 0;
	    if ( errorp ) *errorp = rv;
	}
    }
    UnlockDNS();
#ifdef XP_UNIX
    EnableClock(&oldset);
#endif
    return h ? hp : 0;
}

PR_PUBLIC_API(PRHostEnt *)
PR_gethostbyaddr(char *addrp, int addrsize, int addrtype,
		 PRHostEnt *hp,
		 char *buf, int bufsize, int *errorp)
{
    int rv;
    struct hostent *h;
#ifdef XP_UNIX
    sigset_t oldset;
    DisableClock(&oldset);
#endif
    LockDNS();
    h = gethostbyaddr(addrp, addrsize, addrtype);
    if (h && hp) {
	/*
	** Copy data from h into hp. Any subdata is copied into buf until
	** buf overflows.
	*/
	rv = CopyHostent(hp, h, buf, bufsize, errorp);
	if ((rv != PR_NETDB_OK)) {
	    h = 0;
	    if ( errorp ) *errorp = rv;
	}
    }
    UnlockDNS();
#ifdef XP_UNIX
    EnableClock(&oldset);
#endif
    return h ? hp : 0;
}
