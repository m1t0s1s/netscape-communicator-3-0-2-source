#ifndef prlong_h___
#define prlong_h___

/*
** Long-long (64-bit signed integer type) support.
*/
#include "prmacros.h"
#include "prtypes.h"
#include "prosdep.h"
#include "prcpucfg.h"

NSPR_BEGIN_EXTERN_C

#ifdef HAVE_LONG_LONG

#if defined(OSF1) && defined(__alpha) && defined(__cplusplus)
/*
 *    OSF cxx won't compile long long, but long's are 64 bits anyway.
 */
typedef long int64;
typedef unsigned long uint64;

#define LL_MAXINT		0x7fffffffffffffffL
#define LL_MININT		0x8000000000000000L
#define LL_ZERO			0x0L

#elif defined(_WIN32)

typedef LONGLONG  int64;
typedef DWORDLONG uint64;

#define LL_MAXINT		0x7fffffffffffffffi64
#define LL_MININT		0x8000000000000000i64
#define LL_ZERO			0x0i64

#else

typedef long long int64;
typedef unsigned long long uint64;

#define LL_MAXINT		0x7fffffffffffffffLL
#define LL_MININT		0x8000000000000000LL
#define LL_ZERO			0x0LL

#endif /* variations on typedef-ing int64 and uint64 */

#define LL_IS_ZERO(a)		((a) == 0)
#define LL_EQ(a, b)		((a) == (b))
#define LL_NE(a, b)		((a) != (b))
#define LL_GE_ZERO(a)		((a) >= 0)
#define LL_CMP(a, op, b)	((a) op (b))

#define LL_AND(r, a, b)		((r) = (a) & (b))
#define LL_OR(r, a, b)		((r) = (a) | (b))
#define LL_XOR(r, a, b)		((r) = (a) ^ (b))
#define LL_OR2(r, a)		((r) = (r) | (a))
#define LL_NOT(r, a)		((r) = ~(a))

#define LL_NEG(r, a)		((r) = -(a))
#define LL_ADD(r, a, b)		((r) = (a) + (b))
#define LL_SUB(r, a, b)		((r) = (a) - (b))

#define LL_MUL(r, a, b)		((r) = (a) * (b))
#define LL_DIV(r, a, b)		((r) = (a) / (b))
#define LL_MOD(r, a, b)		((r) = (a) % (b))

#define LL_SHL(r, a, b)		((r) = (a) << (b))
#define LL_SHR(r, a, b)		((r) = (a) >> (b))
#define LL_USHR(r, a, b)	((r) = (uint64)(a) >> (b))
#define LL_ISHL(r, a, b)	((r) = ((int64)(a)) << (b))

#define LL_L2I(i, l)		((i) = (int)(l))
#define LL_L2UI(ui, l)		((ui) =(unsigned int)(l))
#define LL_L2F(f, l)		((f) = (l))
#define LL_L2D(d, l)		((d) = (l))

#define LL_I2L(l, i)		((l) = (i))
#define LL_UI2L(l, ui)		((l) = (ui))
#define LL_F2L(l, f)		((l) = (f))
#define LL_D2L(l, d)		((l) = (d))

#define LL_UDIVMOD(qp, rp, a, b)  \
    (*(qp) = ((uint64)(a) / (b)), \
     *(rp) = ((uint64)(a) % (b)))

#else  /* !HAVE_LONG_LONG */

typedef struct {
#ifdef IS_LITTLE_ENDIAN
    uint32 lo, hi;
#else
    uint32 hi, lo;
#endif
} int64;
typedef int64				uint64;

extern int64 LL_MAXINT, LL_MININT, LL_ZERO;

#define LL_IS_ZERO(a)		(((a).hi == 0) && ((a).lo == 0))
#define LL_EQ(a, b)		(((a).hi == (b).hi) && ((a).lo == (b).lo))
#define LL_NE(a, b)		(((a).hi != (b).hi) || ((a).lo != (b).lo))
#define LL_GE_ZERO(a)		(((a).hi >> 31) == 0)

/*
 * NB: LL_CMP and LL_UCMP work only for strict relationals (<, >).
 */
#define LL_CMP(a, op, b)	(((int32)(a).hi op (int32)(b).hi) ||          \
				 (((a).hi == (b).hi) && ((a).lo op (b).lo)))
#define LL_UCMP(a, op, b)	(((a).hi op (b).hi) ||                        \
				 (((a).hi == (b).hi) && ((a).lo op (b).lo)))

#define LL_AND(r, a, b)		((r).lo = (a).lo & (b).lo,                    \
				 (r).hi = (a).hi & (b).hi)
#define LL_OR(r, a, b)		((r).lo = (a).lo | (b).lo,                    \
				 (r).hi = (a).hi | (b).hi)
#define LL_XOR(r, a, b)		((r).lo = (a).lo ^ (b).lo,                    \
				 (r).hi = (a).hi ^ (b).hi)
#define LL_OR2(r, a)		((r).lo = (r).lo | (a).lo,                    \
				 (r).hi = (r).hi | (a).hi)
