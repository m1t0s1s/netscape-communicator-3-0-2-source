/*-
 * Copyright (c) 1983, 1990, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char copyright[] =
"@(#) Copyright (c) 1983, 1990, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)rsh.c	8.3 (Berkeley) 4/6/94";
#endif /* not lint */

/*
 * $Source: /m/src/ns/security/cmd/rsh/rsh.c,v $
 * $Header: /m/src/ns/security/cmd/rsh/rsh.c,v 1.6 1996/12/03 02:34:25 dhugo Exp $
 */

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include <netinet/in.h>
#include <netdb.h>

#include <err.h>
#include <errno.h>
#include <pwd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef sgi
#include <bstring.h>
#endif
#ifdef USE_SSL
#include <stdarg.h>
#include "sslrcmd.h"
#else
#include <varargs.h>
#endif

#if defined (__sun)
#undef sigmask
#define sigmask(m) (m > 32 ? 0 : ( 1 << ((m)-1)))
#endif


#ifdef GETOPT_BUSTED
extern char *optarg;
extern int optind;
#endif

#include "pathnames.h"

#ifdef KERBEROS
#include <kerberosIV/des.h>
#include <kerberosIV/krb.h>

CREDENTIALS cred;
Key_schedule schedule;
int use_kerberos = 1, doencrypt;
char dst_realm_buf[REALM_SZ], *dest_realm;
extern char *krb_realmofhost();
#endif

/*
 * rsh - remote shell
 */
int	rfd2;

char   *copyargs __P((char **));
void	sendsig __P((int));
void	talk __P((int, long, pid_t, int));
void	usage __P((void));
void	warning __P(());

