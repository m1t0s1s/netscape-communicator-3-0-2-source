/*
 * @(#)string.c	1.24 95/11/29  
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
#include "path_md.h"
#include "sys_api.h"

JRI_PUBLIC_API(Hjava_lang_String *)
makeJavaString(char *str, int len) {
    HArrayOfChar *val;
    unicode *p;

    if (!str) len = 0;
    val = (HArrayOfChar *) ArrayAlloc(T_CHAR, len);
    if (val == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    }
    p = unhand(val)->body;
    if (str) str2unicode(str, p, len);
    return (Hjava_lang_String *)
      execute_java_constructor(EE(), 0, classJavaLangString, "([C)", val);
}

void javaStringPrint(Hjava_lang_String *s) {
    if (s) {
	Classjava_lang_String *str = unhand(s);
	HArrayOfChar *hac = (HArrayOfChar *)str->value;
	unicode *body = unhand(hac)->body;
	printus(body + str->offset, str->count);
    }
}

JRI_PUBLIC_API(int32_t)
javaStringLength(Hjava_lang_String *s) {
    Classjava_lang_String *str;
    if (!s) {
	return 0;
    }
    str = unhand(s);
    return str->count;
}

unicode *javaString2unicode(Hjava_lang_String *s, unicode *buf, int len) {
    if (s) {
	Classjava_lang_String *str = unhand(s);
	HArrayOfChar *hac = (HArrayOfChar *)str->value;
	memcpy(buf, unhand(hac)->body+str->offset, len*sizeof(unicode));
    } else {
	memset(buf, 0, len * sizeof(unicode));
    }
    return buf;
}

JRI_PUBLIC_API(char *)
javaString2CString(Hjava_lang_String *s, char *buf, int buflen) {
    Classjava_lang_String *str;
    HArrayOfChar *hac;

    if (s && (str = unhand(s)) && (hac = (HArrayOfChar *)str->value)) {
	unicode *src = unhand(hac)->body;
	int32_t	len = javaStringLength(s);

	if (len >= buflen)
	    len = buflen - 1;
	unicode2str(src + str->offset, buf, len);
    } else if (buflen) {
	buf[0] = '\0';
    }
    return buf;
}

char *makeCString(Hjava_lang_String *s) {
    extern HArrayOfByte *MakeByteString(char *str, long len);
    int32_t len = javaStringLength(s);
    char *buf;

    HArrayOfByte *hab = MakeByteString(NULL, len + 1);
    if (hab == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    }

    buf = unhand(hab)->body;
    return javaString2CString(s, buf, (size_t)len + 1);
}

char *allocCString(Hjava_lang_String *s) {
    int32_t len = javaStringLength(s);
    char *buf = sysMalloc((size_t)len + 1);
    if (buf == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    }
    return javaString2CString(s, buf, (size_t)len + 1);
}
