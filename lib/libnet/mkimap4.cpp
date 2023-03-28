#include "imap4pvt.h"


/* 45678901234567890123456789012345678901234567890123456789012345678901234567890
*/ 

extern int XP_ERRNO_EWOULDBLOCK;
extern int MK_TCP_READ_ERROR;
extern int MK_TCP_WRITE_ERROR;

// These C functions implemented here are usually executed as TImapFEEvent's

// This function creates an output stream used to present an RFC822 message
void SetupMsgWriteStream(void *blockingConnectionVoid, void *unused)
{
	TNavigatorImapConnection *imapConnection = 
		(TNavigatorImapConnection *) blockingConnectionVoid;
	ActiveEntry *ce = imapConnection->GetActiveEntry();

	StrAllocCopy(ce->URL_s->content_type, MESSAGE_RFC822);
	
	NET_StreamClass *outputStream = 
				 NET_StreamBuilder(FO_PRESENT, ce->URL_s, ce->window_id);	
				 				   // FO_PRESENT is present only, no caching

	imapConnection->SetOutputStream(outputStream);
}

// This function adds one line to current message presentation
void ParseAdoptedMsgLine(void *blockingConnectionVoid, 
                         void *adoptedMsgVoid)
{
	TNavigatorImapConnection *imapConnection = 
		(TNavigatorImapConnection *) blockingConnectionVoid;
	ActiveEntry *ce = imapConnection->GetActiveEntry();
	
	char *adoptedMsg = (char *) adoptedMsgVoid;
	NET_StreamClass *outputStream = imapConnection->GetOutputStream();
	
	if ( (*outputStream->is_write_ready) (outputStream->data_object) < 
	     (strlen(adoptedMsg) + 2))
	{
		// the output stream can't handle this event yet! put it back on the
		// queue
		TImapFEEvent *parseLineEvent = 
			new TImapFEEvent(ParseAdoptedMsgLine, 		// function to call
				 			 blockingConnectionVoid, 	// access to current entry
				 			 adoptedMsgVoid);		    // line to display
		imapConnection->GetFEEventQueue().AdoptEventToBeginning(parseLineEvent);
	}
	else
	{
		// hack to add CRLF
		// we really should be blowing big blocks at this rather than 1 line
		// at a time!, maybe not?
		StrAllocCat(adoptedMsg, CRLF);
		
		
		(*outputStream->put_block) (outputStream->data_object,
									adoptedMsg,
									strlen(adoptedMsg));
		delete adoptedMsg;
	}
}

// This function closes the message presentation stream
void NormalEndMsgWriteStream(void *blockingConnectionVoid, void *unused)
{
	TNavigatorImapConnection *imapConnection = 
		(TNavigatorImapConnection *) blockingConnectionVoid;
	
	NET_StreamClass *outputStream = imapConnection->GetOutputStream();
	(*outputStream->complete) (outputStream->data_object);
}

// This function aborts the message presentation stream
void AbortMsgWriteStream(void *blockingConnectionVoid, void *unused)
{
	TNavigatorImapConnection *imapConnection = 
		(TNavigatorImapConnection *) blockingConnectionVoid;
	
	NET_StreamClass *outputStream = imapConnection->GetOutputStream();
	(*outputStream->abort) (outputStream->data_object, -1);
}

// This function displays a modal alert dialog
void AlertEventFunction(void *blockingConnectionVoid, 
                        void *errorMessageVoid)
{
	TNavigatorImapConnection *imapConnection = 
		(TNavigatorImapConnection *) blockingConnectionVoid;
	ActiveEntry *ce = imapConnection->GetActiveEntry();
	
	FE_Alert(ce->window_id, (const char *) errorMessageVoid);
	
	imapConnection->NotifyEventCompletionMonitor();
}

// This function updates the progress message at the bottom of the
// window
void ProgressEventFunction(void *blockingConnectionVoid, 
                           void *progressMessageVoid)
{
	TNavigatorImapConnection *imapConnection = 
		(TNavigatorImapConnection *) blockingConnectionVoid;
	ActiveEntry *ce = imapConnection->GetActiveEntry();
	
	FE_Progress(ce->window_id, (const char *) progressMessageVoid);
	
	imapConnection->NotifyEventCompletionMonitor();
}

