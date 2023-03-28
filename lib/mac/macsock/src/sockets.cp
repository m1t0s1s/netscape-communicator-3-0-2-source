// Network stuff
#include "CNetwork.h"
#include "sockets.h"
// stdclib
#include <assert.h>
#include <stdlib.h>
// PowerPlant
#include <UException.h>
// NSPR threads
#include "prthread.h"
#include "swkern.h"
#include "prmon.h"

// For HoldMemoryGlue and UnholdMemoryGlue
#include "macutil.h"

#ifdef assert
#undef assert
#endif

//#define SOCKET_DEBUG

#if !defined(DEBUG) && defined(SOCKET_DEBUG)
#error Bruce has forgotten to turn off SOCKET_DEBUG in this file again
#endif

#ifdef DEBUG
#define assert(x) if (!(x)) abort()
#else
#define assert(x) ((void)0)
#endif

#define MAX_OUTSTANDING_WRITES 1

#ifdef PROFILE
#pragma profile on
#endif


void WaitForAsyncIO(void)
{
	if (PR_CurrentThread() != gPrimaryThread) {			// Don’t block main thread
		PR_EnterMonitor(_pr_async_io_intr);
		PR_Wait(_pr_async_io_intr, LL_MAXINT);			// Block until interesting I/O happens
		PR_ExitMonitor(_pr_async_io_intr);
	}
}


// 
// 
// 
// 
struct hostent *macsock_gethostbyaddr(const void *addr, int addrlen, int type)
{
	assert(false);
	return NULL;
}


// Errors returned:
// ENETDOWN - no MacTCP driver
// EPROTONOSUPPORT - bad socket type/protocol
// ENOBUFS - not enough space for another socket, or failure in socket creation routine
int macsock_socket(int domain, int type, int protocol)
{
	int		result;
	
	result = MacSocket::CreateSocket(domain, type, protocol);
	
	if (result < 0)
		return MacSocket::SocketError(result);
	else 
		return MacToStdSocketID(result);
}

// This one is strange, I've guessed what it does
// It sets socket options, but I am not sure of the return values
// Errors:
// EBADF - bad socket id
// EOPNOTSUPP - this op not supported. Should probably implement it
int macsock_ioctl(int sID, unsigned int request, void *value)
{
	MacSocket * s;
	if (!(s = MacSocket::GetSocket(StdToMacSocketID(sID))))
		return MacSocket::SocketError(EBADF);

	switch(request) 
	{
		case FIONBIO:	/* Non-blocking I/O */
			return MacSocket::SocketError(s->SetBlocking(*((Boolean*)value)));
		default :
			return MacSocket::SocketError(EOPNOTSUPP);
	}
}

// Connect
// check the arguments validity
// issue the connect call to the stream
// Errors:
//	EBADF -- bad socket id, bad MacTCP stream
//	EAFNOSUPPORT -- bad address format
//	EADDRINUSE -- we are listening, or duplicate socket
//  EINPROGRESS -- we are connecting right now
//  EISCONN -- already connected
//  ECONNREFUSED -- other side has closed, or open has failed
//  EALREADY -- we are connected
//  EINTR -- user interrupted
int macsock_connect(int sID, struct sockaddr *name, int namelen)
{
	MacSocket * s;
	if (!(s = MacSocket::GetSocket(StdToMacSocketID((sID)))))
		return MacSocket::SocketError(EBADF);
	if (namelen != sizeof(struct sockaddr_in))
		return MacSocket::SocketError(EAFNOSUPPORT);
	// Tell our stream to connect
	struct sockaddr_in * sock_a = (struct sockaddr_in *)name;
	return s->Connect(sock_a->sin_addr.s_addr, sock_a->sin_port);
}

// Errors:
// EBADF -- bad socket id
// EFAULT -- bad buffer pointer
// EPIPE -- connection is closed, or we are not connected
// EINTR -- user interrupted, everything might have been sent
// EIO -- misc io error
// EAGAIN -- we are out of memory right now, try again, we are non-blocking, and write would block
int macsock_write(int sID, const void *buffer, unsigned buflen)
{
	MacSocket * s;
	if (!(s = MacSocket::GetSocket(StdToMacSocketID(sID))))
		return MacSocket::SocketError(EBADF);
	if (buffer == NULL)
		return MacSocket::SocketError(EFAULT);
	if (buflen == 0)
		return 0;
	return s->Write(buffer, buflen);
}

