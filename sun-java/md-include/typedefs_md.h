#ifndef sun_java_typedefs_md_h___
#define sun_java_typedefs_md_h___

/* Hack to work around typedef's in sun's sys_api.h */
#define sys_thread PRThreadStr
#define sys_mon PRMonitorStr

#include "bool.h"
#include "nspr_md.h"
#include "prlong.h"
#include "prntohl.h"
#include "prmem.h"
#include "prglobal.h"

#include <sys/types.h>

#if defined(LINUX) || defined(SCO)
#include <sys/bitypes.h>
#endif

#if defined(__sun) || defined(HPUX) || defined(XP_PC) || defined(AIX) || defined(OSF1) || defined(XP_MAC) || defined(UNIXWARE)
typedef int16 int16_t;
typedef int32 int32_t;
#endif

#if !defined(IRIX6_2)
typedef uint16 uint16_t;
typedef uint32 uint32_t;
#endif

typedef prword_t uintVP_t; /* unsigned that is same size as a void pointer */

#if !defined(BSDI) && !defined(IRIX6_2) && !defined(LINUX)
typedef int64 int64_t;
#else
/*
** On BSDI, for some damn reason, they define long long's for these types
** even though they aren't actually 64 bits wide!
*/
#define int64_t int64
#endif

#if defined(XP_PC) || (defined(__sun) && !defined(SVR4)) || defined(HPUX) || defined(LINUX) || defined(BSDI) || defined(XP_MAC)
typedef unsigned int uint_t;
#endif

/* DLL Entry modifiers... */
#if defined( XP_PC )
#if defined(_WIN32)
#define EXPORTED                	extern __declspec(dllexport)
#define JAVA_PUBLIC_API(ResultType)	_declspec(dllexport) ResultType
#define JAVA_CALLBACK
#else
#define EXPORTED                	extern 
#define JAVA_PUBLIC_API(ResultType)	ResultType _cdecl _export _loadds 
#define JAVA_CALLBACK			_loadds
#endif
#else
#define JAVA_PUBLIC_API(ResultType)	ResultType
#define JAVA_CALLBACK
#endif

/************************************************************************/

#ifdef HAVE_LONG_LONG
#define ll_high(a)              ((long)((a)>>32))
#define ll_low(a)               ((long)(a))
#define int2ll(a)               ((int64_t)(a))
#define uint2ll(a)              ((int64_t)(a))
#define ll2int(a)               ((int)(a))
#define ll_add(a, b)    ((a) + (b))
#define ll_and(a, b)    ((a) & (b))
#define ll_div(a, b)    ((a) / (b))
#define ll_mul(a, b)    ((a) * (b))
#define ll_neg(a)               (-(a))
#define ll_not(a)               (~(a))
#define ll_or(a, b)             ((a) | (b))
#define ll_shl(a, n)    ((a) << (n))
#define ll_shr(a, n)    ((a) >> (n))
#define ll_sub(a, b)    ((a) - (b))
#define ll_ushr(a, n)   ((uint64)(a) >> (n))
#define ll_xor(a, b)    ((a) ^ (b))
#define ll_rem(a,b)             ((a) % (b))

#define float2ll(f)             ((int64_t) (f))
#define ll2float(a)             ((float) (a))
#define ll2double(a)    ((double) (a))
#define double2ll(f)    ((int64_t) (f))

/* comparison operators */
#define ll_ltz(ll)              ((ll)<0)
#define ll_gez(ll)              ((ll)>=0)
#define ll_eqz(a)               ((a) == 0)
#define ll_eq(a, b)             ((a) == (b))
#define ll_ne(a,b)              ((a) != (b))
#define ll_ge(a,b)              ((a) >= (b))
#define ll_le(a,b)              ((a) <= (b))
#define ll_lt(a,b)              ((a) < (b))
#define ll_gt(a,b)              ((a) > (b))

#define ll_zero_const   ((int64_t) 0)
#define ll_one_const    ((int64_t) 1)

#define a2ll(x) atoll(x)

#else

extern int64 ll_zero_const;
extern int64 ll_one_const;

extern int64 int2ll(int32_t i);
extern int64 uint2ll(uint32_t i);
extern int32_t ll2int(int64 ll);
extern int64 float2ll(float f);
extern float ll2float(int64 ll);
extern int64 double2ll(double d);
extern double ll2double(int64 ll);

extern int64 ll_shl(int64 ll, int32_t shift);
extern int64 ll_shr(int64 ll, int32_t shift);
extern int64 ll_ushr(int64 ll, int32_t shift);

extern int64 ll_neg(int64 a);

extern int64 ll_add(int64 a, int64 b);
extern int64 ll_sub(int64 a, int64 b);
extern int64 ll_mul(int64 a, int64 b);
extern int64 ll_div(int64 a, int64 b);
extern int64 ll_rem(int64 a, int64 b);
extern int64 ll_and(int64 a, int64 b);
extern int64 ll_or(int64 a, int64 b);
extern int64 ll_xor(int64 a, int64 b);

#define ll_lt(a,b) LL_CMP(a, <, b)
#define ll_gt(a,b) LL_CMP(a, >, b)
#define ll_eqz(a) LL_IS_ZERO(a)
#define ll_eq(a,b) LL_EQ(a,b)
#define ll_ne(a,b) LL_NE(a,b)

extern int64 a2ll(char *);

#endif

extern void ll2str(int64_t a, char *s, char *limit);

/************************************************************************/

/* XXX The bucket for stuff that seems to have no home... */
/* extern void ErrorUnwind(void); */

#ifdef XP_PC
extern struct tm * gmtime_r( const time_t*, struct tm * );
extern struct tm * localtime_r( const time_t*, struct tm * );

extern void srand48( long seed );
extern double drand48();

#define popen    _popen
#endif  /* XP_PC */

#if defined(XP_PC) || defined(XP_MAC)
extern double rint( double x);
#endif

#ifdef XP_MAC
extern double drem(double dividend, double divisor);

#endif

#endif /* sun_java_typedefs_md_h___ */
