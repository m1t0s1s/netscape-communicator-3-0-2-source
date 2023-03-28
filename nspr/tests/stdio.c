/*
 * stdio.c: See if blocking stdio stuff causes deadlock
 * 
 * Rob McCool
 */

#include <nspr/prthread.h>
#include <nspr/prglobal.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <unistd.h>

#define STACK_SIZE 14096

void reader(void *arg1, void *arg2)
{
    FILE *fr = ((FILE *)arg1);
    FILE *fw = ((FILE *)arg2);
    char buf[8192];
    int nc;

    if( (nc = fread(buf, sizeof(char), sizeof(buf), fr)) == -1)
        perror("fread");
    else {
        buf[nc] = '\0';
        fprintf(stderr, "got string %s\n", buf);
    }
    fwrite("okay", sizeof(char), 4, fw);
    PR_Exit();
}


int main(int argc, char *argv[])
{
    int nc, pfd[2];
    char buf[8192];
    PRThread *t1;
    FILE *childf_r, *childf_w, *myf_r, *myf_w;

    if(socketpair(AF_UNIX, SOCK_STREAM, 0, pfd) == -1) {
        perror("socketpair");
        exit(1);
    }
    PR_Init("block", 1, 1, 0);

    t1 = PR_CreateThread("reader", 31, STACK_SIZE);

    childf_r = fdopen(pfd[0], "r");
    childf_w = fdopen(pfd[0], "w");
    myf_r = fdopen(pfd[1], "r");
    myf_w = fdopen(pfd[1], "w");

    PR_Start(t1, reader, (void *)childf_r, (void *)childf_w);
    PR_Yield();
    fwrite("hello", sizeof(char), 5, myf_w);
    if( (nc = fread(buf, sizeof(char), sizeof(buf), myf_r)) == -1) {
        perror("read");
        PR_Exit();
    }
    buf[nc] = '\0';
    fprintf(stderr, "%s\n", buf);
    PR_Exit();
    return 0;
}
