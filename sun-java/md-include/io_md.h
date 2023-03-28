#ifndef sun_java_io_md_h___
#define sun_java_io_md_h___

#include "prosdep.h"
#include "prfile.h"
#include "sys_api.h"

#ifdef XP_UNIX
#include <dirent.h>
#include <fcntl.h>
#include <sys/param.h>
#endif

#ifdef XP_MAC
typedef long off_t;
#endif

#ifdef SCO_SV
#include <sys/socket.h>
#endif

#if defined(SCO_SV) || defined(UNIXWARE)
#ifndef S_ISSOCK
#define S_ISSOCK(mode) 0
#endif
#endif

#endif /* sun_java_io_md_h___ */
