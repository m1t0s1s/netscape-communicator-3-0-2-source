//##############################################################################
//##############################################################################
//
//	File:		MComponentPeer.cp
//	Author:		Dan Clifford
//	
//	Copyright © 1995-1996, Netscape Communications Corporation
//
//##############################################################################
//##############################################################################

#pragma mark INCLUDES

#include <LPane.h>

#include <stdlib.h>

extern "C" {

#include "native.h"
#include "typedefs_md.h"
#include "java_awt_Color.h"
#include "java_awt_Font.h"
#include "java_awt_Event.h"
#include "java_awt_Component.h"
#include "java_awt_Frame.h"
#include "sun_awt_macos_MComponentPeer.h"
#include "sun_awt_macos_MChoicePeer.h"
#include "sun_awt_macos_MFramePeer.h"
#include "sun_awt_macos_MacintoshEvent.h"

#ifdef UNICODE_FONTLIST
#include "libi18n.h"	
#endif

};

#include <Controls.h>
#include <LCommander.h>

#include "MComponentPeer.h"
#include "LMacGraphics.h"
#include "MChoicePeer.h"
#include "MToolkit.h"
#include "MButtonPeer.h"
#include "MTextAreaPeer.h"
#include "MTextFieldPeer.h"
#include "MCanvasPeer.h"
#include "MListPeer.h"
#include "MMenuBarPeer.h"
#include <typeinfo.h>

#ifdef UNICODE_FONTLIST
#include "mhyper.h"	
#include "cppcntxt.h"	
#include "mcontext.h"	
#endif


//##############################################################################
//##############################################################################
#pragma mark MComponenPeer IMPLEMENTATION
#pragma mark -

LPane	*gCurrentAWTFocusPane = NULL;

LWindow GetComponentWindow()
{

}

Boolean ShouldRefocusAWTPane(LPane *newFocusPane)
{
	return (newFocusPane != MCanvasPeer::GetFocusedPane());	
}

void ClearCachedAWTRefocusPane(void)
{
	LView::OutOfFocus(NULL);
}

void SetJavaTarget(Hsun_awt_macos_MComponentPeer *currentTarget)
{
	if (currentTarget != NULL)
		gTotalClickCount = 0;
	gCurrentJavaTarget = currentTarget;
}

Hsun_awt_macos_MComponentPeer *GetJavaTarget(void)
{
	return gCurrentJavaTarget;
}

CCTabHandle		gControlColorTable = NULL;
RgnHandle		gTempComponentPeerRgn = NULL;

Boolean 	gInPaneFocusDraw = false;

void
InvalidateComponent(LPane *componentPane, Boolean hasFrame)
{
	Hsun_awt_macos_MComponentPeer		*componentPeer = PaneToPeer(componentPane);
	Rect								tempRect;
	Rect								localFrameRect;
	
	if (!(componentPane->IsVisible()))
		return;
		
	if (componentPeer == gCurrentMouseTargetPeer)
		gCurrentMouseTargetPeer = NULL;
	
	componentPane->FocusDraw();
	if ((componentPeer != NULL) && (!hasFrame)) {
		componentPane->CalcLocalFrameRect(localFrameRect);
		if ((unhand(componentPeer))->mClipRgn != 0)
			::InvalRgn((RgnHandle)((unhand(componentPeer))->mClipRgn));
		else
			::InvalRect(&localFrameRect);
	}
	else {
		componentPane->CalcLocalFrameRect(tempRect);
		if (hasFrame)
			::InsetRect(&tempRect, 1, 1);
		::InvalRect(&tempRect);	
	}

}

RGBColor SwitchInControlColors(LStdControl *theControl)
{
	Hsun_awt_macos_MComponentPeer	*parentPeerHandle,
									*componentPeerHandle;
	Classsun_awt_macos_MComponentPeer *componentPeer,
									*parentPeer;
	ControlHandle 					controlHandle = theControl->GetMacControl();
	AuxWinHandle					awHandle;
	CTabHandle						awColorTable;
	ColorTable						*awColorTablePtr;
	RGBColor						backgroundColor = { 221 << 8, 221 << 8, 221 << 8 };
	RGBColor						savedColor;
	long							currentColorNum;
	
	//	Calculate the correct background color for the control
	//	by looking at its container peer’s background.
	
	componentPeerHandle = PaneToPeer(theControl);
	if (componentPeerHandle == NULL)
		return backgroundColor;
	componentPeer = unhand(componentPeerHandle);

	parentPeerHandle = GetComponentParent(componentPeerHandle);
	if (parentPeerHandle == NULL)
		return backgroundColor;
	parentPeer = unhand(parentPeerHandle);
	
	if (componentPeer->mBackColor != NULL)
		backgroundColor = ConvertAWTToMacColor(componentPeer->mBackColor);
	else
		if (parentPeer->mBackColor != NULL)
			backgroundColor = ConvertAWTToMacColor(parentPeer->mBackColor);

	//	Get the color table for the window

	GetAuxWin((**controlHandle).contrlOwner, &awHandle);
	awColorTable = (**awHandle).awCTable;
	awColorTablePtr = *awColorTable;

	//	Save away and replace the background entry in the window’s color table.
	
	for (currentColorNum = 0; currentColorNum < awColorTablePtr->ctSize; currentColorNum++) {
		if (awColorTablePtr->ctTable[currentColorNum].value == wContentColor) {
			savedColor = awColorTablePtr->ctTable[currentColorNum].rgb;
			awColorTablePtr->ctTable[currentColorNum].rgb = backgroundColor;
		}
	}
	
	return savedColor;
	 
}

