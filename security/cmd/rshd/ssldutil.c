#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <syslog.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "ssldutil.h"

#define MAKE_IN_ADDR(a,b,c,d) \
	htonl(((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

int
ssld_connect(int ctrlfd, int s, struct sockaddr *name, int namelen)
{
	struct ssld_proto controlbuf;
	struct sockaddr_in ssld_sin, *addr;

	addr = (struct sockaddr_in *)name;

	/* write real dest to ssld */
	controlbuf.cmd = SSLD_CONNECT;
	memcpy(&controlbuf.port, &addr->sin_port, sizeof(controlbuf.port));
	memcpy(&controlbuf.addr, &addr->sin_addr.s_addr,
						sizeof(controlbuf.addr));
	write(ctrlfd, &controlbuf, SSLD_PROTO_LEN);

	/* set up address of ssld */
	ssld_sin.sin_addr.s_addr = MAKE_IN_ADDR(127,0,0,1);
	ssld_sin.sin_family = AF_INET;

	/* read port number to connect to ssld */
	read(ctrlfd, &controlbuf, SSLD_PROTO_LEN);
	memcpy(&ssld_sin.sin_port, &controlbuf.port, sizeof(controlbuf.port));

	/* connect to ssld */
	return( connect(s, (struct sockaddr *)&ssld_sin, sizeof (ssld_sin)) );
}

int
ssld_getpeername(int ctrlfd, struct sockaddr *name, int *retnamelen)
{
	struct ssld_proto controlbuf;
	struct sockaddr_in *sin;

	sin = (struct sockaddr_in *)name;

	/* write get peer command */
	controlbuf.cmd = SSLD_GETPEER;
	write(ctrlfd, &controlbuf, SSLD_PROTO_LEN);

	/* read peer address back */
	Spin_Read(ctrlfd, &controlbuf, SSLD_PROTO_LEN);
	memcpy(&sin->sin_port, &controlbuf.port, sizeof(controlbuf.port));
	memcpy(&sin->sin_addr.s_addr, &controlbuf.addr,
						sizeof(controlbuf.addr));
	sin->sin_family = AF_INET;

	*retnamelen = sizeof(struct sockaddr_in);
	syslog(LOG_ERR, "getpeername done\n");

	return( 0 );
}

/*
** Spin reading until buffer is filled up with requested amount of data
*/
int Spin_Read(int fd, void *buf, int len)
{
    int count = 0;
    while (len) {
        int nb = read(fd, buf, len);
        if (nb < 0) {
            if ((errno == EINTR) || (errno == EAGAIN)) continue;
            return nb;
        }
        if (nb == 0) break;
        count += nb;
        len -= nb;
        buf = (void*) (((char*)buf) + nb);
    }
    return count;
}

