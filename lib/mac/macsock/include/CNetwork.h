#pragma once
// CNetwork.h
// contains abstract interfaces, that should be implemented separately for
// MacTCP and OpenTransport
#include <OpenTptInternet.h>
// All the constants that tune the performance
#define MAX_SOCKETS 40
#define DEFAULT_BUFFER_SIZE	16384	// Default TCP buffer size
#define MAX_SEND_BLOCK 16384		// Max size of a single write
#define SEND_TIMEOUT 90				// Timeout for sending MAX_SEND_BLOCK of data (seconds)
#define OPEN_TIMEOUT 30				// Timeout for waiting for Open (seconds)
#define CLOSE_TIMEOUT 15			// Timeout for closing
#define RECEIVE_TIMEOUT 30			// Timeout for receiving

// DNS Errors
#define	HOST_NOT_FOUND	-1 /* Authoritative Answer Host not found */
#define	TRY_AGAIN	-2 /* Non-Authoritive Host not found, or SERVERFAIL */
#define	NO_RECOVERY	-3 /* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define	NO_DATA		-4 /* Valid name, no data record of requested type */
#define	NO_ADDRESS	-6 /* no address, look for MX record */
extern int h_errno;


// Map our Socket IDs into the Standard Unix socket ID space.
// We can't have Socket IDs equal to stdin, stdout, or stderr.
#define MacToStdSocketID(sID)	(sID)
#define StdToMacSocketID(sID)	(sID)


#include "sockets.h"

#include <OSUtils.h>

class CNetStream;
class CDNSObject;
class MacSocket;
class LList;

enum SocketSelect {	eReadSocket, 		// NET_SOCKET_FD for reading
					eExceptionSocket,	// NET_SOCKET_FD for writing or exception
					eLocalFileSocket, 	// NET_LOCAL_FILE_FD
					eEverytimeSocket	//NET_EVERYTIME_TYPE
};

// Netlib tells us what to select on. This structure encapsulates it
struct CSelectObject
{
	int fID;
	SocketSelect fType; 		
};

/*************************************************************************************
 * CNetworkDriver
 * Abstract driver class.
 * Makes sure that the driver is opened and closed properly
 * Keeps a list of sockets to be processed
 *************************************************************************************/

class CNetworkDriver 	{
public:
	// Creates a network driver, distinguishes between MacTCP and OT
	static	CNetworkDriver *	CreateNetworkDriver();
				CNetworkDriver();
	virtual		~CNetworkDriver();
// General driver interface
	Boolean 		AssertOpen();	// Makes sure that the driver is open
	void			GentlyClose();
	virtual void	SpendTime()=0;
	Boolean			Spin();			// Spins, allowing user to switch. True if cmd-.
	InetPort		GetNextLocalPort();		// Next port to open a connection on
// Stream callbacks
	virtual CNetStream * CreateStream(MacSocket * socket)=0;	// Creates the stream, can throw

// Socket selections. Really callbacks from netlib
	void SetReadSelect(int sID);
	void ClearReadSelect(int sID);
	void SetConnectSelect(int sID);
	void ClearConnectSelect(int sID);
	void SetFileReadSelect(int fID);
	void ClearFileReadSelect(int fID);
	void SetCallAllTheTime();
	void ClearCallAllTheTime();
		
// DNS lookup interface
	int		StartAsyncDNSLookup(char * hostName, 
								struct hostent ** hoststruct_ptr_ptr, 
								int sID);
	void	ClearDNSSelect(int sID);
	virtual int		GetHostName(char *name, int namelen)=0;		// Unix gethostname function
	virtual CDNSObject * StrToAddress(char * hostName)=0;		// Creates a lookup object. You can hold onto it
	virtual CDNSObject * StrToAddress(char * hostName, MacSocket * socket)=0;	// Creates a lookup object for this socket
	virtual char *		 AddressToString(InetHost host)=0;
// Notification of socket selections
	void		AddToSocketQueue(int id, SocketSelect type);
	void		RemoveFromSocketQueue(int id, SocketSelect type);

// Should we call netlib?
	Boolean CanCallNetlib(CSelectObject & selectOn);

