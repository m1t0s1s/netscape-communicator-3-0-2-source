#include "imap4pvt.h"


/* 45678901234567890123456789012345678901234567890123456789012345678901234567890
*/ 


TSearchResultSequence::TSearchResultSequence()
	: fListOfLines(nil)
{
}

TSearchResultSequence::~TSearchResultSequence()
{
	if (fListOfLines)
		XP_ListDestroy(fListOfLines);
}

void TSearchResultSequence::ResetSequence()
{
	if (fListOfLines)
		XP_ListDestroy(fListOfLines);
	fListOfLines = nil;
}

void TSearchResultSequence::AddSearchResultLine(const char *searchLine)
{
	char *copiedSequence = XP_STRDUP(searchLine + 9); // 9 == "* SEARCH "

	if (!fListOfLines)
	{
		fListOfLines = XP_ListNew();
		fListOfLines->object = copiedSequence;
	}
	else
		XP_ListAddObjectToEnd(fListOfLines, copiedSequence);
}


TSearchResultIterator::TSearchResultIterator(TSearchResultSequence &sequence) :
	fSequence(sequence)
{
	ResetIterator();
}

TSearchResultIterator::~TSearchResultIterator()
{}

void  TSearchResultIterator::ResetIterator()
{
	fCurrentLine = fSequence.fListOfLines;
	if (fCurrentLine)
		fPositionInCurrentLine = (char *) fCurrentLine->object;
	else
		fPositionInCurrentLine = nil;
}

int32 TSearchResultIterator::GetNextMessageNumber()
{
	int32 returnValue = 0;
	if (fPositionInCurrentLine)
	{	
		returnValue = atol(fPositionInCurrentLine);
		
		// eat the current number
		while (isdigit(*++fPositionInCurrentLine))
			;
		
		if (*fPositionInCurrentLine == 0xD)	// found CR, no more digits on line
		{
			fCurrentLine = fCurrentLine->next;
			if (fCurrentLine)
				fPositionInCurrentLine = (char *) fCurrentLine->object;
			else
				fPositionInCurrentLine = nil;
		}
		else	// eat the space
			fPositionInCurrentLine++;
	}
	
	return returnValue;
}

/* member functions for TImapServerState response parser */
TImapServerState::TImapServerState(TIMAP4BlockingConnection &imapConnection) :
	fServerConnection(imapConnection),
	fCurrentCommandTag(nil),
	fNextToken(nil),
	fCurrentLine(nil),
	fLineOfTokens(nil),
	fStartOfLineOfTokens(nil),
	fCurrentFolderReadOnly(FALSE),
	fNumberOfExistingMessages(0),
	fNumberOfRecentMessages(0),
	fNumberOfUnseenMessages(0),
	fIMAPstate(kNonAuthenticated)
{
}

TImapServerState::~TImapServerState()
{
	delete fCurrentCommandTag;
}
/* 
 response        ::= *response_data response_done
*/

void TImapServerState::ParseIMAPServerResponse(const char *currentCommand)
{
	char *copyCurrentCommand = XP_STRDUP(currentCommand);
	char *tagToken           = XP_STRTOK(copyCurrentCommand, " ");
	char *commandToken       = XP_STRTOK(nil, " ");
	if (tagToken)
	{
		delete fCurrentCommandTag;
		fCurrentCommandTag = XP_STRDUP(tagToken);
	}
	
	if (commandToken)
		PreProcessCommandToken(commandToken);
	
	fProcessingTaggedResponse = FALSE;
	fCurrentCommandFailed 	  = FALSE;
	fDisconnected			  = FALSE;
	fSyntaxError              = FALSE;
	
	fSyntaxErrorLine = nil;
	
	ResetLexAnalyzer();
	
	fNextToken = GetNextToken();
	
	while (ContinueParse() && !XP_STRCMP(fNextToken, "*") )
		response_data();
	
	if (ContinueParse())
		response_done();
	
	if (ContinueParse() && !CommandFailed())
	{
		// a sucessful command may change the eIMAPstate
		ProcessOkCommand(commandToken);
	}
	else if (CommandFailed())
	{
		// a failed command may change the eIMAPstate
		ProcessBadCommand(commandToken);
	}
}

