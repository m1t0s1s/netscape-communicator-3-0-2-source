/*
 * dns.c: See if a long-running DNS lookup from one thread locks the program
 * 
 * Rob McCool
 */

#include <nspr/prthread.h>
#include <nspr/prglobal.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define STACK_SIZE 16384

int done = 0;

void do_lookup(void *hn_v, void *unused)
{
    char *hn = (char *) hn_v;
    struct hostent *he;

    he = gethostbyname(hn);
    if(!he)
        fprintf(stderr, "\nNo address found.\n");
    else
        fprintf(stderr, "\nAddress is %s\n", 
                inet_ntoa( *((struct in_addr *)he->h_addr) ) );
    done = 1;
    PR_Exit();
}


int main(int argc, char *argv[])
{
    PRThread *t1;
    int64 ll;

    if(argc != 2) {
        fprintf(stderr, "Usage: dns hostname\n");
        exit(1);
    }
    PR_Init("dns", 1, 1, 0);

    t1 = PR_CreateThread("lookup", 2, STACK_SIZE);

    PR_Start(t1, do_lookup, (void *)argv[1], NULL);
    PR_Yield();
    LL_I2L(ll, 1000);
    while(!done) {
        fprintf(stderr, ".");
        fflush(stderr);
        PR_Sleep(ll);
    }
    PR_Exit();
    return 0;
}
