// Mac utils
#include "MoreMixedMode.h"

// MacOS
#include <Dialogs.h>
#include <Files.h>
#include <Devices.h>

// stdclib
#include <stdlib.h>

// Network stuff
#include <OpenTransport.h>
#include <MacTCP.h>
#include "mactcpdriver.h"

// NSPR stuff
#include "swkern.h"

// For HoldMemoryGlue and UnholdMemoryGlue
#include "macutil.h"

#ifdef PROFILE
#pragma profile on
#endif

#ifdef assert
#undef assert
#endif

#ifdef DEBUG
#define assert(x) if (!(x)) abort()
#else
#define assert(x) ((void)0)
#endif

//#define TCP_DEBUG

#ifdef TCP_DEBUG
#ifndef DEBUG
#error "You can only define TCP_DEBUG in MacTCP builds"
#endif
#endif

#pragma mark AnnotatedPB

// Utility struct for PBs that we just need within the routine
struct AcquirePB	{
	AnnotatedPB * fApb;
	AcquirePB(int streamType, int pbKind, MacTCPStream * stream)	
	{
		fApb = ((MacTCPDriver*)gNetDriver)->GetPB(streamType, pbKind, stream);
	}
	~AcquirePB() { ((MacTCPDriver*)gNetDriver)->ReturnPB(fApb);}; 
};



#pragma mark -
#pragma mark Cwds
#pragma mark -

// =========================================================================
// Cwds
// =========================================================================

// Write data structure for MacTCP
struct Cwds {
	UInt16	fSize;
	void *	fData;
	UInt16	fZero;
	Cwds(UInt16 length, const void * dataCopy);
	~Cwds();
};

// Makes a copy of the data to be written
Cwds::Cwds(UInt16 length, const void * dataCopy)
{
	fSize = length;
	fZero = 0;
	fData = malloc(fSize);
	ThrowIfNil_(fData);
	::BlockMoveData(dataCopy, fData, fSize);
	::HoldMemoryGlue( fData, fSize );
	::HoldMemoryGlue( this, sizeof (*this ));
}
	
Cwds::~Cwds()
{
	if (fData != NULL)
	{
		free(fData);
		::UnholdMemoryGlue( fData, fSize );
		::UnholdMemoryGlue( this, sizeof (*this ));
	}
}




#pragma mark -
#pragma mark MacTCPDriver
#pragma mark -

// =========================================================================
// MacTCPDriver
// =========================================================================

// Reset all the variables. Does not open the driver
MacTCPDriver::MacTCPDriver() : CNetworkDriver()
{
	fDriverNum = 0;
	fOutstanding.qHead = fOutstanding.qTail = NULL;
	fReceived.qHead = fReceived.qTail = NULL;
	fWaitDNSCalls.qHead = fWaitDNSCalls.qTail = NULL;
	::HoldMemoryGlue( this, sizeof(*this) );
}

extern void WaitToQuit();

// Make sure that all async calls have been completed
MacTCPDriver::~MacTCPDriver()
{
	// Delete all the sockets	
	for (int i=0; i<MAX_SOCKETS; i++)
		if (MacSocket::sSockets[i] != NULL)
			MacSocket::sSockets[i]->Destroy(TRUE);

	if ((fOutstanding.qHead == nil) &&
		 (fReceived.qHead == nil) &&
		 (fOutstanding.qHead == nil))
		 return;

	// Put up a dialog here
	DialogPtr alert = ::GetNewDialog( 1042, NULL, (WindowPtr)-1 );
	if (alert)
		DrawDialog(alert);

	// Spin wait for all async calls to complete
	while ((fOutstanding.qHead != nil) ||
		 (fReceived.qHead != nil) ||
		 (fOutstanding.qHead != nil))
	{
			Spin();
			WaitToQuit();
	}
	CloseResolver();
	if (alert)
		DisposeDialog(alert);	
	// Kill a dialog here
	::UnholdMemoryGlue( this, sizeof(this));
}

CNetStream * MacTCPDriver::CreateStream(MacSocket * socket)
{
	return new MacTCPStream(socket);
}

// Receive buffer size for streams. How to calculate is unclear (MTU, or big hardcoded value
// Currently, uses 16K
UInt16 MacTCPDriver::GetStreamBufferSize()
{
	return DEFAULT_BUFFER_SIZE;
}

// Opens the driver. Can fail.
OSErr MacTCPDriver::DoOpenDriver()
{
	ParamBlockRec 		pb; 
	pb.ioParam.ioCompletion	= 0L; 
	pb.ioParam.ioNamePtr 	= "\p.IPP"; 
	pb.ioParam.ioPermssn 	= fsCurPerm;
	
	OSErr err = PBOpenSync(&pb);
	if (err == noErr)
	{
		fDriverNum = pb.ioParam.ioRefNum;
		OpenResolver(NULL);
	}
	fDriverOpen = TRUE;
	return err;
}

// Returns annotatedPB
AnnotatedPB * MacTCPDriver::GetPB(int streamType, int pbKind, MacTCPStream * stream)
{
	AnnotatedPB * apb = NULL;
	if (fPBs.GetCount() == 0)	// No cached PBs, use the build in ones
	{
		try
		{
			apb = new AnnotatedPB;
		}
		catch(OSErr err)
		{
			return NULL;
		}
	}
	else
	{
		fPBs.FetchItemAt(fPBs.GetCount(), &apb);
		fPBs.RemoveItemsAt(1, fPBs.GetCount());
	}
	memset(apb, 0, sizeof(AnnotatedPB));
	apb->streamType = streamType;
	apb->stream = stream;
	apb->driver = (MacTCPDriver*)gNetDriver;
	// Set the standard fields - kind of tcp call, tcp stream, and driver num
	if (streamType == SOCK_STREAM) {
		apb->pb.tcpPB.csCode = pbKind;
		apb->pb.tcpPB.ioCRefNum = fDriverNum;
		apb->pb.tcpPB.tcpStream = stream->fStream;
	}
	else {
		apb->pb.udpPB.csCode = pbKind;
		apb->pb.udpPB.ioCRefNum = fDriverNum;
		apb->pb.udpPB.udpStream = stream->fStream;
	}
	return apb;
}

void MacTCPDriver::ReturnPB(AnnotatedPB * apb)
{
	if (apb)
		fPBs.InsertItemsAt(1, fPBs.GetCount(), &apb);
}

#pragma mark -
#pragma mark Async Callbacks
#pragma mark -
// Callback for async calls. Passes it on to the driver
// Interrupt level routine! No globals or mallocs
void TCPAsyncCallDone(TCPiopb *PB);
void TCPAsyncCallDone(TCPiopb *PB)
{
	short diff = offsetof(AnnotatedPB, pb);
	AnnotatedPB * apb = (AnnotatedPB *)((UInt32)PB - (UInt32)diff);
	apb->driver->CompleteAsyncCall(apb);
}
PROCEDURE(TCPAsyncCallDone, uppTCPIOCompletionProcInfo)


void UDPAsyncCallDone(UDPiopb *pb);
void UDPAsyncCallDone(UDPiopb *pb)
{
}
PROCEDURE(UDPAsyncCallDone, uppUDPIOCompletionProcInfo)

