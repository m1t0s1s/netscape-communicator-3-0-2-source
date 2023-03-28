#ifdef XP_PC

#if 0 /* old one didn't work [jules] */
#include <string.h>

char *optarg;
int optind;

int getopt(int argc, char **argv, char *spec)
{
    char ch, *cp;

    ++optind;
    if (optind == argc) {
	optind = 0;
	return -1;
    }
    if (argv[optind][0] == '-') {
	ch = argv[optind][1];
	cp = strchr(spec, ch);
	if (!cp) {
	    return '?';
	}
	if (cp[1] == ':') {
	    if (argv[optind][2]) {
		optarg = &argv[optind][2];
	    } else {
		optarg = argv[++optind];
	    }
	} else {
	    optarg = 0;
	}
	return cp[0];
    }
    return -1;
}
#else /* here's the "new" one */

/*
**  This comes from the AT&T public-domain getopt published in mod.sources
**  (i.e., comp.sources.unix before the great Usenet renaming).
*/

#include <stdio.h>

#ifdef _WIN32
#define ERR(s, c)					\
    if (opterr) {					\
	char buff[2];					\
	int fd = _fileno (stderr);			\
	buff[0] = c; buff[1] = '\n';			\
	(void)write(fd, av[0], strlen(av[0]));		\
	(void)write(fd, s, strlen(s));			\
	(void)write(fd, buff, 2);			\
    }
#else
#define ERR(s, c)					\
    ; /* Win16 cant handle stderr */
#endif

int	opterr = 1;
int	optind = 1;
int	optopt = 0;
char	*optarg;


/*
**  Return options and their values from the command line.
*/
int
getopt(ac, av, opts)
    int		ac;
    char	*av[];
    char	*opts;
{
    extern char	*strchr();
    static int	i = 1;
    char	*p;

    /* Move to next value from argv? */
    if (i == 1) {
	if (optind >= ac || av[optind][0] != '-' || av[optind][1] == '\0')
	    return EOF;
	if (strcmp(av[optind], "--") == 0) {
	    optind++;
	    return EOF;
	}
    }

    /* Get next option character. */
    if ((optopt = av[optind][i]) == ':'
     || (p = strchr(opts,  optopt)) == NULL) {
	ERR(": illegal option -- ", optopt);
	if (av[optind][++i] == '\0') {
	    optind++;
	    i = 1;
	}
	return '?';
    }

    /* Snarf argument? */
    if (*++p == ':') {
	if (av[optind][i + 1] != '\0')
	    optarg = &av[optind++][i + 1];
	else {
	    if (++optind >= ac) {
		ERR(": option requires an argument -- ", optopt);
		i = 1;
		return '?';
	    }
	    optarg = av[optind++];
	}
	i = 1;
    }
    else {
	if (av[optind][++i] == '\0') {
	    i = 1;
	    optind++;
	}
	optarg = NULL;
    }

    return optopt;
}
#endif /* old or new? */

#endif /* XP_PC */
