// $Header: /m/src/ns/sun-java/jit/win32/borland/jcompsup.c,v 1.7.6.2 1996/09/30 02:44:34 dhopwood Exp $
/*
 * Copyright (c) 1996 Borland International. All Rights Reserved.
 *
 * AppAccelerator(tm) for x86
 *
 * JCompSup.c, R. Crelier,9/20/96
 *
*/
#include "oobj.h"
#include "interpreter.h"
#include "exceptions.h"

#include "jinterf.h"

/*
 * An awful hack, but the dumb MSC compiler #define's exception to _exception
 * for compatibility with non-ANSI names, and this conflicts with the field
 * named 'exception' in 'struct execenv' of interpreter.h.
 */
#include <math.h>
#undef exception

// fpu control word values:

// chop rounding, extended precision, no exceptions
static short cwChop80 = 0x1F7F;

// chop rounding, extended precision, zero divide and invalid operation (0/0)
// exceptions
static short cwChop80ZI = 0x1F7A;


/* Support for invoking interfaces and methods from arrays */
__declspec(naked) void
CompSupport_invokevirtualobject(void)
{
// -> edx: method table
// <- edx: method table
    // edx contains the supposed method table.	If the object we have is
    // really an array, then return classobject's method table instead.
    __asm   {
	test	edx, 0x1f
	je	NotAnArray
	mov	edx, ObjectMethodTable
NotAnArray:
	ret
    }
}


/* Given a handle obj and a method ID, find the method for that object that
 * corresponds to the given ID.
 */
static void * __cdecl
ID2Method(unsigned char *ra, int mslot, long nargs, unsigned long ID,
    struct methodblock *caller_mb, JHandle *obj, ...)	// nargs + 4 params
{
    ClassClass *cb;
    int nslots, slot;
    struct methodblock **mbt, *mb;
    char buf[256];
    int len;
    char *msg;

    if (obj == 0)
    {
	SignalErrorUnwind(0, JAVAPKG "NullPointerException", 0,
	    (unsigned char *)&ra, nargs + 4);		// nargs + 4 params
	return 0;	// not reached
    }
    /* Currently, arrays don't implement any interfaces that have
     * methods in them.  But this is cheap, and we may change our
     * minds some time in the future. */
    if (obj_flags(obj) == T_NORMAL_OBJECT)
    {
	struct methodtable *mtable = obj_methodtable(obj);
	mbt = mtable->methods;
	cb = mtable->classdescriptor;
    }
    else
    {
	cb = classJavaLangObject;
	mbt = cbMethodTable(cb)->methods;
    }
    nslots = cbMethodTableSize(cb);
    if (mslot < nslots && (mb = mbt[mslot]) != 0 && mb->fb.ID == ID)
    	slot = mslot - 1; // set up so that security tests are still done
    else
	slot = 0;

    while (--nslots > 0)	// slot 0 not used
    {
	mb = mbt[++slot];
	if  (mb->fb.ID == ID)
	{
	    if (mb->fb.access & ACC_STATIC)
	    {
		msg = ": method %s used to be dynamic, but is now static";
		break;
	    }
	    if (!(mb->fb.access & ACC_PUBLIC))
	    {
		msg = ": method %s has access restrictions: can't call via interface";
		nslots = -1; // note security violation
		break;
	    }
	    *(int *)(ra - 9) = slot;	// patch mslot hint in caller
	    return mb;
	}
    }
    if (0 == nslots) 
        msg = ": instance method %s not found";

    (*p_classname2string)(classname(cb), buf, sizeof(buf));
    len = strlen(buf);
    /* If buffer overflow, quietly truncate */
    (void) (*p_jio_snprintf)(buf + len, sizeof(buf)-len,
	msg,
	(*p_IDToNameAndTypeString)(ID));

    if (-1 == nslots) {
        SignalErrorUnwind(0, JAVAPKG "IllegalAccessError", buf,
	  (unsigned char *)&ra, nargs + 4);		// nargs + 4 params
    } else {
        SignalErrorUnwind(0, JAVAPKG "IncompatibleClassChangeError", buf,
	  (unsigned char *)&ra, nargs + 4);		// nargs + 4 params
    }

