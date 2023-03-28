// $Header: /m/src/ns/sun-java/jit/win32/Attic/jinterf.c,v 1.9.8.2 1996/09/30 02:46:51 dhopwood Exp $
/*
 * Copyright (c) 1996 Borland International. All Rights Reserved.
 *
 * AppAccelerator(tm) for x86
 *
 * JInterf.c, R. Crelier, 9/24/96
 *
*/
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "oobj.h"
#include "interpreter.h"
#include "exceptions.h"
#include "jinterf.h"

static char logo0 [] = "AppAccelerator(tm) 1.0.2a for Java, x86 version.\n";
static char logo1 [] = "Copyright (c) 1996 Borland International. All Rights Reserved.\n";

//#define   CHECK_REGISTRY
#ifdef	CHECK_REGISTRY
static HKEY baseKey1 = HKEY_LOCAL_MACHINE;
static char regKeyStr1[] = "Software\\Borland\\AppAccelerator(tm)\\1.0\\Licensed Software\\Borland C++ 5.0 Suite";
static HKEY baseKey2 = HKEY_CLASSES_ROOT;
static char regKeyStr2[] = "AppAccelerator(tm), Borland International, Inc.";
static char regKeyStr3[] = "B\\CHG_~JD 2V:!998HM1/>#OG#<0>O95\">1W9;<5M56t";
//static char regKeyStr3[] = "CLSID\\{FA23B531-5AA1-11CF-807A-0000C0424A96}";
#endif

/* Undefine this variable for freeing the bytecode after a compilation.
 * Has no effect if a control panel is present
 */
#define KEEP_BYTECODE

extern void
InitCompiler(void);

extern bool_t
jitCompile(struct methodblock *mb, void **code, void **info, char **err, ExecEnv *ee);

#ifdef	BINARY_DEBUG
static unsigned count = 0;  // number of succesfully compiled methods
#endif


static int
CompilerEnabled = 0;

/* Declaration of the links to the virtual machine */

static int
*p_JavaVersion = NULL;	// NULL indicates that the VM is not linked

static void
(**pp_InitializeForCompiler)(ClassClass *cb);

static bool_t
(**pp_invokeCompiledMethod)(JHandle *o, struct methodblock *mb,
    int args_size, ExecEnv *ee);

static void
(**pp_CompiledCodeSignalHandler)(int sig, void *info, void *uc);

static void
(**pp_CompilerFreeClass)(ClassClass *cb);

#ifdef IN_NETSCAPE_NAVIGATOR
static void
(**pp_CompilerFreeMethod)(struct methodblock *mb);
#endif

static bool_t
(**pp_CompilerCompileClass)(ClassClass *cb);

static bool_t
(**pp_CompilercompileClasses)(Hjava_lang_String *name);

static Hjava_lang_Object *
(**pp_CompilerCommand)(Hjava_lang_Object *any);

static void
(**pp_CompilerEnable)();

static void
(**pp_CompilerDisable)();

static bool_t
(**pp_PCinCompiledCode)(unsigned char *pc, struct methodblock *mb);

static unsigned char *
(**pp_CompiledCodePC)(JavaFrame *frame, struct methodblock *mb);

static void
(**pp_ReadInCompiledCode)(void *context,
			  struct methodblock *mb,
			  int attribute_length,
			  unsigned long (*get1byte)(),
			  unsigned long (*get2bytes)(),
			  unsigned long (*get4bytes)(),
			  void (*getNbytes)());

static JavaFrame *
(**pp_CompiledFramePrev)(JavaFrame *frame, JavaFrame *buf);

#ifdef IN_NETSCAPE_NAVIGATOR
static void
(**pp_SetCompiledFrameAnnotation)(JavaFrame *frame, JHandle *annotation);
#endif

static char *
*p_CompiledCodeAttribute;

static int
*p_UseLosslessQuickOpcodes;

void *
(*p_malloc)(size_t size);

void *
(*p_calloc)(size_t nitems, size_t size);

void *
(*p_realloc)(void *block, size_t size);

void
(*p_free)(void *block);

static ClassClass **
*p_binclasses;

static long
*p_nbinclasses;

static void
(*p_lock_classes)(void);

static void
(*p_unlock_classes)(void);

static ClassClass *
*p_classJavaLangClass;

static ClassClass *
*p_classJavaLangObject;

static ClassClass *
*p_classJavaLangString;

static ClassClass *
*p_classJavaLangThrowable;

static ClassClass *
*p_classJavaLangException;

static ClassClass *
*p_classJavaLangRuntimeException;

static ClassClass *
*p_interfaceJavaLangCloneable;

ExecEnv *
(*p_EE)(void);

void
(*p_SignalError)(struct execenv *ee, char *ename, char *DetailMessage);

static JHandle *
(*p_exceptionInternalObject)(internal_exception_t exc);

char *
(*p_GetClassConstantClassName)(cp_item_type *constant_pool, int index);

bool_t
(*p_ResolveClassConstant)(cp_item_type *constant_pool, unsigned index,
    struct execenv *ee, unsigned mask);

bool_t
(*p_ResolveClassConstantFromClass)(ClassClass *class, unsigned index,
    struct execenv *ee, unsigned mask);

bool_t
(*p_VerifyClassAccess)(ClassClass *current_class, ClassClass *new_class,
    bool_t classloader_only);

ClassClass *
(*p_FindClass)(struct execenv *ee, char *name, bool_t resolve);

ClassClass *
(*p_FindClassFromClass)(struct execenv *ee, char *name,
    bool_t resolve, ClassClass *from);

bool_t
(*p_dynoLink)(struct methodblock *mb);

long
(*p_do_execute_java_method_vararg)(ExecEnv *ee, void *obj,
    char *method_name, char *signature,
    struct methodblock *mb,
    bool_t isStaticCall, va_list args,
    long *highBits, bool_t shortFloats);

bool_t
(*p_is_subclass_of)(ClassClass *cb, ClassClass *dcb, ExecEnv *ee);

static bool_t
(*p_invokeJavaMethod)(JHandle *o, struct methodblock *mb,
    int args_size, ExecEnv *ee);

static bool_t
(*p_invokeSynchronizedJavaMethod)(JHandle *o, struct methodblock *mb,
    int args_size, ExecEnv *ee);

static bool_t
(*p_invokeAbstractMethod)(JHandle *o, struct methodblock *mb,
    int args_size, ExecEnv *ee);

static bool_t
(*p_invokeLazyNativeMethod)(JHandle *o, struct methodblock *mb,
    int args_size, ExecEnv *ee);

static bool_t
(*p_invokeSynchronizedNativeMethod)(JHandle *o, struct methodblock *mb,
    int args_size, ExecEnv *ee);

static bool_t
(*p_invokeCompiledMethod)(JHandle *o, struct methodblock *mb,
    int args_size, ExecEnv *ee);

void
(*p_monitorEnter)(unsigned int key);

void
(*p_monitorExit)(unsigned int key);

void
(*p_monitorRegister)(sys_mon_t *mid, char *name);

int
(*p_sysMonitorSizeof)(void);

int
(*p_sysMonitorEnter)(sys_mon_t *mid);

int
(*p_sysMonitorExit)(sys_mon_t *mid);

HObject *
(*p_ObjAlloc)(ClassClass *cb, long n0);

HObject *
(*p_ArrayAlloc)(int t, int l);

HObject *
(*p_MultiArrayAlloc)(int dimensions, ClassClass *array_cb, stack_item *sizes);

int
(*p_sizearray)(int t, int l);

HObject *
(*p_newobject)(ClassClass *cb, unsigned char *pc, struct execenv *ee);

bool_t
(*p_is_instance_of)(JHandle *h, ClassClass *dcb, ExecEnv *ee);

int
(*p_jio_snprintf)(char *str, size_t count, const char *fmt, ...);

static char *
(*p_javaString2CString)(Hjava_lang_String *s, char *buf, int buflen);

char *
(*p_classname2string)(char *str, char *dst, int size);

char *
(*p_IDToNameAndTypeString)(unsigned short ID);

char *
(*p_IDToType)(unsigned short ID);

static void
(*p_DumpThreads)(void);

static void
(*p_printToConsole)(const char *);

static JavaStack *
(*p_CreateNewJavaStack)(ExecEnv *ee, JavaStack *previous_stack);

static long
(*p_execute_java_static_method)(ExecEnv *ee, ClassClass *cb,
    char *method_name, char *signature, ...);

static bool_t
(*p_ExecuteJava)(unsigned char *initial_pc, ExecEnv *ee);

static long
(*p_now)(void);

static int
*p_java_monitor;

static void
(*p_java_mon)(struct methodblock *caller, struct methodblock *callee, int time);

static unsigned long
*p_JavaStackSize;


/* Declaration of the calls to the control panel */
static void
(*p_notifyCompilation)(struct methodblock *mb, char *err, bool_t *freeByteCode);

static void
(*p_notifyClassInit)(ClassClass *cb);

static void
(*p_notifyClassFree)(ClassClass *cb);

static char *DefaultControlName = "JAVACTRL.DLL";
static char *ControlEnvVarName = "JAVA_CONTROL";
static HMODULE controlHandle = 0;


/* local copy of class java.lang.object */
ClassClass *
classJavaLangObject;

/* local copy of class java.lang.Cloneable */
ClassClass *
interfaceJavaLangCloneable;

/* The methodtable for java.lang.object */
struct methodtable *ObjectMethodTable;


/*
 * Lock around changing a method invoker and accessing the code_ranges array
 */
