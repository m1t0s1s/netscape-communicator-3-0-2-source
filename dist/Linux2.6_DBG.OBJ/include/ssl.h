#ifndef __ssl_h_
#define __ssl_h_

/*
** This file contains prototypes for the SSL functions.
*/
#ifdef XP_WIN
#include "winsock.h"
extern SOCKET __ssl_last_read_fd;
#endif

#ifdef XP_UNIX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#ifdef XP_MAC
#include "macsock.h"
#endif

#include "seccomon.h"

/* Forward references */
struct CERTCertificateStr;
struct SECItemStr;
struct SECKEYPublicKeyStr;
struct SECKEYPrivateKeyStr;
struct CERTDistNamesStr;
struct CERTCertificateListStr;

#ifdef FORTEZZA
#include "ntypes.h" /* get MWContext */
#endif

SEC_BEGIN_PROTOS

/*
** SSL socket api. This api emulates the defacto standard "socket" api.
*/

/*
** Socket creation/connection/control functions. Standard BSD stuff
*/
extern int SSL_Connect(int, const void *, int);
extern int SSL_Accept(int, void *, int *);
extern int SSL_Socket(int, int, int);
extern int SSL_Bind(int, const void *, int);
extern int SSL_Listen(int, int);
extern int SSL_Shutdown(int, int);
extern int SSL_Ioctl(int, int, void*);
extern int SSL_Close(int);

/* Socket i/o */
extern int SSL_Recv(int, void *, int, int);
extern int SSL_Send(int, const void *, int, int);
extern int SSL_Read(int, void *, int);
extern int SSL_Write(int, const void *, int);

/* Socket state functions */
extern int SSL_GetPeerName(int, void *, int *);
extern int SSL_GetSockName(int, void *, int *);
extern int SSL_GetSockOpt(int, int, int, void *, int *);
extern int SSL_SetSockOpt(int, int, int, const void *, int);

/*
** SSL additions to the socket API
*/
extern int SSL_ReadRecord(int fd, struct SECItemStr *result);

/*
** Import an already created file descriptor into SSL. This allows fd to
** be used with the SSL i/o calls (read/write, etc.). The ssl socket will
** be created with the same security parameters as the ssl socket passed in.
*/
extern int SSL_ImportFd(int s, int fd);

/*
** Import an already created file descriptor into SSL. The ssl socket
** returned will be SSL enabled.
*/
extern int SSL_Import(int fd);

#ifdef XP_UNIX
/*
** Just like dup2 in unix, except that oldfd is an ssl socket and newfd
** is the desired new socket. newfd will inherit everything from oldfd,
** including its security, socks, etc.
*/
extern int SSL_Dup2(int oldfd, int newfd);
#endif

#ifdef XP_WINNT
/*
** Need to be able to duplicate a socket so as to be non-inheritable but
** inherit everything else from the oldfd, including its security, socket,
** etc.
*/
extern int SSL_ReplaceSocket(int oldfd, int newfd);
#endif

/*
** Returns non-zero if the SSL library supports domestic encryption
** capabilities. Export-only libraries return 0.
*/
extern int SSL_IsDomestic(void);

/*
** Return a bitvector of the capabilities of the SSL library
** implementation. The value returned is the bitwise-or of the values
** lists below.
*/
extern unsigned long SSL_SecurityCapabilities(void);

#define SSL_SC_RSA		0x00000001L
#define SSL_SC_MD2		0x00000010L
#define SSL_SC_MD5		0x00000020L
#define SSL_SC_RC2_CBC		0x00001000L
#define SSL_SC_RC4		0x00002000L
#define SSL_SC_DES_CBC		0x00004000L
#define SSL_SC_DES_EDE3_CBC	0x00008000L
#define SSL_SC_IDEA_CBC		0x00010000L

/*
** Enable/disable an ssl mode
**
** 	SSL_SECURITY:
** 		enable/disable use of security protocol before connect
**
** 	SSL_SOCKS:
** 		enable/disable use of socks before connect
**
** 	SSL_REQUEST_CERTIFICATE:
** 		require a certificate during secure connect
**
** 	SSL_REQUEST_PASSWORD:
** 		require a password during secure connect
**
**	SSL_ASYNC_WRITES:
**		enable/disable async writes. Normally ssl will loop, waiting
**		for all write data to go out before returning from Send, and
**		Write. Enabling this causes ssl to return on the EWOULDBLOCK
**		error instead of looping.
*/
extern int SSL_Enable(int fd, int which, int on);
extern int SSL_EnableDefault(int which, int on);

