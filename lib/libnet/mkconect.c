/* -*- Mode: C; tab-width: 4 -*-
   generalized portable TCP routines
 *
 */

#include "mkutils.h"

#define RANDOM_DNS_HOST_PICK

#if defined(MOZILLA_CLIENT) || defined(LIVEWIRE)
#define ASYNC_DNS
#endif

#include "mktcp.h"
#include "mkparse.h"
#include "mkgeturl.h"  /* for error codes */
#include "fe_proto.h" /* for externs */
#ifdef MOZILLA_CLIENT
#include "secnav.h"
#endif /* MOZILLA_CLIENT */
#include "merrors.h"
#if defined(XP_UNIX) || defined(XP_WIN32)
#include "prnetdb.h"
#else
#define PRHostEnt struct hostent
#define PR_NETDB_BUF_SIZE 5
#endif
#include "ssl.h"

#ifdef MCC_PROXY
#include "base/session.h"
#include "frame/req.h"
#include "frame/httpact.h"
#endif /* MCC_PROXY */

#if defined(SOLARIS)
extern int gethostname(char *, int);
#endif

#ifndef NADA_VERSION
/* define this to turn security on
 */
/* #define TURN_SECURITY_ON */  /* jwz */
#endif

#ifdef XP_UNIX
/* #### WARNING, this crap is duplicated in mksockrw.c
 */
# include <sys/ioctl.h>
/*
 * mcom_db.h is only included here to set BYTE_ORDER !!!
 * MAXDNAME is pilfered right out of arpa/nameser.h.
 */
# include "mcom_db.h"

# if defined(__hpux) || defined(_HPUX_SOURCE)
#  define BYTE_ORDER BIG_ENDIAN
#  define MAXDNAME        256             /* maximum domain name */
# else
#  include <arpa/nameser.h>
# endif

#include <resolv.h>

#if !defined(__osf__) && !defined(AIXV3) && !defined(_HPUX_SOURCE) && !defined(__386BSD__) && !defined(__linux) && !defined(SCO_SV)
#include <sys/filio.h>
#endif

#endif /* XP_UNIX */

#include "xp_error.h"

/* for XP_GetString() */
#include "xpgetstr.h"
extern int MK_CONNECTION_REFUSED;
extern int MK_CONNECTION_TIMED_OUT;
extern int MK_OUT_OF_MEMORY;
extern int MK_UNABLE_TO_CONNECT;
extern int MK_UNABLE_TO_CREATE_SOCKET;
extern int MK_UNABLE_TO_LOCATE_HOST;
extern int XP_ERRNO_EALREADY;
extern int XP_ERRNO_ECONNREFUSED;
extern int XP_ERRNO_EINPROGRESS;
extern int XP_ERRNO_EISCONN;
extern int XP_ERRNO_ETIMEDOUT;
extern int XP_ERRNO_EWOULDBLOCK;
extern int XP_PROGRESS_CONTACTHOST;
extern int XP_PROGRESS_LOOKUPHOST;
extern int XP_PROGRESS_NOCONNECTION;
extern int XP_PROGRESS_UNABLELOCATE;


#ifdef PROFILE
#pragma profile on
#endif

typedef enum {
	NET_TCP_FINISH_DNS_LOOKUP,
	NET_TCP_FINISH_CONNECT
} TCPStatesEnum;

struct _TCP_ConData {
	TCPStatesEnum       next_state;  /* states of the machine */
	struct sockaddr_in  sin;
	XP_Bool            	use_security;
	time_t              begin_time;
	int					random_host_pick;	/* -1 if there is only
											 * one entry and
											 * positive if one was picked
											 */
	int					random_start;       /* first random pick */
};

/* MAX_DNS_LIST_SIZE controls the cache of resolved hosts.
 * If it is undefined, the cache will grow without bound.
 * If it is defined and >0, then the cache will be limited
 * to that many entries.  If it is 0, there will be no cache.
 */
#if defined(XP_WIN) || defined(XP_UNIX)   /* this fucks up windows, an Unix
											 doesn't need it; make sure it is
											 zero */
# define MAX_DNS_LIST_SIZE 0
#else
# define MAX_DNS_LIST_SIZE 10
#endif /* XP_WIN || XP_UNIX */

/* Global TCP connect variables
 */
PUBLIC u_long NET_SocksHost=0;
PUBLIC short NET_SocksPort=0;

PUBLIC char * NET_SocksHostName=0;

PUBLIC int NET_InGetHostByName = FALSE; /* global semaphore */

/* Use this global variable to
 * cache at least one IP address
 * Maybe in the future I will cache
 * a whole list of numeric IP's but
 * for now just one. :(
 */
typedef struct _DNSEntry {
    char * hostname;
    char * haddr;
    int    h_length;
	int    random_host_pick;
	int    random_start;
} DNSEntry;
   
PRIVATE char   *  net_local_hostname=0;     /* The name of this host */
PRIVATE XP_List * dns_list=0;

#define DEFAULT_TCP_CONNECT_TIMEOUT 35  /* seconds */
PRIVATE uint32 net_tcp_connect_timeout = DEFAULT_TCP_CONNECT_TIMEOUT;/*seconds*/

/* set the number of seconds for the tcp connect timeout
 *
 * this timeout is used to end connections that do not
 * timeout on there own
 */
PUBLIC void
NET_SetTCPConnectTimeout(uint32 seconds)
{
	net_tcp_connect_timeout = seconds;
}

/* socks support function
 *
 * if set to NULL socks is off.  If set to non null string the string will be
 * used as the socks host and socks will be used for all connections.
 *
 * returns 0 if the numonic hostname given is invalid or gethostbyname
 * returns host unknown
 *
 */
