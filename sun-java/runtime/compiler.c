/*
 * @(#)compiler.c	1.8 96/04/26
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

#include "oobj.h"
#include "interpreter.h"
#include "exceptions.h"
#include "java_lang_Compiler.h"
#include "java_lang_String.h"
#include "sys_api.h"
#ifdef XP_MAC
#include "jdk_java_lang_Object.h"
#else
#include "java_lang_Object.h"
#endif

/* This version number should be updated whenever there are changes to
 * the interface between the interpreter and JIT compilers. */
#if 0
#define COMPILER_VERSION 2
#else
/* this is Crelier's modified version, give it a bogus number so that
   other JITs are deliberately not pluggable -jg */
#define COMPILER_VERSION 60
#endif

/**
 * JavaVersion is a unique number which changes whenever the interpreter
 * or compiler interface changes.  Compiler writers should treat this value
 * as a fixed constant (ie, don't extract the numbers used to create it),
 * as how this number is determined may change in future releases.
 */
static int JavaVersion = (COMPILER_VERSION << 24) | (JAVA_VERSION << 16) |
                         JAVA_MINOR_VERSION;

/* The name of the attribute that represents compiled code on this machine. */
char *CompiledCodeAttribute = NULL;

static void (*p_InitializeForCompiler)(ClassClass *cb) = 0;
static void (*p_CompilerFreeClass)(ClassClass *cb) = 0;
static void (*p_CompilerFreeMethod)(struct methodblock *mb) = 0;
static bool_t (*p_invokeCompiledMethod)(JHandle *o, struct methodblock *mb,
			       int args_size, ExecEnv *ee) = 0;
static void (*p_CompiledCodeSignalHandler)(int sig, void *info, void *uc) = 0;

static bool_t (*p_CompilerCompileClass)(ClassClass *cb) = 0;
static bool_t (*p_CompilercompileClasses)(Hjava_lang_String *name) = 0;
static Hjava_lang_Object *(*p_CompilerCommand)(Hjava_lang_Object *any) = 0;
static void (*p_CompilerEnable)() = 0;
static void (*p_CompilerDisable)() = 0;
static bool_t (*p_PCinCompiledCode)(unsigned char *, struct methodblock *) = 0;
static unsigned char *(*p_CompiledCodePC)(JavaFrame *frame, struct methodblock *mb) = 0;
static JavaFrame *(*p_CompiledFramePrev)(JavaFrame *frame, JavaFrame *buf) = 0;
static void       (*p_SetCompiledFrameAnnotation)(JavaFrame *frame, JHandle *obj) = 0;

void (*p_ReadInCompiledCode)(void *context,
			     struct methodblock *mb,
			     int attribute_length,
			     unsigned long (*get1byte)(),
			     unsigned long (*get2bytes)(),
			     unsigned long (*get4bytes)(),
			     void (*getNbytes)());


void InitializeForCompiler(ClassClass *cb) {
    if (p_InitializeForCompiler != NULL)
	p_InitializeForCompiler(cb);
}

void CompilerFreeClass(ClassClass *cb) {
    if (p_CompilerFreeClass != NULL)
	p_CompilerFreeClass(cb);
}

void CompilerFreeMethod(struct methodblock *mb) { 
    if (p_CompilerFreeMethod != NULL) 
	p_CompilerFreeMethod(mb);
}


void CompilerCompileClass(ClassClass *cb) {
    if (p_CompilerCompileClass != NULL)
	p_CompilerCompileClass(cb);
}

bool_t
invokeCompiledMethod(JHandle *o, struct methodblock *mb,
		     int args_size, ExecEnv *ee)
{
    if (p_invokeCompiledMethod == NULL) {
	SignalError(ee, JAVAPKG "InternalError",
		"Error! Compiled methods not supported");
	return FALSE;
    } else {
	return p_invokeCompiledMethod(o, mb, args_size, ee);
    }
}

void CompiledCodeSignalHandler(int sig, void *info, void *uc) {
    if (p_CompiledCodeSignalHandler != NULL)
	p_CompiledCodeSignalHandler(sig, info, uc);
}

void ReadInCompiledCode(void *context, struct methodblock *mb,
			int attribute_length,
			unsigned long (*get1byte)(),
			unsigned long (*get2bytes)(),
			unsigned long (*get4bytes)(),
			void (*getNbytes)())
{
    if (p_ReadInCompiledCode != NULL) {
	p_ReadInCompiledCode(context, mb, attribute_length,
			        get1byte, get2bytes, get4bytes, getNbytes);
    } else {
	getNbytes(context, attribute_length, NULL);
    }
}

bool_t PCinCompiledCode(unsigned char *pc, struct methodblock *mb) {
    return (p_PCinCompiledCode != NULL) && p_PCinCompiledCode(pc, mb);
}

unsigned char *CompiledCodePC(JavaFrame *frame, struct methodblock *mb) {
    return (p_CompiledCodePC == 0) ? NULL : p_CompiledCodePC(frame, mb);
}

JRI_PUBLIC_API(JavaFrame *)CompiledFramePrev(JavaFrame *frame, JavaFrame *buf) {
    return (p_CompiledFramePrev == 0) ? frame->prev : p_CompiledFramePrev(frame, buf);
}

JRI_PUBLIC_API(void)SetCompiledFrameAnnotation(JavaFrame *frame, JHandle *obj) {
    /* ASSERT (p_SetCompiledFrameAnnotation != 0); */
    p_SetCompiledFrameAnnotation(frame, obj);
}




