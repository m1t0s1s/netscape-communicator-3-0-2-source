#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "sec.h"

int
main(int argc, char **argv)
{
    char *pw;
    char *fn;
    int fd;
    
    if ( argc != 2 ) {
	fprintf(stderr, "usage: %s keydir\n", argv[0]);
	return(-1);
    }
    pw = SEC_GetPassword(stdin, stdout, "Password: ", SEC_CheckPassword);

    fn = (char*) malloc(strlen(argv[1])+100);
    sprintf(fn, "%s/%s", argv[1], "dongle");
    fd = open(fn, O_WRONLY | O_CREAT | O_TRUNC, 0600);

    if ( SEC_WriteDongleFile(fd, pw) < 0 ) {
	fprintf(stderr, "error writing dongle file\n");
	return(-1);
    }

    return(0);
}
