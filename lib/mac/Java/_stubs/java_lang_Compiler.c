/* DO NOT EDIT THIS FILE - it is machine generated */
#include "StubPreamble.h"
/* Stubs for class java/lang/Compiler */

/* SYMBOL: "java/lang/Compiler/initialize()V", Java_java_lang_Compiler_initialize_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Compiler_initialize_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_lang_Compiler_initialize(void *);
	(void) java_lang_Compiler_initialize(NULL);
	return _P_;
}

/* SYMBOL: "java/lang/Compiler/compileClass(Ljava/lang/Class;)Z", Java_java_lang_Compiler_compileClass_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Compiler_compileClass_stub(stack_item* _P_, struct execenv* _EE_) {
	extern long java_lang_Compiler_compileClass(void *, void *);
	_P_[0].i = (java_lang_Compiler_compileClass(NULL,((_P_[0].p))) ? 1 : 0);
	return _P_ + 1;
}

/* SYMBOL: "java/lang/Compiler/compileClasses(Ljava/lang/String;)Z", Java_java_lang_Compiler_compileClasses_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Compiler_compileClasses_stub(stack_item* _P_, struct execenv* _EE_) {
	extern long java_lang_Compiler_compileClasses(void *, void *);
	_P_[0].i = (java_lang_Compiler_compileClasses(NULL,((_P_[0].p))) ? 1 : 0);
	return _P_ + 1;
}

/* SYMBOL: "java/lang/Compiler/command(Ljava/lang/Object;)Ljava/lang/Object;", Java_java_lang_Compiler_command_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Compiler_command_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void* java_lang_Compiler_command(void *, void *);
	_P_[0].p = java_lang_Compiler_command(NULL,((_P_[0].p)));
	return _P_ + 1;
}

/* SYMBOL: "java/lang/Compiler/enable()V", Java_java_lang_Compiler_enable_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Compiler_enable_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_lang_Compiler_enable(void *);
	(void) java_lang_Compiler_enable(NULL);
	return _P_;
}

/* SYMBOL: "java/lang/Compiler/disable()V", Java_java_lang_Compiler_disable_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Compiler_disable_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_lang_Compiler_disable(void *);
	(void) java_lang_Compiler_disable(NULL);
	return _P_;
}