// Places the call on the queue of incomplete calls.
// AsyncCallDone removes it, and puts it on the queue of calls to be processed
// We can have only one async call per stream at one time.
// This restriction might be removed later
void MacTCPDriver::MakeAsyncCall(AnnotatedPB * apb)
{
	if (apb->stream->fLastCallInProgress == TRUE)
		assert(FALSE);

#ifdef DEBUG
	if (apb->streamType == SOCK_STREAM)
		apb->stream->fLastAsyncCall = apb->pb.tcpPB.csCode;
	else
		apb->stream->fLastAsyncCall = apb->pb.udpPB.csCode;
#endif

	apb->stream->fLastCallInProgress = TRUE;
	apb->stream->fLastCallResult = -1;
	// Make sure we have no other outstanding calls
	::Enqueue((QElemPtr)apb, &fOutstanding);
	if (apb->streamType == SOCK_STREAM)
		apb->pb.tcpPB.ioCompletion = &PROCPTR(TCPAsyncCallDone);
	else
		apb->pb.udpPB.ioCompletion = &PROCPTR(UDPAsyncCallDone);
		
	HoldMemoryGlue( apb, sizeof(*apb));	// Unhold is CompleteAsyncCall
	OSErr err = PBControlAsync((ParmBlkPtr)&(apb->pb));
	if (err != noErr)
	{
		if (apb->streamType == SOCK_STREAM)
			apb->pb.tcpPB.ioResult = driverError;
		else
			apb->pb.udpPB.ioResult = driverError;
		CompleteAsyncCall(apb);
		SpendTime();
		assert(FALSE);		// OK if you are running surfwatch
	}
}

// Completion just adds it to the queue of things to process
// Interrupt level routine! No globals or mallocs
void MacTCPDriver::CompleteAsyncCall(AnnotatedPB * apb)
{
	UnholdMemoryGlue( apb, sizeof (*apb) );
	fQueueLock = FALSE;	// If we are walking the queue, the walker will check this lock to see if the queue has been modified
	OSErr err = ::Dequeue((QElemPtr)apb,&fOutstanding);
	if (err != noErr)
	{
		assert(FALSE);	
		return;
	}
	apb->qLink = NULL;
	{
		AnnotatedPB * oldApb = (AnnotatedPB *)fReceived.qHead;
		while (oldApb != NULL)
		{
			if (oldApb == apb)
				assert(FALSE);
			oldApb = (AnnotatedPB *)oldApb->qLink;
		}
	}
	::Enqueue((QElemPtr)apb, &fReceived);
}

void MacTCPDriver::DispatchCompleteAsyncCall(AnnotatedPB * apb)
{
	short		code;
	
	apb->stream->fLastCallInProgress = FALSE;
	if (apb->streamType == SOCK_STREAM) {
		apb->stream->fLastCallResult = apb->pb.tcpPB.ioResult;
		code = apb->pb.tcpPB.csCode;
	}
	else {
		apb->stream->fLastCallResult = apb->pb.udpPB.ioResult;
		code = apb->pb.udpPB.csCode;
	}

	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X Dispatching completed Async call.  Code is %d.\n", this, code);
	#endif

	switch (code)
	{	
		case TCPPassiveOpen:
			apb->stream->TCPPassiveOpenComplete(apb);
			break;
		case TCPActiveOpen:
			apb->stream->TCPActiveOpenComplete(apb);
			break;
		case TCPSend:
			apb->stream->TCPSendComplete(apb);
			break;
		case TCPClose:
			apb->stream->TCPCloseComplete(apb);
			break;
		case TCPRelease:
			apb->stream->TCPReleaseComplete(apb);
			break;
/*
		case TCPCreate:
			apb->stream->TCPCreateComplete(apb);
			break;
		case TCPNoCopyRcv:
			assert(false);
			break;
		case TCPRcvBfrReturn:
			assert(FALSE);
			break;
		case TCPRcv:
			apb->stream->TCPRcvComplete(apb);
			break;
		case TCPAbort:
			// apb->stream->TCPAbortComplete(apb);
			break;
		case TCPStatus:
			// apb->stream->TCPStatusComplete(apb);
			break;
		case TCPExtendedStat:
			// apb->stream->TCPExtendedStatComplete(apb);
			break;
		case TCPGlobalInfo:
			// apb->stream->TCPGlobalInfoComplete(apb);
			break;
		case TCPCtlMax:
			// apb->stream->TCPCtlMaxComplete(apb);
			break;
*/
		default:
			assert(FALSE);
	}
}
// When async calls complete, they end up on the fReceived queue
// we figure out what the calls were (stream, operation), and call
// the stream to handle the call (ignored right now
static Boolean spendTimeLock = FALSE;	// Prevents reentrancy
void MacTCPDriver::SpendTime()
{
	AnnotatedPB * apb = NULL;
	while(fReceived.qHead != NULL)
	{
	// Be very careful about the order of these calls
		apb = (AnnotatedPB *)fReceived.qHead;
		OSErr err = ::Dequeue((QElemPtr)apb,&fReceived);
		if (err) assert(FALSE);
		if (apb == (AnnotatedPB *)fReceived.qHead)
			assert(FALSE);
		DispatchCompleteAsyncCall(apb);
		ReturnPB(apb);
	}
	QElemPtr dnsElem = fDoneDNSCalls.qHead;
	while (dnsElem != NULL)
	{
		CMacTCPDNSObject * dns = (CMacTCPDNSObject *)((UInt32)dnsElem - (UInt32)offsetof(CMacTCPDNSObject, fQLink));
		dnsElem = dnsElem->qLink;
		if (dns->fSocket)
			dns->fSocket->DNSNotify();
		else if (dns->fDeleteSelfWhenDone)
			delete dns;
	}
}

// Should be used only to get the status of the connection
OSErr MacTCPDriver::MakeSyncCall(AnnotatedPB * apb)
{
	if (apb->streamType == SOCK_STREAM)
		apb->pb.tcpPB.ioCompletion = NULL;	// Just in case
	else
		apb->pb.udpPB.ioCompletion = NULL;	// Just in case
	
	OSErr err = PBControlSync((ParmBlkPtr)&(apb->pb));
	return err;
}

// Asych TCP notify procedure
// Has our stream class as user data ptr
pascal void TCPNotifyProc (StreamPtr tcpStream,unsigned short eventCode,Ptr userDataPtr,unsigned short terminReason,struct ICMPReport *icmpMsg);
pascal void TCPNotifyProc (	StreamPtr /* tcpStream*/ ,
							unsigned short eventCode,
							Ptr userDataPtr,
							unsigned short terminReason,
							struct ICMPReport *icmpMsg)
{
	MacTCPStream *	myStream =	(MacTCPStream *)userDataPtr;
	myStream->TCPNotify(eventCode, terminReason, icmpMsg);
}

PROCEDURE (TCPNotifyProc, uppTCPNotifyProcInfo)

// Asynch UDP notify procedure
// The creatorÕs stream class will be returned in the user data ptr
pascal void UDPNotifyProc(StreamPtr, unsigned short, Ptr, struct ICMPReport *);
pascal void UDPNotifyProc(	StreamPtr			/* tcpStream */, 
							unsigned short		eventCode, 
							Ptr					userDataPtr, 
							struct ICMPReport	*icmpMsg)
{
	MacTCPStream * myStream = (MacTCPStream *)userDataPtr;
	myStream->UDPNotify(eventCode, icmpMsg);
}

PROCEDURE (UDPNotifyProc, uppUDPNotifyProcInfo)


// ¥¥ DNS interface

PROCEDURE (DNRDone, uppResultProcInfo)

