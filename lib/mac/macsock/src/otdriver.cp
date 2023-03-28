// Mac utils
#include "MoreMixedMode.h"
// PowerPlant
#include <UException.h>

// stdclib
#include <stdlib.h>
#include <assert.h>

// Network stuff
#include <OpenTptInternet.h>
#include "otdriver.h"

// NSPR stuff
#include "swkern.h"

// For HoldMemoryGlue and UnholdMemoryGlue
#include "macutil.h"

#ifdef PROFILE
#pragma profile on
#endif
#define ThrowIfOTErr_(err) if ((err) < kOTNoError) Throw_(err)
#ifdef assert
#undef assert
#endif

#ifdef DEBUG
#define assert(x) if (!(x)) abort()
#else
#define assert(x) ((void)0)
#endif

#ifdef DEBUG
#pragma optimization_level 1
#pragma global_optimizer off
#endif

//#define TCP_DEBUG

#ifdef DEBUG
#include <LList.h>
LList validStreams;	// Keeps list of all open streams
#define ASSERT_VALID_STREAM(s) { if (validStreams.FetchIndexOf(s) == LDynamicArray::index_Bad) Debugger(); }
#else
#define ASSERT_VALID_STREAM(s)
#endif
#if !defined(DEBUG) && defined(TCP_DEBUG)
#error Aleks has forgotten to turn off TCP_DEBUG in this file again
#endif

// WriteBuffer is used to hold data during async writes
struct WriteBuffer	{
	OTStreamEvent streamEvent;
	union 			protocolSpecific {
		size_t			dataLen;			// For TCP
		TUnitData		udpSendInfo;		// For UDP
	} protocolSpecific;
	unsigned char	data[];
};


// TimeoutStruct is used to keep a list of streams that should be deleted on timeout
// See CNetworkDriver for more info
struct TimeoutStruct	{
	SInt32 	fTickCount;
	CNetStream * fStream;
	TimeoutStruct(){};
	TimeoutStruct(CNetStream * stream, SInt32 tickCount) 
	{
		fTickCount = tickCount; 
		fStream = stream;
	}
};

// =========================================================================
// OTDriver
// =========================================================================
#pragma mark OTDriver
#pragma mark -
// Reset all the variables. Does not open the driver
OTDriver::OTDriver() : CNetworkDriver()
{
	::HoldMemoryGlue( this, sizeof(*this) );
	fNotifications.qHead = fNotifications.qTail = NULL;
	fTimeoutStreams = new LList( sizeof (TimeoutStruct) );
}

// Just close all the sockets
OTDriver::~OTDriver()
{
	#ifdef TCP_DEBUG
	XP_Trace("OTDriver: Destructing OTDriver %#x\n", this);
	#endif
	
	// Delete all the sockets	
	for (int i=0; i<MAX_SOCKETS; i++)
		if (MacSocket::sSockets[i] != NULL) {
			#ifdef TCP_DEBUG
			XP_Trace("OTDriver: Destroying MacSocket #%d\n", i);
			#endif
			MacSocket::sSockets[i]->Destroy(TRUE);
		}

	// Delete expiring streams
	LListIterator iter(*fTimeoutStreams,iterate_FromStart);
	TimeoutStruct t;
	while (iter.Next(&t)) {
		#ifdef TCP_DEBUG
		XP_Trace("OTDriver: deleting stream %#x\n", t.fStream);
		#endif		
		delete t.fStream;
	}
	delete fTimeoutStreams;
	
	CloseOpenTransport();
	
	::UnholdMemoryGlue( this, sizeof(*this) );
}


CNetStream * OTDriver::CreateStream(MacSocket * socket)
{
	return new OTStream(socket);
}


// Opens the driver. Can fail.
OSErr OTDriver::DoOpenDriver()
{
	OSStatus err = InitOpenTransport();
	fDriverOpen = (err == kOTNoError);
	return err;
}

// When async calls complete, they end up on the fReceived queue
// we figure out what the calls were (stream, operation), and call
// the stream to handle the call (ignored right now
void OTDriver::SpendTime()
{
	// DNS queue
	
	QElemPtr dnsElem = fDoneDNSCalls.qHead;
	while (dnsElem != NULL)
	{
		COTDNSObject * dns = (COTDNSObject *)((UInt32)dnsElem - (UInt32)offsetof(COTDNSObject, fQLink));
		dnsElem = dnsElem->qLink;
		if (dns->fSocket)
			dns->fSocket->DNSNotify();
		else if (dns->fDeleteSelfWhenDone)
			delete dns;
	}
	
	// streams queue
	while(fNotifications.qHead != NULL)
	{
		OTStreamEvent * event = (OTStreamEvent *)fNotifications.qHead;
		OSErr err = ::Dequeue( (QElemPtr)event, &fNotifications );
		if (err) assert(FALSE);
		ASSERT_VALID_STREAM(event->stream);
		#ifdef TCP_DEBUG
		XP_Trace("OTDriver: %X Processing stream event, event %#x, code %#x , \n", event->stream, event, event->event );
		#endif
		event->stream->ProcessStreamEvent(event);
	}
	
	// Process timeouts
	
	SInt32 now = LMGetTicks();
	LListIterator iter(*fTimeoutStreams,iterate_FromStart);
	TimeoutStruct t;
/*	if (iter.Next(&t))
	{
#ifdef DEBUG
	if (t.fTickCount > (LMGetTicks() + 3600))
		Debugger();
#endif
		if (t.fTickCount < now)
			delete t.fStream;
	}
*/	
	while ( iter.Next(&t) && (t.fTickCount < now) )
		delete t.fStream;
}

