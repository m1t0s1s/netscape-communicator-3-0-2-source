/* Public API for searching mail, news, and LDAP */

#ifndef _MSG_SRCH_H
#define _MSG_SRCH_H

typedef enum
{
	SearchError_First,						/* no functions return this; just for bookkeeping */
	SearchError_Success,					/* no error */
	SearchError_NotImplemented,				/* coming soon */

	SearchError_OutOfMemory,				/* can't allocate required memory */
	SearchError_IndexOutOfRange,			/* no list element at index */
	SearchError_NullPointer,				/* a required pointer parameter was null */
	SearchError_AttributeScopeAgreement,	/* attrib type not supported in this scope */

	SearchError_ResultSetEmpty,				/* search completed, but no matches found */
	SearchError_ResultSetTooBig,			/* too many matches to get them all */

	SearchError_InvalidAttribute,			/* specified attrib not in enum */
	SearchError_InvalidScope,				/* specified scope not in enum */
	SearchError_InvalidOperator,			/* specified op not in enum */
	
	SearchError_InvalidSearchTerm,			/* cookie for search term is bogus */
	SearchError_InvalidSearchScope,			/* cookie for scope term is bogus */
	SearchError_InvalidResultElement,		/* cookie for result element is bogus */

	SearchError_Timeout,					/* network didn't respond */
	SearchError_WaitingForNetwork,			/* nor finished getting output from server */

	SearchError_DateNotSupportedByServer,	/* our news server has capabilities INN doesn't */
	SearchError_Last						/* no functions return this; just for bookkeeping */
} MSG_SearchError;

typedef enum
{
	/* mail and news */
	attribSender			= 0x0001,	
	attribSubject			= 0x0002,	
	attribBody				= 0x0004,	
	attribDate				= 0x0008,	

	/* mail only */
	attribStatus			= 0x0010,	
	attribPriority			= 0x0020,	
	attribRecipientStatus	= 0x0040,	

	/* LDAP only */
	attribCommonName		= 0x0080,	
	attribGivenName			= 0x0100,	
	attribSurName			= 0x0200,	
	attrib822Address		= 0x0400,	
	attribPhoneNumber		= 0x0800	
} MSG_SearchAttribute;

typedef enum
{
	/* for text attributes */
	opContains,
	opDoesntContain,
	opIs,
	opIsnt,

	/* for date attributes */
	opIsBefore,
	opIsAfter,

	/* for message status */
	opMsgStatusIs,

	/* for priority. opIs also applies */
	opIsHigherThan,
	opIsLowerThan,

	/* for recipient status */
	opRecipStatusIs,

	/* for LDAP phoenetic matching */
	opSoundsLike
} MSG_SearchOperator;

typedef enum
{
	scopeMailFolder,
	scopeNewsGroup,
	scopeLdapDirectory
} MSG_ScopeAttribute;

typedef struct MSG_SearchValue
{
	MSG_SearchAttribute attribute;
	union
	{
		char string[256];
		uint16 priority;
		time_t date;
		uint16 msgStatus; /* relying on other headers for hasBeenReplied, hasBeenRead, and the negative of each */
		uint16 recipStatus; /* relying on other headers for onCCList, onToList, onlyRecipient */
	} u;
} MSG_SearchValue;

/* forward-declare these opaque types which are private to LIBMSG */
typedef struct MSG_SearchTerm MSG_SearchTerm;
typedef struct MSG_ScopeTerm MSG_ScopeTerm;
typedef struct MSG_ResultElement MSG_ResultElement;

XP_BEGIN_PROTOS

MSG_SearchError MSG_GetSearchableNewsAttributes (
	MSG_SearchAttribute *result);	/* capabilities of the news server */

typedef void (*MSG_SearchCallback) (
	XP_List *results);				/* linked list of MSG_ResultElement */

/* manage elements in the list of search terms */
MSG_SearchError MSG_CreateSearchTerm (
	MSG_SearchAttribute attrib,		/* attribute for this term */
	MSG_SearchOperator op,			/* operator e.g. opContains */
	MSG_SearchValue *value,			/* value e.g. "Dogbert" */
	MSG_SearchTerm **result);		/* cookie representing this term */
MSG_SearchError MSG_DestroySearchTerm (MSG_SearchTerm *);
MSG_SearchError MSG_DestroySearchList (XP_List *);

/* manage elements in the list of scope terms */
MSG_SearchError MSG_CreateScopeTerm (
	MSG_ScopeAttribute scope,		/* scope of this search e.g. in newsgroups */
	char *value,					/* name of scope e.g. mcom.dev.dogbert */
	MSG_ScopeTerm **result);		/* cookie representing this term */
MSG_SearchError MSG_DestroyScopeTerm (MSG_ScopeTerm *);
MSG_SearchError MSG_DestroyScopeList (XP_List *);

/* Fire off a search */
MSG_SearchError MSG_Search (
	XP_List *scopeList,				/* linked list of places to look */
	XP_List *termList,				/* linked list of criteria terms */
	MSG_SearchCallback callback,	/* FE func ptr we'll call when search is done */
	MSG_SearchAttribute sortKey);	/* which attribute to sort results on */

/* manage elements in list of search hits */
MSG_SearchError MSG_GetResultAttribute (
	MSG_ResultElement *element,		/* cookie out of the list we gave to FE in callback */
	MSG_SearchAttribute attrib,		/* which attribute to get value for */
	MSG_SearchValue *result);		/* filled in value */
MSG_SearchError MSG_DestroyResultElement (MSG_ResultElement *);
MSG_SearchError MSG_DestroyResultList (XP_List *);
MSG_SearchError MSG_OpenResultElement (MSG_ResultElement *); /* on double-click */

XP_END_PROTOS

#endif /* _MSG_SRCH_H */