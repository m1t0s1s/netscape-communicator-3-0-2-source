//##############################################################################
//##############################################################################
//
//	File:		MCheckboxPeer.cp
//	Author:		Dan Clifford
//	
//	Copyright © 1995-1996, Netscape Communications Corporation
//
//##############################################################################
//##############################################################################

#include "MToolkit.h"
#include <Types.h>
#include <Memory.h>
#include <LButton.h>
#include <LStdControl.h>
#include <LView.h>

extern "C" {

#include <stdlib.h>
#include "native.h"
#include "typedefs_md.h"
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Checkbox.h"
#include "java_awt_Component.h"
#include "sun_awt_macos_MComponentPeer.h"
#include "sun_awt_macos_MCheckboxPeer.h"

};

#ifdef UNICODE_FONTLIST
#include "macgui.h"
#include "LMacCompStr.h"
#endif

#include "MCheckboxPeer.h"
#include "MCanvasPeer.h"
#include "LMacGraphics.h"

enum {
	kSmallCheckboxTextSizeThreshold		= 17,
	kSmallCheckboxTextSizeTextTraitsID	= 128
};

//##############################################################################
//##############################################################################
#pragma mark MComponenPeer IMPLEMENTATION
#pragma mark -

#ifdef UNICODE_FONTLIST
MCheckboxPeer::
MCheckboxPeer(struct SPaneInfo &newScrollbarInfo, SInt32 value, Str255 labelText):
	LStdCheckBox(newScrollbarInfo, 0, value, 0, "\p")
{
	this->SetDescriptor(labelText);
}
#else
MCheckboxPeer::
MCheckboxPeer(struct SPaneInfo &newScrollbarInfo, SInt32 value, Str255 labelText):
	LStdCheckBox(newScrollbarInfo, 0, value, 0, labelText)
{
}
#endif

MCheckboxPeer::
~MCheckboxPeer()
{
	DisposeJavaDataForPane(this);
}

Boolean 
MCheckboxPeer::FocusDraw()
{
	Boolean	shouldRefocus = ShouldRefocusAWTPane(this);
	Boolean  result = false;
	gInPaneFocusDraw = true;
	result = inherited::FocusDraw();
	gInPaneFocusDraw = false;
	if (shouldRefocus && result)
		PreparePeerGrafPort(PaneToPeer(this), NULL);
	return result;
}

void 
MCheckboxPeer::HotSpotResult(Int16 inHotSpot)
{
	inherited::HotSpotResult(inHotSpot);
	MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "handleAction", "(Z)V", this->GetValue());
}
#ifdef UNICODE_FONTLIST
enum { 	// this number come from trail and error
	kStdRadioBoxWidthOffset = 17,
	kStdCheckBoxWidthOffset = 19
};
void 
MCheckboxPeer::DrawTitle()
{
	Rect frame;
	Str255 outText;
	CalcLocalFrameRect(frame);
	// Get the fontlist
	LJavaFontList *fontlist = (LJavaFontList *)unhand(PaneToPeer(this))->pInternationalData;
	LMacCompStr string(this->GetDescriptor(outText));
	frame.left += kStdCheckBoxWidthOffset;
	if(!IsEnabled())
	{
		RGBColor color = { 0, 0, 0 };
		::RGBForeColor(&color);
		::TextMode(grayishTextOr);
	}
	string.DrawLeft(fontlist,frame);	
}
StringPtr
MCheckboxPeer::GetDescriptor(
	Str255	outDescriptor) const
{
	return LString::CopyPStr(mText, outDescriptor);
}
void
MCheckboxPeer::SetDescriptor(
	ConstStringPtr	inDescriptor)
{
	mText = inDescriptor;
	Refresh();
}
#endif
void 
MCheckboxPeer::DrawSelf()
{
	RGBColor savedColor;
	savedColor = SwitchInControlColors(this);
	inherited::DrawSelf();
#ifdef UNICODE_FONTLIST
	DrawTitle();
#endif
	SwitchOutControlColors(this, savedColor);
}