// Interrupt level routine
void OTDriver::PostStreamEvent(OTStreamEvent * event)
{
	ASSERT_VALID_STREAM(event->stream);
	#ifdef TCP_DEBUG
	XP_Trace("OTDriver: %X Posting stream event, event %#x, code %#x \n", event->stream, event, event->event);
	#endif
	::Enqueue( (QElemPtr)event, &fNotifications);
}


// Starts the StrToAddress call
CDNSObject * OTDriver::StrToAddress(char * hostName)
{
	AssertOpen();
	COTDNSObject * dns = new COTDNSObject;
		
	ThrowIfNil_(dns);
	strncpy(dns->fHostInfo.name, hostName, kMaxHostNameLen);
	
	dns->fStatus = kDNSInProgress;
	
	OSErr err = dns->fRef->StringToAddress(dns->fHostInfo.name, &dns->fHostInfo);
	
	if (err != kOTNoError)
		dns->fStatus = -1;

	return dns;
}

CDNSObject * OTDriver::StrToAddress(char * hostName, MacSocket * socket)
{
	AssertOpen();
	
	COTDNSObject * dns = new COTDNSObject(socket);
	ThrowIfNil_(dns);
	strcpy(dns->fHostInfo.name, hostName);
	
	OSStatus err = dns->fRef->StringToAddress(dns->fHostInfo.name, &dns->fHostInfo);
	if (err != kOTNoError)
		dns->fStatus = -1;
	return dns;
}


// Get name of this host
int	OTDriver::GetHostName(char *name, int namelen)
{
	//	On a Macintosh, we don’t have the concept of a local host name.
	//	We do though have an IP address & everyone should be happy with
	// 	a string version of that for a name.
	//	The alternative here is to ping a local DNS for our name, they
	//	will often know it.  This is the cheap, easiest, and safest way out.

	assert(name);
	AssertOpen();

	// Make sure the string is as long as the longest possible address.
	if (namelen < strlen("123.123.123.123"))
		return EINVAL;

	OSStatus err;
	InetInterfaceInfo info;

	err = OTInetGetInterfaceInfo(&info, 0);
	if (err != kOTNoError)
		return EINVAL;
	
	OTInetHostToString(info.fAddress, name);
	
	return 0;		
}


char * OTDriver::AddressToString(InetHost host)
{
	AssertOpen();
	assert(fHostEnt.h_name);
	
	OTInetHostToString(host, fHostEnt.h_name);

	return fHostEnt.h_name;
}


void OTDriver::SetStreamTimeout(CNetStream * stream, SInt32 ticks)
{
	TimeoutStruct t(stream, LMGetTicks() + ticks);
	fTimeoutStreams->InsertItemsAt(1, fTimeoutStreams->GetCount(), &t);
}

void OTDriver::RemoveStreamTimeout(CNetStream * stream)
{
	LListIterator iter(*fTimeoutStreams,iterate_FromStart);
	TimeoutStruct t;
	while (iter.Next(&t))
		if (t.fStream == stream)
			fTimeoutStreams->Remove(&t);
}

// =========================================================================
// COTDNSObject
// =========================================================================
#pragma mark -
#pragma mark COTDNSObject
#pragma mark -
// DNR completion
// Interrupt level routine. No mallocs, or accessing global variables
static pascal void DNRDone(void* contextPtr, OTEventCode event, OTResult err, void* cookie)
{
	COTDNSObject * dns = (COTDNSObject*)contextPtr;
	switch(event)
	{
		case T_DNRSTRINGTOADDRCOMPLETE:
			{
			if (err == kOTNoError)
				dns->fStatus = kDNSDoneNoErr;
			else
			{
				assert(err < 0);	// Because our dnsDone flag uses < 0 to signify error
				dns->fStatus = err;
			}
			::Enqueue((QElemPtr)&dns->fQLink, &dns->fInterruptDriver->fDoneDNSCalls);
			}
			break;
			
		default:
			assert(FALSE);
	}
}


COTDNSObject::COTDNSObject(MacSocket * socket) : CDNSObject(socket)
{
	InitOTServices();
	::HoldMemoryGlue( this, sizeof( *this ));
}

COTDNSObject::COTDNSObject() : CDNSObject()
{
	InitOTServices();
	::HoldMemoryGlue( this, sizeof( *this ));
}

void COTDNSObject::InitOTServices()
{
	OSStatus err;
// Creates a service reference
	fRef = OTOpenInternetServices(kDefaultInternetServicesPath, NULL, &err);
	if ( (fRef == NULL) ||  (err != kOTNoError) )
		Throw_(err);
// Install notify function
	err = fRef->InstallNotifier((OTNotifyProcPtr)&DNRDone, this);
	ThrowIfOTErr_(err);
// Put us into async mode
	err = fRef->SetAsynchronous();
	ThrowIfOTErr_(err);
}

COTDNSObject::~COTDNSObject()
{
	if (fRef != 0)
	{
		OSStatus err = OTCloseProvider(fRef);
		assert(err ==kOTNoError);
	}
	::UnholdMemoryGlue( this, sizeof( *this ));
}

// =========================================================================
// OTStream
// =========================================================================
#pragma mark -
#pragma mark OTStream
#pragma mark -

// •• constructors

