/* DO NOT EDIT THIS FILE - it is machine generated */
#include "StubPreamble.h"
/* Stubs for class java/lang/Runtime */

/* SYMBOL: "java/lang/Runtime/exitInternal(I)V", Java_java_lang_Runtime_exitInternal_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_exitInternal_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_lang_Runtime_exitInternal(void *, long);
	(void) java_lang_Runtime_exitInternal(_P_[0].p,((_P_[1].i)));
	return _P_;
}

/* SYMBOL: "java/lang/Runtime/execInternal([Ljava/lang/String;[Ljava/lang/String;)Ljava/lang/Process;", Java_java_lang_Runtime_execInternal_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_execInternal_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void* java_lang_Runtime_execInternal(void *, void *, void *);
	_P_[0].p = java_lang_Runtime_execInternal(_P_[0].p,((_P_[1].p)),((_P_[2].p)));
	return _P_ + 1;
}

/* SYMBOL: "java/lang/Runtime/freeMemory()J", Java_java_lang_Runtime_freeMemory_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_freeMemory_stub(stack_item* _P_, struct execenv* _EE_) {
	Java8 _tval;
	extern int64_t java_lang_Runtime_freeMemory(void *);
	SET_INT64(_tval, _P_, java_lang_Runtime_freeMemory(_P_[0].p));
	return _P_ + 2;
}

/* SYMBOL: "java/lang/Runtime/totalMemory()J", Java_java_lang_Runtime_totalMemory_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_totalMemory_stub(stack_item* _P_, struct execenv* _EE_) {
	Java8 _tval;
	extern int64_t java_lang_Runtime_totalMemory(void *);
	SET_INT64(_tval, _P_, java_lang_Runtime_totalMemory(_P_[0].p));
	return _P_ + 2;
}

/* SYMBOL: "java/lang/Runtime/gc()V", Java_java_lang_Runtime_gc_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_gc_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_lang_Runtime_gc(void *);
	(void) java_lang_Runtime_gc(_P_[0].p);
	return _P_;
}

/* SYMBOL: "java/lang/Runtime/runFinalization()V", Java_java_lang_Runtime_runFinalization_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_runFinalization_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_lang_Runtime_runFinalization(void *);
	(void) java_lang_Runtime_runFinalization(_P_[0].p);
	return _P_;
}

/* SYMBOL: "java/lang/Runtime/traceInstructions(Z)V", Java_java_lang_Runtime_traceInstructions_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_traceInstructions_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_lang_Runtime_traceInstructions(void *, long);
	(void) java_lang_Runtime_traceInstructions(_P_[0].p,((_P_[1].i)));
	return _P_;
}

/* SYMBOL: "java/lang/Runtime/traceMethodCalls(Z)V", Java_java_lang_Runtime_traceMethodCalls_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_traceMethodCalls_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_lang_Runtime_traceMethodCalls(void *, long);
	(void) java_lang_Runtime_traceMethodCalls(_P_[0].p,((_P_[1].i)));
	return _P_;
}

/* SYMBOL: "java/lang/Runtime/initializeLinkerInternal()Ljava/lang/String;", Java_java_lang_Runtime_initializeLinkerInternal_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_initializeLinkerInternal_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void* java_lang_Runtime_initializeLinkerInternal(void *);
	_P_[0].p = java_lang_Runtime_initializeLinkerInternal(_P_[0].p);
	return _P_ + 1;
}

/* SYMBOL: "java/lang/Runtime/buildLibName(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/String;", Java_java_lang_Runtime_buildLibName_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_buildLibName_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void* java_lang_Runtime_buildLibName(void *, void *, void *);
	_P_[0].p = java_lang_Runtime_buildLibName(_P_[0].p,((_P_[1].p)),((_P_[2].p)));
	return _P_ + 1;
}

/* SYMBOL: "java/lang/Runtime/loadFileInternal(Ljava/lang/String;)Z", Java_java_lang_Runtime_loadFileInternal_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_lang_Runtime_loadFileInternal_stub(stack_item* _P_, struct execenv* _EE_) {
	extern long java_lang_Runtime_loadFileInternal(void *, void *);
	_P_[0].i = (java_lang_Runtime_loadFileInternal(_P_[0].p,((_P_[1].p))) ? 1 : 0);
	return _P_ + 1;
}