static sys_mon_t *_invoker_lock;
#define INVOKER_LOCK_INIT() (*p_monitorRegister)(_invoker_lock, "Invoker change lock")
#define INVOKER_LOCK()	    (*p_sysMonitorEnter)(_invoker_lock)
#define INVOKER_UNLOCK()    (*p_sysMonitorExit)(_invoker_lock)


/* Data structure and function to map a pc to a compiled method.
 * This could be avoided by introducing a frame pointer to crawl
 * the compiled stack. We don't have to unwind the stack for handling
 * exceptions, since we map exceptions to native exceptions.
 * However, we need to be able to crawl the stack to correctly implement
 * Throwable.fillInStackTrace and different methods of SecurityManager.
 * Last bit of efficiency is not an issue here.
 */
typedef struct CodeRange
{
    unsigned char *start_pc;
    unsigned char *end_pc;
    struct methodblock *mb;

}   CodeRange;

static CodeRange *
code_ranges = 0;

static int
ncode_ranges = 0;

static int
max_ncode_ranges = 1000;

static unsigned char *
StubRetAddr;


static struct methodblock *
PC2CompiledMethod(unsigned char *pc)
{
    int i, l, r;
    CodeRange *range;
    struct methodblock *mb = 0;

    INVOKER_LOCK();
    l = 0;
    r = ncode_ranges - 1;
    while (l <= r)
    {
    	i = (l + r)/2;
	range = code_ranges + i;
	if (pc >= range->end_pc)
    	    l = i + 1;
	else if (pc < range->start_pc)
    	    r = i - 1;
	else
	{
	    mb = range->mb;
	    break;
	}
    }
    INVOKER_UNLOCK();
    return mb;
}


/* Native code expects a JavaFrame for the caller on the interpreter stack.
 * It is used to check access rights and to fill the stack trace in case of
 * an exception.
 * We also want to walk the stack (both compiled and interpreted) for security
 * checks. So we push a JavaFrame before calling into the interpreter from
 * compiled code in order to find the compiled frame from the interpreted frame.
 * stub_base points to the return address into the caller.
 */
static void __stdcall
PushJavaFrame(unsigned char *stub_base,
	      struct methodblock *caller_mb,
	      long stub_nargs,
	      ExecEnv *ee)
{
    int         nlocals = caller_mb->nlocals;
    JavaFrame   *old_frame = ee->current_frame;
    stack_item	*optop = old_frame->optop;
    JavaStack   *javastack = old_frame->javastack;
    JavaFrame   *frame = (JavaFrame *)(optop + nlocals);
    CodeInfo    *ci = (CodeInfo *)caller_mb->CompiledCodeInfo;
    unsigned char *caller_base;

    // optop points to the first argument passed by the owner of the last
    // pushed frame, i.e. the last interpreted method.
    // These arguments are consumed by the compiled code already.
    // The real arguments for the callee are on the compiled stack.

    sysAssert(caller_mb->fb.access & ACC_MACHINE_COMPILED &&
	      PC2CompiledMethod(*(unsigned char **)stub_base) == caller_mb);

    if (frame->ostack + caller_mb->maxstack > javastack->end_data)
    {
	if (javastack->stack_so_far >= *p_JavaStackSize)
	{
	    (*p_SignalError)(ee, JAVAPKG "StackOverflowError", 0);
	    ErrorUnwind(ee);	// do not try SignalErrorUnwind
	}
	javastack = javastack->next
	        ? javastack->next : (*p_CreateNewJavaStack)(ee, javastack);
	if (javastack == 0)
	{
	    (*p_SignalError)(ee, JAVAPKG "OutOfMemoryError", 0);
	    ErrorUnwind(ee);	// do not try SignalErrorUnwind
	}
	frame = (JavaFrame *)(javastack->data + nlocals);
	// no arguments	to copy
	optop = javastack->data;
    }
    frame->vars = optop;
    frame->javastack = javastack;
    frame->prev = old_frame;
    frame->optop = frame->ostack;
    frame->current_method = caller_mb;
    frame->constant_pool = 0;	// not used
    // so that the method is recognized by PCinCompiledCode:
    frame->lastpc = *(unsigned char **)stub_base;
    frame->monitor = 0;

    caller_base = stub_base + 4*stub_nargs + ci->localSize + ci->frameSize;
    frame->returnpc = caller_base;

#ifdef IN_NETSCAPE_NAVIGATOR
    frame->annotation = *((JHandle **)(caller_base - ci->frameSize));
#endif

    if (caller_mb->args_size) {
	(optop++)->p = *(unsigned char **)(caller_base + 4);	// caller's this, passed in memory
	nlocals--;
    }
    // clear locals to reduce false gc roots
    while (nlocals--) (optop++)->p = 0;

    ee->current_frame = frame;
}



#define UNWINDING	    2
#define UNWINDING_FOR_EXIT  4
#define UNWIND_IN_PROGRESS  (UNWINDING + UNWINDING_FOR_EXIT)

/* Our Java exception code */
#define EXCEPTION_JAVA	0xcafedead

/* Our Java exception registration record */
typedef struct _EXCEPTION_FRAME {
    struct  _EXCEPTION_FRAME *prev;
    PVOID   handler;
} EXCEPTION_FRAME, *PEXCEPTION_FRAME;

/* Compiled methods use __stdcall calling convention, this simplifies
 * the interfacing with native methods which use __cdecl.
 * The method block pointer is always passed in edx.

method call:
    push    last param
    ...
    mov     reg, this
    push    reg
    mov     edx, [reg]Hjava_lang_Object.methods
    mov     edx, [edx]methodtable.methods[slot]
    mov     ecx, caller_mb
    call    dword ptr [edx]methodblock.CompiledCode
    // edx is an implicite parameter

try:
    mov     dword ptr [esp + localSize + 4], new_xctxt
    ...
    mov     dword ptr [esp + localSize + 4], old_xctxt

catch:
    exception object is passed in eax

method prologue (with exception handling, or synchronized):
    push    ebx
    push    esi
    push    edi
    xor     eax, eax
    push    ebp
    push    offset HandleException
    push    dword ptr fs:[eax]
    mov     dword ptr fs:[eax], esp
    push    xctxt		// xctxt of first opcode
    push    edx 		// mb parameter
    sub     esp, -xframe2esp-8  // esp = bottom of operand stack (- 0, 4, or 8,
				// i.e. max function result size, may be trashed
				// by native call results)
epilogue:
    add     esp, -xframe2esp
    pop     dword ptr fs:[0]
    pop     ecx
    pop     ebp
    pop     edi
    pop     esi
    pop     ebx
    ret     4*nargs

method prologue (without exception handling, and not synchronized):
    push    callee-saved    // as used
    push    edx 	    // if mb used (never if jit compilation)
    sub     esp, localSize  // if non-zero

epilogue:
    add     esp, localSize  // (+4 if mb used) if non-zero
    pop     callee-saved    // as used
    ret     4*nargs
*/


extern VOID __stdcall
RtlUnwind(PEXCEPTION_FRAME ef, DWORD ra, PEXCEPTION_RECORD er, DWORD dw);


static JHandle * __stdcall
InstantiateExceptionObject(char *ename)
{
    ExecEnv	*ee = (*p_EE)();
    ClassClass	*cb;
    JHandle	*ret;

    if ((cb = (*p_FindClassFromClass)(ee, ename, TRUE, NULL)) == 0 &&
	(cb = (*p_FindClassFromClass)(ee, JAVAPKG "UnknownException", TRUE, NULL)) == 0)
	return (*p_exceptionInternalObject)(IEXC_NoClassDefinitionFound);

    if ((ret = (*p_ObjAlloc)(cb, 0)) == 0)
	return (*p_exceptionInternalObject)(IEXC_OutOfMemory);

    memset((char *)(unhand(ret)), 0, cbInstanceSize(cb));
    return ret;
}


/* This function translates a native exception code to a Java exception name.
 */