// This function uses the existing NET code to establish
// a connection with the IMAP server.
void StartIMAPConnection(void *blockingConnectionVoid, 
                         void *connectionStatusVoid)
{
	TNavigatorImapConnection *imapConnection = 
		(TNavigatorImapConnection *) blockingConnectionVoid;
	
	int *connectionStatusPtr = (int *) connectionStatusVoid;
	
	ActiveEntry *ce = imapConnection->GetActiveEntry();
	
	FE_SetProgressBarPercent(ce->window_id, -1);

	ce->status = NET_BeginConnect(ce->URL_s->address,
								  ce->URL_s->IPAddressString,
                             	"IMAP4",
                             	IMAP4_PORT,
                             	&ce->socket,
                             	FALSE,
                             	imapConnection->GetTCPConData(),
                             	ce->window_id,
                             	&ce->URL_s->error_msg,
								 ce->socks_host,
								 ce->socks_port);

	imapConnection->SetIOSocket(ce->socket);
	if(ce->socket != SOCKET_INVALID)
    	NET_TotalNumberOfOpenConnections++;

	if(ce->status == MK_CONNECTED)
      {
    	FE_SetReadSelect(ce->window_id, ce->socket);
      }
	else if(ce->status > -1)
      {
    	ce->con_sock = ce->socket;  /* set con sock so we can select on it */
    	FE_SetConnectSelect(ce->window_id, ce->con_sock);
      }
	else if(ce->status < 0)
	  {
		/* close and clear the socket here
		 * so that we don't try and send a RSET
		 */
		if(ce->socket != SOCKET_INVALID)
		  {
    		NET_TotalNumberOfOpenConnections--;
    		FE_ClearConnectSelect(ce->window_id, ce->socket);
    		TRACEMSG(("Closing and clearing socket ce->socket: %d", 
    				 ce->socket));
    		NETCLOSE(ce->socket);
			ce->socket = SOCKET_INVALID;
		  }
	  }
	  
	*connectionStatusPtr = ce->status;
}

// This function uses the existing NET code to establish
// a connection with the IMAP server.
void FinishIMAPConnection(void *blockingConnectionVoid, 
                          void *connectionStatusVoid)
{
	TNavigatorImapConnection *imapConnection = 
		(TNavigatorImapConnection *) blockingConnectionVoid;
	
	int *connectionStatusPtr = (int *) connectionStatusVoid;
	
	ActiveEntry *ce = imapConnection->GetActiveEntry();
	
	ce->status = NET_FinishConnect(ce->URL_s->address,
                              	"IMAP4",
                              	IMAP4_PORT,
                              	&ce->socket,
                             	imapConnection->GetTCPConData(),
                              	ce->window_id,
                              	&ce->URL_s->error_msg);


	if(ce->status == MK_CONNECTED)
      {
    	FE_ClearConnectSelect(ce->window_id, ce->con_sock);
    	FE_SetReadSelect(ce->window_id, ce->socket);
      }
	else if(ce->status > -1)
      {

    	/* unregister the old CE_SOCK from the select list
     	 * and register the new value in the case that it changes
     	 */
    	if(ce->con_sock != ce->socket)
      	  {
        	FE_ClearConnectSelect(ce->window_id, ce->con_sock);
        	ce->con_sock = ce->socket;
        	FE_SetConnectSelect(ce->window_id, ce->con_sock);
      	  }
      }
	else if(ce->status < 0)
	  {
		/* close and clear the socket here
		 * so that we don't try and send a RSET
		 */
    	NET_TotalNumberOfOpenConnections--;
    	FE_ClearConnectSelect(ce->window_id, ce->socket);
    	TRACEMSG(("Closing and clearing socket ce->socket: %d", ce->socket));
    	NETCLOSE(ce->socket);
		ce->socket = SOCKET_INVALID;
	  }
	  
	*connectionStatusPtr = ce->status;
}



// member functions of the TImapFEEvent class
TImapFEEvent::TImapFEEvent(FEEventFunctionPointer *function, 
						   void *arg1, void*arg2) :
	fEventFunction(function),
	fArg1(arg1), fArg2(arg2)
{
	
}

