//##############################################################################
//##############################################################################
//
//	File:		MTextFieldPeer.cp
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
#include "java_awt_Event.h"
#include "java_awt_Font.h"
#include "java_awt_Component.h"
#include "java_awt_TextComponent.h"
#include "sun_awt_macos_MTextFieldPeer.h"
#include "sun_awt_macos_MComponentPeer.h"

};

#include "MTextFieldPeer.h"
#include "MCanvasPeer.h"
#include "LMacGraphics.h"
#include "MToolkit.h"

#ifdef UNICODE_FONTLIST
#include "MComponentPeer.h"
#include "uprefd.h"
#endif

enum {
	kMScrollTextComponentPeerPPobID		= 4096,
	kPasswordAWTTextTraitsID			= 4000,
	kMScrollTextEditPaneID				= 'tapr'
};


//##############################################################################
//##############################################################################
#pragma mark MComponentPeer IMPLEMENTATION
#pragma mark -

MTextFieldPeer::
MTextFieldPeer(const SPaneInfo &inPaneInfo):
	LEditField(inPaneInfo, "\p", kDefaultAWTTextTraitsID, 255, true, false, NULL, NULL)
{
#ifdef UNICODE_FONTLIST
	int16 encoding =intl_GetWindowEncoding(PaneToPeer(this));
	if(! UEncoding::IsMacRoman(encoding))
		this->SetTextTraitsID(UEncoding::GetTextTextTraitsID(encoding));
#endif
	mEchoChar = 0;
}

MTextFieldPeer::
~MTextFieldPeer()
{
	DisposeJavaDataForPane(this);
}

Boolean 
MTextFieldPeer::FocusDraw()
{
	Boolean shouldRefocus = ShouldRefocusAWTPane(this);
	
	SetSuperCommander(GetContainerWindow(this));
	gInPaneFocusDraw = true;
	
	Boolean result = inherited::FocusDraw();
	gInPaneFocusDraw = false;

	if (shouldRefocus && result)
		PreparePeerGrafPort(PaneToPeer(this), NULL);

	if (mEchoChar == 0) 
	{

#ifdef UNICODE_FONTLIST
		int16 encoding = intl_GetWindowEncoding(PaneToPeer(this));
		if(! UEncoding::IsMacRoman(encoding))
			this->SetTextTraitsID(UEncoding::GetTextTextTraitsID(encoding));
#endif

	}
	
	else
	{
		//	Use password font here.
		this->SetTextTraitsID(4000);	
	}

	ForeColor(blackColor);
	BackColor(whiteColor);

	return result;
}

void
MTextFieldPeer::DrawSelf()
{	
	Rect		localFrameRect;
	CalcLocalFrameRect(localFrameRect);
	::InsetRect(&localFrameRect, 1, 1);
	::EraseRect(&localFrameRect);
	inherited::DrawSelf();
}

void
MTextFieldPeer::ClickSelf(const SMouseDownEvent &inMouseDown)
{
	inherited::ClickSelf(inMouseDown);
}

void
MTextFieldPeer::BeTarget()
{
	inherited::BeTarget();
	SetJavaTarget(PaneToPeer(this));
	MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "gotFocus", "()V");
}

void
MTextFieldPeer::DontBeTarget()
{
	inherited::DontBeTarget();
	MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "lostFocus", "()V");
	SetJavaTarget(NULL);
}

void
MTextFieldPeer::FindCommandStatus(
								CommandT	inCommand,
								Boolean		&outEnabled,
								Boolean		&outUsesMark,
								Char16		&outMark,
								Str255		outName)
{
	if (mEchoChar == 0)
	{
		inherited::FindCommandStatus(inCommand, outEnabled, outUsesMark, outMark, outName);
	}
	
	else 
	{
		switch (inCommand) 
		{
		
			case cmd_Cut:
			case cmd_Copy:
				outEnabled = false;
				break;
		
			default:
				inherited::FindCommandStatus(inCommand, outEnabled, outUsesMark, outMark, outName);
				break;
		}
	}
		
}

