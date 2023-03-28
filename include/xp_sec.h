#ifndef __XP_SEC_h_
#define __XP_SEC_h_

#include "xp_core.h"

/*
** Security related XP functions. Useful stuff that makes app writing
** easier.
*/

XP_BEGIN_PROTOS

struct CERTCertificateStr;

/*
** Return a pretty string that describes the status of the security
** connection using the argument information.
** 	"level" the level of security (HIGH, LOW, or OFF)
** 	"cipher" the name of the cipher
** 	"keySize" the total size of the cipher key
** 	"secretKeySize" the portion of the cipher key that is secret
** The above values can be easily determined by calling
** SSL_SecurityStatus().
*/
extern char *XP_PrettySecurityStatus(int level, char *cipher, int keySize,
				     int secretKeySize);

/*
** Pretty print a certificate into a string. Just include the information
** most likely to be used by an end user. The certificate can be easily
** discovered by calling SSL_PeerCertificate().
*/
extern char *XP_PrettyCertInfo(struct CERTCertificateStr *cert);

/*
** Return a dynamically allocated string which describes what security
** version the security library supports. The returned string describes
** in totality the crypto capabilities of the library.
*/
extern char *XP_SecurityCapabilities(void);

/*
** Return a short staticly allocated string (NOT MALLOC'D) which
** describes what security version the security library supports:
** U.S. or international.
** 	"longForm" is non-zero means the return value should be
**	  in the long format, otherwise in the short format.
*/
extern char *XP_SecurityVersion(int longForm);

XP_END_PROTOS

#endif /* __XP_SEC_h_ */