// Starts the StrToAddress call
CDNSObject * MacTCPDriver::StrToAddress(char * hostName)
{
	AssertOpen();
	CMacTCPDNSObject * dns = new CMacTCPDNSObject;
		
	ThrowIfNil_(dns);
	
	strncpy(dns->fHostInfo.name, hostName, kMaxHostNameLen);
	
	::Enqueue((QElemPtr) &dns->fQLink, &fWaitDNSCalls);
	OSErr err = StrToAddr(hostName, &dns->fTCPHostInfo, &PROCPTR(DNRDone), (char*)dns);
	if (err == noErr)
	{
		dns->fStatus = noErr;
		dns->TCPHostInfoToOT(&dns->fTCPHostInfo);
		::Dequeue((QElemPtr) &dns->fQLink,&fWaitDNSCalls);
	}
	else if (err != cacheFault)
	{
		dns->fStatus = err;
		::Dequeue((QElemPtr) &dns->fQLink,&fWaitDNSCalls);
	} 
	return dns;
}

CDNSObject * MacTCPDriver::StrToAddress(char * hostName, MacSocket * socket)
{
	AssertOpen();
	CMacTCPDNSObject * dns = new CMacTCPDNSObject(socket);
	ThrowIfNil_(dns);
	strcpy(dns->fHostInfo.name, hostName);
	::Enqueue((QElemPtr) &dns->fQLink, &fWaitDNSCalls);	
	OSErr err = StrToAddr(hostName, &dns->fTCPHostInfo, &PROCPTR(DNRDone), (char*)dns);
	if (err == noErr)
	{
		dns->fStatus = noErr;
		dns->TCPHostInfoToOT(&dns->fTCPHostInfo);
		::Dequeue((QElemPtr) &dns->fQLink,&fWaitDNSCalls);
	}
	else if (err != cacheFault)
	{
		dns->fStatus = err;
		::Dequeue((QElemPtr) &dns->fQLink,&fWaitDNSCalls);
	} 
	return dns;
}

int	MacTCPDriver::GetHostName(char *name, int namelen)
{
	AssertOpen();
	if (namelen < 16)
		return EINVAL;
	struct GetAddrParamBlock pbr;
	pbr.ioCRefNum 	= fDriverNum;
	pbr.csCode 		= ipctlGetAddr;
	InetHost thisHost;
	if (PBControlSync(ParmBlkPtr(&pbr)) != noErr)
		return ENETDOWN;
	thisHost = pbr.ourAddress;
	if (AddrToStr(thisHost, name) != noErr)
		return EINVAL;
	else
		return 0;
}

char * MacTCPDriver::AddressToString(InetHost host)
{
	AssertOpen();
	assert(fHostEnt.h_name);
	if (AddrToStr(host, fHostEnt.h_name) != noErr)
		return NULL;

	return fHostEnt.h_name;
}

#pragma mark -
#pragma mark CMacTCPDNSObject
#pragma mark -

// =========================================================================
// CMacTCPDNSObject
// =========================================================================

// DNR completion
// Interrupt level routine. No mallocs, or accessing global variables
static pascal void DNRDone (struct hostInfo * hostInformation, Ptr userData)
{
	CMacTCPDNSObject * dns = (CMacTCPDNSObject*)userData;
	dns->fStatus = hostInformation->rtnCode;
	dns->TCPHostInfoToOT(hostInformation);
	OSErr err = ::Dequeue((QElemPtr) &dns->fQLink,&((MacTCPDriver*)dns->fInterruptDriver)->fWaitDNSCalls);
	::Enqueue((QElemPtr)&dns->fQLink, &(dns->fInterruptDriver)->fDoneDNSCalls);
}

CMacTCPDNSObject::CMacTCPDNSObject(MacSocket * socket) : CDNSObject(socket)
{
	::HoldMemoryGlue( this, sizeof(*this) );
}

CMacTCPDNSObject::CMacTCPDNSObject() : CDNSObject()
{
	::HoldMemoryGlue( this, sizeof(*this) );
}

CMacTCPDNSObject::~CMacTCPDNSObject()
{
	::UnholdMemoryGlue( this, sizeof(*this) );
}

// Interrupt level routine. No mallocs, or accessing global variables. Called from DNRDone
void CMacTCPDNSObject::TCPHostInfoToOT(struct hostInfo * hostInformation)
{
	int i;
	for (i=0; i< NUM_ALT_ADDRS; i++)
		fHostInfo.addrs[i] = hostInformation->addr[i];
	fHostInfo.addrs[i] = 0;
}

#pragma mark -
#pragma mark MacTCPStream
#pragma mark -

// =========================================================================
// MacTCPStream
// =========================================================================

// ¥¥ constructors

MacTCPStream::MacTCPStream(MacSocket * socket) : CNetStream(socket)
{
	// Variable init
	fStream = NULL;

	fBufferSize = ((MacTCPDriver*)gNetDriver)->GetStreamBufferSize();
	fBuffer = malloc(fBufferSize);
	ThrowIfNil_(fBuffer);

	fNoMoreWrites = FALSE;
	fLastCallInProgress = FALSE;
	fLastCallResult = 0;
	fIsBound = false;
	fBoundHost = 0;
	fBoundPort = ((MacTCPDriver*)gNetDriver)->GetNextLocalPort();
	fPeekBuffer = NULL;
#ifdef DEBUG
	fConnectCalled = FALSE;
	fLastAsyncCall = 0;
#endif
}


MacTCPStream::~MacTCPStream()
{
	if (fBuffer)
		free(fBuffer);
}


int MacTCPStream::CreateStream()
{
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X CreateStream.\n", this);
	#endif
	
	if (fSocket->fType == SOCK_STREAM)
		return CreateTypedStream(NULL);					// Actually do the creation now - donÕt care about port #
	else
		return TCPErrorToUnix(TCPCreate, noErr);		// Delay until bind() time; we need to know the port
}


// Create a UDP stream
int MacTCPStream::CreateTypedStream(InetPort port)
{
	AcquirePB *		apb;
	
	// Create a new paramblock, initialized correctly
	if (fSocket->fType == SOCK_STREAM) 
		apb = new AcquirePB(SOCK_STREAM, TCPCreate, this);
	else
		apb = new AcquirePB(SOCK_DGRAM, UDPCreate, this);
		
	if (apb->fApb == NULL)
		return -1;
		
	if (fSocket->fType == SOCK_STREAM) {
		apb->fApb->pb.tcpPB.csParam.create.rcvBuff = (Ptr)fBuffer;
		apb->fApb->pb.tcpPB.csParam.create.rcvBuffLen = fBufferSize;
		apb->fApb->pb.tcpPB.csParam.create.userDataPtr = (Ptr)this;
		apb->fApb->pb.tcpPB.csParam.create.notifyProc = &PROCPTR(TCPNotifyProc);
	}
	else {			// fSocket->fType == SOCK_STREAM
		fPendingUDPReceives	= 0;
		apb->fApb->pb.udpPB.csParam.create.rcvBuff = (Ptr)fBuffer;
		apb->fApb->pb.udpPB.csParam.create.rcvBuffLen = fBufferSize;
		apb->fApb->pb.udpPB.csParam.create.userDataPtr = (Ptr)this;
		apb->fApb->pb.udpPB.csParam.create.notifyProc = &PROCPTR(UDPNotifyProc);
		apb->fApb->pb.udpPB.csParam.create.localPort = port;
	}
	
	((MacTCPDriver*)gNetDriver)->MakeSyncCall(apb->fApb);

	if (fSocket->fType == SOCK_STREAM)
		return TCPCreateComplete(apb->fApb);
	else
		return UDPCreateComplete(apb->fApb);
	
}


// Call to close the socket
// If we are not aborting, call the normal close
// close callback calls the TCPRelease
// TCPRelease callback destroys the stream
// TCPRelease cannot 
int MacTCPStream::DestroyStream(Boolean abort)
{
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X DestroyStream\n", this);
	#endif

	int result = 0;

	if ((fStream == NULL) || (fSocket == NULL))
		return 0;
		
	if (fSocket->fType == SOCK_STREAM)
		result = TCPDestroyStream(abort);
	else
		result =  UDPDestroyStream(abort);
	fSocket = NULL;
	return result;
}