TImapFEEvent::~TImapFEEvent()
{
}

void TImapFEEvent::DoEvent()
{
	(*fEventFunction) (fArg1, fArg2);
}



// member functions of the TImapFEEventQueue class
TImapFEEventQueue::TImapFEEventQueue()
{
	fEventList = XP_ListNew();
	fListMonitor = PR_NewMonitor(0);
	fNumberOfEvents = 0;
}

TImapFEEventQueue::~TImapFEEventQueue()
{
	XP_ListDestroy(fEventList);
	PR_DestroyMonitor(fListMonitor);
}

void TImapFEEventQueue::AdoptEventToEnd(TImapFEEvent *event)
{
	PR_EnterMonitor(fListMonitor);
	XP_ListAddObjectToEnd(fEventList, event);
	fNumberOfEvents++;
	PR_ExitMonitor(fListMonitor);
}

void TImapFEEventQueue::AdoptEventToBeginning(TImapFEEvent *event)
{
	PR_EnterMonitor(fListMonitor);
	XP_ListAddObject(fEventList, event);
	fNumberOfEvents++;
	PR_ExitMonitor(fListMonitor);
}

TImapFEEvent*	TImapFEEventQueue::OrphanFirstEvent()
{
	TImapFEEvent *returnEvent = nil;
	PR_EnterMonitor(fListMonitor);
	if (fNumberOfEvents)
	{
		fNumberOfEvents--;
		returnEvent = (TImapFEEvent *) XP_ListRemoveTopObject(fEventList);
	}
	PR_ExitMonitor(fListMonitor);
	
	return returnEvent;
}

int	TImapFEEventQueue::NumberOfEventsInQueue()
{
	return fNumberOfEvents;
}



// member functions of the TIMAPUrl class
TIMAPUrl::TIMAPUrl(const char *url_string)
 : fFolderPathSubString(nil), fMessageSequenceNumber(0)
{
	fUrlString = XP_STRDUP(url_string);
}

TIMAPUrl::TIMAPUrl(const TIMAPUrl &source)
{
	fUrlString = XP_STRDUP(source.fUrlString);
}

TIMAPUrl::~TIMAPUrl()
{
	delete fUrlString;
}

XP_Bool TIMAPUrl::IsBiffUrl()
{
	if (fUrlString && strcasestr(fUrlString, "?check"))
		return true;
	else
		return false;
}

char *TIMAPUrl::CreateFolderPathString()
{
	return XP_STRDUP(fFolderPathSubString);
}

int  TIMAPUrl::GetMessageSequenceNumber()
{
	return fMessageSequenceNumber;
}

// this function leaks all over the place!
XP_Bool TIMAPUrl::IsMessageFetchURL()
{
	XP_Bool isFetch = false;
	
	char *folderpath        = NET_ParseURL(fUrlString, GET_PATH_PART);
	char *messageIdentifier = NET_ParseURL(fUrlString, GET_SEARCH_PART);
	
	if (messageIdentifier)
	{
		if (*messageIdentifier == '?')
			messageIdentifier++;
		if (!XP_STRNCMP(messageIdentifier, "number=", 7))
		{
			isFetch = true;
			// until we can fix NET_ParseURL or write our own
			// eat the '/' character!
			fFolderPathSubString   = folderpath + 1;
			fMessageSequenceNumber = XP_ATOI(messageIdentifier + 7);
		}
	}
	
	
	return isFetch;
}






TIMAP4BlockingConnection::TIMAP4BlockingConnection() :
	fConnectionStatus(0),	// we want NET_ProcessIMAP4 to continue for now
	fBlockedForIO(false),
	fCurrentServerCommandTagNumber(0)
{
	fDataMemberMonitor       = PR_NewMonitor(0);
	fIOMonitor               = PR_NewMonitor(0);
}

TIMAP4BlockingConnection::~TIMAP4BlockingConnection()
{
	PR_DestroyMonitor(fDataMemberMonitor);
	PR_DestroyMonitor(fIOMonitor);
}

void TIMAP4BlockingConnection::NotifyIOCompletionMonitor()
{
	PR_EnterMonitor(fIOMonitor);
	fBlockedForIO = false;
	PR_Notify(fIOMonitor);			
	PR_ExitMonitor(fIOMonitor);
}

