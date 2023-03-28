/* -*- Mode: C; tab-width: 4 -*- */
#include "mkutils.h"
#include "mkparse.h"
#include "mkaccess.h"
#include "prefapi.h"
#include "shist.h"

/* for XP_GetString() */
#include "xpgetstr.h"
extern int XP_CONFIRM_AUTHORIZATION_FAIL;
extern int XP_ACCESS_ENTER_USERNAME;
extern int XP_ACCESS_ENTER_USERNAME;
extern int XP_CONFIRM_PROXYAUTHOR_FAIL;
extern int XP_CONNECT_PLEASE_ENTER_PASSWORD_FOR_HOST;
extern int XP_PROXY_REQUIRES_UNSUPPORTED_AUTH_SCHEME;
extern int XP_LOOPING_OLD_NONCES;
extern int XP_UNIDENTIFIED_PROXY_SERVER;
extern int XP_PROXY_AUTH_REQUIRED_FOR;
extern int XP_CONNECT_PLEASE_ENTER_PASSWORD_FOR_PROXY;
extern int MK_ACCESS_COOKIES_THE_SERVER;
extern int MK_ACCESS_COOKIES_WISHES; 
extern int MK_ACCESS_COOKIES_TOANYSERV; 
extern int MK_ACCESS_COOKIES_TOSELF;
extern int MK_ACCESS_COOKIES_NAME_AND_VAL;
extern int MK_ACCESS_COOKIES_COOKIE_WILL_PERSIST;
extern int MK_ACCESS_COOKIES_SET_IT;

#ifndef XP_FREEIF
#define XP_FREEIF(obj) do { if (obj) { XP_FREE (obj); obj = 0; }} while (0)
#endif

#define MAX_NUMBER_OF_COOKIES  300
#define MAX_COOKIES_PER_SERVER 20
#define MAX_BYTES_PER_COOKIE   4096  /* must be at least 1 */

/*
 * Authentication information for servers and proxies is kept
 * on separate lists, but both lists consist of net_AuthStruct's.
 */

PRIVATE XP_List * net_auth_list = NULL;
PRIVATE XP_List * net_proxy_auth_list = NULL;

PRIVATE Bool cookies_changed = FALSE;

PRIVATE NET_CookiePrefsEnum net_cookie_preference = NET_SilentCookies;
PRIVATE NET_ForeignCookiesEnum NET_ForeignCookies = NET_DoNotAllowForeignCookies;

#if 0
static const char *pref_cookie_acceptance = "network.accept_cookies";
static const char *pref_foreign_cookies = "network.foreign_cookies";
#endif

/*
 * Different schemes supported by the client.
 * Order means the order of preference between authentication schemes.
 *
 */
typedef enum _net_AuthType {
    AUTH_INVALID   = 0,
    AUTH_BASIC     = 1
#ifdef SIMPLE_MD5
	, AUTH_SIMPLEMD5 = 2		/* Much better than "Basic" */
#endif /* SIMPLE_MD5 */
} net_AuthType;


/*
 * This struct describes both Basic and SimpleMD5 authentication stuff,
 * for both HTTP servers and proxies.
 *
 */
typedef struct _net_AuthStruct {
    net_AuthType	auth_type;
    char *			path;			/* For normal authentication only */
	char *			proxy_addr;		/* For proxy authentication only */
	char *			username;		/* Obvious */
	char *			password;		/* Not too cryptic either */
	char *			auth_string;	/* Storage for latest Authorization str */
	char *			realm;			/* For all auth schemes */
#ifdef SIMPLE_MD5
    char *			domain;			/* SimpleMD5 only */
    char *			nonce;			/* SimpleMD5 only */
    char *			opaque;			/* SimpleMD5 only */
    XP_Bool			oldNonce;		/* SimpleMD5 only */
	int				oldNonce_retries;
#endif
} net_AuthStruct;



/*----------------- Normal client-server authentication ------------------ */

PRIVATE net_AuthStruct *
net_CheckForAuthorization(char * address, Bool exact_match)
{

    XP_List * list_ptr = net_auth_list;
    net_AuthStruct * auth_s;

	TRACEMSG(("net_CheckForAuthorization: checking for auth on: %s", address));

    while((auth_s = (net_AuthStruct *) XP_ListNextObject(list_ptr))!=0)
      {
		if(exact_match)
		  {
		    if(!XP_STRCMP(address, auth_s->path))
			    return(auth_s);
		  }
		else
		  {
			/* shorter strings always come last so there can be no
			 * ambiquity
			 */
		    if(!strncasecomp(address, auth_s->path, XP_STRLEN(auth_s->path)))
			    return(auth_s);
		  }
      }
   
    return(NULL);
}

/* returns TRUE if authorization is required
 */
PUBLIC Bool
NET_AuthorizationRequired(char * address)
{
    net_AuthStruct * rv;
	char * last_slash = XP_STRRCHR(address, '/');

	if(last_slash)
		*last_slash = '\0';

	rv = net_CheckForAuthorization(address, FALSE);

	if(last_slash)
		*last_slash = '/';

	if(!rv)
		return(FALSE);
	else
		return(TRUE);
}

/* returns a authorization string if one is required, otherwise
 * returns NULL
 */
PUBLIC char *
NET_BuildAuthString(MWContext * context, URL_Struct *URL_s)
{
	char * address = URL_s->address;
	net_AuthStruct * auth_s = net_CheckForAuthorization(address, FALSE);

	if(!auth_s)
#if defined(XP_WIN) && defined(MOZILLA_CLIENT)
		return(WFE_BuildCompuserveAuthString(URL_s));
#else
		return(NULL);
#endif
	else
	  {
	  	static char * auth_header = 0;
		
		if(auth_header)
			XP_FREE(auth_header);
		auth_header = PR_smprintf("Authorization: Basic %s"CRLF, auth_s->auth_string);
		return(auth_header);
	  }
}

PRIVATE net_AuthStruct *
net_ScanForHostnameRealmMatch(char * address, char * realm)
{
	char * proto_host = NET_ParseURL(address, GET_HOST_PART | GET_PROTOCOL_PART);
    XP_List * list_ptr = net_auth_list;
    net_AuthStruct * auth_s;

    while((auth_s = (net_AuthStruct *) XP_ListNextObject(list_ptr))!=0)
      {
        /* shorter strings always come last so there can be no
         * ambiquity
         */
        if(!strncasecomp(proto_host, auth_s->path, XP_STRLEN(proto_host))
			&& !strcasecomp(realm, auth_s->realm))
		{
			XP_FREE(proto_host);
            return(auth_s);
		}
      }
	XP_FREE(proto_host);
    return(NULL);
}

PRIVATE void
net_free_auth_struct(net_AuthStruct *auth)
{
    FREE(auth->path);
    FREE(auth->proxy_addr);
    FREE(auth->username);
    FREE(auth->password);
    FREE(auth->auth_string);
    FREE(auth->realm);
    FREE(auth);
}

/* blows away all authorization structs currently in the list */
PUBLIC void
NET_RemoveAllAuthorizations()
{
    XP_List * authList = net_auth_list;
	net_AuthStruct * victim;

	if(XP_ListIsEmpty(authList)) /* XP_ListIsEmpty handles null list */
		return;

    while((victim = (net_AuthStruct *) XP_ListNextObject(authList)) != 0)
	{
		net_free_auth_struct(victim);
		authList = net_auth_list;
	}
}

PRIVATE void
net_remove_exact_auth_match_on_cancel(net_AuthStruct *prev_auth, char *cur_path)
{
	if(!prev_auth || !cur_path)
		return;

	if(!XP_STRCMP(prev_auth->path, cur_path))
      {
        /* if the paths are exact and the user cancels
         * remove the mapping
         */
        XP_ListRemoveObject(net_auth_list, prev_auth);
		net_free_auth_struct(prev_auth);
	  }
}

