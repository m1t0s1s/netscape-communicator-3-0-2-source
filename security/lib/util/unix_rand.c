/*
 * 
 * LICENSE AGREEMENT NON-COMMERCIAL USE (Random Seed Generation)
 * 
 * Netscape Communications Corporation ("Netscape") hereby grants you a
 * non-exclusive, non-transferable license to use the source code version
 * of the accompanying random seed generation software (the "Software")
 * subject to the following terms:
 * 
 * 1.  You may use, modify and/or incorporate the Software, in whole or in
 * part, into other software programs (a "Derivative Work") solely in
 * order to evaluate the Software's features, functions and capabilities.
 * 
 * 2.  You may not use the Software or Derivative Works for
 * revenue-generating purposes.  You may not:
 * 
 * 	(a)  license or distribute the Software or any Derivative Work
 * 	in any manner that generates license fees, royalties,
 * 	maintenance fees, upgrade fees or any other form of income.
 * 
 * 	(b)  use the Software or a Derivative Work to provide services
 * 	to others for which you are compensated in any manner.
 * 
 * 	(c)  distribute the Software or a Derivative Work without
 * 	written agreement from the end user to abide by the terms of
 * 	Sections 1 and 2 of this Agreement.
 * 
 * 3.  This license is granted free of charge.
 * 
 * 4.  Any modification of the Software or Derivative Work must
 * prominently state in the code and associated documentation that it has
 * been modified, the date modified and the identity of the person(s) who
 * made the modifications.
 * 
 * 5.  Licensee will promptly notify Netscape of any proposed or
 * implemented improvement in the Software and grants Netscape the
 * perpetual right to incorporate any such improvements into the Software
 * and into Netscape's products which incorporate the Software (which may
 * be distributed to third parties).  Licensee will also promptly notify
 * Netscape of any alleged deficiency in the Software.  If Netscape
 * requests, Licensee will provide Netscape with a copy of each Derivative
 * Work Licensee creates.
 * 
 * 6.  Any copy of the Software or Derivative Work shall include a copy of
 * this Agreement, Netscape's copyright notices and the disclaimer of
 * warranty and limitation of liability.
 * 
 * 7.  Title, ownership rights, and intellectual property rights in and to
 * the Software shall remain in Netscape and/or its suppliers.  You may
 * use the Software only as provided in this Agreement. Netscape shall
 * have no obligation to provide maintenance, support, upgrades or new
 * releases to you or any person to whom you distribute the Software or a
 * Derivative Work.
 * 
 * 8.  Netscape may use Licensee's name in publicity materials as a
 * licensee of the Software.
 * 
 * 9.  Disclaimer of Warranty.  THE SOFTWARE IS LICENSED "AS IS" WITHOUT
 * WARRANTY OF ANY KIND, INCLUDING WITHOUT LIMITATION,  PERFORMANCE,
 * MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE.  THE ENTIRE RISK
 * AS TO THE RESULTS AND PERFORMANCE OF THE SOFTWARE IS ASSUMED BY YOU.
 * 
 * 10.  Limitation of Liability. UNDER NO CIRCUMSTANCES AND UNDER NO LEGAL
 * THEORY SHALL NETSCAPE OR ITS SUPPLIERS BE LIABLE TO YOU OR ANY OTHER
 * PERSON FOR ANY INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * OF ANY CHARACTER INCLUDING WITHOUT LIMITATION ANY COMMERCIAL DAMAGES OR
 * LOSSES , EVEN IF NETSCAPE HAS BEEN INFORMED OF THE POSSIBILITY OF SUCH
 * DAMAGES.
 * 
 * 11.  You may not download or otherwise export or reexport the Software
 * or any underlying information or technology except in full compliance
 * with all United States and other applicable laws and regulations.
 * 
 * 10. Either party may terminate this Agreement immediately in the event
 * of default by the other party.  You may also terminate this Agreement
 * at any time by destroying the Software and all copies thereof.
 * 
 * 11.  Use, duplication or disclosure by the United States Government is
 * subject to restrictions set forth in subparagraphs (a) through (d) of
 * the Commercial Computer-Restricted Rights clause at FAR 52.227-19 when
 * applicable, or in subparagraph (c)(1)(ii) of the Rights in Technical
 * Data and Computer Program clause at DFARS 252.227-7013, and in similar
 * clauses in the NASA FAR Supplement. Contractor/manufacturer is Netscape
 * Communications Corporation, 501 East Middlefield Road, Mountain View,
 * CA 94043.
 * 
 * 12. This Agreement shall be governed by and construed under California
 * law as such law applies to agreements between California residents
 * entered into and to be performed entirely within California, except as
 * governed by Federal law.
 * 
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <assert.h>
#include "secrng.h"

extern RNGContext *sec_base_rng;

/*
 * When copying data to the buffer we want the least signicant bytes
 * from the input since those bits are changing the fastest. The address
 * of least significant byte depends upon whether we are running on
 * a big-endian or little-endian machine.
 *
 * Does this mean the least signicant bytes are the most significant
 * to us? :-)
 */
    
