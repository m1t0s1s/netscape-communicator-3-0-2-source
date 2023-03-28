
#ifndef MKACCESS_H
#define MKACCESS_H

/* returns TRUE if authorization is required
 */
extern Bool NET_AuthorizationRequired(char * address);

/* returns false if the user wishes to cancel authorization
 * and TRUE if the user wants to continue with a new authorization
 * string
 */
extern Bool NET_AskForAuthString(MWContext * context,
                     				URL_Struct *URL_s,
                     				char * authenticate,
                     				char * prot_template,
									Bool already_sent_auth);

/* returns a authorization string if one is required, otherwise
 * returns NULL
 */
extern char * NET_BuildAuthString(MWContext * context, URL_Struct *URL_s);

/* removes all authorization structs from the auth list */
extern void
NET_RemoveAllAuthorizations();

/* removes all cookies structs from the cookie list */
extern void
NET_RemoveAllCookies();

/* returns TRUE if authorization is required
 */
extern char *
NET_GetCookie(MWContext * context, char * address);

/* returns false if the user wishes to cancel authorization
 * and TRUE if the user wants to continue with a new authorization
 * string
 */
extern void
NET_SetCookieString(MWContext * context,
                     char * cur_url,
                     char * set_cookie_header);

/* wrapper of NET_SetCookieString for netlib use. We need outformat and url_struct to determine
 * whether we're dealing with inline cookies */
extern void
NET_SetCookieStringFromHttp(FO_Present_Types outputFormat,
							URL_Struct * URL_s,
							MWContext * context, 
							char * cur_url,
							char * set_cookie_header);

/* saves out the HTTP cookies to disk
 *
 * on entry pass in the name of the file to save
 *
 * returns 0 on success -1 on failure.
 *
 */
extern int NET_SaveCookies(char * filename);

/* reads HTTP cookies from disk
 *
 * on entry pass in the name of the file to read
 *
 * returns 0 on success -1 on failure.
 *
 */
extern int NET_ReadCookies(char * filename);


/*
 * Builds the Proxy-authorization string
 */
extern char *
NET_BuildProxyAuthString(MWContext * context,
			 URL_Struct * url_s,
			 char * proxy_addr);

/*
 * Returns FALSE if the user wishes to cancel proxy authorization
 * and TRUE if the user wants to continue with a new authorization
 * string.
 */
PUBLIC XP_Bool
NET_AskForProxyAuth(MWContext * context,
		    char *   proxy_addr,
		    char *   pauth_params,
		    XP_Bool  already_sent_auth);

/*
 * Figure out better of two {WWW,Proxy}-Authenticate headers;
 * SimpleMD5 is better than Basic.  Uses the order of AuthType
 * enum values.
 *
 */
extern XP_Bool
net_IsBetterAuth(char *new_auth, char *old_auth);

#endif /* MKACCESS_H */
