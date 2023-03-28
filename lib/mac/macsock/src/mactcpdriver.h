#pragma once
// mactcpdriver.h
// OUTSTANDING PROBLEMS:
// How do we tell that the send failed, or that it was successful

#include "CNetwork.h"
#include <LList.h>
#include <MacTCP.h>
#include <AddressXlation.h>

class MacSocket;
struct AnnotatedPB;
class MacTCPStream;
class CMacTCPDNSObject;
#define driverError -23018		// Miscelaneous errors from PBAsync
#define streamBusy	-23019		// We already have an async call going


// =========================================================================
// AnnotatedPB
// =========================================================================
struct AnnotatedPB {
	QElemPtr		qLink;			// Standard QElem fields
	short 			qType;
	int				streamType;		// TCP or UDP?
	MacTCPStream * 	stream;			// Stream that made the request
	MacTCPDriver * 	driver;			// Driver. Needs to be in here since we cannot access globals
	union {
		TCPiopb 		tcpPB;		// Param block recs
		UDPiopb			udpPB;
	}				pb;
};

// =========================================================================
//  PeekBuffer - Buffer used for UDP read-aheads.
// =========================================================================
struct PeekBuffer {
	struct sockaddr		from;
	char				data[];
};


/****************************************************************************************
 * MacTCPDriver
 * makes sure that MacTCP is opened and closed properly
 * takes care of the streams too
 * All the async calls are processed through the driver
 * For VM, all data structures accessed at interrupt time are locked/unlocked with HoldMemory
 ****************************************************************************************/
// DNR completion routine
static pascal void DNRDone (struct hostInfo * hostInformation,
							Ptr userData ); 	 // CMacTCPDNSObject

class MacTCPDriver : public CNetworkDriver
{
public:

					MacTCPDriver();
	virtual 		~MacTCPDriver();
	CNetStream * 	CreateStream(MacSocket * socket);	// Creates the stream, can throw

// ее access
	UInt16			GetStreamBufferSize();				// Receive buffer size for streams.
	
// ее MacTCP calling interface 
	virtual void	SpendTime();						// Process async returns
	AnnotatedPB *	GetPB(int streamType, int pbKind, MacTCPStream * stream); // Returns a PB for TCP calls
	void			ReturnPB(AnnotatedPB * pb);			// If you are not making async call, 
														// you have to return APB manually
	void			MakeAsyncCall(AnnotatedPB * pb);	// Calls MacTCP async call
						// Errors are handled by dispatching return immediately back to stream
						// code driverError
	void			CompleteAsyncCall(AnnotatedPB * pb);			// Completion callback
	void			DispatchCompleteAsyncCall(AnnotatedPB * apb); 	// Dispatch of the completion callback
	OSErr			MakeSyncCall(AnnotatedPB * pb);

// ее DNS interface
	virtual CDNSObject * StrToAddress(char * hostName);
	virtual CDNSObject * StrToAddress(char * hostName, MacSocket * socket);
	virtual int		GetHostName(char *name, int namelen);			// Unix gethostname function
	char *			AddressToString(InetHost host);

protected:
	OSErr DoOpenDriver();
private:
friend static pascal void DNRDone (struct hostInfo *, Ptr userData);
friend class CMacTCPDNSObject;
	Int16	fDriverNum;			// The OS driver number
	LList	fPBs;				// recycled PB's
	QHdr	fOutstanding;		// Outstanding async requests
	QHdr	fReceived;			// Completed async requests
	QHdr	fWaitDNSCalls;		// DNS requests
	Boolean	fQueueLock;			// When walking down async queue, set this and check it to see if queues have been modified
};


class CMacTCPDNSObject : public CDNSObject	{
public:
	CMacTCPDNSObject();
	CMacTCPDNSObject(MacSocket * socket);
	virtual ~CMacTCPDNSObject();
	
	hostInfo fTCPHostInfo;		// MacTCP DNR structure -- the result
	void	TCPHostInfoToOT(struct hostInfo * hostInformation);
};

