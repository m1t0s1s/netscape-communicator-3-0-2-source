/* -*- Mode: C; tab-width: 4; -*- */
/*******************************************************************************
 * JRI Auto-Initialization glue for javah
 * Copyright (c) 1996 Netscape Communications Corporation. All rights reserved.
 ******************************************************************************/

/*
** This file defines stub routines that allow JRI field and method thunks
** to be automatically initialized. These routines are deposited as
** initial values in field and method thunks by javah and are resolved by
** the linker (the code for these routines is linked into the client
** program).  These stubs simply redirect the request to the JRIEnv
** parameter which is passed to them.
**
** Note that we can't just grab the procedure pointer out of the JRI
** interface directly and use it to initialize these structs because we
** don't have a JRIEnv until runtime. We need to put something in the
** struct at compile time so that the request gets redirected through the
** JRI automatically, without the user having to explicitly initialize
** each field or method thunk they want to use.
*/

#ifndef JRIAUTO_H
#define JRIAUTO_H

#include "jri.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

JRI_PUBLIC_API(jref)
JRI_GetFieldByName(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jbool)
JRI_GetFieldByName_boolean(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jbyte)
JRI_GetFieldByName_byte(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jchar)
JRI_GetFieldByName_char(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jshort)
JRI_GetFieldByName_short(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jint)
JRI_GetFieldByName_int(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jlong)
JRI_GetFieldByName_long(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jfloat)
JRI_GetFieldByName_float(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jdouble)
JRI_GetFieldByName_double(JRIEnv* env, jref obj, JRIFieldThunk* field);

/******************************************************************************/

JRI_PUBLIC_API(void)
JRI_SetFieldByName(JRIEnv* env, jref obj, JRIFieldThunk* field, jref value);

JRI_PUBLIC_API(void)
JRI_SetFieldByName_boolean(JRIEnv* env, jref obj, JRIFieldThunk* field, jbool value);

JRI_PUBLIC_API(void)
JRI_SetFieldByName_byte(JRIEnv* env, jref obj, JRIFieldThunk* field, jbyte value);

JRI_PUBLIC_API(void)
JRI_SetFieldByName_char(JRIEnv* env, jref obj, JRIFieldThunk* field, jchar value);

JRI_PUBLIC_API(void)
JRI_SetFieldByName_short(JRIEnv* env, jref obj, JRIFieldThunk* field, jshort value);

JRI_PUBLIC_API(void)
JRI_SetFieldByName_int(JRIEnv* env, jref obj, JRIFieldThunk* field, jint value);

JRI_PUBLIC_API(void)
JRI_SetFieldByName_long(JRIEnv* env, jref obj, JRIFieldThunk* field, jlong value);

JRI_PUBLIC_API(void)
JRI_SetFieldByName_float(JRIEnv* env, jref obj, JRIFieldThunk* field, jfloat value);

JRI_PUBLIC_API(void)
JRI_SetFieldByName_double(JRIEnv* env, jref obj, JRIFieldThunk* field, jdouble value);

/******************************************************************************/

JRI_PUBLIC_API(jref)
JRI_GetStaticFieldByName(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jbool)
JRI_GetStaticFieldByName_boolean(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jbyte)
JRI_GetStaticFieldByName_byte(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jchar)
JRI_GetStaticFieldByName_char(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jshort)
JRI_GetStaticFieldByName_short(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jint)
JRI_GetStaticFieldByName_int(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jlong)
JRI_GetStaticFieldByName_long(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jfloat)
JRI_GetStaticFieldByName_float(JRIEnv* env, jref obj, JRIFieldThunk* field);

JRI_PUBLIC_API(jdouble)
JRI_GetStaticFieldByName_double(JRIEnv* env, jref obj, JRIFieldThunk* field);

/******************************************************************************/

JRI_PUBLIC_API(void)
JRI_SetStaticFieldByName(JRIEnv* env, jref obj, JRIFieldThunk* field, jref value);

JRI_PUBLIC_API(void)
JRI_SetStaticFieldByName_boolean(JRIEnv* env, jref obj, JRIFieldThunk* field, jbool value);

JRI_PUBLIC_API(void)
JRI_SetStaticFieldByName_byte(JRIEnv* env, jref obj, JRIFieldThunk* field, jbyte value);

JRI_PUBLIC_API(void)
JRI_SetStaticFieldByName_char(JRIEnv* env, jref obj, JRIFieldThunk* field, jchar value);

JRI_PUBLIC_API(void)
JRI_SetStaticFieldByName_short(JRIEnv* env, jref obj, JRIFieldThunk* field, jshort value);

JRI_PUBLIC_API(void)
JRI_SetStaticFieldByName_int(JRIEnv* env, jref obj, JRIFieldThunk* field, jint value);

JRI_PUBLIC_API(void)
JRI_SetStaticFieldByName_long(JRIEnv* env, jref obj, JRIFieldThunk* field, jlong value);

JRI_PUBLIC_API(void)
JRI_SetStaticFieldByName_float(JRIEnv* env, jref obj, JRIFieldThunk* field, jfloat value);

JRI_PUBLIC_API(void)
JRI_SetStaticFieldByName_double(JRIEnv* env, jref obj, JRIFieldThunk* field, jdouble value);

/******************************************************************************/

JRI_PUBLIC_API(jref)
JRI_NewObjectByName(JRIEnv* env, jref clazz, JRIMethodThunk* method, ...);

JRI_PUBLIC_API(JRIValue)
JRI_CallMethodByName(JRIEnv* env, jref obj, JRIMethodThunk* method, ...);

JRI_PUBLIC_API(JRIValue)
JRI_CallStaticMethodByName(JRIEnv* env, jref clazz, JRIMethodThunk* method, ...);

/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif /* JRIAUTO_H */
/******************************************************************************/