int MacTCPStream::TCPDestroyStream(Boolean abort)
{
	if (!abort)						// Normal closing
	{
		fLastCallInProgress = FALSE;	// It is OK to multiplex TCPClose calls
		AnnotatedPB * apb = ((MacTCPDriver*)gNetDriver)->GetPB(fSocket->fType, TCPClose, this);
		if (apb == NULL)
			return -1;

		apb->pb.tcpPB.csParam.close.validityFlags = timeoutValue | timeoutAction;
		apb->pb.tcpPB.csParam.close.ulpTimeoutValue 	= CLOSE_TIMEOUT;	// seconds
		apb->pb.tcpPB.csParam.close.ulpTimeoutAction 	= 1;  // 1:abort 0:report
		// TimeoutAction must be abort. Otherwise, the async call never comes back,
		// and we just get notified.
		((MacTCPDriver*)gNetDriver)->MakeAsyncCall(apb);
		return 0;
	}
	else
	{
// WARNING: TCPRelease cannot be called asynchronously, because MacTCP gets
// confused, and reports an infinite number of callbacks (sometimes). So I make
// a sync call, and return
		AcquirePB apb(SOCK_STREAM, TCPRelease, this);
		if (apb.fApb == NULL)
			return -1;
		((MacTCPDriver*)gNetDriver)->MakeSyncCall(apb.fApb);
		TCPReleaseComplete(apb.fApb);	// This deletes this stream
		return 0;
	}
	return 0;
}


int MacTCPStream::UDPDestroyStream(Boolean abort)
{
	AcquirePB	apb(SOCK_DGRAM, UDPRelease, this);
	if (apb.fApb == NULL)
		return -1;
		
	((MacTCPDriver*)gNetDriver)->MakeSyncCall(apb.fApb);
	return 0;
}


// ¥¥ completion routines

// Notification routine
// Async callback routine.
// A5 is OK. Cannot allocate memory here. Can issue other TCP calls
void MacTCPStream::TCPNotify(unsigned short eventCode, 
					unsigned short /* terminReason */, 
					struct ICMPReport * /* icmpMsg */)
{
	switch (eventCode) {
		case TCPClosing:	// Noted
		case TCPTerminate:
			if (fSocket)
				fSocket->CloseNotify();
			fNoMoreWrites = TRUE;
			break;
		case TCPULPTimeout:	// Ignored
		case TCPUrgent:
		case TCPICMPReceived:
			break;
		case TCPDataArrival:
		default:
			break;
	}
	// Notify the NSPR threads mechanism that an I/O interrupt has occured.
	_PR_AsyncIOInterruptHandler();
}


void MacTCPStream::UDPNotify(unsigned short eventCode, struct ICMPReport *icmpMsg)
{
	if (eventCode == UDPDataArrival)
		fPendingUDPReceives++;

	// Notify the NSPR threads mechanism that an I/O interrupt has occured.
	_PR_AsyncIOInterruptHandler();
}


// ¥¥ Completion routines
// each routine:
// translates MacTCP errors into UNIX codes
// notifies sockets that it is ready
// Misc:

// See docs above
// Misc: sets the stream

int MacTCPStream::TCPCreateComplete(AnnotatedPB * apb)
{
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X TCPCreateComplete\n", this);
	#endif
	// Making sure MacTCP has given us the stream
	if ((apb->pb.tcpPB.tcpStream == NULL) && (apb->pb.tcpPB.ioResult == noErr))
		apb->pb.tcpPB.ioResult = insufficientResources;
	if (apb->pb.tcpPB.ioResult != noErr)
		apb->pb.tcpPB.tcpStream = NULL;

	fStream = apb->pb.tcpPB.tcpStream;	// We are ready to roll
	return TCPErrorToUnix(apb->pb.tcpPB.csCode, apb->pb.tcpPB.ioResult);
}


int MacTCPStream::UDPCreateComplete(AnnotatedPB * apb)
{
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X TCPCreateComplete\n", this);
	#endif
	// Making sure MacTCP has given us the stream
	if ((apb->pb.udpPB.udpStream == NULL) && (apb->pb.udpPB.ioResult == noErr))
		apb->pb.udpPB.ioResult = insufficientResources;
	if (apb->pb.udpPB.ioResult != noErr)
		apb->pb.udpPB.udpStream = NULL;

	fBoundPort = apb->pb.udpPB.csParam.create.localPort;
	fStream = apb->pb.udpPB.udpStream;	// We are ready to roll
	return TCPErrorToUnix(apb->pb.udpPB.csCode, apb->pb.udpPB.ioResult);
}


int MacTCPStream::TCPActiveOpenComplete(AnnotatedPB * apb)
{
	short	code, result;
	
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X TCPActiveOpenComplete\n", this);
	#endif
	
	if (apb->streamType == SOCK_STREAM) {
		code = apb->pb.tcpPB.csCode;
		result = apb->pb.tcpPB.ioResult;
	}
	else {
		code = apb->pb.udpPB.csCode;
		result = apb->pb.udpPB.ioResult;
	}

	int err = TCPErrorToUnix(code, result);
	
	if (fSocket)
		fSocket->ConnectComplete(FALSE, err);

	return err;
}

int MacTCPStream::TCPSendComplete(AnnotatedPB * apb)
{
	Cwds * wds = (Cwds *)apb->pb.tcpPB.csParam.send.wdsPtr;
	
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X TCPSendComplete, wds: %Xd\n", this, wds);
	#endif
	
	if (wds != NULL)
		delete wds;
	else
		assert(FALSE);

	int err = TCPErrorToUnix(apb->pb.tcpPB.csCode, apb->pb.tcpPB.ioResult);
	
	if (fSocket)
		fSocket->WriteComplete(err);
		
	return err;
}


int MacTCPStream::TCPPassiveOpenComplete(AnnotatedPB * apb)
{

	int err = TCPErrorToUnix(apb->pb.tcpPB.csCode, apb->pb.tcpPB.ioResult);

	if (fSocket)
		fSocket->AcceptComplete(err);
	return err;
}


// Makes the release call
int MacTCPStream::TCPCloseComplete(AnnotatedPB * apb)
{
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X TCPCloseComplete\n", this);
	#endif
	
	int err = TCPErrorToUnix(apb->pb.tcpPB.csCode, apb->pb.tcpPB.ioResult);

	AcquirePB newApb(SOCK_STREAM, TCPRelease, this);
	if ( newApb.fApb == NULL)
		return -1;
	((MacTCPDriver*)gNetDriver)->MakeSyncCall(newApb.fApb);
	TCPReleaseComplete(newApb.fApb);	

	return err;
}


// deletes the stream
int MacTCPStream::TCPReleaseComplete(AnnotatedPB * apb)
{
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X TCPReleaseComplete\n", this);
	#endif
	
	// ¥¥¥ LAM, it would be nice if we checked for any outstanding async calls here
	int err = TCPErrorToUnix(apb->pb.tcpPB.csCode, apb->pb.tcpPB.ioResult);

#ifdef DEBUG
	if (apb->pb.tcpPB.csParam.create.rcvBuffLen != apb->stream->fBufferSize)
	{
		::DebugStr("\pMacTCP Release leakage;g;");
		apb->stream->fBuffer = NULL;
	}
#endif
	delete apb->stream;

	return err;
}



// Giant lookup table of tcp error codes, to socket ones
// Everyone can get our internal driverError or streamBusy
// TCPCreate, TCPActiveOpen, TCPClose, TCPRelease, TCPSend, TCPNoCpyRvc, 
// TCPRcvBfrReturn, TCPStatus