OTStream::OTStream(MacSocket * socket) : CNetStream(socket)
{
	::HoldMemoryGlue( this, sizeof(*this) );
// Zero out the OT structures
	fEndpoint = NULL;
	fGotOrdRel = false;
	fDeleteOnOrdRel = false;
	memset( &fReqBind, 0, sizeof(fReqBind) );
	memset( &fRetBind, 0, sizeof(fRetBind) );
	memset( &fReqBindAddress, 0, sizeof(fReqBindAddress) );
	memset( &fRetBindAddress, 0, sizeof(fRetBindAddress) );
	memset( &fSndCall, 0 , sizeof(fSndCall) );
	memset( &fConnectAddress, 0, sizeof( fConnectAddress ));
	memset( &fConnectNotification, 0, sizeof( fConnectNotification ) );
	memset( &fWriteNotification, 0, sizeof( fWriteNotification ) );
	memset( &fReleaseNotification, 0, sizeof( fReleaseNotification ) );
// Standard values
	fReqBindAddress.fAddressType = AF_INET;
	fRetBindAddress.fAddressType = AF_INET;
	fReqBind.addr.buf = (UInt8*) &fReqBindAddress;
	fRetBind.addr.buf = (UInt8*) &fRetBindAddress;
	fReqBind.addr.maxlen = sizeof(InetAddress);
	fRetBind.addr.maxlen = sizeof(InetAddress);
// 	fRetBind.addr.len = sizeof(InetAddress); *** do not initialize these until you assign an address
// Always assign a simple local address
 	fReqBind.addr.len = sizeof(InetAddress);
	fReqBindAddress.fPort = ((OTDriver*)gNetDriver)->GetNextLocalPort();
	
	fSndCall.addr.maxlen = sizeof(struct InetAddress);
	fSndCall.addr.len = sizeof(struct InetAddress);
	fSndCall.addr.buf = (unsigned char *) &fConnectAddress;
#ifdef DEBUG
	validStreams.InsertItemsAt(1, LDynamicArray::index_Last, this);
#endif
}

OTStream::~OTStream()
{
	#ifdef TCP_DEBUG
	XP_Trace("OTDriver: %X Destructing OTStream\n", this);
	#endif

	::UnholdMemoryGlue( this, sizeof (*this ));

#ifdef DEBUG
	ArrayIndexT a = validStreams.FetchIndexOf(this);
	if (a == LDynamicArray::index_Bad)
		Debugger();
	validStreams.RemoveItemsAt(1, a);
#endif	

	OSStatus err = kOTNoError;

	if (fEndpoint) {
		#ifdef TCP_DEBUG
		OTResult event;
		OTResult state;
		
		XP_Trace("OTDriver: %X About to close endpoint, %#x\n", this, fEndpoint);
		event = fEndpoint->Look();
		state = fEndpoint->GetEndpointState();
		XP_Trace("OTDriver: %X Closing endpoint, %#x.  Pending event = %#x, Endpoint state = %#x\n", this, fEndpoint, event, state);
		#endif

		err = OTCloseProvider(fEndpoint);
		
		#ifdef TCP_DEBUG
		XP_Trace("OTDriver: %X Successfully closed endpoint\n", this);
		#endif
		
		fEndpoint = NULL;
	}
	
	assert(err == kOTNoError);
	
	((OTDriver*)gNetDriver)->RemoveStreamTimeout(this);
	
	#ifdef TCP_DEBUG
	XP_Trace("OTDriver: Successfully destructed OTStream\n");
	#endif
}

// Async notifier routine. No allocations, or callbacks
pascal void  NotifierRoutine(void * contextPtr, OTEventCode code, OTResult result, void * cookie);
pascal void  NotifierRoutine(void * contextPtr, OTEventCode code, OTResult result, void * cookie)
{
	OTStream * stream = (OTStream *) contextPtr;
	
	#ifdef TCP_DEBUG
	XP_Trace("OTDriver: %X Notified stream code = %#x, result = %d, cookie = %#x\n", stream, code, result, cookie);
	#endif
	
	stream->Notify(code, result, cookie);
}


int OTStream::CreateStream()
{
	#ifdef TCP_DEBUG
	XP_Trace("OTDriver: %X CreateStream.  Type = %d\n", this, fSocket->fType);
	#endif
	Try_
	{
		OSStatus err;
		switch (fSocket->fType){
			case SOCK_STREAM:
				fEndpoint = OTOpenEndpoint(OTCreateConfiguration(kTCPName), 0, &fInfo, &err);
				break;
			case SOCK_DGRAM:
				fEndpoint = OTOpenEndpoint(OTCreateConfiguration(kUDPName), 0, &fInfo, &err);
				break;
		}
		ThrowIfNil_(fEndpoint);
		ThrowIfOTErr_(err);
		err = fEndpoint->InstallNotifier(NotifierRoutine, this);
		ThrowIfOTErr_(err);
		err = fEndpoint->SetAsynchronous();
		ThrowIfOTErr_(err);
		if (fSocket->fType == SOCK_DGRAM)
		{
			err = fEndpoint->AckSends();
			ThrowIfOTErr_(err);
		}
	}
	Catch_(inErr)
	{
		return ENETDOWN;
	}
	EndCatch_
	return 0;
}

