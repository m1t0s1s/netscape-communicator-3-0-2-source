//	===========================================================================
//	VTSMProxy
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VTSMProxy.h"

#include	<VTextView.h>
#include	<LStyleSet.h>
#include	<LTextSelection.h>
#include	<LAETypingAction.h>
#include	<VTextHandler.h>
#include	<UFontsStyles.h>

#include	<UDrawingState.h>
#include	<UAppleEventsMgr.h>
#include	<UMemoryMgr.h>

#include	<Gestalt.h>

#pragma	warn_unusedarg off

//	===========================================================================
//	WTSMManager

Boolean				WTSMManager::sTSMPresent = false;
AEEventHandlerUPP	WTSMManager::sAEHandler = NewAEEventHandlerProc(AEHandlerTSM);
VTSMProxy*			WTSMManager::sCurrent = NULL;

void	WTSMManager::Initialize(void)
{
	OSErr	err = noErr;

	do {
		//	tsm present
		long	result;
		err = ::Gestalt(gestaltTSMgrVersion, &result);
		if (err)
			break;
		
		err = ::InitTSMAwareApplication();
		ThrowIfOSErr_(err);
		
		err = ::AEInstallEventHandler(kTextServiceClass, kUpdateActiveInputArea, sAEHandler, kUpdateActiveInputArea, false);
		ThrowIfOSErr_(err);
		err = ::AEInstallEventHandler(kTextServiceClass, kPos2Offset, sAEHandler, kPos2Offset, false);
		ThrowIfOSErr_(err);
		err = ::AEInstallEventHandler(kTextServiceClass, kOffset2Pos, sAEHandler, kOffset2Pos, false);
		ThrowIfOSErr_(err);
		
		sTSMPresent = true;
	} while (false);
}

void	WTSMManager::Quit(void)
{
	if (sTSMPresent) {
		OSErr	err = CloseTSMAwareApplication();
		Assert_(
			(err == noErr) ||
			(err == tsmDocumentOpenErr)	//	bug in system?
		);
	}
}

VInlineTSMProxy *	WTSMManager::MakeInlineTSMProxy(VTextView &inTextView)
{
	VInlineTSMProxy	*rval = NULL;

	if (sTSMPresent && inTextView.GetSelection() && inTextView.GetEventHandler()) {
		LTextEngine	*engine = inTextView.GetTextEngine();
		if (engine)
			rval = new VInlineTSMProxy(*engine, inTextView);
	}
	
	return rval;
}

VTSMTEProxy *	WTSMManager::MakeTSMTEProxy(TEHandle inTEH)
{
	VTSMTEProxy	*rval = NULL;
	
	Assert_(inTEH);
	if (inTEH) {
		ThrowIfNULL_(*inTEH);
		rval = new VTSMTEProxy(inTEH);
	}

	return rval;
}

pascal OSErr	WTSMManager::AEHandlerTSM(
	const AppleEvent	*inAppleEvent,
	AppleEvent			*outReply,
	Int32				inRefCon)
{
	OSErr	err = noErr;
	
	THz	oldZone = ::LMGetTheZone();	//	Apple bug #115424?
	::LMSetTheZone(LMGetApplZone());
	
	try {

		VInlineTSMProxy* proxy = dynamic_cast<VInlineTSMProxy*>(GetCurrent());
		ThrowIfNULL_(proxy);
		
		StHandleLocker	lock(inAppleEvent->dataHandle);
		LAESubDesc		appleEvent(*inAppleEvent);
		LAEStream		replyStream;
		
		ThrowIf_(((Int32)(void *)proxy) != appleEvent.KeyedItem(keyAETSMDocumentRefcon).ToInt32());
		
		replyStream.OpenRecord();
		
		switch(inRefCon) {

			case kUpdateActiveInputArea:
				proxy->AEUpdate(appleEvent, replyStream);
				break;

			case kPos2Offset:
				proxy->AEPos2Offset(appleEvent, replyStream);
				break;

			case kOffset2Pos:
				proxy->AEOffset2Pos(appleEvent, replyStream);
				break;
		}
		
		replyStream.CloseRecord();
		
		//	Transfer reply parameters to the real reply (hopefully MacOS 8 will have a way around this)
		//	ie, can simply say:
		//
		//		replyStream.Close(outReply);
		//
		StAEDescriptor	reply;
		replyStream.Close(reply);
		LAESubDesc		replySD(reply);
		AEKeyword		key;
		for (Int32 i = 1; i <= replySD.CountItems(); i++) {
			StAEDescriptor	parm;
			replySD.NthItem(i, &key).ToDesc(&parm.mDesc);
			err = ::AEPutParamDesc(outReply, key, &parm.mDesc);
			ThrowIfOSErr_(err);
		}		

	} catch (ExceptionCode inErr) {
		err = inErr;
	} catch (...) {
		err = paramErr;
	}

	::LMSetTheZone(oldZone);	//	Apple bug #115424?
	
	return err;
}

