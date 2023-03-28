#include <nspr/prprf.h>
#include <nspr/prlog.h>
#include <nspr/prlong.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define countof(a) (sizeof(a)/sizeof(a[0]))

/*
** Perform a three way test against PR_smprintf, PR_snprintf, and sprintf.
** Make sure the results are identical
*/
static void test_i(char *pattern, int i)
{
    char *s;
    char buf[200];
    int n;
    static char sbuf[20000];

    /* try all three routines */
    s = PR_smprintf(pattern, i);
    PR_ASSERT(s != 0);
    n = PR_snprintf(buf, sizeof(buf), pattern, i);
    PR_ASSERT(n <= sizeof(buf));
    sprintf(sbuf, pattern, i);

    /* compare results */
    if ((strncmp(s, buf, sizeof(buf)) != 0) ||
	(strncmp(s, sbuf, sizeof(sbuf)) != 0)) {
	fprintf(stderr,
	   "pattern='%s' i=%d\nPR_smprintf='%s'\nPR_snprintf='%s'\n    sprintf='%s'\n",
	   pattern, i, s, buf, sbuf);
	exit(-1);
    }
}

static void TestI(void)
{
    static int nums[] = {
	0, 1, -1, 10, -10,
	32767, -32768,
    };
    static char *signs[] = {
	"",
	"0",	"-",	"+",
	"0-",	"0+",	"-0",	"-+",	"+0",	"+-",
	"0-+",	"0+-",	"-0+",	"-+0",	"+0-",	"+-0",
    };
    static char *precs[] = {
	"", "3", "5", "43",
	"7.3", "7.5", "7.11", "7.43",
    };
    static char *formats[] = {
	"d", "o", "x", "u",
	"hd", "ho", "hx", "hu"
    };
    int f, s, n, p;
    char fmt[20];

    for (f = 0; f < countof(formats); f++) {
	for (s = 0; s < countof(signs); s++) {
	    for (p = 0; p < countof(precs); p++) {
		fmt[0] = '%';
		fmt[1] = 0;
		if (signs[s]) strcat(fmt+strlen(fmt), signs[s]);
		if (precs[p]) strcat(fmt+strlen(fmt), precs[p]);
		if (formats[f]) strcat(fmt+strlen(fmt), formats[f]);
		for (n = 0; n < countof(nums); n++) {
		    test_i(fmt, nums[n]);
		}
	    }
	}
    }
}

/************************************************************************/

/*
** Perform a three way test against PR_smprintf, PR_snprintf, and sprintf.
** Make sure the results are identical
*/
static void test_l(char *pattern, long l)
{
    char *s;
    char buf[200];
    int n;
    static char sbuf[20000];

    /* try all three routines */
    s = PR_smprintf(pattern, l);
    PR_ASSERT(s != 0);
    n = PR_snprintf(buf, sizeof(buf), pattern, l);
    PR_ASSERT(n <= sizeof(buf));
    sprintf(sbuf, pattern, l);

    /* compare results */
    if ((strncmp(s, buf, sizeof(buf)) != 0) ||
	(strncmp(s, sbuf, sizeof(sbuf)) != 0)) {
	fprintf(stderr,
	   "pattern='%s' l=%ld\nPR_smprintf='%s'\nPR_snprintf='%s'\n    sprintf='%s'\n",
	   pattern, l, s, buf, sbuf);
	exit(-1);
    }
}

static void TestL(void)
{
    static long nums[] = {
	0, 1, -1, 10, -10,
	32767, -32768,
	2147483647L, -2147483648L
    };
    static char *signs[] = {
	"",
	"0",	"-",	"+",
	"0-",	"0+",	"-0",	"-+",	"+0",	"+-",
	"0-+",	"0+-",	"-0+",	"-+0",	"+0-",	"+-0",
    };
    static char *precs[] = {
	"", "3", "5", "43",
	".3", ".43",
	"7.3", "7.5", "7.11", "7.43",
    };
    static char *formats[] = { "ld", "lo", "lx", "lu" };
    int f, s, n, p;
    char fmt[40];

    for (f = 0; f < countof(formats); f++) {
	for (s = 0; s < countof(signs); s++) {
	    for (p = 0; p < countof(precs); p++) {
		fmt[0] = '%';
		fmt[1] = 0;
		if (signs[s]) strcat(fmt+strlen(fmt), signs[s]);
		if (precs[p]) strcat(fmt+strlen(fmt), precs[p]);
		if (formats[f]) strcat(fmt+strlen(fmt), formats[f]);
		for (n = 0; n < countof(nums); n++) {
		    test_l(fmt, nums[n]);
		}
	    }
	}
    }
}

/************************************************************************/

