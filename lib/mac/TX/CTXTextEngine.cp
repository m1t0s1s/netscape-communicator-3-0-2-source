// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTXTextEngine.cp					  ©1995 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include	"CTXTextEngine.h"
#include	"CTXStyleSet.h"

#include	<UMemoryMgr.h>
#include	<UDrawingState.h>
#include	<LDataTube.h>
#include	<PP_KeyCodes.h>
#include	<LHandlerView.h>
#include	<URegions.h>
#include	<UDrawingState.h>

#include	"ObjectsRanges.h"
#include	"Textension.h"
#include	"CPageGeometry.h"

#ifdef	INCLUDE_TX_TSM
	#include	<TSMTextension.h>
#endif

#ifndef		__ERRORS__
#include	<Errors.h>
#endif

#ifndef	__AEREGISTRY__
#include "AERegistry.h"
#endif

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void	TextRange2TXRange(const TextRangeT &inRange, TOffsetRange *outRange);
void	TXRange2TextRange(const TOffsetRange &inRange, TextRangeT *outRange);

void	TextRange2TXRange(const TextRangeT &inRange, TOffsetRange *outRange)
{
	Assert_(inRange.Length() >= 0);
	
	if (inRange.Length() > 0)
		outRange->Set(inRange.Start(), inRange.End(), false, true);
/*?	Textension doesn't seem to honor its trailing edge parameters everywhere
	else if (inRange.Insertion() == rangeCode_After)
		outRange->Set(inRange.Start(), inRange.Start(), true, true);
*/
	else
		outRange->Set(inRange.Start(), inRange.Start(), false, false);
}