#define LL_NOT(r, a)		((r).lo = ~(a).lo,	                      \
				 (r).hi = ~(a).hi)

#define LL_NEG(r, a)		((r).lo = -(int32)(a).lo,                     \
				 (r).hi = -(int32)(a).hi - ((r).lo != 0))
#define LL_ADD(r, a, b) {                                                     \
    int64 _a, _b;							      \
    _a = a; _b = b;                                                           \
    (r).lo = _a.lo + _b.lo;                                                   \
    (r).hi = _a.hi + _b.hi + ((r).lo < _b.lo);                                \
}

#define LL_SUB(r, a, b) {                                                     \
    int64 _a, _b;							      \
    _a = a; _b = b;                                                           \
    (r).lo = _a.lo - _b.lo;                                                   \
    (r).hi = _a.hi - _b.hi - (_a.lo < _b.lo);                                 \
}                                                                             \

/*
 * Multiply 64-bit operands a and b to get 64-bit result r.
 * First multiply the low 32 bits of a and b to get a 64-bit result in r.
 * Then add the outer and inner products to r.hi.
 */
#define LL_MUL(r, a, b) {                                                     \
    int64 _a, _b;							      \
    _a = a; _b = b;                                                           \
    LL_MUL32(r, _a.lo, _b.lo);                                                \
    (r).hi += _a.hi * _b.lo + _a.lo * _b.hi;                                  \
}

/* XXX _lo16(a) = ((a) << 16 >> 16) is better on some archs (not on mips) */
#define _lo16(a)		((a) & PR_BITMASK(16))
#define _hi16(a)		((a) >> 16)

/*
 * Multiply 32-bit operands a and b to get 64-bit result r.
 * Use polynomial expansion based on primitive field element (1 << 16).
 */
#define LL_MUL32(r, a, b) {                                                   \
     uint32 _a1, _a0, _b1, _b0, _y0, _y1, _y2, _y3;                           \
     _a1 = _hi16(a), _a0 = _lo16(a);                                          \
     _b1 = _hi16(b), _b0 = _lo16(b);                                          \
     _y0 = _a0 * _b0;                                                         \
     _y1 = _a0 * _b1;                                                         \
     _y2 = _a1 * _b0;                                                         \
     _y3 = _a1 * _b1;                                                         \
     _y1 += _hi16(_y0);                         /* can't carry */             \
     _y1 += _y2;                                /* might carry */             \
     if (_y1 < _y2) _y3 += 1 << 16;             /* propagate */               \
     (r).lo = (_lo16(_y1) << 16) + _lo16(_y0);                                \
     (r).hi = _y3 + _hi16(_y1);                                               \
}

/*
 * Divide 64-bit unsigned operand a by 64-bit unsigned operand b, setting *qp
 * to the 64-bit unsigned quotient, and *rp to the 64-bit unsigned remainder.
 * Minimize effort if one of qp and rp is null.
 */
#define LL_UDIVMOD(qp, rp, a, b)	ll_udivmod(qp, rp, a, b)

extern PR_PUBLIC_API(void) ll_udivmod(uint64 *qp, uint64 *rp, uint64 a, 
                                     uint64 b);

#define LL_DIV(r, a, b) {                                                     \
    int64 _a, _b;                                                             \
    uint32 _negative = (int32)(a).hi < 0;                                     \
    if (_negative) {                                                          \
	LL_NEG(_a, a);                                                        \
    } else {                                                                  \
	_a = a;                                                               \
    }                                                                         \
    if ((int32)(b).hi < 0) {                                                  \
	_negative ^= 1L;                                                       \
	LL_NEG(_b, b);                                                        \
    } else {                                                                  \
	_b = b;                                                               \
    }                                                                         \
    LL_UDIVMOD(&(r), 0, _a, _b);                                              \
    if (_negative)                                                            \
	LL_NEG(r, r);                                                         \
}

#define LL_MOD(r, a, b) {                                                     \
    int64 _a, _b;                                                             \
    uint32 _negative = (int32)(a).hi < 0;                                     \
    if (_negative) {                                                          \
	LL_NEG(_a, a);                                                        \
    } else {                                                                  \
	_a = a;                                                               \
    }                                                                         \
    if ((int32)(b).hi < 0) {                                                  \
	LL_NEG(_b, b);                                                        \
    } else {                                                                  \
	_b = b;                                                               \
    }                                                                         \
    LL_UDIVMOD(0, &(r), _a, _b);                                              \
    if (_negative)                                                            \
	LL_NEG(r, r);                                                         \
}

/*
 * NB: b is a uint32, not int64 or uint64, for the shift ops.
 */
#define LL_SHL(r, a, b) {                                                     \
    if (b) {                                                                  \
	int64 _a;							      \
        _a = a;                                                               \
        if ((b) < 32) {                                                       \
	    (r).lo = _a.lo << (b);                                            \
	    (r).hi = (_a.hi << (b)) | (_a.lo >> (32 - (b)));                  \
	} else {                                                              \
	    (r).lo = 0;                                                       \
	    (r).hi = _a.lo << ((b) & 31);                                     \
	}                                                                     \
    } else {                                                                  \
	(r) = (a);                                                            \
    }                                                                         \
}

