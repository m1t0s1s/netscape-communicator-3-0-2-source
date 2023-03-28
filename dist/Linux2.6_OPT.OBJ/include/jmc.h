/* -*- Mode: C; tab-width: 4; -*- */
/*******************************************************************************
 * Java Module Compiler -- Standard Include File
 * Copyright (c) 1996 Netscape Communications Corporation. All rights reserved.
 ******************************************************************************/

#ifndef JMC_H
#define JMC_H

#include "jritypes.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*******************************************************************************
 * JMCModule
 ******************************************************************************/

typedef struct JMCModule {
    void*	vtable;
    jint	refcount;
} JMCModule;

extern void*
JMCModule_GetInterface(JMCModule* self, jint op, JRIInterfaceID* iid);

extern void
JMCModule_AddRef(JMCModule* self, jint op);

extern void
JMCModule_Release(JMCModule* self, jint op);

/*******************************************************************************
 * JMCJavaObject
 ******************************************************************************/

typedef void	JMCException;

typedef struct JMCJavaObject {
    JMCModule		super;
    jref			obj;
    JRIMethodID*	methodIDs;
	const char**	description;
} JMCJavaObject;

#define JMCJavaObject_GetInterface		JMCModule_GetInterface

#define JMCJavaObject_AddRef			JMCModule_AddRef

#define JMCJavaObject_Release			JMCModule_Release

extern JRI_PUBLIC_API(const char**)
JMCJavaObject_Description(JMCJavaObject* self, jint op);

extern JRI_PUBLIC_API(jref)
JMCJavaObject_CallMethod(JMCJavaObject* self, jint op, ...);

extern JRI_PUBLIC_API(jbool)
JMCJavaObject_CallMethodBoolean(JMCJavaObject* self, jint op, ...);

extern JRI_PUBLIC_API(jbyte)
JMCJavaObject_CallMethodByte(JMCJavaObject* self, jint op, ...);

extern JRI_PUBLIC_API(jchar)
JMCJavaObject_CallMethodChar(JMCJavaObject* self, jint op, ...);

extern JRI_PUBLIC_API(jshort)
JMCJavaObject_CallMethodShort(JMCJavaObject* self, jint op, ...);

extern JRI_PUBLIC_API(jint)
JMCJavaObject_CallMethodInt(JMCJavaObject* self, jint op, ...);

extern JRI_PUBLIC_API(jlong)
JMCJavaObject_CallMethodLong(JMCJavaObject* self, jint op, ...);

extern JRI_PUBLIC_API(jfloat)
JMCJavaObject_CallMethodFloat(JMCJavaObject* self, jint op, ...);

extern JRI_PUBLIC_API(jdouble)
JMCJavaObject_CallMethodDouble(JMCJavaObject* self, jint op, ...);

/******************************************************************************/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* JMC_H */
/******************************************************************************/