void TIMAP4BlockingConnection::IncrementCommandTagNumber()
{
	sprintf(fCurrentServerCommandTag,"%d", ++fCurrentServerCommandTagNumber);
}

void TIMAP4BlockingConnection::WaitForIOCompletion()
{
	PR_EnterMonitor(fIOMonitor);
	fBlockedForIO = true;
	while (fBlockedForIO)
		PR_Wait(fIOMonitor, LL_MAXINT);			
	PR_ExitMonitor(fIOMonitor);
}

XP_Bool TIMAP4BlockingConnection::BlockedForIO()
{
	XP_Bool returnValue;
	
	PR_EnterMonitor(fIOMonitor);
	returnValue = fBlockedForIO;
	PR_ExitMonitor(fIOMonitor);
	
	return returnValue;
}

void TIMAP4BlockingConnection::SetConnectionStatus(int status)
{
	PR_EnterMonitor(GetDataMemberMonitor());
	fConnectionStatus = status;
	PR_ExitMonitor(GetDataMemberMonitor());
}


int TIMAP4BlockingConnection::GetConnectionStatus()
{
	int returnStatus;
	PR_EnterMonitor(GetDataMemberMonitor());
	returnStatus = fConnectionStatus;
	PR_ExitMonitor(GetDataMemberMonitor());
	return returnStatus;
}


void TIMAP4BlockingConnection::SelectMailbox(char *mailboxName)
{
	ProgressUpdateEvent("selecting mailbox...");
	IncrementCommandTagNumber();
	
    PR_snprintf(GetOutputBuffer(), 						// string to create
     			kOutputBufferSize, 					// max size
     			"%s select %s" CRLF, 				// format string
     			GetServerCommandTag(), 			// command tag
     			mailboxName);
     			
    int 		ioStatus = WriteLineToSocket(GetOutputBuffer());
    XP_Bool		taggedResponseReceived = false;
    char 		*newLine = nil;
    
    while ((ioStatus >= 0) && !taggedResponseReceived)
    {
    	// read lines until the tagged response is sent
    	newLine = CreateNewLineFromSocket(ioStatus);
    	if ((ioStatus >= 0) && newLine)
    		taggedResponseReceived = 
    			XP_STRNCMP(newLine, 
    					   GetServerCommandTag(),
    					   XP_STRLEN(GetServerCommandTag())) == 0;
    	else
    		delete newLine;
    			
    }
    
    if (taggedResponseReceived)
    {
    	// a gross hack to verify OK until we have our response parser
    	if (!XP_STRNCMP(newLine + XP_STRLEN(GetServerCommandTag()), " OK", 3) )
    		SetConnectionStatus(0);
    	else
    	{
    		AlertUserEvent(newLine);
    		SetConnectionStatus(-1);	//stub
    	}
    }
}

void TIMAP4BlockingConnection::InsecureLogin(char *userName, char *password)
{
	ProgressUpdateEvent("sending login information...");
	IncrementCommandTagNumber();
	
    PR_snprintf(GetOutputBuffer(), 						// string to create
     			kOutputBufferSize, 					// max size
     			"%s login %s %s" CRLF, 				// format string
     			GetServerCommandTag(), 			// command tag
     			userName, 
     			password);
     			
    int 		ioStatus = WriteLineToSocket(GetOutputBuffer());
    XP_Bool		taggedResponseReceived = false;
    char 		*newLine = nil;
    
    while ((ioStatus >= 0) && !taggedResponseReceived)
    {
    	// read lines until the tagged response is sent
    	newLine = CreateNewLineFromSocket(ioStatus);
    	if ((ioStatus >= 0) && newLine)
    		taggedResponseReceived = 
    			XP_STRNCMP(newLine, 
    					   GetServerCommandTag(),
    					   XP_STRLEN(GetServerCommandTag())) == 0;
    	else
    		delete newLine;
    			
    }
    
    if (taggedResponseReceived)
    {
    	// a gross hack to verify OK until we have our response parser
    	if (!XP_STRNCMP(newLine + XP_STRLEN(GetServerCommandTag()), " OK", 3))
    		SetConnectionStatus(0);
    	else
    	{
    		AlertUserEvent(newLine);
    		SetConnectionStatus(-1);	//stub
    	}
    }
}