// Errors:
// EBADF -- bad socket id
// EFAULT -- bad buffer
// EPIPE -- bad stream
int macsock_read(int sID, void *buf, unsigned nbyte)
{
	MacSocket * s;
	
	if (!(s = MacSocket::GetSocket(StdToMacSocketID(sID))))
		return MacSocket::SocketError(EBADF);
	if (buf == NULL)
		return MacSocket::SocketError(EFAULT);
	if (nbyte == 0)
		return 0;
	return s->Read(buf, nbyte);
}

// Errors:
// EBADF -- bad socket id
int macsock_close(int sID)
{
	MacSocket * s;
	if (!(s = MacSocket::GetSocket(StdToMacSocketID(sID))))
		return MacSocket::SocketError(EBADF);
	return MacSocket::SocketError(s->Destroy(false));
}

// Errors:
// EBADF -- bad socket id
// EFAULT -- bad address format
int macsock_bind(int sID, const struct sockaddr *name, int namelen)
{
	MacSocket * s;
	if (!(s = MacSocket::GetSocket(StdToMacSocketID(sID))))
		return MacSocket::SocketError(EBADF);
	if ((name == NULL) || (namelen != sizeof(sockaddr_in)))
		return MacSocket::SocketError(EFAULT);
	sockaddr_in * sadress = (sockaddr_in *)name;
	return MacSocket::SocketError(s->Bind(sadress->sin_addr.s_addr, sadress->sin_port));
}

// Errors:
// EBADF -- bad socket id
// EOPNOTSUPP -- socket is already connected, and closing
// EISCONN -- already connected
// EINPROGRESS -- connecting right now
int macsock_listen(int sID, int backlog)
{
	MacSocket * s;
	if (!(s = MacSocket::GetSocket(StdToMacSocketID(sID))))
		return MacSocket::SocketError(EBADF);
	return MacSocket::SocketError(s->Listen(backlog));
}

int macsock_accept(int sID, struct sockaddr *addr, int *addrlen)
{
	MacSocket * s;
	if (!(s = MacSocket::GetSocket(StdToMacSocketID(sID))))
		return MacSocket::SocketError(EBADF);
	*addrlen = sizeof(sockaddr_in);
	struct sockaddr_in * sock_a = (struct sockaddr_in *)addr;
	return s->Connect(sock_a->sin_addr.s_addr, sock_a->sin_port);

}

int macsock_dup(int sID)
{
	assert(FALSE);
	return EBADF;
}

int macsock_getpeername(int sID, struct sockaddr *name, int *namelen)
{
	MacSocket * s;
	if (!(s = MacSocket::GetSocket(StdToMacSocketID(sID))))
		return MacSocket::SocketError(EBADF);
	if ((name == NULL) || (namelen == NULL) || *namelen != sizeof(sockaddr_in))
		return MacSocket::SocketError(EFAULT);
	struct sockaddr_in * sock_a = (struct sockaddr_in *)name;
	sock_a->sin_family = AF_INET;
	sock_a->sin_len = 4;
	return MacSocket::SocketError(s->GetPeerName(sock_a->sin_addr.s_addr, sock_a->sin_port));
}

int macsock_getsockname(int sID, struct sockaddr *name, int *namelen)
{
	MacSocket * s;
	if (!(s = MacSocket::GetSocket(StdToMacSocketID(sID))))
		return MacSocket::SocketError(EBADF);
	if ((name == NULL) || (*namelen <=0))
		return MacSocket::SocketError( EFAULT );
	struct sockaddr_in * sock_a = (struct sockaddr_in *)name;
	sock_a->sin_family = AF_INET;
	sock_a->sin_len = 4;
	return MacSocket::SocketError(s->GetSockName(sock_a->sin_addr.s_addr, sock_a->sin_port));
}