Int32 MacTCPStream::TCPErrorToUnix(int tcpCall, int error)
{
	if (error == 0)
		return 0;
	switch (tcpCall)
	{
		case TCPCreate:
		// possible errors:
		// streamAlreadyOpen, invalidLength, invalidBufPtr, insufficientResources
		// Maps to connect errors
			if ((error == streamAlreadyOpen) || (error == streamBusy))
				return EACCESS;
			else
				return ENOBUFS;
			break;
		case TCPActiveOpen:
		// possible errors
		// invalidStreamPtr, connectionExists, duplicateSocket, openFailed, 
			switch (error)
			{
				case invalidStreamPtr:
					return EBADF;
				case connectionExists:
					return EALREADY;
				case duplicateSocket:
					return EADDRINUSE;
				case openFailed:
					return ECONNREFUSED;
				case driverError:
					return EBADF;
				case streamBusy:
					return EINPROGRESS;
				case connectionTerminated:
					return ECONNREFUSED;
				default:
					assert(FALSE);
					return EBADF;
			}
			break;
		case TCPSend:
		// possible errors
		// invalidStreamPtr, invalidLength, invalidWDS	the WDS pointer was 0
		// connectionDoesntExist, connectionClosing	,connectionTerminated
			switch (error)
			{
				case invalidStreamPtr:
					return EINVAL;
				case invalidLength:
				case invalidWDS:
					return EFAULT;
				case connectionDoesntExist:
				case connectionClosing:
				case connectionTerminated:
					return EPIPE;
				case driverError:
					return EPIPE;
				case streamBusy:
					return EWOULDBLOCK;
				default:
					assert(FALSE);
					return EINVAL;
			}
			break;
		case TCPRcv:
			switch (error)
			{
				case connectionClosing:
				case connectionTerminated:
					return 0;
				case streamBusy:
					return EWOULDBLOCK;
				case invalidStreamPtr:
				case connectionDoesntExist:
				case invalidLength:
				case invalidBufPtr:
				case commandTimeout:
				case driverError:
				default:
					return EIO;
			}
			break;
		case TCPClose:
			return error;
			break;
		case TCPRelease:
			return error;
		case TCPPassiveOpen:
			return error;
		case TCPNoCopyRcv:
		case TCPRcvBfrReturn:
		case TCPAbort:
		case TCPStatus:
		case TCPExtendedStat:
		case TCPGlobalInfo:
		case TCPCtlMax:
			assert(FALSE);
	}
/*
case inProgress					= 1,		// I/O in progress
case ipBadLapErr				= -23000,	// bad network configuration
case ipBadCnfgErr				= -23001,	// bad IP configuration error
case ipNoCnfgErr				= -23002,	// missing IP or LAP configuration error
case ipLoadErr					= -23003,	//error in MacTCP load
case ipBadAddr					= -23004,	// error in getting address
case connectionClosing			= -23005,	// connection is closing
case invalidLength				= -23006,
case connectionExists			= -23007,	// this TCP stream already has an open connection
case connectionDoesntExist		= -23008,	// connection does not exist
case insufficientResources		= -23009,	// insufficient resources to perform request
case invalidStreamPtr			= -23010,	// the specified TCP stream is not open
case streamAlreadyOpen			= -23011,
case connectionTerminated		= -23012,
case invalidBufPtr				= -23013,
case invalidRDS					= -23014,
case invalidWDS					= -23014,
case openFailed					= -23015,	// 	the connection came halfway up and then failed
case commandTimeout				= -23016,
case duplicateSocket			= -23017	// a connection already exists between this local IP address and TCP port, and the specified remote IP address and TCP port
*/
	return error;
}


// Connect to a given host, port
// Asynchronous, needs to keep state to know if it has already returned an error code
// returns 
// EINPROGRESS if connection is being made
// ECONNREFUSED if connection is closing/broken
// 0 if connection is up.
int MacTCPStream::ActiveConnect(InetHost host, InetPort port)
{
	short		state;
	
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X TCP Active Connect starting\n", this);
	#endif
	
#ifdef DEBUG
	if (fConnectCalled)
		BreakToSourceDebugger_();	// Should call this only once from the socket layer
	fConnectCalled = TRUE;
#endif
	// Make sure that we have a stream

	AcquirePB acqPB(SOCK_STREAM, TCPStatus, this);
	if ( acqPB.fApb == NULL )
		return ECONNREFUSED;

	OSErr err  = ((MacTCPDriver*)gNetDriver)->MakeSyncCall(acqPB.fApb);
	// What is our connection state
	// What are the proper errors to return here?
	// If closing, tell them that the remote peer has closed the connection
	//		possible error: we decided to close
	// If we are listening, tell them address is in use
	// If we are already trying to establish, tell them so
	if (err == noErr) {
		state = acqPB.fApb->pb.tcpPB.csParam.status.connectionState;
		#ifdef TCP_DEBUG
		XP_Trace("MacTCPDriver: %X TCP Active Connect current state is %d\n", this, state);
		#endif
		switch (state)
		{
			case 0:	//	Closed	no connection exists on this stream
				break;
			case 2:	//	Listen	listening for an incoming connection
			case 4:	//	SYN received	incoming connection is being established
			case 6:	//	SYN sent	outgoing connection is being established
				return EINPROGRESS;
			case 8:	//	Established	connection is up
				return 0;
			case 10:	//	FIN Wait 1	connection is up; close has been issued
			case 12:	//	FIN Wait 2	connection is up; close has been completed
			case 14:	//	Close Wait	connection is up; close has been received
			case 16:	//	Closing	connection is up; close has been issued and 		received
			case 18:	//	Last Ack	connection is up; close has been issued and 		received
			case 20:	//	Time Wait	connection is being broken
				return ECONNREFUSED;
		}
	}
	AnnotatedPB * capb = NULL;
	if ((err == connectionDoesntExist) ||
		((err == noErr) && acqPB.fApb->pb.tcpPB.csParam.status.connectionState == 0))	
	//	No connection, make a call to open it
	{
		capb = 	((MacTCPDriver*)gNetDriver)->GetPB(SOCK_STREAM, TCPActiveOpen, this);
		if (capb == NULL)
			return -1;
		capb->pb.tcpPB.csParam.open.remoteHost = host;
		capb->pb.tcpPB.csParam.open.remotePort = port;
		capb->pb.tcpPB.csParam.open.validityFlags = timeoutValue | timeoutAction;
		capb->pb.tcpPB.csParam.open.ulpTimeoutValue = OPEN_TIMEOUT;
		capb->pb.tcpPB.csParam.open.ulpTimeoutAction = 1;		// 1:abort 0:report
		capb->pb.tcpPB.csParam.open.localHost = fBoundHost;
		capb->pb.tcpPB.csParam.open.localPort = fBoundPort;
		((MacTCPDriver*)gNetDriver)->MakeAsyncCall(capb);
		#ifdef TCP_DEBUG
		XP_Trace("MacTCPDriver: %Xd TCP Active Connect issued.\n", this);
		#endif
	}
	return EINPROGRESS;
}


// This is dependent on the type of connection.  For TCP, itÕs
// easy; we donÕt bind until connect time.
// For UDP, we actually create the socket here since the port 
// is specified at UDPCreate() time.
int	MacTCPStream::Bind(InetHost host, InetPort port)
{
	int 	returnValue;
	
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X Request bind to host %#x, port %d\n", this, host, port);
	#endif
	
	switch (fSocket->fType) {
		case SOCK_STREAM:
			fBoundHost = host;
			if (port != 0)					// Should we override the allcoated one?
				fBoundPort = port;
			returnValue = 0;
			break;
		case SOCK_DGRAM: {
			returnValue = TCPErrorToUnix(UDPCreate, CreateTypedStream(port));
			break;
		}
		default:
			assert(false);					// Better never happen
	}
	
	fIsBound = (noErr == returnValue);
	
	return returnValue;
}


