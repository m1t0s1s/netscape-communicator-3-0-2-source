#if defined(XP_PC) && !defined(_WIN32)

#include <nspr.h>
#include <prlog.h>
#include <prprf.h>
#include <windowsx.h>   /* Edit_xxx(...) macros */
#include <toolhelp.h>   /* TerminateApp(...) */
#include "mdint.h"

/* Leave a bit of room for any malloc header bytes... */
#define MAX_SEGMENT_SIZE    (65536L - 4096L)

extern HINSTANCE _pr_hInstance;
extern LRESULT CALLBACK _loadds ConsoleWndProc(HWND,UINT,WPARAM,LPARAM);
extern void _MD_InitConsoleIO(void);

/*
** stderr is NOT defined for DLLs... So, define it here for NSPR and
** java...
*/
FILE *stderr = NULL;

/*
** Window handles to the NSPR console window (if available).
*/
static HWND hMainWnd;
static HWND hEditWnd;


/************************************************************************/
/*
** Machine dependent initialization routines:
**    __md_InitOS
**    _MD_InitConsole
*/
/************************************************************************/

void __md_InitOS(int when)
{
    if (when == _MD_INIT_AT_START) {
       _MD_InitConsoleIO();
    } 
    else if (when == _MD_INIT_READY) {
#ifdef DEBUG
       stderr = fopen("stderr.log", "rw");
#else
       stderr = fopen("NUL", "rw");
#endif
    }
}


void _MD_InitConsoleIO(void)
{
    /*
    ** Create a top level console window..
    */
#if 0
    if( hMainWnd == NULL ) {
        WNDCLASS cls;

        cls.style         = CS_HREDRAW | CS_VREDRAW;
        cls.lpfnWndProc   = ConsoleWndProc;
        cls.cbClsExtra    = 0;
        cls.cbWndExtra    = 0;
        cls.hInstance     = _pr_hInstance;
        cls.hIcon         = LoadIcon(NULL, IDI_QUESTION);
        cls.hCursor       = LoadCursor(NULL, IDC_ARROW);
        cls.hbrBackground = COLOR_BACKGROUND+1;
        cls.lpszMenuName  = NULL;
        cls.lpszClassName = "NSPR-Console-class";
        
        RegisterClass(&cls);
        hMainWnd = CreateWindow(
                        "NSPR-Console-class",
                        "NSPR Console Window",
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        CW_USEDEFAULT,
                        (HWND)NULL,
                        (HMENU)NULL,
                        _pr_hInstance,
                        NULL );
        /*
        ** Create the embedded Edit window
        */
        if( hMainWnd ) {
            hEditWnd = CreateWindow("EDIT",         /* class name     */
                                    "",             /* window title   */
                                    WS_CHILD | ES_MULTILINE | ES_WANTRETURN,
                                    0,              /* top             */
                                    0,              /* left            */
                                    CW_USEDEFAULT,  /* width           */
                                    CW_USEDEFAULT,  /* height          */
                                    hMainWnd,       /* parent window   */
                                    (HMENU)100,     /* dialog ID       */
                                    _pr_hInstance,  /* owning instance */
                                    NULL );         /* CREATESTRUCT    */
            ShowWindow(hMainWnd, SW_SHOWNORMAL);
        }
    }
#endif /* 0 */
}

#if 0
LRESULT CALLBACK _loadds ConsoleWndProc( HWND hWnd, UINT msg, WPARAM wParam, 
                                 LPARAM lParam ) 
{
    switch( msg ) {
      case WM_SIZE:
        if( hEditWnd ) {
            MoveWindow(hEditWnd, 0, 0, LOWORD(lParam), HIWORD(lParam), FALSE);
            ShowWindow(hEditWnd, SW_SHOWNORMAL);
        }
        break;

      case WM_CLOSE:
        exit(0);
        break;

      case WM_SHOWWINDOW:
        if( hEditWnd && wParam ) {
            ShowWindow(hEditWnd, SW_SHOWNORMAL);
        }
        break;

      case WM_DESTROY:
        hEditWnd = NULL;
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}
#endif /* 0 */


/************************************************************************/
/*
** Machine dependent GC Heap management routines:
**    _MD_GrowGCHeap
*/
/************************************************************************/

void *_MD_GrowGCHeap(uint32 *sizep)
{
    void *addr;

    if( *sizep > MAX_SEGMENT_SIZE ) {
        *sizep = MAX_SEGMENT_SIZE;
    }

    addr = malloc((size_t)*sizep);
    return addr;
}

void _MD_FreeGCSegment(void *base, int32 len)
{
    if (base) {
        free(base);
    }
}


/*
 * We had to write this in order to force using the local
 * data segment, but also preserve the type signatures of
 * the overridden functions (malloc, realloc, etc.), below.
 */
static struct PRMethodCallbackStr * __loadds
GetCbFuncs()
{
    extern struct PRMethodCallbackStr *_pr_callback_funcs;

    return _pr_callback_funcs;
}

/************************************************************************/
/*
** NSPR re-implenentations of various C runtime functions:
**    printf(...)
**    sscanf(...)
**    strftime(...)
**    exit(...)
**    perror(...)
*/
/************************************************************************/

int _export printf(const char *fmt, ...)
{
    char buffer[1024];
    int ret = 0;

    va_list args;
    va_start(args, fmt);

#ifdef DEBUG
    PR_vsnprintf(buffer, sizeof(buffer), fmt, args);
    {   
        struct PRMethodCallbackStr *cbf;

        cbf = GetCbFuncs();
        if (cbf != NULL && cbf->auxOutput != NULL) {
            (* cbf->auxOutput)(buffer);
        } else {
            OutputDebugString(buffer);
        }
    }
#endif

    va_end(args);
    return ret;
}


int _export fprintf(FILE *fPtr, const char *fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);

    PR_vsnprintf(buffer, sizeof(buffer), fmt, args);

    if (fPtr == NULL) {
        struct PRMethodCallbackStr *cbf;

        cbf = GetCbFuncs();
        if (cbf != NULL && cbf->auxOutput != NULL) {
            (* cbf->auxOutput)(buffer);
        } else {
            OutputDebugString(buffer);
        }
    } else {
        fwrite(buffer, 0, strlen(buffer), fPtr); /* XXX Is this a sec. hole? */
    }

return 0;
}

