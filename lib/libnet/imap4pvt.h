/* classes and functions used to implement the IMAP4 netlib module.
   Kevin McEntee
*/

/* 45678901234567890123456789012345678901234567890123456789012345678901234567890
*/ 
#include "mkimap4.h"
#include "prthread.h"
#include "prglobal.h"
#include "prmon.h"
#include "xp_list.h"

#define IMAP4_PORT 143  /* the iana port for imap4 */

/*
	Using a separate thread to drive the IMAP4 server connection algorithm
	is being introduced in Dogbert.  Since this adding another thread to an
	otherwise single threaded program (navigator), then we need a mechanism
	for the IMAP thread to use existing, non thread safe, code in Dogbert.
	It is a basic rule that all pre-existing code should be executed in the 
	'main' thread.
	
	TImapFEEventQueue and TImapFEEvent exist to accomplish this goal.
	
	Whenever the imap thread needs to do something like post an alert or
	operate a thermo, then it will add an TImapFEEvent to a TImapFEEventQueue.
	
	The main thread, in NET_ProcessIMAP4, will periodically check the queue
	and execute any events.
	
	This way of doing things is an artifact of 'single threadedness' of 
	navigator.  It may go away in the future.
	
*/
typedef void (FEEventFunctionPointer)(void*,void*);

class TImapFEEvent {
public:
	TImapFEEvent(FEEventFunctionPointer *function, void *arg1, void*arg2);
	virtual ~TImapFEEvent();
	
	virtual void DoEvent();
private:
	FEEventFunctionPointer *fEventFunction;
	void			  	   *fArg1, *fArg2;
};

// all of the member functions of TImapFEEventQueue are multithread safe
class TImapFEEventQueue {
public:
	TImapFEEventQueue();
	virtual ~TImapFEEventQueue();
	
	virtual void 			AdoptEventToEnd(TImapFEEvent *event);
	virtual void 			AdoptEventToBeginning(TImapFEEvent *event);
	virtual TImapFEEvent*	OrphanFirstEvent();
	
	virtual int				NumberOfEventsInQueue();
private:
	XP_List 	*fEventList;
	PRMonitor   *fListMonitor;
	int			 fNumberOfEvents;
};


/* This class is used by the imap module to get info about a
   url.  This ensures that the knowledge about the URL syntax
   is not spread across hundreds of strcmp calls.
*/

class TIMAPUrl {
public:
	TIMAPUrl(const char *url_string);
	TIMAPUrl(const TIMAPUrl &source);
	virtual ~TIMAPUrl();
	
	XP_Bool IsBiffUrl();
	XP_Bool IsMessageFetchURL();
	
	// these 2 functions Precondition : IsMessageFetchURL()
	char *CreateFolderPathString();
	int  GetMessageSequenceNumber();
private:
	char *fUrlString;
	char *fFolderPathSubString;
	int	  fMessageSequenceNumber;
};


/*
	This virtual base class contains the interface and the code
	that drives the algorithms for performing imap4 server
	transactions.  
	
	This base class contains no knowledge of 'how things were' before
	dogbert.  The knowledge of our URL language, how we do socket io,
	and how we perform events in other threads is found in a concrete
	subclass.
*/

#define kOutputBufferSize 512

class TIMAP4BlockingConnection {
public:
	virtual ~TIMAP4BlockingConnection();
	
	int 			  GetConnectionStatus();
	
	// These 2 functions are thread safe.  If BlockedForIO then it will
	// become unblocked with a call to NotifyIOCompletionMonitor
	void			  NotifyIOCompletionMonitor();
	XP_Bool			  BlockedForIO();
		
	virtual void ProcessBIFFRequest();
	
	virtual void InsecureLogin(char *userName, char *password);
	virtual void SelectMailbox(char *mailboxName);
	virtual void FetchMessage(int messageSequenceNumber) = 0;
	
	virtual void AlertUserEvent(char *message) = 0;
	virtual void ProgressUpdateEvent(char *message) = 0;
	
protected:
	void 		 SetConnectionStatus(int status);
	void		 WaitForIOCompletion();
	void		 WaitForFEEventCompletion();
	
	// manage the IMAP server command tags
	void IncrementCommandTagNumber();

	// does a blocking read of the socket, returns a newly allocated
	// line or nil and a status
	virtual char *CreateNewLineFromSocket(int &socketReadStatus) = 0;
	
	// does a write to the socket
	virtual int WriteLineToSocket(char *line) = 0;
	
	// establish connection, this will not alert the user if there is a problem
	// affects GetConnectionStatus()
	virtual void EstablishServerConnection() = 0;
	
	
	// called from concrete subclass constructor
	TIMAP4BlockingConnection();
	
	char *GetOutputBuffer();
	char *GetServerCommandTag();
	PRMonitor *GetDataMemberMonitor();
	

private:
	XP_Bool				fBlockedForIO;
	int					fConnectionStatus;
	PRMonitor   		*fDataMemberMonitor;
	PRMonitor   		*fIOMonitor;
	char				fCurrentServerCommandTag[10];	// enough for a billion
	int					fCurrentServerCommandTagNumber;
	char				fOutputBuffer[kOutputBufferSize];
};



class TNavigatorImapConnection : public TIMAP4BlockingConnection {
public:
	TNavigatorImapConnection(const TIMAPUrl    &currentUrl,
							 ActiveEntry * ce);
	virtual ~TNavigatorImapConnection();
	
	virtual void ProcessCurrentURL();
	virtual void ProcessFetchRequest();
	
	
	
	TImapFEEventQueue &GetFEEventQueue();
	void			  SetBlockingThread(PRThread *blockingThread);
	
	// this function is thread safe
	// this function is used by the fe functions to signal their completion
	void			  NotifyEventCompletionMonitor();
	
	// not thread safe.  intended to be called from the fe event functions
	ActiveEntry      *GetActiveEntry();
	TCP_ConData      **GetTCPConData();
	
	
	// these functions might seem silly but I don't want the IMAP thread 
	// to ever muck with the ActiveEntry structure fields, so the socket 
	// will be set when we begin the connection.  These setters and getters 
	// are thread safe
	void			SetIOSocket(NETSOCK socket);
	NETSOCK			GetIOSocket();
	
	void			SetOutputStream(NET_StreamClass *outputStream);
	NET_StreamClass *GetOutputStream();
	
	virtual void FetchMessage(int messageSequenceNumber) ;
	virtual void AlertUserEvent(char *message);
	virtual void ProgressUpdateEvent(char *message);
	
	void		 WaitForFEEventCompletion();
	
	// does a blocking read of the socket, returns a newly allocated
	// line or nil and a status
	virtual char *CreateNewLineFromSocket(int &socketReadStatus);
	
	// does a write to the socket
	virtual int WriteLineToSocket(char *line);
	
	// establish connection, this will not alert the user if there is a problem
	// affects GetConnectionStatus()
	virtual void EstablishServerConnection();
	
private:
	XP_Bool				fWaitingForFEEventCompletion;
	TIMAPUrl     		fCurrentUrl;
	ActiveEntry 		*fCurrentEntry;
	TCP_ConData			*fTCPConData;
	TImapFEEventQueue	fFEEventQueue;
	PRThread			*fBlockingThread;
	PRMonitor   		*fEventCompletionMonitor;
	NETSOCK				fIOSocket;
	int32				fInputBufferSize;
	char 			    *fInputSocketBuffer;
	NET_StreamClass		*fOutputStream;
};