void
SwitchOutControlColors(LStdControl *theControl, RGBColor savedColor)
{

	ControlHandle 		controlHandle = theControl->GetMacControl();
	AuxWinHandle		awHandle;
	CTabHandle			awColorTable;
	ColorTable			*awColorTablePtr;
	long				currentColorNum;

	GetAuxWin((**controlHandle).contrlOwner, &awHandle);
	awColorTable = (**awHandle).awCTable;
	awColorTablePtr = *awColorTable;
	
	for (currentColorNum = 0; currentColorNum < awColorTablePtr->ctSize; currentColorNum++) {
	
		if (awColorTablePtr->ctTable[currentColorNum].value == wContentColor) {
		
			awColorTablePtr->ctTable[currentColorNum].rgb = savedColor;
			break;

		}
		
	}

}

void DisposeJavaDataForPane(LPane *deadPane)
{

	Hsun_awt_macos_MComponentPeer *componentPeerHandle = PaneToPeer(deadPane);

	if (componentPeerHandle != NULL) {

		Classsun_awt_macos_MComponentPeer *componentPeer = unhand(componentPeerHandle);

		//	Clear the interface queue of events related to the component to help garbage collection.
		
		ClassClass	*interfaceEventClass;
		
		interfaceEventClass = FindClass(EE(), "sun/awt/macos/InterfaceEvent", TRUE);
		
		if (interfaceEventClass != NULL) {
			MToolkitExecutJavaStaticMethod(interfaceEventClass, "flushInterfaceQueue", "(Lsun/awt/macos/MComponentPeer;)V", componentPeerHandle);
		}
		
		if (unhand(componentPeerHandle)->mClipRgn != NULL)
			 DisposeRgn((RgnHandle)(unhand(componentPeerHandle)->mClipRgn));

		if (unhand(componentPeerHandle)->mObscuredRgn != NULL)
			DisposeRgn((RgnHandle)(unhand(componentPeerHandle)->mObscuredRgn));

		unhand(componentPeerHandle)->mClipRgn = NULL;
		unhand(componentPeerHandle)->mObscuredRgn = NULL;
		unhand(componentPeerHandle)->mOwnerPane = NULL;

		deadPane->SetUserCon(0);

		ClearCachedAWTRefocusPane();
		
 		if (componentPeerHandle == gCurrentMouseTargetPeer)
			gCurrentMouseTargetPeer = NULL;
	
     }

}

void PreparePeerGrafPort(Hsun_awt_macos_MComponentPeer *componentPeerHandle, ControlHandle peerControlHandle)
{
	Classsun_awt_macos_MComponentPeer	*componentPeer;
	LPane								*pane;
	RGBColor							foreColor = { 0, 0, 0 },
										backColor = { 0xFFFF, 0xFFFF, 0xFFFF };
	
	if (gInPaneFocusDraw)
		return;
	
	if (componentPeerHandle == NULL)
		return;
		
	pane = PeerToPane(componentPeerHandle);

	if (pane == NULL)
		return;

	componentPeer = unhand(componentPeerHandle);

#ifdef UNICODE_FONTLIST
	SetUpTextITraits(componentPeer->mFont, componentPeer->mSize, componentPeer->mStyle,
		intl_GetWindowEncoding(componentPeerHandle));
	if((componentPeer->mFont != 0) && (componentPeer->pInternationalData != 0))
	{
		LJavaFontList *fontlist = (LJavaFontList *)componentPeer->pInternationalData;
		fontlist->SetTextFont(intl_GetWindowEncoding(componentPeerHandle));
	}
#else
	SetUpTextITraits(componentPeer->mFont, componentPeer->mSize, componentPeer->mStyle);
#endif

	//	If we are a text field, we have to do some special work to set up the text traits.
	
	if (pane->GetPaneID() == MTextFieldPeer::class_ID) {
	
		MTextFieldPeer			*textPane = (MTextFieldPeer *)pane;
		TextTraitsRecord		textFieldTraits;
	
		textFieldTraits.size = componentPeer->mSize;
		textFieldTraits.style = componentPeer->mStyle;
		textFieldTraits.fontNumber = componentPeer->mFont;
		textFieldTraits.justification = teJustLeft;
		textFieldTraits.mode = srcCopy;
		textFieldTraits.fontName[0] = 0;
	
		UTextTraits::SetTETextTraits(&textFieldTraits, textPane->GetMacTEH());

	}

	//	The same is true if we are a text area.

	else if (pane->GetPaneID() == MTextAreaPeer::class_ID) {
	
		MTextAreaPeer			*textPane = (MTextAreaPeer *)pane;
		MTextEditPeer			*textEditPeer;
		TextTraitsRecord		textFieldTraits;

		textEditPeer = (MTextEditPeer *)((LView *)textPane)->FindPaneByID(MTextEditPeer::class_ID);
	
		textFieldTraits.size = componentPeer->mSize;
		textFieldTraits.style = componentPeer->mStyle;
		textFieldTraits.fontNumber = componentPeer->mFont;
		textFieldTraits.justification = teJustLeft;
		textFieldTraits.mode = srcCopy;
		textFieldTraits.fontName[0] = 0;
	
		UTextTraits::SetTETextTraits(&textFieldTraits, textEditPeer->GetMacTEH());

	}

	//	If there is a control corresponding to the peer, then set up
	//	its control color table apporpriately so that colors show
	//	up correctly.
	
	if (peerControlHandle != NULL) {
	
		if (gControlColorTable == NULL)
			gControlColorTable = (CCTabHandle)NewHandleClear(sizeof(CtlCTab));
	
		if (gControlColorTable != NULL) {
	
			CCTabPtr	controlColorTablePtr = *gControlColorTable;
	
			controlColorTablePtr->ctSize = 3;

			controlColorTablePtr->ctTable[0].value = cFrameColor;
			controlColorTablePtr->ctTable[0].rgb = foreColor;
			controlColorTablePtr->ctTable[1].value = cTextColor;
			controlColorTablePtr->ctTable[1].rgb = foreColor;
			controlColorTablePtr->ctTable[2].value = cBodyColor;
			controlColorTablePtr->ctTable[2].rgb = backColor;
			
			RGBColor savedColor;
			savedColor = SwitchInControlColors((LStdControl *)pane);
			SetControlColor(peerControlHandle, gControlColorTable);
			SwitchOutControlColors((LStdControl *)pane, savedColor);
			
		}
	
	}

	//	Make sure that the clip region for the component is current,
	//	and make sure that we are clipping to it.
	
	CalculateComponentClipRgn(componentPeerHandle);
		
	if (gTempComponentPeerRgn == NULL)
		gTempComponentPeerRgn = ::NewRgn();

	//	Clip to the view rectangle of the 'java' view above us.

	if (gTempComponentPeerRgn != NULL) {
	
		LView		*ourJavaSuperView = pane->GetSuperView();
		Rect		superViewRect;
		GrafPtr		currentPort;
		
		while (1) {
		
			if (ourJavaSuperView == NULL)
				break; 
				
			if (ourJavaSuperView->GetPaneID() == 'java')
				break;
				
			ourJavaSuperView = ourJavaSuperView->GetSuperView();
		
		}
		
		if (ourJavaSuperView != NULL) {

			ourJavaSuperView->GetRevealedRect(superViewRect);

			pane->PortToLocalPoint(topLeft(superViewRect));
			pane->PortToLocalPoint(botRight(superViewRect));
			
			::RectRgn(gTempComponentPeerRgn, &superViewRect);
			::SectRgn((RgnHandle)(componentPeer->mClipRgn), gTempComponentPeerRgn, gTempComponentPeerRgn);

			::SetClip(gTempComponentPeerRgn);

		}
		
		else {
		
			::SetClip((RgnHandle)(componentPeer->mClipRgn));
		
		}
		
	}

	//	Set up the foreground and background colors as well as
	//	the font, style and size information for the port.	
	
	::PenNormal();

	if (componentPeer->mForeColor != NULL) {
		foreColor = ConvertAWTToMacColor(componentPeer->mForeColor);
		::RGBForeColor(&foreColor);
	}
	
	if (componentPeer->mBackColor != NULL) {
		backColor = ConvertAWTToMacColor(componentPeer->mBackColor);
		::RGBBackColor(&backColor);
	}

}

