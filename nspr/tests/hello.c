#include <nspr/prthread.h>
#include <nspr/prglobal.h>
#include <stdio.h>

#define STACK_SIZE 64*1024

void printit(void *a, void *b)
{
    char *toprint = (char *) a;
    FILE *f = (FILE *) b;
    int64 ll;

    LL_I2L(ll, 1);
    while(1) {
        fprintf(f, "%s\n", toprint);
        PR_Sleep(ll);
    }
}


int main(int argc, char *argv[])
{
    PRThread *t1;

    PR_Init("hello", 1, 1, 0);

    t1 = PR_CreateThread("world", 1, STACK_SIZE);

    PR_Start(t1, printit, "world", stderr);
    printit((void *)"hello", (void *)stderr);

    PR_Exit();
    return 0;
}
