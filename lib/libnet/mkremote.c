/* forks a telnet process
 */

/* Please leave outside of ifdef for windows precompiled headers */
#include "mkutils.h"

#ifdef MOZILLA_CLIENT

#include "mkparse.h"
#include "mkremote.h"
#include "glhist.h"
#include "mkgeturl.h"   /* for URL types */
#include "merrors.h"

/*
 * begin a session with a remote host.
 *
 * URL types permitted: Telnet, TN3270, and Rlogin
 */
MODULE_PRIVATE int 
NET_RemoteHostLoad (ActiveEntry  * cur_entry)
{
    char * host_string;
    int url_type;
    char * cp;
    char * username;
    char * hostname;
    char * port_string;

    TRACEMSG(("In NET_RemoteHostLoad"));

    GH_UpdateGlobalHistory(cur_entry->URL_s);

    if(cur_entry->format_out == FO_CACHE_AND_PRESENT || cur_entry->format_out == FO_PRESENT)
      {
        url_type = NET_URL_Type(cur_entry->URL_s->address);
        host_string = NET_ParseURL(cur_entry->URL_s->address, GET_HOST_PART);

    	hostname = XP_STRCHR(host_string, '@');
    	port_string = XP_STRCHR(host_string, ':');

    	if (hostname)
      	  {
        	*hostname++ = '\0';  
    	    username = NET_UnEscape(host_string);
          }
        else
          {
            hostname = host_string;
            username = NULL;  /* no username given */
          }

        if (port_string)
		  {
            *port_string++ = '\0';  

			/* Sanity check the port part
             * prevent telnet://hostname:30;rm -rf *  URL's (VERY BAD)
             * only allow digits
             */
            for(cp=port_string; *cp != '\0'; cp++)
                if(!isdigit(*cp))
                  {
                    *cp = '\0';
                    break;
                  }
		  }

	   
		if(username)
		  {
			/* Sanity check the username part
             * prevent telnet://hostname:30;rm -rf *  URL's (VERY BAD)
             * only allow alphanums
             */
            for(cp=username; *cp != '\0'; cp++)
                if(!isalnum(*cp))
                  {
                    *cp = '\0';
                    break;
                  }
		  }

		/* now sanity check the hostname part
         * prevent telnet://hostname;rm -rf *  URL's (VERY BAD)
         * only allow alphanumeric characters and a few symbols
         */
        for(cp=hostname; *cp != '\0'; cp++)
        if(!isalnum(*cp) && *cp != '_' && *cp != '-' &&
              *cp != '+' && *cp != ':' && *cp != '.' && *cp != '@')
          {
            *cp = '\0';
            break;
          }

		TRACEMSG(("username: %s, hostname: %s, port: %s", 
				  username ? username : "(null)",
				  hostname ? hostname : "(null)",
				  port_string ? port_string : "(null)"));

		if(url_type == TELNET_TYPE_URL)
		  {
		    FE_ConnectToRemoteHost(cur_entry->window_id, FE_TELNET_URL_TYPE, hostname, port_string, username);
		  }
		else if(url_type == TN3270_TYPE_URL)
		  {
		    FE_ConnectToRemoteHost(cur_entry->window_id, FE_TN3270_URL_TYPE, hostname, port_string, username);
		  }
		else if(url_type == RLOGIN_TYPE_URL)
		  {
		    FE_ConnectToRemoteHost(cur_entry->window_id, FE_RLOGIN_URL_TYPE, hostname, port_string, username);
		  }
		/* fall through if it wasn't any of the above url types */

        XP_FREE(host_string);
      }

	cur_entry->status = MK_NO_DATA;
    return -1;
}

#endif /* MOZILLA_CLIENT */
