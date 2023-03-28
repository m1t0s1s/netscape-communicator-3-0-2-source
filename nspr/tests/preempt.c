/*
 * preemt.c: Check that threads can't run away with the CPU
 * 
 * Rob McCool
 */


#include <nspr/prglobal.h>
#include <nspr/prthread.h>

#include <stdio.h>

#define STACK_SIZE 8192

int bail;

void wimp(void *a0, void *a1)
{
    register int x;

    for(x = 0; x < 10; ++x)
        PR_Yield();
    printf("wimp finished.\n");
    bail = 1;
    PR_Exit();
}

int main(int argc, char *argv[])
{
    PRThread *pt;

    PR_Init("preempt", 1, 1, 0);
#ifdef XP_UNIX
    PR_StartEvents(0);
#endif

    printf("Preemption test starting.\n");
    pt = PR_CreateThread("wimp", 1, STACK_SIZE);
    PR_Start(pt, wimp, NULL, NULL);

    while (bail == 0) {
    }

    PR_Exit();
    return 1;
}