Hsun_awt_macos_MComponentPeer *PaneToPeer(const LPane *pane)
{
	if (pane == NULL)
		return NULL;
		
	return (Hsun_awt_macos_MComponentPeer *)(pane->GetUserCon());
}

LPane *PeerToPane(Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	long		ownerPane;

	if (componentPeerHandle == NULL)
		return NULL;
	
	ownerPane = unhand(componentPeerHandle)->mOwnerPane;
		
	if (ownerPane == kJavaComponentPeerHasBeenDisposed)
		return NULL;
	
	return (LPane *)(ownerPane);
}


Hsun_awt_macos_MComponentPeer *GetComponentParent(Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	Classsun_awt_macos_MComponentPeer		*componentPeer = unhand(componentPeerHandle);
	Hsun_awt_macos_MComponentPeer			*parentPeerHandle = NULL;
	LPane									*componentPane,
											*parentPane;
	
	componentPane = (LPane *)componentPeer->mOwnerPane;
	
	parentPane = componentPane->GetSuperView();
	
	if (parentPane != NULL)
		parentPeerHandle = PaneToPeer(parentPane);

	return parentPeerHandle;

}

void OverlapBindToTopLevelFrame(Hsun_awt_macos_MComponentPeer *componentPeerHandle, Hsun_awt_macos_MComponentPeer *parentPeerHandle, long *x, long *y, long *width, long *height)
{	
	Hjava_awt_Component				*parentComponentHandle;
	Classjava_awt_Component			*parentComponent;
	ClassClass						*topLevelFrameClass;
	Boolean							isTopLevel;
	
	if (componentPeerHandle == NULL)
		return;

	//	If our parent is a top-level frame, then we should bump our frame up a pixel
	//	for aesthetic purposes:  When an edge is immediately adjacent to the
	//	parent’s edge, expand so that we inherit the parent’s edge.

	if (parentPeerHandle == NULL)
		parentPeerHandle = GetComponentParent(componentPeerHandle);

	if (parentPeerHandle == NULL)
		return;

	parentComponentHandle = unhand(parentPeerHandle)->target;
	parentComponent = unhand(parentComponentHandle);

	topLevelFrameClass = FindClass(0, "java/awt/Frame", (bool_t)0);
	isTopLevel = is_instance_of((JHandle *)parentComponentHandle, topLevelFrameClass, EE());
	
	if (isTopLevel) {
	
		if ((*x + *width) == parentComponent->width) 
			(*width)++;

		if (*x == 0) {
			(*x)--;
			(*width)++;
		}
		
		if ((*y + *height) == parentComponent->height) 
			(*height)++;
	
		if (*y == 0) {
			(*y)--;
			(*height)++;
		}
	
	}

}

