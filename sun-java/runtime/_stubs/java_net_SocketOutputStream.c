/* DO NOT EDIT THIS FILE - it is machine generated */
#include "StubPreamble.h"
/* Stubs for class java/net/SocketOutputStream */

/* SYMBOL: "java/net/SocketOutputStream/socketWrite([BII)V", Java_java_net_SocketOutputStream_socketWrite_stub */
JRI_PUBLIC_API(stack_item*)
Java_java_net_SocketOutputStream_socketWrite_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void java_net_SocketOutputStream_socketWrite(void *, void *, long, long);
	(void) java_net_SocketOutputStream_socketWrite(_P_[0].p,((_P_[1].p)),((_P_[2].i)),((_P_[3].i)));
	return _P_;
}