// Call to close the socket
// If we are not aborting, call the normal close
/*
Tricky stuff, here are the possibilities
#### Orderly:

Initiated by Mozilla

SndOrderlyDisconnect
Wait for T_ORDREL
RcvOrderlyDisconnect
delete

Initialted by Connected host:

Gets T_ORDREL
Read data
RcvOrderlyDisconnect
SndOrderlyDisconnect
delete

#### Abortive:
Initiated by Mozilla

SndDisconnect
T_DISCONNECTCOMPLETE
delete

Initiated by Connected host:
T_DISCONNECT
RcvDisconnect
delete
*/
int OTStream::DestroyStream(Boolean abort)
{
	OSStatus err;
	OTResult event = fEndpoint->Look();

	if ( (event == T_ORDREL) || fGotOrdRel)	// The other end has issued a close
	{
		#ifdef TCP_DEBUG
		XP_Trace("OTDriver: %X Other end issued a close, event is %d\n", this, event);
		#endif
		fDoNotDelete = true;
		err = fEndpoint->RcvOrderlyDisconnect();
		err = fEndpoint->SndOrderlyDisconnect();
		gNetDriver->SpendTime();	// Clean up all the queued events
		delete this;
	}
	else	// We should close
	{
		OTResult state = fEndpoint->GetEndpointState();
		if (state == T_IDLE)	// Endpoint has already closed
			delete this;
		else
		{
			fDoNotDelete = true;
			fNeedDeletion = false;
			fDeleteOnOrdRel = true;
			err = fEndpoint->SndOrderlyDisconnect();	
			gNetDriver->SpendTime();	// Clean up all the queued events
			if ( err != kOTNoError || fNeedDeletion)
				delete this;
			else
			{
		// else SndOrderlyDisconnect will complete asynchronously, we'll get T_ORDREL and we'll get deleted
				fDoNotDelete = false;
				((OTDriver*)gNetDriver)->SetStreamTimeout(this, 3600);	// If no response in 60 seconds, delete us
			}
		}
	}
	return 0;
}

// •• completion routines

// Notification routine
// Async callback routine.
// A5 is OK. Cannot allocate memory here
void OTStream::Notify(OTEventCode code, OTResult result, void * cookie)
{
/* Possible results for all calls
kOTBadSyncErr
kENOMEMErr
kENOSRErr
kEAGAINErr    --- non-blocking, and we'd have to block
kOTProtocolErr
kEBADFErr
kOTClient
kOTClientNotInittedErr
kOTStateChangeErr -- cannot issue the call
*/
	switch (code)
	{
// Bind callback
		case T_BINDCOMPLETE:
			#ifdef TCP_DEBUG
			XP_Trace("OTDriver: %X Notified of T_BINDCOMPLETE\n", this);
			#endif
			assert(FALSE);
			break;
// Connect callbacks
		case T_CONNECT:
			{
				#ifdef TCP_DEBUG
				XP_Trace("OTDriver: %X Notified of T_CONNECT\n", this);
				#endif
				assert (result == kOTNoError);
			// cookie is a pointer to the fSndCall member of OTStream
				fConnectNotification.event = code;
				fConnectNotification.stream = this;
				if ( result != kOTNoError )	// If we got an error, pretend that we've received disconnect
					fConnectNotification.event = T_DISCONNECT;
				( (OTDriver*) gNetDriver )->PostStreamEvent( &fConnectNotification );
			}
			break;
// Listen callback
		case T_LISTEN:
			#ifdef TCP_DEBUG
			XP_Trace("OTDriver: %X Notified of T_LISTEN\n", this);
			#endif
			if (fSocket)
				fSocket->ConnectComplete(TRUE, OTErrorToUnix(eListen, result));
			break;
		case T_ACCEPTCOMPLETE:
			#ifdef TCP_DEBUG
			XP_Trace("OTDriver: %X Notified of T_ACCEPTCOMPLETE\n", this);
			#endif
			if (fSocket)
				fSocket->AcceptComplete(OTErrorToUnix(eAccept, result));
			break;
// Write callback
		case T_MEMORYRELEASED:
			#ifdef TCP_DEBUG
			XP_Trace("OTDriver: %X Notified of T_MEMORYRELEASED\n", this);
			#endif
			assert(cookie);
			if (cookie) {
				size_t 			diff =  offsetof(WriteBuffer, data);
				WriteBuffer * 	wb = (WriteBuffer *)((Ptr)cookie - diff);
				wb->streamEvent.event = T_MEMORYRELEASED;
				wb->streamEvent.result = result;
				( (OTDriver*)gNetDriver )->PostStreamEvent( &wb->streamEvent );
			}
			break;
// Disconnect callbacks
		case T_ORDREL:
		case T_DISCONNECTCOMPLETE:
		case T_DISCONNECT:
				#ifdef TCP_DEBUG
				XP_Trace("OTDriver: %X Notified of T_ORDREL, T_DISCONNECTCOMPLETE, or T_DISCONNECT\n", this);
				#endif
				if ( fSocket )
					fSocket->CloseNotify();
				fReleaseNotification.event = code;
				fReleaseNotification.stream = this;
				( (OTDriver*) gNetDriver )->PostStreamEvent( &fReleaseNotification );
			break;
// UDP OK to send
		case T_GODATA:
			#ifdef TCP_DEBUG
			XP_Trace("OTDriver: %X Notified of T_GODATA\n", this);
			#endif
			if (fSocket->fType == SOCK_DGRAM)
			{
				assert (fEndpoint);
				assert (cookie);
				if (fEndpoint && cookie) {
					OSStatus	status;
					status = fEndpoint->SndUData((TUnitData *)cookie);
					if (noErr == status)						// Release the buffer
						::UnholdMemoryGlue((TUnitData *)cookie, sizeof(TUnitData) + ((TUnitData *)cookie)->udata.len);
				}
			}
			break;

// UDP Send error
		case T_UDERR:
			#ifdef TCP_DEBUG
			XP_Trace("OTDriver: %X Notified of T_UDERR\n", this);
			#endif
			assert (cookie);
			if (cookie) {
			}
			break;

		default:
			#ifdef TCP_DEBUG
			XP_Trace("OTDriver: %X Unhandled notification.  Code = %#x\n", this, code);
			#endif
			break;
	}
	// Notify the NSPR threads mechanism that an I/O interrupt has occured.
	_PR_AsyncIOInterruptHandler();
}