/* Enables */
#define SSL_SECURITY			1
#define SSL_SOCKS			2
#define SSL_REQUEST_CERTIFICATE		3
#define SSL_ASYNC_WRITES		5
#define SSL_DELAYED_HANDSHAKE		6
#define SSL_HANDSHAKE_AS_CLIENT		7 /* force accept to hs as client */
#define SSL_HANDSHAKE_AS_SERVER		8 /* force connect to hs as server */
#define SSL_ENABLE_SSL2			9 /* enable ssl v2 (on by default) */
#define SSL_ENABLE_SSL3		       10 /* enable ssl v3 (on by default) */
#define SSL_NO_CACHE		       11 /* don't use the session cache */

/*
** Control ciphers that SSL uses. If on is non-zero then the named cipher
** is enabled, otherwise it is disabled. 
** The which values are defined in sslproto.h (the SSL_CK_* values).
** EnableCipher records user preferences.
** SetPolicy sets the policy according to the policy module.
*/
extern SECStatus SSL_EnableCipher(long which, int enabled);
extern SECStatus SSL_SetPolicy(long which, int policy);

#ifdef FORTEZZA
extern int SSL_IsEnabledGroup(int which);
extern int SSL_EnableGroup(int which, int on);
#define	SSL_GroupRSA		0x01
#define	SSL_GroupDiffieHellman	0x02
#define	SSL_GroupFortezza	0x04
#define SSL_GroupAll		0xff
extern void SSL_FortezzaMenu(MWContext *,int type);
#define SSL_FORTEZZA_CARD_SELECT	1
#define SSL_FORTEZZA_CHANGE_PERSONALITY 2
#define SSL_FORTEZZA_VIEW_PERSONALITY	3
#define SSL_FORTEZZA_CARD_INFO		4
#define SSL_FORTEZZA_LOGOUT		5
void FortezzaSetTimeout(int);
#endif

/*
** Reset the handshake state for fd. This will make the complete SSL
** handshake protocol execute from the ground up on the next i/o
** operation.
*/
extern int SSL_ResetHandshake(int fd, int asServer);

/*
** Force the handshake for fd to complete immediately.  This blocks until
** the complete SSL handshake protocol is finished.
*/
extern int SSL_ForceHandshake(int fd);

/*
** Query security status of socket. *on is set to one if security is
** enabled. *keySize will contain the stream key size used. *issuer will
** contain the RFC1485 verison of the name of the issuer of the
** certificate at the other end of the connection. For a client, this is
** the issuer of the server's certificate; for a server, this is the
** issuer of the client's certificate (if any). Subject is the subject of
** the other end's certificate. The pointers can be zero if the desired
** data is not needed.
*/
extern int SSL_SecurityStatus(int fd, int *on, char **cipher, int *keySize,
			      int *secretKeySize,
			      char **issuer, char **subject);

/* Values for "on" */
#define SSL_SECURITY_STATUS_NOOPT	-1
#define SSL_SECURITY_STATUS_OFF		0
#define SSL_SECURITY_STATUS_ON_HIGH	1
#define SSL_SECURITY_STATUS_ON_LOW	2
#define SSL_SECURITY_STATUS_FORTEZZA	3

/*
** Return the certificate for our SSL peer. If the client calls this
** it will always return the server's certificate. If the server calls
** this, it may return NULL if client authentication is not enabled or
** if the client had no certificate when asked.
**	"fd" the socket "file" descriptor
*/
extern struct CERTCertificateStr *SSL_PeerCertificate(int fd);

/*
** Authenticate certificate hook. Called when a certificate comes in
** (because of SSL_REQUIRE_CERTIFICATE in SSL_Enable) to authenticate the
** certificate.
*/
typedef int (*SSLAuthCertificate)(void *arg, int fd, PRBool checkSig,
				  PRBool isServer);
extern int SSL_AuthCertificateHook(int fd, SSLAuthCertificate f, void *arg);

/* An implementation of the certificate authentication hook */
extern int SSL_AuthCertificate(void *arg, int fd, PRBool checkSig,
			       PRBool isServer);

/*
 * Prototype for SSL callback to get client auth data from the application.
 *	arg - application passed argument
 *	caNames - pointer to distinguished names of CAs that the server likes
 *	pRetCert - pointer to pointer to cert, for return of cert
 *	pRetKey - pointer to key pointer, for return of key
 */