//	===========================================================================

VTSMDoEvent::VTSMDoEvent(
	MessageT		inMessage,
	Boolean			inExecuteHost)
:	LAttachment(inMessage, inExecuteHost)
{
}

void	VTSMDoEvent::ExecuteSelf(
	MessageT		inMessage,
	void			*ioParam)
{
	Boolean		executeHost = true;
	VTSMProxy	*proxy = WTSMManager::GetCurrent();
	
	if (proxy) {
		ThrowIfNULL_(ioParam);
		EventRecord	*event = (EventRecord *)ioParam;
		
		if (::TSMEvent(event)) {
			executeHost = false;
		}
	}
	
	SetExecuteHost(executeHost);
}

//	===========================================================================
//	VTSMProxy (base class)

VTSMProxy::VTSMProxy()
{
	mTSMDocID = 0;
	mDocIsActive = false;
}

VTSMProxy::~VTSMProxy()
{
	Assert_(!mDocIsActive);
	Assert_(mTSMDocID == 0);	//	derived class must dispose of document since the derived class created the doc
	if (WTSMManager::GetCurrent() == this)
		WTSMManager::SetCurrent(NULL);
}
	
void	VTSMProxy::Activate(void)
{
	if (!mDocIsActive) {
		OSErr	err;

		#if DEBUG
			//	check to see if a bug in TSM will be encountered
			ProcessSerialNumber	psn,
								csn;
			err = GetCurrentProcess(&psn);
			ThrowIfOSErr_(err);
			err = GetFrontProcess(&csn);
			ThrowIfOSErr_(err);
			Assert_((psn.highLongOfPSN == csn.highLongOfPSN) && (psn.lowLongOfPSN == csn.lowLongOfPSN));
		#endif

		if (mTSMDocID)
			err = ::ActivateTSMDocument(mTSMDocID);
		else
			err = ::UseInputWindow(NULL, true);
		ThrowIfOSErr_(err);
		
		mDocIsActive = true;
	}
		
	WTSMManager::SetCurrent(this);
}

void	VTSMProxy::Deactivate(void)
{
	FlushInput();

	if (mDocIsActive) {
		WTSMManager::SetCurrent(NULL);
		mDocIsActive = false;

		OSErr	err = ::DeactivateTSMDocument(mTSMDocID);
		if (err == tsmDocNotActiveErr) {
			//	Assert_(false);	//	this just seems to happen too much -- it is okay if it happens
		} else {
			ThrowIfOSErr_(err);
		}
	}
}

void	VTSMProxy::FlushInput(void)
{
	OSErr	err = ::FixTSMDocument(mTSMDocID);
	ThrowIfOSErr_(err);
}

//	===========================================================================
//	VInlineTSMProxy

VInlineTSMProxy::VInlineTSMProxy(
	LTextEngine			&inTextEngine,
	VTextView			&inTextView)
:	mTextEngine(inTextEngine)
,	mTextView(inTextView)
{
	OSType	supportedType = kTextService;
	OSErr	err = ::NewTSMDocument(1, &supportedType, &mTSMDocID, (long)(void *)this);
	ThrowIfOSErr_(err);
}

VInlineTSMProxy::~VInlineTSMProxy()
{
	Deactivate();	//	for a bug in TSM.  See TE27

	OSErr	err = noErr;
	if (mTSMDocID)
		err = ::DeleteTSMDocument(mTSMDocID);
	mTSMDocID = 0;
//	Assert_(err == noErr);
}
	
void	VInlineTSMProxy::FlushInput(void)
{
	if (mActiveRange.Length()) {
		inherited::FlushInput();
	}
}