Boolean
MTextFieldPeer::HandleKeyPress(const EventRecord &inKeyEvent)
{
	if ((inKeyEvent.message & charCodeMask) == '\r') {
		MToolkitExecutJavaDynamicMethod(PaneToPeer(this), "handleAction", "()V");
		return true;
	} else
		return inherited::HandleKeyPress(inKeyEvent);
}


//##############################################################################
//##############################################################################

void sun_awt_macos_MTextFieldPeer_create(struct Hsun_awt_macos_MTextFieldPeer *textFieldPeerHandle, struct Hsun_awt_macos_MComponentPeer *hostPeerHandle)
{

	Classsun_awt_macos_MTextFieldPeer		*textFieldPeer = (Classsun_awt_macos_MTextFieldPeer*)unhand(textFieldPeerHandle);
	Classjava_awt_TextComponent				*textFieldTarget = (Classjava_awt_TextComponent*)unhand(textFieldPeer->target);
	LView									*superView = (LView *)PeerToPane(hostPeerHandle);
	SPaneInfo								paneInfo;
	MTextFieldPeer							*textFieldPane;

	memset(&paneInfo, 0, sizeof(paneInfo));
	
	paneInfo.paneID 		= MTextFieldPeer::class_ID;
	paneInfo.visible 		= triState_Off;
	paneInfo.left			= textFieldTarget->x;
	paneInfo.top			= textFieldTarget->y;
	paneInfo.width			= textFieldTarget->width;
	paneInfo.height			= textFieldTarget->height;
	paneInfo.enabled 		= triState_Off;
	paneInfo.userCon 		= (long)textFieldPeerHandle;
	paneInfo.superView		= superView;

	LCommander::SetDefaultCommander(GetContainerWindow(superView));

	//	Create the text field PowerPlant object

	textFieldPane = NULL;
	try {
		textFieldPane = new MTextFieldPeer(paneInfo);
	}
	catch (...) {
		SignalError(0, JAVAPKG "OutOfMemoryError", 0);
		return;
	}
	
	textFieldPane->Enable();

	textFieldPeer->mOwnerPane = (long)textFieldPane;
	textFieldPeer->mRecalculateClip = true;

	ClearCachedAWTRefocusPane();

}

void sun_awt_macos_MTextFieldPeer_setEditable(struct Hsun_awt_macos_MTextFieldPeer *textFieldPeerHandle, long editable)
{
	MTextFieldPeer			*textFieldPeer = (MTextFieldPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);

	if (textFieldPeer == NULL)
		return;

	//	If we are no longer editable and we are the
	//	target, set the target to be our super commander.
	
	if (LCommander::GetTarget() == textFieldPeer)
		LCommander::SwitchTarget(textFieldPeer->GetSuperCommander());
}