void	TXRange2TextRange(const TOffsetRange &inRange, TextRangeT *outRange)
{
	if (inRange.Len()) {
		*outRange = TextRangeT(inRange.Start(), inRange.End());
	} else {
/*?	Textension doesn't seem to honor its trailing edge parameters everywhere
		outRange->SetStart(inRange.Start());
		if (inRange.rangeStart.trailEdge)
			outRange->SetInsertion(rangeCode_After);
		else
			outRange->SetInsertion(rangeCode_Before);
*/
		*outRange = TextRangeT(inRange.Start());
	}
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


Boolean	CTXTextEngine::sTXInited = false;

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


void CTXTextEngine::TXInitialize(void)
{
	OSErr	err;
	
	if (!sTXInited)
		{
		#ifdef	INCLUDE_TX_TSM
			err = CTSMTextension::TSMTextensionStart();
			ThrowIfOSErr_(err);
		#else
			err = CTextension::TextensionStart();
			ThrowIfOSErr_(err);
		#endif
		
		CTXStyleSet *textRun = new CTXStyleSet();

		textRun->Reference();
		CTextension::RegisterRun(textRun);
		}	
		
	SetDefaultRun_(NULL);
	sTXInited = true;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ


CTXTextEngine::CTXTextEngine()
	:	LTextEngine()
{
	GrafPtr				port = UQDGlobals::GetQDGlobals()->thePort;
	
	mChanges = NULL;
	mText = NULL;
	
	try
		{	
		TXInitialize();
	
		#ifdef	INCLUDE_TX_TSM
			mText = new CTSMTextension;
			((CTSMTextension *)mText)->ITSMTextension(port);
		#else
			mText = new CTextension;
			mText->ITextension(port);
		#endif

		mText->Activate(false, false);
		}
	catch (ExceptionCode inErr)
		{
		delete mText;
		throw inErr;	
		}
	
	mFlags = recalc_FormatAllText;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

CTXTextEngine::~CTXTextEngine(void)
{
	mText->Free();
	delete mText;
	if ( mChanges )
		delete mChanges;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextEngine::SetDefaultRun_(CRunObject *inObj)
{
	CTextension::SetDefaultRun(inObj);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

GrafPtr	CTXTextEngine::GetPort(void)
{
	return mText->GetTextPort();
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextEngine::SetPort(GrafPtr inPort)
{
	mText->SetTextPort(inPort);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextEngine::SetWrapMode(Uint16 inMode)
{
	mWrappingMode = inMode;
	
	// FIX ME!!! need to force a reformat at this point
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Uint16 CTXTextEngine::GetWrapMode(void) const
{
	return mWrappingMode;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextEngine::SetPageFrameSize(const SDimension32& inSize)
{
	CPageGeometry* theGeometry = mText->GetPageGeometry();
	theGeometry->SetPageFrameDimensions(inSize);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextEngine::SetAttributes(Uint16 inAttributes)
{
//	textAttr_MultiStyle, textAttr_Editable, textAttr_Selectable, textAttr_WordWrap

	Uint16	oldAttributes = GetAttributes();
	
	inherited::SetAttributes(inAttributes);

	//	word wrap
	{
		Boolean			crOn = (inAttributes & textAttr_WordWrap) == 0;
		CDisplayChanges	changes;

		if (crOn != mText->GetCRFlag()) {
			mText->SetCRFlag(crOn, &changes);
			RefreshTX(changes);
		}
	}
	
	//	fixed height
	{
		CFormatter	*formatter = mText->GetFormatter();
		
		ThrowIfNULL_(formatter);
		
		short	lineHeight,
				lineAscent;
		
		formatter->GetFixedLineHeight(&lineHeight, &lineAscent);
		
		Boolean			fhOn = (lineHeight != 0);
		
		if (inAttributes & textAttr_FixedHeight)
			{
			short		height;
			FontInfo	info;

			Focus();	//	handlerview may not be set yet so...

			//	The operations below will mess with the quickdraw state info
			//	because there is no support for getting font metric information
			//	directly.
			//	
			//	We must preserve the quickdraw state info or we could even
			//	change the the quickdraw state info for the system!			
			StColorPortState	saveState(UQDGlobals::GetCurrentPort());

			CTXStyleSet	*defaultStyle = (CTXStyleSet *) GetDefaultStyleSet();	//	unsafe cast
			Assert_(defaultStyle != NULL);  // if you fail here it's because you probably didnt
											// set the default style set yet.
			defaultStyle->SetRunEnv();
			
			GetFontInfo(&info);
			height = info.ascent + info.descent + info.leading;
			
			if ((lineHeight != height) || (lineAscent != info.ascent))
				{
				formatter->SetFixedLineHeight(height, info.ascent);
				Refresh(recalc_FormatAllText);
				}
			}
		else
			{
			if (fhOn)
				{
				formatter->SetFixedLineHeight();
				Refresh(recalc_FormatAllText);
				}
			}
	}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void	CTXTextEngine::SetTextMargins(const Rect &inMargins)
{
	CPageGeometry* theGeometry = mText->GetPageGeometry();
	CDisplayChanges	changes;

	inherited::SetTextMargins(inMargins);
		
	theGeometry->SetPageMargins(mMargins, &changes);
	RefreshTX(changes);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// NOTE THAT THESE ARE IN PAGE SPACE (which only == image space when the scale is 1/1)

void	CTXTextEngine::GetImageSize(SDimension32* outSize) const
{
	CPageGeometry* theGeometry = mText->GetPageGeometry();
	theGeometry->GetActualPageFrameDimensions(*outSize);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// NOTE THAT THESE ARE IN PAGE SPACE (which only == image space when the scale is 1/1)

void	CTXTextEngine::SetImageSize(const SDimension32& inSize)
/*
	InSize.v = 0 --> automatically adjusted.
*/
{
	SDimension32	oldSize;
	
	GetImageSize(&oldSize);
	if ((oldSize.width == inSize.width) && (oldSize.height == inSize.height)) 
		return;
		
	if ((inSize.height == 0) && (oldSize.width == inSize.width))
		return;

	SDimension32	txSize;
	CPageGeometry* theGeometry = mText->GetPageGeometry();

	txSize.width = inSize.width;
	txSize.height = (inSize.height == 0) ? 0 : inSize.height;
	
	//	text image size
	{
		CDisplayChanges	changes;
		theGeometry->SetPageFrameDimensions(txSize, &changes);
		RefreshTX(changes);
	}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
// This is called by GetRangeScrollVector
void	CTXTextEngine::GetViewRect(
	SPoint32		*outOrigin,
	SDimension32	*outSize) const
{
// FIX ME!!! there's really no need for this to get called
//Assert_(false);

	if (outOrigin)
		mHandlerView->GetScrollPosition(*outOrigin);
	
	if (outSize)
		{
		Rect r;
		mHandlerView->CalcLocalFrameRect(r);
		outSize->height = r.bottom - r.top;
		outSize->width = r.right - r.left;
		}
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
void	CTXTextEngine::RefreshTX(CDisplayChanges &inChanges)
{
	TOffsetPair	pair;
	
	try
		{
		if (!mChanges)
			mChanges = new CDisplayChanges;

		mChanges->SetChangesKind(inChanges.GetChangesKind());
		inChanges.GetFormatRange(&pair);
		mChanges->SetFormatRange(pair);
		}
	catch (ExceptionCode inErr)
		{
		// just in case we couldn't allocate
		}
	
	Refresh(recalc_DisplayChanges);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextEngine::RecalcSelf(void)
{
	OSErr	err;
	
	if (mFlags & recalc_FormatAllText)
		{
		err = mText->Format();
		mFlags &= ~recalc_FormatAllText;
		ThrowIfOSErr_(err);
		}
	
	if (mFlags & recalc_DisplayChanges)
		{

		ThrowIfNULL_(mChanges);
		short	actions = mText->DisplayChanged(*mChanges);
		delete mChanges;
		mChanges = NULL;
		
		if (actions & kRedrawAll)
			BroadcastMessage(msg_InvalidateImage);
		
		if (actions & kCheckSize)
			{
			//	CheckImageSize();
			//	done in inherited method
			}
		
		if (actions & (~(kRedrawAll | kCheckSize)))
			{
			Assert_(false);		// what?
			}

		mFlags &= ~recalc_DisplayChanges;
		}
	
	LTextEngine::RecalcSelf();
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void	CTXTextEngine::DrawArea(
	const SPoint32		&inLocation,
	const SDimension32	&inSize)
{
	Rect	r;
	Point	where;
	
	inherited::DrawArea(inLocation, inSize);
	
	if (mHandlerView->CalcLocalFrameRect(r))
		EraseRect(&r);
		
	mHandlerView->ImageToLocalPoint(inLocation, where);
	r.top = where.v;
	r.left = where.h;
	r.right = inSize.width + r.left;
	r.bottom = inSize.height + r.top;

	mText->Draw(mHandlerView, r, kNoDrawFlags);
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

const TextRangeT& CTXTextEngine::GetTotalRange(void) const
{
	CTXTextEngine* me = (CTXTextEngine *)this;
	
	me->mTotalRange.SetStart(0);
	me->mTotalRange.SetEnd(me->mText->CountCharsBytes());
	
	return inherited::GetTotalRange();
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

//	===========================================================================
//	Text get/set

/*
	Note:
	
		This method is MAYBE only suitable for plain text.  Consider using
		one of the tube methods above unless really needing plain text
		from a non tube source.
*/

void	CTXTextEngine::TextGetThruHandle(const TextRangeT &inRange, Handle outTextH)
{
	TextRangeT	range = inRange;
	
	range.Crop(GetTotalRange());

	Assert_(outTextH);
	SetHandleSize(outTextH, range.Length());
	
	void	*p = mText->GetCharPtr(range.Start());
	ThrowIfNULL_(p);
	BlockMoveData(p, *outTextH, range.Length());
}


// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

/*
	Note:
	
		This method is MAYBE only suitable for plain text.  Consider using
		one of the tube methods above unless really needing plain text
		from a non tube source.
*/

void	CTXTextEngine::TextReplaceByPtr(
	const TextRangeT	&inRange,
	const Ptr			inSrcText,
	Int32				inSrcLength)
{
	TextRangeT			range = inRange;
	Int32				start, end;
	TTextDescriptor		txtDescr((const unsigned char*)inSrcText, (long)inSrcLength);
	TReplaceParams		parms(txtDescr);
	OSErr				err;
	
	range.Crop(GetTotalRange());
	start = range.Start();
	end = range.End();

	if (inSrcLength)
		{
		long	bogusLength;
		const	CRunObject	*run = mText->GetNextRun(range.Start(), &bogusLength);
	
		if (run)
			{
			parms.fReplaceObj = (CRunObject *)run;
			}
		else
			{
			ThrowIfNULL_(mDefaultStyleSet);
			parms.fReplaceObj = (CTXStyleSet *)mDefaultStyleSet;
			}
			
		ThrowIfNULL_(parms.fReplaceObj);
		parms.fTakeObjCopy = false;
		parms.fCalcScriptRuns = false;
		}
	
	SetTextChanged();
	Focus();

	{
		StRecalculator	changeMe(this);	//	950508
	
		TextFluxPre(range);
		{
			StEmptyVisRgn	hideDrawing(UQDGlobals::GetCurrentPort());

			TOffsetPair theLineFormatPair;
			err = mText->ReplaceRange(start, end, parms, false, &theLineFormatPair);

			// the replaced caused a reformat that we need to know about.  The display used to be
			// updated out of textension::EndEdit, but we dont do that anymore.
			// FIX ME!!! this is probably a general reformat problem that needs to be dealt with
			// the the textension object.  when we remove the Is-A relationship between the textension
			// object and merge it into this one, we can notify better of reformat change.

			// Perhaps we could unify textension::EndEdit, Refresh, and RefreshTX

			if (theLineFormatPair.firstOffset != theLineFormatPair.lastOffset)
				{
				CFormatter	*formatter = mText->GetFormatter();
				TOffsetRange sRange, eRange;
				TextRangeT startRange, endRange;
				
				formatter->GetLineRange(theLineFormatPair.firstOffset, &sRange);
				formatter->GetLineRange(theLineFormatPair.lastOffset, &eRange);
				TXRange2TextRange(sRange, &startRange);
				TXRange2TextRange(eRange, &endRange);
				
				TextRangeT theLineRange = startRange;
				theLineRange.Union(endRange);
				Refresh(theLineRange);
				}
		}

// This used to reformat everything on any replace (boooooooo!)
//		err = mText->Format();
	
		TextFluxPost(inSrcLength - range.Length());
		
		ThrowIfOSErr_(err);
	}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

const Handle CTXTextEngine::GetTextHandle(void) const
{
	CArray* theArray = mText->GetArray();
	ThrowIfNULL_(theArray);

	return theArray->GetDataHandle();
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

void CTXTextEngine::SetTextHandle(Handle inHandle)
{
	SetTextChanged();

	//	First delete all the text and let Textension clean up itself.
	TextDelete(sTextAll);
	
	{
		StRecalculator	changeMe(this);

		//	Now set the text...
		TextRangeT	preRange,
					postRange;
		
		preRange = GetTotalRange();
	
		CArray	*array = mText->GetArray();
		ThrowIfNULL_(array);
	
		SetStyleSet(sTextAll, GetDefaultStyleSet());
	
		//	clean the rulersets
		//	???
		
		//	swap the data handle
		TextFluxPre(preRange);
		array->SetDataHandle(inHandle);
		TextFluxPost(GetTotalRange().Length() - preRange.Length(), false);
		Refresh(recalc_FormatAllText);
		
		SetStyleSet(sTextAll, GetDefaultStyleSet());

		//	apply default rulerset
		//	???
	}
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

//	===========================================================================
//	Presentation query

Int32 CTXTextEngine::GetRangeTop(const TextRangeT &inRange) const
{
	CPageGeometry* theGeometry = mText->GetPageGeometry();
	CFormatter	*formatter = mText->GetFormatter();
	TextRangeT	range = inRange,
				totalRange;
				
	totalRange = GetTotalRange();
	range.Crop(totalRange);
	range.Front();
	Int32		offset = range.Start();
	Int32		lineNo = formatter->OffsetToLine(offset);

	SPoint32	Pt;
	
// FIX ME!!! this will need to be an image point instead of a page point
// when we implement scaling
	theGeometry->LineToPagePoint(mHandlerView, lineNo, Pt);

	return Pt.v;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Int32 CTXTextEngine::GetRangeHeight(const TextRangeT &inRange) const
{
	Int32			rval = 0;
	CPageGeometry* theGeometry = mText->GetPageGeometry();
	CFormatter		*formatter = mText->GetFormatter();
	CLineHeights	*frameFormatter = mText->GetLineHeights();
	TextRangeT		range,
					totalRange;
	totalRange = GetTotalRange();
	
	//	first line
	range = inRange;
	range.Crop(totalRange);
	range.Front();
	Int32		startLine = formatter->OffsetToLine(range.Start());
	
	//	end line
	range = inRange;
	range.Crop(totalRange);
	Int32	lastChar;
	if (range.Length()) {
		range.Back();
		lastChar = range.End() - PrevCharSize(range.End());
	} else {
		lastChar = range.Start();
	}
	Int32	lastLine = formatter->OffsetToLine(lastChar);
	
	rval = frameFormatter->GetLineRangeHeight(startLine, lastLine);
	
	return rval;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean	CTXTextEngine::Range2Image(
	const TextRangeT	&inRange,
	Boolean				inLeadingEdge,
	SPoint32			*outWhere) const
/*
	If inRange is an insertion point, inLeadingEdge is ignored.
*/
{
	TextRangeT	range = inRange;
	
	range.Crop(GetTotalRange());
	
	if (inLeadingEdge)
		range.Front();
	else
		range.Back();
	
//	Assert_(mFlags == 0);
	
	TOffset	offset = range.Start();
	CFormatter			*formatter = mText->GetFormatter();
	long				lineNo = formatter->OffsetToLine(offset);
	TOffsetRange		lineRange;
	formatter->GetLineRange(lineNo, &lineRange);

	Boolean				doEOL =	(range.Insertion() == rangeCode_Before) &&
								(range.Start() == lineRange.Start()) &&
								inRange.Length();

	if (doEOL && lineNo)
		lineNo--;	
	Assert_(lineNo >= 0);
	
	CPageGeometry* theGeometry = mText->GetPageGeometry();
	CTextRenderer	*display = mText->GetDisplayHandler();
	CLineHeights		*frameFormatter = mText->GetLineHeights();
	SPoint32			linePoint;
	
// FIX ME!!! this will need to be an image point instead of a page point
// when we implement scaling
	theGeometry->LineToPagePoint(mHandlerView, lineNo, linePoint);
	
	CLine				*line = display->DoLineLayout(lineNo);

	
	linePoint.v += frameFormatter->GetLineRangeHeight(lineNo, lineNo);
	if (doEOL)
		{
		linePoint.h = mImageSize.width - mMargins.right;
		}
	else
		{
		offset = range.Start();
		linePoint.h += line->CharacterToPixel(offset);
		}

	ThrowIfNULL_(outWhere);
	outWhere->h = linePoint.h;
	outWhere->v = linePoint.v;

	return true;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean	CTXTextEngine::Image2Range(
	SPoint32			inWhere,
	Boolean				*outLeadingEdge,
	TextRangeT			*outRange) const
{
	TextRangeT	rangeFound,
				testRange;
	Boolean		leadingEdge;

	//	Get initial rangeFound
	//	independent from mHandlerView...

	TOffsetRange		tRange;

	CTextRenderer	*display = mText->GetDisplayHandler();
	CPageGeometry*		 theGeometry = mText->GetPageGeometry();
	CFormatter			*formatter = mText->GetFormatter();
	CLineHeights		*frameFormatter = mText->GetLineHeights();

	SPoint32			where;
	where.h = inWhere.h;
	where.v = inWhere.v;

	//	¥	Get lineNo
	//	equiv of Display::PointToLine...
//	long				frameNo = frames->PointToFrame(where, &outsideFrames);

	TLongRect thePageText;
	theGeometry->GetPageTextBounds(thePageText);
	Boolean		outsideFrames = !thePageText.IsPointInside(inWhere);
// FIX ME!!! use of inWhere in the above line assumes a zoom scale of 1/1

	Boolean outsideLines = outsideFrames;

	Int32				lineNo;
	TOffsetPair			frameLines;
	//	equiv of CFrame::PointToLine
	if (!frameFormatter->GetTotalLineRange(frameLines))
		{
		lineNo = -1;
		outsideLines = true;
		}
	else
		{
		where.h = inWhere.h;
		where.v = inWhere.v - mMargins.top; //	note correction for margin.

		lineNo = frameFormatter->PixelToLine(&where.v);	// changes where!
		
		if (lineNo > frameLines.lastOffset)
			{
			outsideLines = true;
			lineNo = frameLines.lastOffset;
			}
		}

	//	remainder of CTextensionDisplay::PointToChar...
	if (lineNo < 0)
		{
		tRange.Set(0, 0, true, false);
		} 
	else
		{
		SPoint32			linePoint;
		theGeometry->LineToPagePoint(mHandlerView, lineNo, linePoint);

		where.h = inWhere.h;
		where.v = inWhere.v;
 		
		if (	(lineNo == formatter->CountLines()-1) &&
				where.v > linePoint.v + frameFormatter->GetLineRangeHeight(lineNo, lineNo)
		)	{
				tRange.rangeStart.Set(formatter->GetLineEnd(lineNo), true);
				tRange.rangeEnd = tRange.rangeStart;
			}
		else if (	(lineNo == 0) && (where.v < linePoint.v) )
			{
			tRange.rangeStart.Set(0, false);
			tRange.rangeEnd = tRange.rangeStart;
			}
		else
			{
			CLine	*line = display->DoLineLayout(lineNo);
			line->PixelToCharacter(LongToFixed(where.h - linePoint.h), &tRange);
			}
		}
		
	TXRange2TextRange(tRange, &rangeFound);
	
	if (outsideFrames || outsideLines)
		{
		SPoint32	anchor;
		Boolean		b = Range2Image(rangeFound, true, &anchor);
		
		Assert_(b);
		
		if (inWhere.h <= anchor.h)
			{
			leadingEdge = false;
			rangeFound.SetInsertion(rangeCode_After);
			}
		else
			{
			leadingEdge = true;
			rangeFound.SetInsertion(rangeCode_Before);
			}
		}
	else
		{
		rangeFound.SetLength(ThisCharSize(rangeFound.Start()));		
		
		//	Code from here is paralleled in LTETextEngine...
		SPoint32	anchor,	//	Lower left of found insertion position
					boat;	//	lower left of "next" insertion position

		Range2Image(rangeFound, true, &anchor);
		
		//	normal case
		leadingEdge = where.h >= anchor.h;
		
		if (leadingEdge)
			{
			testRange = rangeFound;
			}
		else
			{
			Int16	len = PrevCharSize(rangeFound.Start());
			
			testRange.SetStart(rangeFound.Start() - len);
			testRange.SetLength(len);
			}
			
		if (testRange.Length())
			{
			Range2Image(testRange, !leadingEdge, &boat);
			if (boat.v == anchor.v)
				rangeFound = testRange;
			}
		}

	//	return results
	rangeFound.Crop(GetTotalRange());

	if (outLeadingEdge)
		*outLeadingEdge = leadingEdge;

	if (outRange)
		*outRange = rangeFound;
		
	return true;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Boolean	CTXTextEngine::SpaceForBytes(Int32 inAdditionalBytes) const
{
	return true;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

//	===========================================================================
//	Parts implementation:

DescType	CTXTextEngine::FindPara(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
/*
	Should only be called by FindPart
*/
{
	//	sanity checks
	ThrowIfNULL_(ioRange);

	TOffsetRange	trange;
	TextRangeT		range = *ioRange,
					rangeFound,
					rangeReported;
	
	if (range.Insertion() == rangeCode_Before) {
		range.SetStart(range.Start() - PrevCharSize(range.Start()));
		range.SetInsertion(rangeCode_After);
	}
	
	//	Initial validity check
	if (range.Start() < inQueryRange.Start())
		return typeNull;
	if (range.Start() >= inQueryRange.End())
		return typeNull;

	//	Find
	mText->CharToParagraph(range.Start(), &trange);
	TXRange2TextRange(trange, &rangeFound);
	
	if (rangeFound.Length() <= 0)
		return typeNull;
	
	//	Truncate to query range.
	rangeReported = rangeFound;
	rangeReported.Intersection(inQueryRange);
	if (rangeReported.Undefined())
		return typeNull;
		
	//	Report
	*ioRange = rangeReported;
	if (outEndOffset)
		*outEndOffset = rangeReported.End();
	
	if (rangeFound == rangeReported)
		return cParagraph;
	else
		return cChar;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

DescType	CTXTextEngine::FindLine(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
/*
	Should only be called by FindPart
*/
{
	//	sanity checks
	ThrowIfNULL_(ioRange);

	Int32			line;
	TOffsetRange	trange;
	TextRangeT		range = *ioRange,
					rangeFound,
					rangeReported;
	
	//	Initial validity check
	if (!inQueryRange.Contains(range))
		return typeNull;

	Int32	offset = range.Start();
	if ((range.Insertion() == rangeCode_Before) && (offset > 0))
		offset--;	//	Even if it isn't a whole character it will be good enough for CharToLine
	
	//	Find
	line = mText->CharToLine(offset, &trange);
	if (line < 0)
		return typeNull;
	TXRange2TextRange(trange, &rangeFound);
	
	//	Truncate to query range.
	rangeReported = rangeFound;
	rangeReported.Intersection(inQueryRange);
	if (rangeReported.Undefined())
		return typeNull;
		
	//	Report
	*ioRange = rangeReported;
	if (outEndOffset)
		*outEndOffset = rangeReported.End();
	
	if (rangeFound == rangeReported)
		return cLine;
	else
		return cChar;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

DescType	CTXTextEngine::FindWord(
	const TextRangeT	&inQueryRange,
	TextRangeT			*ioRange,
	Int32				*outEndOffset) const
{
	//	sanity checks
	ThrowIfNULL_(ioRange);

	TOffsetRange	trange;
	TextRangeT		range = *ioRange,
					rangeFound,
					rangeReported;
	
	if (range.Insertion() == rangeCode_Before) {
		range.SetStart(range.Start() - PrevCharSize(range.Start()));
		range.SetInsertion(rangeCode_After);
	}
	
	//	Initial validity check
	if (range.Start() < inQueryRange.Start())
		return typeNull;
	if (range.Start() >= inQueryRange.End())
		return typeNull;

	//	Find
	mText->CharToWord(ioRange->Start(), &trange);
	TXRange2TextRange(trange, &rangeFound);
	
	if (rangeFound.Length() <= 0)
		return typeNull;
		
	//	Truncate to query range.
	rangeReported = rangeFound;
	rangeReported.Intersection(inQueryRange);
	if (rangeReported.Undefined())
		return typeNull;
		
	//	Report
	*ioRange = rangeReported;
	if (outEndOffset)
		*outEndOffset = rangeReported.End();
	
	if (rangeFound != rangeReported)
		return cChar;
	
	return cWord;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

Int32	CTXTextEngine::CountLines(const TextRangeT &inQueryRange) const
{
	CFormatter	*formatter = mText->GetFormatter();
	ThrowIfNULL_(formatter);
	
	TOffsetRange	qRange;
	
	Int32			startLine,
					endLine;
	
	TextRange2TXRange(inQueryRange, &qRange);
	startLine = formatter->OffsetToLine(qRange.rangeStart);
	endLine = formatter->OffsetToLine(qRange.rangeEnd);
	
	return endLine - startLine + 1;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

DescType	CTXTextEngine::FindNthLine(
	const TextRangeT	&inQueryRange,
	Int32				inPartIndex,
	TextRangeT			*outRange) const
{
	CFormatter	*formatter = mText->GetFormatter();
	ThrowIfNULL_(formatter);
	
	TOffsetRange	qRange,
					oRange;
	TextRangeT		rangeFound,
					rangeReported;
	Int32			originLine;
	
	TextRange2TXRange(inQueryRange, &qRange);
	
	originLine = formatter->OffsetToLine(qRange.rangeStart);
	
	originLine += inPartIndex -1;
	
	formatter->GetLineRange(originLine, &oRange);
	TXRange2TextRange(oRange, &rangeFound);
	
	rangeFound.Crop(inQueryRange);

	ThrowIfNULL_(outRange);
	*outRange = rangeFound;
	
	if (*outRange == rangeFound)
		return cLine;
	else
		return cChar;
}

// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	¥	
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

DescType	CTXTextEngine::FindLineSubRangeIndices(
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inSubRange,
	Int32				*outIndex1,
	Int32				*outIndex2) const
{
	CFormatter	*formatter = mText->GetFormatter();
	ThrowIfNULL_(formatter);
	
	TOffsetRange	tRange,
					sRange,
					eRange;
	TextRangeT		resultRange,
					startRange,
					endRange;
	Int32			originLine,
					startLine,
					endLine;
	
	TextRange2TXRange(inQueryRange, &tRange);
	
	originLine = formatter->OffsetToLine(tRange.rangeStart);
	
	TextRange2TXRange(inSubRange, &tRange);
	startLine = formatter->OffsetToLine(tRange.rangeStart);
	endLine = formatter->OffsetToLine(tRange.rangeEnd);
	
	formatter->GetLineRange(startLine, &sRange);
	formatter->GetLineRange(endLine, &eRange);
	
	TXRange2TextRange(sRange, &startRange);
	TXRange2TextRange(eRange, &endRange);
	
	resultRange = startRange;
	resultRange.Union(endRange);
	
	*outIndex1 = startLine - originLine + 1;
	*outIndex2 = endLine - originLine + 1;
	
	if (resultRange == inSubRange)
		return cLine;
	else
		return cChar;
}