int NET_SetSocksHost(char * host)
{
	XP_Bool is_numeric_ip;
	char *host_cp;

	if(host && *host)
	  {
		char *cp;
		XP_LTRACE(MKLib_trace_flag,1,("Trying to set Socks host: %s\n", host));

		cp = XP_STRRCHR(host, ':');
		if (cp) 
		  {
			*cp = 0;
			NET_SocksPort = atoi(cp+1);
		  }

		is_numeric_ip = TRUE; /* init positive */
		for(host_cp = host; *host_cp; host_cp++)
		    if(!XP_IS_DIGIT(*host_cp) && *host_cp != '.')
			  {
				is_numeric_ip = FALSE;
				break;
			  }

        if (is_numeric_ip)  /* Numeric node address */
          {
		    TRACEMSG(("Socks host is numeric"));

            /* some systems return a number instead of a struct
             */
            NET_SocksHost = inet_addr(host);
			if (NET_SocksHostName) XP_FREE (NET_SocksHostName);
            NET_SocksHostName = XP_STRDUP(host);
          }
        else      /* name not number */
          {
    	    PRHostEnt hpbuf, *hp;
			char dbbuf[PR_NETDB_BUF_SIZE];

			NET_InGetHostByName++; /* global semaphore */
#ifdef MCC_PROXY
			{
				extern Request *glob_rq;
				extern Session *glob_sn;
				Request *rq = glob_rq;
				Session *sn = glob_sn;
				hp = servact_gethostbyname(host, sn, rq);
			}
#else
#if defined(XP_UNIX) || defined(XP_WIN32)
    	    hp = PR_gethostbyname(host, &hpbuf, dbbuf, sizeof(dbbuf), 0);
#else
    	    hp = gethostbyname(host);
#endif
#endif
			NET_InGetHostByName--; /* global semaphore */

            if (!hp)
              {
                XP_LTRACE(MKLib_trace_flag,1,("mktcp.c: Can't find Socks host name `%s'\n", host));
                NET_SocksHost = 0;
				if (NET_SocksHostName) XP_FREE (NET_SocksHostName);
				NET_SocksHostName = 0;
		        XP_LTRACE(MKLib_trace_flag,1,("Socks host is bad. :(\n"));
				if (cp) {
				  *cp = ':';
				}
                return 0;  /* Fail? */
              }
			XP_MEMCPY(&NET_SocksHost, hp->h_addr, hp->h_length);
          }
		if (cp) {
			*cp = ':';
		}
	  }
	else
	  {
        NET_SocksHost = 0;
		if (NET_SocksHostName) XP_FREE (NET_SocksHostName);
		NET_SocksHostName = NULL;
		XP_LTRACE(MKLib_trace_flag,1,("Clearing Socks Host\n"));
	  }

    return(1);
}

/* net_CacheDNSEntry
 *
 * caches the results of a dns lookup for fast
 * retrieval
 */
PRIVATE void
net_CacheDNSEntry(char * hostname, 
				  PRHostEnt * host_pointer,
				  int random_host_pick,
				  int random_start)
{
#if defined(MAX_DNS_LIST_SIZE) && (MAX_DNS_LIST_SIZE == 0)

    /* If MAX_DNS_LIST_SIZE is defined, and 0, don't cache anything. */
    return;

#else /* !defined(MAX_DNS_LIST_SIZE) || (MAX_DNS_LIST_SIZE > 0) */

	/* Otherwise, MAX_DNS_LIST_SIZE is either the number of entries to
	   cache, or is undefined, meaning "unlimited." */

    DNSEntry * new_entry = XP_NEW(DNSEntry);
    char * new_haddr = (char *) XP_ALLOC(host_pointer->h_length);

    if(!new_entry || !new_haddr)
	return;

    if(!dns_list)
	dns_list = XP_ListNew();

    if(!dns_list || !new_entry)
	return;  /* malloc error */

    /* create and add this entry
     */
    new_entry->hostname = 0;
    StrAllocCopy(new_entry->hostname, hostname);

    XP_MEMCPY(new_haddr, host_pointer->h_addr, host_pointer->h_length);
    new_entry->haddr = new_haddr;
    new_entry->h_length = host_pointer->h_length;
	new_entry->random_host_pick = random_host_pick;
	new_entry->random_start = random_start;

    /* add
     */
    XP_ListAddObject(dns_list, new_entry);

# ifdef MAX_DNS_LIST_SIZE
    /* check to make sure the list is not overflowing the maximum size. */
	XP_ASSERT(MAX_DNS_LIST_SIZE > 0);
    if(XP_ListCount(dns_list) > MAX_DNS_LIST_SIZE)
      {
	    DNSEntry * first_entry;
	    first_entry = (DNSEntry *) XP_ListRemoveEndObject(dns_list);
	    if(first_entry)
	      {
	        FREEIF(first_entry->hostname);
	        FREEIF(first_entry->haddr);
	        XP_FREE(first_entry);
	      }
      }
# endif /* defined(MAX_DNS_LIST_SIZE) */

#endif  /* !defined(MAX_DNS_LIST_SIZE) || (MAX_DNS_LIST_SIZE > 0) */

    return;
}

/* net_CheckDNSCache
 *
 * checks the list of cached dns entries and returns
 * the dns info in the form of struct hostent if a match is
 * found for the passed in hostname
 */
PRIVATE DNSEntry *    
net_CheckDNSCache(CONST char * hostname)	
{
    XP_List * list_obj = dns_list;
    DNSEntry * dns_entry;

    if(!hostname)
	return(0);

    while((dns_entry = (DNSEntry *)XP_ListNextObject(list_obj)) != 0)
      {
		TRACEMSG(("net_CheckDNSCache: comparing %s and %s", hostname, 
														dns_entry->hostname));
		if(dns_entry->hostname && !strcasecomp(hostname, dns_entry->hostname))
		  {
	    	return(dns_entry);
		  }
      }
    
    return(0);
}