int
main(argc, argv)
	int argc;
	char **argv;
{
	struct passwd *pw;
	struct servent *sp;
	long omask;
	int argoff, asrsh, ch, dflag, nflag, one, rem;
	pid_t pid;
	uid_t uid;
	char *args, *host, *p, *user;
#ifdef USE_SSL
	char *certDir = 0;
	char *nickname = 0;
	int sslflags = SSL_DONT_ENCRYPT | SSL_NO_PROXY;
	int ssl_encrypt = 0;
#endif /* USE_SSL */

#ifdef USE_SSL
	__err_set_progname(argv[0]);
#endif /* USE_SSL */

	argoff = asrsh = dflag = nflag = 0;
	one = 1;
	host = user = NULL;

	/* if called as something other than "rsh", use it as the host name */
	if (p = strrchr(argv[0], '/'))
		++p;
	else
		p = argv[0];
	if (strcmp(p, "rsh"))
		host = p;
	else
		asrsh = 1;

	/* handle "rsh host flags" */
	if (!host && argc > 2 && argv[1][0] != '-') {
		host = argv[1];
		argoff = 1;
	}

#ifdef KERBEROS
#ifdef CRYPT
#define	OPTIONS	"8KLdek:l:nwx"	/* kerberos & crypt */
#else /* CRYPT */
#define	OPTIONS	"8KLdek:l:nw"	/* kerberos */
#endif /* CRYPT */
#else /* KERBEROS */
#ifdef USE_SSL
#define	OPTIONS	"8Ldel:nwK:k:S:"	/* ssl */
#else /* USE_SSL */
#define	OPTIONS	"8KLdel:nw"	/* regular */
#endif /* USE_SSL */
#endif /* KERBEROS */
	while ((ch = getopt(argc - argoff, argv + argoff, OPTIONS)) != EOF)
		switch(ch) {
		case 'K':
#ifdef KERBEROS
			use_kerberos = 0;
#endif
#ifdef USE_SSL
			certDir = optarg;
#endif
			break;
		case 'S':
#ifdef USE_SSL
		        sslflags = ssl_ParseFlags(optarg, &ssl_encrypt, 0);
#endif /* USE_SSL */
			break;
		case 'L':      /* -8Lew are ignored to allow rlogin aliases */
		case 'e':
		case 'w':
		case '8':
			break;
		case 'd':
			dflag = 1;
			break;
		case 'l':
			user = optarg;
			break;
		case 'k':
#ifdef KERBEROS
			dest_realm = dst_realm_buf;
			strncpy(dest_realm, optarg, REALM_SZ);
#endif
#ifdef USE_SSL
			nickname = optarg;
#endif
			break;
		case 'n':
			nflag = 1;
			break;
#ifdef KERBEROS
#ifdef CRYPT
		case 'x':
			doencrypt = 1;
			des_set_key(cred.session, schedule);
			break;
#endif
#endif
		case '?':
		default:
			usage();
		}
	optind += argoff;

	/* if haven't gotten a host yet, do so */
	if (!host && !(host = argv[optind++]))
		usage();

	/* if no further arguments, must have been called as rlogin. */
	if (!argv[optind]) {
	        if (asrsh)
			*argv = "rlogin";
		execv(_PATH_RLOGIN, argv);
		err(1, "can't exec %s", _PATH_RLOGIN);
	}

	argc -= optind;
	argv += optind;

	if (!(pw = getpwuid(uid = getuid())))
		errx(1, "unknown user id");
	if (!user)
		user = pw->pw_name;

#ifdef KERBEROS
#ifdef CRYPT
	/* -x turns off -n */
	if (doencrypt)
		nflag = 0;
#endif
#endif

	args = copyargs(argv);

	sp = NULL;
#ifdef KERBEROS
	if (use_kerberos) {
		sp = getservbyname((doencrypt ? "ekshell" : "kshell"), "tcp");
		if (sp == NULL) {
			use_kerberos = 0;
			warning("can't get entry for %s/tcp service",
			    doencrypt ? "ekshell" : "kshell");
		}
	}
#endif
#ifdef USE_SSL
	if (ssl_encrypt) {
		sp = getservbyname("shells", "tcp");
	}
#endif

	if (sp == NULL)
		sp = getservbyname("shell", "tcp");
	if (sp == NULL)
		errx(1, "shell/tcp: unknown service");

#ifdef KERBEROS
try_connect:
	if (use_kerberos) {
		struct hostent *hp;

		/* fully qualify hostname (needed for krb_realmofhost) */
		hp = gethostbyname(host);
		if (hp != NULL && !(host = strdup(hp->h_name)))
			err(1, NULL);

		rem = KSUCCESS;
		errno = 0;
		if (dest_realm == NULL)
			dest_realm = krb_realmofhost(host);

#ifdef CRYPT
		if (doencrypt)
			rem = krcmd_mutual(&host, sp->s_port, user, args,
			    &rfd2, dest_realm, &cred, schedule);
		else
#endif
			rem = krcmd(&host, sp->s_port, user, args, &rfd2,
			    dest_realm);
		if (rem < 0) {
			use_kerberos = 0;
			sp = getservbyname("shell", "tcp");
			if (sp == NULL)
				errx(1, "shell/tcp: unknown service");
			if (errno == ECONNREFUSED)
				warning("remote host doesn't support Kerberos");
			if (errno == ENOENT)
				warning("can't provide Kerberos auth data");
			goto try_connect;
		}
	} else {
		if (doencrypt)
			errx(1, "the -x flag requires Kerberos authentication");
		rem = rcmd(&host, sp->s_port, pw->pw_name, user, args, &rfd2);
	}
#else /* KERBEROS */
#ifdef USE_SSL
	rem = SSL_Rcmd(&host, sp->s_port, pw->pw_name, user, args, &rfd2, sslflags, certDir, nickname);
#else /* USE_SSL */
	rem = rcmd(&host, sp->s_port, pw->pw_name, user, args, &rfd2);
#endif /* USE_SSL */
#endif /* KERBEROS */


	if (rem <= 0)
		exit(1);

	if (rfd2 < 0)
		errx(1, "can't establish stderr");

	if (dflag) {
		if (setsockopt(rem, SOL_SOCKET, SO_DEBUG, &one,
		    sizeof(one)) < 0)
			warn("setsockopt");
		if (setsockopt(rfd2, SOL_SOCKET, SO_DEBUG, &one,
		    sizeof(one)) < 0)
			warn("setsockopt");
	}

	(void)setuid(uid);
	omask = sigblock(sigmask(SIGINT)|sigmask(SIGQUIT)|sigmask(SIGTERM));
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGINT, sendsig);
	if (signal(SIGQUIT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGQUIT, sendsig);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		(void)signal(SIGTERM, sendsig);

	if (!nflag) {
		pid = fork();
		if (pid < 0)
			err(1, "fork");
	}

#ifdef KERBEROS
#ifdef CRYPT
	if (!doencrypt)
#endif
#endif
	{
		(void)ioctl(rfd2, FIONBIO, &one);
		(void)ioctl(rem, FIONBIO, &one);
	}


	talk(nflag, omask, pid, rem);

	if (!nflag)
		(void)kill(pid, SIGKILL);
	exit(0);
}