void sun_awt_macos_MTextFieldPeer_select(struct Hsun_awt_macos_MTextFieldPeer *textFieldPeerHandle, long from, long to)
{
	MTextFieldPeer			*textFieldPeer = (MTextFieldPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);
	TEHandle				textEditHandle;

	if (textFieldPeer == NULL)
		return;

	textEditHandle = textFieldPeer->GetMacTEH();
	textFieldPeer->FocusDraw();
#ifdef UNICODE_FONTLIST
	// we need to convert the parameter from and to from characer length to byte count
	CharsHandle	textHandle	= ::TEGetText(textEditHandle);

	HLock((Handle)textHandle);	

	unsigned char	*textPtr = (unsigned char*) *textHandle;
	long realfrom, realto;
	short encoding = intl_GetWindowEncoding((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);
	realfrom = INTL_TextCharLenToByteCount(encoding, textPtr, from);
	realto = realfrom + INTL_TextCharLenToByteCount(encoding, (textPtr+realfrom), (to-from));
	
	HUnlock((Handle)textHandle);

	::TESetSelect(realfrom, realto, textEditHandle);
#else
	::TESetSelect(from, to, textEditHandle);
#endif

	LCommander::SwitchTarget(textFieldPeer);

}

long sun_awt_macos_MTextFieldPeer_getSelectionStart(struct Hsun_awt_macos_MTextFieldPeer *textFieldPeerHandle)
{
	MTextFieldPeer			*textFieldPeer = (MTextFieldPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);
	TEHandle				textEditHandle;

	if (textFieldPeer == NULL)
		return 0;

	textEditHandle = textFieldPeer->GetMacTEH();
#ifdef UNICODE_FONTLIST
	// we need to convert byte count back to character length
	long startByte = (**textEditHandle).selStart;
	CharsHandle	textHandle	= ::TEGetText(textEditHandle);

	HLock((Handle)textHandle);	
	unsigned char	*textPtr = (unsigned char*) *textHandle;
	short encoding = intl_GetWindowEncoding((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);
	long startCharLen = INTL_TextByteCountToCharLen(encoding, textPtr, startByte);
	HUnlock((Handle)textHandle);
	
	return startCharLen;
#else
	return (**textEditHandle).selStart;
#endif
}

long sun_awt_macos_MTextFieldPeer_getSelectionEnd(struct Hsun_awt_macos_MTextFieldPeer *textFieldPeerHandle)
{
	MTextFieldPeer			*textFieldPeer = (MTextFieldPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);
	TEHandle				textEditHandle;

	if (textFieldPeer == NULL)
		return 0;

	textEditHandle = textFieldPeer->GetMacTEH();
#ifdef UNICODE_FONTLIST
	// we need to convert byte count back to character length
	long endByte = (**textEditHandle).selEnd;
	CharsHandle	textHandle	= ::TEGetText(textEditHandle);

	HLock((Handle)textHandle);	
	unsigned char	*textPtr = (unsigned char*) *textHandle;
	short encoding = intl_GetWindowEncoding((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);
	long endCharLen = INTL_TextByteCountToCharLen(encoding, textPtr, endByte);
	HUnlock((Handle)textHandle);
	
	return endCharLen;
#else
	return (**textEditHandle).selEnd;
#endif
}

void sun_awt_macos_MTextFieldPeer_setText(struct Hsun_awt_macos_MTextFieldPeer *textFieldPeerHandle, struct Hjava_lang_String *string)
{
	MTextFieldPeer			*textFieldPeer = (MTextFieldPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);
	TEHandle				textEditHandle;
	char					*newCString;
	
	if (textFieldPeer == NULL)
		return;

	textFieldPeer->FocusDraw();

	textEditHandle = textFieldPeer->GetMacTEH();

#ifdef UNICODE_FONTLIST
	short encoding = intl_GetWindowEncoding((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);
	newCString = intl_allocCString(encoding, string);
	::TESetText(newCString, XP_STRLEN(newCString), textEditHandle);
#else
	newCString = allocCString(string);
	::TESetText(newCString, javaStringLength(string), textEditHandle);
#endif
	free(newCString);

	textFieldPeer->Draw(NULL);

}

struct Hjava_lang_String *sun_awt_macos_MTextFieldPeer_getText(struct Hsun_awt_macos_MTextFieldPeer *textFieldPeerHandle)
{
	MTextFieldPeer				*textFieldPeer = (MTextFieldPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);
	TEHandle					textEditHandle;
	CharsHandle					textHandle;
	struct Hjava_lang_String	*resultString;
	
	if (textFieldPeer == NULL)
		return makeJavaString("", 0);
	
	textEditHandle = textFieldPeer->GetMacTEH();

	textHandle = ::TEGetText(textEditHandle);
	
	HLock((Handle)textHandle);
	
#ifdef UNICODE_FONTLIST
	short encoding = intl_GetWindowEncoding((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);
	resultString = intl_makeJavaString(encoding, (*textHandle), GetHandleSize(textHandle));	
#else	
	resultString = makeJavaString(*textHandle, GetHandleSize(textHandle));	
#endif	
	
	
	HUnlock((Handle)textHandle);

	return resultString;

}

void sun_awt_macos_MTextFieldPeer_setEchoCharacter(struct Hsun_awt_macos_MTextFieldPeer *textFieldPeerHandle, unicode echoChar)
{
	MTextFieldPeer				*textFieldPeer = (MTextFieldPeer *)PeerToPane((Hsun_awt_macos_MComponentPeer *)textFieldPeerHandle);

	if (textFieldPeer == NULL)
		return;

	textFieldPeer->mEchoChar = echoChar;

	textFieldPeer->FocusDraw();
	textFieldPeer->Draw(NULL);
}