void OTStream::ProcessStreamEvent( OTStreamEvent * event)
{
	#ifdef TCP_DEBUG
	XP_Trace("OTDriver: %X ProcessStreamEvent.  event = %#x, event->event = %#x\n", this, event, event->event);
	#endif

	switch (event->event)	{
		case T_CONNECT:
			fEndpoint->RcvConnect(NULL);
			if (fSocket)
				fSocket->ConnectComplete(FALSE, 0);
			break;
		case T_MEMORYRELEASED: {
			size_t			attemptedBytesSent;
			size_t			bufferSize;
			WriteBuffer	*	wb = (WriteBuffer *)event;
					
			// How much did we try to send?
			if (fInfo.servtype == T_CLTS)
				attemptedBytesSent = wb->protocolSpecific.udpSendInfo.udata.len;
			else 
				attemptedBytesSent = wb->protocolSpecific.dataLen;
			
			// Keep track of the fact that we’re done with this write.
			if (attemptedBytesSent == event->result)
				fSocket->WriteComplete(0);
			else {
				assert(FALSE);
				fSocket->WriteComplete(EIO);
			}
			
			#ifdef TCP_DEBUG
			XP_Trace("OTDriver: %X Handling T_MEMORYRELEASED.  Freeing buffer %#x\n", this, wb);
			#endif
			
			::UnholdMemoryGlue(wb, sizeof(WriteBuffer) + attemptedBytesSent);
			
			free(wb);
			
			break;
		}
		case T_ORDREL:
			fGotOrdRel = true;
			if ( fDoNotDelete ) {
				fNeedDeletion = true;
				#ifdef TCP_DEBUG
				XP_Trace("OTDriver: %X Handling T_ORDREL.  fNeedDeletion but fDoNotDelete.\n", this);
				#endif
			}
			else if (fDeleteOnOrdRel)
			{
				OSStatus err = fEndpoint->RcvOrderlyDisconnect();
				#ifdef TCP_DEBUG
				XP_Trace("OTDriver: %X Handling T_ORDREL, doing RcvOrderlyDisconnect\n", this);
				#endif
				assert( err == kOTNoError );
				delete this;
			}
			break;
		case T_DISCONNECTCOMPLETE:
			if ( fDoNotDelete ) {
				fNeedDeletion = true;
				#ifdef TCP_DEBUG
				XP_Trace("OTDriver: %X Handling T_DISCONNECTCOMPLETE.  fNeedDeletion but fDoNotDelete.\n", this);
				#endif
			}
			else {
				#ifdef TCP_DEBUG
				XP_Trace("OTDriver: %X Handling T_DISCONNECTCOMPLETE.  deleting this.\n", this);
				#endif
				delete this;
			}
			break;
		case T_DISCONNECT:
			fEndpoint->RcvDisconnect(NULL);
			if (fSocket)
				fSocket->ConnectComplete(FALSE, -1);
			if (fDoNotDelete)
				fNeedDeletion = true;
			else
				delete this;
			break;
		default:
			assert(FALSE);
	}
}

// Giant lookup table of OpenTransport error codes, to socket ones
int OTStream::OTErrorToUnix(OTCallsEnum callKind, int error)
{
	switch (callKind)	{
		case eConnect:
		case eBind:
		case eListen:
		case eAccept:
			return error;
			break;
		case eRead:
			switch (error)	{
			case kOTNoDataErr:
				return EWOULDBLOCK;
			case kOTLookErr:	// T_DISCONNECT, T_ORDREL
			{
				OTResult result = fEndpoint->Look();
				if (result == T_ORDREL)
					return 0;
				else
					return EIO;
			}
			default:
				return error;
			}
			break;
		case eWrite:
			if (error == kOTFlowErr)
				return EWOULDBLOCK;
			else
				return error;
			break;
	}
	return error;	// For now
}


// Connect to a given host, port
int OTStream::ActiveConnect(InetHost host, InetPort port)
{
#ifdef TCP_DEBUG
XP_Trace("OTDriver: %X Active Connect \n", this);
#endif
	OSStatus err;

	// Bind to a local port; let the system assign it.

	fEndpoint->SetSynchronous();
	err = fEndpoint->Bind(&fReqBind, &fRetBind);
	fEndpoint->SetAsynchronous();
	if (err != kOTNoError)
		return OTErrorToUnix( eConnect, err );

	// Open the connection
	OTInitInetAddress(&fConnectAddress, port, host);

	// The call will complete by calling Notify loop with 
	// T_CONNECT event (on success),
	// T_DISCONNECT event (on failure)
	
	err = fEndpoint->Connect(&fSndCall, NULL);
	
	// Unless Connect returns kOTNoDataErr, notification routine will not 
	// be called
	if (err == kOTNoDataErr)	// Actually, not an error, this is the only correct return
		return EINPROGRESS;
	else
	{
		assert(FALSE);
		return OTErrorToUnix( eConnect, err );
	}
}


