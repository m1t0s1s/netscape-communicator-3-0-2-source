/* Public API for mail (and news?) filters */
#ifndef MSG_RULE_H
#define MSG_RULE_H

/*
	Terminology -	Filter - either a Rule (defined with GUI) or a (Java) Script
					Rule - 
*/
#include "msg_srch.h"

typedef enum
{
	FilterError_Success = 0,	/* no error */
	FilterError_First = SearchError_Last + 1,		/* no functions return this; just for bookkeeping */
	FilterError_NotImplemented,	/* coming soon */
	FilterError_NullPointer,	/* a required pointer parameter was null */
	FilterError_NotRule,		/* tried to get rule for non-rule filter */
	FilterError_NotScript,		/* tried to get a script name for a non-script filter */
	FilterError_Last,			/* no functions return this; just for bookkeeping */
} MSG_FilterError;


typedef enum
 {
         acChangePriority,
         acDelete,
         acMoveToFolder,
         acMarkRead,
         acMarkIgnored,
		 acWatchThread,
 } MSG_RuleActionType;

typedef enum 
{
	filterInboxRule = 0x1,
	filterInboxJavaScript = 0x2,
	filterNewsRule = 0x4,
	filterNewsJavaScript = 0x8,
} MSG_FilterType;

/* opaque struct defs - defined in libmsg/pmsgfilt.h */
typedef struct MSG_Filter MSG_Filter;
typedef struct MSG_Rule MSG_Rule;
typedef struct MSG_RuleAction MSG_RuleAction;

XP_BEGIN_PROTOS

/* Front ends call MSG_GetFilterList to get an XP_List of existing MSG_Filter *.
	These are manipulated by the front ends as a result of user interaction
   with dialog boxes. To apply the new list, fe's call MSG_SetFilterList.

   For example, if the user brings up the rule management UI, deletes a rule,
   and presses OK, the front end calls MSG_GetFilterList, iterates through the 
   list to display the filters, deletes the list element corresponding to the
   filter deleted, calls MSG_DestroyFilter to free it, and calls MSG_SetFilterList
   to apply the new list. Then, when the dialog comes down, call MSG_DestroyFilterList.

  Rule ordering is same as the XP_List ordering.
*/
MSG_FilterError MSG_GetFilterList(XP_List **filterList, MSG_FilterType type, MSG_ScopeTerm *scope);
MSG_FilterError MSG_SetFilterList(XP_List *filterList);
MSG_FilterError MSG_DestroyFilterList(XP_List *filterList);

/* In general, any data gotten with MSG_*Get is good until the owning object
   is deleted, or the data is replaced with a MSG_*Set call. For example, the name
   returned in MSG_GetFilterName is valid until either the filter is destroyed,
   or MSG_SetFilterName is called on the same filter.
 */
MSG_FilterError MSG_CreateFilter (MSG_FilterType type,	char *name,	MSG_Filter *result);			
MSG_FilterError MSG_DestroyFilter(MSG_Filter *filter);
MSG_FilterError MSG_GetFilterType(MSG_Filter *, MSG_FilterType *filterType);
MSG_FilterError MSG_EnableFilter(MSG_Filter *, XP_Bool enable);
MSG_FilterError MSG_IsFilterEnabled(MSG_Filter *, XP_Bool *enabled);
MSG_FilterError MSG_GetFilterRule(MSG_Filter *, MSG_Rule ** result);
MSG_FilterError MSG_GetFilterName(MSG_Filter *, char **name);	

MSG_FilterError MSG_GetFilterScript(MSG_Filter *, char **name);
MSG_FilterError MSG_SetFilterScript(MSG_Filter *, char *name);

MSG_FilterError MSG_RuleSetTerms(MSG_Rule *, XP_List *termList);
MSG_FilterError MSG_RuleGetTerms(MSG_Rule *, XP_List **termList);
MSG_FilterError MSG_RuleSetScope(MSG_Rule *, MSG_ScopeTerm *scope);
MSG_FilterError MSG_RuleGetScope(MSG_Rule *, MSG_ScopeTerm **scope);

/* if type is acChangePriority, value is a pointer to priority.
   If type is acMoveToFolder, value is pointer to folder name.
   Otherwise, value is ignored.
*/
MSG_FilterError MSG_RuleSetAction(MSG_Rule *, MSG_RuleActionType type, void *value);
MSG_FilterError MSG_RuleGetAction(MSG_Rule *, MSG_RuleActionType *type, void **value);

XP_END_PROTOS

#endif
