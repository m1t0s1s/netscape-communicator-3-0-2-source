#include <nspr/nspr.h>

#define STACK_SIZE 4096

void
doit(void* a, void* b)
{
    PRMonitor* m1 = (PRMonitor*)a;
    PRMonitor* m2 = (PRMonitor*)b;

    printf("--foo thread running\n");
    {
	PR_EnterMonitor(m1);
	printf("\t--foo thread in m1\n");

	printf("\t--foo thread notifying main thread that it's alive\n");
	PR_Notify(m1);

	printf("\t--foo thread leaving m1\n");
	PR_ExitMonitor(m1);
    }

    printf("--foo waiting to enter m2\n");
    {
	PR_EnterMonitor(m2);
	printf("\t--foo thread in m2\n");

	/* just needed to enter here to prove my point */

	printf("\t--foo thread leaving m2\n");
	PR_ExitMonitor(m2);
    }

    /* sync back up with main, making sure foo terminates first */
    printf("--foo waiting to enter m1\n");
    {
	PR_EnterMonitor(m1);
	printf("\t--foo thread in m1\n");

	printf("\t--foo waiting notifying main that it's done\n");
	PR_Notify(m1);

	printf("\t--foo thread leaving m1\n");
	PR_ExitMonitor(m1);
    }
    printf("--foo thread terminating\n");
    PR_Exit();
}

void
main()
{
    PRThread* foo;
    PRMonitor* m1;
    PRMonitor* m2;
    PR_Init("main", 1, 1, 0);

    printf("--main thread running\n");
    foo = PR_CreateThread("foo", 5, STACK_SIZE);
    m1 = PR_NewMonitor(0);
    m2 = PR_NewMonitor(0);

    {
	PR_EnterMonitor(m2);
	printf("\t--main thread in m2\n");
	{
	    PR_EnterMonitor(m1);
	    printf("\t\t--main thread in m1\n");

	    printf("\t\t--main thread starting foo thread\n");
	    PR_Start(foo, doit, m1, m2);

	    printf("\t\t--main thread waiting for foo thread to come up\n");
	    PR_Wait(m1, LL_MAXINT);

	    printf("\t\t--main thread leaving m1\n");
	    PR_ExitMonitor(m1);
	}

	printf("\t--main thread suspending foo thread which is waiting on m2\n");
	PR_Suspend(foo);

	printf("\t--main thread leaving m2 (this shouldn't crash!!!)\n");
	PR_ExitMonitor(m2);
    }

    printf("--main thread waiting to enter m1\n");
    {
	PR_EnterMonitor(m1);
	printf("\t--main thread in m1\n");

	printf("\t--main thread resuming foo thread\n");
	PR_Resume(foo);

	printf("\t--main thread waiting for foo to complete\n");
	PR_Wait(m1, LL_MAXINT);

	printf("\t--main thread leaving m1\n");
	PR_ExitMonitor(m1);
    }
    printf("--main thread terminating\n");
    PR_Exit();
}