//	Bind to a particular port.
int	OTStream::Bind(InetHost host, InetPort port)
{
	OSStatus		err;
	

#ifdef TCP_DEBUG
	XP_Trace("OTDriver: %X Request bind to host %#x, port %d\n", this, host, port);
#endif

	// setup our request
	if ((0 == port) && (host == 0))				// Have the driver assign the port.
		fReqBind.addr.len = 0;
	else {
		fReqBindAddress.fPort = port;			// We may or may not get this port.
		fReqBindAddress.fHost = host;		
	}
	
	fReqBind.qlen = 0;
	
	// Bind to local port synchronously, should be instantaneous.
	fEndpoint->SetSynchronous();
	err = fEndpoint->Bind(&fReqBind, &fRetBind);
	fEndpoint->SetAsynchronous();
	
#ifdef TCP_DEBUG
	XP_Trace("OTDriver: %X Bound to host %d, port %d\n", this, fRetBindAddress.fHost, fRetBindAddress.fPort);
#endif

	return OTErrorToUnix( eBind, err );
}


// This will get called only once 
int OTStream::Listen(int backlog)
{
#ifdef TCP_DEBUG
XP_Trace("OTDriver: %X Listen \n", this);
#endif
	OSStatus err;
	if (backlog == 0)
	{
		assert(FALSE);
		backlog = 1;
	}

	fReqBind.qlen = backlog;

	fEndpoint->SetSynchronous();
	err = fEndpoint->Bind(&fReqBind, &fRetBind);
	fEndpoint->SetAsynchronous();
	
	return OTErrorToUnix(eBind, err);
}


// Call this only once per stream
int OTStream::Accept()
{
#ifdef TCP_DEBUG
XP_Trace("OTDriver: %X Accept \n", this);
#endif
	OSStatus err = fEndpoint->Listen( &fSndCall );
	if (err != kOTNoError)
		return OTErrorToUnix(eListen, err);
	err = fEndpoint->Accept(fEndpoint, &fSndCall);
	if (err != kOTNoDataErr)
		return OTErrorToUnix(eAccept, err);
	return EWOULDBLOCK;
}


// Only one write call at a time is allowed
// bytesWritten is always set to whatever we try to write, but it is only valid if
// we return no error
int OTStream::Write(const void *buffer, unsigned int buflen, UInt16& bytesWritten)
{
	#ifdef TCP_DEBUG
	XP_Trace("OTDriver: %X TCP Write.  About to write %d bytes from buffer at %#x\n",this, buflen, buffer);
	DumpBuffer(buffer, buflen);
	#endif
	
/*	WriteBuffer * newBuffer = (WriteBuffer*)malloc(sizeof(WriteBuffer) + buflen);
	if ( newBuffer == NULL )
		return EWOULDBLOCK;
		
	::HoldMemory( newBuffer, sizeof(WriteBuffer) + buflen );	// For async completion
	newBuffer->streamEvent.stream = this;
	newBuffer->protocolSpecific.dataLen = buflen;
	::BlockMoveData(buffer, &newBuffer->data, buflen);
*/	OSStatus status = fEndpoint->Snd(buffer, buflen, 0);
	if (status < 0)
	{
		fSocket->WriteComplete(OTErrorToUnix(eWrite, status));
		return OTErrorToUnix(eWrite, status);
	}
	else
	{
		fSocket->WriteComplete(0);
		bytesWritten = status;
	}
#ifdef TCP_DEBUG
XP_Trace("OTDriver: Wrote %d bytes\n", bytesWritten);
#endif
	return 0;
}

// Does a true non-blocking read
int OTStream::Read(void *buffer, unsigned int buflen, UInt16& bytesRead)
{
	OTResult result;
	OTFlags flags = 0;
	bytesRead = 0;
	
	if (fSocket == NULL)
		return -1;
		
	// We need to loop if we have T_MORE flag set to optimize the reads
	// T_MORE means that there is more data available immediately
	// 
loop:
	result = fEndpoint->Rcv( buffer, buflen, &flags );
	if ( result < 0 )	
	{
	// Error
		if ( bytesRead > 0 )	
		// If we have already read something, and error is not a true error
		// but a signal of socket closing, return no error
		{
			if ( ( result == kOTNoDataErr ) ||	// If we are about to block
				( ( result == kOTLookErr) && ( fEndpoint->Look() == T_ORDREL )))	// Or we've received a disconnect
				{
					#ifdef TCP_DEBUG
						XP_Trace("OTDriver: %X read %d bytes\n", this, bytesRead);
					#endif
				return 0;
				}
			else
			{
				#ifdef TCP_DEBUG
					XP_Trace("OTDriver: %X Read got an error %d after reading %d bytes \n", this, result, bytesRead);
				#endif
				return OTErrorToUnix(eRead, result);
			}
		}
		else
		// Nothing read, and we have an error
		{
			#ifdef TCP_DEBUG
				XP_Trace("OTDriver: %X Read got an error %d after reading %d bytes\n", this, result, bytesRead);
			#endif
			return OTErrorToUnix( eRead, result );
		}
	}
	else
	{
		#ifdef TCP_DEBUG
			XP_Trace("OTDriver: %X TCP Read.  Successfully read %d bytes\n", this, result);
			DumpBuffer(buffer, result);
		#endif
		bytesRead += result;
		buffer = (void *) ( (UInt32) buffer + (UInt32)result );
		buflen -= result;
		if ( (flags & T_MORE) && ( buflen > 128 ))	// If we have more outstanding buffers, read again
			goto loop;
	}
	return 0;
}