#ifdef UNICODE_FONTLIST
static long intl_javaStringCommon(Hjava_lang_String *javaStringHandle, unicode**    uniChars)
{
	// Many awt routines seem to use the convention that NULL pointers to Java strings
	// should be treated as empty strings.  This routine preserves that convention.
	Classjava_lang_String* javaStringClass = unhand( javaStringHandle );
	HArrayOfChar* unicodeArrayHandle = (HArrayOfChar *)javaStringClass->value;
	*uniChars =  unhand(unicodeArrayHandle)->body + javaStringClass->offset;
	return javaStringClass->count;
}
void intl_javaString2CString(int encoding , Hjava_lang_String *javaStringHandle,
									 unsigned char *out, int buflen)
{
	long        length = 0;
	unicode     emptyString = 0;
	unicode*    uniChars = &emptyString;
	if( javaStringHandle != NULL )
		length = intl_javaStringCommon(javaStringHandle, &uniChars);
	INTL_UnicodeToStr(encoding, uniChars, length,  out, buflen);
}
int intl_javaString2CEncodingString(Hjava_lang_String *javaStringHandle,
									 unsigned char *out, int buflen)
{
	long        length = 0;
	unicode     emptyString = 0;
	unicode*    uniChars = &emptyString;
	if( javaStringHandle != NULL )
		length = intl_javaStringCommon(javaStringHandle, &uniChars);
	return INTL_UnicodeToEncodingStr(uniChars, length,  out, buflen);
}
//
//	unicodePStr - Store a unicode string into the Str255 format
// 	Byte 0			length
//  Byte 1			0
//  Byte 2..255		ucs2
//
void intl_javaString2UnicodePStr(Hjava_lang_String *javaStringHandle, Str255 unicodeInPStr)
{
	long        length = 0;
	unicode     emptyString = 0;
	unicode*    uniChars = &emptyString;
	if( javaStringHandle != NULL )
		length = intl_javaStringCommon(javaStringHandle, &uniChars);
	if(length > (0x00FF >> 1))
		length = (0x00FF >> 1);
	unicodeInPStr[0] = (length << 1) + 1;
	unicodeInPStr[1] = '\0';	
	BlockMoveData( uniChars,   &unicodeInPStr[2], unicodeInPStr[0] - 1);
}
char * intl_allocCString(int encoding , Hjava_lang_String *javaStringHandle) 
{
	long        length;
	unicode     emptyString = 0;
	unicode*    uniChars = &emptyString;
	if( javaStringHandle != NULL )
		length = intl_javaStringCommon(javaStringHandle, &uniChars);
	long buflen = INTL_UnicodeToStrLen(encoding, uniChars, length);
    unsigned char * buf = (unsigned char *)sysMalloc(buflen);
    if (buf == 0) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return 0;
	}
	INTL_UnicodeToStr(encoding, uniChars, length, buf, buflen);
	return (char*)buf;
}


#define kMainHyperViewPaneID		1004
#define kMultiFrameHyperViewPaneID	2005

short intl_GetSystemUIEncoding()
{
	return INTL_CharSetNameToID(INTL_ResourceCharSet()) & ~CS_AUTO;
}

short intl_GetDefaultEncoding()
{
	return INTL_DefaultWinCharSetID(0);
}


short intl_GetWindowEncoding(Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	LView* oldsup = NULL;
	for(LView* sup = (LView *)PeerToPane(componentPeerHandle);
		(sup != NULL) && (sup != oldsup);
		oldsup = sup, sup = sup->GetSuperView())
	{
		switch(sup->GetPaneID())
		{
			case kMainHyperViewPaneID:
			case kMultiFrameHyperViewPaneID:
				CHyperView* hview= (CHyperView*) sup;
				return hview->GetWinCsid();
			default:
				break;
		}
	}
	return intl_GetDefaultEncoding();
}
#endif

void InvalidateParentClip(Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	Hsun_awt_macos_MComponentPeer 	*parentPeerHandle;
	
	parentPeerHandle = GetComponentParent(componentPeerHandle);
	if (parentPeerHandle != NULL)
		(unhand(parentPeerHandle))->mRecalculateClip = true;	
}

RgnHandle GetComponentRegion(Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	Classsun_awt_macos_MComponentPeer	*componentPeer = unhand(componentPeerHandle);
	RgnHandle							componentRegion;
	Rect								componentRect;
	LPane								*ownerPane;
	
	componentRegion = ::NewRgn();

	ownerPane = PeerToPane(componentPeerHandle);
	ownerPane->CalcLocalFrameRect(componentRect);

	if (componentPeer->mIsChoice) {
	
		MChoicePeer		*choiceMenuPane;
		RgnHandle		tempRgn;
	
		//	In order to get the component region for 
		//	a popup menu, we need to get the size of the 
		//	menu associated with the popup.  This will
		//	give us the size of the component box.
		
		short width;
		
		choiceMenuPane = (MChoicePeer *)PeerToPane(componentPeerHandle);
		choiceMenuPane->CalculateMenuDimensions(width);
		
		componentRect.right = componentRect.left + width + kChoicePeerDefaultExtraWidth;
		componentRect.bottom = componentRect.top + kChoicePeerDefaultHeight;

		::RectRgn(componentRegion, &componentRect);

		tempRgn = NewRgn();
		
		::OffsetRect(&componentRect, 1, 1);
		componentRect.top += 2;
		componentRect.left += 2;
		::RectRgn(tempRgn, &componentRect);

		::UnionRgn(componentRegion, tempRgn, componentRegion);

		::DisposeRgn(tempRgn);

	}
	
	else if (componentPeer->mIsButton == kButtonPeerIsStandard) {

		SInt16		arcWidth;

		arcWidth = (componentRect.bottom - componentRect.top) >> 1;

		OpenRgn();
		FrameRoundRect(&componentRect, arcWidth, arcWidth);
		CloseRgn(componentRegion);
		
	}
	
	else if (componentPeer->mIsButton == kButtonPeerIsRadio) {

		::OffsetRect(&componentRect, 2, 2);

		::OpenRgn();
		::FrameOval(&componentRect);
		::CloseRgn(componentRegion);

	}
	
	else {
	
		::RectRgn(componentRegion, &componentRect);
			
	}

	//	If we are a resizable frame and we don't have a warning string
	//	(the grow box is in in the java frame), then make sure to obscure the
	//	grow box at the bottom right-hand corner of the frame.
	
	if (componentPeer->mIsResizableFrame) {
	
		Classsun_awt_macos_MFramePeer	*framePeer = (Classsun_awt_macos_MFramePeer *)componentPeer;
		Classjava_awt_Frame				*frameTarget = (Classjava_awt_Frame*)unhand(framePeer->target);
	
/*
		if (frameTarget->warningString == NULL) {
		
			Rect				growRect;
			SDimension16		frameSize;

			LWindow				*containerWindow = (LWindow *)(ownerPane->GetSuperView());
			LPane 				*menuBarPane = containerWindow->FindPaneByID(MMenuBarPeer::class_ID);

			ownerPane->GetFrameSize(frameSize);
			::SetRect(&growRect, frameSize.width - 15, frameSize.height - 15, frameSize.width, frameSize.height);
			::RectRgn((RgnHandle)componentPeer->mObscuredRgn, &growRect);

		}
*/
	
	}
	
	return componentRegion;

}

