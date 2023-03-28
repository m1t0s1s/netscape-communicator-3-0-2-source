// Necessary for FE stuff
//#include "xp_mcom.h"
//#include "cppcntxt.h"
//#include "fe_proto.h"

#define CMD_PERIOD	// do we want cmd-period interrupts?
// stdc
#include <stdlib.h>
#include <assert.h>

// MacOS
#ifdef CMD_PERIOD
#include <Script.h>
#endif

// Network
#include "CNetwork.h"
#include "otdriver.h"
#include "mactcpdriver.h"


#ifdef PROFILE
#pragma profile on
#endif
#ifdef assert
#undef assert
#endif

#ifdef DEBUG
#define assert(x) if (!x) abort()
#else
#define assert(x) ((void)0)
#endif

#ifdef CMD_PERIOD

// A mess that figures out if cmd-. was pressed
/* Borrowed from tech note 263 */

#define kMaskModifiers  	0xFE00     	// we need the modifiers without the
                                   		// command key for KeyTrans
#define kMaskVirtualKey 	0x0000FF00 	// get virtual key from event message
                                   		// for KeyTrans
#define kUpKeyMask      	0x0080
#define kShiftWord      	8          	// we shift the virtual key to mask it
                                   		// into the keyCode for KeyTrans
#define kMaskASCII1     	0x00FF0000 	// get the key out of the ASCII1 byte
#define kMaskASCII2     	0x000000FF 	// get the key out of the ASCII2 byte
#define kPeriod         	0x2E       	// ascii for a period
static Boolean CmdPeriod()
{
	EvQElPtr		eventQ = (EvQElPtr)LMGetEventQueue()->qHead;

	while (eventQ != NULL)
	{
		EventRecord * theEvent =  (EventRecord *)&eventQ->evtQWhat;

	  	UInt16    keyCode;
	  	UInt32	  state;
	  	long     virtualKey, keyInfo, lowChar, highChar, keyCId;
	  	Handle   hKCHR;
		Ptr 		KCHRPtr;
	
	
		if (((*theEvent).what == keyDown) || ((*theEvent).what == autoKey)) {
	
			// see if the command key is down.  If it is, find out the ASCII
			// equivalent for the accompanying key.
	
			if ((*theEvent).modifiers & cmdKey ) {
	
				virtualKey = ((*theEvent).message & kMaskVirtualKey) >> kShiftWord;
				// And out the command key and Or in the virtualKey
				keyCode    = short(((*theEvent).modifiers & kMaskModifiers) | virtualKey);
				state      = 0;
	
	
				hKCHR = nil;  /* set this to nil before starting */
			 	KCHRPtr = (Ptr)GetScriptManagerVariable(smKCHRCache);
	
				if ( !KCHRPtr ) {
					keyCId = GetScriptVariable(short(GetScriptManagerVariable(smKeyScript)), smScriptKeys);
	
					hKCHR   = GetResource('KCHR',short(keyCId));
					KCHRPtr = *hKCHR;
				}
				if (KCHRPtr) {
					keyInfo = KeyTranslate(KCHRPtr, keyCode, &state);
					if (hKCHR)
						ReleaseResource(hKCHR);
				} else
					keyInfo = (*theEvent).message;
	
				lowChar =  keyInfo &  kMaskASCII2;
				highChar = (keyInfo & kMaskASCII1) >> 16;
				if (lowChar == kPeriod || highChar == kPeriod)
					return TRUE;
	
			}  // end the command key is down
		}  // end key down event
		eventQ = (EvQElPtr)eventQ->qLink;
	}
	return FALSE;
}

#endif

#ifndef WAIT_TO_QUIT
extern void WaitToQuit();