/* returns false if the user wishes to cancel authorization
 * and TRUE if the user wants to continue with a new authorization
 * string
 */
/* HARDTS: I took a whack at fixing up some of the strings leaked in this 
 * function.  All the XP_FREEIF()s are new. 
 */
PUBLIC Bool 
NET_AskForAuthString(MWContext *context,
					 URL_Struct * URL_s, 
					 char * authenticate, 
					 char * prot_template,
					 Bool   already_sent_auth)
{
	net_AuthStruct * prev_auth;
	char *address = URL_s->address;
	char *host = NET_ParseURL(address, GET_HOST_PART);
	char * new_address=NULL;
	char *username=NULL;
	char *password=NULL;
	char *u_pass_string=NULL;
	char *auth_string=NULL;
	char * realm;
	char * slash;
	char * at_sign;
	char * authenticate_header_value;
	int32 len = 0;
	char *buf=0;
	int status;
	XP_Bool re_authorize = FALSE;

	TRACEMSG(("Entering NET_AskForAuthString"));

	if(authenticate)
	  {
		/* check for the compuserve Remote-Passphrase type of
	 	 * authentication
	 	 */
		authenticate_header_value = XP_StripLine(authenticate);

#define COMPUSERVE_HEADER_NAME "Remote-Passphrase"

		if(!strncasecomp(authenticate_header_value, 
					 COMPUSERVE_HEADER_NAME, 
					 sizeof(COMPUSERVE_HEADER_NAME) - 1))
	  	{
	  		/* This is a compuserv style header 
	  	 	*/

    XP_FREEIF(host);
#if defined(XP_WIN) && defined(MOZILLA_CLIENT)
		return(WFE_DoCompuserveAuthenticate(context, URL_s, authenticate_header_value));
#else
		return(NET_AUTH_FAILED_DISPLAY_DOCUMENT);		
#endif	
	  	}			 
	  }

	new_address = NET_ParseURL(address,
						GET_PROTOCOL_PART | GET_HOST_PART | GET_PATH_PART);

	if(!new_address) {
    XP_FREEIF(host);
		return NET_AUTH_FAILED_DISPLAY_DOCUMENT;
  }

	/* remove everything after the last slash
	 */
	slash = XP_STRRCHR(new_address, '/');
	if(slash)
		*slash = '\0';

	if(!authenticate)
	  {
		realm = "unknown";
	  }
	else
	  {
		realm = XP_STRCHR(authenticate, '"');
	
		if(realm)
		  {
			realm++;

			/* terminate at next quote */
			XP_STRTOK(realm, "\"");

#define MAX_REALM_SIZE 128
			if(XP_STRLEN(realm) > MAX_REALM_SIZE)
				realm[MAX_REALM_SIZE] = '\0';
	
      }
		else
	      {
		    realm = "unknown";
	 	  }
		
    }

	/* no hostname/realm match search for exact match
	 */
    prev_auth = net_CheckForAuthorization(new_address, FALSE);

    if(prev_auth && !already_sent_auth)
      {
		/* somehow the mapping changed since the time we sent
		 * the authorization.
		 * This happens sometimes because of the parrallel
		 * nature of the requests.
		 * In this case we want to just retry the connection
		 * since it will probably succede now.
		 */
    XP_FREEIF(host);
    XP_FREEIF(new_address);
		return(NET_RETRY_WITH_AUTH);
      }
    else if(prev_auth)
      {
		/* we sent the authorization string and it was wrong
		 */
        if(!FE_Confirm(context, XP_GetString(XP_CONFIRM_AUTHORIZATION_FAIL)))
		  {
        	TRACEMSG(("User canceled login!!!"));

			if(!XP_STRCMP(prev_auth->path, new_address))
			  {
				/* if the paths are exact and the user cancels
				 * remove the mapping
				 */
				net_remove_exact_auth_match_on_cancel(prev_auth, new_address);
        XP_FREEIF(host);
        XP_FREEIF(new_address);
            	return(NET_AUTH_FAILED_DISPLAY_DOCUMENT);
			  }
		  }
		
        username = XP_STRDUP(prev_auth->username);
        password = XP_STRDUP(prev_auth->password);
		re_authorize = TRUE;
      }
	else
	  {
		char *ptr1, *ptr2;

        /* scan all the authorization strings to see if we
         * can find a hostname and realm match.  If we find
         * a match then reduce the authorization path to include
         * the current path as well.
         */
        prev_auth = net_ScanForHostnameRealmMatch(address, realm);
    
        if(prev_auth)
          {
			char *tmp;

            /* compare the two url paths until they deviate
             * once they deviate truncate
             */
            for(ptr1 = prev_auth->path, ptr2 = new_address; *ptr1 && *ptr2; ptr1++, ptr2++)
			  {
                if(*ptr1 != *ptr2)
                  {
					break;        /* end for */
                  }
			  }
			/* truncate at *ptr1 now since the new address may
			 * be just a subpath of the original address and
			 * the compare above will not handle the subpath correctly
			 */
			*ptr1 = '\0'; /* truncate */

			if(*(ptr1-1) == '/')
				*(ptr1-1) = '\0'; /* strip a trailing slash */

			/* make sure a path always has at least a slash
			 * at the end.
			 * If the slash isn't there then
			 * the password will be sent to ports on the
			 * same host since we use a first part match
			 */
			tmp = NET_ParseURL(prev_auth->path, GET_PATH_PART);
			if(!*tmp)
				StrAllocCat(prev_auth->path, "/");
			FREE(tmp);				

			TRACEMSG(("Truncated new auth path to be: %s", prev_auth->path));

			FREE(host);
			FREE(new_address);
			return(NET_RETRY_WITH_AUTH);
          }
	  }
					 
	XP_FREEIF(host);
  host = NET_ParseURL(address, GET_HOST_PART);
	
	if(!host) {
    XP_FREEIF(new_address);
    XP_FREEIF(username);
    XP_FREEIF(password);
		return NET_AUTH_FAILED_DISPLAY_DOCUMENT;
  }

	at_sign = XP_STRCHR(host, '@');
	if (at_sign)
	  {
	  	char * colon;

	  	/* get username */
		*at_sign = '\0';
		
		colon = XP_STRCHR(host, ':');
		if (colon)
		  {
		  	*colon = '\0';
        XP_FREEIF(password);
		  	password = XP_STRDUP(colon+1);
		  }
		
    XP_FREEIF(username);
		username = XP_STRDUP(host);

	  }

	FREE(host);

#if 0
  /* Use username and/or password specified in URL_struct if exists. */
  if (!username && URL_s->username && *URL_s->username) {
    username = XP_STRDUP(URL_s->username);
  }
  if (!password && URL_s->password && *URL_s->password) {
    password = XP_STRDUP(URL_s->password);
  }
#endif

	/* if the password is filled in then the username must
	 * be filled in already.  
	 */
	if(!password || re_authorize)
	  {
	   	host = NET_ParseURL(address, GET_HOST_PART);

		/* malloc memory here to prevent buffer overflow */
		len = XP_STRLEN(XP_GetString(XP_ACCESS_ENTER_USERNAME));
		len += XP_STRLEN(realm) + XP_STRLEN(host) + 10;
		
		buf = (char *)XP_ALLOC(len*sizeof(char));
		
		if(buf)
		  {
			PR_snprintf( buf, len*sizeof(char), 
						XP_GetString(XP_ACCESS_ENTER_USERNAME), 
						realm, host);


			NET_Progress(context, XP_GetString( XP_CONNECT_PLEASE_ENTER_PASSWORD_FOR_HOST) );

      XP_FREEIF(username);
      XP_FREEIF(password);
			status = FE_PromptUsernameAndPassword(context, buf, 
											  	  &username, &password);
	
			FREE(buf);
		  }
		else
		  {
			status = 0;
		  }

		FREE(host);

		if(!status)
		  {
			TRACEMSG(("User canceled login!!!"));

			/* if the paths are exact and the user cancels
			 * remove the mapping
			 */
			net_remove_exact_auth_match_on_cancel(prev_auth, new_address);

      XP_FREEIF(username);
      XP_FREEIF(password);
      XP_FREEIF(new_address);
			return(NET_AUTH_FAILED_DISPLAY_DOCUMENT);
		  }
		else if(!username || !password)
		  {
      XP_FREEIF(username);
      XP_FREEIF(password);
      XP_FREEIF(new_address);
			return(NET_AUTH_FAILED_DISPLAY_DOCUMENT);
		  }
	  }

	StrAllocCopy(u_pass_string, username);
	StrAllocCat(u_pass_string, ":");
	StrAllocCat(u_pass_string, password);

	len = XP_STRLEN(u_pass_string);
	auth_string = (char*) XP_ALLOC((((len+1)*4)/3)+10);

	if(!auth_string)
	  {
    XP_FREEIF(username);
    XP_FREEIF(password);
  	XP_FREEIF(u_pass_string);
		FREE(new_address);
		return(NET_RETRY_WITH_AUTH);
	  }

	NET_UUEncode((unsigned char *)u_pass_string, (unsigned char*) auth_string, len);

	FREE(u_pass_string);

	if(prev_auth)
	  {
	    FREEIF(prev_auth->auth_string);
        prev_auth->auth_string = auth_string;
		FREEIF(prev_auth->username);
		prev_auth->username = username;
        FREEIF(prev_auth->password);
        prev_auth->password = password;
	  }
	else
	  {
		XP_List * list_ptr = net_auth_list;
		net_AuthStruct * tmp_auth_ptr;
		size_t new_len;

		/* construct a new auth_struct
		 */
		prev_auth = XP_NEW_ZAP(net_AuthStruct);
	    if(!prev_auth)
		  {
        XP_FREEIF(auth_string);
        XP_FREEIF(username);
        XP_FREEIF(password);
		    FREE(new_address);
		    return(NET_RETRY_WITH_AUTH);
		  }
		
        prev_auth->auth_string = auth_string;
		prev_auth->username = username;
        prev_auth->password = password;
		prev_auth->path = 0;
		StrAllocCopy(prev_auth->path, new_address);
		prev_auth->realm = 0;
		StrAllocCopy(prev_auth->realm, realm);

		if(!net_auth_list)
		  {
			net_auth_list = XP_ListNew();
		    if(!net_auth_list)
			  {
          /* Maybe should free prev_auth here. */
		   		FREE(new_address);
				return(NET_RETRY_WITH_AUTH);
			  }
		  }		

		/* add it to the list so that it is before any strings of
		 * smaller length
		 */
		new_len = XP_STRLEN(prev_auth->path);
		while((tmp_auth_ptr = (net_AuthStruct *) XP_ListNextObject(list_ptr))!=0)
		  { 
			if(new_len > XP_STRLEN(tmp_auth_ptr->path))
			  {
				XP_ListInsertObject(net_auth_list, tmp_auth_ptr, prev_auth);
		   		FREE(new_address);
				return(NET_RETRY_WITH_AUTH);
			  }
		  }
		/* no shorter strings found in list */	
		XP_ListAddObjectToEnd(net_auth_list, prev_auth);
	  }

	FREE(new_address);
	return(NET_RETRY_WITH_AUTH);
}