// SEARCH is the only command that requires pre-processing for now.
// others will be added here.
void TImapServerState::PreProcessCommandToken(const char *commandToken)
{
	if (!XP_STRCASECMP(commandToken, "SEARCH"))
		fSearchResults.ResetSequence();
}

TSearchResultIterator *TImapServerState::CreateSearchResultIterator()
{
	return new TSearchResultIterator(fSearchResults);
}

TImapServerState::eIMAPstate TImapServerState::GetIMAPstate()
{
	return fIMAPstate;
}

void TImapServerState::ProcessOkCommand(const char *commandToken)
{
	if (!XP_STRCASECMP(commandToken, "LOGIN"))
		fIMAPstate = kAuthenticated;
	else if (!XP_STRCASECMP(commandToken, "LOGOUT"))
		fIMAPstate = kNonAuthenticated;
	else if (!XP_STRCASECMP(commandToken, "SELECT") ||
	         !XP_STRCASECMP(commandToken, "EXAMINE"))
		fIMAPstate = kFolderSelected;
	else if (!XP_STRCASECMP(commandToken, "CLOSE"))
		fIMAPstate = kAuthenticated;
}

void TImapServerState::ProcessBadCommand(const char *commandToken)
{
	if (!XP_STRCASECMP(commandToken, "LOGIN"))
		fIMAPstate = kNonAuthenticated;
	else if (!XP_STRCASECMP(commandToken, "LOGOUT"))
		fIMAPstate = kNonAuthenticated;	// ??
	else if (!XP_STRCASECMP(commandToken, "SELECT") ||
	         !XP_STRCASECMP(commandToken, "EXAMINE"))
		fIMAPstate = kAuthenticated;	// nothing selected
	else if (!XP_STRCASECMP(commandToken, "CLOSE"))
		fIMAPstate = kAuthenticated;	// nothing selected
}
/*
 response_data   ::= "*" SPACE (resp_cond_state / resp_cond_bye /
                              mailbox_data / message_data / capability_data)
                              CRLF
 
 The RFC1730 grammar spec did not allow one symbol look ahead to determine
 between mailbox_data / message_data so I combined the numeric possibilities
 of mailbox_data and all of message_data into numeric_mailbox_data.
 
 The production implemented here is 

 response_data   ::= "*" SPACE (resp_cond_state / resp_cond_bye /
                              mailbox_data / numeric_mailbox_data / 
                              capability_data)
                              CRLF

*/
void TImapServerState::response_data()
{
	fNextToken = GetNextToken();
	
	if (ContinueParse())
	{
		if (!XP_STRCASECMP(fNextToken, "OK") ||
		    !XP_STRCASECMP(fNextToken, "NO") ||
		    !XP_STRCASECMP(fNextToken, "BAD") )
			resp_cond_state();
		else if (!XP_STRCASECMP(fNextToken, "BYE") )
			resp_cond_bye();
		else if (!XP_STRCASECMP(fNextToken, "FLAGS") ||
		    	 !XP_STRCASECMP(fNextToken, "LIST")  ||
		    	 !XP_STRCASECMP(fNextToken, "LSUB")  ||
		    	 !XP_STRCASECMP(fNextToken, "MAILBOX")  ||
		    	 !XP_STRCASECMP(fNextToken, "SEARCH"))
			mailbox_data();
		else if (IsNumericString(fNextToken))
		    numeric_mailbox_data();
		else if (!XP_STRCASECMP(fNextToken, "CAPABILITY"))
			capability_data();
		else
			SetSyntaxError(TRUE);
		
		if (ContinueParse())
			end_of_line();
	}
}

