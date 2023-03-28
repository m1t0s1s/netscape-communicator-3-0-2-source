#ifndef MKTCP_H
#define MKTCP_H

#include "xp_sock.h"
#include "mkutils.h"

/* state machine data for host connect */
typedef struct _TCP_ConData TCP_ConData;  

/* socket buffer and socket buffer size
 * used for reading from sockets
 */
extern char * NET_Socket_Buffer;
extern int    NET_Socket_Buffer_Size;

extern int NET_InGetHostByName;  /* global semaphore */

/* the socks host in U long form
 */
extern u_long NET_SocksHost;
extern short NET_SocksPort;
extern char *NET_SocksHostName;

/* free any cached data in preperation
 * for shutdown or to free up memory
 */
extern void NET_CleanupTCP (void);

/* 
 * do a standard netwrite but echo it to stderr as well
 */
extern int 
NET_DebugNetWrite (int fildes, CONST void *buf, unsigned nbyte);

/* make sure the whole buffer is written in one call
 */
extern int32
NET_BlockingWrite (NETSOCK filedes, CONST void * buf, unsigned int nbyte);


/*  print an IP address from a sockaddr struct
 *
 *  This routine is only used for TRACE messages
 */
#ifdef XP_WIN
extern char *CONST
NET_IpString (struct sockaddr_in* sin);
#else
extern const char *
NET_IpString (struct sockaddr_in* sin);
#endif

/* return the local hostname
*/
#ifdef XP_WIN
extern char *CONST
NET_HostName (void);
#else
extern CONST char * 
NET_HostName (void);
#endif

/* free left over tcp connection data if there is any
 */
extern void NET_FreeTCPConData(TCP_ConData * data);

/*
 * Non blocking connect begin function.  It will most likely
 * return negative. If it does the NET_ConnectFinish() will
 * need to be called repeatably until a connect is established
 *
 * return's negative on error
 * return's MK_WAITING_FOR_CONNECTION when continue is neccessary
 * return's MK_CONNECTED on true connect
 */ 
extern int 
NET_BeginConnect  (CONST char   *url,
				   char         *ip_address_string,
				   char         *protocol,
				   int           default_port,
				   NETSOCK      *s, 
				   Bool       use_security, 
				   TCP_ConData **tcp_con_data, 
				   MWContext    *window_id,
				   char        **error_msg,
				   u_long        socks_host,
				   short         socks_port);


/*
 * Non blocking connect finishing function.  This routine polls
 * the socket until a connection finally takes place or until
 * an error occurs. NET_ConnectFinish() will need to be called 
 * repeatably until a connect is established.
 *
 * return's negative on error
 * return's MK_WAITING_FOR_CONNECTION when continue is neccessary
 * return's MK_CONNECTED on true connect
 */
extern int 
NET_FinishConnect (CONST char   *url,
				   char         *protocol,
				   int           default_port,
				   NETSOCK      *s,  
				   TCP_ConData **tcp_con_data, 
				   MWContext    *window_id,
				   char        **error_msg);

/* 
 * Echo to stderr as well as the socket
 */
extern int 
NET_DebugNetRead (NETSOCK fildes, void *buf, unsigned nbyte);

/* this is a very standard network write routine.
 * 
 * the only thing special about it is that
 * it returns MK_HTTP_TYPE_CONFLICT on special error codes
 *
 */
extern int 
NET_HTTPNetWrite (NETSOCK fildes, CONST void * buf, unsigned nbyte);

/* 
 *
 * trys to get a line of data from the socket or from the buffer
 * that was passed in
 */
extern int NET_BufferedReadLine(NETSOCK sock, 
				char ** line, 
				char ** buffer, 
				int32 * buffer_size, 
				Bool * pause_for_next_read);


extern void NET_SanityCheckDNS (MWContext *context);

#endif   /* MKTCP_H */
