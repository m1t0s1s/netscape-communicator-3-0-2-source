/* DO NOT EDIT THIS FILE - it is machine generated */
#include "StubPreamble.h"
/* Stubs for class sun/awt/macos/MListPeer */

/* SYMBOL: "sun/awt/macos/MListPeer/create(Lsun/awt/macos/MComponentPeer;)V", Java_sun_awt_macos_MListPeer_create_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MListPeer_create_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MListPeer_create(void *, void *);
	(void) sun_awt_macos_MListPeer_create(_P_[0].p,((_P_[1].p)));
	return _P_;
}

/* SYMBOL: "sun/awt/macos/MListPeer/setMultipleSelections(Z)V", Java_sun_awt_macos_MListPeer_setMultipleSelections_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MListPeer_setMultipleSelections_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MListPeer_setMultipleSelections(void *, long);
	(void) sun_awt_macos_MListPeer_setMultipleSelections(_P_[0].p,((_P_[1].i)));
	return _P_;
}

/* SYMBOL: "sun/awt/macos/MListPeer/isSelected(I)Z", Java_sun_awt_macos_MListPeer_isSelected_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MListPeer_isSelected_stub(stack_item* _P_, struct execenv* _EE_) {
	extern long sun_awt_macos_MListPeer_isSelected(void *, long);
	_P_[0].i = (sun_awt_macos_MListPeer_isSelected(_P_[0].p,((_P_[1].i))) ? 1 : 0);
	return _P_ + 1;
}

/* SYMBOL: "sun/awt/macos/MListPeer/addItem(Ljava/lang/String;I)V", Java_sun_awt_macos_MListPeer_addItem_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MListPeer_addItem_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MListPeer_addItem(void *, void *, long);
	(void) sun_awt_macos_MListPeer_addItem(_P_[0].p,((_P_[1].p)),((_P_[2].i)));
	return _P_;
}

/* SYMBOL: "sun/awt/macos/MListPeer/delItems(II)V", Java_sun_awt_macos_MListPeer_delItems_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MListPeer_delItems_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MListPeer_delItems(void *, long, long);
	(void) sun_awt_macos_MListPeer_delItems(_P_[0].p,((_P_[1].i)),((_P_[2].i)));
	return _P_;
}

/* SYMBOL: "sun/awt/macos/MListPeer/select(I)V", Java_sun_awt_macos_MListPeer_select_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MListPeer_select_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MListPeer_select(void *, long);
	(void) sun_awt_macos_MListPeer_select(_P_[0].p,((_P_[1].i)));
	return _P_;
}

/* SYMBOL: "sun/awt/macos/MListPeer/deselect(I)V", Java_sun_awt_macos_MListPeer_deselect_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MListPeer_deselect_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MListPeer_deselect(void *, long);
	(void) sun_awt_macos_MListPeer_deselect(_P_[0].p,((_P_[1].i)));
	return _P_;
}

/* SYMBOL: "sun/awt/macos/MListPeer/makeVisible(I)V", Java_sun_awt_macos_MListPeer_makeVisible_stub */
JRI_PUBLIC_API(stack_item*)
Java_sun_awt_macos_MListPeer_makeVisible_stub(stack_item* _P_, struct execenv* _EE_) {
	extern void sun_awt_macos_MListPeer_makeVisible(void *, long);
	(void) sun_awt_macos_MListPeer_makeVisible(_P_[0].p,((_P_[1].i)));
	return _P_;
}