/*--------------------------------------------------
 * The following routines support the 
 * Set-Cookie: / Cookie: headers
 */

PRIVATE XP_List * net_cookie_list=0;

typedef struct _net_CookieStruct {
    char * path;
	char * host;
	char * name;
    char * cookie;
	time_t expires;
	time_t last_accessed;
	Bool   secure;      /* only send for https connections */
	Bool   is_domain;   /* is it a domain instead of an absolute host? */
} net_CookieStruct;

PRIVATE void
net_FreeCookie(net_CookieStruct * cookie)
{

	if(!cookie)
		return;

	XP_ListRemoveObject(net_cookie_list, cookie);

	FREEIF(cookie->path);
	FREEIF(cookie->host);
	FREEIF(cookie->name);
	FREEIF(cookie->cookie);

	FREE(cookie);

	cookies_changed = TRUE;
}

/* blows away all cookies currently in the list */
PUBLIC void
NET_RemoveAllCookies()
{
    XP_List * cookieList = net_cookie_list;
	net_CookieStruct * victim;

	if(XP_ListIsEmpty(cookieList)) /* XP_ListIsEmpty handles null list */
		return;

    while((victim = (net_CookieStruct *) XP_ListNextObject(cookieList)) != 0)
	{
		net_FreeCookie(victim);
		cookieList = net_cookie_list;
	}
}

PRIVATE void
net_remove_oldest_cookie(void)
{
    XP_List * list_ptr = net_cookie_list;
    net_CookieStruct * cookie_s;
    net_CookieStruct * oldest_cookie;

	if(XP_ListIsEmpty(list_ptr))
		return;

	oldest_cookie = (net_CookieStruct*) list_ptr->next->object;
	
    while((cookie_s = (net_CookieStruct *) XP_ListNextObject(list_ptr))!=0)
      {
		if(cookie_s->last_accessed < oldest_cookie->last_accessed)
			oldest_cookie = cookie_s;
	  }

	if(oldest_cookie)
	  {
		TRACEMSG(("Freeing cookie because global max cookies has been exceeded"));
	    net_FreeCookie(oldest_cookie);
	  }
}

/* checks to see if the maximum number of cookies per host
 * is being exceeded and deletes the oldest one in that
 * case
 */
PRIVATE void
net_CheckForMaxCookiesFromHost(const char * cur_host)
{
    XP_List * list_ptr = net_cookie_list;
    net_CookieStruct * cookie_s;
    net_CookieStruct * oldest_cookie = 0;
	int cookie_count = 0;

    if(XP_ListIsEmpty(list_ptr))
        return;

    while((cookie_s = (net_CookieStruct *) XP_ListNextObject(list_ptr))!=0)
      {
	    if(!strcasecomp(cookie_s->host, cur_host))
		  {
			cookie_count++;
			if(!oldest_cookie 
				|| oldest_cookie->last_accessed > cookie_s->last_accessed)
                oldest_cookie = cookie_s;
		  }
      }

	if(cookie_count >= MAX_COOKIES_PER_SERVER && oldest_cookie)
	  {
		TRACEMSG(("Freeing cookie because max cookies per server has been exceeded"));
        net_FreeCookie(oldest_cookie);
	  }
}


/* search for previous exact match
 */
PRIVATE net_CookieStruct *
net_CheckForPrevCookie(char * path,
                   char * hostname,
                   char * name)
{

    XP_List * list_ptr = net_cookie_list;
    net_CookieStruct * cookie_s;

    while((cookie_s = (net_CookieStruct *) XP_ListNextObject(list_ptr))!=0)
      {
		if(path 
			&& hostname 
				&& cookie_s->path
				   && cookie_s->host
					  && cookie_s->name
					 	 && !XP_STRCMP(name, cookie_s->name)
							&& !XP_STRCMP(path, cookie_s->path)
								&& !XP_STRCMP(hostname, cookie_s->host))
                return(cookie_s);
			
      }

    return(NULL);
}

#if 0
/* callback routine invoked by prefapi when the pref value changes */
MODULE_PRIVATE int PR_CALLBACK
net_CookiePrefChanged(const char * newpref, void * data)
{
	int32	n;

	PREF_GetIntPref(pref_cookie_acceptance, &n);
	NET_SetCookiePrefs((NET_CookiePrefsEnum)n);
	return PREF_NOERROR;
}
#endif

PRIVATE NET_CookiePrefsEnum
net_GetCookiePreference(void)
{
	return net_cookie_preference;
}

