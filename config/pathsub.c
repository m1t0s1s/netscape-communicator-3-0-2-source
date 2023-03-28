/*
** Pathname subroutines.
**
** Brendan Eich, 8/29/95
*/
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pathsub.h"

char *program;

void
fail(char *format, ...)
{
    int error;
    va_list ap;

    error = errno;
    fprintf(stderr, "%s: ", program);
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
    if (error)
	fprintf(stderr, ": %s", strerror(errno));
    putc('\n', stderr);
    exit(1);
}

char *
getcomponent(char *path, char *name)
{
    if (*path == '\0')
	return 0;
    if (*path == '/') {
	*name++ = '/';
    } else {
	do {
	    *name++ = *path++;
	} while (*path != '/' && *path != '\0');
    }
    *name = '\0';
    while (*path == '/')
	path++;
    return path;
}

char *
ino2name(ino_t ino, char *dir)
{
    DIR *dp;
    struct dirent *ep;
    char *name;

    dp = opendir("..");
    if (!dp)
	fail("cannot read parent directory");
    for (;;) {
	if (!(ep = readdir(dp)))
	    fail("cannot find current directory");
	if (ep->d_ino == ino)
	    break;
    }
    name = xstrdup(ep->d_name);
    closedir(dp);
    return name;
}

void *
xmalloc(size_t size)
{
    void *p = malloc(size);
    if (!p)
	fail("cannot allocate %u bytes", size);
    return p;
}

char *
xstrdup(char *s)
{
    return strcpy(xmalloc(strlen(s) + 1), s);
}

char *
xbasename(char *path)
{
    char *cp;

    while ((cp = strrchr(path, '/')) && cp[1] == '\0')
	*cp = '\0';
    if (!cp) return path;
    return cp + 1;
}

void
xchdir(char *dir)
{
    if (chdir(dir) < 0)
	fail("cannot change directory to %s", dir);
}

int
relatepaths(char *from, char *to, char *outpath)
{
    char *cp, *cp2;
    int len;
    char buf[NAME_MAX];

    assert(*from == '/' && *to == '/');
    for (cp = to, cp2 = from; *cp == *cp2; cp++, cp2++)
	if (*cp == '\0')
	    break;
    while (cp[-1] != '/')
	cp--, cp2--;
    if (cp - 1 == to) {
	/* closest common ancestor is /, so use full pathname */
	len = strlen(strcpy(outpath, to));
	if (outpath[len] != '/') {
	    outpath[len++] = '/';
	    outpath[len] = '\0';
	}
    } else {
	len = 0;
	while ((cp2 = getcomponent(cp2, buf)) != 0) {
	    strcpy(outpath + len, "../");
	    len += 3;
	}
	while ((cp = getcomponent(cp, buf)) != 0) {
	    sprintf(outpath + len, "%s/", buf);
	    len += strlen(outpath + len);
	}
    }
    return len;
}

void
reversepath(char *inpath, char *name, int len, char *outpath)
{
    char *cp, *cp2;
    char buf[NAME_MAX];
    struct stat sb;

    cp = strcpy(outpath + PATH_MAX - (len + 1), name);
    cp2 = inpath;
    while ((cp2 = getcomponent(cp2, buf)) != 0) {
	if (strcmp(buf, ".") == 0)
	    continue;
	if (strcmp(buf, "..") == 0) {
	    if (stat(".", &sb) < 0)
		fail("cannot stat current directory");
	    name = ino2name(sb.st_ino, "..");
	    len = strlen(name);
	    cp -= len + 1;
	    strcpy(cp, name);
	    cp[len] = '/';
	    free(name);
	    xchdir("..");
	} else {
	    cp -= 3;
	    strncpy(cp, "../", 3);
	    xchdir(buf);
	}
    }
    strcpy(outpath, cp);
}