// Backlog parameter is ignored for MacTCP streams
int MacTCPStream::Listen(int /* backlog*/)
{
#ifdef TCP_DEBUG
XP_Trace("MacTCPDriver: %X TCP Listen\n", this);
#endif
#ifdef DEBUG
	if (fConnectCalled)
		BreakToSourceDebugger_();	// Should call this only once from the socket layer
	fConnectCalled = TRUE;
#endif
	gNetDriver->SpendTime();

	AcquirePB acqPB(SOCK_STREAM, TCPStatus, this);
	if ( acqPB.fApb == NULL )
		return -1;

	((MacTCPDriver*)gNetDriver)->MakeSyncCall(acqPB.fApb);
	// What is our connection state
	// What are the proper errors to return here?
	// If closing, tell them that the remote peer has closed the connection
	//		possible error: we decided to close
	// If we are listening, tell them address is in use
	// If we are already trying to establish, tell them so
	switch (acqPB.fApb->pb.tcpPB.csParam.status.connectionState)
	{
		case 0:	//	Closed	no connection exists on this stream
			break;
		case 2:	//	Listen	listening for an incoming connection
		case 4:	//	SYN received	incoming connection is being established
		case 6:	//	SYN sent	outgoing connection is being established
			return EINPROGRESS;
		case 8:	//	Established	connection is up
			return EISCONN;
		// ¥¥¥ LAM, I am not sure what the status should be here
		case 10:	//	FIN Wait 1	connection is up; close has been issued
		case 12:	//	FIN Wait 2	connection is up; close has been completed
		case 14:	//	Close Wait	connection is up; close has been received
		case 16:	//	Closing	connection is up; close has been issued and 		received
		case 18:	//	Last Ack	connection is up; close has been issued and 		received
		case 20:	//	Time Wait	connection is being broken
			return EISCONN;
	}

	// Make the call
#ifdef TCP_DEBUG
XP_Trace("MacTCPDriver: %X TCP Listen making the call\n", this);
#endif
	AnnotatedPB * apb = ((MacTCPDriver*)gNetDriver)->GetPB(SOCK_STREAM, TCPPassiveOpen, this);
	if (apb == NULL)
		return -1;
	apb->pb.tcpPB.csParam.open.ulpTimeoutValue = 255;
	apb->pb.tcpPB.csParam.open.ulpTimeoutAction = 1; // 1:abort 0:report
	apb->pb.tcpPB.csParam.open.commandTimeoutValue = 0; // infinity 
	apb->pb.tcpPB.csParam.open.localHost = fBoundHost;
	apb->pb.tcpPB.csParam.open.localPort = fBoundPort;
	((MacTCPDriver*)gNetDriver)->MakeAsyncCall(apb);

	// Success/failure is figured out in accept
	return 0;
}

int MacTCPStream::Accept()
{
	AcquirePB acqPB(SOCK_STREAM, TCPStatus, this);
	if (acqPB.fApb == NULL)
		return -1;
	OSErr err  = ((MacTCPDriver*)gNetDriver)->MakeSyncCall(acqPB.fApb);
	// What is our connection state
	// What are the proper errors to return here?
	// If closing, tell them that the remote peer has closed the connection
	//		possible error: we decided to close
	// If we are listening, tell them address is in use
	// If we are already trying to establish, tell them so
	if (err == noErr)
		switch (acqPB.fApb->pb.tcpPB.csParam.status.connectionState)
		{
			case 0:	//	Closed	no connection exists on this stream
				return ECONNREFUSED;
			case 2:	//	Listen	listening for an incoming connection
			case 4:	//	SYN received	incoming connection is being established
			case 6:	//	SYN sent	outgoing connection is being established
				return EWOULDBLOCK;
			case 8:	//	Established	connection is up
				return 0;
			case 10:	//	FIN Wait 1	connection is up; close has been issued
			case 12:	//	FIN Wait 2	connection is up; close has been completed
			case 14:	//	Close Wait	connection is up; close has been received
			case 16:	//	Closing	connection is up; close has been issued and 		received
			case 18:	//	Last Ack	connection is up; close has been issued and 		received
			case 20:	//	Time Wait	connection is being broken
				return ECONNREFUSED;
		}
	else
		return EBADF;
	return -1;		// Never reached
}

// Only one write call at a time is allowed
// bytesWritten is always set to whatever we try to write, but it is only valid if
// we return no error
int MacTCPStream::Write(const void *buffer, unsigned int buflen, UInt16& bytesWritten)
{
	short		state;
	
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X Starting TCP Write, length %d\n", this, buflen);
	#endif

	bytesWritten = 0;
	AcquirePB acqPB(SOCK_STREAM, TCPStatus, this);
	if ( acqPB.fApb == NULL )
		return -1;
	((MacTCPDriver*)gNetDriver)->MakeSyncCall(acqPB.fApb);

	state = acqPB.fApb->pb.tcpPB.csParam.status.connectionState;

	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X TCP Write socket status: %d\n", this, state);
	#endif

	switch (state)
	{
		case 0:	//	Closed	no connection exists on this stream
		case 2:	//	Listen	listening for an incoming connection
			return EPIPE;
		case 4:	//	SYN received	incoming connection is being established
		case 6:	//	SYN sent	outgoing connection is being established
			return EWOULDBLOCK;
		case 8:	//	Established	connection is up
			break;
		case 10:	//	FIN Wait 1	connection is up; close has been issued
		case 12:	//	FIN Wait 2	connection is up; close has been completed
		case 14:	//	Close Wait	connection is up; close has been received
		case 16:	//	Closing	connection is up; close has been issued and 		received
		case 18:	//	Last Ack	connection is up; close has been issued and 		received
		case 20:	//	Time Wait	connection is being broken
			return EPIPE;
	}
	// At this point, we are doing one of these:
	// - Establishing a connection
	// - Waiting for another write to complete
	// - Nothing
	// Wait for all async calls to complete, or user interrupt
	if (fNoMoreWrites)
		return EPIPE;
	// We might have closed by now, process the notifications
	// If there was an error, return IO error
	if (fLastCallResult != noErr)
		return EIO;

	// Prepare all the stuctures (wds, and the buffer), copy the data to the buffer
	bytesWritten = buflen > MAX_SEND_BLOCK ? MAX_SEND_BLOCK : buflen;
	Cwds * volatile wds = NULL;
	Try_
	{
		wds = new Cwds(bytesWritten, buffer);
	}
	Catch_(inErr)
	{
		if (wds)
			delete wds;
		return EWOULDBLOCK;
	}
	EndCatch_ 
	
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X TCP Write, cwd is %xd\n", this, wds);
	#endif
	
	// Complete the calling structure, make the call
	AnnotatedPB * apb = ((MacTCPDriver*)gNetDriver)->GetPB(SOCK_STREAM, TCPSend, this);
	if (apb == NULL)
		return -1;
	apb->pb.tcpPB.csParam.send.validityFlags 		= timeoutValue | timeoutAction;
	apb->pb.tcpPB.csParam.send.ulpTimeoutValue 		= SEND_TIMEOUT; // seconds
	apb->pb.tcpPB.csParam.send.ulpTimeoutAction 	= 1; // 1:abort 0:report
	// pushFlag, urgentFlag are all 0
	apb->pb.tcpPB.csParam.send.wdsPtr 			= (Ptr)wds;
	((MacTCPDriver*)gNetDriver)->MakeAsyncCall(apb);

	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X TCP Write, complete.\n", this, wds);
	#endif

	return 0;
}

