/*
 * mutex.c: Test NSPR thread mutex stuff (monitors)
 *
 * Currently does two tests: One is a basic shared variable test where 
 * a global is set by one thread, and then set to something else by
 * another. The monitor is supposed to keep them running in the right 
 * order.
 * 
 * Rob McCool
 */

#include <nspr/prglobal.h>
#include <nspr/prthread.h>
#include <nspr/prmon.h>

#include <stdio.h>

#define STACK_SIZE 16384

int global;


void contention_thread(void *vmon, void *unused)
{
    PRMonitor *pm = (PRMonitor *) vmon;

    PR_EnterMonitor(pm);
    if(global != 1) {
        fprintf(stderr, "Contention lock failed, global != 1\n");
        exit(1);
    }
    global = 2;
    PR_ExitMonitor(pm);
    PR_Exit();
}

void try_contention_lock(void)
{
    PRMonitor *pm = PR_NewMonitor(0);
    PRThread *pt;

    PR_EnterMonitor(pm);
    global = 0;

    if(!(pt = PR_CreateThread("contention", 2, STACK_SIZE)) ) {
        fprintf(stderr, "CreateThread failed.\n");
        exit(1);
    }
    PR_Start(pt, contention_thread, (void *)pm, NULL);
    PR_Yield();
    global = 1;
    PR_ExitMonitor(pm);

    PR_EnterMonitor(pm);
    if(global != 2) {
        fprintf(stderr, "Contention lock test failed, global != 2\n");
        exit(1);
    }
    PR_ExitMonitor(pm);
    PR_DestroyMonitor(pm);
    printf("Contention lock test passed.\n");
}


void nested(void *vmon, void *unused)
{
    PRMonitor *pm = (PRMonitor *) vmon;

    PR_EnterMonitor(pm);
    if(global != 2) {
        fprintf(stderr, "Nested test failed, global != 2 in nest thread\n");
        exit(1);
    }
    global = 3;
    PR_ExitMonitor(pm);
    PR_Exit();
}

void try_nested(void)
{
    PRMonitor *pm = PR_NewMonitor(0);
    PRThread *pt;

    PR_EnterMonitor(pm);
    global = 1;

    if(!(pt = PR_CreateThread("nested", 5, STACK_SIZE)) ) {
        fprintf(stderr, "CreateThread failed.\n");
        exit(1);
    }
    PR_Start(pt, nested, (void *)pm, NULL);

    PR_EnterMonitor(pm);
    global = 2;
    PR_ExitMonitor(pm);
    PR_Yield();
    if(global != 2) {
        fprintf(stderr, "Nested test failed, global != 2 in main thread\n");
        exit(1);
    }
    PR_ExitMonitor(pm);
    PR_EnterMonitor(pm);
    if(global != 3) {
        fprintf(stderr, "Nested test failed, global != 3 after nested run\n");
        exit(1);
    }
    PR_ExitMonitor(pm);
    PR_DestroyMonitor(pm);
    printf("Nested test passed.\n");
}


int main(int argc, char *argv[])
{
    PR_Init("mutex", 1, 1, 0);

    try_contention_lock();
    try_nested();
    PR_Exit();
    return 0;
}
