//##############################################################################
//##############################################################################
//
//	File:		MLabelPeer.cp
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
#include <LCaption.h>

extern "C" {

#include <stdlib.h>
#include "native.h"
#include "typedefs_md.h"
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_label.h"
#include "java_awt_Component.h"
#include "sun_awt_macos_MLabelPeer.h"
#include "sun_awt_macos_MComponentPeer.h"

};

#ifdef UNICODE_FONTLIST
#include "LJavaFontList.h"
#include "LMacCompStr.h"
#endif
#include "MLabelPeer.h"
#include "MCanvasPeer.h"

enum {
	kLabelMarginSize	= 4
};

//##############################################################################
//##############################################################################
#pragma mark MComponenPeer IMPLEMENTATION
#pragma mark -


MLabelPeer::
MLabelPeer(struct SPaneInfo &labelInfo, Str255 labelText):LCaption(labelInfo, labelText, 0)
{
}

MLabelPeer::
~MLabelPeer()
{
	DisposeJavaDataForPane(this);
}


Boolean 
MLabelPeer::FocusDraw()
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
MLabelPeer::DrawSelf()
{
	Hsun_awt_macos_MLabelPeer 			*peerHandle = (Hsun_awt_macos_MLabelPeer *)PaneToPeer(this);
	Classsun_awt_macos_MLabelPeer		*peer = unhand(peerHandle);
	Ptr									inText;
	Int32								inLength;
	Int16								inJustification;
	Rect								inRect;
	
	EraseRgn((RgnHandle)(peer->mClipRgn));		
	CalcLocalFrameRect(inRect);

	inJustification = peer->mAlignment;
	
	Fixed	fixedWidth = ::Long2Fix(inRect.right - inRect.left);

	
	Int16	justification = inJustification;
	if (justification == teFlushDefault) {
		justification = ::GetSysDirection();
	}
#ifdef UNICODE_FONTLIST
	LJavaFontList *fontlist = (LJavaFontList *)unhand(PaneToPeer(this))->pInternationalData;
	LMacCompStr string(mText);
	string.DrawJustified(fontlist, inRect, justification);
#else
	inText = (Ptr)&mText[1];
	inLength = mText[0];
	FontInfo	fontInfo;
	::GetFontInfo(&fontInfo);
	Int16 lineHeight = fontInfo.ascent + fontInfo.descent + fontInfo.leading;
	Int16 lineBase = fontInfo.ascent; /* (inRect.bottom + inRect.top + fontInfo.ascent + fontInfo.leading) / 2; */

	Fixed	wrapWidth;
	Int32	blackSpace, lineBytes;
	Int32	textLeft = inLength;
	Ptr		textEnd = inText + inLength;
	StyledLineBreakCode	lineBreak;
	while (inText < textEnd) {
		lineBytes = 1;
		wrapWidth = fixedWidth;
		
		Int32	textLen = textLeft;		// Note in IM:Text 5-80 states
		if (textLen > max_Int16) {		//   that length is limited to the
			textLen = max_Int16;		//   integer range
		}
		
		lineBreak = ::StyledLineBreak(inText, textLen, 0, textLen, 0,
										&wrapWidth, &lineBytes);

		blackSpace = ::VisibleLength(inText, lineBytes);
		
		switch (justification) {
			case teForceLeft:
			case teJustLeft:
				::MoveTo(inRect.left + kLabelMarginSize, lineBase);
				break;

			case teJustRight:
				::MoveTo(inRect.right - ::TextWidth(inText, 0, blackSpace) - kLabelMarginSize,
							lineBase);
				break;

			case teJustCenter:
				::MoveTo(inRect.left + ((inRect.right - inRect.left) -
						::TextWidth(inText, 0, blackSpace)) / 2, lineBase);
				break;
		}
		
		::DrawText(inText, 0, lineBytes);

		lineBase += lineHeight;
		inText += lineBytes;
		textLeft -= lineBytes;
	}
#endif
}


//##############################################################################
//##############################################################################


void sun_awt_macos_MLabelPeer_create(struct Hsun_awt_macos_MLabelPeer *labelPeerHandle, struct Hsun_awt_macos_MComponentPeer *hostPeerHandle)
{
	Classsun_awt_macos_MLabelPeer	*labelPeer = (Classsun_awt_macos_MLabelPeer*)unhand(labelPeerHandle);
	Classjava_awt_Label				*labelTarget = (Classjava_awt_Label*)unhand(labelPeer->target);
	LView							*superView = (LView *)PeerToPane(hostPeerHandle);
	struct SPaneInfo				newLabelInfo;
	Str255							labelText;
	
	memset(&newLabelInfo, 0, sizeof(newLabelInfo));
	
	newLabelInfo.paneID 		= MLabelPeer::class_ID;
	newLabelInfo.visible 		= triState_Off;
	newLabelInfo.left			= labelTarget->x;
	newLabelInfo.top			= labelTarget->y;
	newLabelInfo.width			= labelTarget->width;
	newLabelInfo.height			= labelTarget->height;
	newLabelInfo.enabled 		= triState_Off;
	newLabelInfo.userCon 		= (UInt32)labelPeerHandle;
	newLabelInfo.superView		= superView;
	
#ifdef UNICODE_FONTLIST
	intl_javaString2UnicodePStr(labelTarget->label, labelText);
#else
	javaString2CString(labelTarget->label, (char *)(labelText + 1), 255);
	labelText[0] = strlen((char *)(labelText + 1));
#endif
	
	MLabelPeer *labelPane = NULL;
	
	try {
		labelPane = new MLabelPeer(newLabelInfo, labelText);
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

	labelPeer->mOwnerPane = (long)labelPane;
	labelPeer->mRecalculateClip = true;

	ClearCachedAWTRefocusPane();
}

void sun_awt_macos_MLabelPeer_setText(struct Hsun_awt_macos_MLabelPeer *labelPeerObjectHandle, struct Hjava_lang_String *labelTextObjectHandle)
{
	MLabelPeer						*labelPane = (MLabelPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)labelPeerObjectHandle);
	Str255							labelText;
	
	if (labelPane == NULL)
		return;

	//	Convert the java string into a C string

#ifdef UNICODE_FONTLIST
	intl_javaString2UnicodePStr(labelTextObjectHandle, labelText);
#else
	javaString2CString(labelTextObjectHandle, (char *)(labelText + 1), 255);
	labelText[0] = strlen((char *)(labelText + 1));
#endif
	
	labelPane->SetDescriptor(labelText);
	
	InvalidateComponent(labelPane, false);

}

void sun_awt_macos_MLabelPeer_setAlignment(struct Hsun_awt_macos_MLabelPeer *labelPeerHandle, long alignmentOptions)
{
	Classsun_awt_macos_MLabelPeer	*labelPeer = (Classsun_awt_macos_MLabelPeer*)unhand(labelPeerHandle);
	MLabelPeer						*labelPane = (MLabelPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)labelPeerHandle);

	if (labelPane == NULL)
		return;

	//	Map the AWT alignment option to the corresponding 
	//	TextEdit constant.

	switch (alignmentOptions) {
	
		case 0:
			labelPeer->mAlignment = teJustLeft;
			break;
			
		case 1:
			labelPeer->mAlignment = teJustCenter;
			break;
			
		case 2:
			labelPeer->mAlignment = teJustRight;
			break;
			
		default:
			break;
	
	}

	if (labelPane != NULL)
		InvalidateComponent(labelPane, false);

}