int MacTCPStream::Read(void *buffer, unsigned int buflen, UInt16& bytesRead)
{
	bytesRead = 0;
	UInt16 bytesToRead = 0;
	Boolean readAlready = FALSE;
	// Check our status if we do not already have the data
#ifdef TCP_DEBUG
XP_Trace("MacTCPDriver: %X TCP Read starts.  Reading %d bytes,\n", this, buflen);
#endif
readagain:	// For speed, we loop through the read loop
	// We can read, even if the connection is closing
	// Some data might already be in the buffer
	{
		AcquirePB acqPB(SOCK_STREAM, TCPStatus, this);
		if ( acqPB.fApb == NULL )
			return -1;
		((MacTCPDriver*)gNetDriver)->MakeSyncCall(acqPB.fApb);
		switch (acqPB.fApb->pb.tcpPB.csParam.status.connectionState)
		{
			case 0:		//	Closed	no connection exists on this stream
			case 2:		//	Listen	listening for an incoming connection
				return EPIPE;
			case 4:		//	SYN received	incoming connection is being established
			case 6:		//	SYN sent	outgoing connection is being established
			case 8:		//	Established	connection is up
				break;
			case 10:	//	FIN Wait 1	connection is up; close has been issued
			case 12:	//	FIN Wait 2	connection is up; close has been completed
			case 14:	//	Close Wait	connection is up; close has been received
			case 16:	//	Closing	connection is up; close has been issued and 		received
			case 18:	//	Last Ack	connection is up; close has been issued and 		received
			case 20:	//	Time Wait	connection is being broken
				if (acqPB.fApb->pb.tcpPB.csParam.status.amtUnreadData == 0)	// Nothing more to read, die now
					return 0;
				break;
		}
		bytesToRead = acqPB.fApb->pb.tcpPB.csParam.status.amtUnreadData > buflen ? buflen :  acqPB.fApb->pb.tcpPB.csParam.status.amtUnreadData;
	}
	if (readAlready && (bytesToRead < 500))	// Do not bother with repetitive reads for small chunks
		return 0;
	
	if (bytesToRead == 0)
		return EWOULDBLOCK;
	// Make the rcv call
	{
		AcquirePB acqPB(SOCK_STREAM, TCPRcv, this);
		if ( acqPB.fApb == NULL )
			return -1;

		acqPB.fApb->pb.tcpPB.csParam.receive.commandTimeoutValue = 0; /* seconds, 0 = blocking */
		acqPB.fApb->pb.tcpPB.csParam.receive.rcvBuff 			= (Ptr)buffer;
		acqPB.fApb->pb.tcpPB.csParam.receive.rcvBuffLen 		= bytesToRead;
	
		((MacTCPDriver*)gNetDriver)->MakeSyncCall(acqPB.fApb);
		if (acqPB.fApb->pb.tcpPB.ioResult != 0)
			return TCPErrorToUnix(TCPRcv, acqPB.fApb->pb.tcpPB.ioResult);
		bytesRead += acqPB.fApb->pb.tcpPB.csParam.receive.rcvBuffLen;
		buflen -= acqPB.fApb->pb.tcpPB.csParam.receive.rcvBuffLen;	// Adjust the pointers in case we read again
		buffer = (void*)((UInt32)buffer + (UInt32)acqPB.fApb->pb.tcpPB.csParam.receive.rcvBuffLen);
#ifdef TCP_DEBUG
XP_Trace("MacTCPDriver: %X TCP Read had : %d\n", this, bytesRead);
#endif
	}
	readAlready = TRUE;
	goto readagain;
	return 0;
}

Boolean MacTCPStream::Select(Boolean& readReady,Boolean& writeReady,Boolean& exceptReady)
{
	AcquirePB acqPB(SOCK_STREAM, TCPStatus, this);
	if ( acqPB.fApb == NULL )
		return false;

	readReady = writeReady = exceptReady = FALSE;
	((MacTCPDriver*)gNetDriver)->MakeSyncCall(acqPB.fApb);
	if (acqPB.fApb->pb.tcpPB.ioResult != 0)
		exceptReady = TRUE;
	else
	{
		switch (acqPB.fApb->pb.tcpPB.csParam.status.connectionState)
		{
			case 0:	//	Closed	no connection exists on this stream
			case 2:	//	Listen	listening for an incoming connection
				break;
			case 8:	//	Established	connection is up
				writeReady = TRUE;
				break;
			case 4:	//	SYN received	incoming connection is being established
			case 6:	//	SYN sent	outgoing connection is being established
			case 10:	//	FIN Wait 1	connection is up; close has been issued
			case 12:	//	FIN Wait 2	connection is up; close has been completed
				break;
			case 14:	//	Close Wait	connection is up; close has been received
			case 16:	//	Closing	connection is up; close has been issued and received
			case 18:	//	Last Ack	connection is up; close has been issued and received
			case 20:	//	Time Wait	connection is being broken
				exceptReady = TRUE;	// Not sure what to do here
		}
		readReady = acqPB.fApb->pb.tcpPB.csParam.status.amtUnreadData > 0;
#ifdef TCP_DEBUG
//	XP_Trace("MacTCPDriver:  TCP Select %Xd, read %d, write %d, exception %d\n", this, readReady, writeReady, exceptReady);
#endif
	}
	return (readReady || writeReady || exceptReady);
}

int MacTCPStream::GetPeerName(InetHost& host, InetPort& port)
{
	AcquirePB acqPB(SOCK_STREAM, TCPStatus, this);
	if (acqPB.fApb == NULL)
		return -1;

	((MacTCPDriver*)gNetDriver)->MakeSyncCall(acqPB.fApb);
	switch (acqPB.fApb->pb.tcpPB.csParam.status.connectionState)
	{
		case 8:	//	Established	connection is up
		case 10:	//	FIN Wait 1	connection is up; close has been issued
		case 12:	//	FIN Wait 2	connection is up; close has been completed
		case 14:	//	Close Wait	connection is up; close has been received
		case 16:	//	Closing	connection is up; close has been issued and 		received
		case 18:	//	Last Ack	connection is up; close has been issued and 		received
		case 20:	//	Time Wait	connection is being broken
			host = acqPB.fApb->pb.tcpPB.csParam.status.remoteHost;
			port = acqPB.fApb->pb.tcpPB.csParam.status.remotePort;
			return 0;
		case 0:	//	Closed	no connection exists on this stream
		case 2:	//	Listen	listening for an incoming connection
		case 4:	//	SYN received	incoming connection is being established
		case 6:	//	SYN sent	outgoing connection is being established
			return ENOTCONN;
	}
	return ENOTCONN;
}

int MacTCPStream::GetSockName(InetHost& host, InetPort& port)
{
	AcquirePB acqPB(SOCK_STREAM, TCPStatus, this);
	if ( acqPB.fApb == NULL )
		return -1;
	((MacTCPDriver*)gNetDriver)->MakeSyncCall(acqPB.fApb);
	switch (acqPB.fApb->pb.tcpPB.csParam.status.connectionState)
	{
		case 8:	//	Established	connection is up
		case 10:	//	FIN Wait 1	connection is up; close has been issued
		case 12:	//	FIN Wait 2	connection is up; close has been completed
		case 14:	//	Close Wait	connection is up; close has been received
		case 16:	//	Closing	connection is up; close has been issued and 		received
		case 18:	//	Last Ack	connection is up; close has been issued and 		received
		case 20:	//	Time Wait	connection is being broken
			host = acqPB.fApb->pb.tcpPB.csParam.status.localHost;
			port = acqPB.fApb->pb.tcpPB.csParam.status.localPort;
			return 0;
		case 0:	//	Closed	no connection exists on this stream
				// Can be called after BIND. So, if fBoundHost != 0, fake it
			if (fIsBound)
			{
				host = fBoundHost;
				port = fBoundPort;
				return 0;
			}
			else
				return ENOTCONN;
		case 2:	//	Listen	listening for an incoming connection
		case 4:	//	SYN received	incoming connection is being established
		case 6:	//	SYN sent	outgoing connection is being established
			return ENOTCONN;
	}
	return ENOTCONN;
}