void
MCheckboxPeer::ClickSelf(const SMouseDownEvent &inMouseDown)
{
	RGBColor savedColor;
	savedColor = SwitchInControlColors(this);
	inherited::ClickSelf(inMouseDown);
	SwitchOutControlColors(this, savedColor);
}


#ifdef UNICODE_FONTLIST
MRadioButtonPeer::
MRadioButtonPeer(struct SPaneInfo &newScrollbarInfo, SInt32 value, Str255 labelText):
	LStdRadioButton(newScrollbarInfo, 0, value, 0, "\p")
{
	this->SetDescriptor(labelText);
}
#else
MRadioButtonPeer::
MRadioButtonPeer(struct SPaneInfo &newScrollbarInfo, SInt32 value, Str255 labelText):
	LStdRadioButton(newScrollbarInfo, 0, value, 0, labelText)
{
}
#endif

MRadioButtonPeer::
~MRadioButtonPeer()
{
	//	Set the pane information in the peer to be null so that
	//	we donÕt accidentally get disposed twice.
	Classsun_awt_macos_MComponentPeer	*componentPeer = unhand(PaneToPeer(this));
	
	componentPeer->mOwnerPane = NULL;
}

#ifdef UNICODE_FONTLIST

StringPtr
MRadioButtonPeer::GetDescriptor(
	Str255	outDescriptor) const
{
	return LString::CopyPStr(mText, outDescriptor);
}
void
MRadioButtonPeer::SetDescriptor(
	ConstStringPtr	inDescriptor)
{
	mText = inDescriptor;
	Refresh();
}
void 
MRadioButtonPeer::DrawTitle()
{
	Rect frame;
	Str255 outText;
	CalcLocalFrameRect(frame);
	LJavaFontList *fontlist = (LJavaFontList *)unhand(PaneToPeer(this))->pInternationalData;
	LMacCompStr string(this->GetDescriptor(outText));
	frame.left += kStdCheckBoxWidthOffset;
	if(!IsEnabled())
	{
		RGBColor color = { 0, 0, 0 };
		::RGBForeColor(&color);
		::TextMode(grayishTextOr);
	}
	string.DrawLeft(fontlist,frame);	
}
#endif
Boolean 
MRadioButtonPeer::FocusDraw()
{
	Boolean	shouldRefocus = ShouldRefocusAWTPane(this);
	Boolean  result = false;
	gInPaneFocusDraw = true;
	result = inherited::FocusDraw();
	gInPaneFocusDraw = false;
	if (shouldRefocus && result)
		PreparePeerGrafPort(PaneToPeer(this), NULL);
	return result;
}

void 
MRadioButtonPeer::HotSpotResult(Int16 inHotSpot)
{
	inherited::HotSpotResult(inHotSpot);
	MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "handleAction", "(Z)V", this->GetValue());
}

void 
MRadioButtonPeer::DrawSelf()
{
	RGBColor savedColor;
	savedColor = SwitchInControlColors(this);
	inherited::DrawSelf();
#ifdef UNICODE_FONTLIST
	// Draw the context
	DrawTitle();
#endif
	SwitchOutControlColors(this, savedColor);
}

void
MRadioButtonPeer::ClickSelf(const SMouseDownEvent &inMouseDown)
{
	RGBColor savedColor;
	savedColor = SwitchInControlColors(this);
	inherited::ClickSelf(inMouseDown);
	SwitchOutControlColors(this, savedColor);
}

//##############################################################################
//##############################################################################