//
//
int macsock_send(int sID, const void *msg, int len, int flags)
{
	return macsock_sendto(sID, msg, len, flags, NULL, 0);
}


//
//
int macsock_sendto(int sID, const void *msg, int len, int flags, struct sockaddr *toAddr, int toLen)
{
	MacSocket *s;
	InetHost	host;
	InetPort	port;
	
	s = MacSocket::GetSocket(StdToMacSocketID(sID));
	
	if (NULL == s)
		return MacSocket::SocketError(EBADF);
	
	if (NULL == toAddr) {
		host = NULL;
		port = 0;
	}
	else {
		sockaddr_in * sadress = (sockaddr_in *)toAddr;
		host = sadress->sin_addr.s_addr;
		port = sadress->sin_port;
	}
	return s->SendTo(msg, len, flags, host, port);
}


// Errors:
// EBADF - bad socket ID
// ENOTCONN - no such connection
int macsock_recv(int sID, void *buf, int len, int flags)
{
	return macsock_recvfrom(sID, buf, len, flags, NULL, 0);
}


// Errors:
// EBADF - bad socket id
int macsock_recvfrom(int sID, void *buf, int len, int flags, struct sockaddr *from, int *fromLen)
{
	MacSocket * s;
	
	s = MacSocket::GetSocket(StdToMacSocketID(sID));
	
	if (s == NULL)
		return MacSocket::SocketError(EBADF);
	
	return s->ReceiveFrom(buf, len, flags, from, *fromLen);	
}


// Unimplemented socket options
int macsock_shutdown(int sID, int how) {assert(FALSE);return EBADF;}
int macsock_getsockopt(int sID, int level, int optname, void *optval,int *optlen) {assert(FALSE);return EBADF;}


// Errors:
// EBADF - bad socket id
// ENOTCONN - socket hasn’t been properly created 
int macsock_setsockopt(int sID, int level, int optname, const void *optval, int optlen) 
{
	MacSocket*	s;
	
	s = MacSocket::GetSocket(StdToMacSocketID(sID));
	if (s == NULL)
		return MacSocket::SocketError(EBADF);
	
	return s->SetSocketOption(level, optname, optval);	
}


//
// Provide the IOCTL(FIORNREAD) functionality.
// Returns 0 for failure, 1 for success.
//
// Should we be doing a macsock_ioctl instead?  If we have more of these
// we probably should.
//
int macsock_socketavailable(int sID, size_t *bytesAvailable)
{
	MacSocket * s;
	size_t		available;
	OSStatus	result;
	
	s = MacSocket::GetSocket(StdToMacSocketID(sID));
	if (s == NULL)
		return 0;
	if (NULL == bytesAvailable)
		return 0;	

	result = s->SocketAvailable(available);
	
	if (result)
		*bytesAvailable = available;
	else
		*bytesAvailable = 0;
		
	return result;
}


// Currently ignores the timeout
int select (int nfds, fd_set *readfds, fd_set *writefds,
               fd_set *exceptfds, struct timeval *timeout)
{
	#pragma ignore (timeout)
	
	int howMany = nfds > MAX_SOCKETS ? MAX_SOCKETS : nfds;
	int ready = 0;
	for (int i=0; i<MAX_SOCKETS; i++)
		if ((readfds && FD_ISSET(i, readfds))
		 	|| (writefds && FD_ISSET(i, writefds))
		 	|| (exceptfds && FD_ISSET(i, exceptfds)))
		{
			Boolean readReady, writeReady, exceptReady;
			readReady = writeReady = exceptReady = FALSE;
			MacSocket * s = MacSocket::GetSocket(StdToMacSocketID(i));
			
			s && s->Select(readReady, writeReady, exceptReady);
			
			if (readfds) {
				if (readReady)
					FD_SET(i, readfds);
				else
					FD_CLR(i, readfds);
			}
			
			if (writefds) {
				if (writeReady)
					FD_SET(i, writefds);
				else
					FD_CLR(i, writefds);
			}
			
			if (exceptfds) {
				if (exceptReady)
					FD_SET(i, exceptfds);
				else
					FD_CLR(i, exceptfds);
			}
			
			if (readReady || writeReady || exceptReady)
				ready++;
		}
	return ready;
}


