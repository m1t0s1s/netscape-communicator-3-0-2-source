#include <fcntl.h>
#include <stdio.h>
#include <memory.h>

#ifdef XP_UNIX
#include <unistd.h>
#include <errno.h>
#endif

int main(int argc, char **argv)
{
#ifdef XP_UNIX
    int fd;
    struct flock fl;
    char buf[1000];
    int i, rv;

    if (argc != 2) {
	fprintf(stderr, "usage: locker file\n");
	return -1;
    }

    /* open/create file */
    fprintf(stderr, "Opening %s\n", argv[1]);
    fd = open(argv[1], O_RDWR|O_CREAT, 0666);
    if (fd < 0) {
	perror("open");
	return -1;
    }

    /*
    ** Setup write file lock. If l_len is zero then we lock the entire
    ** file.
    */
    fprintf(stderr, "Locking\n");
    memset(&fl, 0, sizeof(fl));
    fl.l_type = F_WRLCK;
    rv = fcntl(fd, F_SETLK, &fl);
    if (rv < 0) {
	fprintf(stderr, "fcntl failed, errno = %d\n", errno);
	return -1;
    }

    /* Now write a few bytes */
    fprintf(stderr, "Writing\n");
    for (i = 0; i < 1000; i++) {
	int nb = write(fd, buf, sizeof(buf));
	if (nb < 0) break;
    }

    /* Pause ... */
    fprintf(stderr, "Pausing\n");
    pause();

    /* Close the file */
    close(fd);
#endif

    return 0;
}
