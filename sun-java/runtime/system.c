/*
 * @(#)system.c	1.42 95/11/15  
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

/*
 * Modification History:
 *
 * 22 Aug 1994	eugene
 *
 *	inserted a "machine independent" porting layer for machine
 *	specific non-posix lib and system calls.
 *	removed the open boot prom reading and writing code.  should
 *	use getenv and putenv for environment variable moshing.  no
 *	other reason to keep prom code around (we run hosted now!!).
 *
 */

#include <stdlib.h>
#include <fcntl.h>
#include "standardlib.h"

#include "oobj.h"
#include "tree.h"
#include "interpreter.h"
#include "javaString.h"
#include "typecodes.h"
#include "sys_api.h"

#include "java_lang_System.h"
#include "java_io_FileInputStream.h"


int64_t
java_lang_System_currentTimeMillis(Hjava_lang_System *this)
{
    return sysTimeMillis();
}

void
java_lang_System_arraycopy(Hjava_lang_System *this, 
			   HObject *srch, long src_pos,
			   HObject *dsth, long dst_pos, long length) 
{
    OBJECT *srcptr, *dstptr;

    if ((srch == 0) || (dsth == 0)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    } else if ((obj_flags(srch) == T_NORMAL_OBJECT) || 
	       (obj_flags(dsth) == T_NORMAL_OBJECT) || 
	       (obj_flags(srch) != obj_flags(dsth))) {
	SignalError(0, JAVAPKG "ArrayStoreException", 0);
	return;
    } else if ((length < 0) || (src_pos < 0) || (dst_pos < 0) ||
	(length + src_pos > (long) obj_length(srch)) ||
	(length + dst_pos > (long) obj_length(dsth))) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return;
    } 
    srcptr = unhand(srch);
    dstptr = unhand(dsth);
    switch(obj_flags(srch)) { 
         case T_BOOLEAN: case T_BYTE: 
	    memmove(&((ArrayOfByte *)dstptr)->body[dst_pos], 
		    &((ArrayOfByte *)srcptr)->body[src_pos],
		    (size_t)(length * sizeof(((ArrayOfByte *)dstptr)->body[0])));
	    break;

	case T_SHORT:
	    memmove(&((ArrayOfShort *)dstptr)->body[dst_pos], 
		    &((ArrayOfShort *)srcptr)->body[src_pos],
		    (size_t)(length * sizeof(((ArrayOfShort *)dstptr)->body[0])));
	    break;

	case T_CHAR:
	    memmove(&((ArrayOfChar *)dstptr)->body[dst_pos], 
		    &((ArrayOfChar *)srcptr)->body[src_pos],
		    (size_t)(length * sizeof(((ArrayOfChar *)dstptr)->body[0])));
	    break;

	case T_INT:
	    memmove(&((ArrayOfInt *)dstptr)->body[dst_pos], 
		    &((ArrayOfInt *)srcptr)->body[src_pos],
		    (size_t)(length * sizeof(((ArrayOfInt *)dstptr)->body[0])));
	    break;

	case T_LONG:
	    memmove(&((ArrayOfLong *)dstptr)->body[dst_pos], 
		    &((ArrayOfLong *)srcptr)->body[src_pos],
		    (size_t)(length * sizeof(((ArrayOfLong *)dstptr)->body[0])));
	    break;

	case T_FLOAT: 
	    memmove(&((ArrayOfFloat *)dstptr)->body[dst_pos], 
		    &((ArrayOfFloat *)srcptr)->body[src_pos],
		    (size_t)(length * sizeof(((ArrayOfFloat *)dstptr)->body[0])));
	    break;

	case T_DOUBLE: 
	    memmove(&((ArrayOfDouble *)dstptr)->body[dst_pos], 
		    &((ArrayOfDouble *)srcptr)->body[src_pos],
		    (size_t)(length * sizeof(((ArrayOfDouble *)dstptr)->body[0])));
	    break;

	case T_CLASS: {
	    uint32_t srclen = obj_length(srch);
	    uint32_t dstlen = obj_length(dsth);
	    ArrayOfObject *osrcptr = (ArrayOfObject *)srcptr;
	    ArrayOfObject *odstptr = (ArrayOfObject *)dstptr;
	    ClassClass *src_class = (ClassClass *)osrcptr->body[srclen];
	    ClassClass *dst_class = (ClassClass *)odstptr->body[dstlen];
	    uint32_t i;
	    if ((src_class != dst_class) && (dst_class != classJavaLangObject)){
		/* if the array types are not identical we need to check each 
		 * non-null element. Note that you can't do thecheck first and
		 * then use memmove later. This would create an opportunity
		 * for another thread to modify the array. */
		for (i = 0 ; i < (uint32_t) length ; i++) {
		    HObject *elem = osrcptr->body[src_pos + i];
		    if (elem && !is_instance_of(elem, dst_class, 0)) {
			SignalError(0, JAVAPKG "ArrayStoreException", 0);
			return;
		    }
		    odstptr->body[dst_pos + i] = elem;
		}
	    } else {
		memmove(&odstptr->body[dst_pos], &osrcptr->body[src_pos],
			(size_t)(length * sizeof(odstptr->body[0])));
	    }
	} 
    }
}


void 
java_lang_System_setSystemIO(struct Hjava_lang_System    *self,
                             struct Hjava_io_InputStream *in,
                             struct Hjava_io_PrintStream *out,
                             struct Hjava_io_PrintStream *err) 
{
    struct ClassClass *cb = FindClass(0, JAVAPKG "System", TRUE);
    long * temp;

    sysAssert(cb);
    if (!cb) return;

    temp = getclassvariable(cb, "in");
    sysAssert(temp);
    if (temp)
      (*(struct Hjava_io_InputStream **)temp) = in;

    temp = getclassvariable(cb, "out");
    sysAssert(temp);
    if (temp)
      (*(struct Hjava_io_PrintStream **)temp) = out;

    temp = getclassvariable(cb, "err");
    sysAssert(temp);
    if (temp)
      (*(struct Hjava_io_PrintStream **)temp) = err;
}