//=========================================================================================
// MacSocket
//=========================================================================================

Boolean	MacSocket::sNotified = FALSE;
UInt32 MacSocket::sCurrentSerialNumber = 1;
MacSocket* MacSocket::sSockets[MAX_SOCKETS];

// Mimics UNIX socket(int domain, int type, int protocol) call
Int16 MacSocket::CreateSocket(int domain, int type, int protocol)
{
	if (!gNetDriver->AssertOpen())
		return SocketError(ENETDOWN);	// Any better errors when there is no TCP?

	// We only deal with internet domain
	if (domain != AF_INET)
		return SocketError(EPROTONOSUPPORT);
	
	// We only know about tcp & udp
	if (!((type == SOCK_STREAM) || (type == SOCK_DGRAM)))
		return SocketError(EPROTONOSUPPORT);
	
	// Convert default types to specific types.
	if (protocol == 0)  {
		if (type == SOCK_DGRAM)
			protocol = IPPROTO_UDP;
		else if (type == SOCK_STREAM)
			protocol = IPPROTO_TCP;
	}
	
	// Only support default protocol for tcp
	if ((type == SOCK_STREAM)  && (protocol != IPPROTO_TCP))
		return SocketError(EPROTONOSUPPORT);
				
	// Only support default protocol for udp
	if ((type == SOCK_DGRAM)  && (protocol != IPPROTO_UDP))
		return SocketError(EPROTONOSUPPORT);
		
	// Find a place in the socket table
	Int16 sIndex = -1;
	for (int i=3; (i<MAX_SOCKETS) && (sIndex == -1); i++)
		if (sSockets[i] == NULL)
			sIndex = i;

	if (sIndex == -1)
		return SocketError(ENOBUFS);
	
	// Create a socket, we might run out of memory
	MacSocket * volatile s = NULL;
	Try_
	{
		s = new MacSocket(sIndex, type);
		int err = s->Create();
		ThrowIfOSErr_(err);
	}
	Catch_(inErr)
	{
		if (s)
			s->Destroy(TRUE);
		return SocketError(ENOBUFS);
	}
	EndCatch_
	return sIndex;
}

MacSocket::MacSocket(Int16 index, int type)
{
	::HoldMemoryGlue( this, sizeof (*this) );	// Because async completion routines access us
	fIndex = index;
	fBlocking = TRUE;
	fStream = gNetDriver->CreateStream(this);
	assert (sSockets[fIndex] == NULL);
	sSockets[index] = this;
	fReadReady = 
	fConnectException  = fCloseException = fDNSException = fListenException =
	fReadSelect = fConnectSelect = FALSE;
	fDNSLookup = NULL;
	fConnectStatus = eIdle;
	fOutstandingWrites = 0;
	fWriteStatus = 0;
	fType = type;
	fSerialNumber = sCurrentSerialNumber++;	// Needed to uniquely ID a socket.
}

MacSocket::~MacSocket()
{
	sSockets[fIndex] = NULL;
	if (fDNSLookup)
		fDNSLookup->SelfDestruct();
	if (fStream)
		fStream->SocketDeleted();
	// Compliments HoldMemory
	::UnholdMemoryGlue( this, sizeof (*this));
}

 
int MacSocket::StartAsyncDNSLookup(char * hostName, struct hostent ** hoststruct_ptr_ptr)
{
	gNetDriver->SpendTime();	// So that our DNS lookups get processed
	if (fDNSLookup == NULL)
	{
		Try_
		{
			fDNSLookup = gNetDriver->StrToAddress(hostName, this);
			ThrowIfNil_(fDNSLookup);
		}
		Catch_(inErr)
		{
			fDNSLookup = NULL;
			return -1;
		}
		EndCatch_
	}
					
	// DNS lookups might get back right away, give driver time to process
	gNetDriver->SpendTime();
	
	if (fDNSLookup->fStatus == kDNSInProgress)	// In progress
		return EAGAIN;
	else						// Lookup comlete
	{
		if (fDNSLookup->fStatus == kDNSDoneNoErr)
		// success
		{
			fDNSLookup->StoreResults();
			*hoststruct_ptr_ptr = &gNetDriver->fHostEnt;
			fDNSException = FALSE;	// DNS Exception has been processed
			delete fDNSLookup;
			return 0;
		}
		else
		// failure
		{
			delete fDNSLookup;
			fDNSException = FALSE;	// DNS Exception has been processed
			return -1;
		}
	}
}

