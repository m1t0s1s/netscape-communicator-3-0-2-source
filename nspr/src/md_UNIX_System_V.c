/*
 * md_UNIX_System_V.c
 * By Daniel Malmer
 * 4/15/96
 */
 
#include "prtypes.h"
#include "prosdep.h"
#include "prthread.h"
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#include "mdint.h"
#include <errno.h>
 
extern int syscall(int, ...);
 
void _MD_InitOS(int when)
{
    _MD_InitUnixOS(when);
}
 
prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
        (void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(prword_t);
    return (prword_t*) CONTEXT(t);
}


#define max(a, b) ((a>b) ? a : b)
 
int
poll(struct pollfd* pfds, unsigned long width, int timeout)
{
    int i;
    int rv;
    int nfds;
    fd_set in, out, ex;
    struct timeval tv;
 
    FD_ZERO(&in);
    FD_ZERO(&out);
    FD_ZERO(&ex);
 
    /*
     * determine nfds
     */
    nfds = 0;
    for ( i = 0; i < width; i++ ) {
        nfds = max(nfds, pfds[i].fd+1);
    }
 
    for ( i = 0; i < width; i++ ) {
        if ( pfds[i].events & POLLIN ) FD_SET(pfds[i].fd, &in);
        if ( pfds[i].events & POLLOUT ) FD_SET(pfds[i].fd, &out);
        if ( pfds[i].events & POLLPRI ) FD_SET(pfds[i].fd, &ex);
    }
 
    /*
     * timeout is in milliseconds
     */
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = (timeout % 1000) * 1000;
 
    rv = select(nfds, &in, &out, &ex, timeout < 0 ? NULL : &tv);
 
    for ( i = 0; i < width; i++ ) {
        pfds[i].revents = 0;
    }
 
    if ( rv == 0 ) return 0;
 
    if ( rv < 0 ) {
        for ( i = 0; i < width; i++ ) {
            pfds[i].revents = POLLNVAL;
        }
        return rv;
    }
 
    rv = 0;
    for ( i = 0; i < width; i++ ) {
        if ( FD_ISSET(pfds[i].fd, &in) ) pfds[i].revents = POLLIN;
        if ( FD_ISSET(pfds[i].fd, &out) ) pfds[i].revents = POLLOUT;
        if ( FD_ISSET(pfds[i].fd, &ex) ) pfds[i].revents = POLLERR;
        if (  pfds[i].revents != 0 ) rv++;
    }
 
    return rv;
}


/************************************************************************/
 
int _OS_READ(int fd, void *buf, size_t len)
{
    int rv;
    rv = _read(fd, buf, len);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}
 
int _OS_WRITE(int fd, const void *buf, size_t len)
{
    int rv;
    rv = _write(fd, buf, len);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}
 
int _OS_SELECT(int fd, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    int rv;
    rv = _select(fd, r, w, e, t);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}
 
ssize_t _OS_OPEN(const char *path, int oflag, mode_t mode)
{
    int rv;
    rv = _open(path, oflag, mode);
    if(rv < 0)
        return -errno;
    return rv;
}
 
int _OS_CLOSE(int fd)
{
    int rv;
    rv = _close(fd);
    if(rv < 0)
        return -errno;
    return rv;
}
 
int _OS_FCNTL(int fd, int flag, int value)
{
    int rv;
    rv = _fcntl(fd, flag, value);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}
 
int _OS_SEND(int fd, const void *buf, size_t len)
{
    int rv;
    rv = send(fd, buf, len, 0);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}
 
int _OS_RECV(int fd, void *buf, size_t len)
{
    int rv;
    rv = recv(fd, buf, len, 0);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}
 
int _OS_SOCKET(int domain, int type, int protocol)
{
    int rv;
    rv = socket(domain, type, protocol);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}
 
int _OS_CONNECT(int fd, void *addr, int addrlen)
{
    int rv;
    rv = connect(fd, addr, addrlen);
    if (rv < 0) {
        return -errno;
    }
    return rv;
}

/************************************************************************/
 
int
strcasecmp(char* s1, char* s2)
{
    for ( ; *s1 && *s2 && toupper(*s1) == toupper(*s2); s1++, s2++ )
        ;
 
    if ( *s1 == '\0' && *s2 == '\0' ) return 0;
 
    return ( strcmp ( s1, s2) );
}
 
/************************************************************************
 *
 * Using gnu's alloc
 *
 ************************************************************************/

 
#if !defined (__GNUC__) || __GNUC__ < 2
 
#ifndef alloca
 
#define ADDRESS_FUNCTION(arg) &(arg)
 
#if __STDC__
typedef void *pointer;
#else
typedef char *pointer;
#endif
 
#define NULL    0
 
#if 0
#ifndef emacs
#define malloc xmalloc
#endif
extern pointer malloc ();
 
#ifndef STACK_DIRECTION
#define STACK_DIRECTION 0
#endif
 
#if STACK_DIRECTION != 0
 
#define STACK_DIR       STACK_DIRECTION
 
#else
 
static int stack_dir;
#define STACK_DIR       stack_dir
 
static void
find_stack_direction ()
{
  static char *addr = NULL;
  auto char dummy;
 
  if (addr == NULL)
    {
      addr = ADDRESS_FUNCTION (dummy);
 
      find_stack_direction ();
    }
  else
    {
      if (ADDRESS_FUNCTION (dummy) > addr)
        stack_dir = 1;
      else
        stack_dir = -1;
    }
}
#endif /* STACK_DIRECTION == 0 */
 
#ifndef ALIGN_SIZE
#define ALIGN_SIZE      sizeof(double)
#endif
 
typedef union hdr
{
  char align[ALIGN_SIZE];
  struct
    {
      union hdr *next;
      char *deep;
    } h;
} header;
 
static header *last_alloca_header = NULL;
 
pointer
alloca (size)
     unsigned size;
{
  auto char probe;
  register char *depth = ADDRESS_FUNCTION (probe);
 
#if STACK_DIRECTION == 0
  if (STACK_DIR == 0)
    find_stack_direction ();
#endif
 
  {
    register header *hp;
 
    for (hp = last_alloca_header; hp != NULL;)
      if ((STACK_DIR > 0 && hp->h.deep > depth)
          || (STACK_DIR < 0 && hp->h.deep < depth))
        {
          register header *np = hp->h.next;
 
          free ((pointer) hp);
 
          hp = np;
        }
      else
        break;
 
    last_alloca_header = hp;
  }
 
  if (size == 0)
    return NULL;
 
  {
    register pointer new = malloc (sizeof (header) + size);
 
    ((header *) new)->h.next = last_alloca_header;
    ((header *) new)->h.deep = depth;
 
    last_alloca_header = (header *) new;
    return (pointer) ((char *) new + sizeof (header));
  }
}
 
char *
xmalloc (size)
     unsigned int size;
{
  char *result = (char *) malloc (size);
  if (result == 0)
    fprintf (stderr, "virtual memory exhausted");
  return result;
}
 
#endif /* no alloca */
#endif /* not GCC version 2 */
#endif