/*  print an IP address from a sockaddr struct
 *
 *  This routine is only used for TRACE messages
 */
#ifdef XP_WIN
MODULE_PRIVATE char *CONST
NET_IpString (struct sockaddr_in *sin)
#else
MODULE_PRIVATE CONST char * 
NET_IpString (struct sockaddr_in *sin)
#endif
{
    static char buffer[32];
    int a,b,c,d;
    unsigned char *pc;

    pc = (unsigned char *) &sin->sin_addr;

    a = (int) *pc;
    b = (int) *(++pc);
    c = (int) *(++pc);
    d = (int) *(++pc);
    PR_snprintf(buffer, sizeof(buffer), "%d.%d.%d.%d", a,b,c,d);

    return buffer;
}

/* a list of dis-allowed ports for gopher
 * connections
 */
PRIVATE int net_bad_ports_table[] = {
1, 7, 9, 11, 13, 15, 17, 19, 20,
21, 23, 25, 37, 42, 43, 53, 77, 79, 87, 95, 101, 102,
103, 104, 109, 110, 111, 113, 115, 117, 119,
135, 143, 389, 512, 513, 514, 515, 526, 530, 531, 532,
540, 556, 601, 6000, 0 };

/*  find the ip address of a host
 *
 * return 0 on success
 */
