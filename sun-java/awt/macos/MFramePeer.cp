//##############################################################################
//##############################################################################
//
//	File:		MFramePeer.cp
//	Author:		Dan Clifford
//	
//	Copyright © 1995-1996, Netscape Communications Corporation
//
//##############################################################################
//##############################################################################
#pragma mark INCLUDES

#include <Types.h>
#include <Memory.h>
#include <LWindow.h>
#include <LApplication.h>
#include <UDesktop.h>
#include "UGraphicGizmos.h"

#include <stdlib.h>
#include "fredmem.h"

extern "C" {
#include "mdmacmem.h"
#include "java_awt_Color.h"
#include "java_awt_Component.h"
#include "java_awt_Font.h"
#include "java_awt_Frame.h"
#include "java_awt_Dialog.h"
#include "java_awt_Window.h"
#include "native.h"
#undef __cplusplus
#include "sun_awt_macos_MComponentPeer.h"
#include "sun_awt_macos_MDialogPeer.h"
#include "sun_awt_macos_MWindowPeer.h"
#include "sun_awt_macos_MFramePeer.h"
#include "typedefs_md.h"
#include "uapp.h"

#include "n_applet_MozillaAppletContext.h"
#define IMPLEMENT_netscape_applet_EmbeddedAppletFrame
#include "n_applet_EmbeddedAppletFrame.h"
#include "java.h"
};

#include "MFramePeer.h"
#include "MToolkit.h"
#include "MComponentPeer.h"
#include "MMenuBarPeer.h"

#ifdef UNICODE_FONTLIST
#include "LVFont.h"
#include "LMacCompStr.h"
#endif
enum {
	kMFramePeerWindowResID 			= 2050,
	kMinHorizontalFrameOffset		= 5,
	kMinVerticalFrameOffset			= 43,
	kStandardDialogWidth			= 240,
	kStandardDialogHeight			= 120,
	kNumberOfNormalFramePPobs		= 4,
	kNumberOfWindowPPobs			= 4,
	kNumberOfDialogPPobs			= 4,
};


ClassClass *GetEmbeddedAppletFrameClass()
{
	return FindClass(0, "netscape/applet/EmbeddedAppletFrame", (bool_t)0);
}

ClassClass *GetWindowClass()
{
	return FindClass(0, "sun/awt/macos/MWindowPeer", (bool_t)0);
}

ClassClass *GetDialogClass()
{
	return FindClass(0, "sun/awt/macos/MDialogPeer", (bool_t)0);
}


//##############################################################################
//##############################################################################
#pragma mark MComponenPeer IMPLEMENTATION
#pragma mark -

MFramePeer::
MFramePeer(LStream *inStream):LView(inStream)
{
	(MToolkit::GetMToolkit())->AddFrame((LPane *)this);
}

MFramePeer::
~MFramePeer()
{
	(MToolkit::GetMToolkit())->RemoveFrame((LPane *)this);
	DisposeJavaDataForPane(this);
}

void
MFramePeer::Show()
{
	(MToolkit::GetMToolkit())->AddFrame((LPane *)this);
	GetSuperView()->Show();
}

void
MFramePeer::Hide()
{
	GetSuperView()->Hide();
	(MToolkit::GetMToolkit())->RemoveFrame((LPane *)this);
}

Boolean 
MFramePeer::FocusDraw()
{
	Boolean shouldRefocus = ShouldRefocusAWTPane(this);
	Boolean result = LView::FocusDraw();
	if (shouldRefocus && result)
		PreparePeerGrafPort(PaneToPeer(this), NULL);
	return result;
}


MFramePeer *
MFramePeer::CreateMFramePeerStream(LStream *inStream)
{
	return new MFramePeer(inStream);
}

void
MFramePeer::BeTarget()
{
	Hsun_awt_macos_MFramePeer	*peer = (Hsun_awt_macos_MFramePeer *)PaneToPeer(this);
	
	if ((peer != NULL) && !(unhand(peer)->mIsWindow)) {
		SetJavaTarget((Hsun_awt_macos_MComponentPeer *)peer);
		MToolkitExecutJavaDynamicMethod(peer, "gotFocus", "()V");
	}
}

void
MFramePeer::DontBeTarget()
{
	Hsun_awt_macos_MFramePeer	*peer = (Hsun_awt_macos_MFramePeer *)PaneToPeer(this);
	
	if ((peer != NULL) && !(unhand(peer)->mIsWindow)) {
		MToolkitExecutJavaDynamicMethod(peer, "lostFocus", "()V");
		SetJavaTarget(NULL);
	}
	
}

