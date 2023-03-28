#ifdef XP_MAC
#include <stdlib.h>

#include "prmacos.h"
#include "prsystem.h"
#include "macsock.h"
#else
#include "nspr.h"
#endif

#include "mdint.h"

#ifndef XP_MAC

/* Return the value of a system environment variable */
 PR_PUBLIC_API(char *) PR_GetEnv(const char *name)
{
    return getenv(name);
}

#else

typedef struct EnvVariable EnvVariable;

struct EnvVariable {
	char 			*variable;
	char			*value;
	EnvVariable		*next;
};

EnvVariable		*gEnvironmentVariables = NULL;

char *PR_GetEnv(const char *name)
{
	EnvVariable 	*currentVariable = gEnvironmentVariables;

	while (currentVariable) {
		if (!strcmp(currentVariable->variable, name))
			return currentVariable->value;
			
		currentVariable = currentVariable->next;
	}

	return NULL;
}

#endif


#ifndef XP_MAC
/* Set a system environment variable */
PR_PUBLIC_API(int) PR_PutEnv(const char *string)
{
    return putenv((char*)string);
}

#else

int PR_PutEnv(const char *string)
{
	EnvVariable 	*currentVariable = gEnvironmentVariables;
	char			*variableCopy,
					*value,
					*current;
	int				returnValue;
					
	variableCopy = strdup(string);
	
	if (variableCopy == NULL)
		return FALSE;
	
	current = variableCopy;
	while (*current != '=')
		current++;

	*current = 0;
	current++;

	value = current;

	while (currentVariable) {
		if (!strcmp(currentVariable->variable, variableCopy))
			break;
		
		currentVariable = currentVariable->next;
	}

	if (currentVariable == NULL) {
		currentVariable = (EnvVariable *)malloc(sizeof(EnvVariable));
		
		if (currentVariable == NULL) {
			returnValue = FALSE;
			goto done;
		}
		
		currentVariable->variable = strdup(variableCopy);
		currentVariable->value = strdup(value);
		currentVariable->next = gEnvironmentVariables;
		gEnvironmentVariables = currentVariable;
	}
	
	else {
		free(currentVariable->value);
		currentVariable->value = strdup(current);
	}
	
	returnValue = TRUE;

done:
	free(variableCopy);
	
	return returnValue;
}
#endif


/* Query information about the current OS */
PR_PUBLIC_API(long) PR_GetSystemInfo(PR_SYSINFO cmd, char *buf, long count)
{
    long len = 0;

    switch(cmd) {
      /* Return the  unqualified hostname */
      case PR_SI_HOSTNAME:
        if (gethostname(buf, (int)count) == 0) {
            while( buf[len] && len < count ) {
                if( buf[len] == '.' ) {
                    buf[len] = '\0';
                    break;
                }
                len += 1;
            }    
        }
        break;

      /* Return the operating system name */
      case PR_SI_SYSNAME:
        len = _MD_GetOSName(buf, count);
        break;

      /* Return the version of the operating system */
      case PR_SI_RELEASE:
        len = _MD_GetOSVersion(buf, count);
        break;

      /* Return the architecture of the machine (ie. x86, mips, alpha, ...)*/
      case PR_SI_ARCHITECTURE:
        len = _MD_GetArchitecture(buf, count);
        break;
    }
    return len;
}