/* set the users cookie prefs
 */
PUBLIC void
NET_SetCookiePrefs(NET_CookiePrefsEnum pref_enum)
{
	net_cookie_preference = pref_enum;

	if(NET_DisableCookies == net_cookie_preference)
		XP_FileRemove("", xpHTTPCookie);
}

/* set the users foreign cookie prefs */
PUBLIC void
NET_SetForeignCookiePrefs(NET_ForeignCookiesEnum pref_enum)
{
	NET_ForeignCookies = pref_enum;
}

#if 0
/* callback routine invoked by prefapi when the pref value changes */
MODULE_PRIVATE int PR_CALLBACK
NET_ForeignCookiePrefChanged(const char * newpref, void * data)
{
	int32	n;

	PREF_GetIntPref(pref_foreign_cookies, &n);
	NET_SetForeignCookiePrefs((NET_ForeignCookiesEnum)n);
	return PREF_NOERROR;
}
#endif

PRIVATE NET_ForeignCookiesEnum
NET_GetForeignCookiePreference(void)
{
	return NET_ForeignCookies;
}

#if 0
/* called from mkgeturl.c, NET_InitNetLib(). This sets the module local cookie pref variables
   and registers the callbacks */
PUBLIC void
NET_RegisterCookiePrefCallbacks(void)
{
	int32	n;

	PREF_GetIntPref(pref_cookie_acceptance, &n);
	NET_SetCookiePrefs((NET_CookiePrefsEnum)n);
	PREF_RegisterCallback(pref_cookie_acceptance, net_CookiePrefChanged, NULL);

	PREF_GetIntPref(pref_foreign_cookies, &n);
	NET_SetForeignCookiePrefs((NET_ForeignCookiesEnum)n);
	PREF_RegisterCallback(pref_foreign_cookies, NET_ForeignCookiePrefChanged, NULL);
}
#endif

/* returns TRUE if authorization is required
 */
PUBLIC char *
NET_GetCookie(MWContext * context, char * address)
{
	char *name=0;
	char *host = NET_ParseURL(address, GET_HOST_PART);
	char *path = NET_ParseURL(address, GET_PATH_PART);
    XP_List * list_ptr = net_cookie_list;
    net_CookieStruct * cookie_s;
	Bool first=TRUE;
	Bool secure_path=FALSE;
	time_t cur_time = time(NULL);
	int host_length;
	int domain_length;

	/* keep this static so that no one else has to free it
	 */
	static char * rv=0;

	/* disable cookie's if the user's prefs say so
	 */
	if(NET_DisableCookies == net_GetCookiePreference())
		return NULL;

#ifndef NO_SSL /* added by jwz */
	if(!strncasecomp(address, "https", 5))
		secure_path = TRUE;
#endif /* NO_SSL */

	if(rv)
	  {
		/* free previous data */
		FREE(rv);
		rv = 0;
	  }

	/* search for all cookies
	 */
    while((cookie_s = (net_CookieStruct *) XP_ListNextObject(list_ptr))!=0)
      {
		if(!cookie_s->host)
			continue;

		/* check the host or domain first
		 */
		if(cookie_s->is_domain)
		  {
			char *cp;
			domain_length = XP_STRLEN(cookie_s->host);

			/* calculate the host length by looking at all characters up to
			 * a colon or '\0'.  That way we won't include port numbers
			 * in domains
			 */
			for(cp=host; *cp != '\0' && *cp != ':'; cp++)
				; /* null body */ 

			host_length = cp - host;
			if(domain_length > host_length 
				|| strncasecomp(cookie_s->host, 
								&host[host_length - domain_length], 
								domain_length))
			  {
				/* no match. FAIL 
				 */
				continue;
			  }
			
		  }
		else if(strcasecomp(host, cookie_s->host))
		  {
			/* hostname matchup failed. FAIL
			 */
			continue;
		  }

        /* shorter strings always come last so there can be no
         * ambiquity
         */
        if(cookie_s->path && !XP_STRNCMP(path,
                                         cookie_s->path,
                                         XP_STRLEN(cookie_s->path)))
          {

			/* if the cookie is secure and the path isn't
			 * dont send it
			 */
			if(cookie_s->secure && !secure_path)
				continue;  /* back to top of while */
			
			/* check for expired cookies
			 */
			if(cookie_s->expires && cookie_s->expires < cur_time)
			  {
				/* expire and remove the cookie 
				 */
		   		net_FreeCookie(cookie_s);

				/* start the list parsing over :(
				 * we must also start the string over
				 */
				FREE_AND_CLEAR(rv);
				list_ptr = net_cookie_list;
				first = TRUE; /* reset first */
				continue;
			  }

			if(first)
				first = FALSE;
			else
				StrAllocCat(rv, "; ");
			
			if(cookie_s->name && *cookie_s->name != '\0')
			  {
				cookie_s->last_accessed = cur_time;
            	StrAllocCopy(name, cookie_s->name);
            	StrAllocCat(name, "=");

#ifdef PREVENT_DUPLICATE_NAMES
				/* make sure we don't have a previous
				 * name mapping already in the string
				 */
				if(!rv || !XP_STRSTR(rv, name))
			      {	
            	    StrAllocCat(rv, name);
            	    StrAllocCat(rv, cookie_s->cookie);
				  }
#else
            	StrAllocCat(rv, name);
                StrAllocCat(rv, cookie_s->cookie);
#endif /* PREVENT_DUPLICATE_NAMES */
			  }
			else
			  {
            	StrAllocCat(rv, cookie_s->cookie);
			  }
          }
	  }

	FREEIF(name);
	FREE(path);
	FREE(host);

	/* may be NULL */
	return(rv);
}