void CalculateComponentClipRgn(Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	Hsun_awt_macos_MComponentPeer			*parentPeerHandle;
	Classsun_awt_macos_MComponentPeer		*componentPeer = unhand(componentPeerHandle);
	Rect									wideOpenRect = { -24000, -24000, 24000, 24000 };
	Point									subPanePositionPoint;
	LPane									*ownerPane;
	RgnHandle								obscuredRgn;
	
	//	The default clip region is the intersection
	//	of our obscured region and our component region.
	
	ownerPane = PeerToPane(componentPeerHandle);
	
	if ((!(componentPeer->mRecalculateClip)) || (ownerPane == NULL))
		return;
		
	componentPeer->mRecalculateClip = false;

	//	Check to see if our parent needs clip recalculation.
	//	If so, then reclip our parent first.
	
	parentPeerHandle = GetComponentParent(componentPeerHandle);
	
	if (parentPeerHandle != NULL)
		CalculateComponentClipRgn(parentPeerHandle);

	//	If we already have a clip region, then we use it as the
	//	starting point for the new obscured region.  This
	//	saves us needlessly calling ::DisposeRgn on the clip
	//	and then immediately ::NewRgn for the obscured rgn. 

	if (componentPeer->mClipRgn != NULL) {
		obscuredRgn = (RgnHandle)componentPeer->mClipRgn;
		SetEmptyRgn(obscuredRgn);
	} else
		obscuredRgn = ::NewRgn();
		
	componentPeer->mClipRgn = (long)GetComponentRegion(componentPeerHandle);

	//	The obscured region to begin with (for this components)
	//	children) is the wide open rgn minus the component's own 
	//  obscured rgn, minus the components clip.  Also, the clip
	//	rgn for the component is its component region minus
	//	the region obsured by its parent and peers.

	::DiffRgn((RgnHandle)componentPeer->mClipRgn, 
			(RgnHandle)componentPeer->mObscuredRgn, 
			(RgnHandle)componentPeer->mClipRgn);

	if (componentPeer->mIsContainer) {
	
		::RectRgn(obscuredRgn, &wideOpenRect);
		::DiffRgn(obscuredRgn, (RgnHandle)componentPeer->mClipRgn, obscuredRgn);

		LView			*parentPaneAsView = (LView *)ownerPane;
		LListIterator	iterator(parentPaneAsView->GetSubPanes(), iterate_FromStart);
		LPane			*theSub;

		while (iterator.Next(&theSub)) {
		
			Hsun_awt_macos_MComponentPeer		*siblingPeerHandle;
			Classsun_awt_macos_MComponentPeer	*siblingPeer;
			RgnHandle							componentRegion;
			
			siblingPeerHandle = PaneToPeer(theSub);
		
			//	Since the obscured region for our children
			//	is changing, then we have to mark our kids
			//	as invalid so that they re-do they clips too.
		
			if (siblingPeerHandle == NULL)
				continue;
				
			siblingPeer = unhand(siblingPeerHandle);
			siblingPeer->mRecalculateClip = true;
			
			//	Set the obscured region for the sub pane, and
			//	force the sub pane to recalculate its clipping
		
			::CopyRgn(obscuredRgn, (RgnHandle)(siblingPeer->mObscuredRgn));

			//	Make sure that the obscured region is in local
			//	coordinates for the pane/view.

			subPanePositionPoint.v = subPanePositionPoint.h = 0;
			ownerPane->LocalToPortPoint(subPanePositionPoint);
			theSub->PortToLocalPoint(subPanePositionPoint);
			
			::OffsetRgn((RgnHandle)(siblingPeer->mObscuredRgn), 
					subPanePositionPoint.h, subPanePositionPoint.v);
			
			//	If the pane is not visible, then don’t calculate 
			//	its clipping information
			
			LPane	*componentPane;
		
			componentPane = PeerToPane(siblingPeerHandle);
			if ((componentPane == NULL) || !(componentPane->IsVisible()))
				continue;
		
			//	Subtract the sub-pane’s region from our 
			//	clip region.
			
			componentRegion = GetComponentRegion(siblingPeerHandle);
			
			//	Make sure the sub-component is in local coordinates
			//	to the parent pane.
			
			::OffsetRgn(componentRegion, -subPanePositionPoint.h, -subPanePositionPoint.v);

			::DiffRgn((RgnHandle)componentPeer->mClipRgn, componentRegion, (RgnHandle)componentPeer->mClipRgn);
			::UnionRgn(obscuredRgn, componentRegion, obscuredRgn);
			::DisposeRgn(componentRegion);
			
		}
			
	}
	
	::DisposeRgn(obscuredRgn);
	
}


