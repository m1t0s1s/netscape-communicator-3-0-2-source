/*
 * @(#)class.c	1.44 95/11/15  
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
 *      Implementation of class Class
 *
 *      former threadruntime.c, Sun Sep 22 12:09:39 1991
 */

#include <stdio.h>
#include <signal.h>

#include "tree.h"
#include "javaString.h"

#include "monitor.h"
#include "javaThreads.h"
#include "exceptions.h"

#include "java_lang_Class.h"
#include "java_lang_ClassLoader.h"

extern void lock_classes(void), unlock_classes(void);

HClass *
java_lang_Class_forName(HClass *this, HString *classname)
{
    char clname[256], *p;
    ClassClass *cb;
    HClass *h;
    int c;

    javaString2CString(classname, clname, sizeof(clname));
    for (p = clname ; (c = *p); p++) {
	if (c == '.') {
	    *p = '/';
	}
    }
    if ((cb = FindClass(EE(), clname, TRUE)) == 0) {
	SignalError(0, JAVAPKG "ClassNotFoundException", clname);
	return 0;
    }
    h = cbHandle(cb);
    return h;
}

Hjava_lang_String *
java_lang_Class_getName(HClass *this)
{
    ClassClass *cb;
    char clname[256];

    if ((this == NULL) || ((cb = unhand(this)) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    classname2string(classname(cb), clname, sizeof(clname));
    return makeJavaString(clname, strlen(clname));
}

HClass *
java_lang_Class_getSuperclass(HClass *this)
{
    ClassClass *cb;
    if ((this == NULL) || ((cb = unhand(this)) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    return cbSuperclass(cb);
}

HArrayOfObject *
java_lang_Class_getInterfaces(HClass *this)
{
    int i;
    ClassClass *cb;
    HArrayOfObject *list;
    ExecEnv *ee = EE();
    HClass **listdata;

    if ((this == NULL) || ((cb = unhand(this)) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }

    list = (HArrayOfObject *)ArrayAlloc(T_CLASS, cb->implements_count);
    if (list == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    }
    listdata = (HClass **)unhand(list)->body;
    listdata[cb->implements_count] = (void *)classJavaLangClass;

    /* See if any of the interfaces that we implement possibly implement icb */
    for (i = 0 ; i < (int)cb->implements_count ; i++) {
	ResolveClassConstant(cb->constantpool, cb->implements[i], ee, 1 << CONSTANT_Class);

	listdata[i] = cbHandle((ClassClass *)(cb->constantpool[cb->implements[i]].p));

    }

    return list;
}

Hjava_lang_ClassLoader *
java_lang_Class_getClassLoader(HClass *this)
{
    ClassClass *cb;
    if ((this == NULL) || ((cb = unhand(this)) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    return (Hjava_lang_ClassLoader *)cb->loader;
}

HObject *
java_lang_Class_newInstance(HClass *this)
{
    ExecEnv *ee = EE();
    ClassClass *cb;

    if ((this == NULL) || ((cb = unhand(this)) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    return execute_java_constructor(ee, NULL, cb, "()");
}

long 
java_lang_Class_isInterface(HClass *this) 
{
    ClassClass *cb;
    if ((this == NULL) || ((cb = unhand(this)) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    return (cb->access & ACC_INTERFACE) ? 1 : 0;
}

/**
 * Class loader
 */

extern ClassClass *(*class_loader_func)(HObject *loader, char *name, long resolve);

static ClassClass *
class_loader(HObject *loader, char *name, long resolve) 
{
    char buf[256], *p;
    HObject *c;

    jio_snprintf(buf, sizeof(buf), "%s", name); /* XXX this should be a *MUCH* more efficient strncpy() */

    for (p = buf ; *p ; p++) {
	if (*p == '/') {
	    *p = '.';
	}
    }
    
    /* XXX: Should we throw an exception when we see any illegal characters here??
     This note was identified by David Hopwood.*/

    c = (HObject *)execute_java_dynamic_method(0, loader, "loadClass", 
					      "(L" JAVAPKG "String;Z)L" JAVAPKG "Class;", 
					      makeJavaString(buf, strlen(buf)), resolve);
    return c ? (ClassClass *)unhand(c) : NULL;
}

HClass *
java_lang_ClassLoader_defineClass(Hjava_lang_ClassLoader *this, Hjava_lang_String * name, HArrayOfByte *data, long offset, long length) 
{
    ClassClass	    *cb;
    unsigned char   *body;
    int32_t	    body_len;
    char	    *detail;
    char	    *ename;
    HClass	    *h;
    char	    *namep;
    int		    ret_val;


    /* XXX: Get a boolean return from the following, and it will not be necessary to create an EE() */
    java_lang_ClassLoader_checkInitialized(this);
    if (exceptionOccurred(EE()))
      return 0;

    if (data == 0) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return 0;
    }
    cb = AllocClass();
    if (cb == 0) {
	SignalError(0, JAVAPKG "OutOfMemoryError", 0);
	return 0;
    }
    cb->loader = (HObject *)this;
    body = (unsigned char *) unhand(data)->body;
    body_len = (int32_t) obj_length(data);
    if (offset < 0 || offset >= body_len || length < 0
		   || offset + length > body_len) {
	SignalError(0, JAVAPKG "ArrayIndexOutOfBoundsException", 0);
	return 0;
    }

    sysAssert(!cb->name);
    if (name) {                        /* Expected class name is optional */
        char *p;
        namep = allocCString(name);
        if (!namep)                    /* out of memory was already signaled */
            return 0;
        for (p = namep; *p; p++)       /* Translate from '.' to '/' separators */
            if ('.' == *p) 
                *p = '/';  
        cb->name = namep;              /* Provide expected name */
    } else {
        cb->name = namep = NULL;       /* Indicate no expectations */
    }

    /* We need to lock the classes while creating the class and adding
     * the class to the list of classes
    */
    lock_classes();
    ret_val = createInternalClass(body + offset, body + offset + length, cb);
    if (namep) {
        sysFree(namep);
        namep = 0;
    }
    if (!ret_val) {
	/*	If there was no exception already (OutOfMemory), then 
		throw a class format error. */
	if (!exceptionOccurred(EE()))
		SignalError(0, JAVAPKG "ClassFormatError", 0);
	unlock_classes();
	return 0;
    }
    AddBinClass(cb);

    /*
    ** AddBinClasses() sets the CCF_IsResolving bit to prevent the class
    ** from being freed during class initialization...
    ** Clear this bit since cb references the class and protects it from
    ** being GCed...
    */
    sysAssert(CCIs(cb, Resolving));
    CCClear(cb, Resolving);

    unlock_classes();

    /*
    ** InitializeClass will lock the class and initialize it if
    ** necessary. Note that we need to hold the CCF_Locked across the
    ** call to prevent the class unloader from unloading this currently
    ** unreferenced class!
    */
    if ((ename = InitializeClass(cb, &detail)) != 0) {
	SignalError(0, ename, detail);
	return 0;
    }
    h = cb->HandleToSelf;		/* now there is a reference from C */
    return h;
}

void
java_lang_ClassLoader_resolveClass(Hjava_lang_ClassLoader *this, HClass *c)
{
    ClassClass  *cb;
    char	*detail;
    char	*ename;

    java_lang_ClassLoader_checkInitialized(this);
    if (exceptionOccurred(EE()))
      return;

    if ((this == NULL) || ((cb = unhand(c)) == NULL)) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return;
    }

    /* ResolveClass will lock the class and resolve it if necessary */
    if ((ename = ResolveClass(cb, &detail)) != 0) {
	SignalError(0, ename, detail);
    }
}

HClass *
java_lang_ClassLoader_findSystemClass(Hjava_lang_ClassLoader *this, HString *classname)
{
    char clname[256], *p;
    ClassClass *cb;
    HClass *h;
    int c;

    java_lang_ClassLoader_checkInitialized(this);
    if (exceptionOccurred(EE()))
      return 0;

    javaString2CString(classname, clname, sizeof(clname));
    for (p = clname ; (c = *p); p++) {
	if (c == '.') {
	    *p = '/';
	}
    }
    if ((cb = FindClassFromClass(0, clname, TRUE, 0)) == 0) {
	SignalError(0, JAVAPKG "ClassNotFoundException", clname);
	return 0;
    }
    h = cbHandle(cb);
    return h;
}

void
java_lang_ClassLoader_init(Hjava_lang_ClassLoader *this)
{
    /* make sure the runtime can do upcalls */
    class_loader_func = class_loader;
}

void 
java_lang_ClassLoader_checkInitialized(Hjava_lang_ClassLoader *this) {
    if (this == NULL) {
        SignalError(0, JAVAPKG "NullPointerException", 0);
        return;
    }

    if (!unhand(this)->initialized) {
        SignalError(0, JAVAPKG "SecurityException", "Attempted use of uninitialized ClassLoader");
        return;
    }
}
