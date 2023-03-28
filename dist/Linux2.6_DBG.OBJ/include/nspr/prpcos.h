#ifndef prpcos_h___
#define prpcos_h___

#ifdef XP_PC

#include <prmacros.h>
#include <prtypes.h>

#include <windows.h>
#include <setjmp.h>
#include <stdlib.h>

#if defined(NSWINDBGMALLOC)
#include "prdbmem.h"
#elif defined(DEBUG) && defined(_CRTDBG_MAP_ALLOC)
/*
 * Microsoft's crtdbg.h debug malloc routines are incompatible with their
 * stdlib.h and malloc.h headers. Go figure.
 */
#define malloc bogus_malloc
#define calloc bogus_calloc
#define realloc bogus_realloc
#define _expand bogus__expand
#define free bogus_free
#define _msize bogus__msize
#include <stdlib.h>
#include <malloc.h>
#undef malloc
#undef calloc
#undef realloc
#undef _expand
#undef free
#undef _msize
#include <CRTDBG.H>	/* Microsoft's memory debugging routines */
#endif

#define USE_SETJMP
#define DIRECTORY_SEPARATOR         '\\'
#define DIRECTORY_SEPARATOR_STR     "\\"
#define PATH_SEPARATOR              ';'

#ifdef _WIN32
#define GCPTR
#else
#define GCPTR __far
#endif

/*
** Routines for processing command line arguments
*/
NSPR_BEGIN_EXTERN_C

extern char *optarg;
extern int optind, opterr, optopt;
extern int getopt(int argc, char **argv, char *spec);

NSPR_END_EXTERN_C

#define gcmemcpy(a,b,c) memcpy(a,b,c)


/*
** Definitions of directory structures amd functions
** These definitions are from:
**      <dirent.h>
*/
#include <sys/stat.h>
#include <io.h>
#include <fcntl.h>          /* O_BINARY */


#ifndef _WIN32
#include <stdio.h>
#include <winsock.h>

#if !defined(stderr)
extern FILE *stderr;
#endif

NSPR_BEGIN_EXTERN_C

/*
** The following RTL routines are completely missing from WIN16
*/
extern int  printf(const char *, ...);
extern void perror(const char*);

/*
** The following RTL routines are unavailable for WIN16 DLLs
*/
#ifdef _WINDLL

extern void * malloc(size_t);
extern void * realloc(void*, size_t);
extern void * calloc(size_t, size_t);
extern void   free(void*);

/* XXX:  Need to include all of the winsock calls as well... */
extern void   exit(int);
extern size_t strftime(char *, size_t, const char *, const struct tm *);
extern int    sscanf(const char *, const char *, ...);

#endif /* _WINDLL */

NSPR_END_EXTERN_C
#endif /* ! _WIN32 */


typedef int PROSFD;

NSPR_BEGIN_EXTERN_C

#if defined(XP_PC) && !defined(_WIN32)

struct PRMethodCallbackStr {
    void*   (PR_CALLBACK_DECL *malloc) (size_t);
    void*   (PR_CALLBACK_DECL *realloc)(void *, size_t);
    void*   (PR_CALLBACK_DECL *calloc) (size_t, size_t);
    void    (PR_CALLBACK_DECL *free)   (void *);
    int     (PR_CALLBACK_DECL *gethostname)(char * name, int namelen);
    struct hostent * (PR_CALLBACK_DECL *gethostbyname)(const char * name);
    struct hostent * (PR_CALLBACK_DECL *gethostbyaddr)(const char * addr, int len, int type);
    char*   (PR_CALLBACK_DECL *getenv)(const char *varname);
    int     (PR_CALLBACK_DECL *putenv)(const char *assoc);
    int     (PR_CALLBACK_DECL *auxOutput)(const char *outputString);
    void    (PR_CALLBACK_DECL *exit)(int status);
    size_t  (PR_CALLBACK_DECL *strftime)(char *s, size_t len, const char *fmt, const struct tm *p);

    u_long  (PR_CALLBACK_DECL *ntohl)       (u_long netlong);
    u_short (PR_CALLBACK_DECL *ntohs)       (u_short netshort);
    int     (PR_CALLBACK_DECL *closesocket) (SOCKET s);
    int     (PR_CALLBACK_DECL *setsockopt)  (SOCKET s, int level, int optname, const char FAR * optval, int optlen);
    SOCKET  (PR_CALLBACK_DECL *socket)      (int af, int type, int protocol);
    int     (PR_CALLBACK_DECL *getsockname) (SOCKET s, struct sockaddr FAR *name, int FAR * namelen);
    u_long  (PR_CALLBACK_DECL *htonl)       (u_long hostlong);
    u_short (PR_CALLBACK_DECL *htons)       (u_short hostshort);
    unsigned long (PR_CALLBACK_DECL *inet_addr) (const char FAR * cp);
    int     (PR_CALLBACK_DECL *WSAGetLastError)(void);
    int     (PR_CALLBACK_DECL *connect)     (SOCKET s, const struct sockaddr FAR *name, int namelen);
    int     (PR_CALLBACK_DECL *recv)        (SOCKET s, char FAR * buf, int len, int flags);
    int     (PR_CALLBACK_DECL *ioctlsocket) (SOCKET s, long cmd, u_long FAR *argp);
    int     (PR_CALLBACK_DECL *recvfrom)    (SOCKET s, char FAR * buf, int len, int flags, struct sockaddr FAR *from, int FAR * fromlen);
    int     (PR_CALLBACK_DECL *send)        (SOCKET s, const char FAR * buf, int len, int flags);
    int     (PR_CALLBACK_DECL *sendto)      (SOCKET s, const char FAR * buf, int len, int flags, const struct sockaddr FAR *to, int tolen);
    SOCKET  (PR_CALLBACK_DECL *accept)      (SOCKET s, struct sockaddr FAR *addr, int FAR *addrlen);
    int     (PR_CALLBACK_DECL *listen)      (SOCKET s, int backlog);
    int     (PR_CALLBACK_DECL *bind)        (SOCKET s, const struct sockaddr FAR *addr, int namelen);
};

#endif

extern PR_PUBLIC_API(void) PR_MDInit(struct PRMethodCallbackStr *);

NSPR_END_EXTERN_C

#endif /* XP_PC */
#endif /* prpcos_h___ */