void TImapServerState::end_of_line()
{
	if (!at_end_of_line())
		SetSyntaxError(TRUE);
	else if (fProcessingTaggedResponse)
		ResetLexAnalyzer();	// no more tokens until we send a command
	else
		fNextToken = GetNextToken();
}

void TImapServerState::skip_to_CRLF()
{
	while (Connected() && !at_end_of_line())
		fNextToken = GetNextToken();
}

/*
 mailbox_data    ::=  "FLAGS" SPACE flag_list /
                               "LIST" SPACE mailbox_list /
                               "LSUB" SPACE mailbox_list /
                               "MAILBOX" SPACE text /
                               "SEARCH" [SPACE 1#nz_number] /
                               number SPACE "EXISTS" / number SPACE "RECENT"

This production was changed to accomodate predictive parsing 

 mailbox_data    ::=  "FLAGS" SPACE flag_list /
                               "LIST" SPACE mailbox_list /
                               "LSUB" SPACE mailbox_list /
                               "MAILBOX" SPACE text /
                               "SEARCH" [SPACE 1#nz_number]
*/
void TImapServerState::mailbox_data()
{
	// implement later!
	if (!XP_STRCASECMP(fNextToken, "FLAGS"))
		skip_to_CRLF();
	else if (!XP_STRCASECMP(fNextToken, "LIST") ||
	         !XP_STRCASECMP(fNextToken, "LSUB"))
	{
		fNextToken = GetNextToken();
		if (ContinueParse())
			mailbox_list();
	}
	else if (!XP_STRCASECMP(fNextToken, "MAILBOX"))
		skip_to_CRLF();
	else if (!XP_STRCASECMP(fNextToken, "SEARCH"))
	{
		fSearchResults.AddSearchResultLine(fCurrentLine);
		skip_to_CRLF();
	}
}

/*
 mailbox_list    ::= "(" #("\Marked" / "\Noinferiors" /
                              "\Noselect" / "\Unmarked" / flag_extension) ")"
                              SPACE (<"> QUOTED_CHAR <"> / nil) SPACE mailbox
*/

void TImapServerState::mailbox_list()
{
	XP_Bool endOfFlags = FALSE;
	fNextToken++;	// eat the first "("
	do {
		if (!XP_STRNCASECMP(fNextToken, "\\Marked", 7))
			;	// wait for folder data base stuff
		else if (!XP_STRNCASECMP(fNextToken, "\\Unmarked", 9))
			;	// wait for folder data base stuff
		else if (!XP_STRNCASECMP(fNextToken, "\\Noinferiors", 12))
			;	// wait for folder data base stuff
		else if (!XP_STRNCASECMP(fNextToken, "\\Noselect", 9))
			;	// wait for folder data base stuff
		else
			;	// we ignore flag extensions
		
		endOfFlags = *(fNextToken + strlen(fNextToken) - 1) == ')';
		fNextToken = GetNextToken();
	} while (!endOfFlags && ContinueParse());
	
	if (ContinueParse())
	{
		if (*fNextToken == '"')
		{
			// this "x" character is the dir separator for server machine
			char separator = *(fNextToken + 1);	// handle this ;ater
		}
		fNextToken = GetNextToken();	
		if (ContinueParse())
			mailbox();	
	}
	
	
}

/* mailbox         ::= "INBOX" / astring
*/
void TImapServerState::mailbox()
{
	if (!XP_STRCASECMP(fNextToken, "INBOX"))
		fNextToken = GetNextToken();
	else if (*fNextToken == '{')
		char *boxname = CreateLiteral();	// literal
	else if (*fNextToken == '"')
		char *boxname = CreateQuoted();		// quoted
	else
	{
		char *boxname = XP_STRDUP(fNextToken);	// atomic string
		fNextToken = GetNextToken();
	}
}