Boolean OTStream::Select(Boolean& readReady, Boolean& writeReady, Boolean& exceptReady)
{
	readReady = writeReady = false;
	exceptReady = fGotOrdRel;
	
	#ifdef TCP_DEBUG
		XP_Trace("OTDriver: %X Starting Select\n", this);
	#endif

	OTResult result = fEndpoint->Look();
	switch (result)	{
		case T_DATA:
			readReady = TRUE;
			break;
		case T_CONNECT:
		case T_DISCONNECT:
		case T_LISTEN:
		case T_ORDREL:
			exceptReady = TRUE;
			break;
	}
	switch (fEndpoint->GetEndpointState())	{
		case T_DATAXFER:
		case T_INREL:
			writeReady = TRUE;
			break;
		default:
			writeReady = FALSE;
	}
	return (readReady || writeReady || exceptReady);
}

int OTStream::GetPeerName(InetHost& host, InetPort& port)
{
	TBind remoteBind;
	InetAddress remoteAddress;
	OSStatus err;
	memset(&remoteBind, 0, sizeof (remoteBind) );
	memset(&remoteAddress, 0, sizeof (remoteAddress) );
	remoteBind.addr.buf = ( UInt8* ) &remoteAddress;
	remoteBind.addr.maxlen = sizeof(remoteAddress);
	Try_
	{
		ThrowIfNil_(fEndpoint);
		err = fEndpoint->SetSynchronous();
		ThrowIfOTErr_(err);
		err = fEndpoint->GetProtAddress(NULL, &remoteBind);
		ThrowIfOTErr_(err);
		err = fEndpoint->SetAsynchronous();
		ThrowIfOTErr_(err);
		host = remoteAddress.fHost;
		port = remoteAddress.fPort;
	}
	Catch_(inErr)
	{
		return ENOTCONN;
	}
	EndCatch_
	return 0;
}

// Mirrors GetSockName
int OTStream::GetSockName(InetHost& host, InetPort& port)
{
	TBind bind;
	InetAddress address;
	OSStatus err;
	memset(&bind, 0, sizeof (bind) );
	memset(&address, 0, sizeof (address) );
	bind.addr.buf = ( UInt8* ) &address;
	bind.addr.maxlen = sizeof(address);
	Try_
	{
		ThrowIfNil_(fEndpoint);
		err = fEndpoint->SetSynchronous();
		ThrowIfOTErr_(err);
		err = fEndpoint->GetProtAddress(&bind, NULL);
		ThrowIfOTErr_(err);
		err = fEndpoint->SetAsynchronous();
		ThrowIfOTErr_(err);
		host = address.fHost;
		port = address.fPort;
	}
	Catch_(inErr)
	{
		return ENOTCONN;
	}
	EndCatch_
	return 0;
}


// Send data to a UDP target
int OTStream::UDPSendTo(const void *msg, unsigned int msgLen, unsigned int flags, 
						InetHost host, InetPort port, unsigned int& bytesWritten)
{
	OSStatus		status;
	size_t			bufferLen = offsetof(WriteBuffer, data) + msgLen;		// Data is appended to TUnitData structure
	WriteBuffer		*udpSendTgt = (WriteBuffer *)malloc(bufferLen);
	
	#ifdef TCP_DEBUG
		XP_Trace("OTDriver: %X UDP SendTo, msgLen = %d, host = %#x, port = %d\n", this, msgLen, host, port);
		DumpBuffer(msg, msgLen);
	#endif

	if (NULL == udpSendTgt)
		FailOSErr_(OTErrorToUnix(eWrite, kOTOutOfMemoryErr));
		
	#ifdef TCP_DEBUG
		XP_Trace("OTDriver: %X Using write buffer %#x\n", this, udpSendTgt);
	#endif

	// Set up the stream event structure for completion-time use.
	udpSendTgt->streamEvent.stream = this;
	
	// Set up the send buffer
	OTInitInetAddress(&fConnectAddress, port, host);
	udpSendTgt->protocolSpecific.udpSendInfo.addr.buf = (UInt8 *)&fConnectAddress;
	udpSendTgt->protocolSpecific.udpSendInfo.addr.len = sizeof(fConnectAddress);
	udpSendTgt->protocolSpecific.udpSendInfo.opt.len = 0;							// No options
	udpSendTgt->protocolSpecific.udpSendInfo.udata.buf = &udpSendTgt->data[0];
	udpSendTgt->protocolSpecific.udpSendInfo.udata.len = msgLen;
	::BlockMoveData(msg, &(udpSendTgt->data), msgLen);				// Make a safe copy of the data
	
	::HoldMemoryGlue(udpSendTgt, bufferLen);							// Because we’ll complete asynch
	
	status = fEndpoint->SndUData(&udpSendTgt->protocolSpecific.udpSendInfo);

	#ifdef TCP_DEBUG
		XP_Trace("OTDriver: %X UDPSendTo complete.  Status = %d\n", this, status);
	#endif
	
	if (kOTFlowErr == status)		// We need to wait for the port to free -- we’ll be notified
		return noErr;
		
	return OTErrorToUnix(eWrite, status);
}


// Receive data from a UDP socket.
int OTStream::UDPReceiveFrom(void *buffer, unsigned int len, unsigned int flags, 
							struct sockaddr *from, unsigned int& fromLen)
{
	OSStatus		err;
	TUnitData		rcvData;
	Boolean			peekOnly;
	OTBuffer*		readOnlyBuffer = NULL;
	OTFlags			rcvFlags;
	OTBufferInfo	bufferInfo;
	
	#ifdef TCP_DEBUG
		XP_Trace("OTDriver: %X Starting UDPReceiveFrom, buffer length = %d\n", this, len);
	#endif