int MacTCPStream::UDPSendTo(const void *msg, unsigned int msgLen, unsigned int flags, 
								InetHost host, InetPort port, unsigned int& bytesSent)
{
	Cwds *			volatile wds = NULL;
	short			ioResult;
	
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X UDP SendTo, msgLen = %d, host = %#x, port = %d\n", this, msgLen, host, port);
	DumpBuffer(msg, msgLen);
	#endif
	
	// We need to create the Write Data Structure
	bytesSent = (msgLen > MAX_SEND_BLOCK) ? MAX_SEND_BLOCK : msgLen;
	Try_
	{
		wds = new Cwds(bytesSent, msg);
	}
	Catch_(inErr)
	{
		if (wds)
			delete wds;
		return EWOULDBLOCK;
	}
	EndCatch_
	
	#ifdef TCP_DEBUG
	XP_Trace("MacTCPDriver: %X UDP Send To, cwd is %#x\n", this, wds);
	#endif
	
	// Complete the calling structure, make the call
	AnnotatedPB * apb = ((MacTCPDriver*)gNetDriver)->GetPB(SOCK_DGRAM, UDPWrite, this);
	if (apb == NULL)
		return -1;
	apb->pb.udpPB.csParam.send.remoteHost = host;
	apb->pb.udpPB.csParam.send.remotePort = port;
	apb->pb.udpPB.csParam.send.wdsPtr = (Ptr)wds;
	apb->pb.udpPB.csParam.send.checkSum = false;
	apb->pb.udpPB.csParam.send.filler = 0;
	apb->pb.udpPB.csParam.send.userDataPtr = (Ptr)this;
	((MacTCPDriver*)gNetDriver)->MakeSyncCall(apb);

	// Writes are done synchronously, we can tear down our data structures.
	delete wds;
	
	// Tell our socket weÕre done.
	fSocket->WriteComplete(ioResult = apb->pb.udpPB.ioResult);
	
	// Success?
	if (ioResult == noErr)
		return noErr;
	else if (ioResult == insufficientResources)
		return EWOULDBLOCK;
	else
		return -1;
}


int MacTCPStream::UDPReadIntoBuffer(void* buffer, unsigned int len, struct sockaddr* from)
{
	short			bytesToRead;
	short			receivedBufferLen;
	sockaddr_in* 	returnAddress;
	
	AnnotatedPB * apb = ((MacTCPDriver*)gNetDriver)->GetPB(SOCK_DGRAM, UDPRead, this);
	
	ThrowIfNil_(apb);
	
	apb->pb.udpPB.csParam.receive.timeOut = 2;							// Minimum suported.

	ThrowIfOSErr_(((MacTCPDriver*)gNetDriver)->MakeSyncCall(apb));		// Go for it.  Asynch not supported.
	
	ThrowIfOSErr_(apb->pb.udpPB.ioResult);								// Successful?
	
	receivedBufferLen = apb->pb.udpPB.csParam.receive.rcvBuffLen;
	// Read was successful, get what we want out of the param block.
	bytesToRead = MIN(receivedBufferLen, len);										// How much to read?
	BlockMoveData(apb->pb.udpPB.csParam.receive.rcvBuff, buffer, bytesToRead);		// What do we do with the extra?
	
	returnAddress = (sockaddr_in *)from;											// Pull out the senderÕs address
	from->sa_len = offsetof(sockaddr,sa_data) + sizeof(in_addr);
	from->sa_family = SOCK_DGRAM;
	returnAddress->sin_port = apb->pb.udpPB.csParam.receive.remotePort;
	returnAddress->sin_addr.s_addr = apb->pb.udpPB.csParam.receive.remoteHost;
	
	// Release the buffer
	apb->pb.udpPB.csCode = UDPBfrReturn;
	((MacTCPDriver*)gNetDriver)->MakeSyncCall(apb);		// Release it.
	fPendingUDPReceives--;
	
	return bytesToRead;
}


int MacTCPStream::UDPReceiveFrom(void *buffer, unsigned int len, unsigned int flags, struct sockaddr *from, unsigned int& fromLen)
{
	int				numBytes;
	unsigned short	receivedBufferLen;
	unsigned int 	bytesToRead;
	sockaddr_in * 	returnAddress;
	Boolean			isPeek;
	sockaddr_in*	fromAddress = (sockaddr_in *)from;
	
	// Are we doing a peek?
	isPeek = (flags & MSG_PEEK);
	
	ThrowIfNil_(from);
	ThrowIf_(fromLen < sizeof(sockaddr_in));
	ThrowIfNil_(buffer);

	// If nothing has been received, donÕt try to read
	if ((fPendingUDPReceives == 0) && (NULL == fPeekBuffer))
		return EWOULDBLOCK;

	// 4 different situations here:
	//		1)  We are doing a peek & havenÕt done one before Ñ> read into buffer
	//		2)  We are doing a peek & have done one before Ñ> donÕt read
	//		3)  We are doing a read & we havenÕt done a peek Ñ> read directly
	//		4)  We are doing a read & weÕve done a peek Ñ> read from buffer & clear buffer
	Try_{
		if (isPeek && (NULL == fPeekBuffer)) {
			fPeekBuffer = (PeekBuffer *)malloc(fBufferSize);
			ThrowIfNil_(fPeekBuffer);
			numBytes = UDPReadIntoBuffer(fPeekBuffer, fBufferSize, from);
			fPeekBuffer->from = *from;
		}
		
		else if (isPeek && (NULL != fPeekBuffer)) {
			numBytes = MIN(len, fBufferSize);
			::BlockMoveData(&(fPeekBuffer->data), buffer, numBytes);
			*from = fPeekBuffer->from;
		}
		
		else if (!isPeek && (NULL == fPeekBuffer)) {
			numBytes = UDPReadIntoBuffer(buffer, len, from);
		}
		
		else {	// !isPeek && (NULL != fPeekBuffer)
			numBytes = MIN(len, fBufferSize);
			::BlockMoveData(&(fPeekBuffer->data), buffer, numBytes);
			*from = fPeekBuffer->from;
			free(fPeekBuffer);
			fPeekBuffer = NULL;
		}
	}
	Catch_(inErr) {
		return -1;
	}
	
	fromLen = from->sa_len;
	return numBytes;
}


int MacTCPStream::SetSocketOption(int level, int optname, const void *optval)
{ 
	// No applicable options.
	return 0;
}


int MacTCPStream::BytesAvailable(size_t& bytesAvailable)
{
	AcquirePB 	acqPB(SOCK_STREAM, TCPStatus, this);
	
	if ( acqPB.fApb == NULL )
		return -1;
		
	if (((MacTCPDriver*)gNetDriver)->MakeSyncCall(acqPB.fApb) != noErr)
		return -1;

	if (acqPB.fApb->pb.tcpPB.ioResult != noErr)
		return -1;
	
	bytesAvailable = acqPB.fApb->pb.tcpPB.csParam.status.amtUnreadData;
	return noErr;
}


#ifdef PROFILE
#pragma profile off
#endif