void MacSocket::SetDNSLookupObject(CDNSObject* dns)
{
	fDNSLookup = dns;
}

void MacSocket::ClearDNSSelect()
{
	if (fDNSLookup)
	{
		fDNSLookup->SelfDestruct();
		fDNSLookup = NULL;
	}
}


// •• sock routines interface
int MacSocket::Create()
{
	return SocketError(fStream->CreateStream());
}

int MacSocket::Destroy(Boolean doAbort)
{
	if (fStream)
	{
		fCloseException = FALSE;
		fSerialNumber = 0;				// Make sure we clear these, 
		fWriteStatus = 0;				// other threads might be watching them
		fStream->DestroyStream(doAbort);
	}
	#ifdef SOCKET_DEBUG
	XP_Trace("Sockets: Destroyed socket %X with stream %X.\n", this, fStream);
	#endif
	
	delete this;
	return 0;	// Always successful
}

int MacSocket::SetBlocking(Boolean blocking)
{
	fBlocking = blocking;
	return 0;
}


Boolean MacSocket::Select(Boolean& readReady,Boolean& writeReady,Boolean& exceptReady)
{
	if (fStream == NULL)
	{
		exceptReady = true;
		return true;
	}

	fStream->Select(readReady,writeReady,exceptReady);
	exceptReady = exceptReady ||
				fConnectException || 
				fCloseException || 
				fDNSException;
	writeReady = writeReady
				&& ( fConnectStatus == eConnected )
				&& ( fOutstandingWrites < MAX_OUTSTANDING_WRITES );
	writeReady = writeReady || fListenException;	// Because you select on read for accept
	if (readReady || writeReady || exceptReady)
		return TRUE;
	return FALSE;
}

// INTERRUPT routine, no callbacks
void MacSocket::ConnectComplete(Boolean isListen, OSStatus err)
{
	fConnectException = TRUE;
	// Listening sockets
	if (isListen)
	{
		if (err == noErr)
			fConnectStatus = eListenSuccess;
		else
			fConnectStatus = eListenFailed;
	}
	else
	{
		if (err == noErr)
			fConnectStatus = eConnectSuccess;
		else
			fConnectStatus = eConnectFailed;
	}
}

/*
	Bridge between two interfaces
	Streams get called with connect only once
	Sockets get called repeatedly until success
	We keep a state machine in fConnectStatus
*/
int MacSocket::Connect(InetHost host, InetPort port)
{
	int err;
	if ( fStream == NULL )
		return SocketError(ECONNREFUSED);

	switch (fConnectStatus)	{
		case eIdle:
			fConnectStatus = eConnectInProgress;
			err = fStream->ActiveConnect(host, port);
			if ( (err == noErr) || (err != EINPROGRESS) )
			{
				fConnectException = FALSE;
				fConnectStatus = eIdle;
				return SocketError(0);
			}
			// Drop to the usual EINPROGRESS state
		case eConnectInProgress:
			// Here, err is EINPROGRESS
			gNetDriver->SpendTime();	// Process the pending completed requests

			if (fBlocking)				// We'll get notified at interrupt time
				while (fConnectStatus == eConnectInProgress)	
					gNetDriver->SpendTime();
			
			
			if (fConnectStatus == eConnectSuccess)
			{
				fConnectStatus = eConnected;
				fConnectException = FALSE;
				return 0;
			}
			else if (fConnectStatus == eConnectInProgress)
			{
				return SocketError(EINPROGRESS);
			}
			else	// CatchAll, status should be eConnectFailed here
			{
				fConnectStatus = eIdle;
				fConnectException = FALSE;
				return SocketError(ECONNREFUSED);
			}

			break;
		case eConnectSuccess:
			fConnectStatus = eConnected;
			fConnectException = FALSE;
			return 0;
		case eConnectFailed:
			fConnectStatus = eIdle;
			fConnectException = FALSE;
			return SocketError(ECONNREFUSED);
		case eListenInProgress:
		case eListenFailed:
		case eListenSuccess:
		case eAcceptInProgress:
		case eAcceptSuccess:
		case eAcceptFailed:
			fConnectStatus = eIdle;
			fConnectException = FALSE;
			return SocketError(EIO);	// Not really sure what to return here
		case eConnected:
			return 0;
	}
	return -1;	// Never reached
}


