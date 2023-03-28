/* public interfaces for the ssld utility library */
#ifndef _SSLD_UTIL_H_

int ssld_connect(int ctrlfd, int s, struct sockaddr *name, int namelen);
int ssld_getpeername(int ctrlfd, struct sockaddr *name, int *retnamelen);
int Spin_Read(int fd, void *buf, int len);

struct ssld_proto {
	char cmd;
	char port[2];
	char addr[4];
};

#define SSLD_PROTO_LEN 7 /* dont trust compiler padding */

#define SSLD_CONNECT 0
#define SSLD_GETPEER 1

#define _SSLD_UTIL_H_
#endif /* _SSLD_UTIL_H_ */

