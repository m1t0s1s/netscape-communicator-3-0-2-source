//##############################################################################
//##############################################################################
//
//	File:		MScrollbarPeer.cp
//	Author:		Dan Clifford
//	
//	Copyright © 1995-1996, Netscape Communications Corporation
//
//##############################################################################
//##############################################################################
#pragma mark INCLUDES

#include <Types.h>
#include <Memory.h>
#include <LStdControl.h>

extern "C" {

#include <stdlib.h>
#include "native.h"
#include "typedefs_md.h"
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Scrollbar.h"
#include "java_awt_Component.h"
#include "sun_awt_macos_MScrollbarPeer.h"
#include "sun_awt_macos_MComponentPeer.h"

};

#include "MScrollbarPeer.h"
#include "MCanvasPeer.h"
#include "MToolkit.h"

//##############################################################################
//##############################################################################
#pragma mark MComponenPeer SCROLLBAR ACTION PROC
#pragma mark -

MScrollbarPeer		*gCurrentScrollbar = NULL;
Boolean				gFirstTimeThroughAction = false;

pascal void ScrollbarPeerActionProc(ControlRef theControl, ControlPartCode partCode);

#if GENERATINGCFM
RoutineDescriptor 	gScrollbarPeerActionProcRD = BUILD_ROUTINE_DESCRIPTOR(uppControlActionProcInfo, &ScrollbarPeerActionProc);
#else
#define gScrollbarPeerActionProcRD ScrollbarPeerActionProc
#endif

pascal void ScrollbarPeerActionProc(ControlRef theControl, ControlPartCode partCode)
{
	Hsun_awt_macos_MScrollbarPeer		*scrollbarPeerHandle = (Hsun_awt_macos_MScrollbarPeer *)PaneToPeer(gCurrentScrollbar);
	Classsun_awt_macos_MScrollbarPeer	*scrollbarPeer = unhand(scrollbarPeerHandle);
	StPortOriginState					savedPortInfo(gCurrentScrollbar->GetMacPort());

	switch (partCode) {
	
		case kControlUpButtonPart:
			gCurrentScrollbar->SetValue(gCurrentScrollbar->GetValue() - scrollbarPeer->mLineIncrement);
			MToolkitExecutJavaDynamicMethod(scrollbarPeerHandle, "lineUp", "(I)V", gCurrentScrollbar->GetValue());
			break;
	
		case kControlDownButtonPart:
			gCurrentScrollbar->SetValue(gCurrentScrollbar->GetValue() + scrollbarPeer->mLineIncrement);
			MToolkitExecutJavaDynamicMethod(scrollbarPeerHandle, "lineDown", "(I)V", gCurrentScrollbar->GetValue());
			break;
	
		case kControlPageUpPart:
			gCurrentScrollbar->SetValue(gCurrentScrollbar->GetValue() - scrollbarPeer->mPageIncrement);
			MToolkitExecutJavaDynamicMethod(scrollbarPeerHandle, "pageUp", "(I)V", gCurrentScrollbar->GetValue());
			break;
	
		case kControlPageDownPart:
			gCurrentScrollbar->SetValue(gCurrentScrollbar->GetValue() + scrollbarPeer->mPageIncrement);
			MToolkitExecutJavaDynamicMethod(scrollbarPeerHandle, "pageDown", "(I)V", gCurrentScrollbar->GetValue());
			break;
			
		default:		
			break;
			
	}

	//	Make sure that we invalidate our current view so that PowerPlant
	//	does not get confused with our extra clipping.
	
	LView::OutOfFocus(NULL);
	
	if (gFirstTimeThroughAction) {
		int64	sleepTime;
		gFirstTimeThroughAction = false;
		LL_I2L(sleepTime, 500);
		PR_Sleep(sleepTime);
	}
	
}

//##############################################################################
//##############################################################################
#pragma mark MComponenPeer IMPLEMENTATION
#pragma mark -


MScrollbarPeer::
MScrollbarPeer(struct SPaneInfo &newScrollbarInfo, UInt32 value, UInt32 min, UInt32 max):
	LStdControl(newScrollbarInfo, msg_Nothing, 0, 0, 0, scrollBarProc, 0, "\p", 0)

{
	SetValue(value);
	SetMinValue(min);
	SetMaxValue(max);
	
	SetActionProc(&gScrollbarPeerActionProcRD);
	
}

MScrollbarPeer::
~MScrollbarPeer()
{
	DisposeJavaDataForPane(this);
}