void
MFramePeer::ClickSelf(const SMouseDownEvent &inMouseDown)
{
	FocusDraw();
	if ((GetTarget() != this) && (PtInRgn(inMouseDown.whereLocal, (RgnHandle)(unhand(PaneToPeer(this))->mClipRgn))))
		SwitchTarget(this);
	
}

void
MFramePeer::DrawSelf()
{
	Hsun_awt_macos_MComponentPeer		*componentPeer = PaneToPeer(this);

	if ((componentPeer != NULL) && (unhand(componentPeer)->mClipRgn != NULL))
		EraseRgn((RgnHandle)(unhand(componentPeer)->mClipRgn));		
}

//##############################################################################
//##############################################################################

MWindow::
MWindow(LStream *inStream):LWindow(inStream)
{
}

MWindow::
~MWindow()
{
	//	When we close an AWT window, make sure to flush
	//	all of the activate events from the event queue. 
	//	This makes sure that we don't crash on an activate
	//	for a window that we just closed.
	if ((WindowPtr)GetMacPort() == ::FrontWindow())
		FlushEvents(activMask, 0);
}

MWindow *
MWindow::CreateMWindowStream(LStream *inStream)
{
	return new MWindow(inStream);
}

void
MWindow::SetAEProperty(DescType inProperty, const AEDesc &inValue, AEDesc& outAEReply)
{
	switch (inProperty) {

		case pBounds:

			Rect	theBounds;

			UExtractFromAEDesc::TheRect(inValue, theBounds);
			
			MFramePeer							*framePeer = (MFramePeer *)FindPaneByID('java');
			Hsun_awt_macos_MFramePeer			*framePeerHandle = (Hsun_awt_macos_MFramePeer *)PaneToPeer(framePeer);

			//	If there is a java object associated with the peer, then 
			//	call it to do the resizing.

			if (framePeerHandle != NULL) {

				LPane 		*menuBarPane = FindPaneByID(MMenuBarPeer::class_ID);

				//	Adjust the bounds rectangle for the menu bar and the
				//	warning area.
				
				if (menuBarPane != NULL)
					theBounds.top += MMenuBarPeer::kMenuBarHeight;

				theBounds.bottom -= 15;

				//	Call through to java to do the reshaping.

				MToolkitExecutJavaDynamicMethod(framePeerHandle, 
					"handleFrameReshape", 
					"(IIII)V",
					theBounds.left, 
					theBounds.top, 
					theBounds.right - theBounds.left, 
					theBounds.bottom - theBounds.top);

			}
		
			else {
				DoSetBounds(theBounds);
			}

			break;

		default:
			inherited::SetAEProperty(inProperty, inValue, outAEReply);

	}
}

Boolean
MWindow::ObeyCommand(CommandT inCommand, void *ioParam)
{
	Boolean 	commandObeyed;

	if (inCommand == cmd_Close) {
	
		MFramePeer						*framePane = (MFramePeer *)FindPaneByID('java');
		Hsun_awt_macos_MFramePeer		*framePeerHandle = (Hsun_awt_macos_MFramePeer *)PaneToPeer(framePane);
		
		MToolkitExecutJavaDynamicMethod(framePeerHandle, "handleWindowClose", "()V");
	
		commandObeyed = true;
	
	}
	
	else {
		commandObeyed = LWindow::ObeyCommand(inCommand, ioParam);
	}
	
	return commandObeyed;
	
}

void
MWindow::FindCommandStatus(
								CommandT			inCommand,
								Boolean				&outEnabled,
								Boolean				&outUsesMark,
								Char16				&outMark,
								Str255				outName)
{
	if (inCommand == cmd_Close)
		outEnabled = true;
	else
		inherited::FindCommandStatus(inCommand, outEnabled, outUsesMark, outMark, outName);
}


//##############################################################################
//##############################################################################

CIconHandle MUnsecureFramePane::gUnsecureWarningCICN = NULL;

MUnsecureFramePane::
MUnsecureFramePane(LStream *inStream):LPane(inStream)
{
	warningText[0] = 0;
}

MUnsecureFramePane *
MUnsecureFramePane::CreateMUnsecureFramePaneStream(LStream *inStream)
{
	return new MUnsecureFramePane(inStream);
}