static JHandle * __stdcall
GetJavaExceptionObject(DWORD exceptionCode, PCONTEXT pcontext)
{
    struct methodblock *mb;
    unsigned char *ra, *base, *stub_base;
    long stub_nargs, espLevel;
    ExecEnv *ee;
    JavaFrame *frame;
    char *ename = JAVAPKG "UnknownException";
    char *message = 0;

    if ((unsigned)exceptionCode < EXCEPTION_ACCESS_VIOLATION)
    {
/*	switch	(exceptionCode)
	{
	case	EXCEPTION_DATATYPE_MISALIGNMENT:
	case	EXCEPTION_BREAKPOINT:
	case	EXCEPTION_SINGLE_STEP:
	case	EXCEPTION_GUARD_PAGE:
	    ename = JAVAPKG "UnknownException"; break;
	}
*/  }
    else if ((unsigned)exceptionCode < EXCEPTION_ARRAY_BOUNDS_EXCEEDED)
    {
	switch	(exceptionCode)
	{
	case	EXCEPTION_ACCESS_VIOLATION:
	    ename = JAVAPKG "NullPointerException"; break;

/*	case	EXCEPTION_IN_PAGE_ERROR:
	case	EXCEPTION_ILLEGAL_INSTRUCTION:
	case	EXCEPTION_NONCONTINUABLE_EXCEPTION:
	case	EXCEPTION_INVALID_DISPOSITION:
	    ename = JAVAPKG "UnknownException"; break;
*/	}
    }
    else if ((unsigned)exceptionCode < EXCEPTION_STACK_OVERFLOW)
    {
	switch	(exceptionCode)
	{
	case	EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
	    ename = JAVAPKG "ArrayIndexOutOfBoundsException"; break;

	// An EXCEPTION_FLT_xxxx occurs only if its corresponding bit in the
	// fpu control word is cleared. Will never occur for Java real or double
	// arithmetic, but may occur for Java long (64-bit) division and
	// remainder which are computed by the fpu.

	case	EXCEPTION_FLT_DIVIDE_BY_ZERO:
	case	EXCEPTION_FLT_INVALID_OPERATION:
	case	EXCEPTION_INT_DIVIDE_BY_ZERO:
	    message = "/ by zero";
	    // fall thru

	case	EXCEPTION_FLT_DENORMAL_OPERAND:
	case	EXCEPTION_FLT_INEXACT_RESULT:
	case	EXCEPTION_FLT_OVERFLOW:
	case	EXCEPTION_FLT_STACK_CHECK:
	case	EXCEPTION_FLT_UNDERFLOW:

	case	EXCEPTION_INT_OVERFLOW:
	    ename = JAVAPKG "ArithmeticException"; break;

/*	case	EXCEPTION_PRIV_INSTRUCTION:
	    ename = JAVAPKG "UnknownException"; break;
*/	}
    }
    else if (exceptionCode == EXCEPTION_STACK_OVERFLOW)
	ename = JAVAPKG "StackOverflowException";

/*  else if (exceptionCode == CONTROL_C_EXIT)
	ename = JAVAPKG "UnknownException";
*/
    /* We could simply call
     *
     * return InstantiateExceptionObject(ename);
     *
     * but we would not have a stack trace.
     * Instead, we try to call SignalError after pushing a JavaFrame.
     * We have to be careful only to push a frame if we are executing
     * in a compiled method.
     */
    mb = PC2CompiledMethod((unsigned char *)pcontext->Eip);
    if (mb)
    {
	base = (unsigned char *)pcontext->Esp - 4 +
	    ((CodeInfo *)mb->CompiledCodeInfo)->localSize +
	    ((CodeInfo *)mb->CompiledCodeInfo)->frameSize;
	espLevel = 0;
	while (espLevel > -64)
	{
	    ra = *(unsigned char **)(base - espLevel);
	    if (ra == StubRetAddr || PC2CompiledMethod(ra))
	    {
		ee = (*p_EE)();
		frame = ee->current_frame;
		// stub_base must point to a return address in mb:
		stub_base = (unsigned char *)&pcontext->Eip;
		// stub_nargs is the number of dwords between stub_base and esp - 4:
		stub_nargs = ((unsigned char *)pcontext->Esp - espLevel - stub_base)/4 - 1;
		PushJavaFrame(stub_base, mb, stub_nargs, ee);
		(*p_SignalError)(ee, ename, message);
		ee->current_frame = frame;	// pop frame before handling native exception
		return ee->exception.exc;	// don't need to clear exception, done in handler
	    }
	    espLevel -= 4;
	}
    }
    return InstantiateExceptionObject(ename);
}


/* The following code is derived from, and modeled after ProcedureFindThrowTag() 
   in runtime/interpreter.c.  Common code should be factored out, so that the near
   cut/paste status can be discontinued (and bugs fixed in one place) */
static void * __stdcall
GetJavaExceptionHandler(long pcTarget, struct methodblock *mb, JHandle *object)
{
    /* Search current class for catch frame, based on pc value,
       matching specified type. */
    ClassClass *methodClass = fieldclass(&mb->fb);
    ClassClass *objectClass = obj_array_classblock(object);
    struct CatchFrame *cf = mb->exception_table;
    struct CatchFrame *cfEnd = cf + mb->exception_table_length;
    cp_item_type *constant_pool = methodClass->constantpool;
    for (; cf < cfEnd; cf++)
    {
	if (cf->start_pc <= pcTarget && pcTarget < cf->end_pc)
	{
	    /* Is the class name a superclass of object's class. */
	    if (cf->catchType == 0)
		/* Special hack put in for finally.  Always succeed. */
		goto found;
	    else
	    {
		/* The usual case */
		int catchType = cf->catchType;
		char *catchName =
		    (*p_GetClassConstantClassName)(constant_pool, catchType);
		ClassClass *catchClass = NULL;
		ClassClass *cb;
		for (cb = objectClass;; cb = unhand(cbSuperclass(cb)))
		{
		    if (strcmp(classname(cb), catchName) == 0)
		    {
			if (cbLoader(cb) == cbLoader(methodClass))
			    goto found;
			if (catchClass == NULL)
			{
			    /* I haven't yet found the actual
			     * class corresponding to catchType, but it's
			     * got the right name.  Find the actual class */
			    if (!(*p_ResolveClassConstantFromClass)(
				   methodClass, catchType, (*p_EE)(),
				   1 << CONSTANT_Class))
				return 0;
			    catchClass =
				(ClassClass *)(constant_pool[catchType].p);
			}
			if (cb == catchClass)
			    goto found;
		    }
		    if (cbSuperclass(cb) == 0)
			/* Not found.  Try next catch frame. */
			break;
		}
	    }
	}
    }
    /* not found in this execution environment */
    return 0;
found:
    return (char *)mb->CompiledCode + (long)cf->compiled_CatchFrame;
    // compiled_CatchFrame is actually the relative
    // address of the compiled handler
}


void __stdcall
ErrorUnwind(struct execenv *ee)
{
    /*	This function does not unwind frames as the solaris version does.
	It just raises a native exception. Exception handlers will do unwinding
	and code cleanup.
	However, when the exception drops on the floor, a stack trace is available
	if ErrorUnwind was called by SignalErrorUnwind.
    */
    if (!ee)
    	ee = (*p_EE)();

    if (ee->exceptionKind == EXCKIND_STKOVRFLW)
	//  We hope we have now enough stack space to raise the exception.
	//  If not, the OS will do it for us.
	ee->exception.exc = InstantiateExceptionObject(JAVAPKG "StackOverflowException");

    RaiseException(EXCEPTION_JAVA, EXCEPTION_NONCONTINUABLE, 1, &((DWORD)ee->exception.exc));
}


void __stdcall
SignalErrorUnwind(struct execenv *ee, char *ename, char *DetailMessage,
    unsigned char *stub_base, long stub_nargs)
{
    JavaFrame *frame;
    struct methodblock *caller_mb;

    if (!ee)
    	ee = (*p_EE)();

    caller_mb = PC2CompiledMethod(*(unsigned char **)stub_base);
    sysAssert(caller_mb != 0);
    frame = ee->current_frame;
    PushJavaFrame(stub_base, caller_mb, stub_nargs, ee);
    (*p_SignalError)(ee, ename, DetailMessage);
    ee->current_frame = frame;	// pop frame before raising native exception
    ErrorUnwind(ee);
}


static void __stdcall
CompiledCodeCleanup(struct methodblock *mb, HObject *o)
{
    if (mb->fb.access & ACC_SYNCHRONIZED)
	if (mb->fb.access & ACC_STATIC)
	    (*p_monitorExit)(obj_monitor(cbHandle(fieldclass(&mb->fb))));
	else
	    (*p_monitorExit)(obj_monitor(o));
}


typedef struct methodblock methodblock;     /* for use in in-line assembly */
typedef struct fieldblock  fieldblock;      /* for use in in-line assembly */

__declspec(naked) int
HandleException(void)
//  [esp+ 4] PEXCEPTION_RECORD
//  [esp+ 8] PEXCEPTION_FRAME
//  [esp+12] PCONTEXT
//  [esp+16] PVOID
{
    __asm   {
	mov	eax, [esp+4]
	test	dword ptr [eax]EXCEPTION_RECORD.ExceptionFlags, UNWIND_IN_PROGRESS
	jne	Unwinding

	mov	ecx, [eax]EXCEPTION_RECORD.ExceptionCode
	cmp	ecx, EXCEPTION_JAVA
	jne	HandleNonJavaException

	mov	eax, [eax]EXCEPTION_RECORD.ExceptionInformation[0]  // exception object
	jmp	FindHandler

HandleNonJavaException:
	mov	eax, [esp+12]
	push	eax
	push	ecx
	call	GetJavaExceptionObject

FindHandler:
	mov	edx, [esp+8]
	push	eax		    // save exception object

	mov	ecx, [edx-8]	    // mb
	mov	edx, [edx-4]	    // xctxt
	cmp	edx, 0
	jl	NoHandler

	push	eax		    // object
	push	ecx		    // mb
	push	edx		    // xctxt
	call	GetJavaExceptionHandler
	cmp	eax, 0
	jne	Unwind		    // handler found, call RtlUnwind

NoHandler:
	mov	eax, [esp+4+4]
	mov	ecx, [eax]EXCEPTION_RECORD.ExceptionCode
	cmp	ecx, EXCEPTION_JAVA
	jne	ReRaise 	    // to avoid same conversion again
	pop	ecx		    // discard exception object
	jmp	Return1

ReRaise:
	push	esp		    // address of exception object
	push	1
	push	EXCEPTION_NONCONTINUABLE
	push	EXCEPTION_JAVA
	push	[eax]EXCEPTION_RECORD.ExceptionAddress	// user address
	jmp	dword ptr [RaiseException]

Unwind:
	push	eax		    // handler address

	finit	// 0x137F, round to nearest, extended precision, no exceptions
	fwait
	push	0x127F	// round to nearest, double precision, no exceptions
	fldcw	[esp]
	pop	ecx

	mov	eax, [esp+4+8]
	mov	edx, [esp+8+8]
	or	dword ptr [eax]EXCEPTION_RECORD.ExceptionFlags, UNWINDING

	push	0
	push	eax
	push	offset	ReturnInHandleException
	push	edx
	call	RtlUnwind

ReturnInHandleException:
	// clear exception
	call	dword ptr [p_EE]
	mov	byte ptr [eax]ExecEnv.exceptionKind, EXCKIND_NONE
	pop	ecx		// handler address
	pop	eax		// exception object

	mov	edx, [esp+8]	// edx points to the exception frame
	mov	ebp, [edx-8]	// ebp = mb
	mov	ebp, [ebp]methodblock.CompiledCodeInfo
	mov	ebp, [ebp]CodeInfo.xframe2esp
	add	ebp, edx	// ebp += exception frame address
	mov	esp, ebp	// ebp is used as a scratch register here

	jmp	ecx	// eax contains the exception object

Unwinding:
	mov	edx, [esp+8]
	mov	eax, [edx-4]
	cmp	eax, NO_CLEANUP	// if exception signaled in CompSupport_monitorexit
	je	Return1		// no cleanup

	mov	eax, [edx+28]	// this if virtual, else not used
	mov	ecx, [edx-8]	// mb

	push	eax
	push	ecx
	call	CompiledCodeCleanup

Return1:
	mov	eax, 1
	ret
    }
}