/* a is an int32, b is int32, r is int64 */
#define LL_ISHL(r, a, b) {                                                    \
    if (b) {                                                                  \
	int64 _a;                                                             \
	_a.lo = (a);                                                          \
	_a.hi = 0;                                                            \
        if ((b) < 32) {                                                       \
	    (r).lo = (a) << (b);                                              \
	    (r).hi = ((a) >> (32 - (b)));                                     \
	} else {                                                              \
	    (r).lo = 0;                                                       \
	    (r).hi = (a) << ((b) & 31);                                       \
	}                                                                     \
    } else {                                                                  \
	(r).lo = (a);                                                         \
	(r).hi = 0;                                                           \
    }                                                                         \
}

#define LL_SHR(r, a, b) {                                                     \
    if (b) {                                                                  \
	int64 _a;							      \
        _a = a;                                                               \
	if ((b) < 32) {                                                       \
	    (r).lo = (_a.hi << (32 - (b))) | (_a.lo >> (b));                  \
	    (r).hi = (int32)_a.hi >> (b);                                     \
	} else {                                                              \
	    (r).lo = (int32)_a.hi >> ((b) & 31);                              \
	    (r).hi = (int32)_a.hi >> 31;                                      \
	}                                                                     \
    } else {                                                                  \
	(r) = (a);                                                            \
    }                                                                         \
}

#define LL_USHR(r, a, b) {                                                    \
    if (b) {                                                                  \
	int64 _a;							      \
        _a = a;                                                               \
	if ((b) < 32) {                                                       \
	    (r).lo = (_a.hi << (32 - (b))) | (_a.lo >> (b));                  \
	    (r).hi = _a.hi >> (b);                                            \
	} else {                                                              \
	    (r).lo = _a.hi >> ((b) & 31);                                     \
	    (r).hi = 0;                                                       \
	}                                                                     \
    } else {                                                                  \
	(r) = (a);                                                            \
    }                                                                         \
}

#define LL_L2I(i, l)		((i) = (l).lo)
#define LL_L2UI(ui, l)		((ui) = (l).lo)
#define LL_L2F(f, l)		{ double _d; LL_L2D(_d, l); (f) = (float) _d; }

#define LL_L2D(d, l) {                                                        \
    int32 _negative;                                                            \
    int64 _absval;                                                            \
                                                                              \
    _negative = (l).hi >> 31;                                                 \
    if (_negative) {                                                          \
	LL_NEG(_absval, l);                                                   \
    } else {                                                                  \
	_absval = l;                                                          \
    }                                                                         \
    (d) = (double)_absval.hi * 4.294967296e9 + _absval.lo;                    \
    if (_negative)                                                            \
	(d) = -(d);                                                           \
}

#define LL_I2L(l, i)		{ int32 _i = (i); (l).hi = _i >> 31;          \
						  (l).lo = _i; }
#define LL_UI2L(l, ui)		{ (l).hi = 0; (l).lo = (ui); }
#define LL_F2L(l, f)		{ double _d = (double) f; LL_D2L(l, _d); }

#define LL_D2L(l, d) {                                                        \
    int _negative;                                                            \
    double _absval, _d_hi;                                                    \
    int64 _lo_d;                                                              \
                                                                              \
    _negative = ((d) < 0);                                                    \
    _absval = _negative ? -(d) : (d);                                         \
                                                                              \
    (l).hi = (uint32)(_absval / 4.294967296e9);                               \
    (l).lo = 0;                                                               \
    LL_L2D(_d_hi, l);                                                         \
    _absval -= _d_hi;                                                         \
    _lo_d.hi = 0;                                                             \
    if (_absval < 0) {                                                        \
	_lo_d.lo = (uint32) -_absval;                                         \
	LL_SUB(l, l, _lo_d);                                                  \
    } else {                                                                  \
	_lo_d.lo = (uint32) _absval;                                          \
	LL_ADD(l, l, _lo_d);                                                  \
    }                                                                         \
                                                                              \
    if (_negative)                                                            \
	LL_NEG(l, l);                                                         \
}

#endif /* !HAVE_LONG_LONG */

/*
** Convert a longlong into ascii. Store the ascii into s (make sure it's
** about 30 characters big for base 10 conversions).
** 	"i" the 64 bit integer to conver to ascii
** 	"radix" the radix to use (0 means use base 10), 2 means binary, etc.
** 	"s" the buffer to store the ascii conversion into
**
** Radix 8 and 16 are unsigned, radix 10 is signed and radix -10 is
** unsigned.
*/
extern PR_PUBLIC_API(char *) LL_TO_S(int64 i, int radix, char *s, size_t slen);

NSPR_END_EXTERN_C

#endif /* prlong_h___ */
