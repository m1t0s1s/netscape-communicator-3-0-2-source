/* -*- Mode: C; tab-width: 4; -*- */
/*******************************************************************************
 * JRI Auto-Initialization glue for javah
 * Copyright (c) 1996 Netscape Communications Corporation. All rights reserved.
 ******************************************************************************/

#include "jriauto.h"

/******************************************************************************/

#define DEFINE_ACCESSORS(ResultType, Suffix, Accessor)					  \
JRI_PUBLIC_API(ResultType)												  \
JRI_GetField##Suffix(JRIEnv* env, jref obj, JRIFieldThunk* field)		  \
{																		  \
	return (*env)->Get##Accessor(env, obj, field);						  \
}																		  \
JRI_PUBLIC_API(void)													  \
JRI_SetField##Suffix(JRIEnv* env, jref obj, JRIFieldThunk* field,		  \
					 ResultType value)									  \
{																		  \
	(*env)->Set##Accessor(env, obj, field, value);						  \
}																		  \
JRI_PUBLIC_API(ResultType)												  \
JRI_GetStaticField##Suffix(JRIEnv* env, jref clazz, JRIFieldThunk* field) \
{																		  \
	return (*env)->Get##Accessor(env, clazz, field);					  \
}																		  \
JRI_PUBLIC_API(void)													  \
JRI_SetStaticField##Suffix(JRIEnv* env, jref clazz, JRIFieldThunk* field, \
						   ResultType value)							  \
{																		  \
	(*env)->Set##Accessor(env, clazz, field, value);					  \
}																		  \

DEFINE_ACCESSORS(jref,		ByName,			Field)
DEFINE_ACCESSORS(jbool,		ByName_boolean,	Field_boolean)
DEFINE_ACCESSORS(jbyte,		ByName_byte,	Field_byte)
DEFINE_ACCESSORS(jchar,		ByName_char,	Field_char)
DEFINE_ACCESSORS(jshort,	ByName_short,	Field_short)
DEFINE_ACCESSORS(jint,		ByName_int,		Field_int)
DEFINE_ACCESSORS(jlong,		ByName_long,	Field_long)
DEFINE_ACCESSORS(jfloat,	ByName_float,	Field_float)
DEFINE_ACCESSORS(jdouble,	ByName_double,	Field_double)

/******************************************************************************/

JRI_PUBLIC_API(jref)
JRI_NewObjectByName(JRIEnv* env, jref clazz, JRIMethodThunk* method, ...)
{
    jref result;
	va_list args;
    va_start(args, method);
	result = JRI_NewObjectV(env, clazz, method, args);
	va_end(args);
	return result;
}

JRI_PUBLIC_API(JRIValue)
JRI_CallMethodByName(JRIEnv* env, jref obj, JRIMethodThunk* method, ...)
{
	JRIValue result;
	va_list args;
    va_start(args, method);
	result = JRI_CallMethodV(env, obj, method, args);
	va_end(args);
	return result;
}

JRI_PUBLIC_API(JRIValue)
JRI_CallStaticMethodByName(JRIEnv* env, jref clazz, JRIMethodThunk* method, ...)
{
    JRIValue result;
	va_list args;
    va_start(args, method);
	result = JRI_CallStaticMethodV(env, clazz, method, args);
	va_end(args);
	return result;
}

/******************************************************************************/