// this seems very involved for finding a close ".  But I have to go
// character by character because I cannot tokenize a quoted  string.  It
// may have white space in it. Also, if the current line ends without the
// closed quote then I have to fetch another line from the server!

char *TImapServerState::CreateQuoted()
{
	char *currentChar = fCurrentLine + 
					    (fNextToken - fStartOfLineOfTokens)
					    + 1;	// one char past opening '"'

	char *returnString = XP_STRDUP(currentChar);
	int  charIndex = 0;
	XP_Bool closeQuoteFound = FALSE;
	
	while (!closeQuoteFound && ContinueParse() )
	{
		if (!*(returnString + charIndex))
		{
			AdvanceToNextLine();
			returnString = XP_STRCAT(returnString, fCurrentLine);
			charIndex++;
		}
		else if (*(returnString + charIndex) == '"')
		{
			if (*(returnString + charIndex - 1) == '\\')
				charIndex++;
			else
				closeQuoteFound = TRUE;
		}
		else
			charIndex++;
	}
	
	if (closeQuoteFound)
	{
		*(returnString + charIndex) = 0;
		skip_to_CRLF();
	}
	return returnString;
}

char *TImapServerState::CreateLiteral()
{
	int32 numberOfCharsInMessage = atol(fNextToken + 1);
	int32 charsReadSoFar = 0;
	
	char *returnString = new char[numberOfCharsInMessage + 1];
	
	while (ContinueParse() && (charsReadSoFar < numberOfCharsInMessage))
	{
		AdvanceToNextLine();
		if (ContinueParse())
		{
			XP_STRCPY(returnString + charsReadSoFar, fCurrentLine);
			charsReadSoFar += strlen(fCurrentLine);	
		}
	}
	
	if (ContinueParse())
	{
		skip_to_CRLF();		// GetNextToken will pick it up at CRLF
		*(returnString + numberOfCharsInMessage) = 0;
	}
	
	return returnString;
}


/*
 message_data    ::= nz_number SPACE ("EXPUNGE" /
                              ("FETCH" SPACE msg_fetch) / msg_obsolete)

was changed to

numeric_mailbox_data ::=  number SPACE "EXISTS" / number SPACE "RECENT"
 / nz_number SPACE ("EXPUNGE" /
                              ("FETCH" SPACE msg_fetch) / msg_obsolete)

*/
void TImapServerState::numeric_mailbox_data()
{
	int32 tokenNumber = atol(fNextToken);
	fNextToken = GetNextToken();
	
	if (ContinueParse())
	{
		if (!XP_STRCASECMP(fNextToken, "EXISTS"))
		{
			fNumberOfExistingMessages = tokenNumber;
			fNextToken = GetNextToken();
		}
		else if (!XP_STRCASECMP(fNextToken, "RECENT"))
		{
			fNumberOfRecentMessages = tokenNumber;
			fNextToken = GetNextToken();
		}
		else if (!XP_STRCASECMP(fNextToken, "EXPUNGE"))
			skip_to_CRLF(); // later
		else if (!XP_STRCASECMP(fNextToken, "FETCH"))
		{
			fNextToken = GetNextToken();
			if (ContinueParse())
				msg_fetch(); 
		}
		else
			msg_obsolete();
	}
}

/*
 msg_fetch       ::= "(" 1#("BODY" SPACE body /
                              "BODYSTRUCTURE" SPACE body /
                              "BODY[" section "]" SPACE nstring /
                              "ENVELOPE" SPACE envelope /
                              "FLAGS" SPACE "(" #(flag / "\Recent") ")" /
                              "INTERNALDATE" SPACE date_time /
                              "RFC822" [".HEADER" / ".TEXT"] SPACE nstring /
                              "RFC822.SIZE" SPACE number /
                              "UID" SPACE uniqueid) ")"

*/