/* ReportException catches every exception, performs a global unwind,
 * translates a native exception to a Java exception (if necessary), reports
 * it into the current execenv, puts the fpu into a consistent state, pushes a
 * dummy result onto the fpu stack if necessary, restores registers, and returns
 * from the __stdcall stub.
 */
__declspec(naked) static int
ReportException(void)
//  [esp+ 4] PEXCEPTION_RECORD
//  [esp+ 8] PSTUB_EXCEPTION_FRAME
//  [esp+12] PCONTEXT
//  [esp+16] PVOID
{
    __asm   {
	mov	eax, [esp+4]
	test	dword ptr [eax]EXCEPTION_RECORD.ExceptionFlags, UNWIND_IN_PROGRESS
	jne	GiveUp	/* Something really bad must have happened:
			 * we should never see unwinding here, since we
			 * catch everything.
			 */

	mov	ecx, [eax]EXCEPTION_RECORD.ExceptionCode
	cmp	ecx, EXCEPTION_JAVA
	jne	HandleNonJavaException

	mov	eax, [eax]EXCEPTION_RECORD.ExceptionInformation[0]  // exception object
	jmp	Unwind

HandleNonJavaException:
	mov	eax, [esp+12]
	push	eax
	push	ecx
	call	GetJavaExceptionObject

Unwind:
	push	eax	// save exception object

	finit	// 0x137F, round to nearest, extended precision, no exceptions
	fwait
	push	0x127F	// round to nearest, double precision, no exceptions
	fldcw	[esp]
	pop	ecx

	mov	eax, [esp+4+4]
	mov	edx, [esp+8+4]
	or	dword ptr [eax]EXCEPTION_RECORD.ExceptionFlags, UNWINDING

	push	0
	push	eax
	push	offset	ReturnInReportException
	push	edx
	call	RtlUnwind

ReturnInReportException:
	// If the method returns a float (flag == 2) or a double (flag == 3),
	// push 0 onto the fpu stack.
	// This is necessary only if the invalid op exception is enabled,
	// but we do it anyway.
	mov	eax, [esp+8+4]	    // eax points to the exception frame
	mov	eax, [eax-4]	    // mb (no xframe2esp, nor xctxt in stub)
	mov	eax, [eax]methodblock.CompiledCodeFlags
	and	eax, 7
	sub	eax, 2
	cmp	eax, 1
	ja	SetExecEnv
	fldz			    // the fpu stack needs to hold a result

SetExecEnv:
	call	dword ptr [p_EE]
	mov	byte ptr [eax]ExecEnv.exceptionKind, EXCKIND_THROW
	pop	ecx
	mov	[eax]ExecEnv.exception.exc, ecx

	mov	esp, [esp+8]	    // esp = exception frame address

	// same epilogue as in InvokeCompiledMethodStub:
	pop	dword ptr fs:[0]    // unlink exception frame
	pop	ecx		    // discard handler address
	pop	ebp
	pop	edi
	pop	esi
	pop	ebx
	ret	12

GiveUp:
	mov	eax, 1
	ret
    }
}


/* InvokeCompiledMethodStub(mb, optop, args_size) is used to
 * trampoline from interpreted code to compiled code.  It is called using
 * in-line assembly, because the return value type is variable.
 * This function also catches any exception raised in the invoked method and
 * reports it into the current execenv before returning.
 */
__declspec(naked) static void __stdcall
InvokeCompiledMethodStub(struct methodblock *mb, stack_item *optop,
    int argument_count)
{
// -> [esp+4 ]	struct methodblock *mb
// -> [esp+8 ]	stack_item *optop
// -> [esp+12]	int argument_count
    __asm   {
	push	ebx	// Callee-saved registers have to be saved
	push	esi	// because they need to be restored in case
	push	edi	// of an exception.
	push	ebp
	xor	eax, eax
	mov	edx, [esp+16+4 ]    			// method block
	mov	ebx, [edx]methodblock.CompiledCode	// code address
	mov	ecx, [esp+16+12]			// argument count
	push	offset ReportException
	push	dword ptr fs:[eax]
	mov	dword ptr fs:[eax], esp
	push	edx		    // mb for ReportException
	sub	ecx, 1
	jl	CallMethod	    // no arguments
	mov	esi, [esp+28+8]	    // pointer to arguments

CopyArgs:
	mov	eax, [esi+ecx*4]
	sub	ecx, 1
	push	eax
	jge	CopyArgs

CallMethod:
	mov	StubRetAddr, offset ReturnInStub
	call	ebx		    // call the __stdcall method, mb is in edx
ReturnInStub:			    // result may be in eax (and edx), or st(0)
	pop	ecx		    // discard mb
	pop	dword ptr fs:[0]    // unlink exception frame
	pop	ecx		    // discard handler address
	pop	ebp
	pop	edi
	pop	esi
	pop	ebx
	ret	12
    }
}


/* These functions are used to trampoline from compiled code to interpreted
 * code.  They are stored in the CompiledCode slot of an interpreted method.
 */
__declspec(naked) static void __stdcall
CallbackInterpretedMethod()
{
// -> edx	    callee mb
// -> ecx	    caller mb
// -> ebx	    args
// -> ebp	    isStaticCall
// -> [esp+20+4*i]  arg(i), i = 0, 1, ...
// <- reg(s)	    method result, if any
    __asm   {
	mov	edi, edx	// save callee mb
	mov	esi, ecx	// save caller mb

	call	dword ptr [p_EE]

	lea	edx, [esp+16]	// stub base
	push	eax		// ee
        movzx	ecx, word ptr [edi]methodblock.args_size
	push	ecx		// callee->args_size
	push	esi		// caller
	push	edx		// stub base
	mov	esi, eax	// save ee
	call	PushJavaFrame

	push	eax		// place holder for hiBits
	push	eax		// place holder for lowBits
	mov	edx, esp	// &lowBits

	// call the interpreted method:
	//  do_execute_java_method_vararg(ExecEnv *ee, void *obj,
	//	  char *method_name, char *method_signature,
	//	  struct methodblock *mb,
	//	  bool_t isStaticCall, va_list args,
	//	  long *otherBits, bool_t shortFloats)

	push	1		// shortFloats param == true
	push	edx		// otherBits
	push	ebx		// args
	push	ebp		// isStaticCall
	push	edi		// mb
	mov	edx, [edx+20+8] // obj = arg0, not used if isStaticCall
	push	0		// method_signature (not needed)
	push	0		// method_name (not needed)
	push	edx		// obj
	push	esi		// ee

	call	dword ptr [p_do_execute_java_method_vararg]
	add	esp, 36

	// pop JavaFrame
	mov	edx, [esi]ExecEnv.current_frame
	mov	ecx, [edx]JavaFrame.prev
	mov	[esi]ExecEnv.current_frame, ecx

	// if there is an error reported in the ee, then raise an exception
	cmp	byte ptr [esi]ExecEnv.exceptionKind, 0
	jne	CallErrorUnwind     // indirect for breakpoint setting

	mov	ecx, [edi]methodblock.CompiledCodeFlags
	// 32-bit result is returned in eax
	// 64-bit result is returned in eax (hiBits) and [esp] (lowBits)
	// move result into result reg(s) according to ecx:
	//  0 for returning an int
	//  1 for returning a long
	//  2 for returning a float
	//  3 for returning a double
	//  4 for returning a void
	and	ecx, 3		    // test 2 bits, only, handle void like int
	cmp	ecx, 1
	jl	Return		    // int in eax or void
	jg	Float_Double
	mov	edx, eax	    // hiBits were returned in eax
	pop	eax		    // lowBits at [esp]
	pop	ecx
	// this is tricky, we need to do a __stdcall return, but the number
	// of args is variable, eax and edx are live, only ecx is available,
	// so we move the return address
	movzx	ecx, word ptr [edi]methodblock.args_size
	mov	esi, [esp+16]	    // read return address
	mov	[esp+ecx*4+16], esi // write return address
	pop     ebp
	pop     edi
	pop     esi
	pop     ebx
	lea	esp, [esp+ecx*4]
	ret

Float_Double:
	cmp	ecx, 3
	je	Double
	mov	[esp], eax	    // lowBits
	fld	dword ptr [esp]
	jmp	Return

Double:
	mov	[esp+4], eax	    // hiBits
	fld	qword ptr [esp]

Return:
	pop	ecx		    // discard lowBits
	pop	ecx		    // discard hiBits
	movzx	ecx, word ptr [edi]methodblock.args_size
	pop     ebp
	pop     edi
	pop     esi
	pop     ebx
	pop	edx		    // return address
	lea	esp, [esp+ecx*4]
	jmp	edx

CallErrorUnwind:
	push	esi
	call	ErrorUnwind
    }
}


