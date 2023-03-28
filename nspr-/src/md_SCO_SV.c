#include "prtypes.h"
#include "prosdep.h"
#include "prthread.h"
#include "prlog.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include <ieeefp.h>
#include <sys/select.h>

/*
 * Select doesn't work on NFS files.
 */
#define NFS_SELECT_BROKEN

#include "mdint.h"
void _MD_InitOS(int when)
{
    _MD_InitUnixOS(when);

    if (when == _MD_INIT_READY) {
	fpsetmask( 0 );
    }
}

prword_t *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) sigsetjmp(CONTEXT(t),1);
    }
    *np = sizeof(CONTEXT(t)) / sizeof(prword_t);
    return (prword_t*) CONTEXT(t);
}

/************************************************************************/

int _OS_OPEN(const char *path, int oflag, mode_t mode)
{
    int rv = open(path, oflag, mode);
    return (rv < 0 ? -errno : rv);
}

int _OS_CLOSE(int fd)
{
    int rv = close(fd);
    return (rv < 0 ? -errno : rv);
}

int _OS_READ(int fd, void *buf, unsigned nbyte)
{
    int rv;
    rv = read(fd, buf, nbyte);
    return (rv < 0 ? -errno : rv);
}


int _OS_WRITE(int fd, void *buf, unsigned nbyte)
{
    int rv;
    rv = write(fd, buf, nbyte);
    return (rv < 0 ? -errno : rv);
}

int _OS_SOCKET(int domain, int type, int flags)
{
    int rv;

    rv = socket(domain, type, flags);
    return (rv < 0 ? -errno : rv);
}

int _OS_CONNECT(int fd, void *addr, int addrlen)
{
    int rv;

    rv = connect(fd, addr, addrlen);
    return (rv < 0 ? -errno : rv);
}

int _OS_RECV(int s, char *buf, int len, int flags)
{
    int rv;

    rv = recv(s, buf, len, flags);
    return (rv < 0 ? -errno : rv);
}

int _OS_SEND(int s, const char *msg, int len, int flags)
{
    int rv;

    rv = send(s, msg, len, flags);
    return (rv < 0 ? -errno : rv);
}

int _OS_FCNTL(int filedes, int cmd, int arg)
{
    int rv;
    rv = fcntl(filedes, cmd, arg);
    return (rv < 0 ? -errno : rv);
}