    return 0;	// not reached
}


typedef struct methodblock methodblock;     /* for use in in-line assembly */

__declspec(naked) void
CompSupport_invokeinterface(void)
{
// -> [esp+20+4*i]  arg(i), i = 0, 1, ...
// -> [esp+16]	    caller mb
// -> [esp+12]	    ID
// -> [esp+8]	    nargs
// -> [esp+4]	    mslot hint
// -> [esp]	    return address
// <- reg(s)	    interface result, if any
    __asm   {
	call	ID2Method	// __cdecl to keep params on the stack
	pop	edx		// pop return address
	add	esp, 12		// discard hint, nargs, and ID
	pop	ecx		// pop caller mb
	push	edx		// push return address
	mov	edx, eax	// callee mb
	mov	eax, [eax]methodblock.CompiledCode
	jmp	eax		// jump to the compiled function
    }
}


HObject * __stdcall
CompSupport_new(ClassClass *cb)
{
    HObject *ret;

    if ((ret = (*p_ObjAlloc)(cb, 0)) == 0)
    {
	(*p_SignalError)(0, JAVAPKG "OutOfMemoryError", 0);
	ErrorUnwind(0);
	return 0;	// not reached
    }
    memset((char *)(unhand(ret)), 0, cbInstanceSize(cb));
    return ret;
}


HObject * __stdcall
CompSupport_newarray(long T, long size)		// 2 params
{
    HObject *ret;

    if (size < 0)
    {
	SignalErrorUnwind(0, JAVAPKG "NegativeArraySizeException", 0,
	    (unsigned char *)&T - 4, 2);	// 2 params
	return 0;	// not reached
    }
    ret = (*p_ArrayAlloc)(T, size);
    if (ret == 0)
    {
	(*p_SignalError)(0, JAVAPKG "OutOfMemoryError", 0);
	ErrorUnwind(0);
    }
    else
    {
	memset(unhand(ret), 0, (*p_sizearray)(T, size));
    }
    return ret;
}


HArrayOfObject * __stdcall
CompSupport_anewarray(ClassClass *cb, long size)	// 2 params
{
    HArrayOfObject *ret;

    if (size < 0)
    {
	SignalErrorUnwind(0, JAVAPKG "NegativeArraySizeException", 0,
	    (unsigned char *)&cb - 4, 2);		// 2 params
	return 0;	// not reached
    }
    ret = (HArrayOfObject *)(*p_ArrayAlloc)(T_CLASS, size);
    if (ret == 0)
    {
	(*p_SignalError)(0, JAVAPKG "OutOfMemoryError", 0);
	ErrorUnwind(0);
    }
    else
    {
	memset(unhand(ret), 0, (*p_sizearray)(T_CLASS, size));
	unhand(ret)->body[size] = (HObject *)cb;
    }
    return ret;
}


HArrayOfObject * __cdecl
CompSupport_multianewarray(ClassClass *cb, int dimensions, ...)	// dimensions + 2 params
{
    struct execenv *ee = (*p_EE)();
    HArrayOfObject *result;
    stack_item sizes[256];  /* maximum possible size */
    int i;
    va_list args;

/* done in the compiler, class cannot change at run time.
    (*p_ResolveClassConstantFromClass)(cb, CONSTANT_POOL_ARRAY_CLASS_INDEX,
	ee, 1 << CONSTANT_Class);
*/
    va_start(args, dimensions);
    for (i = 0; i < dimensions; i++)
    {
	if ((sizes[i].i = va_arg(args, long)) < 0)
	{
	    SignalErrorUnwind(ee, JAVAPKG "NegativeArraySizeException", 0,
		(unsigned char *)&cb - 4, dimensions + 2);	// dimensions + 2 params
            return 0;	// not reached
        }
    }
    va_end(args);
    result = (HArrayOfObject *)(*p_MultiArrayAlloc)(dimensions, cb, sizes);
    if (result == 0)
	(*p_SignalError)(0, JAVAPKG "OutOfMemoryError", 0);
    if (exceptionOccurred(ee))
	ErrorUnwind(ee);
    return result;
}


