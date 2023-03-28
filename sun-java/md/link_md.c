#include <string.h>
#include <stdio.h>

#include "nspr_md.h"
#include "sys_api.h"
#include "installpath.h"
#include "path_md.h"
#include "prlink.h"
#include "prmem.h"

#ifdef XP_MAC
#include "prmacos.h"
#endif


/*
 * Save the ld path
 */
void sysSaveLDPath(char *ldp)
{
    putenv(ldp);
}


/*
 * create a string for the dynamic lib open call by adding the
 * appropriate pre and extensions to a filename and the path
 */
void sysBuildLibName(char *holder, int holderlen, char *pname, char *fname)
{
    char *rv;
    rv = PR_GetLibName(pname, fname);
    if (rv) {
	int len = strlen(rv) + 1;
	memcpy(holder, rv, (len > holderlen) ? holderlen : len);
	if (holderlen > 0) {
	    holder[holderlen-1] = 0;
	}
    }
    free(rv);
}


/*
 * Initialize the linker
 */
char *sysInitializeLinker()
{
    const char *nspr_path = PR_GetLibraryPath();
	if(nspr_path) {
    char *buf = malloc(strlen(nspr_path) + 1);
    if (buf) {
        strcpy(buf, nspr_path);
	return buf;
    }
	}
    return 0;
}

/* 
 * Add a shared library to the list
 */
int sysAddDLSegment(char *fn)
{
    PRLibrary* lib = PR_LoadLibrary(fn);
#if GENERATING68K
	return 1;
#else
    return lib != NULL;
#endif
}

long sysDynamicLink(char *sym_p, void* *lib)
{
    return (long) PR_FindSymbolAndLibrary(sym_p, (PRLibrary**)lib);
}

