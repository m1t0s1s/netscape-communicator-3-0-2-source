/*
 * create.c: Test thread creation and stack allocation
 * 
 * Rob McCool
 */

#include <nspr/prthread.h>
#include <nspr/prglobal.h>

#include <stdio.h>


void printstats(void *arg1, void *arg2)
{
    int i;

    printf("Thread args: 0x%x 0x%x\n", arg1, arg2);
    printf("New thread current stack pointer is 0x%x\n", &i);
    PR_Exit();
}

void main(int argc, char *argv[])
{
    PRThread *pt;
    int i;

    PR_Init("create", 1, 1, 0);
    printf("Primordial thread current stack pointer is 0x%x\n", &i);
    pt = PR_CreateThread("printstats", 1, 16384);
    PR_Start(pt, printstats, (void *)0xdeadfeed, (void *)0xbeefcafe);
    PR_Exit();
}
