//##############################################################################
//##############################################################################
//
//	File:		MTextAreaPeer.cp
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
#include "java_awt_Component.h"
#include "java_awt_TextComponent.h"
#include "sun_awt_macos_MTextAreaPeer.h"
#include "sun_awt_macos_MComponentPeer.h"

};

#include "MTextAreaPeer.h"
#include "MCanvasPeer.h"
#include "UTextTraits.h"
#include "LMacGraphics.h"
#include "MToolkit.h"

#ifdef UNICODE_FONTLIST
#include "MComponentPeer.h"
#endif
enum {
	kMScrollTextComponentPeerPPobID		= 4096,
	kTextAreaMaxCharacters				= 16384,
	kTextAreaScrollBarWidth				= 16,
	kTextAreaHorizMarginSize			= 4,
	kMinimumEnclosedFrameWidth			= 500,
	kMinimumTextAreaDimension			= 16
};


//##############################################################################
//##############################################################################
#pragma mark MComponentPeer IMPLEMENTATION
#pragma mark -


MTextAreaPeer::
MTextAreaPeer(LStream *inStream):
	LView(inStream)
{
}

MTextAreaPeer::
~MTextAreaPeer()
{
	DisposeJavaDataForPane(this);
}

Boolean 
MTextAreaPeer::FocusDraw()
{
	Boolean	shouldRefocus = ShouldRefocusAWTPane(this);
	Boolean  result = false;
	result = inherited::FocusDraw();
	if (shouldRefocus && result)
		PreparePeerGrafPort(PaneToPeer(this), NULL);
	ForeColor(blackColor);
	return result;
}

void
MTextAreaPeer::DrawSelf()
{
	Rect eraseRect;
	
	//	Erase the area where the grow box should be.
	
	CalcLocalFrameRect(eraseRect);
	eraseRect.top = eraseRect.bottom - kTextAreaScrollBarWidth;
	eraseRect.left = eraseRect.right - kTextAreaScrollBarWidth;
	::EraseRect(&eraseRect);
}


LView* 
MTextAreaPeer::CreateMTextAreaPeerStream(LStream *inStream)
{
	return new MTextAreaPeer(inStream);
}

MTextEditPeer::
MTextEditPeer(LStream *inStream):
	LTextEdit(inStream)
{
}

MTextEditPeer::
~MTextEditPeer()
{
}

LView* 
MTextEditPeer::CreateMTextEditPeerStream(LStream *inStream)
{
	return new MTextEditPeer(inStream);
}

void
MTextEditPeer::BeTarget()
{
	inherited::BeTarget();
	SetJavaTarget(PaneToPeer(this));
	MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "gotFocus", "()V");
}

void	
MTextEditPeer::SetEditable(Boolean editable)
{
	if (editable)
		mTextAttributes |= textAttr_Editable;
	else
		mTextAttributes &= ~textAttr_Editable;
}



Boolean 
MTextEditPeer::FocusDraw()
{
	Classsun_awt_macos_MComponentPeer	*componentPeer;
	RGBColor							foreColor, backColor;
	
	SetSuperCommander(GetContainerWindow(this));

	Boolean result = inherited::FocusDraw();
	
	componentPeer = unhand(PaneToPeer(this));
	
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
#ifdef UNICODE_FONTLIST
	int16 encoding = intl_GetWindowEncoding(PaneToPeer(this));
	SetUpTextITraits(componentPeer->mFont, componentPeer->mSize, componentPeer->mStyle, encoding);
	if((componentPeer->mFont != 0) && (! UEncoding::IsMacRoman(encoding)))
		this->SetTextTraitsID(UEncoding::GetTextTextTraitsID(encoding));
#else
	SetUpTextITraits(componentPeer->mFont, componentPeer->mSize, componentPeer->mStyle);
#endif

	return result;
}


void
MTextEditPeer::DrawSelf()
{
	Rect eraseRect;
	
	//	Erase the text pane in the background color
	//	first.
	
	CalcLocalFrameRect(eraseRect);
	::EraseRect(&eraseRect);
	
	//	Draw the text.
	
	inherited::DrawSelf();

}