PUBLIC int 
NET_FindAddress (const char *host_ptr, 
				 struct sockaddr_in *sin, 
				 MWContext * window_id, 
				 int sock, 
				 int *random_host_pick, 
				 int *random_started_from)
{
    PRHostEnt *hoststruct_pointer; /* Pointer to host - See netdb.h */
    DNSEntry * cache_pointer;
    char *port;
    char *host_port=0;
	static XP_Bool first_dns_failure=TRUE;
	static int random_host_number = -1;
	static time_t random_host_expiration = 0;
    XP_Bool is_numeric_ip;
    char *host_cp;

	if(!host_ptr || !*host_ptr)
		return -1;

    StrAllocCopy(host_port, host_ptr);      /* Make a copy we can mess with */

    /* Parse port number if present
     */  
	port = XP_STRCHR(host_port, ':');  
    if (port) 
      {
        *port++ = 0;       
        if (XP_IS_DIGIT(*port)) 
		  {
			unsigned short port_num = (unsigned short) atol(port);
			int i;

			/* check for illegal ports */
			/* if the explicit port number equals
			 * the default port number let it through
			 */
			if(port_num != ntohs(sin->sin_port))
			  {
            	/* disallow well known ports */
            	for(i=0; net_bad_ports_table[i]; i++)
                	if(port_num == net_bad_ports_table[i])
                  	  {
                    	char *error_msg = XP_STRDUP(
									"Sorry, access to the port number given\n"
									"has been disabled for security reasons");
	
                    	if(error_msg)
					  	  {
                        	FE_Alert(window_id, error_msg);
							FREE(error_msg);
					  	  }

						/* return special error code
						 * that NET_BeginConnect knows
						 * about.
						 */
						FREE(host_port);
                    	return(MK_UNABLE_TO_CONNECT);
                  	  }

            	sin->sin_port = htons(port_num);
			  }

		  }
      }

    /* see if this host entry is already cached
	 *
 	 * don't check in the case where we need to pick
	 * another host from an alias list since we dont
	 * cache the aliases.
     */
    if(*random_host_pick < 0)
    	cache_pointer = net_CheckDNSCache(host_port);
	else
    	cache_pointer = NULL;

    if(cache_pointer)
      {
	    XP_MEMCPY(&sin->sin_addr, cache_pointer->haddr, cache_pointer->h_length);
		*random_host_pick = cache_pointer->random_host_pick;
		*random_started_from = cache_pointer->random_start;
	    XP_FREE(host_port);
	    return(0);  /* FOUND OK */
      }

	is_numeric_ip = TRUE; /* init positive */
	for(host_cp = host_port; *host_cp; host_cp++)
	if(!XP_IS_DIGIT(*host_cp) && *host_cp != '.')
	  {
		is_numeric_ip = FALSE;
		break;
	  }

    /* Parse host number if present.
     */  
    if (is_numeric_ip)  /* numeric ip addr */
      {
        /* some systems return a number instead of a struct 
         */
#if defined(ALT_SADDR) || defined(XP_UNIX) || defined(_WINDOWS) || defined (XP_MAC)
        sin->sin_addr.s_addr = inet_addr(host_port);    /* See arpa/inet.h */
#else
        sin->sin_addr = inet_addr(host_port);           /* See arpa/inet.h */
#endif

      } 
    else      /* name not number */
      {
	    char *remapped_host_port=0;

		/* randomly remap home.netscape.com to home1.netscape.com through
		 * home32.netscape.com
		 * cache the original name not the new one.
		 */
		if(!strncasecomp(host_port, "home.", 5)
			&& (strcasestr(host_port+4, ".netscape.com")
				|| strcasestr(host_port+4, ".mcom.com")))
		  {
			time_t cur_time = time(NULL);
			char temp_string[32];

			temp_string[0] = '\0';

			if(random_host_number == -1 || 
				random_host_expiration < cur_time)
			  {
				/* pick a new random number
				 */
				random_host_expiration = cur_time + (60 * 5); /* five min */
				random_host_number = (XP_RANDOM() & 31);
			  }
				
			PR_snprintf(temp_string, sizeof(temp_string), "home%d%s",
					   random_host_number+1, host_port+4);

			StrAllocCopy(remapped_host_port, temp_string);

		    TRACEMSG(("Remapping %s to %s", host_port, remapped_host_port));
		  }
		
#ifdef XP_UNIX
#ifndef DEBUG_jwz /* don't seem to have res_init on linux 2.4.9? */
		  {
		    /*
		    ** Implement a terrible hack just for unix. If the environment
		    ** variable SOCKS_NS is defined, stomp on the host that the DNS
		    ** code uses for host lookup to be a specific ip address.
		    */
		    static char firstTime = 1;
		    if (firstTime) 
			  {
			    char *ns = getenv("SOCKS_NS");
			    firstTime = 0;
			    if (ns && ns[0]) 
				  {
				    /* Use a specific host for dns lookups */
					extern int res_init (void);
				    res_init();
				    _res.nsaddr_list[0].sin_addr.s_addr = inet_addr(ns);
				    _res.nscount = 1;
			      }
		      }
		  }
#endif /* !DEBUG_jwz */
#endif

		{
			/* malloc the string to prevent overflow
			 */
			char * msg = PR_smprintf(XP_GetString(XP_PROGRESS_LOOKUPHOST), host_port);

			if(msg)
			  {
        		NET_Progress(window_id, msg);
				FREE(msg);
			  }
	    }

#ifdef ASYNC_DNS
		  {
			/* FE_StartAsyncDNSLookup  should fill in the hoststruct_pointer
			 * or leave zero the pointer if not found
			 *
			 * it can also return MK_WAITING_FOR_LOOKUP
			 * if it's not done yet
			 */
			int status = FE_StartAsyncDNSLookup(window_id, 
											    remapped_host_port ? 
												    remapped_host_port : 
													    host_port, 
												&hoststruct_pointer, 
												(int)sock);
			if(status == MK_WAITING_FOR_LOOKUP)
			  {
				FREE(host_port);
				return(status);
			  }
		  }

#else /* !ASYNC_DNS */

		NET_InGetHostByName++; /* global semaphore */
# ifdef MCC_PROXY
		{
			extern Request *glob_rq;
			extern Session *glob_sn;
			Request *rq = glob_rq;
			Session *sn = glob_sn;

			hoststruct_pointer =
			  servact_gethostbyname(remapped_host_port ? 
									remapped_host_port : host_port, sn, rq);
		}
# else /* !MCC_PROXY */
		{
			char dbbuf[PR_NETDB_BUF_SIZE];
			PRHostEnt hpbuf;
			hoststruct_pointer =
				PR_gethostbyname(remapped_host_port ? 
								 remapped_host_port : host_port, &hpbuf,
								 dbbuf, sizeof(dbbuf), 0);
		}
# endif /* !MCC_PROXY */
		NET_InGetHostByName--; /* global semaphore */

#endif /*  !ASYNC_DNS */

        if (!hoststruct_pointer) 
          {
			if(first_dns_failure)
			  {
				first_dns_failure = FALSE;
#ifndef MCC_PROXY
				/* On the proxy causes confusing messages */
				NET_SanityCheckDNS(window_id);
#endif /* !MCC_PROXY */
			  }
            XP_LTRACE(MKLib_trace_flag,1,("mktcp.c: Can't find host name `%s'.  Errno #%d\n",
									host_port, SOCKET_ERRNO));
			FREE(host_port);
			XP_LTRACE(MKLib_trace_flag,1,("gethostbyname failed with error: %d\n", SOCKET_ERRNO));
            return -1;  /* Fail? */
          }

#ifdef RANDOM_DNS_HOST_PICK
		{
			int32 n;

			/* count the number of addresses listed
			 */
			for(n=0; hoststruct_pointer->h_addr_list[n]; n++)
				; /* NULL body */

			if(n == 0)
			  {
				XP_ASSERT(0); /* shouldn't happen, but */
				return -1; /* fail */
			  }
			else if(n > 1)
			  {
				/* previous random pick */
				if(*random_host_pick > -1)
				  {
					n = (*random_host_pick + 1) % n;
					if(n == *random_started_from)
					  {
						FREE(host_port);
						return -1;
					  }
				  }
				else
				  {
					/* don't be random any more.  Use
					 * the first one always.
					 * But keep the failover stuff
					 */
#if 0
					n = XP_RANDOM() % n;
#else			
					n = 0;
#endif
					*random_started_from = n;
				  }
				*random_host_pick = n;
			  }
			else
			  {
				n = 0;
				*random_host_pick = -1;
			  }

    		XP_MEMCPY(&sin->sin_addr, 
					  hoststruct_pointer->h_addr_list[n], 
					  hoststruct_pointer->h_length);
		}
#else /* !RANDOM_DNS_HOST_PICK */
    	XP_MEMCPY(&sin->sin_addr, 
				  hoststruct_pointer->h_addr, 
				  hoststruct_pointer->h_length);
		*random_host_pick = -1;
#endif /* !RANDOM_DNS_HOST_PICK */

    	/* cache this DNS entry 
     	 */
    	net_CacheDNSEntry(host_port, 
						  hoststruct_pointer, 
						  *random_host_pick, 
						  *random_started_from);
      }

    TRACEMSG(("TCP.c: Found address %s and port %d", NET_IpString(sin), (int)ntohs(sin->sin_port)));

	FREE(host_port);
    return(0);   /* OK */
}

/*  Find out what host this is that we are running on
 */
#ifdef XP_WIN
MODULE_PRIVATE char *CONST NET_HostName ()
#else
MODULE_PRIVATE const char * NET_HostName ()
#endif
{
	if(!net_local_hostname)
	  {
        char tmp_local_hostname[256];

        if (gethostname(tmp_local_hostname, 254) < 0) /* Error, just get an empty string */
        	tmp_local_hostname[0] = 0;
	    StrAllocCopy(net_local_hostname, tmp_local_hostname);
			
        TRACEMSG(("TCP.c: Found local host name: %s", net_local_hostname));
	  }
    return net_local_hostname;
}