__declspec(naked) static void __stdcall
CallbackDynamicMethod()
{
// -> edx	    callee mb
// -> ecx	    caller mb
// -> [esp+4+4*i]   arg(i), i = 0, 1, ...
// <- reg(s)	    method result, if any
    __asm   {
	push    ebx
	push    esi
	push    edi
	push    ebp
	lea	ebx, [esp+24]	// &arg1
	mov	ebp, 0		// isStaticCall = false
	jmp	CallbackInterpretedMethod
    }
}


__declspec(naked) static void __stdcall
CallbackStaticMethod()
{
// -> edx	    callee mb
// -> ecx	    caller mb
// -> [esp+4+4*i]   arg(i), i = 0, 1, ...
// <- reg(s)	    method result, if any
    __asm   {
	push    ebx
	push    esi
	push    edi
	push    ebp
	lea	ebx, [esp+20]	// &arg0
	mov	ebp, 1		// isStaticCall = true
	jmp	CallbackInterpretedMethod
    }
}


/* Returns the address of the native code stub after loading it if necessary.
 */
static void * __stdcall
GetStubAddr(struct methodblock *mb)
{
    if (mb->code == 0)
    {
	(*p_dynoLink)(mb);
	if (mb->code == 0)
	{
            ExecEnv *ee = (*p_EE)();
    	    // JavaFrame is already pushed for correct stack trace
	    (*p_SignalError)(ee, JAVAPKG "UnsatisfiedLinkError", fieldname(&mb->fb));
	    // pop frame before raising native exception
            ee->current_frame = ee->current_frame->prev;
	    ErrorUnwind(ee);
	}
    }
    return mb->code;
}


__declspec(naked) static void __stdcall
exitNativeInvoker()
{
// -> ecx	    returned top of operand stack
// <- reg(s)	    method result, if any
    __asm   {
	// pop JavaFrame
	mov	edx, [esi]ExecEnv.current_frame
	mov	eax, [edx]JavaFrame.prev
	mov	[esi]ExecEnv.current_frame, eax

	// if there is an error reported in the ee, then raise an exception
	cmp	byte ptr [esi]ExecEnv.exceptionKind, 0
	jne	CallErrorUnwind     // indirect for breakpoint setting

	mov	esi, [edi]methodblock.CompiledCodeFlags
	// read result from [ecx] into result reg(s) according to esi:
	//  0 for returning an int
	//  1 for returning a long
	//  2 for returning a float
	//  3 for returning a double
	//  4 for returning a void
	and	esi, 7
	cmp	esi, 1
	jg	Float_Double_Void
	je	Long_Return
	mov	eax, [ecx-4]
	jmp	Return

Float_Double_Void:
	cmp	esi, 3
	jg	Return
	je	Double
	fld	dword ptr [ecx-4]
	jmp	Return

Double:
	fld	qword ptr [ecx-8]

Return:
	movzx	ecx, word ptr [edi]methodblock.args_size
	pop	edi
	pop	esi
	pop	edx		    // return address
	lea	esp, [esp+ecx*4]
	jmp	edx

Long_Return:
	mov	eax, [ecx-8]
	mov	edx, [ecx-4]
	// this is tricky, we need to do a __stdcall return, but the number
	// of args is variable, eax and edx are live, only ecx is available,
	// so we move the return address
	movzx	ecx, word ptr [edi]methodblock.args_size
	mov	esi, [esp+8]	    // read return address
	mov	[esp+ecx*4+8], esi  // write return address
	pop	edi
	pop	esi
	lea	esp, [esp+ecx*4]
	ret

CallErrorUnwind:
	push	esi
	call	ErrorUnwind
    }
}


__declspec(naked) static void __stdcall
InvokeNativeMethod()
{
// -> edx	    callee mb
// -> ecx	    caller mb
// -> [esp+4+4*i]   arg(i), i = 0, 1, ...
// <- reg(s)	    method result, if any
    __asm   {
	push	esi
	push	edi
	mov	edi, edx	// save callee mb
	mov	esi, ecx	// save caller mb

	call	dword ptr [p_EE]

	lea	edx, [esp+8]	// stub base
	push	eax		// ee
        movzx	ecx, word ptr [edi]methodblock.args_size
	push	ecx		// callee->args_size
	push	esi		// caller
	push	edx		// stub base
	mov	esi, eax	// save ee
        call	PushJavaFrame

	push	edi		// callee
	call	GetStubAddr

	// call the native method __cdecl stub(stack_item *p, struct execenv *ee)
	lea	ecx, [esp+8+4]
	push	esi		// ee
	push	ecx		// pointer to the arguments
	call	eax		// call native stub
	add	esp, 8
	mov	ecx, eax	// save returned top of operand stack

	jmp	exitNativeInvoker
    }
}


__declspec(naked) static void __stdcall
InvokeSynchronizedNativeMethod()
{
// -> edi	    callee mb
// -> esi	    caller mb
// -> ebp	    monitor, i.e. this or class handle
// -> [esp+20+4*i]  arg(i), i = 0, 1, ...
// <- reg(s)	    method result, if any
    __asm   {
	push	ebp
	call	dword ptr [p_monitorEnter]	// __cdecl
	pop	ecx

	call	dword ptr [p_EE]

	lea	edx, [esp+16]	// stub base
	push	eax		// ee
        movzx	ecx, word ptr [edi]methodblock.args_size
	push	ecx		// callee->args_size
	push	esi		// caller
	push	edx		// stub base
	mov	esi, eax	// save ee
        call	PushJavaFrame

	push	edi		// callee
	call	GetStubAddr

	// call the native method __cdecl stub(stack_item *p, struct execenv *ee)
	lea	ecx, [esp+16+4]
	push	esi		// ee
	push	ecx		// pointer to the arguments
	call	eax		// call native stub
	add	esp, 8
	mov	ebx, eax	// save returned top of operand stack
	
	push	ebp
	call	dword ptr [p_monitorExit]	// __cdecl
	pop	ecx

	mov	ecx, ebx
	pop	ebp
	pop	ebx
	jmp	exitNativeInvoker
    }
}


__declspec(naked) static void __stdcall
InvokeSynchronizedNativeDynamicMethod()
{
// -> edx	    callee mb
// -> ecx	    caller mb
// -> [esp+4+4*i]   arg(i), i = 0, 1, ...
// <- reg(s)	    method result, if any
    __asm   {
	push	esi
	push	edi
	push	ebx
	push	ebp
	mov	edi, edx	// save callee mb
	mov	esi, ecx	// save caller mb

	mov	ebp, [esp+16+4]	// the receiver to be synchronized
	jmp	InvokeSynchronizedNativeMethod
    }
}


__declspec(naked) static void __stdcall
InvokeSynchronizedNativeStaticMethod()
{
// -> edx	    callee mb
// -> ecx	    caller mb
// -> [esp+4+4*i]   arg(i), i = 0, 1, ...
// <- reg(s)	    method result, if any
    __asm   {
	push	esi
	push	edi
	push	ebx
	push	ebp
	mov	edi, edx	// save callee mb
	mov	esi, ecx	// save caller mb

	mov	edx, [edx]fieldblock.clazz	// first field in methodblock is fieldblock
	mov	ebp, [edx]ClassClass.HandleToSelf	// the handle to be synchronized
	jmp	InvokeSynchronizedNativeMethod
    }
}


bool_t
invokeCompiledMethod(JHandle * o, struct methodblock * mb,
	     int args_size, struct execenv * ee)
{
    // object o not used
    JavaFrame *frame = ee->current_frame;
    stack_item *optop = frame->optop;
    // Java8 t;

    switch((mb->CompiledCodeFlags >> 24) & 0xFF)
    {
    default:
	// optop->i = InvokeCompiledMethodInt(mb, optop, args_size);
	__asm	{
	    push    args_size
	    push    optop
	    push    mb
	    call    InvokeCompiledMethodStub
	    mov     ecx, optop
	    mov     [ecx], eax
	}
	sysAssert(frame->optop == optop);
	frame->optop = optop + 1;
	break;
    case SIGNATURE_LONG:
	// SET_INT64(t, optop,
	//	InvokeCompiledMethodLong(mb, optop, args_size));
	__asm	{
	    push    args_size
	    push    optop
	    push    mb
	    call    InvokeCompiledMethodStub
	    mov     ecx, optop
	    mov     [ecx], eax
	    mov     [ecx+4], edx
	}
	sysAssert(frame->optop == optop);
	frame->optop = optop + 2;
	break;
    case SIGNATURE_FLOAT:
	// optop->f = InvokeCompiledMethodFloat(mb, optop, args_size);
	__asm	{
	    push    args_size
	    push    optop
	    push    mb
	    call    InvokeCompiledMethodStub
	    mov     ecx, optop
	    fstp    dword ptr [ecx]
	}
	sysAssert(frame->optop == optop);
	frame->optop = optop + 1;
	break;
    case SIGNATURE_DOUBLE:
	// SET_DOUBLE(t, optop,
	//	InvokeCompiledMethodDouble(mb, optop, args_size));
	__asm	{
	    push    args_size
	    push    optop
	    push    mb
	    call    InvokeCompiledMethodStub
	    mov     ecx, optop
	    fstp    qword ptr [ecx]
	}
	sysAssert(frame->optop == optop);
	frame->optop = optop + 2;
	break;
    case SIGNATURE_VOID:
	InvokeCompiledMethodStub(mb, optop, args_size);
	sysAssert(frame->optop == optop);
	break;
    }
    return !exceptionOccurred(ee);
}

