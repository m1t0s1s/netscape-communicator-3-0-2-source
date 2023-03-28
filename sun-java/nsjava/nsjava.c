#include <oobj.h>
#include <tree.h>           /* verifyclasses */
#include <interpreter.h>    /* VERIFY_ALL */
#include <io_md.h>
#include <prgc.h>           /* PR_GetGCInfo() */
#include <prmon.h>
#include <prlink.h>
#include <stdlib.h>         /* _MAX_PATH */
#ifdef XP_UNIX
#define O_BINARY    0
#endif

#if defined(XP_PC)

/*
** Win16 uses the PRStaticLinkTable structures to get case-sensitive
** function binding (via PR_FindSymbol(...))
**
** This is necessary, since GetProcAddress(...) is NOT case sensitive
** for Win16
*/
#if !defined(_WIN32)
extern PRStaticLinkTable rt_nodl_tab[];
#endif

#if defined(_WIN32)
BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
        verifyclasses = VERIFY_ALL; /* we've decided this is the default */
#ifdef DEBUG
        trace = 0;
#endif
        break;

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;

    case DLL_PROCESS_DETACH:
      break;
  }

    return TRUE;
}
#else   /* !WIN32 */

int CALLBACK LibMain( HINSTANCE hInst, WORD wDataSeg, 
                      WORD cbHeapSize, LPSTR lpszCmdLine )
{
    return TRUE;
}

BOOL CALLBACK _export _loadds WEP(BOOL fSystemExit)
{
    return TRUE;
}

/*
** This routine returns the nodl_table containing ALL java native methods.
** This table is used by NSPR when looking up symbols in the DLL.
**
** Since Win16 GetProcAddress() is NOT case-sensitive, it is necessary to
** use the nodl table to maintain the case-sensitive java native method names...
*/
PRStaticLinkTable * CALLBACK __export __loadds NODL_TABLE(void)
{
    return rt_nodl_tab;
}

#endif  /* !WIN32 */


#endif  /* XP_PC */

void ClassDefHook(ClassClass *cb) { }

int OpenCode(char *fn, char *sfn, char *dir, struct stat * st)
{
    long fd = -1;

    if ((st != 0) && (fn != 0) &&
        ((fd = sysOpen(fn, O_RDONLY, 0400)) >= 0) &&
        (fstat(fd, st) < 0)) {
        sysClose(fd);
        fd = -1;
    }
    return fd < 0 ? -2 : fd;
}

extern ClassClass *Thread_classblock;
ClassClass *GetThreadClassBlock(void) 
{
    return Thread_classblock;
}


void PreventAsyncGC( BOOL flag )
{
    if( flag ) {
        PR_EnterMonitor(PR_GetGCInfo()->lock);
    } else {
        PR_ExitMonitor(PR_GetGCInfo()->lock);
    }
}

char * privateGetenv(const char *e)
{
    return getenv(e);
}

int privatePutenv(char * e)
{
    return putenv(e);
}