void TImapServerState::msg_fetch()
{
	fNextToken++;	// eat the '(' character
	
	// some of these productions are ignored for now
	while (ContinueParse() && (*fNextToken != ')') )
	{
		// I only fetch RFC822 so I should never see these BODY responses
		if (!XP_STRCASECMP(fNextToken, "BODY"))
			skip_to_CRLF(); // I never ask for this
		else if (!XP_STRCASECMP(fNextToken, "BODYSTRUCTURE"))
			skip_to_CRLF(); // I never ask for this
		else if (!XP_STRNCASECMP(fNextToken, "BODY[", 6))
			skip_to_CRLF(); // I never ask for this
		else if (!XP_STRCASECMP(fNextToken, "ENVELOPE"))
			skip_to_CRLF(); // I never ask for this
		else if (!XP_STRCASECMP(fNextToken, "INTERNALDATE"))
			skip_to_CRLF(); // I will need to implement this
		else if (!XP_STRCASECMP(fNextToken, "FLAGS"))
		{
			fNextToken = GetNextToken();
			if (ContinueParse())
				flags();
			
			if (ContinueParse())
			{	// eat the closing ')'
				fNextToken++;
				// there may be another ')' to close out
				// msg_fetch.  If there is then don't advance
				if (*fNextToken != ')')
					fNextToken = GetNextToken();
			}
		}
		else if (!XP_STRCASECMP(fNextToken, "RFC822") ||
				 !XP_STRCASECMP(fNextToken, "RFC822.HEADER") ||
				 !XP_STRCASECMP(fNextToken, "RFC822.TEXT") )
		{
			fNextToken = GetNextToken();
			if (ContinueParse())
				msg_fetch_nstring();
		}
		else if (!XP_STRCASECMP(fNextToken, "RFC822.SIZE"))
		{
			fNextToken = GetNextToken();
			if (ContinueParse())
			{
				int32 msgSizeDropOnFloorForNow = atol(fNextToken);
				// if this token ends in ')', then it is the last token
				// else we advance
				if ( *(fNextToken + strlen(fNextToken) - 1) == ')')
					fNextToken += strlen(fNextToken) - 1;
				else
					fNextToken = GetNextToken();
			}
		}
		else if (!XP_STRCASECMP(fNextToken, "UID"))
		{
			fNextToken = GetNextToken();
			if (ContinueParse())
			{
				int32 msgUIDDropOnFloorForNow = atol(fNextToken);
				// if this token ends in ')', then it is the last token
				// else we advance
				if ( *(fNextToken + strlen(fNextToken) - 1) == ')')
					fNextToken += strlen(fNextToken) - 1;
				else
					fNextToken = GetNextToken();
			}
		}
	}
	
	if (ContinueParse())
		fNextToken = GetNextToken();	// eat the ')' ending token
										// should be at end of line
}

void TImapServerState::flags()
{
	while (ContinueParse() && !strcasestr(fNextToken, ")"))
		fNextToken = GetNextToken();
	
	if (ContinueParse())
		while(*fNextToken != ')')
			fNextToken++;
}
	
XP_Bool		TImapServerState::CommandFailed()
{
	return fCurrentCommandFailed;
}

/* 
 resp_cond_state ::= ("OK" / "NO" / "BAD") SPACE resp_text
*/
void TImapServerState::resp_cond_state()
{
	if ((!XP_STRCASECMP(fNextToken, "NO") ||
	     !XP_STRCASECMP(fNextToken, "BAD") ) &&
	    	fProcessingTaggedResponse)
		fCurrentCommandFailed = TRUE;
	
	fNextToken = GetNextToken();
	if (ContinueParse())
		resp_text();
}

/*
 resp_text       ::= ["[" resp_text_code "]" SPACE] (text_mime2 / text)
 
 was changed to in order to enable a one symbol look ahead predictive
 parser.
 
 resp_text       ::= ["[" resp_text_code  SPACE] (text_mime2 / text)
 */