int MacSocket::Bind(InetHost host, InetPort port)
{
	if ( fStream == NULL )
		return ECONNREFUSED;
	int err;
loop:
	err = fStream->Bind(host, port);
	if (fBlocking && (err == EINPROGRESS))
	{	
		gNetDriver->SpendTime();	// Allow for processing of async notifications
		goto loop;
	}
	return SocketError(err);
}

// Similar to connect
// Can be called only once.
int MacSocket::Listen(int backlog)
{
	int err;
	switch (fConnectStatus)	{
		case eIdle:
			fConnectStatus = eListenInProgress;
			err = fStream->Listen(backlog);
			if ( err != noErr )
				return SocketError(err);
			else
				return SocketError(0);
		case eListenInProgress:
		case eListenSuccess:
		case eListenFailed:
		case eConnectFailed:
		case eConnectSuccess:
		case eConnected:
		case eAcceptInProgress:
		case eAcceptSuccess:
		case eAcceptFailed:
			return SocketError( EIO );
	}
	return SocketError(-1);	// Never reached
}

void MacSocket::AcceptComplete(OSStatus err)
{
	if (err != noErr)
		fConnectStatus = eAcceptFailed;
	else
		fConnectStatus = eAcceptSuccess;
}

int MacSocket::Accept(InetHost &host, InetPort &port)
{
	if ( fStream == NULL )
		return MacSocket::SocketError(ECONNREFUSED);

	gNetDriver->SpendTime();	// Process the pending completed requests
	switch (fConnectStatus)	{
		case eIdle:
			return SocketError( EPROTO );
		case eListenInProgress:
			return SocketError( EWOULDBLOCK );
		case eListenSuccess:
			fConnectStatus = eAcceptInProgress;
			int err =  fStream->Accept();
			if ( (err == EWOULDBLOCK) && fBlocking )
				while (fConnectStatus == eAcceptInProgress)
					gNetDriver->SpendTime();
			if (fConnectStatus == eAcceptSuccess)
			{
				fStream->GetPeerName(host, port);
				fConnectStatus = eConnected;
				return 0;
			}
			else
				return SocketError(err);
			break;
		case eAcceptSuccess:
			fConnectStatus = eConnected;
			fStream->GetPeerName(host, port);
			return SocketError( 0 );
		case eAcceptFailed:
		case eListenFailed:
			fConnectStatus  = eIdle;
			return SocketError( ECONNREFUSED );
		case eConnectFailed:
		case eConnectSuccess:
			return SocketError ( EIO );
		case eConnected:
			return SocketError ( EALREADY );
	} 
	return SocketError(EIO);
}



void MacSocket::WriteComplete(int err)
{
	fWriteStatus = err;
	fOutstandingWrites--;
//	assert( fOutstandingWrites >= 0 );
	if (fOutstandingWrites < 0)
	{
//		SysBeep(1);
// 		km - I commented out this beep to fix bug #31473.
//			 Mailing attachments over slow connections will
//			 run through this code thousands of times.
//			 For Dogbert we should review all SysBeep calls
//			 and justify their existence.
		fOutstandingWrites = 0;
	}
}