/* Perform runtime patches on the compiled code. */
static bool_t PatchCompiledCode(struct methodblock *mb, unsigned char *code,
				unsigned char *patch, int patchSize)
{
    // precompiled method not supported
    return  0;
}


bool_t
invokeCompiler(JHandle *o, struct methodblock *mb, int args_size, ExecEnv *ee);


/* Modify the V-table for mb, so that mb is compiled and executed natively
 * the next time it is invoked.
 */
static bool_t
compileAndGoOnInvoke(struct methodblock *mb)
{
    unsigned int access;
    bool_t res;

    INVOKER_LOCK();
    access = mb->fb.access;
    if ((access & ACC_MACHINE_COMPILED) && mb->code == 0)
    {
	res = FALSE;
	// method was precompiled and no byte code was loaded:
	// mb->invoker initialized to invokeCompiledMethod in
	// InitializeInvoker (classruntime.c)
	// mb->CompiledCode initialized in
	// ReadInCompiledCode (classloader.c)
	// or the byte code was discarded after a compilation:
	// mb->invoker and mb->CompiledCode initialized in goOnInvoke
    }
    else if ((access & (ACC_ABSTRACT | ACC_NATIVE)) == 0)
    {
	sysAssert(mb->code);
	res = TRUE;
	mb->invoker = invokeCompiler;
	if  ((access & ACC_MACHINE_COMPILED) && mb->CompiledCode)
	{
/* we cannot free code here, may still be in use,
 * should be garbage collected, how???
	    (*p_free)(mb->CompiledCode);    // discard old compiled code
            (*p_free)(mb->CompiledCodeInfo);
*/
	    mb->CompiledCode = 0;
            mb->CompiledCodeInfo = 0;
	}
	mb->CompiledCode = (access & ACC_STATIC) ?
	    &CallbackStaticMethod : &CallbackDynamicMethod;

	mb->fb.access &= ~ACC_MACHINE_COMPILED;
    }
    else if (access & ACC_ABSTRACT)
    {
	res = FALSE;
	// mb->invoker initialized to invokeAbstractMethod in
	// InitializeInvoker (classruntime.c)
	mb->CompiledCode = (access & ACC_STATIC) ?
	    &CallbackStaticMethod : &CallbackDynamicMethod;
    }
    else
    {
	res = FALSE;
	// mb->invoker initialized to invokeLazyNativeMethod in
	// InitializeInvoker (classruntime.c)
	mb->CompiledCode = (access & ACC_SYNCHRONIZED) ?
            ( (access & ACC_STATIC) ?
		&InvokeSynchronizedNativeStaticMethod :
		&InvokeSynchronizedNativeDynamicMethod )
	    : &InvokeNativeMethod;
    }
    INVOKER_UNLOCK();
    return res;
}


/* Insert the compiled pc range in an array sorted in increasing order for
 * fast pc-to-mb mapping.
 */
static char *
insertCodeRange(struct methodblock *mb, CodeInfo *codeInfo)
{
    CodeRange *new_code_ranges, *range;
    unsigned char *start_pc = codeInfo->start_pc;
    unsigned char *end_pc = codeInfo->end_pc;
    int len, i, l, r;

    if (ncode_ranges == max_ncode_ranges)
    {
	new_code_ranges = (*p_realloc)(code_ranges, 2*max_ncode_ranges*sizeof(CodeRange));
	if (new_code_ranges == 0)
	    return "Not enough memory";

	max_ncode_ranges *= 2;
        code_ranges = new_code_ranges;
    }
    // optimize the cases where blocks are allocated in sorted order (incr or decr)
    if (ncode_ranges == 0 || end_pc < code_ranges->start_pc)
    {
	range = code_ranges;
	len = ncode_ranges;
    }
    else if (start_pc >= code_ranges[ncode_ranges-1].end_pc)
    {
	range = code_ranges + ncode_ranges;
    	len = 0;
    }
    else
    {
        // binary search
	l = 0;
	r = ncode_ranges - 1;
	while (l <= r)
	{
	    i = (l + r)/2;
	    range = code_ranges + i;
	    if (start_pc >= range->end_pc)
		l = i + 1;
	    else if (end_pc < range->start_pc)
		r = i - 1;
	    else
    		sysAssert(!"Range overlap");
	}
        sysAssert(r == l - 1);
	range = code_ranges + l;
	len = ncode_ranges - l;
	sysAssert(start_pc < range->start_pc && start_pc >= range[-1].end_pc);
    }
    if (len)
	memmove(range + 1, range, len*sizeof(CodeRange));	// overlapping copy
    range->start_pc = start_pc;
    range->end_pc = end_pc;
    range->mb = mb;
    ncode_ranges++;
    return 0;
}


static void
deleteCodeRange(CodeInfo *codeInfo)
{
    CodeRange *range;
    unsigned char *start_pc = codeInfo->start_pc;
    unsigned char *end_pc = codeInfo->end_pc;
    int len, i, l, r;

    sysAssert(ncode_ranges != 0);
    // binary search
    l = 0;
    r = ncode_ranges - 1;
    while (l <= r)
    {
	i = (l + r)/2;
	range = code_ranges + i;
	if (start_pc >= range->end_pc)
	    l = i + 1;
	else if (end_pc < range->start_pc)
	    r = i - 1;
	else
	    break;
    }
    sysAssert(range->start_pc == start_pc && range->end_pc == end_pc);
    len = code_ranges + ncode_ranges - range - 1;
    if (len)
	memmove(range, range + 1, len*sizeof(CodeRange));       // overlapping copy
    ncode_ranges--;
}


static char *
goOnInvoke(struct methodblock *mb, char *compiledCode, CodeInfo *codeInfo)
{
    char *err = 0;

    INVOKER_LOCK();
    err = insertCodeRange(mb, codeInfo);
    if (!err)
    {
	mb->invoker = invokeCompiledMethod;
	mb->CompiledCode = compiledCode;
	mb->CompiledCodeInfo = codeInfo;
	mb->fb.access |= ACC_MACHINE_COMPILED;
    }
    INVOKER_UNLOCK();
    return err;
}


/* Modify the V-table for mb, so that mb is interpreted
 * the next time it is invoked.
 */
static bool_t
interpretOnInvoke(struct methodblock *mb, bool_t freeCompiledCode)
{
    unsigned int access;
    bool_t res;

    INVOKER_LOCK();
    access = mb->fb.access;
    if ((access & ACC_MACHINE_COMPILED) && mb->code == 0)
    {
	res = FALSE;
	// method was precompiled and no byte code was loaded:
	// mb->invoker initialized to invokeCompiledMethod in
	// InitializeInvoker (classruntime.c)
	// mb->CompiledCode initialized in
	// ReadInCompiledCode (classloader.c)
	// or the byte code was discarded after a compilation:
	// mb->invoker and mb->CompiledCode initialized in goOnInvoke
    }
    else if ((access & (ACC_ABSTRACT | ACC_NATIVE)) == 0)
    {
/*
	if  ((access & ACC_MACHINE_COMPILED) && mb->exception_table_length)
	    // handler_pc was modified, cannot return to interpreted code
	    // before a new field compiled_CatchFrame is introduced and used
	    res = FALSE;
	    // mb->invoker and mb->CompiledCode initialized in goOnInvoke,
	    // cannot be changed

	else
*/	{
	    res = TRUE;
	    mb->invoker = (access & ACC_SYNCHRONIZED) ?
		p_invokeSynchronizedJavaMethod : p_invokeJavaMethod;

	    if (freeCompiledCode && (access & ACC_MACHINE_COMPILED))
	    {
/* we cannot free code here, may still be in use,
 * should be garbage collected, how???
		(*p_free)(mb->CompiledCode);	// discard old compiled code
		(*p_free)(mb->CompiledCodeInfo);
*/
		mb->CompiledCode = 0;
		mb->CompiledCodeInfo = 0;
	    }

	    mb->CompiledCode = (access & ACC_STATIC) ?
		&CallbackStaticMethod : &CallbackDynamicMethod;

	    mb->fb.access &= ~ACC_MACHINE_COMPILED;
	}
    }
    else if (access & ACC_ABSTRACT)
    {
	res = FALSE;
	// mb->invoker initialized to invokeAbstractMethod in
	// InitializeInvoker (classruntime.c)
	mb->CompiledCode = (access & ACC_STATIC) ?
	    &CallbackStaticMethod : &CallbackDynamicMethod;
    }
    else
    {
	res = FALSE;
	// mb->invoker initialized to invokeLazyNativeMethod in
	// InitializeInvoker (classruntime.c)
	mb->CompiledCode = (access & ACC_SYNCHRONIZED) ?
            ( (access & ACC_STATIC) ?
		&InvokeSynchronizedNativeStaticMethod :
		&InvokeSynchronizedNativeDynamicMethod )
	    : &InvokeNativeMethod;
    }
    INVOKER_UNLOCK();
    return res;
}