void
PrintToConsole(const char* bytes)
{
    int len = strlen(bytes);
	ClassClass* system;
	JHandle *out;

	system = FindClass(0, JAVAPKG "System", TRUE);
	if(system)
	{
		out = *(JHandle **)getclassvariable(system, "out");
		if(out)
		{
			execute_java_dynamic_method(0, out, 
								"print", "(Ljava/lang/String;)V",
								makeJavaString((char*)bytes, len));
		}
	}
}

void* mymalloc(size_t x) { return malloc(x); }
void* mycalloc(size_t x, int n) { return calloc(x, n); }
void* myrealloc(void* p, size_t x) { return realloc(p, x); }
void myfree(void* p) { free(p); }

void* CompiledCodeLinkVector[] = {
	(void *)&JavaVersion,

	(void *)&p_InitializeForCompiler,
	(void *)&p_invokeCompiledMethod,
	(void *)&p_CompiledCodeSignalHandler,
	(void *)&p_CompilerFreeClass,
	(void *)&p_CompilerFreeMethod,
	(void *)&p_CompilerCompileClass,
	(void *)&p_CompilercompileClasses,
	(void *)&p_CompilerEnable,
	(void *)&p_CompilerDisable,
	(void *)&p_ReadInCompiledCode,
	(void *)&p_PCinCompiledCode,
	(void *)&p_CompiledCodePC,
	(void *)&p_CompiledFramePrev,
	(void *)&p_SetCompiledFrameAnnotation,

	(void *)&CompiledCodeAttribute,
	(void *)&UseLosslessQuickOpcodes,

	(void *)&mymalloc,
	(void *)&mycalloc,
	(void *)&myrealloc,
	(void *)&myfree,

	(void *)&binclasses,
	(void *)&nbinclasses,
	(void *)&lock_classes,
	(void *)&unlock_classes,

	(void *)&classJavaLangClass,
	(void *)&classJavaLangObject,
	(void *)&classJavaLangString,
	(void *)&classJavaLangThrowable,
	(void *)&classJavaLangException,
	(void *)&classJavaLangRuntimeException,
	(void *)&interfaceJavaLangCloneable,

	(void *)&EE,
	(void *)&SignalError,
	(void *)&exceptionInternalObject,

	(void *)&GetClassConstantClassName,
	(void *)&ResolveClassConstant,
	(void *)&ResolveClassConstantFromClass,
	(void *)&VerifyClassAccess,
	(void *)&FindClass,
	(void *)&FindClassFromClass,
	(void *)&dynoLink,
	(void *)&do_execute_java_method_vararg,
	(void *)&is_subclass_of,

	(void *)&invokeJavaMethod,
	(void *)&invokeSynchronizedJavaMethod,
	(void *)&invokeAbstractMethod,
	(void *)&invokeLazyNativeMethod,
	(void *)&invokeSynchronizedNativeMethod,
	(void *)&invokeCompiledMethod,

	(void *)&monitorEnter,
	(void *)&monitorExit,
	(void *)&monitorRegister,
	(void *)&sysMonitorSizeof,
	(void *)&sysMonitorEnter,
	(void *)&sysMonitorExit,

	(void *)&ObjAlloc,
	(void *)&ArrayAlloc,
	(void *)&MultiArrayAlloc,
	(void *)&sizearray,
	(void *)&newobject,
	(void *)&is_instance_of,
	(void *)&jio_snprintf,

	(void *)&javaString2CString,
	(void *)&classname2string,
        (void *)&IDToNameAndTypeString,
        (void *)&IDToType,

#ifdef DEBUG
	(void *)&DumpThreads,
#else
	0,
#endif
	(void *)&PrintToConsole,

	(void *)&CreateNewJavaStack,
	(void *)&execute_java_static_method,
	(void *)&ExecuteJava,

	(void *)&now,
	(void *)&java_monitor,
	(void *)&java_mon,
	(void *)&JavaStackSize
};

void java_lang_Compiler_initialize (Hjava_lang_Compiler *this)
{
    void* lib;
    void *address = (void *)sysDynamicLink("java_lang_Compiler_start", &lib);
    if (address != 0) {
	 (*(void (*) (void **)) address) (CompiledCodeLinkVector);
    }
}

#if 0
/* apparently not used */
long
java_lang_Compiler_isSilent(Hjava_lang_Compiler *this) {
  return TRUE;
}
#endif

long
java_lang_Compiler_compileClass(Hjava_lang_Compiler *this,
				     Hjava_lang_Class *clazz) {
    if (clazz == NULL) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return FALSE;
    } else if (p_CompilerCompileClass != NULL) {
	return p_CompilerCompileClass(unhand(clazz));
    } else {
	return FALSE;
    }
}

long
java_lang_Compiler_compileClasses(Hjava_lang_Compiler *this,
				     Hjava_lang_String *name) {
    if (name == NULL) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return FALSE;
    } else if (p_CompilercompileClasses != NULL) {
	return p_CompilercompileClasses(name);
    } else {
	return FALSE;
    }
}

Hjava_lang_Object *
java_lang_Compiler_command(Hjava_lang_Compiler *this,
			   Hjava_lang_Object *x) {
    if (x == NULL) {
	SignalError(0, JAVAPKG "NullPointerException", 0);
	return NULL;
    } else if (p_CompilerCommand != NULL) {
	return p_CompilerCommand(x);
    } else {
	return NULL;
    }
}

void
java_lang_Compiler_enable(Hjava_lang_Compiler *this) {
    if (p_CompilerEnable != NULL)
	p_CompilerEnable();
}


void
java_lang_Compiler_disable(Hjava_lang_Compiler *this) {
    if (p_CompilerDisable != NULL)
	p_CompilerDisable();
}