void __stdcall
CompSupport_aastore(HArrayOfObject *arrh, unsigned int index, HObject *value)	// 3 params
{
    ArrayOfObject *arr;
    ClassClass *array_cb;

    if (arrh == 0)
    {
	SignalErrorUnwind(0, JAVAPKG "NullPointerException", 0,
	    (unsigned char *)&arrh - 4, 3);					// 3 params
	return;	// not reached
    }
    arr = unhand(arrh);
    if (index >= obj_length(arrh))
    {
	char buf[30];
	/* On (unlikely) overflow, quietly truncate */
	(void) (*p_jio_snprintf)(buf, sizeof(buf), "%d", index);
	SignalErrorUnwind(0, JAVAPKG "ArrayIndexOutOfBoundsException", buf,
	    (unsigned char *)&arrh - 4, 3);					// 3 params
	return;	// not reached
    }
    array_cb = (ClassClass *)(arr->body[obj_length(arrh)]);
    if (array_cb != classJavaLangObject && value != 0	// value != 0 is an optimization
    	// && (array_cb->access & ACC_FINAL) == 0
	// NO! type check not required if compile-time array type is final,
	// array_cb is the runtime array type
	&& !(p_is_instance_of)(value, array_cb, (*p_EE)()))
    {
	/* The value isn't null and doesn't match the type. */
	SignalErrorUnwind(0, JAVAPKG "ArrayStoreException", 0,
	    (unsigned char *)&arrh - 4, 3);					// 3 params
	return;	// not reached
    }
    arr->body[index] = value;
}


/*
HObject * __stdcall
CompSupport_checkcast(ClassClass *cb, HObject *h)
{
    if (!(*p_is_instance_of)(h, cb, (*p_EE)()))
    {
    	...
    }
    return h;
}
*/


/* another version optimizing the usual case */
HObject * __stdcall
CompSupport_checkcast(ClassClass *cb, HObject *h)	// 2 params
{
    ClassClass *hcb;
    char buf[256];

    if (h == NULL)	// null can be cast to anything
	return NULL;

    if (obj_flags(h) == T_NORMAL_OBJECT)
    {
	if ((cb->access & ACC_INTERFACE) == 0)
	{
	    hcb = obj_classblock(h);
	    for (; ; hcb = unhand(cbSuperclass(hcb)))
	    {
		if (hcb == cb)
		    return h;
		if (cbSuperclass(hcb) == 0)
		    break;	// error
	    }
	}
	else if ((*p_is_subclass_of)(obj_classblock(h), cb, (*p_EE)()))
	    return h;
    }
    else if (cb->name[0] == SIGNATURE_ARRAY)
    {
        if ((*p_is_instance_of)(h, cb, (*p_EE)()))	// not the usual case
	    return h;
    }
    else if (cb == classJavaLangObject || cb == interfaceJavaLangCloneable)
	return h;

    (*p_classname2string)(classname(obj_array_classblock(h)),
	buf, sizeof(buf));
    SignalErrorUnwind(0, JAVAPKG "ClassCastException", buf,
	(unsigned char *)&cb - 4, 2);			// 2 params
    return 0;	// not reached
}


/*
int __stdcall
CompSupport_instanceof(ClassClass *cb, HObject *h)
{
    struct execenv *ee = (*p_EE)();
    int result = h != 0 && (*p_is_instance_of)(h, cb, ee);
    if (exceptionOccurred(ee))
	ErrorUnwind();
    return result;
}
*/


/* another version optimizing the usual case */
int __stdcall
CompSupport_instanceof(ClassClass *cb, HObject *h)
{
    ClassClass *hcb;
    struct execenv *ee;
    int result;

    if (h == NULL)
	return FALSE;

    if (obj_flags(h) == T_NORMAL_OBJECT)
    {
	if ((cb->access & ACC_INTERFACE) == 0)
	{
	    hcb = obj_classblock(h);
	    for (; ; hcb = unhand(cbSuperclass(hcb)))
	    {
		if (hcb == cb)
		    return TRUE;
		if (cbSuperclass(hcb) == 0)
		    return FALSE;
	    }
	}
	else
	    return (*p_is_subclass_of)(obj_classblock(h), cb, (*p_EE)());

    }
    else if (cb->name[0] != SIGNATURE_ARRAY)
	return (cb == classJavaLangObject || cb == interfaceJavaLangCloneable);

    // not the usual case
    ee = (*p_EE)();
    // exception is unlikely, don't bother setting a JavaFrame here
    result = (*p_is_instance_of)(h, cb, ee);
    if (exceptionOccurred(ee))
	ErrorUnwind(ee);

    return result;
}