void _export perror(const char*str) {
}


int sscanf(const char *buf, const char *fmt, ...) {
    return 0;
}

size_t strftime(char *s, size_t len, const char *fmt, const struct tm *p) {
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->strftime)(s, len, fmt, p);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}

void exit(int n)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if (cbf != NULL && cbf->exit != NULL) {
        (* cbf->exit)(n);
    } else {
        TerminateApp(NULL, n);    
    }
}

void *  malloc(size_t size) 
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->malloc)(size);
    } 
    return GlobalAllocPtr(GPTR, size);
}

void *  realloc(void *p, size_t size) 
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->realloc)(p, size);
    } 
    return GlobalReAllocPtr(p, size, GPTR);
}

void *  calloc(size_t size, size_t number) 
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->calloc)(size, number);
    } 
    return GlobalAllocPtr(GPTR, size*number);
}

void  free(void *p) 
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        (*cbf->free)(p);
    } else {
        GlobalFreePtr(p);
    }
}

int PASCAL  gethostname(char * name, int namelen) 
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->gethostname)(name, namelen);
    } else {
        PR_ASSERT(0);
        return -1;
    }
}

struct hostent * PASCAL  gethostbyname(const char * name) 
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->gethostbyname)(name);
    } else {
        PR_ASSERT(0);
        return NULL;
    }
}

struct hostent * PASCAL  gethostbyaddr(const char * addr, int len, int type) 
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->gethostbyaddr)(addr, len, type);
    } else {
        PR_ASSERT(0);
        return NULL;
    }
}

u_long PASCAL FAR ntohl (u_long netlong)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->ntohl)(netlong);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


u_short PASCAL FAR ntohs (u_short netshort)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->ntohs)(netshort);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


int PASCAL FAR closesocket (SOCKET s)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->closesocket)(s);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


int PASCAL FAR setsockopt (SOCKET s, int level, int optname,
                           const char FAR * optval, int optlen)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->setsockopt)(s, level, optname, optval, optlen);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


SOCKET PASCAL FAR socket (int af, int type, int protocol)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->socket)(af, type, protocol);
    } else {
        PR_ASSERT(0);
        return (SOCKET)NULL;
    }
}


int PASCAL FAR getsockname (SOCKET s, struct sockaddr FAR *name,
                            int FAR * namelen)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->getsockname)(s, name, namelen);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}

u_long PASCAL FAR htonl (u_long hostlong)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->htonl)(hostlong);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


u_short PASCAL FAR htons (u_short hostshort)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->htons)(hostshort);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


unsigned long PASCAL FAR inet_addr (const char FAR * cp)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->inet_addr)(cp);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


int PASCAL FAR WSAGetLastError(void)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*WSAGetLastError)();
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


int PASCAL FAR connect (SOCKET s, const struct sockaddr FAR *name, int namelen)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*connect)(s, name, namelen);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


int PASCAL FAR recv (SOCKET s, char FAR * buf, int len, int flags)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*recv)(s, buf, len, flags);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


int PASCAL FAR ioctlsocket (SOCKET s, long cmd, u_long FAR *argp)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*ioctlsocket)(s, cmd, argp);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


int PASCAL FAR recvfrom (SOCKET s, char FAR * buf, int len, int flags,
                         struct sockaddr FAR *from, int FAR * fromlen)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*recvfrom)(s, buf, len, flags, from, fromlen);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


int PASCAL FAR send (SOCKET s, const char FAR * buf, int len, int flags)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*send)(s, buf, len, flags);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


int PASCAL FAR sendto (SOCKET s, const char FAR * buf, int len, int flags,
                       const struct sockaddr FAR *to, int tolen)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*sendto)(s, buf, len, flags, to, tolen);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}


SOCKET PASCAL FAR accept (SOCKET s, struct sockaddr FAR *addr,
                          int FAR *addrlen)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*accept)(s, addr, addrlen);
    } else {
        PR_ASSERT(0);
        return (SOCKET)NULL;
    }
}


int PASCAL FAR listen (SOCKET s, int backlog)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*listen)(s, backlog);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}

int PASCAL FAR bind (SOCKET s, const struct sockaddr FAR *addr, int namelen)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*bind)(s, addr, namelen);
    } else {
        PR_ASSERT(0);
        return 0;
    }
}













char * __cdecl  getenv(const char *varname)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->getenv)(varname);
    } else {
        PR_ASSERT(0);
        return NULL;
    }
}

int __cdecl putenv(const char *assoc)
{
    struct PRMethodCallbackStr *cbf;

    cbf = GetCbFuncs();
    if( cbf ) {
        return (*cbf->putenv)(assoc);
    } else {
        PR_ASSERT(0);
        return NULL;
    }
}

#endif /* XP_PC && !_WIN32 */