static size_t CopyLowBits(void *dst, size_t dstlen, void *src, size_t srclen)
{
    union endianness {
	int32 i;
	char c[4];
    } u;

    if (srclen <= dstlen) {
	memcpy(dst, src, srclen);
	return srclen;
    }
    u.i = 0x01020304;
    if (u.c[0] == 0x01) {
	/* big-endian case */
	memcpy(dst, (char*)src + (srclen - dstlen), dstlen);
    } else {
	/* little-endian case */
	memcpy(dst, src, dstlen);
    }
    return dstlen;
}

#if defined(SCO) || defined(UNIXWARE) || defined(BSDI)
#include <unistd.h>
#include <sys/times.h>

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    int ticks;
    struct tms buffer;

    ticks=times(&buffer);
    return CopyLowBits(buf, maxbytes, &ticks, sizeof(ticks));
}

static void
GiveSystemInfo(void)
{
    long si;

    /* 
     * Is this really necessary?  Why not use rand48 or something?
     */
    si = sysconf(_SC_CHILD_MAX);
    RNG_RandomUpdate(&si, sizeof(si));

    si = sysconf(_SC_STREAM_MAX);
    RNG_RandomUpdate(&si, sizeof(si));

    si = sysconf(_SC_OPEN_MAX);
    RNG_RandomUpdate(&si, sizeof(si));
}
#endif

#if defined(__sun)
#if defined(__svr4) || defined(SVR4)
#include <sys/systeminfo.h>
#include <sys/times.h>
#include <wait.h>

int gettimeofday(struct timeval *);
int gethostname(char *, int);

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[2000];

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    hrtime_t t;
    t = gethrtime();
    if (t) {
	return CopyLowBits(buf, maxbytes, &t, sizeof(t));
    }
    return 0;
}
#else /* SunOS (Sun, but not SVR4) */

#include <sys/wait.h>
#include "sunos4.h"

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    long si;

    /* This is not very good */
    si = sysconf(_SC_CHILD_MAX);
    RNG_RandomUpdate(&si, sizeof(si));
}
#endif
#endif /* Sun */

#if defined(__hpux)
#include <sys/unistd.h>
#include <sys/wait.h>

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    extern int ret_cr16();
    int cr16val;

    cr16val = ret_cr16();
    return CopyLowBits(buf, maxbytes, &cr16val, sizeof(cr16val));
}

static void
GiveSystemInfo(void)
{
    long si;

    /* This is not very good */
    si = sysconf(_AES_OS_VERSION);
    RNG_RandomUpdate(&si, sizeof(si));
    si = sysconf(_SC_CPU_VERSION);
    RNG_RandomUpdate(&si, sizeof(si));
}
#endif /* HPUX */

#if defined(__alpha)
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/wait.h>
#include <sys/systeminfo.h>
#include <c_asm.h>

static void
GiveSystemInfo(void)
{
    char buf[BUFSIZ];
    int rv;
    int off = 0;

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}

/*
 * Use the "get the cycle counter" instruction on the alpha.
 * The low 32 bits completely turn over in less than a minute.
 * The high 32 bits are some non-counter gunk that changes sometimes.
 */