void	VInlineTSMProxy::DrawHilite(void)
{
	//	owning text view is already focused
	TextRangeT	lineRange;
	DescType	partFound;
	
	vector<RangeHiliteRec>::iterator	ip = mHilites.begin(),
										ie = mHilites.end();
	while (ip != ie) {
		
		TextRangeT	hiliteRange = (*ip).range,
					range = hiliteRange;

		if (lineRange.Undefined()) {
			Assert_(ip == mHilites.begin());
			lineRange = hiliteRange;
			lineRange.Front();
		}
		if (!lineRange.Length()) {
			partFound = mTextEngine.FindPart(LTextEngine::sTextAll, &lineRange, cLine);
			ThrowIf_(partFound == typeNull);
			if (!lineRange.Length()) {
				Assert_(false);
				break;
			}
		}
		
		range.Crop(lineRange);
		if (hiliteRange.End() > lineRange.End()) {
			DrawRangeHilite(range, (*ip).hilite);
			lineRange = range;
			lineRange.After();
			continue;
		}
		
		DrawRangeHilite(range, (*ip).hilite);

		ip++;
	}
}

void	VInlineTSMProxy::DrawRangeHilite(
	const TextRangeT	&inRange,
	Int16				inHiliteStyle)
/*
	inRange is assumed not to cross line boundaries
*/
{
	if (inRange.Length()) {
		SPoint32	i1, i2;
		Point		p1, p2;
		
		mTextEngine.Range2Image(inRange, true, &i1);
		mTextEngine.Range2Image(inRange, false, &i2);
		mTextView.ImageToLocalPoint(i1, p1);
		mTextView.ImageToLocalPoint(i2, p2);
		Range32T	span;
		span.SetRange(p1.h, p2.h);
		
		StColorPenState	saveQDState;
		
		if ((inHiliteStyle == kRawText) || (inHiliteStyle == kSelectedRawText)) 
			::PenPat(&UQDGlobals::GetQDGlobals()->gray);
		else
			::PenPat(&UQDGlobals::GetQDGlobals()->black);
		
		p1.h += 1;
		p2.h -= 2;
		p1.v -= 2;
		p2.v -= 2;
		if ((inHiliteStyle == kSelectedRawText) || (inHiliteStyle == kSelectedConvertedText)) {
			PenSize(1, 2);
		} else {
			PenSize(1, 1);
		}
		MoveTo(p1.h, p1.v);
		LineTo(p2.h, p2.v);
	}
}

void	VInlineTSMProxy::AEUpdate(
	const LAESubDesc	&inAppleEvent,
	LAEStream			&inStream)
{
	LTextView::OutOfFocus(&mTextView);

	//	input
	LAESubDesc	textSD(inAppleEvent.KeyedItem(keyAETheData), typeChar);

	ScriptLanguageRecord	writingCode;
	inAppleEvent.KeyedItem(keyAETSMScriptTag).ToPtr(typeIntlWritingCode, &writingCode, sizeof(writingCode));

	Int32	fixLength = inAppleEvent.KeyedItem(keyAEFixLength).ToInt32();
	if (fixLength < 0)
		fixLength = textSD.GetDataLength();

	//	process
	LTextSelection&	selection(dynamic_cast<LTextSelection&>(*mTextView.GetSelection()));
	VTextHandler&	handler(dynamic_cast<VTextHandler&>(*(mTextView.GetEventHandler())));
	
	if (selection.GetCurrentStyle()->GetScript() != writingCode.fScript)
		Throw_(paramErr);
		
//-?	handler.FollowKeyScript();	//	if key script was changed
	LAETypingAction	*typing = selection.GetTypingAction();
	if (!typing || typing->NewTypingLocation())	{
		typing = handler.NewTypingAction();
		handler.PostAction(typing);
		mActiveRange = selection.GetSelectionRange();
	} else {
		if (!mActiveRange.Length())
			mActiveRange = selection.GetSelectionRange();
	}

	StRecalculator	change(&mTextEngine);
	TextRangeT		oldRange = mActiveRange,
					newRange = mActiveRange;

	typing->KeyStreamRememberPrepend(oldRange);	
	newRange.SetLength(textSD.GetDataLength());
	mTextEngine.TextReplaceByPtr(mActiveRange, (const Ptr)textSD.GetDataPtr(), newRange.Length());
	mTextEngine.SetStyleSet(newRange, selection.GetCurrentStyle());
	typing->KeyStreamRememberNew(newRange);
	
	mActiveRange = TextRangeT(oldRange.Start() + fixLength, newRange.End());
	TextRangeT	newSelRange = mActiveRange;
	newSelRange.Back();
	
	//	input (optional)
	mHilites.erase(mHilites.begin(), mHilites.end());
	if (inAppleEvent.KeyExists(keyAEHiliteRange)) {
		LAESubDesc	hiliteSD(inAppleEvent.KeyedItem(keyAEHiliteRange), typeTextRangeArray);
		TextRangeArrayPtr	p = (TextRangeArrayPtr)hiliteSD.GetDataPtr();
		mHilites.reserve(p->fNumOfRanges);
		for (Int32 i = 0; i < p->fNumOfRanges; i++) {
			RangeHiliteRec	record;
			record.range = TextRangeT(p->fRange[i].fStart, p->fRange[i].fEnd);
			record.hilite = p->fRange[i].fHiliteStyle;
			if (record.hilite < 0)
				record.hilite = -record.hilite;
			if (!record.range.Length()) {
				newSelRange = record.range;
				newSelRange.SetStart(mActiveRange.Start() + newSelRange.Start());
				Assert_(mActiveRange.WeakContains(newSelRange));
				continue;
			}
			record.range.SetStart(record.range.Start() + oldRange.Start());
			mHilites.push_back(record);
		}
	}
	selection.SetSelectionRange(newSelRange);
	
	typing->UpdateTypingLocation();
	
/*?
	inAppleEvent.KeyedItem(keyAEUpdateRange);
	typeTextRangeArray
*/	

	if (inAppleEvent.KeyExists(keyAEPinRange)) {
		TextRange	pinRange;
		inAppleEvent.KeyedItem(keyAEPinRange).ToPtr(typeTextRange, &pinRange, sizeof(pinRange));
		TextRangeT	scrollRange(pinRange.fStart + oldRange.Start(), pinRange.fEnd + oldRange.Start());
		mTextEngine.Recalc();	//	to get coordinates correct for ScrollToRange
		mTextEngine.ScrollToRange(scrollRange);
	}

/*?
	inAppleEvent.KeyedItem(keyAEClauseOffsets);
	typeOffsetArray
*/
	
	//	output
	LTextView::OutOfFocus(&mTextView);
}