int MacSocket::Write (const void * buffer, unsigned int buflen)
{
	assert(fStream);
	
	if (fStream == NULL )
		return MacSocket::SocketError(EPIPE);

	if ( (buffer == NULL) || ( buflen == 0 ) )
		return SocketError(EFAULT);

	UInt16 bytesWritten;
	int err;

	gNetDriver->SpendTime();
	if (fOutstandingWrites >= MAX_OUTSTANDING_WRITES) 
		if (!fBlocking )
			return SocketError(EWOULDBLOCK);
		else
			while ( fOutstandingWrites >= MAX_OUTSTANDING_WRITES )
				gNetDriver->SpendTime();
				
	if ( (fWriteStatus != 0) && (fWriteStatus != EWOULDBLOCK))
		return SocketError(EPIPE);		// Our last write has failed

loop:	// Loop until we are able to write

	fWriteStatus = 1;
	if ( fOutstandingWrites >= MAX_OUTSTANDING_WRITES )	// Wait
		err = EWOULDBLOCK;
	else
	{
		fOutstandingWrites++;
		err = fStream->Write(buffer, buflen, bytesWritten);
		if (err != 0)
			fOutstandingWrites--;
	}

	if ( fBlocking &&  (err == EWOULDBLOCK) )
	{	
		gNetDriver->SpendTime();	// Allow for processing of async notifications
		goto loop;
	}
	// If we are broken while attempting to write, return an error
	if (err != noErr)
		return SocketError(err);
	// If we are blocking, loop till we are done
	if (fBlocking)
	{
		while (fWriteStatus == 1)
			gNetDriver->SpendTime();
		if (fWriteStatus < 0)
			return SocketError(fWriteStatus);
		else
			return buflen;
	}
	else
		return bytesWritten;
}


int MacSocket::Read(void * buffer, unsigned int buflen)
{
	if ( fStream == NULL )
		return SocketError(ENOTCONN);

	UInt16 bytesRead = 0;
	int err = fStream->Read(buffer, buflen, bytesRead);
			
	if (fBlocking && (err == EWOULDBLOCK))	{	
		// Tricky logic ahead
		// We need to spin.  If this is a read, we spin until the buffer is full.
		// If this is a recv, we read until the read is complete.
		buffer = (void*)((unsigned long)buffer + (unsigned long)bytesRead);
		buflen -= bytesRead;
	loop:	// Exit if: read enough to fill up the buffer || encounter an error
		assert(buflen >= 0);
		if (buflen <= 0)
			return bytesRead;
		err = fStream->Read(buffer, buflen, bytesRead);
		buffer = (void*)((unsigned long)buffer + (unsigned long)bytesRead);
		buflen -= bytesRead;
		
		if (buflen <= 0)				// Even if we got an error, we got the bytes we need
			return bytesRead;
			
		if ((err == noErr) || (err == EWOULDBLOCK))
		{	UInt32		currentSocketID = fSerialNumber;
		
			gNetDriver->SpendTime();			// Allow for processing of async notifications
			
			WaitForAsyncIO();

			// While we were gone, did another thread delete us?  
			// If we were deleted, did another thread reuse the memory?
			if ((fStream == NULL) || (currentSocketID != fSerialNumber))
				return SocketError(ENOTCONN);
			
			goto loop;
		}
		
		// err is something other than noErr & EWOULDBLOCK, bail.
		return SocketError(err);
	}
	
	else	// Nonblocking
	{
		if (err == noErr)
			return bytesRead;
		else
			return SocketError(err);
	}
}


	
int MacSocket::GetPeerName(InetHost& host, InetPort& port)
{
	if (fStream == NULL)
		return SocketError(ENOTCONN);
	else 
		return fStream->GetPeerName(host, port);
}


int MacSocket::GetSockName(InetHost& host, InetPort& port)
{
	if (fStream == NULL)
		return SocketError(ENOTCONN);
	else 
		return fStream->GetSockName(host, port);
}


int MacSocket::SendTo(const void *msg, int len, int flags, InetHost host, InetPort port)
{
	int		result;
		
	if (fStream == NULL)
		return SocketError(ENOTCONN);
		
	else if ((NULL == msg) || (0 == len))		// Do we have anything to send?
		return SocketError(EFAULT);
		
	else switch (fType) {
		case SOCK_STREAM:
				result = Write(msg, len);
				break;
		case SOCK_DGRAM:
				result = UDPSendTo(msg, len, flags, host, port);
				break;
		default:
				assert(false);
	}
	if (result < noErr)	
		return SocketError(result);
	else {
		SocketError(noErr);
		return result;
	}
}


