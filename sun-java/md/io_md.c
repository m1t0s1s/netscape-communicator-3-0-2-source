#include "sys_api.h"
#include "io_md.h"
#include "fd_md.h"
#include "jio.h"

#ifdef XP_UNIX
#include <sys/stat.h>
#include <sys/ioctl.h>
#endif

#ifdef XP_PC
#include <sys/stat.h>
#endif

#ifdef XP_MAC
#include <fcntl.h>
#include <unistd.h>
#include <stat.h>
#endif

#if defined(SOLARIS) || defined(UNIXWARE)
#include <sys/filio.h>
#endif

#ifdef SCO_SV
#include <sys/socket.h>
#endif

#define IO_EXCEPTION "java/io/IOException"

static char *security_exception = JAVAPKG "SecurityException";
static char *no_stdin = "stdin is disabled";
static char *no_stdout = "stdout/stderr is disabled";

static int java_io_mode = JIO_NONE;

void set_java_io_mode(int new_mode)
{
    java_io_mode = new_mode;
}

/************************************************************************/

/*
** XXX Still need per fd monitors so that we can synchronize all access to
** XXX them including closing.
*/

void sysInitFD(Classjava_io_FileDescriptor *fd, int fdnum)
{
    if ((fdnum < 0) || (fdnum > 2)) {
	SignalError(0, JAVAPKG "SecurityException", 0);
	return;
    }
    fd->fd = fdnum+1;
}

int sysOpenFD(Classjava_io_FileDescriptor *fd, const char *name,
	      int flags, int mode)
{
    int fdnum;
    int rdwr = mode & 3;

    if (((rdwr == O_RDONLY) || (rdwr == O_RDWR)) &&
	!(java_io_mode & JIO_ALLOW_FILE_INPUT)) {
	SignalError(0, security_exception, "file input is prohibited");
	return -1;
    } else if (((rdwr == O_WRONLY) || (rdwr == O_RDWR)) &&
	!(java_io_mode & JIO_ALLOW_FILE_OUTPUT)) {
	SignalError(0, security_exception, "file output is prohibited");
	return -1;
    }
    fdnum = sysOpen(name, flags, mode);
    if (fdnum != -1) {
	fd->fd = fdnum + 1;
    }
    return (fdnum != -1) ? fdnum + 1 : fdnum;
}

int sysCloseFD(Classjava_io_FileDescriptor *fd)
{
    int fdnum = (int) fd->fd - 1;
    
	/* 
	** fdnum is -1 if the FileDescriptor was already closed...
	** Never ever close stdin, stdout or stderr 
	*/
    if ( (fdnum < 0) || ((fdnum >= 0) && (fdnum <= 2)) ) {
	return -1;
    }
    fd->fd = 0;
    return sysClose(fdnum);
}

int sysCloseSocketFD(Classjava_io_FileDescriptor *fd)
{
    int fdnum = fd->fd - 1;

	/* 
	** fdnum is -1 if the FileDescriptor was already closed...
	** Never ever close stdin, stdout or stderr 
	*/
#ifdef XP_MAC
	// On a Mac, our FDs start at 0 for sockets.
	if (fdnum < 0)
		return -1;
#else
    if ( (fdnum < 0) || ((fdnum >= 0) && (fdnum <= 2)) )
		return -1;
#endif
  
    fd->fd = 0;
    return sysCloseSocket(fdnum);
}

int sysReadFD(Classjava_io_FileDescriptor *fd, char *buf, int32_t nbytes)
{
    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    if ((fdnum == 1) || (fdnum == 2)) {
	/* Not allowed to read on stdout/stderr */
	return -1;
    }
    if ((fdnum == 0) && !(java_io_mode & JIO_ALLOW_STDIN)) {
	SignalError(0, security_exception, no_stdin);
	return -1;
    }
    return sysRead(fdnum, buf, nbytes);
}

int sysWriteFD(Classjava_io_FileDescriptor *fd, char *buf, int32_t nbytes)
{
    int fdnum = (int)fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    if (fdnum == 0) {
	/* Not allowed to write on stdin */
	return -1;
    }
    if (((fdnum == 1) || (fdnum == 2)) && !(java_io_mode & JIO_ALLOW_STDOUT)) {
	SignalError(0, security_exception, no_stdout);
	return -1;
    }
 #if defined(XP_PC) && !defined(_WIN32)
    if ( (fdnum == 1) || (fdnum == 2) ) {
        /*
        ** Hack this for win16, because there really is no "console" as in
        ** Unix or Win32. Its okay that this is a little slow, because
        ** its going to stdout/stderr, instead of a real file.
        */
        int32_t transferred = 0;		
        while (transferred < nbytes) {
            char transfer[33];
            int32_t toMove;
            
            toMove = nbytes - transferred;
            if (toMove > 32) {
                toMove = 32;
            }
            strncpy(transfer, buf, (int) toMove);
            transfer[toMove] = '\0';
            printf(transfer);
            transferred += toMove;
            buf += toMove;
        }
        return (int) nbytes;
    } 
    else {
        return sysWrite(fdnum, buf, nbytes);
    }
 #else
    return sysWrite(fdnum, buf, nbytes);
 #endif
}

/* XXX long* pbytes probably wrong on OSF1 */
int sysAvailableFD(Classjava_io_FileDescriptor *fd, long *pbytes)
{
    off_t cur, end;

    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return 0;
    }
    if ((fdnum == 0) && !(java_io_mode & JIO_ALLOW_STDIN)) {
	SignalError(0, security_exception, no_stdin);
	return 0;
    }
    if (((fdnum == 1) || (fdnum == 2)) && !(java_io_mode & JIO_ALLOW_STDOUT)) {
	SignalError(0, security_exception, no_stdout);
	return 0;
    }

#ifdef XP_UNIX
    {
	struct stat stbuf;
	if (fstat(fdnum, &stbuf) >= 0) {
	    int mode = stbuf.st_mode;
	    if (S_ISCHR(mode) || S_ISFIFO(mode) || S_ISSOCK(mode)) {
		if (ioctl(fdnum, FIONREAD, pbytes) >= 0) {
		    return 1;
		}
	    }
	}
    }
#endif

    if ((cur = lseek(fdnum, 0L, SEEK_CUR)) == -1) {
        return 0;
    } else if ((end = lseek(fdnum, 0L, SEEK_END)) == -1) {
        return 0;
    } else if (lseek(fdnum, cur, SEEK_SET) == -1) {
        return 0;
    }
    *pbytes = end - cur;
    return 1;
}

long sysLseekFD(Classjava_io_FileDescriptor *fd, long offset, long whence)
{
    int fdnum = (int) fd->fd - 1;
    if (fdnum == -1) {
	return -1;
    }
    if ((fdnum == 0) && !(java_io_mode & JIO_ALLOW_STDIN)) {
	SignalError(0, security_exception, no_stdin);
	return -1;
    }
    if (((fdnum == 1) || (fdnum == 2)) && !(java_io_mode & JIO_ALLOW_STDOUT)) {
	SignalError(0, security_exception, no_stdout);
	return -1;
    }
    return sysSeek(fdnum, offset, whence);
}


