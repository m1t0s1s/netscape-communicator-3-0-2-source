/* DO NOT EDIT THIS FILE - it is machine generated */
#include "StubPreamble.h"
/* Stubs for class sun/awt/macos/MTextFieldPeer */

/* SYMBOL: "sun/awt/macos/MTextFieldPeer/create(Lsun/awt/macos/MComponentPeer;)V", Java_sun_awt_macos_MTextFieldPeer_create_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MTextFieldPeer_create_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MTextFieldPeer_create(void *, void *);
	(void) sun_awt_macos_MTextFieldPeer_create(_P_[0].p,((_P_[1].p)));
	return _P_;
}

/* SYMBOL: "sun/awt/macos/MTextFieldPeer/setEditable(Z)V", Java_sun_awt_macos_MTextFieldPeer_setEditable_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MTextFieldPeer_setEditable_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MTextFieldPeer_setEditable(void *, long);
	(void) sun_awt_macos_MTextFieldPeer_setEditable(_P_[0].p,((_P_[1].i)));
	return _P_;
}

/* SYMBOL: "sun/awt/macos/MTextFieldPeer/select(II)V", Java_sun_awt_macos_MTextFieldPeer_select_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MTextFieldPeer_select_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MTextFieldPeer_select(void *, long, long);
	(void) sun_awt_macos_MTextFieldPeer_select(_P_[0].p,((_P_[1].i)),((_P_[2].i)));
	return _P_;
}

/* SYMBOL: "sun/awt/macos/MTextFieldPeer/getSelectionStart()I", Java_sun_awt_macos_MTextFieldPeer_getSelectionStart_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MTextFieldPeer_getSelectionStart_stub(stack_item* _P_, struct execenv* _EE_) {
	extern long sun_awt_macos_MTextFieldPeer_getSelectionStart(void *);
	_P_[0].i = sun_awt_macos_MTextFieldPeer_getSelectionStart(_P_[0].p);
	return _P_ + 1;
}

/* SYMBOL: "sun/awt/macos/MTextFieldPeer/getSelectionEnd()I", Java_sun_awt_macos_MTextFieldPeer_getSelectionEnd_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MTextFieldPeer_getSelectionEnd_stub(stack_item* _P_, struct execenv* _EE_) {
	extern long sun_awt_macos_MTextFieldPeer_getSelectionEnd(void *);
	_P_[0].i = sun_awt_macos_MTextFieldPeer_getSelectionEnd(_P_[0].p);
	return _P_ + 1;
}

/* SYMBOL: "sun/awt/macos/MTextFieldPeer/setText(Ljava/lang/String;)V", Java_sun_awt_macos_MTextFieldPeer_setText_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MTextFieldPeer_setText_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MTextFieldPeer_setText(void *, void *);
	(void) sun_awt_macos_MTextFieldPeer_setText(_P_[0].p,((_P_[1].p)));
	return _P_;
}

/* SYMBOL: "sun/awt/macos/MTextFieldPeer/getText()Ljava/lang/String;", Java_sun_awt_macos_MTextFieldPeer_getText_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MTextFieldPeer_getText_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void* sun_awt_macos_MTextFieldPeer_getText(void *);
	_P_[0].p = sun_awt_macos_MTextFieldPeer_getText(_P_[0].p);
	return _P_ + 1;
}

/* SYMBOL: "sun/awt/macos/MTextFieldPeer/setEchoCharacter(C)V", Java_sun_awt_macos_MTextFieldPeer_setEchoCharacter_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MTextFieldPeer_setEchoCharacter_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MTextFieldPeer_setEchoCharacter(void *, long);
	(void) sun_awt_macos_MTextFieldPeer_setEchoCharacter(_P_[0].p,((_P_[1].i)));
	return _P_;
}