void TIMAP4BlockingConnection::ProcessBIFFRequest()
{
	EstablishServerConnection();
	if (GetConnectionStatus() >= 0)
		SetConnectionStatus(-1);		// stub!
}


char *TIMAP4BlockingConnection::GetOutputBuffer()
{
	return fOutputBuffer;
}

char *TIMAP4BlockingConnection::GetServerCommandTag()
{
	return fCurrentServerCommandTag;
}

PRMonitor *TIMAP4BlockingConnection::GetDataMemberMonitor()
{
	return fDataMemberMonitor;
}





TNavigatorImapConnection::TNavigatorImapConnection(const TIMAPUrl &currentUrl,
							 					   ActiveEntry * ce) :
	TIMAP4BlockingConnection(),
	fCurrentUrl(currentUrl), 
	fCurrentEntry(ce), 
	fWaitingForFEEventCompletion(false),
	fInputBufferSize(0),
	fInputSocketBuffer(nil),
	fBlockingThread(nil),
	fTCPConData(nil)
{
	fEventCompletionMonitor  = PR_NewMonitor(0);
}

TNavigatorImapConnection::~TNavigatorImapConnection()
{
	// we don't want the thread to continue running and use this
	// deleted object
	if (fBlockingThread && (fBlockingThread->state != _PR_ZOMBIE))
		PR_DestroyThread(fBlockingThread);
		
	PR_DestroyMonitor(fEventCompletionMonitor);
}


void TNavigatorImapConnection::WaitForFEEventCompletion()
{
	PR_EnterMonitor(fEventCompletionMonitor);
	fWaitingForFEEventCompletion = true;
	while (fWaitingForFEEventCompletion)
		PR_Wait(fEventCompletionMonitor, LL_MAXINT);			
	PR_ExitMonitor(fEventCompletionMonitor);
}


void TNavigatorImapConnection::NotifyEventCompletionMonitor()
{
	PR_EnterMonitor(fEventCompletionMonitor);
	fWaitingForFEEventCompletion = false;
	PR_Notify(fEventCompletionMonitor);			
	PR_ExitMonitor(fEventCompletionMonitor);
}


TImapFEEventQueue &TNavigatorImapConnection::GetFEEventQueue()
{
	return fFEEventQueue;
}

void TNavigatorImapConnection::FetchMessage(int messageSequenceNumber)
{
	ProgressUpdateEvent("fetching message...");
	IncrementCommandTagNumber();
	
    PR_snprintf(GetOutputBuffer(), 						// string to create
     			kOutputBufferSize, 					// max size
     			"%s fetch %d (RFC822)" CRLF, 		// format string
     			GetServerCommandTag(), 			// command tag
     			messageSequenceNumber);
     			
    int 		ioStatus = WriteLineToSocket(GetOutputBuffer());
    XP_Bool		fetchResponseReceived = false;
    char 		*newLine = nil;
    
    while ((ioStatus >= 0) && !fetchResponseReceived)
    {
    	// read lines until the fetch response is sent or bad
    	// this will be replaced by calls to parser
    	newLine = CreateNewLineFromSocket(ioStatus);
    	if ((ioStatus >= 0) && newLine)
    		fetchResponseReceived = 
    			strcasestr(newLine, "FETCH (RFC822 {") != 0; 
    	
    	if (!fetchResponseReceived)
    	{
    		if (strcasestr(newLine, "BAD") != 0)
    		{
    			AlertUserEvent(newLine);
    			ioStatus = -1;
    		}
    		delete newLine;
    	}
    		
    			
    }
    
    if (fetchResponseReceived)
    {
    	
		TImapFEEvent *setupStreamEvent = 
			new TImapFEEvent(SetupMsgWriteStream, 	// function to call
				 			this, 					// access to current entry
				 			nil);					   // nil

		fFEEventQueue.AdoptEventToEnd(setupStreamEvent);
		
    	char *byteCountSubString = strcasestr(newLine, "{") + 1;
    	int  bytesToRead = atoi(byteCountSubString);
    	
    	delete  newLine;
    	int		numberOfBytesFetched  = 0;
    	while ((ioStatus >= 0) && (numberOfBytesFetched < bytesToRead))
    	{
    		newLine = CreateNewLineFromSocket(ioStatus);
    		if ((ioStatus >= 0) && newLine)
    		{
    			numberOfBytesFetched += strlen(newLine) + 2; // 2 = CRLF
    			
    			// hack to see if newline is being stepped on
    			char *bogusNewLine = XP_STRDUP(newLine);
    			
				TImapFEEvent *parseLineEvent = 
					new TImapFEEvent(ParseAdoptedMsgLine, // function to call
						 			this, 			// access to current entry
						 			bogusNewLine);	// line to display
	
				fFEEventQueue.AdoptEventToEnd(parseLineEvent);
				PR_Yield();	// give the fe some time to eat lines
    		}
    	}
    	
    	// end the stream
    	if (numberOfBytesFetched >= bytesToRead)
    	{
			TImapFEEvent *endEvent = 
				new TImapFEEvent(NormalEndMsgWriteStream, // function to call
					 			this, 				// access to current entry
					 			nil);				// unused

			fFEEventQueue.AdoptEventToEnd(endEvent);
    	}
    	else
    	{
			TImapFEEvent *endEvent = 
				new TImapFEEvent(AbortMsgWriteStream, 	// function to call
					 			this, 				// access to current entry
					 			nil);				// unused

			fFEEventQueue.AdoptEventToEnd(endEvent);
    	}
    }
}




