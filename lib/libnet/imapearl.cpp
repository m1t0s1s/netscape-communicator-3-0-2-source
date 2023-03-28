#include "imap4url.h"
#include "imap4pvt.h"

// member functions of the TIMAPUrl class
TIMAPUrl::TIMAPUrl(const char *url_string)
 : fHostSubString(nil),
   fUrlidSubString(nil),
   fSourceFolderPathSubString(nil),
   fDestinationFolderPathSubString(nil),
   fSearchCriteriaString(nil),
   fListOfMessageIds(nil),
   fIdsAreUids(FALSE)
{
	fUrlString = XP_STRDUP(url_string);
	Parse();
}


TIMAPUrl::~TIMAPUrl()
{
	delete fUrlString;
}

void TIMAPUrl::ParseSourceFolderString()
{
	fSourceFolderPathSubString = strtok(nil, "?");
	if (!fSourceFolderPathSubString)
		fValidURL = FALSE;
}

void TIMAPUrl::ParseDestinationFolderString()
{
	fDestinationFolderPathSubString = strtok(nil, "?");
	if (!fDestinationFolderPathSubString)
		fValidURL = FALSE;
}

void TIMAPUrl::ParseSearchCriteriaString()
{
	fSearchCriteriaString = strtok(nil, "?");
	if (!fSearchCriteriaString)
		fValidURL = FALSE;
}

void TIMAPUrl::ParseUidChoice()
{
	char *uidChoiceString = strtok(nil, "?");
	if (!uidChoiceString)
		fValidURL = FALSE;
	else
		fIdsAreUids = XP_STRCMP(uidChoiceString, "UID") == 0;
}


void TIMAPUrl::ParseListofMessageIds()
{
	fListOfMessageIds = strtok(nil, "?");
	if (!fListOfMessageIds)
		fValidURL = FALSE;
}

void TIMAPUrl::Parse()
{
	fValidURL = TRUE;	// hope for the best
	
	char *urlStartToken = strtok(fUrlString, "?");
	
	if (!XP_STRNCASECMP(urlStartToken, "mailbox:",8) )
	{
		fHostSubString  = urlStartToken + 8;
		fUrlidSubString = strtok(nil, "?");
		if (!XP_STRCASECMP(fUrlidSubString, "fetch"))
		{
			fIMAPstate 					 = kSelectedStateURL;
			fUrlType   					 = kMsgFetch;
			ParseUidChoice();
			ParseSourceFolderString();
			ParseListofMessageIds();
		}
		else if (!XP_STRCASECMP(fUrlidSubString, "header"))
		{
			fIMAPstate 					 = kSelectedStateURL;
			fUrlType   					 = kMsgHeader;
			ParseUidChoice();
			ParseSourceFolderString();
			ParseListofMessageIds();
		}
		else if (!XP_STRCASECMP(fUrlidSubString, "deletemsg"))
		{
			fIMAPstate 					 = kSelectedStateURL;
			fUrlType   					 = kDeleteMsg;
			ParseUidChoice();
			ParseSourceFolderString();
			ParseListofMessageIds();
		}
		else if (!XP_STRCASECMP(fUrlidSubString, "markread"))
		{
			fIMAPstate 					 = kSelectedStateURL;
			fUrlType   					 = kMarkMsgRead;
			ParseUidChoice();
			ParseSourceFolderString();
			ParseListofMessageIds();
		}
		else if (!XP_STRCASECMP(fUrlidSubString, "markunread"))
		{
			fIMAPstate 					 = kSelectedStateURL;
			fUrlType   					 = kMarkMsgUnRead;
			ParseUidChoice();
			ParseSourceFolderString();
			ParseListofMessageIds();
		}
		else if (!XP_STRCASECMP(fUrlidSubString, "onlinecopy"))
		{
			fIMAPstate 					 = kSelectedStateURL;
			fUrlType   					 = kOnlineCopy;
			ParseUidChoice();
			ParseSourceFolderString();
			ParseListofMessageIds();
			ParseDestinationFolderString();
		}
		else if (!XP_STRCASECMP(fUrlidSubString, "search"))
		{
			fIMAPstate 					 = kSelectedStateURL;
			fUrlType   					 = kSearch;
			ParseUidChoice();
			ParseSourceFolderString();
			ParseSearchCriteriaString();
		}
		else if (!XP_STRCASECMP(fUrlidSubString, "test"))
		{
			fIMAPstate 					 = kAuthenticatedStateURL;
			fUrlType   					 = kTest;
		}
		else if (!XP_STRCASECMP(fUrlidSubString, "create"))
		{
			fIMAPstate 					 = kAuthenticatedStateURL;
			fUrlType   					 = kCreateFolder;
			ParseSourceFolderString();
		}
		else if (!XP_STRCASECMP(fUrlidSubString, "delete"))
		{
			fIMAPstate 					 = kAuthenticatedStateURL;
			fUrlType   					 = kDeleteFolder;
			ParseSourceFolderString();
		}
		else if (!XP_STRCASECMP(fUrlidSubString, "rename"))
		{
			fIMAPstate 					 = kAuthenticatedStateURL;
			fUrlType   					 = kRenameFolder;
			ParseSourceFolderString();
			ParseDestinationFolderString();
		}
		else if (!XP_STRCASECMP(fUrlidSubString, "list"))
		{
			fIMAPstate 					 = kAuthenticatedStateURL;
			fUrlType   					 = kListFolders;
		}
	}
	else
		fValidURL = FALSE;	
}

TIMAPUrl::EUrlIMAPstate TIMAPUrl::GetUrlIMAPstate()
{
	return fIMAPstate;
}

TIMAPUrl::EIMAPurlType TIMAPUrl::GetIMAPurlType()
{
	return fUrlType;
}



char *TIMAPUrl::CreateSourceFolderPathString()
{
	return XP_STRDUP(fSourceFolderPathSubString);
}

char *TIMAPUrl::CreateDestinationFolderPathString()
{
	return XP_STRDUP(fDestinationFolderPathSubString);
}

char *TIMAPUrl::CreateSearchCriteriaString()
{
	return XP_STRDUP(fSearchCriteriaString);
}

char *TIMAPUrl::CreateListOfMessageIdsString()
{
	return XP_STRDUP(fListOfMessageIds);
}


XP_Bool TIMAPUrl::ValidIMAPUrl()
{
	return fValidURL;
}

XP_Bool TIMAPUrl::MessageIdsAreUids()
{
	return fIdsAreUids;
}

