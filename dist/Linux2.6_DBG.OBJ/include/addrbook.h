#ifndef _AddrBook_H_
#define	_AddrBook_H_

#include "xp_core.h"
#include "aberror.h"
#include "abdefn.h"
#include "dwordarr.h"


typedef struct ABNeoEntry ABEntry;
typedef struct ABNeoPersonEntry ABPersonEntry;
typedef struct ABNeoListEntry ABListEntry;
typedef struct AddBookContext ABookContext;
typedef struct AddressBook ABook;

XP_BEGIN_PROTOS

/****************************************************************************/
/* Create and initialize the address book context which is the a view on an */
/* address book. It will provide a sorted list of all the address book */
/* entries ids .*/
/****************************************************************************/
ABErr AB_InitAddressBookContext(ABookContext** ppABookContext, 
								ABook* pABook);

/****************************************************************************/
/* Close the address book context. Called when the view on an address book */
/* is being closed */
/****************************************************************************/
ABErr AB_CloseAddressBookContext(ABookContext** ppABookContext);

/****************************************************************************/
/* Create and initialize a mailing list context which is the a view on an */
/* mailing list. It will provide a sorted view of all the entries in a */
/* mailing list.*/
/****************************************************************************/
ABErr AB_InitMailingListContext(ABookContext** ppMLContext, ABook* pABook, 
								ABID listID);

/****************************************************************************/
/* Close the mailing list context. Called when the view on an mailing list */
/* is being closed */
/****************************************************************************/
ABErr AB_CloseMailingListContext(ABookContext** ppMLContext);

/****************************************************************************/
/* Create and initialize the address book database */
/****************************************************************************/
ABErr AB_InitAddressBook(ABook** ppABook, ABEventFunction* function);

/****************************************************************************/
/* Close the address book database */
/****************************************************************************/
ABErr AB_CloseAddressBook(ABook** ppABook);

/****************************************************************************/
/* Add a person entry to the database */
/****************************************************************************/
ABErr AB_AddUser(ABook* pABook, PersonEntry* pPerson, ABID* entryID);

/****************************************************************************/
/* Add a mailing list to the database */
/****************************************************************************/
ABErr AB_AddMailingList(ABook* pABook, MailingListEntry* pABList, ABID* entryID);

/****************************************************************************/
/* Remove an entry either person or mailing list from the database */
/****************************************************************************/
ABErr AB_DeleteEntryID(ABook* pABook, ABID entryID);


/****************************************************************************/
/* Get and Set how full names are constructed for people entries */
/* first middle last or last first middle */
/****************************************************************************/
XP_Bool AB_GetSortByFirstName(ABook* pABook);
void	AB_SetSortByFirstName(ABook* pABook, XP_Bool sortby);

/****************************************************************************/
/* Import Export various formats */
/****************************************************************************/
ABErr AB_ImportFromEudora(ABook* pABook, const char* pfilename);
ABErr AB_ImportFromText(ABook* pABook, const char* pfilename);
ABErr AB_ImportFromHTML(ABook* pABook, const char* pfilename);
ABErr AB_ImportFromExchange(ABook* pABook, const char* pfilename);
ABErr AB_ImportFromVcard(ABook* pABook, const char* pVcard);
ABErr AB_ExportToText(ABook* pABook, const char* pfilename);
ABErr AB_ExportToHTML(ABook* pABook, const char* pfilename);
ABErr AB_ExportToVcard(ABook* pABook, ABID entryID, const char* pVcard);


/****************************************************************************/
/* Operations on contexts/view */
/****************************************************************************/

/****************************************************************************/
/* Get/Set sort by full name, sort by nickname */
/****************************************************************************/
ABErr	AB_SetSortType(ABookContext* pABookContext, ABSortBy sortby);
ABErr	AB_GetSortType(ABookContext* pABookContext, ABSortBy* sortby);

/****************************************************************************/
/* Get the unique database id at a particular context index */
/****************************************************************************/
ABID	AB_GetEntryIDAt(ABookContext* pABookContext, uint index);

