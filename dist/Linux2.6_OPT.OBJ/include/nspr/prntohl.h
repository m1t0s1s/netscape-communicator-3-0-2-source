#ifndef prntohl_h___
#define prntohl_h___

/*
** Header file used to find the "ntohl" definitions on all supported
** platforms. It's a shame it's not in the same spot everywhere...
*/

#if defined(AIXV3)
#include <netinet/in.h>

#elif defined(BSDI)
#include <machine/endian.h>

#elif defined(HPUX)
#include <netinet/in.h>

#elif defined(IRIX)
#include <netinet/in.h>

#elif defined(LINUX)
#include <netinet/in.h>

#elif defined(OSF1)
#include <machine/endian.h>

#elif defined(SOLARIS)
#include <netinet/in.h>

#elif defined(SUNOS4)
#include <netinet/in.h>

#elif defined(XP_PC)

#elif defined(XP_MAC)
#include "macsock.h"
#endif

#endif /* prntohl_h___ */
