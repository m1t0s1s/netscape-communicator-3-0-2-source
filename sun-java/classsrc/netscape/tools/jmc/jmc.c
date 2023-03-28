/* -*- Mode: C; tab-width: 4; -*- */
/*******************************************************************************
 * Java Module Compiler
 * Copyright (c) 1996 Netscape Communications Corporation. All rights reserved.
 ******************************************************************************/

#include "jmc.h"
#include "jri.h"
#include <stdarg.h>

/*******************************************************************************
 * Standard Methods
 ******************************************************************************/

void*
JMCModule_GetInterface(JMCModule* self, jint op, JRIInterfaceID* iid)
{
    return NULL;
}

void
JMCModule_AddRef(JMCModule* self, jint op)
{
    self->refcount++;
}

void
JMCModule_Release(JMCModule* self, jint op)
{
    self->refcount--;
    if (self->refcount == 0) {
	/* free(self); */	/* XXX */
    }
}

/*******************************************************************************
 * Methods for calling into the interpreter
 ******************************************************************************/

JRI_PUBLIC_API(const char**)
JMCJavaObject_Description(JMCJavaObject* self, jint op)
{
    return self->description;
}

JRI_PUBLIC_API(jref)
JMCJavaObject_CallMethod(JMCJavaObject* self, jint op, ...)
{
    jref result;
    JRIEnv* env = JRI_GetCurrentEnv();
    va_list args;
    va_start(args, op);
    result = JRI_CallMethodV(env, self->obj, self->methodIDs[op], args);
    va_end(args);
    return result;
}

#define DEFINE_CALLMETHOD(ResultType, Suffix)			    \
JRI_PUBLIC_API(ResultType)					    \
JMCJavaObject_CallMethod##Suffix(JMCJavaObject* self, jint op, ...) \
{								    \
    ResultType result;						    \
    JRIEnv* env = JRI_GetCurrentEnv();				    \
    va_list args;						    \
    va_start(args, op);						    \
    result = JRI_CallMethod##Suffix##V(env, self->obj,		    \
				       self->methodIDs[op], args);  \
    va_end(args);						    \
    return result;						    \
}								    \

DEFINE_CALLMETHOD(jbool,	Boolean)
DEFINE_CALLMETHOD(jbyte,	Byte)
DEFINE_CALLMETHOD(jchar,	Char)
DEFINE_CALLMETHOD(jshort,	Short)
DEFINE_CALLMETHOD(jint,		Int)
DEFINE_CALLMETHOD(jlong,	Long)
DEFINE_CALLMETHOD(jfloat,	Float)
DEFINE_CALLMETHOD(jdouble,	Double)

/******************************************************************************/