/* Java script is calling NET_SetCookieString, netlib is calling this via NET_SetCookieStringFromHttp */
PUBLIC void 
NET_SetCookieString(MWContext * context, 
					char * cur_url,
					char * set_cookie_header)
{
	net_CookieStruct * prev_cookie;
	char * path_from_header=0;
	char * host_from_header=0;
	char * name_from_header=0;
	char * cookie_from_header=0;
	time_t expires=0;
	char * cur_path = NET_ParseURL(cur_url, GET_PATH_PART);
	char * cur_host = NET_ParseURL(cur_url, GET_HOST_PART);
	char * semi_colon;
	char * ptr, *equal;
	Bool   set_secure = FALSE;  /* default */
	Bool   is_domain = FALSE;
	
	if(NET_DisableCookies == net_GetCookiePreference()
		|| NET_DontAcceptNewCookies == net_GetCookiePreference())
		return;

	/* it will probably change every time we call this function
	 */
	cookies_changed = TRUE;

	if(XP_ListCount(net_cookie_list) > MAX_NUMBER_OF_COOKIES-1)
	  {
		net_remove_oldest_cookie();
	  }

	/* terminate at any carriage return or linefeed */
	for(ptr=set_cookie_header; *ptr; ptr++)
		if(*ptr == LF || *ptr == CR)
		  {
			*ptr = '\0';
			break;
		  }

	/* parse path and expires attributes from header if
 	 * present
	 */
	semi_colon = XP_STRCHR(set_cookie_header, ';');

	if(semi_colon)
	  {
		/* truncate at semi-colon and advance 
		 */
		*semi_colon++ = '\0';

		/* there must be some attributes. (hopefully)
		 */
		if(strcasestr(semi_colon, "secure"))
			set_secure = TRUE;

		/* look for the path attribute
		 */
		ptr = strcasestr(semi_colon, "path=");

		if(ptr)
		  {
			/* allocate more than we need */
			StrAllocCopy(path_from_header, XP_StripLine(ptr+5));
			/* terminate at first space or semi-colon
			 */
			for(ptr=path_from_header; *ptr != '\0'; ptr++)
				if(XP_IS_SPACE(*ptr) || *ptr == ';' || *ptr == ',')
				  {
					*ptr = '\0';
					break;
				  }
		  }

		/* look for the URI or URL attribute
		 *
		 * This might be a security hole so I'm removing
		 * it for now.
		 */

		/* look for a domain */
        ptr = strcasestr(semi_colon, "domain=");

        if(ptr)
          {
			char * domain_from_header=0;
			char * dot;
			char * colon;
			int domain_length;
			int cur_host_length;

            /* allocate more than we need */
			StrAllocCopy(domain_from_header, XP_StripLine(ptr+7));

            /* terminate at first space or semi-colon
             */
            for(ptr=domain_from_header; *ptr != '\0'; ptr++)
                if(XP_IS_SPACE(*ptr) || *ptr == ';' || *ptr == ',')
                  {
                    *ptr = '\0';
                    break;
                  }

            /* verify that this host has the authority to set for
             * this domain.   We do this by making sure that the
             * host is in the domain
             * We also require that a domain have at least two
             * periods to prevent domains of the form ".com"
             * and ".edu"
			 *
			 * Also make sure that there is more stuff after
			 * the second dot to prevent ".com."
             */
            dot = XP_STRCHR(domain_from_header, '.');
			if(dot)
                dot = XP_STRCHR(dot+1, '.');

			if(!dot || *(dot+1) == '\0')
			  {
				/* did not pass two dot test. FAIL
				 */
				FREEIF(domain_from_header);
				FREEIF(cur_path);
				FREEIF(cur_host);
				TRACEMSG(("DOMAIN failed two dot test"));
				return;
			  }

			/* strip port numbers from the current host
			 * for the domain test
			 */
			colon = XP_STRCHR(cur_host, ':');
			if(colon)
			   *colon = '\0';

			domain_length   = XP_STRLEN(domain_from_header);
			cur_host_length = XP_STRLEN(cur_host);

			/* check to see if the host is in the domain
			 */
			if(domain_length > cur_host_length
				|| strcasecomp(domain_from_header, 
							   &cur_host[cur_host_length-domain_length]))
			  {
				TRACEMSG(("DOMAIN failed host within domain test."
					  " Domain: %s, Host: %s", domain_from_header, cur_host));
				FREEIF(domain_from_header);
				FREEIF(cur_path);
				FREEIF(cur_host);
				return;
              }

			/* all tests passed, copy in domain to hostname field
			 */
			StrAllocCopy(host_from_header, domain_from_header);
			is_domain = TRUE;

			TRACEMSG(("Accepted domain: %s", host_from_header));

			FREE(domain_from_header);
          }

		/* now search for the expires header 
		 * NOTE: that this part of the parsing
		 * destroys the original part of the string
		 */
		ptr = strcasestr(semi_colon, "expires=");

		if(ptr)
		  {
			char *date =  ptr+8;
			/* terminate the string at the next semi-colon
			 */
			for(ptr=date; *ptr != '\0'; ptr++)
				if(*ptr == ';')
				  {
					*ptr = '\0';
					break;
				  }
			expires = NET_ParseDate(date);

			TRACEMSG(("Have expires date: %ld", expires));
		  }
	  }

	if(!path_from_header)
      {
        /* strip down everything after the last slash
         * to get the path.
         */
        char * slash = XP_STRRCHR(cur_path, '/');
        if(slash)
            *slash = '\0';

		path_from_header = cur_path;
	  }
	else
	  {
		FREE(cur_path);
	  }

	if(!host_from_header)
		host_from_header = cur_host;
	else
		FREE(cur_host);

	/* limit the number of cookies from a specific host or domain
	 */
	net_CheckForMaxCookiesFromHost(host_from_header);

	/* keep cookies under the max bytes limit 
	 */
	if(XP_STRLEN(set_cookie_header) > MAX_BYTES_PER_COOKIE)
		set_cookie_header[MAX_COOKIES_PER_SERVER-1] = '\0';

	/* separate the name from the cookie
	 */
	equal = XP_STRCHR(set_cookie_header, '=');

	if(equal)
	  {
		*equal = '\0';
		StrAllocCopy(name_from_header, XP_StripLine(set_cookie_header));
		StrAllocCopy(cookie_from_header, XP_StripLine(equal+1));
	  }
	else
	  {
		TRACEMSG(("Warning: no name found for cookie"));
		StrAllocCopy(cookie_from_header, XP_StripLine(set_cookie_header));
		StrAllocCopy(name_from_header, "");
	  }

	if(NET_WhineAboutCookies == net_GetCookiePreference())
	  {
		/* the user wants to know about cookies so let them
		 * know about every one that is set and give them
		 * the choice to accept it or not
		 */
		char * new_string = 0;
		char * tmp_host = NET_ParseURL(cur_url, GET_HOST_PART);

		StrAllocCopy(new_string, XP_GetString(MK_ACCESS_COOKIES_THE_SERVER));
		StrAllocCat(new_string, tmp_host ? tmp_host : "");
		StrAllocCat(new_string, XP_GetString(MK_ACCESS_COOKIES_WISHES));

		FREE(tmp_host);

		if(is_domain)
		  {
			StrAllocCat(new_string, XP_GetString(MK_ACCESS_COOKIES_TOANYSERV));
			StrAllocCat(new_string, host_from_header);
		  }
		else
		  {
			StrAllocCat(new_string, XP_GetString(MK_ACCESS_COOKIES_TOSELF));
		  }

		StrAllocCat(new_string, XP_GetString(MK_ACCESS_COOKIES_NAME_AND_VAL));

		StrAllocCat(new_string, name_from_header);
		StrAllocCat(new_string, "=");
		StrAllocCat(new_string, cookie_from_header);
		StrAllocCat(new_string, "\n");

		if(expires)
		  {
			StrAllocCat(new_string, XP_GetString(MK_ACCESS_COOKIES_COOKIE_WILL_PERSIST));
			StrAllocCat(new_string, ctime(&expires));
		  }

		StrAllocCat(new_string, XP_GetString(MK_ACCESS_COOKIES_SET_IT));

		if(!FE_Confirm(context, new_string))
			return;
	  }

	TRACEMSG(("mkaccess.c: Setting cookie: %s for host: %s for path: %s",
					cookie_from_header, host_from_header, path_from_header));
	
    prev_cookie = net_CheckForPrevCookie(path_from_header, 
								   		 host_from_header, 
								   		 name_from_header);

    if(prev_cookie)
      {
        prev_cookie->expires = expires;
        FREEIF(prev_cookie->cookie);
        FREEIF(prev_cookie->path);
        FREEIF(prev_cookie->host);
        FREEIF(prev_cookie->name);
        prev_cookie->cookie = cookie_from_header;
        prev_cookie->path = path_from_header;
        prev_cookie->host = host_from_header;
        prev_cookie->name = name_from_header;
        prev_cookie->secure = set_secure;
        prev_cookie->is_domain = is_domain;
		prev_cookie->last_accessed = time(NULL);
      }
	else
	  {
		XP_List * list_ptr = net_cookie_list;
		net_CookieStruct * tmp_cookie_ptr;
		size_t new_len;

        /* construct a new cookie_struct
         */
        prev_cookie = XP_NEW(net_CookieStruct);
        if(!prev_cookie)
          {
			FREEIF(path_from_header);
			FREEIF(host_from_header);
			FREEIF(name_from_header);
			FREEIF(cookie_from_header);
            return;
          }
    
        /* copy
         */
        prev_cookie->cookie  = cookie_from_header;
        prev_cookie->name    = name_from_header;
        prev_cookie->path    = path_from_header;
        prev_cookie->host    = host_from_header;
        prev_cookie->expires = expires;
        prev_cookie->secure  = set_secure;
        prev_cookie->is_domain = is_domain;
		prev_cookie->last_accessed = time(NULL);

		if(!net_cookie_list)
		  {
			net_cookie_list = XP_ListNew();
		    if(!net_cookie_list)
			  {
				FREEIF(path_from_header);
				FREEIF(name_from_header);
				FREEIF(host_from_header);
				FREEIF(cookie_from_header);
				FREE(prev_cookie);
				return;
			  }
		  }		

		/* add it to the list so that it is before any strings of
		 * smaller length
		 */
		new_len = XP_STRLEN(prev_cookie->path);
		while((tmp_cookie_ptr = (net_CookieStruct *) XP_ListNextObject(list_ptr))!=0)
		  { 
			if(new_len > XP_STRLEN(tmp_cookie_ptr->path))
			  {
				XP_ListInsertObject(net_cookie_list, tmp_cookie_ptr, prev_cookie);
				return;
			  }
		  }
		/* no shorter strings found in list */	
		XP_ListAddObjectToEnd(net_cookie_list, prev_cookie);
	  }

	return;
}