void __stdcall
CompSupport_athrow(HObject *obj)
{
    struct execenv *ee = (*p_EE)();
    exceptionThrow(ee, obj);
    ErrorUnwind(ee);
}


void __stdcall
CompSupport_monitorenter(HObject *obj)			// 1 param
{
    if (obj == 0)
    {
	SignalErrorUnwind(0, JAVAPKG "NullPointerException", 0,
	    (unsigned char *)&obj - 4, 1);		// 1 param
	return;	// not reached
    }
    // no exception thrown by monitorEnter
    (*p_monitorEnter)(obj_monitor(obj));
}


void __stdcall
CompSupport_monitorexit(HObject *obj, long obj2xctxt)	// 2 params
{
    struct execenv *ee = (*p_EE)();

    if (obj == 0)
    {
	SignalErrorUnwind(ee, JAVAPKG "NullPointerException", 0,
	    (unsigned char *)&obj - 4, 2);		// 2 params
	return;	// not reached
    }
    // exception is unlikely, don't bother setting a JavaFrame here
    (*p_monitorExit)(obj_monitor(obj));
    if (exceptionOccurred(ee))
    {
    	if (obj2xctxt)
    	    // we are returning from a method, do not cleanup code again
	    *((long *)((unsigned char *)&obj + obj2xctxt)) = NO_CLEANUP;

	ErrorUnwind(ee);
    }
}


void __stdcall
CompSupport_throwArrayIndexOutOfBounds(long index)	// 1 param
{
    char buf[30];
    /* On (unlikely) overflow, quietly truncate */
    (void) (*p_jio_snprintf)(buf, sizeof(buf), "%d", index);
    SignalErrorUnwind(0, JAVAPKG "ArrayIndexOutOfBoundsException", buf,
	(unsigned char *)&index - 4, 1);		// 1 param
}


/* Support for long (int64_t).
 * The fpu is used for ldiv and lrem. This is faster than 64-bit integer
 * arithmetic on a Pentium.
 * The fpu cannot be used for lmul, because we need modulo arithmetic in
 * case of an overflow.
 */

__declspec(naked) void
CompSupport_lmul(void)
{
// -> [esp+4]	x low
// -> [esp+8]	x high
// -> [esp+12]	y low
// -> [esp+16]	y high
// <- eax	result low
// <- edx	result high
    __asm   {
    	mov	eax, [esp+16]
	mul	dword ptr [esp+4]
	mov	ecx, eax
	mov	eax, [esp+8]
	mul	dword ptr [esp+12]
	add	ecx, eax
	mov	eax, [esp+4]
	mul	dword ptr [esp+12]
	add	edx, ecx
	ret	16
    }
}


__declspec(naked) void
CompSupport_ldiv(void)
{
// -> [esp+4]	dividend low
// -> [esp+8]	dividend high
// -> [esp+12]	divisor low
// -> [esp+16]	divisor high
// <- eax	result low
// <- edx	result high
    __asm   {
	sub	esp, 12
	fstcw	[esp]
	fwait	// wait for pending unmasked exceptions (none in Java)
	fninit	// clear pending exceptions before unmasking Z and I
	fldcw	cwChop80ZI
	fild	qword ptr [esp+12+4]
	fild	qword ptr [esp+12+12]
	fdivp	st(1), st   // only possible integer range overflow:
			    // (-1e64)/(-1) = +1e64
			    // +1e64 is written as 0x80000000 by fistp
			    // this corresponds to the Java specification
	fistp	qword ptr [esp+4]   // Z or I exceptions detected here
	fldcw	[esp]
	add	esp, 4
	pop	eax
	pop	edx
	ret	16
    }
}


