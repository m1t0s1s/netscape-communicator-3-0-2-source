/* DO NOT EDIT THIS FILE - it is machine generated */
#include "StubPreamble.h"
/* Stubs for class java/util/Date */

/* SYMBOL: "java/util/Date/toString()Ljava/lang/String;", Java_java_util_Date_toString_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_util_Date_toString_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void* java_util_Date_toString(void *);
	_P_[0].p = java_util_Date_toString(_P_[0].p);
	return _P_ + 1;
}

/* SYMBOL: "java/util/Date/toLocaleString()Ljava/lang/String;", Java_java_util_Date_toLocaleString_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_util_Date_toLocaleString_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void* java_util_Date_toLocaleString(void *);
	_P_[0].p = java_util_Date_toLocaleString(_P_[0].p);
	return _P_ + 1;
}

/* SYMBOL: "java/util/Date/toGMTString()Ljava/lang/String;", Java_java_util_Date_toGMTString_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_util_Date_toGMTString_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void* java_util_Date_toGMTString(void *);
	_P_[0].p = java_util_Date_toGMTString(_P_[0].p);
	return _P_ + 1;
}

/* SYMBOL: "java/util/Date/expand()V", Java_java_util_Date_expand_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_util_Date_expand_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_util_Date_expand(void *);
	(void) java_util_Date_expand(_P_[0].p);
	return _P_;
}

/* SYMBOL: "java/util/Date/computeValue()V", Java_java_util_Date_computeValue_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_util_Date_computeValue_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_util_Date_computeValue(void *);
	(void) java_util_Date_computeValue(_P_[0].p);
	return _P_;
}