void WaitToQuit()
{
	EventRecord		macEvent;
	Boolean	gotEvent = ::WaitNextEvent(everyEvent, &macEvent, 6,
										NULL);
	if (!gotEvent)	// Only process mouseDown events.
		return;

	if (IsDialogEvent(&macEvent) || (macEvent.what == updateEvt))
	{
		DialogPtr theDialog;
		short itemHit;
		DialogSelect(&macEvent, &theDialog, &itemHit);
	}
	WindowPtr	macWindowP;
	Int16	thePart = ::FindWindow(macEvent.where, &macWindowP);
	
	switch (thePart) {
		case inMenuBar:
			long	menuChoice =  MenuSelect(macEvent.where);
			// Incomplete, needs to do context switch here
			break;
		case inSysWindow:
			::SystemClick(&macEvent, macWindowP);
           	break;
        case inDrag:
        	Rect limitRect;
        	limitRect.top = limitRect.left = 0;
        	limitRect.bottom = limitRect.right = 1000;
        	if (macWindowP)
				DragWindow(macWindowP, macEvent.where, &limitRect);
			break;
      default:
        	break;
	}
}
#endif
int h_errno;
// Unix routines
unsigned long inet_addr(const char * address)
{
	unsigned long numAddress;
	Try_
	{
		CDNSObject * dns = gNetDriver->StrToAddress((char*)address);
		while (dns->fStatus == kDNSInProgress)
		{
			if (gNetDriver->Spin())
			{
				h_errno = TRY_AGAIN;
				dns->SelfDestruct();
				return 0;
			}
		}
		if (dns->fStatus == kDNSDoneNoErr)
			numAddress = dns->fHostInfo.addrs[0];
		else
		{
			address = 0;
			h_errno = dns->fStatus;
		}
		delete dns;
	}
	Catch_(inErr)
	{
	}
	EndCatch_
	return numAddress;
}

char *inet_ntoa(struct in_addr in)
{
	return gNetDriver->AddressToString(in.s_addr);
}


struct hostent * gethostbyname(char * name)
{
	Try_
	{
		CDNSObject * dns = gNetDriver->StrToAddress(name);

		while (dns->fStatus == kDNSInProgress)
		{
			if (gNetDriver->Spin())
			{
				h_errno = TRY_AGAIN;		
				dns->SelfDestruct();
				return NULL;
			}
		}
		
		if (dns->fStatus != kDNSDoneNoErr)	// Error return
		{
			delete dns;
			return NULL;
		}
		else
		{
			dns->StoreResults();
			delete dns;
		}
		return &gNetDriver->fHostEnt;
	}
	Catch_(inErr)
	{
		return NULL;
	}
	EndCatch_
	return NULL;
}

int gethostname (char *name, int namelen)
{
	if (!gNetDriver->AssertOpen())
		return EINVAL;
	return gNetDriver->GetHostName(name, namelen);
}


CNetworkDriver * gNetDriver = NULL;

// CNetworkDriver

CNetworkDriver * CNetworkDriver::CreateNetworkDriver()
{
	Boolean hasOTTCPIP = FALSE;
	Boolean hasOT;
	long result;
	
	OSErr err = ::Gestalt(gestaltOpenTpt, &result);
	
	hasOT = err == noErr && 
			((result & gestaltOpenTptPresent) != 0);
	hasOTTCPIP = hasOT  &&
			((result & gestaltOpenTptTCPPresent) != 0);

#ifdef __powerc
// For now, do the OT/TCP switch only on PowerPC machines
	if (hasOTTCPIP)
		gNetDriver = new OTDriver;
	else
		gNetDriver = new MacTCPDriver;
#else
	gNetDriver = new MacTCPDriver;
#endif
	return gNetDriver;
}

CNetworkDriver::CNetworkDriver()
{
	fDriverOpen = FALSE;
	fSelectQueue = new LList(sizeof(CSelectObject));
	fHostEnt.h_name = (char*)malloc(255);
	fHostEnt.h_aliases = NULL;
	fHostEnt.h_addrtype = AF_INET;
	fHostEnt.h_length = sizeof (long);
	fHostEnt.h_addr_list = NULL;
	fDoneDNSCalls.qHead = fDoneDNSCalls.qTail = NULL;
	fNextLocalPort = 1024 + 1000 * abs(::Random() )/32000;
}

CNetworkDriver::~CNetworkDriver() 
{
	delete fSelectQueue;
}

Boolean CNetworkDriver::AssertOpen()
{
	if (fDriverOpen)
		return TRUE;
	else
		return (DoOpenDriver() == noErr);
}

// True if we should interrupt
Boolean CNetworkDriver::Spin()
{
	SpendTime();

#ifdef CMD_PERIOD
	if (CmdPeriod())
		return true;
#endif

	return false;
}

/* ALEKS We need an algorithm that allocates local ports in sequence 
	This is because if we try to connect with the same local port to the
	same http host succesively, we will be rejected because of lingering TCP
	problem. */
UInt16	CNetworkDriver::GetNextLocalPort()
{
	fNextLocalPort++;
	if (fNextLocalPort > 5900)
		fNextLocalPort = 1000;
	return fNextLocalPort;
}


void CNetworkDriver::SetReadSelect(int sID)
{
	MacSocket*	s = MacSocket::GetSocket(StdToMacSocketID(sID));
	if (s == NULL)
	{
		assert(FALSE);
		return;
	}
	AddToSocketQueue(sID, eReadSocket);
	s->SetReadSelect();
}