__declspec(naked) void
CompSupport_lrem(void)
{
// -> [esp+4]	dividend low
// -> [esp+8]	dividend high
// -> [esp+12]	divisor low
// -> [esp+16]	divisor high
// <- eax	result low
// <- edx	result high
    __asm   {
	sub	esp, 12
	fstcw	[esp]
	fwait	// wait for pending unmasked exceptions (none in Java)
	fninit	// clear pending exceptions before unmasking Z and I
	fldcw	cwChop80ZI  // round to nearest would work too
	fild	qword ptr [esp+12+12]	// divisor
	fild	qword ptr [esp+12+4]	// dividend
	// test for zero divisor
	mov     eax, [esp+12+12]
	cmp	eax, 0
	jne	nonzero
	mov     eax, [esp+12+16]
	cmp	eax, 0
	jne	nonzero
	// generate a zero divide or invalid operation (0/0) exception
	fxch
	fdivp	st(1), st
	jmp done
nonzero:
	fprem
	fnstsw	ax
	test	eax, 0x400  // C2 == 0?
	je	done
	fprem	// we had (-1e64) rem (+/- 1), one more step is necessary
done:
	fistp	qword ptr [esp+4]   // Z or I exception detected here
	fstp	st(0)	// pop divisor
	fldcw	[esp]
	add	esp, 4
	pop	eax
	pop	edx
	ret	16
    }
}


__declspec(naked) void
CompSupport_lshl(void)
{
// -> [esp+4]	arg low
// -> [esp+8]	arg high
// -> [esp+12]	shift count, low six bits are significative
// <- eax	result low
// <- edx	result high
    __asm   {
    	mov	ecx, [esp+12]
	test	ecx, 32
	je	lshl_below32
	mov	edx, [esp+4]
	xor	eax, eax
	shl	edx, cl
	ret	12
lshl_below32:
	mov	eax, [esp+4]
	mov	edx, [esp+8]
	shld	edx, eax, cl
	shl	eax, cl
	ret	12
    }
}


__declspec(naked) void
CompSupport_lshr(void)
{
// -> [esp+4]	arg low
// -> [esp+8]	arg high
// -> [esp+12]	shift count, low six bits are significative
// <- eax	result low
// <- edx	result high
    __asm   {
    	mov	ecx, [esp+12]
	test	ecx, 32
	je	lshr_below32
	mov	eax, [esp+8]
	cdq
	sar	eax, cl
	ret	12
lshr_below32:
	mov	eax, [esp+4]
	mov	edx, [esp+8]
	shrd	eax, edx, cl
	sar	edx, cl
	ret	12
    }
}


__declspec(naked) void
CompSupport_lushr(void)
{
// -> [esp+4]	arg low
// -> [esp+8]	arg high
// -> [esp+12]	shift count, low six bits are significative
// <- eax	result low
// <- edx	result high
    __asm   {
    	mov	ecx, [esp+12]
	test	ecx, 32
	je	lushr_below32
	mov	eax, [esp+8]
	xor	edx, edx
	shr	eax, cl
	ret	12
lushr_below32:
	mov	eax, [esp+4]
	mov	edx, [esp+8]
	shrd	eax, edx, cl
	shr	edx, cl
	ret	12
    }
}


__declspec(naked) void
CompSupport_f2i(void)
{
// -> [esp+4]	arg
// <- eax	result
    __asm   {
	fld	dword ptr [esp+4]
	sub	esp, 8
	fstcw	[esp]
	fwait	// wait for pending unmasked exceptions (none in Java)
	fldcw	cwChop80
	fistp	dword ptr [esp+4]
	fldcw	[esp]
	add	esp, 4
	pop	eax
	cmp	eax, 0x80000000
	je	f2i_verify
	ret	4
/* Result for underflow is 0x80000000, OK. Interpreter does it wrong.
 * Result for overflow is 0x80000000, not OK:
 * Java specification wants 0x7fffffff, the interpreter does it wrong.
 * Result for NaN is 0x80000000, not OK:
 * Java specification wants 0, the interpreter does it wrong too.
 */
f2i_verify:
	mov	ecx, dword ptr [esp+4]
	mov	edx, ecx
	// if NaN return 0
	and	ecx, 0x7fc00000
	cmp	ecx, 0x7f800000
	jne	f2i_RealOrQuietNaN
	// exp is all 1 and high bit of mantissa is 0, signaling NaN or infinity
	test	edx, 0x003fffff
	je	f2i_Real    // mantissa is all 0, infinity
f2i_NaN:
	xor	eax, eax
	ret	4
f2i_RealOrQuietNaN:
	cmp	ecx, 0x7fc00000
	je	f2i_NaN
f2i_Real:
	// if negative return eax else return eax-1
	cmp	edx, 0x80000000     // CF = arg is positive
	sbb	eax, 0
	ret	4
    }
}