void MUnsecureFramePane::DrawSelf()
{
	if (gUnsecureWarningCICN == NULL)
		gUnsecureWarningCICN = GetCIcon(kWarningCicnResID);
	
	Rect	frameRect;
	Rect	greyRect;
	Rect	iconRect;
	
	CalcLocalFrameRect(frameRect);

	::ForeColor(blackColor);
	::BackColor(whiteColor);
	
	::MoveTo(frameRect.left, frameRect.top);
	::LineTo(frameRect.right, frameRect.top);

	if (warningText[0] != 0) {
	
		iconRect = frameRect;
		iconRect.right = iconRect.left + 16;
		iconRect.bottom = iconRect.top + 16;
		if (gUnsecureWarningCICN != NULL)
			PlotCIcon(&iconRect, gUnsecureWarningCICN);

		iconRect = frameRect;
		iconRect.right++;
		iconRect.left = iconRect.right - 16;
		iconRect.bottom = iconRect.top + 16;
		if (gUnsecureWarningCICN != NULL)
			PlotCIcon(&iconRect, gUnsecureWarningCICN);
	
		frameRect.left += 15;
		frameRect.right -= 15;	

	}
	
	if (warningText[0] == 0) {
		frameRect.left--;
		frameRect.right++;
	}

	frameRect.bottom++;
	::ForeColor(blackColor);
	::FrameRect(&frameRect);
	
	::InsetRect(&frameRect, 1, 1);
	
	UGraphicGizmos::BevelRect(frameRect, 1, kFramePeerTopBevelColor, kFramePeerBottomBevelColor);
	::InsetRect(&frameRect, 1, 1);
	::PmForeColor(kFramePeerMiddleBevelColor);
	::PaintRect(&frameRect);

	if (warningText[0] != 0) {
	
		::TextFont(0);
		::TextFace(normal);
		::TextSize(0);
		ForeColor(blackColor);
		TextMode(srcOr);
		::MoveTo(frameRect.left + 3, frameRect.bottom - 3);
		// 	i18n fix!!!
#ifdef UNICODE_FONTLIST
		LMacCompStr compStr(warningText);
		LJavaFontList fontlist;
		compStr.Draw(&fontlist, frameRect.left + 3, frameRect.bottom - 2);	
#else	
		::DrawString(warningText);
#endif	

	}

	::PenNormal();

}

//##############################################################################
//##############################################################################
#pragma mark -
#pragma mark JAVA STUBS

Boolean		haveRegisteredCallback = false;

