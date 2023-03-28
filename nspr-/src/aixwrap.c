#include <errno.h>
#include <sys/select.h>

int _OS_SELECT(int width, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    int rv = select(width, r, w, e, t);
    if (rv < 0) {
	return -errno;
    }
    return rv;
}
