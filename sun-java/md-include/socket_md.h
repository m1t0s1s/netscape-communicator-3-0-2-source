#ifndef sun_java_socket_md_h___
#define sun_java_socket_md_h___

#include "prosdep.h"
#include "prnetdb.h"

#if defined(XP_UNIX)

	#include <errno.h>
	#include <sys/socket.h>

	#if defined(SOLARIS) || defined(IRIX) || defined(OSF1)
		#include <sys/systeminfo.h>
	#endif

	#if defined(SOLARIS) || defined(UNIXWARE)
		#include <sys/filio.h>
	#endif

	#ifdef LINUX
		/* linux!=posix: FD_SET lives in linux/time.h */
		#include <sys/time.h>
	#endif

	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <arpa/nameser.h>
	#include <resolv.h>

#elif defined(XP_PC)

	#include <winsock.h>

	/*
	** The namespace terrorists up north are busy..busy..busy...
	*/
	#ifdef h_errno
	#undef h_errno
	#endif
	
	#define MAXHOSTNAMELEN          64
	
	/*
	** definitions for sysinfo()
	*/
	
	extern long sysinfo( int command, char *buffer, long count );
	
	#define SI_SYSNAME              1       /* return name of operating system */
	#define SI_HOSTNAME             2       /* return name of node */
	
	
	/*
	** Error codes
	*/
	
	#define EISCONN                 WSAEISCONN


#elif defined(XP_MAC)
	#include "macsock.h"
	
	#define MAXHOSTNAMELEN 255			// See OpenTptInternet.h - enum there
#endif

#if !defined(EPROTO) && !defined(XP_MAC)
	/* Defined in an enum on the mac */
	#define EPROTO -1
#endif

#ifndef MAXHOSTNAMELEN
	#define MAXHOSTNAMELEN	256
#endif

#endif