int TNavigatorImapConnection::WriteLineToSocket(char *line)
{
	int returnValue = 0;
	
	// NET_BlockingWrite is not thread safe, pretend for now
	// accessing fCurrentEntry fields is not thread safe
	int writeStatus = (int) NET_BlockingWrite(fCurrentEntry->socket,
										      line,
										      XP_STRLEN(line));
	if(writeStatus < 0)
	{
		fCurrentEntry->URL_s->error_msg = 
			NET_ExplainErrorDetails(MK_TCP_WRITE_ERROR, SOCKET_ERRNO);
		returnValue = MK_TCP_WRITE_ERROR;
	}
	else
		PR_Yield();		// WaitForIOCompletion();
	
	return returnValue;
}

char *TNavigatorImapConnection::CreateNewLineFromSocket(int &socketReadStatus)
{
	Bool	pauseForRead = true;
	char    *newLine = nil;
	
	while (pauseForRead)
	{
		// for now let's pretend that NET_BufferedReadLine
		// I need to rewrite a thread safe version
		// later so I don't do every thing in these
		// fe events!  
		socketReadStatus = NET_BufferedReadLine(GetIOSocket(),
										  	    &newLine,
										  	    &fInputSocketBuffer,
										  	    &fInputBufferSize,
										  	    &pauseForRead);
										  	    
		if (socketReadStatus == 0)	// should not happen, but check anyway
			pauseForRead = false;
		else if (socketReadStatus < 0)	// error
		{
	        int socketError = SOCKET_ERRNO;
	
	        if (socketError == XP_ERRNO_EWOULDBLOCK)
	        {
	            pauseForRead = true;
	            WaitForIOCompletion();
	        }
			else
			{
				// not thread safe!!!
	        	fCurrentEntry->URL_s->error_msg =
	        			NET_ExplainErrorDetails(MK_TCP_READ_ERROR, 
	        									socketError);
	            pauseForRead = false;
	        	socketReadStatus = MK_TCP_READ_ERROR;
	        }
		}
		else if (pauseForRead && !newLine)
			WaitForIOCompletion();
		else
			pauseForRead = false;
		
	}
	
	return newLine;
}

void TNavigatorImapConnection::SetIOSocket(NETSOCK socket)
{
	PR_EnterMonitor(GetDataMemberMonitor());
	fIOSocket = socket;
	PR_ExitMonitor(GetDataMemberMonitor());
}

NETSOCK TNavigatorImapConnection::GetIOSocket()
{
	NETSOCK returnSocket;
	PR_EnterMonitor(GetDataMemberMonitor());
	returnSocket = fIOSocket;
	PR_ExitMonitor(GetDataMemberMonitor());
	return returnSocket;
}