void TImapServerState::resp_text()
{
	if (ContinueParse() && (*fNextToken == '['))
		resp_text_code();
		
	if (ContinueParse())
	{
		if (!XP_STRCMP(fNextToken, "=?"))
			text_mime2();
		else
			text();
	}
}
/*
 text_mime2       ::= "=?" <charset> "?" <encoding> "?"
                               <encoded-text> "?="
                               ;; Syntax defined in [MIME-2]
*/
void TImapServerState::text_mime2()
{
	skip_to_CRLF();
}

/*
 text            ::= 1*TEXT_CHAR

*/
void TImapServerState::text()
{
	skip_to_CRLF();
}


/*
 resp_text_code  ::= "ALERT" / "PARSE" /
                              "PERMANENTFLAGS" SPACE "(" #(flag / "\*") ")" /
                              "READ-ONLY" / "READ-WRITE" / "TRYCREATE" /
                              "UIDVALIDITY" SPACE nz_number /
                              "UNSEEN" SPACE nz_number /
                              atom [SPACE 1*<any TEXT_CHAR except "]">]
                              
 was changed to in order to enable a one symbol look ahead predictive
 parser.
 
  resp_text_code  ::= ("ALERT" / "PARSE" /
                              "PERMANENTFLAGS" SPACE "(" #(flag / "\*") ")" /
                              "READ-ONLY" / "READ-WRITE" / "TRYCREATE" /
                              "UIDVALIDITY" SPACE nz_number /
                              "UNSEEN" SPACE nz_number /
                              atom [SPACE 1*<any TEXT_CHAR except "]">] )
                      "]"

 
*/
void TImapServerState::resp_text_code()
{
	// this is a special case way of advancing the token
	// strtok won't break up "[ALERT]" into separate tokens
	if (XP_STRLEN(fNextToken) > 1)
		fNextToken++;
	else 
		fNextToken = GetNextToken();
	
	if (ContinueParse())
	{
		if (!XP_STRCASECMP(fNextToken,"ALERT]"))
		{
			// do nothing for now
			fNextToken = GetNextToken();
		}
		else if (!XP_STRCASECMP(fNextToken,"PARSE]"))
		{
			// do nothing for now
			fNextToken = GetNextToken();
		}
		else if (!XP_STRCASECMP(fNextToken,"PERMANENTFLAGS"))
		{
			// do nothing but eat tokens until we see the ] or CRLF
			// we should see the ] but we don't want to go into an
			// endless loop if the CRLF is not there
			do {
				fNextToken = GetNextToken();
			} while (!strcasestr(fNextToken, "]") && 
					 !at_end_of_line() &&
					 ContinueParse());
		}
		else if (!XP_STRCASECMP(fNextToken,"READ-ONLY]"))
		{
			fCurrentFolderReadOnly = TRUE;
			fNextToken = GetNextToken();
		}
		else if (!XP_STRCASECMP(fNextToken,"READ-WRITE]"))
		{
			fCurrentFolderReadOnly = FALSE;
			fNextToken = GetNextToken();
		}
		else if (!XP_STRCASECMP(fNextToken,"TRYCREATE]"))
		{
			// do nothing for now
			fNextToken = GetNextToken();
		}
		else if (!XP_STRCASECMP(fNextToken,"UIDVALIDITY"))
		{
			fNextToken = GetNextToken();
			if (ContinueParse())
			{
				fFolderUIDValidity = atol(fNextToken);
				fNextToken = GetNextToken();
			}
		}
		else if (!XP_STRCASECMP(fNextToken,"UNSEEN"))
		{
			fNextToken = GetNextToken();
			if (ContinueParse())
			{
				fNumberOfUnseenMessages = atol(fNextToken);
				fNextToken = GetNextToken();
			}
		}
		else 	// just text
		{
			// do nothing but eat tokens until we see the ] or CRLF
			// we should see the ] but we don't want to go into an
			// endless loop if the CRLF is not there
			do {
				fNextToken = GetNextToken();
			} while (!strcasestr(fNextToken, "]") && 
			         !at_end_of_line() &&
			         ContinueParse());
		}
	}
}