int MacSocket::ReceiveFrom(void *buffer, int len, int flags, struct sockaddr *from, int& fromLen)
{
	int		result;
	
	if (NULL == fStream)
		return SocketError(ENOTCONN);
		
	if (NULL == buffer)
		return SocketError(EFAULT);
	
	// rcv() & rcvfrom() can be called on any type of socket connection.  We need to determine
	// which type of protocol is being used & do the proper type of read.
	switch (fType) {
		case SOCK_STREAM:
				result =  Read(buffer, len);
				break;
		case SOCK_DGRAM:
				result =  UDPReceiveFrom(buffer, len, flags, from, fromLen);
				break;
		default:
				assert(false);
	}
	if (result < noErr)							// Did an error occur?
		return SocketError(result);
	else {
		SocketError(noErr);
		return result;
	}
}


int MacSocket::UDPSendTo(const void *msg, int len, int flags, InetHost host, InetPort port)
{
	OSStatus		status;
	int				bytesWritten = 0;
	UInt32			currentSocketID = fSerialNumber;

	if (fStream == NULL)
		return SocketError(ENOTCONN);
		
	else if ((NULL == msg) || (0 == len))		// Do we have anything to send?
		return SocketError(EFAULT);

loop:		// Spin until we are able to write
	fWriteStatus = 1;
	
	if ( fOutstandingWrites >= MAX_OUTSTANDING_WRITES )	// Wait
		status = EWOULDBLOCK;
	else {
		fOutstandingWrites++;
		status = fStream->UDPSendTo(msg, len, flags, host, port, bytesWritten);
		if (status != 0)
			fOutstandingWrites--;
	}
	if ( fBlocking &&  (status == EWOULDBLOCK) ) {
		
		gNetDriver->SpendTime();	// Allow for processing of async notifications
		
		WaitForAsyncIO();
		
		// While we were gone, was this socket shut down & we were deleted?
		if ((fStream == NULL) || (currentSocketID != fSerialNumber))
			return SocketError(ENOTCONN);
		goto loop;
	}
	
	// We wrote.  Now spin & wait for the completion if we need to.
	if (fBlocking) {
		while (fWriteStatus == 1) {
			gNetDriver->SpendTime();
			WaitForAsyncIO();
		}
		if (fWriteStatus < noErr)
			return SocketError(fWriteStatus);
		else if (currentSocketID != fSerialNumber)			// Socket was closed underneath us.
			return SocketError(ENOTCONN);
		else
			return len;
	}
	else
		return bytesWritten;
	
}


int MacSocket::UDPReceiveFrom(void *buffer, int len, int flags, struct sockaddr *from, int& fromLen)
{
	OSStatus	status;
	UInt32		currentSocketID = fSerialNumber;
	
	while (true) {
		status = fStream->UDPReceiveFrom(buffer, len, flags, from, fromLen);
		
		if (fBlocking && (status == EWOULDBLOCK)) {			// “Block” if we need to
			gNetDriver->SpendTime();						// Allow for processing of async notifications
			
			WaitForAsyncIO();
			
			// While we were gone, was this socket shut down & were we deleted?
			if ((fStream == NULL) || (currentSocketID != fSerialNumber))
				return SocketError(ENOTCONN);
				
			continue;
		}
		else				// If we’re done, get out of Dodge
			break;
	}
	return status;
}


int MacSocket::SetSocketOption(int level, int optname, const void *optval)
{	
	if (fStream == NULL)
		return ENOTCONN;
	else
		return fStream->SetSocketOption(level, optname, optval);
}


int MacSocket::SocketAvailable(size_t& bytesAvailable)
{
	OSStatus	result;
	
	// Careful of the return value here.  0 => failure, 1 => success
	if (fStream == NULL)
		return 0;
	else
		result = fStream->BytesAvailable(bytesAvailable);
	
	return (result == noErr);
}



#ifdef PROFILE
#pragma profile off
#endif