__declspec(naked) void
CompSupport_d2i(void)
{
// -> [esp+4]	arg
// -> [esp+8]	arg
// <- eax	result
    __asm   {
	fld	qword ptr [esp+4]
	sub	esp, 8
	fstcw	[esp]
	fwait	// wait for pending unmasked exceptions (none in Java)
	fldcw	cwChop80
	fistp	dword ptr [esp+4]
	fldcw	[esp]
	add	esp, 4
	pop	eax
	cmp	eax, 0x80000000
	je	d2i_verify
	ret	8
/* Result for underflow is 0x80000000, OK. Interpreter does it wrong.
 * Result for overflow is 0x80000000, not OK:
 * Java specification wants 0x7fffffff, the interpreter does it wrong.
 * Result for NaN is 0x80000000, not OK:
 * Java specification wants 0, the interpreter does it wrong too.
 */
d2i_verify:
	mov	ecx, dword ptr [esp+8]
	mov	edx, ecx
	// if NaN return 0
	and	ecx, 0x7ff80000
	cmp	ecx, 0x7ff00000
	jne	d2i_RealOrQuietNaN
	// exp is all 1 and high bit of mantissa is 0, signaling NaN or infinity
	test	edx, 0x0007ffff
	jne	d2i_NaN
	// high part of mantissa is all 0, signaling NaN or infinity
	cmp	dword ptr [esp+4], 0
	je	d2i_Real    // mantissa is all 0, infinity
d2i_NaN:
	xor	eax, eax
	ret	8
d2i_RealOrQuietNaN:
	cmp	ecx, 0x7ff80000
	je	d2i_NaN
d2i_Real:
	// if negative return <edx,eax> else return <edx,eax>-1
	cmp	edx, 0x80000000     // CF = arg is positive
	sbb	eax, 0
	ret	8
    }
}


__declspec(naked) void
CompSupport_f2l(void)
{
// -> [esp+4]	arg
// <- eax	result low
// <- edx	result high
    __asm   {
	fld	dword ptr [esp+4]
	sub	esp, 12
	fstcw	[esp]
	fwait	// wait for pending unmasked exceptions (none in Java)
	fldcw	cwChop80
	fistp	qword ptr [esp+4]
	fldcw	[esp]
	add	esp, 4
	pop	eax
	pop	edx
	cmp	edx, 0x80000000
	jne	f2l_done
	cmp	eax, 0
	je	f2l_verify
f2l_done:
	ret	4
/* Result for underflow is 0x8000000000000000, OK. Interpreter does it wrong.
 * Result for overflow is 0x8000000000000000, not OK:
 * Java specification wants 0x7fffffffffffffff, the interpreter does it wrong.
 * Result for NaN is 0x8000000000000000, not OK:
 * Java specification wants 0, the interpreter does it wrong too.
 */
f2l_verify:
	mov	ecx, dword ptr [esp+4]
	// if NaN return 0
	and	ecx, 0x7fc00000
	cmp	ecx, 0x7f800000
	jne	f2l_RealOrQuietNaN
	// exp is all 1 and high bit of mantissa is 0, signaling NaN or infinity
	mov	ecx, dword ptr [esp+4]
	test	ecx, 0x003fffff
	je	f2l_Real    // mantissa is all 0, infinity
f2l_NaN:
	xor	edx, edx    // eax == 0
	ret	4
f2l_RealOrQuietNaN:
	cmp	ecx, 0x7fc00000
	je	f2l_NaN
	mov	ecx, dword ptr [esp+4]
f2l_Real:
	// if negative return <edx,eax> else return <edx,eax>-1
	cmp	ecx, 0x80000000     // CF = arg is positive
	sbb	eax, 0
	sbb	edx, 0
	ret	4
    }
}