typedef int (*SSLGetClientAuthData)(void *arg,
			     int fd,
			     struct CERTDistNamesStr *caNames,
			     struct CERTCertificateStr **pRetCert,/*return */
			     struct SECKEYPrivateKeyStr **pRetKey);/* return */

/*
 * Set the client side callback for SSL to retrieve user's private key
 * and certificate.
 *	fd - the file descriptor for the connection in question
 *	f - the application's callback that delivers the key and cert
 *	a - application specific data
 */
extern int SSL_GetClientAuthDataHook(int fd, SSLGetClientAuthData f, void *a);

/*
** This is a callback for dealing with server certs that are not authenticated
** by the client.  The client app can decide that it actually likes the
** cert by some external means and restart the connection.
*/
typedef int (*SSLBadCertHandler)(void *arg, int fd);
extern int SSL_BadCertHook(int fd, SSLBadCertHandler f, void *arg);

/*
** Configure ssl for running a secure server. Needs the signed
** certificate for the server and the servers private key. The arguments
** are copied.
*/
extern int SSL_ConfigSecureServer(struct SECItemStr *signedCertItem,
				  struct SECKEYPrivateKeyStr *privateKey,
				  struct CERTCertificateListStr *certChain,
				  struct CERTDistNamesStr *caNames);

/*
** Configure a secure servers session-id cache. Define the maximum number
** of entries in the cache, the longevity of the entires. These values
** can be zero, and if so the implementation will choose defaults.
*/
extern int SSL_ConfigServerSessionIDCache(int maxCacheEntries,
					  time_t timeout, time_t ssl3_timeout,
					  const char *directory);

/*
** Set the global accept hook that SSL will use in place of the
** underlying OS's accept call.
*/
typedef int (*SSLAcceptFunc)(int fd, struct sockaddr *a, int *ap);
extern void SSL_AcceptHook(SSLAcceptFunc func);

/*
** Set the callback on a particular socket that gets called when we finish
** performing a handshake.
*/
typedef int (*SSLHandshakeCallback)(int fd, void *client_data);
extern int SSL_HandshakeCallback(int fd, SSLHandshakeCallback cb,
				 void *client_data);

/*
** For the server, request a new handshake.  For the client, begin a new
** handshake.
*/
extern int SSL_RedoHandshake(int fd);

/*
** Setup default certificates that ssl will use to verify server
** certificates.
*/
extern int SSL_SetupDefaultCertificates(void);

/*
** Return 1 if the socket is direct, 0 if not, -1 on error
*/
extern int SSL_CheckDirectSock(int s);

/*
** A cousin to SSL_Bind, this takes an extra arg: dsthost, so we can
** set up sockd connection. This should be used with socks enabled.
*/
extern int SSL_BindForSockd(int s, const void *sa, int salen, long dsthost);

/*
** Configure ssl for using socks.
*/
extern int SSL_ConfigSockd(int fd, unsigned long addr, short port);

/*
** Prepare SSL for secure client usage. Read in the key file and also
** read in the certificate file for the key.
** 	"keyDir" the pathname for the directory that contains the key file
** 	  and the certificate file.
** 	"pw" the password that will be used to decrypt the key file
** The files opened are keydir/.key.der and keydir/.keycert.der
*/
extern int SSL_ConfigClientAuth(char *keyDir, char *pw);
extern int SSL_GetClientCertificate(void *notused, struct SECItemStr *result);
extern struct SECKEYPrivateKeyStr *SSL_GetClientPrivateKey(void *notused);

/*
 * Allow the application to pass a URL or hostname into the SSL library
 */
extern int SSL_SetURL(int fd, char *url);

extern char *SSL_GetMessage(int error);

extern void SSL3_Init(void);

/*
** Return the number of bytes that SSL has waiting in internal buffers.
** Return 0 if security is not enabled.
*/
extern int SSL_DataPending(int fd);

/*
** Invalidate the SSL session associated with fd.
*/
extern int SSL_InvalidateSession(int fd);

/*
** Clear out the SSL session cache.
*/
extern void SSL_ClearSessionCache(void);

/*
** Set peer information so we can correctly look up SSL session later.
** You only have to do this if you're tunneling through a proxy.
*/
extern int SSL_SetSockPeerID(int s, char *peerID);

SEC_END_PROTOS

#endif /* __ssl_h_ */
