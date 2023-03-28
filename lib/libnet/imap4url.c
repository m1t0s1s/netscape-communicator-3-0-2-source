#ifndef IMAP4URL_H
#include "imap4url.h"
#endif

#ifndef _MCOM_H_
#include "xp_mcom.h"
#endif

/* 45678901234567890123456789012345678901234567890123456789012345678901234567890
*/ 

char *createStartOfIMAPurl(const char *imapHost)
{
	char *returnString = XP_STRDUP("Mailbox://");
	StrAllocCat(returnString, imapHost);
	StrAllocCat(returnString, "?");
	return returnString;
}


/* Creating a mailbox */
/* imap4://HOST?create?MAILBOXPATH */
char *CreateImapMailboxCreateUrl(const char *imapHost, const char *mailbox)
{
	char *returnString = createStartOfIMAPurl(imapHost);
	StrAllocCat(returnString, "create?");
	StrAllocCat(returnString, mailbox);
	
	return returnString;
}

/* deleting a mailbox */
/* imap4://HOST?delete?MAILBOXPATH */
char *CreateImapMailboxDeleteUrl(const char *imapHost, const char *mailbox)
{
	char *returnString = createStartOfIMAPurl(imapHost);
	StrAllocCat(returnString, "delete?");
	StrAllocCat(returnString, mailbox);
	
	return returnString;
}

/* renaming a mailbox */
/* imap4://HOST?rename?OLDNAME?NEWNAME */
char *CreateImapMailboxRenameUrl(const char *imapHost, 
								 const char *oldBoxName,
								 const char *newBoxName)
{
	char *returnString = createStartOfIMAPurl(imapHost);
	StrAllocCat(returnString, "rename?");
	StrAllocCat(returnString, oldBoxName);
	StrAllocCat(returnString, "?");
	StrAllocCat(returnString, newBoxName);
	
	return returnString;
}

/* listing available mailboxes */
/* imap4://HOST?list */
char *CreateImapListUrl(const char *imapHost)
{
	char *returnString = createStartOfIMAPurl(imapHost);
	StrAllocCat(returnString, "list");
	
	return returnString;
}

/* fetching RFC822 messages */
/* imap4://HOST?fetch?<UID/SEQUENCE>?MAILBOXPATH?x */
/*   'x' is the message UID or sequence number list */
/* will set the 'SEEN' flag */
char *CreateImapMessageFetchUrl(const char *imapHost,
								const char *mailbox,
								const char *messageIdentifierList,
								XP_Bool messageIdsAreUID)
{
	char *returnString = createStartOfIMAPurl(imapHost);
	if (messageIdsAreUID)
		StrAllocCat(returnString, "fetch?UID?");
	else
		StrAllocCat(returnString, "fetch?SEQUENCE?");

	StrAllocCat(returnString, mailbox);
	StrAllocCat(returnString, "?");
	StrAllocCat(returnString, messageIdentifierList);
	
	return returnString;
}

/* fetching the headers of RFC822 messages */
/* imap4://HOST?header?<UID/SEQUENCE>?MAILBOXPATH?x */
/*   'x' is the message UID or sequence number list */
/* will not affect the 'SEEN' flag */
char *CreateImapMessageHeaderUrl(const char *imapHost,
								 const char *mailbox,
								 const char *messageIdentifierList,
								 XP_Bool messageIdsAreUID)
{
	char *returnString = createStartOfIMAPurl(imapHost);
	if (messageIdsAreUID)
		StrAllocCat(returnString, "header?UID?");
	else
		StrAllocCat(returnString, "header?SEQUENCE?");

	StrAllocCat(returnString, mailbox);
	StrAllocCat(returnString, "?");
	StrAllocCat(returnString, messageIdentifierList);
	
	return returnString;
}

/* search an online mailbox */
/* imap4://HOST?search?<UID/SEQUENCE>?MAILBOXPATH?SEARCHSTRING */
/*   'x' is the message sequence number list */
char *CreateImapSearchUrl(const char *imapHost,
						  const char *mailbox,
						  const char *searchString,
						  XP_Bool messageIdsAreUID)
{
	char *returnString = createStartOfIMAPurl(imapHost);
	if (messageIdsAreUID)
		StrAllocCat(returnString, "search?UID?");
	else
		StrAllocCat(returnString, "search?SEQUENCE?");

	StrAllocCat(returnString, mailbox);
	StrAllocCat(returnString, "?");
	StrAllocCat(returnString, searchString);
	
	return returnString;	
}

/* delete messages */
/* imap4://HOST?deletemsg?<UID/SEQUENCE>?MAILBOXPATH?x */
/*   'x' is the message UID or sequence number list */
char *CreateImapDeleteMessageUrl(const char *imapHost,
								 const char *mailbox,
								 const char *messageIds,
								 XP_Bool idsAreUids)
{
	char *returnString = createStartOfIMAPurl(imapHost);
	if (idsAreUids)
		StrAllocCat(returnString, "deletemsg?UID?");
	else
		StrAllocCat(returnString, "deletemsg?SEQUENCE?");

	StrAllocCat(returnString, mailbox);
	StrAllocCat(returnString, "?");
	StrAllocCat(returnString, messageIds);
	
	return returnString;
}

/* mark messages as read */
/* imap4://HOST?markread?<UID/SEQUENCE>?MAILBOXPATH?x */
/*   'x' is the message UID or sequence number list */
char *CreateImapMarkMessageReadUrl(const char *imapHost,
								   const char *mailbox,
								   const char *messageIds,
								   XP_Bool idsAreUids)
{
	char *returnString = createStartOfIMAPurl(imapHost);
	if (idsAreUids)
		StrAllocCat(returnString, "markread?UID?");
	else
		StrAllocCat(returnString, "markread?SEQUENCE?");

	StrAllocCat(returnString, mailbox);
	StrAllocCat(returnString, "?");
	StrAllocCat(returnString, messageIds);
	
	return returnString;
}

/* mark messages as unread */
/* imap4://HOST?markunread?<UID/SEQUENCE>?MAILBOXPATH?x */
/*   'x' is the message UID or sequence number list */
char *CreateImapMarkMessageUnReadUrl(const char *imapHost,
								     const char *mailbox,
								 	 const char *messageIds,
								 	 XP_Bool idsAreUids)
{
	char *returnString = createStartOfIMAPurl(imapHost);
	if (idsAreUids)
		StrAllocCat(returnString, "markunread?UID?");
	else
		StrAllocCat(returnString, "markunread?SEQUENCE?");

	StrAllocCat(returnString, mailbox);
	StrAllocCat(returnString, "?");
	StrAllocCat(returnString, messageIds);
	
	return returnString;
}

/* copy messages from one online box to another */
/* imap4://HOST?onlineCopy?<UID/SEQUENCE>?SOURCEMAILBOXPATH?x?
			DESTINATIONMAILBOXPATH */
/*   'x' is the message UID or sequence number list */
char *CreateImapOnlineCopyUrl(const char *imapHost,
							  const char *sourceMailbox,
							  const char *messageIds,
							  const char *destinationMailbox,
							  XP_Bool idsAreUids)
{
	char *returnString = createStartOfIMAPurl(imapHost);
	if (idsAreUids)
		StrAllocCat(returnString, "onlinecopy?UID?");
	else
		StrAllocCat(returnString, "onlinecopy?SEQUENCE?");

	StrAllocCat(returnString, sourceMailbox);
	StrAllocCat(returnString, "?");
	StrAllocCat(returnString, messageIds);
	StrAllocCat(returnString, "?");
	StrAllocCat(returnString, destinationMailbox);
	
	return returnString;
}
