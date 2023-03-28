/*
** Copyright (c) 1995.  Netscape Communications Corporation.  All rights
** reserved.  This use of this Secure Sockets Layer Reference
** Implementation (the "Software") is governed by the terms of the SSL
** Reference Implementation License Agreement.  Please read the
** accompanying "License" file for a description of the rights granted.
** Any other third party materials you use with this Software may be
** subject to additional license restrictions from the licensors of such
** third party software and/or additional export restrictions.  The SSL
** Implementation License Agreement grants you no rights to any such
** third party material.
*/
#ifndef sslref_ssld_h_
#define sslref_ssld_h_

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifndef FD_SET
 #if defined(LINUX)
  #include <linux/posix_types.h>
  #include <linux/time.h>
 #else
  #include <sys/select.h>
 #endif                    /* LINUX */
#endif
#ifndef va_start
#include <varargs.h>
#endif

/* control protocol stuff */
struct ssld_proto {
	char cmd;
	char port[2];
	char addr[4];
};

#define SSLD_PROTO_LEN 7
#define SSLD_CONNECT 0
#define SSLD_GETPEER 1

/* Use ansi routine for FD op's */
#define bzero(a,b) memset(a,0,b)

typedef struct TodoStr Todo;
typedef struct SockStr Sock;

/*
** Sock maintains fd's for a clear connection and a secure connection
*/
struct SockStr {
    int clearfd;
    int sslfd;
    Sock *next;
};

struct TodoStr {
    int controlfd;
    unsigned char controlbuf[SSLD_PROTO_LEN];
    int cbufoff;
    Sock *sockets;
    Todo *next;
};

typedef struct ACLNodeStr ACLNode;

struct ACLNodeStr {
    char *name;
    ACLNode *next;
};

typedef struct ServiceStr Service;

struct ServiceStr {
    int port;
    int client;
    int auth;
    int action;

    int listen;

    struct sockaddr_in forward;
    char *file;
    char **avp;

    Todo *todo;

    ACLNode *acl;
    
    char *keyname;
    char *certname;
    Service *next;
};

typedef struct CallbackStr Callback;

struct CallbackStr {
    Sock *sock;
    Todo *todo;
    Service *service;
};

#define FORWARDER	1
#define EXECER		2

#define SSLD_CLIENT	0
#define SSLD_SERVER	1

extern int SEC_ERROR_BAD_PASSWORD;

typedef struct AccessNodeStr AccessNode;

struct AccessNodeStr {
    unsigned char *issuer;
    unsigned issuerlen;
    unsigned char *subject;
    unsigned subjectlen;

    AccessNode *next;
};

extern char *progName;

/* Printing and dealing with errors */
extern char *ErrorMessage(int err);

#ifdef __alpha
#else
extern void DebugPrint(char *va_alist, ...);
extern void Notice(char *va_alist, ...);
extern void Error(char *va_alist, ...);
extern void Fatal(char *va_alist, ...);
#endif

/* Check the Service's ACLNode. return 0 if OK, -1 if not */
extern int checkACL(char *name, Service *s);

/* This is from ssld_conf.c */
extern Service *ReadConfFile(char *fileName);

#endif /* sslref_ssld_h_ */