void CNetworkDriver::ClearReadSelect(int sID)
{
	MacSocket*	s = MacSocket::GetSocket(StdToMacSocketID(sID));
	if (s == NULL)	// This happens, when we are shutting down netlib
		return;
	RemoveFromSocketQueue(sID, eReadSocket);
	s->ClearReadSelect();	
}

void CNetworkDriver::SetConnectSelect(int sID)
{
	MacSocket*	s = MacSocket::GetSocket(StdToMacSocketID(sID));
	if (s == NULL)
	{
		assert(FALSE);
		return;
	}
	AddToSocketQueue(sID, eExceptionSocket);
	s->SetConnectSelect();		
}

void CNetworkDriver::ClearConnectSelect(int sID)
{
	MacSocket*	s = MacSocket::GetSocket(StdToMacSocketID(sID));
	if (s == NULL)
	{
		assert(FALSE);
		return;
	}
	RemoveFromSocketQueue(sID, eExceptionSocket);
	s->ClearConnectSelect();		
}

void CNetworkDriver::SetFileReadSelect(int fID)
{
	AddToSocketQueue(fID, eLocalFileSocket);
}

void CNetworkDriver::ClearFileReadSelect(int fID)
{
	RemoveFromSocketQueue(fID, eLocalFileSocket);
}

void CNetworkDriver::SetCallAllTheTime()
{
	AddToSocketQueue(-1, eEverytimeSocket);
}

void CNetworkDriver::ClearCallAllTheTime()
{
	RemoveFromSocketQueue(-1, eEverytimeSocket);
}

int CNetworkDriver::StartAsyncDNSLookup(char * host_port, struct hostent ** hoststruct_ptr_ptr, int sID)
{
	MacSocket*	s = MacSocket::GetSocket(StdToMacSocketID(sID));
	if (s == NULL)
		return 0;
	else
	{
		AddToSocketQueue(sID, eExceptionSocket);
		return s->StartAsyncDNSLookup(host_port, hoststruct_ptr_ptr);
	}
}

void CNetworkDriver::ClearDNSSelect(int sID)
{
	MacSocket*	s = MacSocket::GetSocket(StdToMacSocketID(sID));
	if (s != NULL)
		s->ClearDNSSelect();
}

Boolean CNetworkDriver::CanCallNetlib(CSelectObject & selectOn)
{
	if (fSelectQueue->GetCount() == 0)
		return FALSE;

	int howMany = fSelectQueue->GetCount();
	// Loop through everything in the select queue, see if there is anything to select on
	Boolean hasBusySocket = FALSE;
	Boolean deleteFromQueue;
	for (int i = 1; hasBusySocket == FALSE && i<=howMany; i++)
	{
		fSelectQueue->FetchItemAt(1, &selectOn);
		fSelectQueue->RemoveItemsAt(1,1);
		if ((selectOn.fType == eLocalFileSocket) || 
			(selectOn.fType == eEverytimeSocket))
		{
			hasBusySocket = TRUE;
			deleteFromQueue = FALSE;
		}
		else // selectOn.fType is socket, see if it is avaliable
		{
			MacSocket * s = MacSocket::GetSocket(selectOn.fID);
			if (s == NULL)	// Socket got deleted ?
				deleteFromQueue = TRUE;
			else
			{
				deleteFromQueue = FALSE;
				Boolean readReady,writeReady,exceptReady;
				s->Select(readReady,writeReady,exceptReady);
				if (selectOn.fType == eExceptionSocket)
				{
					if (writeReady || exceptReady)
						hasBusySocket = TRUE;
				}
				else	// selectOn.fType == eReadSocket
					if (readReady || exceptReady)
						hasBusySocket = TRUE;
			}
		}
		if (!deleteFromQueue)
			fSelectQueue->InsertItemsAt(1, howMany, &selectOn);
	}
	return hasBusySocket;
}

void CNetworkDriver::AddToSocketQueue(int id, SocketSelect type)
{
	CSelectObject s;
	s.fID = StdToMacSocketID(id);
	s.fType = type;
	LListIterator iter(*fSelectQueue,iterate_FromStart);
	CSelectObject sOld;
	while (iter.Next(&sOld))	// Never duplicate the stuff in the queue
		if ((sOld.fID == s.fID) && (sOld.fType == s.fType))
			return;
	fSelectQueue->InsertItemsAt(1, fSelectQueue->GetCount(), &s);
}

