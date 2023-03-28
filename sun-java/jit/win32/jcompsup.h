// $Header: /m/src/ns/sun-java/jit/win32/Attic/jcompsup.h,v 1.2.10.1 1996/09/29 20:54:16 jg Exp $
/*
 * Copyright (c) 1996 Borland International. All Rights Reserved.
 *
 * AppAccelerator(tm) for x86
 *
 * JCompSup.h, R. Crelier, 7/31/96
 *
*/

/* Compiler support functions */

void
CompSupport_invokevirtualobject(void);

void
CompSupport_invokeinterface(void);

HObject * __stdcall
CompSupport_new(ClassClass *cl);

HObject * __stdcall
CompSupport_newarray(long T, long size);

HArrayOfObject * __stdcall
CompSupport_anewarray(ClassClass *cb, long size);

HArrayOfObject * __cdecl
CompSupport_multianewarray(ClassClass *cb, int dimensions, ...);

void __stdcall
CompSupport_aastore(HArrayOfObject *arrh, unsigned int index, HObject *value);

HObject * __stdcall
CompSupport_checkcast(ClassClass *cb, HObject *h);

int __stdcall
CompSupport_instanceof(ClassClass *cb, HObject *h);

void __stdcall
CompSupport_athrow(HObject *obj);

void __stdcall
CompSupport_monitorenter(HObject *obj);

void __stdcall
CompSupport_monitorexit(HObject *obj, long obj2xctxt);

void __stdcall
CompSupport_throwArrayIndexOutOfBounds(long index);

/* Compiler support variables: */

extern double CompSupport_dconst_0;
extern double CompSupport_dconst_1;

extern float CompSupport_fconst_0;
extern float CompSupport_fconst_1;
extern float CompSupport_fconst_2;


/* Support for long (i.e. int64_t) */

void
CompSupport_lmul(void);

void
CompSupport_ldiv(void);

void
CompSupport_lrem(void);

void
CompSupport_lshl(void);

void
CompSupport_lshr(void);

void
CompSupport_lushr(void);

double __stdcall
CompSupport_drem(double dividend, double divisor);

void
CompSupport_f2i(void);

void
CompSupport_d2i(void);

void
CompSupport_f2l(void);

void
CompSupport_d2l(void);

/* exception handler registered in the prologue of compiled methods */
int
HandleException(void);


