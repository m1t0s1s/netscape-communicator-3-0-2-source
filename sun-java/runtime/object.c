/*
 * @(#)object.c	1.42 95/12/01
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

/*-
 *      Implementation of class Object
 *
 *      former threadruntime.c, Sun Sep 22 12:09:39 1991
 */

#ifdef __cplusplus
#error
#endif

#include <stdio.h>
#include <signal.h>

#include "tree.h"
#include "decode.h"
#include "javaString.h"
#include "signature.h"
#include "typecodes.h"

#include "monitor.h"
#include "javaThreads.h"
#include "exceptions.h"
#include "path.h"

#ifdef XP_MAC
#include "macjavalib.h"
#endif



#ifdef XP_MAC
#include "jdk_java_lang_Object.h"
#else
#include "java_lang_Object.h"
#endif

long
java_lang_Object_hashCode(JHandle *this)
{
    return (long)this;
}

void
java_lang_Object_wait(JHandle* p, int64_t millis)
{
    int32_t ms = ll2int(millis);
    if (p == 0)
	return;
    /* REMIND: monitorWait should really take a long */
    if (ms) {
	monitorWait(obj_monitor(p), ms);
    } else {
	monitorWait(obj_monitor(p), TIMEOUT_INFINITY);
    }
    return;
}

void
java_lang_Object_notify(JHandle* p)
{
    if (p == 0)
	return;
    monitorNotify(obj_monitor(p));
}

void
java_lang_Object_notifyAll(JHandle*p)
{
    if (p == 0)
	return;
    monitorNotifyAll(obj_monitor(p));
}

HObject *java_lang_Object_clone(HObject *this)
{
    if (obj_flags(this) == T_NORMAL_OBJECT) {
	ClassClass *cb = obj_classblock(this);
	HObject *clone;
	if (!is_instance_of(this, interfaceJavaLangCloneable, EE())) { 
	    char clname[256];
	    classname2string(classname(cb), clname, sizeof(clname));
	    SignalError(0, JAVAPKG "CloneNotSupportedException", clname);
	    return NULL;
	}
	clone = ObjAlloc(cb, 0);
	if (clone == 0) {
	    char buf[128];
	    SignalError(0, JAVAPKG "OutOfMemoryError", 
			classname2string(classname(cb), buf, sizeof(buf)));
	    return NULL;
	} 
	memcpy((char*)unhand(clone), (char*)unhand(this), cbInstanceSize(cb));
	return clone;
    } else {
	uint32_t type = obj_flags(this);
	uint32_t size = obj_length(this);
	int32_t bytes = 
	       sizearray(type, size) + (type == T_CLASS ? sizeof(OBJECT) : 0);
	HObject *clone = ArrayAlloc(type, size);
	if (clone == 0) {
	    SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	    return NULL;
	}
	memcpy((char*)unhand(clone), (char*)unhand(this), (size_t) bytes);
	return clone;
    }
}


static char *
class2string(ClassClass * cb, char *buf, char *limit)
{
    if (cb == 0)
	buf = addstr("NULL-classblock", buf, limit, 0);
    else {
	if (cbSuperclass(cb) 
	          && unhand(cbSuperclass(cb)) != classJavaLangObject) {
	    buf = class2string(unhand(cbSuperclass(cb)), buf, limit);
	    buf = addstr(".", buf, limit, 0);
	}
	buf = addstr(classname(cb), buf, limit, 0);
    }
    return buf;
}

HClass *
java_lang_Object_getClass(HObject *p)
{
    if (p == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    } else if (obj_flags(p) == T_NORMAL_OBJECT) {
	return cbHandle(obj_classblock(p));
    } else {
	char buffer_space[256];	/* usually, more than enough space */
	char *buffer = buffer_space; /* buffer != buffer_space means malloc'ed*/
	char *classname = NULL;
	ClassClass *result, *fromClass = NULL;
	switch (obj_flags(p)) {
	    case T_INT:      
		classname = SIGNATURE_ARRAY_STRING SIGNATURE_INT_STRING; 
		break;
	    case T_LONG:     
		classname = SIGNATURE_ARRAY_STRING SIGNATURE_LONG_STRING; 
		break;
	    case T_FLOAT:  
		classname = SIGNATURE_ARRAY_STRING SIGNATURE_FLOAT_STRING;
		break;
	    case T_DOUBLE:  
		classname = SIGNATURE_ARRAY_STRING SIGNATURE_DOUBLE_STRING;
		break;
	    case T_BOOLEAN: 
		classname = SIGNATURE_ARRAY_STRING SIGNATURE_BOOLEAN_STRING;
		break;
	    case T_BYTE:    
		classname = SIGNATURE_ARRAY_STRING SIGNATURE_BYTE_STRING;
		break;
	    case T_CHAR:    
		classname = SIGNATURE_ARRAY_STRING SIGNATURE_CHAR_STRING;
		break;
	    case T_SHORT:   
		classname = SIGNATURE_ARRAY_STRING SIGNATURE_SHORT_STRING;
		break;
	    case T_CLASS: {
		ArrayOfObject *array = (ArrayOfObject *)unhand(p);
		char *fromName;
		int fromNameLength;

		fromClass = (ClassClass *)(array->body[obj_length(p)]);
		fromName = fromClass->name;
		fromNameLength = strlen(fromName);
		/* Check if there is enough space in buffer.  Otherwise,
		 * make it bigger by allocating more space.
		 */
		if (fromNameLength + 5 >= sizeof(buffer_space)) {
		    buffer = sysMalloc(fromNameLength + 5);
		}
		classname = buffer;
		buffer[0] = SIGNATURE_ARRAY;
		if (fromClass->name[0] == SIGNATURE_ARRAY) {
		    /* See above. This is safe. */
		    strcpy(buffer + 1, fromName);
		} else { 
		    /* See above. This is safe. We know buffer is big enough */
		    buffer[1] = SIGNATURE_CLASS;
		    strncpy(buffer + 2, fromName, fromNameLength);
		    buffer[2 + fromNameLength]  = SIGNATURE_ENDCLASS;
		    buffer[3 + fromNameLength]  = '\0';
		}
	    }
	}
	result = FindClassFromClass(0, classname, FALSE, fromClass);
	if (buffer != buffer_space) 
	    sysFree(buffer);
	if (result) {
	    HClass *h = cbHandle(result);
	    return h;
	}
	return 0;
    }
}


