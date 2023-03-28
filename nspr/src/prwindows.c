#ifdef XP_PC 
#include <windows.h>
#include <stdarg.h>
#include <string.h>
#include "prfile.h"
#include "prmem.h"
#include "prthread.h"

struct PRMethodCallbackStr * _pr_callback_funcs;

/*
** Initialization routine for the NSPR DLL...
*/

/*
** Global Instance handle...
** In Win32 this is the module handle of the DLL.
**
** In Win16 this is the instance handle of the application
** which loaded the DLL.
*/
HINSTANCE _pr_hInstance;

#ifdef _WIN32
BOOL WINAPI DllMain (HINSTANCE hDLL, DWORD dwReason, LPVOID lpReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
      _pr_hInstance = hDLL;
      break;

    case DLL_THREAD_ATTACH:
      break;

    case DLL_THREAD_DETACH:
      break;

    case DLL_PROCESS_DETACH:
      _pr_hInstance = NULL;
      break;
  }

    return TRUE;
}

#else  /* ! _WIN32 */

int CALLBACK LibMain( HINSTANCE hInst, WORD wDataSeg, 
                      WORD cbHeapSize, LPSTR lpszCmdLine )
{
    _pr_hInstance = hInst;
    return TRUE;
}

BOOL CALLBACK __loadds WEP(BOOL fSystemExit)
{
    _pr_hInstance = NULL;
    return TRUE;
}
#endif /* ! _WIN32 */


/* weak symbols for real implementations of system calls */

extern int _open(const char*, int, ...);
extern int _stat(const char *path, struct _stat * buf);
extern int _access(const char *path, int mode);

/* Private routine to unixify the slashes */
extern void FlipSlashes( char *, int );

/* Win16 sucks !! */
#if defined(XP_PC) && !defined(_WIN32)
#define MAX_PATH            _MAX_PATH
#endif

int open(const char *name, int flags, ...)
{
    int mode = 0;
    va_list ap;
    int fd;
    char buffer[ MAX_PATH ];

    if (flags & O_CREAT) {
        va_start(ap, flags);
        mode = va_arg(ap, int);
        va_end(ap);
    }

    strncpy( buffer, name, MAX_PATH );
    FlipSlashes( buffer, strlen(buffer) );

    fd = PR_OpenFile(buffer, flags, mode);
    return fd;
}

int stat( const char *path, struct stat * buf )
{
    int fd;
    char cp[ MAX_PATH ];

    if( path ) {
        strncpy( cp, path, MAX_PATH );
        FlipSlashes( cp, sizeof(cp)  );

        fd = _stat( cp, (struct _stat *)buf );
    } else {
        fd = -1;
    }
    return fd;
}

int access( const char *path, int mode )
{
    int fd;
    char cp[ MAX_PATH ];

    if( path ) {
        strncpy( cp, path, MAX_PATH );
        FlipSlashes( cp, sizeof(cp) );

        fd = _access( cp, mode );
    } else {
        fd = -1;
    }
    return fd;
}


char * _OS_PREPARE_NAME( const char *name, int len )
{
    int i;
    char * fn = 0;
    /*
    ** The following code ONLY works under Win95 or some other OS 
    ** which supports VFAT
    */
#if 0
	char tx1[261];
	char tx[261];
	extern int LFNConvertFromLongFilename(char *translation, char *filename);
#endif


    /*
    ** The following code ONLY works under Win95 or some other OS 
    ** which supports VFAT
    */
#if 0
	strncpy(tx1, name, len);
	tx1[len] = '\0';

	if (LFNConvertFromLongFilename(tx, tx1)) {
		name = tx;
		len  = strlen(tx);
	}
#endif

    fn = malloc( len+1 );
    if( fn != NULL ) {
        strncpy( fn, name, len );
        fn[len] = '\0';

        for(i=0; i<len; i++) if( fn[i] == '/' ) fn[i] = '\\';
    }
    return fn;
}

void _OS_InitIO(void)
{
}

PR_PUBLIC_API(void) PR_NonBlockIO(int fd, int yes)
{
}


/*
** Wait for FD read/write data to appear.
*/
PR_PUBLIC_API(int) _PR_WaitForFD(PRFileDesc *fd, int rd)
{
    return 0;
}

PR_PUBLIC_API(void) PR_MDInit(struct PRMethodCallbackStr *f)
{
    _pr_callback_funcs = f;
}

#endif    /* XP_PC */