/* FREE left over tcp connection data if there is any
 */
MODULE_PRIVATE void 
NET_FreeTCPConData(TCP_ConData * data)
{
	TRACEMSG(("Freeing TCPConData...Done with connect (i hope)"));
	FREEIF(data)
}

PRIVATE int
net_start_first_connect(const char   *host, 
						NETSOCK       sock, 
						MWContext    *window_id, 
						TCP_ConData  *tcp_con_data,
						char        **error_msg)
{
    {
        /* malloc the string to prevent overflow
         */
        int32 len = XP_STRLEN(XP_GetString(XP_PROGRESS_CONTACTHOST));
        char * buf;

        len += XP_STRLEN(host);

        buf = (char *)XP_ALLOC((len+10)*sizeof(char));
        if(buf)
          {
            PR_snprintf(buf, (len+10)*sizeof(char),
                    XP_GetString(XP_PROGRESS_CONTACTHOST), host);
            NET_Progress(window_id, buf);
            FREE(buf);
          }
    }

	/* set the begining time to be the current time.
	 * the timeout value will be  compared to this
	 * later
	 */
	tcp_con_data->begin_time = time(NULL);

    /* send the initial connect.  This will probably return negative
     * since it takes a while to make the actual connect
     */
    /* if it's not equal to zero something went wrong
     */
    if(0 != NETCONNECT (sock,
                       (struct sockaddr*)&(tcp_con_data->sin),
                       sizeof(tcp_con_data->sin) ))
      {

        /* the server should return EINPROGRESS if the connection is in
         * the process of connecting.  On SVR4 machines it may also
         * return EAGAIN
         */
        int rv = SOCKET_ERRNO;

#ifdef SVR4
        if((rv == EINPROGRESS) || (rv == EAGAIN))
#else
        if ((rv == XP_ERRNO_EWOULDBLOCK) || (rv == XP_ERRNO_EINPROGRESS))
#endif /* SVR4 */
          {
            tcp_con_data->next_state = NET_TCP_FINISH_CONNECT;
            return(MK_WAITING_FOR_CONNECTION);  /* not connected yet */
          }
        else
          {
            NETCLOSE(sock);
            if(rv == XP_ERRNO_ECONNREFUSED)
              {
	    		*error_msg = NET_ExplainErrorDetails(MK_CONNECTION_REFUSED, host);

                XP_LTRACE(MKLib_trace_flag,1,("connect: refused\n"));

                return(MK_CONNECTION_REFUSED);
              }
            else if(rv == XP_ERRNO_ETIMEDOUT)
              {
	    		*error_msg = NET_ExplainErrorDetails(MK_CONNECTION_TIMED_OUT);

                XP_LTRACE(MKLib_trace_flag,1,("connect: timed out\n"));

                return(MK_CONNECTION_TIMED_OUT);
              }
			else 
              {
			    *error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CONNECT, rv);

                XP_LTRACE(MKLib_trace_flag,1,("connect: unable to connect %i\n", rv));

                return (MK_UNABLE_TO_CONNECT);
              }
          }
      }
 
    /* else  good connect */

    return(MK_CONNECTED);
}

PRIVATE void
net_connection_failed(CONST char *hostname)
{

	char *colon = XP_STRCHR(hostname, ':');


	/* strip colon if present */
	if(colon)
		*colon = '\0';

	/* if we randomly remaped home.netscape.com
	 * and we failed to connect, unmap it now
	 * so it will get randomly remapped.
 	 */
	if(!strncasecomp(hostname, "home.", 5)
		&& (strcasestr(hostname+4, ".netscape.com")
		|| strcasestr(hostname+4, ".mcom.com")))
	  {
		DNSEntry *dns_entry = net_CheckDNSCache(hostname);

		if(dns_entry)
		  {
			XP_ListRemoveObject(dns_list, dns_entry);
			FREEIF(dns_entry->hostname);
        	FREEIF(dns_entry->haddr);
        	XP_FREE(dns_entry);
		  }
	  }
	
	if(colon)
		*colon = ':';
}


/*
 * Non blocking connect begin function.  It will most likely
 * return negative. If it does, NET_ConnectFinish() will
 * need to be called repeatably until a connect is established.
 *
 * return's negative on error
 * return's MK_WAITING_FOR_CONNECTION when continue is neccessary
 * return's MK_CONNECTED on true connect
 */ 
