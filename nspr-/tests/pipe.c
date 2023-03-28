#include <nspr/prthread.h>
#include <nspr/prglobal.h>
#include <nspr/prmon.h>
#include <nspr/prmem.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>

#if defined(AIX)
#include <sys/select.h>
#endif

int count;
PRMonitor *gate;
PRThread *r, *w;

void Reader(int fd)
{
    int i;
    char buf[100];

    /* Wait for main to let us run */
    PR_EnterMonitor(gate);
    PR_ExitMonitor(gate);

    for (i = 0; i < count; i++) {
	int rv;
	fd_set rd;
	memset(&rd, 0, sizeof(rd));
	FD_SET(fd, &rd);
	rv = select(fd+1, &rd, 0, 0, 0);
	if (rv < 0) {
	    perror("reader: select");
	    exit(-1);
	}
	rv = read(fd, buf, sizeof(buf));
	if (rv < 0) {
	    perror("reader: read");
	    exit(-1);
	}
	write(1, buf, rv);
    }

    /* Tell main we are all done */
    PR_EnterMonitor(gate);
    PR_Notify(gate);
    PR_ExitMonitor(gate);
}

void Writer(int fd)
{
    int i;
    char buf[20];
    int64 n;

    /* Wait for main to let us run */
    PR_EnterMonitor(gate);
    PR_ExitMonitor(gate);

    LL_I2L(n, 1);
    for (i = 0; i < count; i++) {
	sprintf(buf, "message %d\n", i);
	write(fd, buf, strlen(buf));
	PR_Sleep(n);
    }
}

void main(int argc, char **argv)
{
    int f[2];

    if (argc > 1)
	count = atoi(argv[1]);
    else
	count = 10;

    if (pipe(f) < 0) {
	perror("pipe");
	exit(-1);
    }

    PR_Init("pipe", 1, 1, 0);
    gate = PR_NewMonitor(1);

    r = PR_CreateThread("reader", 5, 32768);
    PR_Start(r, (void (*)(void*,void*)) Reader, (void*) f[0], 0);

    w = PR_CreateThread("writer", 1, 32768);
    PR_Start(w, (void (*)(void*,void*)) Writer, (void*) f[1], 0);

    PR_Wait(gate, LL_MAXINT);
    PR_ExitMonitor(gate);
    PR_Exit();
}
