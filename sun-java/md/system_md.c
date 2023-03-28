#include <string.h>
#include <stdarg.h>

#include "typedefs_md.h"
#include "jmath_md.h"
#include "prprf.h"
#include "prlog.h"
#include "sys_api.h"

void ll2str(int64_t a, char *s, char *limit)
{
    LL_TO_S(a, 0, s, limit - s);
}

EXPORTED void sysExit(int rv)
{
#if !defined(XP_PC) || defined(_WIN32)
	exit(rv);
#else
    PR_Exit(); /* XXX This really won't do */
#endif

}

EXPORTED int sysAtexit(void (*func)(void))
{
    return 0;
}

EXPORTED void sysAbort(void)
{
    PR_Abort();
}

#ifndef HAVE_LONG_LONG

#ifdef IS_LITTLE_ENDIAN
int64 ll_zero_const = { 0, 0 };
int64 ll_one_const = { 1, 0 };
#else
int64 ll_zero_const = { 0, 0 };
int64 ll_one_const = { 0, 1 };
#endif

int64 int2ll(int32_t i) {
    int64 ll;
    LL_I2L(ll, i);
    return ll;
}

int64 uint2ll(uint32_t i) {
    int64 ll;
    LL_UI2L(ll, i);
    return ll;
}

int32_t ll2int(int64 ll) {
    int32_t r;
    LL_L2I(r, ll);
    return r;
}

int64 float2ll(float f) {
    int64 ll;
    LL_F2L(ll, f);
    return ll;
}

float ll2float(int64 ll) {
    float f;
    LL_L2F(f, ll);
    return f;
}

int64 double2ll(double d) {
    int64 ll;
    LL_D2L(ll, d);
    return ll;
}

double ll2double(int64 ll) {
    double d;
    LL_L2D(d, ll);
    return d;
}

int64 ll_shl(int64 ll, int32_t shift) {
    int64 r;
    LL_SHL(r, ll, shift);
    return r;
}
int64 ll_shr(int64 ll, int32_t shift) {
    int64 r;
    LL_SHR(r, ll, shift);
    return r;
}
int64 ll_ushr(int64 ll, int32_t shift) {
    int64 r;
    LL_USHR(r, ll, shift);
    return r;
}

int64 ll_neg(int64 a) {
    LL_NEG(a, a);
    return a;
}

int64 ll_add(int64 a, int64 b) {
    LL_ADD(a, a, b);
    return a;
}

int64 ll_sub(int64 a, int64 b) {
    LL_SUB(a, a, b);
    return a;
}

int64 ll_mul(int64 a, int64 b) {
    LL_MUL(a, a, b);
    return a;
}

int64 ll_div(int64 a, int64 b) {
    LL_DIV(a, a, b);
    return a;
}

int64 ll_rem(int64 a, int64 b) {
    LL_MOD(a, a, b);
    return a;
}

int64 ll_and(int64 a, int64 b) {
    LL_AND(a, a, b);
    return a;
}

int64 ll_or(int64 a, int64 b) {
    LL_OR(a, a, b);
    return a;
}

int64 ll_xor(int64 a, int64 b) {
    LL_XOR(a, a, b);
    return a;
}

/*
** XXX semantics of this are weakly defined by java...It will need
** XXX updating once they pin it down
*/
int64 a2ll(char *s) {
    int64 r, digit, ten;
    char ch;
    int started = 0;
    int sign = 0;

    LL_I2L(ten, 10);
    r = ll_zero_const;
    while ((ch = *s++) != 0) {
	if (!started && !sign && (ch == '-')) {
	    sign = 1;
	    continue;
	}
	if ((ch >= '0') && (ch <= '9')) {
	    started = 1;
	    ch -= '0';
	    LL_I2L(digit, ch);
	    LL_MUL(r, r, ten);
	    LL_ADD(r, r, digit);
	} else
	    break;
    }
    if (sign) {
	LL_NEG(r, r);
    }
    return r;
}

#endif /* ! HAVE_LONG_LONG */