void CNetworkDriver::RemoveFromSocketQueue(int id, SocketSelect type)
{
	CSelectObject s;
	s.fID = StdToMacSocketID(id);
	s.fType = type;
	LListIterator iter(*fSelectQueue,iterate_FromStart);
	CSelectObject sOld;
	while (iter.Next(&sOld))	// Never duplicate the stuff in the queue
		if ((sOld.fID == s.fID) && (sOld.fType == s.fType))
			fSelectQueue->Remove(&sOld);
}

//================================================================================
// CDNSObject
//================================================================================
CDNSObject::CDNSObject()
{
	fStatus = kDNSInProgress;
	fDeleteSelfWhenDone = FALSE;
	fSocket = NULL;
	fInterruptDriver = gNetDriver;
	fQLink = NULL;
}

CDNSObject::CDNSObject(MacSocket * socket)
{
	fStatus = kDNSInProgress;
	fDeleteSelfWhenDone = FALSE; 
	fSocket = socket;
	fInterruptDriver = gNetDriver;
	socket->SetDNSLookupObject(this);
	fQLink = NULL;
}

CDNSObject::~CDNSObject()
{
	if (fSocket)
		fSocket->SetDNSLookupObject(NULL);
	::Dequeue((QElemPtr)&fQLink,&gNetDriver->fDoneDNSCalls);
}

void CDNSObject::SelfDestruct()
{
	if (fSocket)
		fSocket->SetDNSLookupObject(NULL);
	fDeleteSelfWhenDone = TRUE;
	fSocket = NULL;
}

static InetHostInfo sHostInfo;
static InetHost * sAddresses[kMaxHostAddrs+1];

void CDNSObject::StoreResults()
{
	strcpy(gNetDriver->fHostEnt.h_name, fHostInfo.name);
	::BlockMoveData(&fHostInfo, &sHostInfo, sizeof(InetHostInfo));
	gNetDriver->fHostEnt.h_name = sHostInfo.name;
	int i;
	for (i=0; i<kMaxHostAddrs && sHostInfo.addrs[i] != 0; i++)	// Copy just the right amount
		sAddresses[i] = &sHostInfo.addrs[i];
	sAddresses[i] = NULL;	
	gNetDriver->fHostEnt.h_addr_list = (char **)sAddresses;
}

//================================================================================
// CNetStream
//================================================================================

CNetStream::CNetStream(MacSocket * socket)
{
	ThrowIfNil_(socket);
	fSocket = socket;
	fBlocking = fSocket->fBlocking;
}

CNetStream::~CNetStream()
{
	if ( fSocket )
		fSocket->fStream = NULL;
}

OSErr CNetStream::SetBlocking(Boolean doBlock)
{
	fBlocking = doBlock;
	return noErr;
}


void DumpBuffer(const void * buffer, size_t len)
{
	// Use this when debugging & you need to dump a buffer to the trace log.
	//
	unsigned char *		bufferCursor;
	int					inLineCount;
	char				hexVersion[68];
	char				stringVersion[36];
	char *				hexCursor;
	char *				stringCursor;
	
	if (len > 100)
		return;
		
	inLineCount = 0;
	sprintf(hexVersion, "");
	sprintf(stringVersion, "");
	hexCursor = hexVersion;
	stringCursor = stringVersion;
	for (bufferCursor = (unsigned char *)buffer; ((size_t)bufferCursor < ((size_t)buffer + len)); bufferCursor++) {
		sprintf(hexCursor, "%02x", (unsigned char *)*bufferCursor);	// Add the hext digit
		hexCursor += 2;
		
		if ((*bufferCursor > 31) && (*bufferCursor < 127))			// If itÕs printable,
			sprintf(stringCursor, "%c", (char *)*bufferCursor);		// add the character
		else
			sprintf(stringCursor, "×");								// OTW add a token
		stringCursor += 1;
			
		inLineCount++;												// Keep track of position in line
		
		if ((inLineCount % 4) == 0) {								// Add a space between longs
			sprintf(hexCursor, " ");
			hexCursor += 1;
		}
		
		if ((inLineCount == 16) || ((size_t)bufferCursor == (((size_t)buffer + len - 1)))) {
	#ifdef TCP_DEBUG
			XP_Trace("     %-40s    %-20s\n", hexVersion, stringVersion);
	#endif
			inLineCount = 0;
			sprintf(hexVersion, "");
			sprintf(stringVersion, "");
			hexCursor = hexVersion;
			stringCursor = stringVersion;
		}
	}
}

#ifdef PROFILE
#pragma profile off
#endif