/* Determines whether the inlineHost is in the same domain as the currentHost. For use with rfc 2109
 * compliance/non-compliance. */
PRIVATE int
NET_SameDomain(char * currentHost, char * inlineHost)
{
	char * dot = 0;
	char * currentDomain = 0;
	char * inlineDomain = 0;

	if(!currentHost || !inlineHost)
		return 0;

	/* case insensitive compare */
	if(XP_STRCASECMP(currentHost, inlineHost) == 0)
		return 1;

	currentDomain = XP_STRCHR(currentHost, '.');
	inlineDomain = XP_STRCHR(inlineHost, '.');

	if(!currentDomain || !inlineDomain)
		return 0;
	
	/* check for at least two dots before continuing, if there are
	   not two dots we don't have enough information to determine
	   whether or not the inlineDomain is within the currentDomain */
	dot = XP_STRCHR(inlineDomain, '.');
	if(dot)
		dot = XP_STRCHR(dot+1, '.');
	else
		return 0;

	/* handle .com. case */
	if(!dot || (*(dot+1) == '\0') )
		return 0;

	if(!XP_STRCASECMP(inlineDomain, currentDomain))
		return 1;
	return 0;
}

/* This function wrapper wraps NET_SetCookieString for the purposes of determining
   whether or not a cookie is inline (we need the URL struct, and outputFormat to do so).
   this is called from NET_ParseMimeHeaders in mkhttp.c*/
PUBLIC void
NET_SetCookieStringFromHttp(FO_Present_Types outputFormat,
							URL_Struct * URL_s,
							MWContext * context, 
							char * cur_url,
							char * set_cookie_header)
{
	/* If the outputFormat is not PRESENT (the url is not going to the screen), and not
	*  SAVE AS (shift-click) then 
	*  the cookie being set is defined as inline so we need to do what the user wants us
	*  to based on his preference to deal with foreign cookies. If it's not inline, just set
	*  the cookie. */
	if(CLEAR_CACHE_BIT(outputFormat) != FO_PRESENT && CLEAR_CACHE_BIT(outputFormat) != FO_SAVE_AS)
	{
		if (NET_GetForeignCookiePreference() == NET_DoNotAllowForeignCookies)
		{
			/* the user doesn't want foreign cookies, check to see if its foreign */
			char * curSessionHistHost = 0;
			char * theColon = 0;
			char * curHost = NET_ParseURL(cur_url, GET_HOST_PART);
			History_entry * shistEntry = SHIST_GetCurrent(&context->hist);
			curSessionHistHost = NET_ParseURL(shistEntry->address, GET_HOST_PART);

			if(!curHost || !curSessionHistHost)
			{
				XP_FREEIF(curHost);
				XP_FREEIF(curSessionHistHost);
				return;
			}

			/* strip ports */
			theColon = XP_STRCHR(curHost, ':');
			if(theColon)
			   *theColon = '\0';
			theColon = XP_STRCHR(curSessionHistHost, ':');
			if(theColon)
				*theColon = '\0';

			/* if it's foreign, get out of here after a little clean up */
			if(!NET_SameDomain(curHost, curSessionHistHost))
			{
				XP_FREEIF(curHost);	
				XP_FREEIF(curSessionHistHost);
				return;
			}
			XP_FREEIF(curHost);	
			XP_FREEIF(curSessionHistHost);
		}
	}
	NET_SetCookieString(context, cur_url, set_cookie_header);
}

/* saves out the HTTP cookies to disk
 *
 * on entry pass in the name of the file to save
 *
 * returns 0 on success -1 on failure.
 *
 */
PUBLIC int
NET_SaveCookies(char * filename)
{
    XP_List * list_ptr = net_cookie_list;
    net_CookieStruct * cookie_s;
	time_t cur_date = time(NULL);
	XP_File fp;
	char date_string[36];

	if(NET_DisableCookies == net_GetCookiePreference())
	  return(-1);

	if(!cookies_changed)
	  return(-1);

	if(XP_ListIsEmpty(net_cookie_list))
		return(-1);

	if(!(fp = XP_FileOpen(filename, xpHTTPCookie, XP_FILE_WRITE)))
		return(-1);

	XP_FileWrite("# Netscape HTTP Cookie File" LINEBREAK
				 "# http://www.netscape.com/newsref/std/cookie_spec.html"
				 LINEBREAK "# This is a generated file!  Do not edit."
				 LINEBREAK LINEBREAK,
				 -1, fp);

	/* format shall be:
 	 *
	 * host \t is_domain \t path \t secure \t expires \t name \t cookie
	 *
	 * is_domain is TRUE or FALSE
	 * secure is TRUE or FALSE  
	 * expires is a time_t integer
	 * cookie can have tabs
	 */
    while((cookie_s = (net_CookieStruct *) XP_ListNextObject(list_ptr)) != NULL)
      {
		if(cookie_s->expires < cur_date)
			continue;  /* don't write entry if cookie has expired 
						* or has no expiration date
						*/

		XP_FileWrite(cookie_s->host, -1, fp);
		XP_FileWrite("\t", 1, fp);
		
		if(cookie_s->is_domain)
			XP_FileWrite("TRUE", -1, fp);
		else
			XP_FileWrite("FALSE", -1, fp);
		XP_FileWrite("\t", 1, fp);

		XP_FileWrite(cookie_s->path, -1, fp);
		XP_FileWrite("\t", 1, fp);

		if(cookie_s->secure)
		    XP_FileWrite("TRUE", -1, fp);
		else
		    XP_FileWrite("FALSE", -1, fp);
		XP_FileWrite("\t", 1, fp);

		PR_snprintf(date_string, sizeof(date_string), "%lu", cookie_s->expires);
		XP_FileWrite(date_string, -1, fp);
		XP_FileWrite("\t", 1, fp);

		XP_FileWrite(cookie_s->name, -1, fp);
		XP_FileWrite("\t", 1, fp);

		XP_FileWrite(cookie_s->cookie, -1, fp);
 		XP_FileWrite(LINEBREAK, -1, fp);
      }

	cookies_changed = FALSE;

	XP_FileClose(fp);

    return(0);
}


/* reads HTTP cookies from disk
 *
 * on entry pass in the name of the file to read
 *
 * returns 0 on success -1 on failure.
 *
 */
#define LINE_BUFFER_SIZE 4096