void TNavigatorImapConnection::SetOutputStream(NET_StreamClass *outputStream)
{
	PR_EnterMonitor(GetDataMemberMonitor());
	fOutputStream = outputStream;
	PR_ExitMonitor(GetDataMemberMonitor());
}

NET_StreamClass *TNavigatorImapConnection::GetOutputStream()
{
	NET_StreamClass *returnStream;
	PR_EnterMonitor(GetDataMemberMonitor());
	returnStream = fOutputStream;
	PR_ExitMonitor(GetDataMemberMonitor());
	return returnStream;
}


void TNavigatorImapConnection::SetBlockingThread(PRThread *blockingThread)
{
	fBlockingThread = blockingThread;
}

ActiveEntry      *TNavigatorImapConnection::GetActiveEntry()
{
	return fCurrentEntry;
}

TCP_ConData      **TNavigatorImapConnection::GetTCPConData()
{
	return &fTCPConData;
}


void TNavigatorImapConnection::ProcessCurrentURL()
{
	SetConnectionStatus(0);		// continue connection for now
	if (fCurrentUrl.IsBiffUrl())
		ProcessBIFFRequest();
	if (fCurrentUrl.IsMessageFetchURL())
		ProcessFetchRequest();
	else
		SetConnectionStatus(-1);	// this URL was bogus,
									// for now, quietly do nothing
}

void TNavigatorImapConnection::EstablishServerConnection()
{
	// let the fe-thread start the connection since we are using
	// old non-thread safe functions to do it.
	int connectionStatus = -1;
	TImapFEEvent *feStartConnectionEvent = 
		new TImapFEEvent(StartIMAPConnection, 		// function to call
						 this, 						// for access to current
						 							// entry and monitor
						 &connectionStatus);		// result
	
	fFEEventQueue.AdoptEventToEnd(feStartConnectionEvent);
	PR_Yield();
	
	// wait here for the connection start io to finish
	WaitForIOCompletion();
	
	// call NET_FinishConnect until we are connected or errored out
	while (connectionStatus == 	MK_WAITING_FOR_CONNECTION)
	{
		TImapFEEvent *feFinishConnectionEvent = 
			new TImapFEEvent(FinishIMAPConnection, 	// function to call
							 this, 					// for access to current
							 						// entry and monitor
							 &connectionStatus);	// result
		
		fFEEventQueue.AdoptEventToEnd(feFinishConnectionEvent);
		PR_Yield();
		
		// wait here for the connection finish io to finish
		WaitForIOCompletion();
		
	}	
	
	if (connectionStatus == MK_CONNECTED)
	{
		// get the one line response from the IMAP server
		char *serverResponse = CreateNewLineFromSocket(connectionStatus);
		if (serverResponse && !XP_STRNCASECMP(serverResponse, "* OK", 4) )
			connectionStatus = 0;
		else
			connectionStatus = -1;
	}
	
	SetConnectionStatus(connectionStatus);
}

void TNavigatorImapConnection::ProcessFetchRequest()
{
	EstablishServerConnection();
	if (GetConnectionStatus() >= 0)
	{
		InsecureLogin("kmcentee", "dog4bert");
		
		if (GetConnectionStatus() >= 0)
		{
			char *mailboxName = fCurrentUrl.CreateFolderPathString();
			if (mailboxName)
			{
				SelectMailbox(mailboxName);
				if (GetConnectionStatus() >= 0)
				{
					FetchMessage(fCurrentUrl.GetMessageSequenceNumber());
					SetConnectionStatus(-1);	// stub, should logout!
				}
			}
			else
			{
				AlertUserEvent(
				"Internal mail error: no specified mailbox in IMAP fetch URL");
				SetConnectionStatus(-1);
			}
		}
	}
}

void TNavigatorImapConnection::AlertUserEvent(char *message)
{
	TImapFEEvent *alertEvent = 
		new TImapFEEvent(AlertEventFunction, 		// function to call
						 this, 						// access to current entry
						 message);					// alert message
	
	fFEEventQueue.AdoptEventToEnd(alertEvent);
	WaitForFEEventCompletion();
}