void sun_awt_macos_MFramePeer_create(struct Hsun_awt_macos_MFramePeer *framePeerHandle, struct Hsun_awt_macos_MComponentPeer *hostPeerHandle)
{
	Classsun_awt_macos_MFramePeer	*framePeer = (Classsun_awt_macos_MFramePeer*)unhand(framePeerHandle);
	Classjava_awt_Frame				*frameTarget = (Classjava_awt_Frame*)unhand(framePeer->target);
	Boolean							isDialogOrWindow,
									isDialog;
	LWindow							*topLevelFrame;
	MFramePeer						*framePane;
	Rect							inBounds;
	SInt16							windowPPobResID = kMFramePeerWindowResID;
		
	//	Every new applet (frame) requires at least 96K of available memory,
	//	which is part of our pre-flight allocation strategy.

	if (!Memory_ReserveInMacHeap(96 * 1024)) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

	ClearCachedAWTRefocusPane();

	//	Create a new window from the template we will find in the hosting
	//	applicationÕs resource fork.
	
	if (!haveRegisteredCallback) {
		URegistrar::RegisterClass(MFramePeer::class_ID, (ClassCreatorFunc)MFramePeer::CreateMFramePeerStream);
		URegistrar::RegisterClass(MUnsecureFramePane::class_ID, (ClassCreatorFunc)MUnsecureFramePane::CreateMUnsecureFramePaneStream);
		URegistrar::RegisterClass(MWindow::class_ID, (ClassCreatorFunc)MWindow::CreateMWindowStream);
		haveRegisteredCallback = true;
	}

	isDialogOrWindow = is_instance_of((JHandle *)framePeerHandle, GetWindowClass(), EE());

	if (isDialogOrWindow) {
	
		// 	We have different PPobs for windows, dialogs and simple
		//	frames.  Make sure that we get the appropriate one.
	
		windowPPobResID += kNumberOfNormalFramePPobs;
		
		isDialog = is_instance_of((JHandle *)framePeerHandle, GetDialogClass(), EE());		
		if (isDialog) {
			
			Classjava_awt_Dialog *dialogTarget = (Classjava_awt_Dialog *)unhand(framePeer->target);
		
			windowPPobResID += kNumberOfWindowPPobs;
	
			if (dialogTarget->resizable)
				windowPPobResID++;
	
		}
	
	} 
	
	else {

		//	Use the resizable frame PPob if the window is resizable

		if (frameTarget->resizable)
			windowPPobResID++;
			
	}

	ClassClass					*embeddedAppletFrameClass;
	bool_t                  	isEmbedded;

	//	If we are creating an EmbeddedAppletFrame instead of a 
	//	real, top-level frame, then we need to use the LView that the
	//	browser created for us in GetJavaAppSize rather than creating
	//	a new window.  We can get this information out of the 
	//	appletData structure which was squirreled away off of the 
	//	pData of the EmbdeddedAppletFrame.

	embeddedAppletFrameClass = GetEmbeddedAppletFrameClass();
	isEmbedded = is_instance_of((JHandle *)(framePeer->target), embeddedAppletFrameClass, EE());

	if (isEmbedded) {
	
		netscape_applet_EmbeddedAppletFrame	*appletFrame;
		LJAppletData 						*appletData;
		JRIEnv* env							= (JRIEnv*)EE();	/* Mixing jri and jdk natives! */
		
		appletFrame = (netscape_applet_EmbeddedAppletFrame *)framePeer->target;
		
		if ((appletFrame != NULL) && (get_netscape_applet_EmbeddedAppletFrame_pData(env, appletFrame) != NULL)) {
			appletData = (LJAppletData *)get_netscape_applet_EmbeddedAppletFrame_pData(env, appletFrame);
			framePane = (MFramePeer *)appletData->window;
		}
         
		framePeer->mFrameIsEmbedded = true;
		framePeer->mFrameIsSecure = false;
		framePeer->mIsResizableFrame = false;
		framePeer->mRecalculateClip = true;
		framePeer->mIsContainer = true;
		framePeer->mOwnerPane = (long)framePane;

		framePane->SetUserCon((long)framePeerHandle);
		
		framePane->Activate();

		return;

	}
	
	else {

		try {
			topLevelFrame = LWindow::CreateWindow(windowPPobResID, CFrontApp::GetApplication());
		}
		catch (...) {
			SignalError(0, JAVAPKG "OutOfMemoryError", 0);
			return;
		}

		framePane = (MFramePeer *)(topLevelFrame->FindPaneByID('java'));

		framePeer->mFrameIsEmbedded = false;
		framePeer->mFrameIsSecure = false;

		MUnsecureFramePane		*warningPane;		

		warningPane = (MUnsecureFramePane *)(topLevelFrame->FindPaneByID(MUnsecureFramePane::class_ID));
		
		if (frameTarget->warningString != NULL) {
		
	// 	i18n fix!!!
#ifdef UNICODE_FONTLIST
			intl_javaString2UnicodePStr(frameTarget->warningString, warningPane->warningText);
#else
			javaString2CString(frameTarget->warningString, (char *)(warningPane->warningText + 1), 255);
			warningPane->warningText[0] = strlen((char *)warningPane->warningText + 1);
#endif	
		}
		
		else {
		
			warningPane->warningText[0] = 0;
		
		}

	}

	framePeer->mRecalculateClip = true;
	
	if (isDialogOrWindow)
		framePeer->mIsResizableFrame = false;
	else
		framePeer->mIsResizableFrame = frameTarget->resizable;

	framePeer->mIsContainer = true;
	framePeer->mOwnerPane = (long)framePane;
	
	framePane->SetUserCon((long)framePeerHandle);

	inBounds.left = frameTarget->x + kMinHorizontalFrameOffset;
	inBounds.right = frameTarget->x + frameTarget->width + kMinHorizontalFrameOffset;
	inBounds.top = frameTarget->y + kMinVerticalFrameOffset;
	inBounds.bottom = frameTarget->y + frameTarget->height + kMinVerticalFrameOffset + 15;

	topLevelFrame->DoSetBounds(inBounds);
	
}

