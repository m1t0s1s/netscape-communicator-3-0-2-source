#ifndef prfile_h___
#define prfile_h___

#include "prlong.h"
#include "prosdep.h"
#include "prmacros.h"
#include "prtypes.h"

/* jwz: added linux */
#if defined(AIXV3) || defined(LINUX)
/* Needed to get fd_set typedef */
#include <sys/select.h>
#endif /* AIXV3 */


#ifdef XP_MAC
#include <stdio.h>			// Needed to get FILE typedef
#include <stddef.h>			// Needed for size_t typedef

#include "prmacos.h"
#endif /* XP_MAC */


NSPR_BEGIN_EXTERN_C

/* XXX new additions: what about error codes? */

/* Typedefs */
typedef int PRFileHandle;

typedef struct PRFileDescStr PRFileDesc;
typedef struct PRIOMethodsStr PRIOMethods;

#if 0
typedef struct PRIOVecStr PRIOVec;
struct PRIOVecStr {
    void *base;
    int32 len;
};
#endif

struct PRIOMethodsStr {
    int32 (*init)  (PRFileDesc * fd);
    int32 (*close) (PRFileDesc * fd);
    int32 (*read)  (PRFileDesc * fd, void *buf, int32 amount);
    int32 (*write) (PRFileDesc * fd, const void *buf, int32 amount);
};

struct PRFileDescStr {
    /* Underlying system specific descriptor */
    PROSFD osfd;

    /* Flag data per descriptor */
    unsigned char flags;

    /* Usage count */
    unsigned char count;

    /* Private instance data */
    void *instance;

    /* Method table for this file descriptor */
    PRIOMethods *methods;
};

/*
** Table of File Descriptors
*/
extern PRFileDesc pr_fdTable[];
#define PR_HANDLE_TO_DESC(fd)   (&pr_fdTable[(fd)])


/*
** Avoid ANSI warnings...
*/

#ifndef HPUX
struct timeval;
#endif /* HPUX */
struct iovec;

/*
** Open a file: object
*/
extern PR_PUBLIC_API(PRFileHandle) PR_OpenFile(const char *fileName, int flags, 
                                              int mode);

/* XXX flags */
/* XXX mode */

/*
** Create a socket object
*/
extern PR_PUBLIC_API(PRFileDesc *) PR_Socket(int, int, int);

extern PR_PUBLIC_API(void) PR_NonBlockIO(int desc, int on);/* XXX legacy */

extern PR_PUBLIC_API(PRFileHandle) PR_MapFileHandle(PROSFD osfd, void *data, 
                                                   PRIOMethods *methods);

extern PR_PUBLIC_API(PRFileHandle) PR_CreateSocket(int domain, int type, 
                                                  int protocol);
extern PR_PUBLIC_API(PRFileHandle) PR_ImportSocket(PROSFD osfd);
extern PR_PUBLIC_API(int32) PR_ConnectSocket(PRFileDesc *fd, void *addr, 
                                            int addrlen);

extern PR_PUBLIC_API(int32) PR_READ(PRFileHandle hdl, void *buf, int32 size);
extern PR_PUBLIC_API(int32) PR_WRITE(PRFileHandle hdl, const void *buf, 
                                    int32 size);
extern PR_PUBLIC_API(int32) PR_CLOSE(PRFileHandle hdl);

/* Macros to access methods on the FD */
#define PR_CONNECT(fd,b,c) PR_ConnectSocket(PR_HANDLE_TO_DESC(fd), b, c)

#ifdef XP_UNIX
#if 0	/* -- rjp */
#define PR_RECV(fd,b,c,d) (*(fd)->methods->recv)(fd,b,c,d)
#define PR_RECVFROM(fd,b,c,d,e,f) (*(fd)->methods->recvfrom)(fd,b,c,d,e,f)
#define PR_RECVMSG(fd,b,c) (*(fd)->methods->recvmsg)(fd,b,c)
#define PR_READV(fd,b,c) (*(fd)->methods->readv)(fd,b,c)

#define PR_SEND(fd,b,c,d) (*(fd)->methods->send)(fd,b,c,d)
#define PR_SENDTO(fd,b,c,d,e,f) (*(fd)->methods->sendto)(fd,b,c,d,e,f)
#define PR_SENDMSG(fd,b,c) (*(fd)->methods->sendmsg)(fd,b,c)
#define PR_WRITEV(fd,b,c) (*(fd)->methods->writev)(fd,b,c)

#ifdef HAVE_STREAMS
#define PR_GETMSG(fd,b,c,d) (*(fd)->methods->getmsg)(fd,b,c,d)
#define PR_PUTMSG(fd,b,c,d) (*(fd)->methods->putmsg)(fd,b,c,d)
PR_PUBLIC_API extern int PR_Poll(struct pollfd *fds, unsigned long nfds, 
                                 int timeout);
#endif  /* HAVE_STREAMS */
#endif /* 0 - rjp */
extern PR_PUBLIC_API(int) PR_Select(int nfd, fd_set *r, fd_set *w, fd_set *e,
                                   struct timeval *t);
#endif  /* UNIX */

typedef struct PRDirStr PRDir;
typedef struct PRDirEntryStr PRDirEntry;
typedef struct PRFileInfoStr PRFileInfo;

struct PRDirEntryStr {
    char *d_name;
};

/*
** Basic information about a file
*/
struct PRFileInfoStr {
    int type;                   /* Type of file; see below */
    prword_t size;              /* Size, in bytes, of file's contents */
    int64 modifyTime;           /* Last modification time */
};

/* File info type's */
#define PR_FI_FILE          1   /* Regular file */
#define PR_FI_DIRECTORY     2   /* Directory */
#define PR_FI_OTHER         3   /* Something else (device?) */

