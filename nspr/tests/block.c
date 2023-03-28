/*
 * block.c: Test if making a blocking system call blocks both threads
 * 
 * Rob McCool
 */


#include <nspr/prthread.h>
#include <nspr/prglobal.h>
#include <nspr/prfile.h>


#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>

#define STACK_SIZE 32768

int done = 0;

void reader(void *arg1, void *arg2)
{
    int fd = *((int *)arg1), nc;
    PRFileHandle prfd;
    char buf[8192];

    prfd = PR_ImportSocket(fd);
    if( (nc = PR_READ(prfd, buf, sizeof(buf))) == -1)
        perror("read");
    else {
        buf[nc] = '\0';
        fprintf(stderr, "got string %s\n", buf);
    }
    PR_WRITE(prfd, "okay", 4);
    PR_Exit();
}


int main(int argc, char *argv[])
{
    int nc, pfd[2];
    char buf[8192];
    PRThread *t1;
    PRFileHandle prfd1;

    PR_Init("block", 1, 1, 0);
    if(socketpair(AF_UNIX, SOCK_STREAM, 0, pfd) == -1) {
        perror("socketpair");
        exit(1);
    }

    t1 = PR_CreateThread("reader", 31, STACK_SIZE);

    PR_Start(t1, reader, (void *)&pfd[0], NULL);
    PR_Yield();
    prfd1 = PR_ImportSocket(pfd[1]);

    PR_WRITE(prfd1, "hello", 5);
    if( (nc = PR_READ(prfd1, buf, sizeof(buf))) == -1) {
        perror("read");
        PR_Exit();
    }
    buf[nc] = '\0';
    fprintf(stderr, "%s\n", buf);
    PR_Exit();
    return 0;
}