__declspec(naked) void
CompSupport_d2l(void)
{
// -> [esp+4]	arg
// -> [esp+8]	arg
// <- eax	result low
// <- edx	result high
    __asm   {
	fld	qword ptr [esp+4]
	sub	esp, 12
	fstcw	[esp]
	fwait	// wait for pending unmasked exceptions (none in Java)
	fldcw	cwChop80
	fistp	qword ptr [esp+4]
	fldcw	[esp]
	add	esp, 4
	pop	eax
	pop	edx
	cmp	edx, 0x80000000
	jne	d2l_done
	cmp	eax, 0
	je	d2l_verify
d2l_done:
	ret	8
/* Result for underflow is 0x8000000000000000, OK. Interpreter does it wrong.
 * Result for overflow is 0x8000000000000000, not OK:
 * Java specification wants 0x7fffffffffffffff, the interpreter does it wrong.
 * Result for NaN is 0x8000000000000000, not OK:
 * Java specification wants 0, the interpreter does it wrong too.
 */
d2l_verify:
	mov	ecx, dword ptr [esp+8]
	// if NaN return 0
	and	ecx, 0x7ff80000
	cmp	ecx, 0x7ff00000
	jne	d2l_RealOrQuietNaN
	// exp is all 1 and high bit of mantissa is 0, signaling NaN or infinity
	mov	ecx, dword ptr [esp+8]
	test	ecx, 0x0007ffff
	jne	d2l_NaN
	// high part of mantissa is all 0, signaling NaN or infinity
	cmp	dword ptr [esp+4], 0
	je	d2l_Real    // mantissa is all 0, infinity
d2l_NaN:
	xor	edx, edx    // eax == 0
	ret	8
d2l_RealOrQuietNaN:
	cmp	ecx, 0x7ff80000
	je	d2l_NaN
	mov	ecx, dword ptr [esp+8]
d2l_Real:
	// if negative return <edx,eax> else return <edx,eax>-1
	cmp	ecx, 0x80000000     // CF = arg is positive
	sbb	eax, 0
	sbb	edx, 0
	ret	8
    }
}


static int __stdcall
finite(double *d)
{
    __asm   {
	mov	edx, d
	mov	eax, dword ptr [edx+4]
	and	eax, 0x7ff00000
	sub	eax, 0x7ff00000
	cmp	eax, 1
	sbb	eax, eax    // eax = exp == all 1 ? -1 : 0
	inc	eax
    }
}


static int __stdcall
isnan(double *d)
{
// isnan(infinity) -> 0
    __asm   {
	mov	edx, d
	mov	eax, dword ptr [edx+4]
	and	eax, 0x7ff80000
	cmp	eax, 0x7ff00000
	jne	RealOrQuietNaN
	// exp is all 1 and high bit of mantissa is 0, signaling NaN or infinity
	test	dword ptr [edx+4], 0x0007ffff
	jne	NaN	// not infinity
	// high part of mantissa is all 0, signaling NaN or infinity
	cmp	dword ptr [edx+0], 0
	jne	NaN	// not infinity
	// infinity, eax == 0x7ff00000 -> returns 0
RealOrQuietNaN:
	cmp	eax, 0x7ff80000
	mov	eax, 0
	jne	Real
NaN:
	mov	eax, 1
Real:
    }
}


double __stdcall
CompSupport_drem(double dividend, double divisor)
{
    if (!finite(&divisor))
    {
	if (isnan(&divisor))
	    return divisor;
	if (finite(&dividend))
	    return dividend;
    }
    return fmod(dividend, divisor);
}


double CompSupport_dconst_0 = 0.0;
double CompSupport_dconst_1 = 1.0;

float CompSupport_fconst_0 = 0.0f;
float CompSupport_fconst_1 = 1.0f;
float CompSupport_fconst_2 = 2.0f;