void sun_awt_macos_MCheckboxPeer_create(struct Hsun_awt_macos_MCheckboxPeer *checkboxPeerHandle, struct Hsun_awt_macos_MComponentPeer *hostPeerHandle)
{
	Classsun_awt_macos_MCheckboxPeer	*checkboxPeer = (Classsun_awt_macos_MCheckboxPeer *)unhand(checkboxPeerHandle);
	Classjava_awt_Checkbox				*checkboxTarget = (Classjava_awt_Checkbox*)unhand(checkboxPeer->target);
	LView								*superView = (LView *)PeerToPane(hostPeerHandle);
	struct SPaneInfo					newCheckboxInfo;
	ResIDT								textTraitsId = 0;
	Str255								labelText;
	LStdControl							*checkboxPane;

	memset(&newCheckboxInfo, 0, sizeof(newCheckboxInfo));
	
	newCheckboxInfo.visible		= triState_Off;
	newCheckboxInfo.left		= checkboxTarget->x;
	newCheckboxInfo.top			= checkboxTarget->y;
	newCheckboxInfo.width		= checkboxTarget->width;
	newCheckboxInfo.height		= checkboxTarget->height;
	newCheckboxInfo.enabled		= triState_Off;
	newCheckboxInfo.userCon 	= (UInt32)checkboxPeerHandle;
	newCheckboxInfo.superView	= superView;

#ifdef UNICODE_FONTLIST
	intl_javaString2UnicodePStr(checkboxTarget->label , labelText);
#else
	javaString2CString(checkboxTarget->label, (char *)(labelText + 1), 255);
	labelText[0] = strlen((char *)(labelText + 1));
#endif
	
	try {
	
		if (checkboxTarget->group == NULL) {

			newCheckboxInfo.paneID = MCheckboxPeer::class_ID;
			checkboxPane = new MCheckboxPeer(newCheckboxInfo, checkboxTarget->state, labelText);
			
		}
		
		else {
		
			newCheckboxInfo.paneID = MRadioButtonPeer::class_ID;
			checkboxPane = new MRadioButtonPeer(newCheckboxInfo, checkboxTarget->state, labelText);

		}

	}
	
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

	LPane	*oldPane = (LPane *)checkboxPeer->mOwnerPane;

	checkboxPeer->mOwnerPane = (long)checkboxPane;

	if (oldPane == (LPane *)kJavaComponentPeerHasBeenDisposed)
		checkboxPane->Show();

	checkboxPeer->isRadioButton = (checkboxTarget->group != NULL);
	checkboxPeer->mRecalculateClip = true;

	checkboxPane->Enable();
	
	ClearCachedAWTRefocusPane();

}

void sun_awt_macos_MCheckboxPeer_setLabel(struct Hsun_awt_macos_MCheckboxPeer *checkboxPeerHandle, struct Hjava_lang_String *labelString)
{

	LStdCheckBox		*checkBox = (LStdCheckBox *)PeerToPane((Hsun_awt_macos_MComponentPeer *)checkboxPeerHandle);
	Str255				labelText;
	
	if (checkBox == NULL)
		return;
	
	//	Convert the java string into a C string
#ifdef UNICODE_FONTLIST
	intl_javaString2UnicodePStr(labelString , labelText);
#else
	javaString2CString(labelString, (char *)(labelText + 1), 255);
	labelText[0] = strlen((char *)(labelText + 1));
#endif
	
	RGBColor savedColor;
	savedColor = SwitchInControlColors(checkBox);
	checkBox->SetDescriptor(labelText);
	SwitchOutControlColors(checkBox, savedColor);
	
}

void sun_awt_macos_MCheckboxPeer_setState(struct Hsun_awt_macos_MCheckboxPeer *checkboxPeerHandle, long newState)
{
	LStdCheckBox		*checkBox = (LStdCheckBox *)PeerToPane((Hsun_awt_macos_MComponentPeer *)checkboxPeerHandle);

	if (checkBox == NULL)
		return;
	
	//	Set the control state only if it does not already
	//	have the value.  This will make sure that no
	//	unnecessary drawing is done.
	
	if (newState != checkBox->GetValue()) {
		RGBColor savedColor;
		savedColor = SwitchInControlColors(checkBox);
		checkBox->SetValue(newState);
		SwitchOutControlColors(checkBox, savedColor);
	}
	
}
