#include "prtypes.h"
#include "prglobal.h"

#ifdef XP_UNIX
#include <unistd.h>
#endif

#ifdef XP_PC
#include <windows.h>
#endif

prword_t pr_pageSize;
prword_t pr_pageShift;

PR_PUBLIC_API(void) PR_InitPageStuff(void)
{
    /* Get system page size */
    if (!pr_pageSize) {
#ifdef XP_PC
#ifdef _WIN32
	SYSTEM_INFO info;
	GetSystemInfo(&info);
	pr_pageSize = info.dwPageSize;
#else
        pr_pageSize = 4 * 1024; // If its good enough for the Mac...
#endif
#endif  /* XP_PC */

#ifdef XP_UNIX
	/* Get page size */
#if defined SUNOS4 || defined LINUX || defined BSDI || defined AIX
	pr_pageSize = getpagesize();
#elif defined(HPUX)
        /* I have no idea. Don't get me started. --Rob */
	pr_pageSize = sysconf(_SC_PAGE_SIZE);
#else
	pr_pageSize = sysconf(_SC_PAGESIZE);
#endif
#endif  /* XP_UNIX */

#ifdef XP_MAC
	pr_pageSize = 4 * 1024; // This is as good as any... --DKC
#endif  /* XP_MAX */

	pr_pageShift = PR_CeilingLog2(pr_pageSize);
    }
}
