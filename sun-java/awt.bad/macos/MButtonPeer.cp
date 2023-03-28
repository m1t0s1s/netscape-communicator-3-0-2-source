//##############################################################################
//##############################################################################
//
//	File:		MButtonPeer.cp
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
#include "java_awt_button.h"
#include "java_awt_Component.h"
#include "sun_awt_macos_MButtonPeer.h"
#include "sun_awt_macos_MComponentPeer.h"

};

enum {
	kSmallButtonTextSizeThreshold		= 17,
	kSmallButtonTextSizeTextTraitsID	= 128
};

#include "MButtonPeer.h"
#include "MCanvasPeer.h"
#include "LMacGraphics.h"

#ifdef UNICODE_FONTLIST
#include "LMacCompStr.h"
#endif
//##############################################################################
//##############################################################################
#pragma mark MButtonPeer IMPLEMENTATION
#pragma mark -


MButtonPeer::
MButtonPeer(struct SPaneInfo &newScrollbarInfo, Str255 labelText):
	LStdButton(newScrollbarInfo, 0, kDefaultAWTTextTraitsID, labelText)
{
}


MButtonPeer::
~MButtonPeer()
{
	DisposeJavaDataForPane(this);
}

Boolean 
MButtonPeer::FocusDraw()
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
MButtonPeer::HotSpotResult(Int16 inHotSpot)
{
	MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "handleAction", "()V");
}
#ifdef UNICODE_FONTLIST
enum {
	kStdButtonRoundWidth = 10,
	kStdButtonRoundHeight = 10
};
void
MButtonPeer::HotSpotAction(
	Int16		/* inHotSpot */,
	Boolean		inCurrInside,
	Boolean		inPrevInside)
{
	if (inCurrInside != inPrevInside) {	
	
		FocusDraw();
		Rect	frame;
		CalcLocalFrameRect(frame);
		short   weight = (frame.bottom-frame.top) / 2;
		StColorPenState::Normalize();
		::InvertRoundRect(&frame,weight,weight);
	}
}
Boolean
MButtonPeer::TrackHotSpot(
	Int16	inHotSpot,
	Point 	inPoint)
{
									// For the initial mouse down, the
									// mouse is currently inside the HotSpot
									// when it was previously outside
	Boolean		currInside = true;
	Boolean		prevInside = false;
	HotSpotAction(inHotSpot, currInside, prevInside);
	
									// Track the mouse while it is down
	Point	currPt = inPoint;
	while (StillDown()) {
		GetMouse(&currPt);			// Must keep track if mouse moves from
		prevInside = currInside;	// In-to-Out or Out-To-In
		currInside = PointInHotSpot(currPt, inHotSpot);
		HotSpotAction(inHotSpot, currInside, prevInside);
	}
	
	EventRecord	macEvent;			// Get location from MouseUp event
	if (GetOSEvent(mUpMask, &macEvent)) {
		currPt = macEvent.where;
		GlobalToLocal(&currPt);
	}

	this->DrawSelf();
									// Check if MouseUp occurred in HotSpot
	return PointInHotSpot(currPt, inHotSpot);
}

void
MButtonPeer::DrawTitle()
{
	Rect frame;
	Str255 outText;
	CalcLocalFrameRect(frame);
	// Get the fontlist
	LJavaFontList *fontlist = (LJavaFontList *)unhand(PaneToPeer(this))->pInternationalData;
	LMacCompStr string(this->GetDescriptor(outText));
	if(!IsEnabled())
	{
		RGBColor color = { 0, 0, 0 };
		::RGBForeColor(&color);
		::TextMode(grayishTextOr);
	}
	string.Draw(fontlist,frame);
}
void
MButtonPeer::DrawSelf()
{
	// Do whatever a control shoul do.
	Rect	frame;
	CalcLocalFrameRect(frame);
	short   weight = (frame.bottom-frame.top) / 2;
	StColorPenState::Normalize();
	::FillRoundRect(&frame,weight,weight, &UQDGlobals::GetQDGlobals()->white);
	::FrameRoundRect(&frame,weight,weight);
	DrawTitle();
}

#endif
//##############################################################################
//##############################################################################


void sun_awt_macos_MButtonPeer_create(struct Hsun_awt_macos_MButtonPeer *newButtonPeerHandle, struct Hsun_awt_macos_MComponentPeer *hostPeerHandle)
{
	Classsun_awt_macos_MButtonPeer	*buttonPeer = (Classsun_awt_macos_MButtonPeer*)unhand(newButtonPeerHandle);
	Classjava_awt_Button			*buttonTarget = (Classjava_awt_Button*)unhand(buttonPeer->target);
	LView							*superView = (LView *)PeerToPane(hostPeerHandle);
	struct SPaneInfo				newButtonInfo;
	ResIDT							textTraitsId = 0;
	Str255							labelText;

	memset(&newButtonInfo, 0, sizeof(newButtonInfo));
	
	newButtonInfo.paneID 		= MButtonPeer::class_ID;
	newButtonInfo.visible 		= triState_Off;
	newButtonInfo.left			= buttonTarget->x;
	newButtonInfo.top			= buttonTarget->y;
	newButtonInfo.width			= buttonTarget->width;
	newButtonInfo.height		= buttonTarget->height;
	newButtonInfo.enabled 		= triState_Off;
	newButtonInfo.userCon 		= (UInt32)newButtonPeerHandle;
	newButtonInfo.superView		= superView;

#ifdef UNICODE_FONTLIST
	intl_javaString2UnicodePStr(buttonTarget->label, labelText);
#else
	javaString2CString(buttonTarget->label, (char *)(labelText + 1), 255);
	labelText[0] = strlen((char *)(labelText + 1));
#endif	
	
	if (newButtonInfo.height <= kSmallButtonTextSizeThreshold)
		textTraitsId = kSmallButtonTextSizeTextTraitsID;

	MButtonPeer *buttonPane = NULL;

	try {
		buttonPane = new MButtonPeer(newButtonInfo, labelText);
	}
	
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

	buttonPane->Enable();

	buttonPeer->mIsButton = kButtonPeerIsStandard;

	buttonPeer->mOwnerPane = (long)buttonPane;
	buttonPeer->mRecalculateClip = true;

	ClearCachedAWTRefocusPane();

}

void sun_awt_macos_MButtonPeer_setLabel(struct Hsun_awt_macos_MButtonPeer *buttonPeerHandle, struct Hjava_lang_String *labelHandle)
{

	MButtonPeer		*buttonPane = (MButtonPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)buttonPeerHandle);
	Str255			labelText;
	
	if (buttonPane == NULL)
		return;
	
	//	Convert the java string into a C string

#ifdef UNICODE_FONTLIST
	intl_javaString2UnicodePStr(labelHandle, labelText);
#else
	javaString2CString(labelHandle, (char *)(labelText + 1), 255);
	labelText[0] = strlen((char *)(labelText + 1));
#endif	
	
	buttonPane->SetDescriptor(labelText);
	
}
