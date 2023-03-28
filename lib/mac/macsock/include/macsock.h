#pragma once
// macsock.h
// Interface visible to xp code
// C socket type definitions and routines
// from sys/socket.h
#include <OpenTptInternet.h>	// All the internet typedefs
#include <utime.h>				// For timeval

struct sockaddr {
	unsigned char	sa_len;			/* total length */
	unsigned char	sa_family;		/* address family */
	char	sa_data[14];		/* actually longer; address value */
};

// from netinet/in.h
struct in_addr {
	unsigned long s_addr;
};

struct sockaddr_in {
	unsigned char	sin_len;
	unsigned char	sin_family;		// AF_INET
	unsigned short	sin_port;
	struct	in_addr sin_addr;
	char		sin_zero[8];
};

struct	hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	char	**h_addr_list;	/* list of addresses from name server */
#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
};


// Necessary network defines, found by grepping unix headers when XP code would not compile
#define FIONBIO 1
#define SOCK_STREAM 1
#define SOCK_DGRAM 		2
#define	EAFNOSUPPORT	ELASTERRNO+1	// Address family not supported by protocol family
#define EACCESS			ELASTERRNO+2	// Permission to create a socket of the specified type and/or protocol is denied.
#define EFAULT			14				// Name lookup produced an invalid address
#define IPPROTO_TCP 	INET_TCP		// Default TCP protocol
#define IPPROTO_UDP		INET_UDP		// Default UDP protocol
#define INADDR_ANY		kOTAnyInetAddress
#define SOL_SOCKET		XTI_GENERIC		// Any type of socket
#define SO_REUSEADDR	IP_REUSEADDR
#define MSG_PEEK		0x2				// Just look at a message waiting, donÕt actually read it.

// select support 
#define	NBBY	8
typedef unsigned long u_long;
typedef long	fd_mask;
#define NFDBITS	(sizeof(fd_mask) * NBBY)	/* bits per mask */

#ifndef howmany
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif
#define FD_SETSIZE 64
typedef	struct fd_set{
	fd_mask	fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= (1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= ~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & (1 << ((n) % NFDBITS)))
#define	FD_ZERO(p)	memset (p, 0, sizeof(*(p)))



#ifdef __cplusplus
extern "C" {
#endif

unsigned long inet_addr(const char *cp);
struct hostent *gethostbyname(char * name);
int gethostname (char *name, int namelen);
char *inet_ntoa(struct in_addr in);

inline unsigned long htonl(unsigned long hostlong) {return hostlong;}
inline unsigned long ntohl(unsigned long netlong) {return netlong;}
inline unsigned short ntohs(unsigned short netshort) {return netshort;}
inline unsigned short htons(unsigned short hostshort) {return hostshort;}


// UNIX look-alike routines
// They make sure that the arguments passed in are valid, and then
//
extern struct hostent *macsock_gethostbyaddr(const void *addr, int addrlen, int type);

extern int macsock_socket(int domain, int type, int protocol);
extern int macsock_ioctl(int sID, unsigned int request, void *value);
extern int macsock_connect(int sID, struct sockaddr *name, int namelen);
extern int macsock_write(int sID, const void *buffer, unsigned buflen);
extern int macsock_read(int sID, void *buf, unsigned nbyte);
extern int macsock_close(int sID);

extern int macsock_accept(int sID, struct sockaddr *addr, int *addrlen);
extern int macsock_bind(int sID, const struct sockaddr *name, int namelen);
extern int macsock_listen(int sID, int backlog);

extern int macsock_shutdown(int sID, int how);
extern int macsock_getpeername(int sID, struct sockaddr *name, int *namelen);
extern int macsock_getsockname(int sID, struct sockaddr *name, int *namelen);
extern int macsock_getsockopt(int sID, int level, int optname, void *optval,int *optlen);
extern int macsock_setsockopt(int sID, int level, int optname, const void *optval,int optlen);
extern int macsock_socketavailable(int sID, size_t *bytesAvailable);
extern int macsock_dup(int sID);

extern int macsock_send(int sID, const void *msg, int len, int flags);
extern int macsock_sendto(int sID, const void *msg, int len, int flags, struct sockaddr *toAddr, int toLen);
extern int macsock_recvfrom(int sID, void *buf, int len, int flags, struct sockaddr *from, int *fromLen);
extern int macsock_recv(int sID, void *buf, int len, int flags);

extern int select (int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);

#ifdef __cplusplus
}
#endif
//extern int errno;

/*
macsock_sendmsg
macsock_readv
macsock_writev
*/