	struct hostent fHostEnt;	// Static variable, used to return DNS entries for those silly Unix network routines
	QHdr			fDoneDNSCalls;		// Completed DNS requests. Public because completion routines need it
protected:
	virtual OSErr DoOpenDriver()=0;
	Boolean 	fDriverOpen;
	InetPort fNextLocalPort;	// Next port to allocate locally

private:
	friend struct hostent * gethostbyname(char * name);
// Socket queue interface
	LList *		fSelectQueue;	// Queue of sockets that can be called
};

extern CNetworkDriver * gNetDriver;


/*************************************************************************************
 * CNetStream
 * Abstract driver class.
 * Makes sure that the driver is opened and closed properly
 *************************************************************************************/
class CNetStream	{
public:
	CNetStream(MacSocket * socket);
	virtual ~CNetStream();
	
	// Access
	OSErr	SetBlocking(Boolean doBlock);
	
	// standard socket calls
	virtual int CreateStream()=0;
	virtual int	DestroyStream(Boolean abort)=0;
	
	virtual int	ActiveConnect(InetHost host, InetPort port)=0;
	virtual int	Bind(InetHost host, InetPort port)=0;
	virtual int Listen(int backlog)=0;
	virtual int Accept()=0;
	// Write implementation must call WriteComplete in the socket library
	// It should return 0 if the completion routine will be called, error otherwise
	virtual int Write(const void *buffer, unsigned int buflen, UInt16& bytesWritten)=0;
	virtual int Read(void *buffer, unsigned int buflen, UInt16& bytesRead)=0;

	virtual Boolean Select(Boolean& readReady,Boolean& writeReady,Boolean& exceptReady)=0;
	virtual int GetPeerName(InetHost& host, InetPort& port)=0;
	virtual int GetSockName(InetHost& host, InetPort& port)=0;
	virtual int UDPSendTo(const void *msg, unsigned int len, unsigned int flags, InetHost host, InetPort port, unsigned int& bytesWritten)=0;
	virtual int UDPReceiveFrom(void *buffer, unsigned int len, unsigned int flags, struct sockaddr *from, unsigned int& fromLen)=0;
	virtual int SetSocketOption(int level, int optname, const void *optval)=0;
	virtual int BytesAvailable(size_t& bytesAvailable)=0;

	void SocketDeleted()	{fSocket = NULL;}
protected:
	MacSocket * fSocket;
	Boolean		fBlocking;		// Are we blocking?
};

/*************************************************************************************
 * CDNSObject
 * DNS lookup class
 * Associated with a socket (usually)
 * Lookup gets done asynchronously, and the object gets destroyed inside CNetDriver::SpendTime()
 *************************************************************************************/
enum {kDNSDoneNoErr = 0,	kDNSInProgress = 1};		// Values for fStatus
	
class CDNSObject	{
public:
				CDNSObject();
				CDNSObject(MacSocket * socket) ;
	virtual 	~CDNSObject();
	
	void		SelfDestruct();
	void		StoreResults();
	
	OTResult		fStatus;			// Are we done yet? 0 = done, 1 in progress, <0 error done
	InetHostInfo	fHostInfo;
	MacSocket *		fSocket;
	Boolean			fDeleteSelfWhenDone;
	QElemPtr		fQLink;				// OS queue elements
	CNetworkDriver *fInterruptDriver;	// We need a copy of gNetDriver in here,
										// because it needs to be accessed in interrupt routine
};


// Netlib utility routine for NET_ProcessNet
int SocketSelectToFD_Type(SocketSelect s);

// Internal debugging routines.
void DumpBuffer(const void * buffer, size_t len);
