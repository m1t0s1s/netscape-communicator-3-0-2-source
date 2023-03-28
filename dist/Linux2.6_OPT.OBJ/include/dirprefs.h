/* Netscape-specific (maybe Navigator-specific?) directory APIs */

#ifndef _DIRPREFS_H_
#define _DIRPREFS_H_


typedef struct DIR_Server
{
	char *description;		/* optional, but looks nice         */
	char *serverName;		/* required                         */
	char *searchBase;		/* optional, use it if you have it  */
	char *htmlGateway;      /* optional, may be null if none    */
	int port;				/* optional                         */
	int maxHits;			/* optional                         */
	XP_Bool isSecure;		/* required                         */
	XP_Bool savePassword;	/* required                         */
} DIR_Server;

XP_BEGIN_PROTOS

/* Since the strings in DIR_Server are allocated, we have bottleneck
 * routines to help with memory mgmt
 */
int DIR_CopyServer (DIR_Server *in, DIR_Server **out);
int DIR_DeleteServer (DIR_Server *);

int DIR_ValidateServer (DIR_Server *);

int DIR_GetLdapServers (XP_List *wholeList, XP_List **subList);
int DIR_GetHtmlServers (XP_List *wholeList, XP_List **subList);

XP_END_PROTOS


#endif /* dirprefs.h */