bool_t __cdecl
invokeCompiler(JHandle *o, struct methodblock *mb, int args_size, ExecEnv *ee)
{
    char *compiledCode;
    CodeInfo *codeInfo;
    char *err;
    bool_t res, freeByteCode;

    classJavaLangObject = *p_classJavaLangObject;
    interfaceJavaLangCloneable = *p_interfaceJavaLangCloneable;
    ObjectMethodTable = cbMethodTable(classJavaLangObject);
    /* args were popped from the operand stack by the interpreter.
     * repush them during compilation, otherwise they could be destroyed
     * by a call to do_execute_java_method_vararg (static initializers are
     * invoked during constant pool resolution).
     */
    ee->current_frame->optop += args_size;
    res = jitCompile(mb, &compiledCode, &codeInfo, &err, ee);
    ee->current_frame->optop -= args_size;
    if (res)
    {
#ifdef	BINARY_DEBUG
	static unsigned div = 1;
	static unsigned rem = 1;    // div == 1 -> go == rem
	static unsigned always = 0xffffffff;
	static unsigned from = 0;   // go if in range [from..to[
	static unsigned to = 0xffffffff;

	if  (!(count >= from && count < to) || (count % div == rem && count != always))
	{
	    count++;
	    interpretOnInvoke(mb, FALSE);
	}
	else
	    count++,
#endif
	    (err = goOnInvoke(mb, compiledCode, codeInfo)) ?
		interpretOnInvoke(mb, FALSE) : (void)0;
    }
    else
	// could not compile, do not try again at next invocation
	interpretOnInvoke(mb, FALSE);

#ifdef	DEBUG
    if (err /*&& !strcmp(err, "opc_invokevirtual_quick")*/)
    cprintf(stderr, "\nJIT compiler: %s,\n        Method %s%s of class %s\n",
	    err, mb->fb.name, mb->fb.signature, mb->fb.clazz->name);
#endif
    if (p_notifyCompilation)
    {
	freeByteCode = FALSE;
	(*p_notifyCompilation)(mb, err, &freeByteCode);
	// control panel may call interpretOnInvoke at this point
    }
    else
#ifdef KEEP_BYTECODE
	freeByteCode = FALSE;
#else
	freeByteCode = TRUE;	// danger! only works if first invocation
				// code may still be in use otherwise
#endif
    if (!err && freeByteCode)
    {
	mb->code_length = 0;
	(*p_free)(mb->code);
	mb->code = 0;
    }
    return mb->invoker(o, mb, args_size, ee);
}


/* Determine the value that should code in the CompiledCodeFlags field.
 * For all implementations:
 *   Bits 24-31 give the return type character of the signature.
 * For Intel:
 *   Bits 0-2 give a number describing the return type of the function.
 */
static long calculateFlags(struct methodblock *mb)
{
    char *signature = fieldsig(&mb->fb);
    char *p;
    long flags = 0;
    for (p = signature + 1; *p != SIGNATURE_ENDFUNC; p++);
    switch (p[1])
    {
	case SIGNATURE_LONG:
	    flags = 1; break;
	case SIGNATURE_FLOAT:
	    flags = 2; break;
	case SIGNATURE_DOUBLE:
	    flags = 3; break;
	case SIGNATURE_VOID:
	    flags = 4; break;
    }
    flags |= p[1] << 24;
    return flags;
}


/* Called from the initializer or from init when the compiler class is loaded.
 * Initialize the methods in this class in whatever way is needed by
 * Compiled code. In particular, we should
 *    1) Patch compiled code in whatever way necessary
 *    2) Initialize the mb->CompiledCode field for non-compiled functions.
 *    3) Initialize the mb->CompiledCodeFlags field.
 *    4) Change the mb->invoker field to invokeCompiler for jit compilation if
 *	 no control panel is present, otherwise, the panel decided already.
 *
 * This function runs under a lock for cb or under lock_classes.
 */
static void __cdecl
InitializeForCompiler(ClassClass *cb)
{
    struct methodblock *mb = cbMethods(cb);
    int size;

//  debugging:
//  CompilerEnabled = strncmp(cb->name, "java", 4) && strncmp(cb->name, "sun", 3);

    for (size = 0; size < (int) cb->methods_count; size++, mb++)
    {
	if (mb->fb.access & ACC_MACHINE_COMPILED)
	// precompiled methods not supported, invokeCompiledMethod in
	// compiler.c will complain
	{
	    // The mb->code field temporarily holds the patches.  We should
	    // free them when we're done.
	    PatchCompiledCode(mb, mb->CompiledCode, mb->code, mb->code_length);
	    (*p_free)(mb->code);
	    mb->code = NULL;
	    mb->code_length = 0;
	}
	else if (p_notifyClassInit == 0)
	// otherwise, control panel will do the initialization
	{
	    if (CompilerEnabled)
		compileAndGoOnInvoke(mb);
	    else
		interpretOnInvoke(mb, FALSE);
	}

	mb->CompiledCodeFlags = calculateFlags(mb);
    }
    if (p_notifyClassInit)
	(*p_notifyClassInit)(cb);
	// control panel must call compileAndGoOnInvoke or interpretOnInvoke for
	// each method of cb
}


static void __cdecl
CompilerFreeClass(ClassClass *cb)
{
    struct methodblock *mb = cbMethods(cb);
    int size;

    INVOKER_LOCK();
    if (p_notifyClassFree)
	(*p_notifyClassFree)(cb);

    for (size = 0; size < (int) cb->methods_count; size++, mb++)
    {
	if  ((mb->fb.access & ACC_MACHINE_COMPILED) && mb->CompiledCode)
	{
/* can we really free CompiledCode here?
 * were compiled methods on the stack properly marked by the gc?
	    (*p_free)(mb->CompiledCode);
            (*p_free)(mb->CompiledCodeInfo);
*/
	    mb->CompiledCode = 0;
            mb->CompiledCodeInfo = 0;
	    // kept mb->code of a compiled method freed by VM
	}
    }
    INVOKER_UNLOCK();
}


#ifdef IN_NETSCAPE_NAVIGATOR
static void __cdecl
CompilerFreeMethod(struct methodblock *mb)
{
    INVOKER_LOCK();
    if  ((mb->fb.access & ACC_MACHINE_COMPILED) && mb->CompiledCode)
    {
        deleteCodeRange(mb->CompiledCodeInfo);
	(*p_free)(mb->CompiledCode);
	(*p_free)(mb->CompiledCodeInfo);
	mb->CompiledCode = 0;
	mb->CompiledCodeInfo = 0;
	// kept mb->code of a compiled method freed by VM
    }
    INVOKER_UNLOCK();
}
#endif

static bool_t __cdecl
CompilercompileClass(ClassClass *cb)
{
    struct methodblock *mb;
    int size;

    mb = cbMethods(cb);
    for (size = 0; size < (int) cb->methods_count; size++, mb++)
    	if ((mb->fb.access & ACC_MACHINE_COMPILED) == 0) // do not recompile
	    compileAndGoOnInvoke(mb);
    return TRUE;
}


/* The methods of all the loaded classes with a name matching the given prefix
 * will be compiled at next invocation.
 */
static bool_t __cdecl
CompilercompileClasses(Hjava_lang_String *name)
{
    struct methodblock *mb;
    int j, size, len;
    ClassClass **pcb, *cb;
    char clname[256], *p;
    int c;
    bool_t found_one = FALSE;

    (*p_javaString2CString)(name, clname, sizeof(clname));
    for (p = clname ; (c = *p); p++) {
	if (c == '.') {
	    *p = '/';
	}
    }
    len = p - clname;

    (*p_lock_classes)();
    pcb = *p_binclasses;
    for (j = *p_nbinclasses; --j >= 0; pcb++)
    {
	cb = *pcb;
	if (strncmp(cb->name, clname, len) == 0)	// test classloader ??????
	{
            found_one = TRUE;
	    mb = cbMethods(cb);
	    for (size = 0; size < (int) cb->methods_count; size++, mb++)
		if ((mb->fb.access & ACC_MACHINE_COMPILED) == 0) // do not recompile
		    compileAndGoOnInvoke(mb);
	}
    }
    (*p_unlock_classes)();
    return found_one;
}


static Hjava_lang_Object * __cdecl
CompilerCommand(Hjava_lang_Object *any)
{
    return NULL;    // no command implemented
}


/* The semantic of CompilerEnable and CompilerDisable is not well
 * defined in the Java API Doc. We implement them as follows:
 * Calling these functions does not modify the state (ACC_MACHINE_COMPILED)
 * of already executed methods. Otherwise, it breaks our stack crawling
 * mechanism. Also, it has no effect on methods of already loaded classes, i.e.
 * a method will be compiled at its first invocation if CompilerEnabled was true
 * at the time its class was loaded, it will never be compiled otherwise.
 * These functions define the compiler behavior for methods of classes being
 * loaded after calling these functions.
 */
static void __cdecl
CompilerEnable(void)
{
    CompilerEnabled = 1;
}


static void __cdecl
CompilerDisable(void)
{
    CompilerEnabled = 0;
}


static bool_t __cdecl
PCinCompiledCode(unsigned char *pc, struct methodblock *mb)
{
    CodeInfo *ci = (CodeInfo *)mb->CompiledCodeInfo;
    return pc >= ci->start_pc && pc < ci->end_pc;
}


static unsigned char * __cdecl
CompiledCodePC(JavaFrame *frame, struct methodblock *mb)
{
    return frame->lastpc;
}


