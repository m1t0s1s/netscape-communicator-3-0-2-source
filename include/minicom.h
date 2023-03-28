/* -*- Mode: C; tab-width: 4; -*- */
/*******************************************************************************
 * Mini Component Object Model
 * Copyright (c) 1996 Netscape Communications Corporation. All rights reserved.
 ******************************************************************************/
 
#ifndef MINICOM_H
#define MINICOM_H

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************/

#if defined(XP_PC) 
#if defined(_WIN32)
#include "objbase.h"
#else
#include <windows.h>
#include "compobj.h"
#endif
#else /* !XP_PC */

typedef struct _GUID {
	long		Data1;
	short		Data2;
	short		Data3;
	char		Data4[8];
} GUID;

typedef GUID	IID;
typedef GUID	CLSID;

typedef IID*	REFIID;
typedef GUID*	REFGUID;
typedef CLSID*	REFCLSID;

#ifdef __cplusplus
#define EXTERN_C		extern "C"
#else
#define EXTERN_C
#endif /* cplusplus */

#ifndef INITGUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID name
#else
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
#endif /* INITGUID */

#endif /* !XP_PC */

#define JRI_DEFINE_GUID(name, l, w1, w2) \
	DEFINE_GUID(name, l, w1, w2,		 \
				0xbb, 0x58, 0x00, 0x80, 0x5f, 0x74, 0x03, 0x79)

typedef long
(*MCOM_QueryInterface_t)(void* self, REFIID id, void* *result);

typedef long
(*MCOM_AddRef_t)(void* self);

typedef long
(*MCOM_Release_t)(void* self);

#ifndef XP_PC

typedef struct IUnknown {
    MCOM_QueryInterface_t	QueryInterface;
    MCOM_AddRef_t			AddRef;
    MCOM_Release_t			Release;
} IUnknown;

#define IUnknown_QueryInterface(self, interfaceID, resultingInterface)		\
		(((*((IUnknown**)self))->QueryInterface)(self, interfaceID, resultingInterface))

#define IUnknown_AddRef(self)							\
		(((*((IUnknown**)self))->AddRef)(self))

#define IUnknown_Release(self)							\
		(((*((IUnknown**)self))->Release)(self))

#endif /* !XP_PC */

typedef long
(*MCOM_CreateInstance_t)(void* self, IUnknown* outer, REFIID interfaceID,
						 void** newInstance);

typedef long
(*MCOM_LockServer_t)(void* self, int doLock);

/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif /* MINICOM_H */
/******************************************************************************/