void 
MScrollbarPeer::HotSpotResult(Int16 inHotSpot)
{
	switch (inHotSpot) {
			
		case kControlIndicatorPart:
			MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "dragAbsolute", "(I)V", GetValue());
			break;
	
		default:		
			break;
			
	}
	
	inherited::HotSpotResult(inHotSpot);
	
}


Boolean 
MScrollbarPeer::TrackHotSpot(Int16 inHotSpot, Point inPoint)
{
	Boolean result;
	gCurrentScrollbar = this;
	valueChanged = false;
	result = inherited::TrackHotSpot(inHotSpot, inPoint);
	FocusDraw();
	Draw(NULL);
	return inHotSpot;
}

Boolean 
MScrollbarPeer::FocusDraw()
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


//##############################################################################
//##############################################################################


void sun_awt_macos_MScrollbarPeer_create(struct Hsun_awt_macos_MScrollbarPeer *scrollbarPeerHandle, struct Hsun_awt_macos_MComponentPeer *hostPeerHandle)
{
	Classsun_awt_macos_MScrollbarPeer	*scrollbarPeer = (Classsun_awt_macos_MScrollbarPeer*)unhand(scrollbarPeerHandle);
	Classjava_awt_Scrollbar				*scrollbarTarget = (Classjava_awt_Scrollbar*)unhand(scrollbarPeer->target);
	LView								*superView = (LView *)PeerToPane(hostPeerHandle);
	struct SPaneInfo					newScrollbarInfo;
	long								currentValue, 
										scrollMin,
										scrollMax;

	memset(&newScrollbarInfo, 0, sizeof(newScrollbarInfo));
	
	newScrollbarInfo.paneID 		= 0xCAFE;
	newScrollbarInfo.visible 		= triState_Off;
	newScrollbarInfo.enabled 		= triState_Off;
	newScrollbarInfo.left			= scrollbarTarget->x;
	newScrollbarInfo.top			= scrollbarTarget->y;
	newScrollbarInfo.width			= scrollbarTarget->width;
	newScrollbarInfo.height			= scrollbarTarget->height;

	newScrollbarInfo.userCon 		= (UInt32)scrollbarPeerHandle;
	newScrollbarInfo.superView		= superView;
	
	currentValue					= scrollbarTarget->value;
	scrollMin						= scrollbarTarget->minimum;
	scrollMax						= scrollbarTarget->maximum;
	
	MScrollbarPeer *scrollbarPane = NULL;
	
	try {
		scrollbarPane = new MScrollbarPeer(newScrollbarInfo, currentValue, scrollMin, scrollMax);
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

	scrollbarPane->SetValue(scrollbarTarget->value);
	
	scrollbarPane->Enable();

	scrollbarPeer->mOwnerPane = (long)scrollbarPane;
	scrollbarPeer->mLineIncrement = 1;
	scrollbarPeer->mPageIncrement = 10;

	ClearCachedAWTRefocusPane();

}

void sun_awt_macos_MScrollbarPeer_setValue(struct Hsun_awt_macos_MScrollbarPeer *scrollbarPeerHandle, SInt32 value)
{
	MScrollbarPeer *scrollbarPane = (MScrollbarPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)scrollbarPeerHandle);
	if (scrollbarPane == NULL)
		return;
	scrollbarPane->SetValue(value);
}

void sun_awt_macos_MScrollbarPeer_setValues(struct Hsun_awt_macos_MScrollbarPeer *scrollbarPeerHandle, SInt32 value, SInt32 visible, SInt32 minimum, SInt32 maximum)
{
	MScrollbarPeer *scrollbarPane = (MScrollbarPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)scrollbarPeerHandle);
	if (scrollbarPane == NULL)
		return;
	scrollbarPane->SetValue(value);
	scrollbarPane->SetMinValue(minimum);
	scrollbarPane->SetMaxValue(maximum);
}

void sun_awt_macos_MScrollbarPeer_setLineIncrement(struct Hsun_awt_macos_MScrollbarPeer *scrollbarPeerHandle, SInt32 value)
{
	Classsun_awt_macos_MScrollbarPeer 	*scrollbarPeer = unhand(scrollbarPeerHandle);
	if (scrollbarPeer == NULL)
		return;
	scrollbarPeer->mLineIncrement = value;
}

void sun_awt_macos_MScrollbarPeer_setPageIncrement(struct Hsun_awt_macos_MScrollbarPeer *scrollbarPeerHandle, SInt32 value)
{
	Classsun_awt_macos_MScrollbarPeer 	*scrollbarPeer = unhand(scrollbarPeerHandle);
	if (scrollbarPeer == NULL)
		return;
	scrollbarPeer->mPageIncrement = value;
}