/*
** Perform a three way test against PR_smprintf, PR_snprintf, and sprintf.
** Make sure the results are identical
*/
static void test_ll(char *pattern, int64 l)
{
    char *s;
    char buf[200];
    int n;
    static char sbuf[20000];

    /* try all three routines */
    s = PR_smprintf(pattern, l);
    PR_ASSERT(s != 0);
    n = PR_snprintf(buf, sizeof(buf), pattern, l);
    PR_ASSERT(n <= sizeof(buf));
    sprintf(sbuf, pattern, l);

    /* compare results */
    if ((strncmp(s, buf, sizeof(buf)) != 0) ||
	(strncmp(s, sbuf, sizeof(sbuf)) != 0)) {
	fprintf(stderr,
	   "pattern='%s' ll=%lld\nPR_smprintf='%s'\nPR_snprintf='%s'\n    sprintf='%s'\n",
	   pattern, l, s, buf, sbuf);
	exit(-1);
    }
}

static void TestLL(void)
{
#ifdef HAVE_LONG_LONG
/* XXX lazy kipp */
    static int64 nums[] = {
	0, 1, -1, 10, -10,
	32767, -32768,
	2147483647L, -2147483648L,
#ifndef XP_PC
	9223372036854775807LL, -9223372036854775808LL
#else
	9223372036854775807i64, -9223372036854775808i64
#endif
    };
    static char *signs[] = {
	"",
	"0",	"-",	"+",
	"0-",	"0+",	"-0",	"-+",	"+0",	"+-",
	"0-+",	"0+-",	"-0+",	"-+0",	"+0-",	"+-0",
    };
    static char *precs[] = {
	"", "3", "5", "43",
	".3", ".43",
	"7.3", "7.5", "7.11", "7.43",
    };
    static char *formats[] = { "lld", "llo", "llx", "llu" };
    int f, s, n, p;
    char fmt[40];

    for (f = 0; f < countof(formats); f++) {
	for (s = 0; s < countof(signs); s++) {
	    for (p = 0; p < countof(precs); p++) {
		fmt[0] = '%';
		fmt[1] = 0;
		if (signs[s]) strcat(fmt+strlen(fmt), signs[s]);
		if (precs[p]) strcat(fmt+strlen(fmt), precs[p]);
		if (formats[f]) strcat(fmt+strlen(fmt), formats[f]);
		for (n = 0; n < countof(nums); n++) {
		    test_ll(fmt, nums[n]);
		}
	    }
	}
    }
#endif
}

/************************************************************************/

/*
** Perform a three way test against PR_smprintf, PR_snprintf, and sprintf.
** Make sure the results are identical
*/
static void test_s(char *pattern, char *ss)
{
    char *s;
    unsigned char before[8];
    char buf[200];
    unsigned char after[8];
    int n;
    static char sbuf[20000];

    memset(before, 0xBB, 8);
    memset(after, 0xAA, 8);

    /* try all three routines */
    s = PR_smprintf(pattern, ss);
    PR_ASSERT(s != 0);
    n = PR_snprintf(buf, sizeof(buf), pattern, ss);
    PR_ASSERT(n <= sizeof(buf));
    sprintf(sbuf, pattern, ss);

    for (n = 0; n < 8; n++) {
	PR_ASSERT(before[n] == 0xBB);
	PR_ASSERT(after[n] == 0xAA);
    }

    /* compare results */
    if ((strncmp(s, buf, sizeof(buf)) != 0) ||
	(strncmp(s, sbuf, sizeof(sbuf)) != 0)) {
	fprintf(stderr,
	   "pattern='%s' ss=%.20s\nPR_smprintf='%s'\nPR_snprintf='%s'\n    sprintf='%s'\n",
	   pattern, ss, s, buf, sbuf);
	exit(-1);
    }
}

static void TestS(void)
{
    static char *strs[] = {
	"",
	"a",
	"abc",
	"abcde",
	"abcdefABCDEF",
	"abcdefghijklmnopqrstuvwxyz0123456789!@#$"
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$"
	    "abcdefghijklmnopqrstuvwxyz0123456789!@#$",
    };
    static char *signs[] = {
	"",
	"0",	"-",	"+",
	"0-",	"0+",	"-0",	"-+",	"+0",	"+-",
	"0-+",	"0+-",	"-0+",	"-+0",	"+0-",	"+-0",
    };
    static char *precs[] = {
	"", "3", "5", "43",
	".3", ".43",
	"7.3", "7.5", "7.11", "7.43",
    };
    static char *formats[] = { "s" };
    int f, s, n, p;
    char fmt[40];

    for (f = 0; f < countof(formats); f++) {
	for (s = 0; s < countof(signs); s++) {
	    for (p = 0; p < countof(precs); p++) {
		fmt[0] = '%';
		fmt[1] = 0;
		if (signs[s]) strcat(fmt+strlen(fmt), signs[s]);
		if (precs[p]) strcat(fmt+strlen(fmt), precs[p]);
		if (formats[f]) strcat(fmt+strlen(fmt), formats[f]);
		for (n = 0; n < countof(strs); n++) {
		    test_s(fmt, strs[n]);
		}
	    }
	}
    }
}

/************************************************************************/

int main(int argc, char **argv)
{
    TestI();
    TestL();
#ifndef __alpha
    /* This test doesn't work on the alpha because their sprintf doesn't
       support %lld and so on */
    TestLL();
#endif
    TestS();
    return 0;
}