void RemoveAllSubpanes(LView *containerPane)
{
	while (1) {
		LList 	subPaneList = containerPane->GetSubPanes();
		LPane	*currentPane;

		if (subPaneList.GetCount() == 0) break;
		subPaneList.FetchItemAt(1, &currentPane);
		containerPane->RemoveSubPane(currentPane);
	}
}

void NewLinesToReturns(char *textPtr, long textSize)
{
	while (textSize--) {
		if (*textPtr == '\n')
			*textPtr = '\r';
		textPtr++;		
	}
}

void ReturnsToNewLines(char *textPtr, long textSize)
{
	while (textSize--) {
		if (*textPtr == '\r')
			*textPtr = '\n';
		textPtr++;		
	}
}

void DumpPaneHierarchyHelper(LPane *somePane, LPane *paneToMark, UInt32 level)
{
	for (UInt32	tab=0; tab<level; tab++)
		printf("\t");
	
	UInt32	paneID = somePane->GetPaneID();
	
	if (((paneID >= 1000) && (paneID <= 3000)) || (paneID == 'cvpr') || (paneID == 'GrBv') || (paneID == 'java'))
		printf("VIEW: ");
	else
		printf("PANE: ");
	
	
	if ((((paneID & 0xFF000000) >> 24) >= 'A') && (((paneID & 0xFF000000) >> 24) <= 'z'))
		printf("(ID = '%c%c%c%c') ", paneID >> 24, ((paneID >> 16) & 0x000000FF), ((paneID >> 8) & 0x000000FF), ((paneID >> 0) & 0x000000FF));
	else
		printf("(ID = 0x%lx) ", paneID);
	
	printf("@ 0x%lx ", somePane);
	
	if (paneToMark == somePane)
		printf("*** ");
		
	printf("\n");
	
	if (((paneID >= 1000) && (paneID <= 3000)) || (paneID == 'cvpr') || (paneID == 'GrBv') || (paneID == 'java')) {
	
		LView	*parentPaneAsView = (LView *)somePane;
	
		LListIterator	iterator(parentPaneAsView->GetSubPanes(), iterate_FromStart);
		LPane			*theSub;

		while (iterator.Next(&theSub))
			DumpPaneHierarchyHelper(theSub, paneToMark, level + 1);

	}
		
}

void DumpPaneHierarchy(LPane *somePane, LPane *paneToMark)
{
	while (somePane->GetSuperView())
		somePane = somePane->GetSuperView();

	DumpPaneHierarchyHelper(somePane, paneToMark, 0);
}

//##############################################################################
//##############################################################################