/*
 response_done   ::= response_tagged / response_fatal
 */
void TImapServerState::response_done()
{
	if (ContinueParse())
	{
		if (!XP_STRCASECMP(fNextToken, fCurrentCommandTag))
			response_tagged();
		else
			response_fatal();
	}
}

/*  response_tagged ::= tag SPACE resp_cond_state CRLF
*/
void TImapServerState::response_tagged()
{
	// eat the tag
	fNextToken = GetNextToken();
	if (ContinueParse())
	{
		fProcessingTaggedResponse = TRUE;
		resp_cond_state();
		if (ContinueParse())
			end_of_line();
	}
}

/* response_fatal  ::= "*" SPACE resp_cond_bye CRLF
*/
void TImapServerState::response_fatal()
{
	// eat the "*"
	fNextToken = GetNextToken();
	if (ContinueParse())
	{
		resp_cond_bye();
		if (ContinueParse())
			end_of_line();
	}
}
/*
resp_cond_bye   ::= "BYE" SPACE resp_text
                              ;; Server will disconnect condition
                    */
void TImapServerState::resp_cond_bye()
{
	SetConnected(FALSE);
	skip_to_CRLF();
}

/* nstring         ::= string / nil
string          ::= quoted / literal
 nil             ::= "NIL"

*/
void TImapServerState::msg_fetch_nstring()
{
	if (XP_STRCASECMP(fNextToken, "NIL"))
	{
		if (*fNextToken == '"')
			msg_fetch_quoted();
		else
			msg_fetch_literal();
	}
	else
		fNextToken = GetNextToken();	// eat "NIL"
}

/*
 quoted          ::= <"> *QUOTED_CHAR <">

          QUOTED_CHAR     ::= <any TEXT_CHAR except quoted_specials> /
                              "\" quoted_specials

          quoted_specials ::= <"> / "\"
*/
// This algorithm seems gross.  Isn't there an easier way?
void TImapServerState::msg_fetch_quoted()
{
	XP_Bool closeQuoteFound = FALSE;
	char *scanLine = fCurrentLine++;  // eat the first "
	// this won't work!  The '"' char does not start
	// the line!  why would anybody format a message like this!
	while (!closeQuoteFound && ContinueParse())
	{
		char *quoteSubString = strcasestr(scanLine, "\"");
		while (quoteSubString && !closeQuoteFound)
		{
			if (quoteSubString > scanLine)
			{
				closeQuoteFound = *(quoteSubString - 1) != '\\';
				if (!closeQuoteFound)
					quoteSubString = strcasestr(quoteSubString+1, "\"");
			}
			else
				closeQuoteFound = TRUE;	// 1st char is a '"'
		}
		
		// send the string to the connection object so he can display or
		// cache or whatever
		fServerConnection.HandleMessageLine(fCurrentLine);
		AdvanceToNextLine();
		scanLine = fCurrentLine;
	}
}
/* msg_obsolete    ::= "COPY" / ("STORE" SPACE msg_fetch)
                              ;; OBSOLETE untagged data responses */
void TImapServerState::msg_obsolete()
{
	if (!XP_STRCASECMP(fNextToken, "COPY"))
		fNextToken = GetNextToken();
	else if (!XP_STRCASECMP(fNextToken, "STORE"))
	{
		fNextToken = GetNextToken();
		if (ContinueParse())
			msg_fetch();
	}
	else
		SetSyntaxError(TRUE);
	
}
void TImapServerState::capability_data()
{
	skip_to_CRLF();
}
/*
 literal         ::= "{" number "}" CRLF *CHAR8
                              ;; Number represents the number of CHAR8 octets
*/
void TImapServerState::msg_fetch_literal()
{
	int32 numberOfCharsInMessage = atol(fNextToken + 1);
	int32 charsReadSoFar = 0;
	
	while (ContinueParse() && (charsReadSoFar < numberOfCharsInMessage))
	{
		AdvanceToNextLine();
		if (ContinueParse())
		{
			charsReadSoFar += strlen(fCurrentLine);	
			char *adoptedMessageLine = XP_STRDUP(fCurrentLine);
			fServerConnection.HandleMessageLine(adoptedMessageLine);
		}
	}
	
	if (ContinueParse())
	{
		skip_to_CRLF();
		fNextToken = GetNextToken();
	}
}

