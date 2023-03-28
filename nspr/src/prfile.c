#include <stdarg.h>
#include "prmon.h"
#include "prfile.h"
#include "prglobal.h"

/*
** Generic "machine independent" portion of the NSPR i/o abstraction
*/

PRMonitor *pr_asyncInput;
PRMonitor *pr_asyncOutput;
PRMonitor *pr_fdLock;

#define MAX_FILE_HANDLE         128

extern PRIOMethods _pr_file_methods;
PRFileDesc pr_fdTable[MAX_FILE_HANDLE];

PRFileHandle _PR_InitFileHandle(PROSFD osfd, PRIOMethods *m)
{
    PRFileDesc *fd;

    if( (osfd < 0) || (osfd >= MAX_FILE_HANDLE) ) {
        return -1;
    }

    PR_ASSERT(pr_fdLock != 0);
    PR_EnterMonitor(pr_fdLock);

    fd = &pr_fdTable[osfd];

    fd->osfd    = osfd;
    fd->methods = m;

    PR_ExitMonitor(pr_fdLock);
    return (PRFileHandle)osfd;
}

void _PR_DestroyFileHandle(PRFileHandle hdl)
{
    PRFileDesc *fd;

    fd = PR_HANDLE_TO_DESC(hdl);

    PR_ASSERT(pr_fdLock != 0);
    PR_EnterMonitor(pr_fdLock);

    fd->flags   = 0;
    fd->osfd    = _OS_INVALID_FD_VALUE;
    fd->methods = &_pr_file_methods;

    PR_ExitMonitor(pr_fdLock);
    return;
}

void _PR_InitIO(void)
{
    PRFileDesc *fd;
    int i;

    if (!pr_asyncInput) {
        pr_asyncInput = PR_NewNamedMonitor(0, "async-input");
        pr_asyncOutput = PR_NewNamedMonitor(0, "async-output");
        pr_fdLock = PR_NewNamedMonitor(0, "fd-lock");

        for(i=0; i<MAX_FILE_HANDLE; i++ ) {
            fd = PR_HANDLE_TO_DESC(i);
            fd->osfd = i;
            fd->methods = &_pr_file_methods;
        }

        _OS_InitIO();
    }
}

PR_PUBLIC_API(PRFileHandle) PR_MapFileHandle(PROSFD osfd, void *data, 
                                            PRIOMethods *methods)
{
    PRFileDesc *fd;

    if( (osfd < 0) || (osfd >= MAX_FILE_HANDLE) ) {
        return -1;
    }

    PR_ASSERT(pr_fdLock != 0);
    PR_EnterMonitor(pr_fdLock);

    fd = PR_HANDLE_TO_DESC(osfd);
    fd->flags    = 0;
    fd->osfd     = osfd;
    fd->methods  = methods;
    fd->instance = data;

    if( fd->methods->init ) {
        (*fd->methods->init)(fd);
    }
    PR_ExitMonitor(pr_fdLock);


    return (PRFileHandle)osfd;
}

PR_PUBLIC_API(int32) PR_READ(PRFileHandle hdl, void *buf, int32 size)
{
    PRFileDesc *fd;

    fd = PR_HANDLE_TO_DESC(hdl);
    return (*fd->methods->read)(fd,buf,size);
}

PR_PUBLIC_API(int32) PR_WRITE(PRFileHandle hdl, const void *buf, int32 size)
{
    PRFileDesc *fd;

    fd = PR_HANDLE_TO_DESC(hdl);
    return (*fd->methods->write)(fd,buf,size);
}

PR_PUBLIC_API(int32) PR_CLOSE(PRFileHandle hdl)
{
    PRFileDesc *fd;
    int32 ret;

    fd = PR_HANDLE_TO_DESC(hdl);
    ret = (*fd->methods->close)(fd);

    _PR_DestroyFileHandle(hdl);
    return ret;
}