#define BIG_WDS_COUNT 20
/****************************************************************************************
 * Abstraction of MacTCP streams.
 * Each MacTCPStream is associated with 1 MacTCP stream.
 * Most calls are made asynchronously for speed.
 * Some asynchronous calls caused crashes in MacTCP, so they are synchronous
 *
 * Calls currently used by the driver:
 * TCPCreate, TCPActiveOpen, TCPClose, TCPRelease, TCPSend, TCPRead, TCPStatus
 ****************************************************************************************/
class MacTCPStream : public CNetStream
{
friend class MacTCPDriver;
public:
// ее constructors
	MacTCPStream(MacSocket * socket);
	~MacTCPStream();

// ее completion routines
	// Notification callback
	void TCPNotify(unsigned short eventCode, unsigned short terminReason, struct ICMPReport *icmpMsg);
	void UDPNotify(unsigned short eventCode, struct ICMPReport *icmpMsg);
	// MacTCPDriver async calls callbacks. They return Unix error codes
	int CreateTCPComplete(AnnotatedPB * apb);
	int CreateUDPComplete(AnnotatedPB * apb);
	int TCPActiveOpenComplete(AnnotatedPB * apb);
	int TCPPassiveOpenComplete(AnnotatedPB * apb);
	int TCPSendComplete(AnnotatedPB * apb);
	int TCPCloseComplete(AnnotatedPB * apb);
	int TCPReleaseComplete(AnnotatedPB * apb);

public:
// ее socket interface
	
	// standard socket calls
	virtual int CreateStream();
	virtual int	DestroyStream(Boolean abort);
	
	virtual int	ActiveConnect(InetHost host, InetPort port);
	virtual int	Bind(InetHost host, InetPort port);
	virtual int Listen(int backlog);
	virtual int Accept();
	virtual int Write(const void *buffer, unsigned int buflen, UInt16& bytesWritten);
	virtual int Read(void *buffer, unsigned int buflen, UInt16& bytesRead);

	virtual int UDPSendTo(const void *msg, unsigned int msgLen, unsigned int flags, InetHost host, InetPort port, unsigned int& bytesSent);
	virtual int UDPReceiveFrom(void *buffer, unsigned int len, unsigned int flags, struct sockaddr *from, unsigned int& fromLen);
	virtual int SetSocketOption(int level, int optname, const void *optval);
	virtual int BytesAvailable(size_t& bytesAvailable);

	virtual Boolean Select(Boolean& readReady,Boolean& writeReady,Boolean& exceptReady);
	virtual int GetPeerName(InetHost& host, InetPort& port);
	virtual int GetSockName(InetHost& host, InetPort& port);
private:
	Int32		TCPErrorToUnix(int tcpCall, int error);	// Checks error, and translates it into proper Unix error code
	
	// Variables
	StreamPtr	fStream;				// MacTCP stream
	void *		fBuffer;				// MacTCP buffer. Belongs to MacTCP.
	UInt16		fBufferSize;			// Size of the buffer

	Boolean		fLastCallInProgress;	// True, there is an outstanding async call
										// False, there are no outstanding async calls
	Int16		fLastCallResult;		// Progress of the last asynchronous call
	
	int			fType;

	Boolean		fIsBound;				// Has this port been bound?  Can╒t use host address for UDP, it will be zero
	InetHost	fBoundHost;				// Default port/host
	InetPort 	fBoundPort;
	UInt16		fPendingUDPReceives;	// How many outstanding unfinished reads? 
		
// Hacks
	struct PeekBuffer* fPeekBuffer;		// UDP read-ahead buffer
	Boolean		fNoMoreWrites;			// No more writes
// ее Misc utility functions
	// Are we waiting for a stream? Returns true if we have a stream (TCPCreate is complete)
	int			WaitForStream(Boolean& waiting);
	// Generic routine to create any type of socket
	int 		CreateTypedStream(InetPort port);
	
	int 		TCPDestroyStream(Boolean abort);
	int			UDPDestroyStream(Boolean abort);
	
#ifdef DEBUG
	Boolean 	fConnectCalled;				// Connection has failed?
	int			fLastAsyncCall;				// Last call we've made
#endif

	int 		TCPCreateComplete(AnnotatedPB * apb);
	int			UDPCreateComplete(AnnotatedPB * apb);
	int			UDPReadIntoBuffer(void* buffer, unsigned int len, struct sockaddr* from);

};