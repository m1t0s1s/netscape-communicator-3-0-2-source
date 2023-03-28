#include "sec.h"
#include <unistd.h>

static void Usage(char *progName)
{
    fprintf(stderr, "Usage: %s [-r] [-k key -w]\n", progName);
    fprintf(stderr, "%-20s Specify the nodelock key file (used for -w only)\n",
	    "-k keyfile");
    fprintf(stderr, "%-20s Specify node-lock file to examine\n",
	    "-r input");
    fprintf(stderr, "%-20s Specify node-lock file to create\n",
	    "-w");
    exit(-1);
}

int main(int argc, char **argv)
{
    char *progName;
    DSStatus rv;
    unsigned long appid, sysid;
    int doread, dowrite, o;
    SECPrivateKey *key;
    time_t expiration;

    progName = strrchr(argv[0], '/');
    progName = progName ? progName+1 : argv[0];

/*    appid = 0x101970;  proxy */
    appid = 0x41773;  /* secure */
/*    appid = 0x377140; commun  */
/*    sysid = SEC_GetHostID();  */
    sysid = inet_addr("198.93.93.54"); 
    doread = 0;
    dowrite = 0;
    key = 0;
    while ((o = getopt(argc, argv, "k:rw")) != -1) {
	switch (o) {
	  case '?':
	    Usage(progName);
	    break;

	  case 'k':
	    key = SEC_GetPrivateKey(optarg, stdin, stderr);
	    if (!key) {
		fprintf(stderr, "%s: bad key file\n", progName);
		return -1;
	    }
	    break;

	  case 'r':
	    doread = 1;
	    break;

	  case 'w':
	    dowrite = 1;
	    break;
	}
    }
    if (!doread && !dowrite) Usage(progName);

    if (dowrite) {
	if (!key) Usage(progName);
	/* Create a nodelock file for this program */
	rv = SEC_WriteNodeLockEntry("NODE-LOCK", sysid, appid,
				    time(NULL)+3628800, key,
				    "Netsite Commerce Server");
	if (rv) {
	    fprintf(stderr, "%s: write node lock failed, error=%d\n",
		    progName, XP_GetError());
	    return -1;
	}
    }

    if (doread) {
	rv = SEC_TestNodeLock("NODE-LOCK", appid, &expiration);
	if (rv) {
	    fprintf(stderr, "%s: test node lock failed, error=%d\n",
		    progName, XP_GetError());
	    return -1;
	}
	fprintf(stderr, "%s: node lock test passed\n", progName);
    }
    return 0;
}
