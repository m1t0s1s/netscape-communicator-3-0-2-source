/*
 * have-weak.c: Determine if a platform has weak symbols.
 * 
 * Rob McCool
 */


#include <stdio.h>
#ifdef XP_UNIX
#include <unistd.h>
#ifdef SUNOS4
typedef int ssize_t;
#endif
#else
typedef unsigned int ssize_t;
#endif

ssize_t _write(int filedes, const void *buf, unsigned nbyte);

ssize_t write(int filedes, const void *buf, unsigned nbyte)
{
    _write(2, "in weak write\n", sizeof("in weak write\n"));
    return _write(filedes, buf, nbyte);
}

int main(int argc, char *argv[])
{
    fprintf(stderr, "stdiowrite\n");
    write(2, "writeme\n", sizeof("writeme\n"));
    PR_Exit();
    return 0;
}
