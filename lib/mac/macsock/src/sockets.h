#pragma once
// sockets.h
#include "CNetwork.h"
#include "macsock.h"
#include <PP_Types.h>
#include <errno.h>
class CNetStream;
class CDNSObject;
class MacTCPDriver;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/*************************************************************************************
 * MacSocket
 * Encapsulates operations on individual sockets, and socket table management
 * Sockets operate on top of tcp streams
 *************************************************************************************/
class MacSocket	{
public:
// •• static

	// Creates a new socket. Sets errno
	static Int16 CreateSocket(int domain, int type, int protocol);
	// Use in socket routines to return an error instead of errno = ENOBUF, return -1
	static int   SocketError(int err) {errno = err; return err == 0 ? 0 : -1;}
	// Socket table
	static MacSocket*	GetSocket(int socketID) { return (((socketID < MAX_SOCKETS) && (socketID >= 0))? 
													sSockets[socketID] : NULL);};

// •• constructors
	
	MacSocket(Int16 index, int type);	// Index is pointer to a free place in the socket table
	~MacSocket();			// Never use delete. Call Destroy

// •• Netscape select mechanism
	void SetReadSelect() {fReadSelect = TRUE;};
	void ClearReadSelect() {fReadSelect = FALSE;};
	void SetConnectSelect() {fConnectSelect = TRUE;};
	void ClearConnectSelect() {fConnectSelect = FALSE;};
	
// •• DNS lookups
	int StartAsyncDNSLookup(char * hostName, struct hostent ** hoststruct_ptr_ptr);
	void ClearDNSSelect();
	void SetDNSLookupObject(CDNSObject * dns);

// •• sock routines interface
	int Create();
	int Destroy(Boolean doAbort);
	int Connect(InetHost host, InetPort port);
	int Accept(InetHost &host, InetPort &port);
	int Bind(InetHost host, InetPort port);
	int Listen(int backlog);
	int Write (const void * buffer, unsigned int buflen); 
	int Read (void * buffer, unsigned int buflen);
	
	int SetBlocking(Boolean blocking);	// Are we a blocking socket?
	int SetSocketOption(int level, int optname, const void *optval);
	int SocketAvailable(size_t& bytesAvailable);
	Boolean Select(Boolean& readReady,Boolean& writeReady,Boolean& exceptReady);
	int GetPeerName(InetHost& host, InetPort& port);
	int GetSockName(InetHost& host, InetPort& port);
	int SendTo(const void *msg, int len, int flags, InetHost host, InetPort port);
	int ReceiveFrom(void *buffer, int len, int flags, struct sockaddr *from, int& fromLen);
	int UDPSendTo(const void *msg, int len, int flags, InetHost host, InetPort port);
	int UDPReceiveFrom(void *buffer, int len, int flags, struct sockaddr *from, int& fromLen);
// •• stream notifications
	void HadClosingException();
	void HadOpenExcetion();
	void ConnectComplete(Boolean isListen, OSStatus err);
	void AcceptComplete(OSStatus err);
// •• select mechanism, callbacks from streams
	void ReadNotify() {fReadReady = TRUE;};
	void WriteComplete(int err);
	void CloseNotify() {fCloseException = TRUE;};
	void DNSNotify() {fDNSException = TRUE;};

	int					fType;					// Type of socket (stream, udp)

private:
	static MacSocket* sSockets[MAX_SOCKETS];
	
	Int16			fIndex;
	CNetStream *	fStream;
	Boolean			fReadReady;
	Boolean			fConnectException, 	// Exception flags. Set after these operations are completed
					fCloseException,
					fDNSException,
					fListenException;
	UInt32			fSerialNumber;		// This socket’s ID
// Streams and sockets deal with return of the connect/listen call differently
// With streams, you make the call once, and wait for the async return
// With sockets, you make the same call repeatedly, and wait for it to return failure
// This behaviour is abstracted in sockets
	enum ConnectStatus	{ 
		eIdle, 
		
		eConnectInProgress, 	// After calling connect
		eConnectSuccess,
		eConnectFailed,
		eConnected,
		
		eListenInProgress, 		// After calling listen
		eListenSuccess,
		eListenFailed,
		eAcceptInProgress,
		eAcceptSuccess,
		eAcceptFailed
	};

	ConnectStatus fConnectStatus;

	CDNSObject * 		fDNSLookup;				// DNS lookup object
	friend class 		CNetStream;
	friend class 		MacTCPDriver;
	friend class 		OTDriver;
	Boolean 			fBlocking;				// Blocking socket?
	Boolean				fConnectSelect, fReadSelect;
	int					fWriteStatus;			// Are we done yet 0 = done, 1 in progress, <0 error done
	int					fOutstandingWrites;
	static				Boolean		sNotified;	// Do any sockets have notifications
	static 	UInt32 		sCurrentSerialNumber;	// The current ID we should use.
	
};