void TNavigatorImapConnection::ProgressUpdateEvent(char *message)
{
	TImapFEEvent *progressEvent = 
		new TImapFEEvent(ProgressEventFunction, 	// function to call
						 this, 						// access to current entry
						 message);					// progress message
	
	fFEEventQueue.AdoptEventToEnd(progressEvent);
	WaitForFEEventCompletion();
}

void imap_thread_main_function(void *imapConnectionVoid,
							   void *arg2void)
{
	TNavigatorImapConnection    *imapConnection = 
		(TNavigatorImapConnection *) imapConnectionVoid;
	
	imapConnection->ProcessCurrentURL();
}

	
/* start a imap4 load
 */
extern int MK_POP3_PASSWORD_UNDEFINED;

MODULE_PRIVATE int
NET_IMAP4Load (ActiveEntry * ce)
{
	// hack for now to strip the ONLINE keyword.  We may do away
	// with the keyword altogether since we have // that identifies
	// an url as a net url.
	char *substring = strncasestr(ce->URL_s->address, "ONLINE:",16);
	if (substring)				
		XP_MEMMOVE(substring, 			/* start at beginning of substring */
				   substring + 7,		/* 7 = strlen("ONLINE:") */
				   strlen(ce->URL_s->address) - 14);/*-14=-15+1, 1 == '\0' */

	// create the blocking connection object
	TIMAPUrl url(ce->URL_s->address);
	TNavigatorImapConnection *blockingConnection = 
		new TNavigatorImapConnection(url, ce);
	
	// start the imap thread
	PRThread *imapThread = PR_CreateThread("imapIOthread",
											31,	// highest priority
											0);	// standard stack
	
	// save the thread object so an IMAP interrupt will
	// cause the TIMAP4BlockingConnection destructor to
	// destroy it.										
	blockingConnection->SetBlockingThread(imapThread);
	ce->con_data = blockingConnection;
	
	PR_Start(imapThread, 						// thread to start
			 imap_thread_main_function, 		// first function to call
			 blockingConnection,				// function argument
			 nil);								// function argument
	
	PR_Yield();
		
	
	return NET_ProcessIMAP4(ce);
}

/* NET_ProcessIMAP4  will control the state machine that
 * loads messages from a iamp4 server
 *
 * returns negative if the transfer is finished or error'd out
 *
 * returns zero or more if the transfer needs to be continued.
 */
MODULE_PRIVATE int
NET_ProcessIMAP4 (ActiveEntry *ce)
{
	TNavigatorImapConnection *imapConnection = 
		(TNavigatorImapConnection *) ce->con_data;
	assert(imapConnection);
	
	if (imapConnection->BlockedForIO() && 
	    !imapConnection->GetFEEventQueue().NumberOfEventsInQueue())
	{
		// we entered NET_ProcessIMAP4 because a net read is finished
		imapConnection->NotifyIOCompletionMonitor();
	}
	
	
		
	int connectionStatus = imapConnection->GetConnectionStatus();

	while ((connectionStatus >= 0) && 
	       		(!imapConnection->BlockedForIO() || 
	        	imapConnection->GetFEEventQueue().NumberOfEventsInQueue()))
	{	
		if (imapConnection->GetFEEventQueue().NumberOfEventsInQueue())
		{
			TImapFEEvent *event = 
				imapConnection->GetFEEventQueue().OrphanFirstEvent();
			event->DoEvent();
			delete event;
		}
		
		PR_Yield();
		connectionStatus = imapConnection->GetConnectionStatus();
	}
	
		
	if (connectionStatus < 0)
	{
		// we are done or errored out
		while (imapConnection->GetFEEventQueue().NumberOfEventsInQueue())
		{
			TImapFEEvent *event = 
				imapConnection->GetFEEventQueue().OrphanFirstEvent();
			event->DoEvent();
			delete event;
		}
		
		ce->con_data = nil;
		delete imapConnection;
	}
	
	return connectionStatus;
}


/* abort the connection in progress
 */
MODULE_PRIVATE int
NET_InterruptIMAP4(ActiveEntry * ce)
{
	TNavigatorImapConnection *imapConnection = 
		(TNavigatorImapConnection *) ce->con_data;
	
	ce->con_data = nil;
	ce->status = MK_INTERRUPTED;
	delete imapConnection;
	
	return -1;
}