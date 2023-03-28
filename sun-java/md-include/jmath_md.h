#ifndef sun_java_jmath_md_h___
#define sun_java_jmath_md_h___

#ifdef SOLARIS
#include <ieeefp.h>
#endif

/*
** Those namespace terrorists up in Redmond are holding "exception" hostage
** Free it now !!!
*/
#if defined(exception)
#undef exception
#endif

#define DREM(a,b) fmod(a,b)

#if defined(SOLARIS) || defined(SCO) || defined(UNIXWARE)
#define IEEEREM(a,b) remainder(a,b)
#else
#define IEEEREM(a,b) drem(a, b)
#endif

#endif /* sun_java_jmath_md_h___ */
