#ifndef prsystem_h___
#define prsystem_h___

/*
** API to NSPR functions returning system info...
*/

#include "prmacros.h"

NSPR_BEGIN_EXTERN_C

/* Types of information available via PR_GetSystemInfo(...) */
typedef enum {
    PR_SI_HOSTNAME,
    PR_SI_SYSNAME,
    PR_SI_RELEASE,
    PR_SI_ARCHITECTURE
} PR_SYSINFO;

/* Return the value of a system environment variable */
extern PR_PUBLIC_API(char *) PR_GetEnv(const char *name);

/* Set a system environment variable */
extern PR_PUBLIC_API(int) PR_PutEnv(const char *string);

/* Query information about the current OS */
extern PR_PUBLIC_API(long) PR_GetSystemInfo(PR_SYSINFO cmd, char *buf, 
                                           long count);


NSPR_END_EXTERN_C

#endif /* prsystem_h___ */