static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    unsigned long t;

    t = asm("rpcc %v0");
    return CopyLowBits(buf, maxbytes, &t, sizeof(t));
}

#endif /* Alpha */

#if defined(_IBMR2)
static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    /* XXX haven't found any yet! */
}
#endif /* IBM R2 */

#if defined(__linux)
#include <linux/kernel.h>

int putenv(const char *);

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    /* XXX sysinfo() does not seem be implemented anywhwere */
#if 0
    struct sysinfo si;
    char hn[2000];
    if (sysinfo(&si) == 0) {
	RNG_RandomUpdate(&si, sizeof(si));
    }
#endif
}
#endif /* __linux */

#if defined(NCR)

#include <sys/utsname.h>
#include <sys/systeminfo.h>

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[2000];

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}

#endif /* NCR */


#if defined(sgi)
#include <fcntl.h>
#undef PRIVATE
#include <sys/mman.h>
#include <sys/syssgi.h>
#include <sys/immu.h>
#include <sys/systeminfo.h>
#include <sys/utsname.h>
#include <wait.h>

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[4096];

    rv = syssgi(SGI_SYSID, &buf[0]);
    if (rv > 0) {
	RNG_RandomUpdate(buf, MAXSYSIDSIZE);
    }
#ifdef SGI_RDUBLK
    rv = syssgi(SGI_RDUBLK, getpid(), &buf[0], sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, sizeof(buf));
    }
#endif /* SGI_RDUBLK */
    rv = syssgi(SGI_INVENT, SGI_INV_READ, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, sizeof(buf));
    }
    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}

static size_t GetHighResClock(void *buf, size_t maxbuf)
{
    unsigned phys_addr, raddr, cycleval;
    static volatile unsigned *iotimer_addr = NULL;
    static int tries = 0;
    static int cntr_size;
    int mfd;
    long s0[2];
    struct timeval tv;

#ifndef SGI_CYCLECNTR_SIZE
#define SGI_CYCLECNTR_SIZE      165     /* Size user needs to use to read CC */
#endif

    if (iotimer_addr == NULL) {
	if (tries++ > 1) {
	    /* Don't keep trying if it didn't work */
	    return 0;
	}

	/*
	** For SGI machines we can use the cycle counter, if it has one,
	** to generate some truly random numbers
	*/
	phys_addr = syssgi(SGI_QUERY_CYCLECNTR, &cycleval);
	if (phys_addr) {
	    int pgsz = getpagesize();
	    int pgoffmask = pgsz - 1;

	    raddr = phys_addr & ~pgoffmask;
	    mfd = open("/dev/mmem", O_RDONLY);
	    if (mfd < 0) {
		return 0;
	    }
	    iotimer_addr = (unsigned *)
		mmap(0, pgoffmask, PROT_READ, MAP_PRIVATE, mfd, (int)raddr);
	    if (iotimer_addr == (void*)-1) {
		close(mfd);
		iotimer_addr = NULL;
		return 0;
	    }
	    iotimer_addr = (unsigned*)
		((__psint_t)iotimer_addr | (phys_addr & pgoffmask));
	    /*
	     * The file 'mfd' is purposefully not closed.
	     */
	    cntr_size = syssgi(SGI_CYCLECNTR_SIZE);
	    if (cntr_size < 0) {
		struct utsname utsinfo;

		/* 
		 * We must be executing on a 6.0 or earlier system, since the
		 * SGI_CYCLECNTR_SIZE call is not supported.
		 * 
		 * The only pre-6.1 platforms with 64-bit counters are
		 * IP19 and IP21 (Challenge, PowerChallenge, Onyx).
		 */
		uname(&utsinfo);
		if (!strncmp(utsinfo.machine, "IP19", 4) ||
		    !strncmp(utsinfo.machine, "IP21", 4))
			cntr_size = 64;
		else
			cntr_size = 32;
	    }
	    cntr_size /= 8;	/* Convert from bits to bytes */
	}
    }

    s0[0] = *iotimer_addr;
    if (cntr_size > 4)
	s0[1] = *(iotimer_addr + 1);
    memcpy(buf, (char *)&s0[0], cntr_size);
    return CopyLowBits(buf, maxbuf, &s0, cntr_size);
}
#endif