void TImapServerState::ResetLexAnalyzer()
{
	delete fCurrentLine;
	delete fStartOfLineOfTokens;
	
	fCurrentLine = fNextToken = fLineOfTokens = fStartOfLineOfTokens = nil;
	fAtEndOfLine = FALSE;
}

#define WHITESPACE " \015\012"     // token delimiter

char *TImapServerState::GetNextToken()
{
	if (!fCurrentLine || fAtEndOfLine)
		AdvanceToNextLine();
	else if (Connected())
	{
		fNextToken = strtok(nil, WHITESPACE);
		if (!fNextToken)
		{
			fAtEndOfLine = TRUE;
			fNextToken = CRLF;
		}
	}
	
	return fNextToken;
}

void TImapServerState::AdvanceToNextLine()
{
	delete fCurrentLine;
	delete fStartOfLineOfTokens;
	
	int readStatus;
	fCurrentLine = fServerConnection.CreateNewLineFromSocket(readStatus);
	if (readStatus < 0)
	{
		SetConnected(FALSE);
		fStartOfLineOfTokens = nil;
		fLineOfTokens = nil;
		fNextToken = nil;
	}
	else
	{
		fStartOfLineOfTokens = XP_STRDUP(fCurrentLine);
		fLineOfTokens = fStartOfLineOfTokens;
		fNextToken = strtok(fLineOfTokens, WHITESPACE);
		if (!fNextToken)
		{
			fAtEndOfLine = TRUE;
			fNextToken = CRLF;
		}
		else
			fAtEndOfLine = FALSE;
	}
}

XP_Bool TImapServerState::LastCommandSuccessful()
{
	return Connected() && !SyntaxError() && !CommandFailed();
}

void TImapServerState::SetSyntaxError(XP_Bool error)
{
	fSyntaxError = error;
	fSyntaxErrorLine = XP_STRDUP(fCurrentLine);
}

char *TImapServerState::CreateSyntaxErrorLine()
{
	return XP_STRDUP(fSyntaxErrorLine);
}

XP_Bool TImapServerState::CurrentFolderReadOnly()
{
	return fCurrentFolderReadOnly;
}

int	TImapServerState::NumberOfMessages()
{
	return fNumberOfExistingMessages;
}

int 	TImapServerState::NumberOfRecentMessages()
{
	return fNumberOfRecentMessages;
}

int 	TImapServerState::NumberOfUnseenMessages()
{
	return fNumberOfUnseenMessages;
}

int32 TImapServerState::FolderUID()
{
	return fFolderUIDValidity;
}


XP_Bool TImapServerState::SyntaxError()
{
	return fSyntaxError;
}

void TImapServerState::SetConnected(XP_Bool connected)
{
	fDisconnected = !connected;
}

XP_Bool TImapServerState::Connected()
{
	return !fDisconnected;
}

XP_Bool TImapServerState::ContinueParse()
{
	return !fSyntaxError && !fDisconnected;
}


XP_Bool TImapServerState::at_end_of_line()
{
	return XP_STRCMP(fNextToken, CRLF) == 0;
}

XP_Bool TImapServerState::IsNumericString(const char *string)
{
	int length = strlen(string);
	XP_Bool numeric=TRUE;
	
	for(int index=0; (index < length) && numeric; index++)
		numeric = isdigit(*(string+index));
	
	return numeric;
}