void sun_awt_macos_MFramePeer_setTitle(struct Hsun_awt_macos_MFramePeer *framePeerHandle, struct Hjava_lang_String *stringHandle)
{
	Classsun_awt_macos_MFramePeer	*framePeer = unhand(framePeerHandle);
	MFramePeer						*framePane = (MFramePeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)framePeerHandle);
	Str255							pNewTitle;
	
	if (framePane == NULL)
		return;

	if (framePeer->mFrameIsEmbedded)
		return;
	
	//	Convert the java string into a C string
	// 	i18n fix!!!
#ifdef UNICODE_FONTLIST
	short encoding = intl_GetSystemUIEncoding();
	intl_javaString2CString(encoding, stringHandle, 
								(unsigned char *)(pNewTitle + 1), 255);
#else
	javaString2CString(stringHandle, (char *)(pNewTitle + 1), 255);
#endif	
	pNewTitle[0] = strlen((char *)(pNewTitle + 1));
	
	//	The PowerPlany window is actually this frameÕs parent,
	//	so pass the message up.
	
	framePane->GetSuperView()->SetDescriptor(pNewTitle);
	
}

void sun_awt_macos_MFramePeer_pShow(struct Hsun_awt_macos_MFramePeer *framePeerHandle)
{
	Classsun_awt_macos_MFramePeer	*framePeer = unhand(framePeerHandle);
	ClassClass						*embeddedAppletFrameClass;
	bool_t                  		isEmbedded;

	embeddedAppletFrameClass = 	embeddedAppletFrameClass = GetEmbeddedAppletFrameClass();
	isEmbedded = is_instance_of((JHandle *)(framePeer->target), embeddedAppletFrameClass, EE());

	if (!isEmbedded) {
	
		LView		*frameView = (LWindow *)PeerToPane((Hsun_awt_macos_MComponentPeer *)framePeerHandle);

		if (frameView == NULL)
			return;

		LWindow		*frameWindow = (LWindow *)(frameView->GetSuperView());
		
		//	Make sure the target has been validated.
		
		if (!(frameWindow->IsVisible())) {
			
			Hjava_awt_Component *target = framePeer->target;
			
			MToolkitExecutJavaDynamicMethod(target, "validate", "()V");
		
			frameWindow->Show();
			
		}
		
		UDesktop::SelectDeskWindow((LWindow *)(frameView->GetSuperView()));
		
	}
	
	else {
		LView		*frameView = (LWindow *)PeerToPane((Hsun_awt_macos_MComponentPeer *)framePeerHandle);
		if (frameView == NULL)
			return;
		frameView->Show();
	}
		

}

void sun_awt_macos_MFramePeer_pReshape(struct Hsun_awt_macos_MFramePeer *framePeerHandle, long x, long y, long width, long height)
{
	Classsun_awt_macos_MFramePeer	*framePeer = unhand(framePeerHandle);
	Classjava_awt_Frame				*frame = unhand((Hjava_awt_Frame *)(framePeer->target));
	MFramePeer						*framePane = (MFramePeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)framePeerHandle);
	SDimension32					newImageDimension = { width, height };
	Rect							inBounds;
	
	if (height == 0)
		height = 25;
		
	if (width == 0)
		width = 25;
	
	ClearCachedAWTRefocusPane();

	if (framePane == NULL)
		return;
	
	if (framePeer->mFrameIsEmbedded)
		return;

	::SetRect(&inBounds, x, y, x + width, y + height);
	
	//	If we are a window or dialog, then we have to enforce our own width and height.
	
	if ((is_instance_of((JHandle *)framePeerHandle, GetWindowClass(), EE())) ||
		 (is_instance_of((JHandle *)framePeerHandle, GetDialogClass(), EE()))) {
	
		//	If the dialog is at ( 0, 0 ), then reposition to the standard dialog
		//	position (alert position).
		
		if ((x == 0) && (y == 0)) {
		
			GDHandle	mainDeviceHandle;
			Rect		mainDeviceRect;
		
			mainDeviceHandle = GetMainDevice();
			
			mainDeviceRect = (**mainDeviceHandle).gdRect;
			
			OffsetRect(&inBounds, 
				((mainDeviceRect.right - mainDeviceRect.left) / 2 ) - (width / 2),
				((mainDeviceRect.bottom - mainDeviceRect.top) / 3) - (height / 2));
	
		}

	}

	if (x < kMinHorizontalFrameOffset)
		::OffsetRect(&inBounds, kMinHorizontalFrameOffset - x, 0);
				
	LWindow		*containerWindow = (LWindow *)(framePane->GetSuperView());

	if (frame->menuBar == NULL) {

		if (y < kMinVerticalFrameOffset) {
			::OffsetRect(&inBounds, 0, kMinVerticalFrameOffset - y);
		}

	}
	
	else {
	
		if (y < kMinVerticalFrameOffset + MMenuBarPeer::kMenuBarHeight) {
			::OffsetRect(&inBounds, 0, (kMinVerticalFrameOffset + MMenuBarPeer::kMenuBarHeight) - y);
		}
	
	}

	inBounds.bottom += 15;

	if (frame->menuBar != NULL)
		inBounds.top -= MMenuBarPeer::kMenuBarHeight;

	//	Resize the java pane.

	framePane->ResizeImageTo(newImageDimension.width, newImageDimension.height, false);

	//	And its enclosing window.

	containerWindow->DoSetBounds(inBounds);
	
	//	Make sure that our clipping regions get recalculated.
	
	framePeer->mRecalculateClip = true;

}