void sun_awt_macos_MComponentPeer_pSetup(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	Classsun_awt_macos_MComponentPeer		*componentPeer = unhand(componentPeerHandle);
	
	componentPeer->mFont = 0;
	componentPeer->mSize = 0;
	componentPeer->mStyle = 0;
	
	componentPeer->mRecalculateClip = true;
	
	componentPeer->mClipRgn = NULL;
	componentPeer->mObscuredRgn = (long)::NewRgn();
	
	componentPeer->mIsResizableFrame = false;

	componentPeer->mIsContainer = false;
	componentPeer->mIsButton = false;
	componentPeer->mIsChoice = false;
	
	componentPeer->pInternationalData = NULL;

	try {
		componentPeer->pInternationalData = (long) new LJavaFontList();
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

}

void sun_awt_macos_MComponentPeer_pShow(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	Hsun_awt_macos_MComponentPeer			*parentPeerHandle;
	Classsun_awt_macos_MComponentPeer		*componentPeer = unhand(componentPeerHandle),
											*parentPeer;
	LPane									*componentPane;
	Boolean									wasVisible;

	if (componentPeerHandle == NULL)
		return;
		
	componentPane = PeerToPane(componentPeerHandle);

	//	If the pane has been killed, exit.
	
	if (componentPane == NULL)
		return;
	
	wasVisible = componentPane->IsVisible();
	
	if (!wasVisible) {

		componentPane->Show();

	 	InvalidateComponent(componentPane, false);

		if (componentPeerHandle != NULL) {

			//	Since we are just becoming visisble, we will change our parent’s
			//	clipping characteristics.  So, we have to mark our parent
			//	for clipping recalc.
			
			parentPeerHandle = GetComponentParent(componentPeerHandle);

			if (parentPeerHandle == NULL)
				componentPeer->mRecalculateClip = true;
			else {
				parentPeer = unhand(parentPeerHandle);
				parentPeer->mRecalculateClip = true;
			}

		}
	
	}
	
}

void sun_awt_macos_MComponentPeer_pHide(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	Hsun_awt_macos_MComponentPeer			*parentPeerHandle;
	Classsun_awt_macos_MComponentPeer		*componentPeer = unhand(componentPeerHandle),
											*parentPeer;
	LPane									*componentPane;
	
	if (componentPeerHandle == NULL)
		return;
		
	componentPane = PeerToPane(componentPeerHandle);

	//	If the pane has been killed, exit.
	
	if (componentPane == NULL)
		return;
		
	InvalidateComponent(componentPane, false);
	
	if (componentPeerHandle != NULL) {

		//	Since we are just becoming visisble, we will change our parent’s
		//	clipping characteristics.  So, we have to mark our parent
		//	for clipping recalc.
		
		parentPeerHandle = GetComponentParent(componentPeerHandle);

		if (parentPeerHandle == NULL)
			componentPeer->mRecalculateClip = true;
		else {
			parentPeer = unhand(parentPeerHandle);
			parentPeer->mRecalculateClip = true;
		}

	}
	
	componentPane->Hide();
}

void sun_awt_macos_MComponentPeer_pEnable(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	Classsun_awt_macos_MComponentPeer		*componentPeer = unhand(componentPeerHandle);
	LPane									*componentPane;
	
	componentPane = PeerToPane(componentPeerHandle);

	//	If the pane has been killed, exit.
	
	if (componentPane == NULL)
		return;
	
	componentPane->Enable();
}

void sun_awt_macos_MComponentPeer_pDisable(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	Classsun_awt_macos_MComponentPeer		*componentPeer = unhand(componentPeerHandle);
	LPane									*componentPane;
	
	componentPane = PeerToPane(componentPeerHandle);

	//	If the pane has been killed, exit.
	
	if (componentPane == NULL)
		return;
	
	componentPane->Disable();
}

void sun_awt_macos_MComponentPeer_pReshape(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle, long x, long y, long width, long height)
{
	Hsun_awt_macos_MComponentPeer			*parentPeerHandle;
	Classsun_awt_macos_MComponentPeer		*parentPeer,
											*componentPeer = unhand(componentPeerHandle);
	LPane									*componentPane;
	
	ClearCachedAWTRefocusPane();

	componentPane = PeerToPane(componentPeerHandle);
	
	//	If the pane has been killed, exit.
	
	if (componentPane == NULL)
		return;
	
	if (componentPane->IsVisible())
		componentPane->Refresh();

	componentPane->PlaceInSuperImageAt(x, y, false);
	componentPane->ResizeFrameTo(width, height, false);

	if (componentPane->IsVisible())
		componentPane->Refresh();
	
	componentPeer->mRecalculateClip = true;
	
	parentPeerHandle = GetComponentParent(componentPeerHandle);

	if (parentPeerHandle != NULL) {
		parentPeer = unhand(parentPeerHandle);
		parentPeer->mRecalculateClip = true;
	}
	
}

void sun_awt_macos_MComponentPeer_pRepaint(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle,int64_t wh,long x,long y,long width,long height)
{
	Classsun_awt_macos_MComponentPeer		*componentPeer = unhand(componentPeerHandle);
	LPane									*componentPane;
	
	componentPane = PeerToPane(componentPeerHandle);

	//	If the pane has been killed, exit.
	
	if (componentPane == NULL)
		return;
	
	if (componentPane->IsVisible()) {
		componentPane->FocusDraw();
		MToolkitExecutJavaDynamicMethod(componentPeerHandle, "handleRepaint", "(IIII)V", x, y, width, height);
	}
}

void sun_awt_macos_MComponentPeer_pDispose(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	LPane		*componentPane;

	componentPane = PeerToPane(componentPeerHandle);
	if (componentPane != NULL) {
		InvalidateComponent(componentPane, false);
#ifdef UNICODE_FONTLIST
		if (unhand(componentPeerHandle)->pInternationalData != 0)
		{
			LJavaFontList* fontlist = 
				(LJavaFontList*) unhand(componentPeerHandle)->pInternationalData;
			delete fontlist;
			unhand(componentPeerHandle)->pInternationalData = NULL;
		}
#endif
		delete componentPane;

		//	Mark the pane as disposed (kJavaComponentPeerHasBeenDisposed
		//	in mOwnerPane).  This is important for checkboxes that 
		//	need to be shown when they are installed in a radio group. 
		unhand(componentPeerHandle)->mOwnerPane = kJavaComponentPeerHasBeenDisposed;

	}
}

void sun_awt_macos_MComponentPeer_setFont(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle, struct Hjava_awt_Font *fontObject)
{
	Classsun_awt_macos_MComponentPeer		*componentPeer = unhand(componentPeerHandle);
	LPane									*componentPane = PeerToPane(componentPeerHandle);
	OSType									paneType;
	Boolean									hasFrame;
	short									font, size;
	Style									style;

	ConvertFontToNumSizeAndStyle(fontObject, &font, &size, &style);

#ifdef UNICODE_FONTLIST
	if(componentPeer->pInternationalData)
	{
		LJavaFontList* fontlist = 
				(LJavaFontList*) componentPeer->pInternationalData;
		delete fontlist;
	}
	try {
		componentPeer->pInternationalData = (long) new LJavaFontList(unhand(fontObject));
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}
#endif
	componentPeer->mFont = font;
	componentPeer->mSize = size;
	componentPeer->mStyle = style;
	
	paneType = componentPane->GetPaneID();
	
	hasFrame = ((paneType == MTextAreaPeer::class_ID) || 
				(paneType == MListPeer::class_ID) || 
				(paneType == MTextFieldPeer::class_ID));
	
	InvalidateComponent(PeerToPane(componentPeerHandle), hasFrame);
}

void sun_awt_macos_MComponentPeer_requestFocus(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	LPane *componentPane = PeerToPane(componentPeerHandle);

	if (componentPane == NULL)
		return;
	
	switch (componentPane->GetPaneID()) {
		
		case MCanvasPeer::class_ID:
			LCommander::SwitchTarget((MCanvasPeer *)componentPane);
			break;
			
		case MTextFieldPeer::class_ID:
			LCommander::SwitchTarget((MTextFieldPeer *)componentPane);
			break;

		case MTextAreaPeer::class_ID:
			MTextEditPeer	*textEditPeer;
			textEditPeer = (MTextEditPeer *)((LView *)componentPane)->FindPaneByID(MTextEditPeer::class_ID);
			LCommander::SwitchTarget(textEditPeer);
			break;
			
		default:
			break;
	}

}

void sun_awt_macos_MComponentPeer_nextFocus(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle)
{
	LPane			*componentPane;
	LView			*parentPane;
	LPane			*nextPane;

	componentPane = PeerToPane(componentPeerHandle);
	if (componentPane == NULL)
		return;
	
	parentPane = componentPane->GetSuperView();
	if (parentPane == NULL)
		return;
		
	LListIterator	iterator(parentPane->GetSubPanes(), iterate_FromStart);
	
	//	First find the location of this pane in the list.
	
	while (1) {
		iterator.Next(&nextPane);
		if (nextPane == componentPane)
			break;
	}
	
	//	Then find the next pane (with wrap around) that is 
	//	targetable.
	
	Boolean 	done = false;
	UInt32		timesAround = 0;
	
	while (!done) {
	
		iterator.Next(&nextPane);
		if (nextPane == NULL) {
		
			iterator.ResetTo(1);
			iterator.Current(&nextPane);
		
			//	If we around going around and around,
			//	then bail out.
			
			timesAround++;
			if (timesAround == 2)
				return;
		
		}
		
		switch (nextPane->GetPaneID()) {
			
			case MCanvasPeer::class_ID:
				LCommander::SwitchTarget((MCanvasPeer *)nextPane);
				done = true;
				break;
				
			case MTextFieldPeer::class_ID:
				LCommander::SwitchTarget((MTextFieldPeer *)nextPane);
				done = true;
				break;

			case MTextAreaPeer::class_ID:
				MTextEditPeer	*textEditPeer;
				textEditPeer = (MTextEditPeer *)((LView *)nextPane)->FindPaneByID(MTextEditPeer::class_ID);
				LCommander::SwitchTarget(textEditPeer);
				done = true;
				break;

			default:
				break;
		};
	
	}
	

}

extern EventRecord			gEventRecordHistory[];

long sun_awt_macos_MComponentPeer_handleEvent(struct Hsun_awt_macos_MComponentPeer *componentPeerHandle, struct Hjava_awt_Event *eventHandle)
{
	Classsun_awt_macos_MComponentPeer		*componentPeer = unhand(componentPeerHandle);
	LPane									*componentPane = PeerToPane(componentPeerHandle);
	Classjava_awt_Event						*event = unhand(eventHandle);
	Classsun_awt_macos_MacintoshEvent		*macEvent = (Classsun_awt_macos_MacintoshEvent *)event;
	EventRecord								*lastEventRecord;
	static Handle							keyResourceMap = NULL;
	static UInt32							keyState = 0;
	char									character;
	Boolean									processed = false;
	
	if (componentPane == NULL)
		return false;
	
	switch (event->id) {
	
		case java_awt_Event_KEY_PRESS:
		case java_awt_Event_KEY_ACTION:
			
			lastEventRecord = gEventRecordHistory + macEvent->privateData;
			
			UInt16	newModifiers = 0;
			
			if (event->modifiers & java_awt_Event_SHIFT_MASK)
				newModifiers |= shiftKey;

			if (event->modifiers & java_awt_Event_CTRL_MASK)
				newModifiers |= controlKey;

			if (event->modifiers & java_awt_Event_META_MASK)
				newModifiers |= cmdKey;
			
			if (event->modifiers & java_awt_Event_ALT_MASK)
				newModifiers |= optionKey;

			character = lastEventRecord->message & charCodeMask;

			//	If the character is an arrow key, send it through
			//	un-modified.
			
			switch (character) {

				case 0x1C:
				case 0x1D:
				case 0x1E:
				case 0x1F:
					break;
				
				default:

					//	FIX ME.  Assumes ROMAN.

					//	If the key was an ascii charater, retranslate the 
					//	returned key using the new modifiers.
					if ((event->key == (character)) &&
						(((event->key & charCodeMask) >= ' ') && 
						((event->key & charCodeMask) <= 0x80))) {
						
						UInt32		translateResult;
						
						if (keyResourceMap == NULL)
							keyResourceMap = GetResource('KCHR', 0);

						translateResult = KeyTranslate((void *)*keyResourceMap, 
								(((lastEventRecord->message & keyCodeMask) & 0x7F00) >> 8) | newModifiers,
								&keyState);

						if (translateResult <= 255)
							lastEventRecord->message = (lastEventRecord->message & 0xFFFFFF00) |
									(translateResult & 0x00FF);		
					}
					
					else {		
						if (event->key == '\n')
							event->key = '\r';
						lastEventRecord->message = (lastEventRecord->message & 0xFFFFFF00) |
								(event->key & 0x00FF);		
					}
					
					lastEventRecord->modifiers = newModifiers;

					break;
					
			}
			
			//	If the character has not been changed, then 
			//	seen if any of modifier flags have been changed.
			//	If the shift key has been toggled, then capitalize 
			//	numbers and letters.
			
			if (componentPane->GetPaneID() == MTextFieldPeer::class_ID) {
				processed = ((MTextFieldPeer *)componentPane)->HandleKeyPress(*lastEventRecord);
			}

			if (componentPane->GetPaneID() == MTextAreaPeer::class_ID) {
				MTextEditPeer 	*textAreaPane;
				textAreaPane = (MTextEditPeer *)componentPane->FindPaneByID(MTextEditPeer::class_ID);
				processed = textAreaPane->HandleKeyPress(*lastEventRecord);
			}

			break;		

		case java_awt_Event_MOUSE_DOWN:
		
			SMouseDownEvent		mouseDown;
			
			lastEventRecord = gEventRecordHistory + macEvent->privateData;

			mouseDown.macEvent = *lastEventRecord;	
			mouseDown.wherePort = mouseDown.macEvent.where;
			componentPane->GlobalToPortPoint(mouseDown.wherePort);
			mouseDown.whereLocal = mouseDown.wherePort;
			componentPane->PortToLocalPoint(mouseDown.whereLocal);
			mouseDown.delaySelect = false;
			
			componentPane->ClickSelf(mouseDown);
			
			break;

		default:
			break;
	
	}
	
	return processed;
	
}
