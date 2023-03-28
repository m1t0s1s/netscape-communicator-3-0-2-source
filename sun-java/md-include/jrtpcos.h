/*
 * @(#)jrtpcos.h	1.2 95/06/14 Dennis Tabuena
 *
 * Copyright (c) 1996 Netscape Communications Corporation. All Rights Reserved.
 *
 */
#ifndef jrtpcos_h___
#define jrtpcos_h___

#include "prpcos.h"

#if defined(XP_PC) && !defined(_WIN32)

#ifdef __cplusplus
extern "C" {
#endif

extern void JRTPC_SetClientOutputCallback( void (*f)(const char *) );

/*
 * Initialize the java runtime for the PC. The module jrtpcos must
 * be staticially linked with the executable. The cbStruct may be null,
 * in which case we will use default implementations.
 */
extern HINSTANCE JRTPC_Init(struct PRMethodCallbackStr *cbStruct);

#ifdef __cplusplus
}; /* extern "C" */
#endif

#endif

#endif /* jrtpcos_h___ */