/*
** Return information about a file. Returns -1 if the file cannot be
** checked.
*/

/* Filesystem */
extern PR_PUBLIC_API(int) PR_GetFileInfo(char *fn, PRFileInfo *info);
extern PR_PUBLIC_API(int) PR_RenameFile (char *from, char *to);
extern PR_PUBLIC_API(int) PR_AccessFile (char *name, int how);

#define PR_AF_EXISTS        1
#define PR_AF_WRITE_OK      2
#define PR_AF_READ_OK       3

extern PR_PUBLIC_API(int) PR_Mkdir(char *name, int mask);

#define PR_RD_SKIP_DOT      0x1
#define PR_RD_SKIP_DOT_DOT  0x2

extern PR_PUBLIC_API(PRDir *) PR_OpenDirectory(char *name);
extern PR_PUBLIC_API(PRDirEntry *) PR_ReadDirectory(PRDir *dir, int flags);

extern PR_PUBLIC_API(void) PR_CloseDirectory(PRDir *dir);

#ifdef _NSPR
/* PRFileDesc flags */
#define _PR_ASYNC           0x1
#define _PR_USER_ASYNC      0x2

extern void _PR_InitIO(void);
extern void _OS_InitIO(void);

extern PR_PUBLIC_API(int) _PR_WaitForFD(PRFileDesc *fd, int rd);
/*
extern PRFileHandle _PR_FindFileHandle(PROSFD osfd);
extern PRFileHandle _PR_FindFDFromOSFD(PROSFD osfd);
*/

extern PRMonitor *pr_fdLock;
extern PRMonitor *pr_asyncInput;
extern PRMonitor *pr_asyncOutput;

#ifdef XP_UNIX
#include <errno.h>
#include <fcntl.h>
#define _OS_PREPARE_NAME(a,b)   ((char*) (a))
#define _OS_FREE_NAME(a)
#define _OS_COMPARE_FDS(a,b)    ((a) == (b))
#define _OS_INVALID_FD(fd)      ((fd)->osfd < 0)
#define _OS_INVALID_FD_VALUE    -1
#endif

#ifdef XP_PC
#include <errno.h>
#include <fcntl.h>
#include <winsock.h>
#define EWOULDBLOCK             WSAEWOULDBLOCK

extern char * _OS_PREPARE_NAME( const char *name, int len );
#define _OS_FREE_NAME(a)        free(a)
#define _OS_COMPARE_FDS(a,b)    ((a) == (b))
#define _OS_INVALID_FD(fd)      ((fd)->osfd < 0)
#define _OS_INVALID_FD_VALUE    -1
#endif  /* XP_PC */


#ifdef XP_MAC
// Custom NSPR routines for Macintosh
extern char *_OS_PREPARE_NAME(const char *name, int len);
extern FILE *_OS_FOPEN(const char *filename, const char *mode);
extern int PR_Stat(const char *path, struct stat *buf);
extern long PR_Seek(int fd, long off, int whence);
extern int PR_Open(const char*, int, int);
extern int PR_Close(int);
extern int PR_Unlink(const char *);

// NSPR Routines which are just aliases to standard C library routines.
#define _OS_WRITE(filedes, buf, count)         write((filedes), (buf), (count))

// Useful Mac-specific File routines.  Should these go elsewhere?
extern void dprintf(const char *format, ...);

#endif  /* XP_MAC */


#ifdef XP_UNIX
extern int _OS_OPEN(const char*, int, int);
extern int _OS_CLOSE(int);

extern int _OS_SOCKET(int, int, int);
extern int _OS_CONNECT(int, void *, int);
extern int _OS_ACCEPT(int, void *, int *);
extern int _OS_BIND(int, void *, int);
extern int _OS_LISTEN(int, int);

#define _OS_CLOSE_SOCKET(a) _OS_CLOSE(a)
extern int _OS_RECV(int, void*, int, int);
extern int _OS_SEND(int, const void*, int, int);

extern int _OS_FCNTL(int, int, int);
extern int _OS_IOCTL(int, int, int);

#if 0
extern int _OS_RECVFROM(int, void*, int, int, void *, int *);
extern int _OS_RECVMSG(int, struct msghdr *, int);
extern int _OS_READV(int, struct iovec *, int);
#endif
extern int _OS_READ(int, void*, size_t);

#if 0
extern int _OS_SENDTO(int, const void*, int, int, const void *, int);
extern int _OS_SENDMSG(int, const struct msghdr*, int);
extern int _OS_WRITEV(int, const struct iovec *, int);
#endif
extern int _OS_WRITE(int, const void*, size_t);

extern int _OS_SELECT(int, fd_set *, fd_set *, fd_set *, struct timeval *);

#ifdef HAVE_STREAMS
extern int _OS_POLL(struct pollfd *, unsigned long, int);
extern int _OS_GETMSG(int, struct strbuf *, struct strbuf *, int *);
extern int _OS_PUTMSG(int, const struct strbuf *, const struct strbuf *, int);
#endif /* HAVE_STREAMS */

#endif /* XP_UNIX */

#ifdef XP_PC
#define _OS_OPEN(name, flags, mode)     _open(name,flags,mode)
#define _OS_CLOSE(fd)                   _close(fd)

#define _OS_READ(fd, buf, len)          _read(fd,buf,(uint)(len))
#define _OS_WRITE(fd, buf, len)         _write(fd,buf,(uint)(len))

#define _OS_SOCKET(a,b,c)		_socket(a,b,c)
#define _OS_CLOSE_SOCKET(a,b,c)		???
#endif  /* XP_PC */

#endif /* _NSPR */

NSPR_END_EXTERN_C

#endif /* prfile_h___ */