//	so which is it?
#define	keyAELeadingEdge	keyAELeftSide

void	VInlineTSMProxy::AEPos2Offset(
	const LAESubDesc	&inAppleEvent,
	LAEStream			&inStream) const
{
	OSErr	err = noErr;
	
	//	input
	Point	where;
	Boolean	dragging = false;

	inAppleEvent.KeyedItem(keyAECurrentPoint).ToPtr(typeQDPoint, &where, sizeof(where));

	LAESubDesc	sd = inAppleEvent.KeyedItem(keyAEDragging);
	if (sd.GetType() != typeNull)	//	keyAEdragging is optional
		dragging = sd.ToBoolean();
	
	//	process
	LTextView::OutOfFocus(&mTextView);
	mTextView.FocusDraw();	//	for GlobalToLocal
	::GlobalToLocal(&where);
	LTextView::OutOfFocus(&mTextView);
	SPoint32	where32;
	mTextView.LocalToImagePoint(where, where32);
	Boolean		leadingEdge;
	TextRangeT	range;
	mTextEngine.Image2Range(where32, &leadingEdge, &range);
	if (dragging)
		range.Crop(mActiveRange);
		
	//	output
	inStream.WriteKey(keyAEOffset);
	Int32	offset = range.Start();
	offset -= mActiveRange.Start();
	inStream.WriteDesc(typeLongInteger, &offset, sizeof(offset));
	
	inStream.WriteKey(keyAERegionClass);
	short	aShort = kTSMOutsideOfBody;
	
	SDimension32 size;
	mTextEngine.GetImageSize(&size);
	
	if ((0 <= where32.h) && (where32.h < size.width) && (0 <= where32.v) && (where.v < size.height))
		{
		if (mActiveRange.Contains(range))
			aShort = kTSMInsideOfActiveInputArea;
		else
			aShort = kTSMInsideOfBody;
		}

	inStream.WriteDesc(typeShortInteger, &aShort, sizeof(aShort));

	//	output (optional)
	inStream.WriteKey(keyAELeadingEdge);
	char	aChar = leadingEdge;
	inStream.WriteDesc(typeBoolean, &aChar, sizeof(aChar));
}