MODULE_PRIVATE int 
NET_BeginConnect (CONST char   *url, 
				  char   *ip_address_string,
          	      char         *prot_name, 
          	      int           def_port, 
          	      NETSOCK      *sock, 
			      Bool       use_security,
			      TCP_ConData **tcp_con_data,
          	      MWContext    *window_id,
			      char        **error_msg,
				  u_long		socks_host,
				  short			socks_port)
{
    CONST char *host; 
    char *althost=0; 
   	int status;
    int fd;
    char *host_string=0;
    char *at_sign;
    int opt;
    NETSOCK sock_status;      
#ifdef XP_WIN
	struct linger {
		int l_onoff;
		int l_linger;
	} lingerstruct;
#endif

	TRACEMSG(("NET_BeginConnect called for url: %s", url));

	/* in case finish connect calls us 
	 */
	if(*tcp_con_data)
		NET_FreeTCPConData(*tcp_con_data);
	
	/* construct state table data
	 */	
	*tcp_con_data = XP_NEW(TCP_ConData);
	
	if(!*tcp_con_data)
	  {
	    *error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
		return(MK_OUT_OF_MEMORY);
	  }

	XP_MEMSET(*tcp_con_data, 0, sizeof(TCP_ConData));

    (*tcp_con_data)->random_host_pick = -1;
    (*tcp_con_data)->random_start = -1;

    /* Set up Internet defaults */
    (*tcp_con_data)->sin.sin_family = AF_INET;
    (*tcp_con_data)->sin.sin_port = htons((unsigned short)def_port);

    if(NET_URL_Type(url)) 
      {
        host_string = NET_ParseURL(url, GET_HOST_PART);
		host = host_string;
      }
    else
      {
        /* assume that a hostname was passed directly 
         * instead of a URL
         */
        host = url;
      }

    /* if theres an @ in the hostname then use the stuff after it as 
	 * the hostname 
	 */
    if((at_sign = XP_STRCHR(host,'@')) != NULL)
        host = at_sign+1;

#ifdef MCC_PROXY
#define PROXY_INT_TOKEN	"iNt-cOnNs-hAnDlEd"
	{
		extern Request *glob_rq;
		extern Session *glob_sn;
		Request *rq = glob_rq;
		Session *sn = glob_sn;

		if (rq && !pblock_findval(PROXY_INT_TOKEN, rq->vars)) {
			int one = 1;
			char *p = strchr(host, ':');
			int port = (p ? atoi(p+1) :
						(!strncmp(url, "http:",   5)) ?  80 :
						(!strncmp(url, "https:",  6)) ? 443 :
						(!strncmp(url, "snews:",  6)) ? 563 :
						(!strncmp(url, "ftp:",    4)) ?  21 :
						(!strncmp(url, "gopher:", 7)) ?  70 : 80);

			pblock_nvinsert(PROXY_INT_TOKEN, "1", rq->vars);

			if (p) *p = '\0';
			*sock = servact_connect((char *)host, port, sn, rq);
			if (p) *p = ':';

			switch (*sock) {
			  case -2:						/* == REQ_NOACTION */
				break;						/* Use the native connect */

			  case -1:						/* == REQ_ABORTED */
				*sock = SOCKET_INVALID;		/* Conn. function failed connect */
				return MK_UNABLE_TO_CONNECT;

			  default:						/* Connect function succeeded */
#ifndef NO_SSL /* added by jwz */
				SSL_Import(*sock);
				SSL_Ioctl(*sock, FIONBIO, &one);
				return MK_CONNECTED;
#endif /* NO_SSL -- added by jwz */

		    }
		}
	}
#endif	/* MCC_PROXY */


    /* build a socket
     */
    *sock = NETSOCKET(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if(*sock == SOCKET_INVALID)
	  {                                                
	  	int error = SOCKET_ERRNO;
		TRACEMSG(("NETSOCKET call returned INVALID_SOCKET: %d", SOCKET_ERRNO));
		NET_FreeTCPConData(*tcp_con_data);
		*tcp_con_data = 0;
	    *error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CREATE_SOCKET, error);
        FREE_AND_CLEAR(host_string);
	    return(MK_UNABLE_TO_CREATE_SOCKET);
	  }

#ifdef XP_WIN
    FE_SetupAsyncSelect(*sock, window_id);
#endif

#ifdef XP_WIN16
	lingerstruct.l_onoff = 1;
	lingerstruct.l_linger = 0;
	SSL_SetSockOpt(*sock, SOL_SOCKET, SO_LINGER, 
				   &lingerstruct, sizeof(lingerstruct));
#endif /* XP_WIN16 */

	/* copy the sock no
	 */
    fd = *sock;
#ifndef NO_SSL
	if (socks_host)
	  {
		TRACEMSG(("Proxy autoconfig enables SOCKS"));
		SSL_Enable(fd, SSL_SOCKS, 1);
		SSL_ConfigSockd(fd, socks_host, socks_port);
	  }
    else if(NET_SocksHost)
      {
		TRACEMSG(("Using socks for this connection"));
        SSL_Enable(fd, SSL_SOCKS, 1);

        SSL_ConfigSockd(fd, NET_SocksHost, NET_SocksPort);
      }
#ifdef TURN_SECURITY_ON
	if(use_security)
	  {
		int err;

		TRACEMSG(("Using security for this connection"));
    	status = SSL_Enable(*sock, SSL_SECURITY, 1);
    	if (status < 0) 
		  {
			loser:
            FREE_AND_CLEAR(host_string);
			NET_FreeTCPConData(*tcp_con_data);
			*tcp_con_data = 0;
			err = XP_GetError();
	        *error_msg = NET_ExplainErrorDetails(err);
	        *sock = SOCKET_INVALID;
            return(err);
		  }

#ifdef MOZILLA_CLIENT
		status = SECNAV_SetupSecureSocket(*sock, (char *)url, window_id);
#endif /* MOZILLA_CLIENT */
		
		if (status) {
			goto loser;
		}

	  }
#endif /* TURN_SECURITY_ON */
#endif /* NO_SSL */

    /* lets try to set the socket non blocking
     * so that we can do multi threading :)
     */
	opt = 1;
    sock_status = SOCKET_IOCTL(fd, FIONBIO, &opt);
#ifndef XP_WIN
    if (sock_status == SOCKET_INVALID)
        NET_Progress(window_id, XP_GetString(XP_PROGRESS_NOCONNECTION));
#endif /* XP_WIN */

	/* if an IP address string was passed in, then use it instead of the
	 * hostname to do the DNS lookup.
	 */
	if ( ip_address_string ) {
		char *port;

		StrAllocCopy(althost, ip_address_string);

		port = XP_STRCHR(host, ':');
		if ( port ) {
			StrAllocCat(althost, port);
		}
	}
	
    status = NET_FindAddress(althost?althost:host, 
				 &((*tcp_con_data)->sin), 
				 window_id, 
				 (int)*sock,
				 &((*tcp_con_data)->random_host_pick),
				 &((*tcp_con_data)->random_start));

	if (althost) {
		XP_FREE(althost);
	}
	
	if(status == MK_WAITING_FOR_LOOKUP)
	  {
		(*tcp_con_data)->next_state = NET_TCP_FINISH_DNS_LOOKUP;
        FREE_AND_CLEAR(host_string);
       	return(MK_WAITING_FOR_CONNECTION);  /* not connected yet */
	  }
    else if (status < 0)
      {
        {
            /* malloc the string to prevent overflow
             */
            int32 len = XP_STRLEN(XP_GetString(XP_PROGRESS_UNABLELOCATE));
            char * buf;

            len += XP_STRLEN(host);

            buf = (char *)XP_ALLOC((len+10)*sizeof(char));
            if(buf)
              {
                PR_snprintf(buf, (len+10)*sizeof(char),
                        XP_GetString(XP_PROGRESS_UNABLELOCATE), host);
                NET_Progress(window_id, buf);
                FREE(buf);
              }
        }

		NET_FreeTCPConData(*tcp_con_data);
		*tcp_con_data = 0;
		NETCLOSE(*sock);
	    *sock = SOCKET_INVALID;

		/* @@@@@ HACK!  If we detect an illegal port in
		 * NET_FindAddress we put up an error message
		 * and return MK_UNABLE_TO_CONNECT.  Don't
		 * print another message
		 */
		if(status != MK_UNABLE_TO_CONNECT)
	    	*error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_LOCATE_HOST, 
												*host ? host : "(no name specified)");
        FREE_AND_CLEAR(host_string);
        return MK_UNABLE_TO_LOCATE_HOST;
      }

    status = net_start_first_connect(host, *sock, window_id, 
									 *tcp_con_data, error_msg);

	if(status != MK_WAITING_FOR_CONNECTION)
	  {
		NET_FreeTCPConData(*tcp_con_data);
		*tcp_con_data = 0;
	  }
	else
	  {
       	(*tcp_con_data)->next_state = NET_TCP_FINISH_CONNECT;
		/* save in case we need it */
       	(*tcp_con_data)->use_security = use_security; 
	  }

	if(status < 0)
	  {
		net_connection_failed(host);
		NETCLOSE(*sock);
	    *sock = SOCKET_INVALID;
	  }

    FREE_AND_CLEAR(host_string);
	
    return(status);
}


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
PUBLIC int 
NET_FinishConnect (CONST char   *url,
                   char         *prot_name,
                   int           def_port,
                   NETSOCK      *sock, 
			       TCP_ConData **tcp_con_data,
                   MWContext    *window_id,
				   char        **error_msg)
{
	int status;

	TRACEMSG(("NET_FinishConnect called for url: %s", url));

	if(!*tcp_con_data)  /* safty valve */
	  {
	    *error_msg = NET_ExplainErrorDetails(MK_OUT_OF_MEMORY);
		return(MK_OUT_OF_MEMORY);
	  }

	switch((*tcp_con_data)->next_state)
	{
	case NET_TCP_FINISH_DNS_LOOKUP:
	  {
		char * host_string=NULL;	
		const char * host = NULL;

        if(NET_URL_Type(url))
          {
            char *at_sign;
            host_string = NET_ParseURL(url, GET_HOST_PART);
            /* if theres an @ then use the stuff after it as a hostname
             */
            if((at_sign = XP_STRCHR(host_string,'@')) != NULL)
                host = at_sign+1;
            else
                host = host_string;
          }
        else
          {
            /* assume that a hostname was passed directly
             * instead of a URL
             */
            host = url;
          }

		status = NET_FindAddress(host, 
					&((*tcp_con_data)->sin), 
					window_id, 
					(int)*sock,
					&((*tcp_con_data)->random_host_pick),
				 	&((*tcp_con_data)->random_start));

        if(status == MK_WAITING_FOR_LOOKUP)
          {
            (*tcp_con_data)->next_state = NET_TCP_FINISH_DNS_LOOKUP;
            return(MK_WAITING_FOR_CONNECTION);  /* not connected yet */
          }
        else if (status < 0)
          {
        	{
            	/* malloc the string to prevent overflow
             	 */
            	int32 len = XP_STRLEN(XP_GetString(XP_PROGRESS_UNABLELOCATE));
            	char * buf;
	
            	len += XP_STRLEN(host);
	
            	buf = (char *)XP_ALLOC((len+10)*sizeof(char));
            	if(buf)
              	  {
                	PR_snprintf(buf, (len+10)*sizeof(char),
                        	XP_GetString(XP_PROGRESS_UNABLELOCATE), host);
                	NET_Progress(window_id, buf);
                	FREE(buf);
              	  }
        	}

            NET_FreeTCPConData(*tcp_con_data);
            *tcp_con_data = 0;
	        *error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_LOCATE_HOST, host);
            FREE_AND_CLEAR(host_string);
            return MK_UNABLE_TO_LOCATE_HOST;
          }

        status = net_start_first_connect(host, *sock, window_id, 
										 *tcp_con_data, error_msg);

        if(status != MK_WAITING_FOR_CONNECTION)
          {
            NET_FreeTCPConData(*tcp_con_data);
            *tcp_con_data = 0;
          }
		else
		  {
        	(*tcp_con_data)->next_state = NET_TCP_FINISH_CONNECT;
		  }

        if(status < 0)
          {
			net_connection_failed(host);
			XP_LTRACE(MKLib_trace_flag,1,("mktcp.c: Error during connect %d\n", SOCKET_ERRNO));
            NETCLOSE(*sock);
            *sock = SOCKET_INVALID;
          }  

        FREE_AND_CLEAR(host_string);

        return(status);

	  } /* end of this part of the case */
	
	case NET_TCP_FINISH_CONNECT:

        /* do another connect here to see if we are actually connected
         */
        if(0 != NETCONNECT(*sock, 
						  (struct sockaddr*)&((*tcp_con_data)->sin), 
						  sizeof((*tcp_con_data)->sin)))
        {
		    int error;
    
		    error = SOCKET_ERRNO;
		
#ifdef WHY_DID_WE_DO_THIS
			/* this code causes the NTS stack to
			 * spin forever because it returns an
			 * addrinuse errno on almost all bad
			 * connects
			 */
			if(error == XP_ERRNO_EADDRINUSE)
			  {
				/* try the whole connect process over again
				 */
				NETCLOSE(*sock);
				*sock = SOCKET_INVALID;

        		return(NET_BeginConnect (url,
								 NULL,
                                 prot_name,
                                 def_port,
                                 sock,
                                 (*tcp_con_data)->use_security,
                                 tcp_con_data,
                                 window_id,
								 error_msg));

			  }
		    else 
#endif /* WHY DID WE DO THIS? */

			if(error == XP_ERRNO_EINPROGRESS
					  || error == XP_ERRNO_EWOULDBLOCK
#ifdef XP_WIN
					/* ealready is the correct error code for
					 * working but not yet completed.  It
					 * should be so for all systems but it's
					 * too late to experiment now.
					 */
						|| error == XP_ERRNO_EALREADY
					/* add wsaeinval to the if for windows */
					    || error == WSAEINVAL)   
#else
					)  /* no other if's */
#endif /* XP_WIN */
		      {

				/* check the begin time against the current time minus
				 * the timeout value.  Error out if past the timeout
				 */
				if(time(NULL)-net_tcp_connect_timeout > (*tcp_con_data)->begin_time)
				  {
					error = XP_ERRNO_ETIMEDOUT;
					goto error_out;
				  }

			    /* still waiting
			     */
                return MK_WAITING_FOR_CONNECTION;
		      }
		    else if(error != XP_ERRNO_EISCONN && error != XP_ERRNO_EALREADY)
		      {
error_out:
		        /* if EISCONN then we actually are connected
		         * otherwise return an error
		         */
			    XP_LTRACE(MKLib_trace_flag,1,("mktcp.c: Error during connect: %d\n", error));
				if((*tcp_con_data)->random_host_pick > -1)
				  {
					char * host = NET_ParseURL(url, GET_HOST_PART);
					int status;

					if(*host == '\0')
					  {
						/* the url is a hostname */
						FREE(host);
						host = XP_STRDUP(url);
					  }

					/* there are more than one alias
					 * pick another and retry
					 * this will fail if we have already tried them all
					 */
    				status = NET_FindAddress(host, 
				 				&((*tcp_con_data)->sin), 
				 				window_id, 
				 				(int)*sock,
				 				&((*tcp_con_data)->random_host_pick),
				 				&((*tcp_con_data)->random_start));

					FREE(host);
					if(status == 0)
        				return(NET_BeginConnect (url,
												NULL,
                                 				prot_name,
                                 				def_port,
                                 				sock,
                                 				(*tcp_con_data)->use_security,
                                 				tcp_con_data,
                                 				window_id,
								 				error_msg,
												0,
												0));
					/* else fall through to the error */
				  }

				{
					char * host = NET_ParseURL(url, GET_HOST_PART);
					/* at this point we are garenteed to fail */
					net_connection_failed(host);
					FREE(host);
				}

                if (error == XP_ERRNO_ECONNREFUSED)
					{
							char * host = NET_ParseURL(url, GET_HOST_PART);
	        			    *error_msg = NET_ExplainErrorDetails(
												MK_CONNECTION_REFUSED,
												host);
							FREE(host);
		    			    NET_FreeTCPConData(*tcp_con_data);
		    			    *tcp_con_data = 0;
                        	return MK_CONNECTION_REFUSED;
					}
                else if (error == XP_ERRNO_ETIMEDOUT)
					{
		    			NET_FreeTCPConData(*tcp_con_data);
		    			*tcp_con_data = 0;
	        			*error_msg = NET_ExplainErrorDetails(MK_CONNECTION_TIMED_OUT);
                        return MK_CONNECTION_TIMED_OUT;
					}
                else
					{
		    			NET_FreeTCPConData(*tcp_con_data);
		    			*tcp_con_data = 0;
	        			*error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CONNECT, error);
                        return MK_UNABLE_TO_CONNECT;
					}
		      }
        }
    
		TRACEMSG(("mktcp.c: Successful connection (message 1)"));
		NET_FreeTCPConData(*tcp_con_data);
		*tcp_con_data = 0;
        return MK_CONNECTED;

	default:
		NET_FreeTCPConData(*tcp_con_data);
		*tcp_con_data = 0;
		TRACEMSG(("mktcp.c: bad state during connect"));

		assert(0);  /* should never happen */

	    *error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CONNECT, 0);
		return(MK_UNABLE_TO_CONNECT);

	} /* end switch on next_state */

	assert(0);  /* should never get here */

	*error_msg = NET_ExplainErrorDetails(MK_UNABLE_TO_CONNECT, 0);
	return(MK_UNABLE_TO_CONNECT);
}


/* Free any memory used up
 */
MODULE_PRIVATE void
NET_CleanupTCP(void)
{
    if(dns_list)
      {
        DNSEntry * tmp_entry;

        while((tmp_entry = (DNSEntry *)XP_ListRemoveTopObject(dns_list)) != NULL)
          {
             FREE(tmp_entry->hostname);
             FREE(tmp_entry->haddr);
             FREE(tmp_entry);
          }

        XP_ListDestroy(dns_list);
		dns_list = 0;
      }

	/* we really should free the socket_buffer but
	 * that is in the other module as a private :(
	 */

    FREE_AND_CLEAR(net_local_hostname);

    return;
}

#ifdef PROFILE
#pragma profile off
#endif