void sun_awt_macos_MFramePeer_pDispose(struct Hsun_awt_macos_MFramePeer *framePeerHandle)
{
	LPane				*componentPane;
	LWindow				*componentPaneAsWindow;
	
	//	We donÕt want to dispose embedded frames.  We leave this to the browser.

	componentPane = PeerToPane((Hsun_awt_macos_MComponentPeer *)framePeerHandle);
	
	if (componentPane != NULL) {
	
		DisposeJavaDataForPane(componentPane);
		
		if (!(unhand(framePeerHandle))->mFrameIsEmbedded) {
			componentPaneAsWindow = (LWindow *)(componentPane->GetSuperView());
			delete componentPaneAsWindow;
		}
		
		unhand(framePeerHandle)->mOwnerPane = NULL;

	}

}

void sun_awt_macos_MFramePeer_toFront(struct Hsun_awt_macos_MFramePeer *framePeerHandle)
{
	if ((unhand(framePeerHandle))->mFrameIsEmbedded)
		return;

	LView		*frameView = (LWindow *)PeerToPane((Hsun_awt_macos_MComponentPeer *)framePeerHandle);

	if (frameView == NULL)
		return;

	LWindow		*frameWindow = (LWindow *)(frameView->GetSuperView());
	
	UDesktop::SelectDeskWindow(frameWindow);

	if (!(frameWindow->IsVisible())) {
		
		Hjava_awt_Component *target = unhand(framePeerHandle)->target;
		
		MToolkitExecutJavaDynamicMethod(target, "invalidate", "()V"); // FIX ME

		MToolkitExecutJavaDynamicMethod(target, "validate", "()V");
	
		frameWindow->Show();
		
	}

}

void sun_awt_macos_MFramePeer_toBack(struct Hsun_awt_macos_MFramePeer *framePeerHandle)
{
	if ((unhand(framePeerHandle))->mFrameIsEmbedded)
		return;

	LView		*frameView = (LWindow *)PeerToPane((Hsun_awt_macos_MComponentPeer *)framePeerHandle);

	if (frameView == NULL)
		return;

	LWindow		*frameWindow = (LWindow *)(frameView->GetSuperView());
	frameWindow->Hide();
	SendBehind((WindowPtr)(frameWindow->GetMacPort()), NULL);
	frameWindow->Show();

}

static Boolean			gUsingCustomFrameCursor = false;
static CursHandle 		sIBeamCursor = NULL;
static CursHandle 		sCrossCursor = NULL;
static CursHandle 		sPlusCursor = NULL;
static CursHandle 		sWatchCursor = NULL;

static UInt8			sHandCursor[] = {
	0x01, 0x80, 0x1A, 0x70, 0x26, 0x48, 0x26, 0x4A,
	0x12, 0x4D, 0x12, 0x49, 0x68, 0x09, 0x98, 0x01, 
	0x88, 0x02, 0x40, 0x02, 0x20, 0x02, 0x20, 0x04,
	0x10, 0x04, 0x08, 0x08, 0x04, 0x08, 0x04, 0x08,
	0x01, 0x80, 0x1B, 0xF0, 0x3F, 0xF8, 0x3F, 0xFA,
	0x1F, 0xFF, 0x1F, 0xFF, 0x7F, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFE, 0x7F, 0xFE, 0x3F, 0xFE, 0x3F, 0xFC, 
	0x1F, 0xFC, 0x0F, 0xF8, 0x07, 0xF8, 0x07, 0xF8,
	0x00, 0x09, 0x00, 0x08
};