void	VInlineTSMProxy::AEOffset2Pos(
	const LAESubDesc	&inAppleEvent,
	LAEStream			&inStream) const
{
	OSErr	err = noErr;
	
	//	input
	Int32	offset = inAppleEvent.KeyedItem(keyAEOffset).ToInt32();
	offset += mActiveRange.Start();
	
	TextRangeT	range(offset);
	range.SetInsertion(rangeCode_After);
	SPoint32	where32;
	mTextEngine.Range2Image(range, true, &where32);
	Point		where;
	mTextView.ImageToLocalPoint(where32, where);
	LTextView::OutOfFocus(&mTextView);
	mTextView.FocusDraw();	//	for LocalToGlobal
	::LocalToGlobal(&where);
	
	//	output
	inStream.WriteKey(keyAEPoint);
	inStream.WriteDesc(typeQDPoint, &where, sizeof(where));
	
	//	output (optional)
	LStyleSet	*style = mTextEngine.GetStyleSet(range);
	Int32		aLong;
	Fixed		aFixed;
	Int16		aShort = style->GetHeight();
	{
		static Str255	fName;
		StAEDescriptor	sizeD,
						fontD;
		style->GetAEProperty(pPointSize, UAppleEventsMgr::sAnyType, sizeD);
		style->GetAEProperty(pFont, UAppleEventsMgr::sAnyType, fontD);
		LAESubDesc	sizeSD(sizeD),
					fontSD(fontD);
		aFixed = WFixed::Long2FixedT(sizeSD.ToInt32()).ToFixed();
		fontSD.ToPString(fName);
		aLong = UFontsStyles::GetFontId(fName);
	}
	LTextView::OutOfFocus(&mTextView);
	
	inStream.WriteKey(keyAETSMTextFont);
	inStream.WriteDesc(typeLongInteger, &aLong, sizeof(aLong));

	inStream.WriteKey(keyAETSMTextPointSize);
	inStream.WriteDesc(typeFixed, &aFixed, sizeof(aFixed));

	inStream.WriteKey(keyAETextLineHeight);
	inStream.WriteDesc(typeShortInteger, &aShort, sizeof(aShort));

	aShort = style->GetAscent();
	inStream.WriteKey(keyAETextLineAscent);
	inStream.WriteDesc(typeShortInteger, &aShort, sizeof(aShort));

/*?
	inStream.WriteKey(keyAEAngle)
	typeFixed
	...
*/
}

//	===========================================================================
//	VTSMTEProxy

TSMTEPreUpdateUPP	VTSMTEProxy::sPreUpdateUPP = NewTSMTEPreUpdateProc(VTSMTEProxy::PreUpdate);

VTSMTEProxy::VTSMTEProxy(TEHandle inTEHandle)
{
	ThrowIfNULL_(sPreUpdateUPP);

	mTSMTEHandle = NULL;

	OSErr theErr;
	theErr = ::Gestalt(gestaltTSMTEVersion, &mVersion);
    ThrowIfOSErr_(theErr);		

	OSType theType = kTSMTEInterfaceType;
	theErr = ::NewTSMDocument(1, &theType, &mTSMDocID, (long) &mTSMTEHandle);
	ThrowIfOSErr_(theErr);

	if (!mTSMTEHandle && mTSMDocID)
		{
		::DeleteTSMDocument(mTSMDocID);
		Throw_(paramErr);
		}
	
	(*mTSMTEHandle)->textH = inTEHandle;
	(*mTSMTEHandle)->preUpdateProc = sPreUpdateUPP;
	(*mTSMTEHandle)->postUpdateProc = NULL;
	(*mTSMTEHandle)->updateFlag = kTSMTEAutoScroll;
	(*mTSMTEHandle)->refCon = (Int32)this;
}

VTSMTEProxy::~VTSMTEProxy()
{
//? FIX ME!!! do we need to dispose of mTSMTEHandle????

	// Deactivate can throw, but we dont care at this point
	try
		{
		Deactivate();	//	for a bug in TSM.  See TE27

		OSErr	err = noErr;
		if (mTSMDocID)
			err = ::DeleteTSMDocument(mTSMDocID);
		mTSMDocID = 0;
	//	Assert_(err == noErr);
		}
	catch (ExceptionCode inErr)
		{
		// no need to propogate the error here	
		}
}

pascal void VTSMTEProxy::PreUpdate(
	TEHandle 	inTEHandle,
	Int32		inRefCon)
{
	VTSMTEProxy* theProxy = (VTSMTEProxy*)inRefCon;
	Assert_(theProxy != NULL);
	
	if (theProxy->mVersion == gestaltTSMTE1)
		{
		ScriptCode keyboardScript = ::GetScriptManagerVariable(smKeyScript);
		TEHandle theTEH = theProxy->mTEH;
		
		TextStyle theStyle;
		Int16 theMode = doFont;
		if (!(::TEContinuousStyle(&theMode, &theStyle, theTEH) &&
					::FontToScript(theStyle.tsFont) == keyboardScript))
			{
			theStyle.tsFont = ::GetScriptVariable(keyboardScript, smScriptAppFond);
			::TESetStyle(doFont, &theStyle, false, theTEH);
			}
		}
}