int _OS_IOCTL(int fd, int tag, int arg)
{
    int rv;

    rv = ioctl(fd, tag, arg);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

#ifndef NFS_SELECT_BROKEN
int _OS_SELECT(int fd, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    int rv;
    rv = _R_OS_SELECT(fd, r, w, e, t);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}

#else /* NFS_SELECT_BROKEN */
/*
 * A select system call on an NFS file does NOT work! :-(
 * This routine is a hack around that problem.  If we get a
 * failure based on invalid argument, we will look thru the
 * lists of file descriptors, remove any NFS files and try
 * the select again.  If it succeeds we will add the NFS
 * files to the list as ready to be serviced since they
 * probably are.  We will not support exceptional conditions
 * on NFS files.  There will not be any.
 *
 * Many thanks to donwo@sco.com for actually writting all this
 * ugly code.
 */

#include <sys/stat.h>
#include <sys/statfs.h>
#include <sys/fstyp.h>

static void RestoreNfsFds();

int
_OS_SELECT(int nfds, fd_set *ReadFds, fd_set *WriteFds, fd_set *ExceptFds,
						    struct timeval *TimeOut)
{
    int			ReadyFdCnt;
    fd_set		NfsReadFds;
    fd_set		NfsWriteFds;
    fd_set		NfsExceptFds;
    struct timeval	PollTimer;
    int			RwCnt;
    int			ExcptCnt;


    ReadyFdCnt = _R_OS_SELECT(nfds, ReadFds, WriteFds, ExceptFds, TimeOut);
    if ((ReadyFdCnt != -1) || (errno != EINVAL))
	return(ReadyFdCnt);

    RwCnt =	DeleteNfsFds(nfds, ReadFds, &NfsReadFds) +
		DeleteNfsFds(nfds, WriteFds, &NfsWriteFds);
    ExcptCnt =	DeleteNfsFds(nfds, ExceptFds, &NfsExceptFds);
    if (!RwCnt && !ExcptCnt)
	return(-1);		/* Nothing NFS */

    if (!RwCnt)
	PollTimer = *TimeOut;	/* The error was probably NFS exceptions */
    else
    {
	PollTimer.tv_sec = 0;
	PollTimer.tv_usec = 0;
    }

    ReadyFdCnt = _R_OS_SELECT(nfds, ReadFds, WriteFds, ExceptFds, &PollTimer);
    if (ReadyFdCnt == -1)
	return(ReadyFdCnt);		/* Cannot fix this error */

    RestoreNfsFds(nfds, ReadFds, &NfsReadFds);
    RestoreNfsFds(nfds, WriteFds, &NfsWriteFds);
    return(ReadyFdCnt + RwCnt);
}

static int
IsNfs(int Fd)
{
    struct stat		StatBuf;
    struct statfs	FsBuf;
    static int		NfsIndex = -1;
    int	fstatfs(int , struct statfs *, int , int);
    int	sysfs(int opcode, char *Name);

    if (fstat(Fd, &StatBuf) ||
	!S_ISREG(StatBuf.st_mode) ||
	fstatfs(Fd, &FsBuf, sizeof(FsBuf), 0))
	    return(0);			/* It is (probably) not an NFS file */

    if ((NfsIndex < 0) &&
	((NfsIndex = sysfs(GETFSIND, "NFS")) < 0))
	    return(0);

    return((int)FsBuf.f_fstyp == NfsIndex);
}

static int
DeleteNfsFds(int nfds, fd_set *UsrSet, fd_set *NfsSet)
{
    int		NfsCnt = 0;
    int		Fd;


    if (!UsrSet || !NfsSet)
	return(NfsCnt);

    FD_ZERO(NfsSet);
    for (Fd = 0; Fd < nfds; ++Fd)
	if (FD_ISSET(Fd, UsrSet) && IsNfs(Fd))
	{
	    FD_SET(Fd, NfsSet);
	    FD_CLR(Fd, UsrSet);
	    ++NfsCnt;
	}

    return(NfsCnt);
}

static void
RestoreNfsFds(int nfds, fd_set *UsrSet, fd_set *NfsSet)
{
    int		Fd;


    if (!UsrSet || !NfsSet)
	return;

    for (Fd = 0; Fd < nfds; ++Fd)
	if (FD_ISSET(Fd, NfsSet))
	    FD_SET(Fd, UsrSet);
}
#endif
#ifdef SCO_PM

#include <stdio.h>
#include <signal.h>
#include <syslog.h>
#include <locale.h>
#include <iforpmapi.h>
#include <licenseIDs.h>

/* #include "./lic_msg.h"*/
#ifndef _H_LIC_MSG
#define _H_LIC_MSG
#include <limits.h>
#include <nl_types.h>
#define MF_LIC "lic.cat@tcp"
#ifndef MC_FLAGS
#define MC_FLAGS NL_CAT_LOCALE
#endif
#ifndef MSGSTR
extern char *catgets_safe(nl_catd, int, int, char *);
#ifdef lint
#define MSGSTR(num,str) (str)
#define MSGSTR_SET(set,num,str) (str)
#else
#define MSGSTR(num,str) catgets_safe(catd, MS_LIC, (num), (str))
#define MSGSTR_SET(set,num,str) catgets_safe(catd, (set), (num), (str))
#endif
#endif
/* The following was generated from */
/* lic.gen */
#define MS_NET 1
#define NET_WRN 1
#define NET_PMD_FATAL_REQ 2
#define NET_PMD_ERR_REQ 3
#define NET_PMD_FATAL_INIT 4
#define NET_PMD_ERR_INIT 5
#define NET_PMD_SESS_TERM 6
#define NET_PMD_NO_LIC 7
#define NET_ALLOW_CONT 8
#endif /* _H_LIC_MSG */

#ifdef MCC_HTTPD
#include "base/ereport.h"
#endif

static nl_catd  catd;

static sigset_t timer_set, oldset;
static int first_time = 1;

void PR_DisableClock()
{
    if (first_time) {
	sigemptyset(&timer_set);
	sigaddset(&timer_set, SIGALRM);
	first_time = 0;
    }
    sigprocmask(SIG_BLOCK, &timer_set, &oldset);
}

void PR_EnableClock()
{
    sigprocmask(SIG_SETMASK, &oldset, 0);
}

/*
 * sco_license_error
 */
void
sco_license_error(int level, char* fmt, char* msg)
{
#ifdef MCC_HTTPD
  syslog(level, fmt, msg);
  ereport(level, fmt, msg);
#else
  syslog(level, fmt, msg);
  fprintf(stderr, fmt, msg);
  fprintf(stderr, "\n");
#endif
}


void
net_pm_signal_handler( 
char	*msg,
unsigned int flags, 
ifor_pm_trns_hndl_t handle,
char	*vmsg)
{
	if (flags & IFOR_PM_VND_MSG) {
		sco_license_error(LOG_INFO, "%s", vmsg);
	}
	if (flags & IFOR_PM_WARN) {
		sco_license_error(LOG_WARNING, MSGSTR_SET(MS_NET, NET_WRN, 
		    "Warning: %s"),msg);
	}
	if (flags & IFOR_PM_CONTINUE)
		return;
	if (flags & IFOR_PM_EXIT) 
	{
		sco_license_error(LOG_ERR, MSGSTR_SET(MS_NET, NET_PMD_NO_LIC, 
		    "Failed to obtain a license for this program."), NULL);
		exit(1); 
	}
	if (flags & IFOR_PM_EXIT)
	{
		sco_license_error(LOG_ERR, MSGSTR_SET(MS_NET, NET_PMD_SESS_TERM, 
		    "Program terminated by policy manager."), NULL);
		exit(2);
	}
	return;
}


/*
 * request_license
 * version is set to "everest" as per Sco's instructions.
 * It's some sort of work-around for their license manager.
 */
void request_license(int p_id, char *progname)
{
	int	status, n;
	unsigned long prod_id;
	char 	*version = "everest";
	ifor_pm_trns_hndl_t handle;
	int res;

        PR_DisableClock();
#ifdef INTL
	setlocale(LC_ALL, "");
	catd = catopen (MF_LIC,MC_FLAGS);
	openlog(progname, LOG_PID, LOG_DAEMON);
#endif

	/* Contact the license server.
 	 * We set up a handler, and perhaps a signal, that the license
 	 * server can use to contact us.
 	 */

	res = ifor_pm_init_sco(net_pm_signal_handler,IFOR_PM_NO_SIGNAL);

	switch (res)
	{
	case IFOR_PM_OK :
	case IFOR_PM_REINIT :		
		break;
	case IFOR_PM_BAD_PARAM :
		sco_license_error(LOG_WARNING, MSGSTR_SET(MS_NET, NET_PMD_ERR_INIT,
		    "An error has occurred during a call to initialize the license server."), NULL);
		break;
	case IFOR_PM_FATAL :
		sco_license_error(LOG_ERR, MSGSTR_SET(MS_NET, NET_PMD_FATAL_INIT,
		    "fatal error while trying to initialize license server."), NULL);
		exit(3);
	default :			
		break;
	}

	/* Try to grab a license */
	res = ifor_pm_request_sco(p_id,version,IFOR_PM_SYNC,&handle);

	switch(res)
	{
	case IFOR_PM_OK :		
		break;
	case IFOR_PM_BAD_PARAM :
		sco_license_error(LOG_WARNING, MSGSTR_SET(MS_NET, NET_PMD_ERR_REQ,
		    "An error has occurred during a call to request a license."), NULL);
		sco_license_error(LOG_WARNING, MSGSTR_SET(MS_NET, NET_ALLOW_CONT,
		    "Allowing session to continue."), NULL);
		break;
	case IFOR_PM_FATAL :
		sco_license_error(LOG_ERR, MSGSTR_SET(MS_NET, NET_PMD_FATAL_REQ,
		    "fatal error while trying to request license."), NULL);
		exit(4);
	default :			
		break;
	}

	PR_EnableClock();

}
#endif
