/* DO NOT EDIT THIS FILE - it is machine generated */
#include "StubPreamble.h"
/* Stubs for class java/lang/ClassLoader */


/* SYMBOL: "java/lang/ClassLoader/defineClass(Ljava/lang/String;[BII)Ljava/lang/Class;", Java_java_lang_ClassLoader_defineClass_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_ClassLoader_defineClass_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void* java_lang_ClassLoader_defineClass(void *, void *, void *, long, long);
	_P_[0].p = java_lang_ClassLoader_defineClass(_P_[0].p,((_P_[1].p)),((_P_[2].p)),((_P_[3].i)),((_P_[4].i)));
	return _P_ + 1;
}

/* SYMBOL: "java/lang/ClassLoader/resolveClass(Ljava/lang/Class;)V", Java_java_lang_ClassLoader_resolveClass_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_ClassLoader_resolveClass_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_lang_ClassLoader_resolveClass(void *, void *);
	(void) java_lang_ClassLoader_resolveClass(_P_[0].p,((_P_[1].p)));
	return _P_;
}

/* SYMBOL: "java/lang/ClassLoader/findSystemClass(Ljava/lang/String;)Ljava/lang/Class;", Java_java_lang_ClassLoader_findSystemClass_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_ClassLoader_findSystemClass_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void* java_lang_ClassLoader_findSystemClass(void *, void *);
	_P_[0].p = java_lang_ClassLoader_findSystemClass(_P_[0].p,((_P_[1].p)));
	return _P_ + 1;
}

/* SYMBOL: "java/lang/ClassLoader/init()V", Java_java_lang_ClassLoader_init_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_ClassLoader_init_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_lang_ClassLoader_init(void *);
	(void) java_lang_ClassLoader_init(_P_[0].p);
	return _P_;
}

/* SYMBOL: "java/lang/ClassLoader/checkInitialized()V", Java_java_lang_ClassLoader_checkInitialized_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_ClassLoader_checkInitialized_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_lang_ClassLoader_checkInitialized(void *);
	(void) java_lang_ClassLoader_checkInitialized(_P_[0].p);
	return _P_;
}