PUBLIC int
NET_ReadCookies(char * filename)
{
    XP_List * list_ptr = net_cookie_list;
    net_CookieStruct *new_cookie, *tmp_cookie_ptr;
	size_t new_len;
    XP_File fp;
	char buffer[LINE_BUFFER_SIZE];
	char *host, *is_domain, *path, *secure, *expires, *name, *cookie;
	Bool added_to_list;

    if(!(fp = XP_FileOpen(filename, xpHTTPCookie, XP_FILE_READ)))
        return(-1);

    /* format is:
     *
     * host \t is_domain \t path \t secure \t expires \t name \t cookie
     *
     * is_domain is TRUE or FALSE	-- defaulting to FALSE
     * secure is TRUE or FALSE   -- should default to TRUE
     * expires is a time_t integer
     * cookie can have tabs
     */
    while(XP_FileReadLine(buffer, LINE_BUFFER_SIZE, fp))
      {
		added_to_list = FALSE;

		if (*buffer == '#' || *buffer == CR || *buffer == LF || *buffer == 0)
		  continue;

		host = buffer;
		
		is_domain = XP_STRCHR(host, '\t');
		
		if(!is_domain)
			continue;
			
		*is_domain++ = '\0';
		
		path = XP_STRCHR(is_domain, '\t');

		if(!path)
			continue;
			
		*path++ = '\0';

		secure = XP_STRCHR(path, '\t');

		if(!secure)
			continue;
		
		*secure++ = '\0';

		expires = XP_STRCHR(secure, '\t');

		if(!expires)
			continue;
		
		*expires++ = '\0';

        name = XP_STRCHR(expires, '\t');

        if(!name)
            continue;

        *name++ = '\0';

        cookie = XP_STRCHR(name, '\t');

        if(!cookie)
            continue;

        *cookie++ = '\0';

		/* remove the '\n' from the end of the cookie */
		XP_StripLine(cookie);

        /* construct a new cookie_struct
         */
        new_cookie = XP_NEW(net_CookieStruct);
        if(!new_cookie)
          {
            return(-1);
          }

		XP_MEMSET(new_cookie, 0, sizeof(net_CookieStruct));
    
        /* copy
         */
        StrAllocCopy(new_cookie->cookie, cookie);
        StrAllocCopy(new_cookie->name, name);
        StrAllocCopy(new_cookie->path, path);
        StrAllocCopy(new_cookie->host, host);
        new_cookie->expires = atol(expires);
		if(!XP_STRCMP(secure, "FALSE"))
        	new_cookie->secure = FALSE;
		else
        	new_cookie->secure = TRUE;
        if(!XP_STRCMP(is_domain, "TRUE"))
        	new_cookie->is_domain = TRUE;
        else
        	new_cookie->is_domain = FALSE;

		if(!net_cookie_list)
		  {
			net_cookie_list = XP_ListNew();
		    if(!net_cookie_list)
			  {
				return(-1);
			  }
		  }		

		/* add it to the list so that it is before any strings of
		 * smaller length
		 */
		new_len = XP_STRLEN(new_cookie->path);
		while((tmp_cookie_ptr = (net_CookieStruct *) XP_ListNextObject(list_ptr)) != NULL)
		  { 
			if(new_len > XP_STRLEN(tmp_cookie_ptr->path))
			  {
				XP_ListInsertObject(net_cookie_list, tmp_cookie_ptr, new_cookie);
				added_to_list = TRUE;
				break;
			  }
		  }

		/* no shorter strings found in list */	
		if(!added_to_list)
		    XP_ListAddObjectToEnd(net_cookie_list, new_cookie);
	  }

    XP_FileClose(fp);

	cookies_changed = FALSE;

    return(0);
}



/* --- New stuff: General auth utils (currently used only by proxy auth) --- */

/*
 * Figure out the authentication scheme used; currently supported:
 *
 *		* Basic
 *		* SimpleMD5
 *
 */
PRIVATE net_AuthType
net_auth_type(char *name)
{
	if (name) {
		while (*name && XP_IS_SPACE(*name))
			name++;
		if (!strncasecomp(name, "basic", 5))
			return AUTH_BASIC;
#ifdef SIMPLE_MD5
		else if (!strncasecomp(name, "simplemd5", 9))
			return AUTH_SIMPLEMD5;
#endif
	}
	return AUTH_INVALID;
}


/*
 * Figure out better of two {WWW,Proxy}-Authenticate headers;
 * SimpleMD5 is better than Basic.  Uses the order of AuthType
 * enum values.
 *
 */
MODULE_PRIVATE XP_Bool
net_IsBetterAuth(char *new_auth, char *old_auth)
{
	if (!old_auth || net_auth_type(new_auth) >= net_auth_type(old_auth))
		return TRUE;
	else
		return FALSE;
}


/*
 * Turns binary data of given lenght into a newly-allocated HEX string.
 *
 *
 */
PRIVATE char *
bin2hex(unsigned char *data, int len)
{
    char *buf = (char *)XP_ALLOC(2 * len + 1);
    char *p = buf;

	if(!buf)
		return NULL;

    while (len-- > 0) {
		sprintf(p, "%02x", *data);
		p += 2;
		data++;
    }
    *p = '\0';
    return buf;
}


/*
 * Parse {WWW,Proxy}-Authenticate header parameters into a net_AuthStruct
 * structure.
 *
 */
#define SKIP_WS(p) while((*(p)) && XP_IS_SPACE(*(p))) p++

PRIVATE XP_Bool
next_params(char **pp, char **name, char **value)
{
    char *q, *p = *pp;

    SKIP_WS(p);
    if (!p || !(*p) || !(q = strchr(p, '=')))
		return FALSE;
    *name = p;
    *q++ = '\0';
    if (*q == '"') {
		*value = q + 1;
		q = strchr(*value, '"');
		if (q)
		  *q++ = '\0';
    }
    else {
		*value = q;
		while (*q && !XP_IS_SPACE(*q)) q++;
		if (*q)
		  *q++ = '\0';
    }

    *pp = q;
    return TRUE;
}

PRIVATE net_AuthStruct *
net_parse_authenticate_line(char *auth, net_AuthStruct *ret)
{
    char *name, *value, *p = auth;

	if (!auth || !*auth)
		return NULL;

	if (!ret)
		ret = XP_NEW_ZAP(net_AuthStruct);

	if(!ret)
		return NULL;

    SKIP_WS(p);
	ret->auth_type = net_auth_type(p);
    while (*p && !XP_IS_SPACE(*p)) p++;

    while (next_params(&p, &name, &value)) {
		if (!strcasecomp(name, "realm"))
		  {
			  StrAllocCopy(ret->realm, value);
		  }
#ifdef SIMPLE_MD5
		else if (!strcasecomp(name, "domain"))
		  {
			  StrAllocCopy(ret->domain, value);
		  }
		else if (!strcasecomp(name, "nonce"))
		  {
			  StrAllocCopy(ret->nonce, value);
		  }
		else if (!strcasecomp(name, "opaque"))
		  {
			  StrAllocCopy(ret->opaque, value);
		  }
		else if (!strcasecomp(name, "oldnonce"))
		  {
			  ret->oldNonce = (!strcasecomp(value, "TRUE")) ? TRUE : FALSE;
		  }
#endif /* SIMPLE_MD5 */
    }

#ifdef SIMPLE_MD5
	if (!ret->oldNonce)
		ret->oldNonce_retries = 0;
#endif /* SIMPLE_MD5 */

    return ret;
}


#ifdef SIMPLE_MD5
/* ---------- New stuff: SimpleMD5 Authentication for proxies -------------- */

PRIVATE void do_md5(unsigned char *stuff, unsigned char digest[16])
{
	MD5Context *cx = MD5_NewContext();
	unsigned int len;

	if (!cx)
		return;

	MD5_Begin(cx);
	MD5_Update(cx, stuff, strlen((char*)stuff));
	MD5_End(cx, digest, &len, 16);	/* len had better be 16 when returned! */

	MD5_DestroyContext(cx, (DSBool)TRUE);
}


/*
 * Generate a response for a SimpleMD5 challenge.
 *
 *	HEX( MD5("challenge password method url"))
 *
 */