void
MTextEditPeer::DontBeTarget()
{
	inherited::DontBeTarget();
	MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "lostFocus", "()V");
	SetJavaTarget(NULL);
}

//##############################################################################
//##############################################################################

void sun_awt_macos_MTextAreaPeer_create(struct Hsun_awt_macos_MTextAreaPeer *textAreaPeerHandle, struct Hsun_awt_macos_MComponentPeer *hostPeerHandle)
{

	Classsun_awt_macos_MTextAreaPeer		*textAreaPeer = (Classsun_awt_macos_MTextAreaPeer*)unhand(textAreaPeerHandle);
	Classjava_awt_TextComponent				*textAreaTarget = (Classjava_awt_TextComponent*)unhand(textAreaPeer->target);
	LView									*superView = (LView *)PeerToPane(hostPeerHandle);
	long									x = textAreaTarget->x,
											y = textAreaTarget->y,
											width = textAreaTarget->width,
											height = textAreaTarget->height;
	
	//	Create the text area and its containing scrollbars
	//	from a PPob resource.

	LPane::SetDefaultView(superView);
	
	LCommander::SetDefaultCommander(GetContainerWindow(superView));
	
	LView	*scrolledTextAreaPane = NULL;

	try {
		scrolledTextAreaPane = (LView *)UReanimator::ReadObjects('PPob', kMScrollTextComponentPeerPPobID);
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}

	if (width < kMinimumTextAreaDimension)
		width = kMinimumTextAreaDimension;
		
	if (height < kMinimumTextAreaDimension)
		height = kMinimumTextAreaDimension;

	OverlapBindToTopLevelFrame((struct Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle, hostPeerHandle, &x, &y, &width, &height);

	//	Install the new text area in its super pane.

	scrolledTextAreaPane->PlaceInSuperFrameAt(x, y, false);
	scrolledTextAreaPane->ResizeFrameTo(width, height, false);		
	scrolledTextAreaPane->FinishCreate();

	scrolledTextAreaPane->Enable();
	scrolledTextAreaPane->Activate();

	//	Finish things upÉ

	MTextEditPeer *textEditPane = (MTextEditPeer *)scrolledTextAreaPane->FindPaneByID(MTextEditPeer::class_ID);

	UInt32	enclosedPaneWidth = width - kTextAreaScrollBarWidth - kTextAreaHorizMarginSize;
	
	if (enclosedPaneWidth < kMinimumEnclosedFrameWidth)
		enclosedPaneWidth = kMinimumEnclosedFrameWidth;

	textEditPane->ResizeImageTo(enclosedPaneWidth, height, false);
	textEditPane->SetUserCon((long)textAreaPeerHandle);
	textEditPane->SetEditable(textAreaTarget->editable);

	scrolledTextAreaPane->SetUserCon((long)textAreaPeerHandle);
	scrolledTextAreaPane->SetPaneID(MTextAreaPeer::class_ID);

	textAreaPeer->mOwnerPane = (long)scrolledTextAreaPane;
	textAreaPeer->mRecalculateClip = true;

	textEditPane->SetPaneID(MTextEditPeer::class_ID);
	
	ClearCachedAWTRefocusPane();

}

void sun_awt_macos_MTextAreaPeer_setEditable(struct Hsun_awt_macos_MTextAreaPeer *textComponentPeerObjectHandle, long editable)
{
	LView					*scrollTextAreaPane = (LView *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textComponentPeerObjectHandle);
	MTextEditPeer			*textAreaPane;

	if (scrollTextAreaPane == NULL)
		return;

	textAreaPane = (MTextEditPeer *)scrollTextAreaPane->FindPaneByID(MTextEditPeer::class_ID);
	
	textAreaPane->SetEditable(editable);	

}

void sun_awt_macos_MTextAreaPeer_select(struct Hsun_awt_macos_MTextAreaPeer *textAreaPeerHandle, long from, long to)
{
	LView					*scrollTextAreaPane = (LView *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle);
	MTextEditPeer			*textAreaPane;
	TEHandle				textEditHandle;
	
	if (scrollTextAreaPane == NULL)
		return;

	textAreaPane = (MTextEditPeer *)scrollTextAreaPane->FindPaneByID(MTextEditPeer::class_ID);
	textAreaPane->FocusDraw();

	textEditHandle = textAreaPane->GetMacTEH();
	
	return;

#ifdef UNICODE_FONTLIST
	// we need to convert the parameter from and to from characer length to byte count
	CharsHandle	textHandle	= (**textEditHandle).hText;  // DONT CALL TEGETTEXT!

	HLock((Handle)textHandle);	

	unsigned char	*textPtr = (unsigned char*) *textHandle;
	long realfrom, realto;
	short encoding = intl_GetWindowEncoding((Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle);
	realfrom = INTL_TextCharLenToByteCount(encoding, textPtr, from);
	realto = realfrom + INTL_TextCharLenToByteCount(encoding, (textPtr+realfrom), (to-from));
	
	HUnlock((Handle)textHandle);

	::TESetSelect(realfrom, realto, textEditHandle);
#else
	::TESetSelect(from, to, textEditHandle);
#endif

	LCommander::SwitchTarget(textAreaPane);
}

long sun_awt_macos_MTextAreaPeer_getSelectionStart(struct Hsun_awt_macos_MTextAreaPeer *textAreaPeerHandle)
{
	LView					*scrollTextAreaPane = (LView *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle);
	MTextEditPeer			*textAreaPane;
	TEHandle				textEditHandle;

	if (scrollTextAreaPane == NULL)
		return 0;

	textAreaPane = (MTextEditPeer *)scrollTextAreaPane->FindPaneByID(MTextEditPeer::class_ID);

	textEditHandle = textAreaPane->GetMacTEH();
#ifdef UNICODE_FONTLIST
	// we need to convert byte count back to character length
	long startByte = (**textEditHandle).selStart;
	CharsHandle	textHandle	= (**textEditHandle).hText;

	HLock((Handle)textHandle);	
	unsigned char	*textPtr = (unsigned char*) *textHandle;
	short encoding = intl_GetWindowEncoding((Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle);
	long startCharLen = INTL_TextByteCountToCharLen(encoding, textPtr, startByte);
	HUnlock((Handle)textHandle);
	
	return startCharLen;
#else
	return (**textEditHandle).selStart;
#endif
}

long sun_awt_macos_MTextAreaPeer_getSelectionEnd(struct Hsun_awt_macos_MTextAreaPeer *textAreaPeerHandle)
{
	LView					*scrollTextAreaPane = (LView *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle);
	MTextEditPeer			*textAreaPane;
	TEHandle				textEditHandle;

	if (scrollTextAreaPane == NULL)
		return 0;

	textAreaPane = (MTextEditPeer *)scrollTextAreaPane->FindPaneByID(MTextEditPeer::class_ID);

	textEditHandle = textAreaPane->GetMacTEH();
#ifdef UNICODE_FONTLIST
	// we need to convert byte count back to character length
	long endByte = (**textEditHandle).selEnd;
	CharsHandle	textHandle	= (**textEditHandle).hText;  // DONT CALL TEGETTEXT!

	HLock((Handle)textHandle);	
	unsigned char	*textPtr = (unsigned char*) *textHandle;
	short encoding = intl_GetWindowEncoding((Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle);
	long endCharLen = INTL_TextByteCountToCharLen(encoding, textPtr, endByte);
	HUnlock((Handle)textHandle);
	
	return endCharLen;
#else
	return (**textEditHandle).selEnd;
#endif
}

void sun_awt_macos_MTextAreaPeer_setText(struct Hsun_awt_macos_MTextAreaPeer *textAreaPeerHandle, struct Hjava_lang_String *string)
{
	sun_awt_macos_MTextAreaPeer_replaceText(textAreaPeerHandle, string, 0, 99999);
}

struct Hjava_lang_String *sun_awt_macos_MTextAreaPeer_getText(struct Hsun_awt_macos_MTextAreaPeer *textAreaPeerHandle)
{
	LView						*scrollTextAreaPane = (LView *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle);
	TEHandle					textEditHandle;
	CharsHandle					textHandle;
	char		 				*textPtr;					
	long						textSize;
	struct Hjava_lang_String	*resultString;
	MTextEditPeer				*textAreaPane;

	if (scrollTextAreaPane == NULL)
		return makeJavaString("", 0);	

	textAreaPane = (MTextEditPeer *)scrollTextAreaPane->FindPaneByID(MTextEditPeer::class_ID);
	LMSetToolScratch(textAreaPane);

	textEditHandle = textAreaPane->GetMacTEH();
	LMSetApplScratch(textEditHandle);

	textHandle = (**textEditHandle).hText;
	textSize = GetHandleSize(textHandle);
	
	HLock((Handle)textHandle);
	textPtr = *textHandle;
	ReturnsToNewLines(textPtr, textSize);

#ifdef UNICODE_FONTLIST
	short encoding = intl_GetWindowEncoding((Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle);
	resultString = intl_makeJavaString(encoding, textPtr, textSize);	
#else	
	resultString = makeJavaString(textPtr, textSize);	
#endif	
	NewLinesToReturns(textPtr, textSize);
	HUnlock((Handle)textHandle);
	
	return resultString;

}

void sun_awt_macos_MTextAreaPeer_replaceText(struct Hsun_awt_macos_MTextAreaPeer *textAreaPeerHandle, struct Hjava_lang_String *textToReplaceWith, long start, long end)
{
	LView				*scrollTextAreaPane = (LView *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle);
	long				replaceTextLength;
	char				*newCString;
	MTextEditPeer		*textAreaPane;

	if (scrollTextAreaPane == NULL)
		return;

	textAreaPane = (MTextEditPeer *)scrollTextAreaPane->FindPaneByID(MTextEditPeer::class_ID);
	
#ifdef UNICODE_FONTLIST
	short encoding = intl_GetWindowEncoding((Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle);
	newCString = intl_allocCString(encoding, textToReplaceWith);
#else
	newCString = allocCString(textToReplaceWith);
#endif

	//	To prevent horrible infinite recursions on out-of-memory
	//	errors, silently fail on errors.

	exceptionClear(EE());
	
	if (newCString != NULL) {

		TEHandle					textEditHandle;
		CharsHandle					currentContents;
		char						*newTempBuffer;
		long						newSize,
									currentSize;

		replaceTextLength = strlen(newCString);

		textEditHandle = textAreaPane->GetMacTEH();
		currentContents = (**textEditHandle).hText;
		currentSize = GetHandleSize(currentContents);

#ifdef UNICODE_FONTLIST
		// we need to convert start and end from character length to byte count
		CharsHandle	textHandle	= (**textEditHandle).hText;  // DONT CALL TEGETTEXT!

		HLock((Handle)textHandle);	

		unsigned char	*textPtr = (unsigned char*) *textHandle;
		short encoding = intl_GetWindowEncoding((Hsun_awt_macos_MComponentPeer *)textAreaPeerHandle);
		if(start != 0)
			start = INTL_TextCharLenToByteCount(encoding, textPtr, start);
		if(end != 99999)
			end = 	INTL_TextCharLenToByteCount(encoding, textPtr, end);

		HUnlock((Handle)textHandle);			
#endif
		if (start > currentSize)
			start = currentSize;
			
		if (end > currentSize)
			end = currentSize;

		if (start < 0)
			start = 0;
		
		if (end < 0)
			end = 0;
			
		if (end < start)
			end = start;

		long		firstSegment = start,
					lastSegment = currentSize - end;
		
		newSize = firstSegment + replaceTextLength + lastSegment;
		
		newTempBuffer = (char *)malloc(newSize);
		
		if (newTempBuffer != NULL) {
		
			//	Munge the existing text to add the new text.
		
			BlockMoveData(*currentContents, newTempBuffer, firstSegment);
			BlockMoveData(newCString, newTempBuffer + firstSegment, replaceTextLength);
			BlockMoveData(*currentContents + end, newTempBuffer + firstSegment + replaceTextLength, lastSegment);
		
			//	Convert all the new lines to CRs, and set the new text.
		
			NewLinesToReturns(newTempBuffer + firstSegment, replaceTextLength);

			//	If the new text is longer than our limit, cut 
			//	some text off of the beginning.
			
			if (newSize > kTextAreaMaxCharacters) {
			
				UInt32 truncationAmount = newSize - kTextAreaMaxCharacters;
				BlockMoveData(newTempBuffer + truncationAmount, newTempBuffer, kTextAreaMaxCharacters);
				newSize = kTextAreaMaxCharacters;
			
			}
		
			::TESetText((Ptr)newTempBuffer, newSize, textEditHandle);
			::TECalText(textEditHandle);
			textAreaPane->AdjustImageToText();

			free(newTempBuffer);			

			//	If we are inserting at the end of the current text, or
			//	completely replacing it, make sure to scroll to end
			//	of the text. 
			
			Rect 			tempRect;

			if (lastSegment == 0) {
			
				SDimension32	imageSize;
				SInt32			whereToScroll;
				textAreaPane->CalcLocalFrameRect(tempRect);
				textAreaPane->GetImageSize(imageSize);
				
				if (tempRect.bottom <= tempRect.top)
					whereToScroll = 0;
				else
					whereToScroll = imageSize.height - (tempRect.bottom - tempRect.top);
	
				LView::OutOfFocus(NULL);
				textAreaPane->ScrollImageTo(0, (whereToScroll < 0) ? 0 : whereToScroll, false);
				
			}
			
			//	Invalidate the text area, minus the scrollbars
			//	so that the text is re-drawn.
			//	dkc 2/7/96
	
			if (textAreaPane->FocusDraw())
				textAreaPane->Draw(NULL);
			
		}
		
		free(newCString); 
		
	}

}

void sun_awt_macos_MTextAreaPeer_insertText(struct Hsun_awt_macos_MTextAreaPeer *textAreaPeerHandle, struct Hjava_lang_String *textToInsert, long position)
{
	sun_awt_macos_MTextAreaPeer_replaceText(textAreaPeerHandle, textToInsert, position, position);
}

void sun_awt_macos_MTextAreaPeer_reshape(struct Hsun_awt_macos_MTextAreaPeer *componentPeerHandle, long x, long y, long width, long height)
{
	LView 			*scrolledTextAreaPane = (LView *)PeerToPane((Hsun_awt_macos_MComponentPeer *)componentPeerHandle);
	MTextEditPeer 	*textAreaPane;
	
	if (scrolledTextAreaPane == NULL)
		return;
	
	if (width < kMinimumTextAreaDimension)
		width = kMinimumTextAreaDimension;
		
	if (height < kMinimumTextAreaDimension)
		height = kMinimumTextAreaDimension;

	textAreaPane = (MTextEditPeer *)scrolledTextAreaPane->FindPaneByID(MTextEditPeer::class_ID);

	if (textAreaPane == NULL)
		return;
	
	OverlapBindToTopLevelFrame((struct Hsun_awt_macos_MComponentPeer *)componentPeerHandle, NULL, &x, &y, &width, &height);

	UInt32	enclosedPaneWidth = width - kTextAreaScrollBarWidth - kTextAreaHorizMarginSize;
	
	if (enclosedPaneWidth < kMinimumEnclosedFrameWidth)
		enclosedPaneWidth = kMinimumEnclosedFrameWidth;

	textAreaPane->ResizeImageTo(enclosedPaneWidth, height, false);

	sun_awt_macos_MComponentPeer_pReshape((struct Hsun_awt_macos_MComponentPeer *)componentPeerHandle, x, y, width, height);
}

