//##############################################################################
//##############################################################################
//
//	File:		MCanvasPeer.cp
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
#include "java_awt_Canvas.h"
#include "java_awt_Component.h"
#include "sun_awt_macos_MComponentPeer.h"
#include "sun_awt_macos_MCanvasPeer.h"

};

#include "uapp.h"

#include "MCanvasPeer.h"
#include "MToolkit.h"

//##############################################################################
//##############################################################################
#pragma mark MComponenPeer IMPLEMENTATION
#pragma mark -

MCanvasPeer::
MCanvasPeer(struct SPaneInfo &newScrollbarInfo, struct SViewInfo &newCanvasViewInfo, LCommander *inSuper):
	LView(newScrollbarInfo, newCanvasViewInfo), LCommander(inSuper)
{
}

MCanvasPeer::
~MCanvasPeer()
{
	DisposeJavaDataForPane(this);
}

Boolean 
MCanvasPeer::FocusDraw()
{
	Boolean	shouldRefocus = ShouldRefocusAWTPane(this);
	Boolean  result = false;
	result = LView::FocusDraw();
	if (shouldRefocus && result)
		PreparePeerGrafPort(PaneToPeer(this), NULL);
	return result;
}

void
MCanvasPeer::DrawSelf()
{
	MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "handleExpose", "()V");
}

void
MCanvasPeer::BeTarget()
{
	SetJavaTarget(PaneToPeer(this));
	MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "gotFocus", "()V");
}

void
MCanvasPeer::DontBeTarget()
{
	MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "lostFocus", "()V");
	SetJavaTarget(NULL);
}


//##############################################################################
//##############################################################################


void sun_awt_macos_MCanvasPeer_create(struct Hsun_awt_macos_MCanvasPeer *canvasPeerHandle, struct Hsun_awt_macos_MComponentPeer *hostPeerHandle)
{
	Classsun_awt_macos_MCanvasPeer		*canvasPeer = (Classsun_awt_macos_MCanvasPeer*)unhand(canvasPeerHandle);
	Classjava_awt_Canvas				*canvasTarget = (Classjava_awt_Canvas*)unhand(canvasPeer->target);
	LView								*superView = (LView *)PeerToPane(hostPeerHandle);
	MCanvasPeer							*canvasPane = NULL;
	SPaneInfo							newCanvasInfo;
	SViewInfo							newCanvasViewInfo;
	ResIDT								textTraitsId = 0;
	
	//	Force our parent to recalculate its clipping information
	//	and that of its children.
	
	memset(&newCanvasInfo, 0, sizeof(newCanvasInfo));
	
	newCanvasInfo.paneID 		= MCanvasPeer::class_ID;
	newCanvasInfo.visible		= triState_Off;
	newCanvasInfo.left			= canvasTarget->x;
	newCanvasInfo.top			= canvasTarget->y;
	newCanvasInfo.width			= canvasTarget->width;
	newCanvasInfo.height		= canvasTarget->height;
	newCanvasInfo.enabled		= triState_Off;
	newCanvasInfo.userCon 		= (long)canvasPeerHandle;
	newCanvasInfo.superView		= superView;
	
	memset(&newCanvasViewInfo, 0, sizeof(newCanvasViewInfo));
	
	newCanvasViewInfo.imageSize.width 	= canvasTarget->width;
	newCanvasViewInfo.imageSize.height 	= canvasTarget->height;
	
	try {
		canvasPane = new MCanvasPeer(newCanvasInfo, newCanvasViewInfo, GetContainerWindow(superView));
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

	canvasPane->Enable();
	canvasPane->Activate();

	canvasPeer->mOwnerPane = (long)canvasPane;
	canvasPeer->mRecalculateClip = true;
	canvasPeer->mIsContainer = true;
	
	InvalidateParentClip((Hsun_awt_macos_MComponentPeer *)canvasPeerHandle);

	ClearCachedAWTRefocusPane();
	
}