char *net_generate_md5_challenge_response(char *challenge,
										  char *password,
										  int   method,
										  char *url)
{
    unsigned char digest[16];
    unsigned char *cookie =
	  (unsigned char *)XP_ALLOC(strlen(challenge) + strlen(password) +
								strlen(url) + 10);

	if(!cookie)
		return NULL;

    sprintf((char *)cookie, "%s %s %s %s", challenge, password,
			(method==URL_POST_METHOD ? "POST" :
			 method==URL_HEAD_METHOD ? "HEAD" : "GET"),
			url);
    do_md5(cookie, digest);
    return bin2hex(digest, 16);
}


#define SIMPLEMD5_AUTHORIZATION_FMT "SimpleMD5\
 username=\"%s\",\
 realm=\"%s\",\
 nonce=\"%s\",\
 response=\"%s\",\
 opaque=\"%s\""

#endif /* SIMPLE_MD5 */

PRIVATE
char *net_generate_auth_string(URL_Struct *url_s,
							   net_AuthStruct *auth_s)
{
	if (!auth_s)
		return NULL;

	switch (auth_s->auth_type) {

	  case AUTH_INVALID:
		break;

	  case AUTH_BASIC:
		if (!auth_s->auth_string) {
			int len;
			char *u_pass_string = NULL;

			StrAllocCopy(u_pass_string, auth_s->username);
			StrAllocCat (u_pass_string, ":");
			StrAllocCat (u_pass_string, auth_s->password);

			len = XP_STRLEN(u_pass_string);
			if (!(auth_s->auth_string = (char*) XP_ALLOC((((len+1)*4)/3)+20)))
			  {
				  return NULL;
			  }

			XP_STRCPY(auth_s->auth_string, "Basic ");
			NET_UUEncode((unsigned char *)u_pass_string,
						 (unsigned char *)&auth_s->auth_string[6],
						 len);

			FREE(u_pass_string);
		}
		break;

#ifdef SIMPLE_MD5
	  case AUTH_SIMPLEMD5:
	    if (auth_s->username && auth_s->password &&
			auth_s->nonce    && auth_s->opaque   &&
			url_s            && url_s->address)
		  {
			  char *resp;

			  FREEIF(auth_s->auth_string);
			  auth_s->auth_string = NULL;

			  if ((resp = net_generate_md5_challenge_response(auth_s->nonce,
															  auth_s->password,
															  url_s->method,
															  url_s->address)))
				{
					if ((auth_s->auth_string =
						 (char *)XP_ALLOC(XP_STRLEN(auth_s->username) +
										  XP_STRLEN(auth_s->realm)    +
										  XP_STRLEN(auth_s->nonce)    +
										  XP_STRLEN(resp)             +
										  XP_STRLEN(auth_s->opaque)   +
										  100)))
					  {
						  sprintf(auth_s->auth_string,
								  SIMPLEMD5_AUTHORIZATION_FMT,
								  auth_s->username,
								  auth_s->realm,
								  auth_s->nonce,
								  resp,
								  auth_s->opaque);
					  }
					FREE(resp);
				}
		  }
		break;
#endif /* SIMPLE_MD5 */
    }

	return auth_s->auth_string;
}


/* --------------- New stuff: client-proxy authentication --------------- */

PRIVATE net_AuthStruct *
net_CheckForProxyAuth(char * proxy_addr)
{
	XP_List * lp = net_proxy_auth_list;
	net_AuthStruct * s;

	while ((s = (net_AuthStruct *)XP_ListNextObject(lp)) != NULL)
      {
		  if (!strcasecomp(s->proxy_addr, proxy_addr))
			  return s;
	  }

    return NULL;
}


/*
 * returns a proxy authorization string if one is required, otherwise
 * returns NULL
 */
PUBLIC char *
NET_BuildProxyAuthString(MWContext * context,
						 URL_Struct * url_s,
						 char * proxy_addr)
{
	net_AuthStruct * auth_s = net_CheckForProxyAuth(proxy_addr);

	return auth_s ? net_generate_auth_string(url_s, auth_s) : NULL;
}


/*
 * Returns FALSE if the user wishes to cancel proxy authorization
 * and TRUE if the user wants to continue with a new authorization
 * string.
 */
#define INVALID_AUTH_HEADER XP_GetString( XP_PROXY_REQUIRES_UNSUPPORTED_AUTH_SCHEME )

#define LOOPING_OLD_NONCES XP_GetString( XP_LOOPING_OLD_NONCES )

PUBLIC XP_Bool
NET_AskForProxyAuth(MWContext * context,
					char *   proxy_addr,
					char *   pauth_params,
					XP_Bool  already_sent_auth)
{
	net_AuthStruct * prev;
	XP_Bool new_entry = FALSE;
	char * username = NULL;
	char * password = NULL;
	char * buf;
	int32  len=0;

	TRACEMSG(("Entering NET_AskForProxyAuth"));

	if (!proxy_addr || !*proxy_addr || !pauth_params || !*pauth_params)
		return FALSE;

	prev = net_CheckForProxyAuth(proxy_addr);
    if (prev) {
		new_entry = FALSE;
		net_parse_authenticate_line(pauth_params, prev);
	}
	else {
		new_entry = TRUE;
		if (!(prev = net_parse_authenticate_line(pauth_params, NULL)))
		  {
			  FE_Alert(context, INVALID_AUTH_HEADER);
			  return FALSE;
		  }
		StrAllocCopy(prev->proxy_addr, proxy_addr);
	}

	if (!prev->realm || !*prev->realm)
		StrAllocCopy(prev->realm, XP_GetString( XP_UNIDENTIFIED_PROXY_SERVER ) );

    if (!new_entry) {
		if (!already_sent_auth)
		  {
			  /* somehow the mapping changed since the time we sent
			   * the authorization.
			   * This happens sometimes because of the parrallel
			   * nature of the requests.
			   * In this case we want to just retry the connection
			   * since it will probably succeed now.
			   */
			  return TRUE;
		  }
#ifdef SIMPLE_MD5
		else if (prev->oldNonce && prev->oldNonce_retries++ < 3)
		  {
			  /*
			   * We already sent the authorization string and the
			   * nonce was expired -- auto-retry.
			   */
			  if (!FE_Confirm(context, LOOPING_OLD_NONCES))
				  return FALSE;
		  }
#endif /* SIMPLE_MD5 */
		else
		  {
			  /*
			   * We already sent the authorization string and it failed.
			   */
			  if (!FE_Confirm(context, XP_GetString(XP_CONFIRM_PROXYAUTHOR_FAIL)))
				  return FALSE;
		  }
	}

	username = prev->username;
	password = prev->password;

	len = XP_STRLEN(prev->realm) + XP_STRLEN(proxy_addr) + 50;
	buf = (char*)XP_ALLOC(len*sizeof(char));

	if(buf)
	  {
		PR_snprintf(buf, len*sizeof(char), XP_GetString( XP_PROXY_AUTH_REQUIRED_FOR ), prev->realm, proxy_addr);

		NET_Progress(context, XP_GetString( XP_CONNECT_PLEASE_ENTER_PASSWORD_FOR_PROXY ) );
		len = FE_PromptUsernameAndPassword(context, buf, 
									       &username, &password);
		FREE(buf);
	  }
	else
	  {
		len = 0;
	  }

	if (!len)
	  {
		  TRACEMSG(("User canceled login!!!"));
		  return FALSE;
	  }
	else if (!username || !password)
	  {
		  return FALSE;
	  }

	FREEIF(prev->auth_string);
	prev->auth_string = NULL;		/* Generate a new one */
	FREEIF(prev->username);
	prev->username = username;
	FREEIF(prev->password);
	prev->password = password;

	if (new_entry)
	  {
		  if (!net_proxy_auth_list)
			{
				net_proxy_auth_list = XP_ListNew();
				if (!net_proxy_auth_list)
				  {
					  return TRUE;
				  }
			}
		  XP_ListAddObjectToEnd(net_proxy_auth_list, prev);
	  }

	return TRUE;
}

