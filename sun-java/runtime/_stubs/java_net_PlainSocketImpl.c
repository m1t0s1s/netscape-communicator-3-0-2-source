/* DO NOT EDIT THIS FILE - it is machine generated */
#include "StubPreamble.h"
/* Stubs for class java/net/PlainSocketImpl */

/* SYMBOL: "java/net/PlainSocketImpl/socketCreate(Z)V", Java_java_net_PlainSocketImpl_socketCreate_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_net_PlainSocketImpl_socketCreate_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_net_PlainSocketImpl_socketCreate(void *, long);
	(void) java_net_PlainSocketImpl_socketCreate(_P_[0].p,((_P_[1].i)));
	return _P_;
}

/* SYMBOL: "java/net/PlainSocketImpl/socketConnect(Ljava/net/InetAddress;I)V", Java_java_net_PlainSocketImpl_socketConnect_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_net_PlainSocketImpl_socketConnect_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_net_PlainSocketImpl_socketConnect(void *, void *, long);
	(void) java_net_PlainSocketImpl_socketConnect(_P_[0].p,((_P_[1].p)),((_P_[2].i)));
	return _P_;
}

/* SYMBOL: "java/net/PlainSocketImpl/socketBind(Ljava/net/InetAddress;I)V", Java_java_net_PlainSocketImpl_socketBind_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_net_PlainSocketImpl_socketBind_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_net_PlainSocketImpl_socketBind(void *, void *, long);
	(void) java_net_PlainSocketImpl_socketBind(_P_[0].p,((_P_[1].p)),((_P_[2].i)));
	return _P_;
}

/* SYMBOL: "java/net/PlainSocketImpl/socketListen(I)V", Java_java_net_PlainSocketImpl_socketListen_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_net_PlainSocketImpl_socketListen_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_net_PlainSocketImpl_socketListen(void *, long);
	(void) java_net_PlainSocketImpl_socketListen(_P_[0].p,((_P_[1].i)));
	return _P_;
}

/* SYMBOL: "java/net/PlainSocketImpl/socketAccept(Ljava/net/SocketImpl;)V", Java_java_net_PlainSocketImpl_socketAccept_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_net_PlainSocketImpl_socketAccept_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_net_PlainSocketImpl_socketAccept(void *, void *);
	(void) java_net_PlainSocketImpl_socketAccept(_P_[0].p,((_P_[1].p)));
	return _P_;
}

/* SYMBOL: "java/net/PlainSocketImpl/socketAvailable()I", Java_java_net_PlainSocketImpl_socketAvailable_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_net_PlainSocketImpl_socketAvailable_stub(stack_item* _P_, struct execenv* _EE_) {
	extern long java_net_PlainSocketImpl_socketAvailable(void *);
	_P_[0].i = java_net_PlainSocketImpl_socketAvailable(_P_[0].p);
	return _P_ + 1;
}

/* SYMBOL: "java/net/PlainSocketImpl/socketClose()V", Java_java_net_PlainSocketImpl_socketClose_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_net_PlainSocketImpl_socketClose_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_net_PlainSocketImpl_socketClose(void *);
	(void) java_net_PlainSocketImpl_socketClose(_P_[0].p);
	return _P_;
}
