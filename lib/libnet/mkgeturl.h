
#ifndef MKGETURL_H
#define MKGETURL_H

#include "mkutils.h"
#include "xp.h"
#include "mktcp.h"
#include "nslocks.h"

/* Debugging routine prints an URL (and string "header")
 */
#ifdef DEBUG
extern void TraceURL (URL_Struct *url, char *header);
#else
#define TraceURL(U,M)
#endif /* DEBUG */

/* structure for maintaining multiple active data transfers
 */
typedef struct _ActiveEntry {
    URL_Struct * URL_s;           /* the URL data */
    int          status;      	  /* current status */
    int32        bytes_received;  /* number of bytes received so far */
    NETSOCK      socket;          /* data sock */
    NETSOCK      con_sock;        /* socket waiting for connection */
    Bool      local_file;      /* are we reading a local file */
	Bool      memory_file;     /* are we reading from memory? */
    int          protocol;        /* protocol used for transfer */
    void       * con_data;        /* data about the transfer connection and status */
                                  /* routine to call when finished */
    Net_GetUrlExitFunc *exit_routine;
    MWContext  * window_id;       /* a unique window id */
    FO_Present_Types format_out;  /* the output format */

	NET_StreamClass	* save_stream; /* used for cacheing of partial docs
									* The file code opens this stream
									* and writes part of the file down it.
									* Then the stream is saved
									* and the rest is loaded from the
									* network
									*/
    Bool      busy;

    char *    proxy_conf;	/* Proxy autoconfig string */
    char *    proxy_addr;	/* Proxy address in host:port format */
    u_long    socks_host;	/* SOCKS host IP address */
    short     socks_port;	/* SOCKS port number */

} ActiveEntry;

extern int NET_TotalNumberOfOpenConnections;
extern int NET_MaxNumberOfOpenConnections;
extern CacheUseEnum NET_CacheUseMethod;
extern time_t NET_StartupTime;  /* time we began the program */
extern void NET_NTrace(char *msg, int32 length); 
extern XP_Bool NET_ProxyAcLoaded;
/*
 * Silently Interrupts all transfers in progress that have the same
 * window id as the one passed in.
 */
extern int NET_SilentInterruptWindow(MWContext * window_id);


#endif /* not MKGetURL_H */
