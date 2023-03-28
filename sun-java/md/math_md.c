/*
 * @(#)math_md.c	1.6 95/08/30
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
#include "typecodes.h"

#include "jmath.h"

#ifdef SOLARIS
double drem(double dividend, double divisor) {
    static double nan = 0.0;	/* compiler won't let me use 0.0/0.0 */
    if (finite(dividend) && (divisor != 0.0)) {
	if (finite(divisor))
	    return remainder(dividend, divisor);
	if (!isnan(divisor))
	    return dividend;
    } 
    /* need to return nan */
    if (nan == 0.0)		/* generate nan if first time */
	nan = nan/nan;
    return nan;
}

int 
matherr(struct exception *exc) 
{ 
    /* By returning a non-zero value, we prevent an error message from
     * being printed out, and errno from being set.
     */
    return 1;
}

double math_infinity;
double math_minusinfinity;
double math_minuszero;
double math_nan;

void math_init() { 
    math_infinity = HUGE_VAL;
    math_minusinfinity = -HUGE_VAL;
    math_minuszero = copysign(0.0, -1.0);
    math_nan = math_minuszero / math_minuszero;
}

#endif

#ifdef XP_PC
#include <float.h>              /* _finite(...) */
double drem(double dividend, double divisor) {
#if defined(_WIN32)
    static double nan = 0.0;    /* compiler won't let me use 0.0/0.0 */
    if (_finite(dividend) && (divisor != 0.0)) {
        if (_finite(divisor))
            return fmod(dividend, divisor);
        /* XXX fix me! */
        else
            return dividend;
    } 
    /* need to return nan */
    if (nan == 0.0)             /* generate nan if first time */
        nan = nan/nan;
    return nan;
#else
	OutputDebugString("gcvt is not implemented");
	return 0.0;
#endif
}
#endif /* XP_PC */


#ifdef XP_MAC
#include <Types.h>
char _finite(double finite)
{
    static double nan = 0.0/0.0;    /* compiler won't let me use 0.0/0.0 */
	if (finite == nan)
		return 0;
	else
		return 1;
}
double drem(double dividend, double divisor) {
    static double nan = 0.0/0.0;    /* compiler won't let me use 0.0/0.0 */
    if (_finite(dividend) && (divisor != 0.0)) {
        if (_finite(divisor))
            return fmod(dividend, divisor);
        /* XXX fix me! */
        else
            return dividend;
    } 
    return nan;
}
#endif
