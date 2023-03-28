// $Header: /m/src/ns/sun-java/jit/win32/borland/jinterf.h,v 1.2.10.2 1996/09/30 02:47:53 dhopwood Exp $
/*
 * Copyright (c) 1996 Borland International. All Rights Reserved.
 *
 * AppAccelerator(tm) for x86
 *
 * JInterf.h, R. Crelier, 7/31/96
 *
*/

#define	NO_CLEANUP 0xFFFFFFFE

/* info block to be stored in mb->CompiledCodeInfo */
typedef struct CodeInfo
{
    unsigned char *start_pc;	// address range of native code	[start_pc..end_pc[
    unsigned char *end_pc;
    long frameSize;	// size of ret addr, saved regs, and exception frame if any
    long localSize;	// size of local variables, operand stack, and place holders
    long baseOff;	// offset of the first operand rel to esp
    long xframe2esp;	// 0 if no exception frame

}   CodeInfo;


/* Links to the virtual machine used by the compiler */

extern void *
(*p_malloc)(size_t size);

extern void *
(*p_calloc)(size_t nitems, size_t size);

extern void *
(*p_realloc)(void *block, size_t size);

extern void
(*p_free)(void *block);

extern ExecEnv *
(*p_EE)(void);

extern void
(*p_SignalError)(struct execenv *ee, char *ename, char *DetailMessage);

extern char *
(*p_GetClassConstantClassName)(cp_item_type *constant_pool, int index);

extern bool_t
(*p_ResolveClassConstant)(cp_item_type *constant_pool, unsigned index,
    struct execenv *ee, unsigned mask);

extern bool_t
(*p_ResolveClassConstantFromClass)(ClassClass *class, unsigned index,
    struct execenv *ee, unsigned mask);

extern bool_t
(*p_VerifyClassAccess)(ClassClass *current_class, ClassClass *new_class,
    bool_t classloader_only);

extern ClassClass *
(*p_FindClassFromClass)(struct execenv *ee, char *name,
    bool_t resolve, ClassClass *from);

extern bool_t
(*p_is_subclass_of)(ClassClass *cb, ClassClass *dcb, ExecEnv *ee);

extern void
(*p_monitorEnter)(unsigned int key);

extern void
(*p_monitorExit)(unsigned int key);

extern HObject *
(*p_ObjAlloc)(ClassClass *cb, long n0);

extern HObject *
(*p_ArrayAlloc)(int t, int l);

extern HObject *
(*p_MultiArrayAlloc)(int dimensions, ClassClass *array_cb, stack_item *sizes);

extern int
(*p_sizearray)(int t, int l);

extern bool_t
(*p_is_instance_of)(JHandle *h, ClassClass *dcb, ExecEnv *ee);

extern int
(*p_jio_snprintf)(char *str, size_t count, const char *fmt, ...);

extern char *
(*p_classname2string)(char *str, char *dst, int size);

extern char *
(*p_IDToNameAndTypeString)(unsigned short ID);

extern char *
(*p_IDToType)(unsigned short ID);

/* local copy of class java.lang.object */
extern ClassClass *
classJavaLangObject;

/* local copy of class java.lang.Cloneable */
extern ClassClass *
interfaceJavaLangCloneable;

/* The methodtable for java.lang.object */
extern struct methodtable *
ObjectMethodTable;

/* Functions raising a native exception */
void __stdcall
ErrorUnwind(struct execenv *ee);

void __stdcall
SignalErrorUnwind(struct execenv *ee, char *ename, char *DetailMessage,
    unsigned char *stub_base, long stub_nargs);

/* Functions used by assert */
void
DumpThreads(void);

int
cprintf(FILE *strm, const char *format, .../* args */);