	// This is somewhat bogus.  We should support the MSG_PEEK flag; but OT doesn’t
	// seem to support true peeking.  Since all we really usually want is the address
	// of the sender; we can usually get away with just doing reads of size 0.
	#ifdef DEBUG
		assert (!((flags & MSG_PEEK) && (len > 0)));
	#endif
		if ((flags & MSG_PEEK) && (len > 0))
			return -1;
	
	// MUST have legit buffers.
	ThrowIfNil_(buffer);
	ThrowIfNil_(from);
	ThrowIf_(fromLen < sizeof(sockaddr_in));
	
	if (NULL == fEndpoint)							// Make sure there’s somebody to talk to.
		FailOSErr_(ENOTCONN);
		
	// Set up the OT structure which will keep track of what we are receiving
	rcvData.addr.maxlen = sizeof(fConnectAddress);
	rcvData.addr.buf = (UInt8 *)&fConnectAddress;
	rcvData.opt.maxlen = 0;							// We don't care about the options
	rcvData.opt.buf = NULL;

	rcvData.udata.buf = (UInt8 *)buffer;
	rcvData.udata.maxlen = len;
			
loop:
	err = fEndpoint->RcvUData(&rcvData, &rcvFlags);
	
	if (kOTLookErr == err) {						// Somebody probably wrote to us on a non-existent port.
		TUDErr		uderr;
		InetAddress	errAddress;
		
		uderr.addr.maxlen = sizeof(errAddress);
		uderr.addr.buf = (UInt8 *)&errAddress;
		uderr.opt.len = 0;							// We really don’t care
		
		OTResult event = fEndpoint->Look();
		assert (T_UDERR == event);					// What else could it be?
		err = fEndpoint->RcvUDErr(&uderr);			// Just shut up.
		#ifdef TCP_DEBUG
			XP_Trace("OTDriver: %X Error %d during read.  From host %#x, port %d.\n", this, err, errAddress.fHost, errAddress.fPort);
		#endif
		goto loop;									// Ignore them
	}
	
	if (kOTNoDataErr == err)						// Return, the sockets level will deal with blocking issues.
		return OTErrorToUnix(eRead, err);

	if (err < noErr) {								// A true error occured.
		#ifdef TCP_DEBUG
			XP_Trace("OTDriver: %X UDPReceiveFrom, finished, err = %d, flags = %#x\n", this, err, rcvFlags);
		#endif
		return OTErrorToUnix(eRead, err);
	}
	
	// Error must == noErr; pick out the address
	sockaddr_in * socketAddress = (sockaddr_in *)from;
	from->sa_len = sizeof(sockaddr_in);
	from->sa_family = fConnectAddress.fAddressType;
	socketAddress->sin_port = fConnectAddress.fPort;
	socketAddress->sin_addr.s_addr = fConnectAddress.fHost;
	
	#ifdef TCP_DEBUG
		XP_Trace("OTDriver: %X UDP ReceiveFrom, finished, %d bytes received, err = %d, flags = %#x\n", this, rcvData.udata.len, err, rcvFlags);
		XP_Trace("OTDriver: %X UDP ReceiveFrom, finished, read from Host %#x, port %d\n", this, fConnectAddress.fHost, fConnectAddress.fPort);
		DumpBuffer(buffer, rcvData.udata.len);
	#endif

	return rcvData.udata.len;
}


// Set a specific option.
// We only implement the minimal subset needed by the Navigator
int OTStream::SetSocketOption(int level, int optname, const void *optval)
{
	TOptMgmt	optionRequest, optionResult;
	TOption		requestDetail, resultDetail;
	UInt32		value;
	OSErr		err;
	
	
	#ifdef TCP_DEBUG
		XP_Trace("OTDriver: %X Setting socket option.  Level = %#x, optname = %d, optval = %s\n", this, level, optname, optval);
	#endif

	// Translate the option value from a string to a constant
	assert(optval != NULL);
	switch (optname) {
		case IP_REUSEADDR:	if (strcmp((char *)optval, "yes") == 0)
								value = T_YES;
							else
								value = T_NO;
							break;
	}
	
	// Setup the request
	requestDetail.len = sizeof(requestDetail);
	requestDetail.level = level;
	requestDetail.name = optname;
	requestDetail.value[0] = value;

	optionRequest.opt.len = requestDetail.len;
	optionRequest.opt.buf = (UInt8 *)&requestDetail;
	optionRequest.flags = T_NEGOTIATE;
	
	// Setup for the result
	resultDetail.len = sizeof(resultDetail);
	
	optionResult.opt.buf = (UInt8 *)&resultDetail;
	optionResult.opt.maxlen = sizeof(resultDetail);

	// Ask OT, “pretty please…”
	Try_
	{
		ThrowIfNil_(fEndpoint);
		err = fEndpoint->OptionManagement(&optionRequest, &optionResult);
	}
	Catch_(inErr)
	{
		return ENOTCONN;
	}
	EndCatch_
	
	return err;
}


int OTStream::BytesAvailable(size_t& bytesAvailable)
{
	OTResult	result;
	
	// Is this totally correct?  Should we check 1st to see if there’s pending
	// data to be read?
	result = fEndpoint->CountDataBytes(&bytesAvailable);

	#ifdef TCP_DEBUG
		XP_Trace("OTDriver: %X BytesAvailable, finished result = %d bytes available = %d.\n", this, result, bytesAvailable);
	#endif	
	
	if ((result == kOTLookErr) || 		// Not really errors, we just need to do a read,
	   (result == kOTNoDataErr))		// or there’s nothing there.
		result = kOTNoError;
		
	return result;
}


#ifdef PROFILE
#pragma profile off
#endif