#if defined(sony)
#include <sys/systeminfo.h>

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[2000];

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}
#endif /* sony */

#if defined(sinix)
#include <unistd.h>
#include <sys/systeminfo.h>
#include <sys/times.h>

int gettimeofday(struct timeval *, struct timezone *);
int gethostname(char *, int);

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    int ticks;
    struct tms buffer;

    ticks=times(&buffer);
    return CopyLowBits(buf, maxbytes, &ticks, sizeof(ticks));
}

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[2000];

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}
#endif /* sinix */

#if defined(nec_ews)
#include <sys/systeminfo.h>

#define getdtablesize() sysconf(_SC_OPEN_MAX)

static size_t
GetHighResClock(void *buf, size_t maxbytes)
{
    return 0;
}

static void
GiveSystemInfo(void)
{
    int rv;
    char buf[2000];

    rv = sysinfo(SI_MACHINE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_RELEASE, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
    rv = sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
    if (rv > 0) {
	RNG_RandomUpdate(buf, rv);
    }
}
#endif /* nec_ews */

size_t RNG_GetNoise(void *buf, size_t maxbytes)
{
    struct timeval tv;
    int n = 0;
    int c;

    n = GetHighResClock(buf, maxbytes);
    maxbytes -= n;

#if defined(__sun) && (defined(_svr4) || defined(SVR4)) || defined(sony)
    (void)gettimeofday(&tv);
#else
    (void)gettimeofday(&tv, 0);
#endif
    c = CopyLowBits((char*)buf+n, maxbytes, &tv.tv_usec, sizeof(tv.tv_usec));
    n += c;
    maxbytes -= c;
    c = CopyLowBits((char*)buf+n, maxbytes, &tv.tv_sec, sizeof(tv.tv_sec));
    n += c;
    return n;
}

#define SAFE_POPEN_MAXARGS	10	/* must be at least 2 */

/*
 * safe_popen is static to this module and we know what arguments it is
 * called with. Note that this version only supports a single open child
 * process at any time.
 */
static pid_t safe_popen_pid;
static struct sigaction oldact;

static FILE *
safe_popen(char *cmd)
{
    int p[2], fd, argc;
    pid_t pid;
    char *argv[SAFE_POPEN_MAXARGS + 1];
    FILE *fp;
    static char blank[] = " \t";
    static struct sigaction newact;

    if (pipe(p) < 0)
	return 0;

    /* Setup signals so that SIGCHLD is ignored as we want to do waitpid */
    newact.sa_handler = SIG_DFL;
    newact.sa_flags = 0;
    sigfillset(&newact.sa_mask);
    sigaction (SIGCHLD, &newact, &oldact);

    pid = fork();
    switch (pid) {
      case -1:
	close(p[0]);
	close(p[1]);
	sigaction (SIGCHLD, &oldact, NULL);
	return 0;

      case 0:
	/* dup write-side of pipe to stderr and stdout */
	if (p[1] != 1) dup2(p[1], 1);
	if (p[1] != 2) dup2(p[1], 2);
	close(0);
	for (fd = getdtablesize(); --fd > 2; close(fd))
	    ;

	/* clean up environment in the child process */
	putenv("PATH=/bin:/usr/bin:/sbin:/usr/sbin:/etc:/usr/etc");
	putenv("SHELL=/bin/sh");
	putenv("IFS= \t");

	/*
	 * The caller may have passed us a string that is in text
	 * space. It may be illegal to modify the string
	 */
	cmd = strdup(cmd);
	/* format argv */
	argv[0] = strtok(cmd, blank);
	argc = 1;
	while ((argv[argc] = strtok(0, blank)) != 0) {
	    if (++argc == SAFE_POPEN_MAXARGS) {
		argv[argc] = 0;
		break;
	    }
	}

	/* and away we go */
	execvp(argv[0], argv);
	exit(127);
	break;

      default:
	close(p[1]);
	fp = fdopen(p[0], "r");
	if (fp == 0) {
	    close(p[0]);
	    sigaction (SIGCHLD, &oldact, NULL);
	    return 0;
	}
	break;
    }

    /* non-zero means there's a cmd running */
    safe_popen_pid = pid;
    return fp;
}

static int
safe_pclose(FILE *fp)
{
    pid_t pid;
    int count, status;

    if ((pid = safe_popen_pid) == 0)
	return -1;
    safe_popen_pid = 0;

    /* if the child hasn't exited, kill it -- we're done with its output */
    count = 0;
    while (waitpid(pid, &status, WNOHANG) == 0) {
    	if (kill(pid, SIGKILL) < 0 && errno == ESRCH)
	    break;
	if (++count == 1000)
	    break;
    }

    /* Reset SIGCHLD signal hander before returning */
    sigaction(SIGCHLD, &oldact, NULL);

    fclose(fp);
    return status;
}

void RNG_SystemInfoForRNG(void)
{
    FILE *fp;
    char buf[BUFSIZ];
    size_t bytes;
    extern char **environ;
    char **cp;
    char *randfile;
    char *files[] = {
	"/etc/passwd",
	"/etc/utmp",
	"/tmp",
	"/var/tmp",
	"/usr/tmp",
	0
    };

#ifdef DO_PS
For now it is considered that it is too expensive to run the ps command
for the small amount of entropy it provides.
#if defined(__sun) && (!defined(__svr4) && !defined(SVR4)) || defined(bsdi) || defined(__linux)
    static char ps_cmd[] = "ps aux";
#else
    static char ps_cmd[] = "ps -el";
#endif
#endif /* DO_PS */
    static char netstat_ni_cmd[] = "netstat -ni";

    GiveSystemInfo();

    bytes = RNG_GetNoise(buf, sizeof(buf));
    RNG_RandomUpdate(buf, bytes);

#ifdef DO_PS
    fp = safe_popen(ps_cmd);
    if (fp != NULL) {
	while ((bytes = fread(buf, 1, sizeof(buf), fp)) > 0)
	    RNG_RandomUpdate(buf, bytes);
	safe_pclose(fp);
    }
#endif
    fp = safe_popen(netstat_ni_cmd);
    if (fp != NULL) {
	while ((bytes = fread(buf, 1, sizeof(buf), fp)) > 0)
	    RNG_RandomUpdate(buf, bytes);
	safe_pclose(fp);
    }

    /*
     * Pass the C environment and the addresses of the pointers to the
     * hash function. This makes the random number function depend on the
     * execution environment of the user and on the platform the program
     * is running on.
     */
    cp = environ;
    while (*cp) {
	RNG_RandomUpdate(*cp, strlen(*cp));
	cp++;
    }
    RNG_RandomUpdate(environ, (char*)cp - (char*)environ);

    /* Give in system information */
    if (gethostname(buf, sizeof(buf)) > 0) {
	RNG_RandomUpdate(buf, strlen(buf));
    }
    GiveSystemInfo();

    /* If the user points us to a random file, pass it through the rng */
    randfile = getenv("NSRANDFILE");
    if ( ( randfile != NULL ) && ( randfile[0] != '\0') ) {
	RNG_FileForRNG(randfile);
    }

    /* pass other files through */
    for (cp = files; *cp; cp++)
	RNG_FileForRNG(*cp);

}

void RNG_FileForRNG(char *fileName)
{
    struct stat stat_buf;
    unsigned char buffer[BUFSIZ];
    size_t bytes;
    FILE *file;
    static size_t totalFileBytes = 0;
    
    if (stat((char *)fileName, &stat_buf) < 0)
	return;
    RNG_RandomUpdate(&stat_buf, sizeof(stat_buf));
    
    file = fopen((char *)fileName, "r");
    if (file != NULL) {
	for (;;) {
	    bytes = fread(buffer, 1, sizeof(buffer), file);
	    if (bytes == 0) break;
	    RNG_RandomUpdate(buffer, bytes);
	    totalFileBytes += bytes;
	    if (totalFileBytes > 1024*1024) break;
	}
	fclose(file);
    }
    /*
     * Pass yet another snapshot of our highest resolution clock into
     * the hash function.
     */
    bytes = RNG_GetNoise(buffer, sizeof(buffer));
    RNG_RandomUpdate(buffer, bytes);
}