/****************************************************************************/
/* Set/Get whether a context sorted id list has been updated */
/****************************************************************************/
XP_Bool AB_GetContextIsChanged(ABookContext* pABookContext);
void	AB_SetContextIsChanged(ABookContext* pABookContext, XP_Bool changed);

/****************************************************************************/
/* Get number of entries in a particular context */
/* For the address book you can ask for ALL, people, or mailing lists */
/* For mailing list contexts you will only be returned ALL */
/****************************************************************************/
ABErr AB_GetEntryCount(ABookContext* pABookContext, uint* count, 
					   ABEntryType etype);

/****************************************************************************/
/* Get information for index entries in a context. These are convenience */
/* functions and will change depending on what the ui group determines */
/* should be listed for each entry */
/****************************************************************************/
ABErr AB_GetTypeAt(ABookContext* pABookContext, uint index, ABEntryType* type);
ABErr AB_GetFullNameAt(ABookContext* pABookContext, uint index, char* pname);
ABErr AB_GetNicknameAt(ABookContext* pABookContext, uint index, char* pname);
ABErr AB_DeleteEntryAt(ABookContext* pABookContext, uint index);

/****************************************************************************/
/* Modify information for an entry (person or mailing list) */
/****************************************************************************/
ABErr AB_ModifyUser(ABook* pABook, ABID entryID, PersonEntry* pPerson);
ABErr AB_ModifyMailingList(ABook* pABook, ABID entryID, MailingListEntry* pABList);

/****************************************************************************/
/* Get information for every entry (person or mailing list) */
/****************************************************************************/
ABErr AB_GetType(ABook* pABook, ABID entryID, ABEntryType* type);
ABErr AB_GetFullName(ABook* pABook, ABID entryID, char* pname);
ABErr AB_GetNickname(ABook* pABook, ABID entryID, char* pname);

/****************************************************************************/
/* Get information for every person entry */
/****************************************************************************/
ABErr AB_GetGivenName(ABook* pABook, ABID entryID, char* pname);
ABErr AB_GetMiddleName(ABook* pABook, ABID entryID, char* pname);
ABErr AB_GetFamilyName(ABook* pABook, ABID entryID, char* pname);
ABErr AB_GetCompanyName(ABook* pABook, ABID entryID, char* pname);
ABErr AB_GetLocality(ABook* pABook, ABID entryID, char* pname);
ABErr AB_GetRegion(ABook* pABook, ABID entryID, char* pname);
ABErr AB_GetEmailAddress(ABook* pABook, ABID entryID, char* paddress);
ABErr AB_GetInfo(ABook* pABook, ABID entryID, char* pinfo);
ABErr AB_GetHTMLMail(ABook* pABook, ABID entryID, char* pinfo);

/****************************************************************************/
/* Add/Remove entries for a mailing list */
/****************************************************************************/
ABErr AB_AddIDToMailingList(ABook* pABook, ABID listID, ABID entryID);
ABErr AB_RemoveIDFromMailingList(ABook* pABook, ABID listID, ABID id);

/****************************************************************************/
/* Find index to first entry in a context that matches the typedown */
/****************************************************************************/
ABErr AB_GetIndexMatching(ABookContext* pABookContext, uint* index, 
	const char* aValue, uint startIndex, XP_Bool forward);

/****************************************************************************/
/* Return the ids of entries that match a string.  The fields that we are */
/* matching on is TBD but for now it is nickname */
/* still depends on how the name completion code is supposed to work */
/****************************************************************************/
ABErr AB_GetEntryIDsMatching(ABook* pABook, XPDWordArray* plist, 
							 const char* aValue);

/****************************************************************************/
/* Return the ids of mailing lists that a entry is a member of */
/* This was mentioned at one time as a requirement in the ui but it may */
/* go away */
/****************************************************************************/
ABErr AB_GetMailingListsContainingID(ABook* pABook, XPDWordArray* plist, 
									 ABID entryID);

XP_END_PROTOS

#endif
