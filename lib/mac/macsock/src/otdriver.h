#pragma once
// otdriver.h

#include "CNetwork.h"

class MacSocket;
class COTDNSObject;
class OTStream;


// OTStreamEvent is used to place OTStream requests for completion
// on the OTDriver completion queue
// Big issue on how are these created and disposed of
struct OTStreamEvent	{
	QElemPtr		qLink;	// Standard QElem fields
	short 		qType;
	OTEventCode	event;
	OTResult		result;
	OTStream *	stream;	// Whose event is this?
};

/****************************************************************************************
 * OTDriver
 * makes sure that OT is opened and closed properly
 * Dispatches the DNS calls
 * Stream housekeeping (port numbers, asynchronous notifications)
 ****************************************************************************************/
class OTDriver : public CNetworkDriver
{
public:

					OTDriver();
	virtual 		~OTDriver();
	CNetStream * CreateStream(MacSocket * socket);	// Creates the stream, can throw

// ¥¥ access
	UInt16			GetStreamBufferSize();	// Receive buffer size for streams.

// ¥¥ asynchronous processing
	virtual void	SpendTime();			// Process async returns
	void			PostStreamEvent(OTStreamEvent * event);

protected:
	QHdr			fNotifications;			// Queue of OTStreamEvent

// ¥¥ DNS interface
public:
	virtual CDNSObject * StrToAddress(char * hostName);
	virtual CDNSObject * StrToAddress(char * hostName, MacSocket * socket);
	virtual int		GetHostName(char *name, int namelen);// Unix gethostname function
	char *			AddressToString(InetHost host);

protected:
	OSErr DoOpenDriver();
private:
	friend class COTDNSObject;

// Stream timeout interface
// When stream is closing, OT streams do not timeout on disconnect call, so we do not know
// when to delete them.
public:
	void		SetStreamTimeout(CNetStream * stream, SInt32 ticks);	// Timeout stream ticks from now
	void		RemoveStreamTimeout(CNetStream * stream);

private:
	LList *		fTimeoutStreams;	// List of TimeoutStructs
		// Because OT SndDisconnect calls do not time out, we need to time them out ourselves

};

/****************************************************************************************
 * class COTDNSObject
 ****************************************************************************************/
class COTDNSObject : public CDNSObject	{
public:
	COTDNSObject();
	COTDNSObject(MacSocket * socket);
	virtual ~COTDNSObject();
	InetSvcRef	fRef;
private:
	void		InitOTServices();
};

/****************************************************************************************
 * class OTStream
 ****************************************************************************************/
class OTStream : public CNetStream
{
friend class OTDriver;
public:
// ¥¥ constructors
	OTStream(MacSocket * socket);
	~OTStream();

//  ¥¥ notification mechanism
	// async notification routine
	void Notify(OTEventCode code, OTResult result, void * cookie);
	// delayed notification from the idle loop
	void ProcessStreamEvent( OTStreamEvent * event);
	
// ¥¥ socket interface
	// standard socket calls
	virtual int CreateStream();
	virtual int	DestroyStream(Boolean abort);
	
	virtual int	ActiveConnect(InetHost host, InetPort port);
	virtual int	Bind(InetHost host, InetPort port);
	virtual int Listen(int backlog);
	virtual int Accept();
	virtual int Write(const void *buffer, unsigned int buflen, UInt16& bytesWritten);
	virtual int Read(void *buffer, unsigned int buflen, UInt16& bytesRead);

	virtual Boolean Select(Boolean& readReady,Boolean& writeReady,Boolean& exceptReady);
	virtual int GetPeerName(InetHost& host, InetPort& port);
	virtual int GetSockName(InetHost& host, InetPort& port);
	
	virtual int UDPSendTo(const void *msg, unsigned int msgLen, unsigned int flags, InetHost host, InetPort port, unsigned int& bytesWritten);
	virtual int UDPReceiveFrom(void *buffer, unsigned int len, unsigned int flags, struct sockaddr *from, unsigned int& fromLen);
	
	virtual int SetSocketOption(int level, int optname, const void *optval);
	virtual int BytesAvailable(size_t& bytesAvailable);

private:
	enum OTCallsEnum	{
		eConnect,
		eBind,
		eListen,
		eRead,
		eWrite,
		eAccept,
		eReceive
	};
	int OTErrorToUnix(OTCallsEnum callKind, int error);	// Checks error, and translates it into proper Unix error code
	
// Open Transport variables
	TEndpointInfo	fInfo;
	TEndpoint *		fEndpoint;
	
	// Bind
	TBind				fReqBind;			// req address to Bind, p52, OT client guide
	TBind				fRetBind;			// ret address to Bind
	InetAddress			fReqBindAddress;	// local address, pointed to by fLocalBind
	InetAddress			fRetBindAddress;	// remote address pointed to by fRetBind
	// Connect/Listen
	TCall				fSndCall;			// Connect arguments
	InetAddress			fConnectAddress;	// Also used by UDP send.
	// Closing 
	Boolean				fDoNotDelete;		// Set the flag when closing
	Boolean				fNeedDeletion;		// Set the flag if we receive a delete event while closing
	Boolean				fGotOrdRel;			// True if we got T_ORDREL event (can read, can't write)
	Boolean				fDeleteOnOrdRel;	// True if we should delete the socket on ordRel
// Notification variables
	OTStreamEvent		fConnectNotification;
	OTStreamEvent		fWriteNotification;
	OTStreamEvent		fReleaseNotification;
};
