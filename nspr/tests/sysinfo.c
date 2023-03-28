#include <nspr.h>
#include <stdio.h>


int main(int argc, char *argv[])
{
    char buffer[1024];
    long len;

#ifdef XP_PC
    WSADATA wsaData;
    WORD wVersionRequested = MAKEWORD(1,1);

    if( WSAStartup( wVersionRequested, &wsaData ) ) {
        printf("winsock failed to initialize\n");
        return 1;
    }
#endif

   /* Test for PR_SI_HOSTNAME */
   len = PR_GetSystemInfo(PR_SI_HOSTNAME, buffer, sizeof(buffer));
    if( len ) {
	printf("PR_SI_HOSTNAME: %s (len = %d)\n", buffer, len);
    } else {
	printf("*** PR_SI_HOSTNAME failed ***\n");
    }
    
   /* Test for PR_SI_SYSNAME */
   len = PR_GetSystemInfo(PR_SI_SYSNAME, buffer, sizeof(buffer));
    if( len ) {
	printf("PR_SI_SYSNAME: %s (len = %d)\n", buffer, len);
    } else {
	printf("*** PR_SI_SYSNAME failed ***\n");
    }

   /* Test for PR_SI_RELEASE */
   len = PR_GetSystemInfo(PR_SI_RELEASE, buffer, sizeof(buffer));
    if( len ) {
	printf("PR_SI_RELEASE: %s (len = %d)\n", buffer, len);
    } else {
	printf("*** PR_SI_RELEASE failed ***\n");
    }

   /* Test for PR_SI_ARCHITECTURE */
   len = PR_GetSystemInfo(PR_SI_ARCHITECTURE, buffer, sizeof(buffer));
    if( len ) {
	printf("PR_SI_ARCHITECTURE: %s (len = %d)\n", buffer, len);
    } else {
	printf("*** PR_SI_ARCHITECTURE failed ***\n");
    }

    return 0;
}
