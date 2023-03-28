/*
 * @(#)math.c	1.28 95/11/29  
 *
 * Copyright (c) 1994 Sun Microsystems, Inc. All Rights Reserved.
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for NON-COMMERCIAL purposes and without
 * fee is hereby granted provided that this copyright notice
 * appears in all copies. Please refer to the file "copyright.html"
 * for further important copyright and licensing information.
 *
 * SUN MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE SUITABILITY OF
 * THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE, OR NON-INFRINGEMENT. SUN SHALL NOT BE LIABLE FOR
 * ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING, MODIFYING OR
 * DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

#include "oobj.h"
#include "interpreter.h"
#include "javaString.h"
#include "typecodes.h"

#ifdef XP_MAC
#include "typedefs_md.h"
#endif

#include "java_lang_Math.h"
#include "java_lang_Number.h"
#include "java_lang_Integer.h"
#include "java_lang_Long.h"
#include "java_lang_Float.h"
#include "java_lang_Double.h"

#include "jmath.h"
#include "path.h"

#if defined(XP_PC) && !defined(_WIN32)
//
// On win16, %g produces a leading zero in the exponent
// resulting in three exponent digits. This is different
// from our reference platform.
//
static void FixW16GFmt(char *buf)
{
	char *e;
	char *r;
	char *lead;

	e=buf;
	while ( *e != '\0' && *e != 'e' )
		e += 1;
	if (*e == '\0')
		return; // No exponent

	e += 2; /* Skip over e[+/-], moving to the first digit of the exponent */
	if (e[0] != '0')
		return; // No leading zero
	
	if (e[1] == '\0' || e[2] == '\0')
		return; // Only two digits this is ok.

	r = e;
	e += 1;
	while ( *e != '\0' )
		*r++ = *e++;
	*r++ = '\0';

}
#endif

/*
** I have put this call here because asin/atan and other math library
** functions call this function whenever they generate an error. On
** SunOS4 putting it in sun-java/md/math_md.c as is done Solaris lead
** to linker problems. GRRR!!!
** -rrj
*/
#ifdef SUNOS4
int 
matherr(struct exception *exc) 
{ 
    /* By returning a non-zero value, we prevent an error message from
     * being printed out, and errno from being set.
     */
    return 1;
}
#endif

Hjava_lang_String *
java_lang_Float_toString(struct Hjava_lang_Float *s, float f)
{
    char buf[64];
    sprintf(buf,"%g", f);
#if defined(XP_PC) && !defined(_WIN32)
	FixW16GFmt(buf);
#endif
    return makeJavaString(buf, strlen(buf));
}

Hjava_lang_String *
java_lang_Double_toString(struct Hjava_lang_Double *s, double f) 
{
    char buf[64];
#if defined(XP_PC) && !defined(_WIN32)
    sprintf(buf,"%lg", (double) f);
    FixW16GFmt(buf);
#else
    sprintf(buf,"%g", f);
#endif
    return makeJavaString(buf, strlen(buf));
}


Hjava_lang_Float *
java_lang_Float_valueOf(Hjava_lang_Float *f, struct Hjava_lang_String *s) 
{
    char buf[64], *p;
    float result;

    if (s == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    javaString2CString(s, buf, sizeof(buf));
    result = (float)strtod(buf, &p);

    if (p == buf) {
	SignalError(0, JAVAPKG "NumberFormatException", buf);
	return 0;
    }

    return (Hjava_lang_Float *)
         execute_java_constructor(0, JAVAPKG "Float", 0, "(F)", result);
}

Hjava_lang_Double *
java_lang_Double_valueOf(Hjava_lang_Double *f, struct Hjava_lang_String *s) 
{
    char buf[64], *p;
    double result;

    if (s == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    javaString2CString(s, buf, sizeof(buf));
    result = strtod(buf, &p);

    if (p == buf) {
	SignalError(0, JAVAPKG "NumberFormatException", buf);
	return 0;
    }

    return (Hjava_lang_Double *)
        execute_java_constructor(0, JAVAPKG "Double", 0, "(D)", result);
}


/* Find the float corresponding to a given bit pattern */
float
java_lang_Float_intBitsToFloat(Hjava_lang_Float *this, long v)
{
    union {
	int32_t i;
	float f;
    } u;
    u.i = v;
    return u.f;
}

/* Find the bit pattern corresponding to a given float */

long
java_lang_Float_floatToIntBits(Hjava_lang_Float *this, float v)
{
    union {
	int32_t i;
	float f;
    } u;
    u.f = v;
    return u.i;
}

/* Find the double float corresponding to a given bit pattern */
double
java_lang_Double_longBitsToDouble(Hjava_lang_Double *this, int64_t v)
{
    union {
	int64_t l;
	double d;
    } u;
    u.l = v;
    return u.d;
}


/* Find the bit pattern corresponding to a given double float */
int64_t
java_lang_Double_doubleToLongBits(Hjava_lang_Double *this, double v)
{
    union {
	int64_t l;
	double d;
    } u;
    u.d = v;
    return u.l;
}


double 
java_lang_Math_cos(Hjava_lang_Math *fh, double f) 
{
    return cos(f);
}

double 
java_lang_Math_sin(Hjava_lang_Math *fh, double f) 
{
    return sin(f);
}

double 
java_lang_Math_tan(Hjava_lang_Math *fh, double f) 
{
    return tan(f);
}

double 
java_lang_Math_asin(Hjava_lang_Math *fh, double f) 
{
    return asin(f);
}

double 
java_lang_Math_acos(Hjava_lang_Math *fh, double f) 
{
    return acos(f);
}

double 
java_lang_Math_atan(Hjava_lang_Math *fh, double f) 
{
    return atan(f);
}

double 
java_lang_Math_exp(Hjava_lang_Math *fh, double f) 
{
    return exp(f);
}

double 
java_lang_Math_log(Hjava_lang_Math *fh, double f) 
{
    return log(f);
}

double 
java_lang_Math_sqrt(Hjava_lang_Math *fh, double f) 
{
    return sqrt(f);
}

double 
java_lang_Math_ceil(Hjava_lang_Math *fh, double f) 
{
    return ceil(f);
}

double 
java_lang_Math_floor(Hjava_lang_Math *fh, double f) 
{
    return floor(f);
}

double 
java_lang_Math_rint(Hjava_lang_Math *fh, double f) 
{
#ifdef XP_MAC
#if GENERATING68K
	if (f < 0)
		return ceil(f - 0.5);
	else
		return floor(f + 0.5);
#else
    return rint(f);
#endif
#else
    return rint(f);
#endif
}

double 
java_lang_Math_atan2(Hjava_lang_Math *f, double f1, double f2) 
{
    return atan2(f1,f2);
}

double 
java_lang_Math_pow(Hjava_lang_Math *f, double f1, double f2) 
{
    return pow(f1,f2);
}

double
java_lang_Math_IEEEremainder(Hjava_lang_Math *f, double f1, double f2)
{
    return IEEEREM(f1, f2);
}