void
talk(nflag, omask, pid, rem)
	int nflag;
	long omask;
	pid_t pid;
	int rem;
{
	int cc, wc;
	fd_set readfrom, ready, rembits;
	char *bp, buf[BUFSIZ];
	int maxfd;

	if (!nflag && pid == 0) {
		SSL_Close(rfd2);

reread:		errno = 0;
		if ((cc = read(0, buf, sizeof buf)) <= 0)
			goto done;
		bp = buf;

rewrite:	
		FD_ZERO(&rembits);
		FD_SET(rem, &rembits);
		if (select(rem+1, 0, &rembits, 0, 0) < 0) {
			if (errno != EINTR)
				err(1, "select1");
			goto rewrite;
		}
		if (!FD_ISSET(rem, &rembits))
			goto rewrite;
#ifdef USE_SSL
		wc = SSL_Write(rem, bp, cc);
#else /* USE_SSL */
#ifdef KERBEROS
#ifdef CRYPT
		if (doencrypt)
			wc = des_write(rem, bp, cc);
		else
#endif /* CRYPT */
#endif /* KERBEROS */
			wc = write(rem, bp, cc);
#endif /* USE_SSL */
		if (wc < 0) {
			if (errno == EWOULDBLOCK)
				goto rewrite;
			goto done;
		}
		bp += wc;
		cc -= wc;
		if (cc == 0)
			goto reread;
		goto rewrite;
done:
		/* XXX THIS NEEDS TO BE SSL_SOMETHING */
		(void)shutdown(rem, 1);
		exit(0);
	}

	(void)sigsetmask(omask);
	FD_ZERO(&readfrom);
	FD_SET(rfd2, &readfrom);
	FD_SET(rem, &readfrom);
	maxfd = ( rfd2 > rem ) ? rfd2 : rem;
	do {
		ready = readfrom;
		if (select(maxfd + 1, &ready, 0, 0, 0) < 0) {
			if (errno != EINTR)
				err(1, "select2");
			continue;
		}
		if (FD_ISSET(rfd2, &ready)) {
			errno = 0;

#ifdef USE_SSL
			cc = SSL_Read(rfd2, buf, sizeof buf);
#else /* USE_SSL */
#ifdef KERBEROS
#ifdef CRYPT
			if (doencrypt)
				cc = des_read(rfd2, buf, sizeof buf);
			else
#endif /* CRYPT */
#endif /* KERBEROS */
				cc = read(rfd2, buf, sizeof buf);
#endif /* USE_SSL */
			if (cc <= 0) {
#ifdef USE_SSL
				if ((cc == 0) || (PORT_GetError() != EWOULDBLOCK)) {
#else
				if ((cc == 0) || (errno != EWOULDBLOCK)) {
#endif /* USE_SSL */
					FD_CLR(rfd2, &readfrom);
				}
			} else {
				(void)write(2, buf, cc);
			}
		}
		if (FD_ISSET(rem, &ready)) {
			errno = 0;
#ifdef USE_SSL
			cc = SSL_Read(rem, buf, sizeof buf);
#else /* USE_SSL */
#ifdef KERBEROS
#ifdef CRYPT
			if (doencrypt)
				cc = des_read(rem, buf, sizeof buf);
			else
#endif /* CRYPT */
#endif /* KERBEROS */
				cc = read(rem, buf, sizeof buf);
#endif /* USE_SSL */
			if (cc <= 0) {
#ifdef USE_SSL
                                if ((cc == 0) || (PORT_GetError() != EWOULDBLOCK)) {
#else
                                if ((cc == 0) || (errno != EWOULDBLOCK)) {
#endif /* USE_SSL */
					FD_CLR(rem, &readfrom);
				}
			} else {
				(void)write(1, buf, cc);
			}
		}
	} while (FD_ISSET(rfd2, &readfrom) || FD_ISSET(rem, &readfrom));
}

void
sendsig(sig)
	int sig;
{
	char signo;

	signo = sig;
#ifdef USE_SSL
	SSL_Write(rfd2, &signo, 1);
#else /* USE_SSL */
#ifdef KERBEROS
#ifdef CRYPT
	if (doencrypt)
		(void)des_write(rfd2, &signo, 1);
	else
#endif /* CRYPT */
#endif /* KERBEROS */
		(void)write(rfd2, &signo, 1);
#endif /* USE_SSL */
}

#ifdef KERBEROS
/* VARARGS */
void
warning(va_alist)
va_dcl
{
	va_list ap;
	char *fmt;

	(void)fprintf(stderr, "rsh: warning, using standard rsh: ");
	va_start(ap);
	fmt = va_arg(ap, char *);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	(void)fprintf(stderr, ".\n");
}
#endif

char *
copyargs(argv)
	char **argv;
{
	int cc;
	char **ap, *args, *p;

	cc = 0;
	for (ap = argv; *ap; ++ap)
		cc += strlen(*ap) + 1;
	if (!(args = malloc((u_int)cc)))
		err(1, NULL);
	for (p = args, ap = argv; *ap; ++ap) {
		(void)strcpy(p, *ap);
		for (p = strcpy(p, *ap); *p; ++p);
		if (ap[1])
			*p++ = ' ';
	}
	return (args);
}

void
usage()
{

	(void)fprintf(stderr,
	    "usage: rsh [-nd%s]%s[-l login] host [command]\n",
#ifdef KERBEROS
#ifdef CRYPT
	    "x", " [-k realm] ");
#else
	    "", " [-k realm] ");
#endif
#else
#ifdef USE_SSL
	    "", " [-S secargs] [-K certdir] [-k name] ");
#else /* USE_SSL */
	    "", " ");
#endif /* USE_SSL */
#endif
	exit(1);
}