static JavaFrame * __cdecl
CompiledFramePrev(JavaFrame *frame, JavaFrame *buf)
{
    struct methodblock *caller_mb;
    unsigned char *ra, *base;
    CodeInfo *ci;

    sysAssert(frame->current_method != 0 &&
	      (frame->current_method->fb.access & ACC_MACHINE_COMPILED));

    ra = *((unsigned char **)frame->returnpc);
    if (ra == StubRetAddr || (caller_mb = PC2CompiledMethod(ra)) == 0)
	return frame->prev;

    ci = (CodeInfo *)caller_mb->CompiledCodeInfo;
    base = frame->returnpc + 4*frame->current_method->args_size +
	ci->localSize + ci->frameSize;
    buf->current_method = caller_mb;
    buf->vars = (stack_item *)(base + 4);
#ifdef IN_NETSCAPE_NAVIGATOR
    buf->annotation = *((JHandle **)(base - ci->frameSize));
#endif
    buf->lastpc = ra;	// used in CompiledCodePC
    buf->returnpc = base;
    buf->prev = frame->prev;    // frame may be buf !
    return buf;
}


#ifdef IN_NETSCAPE_NAVIGATOR
static void __cdecl
SetCompiledFrameAnnotation(JavaFrame *frame, JHandle *annotation)
{
    struct methodblock *mb = frame->current_method;
    CodeInfo *ci = (CodeInfo *)mb->CompiledCodeInfo;

    sysAssert(mb->fb.access & ACC_MACHINE_COMPILED);

    *((JHandle **)(frame->returnpc - ci->frameSize)) = annotation;
}
#endif


/* 2 functions used by sysAssert */
void
DumpThreads(void)
{
    (*p_DumpThreads)();
}

void
printToConsole(const char *txt)
{
    if(p_printToConsole)
        (*p_printToConsole)(txt);
}

int
cprintf(FILE *strm, const char *format, .../* args */)
{
    va_list ap;
    char buf[512];
    int r;

    va_start(ap, format);
	/* cant use safe variant because of linkage */
    r = vsprintf(buf, format, ap);
    printToConsole(buf);
    va_end(ap);
    return r;
}


static ClassClass** __cdecl
get_binclasses(void)
{
    return *p_binclasses;
}


static long __cdecl
get_nbinclasses(void)
{
    return *p_nbinclasses;
}



static void
LinkCTRL(void)
{
    int (*p_link)(void **);
    void* linkVect[] = {

	&p_notifyCompilation,
	&p_notifyClassInit,
	&p_notifyClassFree,

	&get_binclasses,
	&get_nbinclasses,
	&compileAndGoOnInvoke,
	&goOnInvoke,
	&interpretOnInvoke,
    };

    (FARPROC)p_link = GetProcAddress(controlHandle, "LinkCTRL");
    if (p_link)
    (*p_link)(linkVect);
}


static void
LoadCTRL(void)
{
    char envVar[256];
    char *controlName = DefaultControlName;
    long len;

    len = GetEnvironmentVariable(ControlEnvVarName, envVar, sizeof(envVar));
    if (len > 0 && len < sizeof(envVar))
	controlName = envVar;

    controlHandle = LoadLibrary(controlName);
    if (controlHandle)
	LinkCTRL();
}


static void
Init(void)
{
    int 	j;
    ClassClass	**pcb, *cb;

    InitCompiler();

    _invoker_lock = (sys_mon_t *) (*p_malloc)((*p_sysMonitorSizeof)());
    memset((char*) _invoker_lock, 0, (*p_sysMonitorSizeof)());
    INVOKER_LOCK_INIT();

    cprintf(stderr, logo0);
    cprintf(stderr, logo1);

//    LoadCTRL();

    code_ranges = (*p_calloc)(max_ncode_ranges, sizeof(CodeRange));
    (*p_lock_classes)();
    CompilerEnabled = 1;
    pcb = *p_binclasses;
    for (j = *p_nbinclasses; --j >= 0; pcb++)
    {
	cb = *pcb;
	InitializeForCompiler(cb);
    }
    (*p_unlock_classes)();
}


#define LINK(p) (void *)(p) = vect[i++]

/* this is a deliberate attempt to make this jit not work
   with other runtimes at Borland's request */
#define COMPILER_VERSION 60

/* Link the virtual machine if its version is compatible
 * with this version of the jit compiler.
 */
__declspec(dllexport) void
java_lang_Compiler_start(void **vect)
{
    int i = 0;

#ifdef	CHECK_REGISTRY
    {
	HKEY ctrlKey1, ctrlKey2;
	char regKeyStr[45];
	int i;

	RegOpenKeyEx(baseKey1, regKeyStr1, 0, KEY_READ, &ctrlKey1);
	if (ctrlKey1)
            RegCloseKey(ctrlKey1);
	else
        {
            fprintf(stderr, "*** AppAccelerator was not properly installed.\n");
	    fflush(stderr);
	    return;
	}
	for (i = 0; i < 44; i++)
	    regKeyStr[i] = regKeyStr3[i] ^ (regKeyStr2[i] & 31);

	regKeyStr[44] = 0;
	RegOpenKeyEx(baseKey2, regKeyStr, 0, KEY_READ, &ctrlKey2);
	if (ctrlKey2)
	    RegCloseKey(ctrlKey2);
	else
	    return;
    }
#endif

    LINK(p_JavaVersion);

    if (*p_JavaVersion != ((COMPILER_VERSION << 24) | (JAVA_VERSION << 16) |
			  JAVA_MINOR_VERSION))
    {
    fprintf(stderr, "JIT incompatible with this Java runtime.\n");
	fflush(stderr);
	return;
    }

    LINK(pp_InitializeForCompiler);
    *pp_InitializeForCompiler = &InitializeForCompiler;

    LINK(pp_invokeCompiledMethod);	// no precompiled methods
    LINK(pp_CompiledCodeSignalHandler); // not used

    LINK(pp_CompilerFreeClass);
    *pp_CompilerFreeClass = &CompilerFreeClass;

#ifdef IN_NETSCAPE_NAVIGATOR
    LINK(pp_CompilerFreeMethod);
    *pp_CompilerFreeMethod = &CompilerFreeMethod;
#endif

    LINK(pp_CompilerCompileClass);
    *pp_CompilerCompileClass = &CompilercompileClass;

    LINK(pp_CompilercompileClasses);
    *pp_CompilercompileClasses = &CompilercompileClasses;

    LINK(pp_CompilerEnable);
    *pp_CompilerEnable = &CompilerEnable;

    LINK(pp_CompilerDisable);
    *pp_CompilerDisable = &CompilerDisable;

    LINK(pp_ReadInCompiledCode);  // no precompiled methods

    LINK(pp_PCinCompiledCode);	
    *pp_PCinCompiledCode = &PCinCompiledCode;

    LINK(pp_CompiledCodePC);
    *pp_CompiledCodePC = &CompiledCodePC;

    LINK(pp_CompiledFramePrev);
    *pp_CompiledFramePrev = &CompiledFramePrev;

#ifdef IN_NETSCAPE_NAVIGATOR
    LINK(pp_SetCompiledFrameAnnotation);
    *pp_SetCompiledFrameAnnotation = &SetCompiledFrameAnnotation;
#endif

    LINK(p_CompiledCodeAttribute);  // no precompiled methods

    LINK(p_UseLosslessQuickOpcodes);
    //*p_UseLosslessQuickOpcodes = 1;

    LINK(p_malloc);
    LINK(p_calloc);
    LINK(p_realloc);
    LINK(p_free);

    LINK(p_binclasses);
    LINK(p_nbinclasses);
    LINK(p_lock_classes);
    LINK(p_unlock_classes);

    LINK(p_classJavaLangClass);
    LINK(p_classJavaLangObject);
    LINK(p_classJavaLangString);
    LINK(p_classJavaLangThrowable);
    LINK(p_classJavaLangException);
    LINK(p_classJavaLangRuntimeException);
    LINK(p_interfaceJavaLangCloneable);

    LINK(p_EE);
    LINK(p_SignalError);
    LINK(p_exceptionInternalObject);

    LINK(p_GetClassConstantClassName);
    LINK(p_ResolveClassConstant);
    LINK(p_ResolveClassConstantFromClass);
    LINK(p_VerifyClassAccess);
    LINK(p_FindClass);
    LINK(p_FindClassFromClass);
    LINK(p_dynoLink);
    LINK(p_do_execute_java_method_vararg);
    LINK(p_is_subclass_of);

    LINK(p_invokeJavaMethod);
    LINK(p_invokeSynchronizedJavaMethod);
    LINK(p_invokeAbstractMethod);
    LINK(p_invokeLazyNativeMethod);
    LINK(p_invokeSynchronizedNativeMethod);
    LINK(p_invokeCompiledMethod);

    LINK(p_monitorEnter);
    LINK(p_monitorExit);
    LINK(p_monitorRegister);
    LINK(p_sysMonitorSizeof);
    LINK(p_sysMonitorEnter);
    LINK(p_sysMonitorExit);

    LINK(p_ObjAlloc);
    LINK(p_ArrayAlloc);
    LINK(p_MultiArrayAlloc);
    LINK(p_sizearray);
    LINK(p_newobject);
    LINK(p_is_instance_of);
    LINK(p_jio_snprintf);

    LINK(p_javaString2CString);
    LINK(p_classname2string);
    LINK(p_IDToNameAndTypeString);
    LINK(p_IDToType);
    LINK(p_DumpThreads);

    LINK(p_printToConsole);
    LINK(p_CreateNewJavaStack);
    LINK(p_execute_java_static_method);
    LINK(p_ExecuteJava);

    LINK(p_now);
    LINK(p_java_monitor);
    LINK(p_java_mon);
    LINK(p_JavaStackSize);

    Init();
}