void sun_awt_macos_MFramePeer_setCursor(struct Hsun_awt_macos_MFramePeer *framePeerHandle, long cursor)
{
	gUsingCustomFrameCursor = false;

	switch (cursor) {

		case java_awt_Frame_DEFAULT_CURSOR:
			SetCursor(&(qd.arrow));
			break;

		case java_awt_Frame_CROSSHAIR_CURSOR:
			if (sCrossCursor == NULL)
				sCrossCursor = GetCursor(crossCursor);
			if (sCrossCursor != NULL)
				SetCursor(*sCrossCursor);	
			gUsingCustomFrameCursor = true;
			break;

		case java_awt_Frame_TEXT_CURSOR:
			if (sIBeamCursor == NULL)
				sIBeamCursor = GetCursor(iBeamCursor);
			if (sIBeamCursor != NULL)
				SetCursor(*sIBeamCursor);	
			gUsingCustomFrameCursor = true;
			break;

		case java_awt_Frame_WAIT_CURSOR:
			if (sWatchCursor == NULL)
				sWatchCursor = GetCursor(watchCursor);
			if (sWatchCursor != NULL)
				SetCursor(*sWatchCursor);	
			gUsingCustomFrameCursor = true;
			break;

		case java_awt_Frame_HAND_CURSOR:
			SetCursor((Cursor *)sHandCursor);	
			gUsingCustomFrameCursor = true;
			break;

		case java_awt_Frame_SW_RESIZE_CURSOR:
		case java_awt_Frame_SE_RESIZE_CURSOR:
		case java_awt_Frame_NW_RESIZE_CURSOR:
		case java_awt_Frame_NE_RESIZE_CURSOR:
		case java_awt_Frame_N_RESIZE_CURSOR:
		case java_awt_Frame_S_RESIZE_CURSOR:
		case java_awt_Frame_W_RESIZE_CURSOR:
		case java_awt_Frame_E_RESIZE_CURSOR:
		case java_awt_Frame_MOVE_CURSOR:
		default:
			break;	
	
	}
}

Boolean UsingCustomAWTFrameCursor(void)
{
	return gUsingCustomFrameCursor;
}


void sun_awt_macos_MDialogPeer_create(struct Hsun_awt_macos_MDialogPeer *dialog, struct Hsun_awt_macos_MComponentPeer *hostPeer)
{
	Hjava_awt_Dialog	*targetDialog;
	
	sun_awt_macos_MFramePeer_create((Hsun_awt_macos_MFramePeer *)dialog, hostPeer);
	
	targetDialog = (Hjava_awt_Dialog *)(unhand(dialog)->target);
	
	sun_awt_macos_MDialogPeer_setTitle(dialog, unhand(targetDialog)->title);
	
}

void sun_awt_macos_MDialogPeer_setTitle(struct Hsun_awt_macos_MDialogPeer *dialogPeerHandle,struct Hjava_lang_String *stringHandle)
{
	Classsun_awt_macos_MDialogPeer	*framePeer = unhand(dialogPeerHandle);
	MFramePeer						*framePane = (MFramePeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)dialogPeerHandle);
	Str255							pNewTitle;
	
	if (framePane == NULL)
		return;

	if (framePeer->mFrameIsEmbedded)
		return;
	
	//	Convert the java string into a C string

#ifdef UNICODE_FONTLIST
        short encoding = intl_GetSystemUIEncoding();
        intl_javaString2CString(encoding, stringHandle,
                      (unsigned char *)(pNewTitle + 1), 255);
#else
        javaString2CString(stringHandle, (char *)(pNewTitle + 1), 255);
#endif

	pNewTitle[0] = strlen((char *)(pNewTitle + 1));
	
	//	The PowerPlany window is actually this frameÕs parent,
	//	so pass the message up.
	
	framePane->GetSuperView()->SetDescriptor(pNewTitle);
	
}

void sun_awt_macos_MDialogPeer_setResizable(struct Hsun_awt_macos_MDialogPeer *, long)
{
	// This doesn't (and may never) work...
}

void sun_awt_macos_MWindowPeer_create(struct Hsun_awt_macos_MWindowPeer *window, struct Hsun_awt_macos_MComponentPeer *hostPeer)
{
	sun_awt_macos_MFramePeer_create((Hsun_awt_macos_MFramePeer *)window, hostPeer);
}


