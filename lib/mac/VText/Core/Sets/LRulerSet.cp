//	===========================================================================
//	LRulerSet.h	
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#include	"LRulerSet.h"
#include	<AEVTextSuite.h>
#include	<VOTextModel.h>
#include	<LStream.h>
#include	<VLine.h>
#include	<VPartCache.h>
#include	<UAppleEventsMgr.h>
#include	<UDrawingState.h>
#include	<VCharScanner.h>
#include	<LString.h>

#pragma	warn_unusedarg off

//	----------------------------------------------------------------------

StSystemDirection::StSystemDirection(Boolean inLeftToRight)
{
	Boolean	isLeftToRight = (GetSysDirection() == 0);
	
	mWasLeftToRight = isLeftToRight;
	
	if (isLeftToRight != inLeftToRight)
		SetSysDirection(inLeftToRight ? 0 : -1);
}

StSystemDirection::~StSystemDirection()
{
	Boolean	isLeftToRight = (GetSysDirection() == 0);
	
	if (isLeftToRight != mWasLeftToRight)
		SetSysDirection(mWasLeftToRight ? 0 : -1);
}

//	----------------------------------------------------------------------

/*-
FixedT	TabRecord::NextStop(FixedT  inIndentInTab) const
{
	FixedT	rval = location;
	
	if (inIndentInTab > FixedT::sZero) {
		ThrowIf_(repeat == FixedT::sZero);
		Int16	occurance = inIndentInTab / repeat;
		FixedT	remainder = inIndentInTab / repeat;
		
		if (remainder != FixedT::sZero)
			occurance++;
		
		rval += remainder * occurance;
	}
	
	return rval;
}
*/

LRulerSet::LRulerSet(const LRulerSet *inOriginal)
{
	mName[0] = '\0';
	SetModelKind(cRulerSet);
	mParent = inOriginal->mParent;

	mBegin = inOriginal->mBegin;
	mBeginFirst = inOriginal->mBeginFirst;
	mEnd = inOriginal->mEnd;
	
	mBeginDelta = FixedT::sZero;
	mBeginFirstDelta = FixedT::sZero;
	mEndDelta = FixedT::sZero;

	mJustification = inOriginal->mJustification;
	mIsLeftToRight = inOriginal->mIsLeftToRight;
	
	mPixels = inOriginal->mPixels;
	mParentPixels = inOriginal->mParentPixels;
	
	mTabs = inOriginal->mTabs;
}

LRulerSet::LRulerSet()
{
	mName[0] = '\0';
	SetModelKind(cRulerSet);
	mParent = NULL;

	mBegin = FixedT::sZero;
	mBeginFirst = FixedT::sZero;
	mEnd = FixedT::sZero;
	
	mBeginDelta = FixedT::sZero;
	mBeginFirstDelta = FixedT::sZero;
	mEndDelta = FixedT::sZero;

	mJustification = kVTextRulerLeading;
	mIsLeftToRight = true;
	
#if	1
//	the release code
	TabRecord	tab;
	tab.location = FixedT::sZero;
	tab.repeat = WFixed::Long2FixedT(36);	//	half inch
	tab.repeat = WFixed::Long2FixedT(24);	//	4 monaco 9pt spaces
	tab.orientation = kVTextTabLeading;
	tab.delimiter = 0;

	mTabs.push_back(tab);

#else

//	just for a promo
	#define		inch	72
	TabRecord	tab;

/*
	tab.orientation = kVTextTabLeading;
	tab.location = WFixed::Long2FixedT(inch);
	tab.repeat = WFixed::Long2FixedT(4 * inch);
//tab.repeat = FixedT::sZero;
	tab.delimiter = 0;
	mTabs.push_back(tab);
*/
	tab.orientation = kVTextTabCentered;
	tab.location = WFixed::Long2FixedT(2 * inch);
	tab.repeat = WFixed::Long2FixedT(4 * inch);
//tab.repeat = FixedT::sZero;
	tab.delimiter = 0;
	mTabs.push_back(tab);

	tab.orientation = kVTextTabAligned;
	tab.location = WFixed::Long2FixedT(4 * inch + inch / 2);
	tab.repeat = WFixed::Long2FixedT(4 * inch);
//tab.repeat = FixedT::sZero;
	tab.delimiter = '.';
	mTabs.push_back(tab);

	tab.orientation = kVTextTabTrailing;
	tab.location = WFixed::Long2FixedT(7 * inch);
	tab.repeat = WFixed::Long2FixedT(4 * inch);
//tab.repeat = FixedT::sZero;
	tab.delimiter = 0;
	mTabs.push_back(tab);
#endif
}

LRulerSet::~LRulerSet()
{
}

#ifndef	PP_No_ObjectStreaming

void	LRulerSet::WriteObject(LStream &inStream) const
{
	inStream << (Int16) 1;
	inStream << mJustification;
	inStream << (mIsLeftToRight ? kVTextLeftToRight : kVTextRightToLeft);
	inStream <<	mBeginDelta.ToFixed();
	inStream << mBeginFirstDelta.ToFixed();
	inStream << mEndDelta.ToFixed();
	inStream << mName;
	
	inStream << (Int16) mTabs.size();
	
	vector<TabRecord>::const_iterator	ip = mTabs.begin();
	while (ip != mTabs.end()) {
		const TabRecord	&tab = *ip;
		
		inStream << (Int16) 1;
		inStream << tab.orientation;
		inStream << tab.location.ToFixed();
		inStream << tab.repeat.ToFixed();
		inStream << tab.delimiter;
		
		ip++;
	}
	
	inStream << dynamic_cast<LTextModel *>(GetSuperModel());
	inStream << (LRulerSet *)NULL;
}

void	LRulerSet::ReadObject(LStream &inStream)
{
	Int16	version;
	
	inStream >> version;
	switch (version) {
		case 1:
		{
			inStream >> mJustification;

			OSType	type;
			inStream >> type;
			mIsLeftToRight = (type == kVTextLeftToRight) ? true : false;

			Fixed	fixed;
			inStream >> fixed;
			mBeginDelta = WFixed::Fix2FixedT(fixed);

			inStream >> fixed;
			mBeginFirstDelta = WFixed::Fix2FixedT(fixed);

			inStream >> fixed;
			mEndDelta = WFixed::Fix2FixedT(fixed);

			inStream >> mName;
			
			Int16	count;
			inStream >> count;
			
			mTabs.erase(mTabs.begin(), mTabs.end());
			TabRecord	tab;
			for (Int16 i = 0; i < count; i++) {
				inStream >> version;
				switch (version) {
					case 1:
					{
						inStream >> tab.orientation;

						inStream >> fixed;
						tab.location = WFixed::Fix2FixedT(fixed);

						inStream >> fixed;
						tab.repeat = WFixed::Fix2FixedT(fixed);
						
						inStream >> tab.delimiter;
						
						mTabs.push_back(tab);
						break;
					}
				
					default:
						Throw_(paramErr);
						break;
				}
			}
			break;
		}
		
		default:
			Throw_(paramErr);	//	unrecognized version
			break;
	}
	
	SetSuperModel(ReadStreamable<LTextModel *>()(inStream));
	
	LRulerSet	*parent = ReadStreamable<LRulerSet *>()(inStream);
	Assert_(!parent);	//	not yet implemented;
		
	Changed();
}

#endif

Boolean	LRulerSet::operator==(const LRulerSet &inRhs) const
{
	if (this == &inRhs)
		return true;

/*?
	return	(mFontNumber == inRhs.mFontNumber) &&
			(mFontSize == inRhs.mFontSize) &&
			(mFontFace == inRhs.mFontFace) &&
			(mXferMode == inRhs.mXferMode) &&
			(mFontColor.red == inRhs.mFontColor.red) &&
			(mFontColor.green == inRhs.mFontColor.green) &&
			(mFontColor.blue == inRhs.mFontColor.blue);
*/
return false;
}
	
//	----------------------------------------------------------------------

void	LRulerSet::WriteFlavorRecord(
	FlavorType	inFlavor,
	LStream		&inStream) const
{
	switch (inFlavor) {

		case typeVRulerSets:
		{
			StAEDescriptor	desc;
			GetImportantAEProperties(desc.mDesc);
			inStream << desc.mDesc.descriptorType;
			inStream << desc.mDesc.dataHandle;
			break;
		}
		
		case typeVRulerSetRuns:
		default:
			Throw_(paramErr);
	}
}

void	LRulerSet::ReadFlavorRecord(
	FlavorType	inFlavor,
	LStream		&inStream)
{
	switch (inFlavor) {

		case typeVRulerSets:
		{
/*
			StAEDescriptor	desc;
			GetImportantAEProperties(desc.mDesc);
			inStream << desc.mDesc.descriptorType;
			inStream << desc.mDesc.dataHandle;
*/
Assert_(false);
			break;
		}
		
		case typeVRulerSetRuns:
		default:
			Throw_(paramErr);
	}
}

//	----------------------------------------------------------------------

void	LRulerSet::SetSuperModel(LModelObject *inSuperModel)
{
	LModelObject::SetSuperModel(inSuperModel);
	
	if (!GetParent())
		Changed();
}

void	LRulerSet::SetParent(const LRulerSet *inRuler)
{
	LRulerSet	*ruler = (LRulerSet*)inRuler;
	
	if (mParent != ruler) {
		ReplaceSharable(mParent, ruler, this);
		Changed();
	}
}

Boolean	LRulerSet::IsLeftToRight(void) const
{
	return mIsLeftToRight;
}

const Range32T &	LRulerSet::GetParentPixels(void) const
{
	return mParentPixels;
}

const Range32T &	LRulerSet::GetPixels(void) const
{
	return mPixels;
}

void	LRulerSet::Changed(void)
{
	VOTextModel *model = dynamic_cast<VOTextModel*>(GetSuperModel());

	if (model) {
		if (mParent) {
			mParentPixels = mParent->GetPixels();

			mBegin = mParent->mBegin;
			mBeginFirst = mParent->mBeginFirst;
			mEnd = mParent->mEnd;
		} else {
			LTextEngine	*textEngine = model->GetTextEngine();
			
			if (textEngine) {
				Rect			r;
				SDimension32	size;
				textEngine->GetTextMargins(&r);
				textEngine->GetImageSize(&size);
				if (size.width > (r.left + r.right))
					mParentPixels = Range32T(r.left, size.width - r.right);
				else
					mParentPixels = Range32T::sUndefined;
			} else {
				mParentPixels = Range32T::sUndefined;
			}
		
			mBegin = FixedT::sZero;
			mBeginFirst = FixedT::sZero;
			mEnd = WFixed::Long2FixedT(mParentPixels.Length());
		}
		mBegin += mBeginDelta;
		mBeginFirst = mBegin;
		mBeginFirst += mBeginFirstDelta;
		mEnd += mEndDelta;

		mPixels = GetParentPixels();
		if (IsLeftToRight()) {
			mPixels.SetStart(mPixels.Start() + mBeginDelta.ToLong());
			mPixels.SetEnd(mPixels.End() + mEndDelta.ToLong());
		} else {
			mPixels.SetStart(mPixels.Start() - mEndDelta.ToLong());
			mPixels.SetEnd(mPixels.End() - mBeginDelta.ToLong());
		}

		model->RulerChanged(this);
	}
}

//	----------------------------------------------------------------------

vector<TabRecord>::const_iterator	LRulerSet::FindTab(FixedT inIndent) const
{
	vector<TabRecord>::const_iterator	tp = mTabs.end();
	
	if (mTabs.size()) {
		tp--;
		while (true) {
			const TabRecord &	tab = *tp;

			if (inIndent >= (tab.location)) {
				if (tab.repeat == FixedT::sZero) {
					tp++;
					break;
				} else {
					FixedT	stop = tab.location;					
					stop += tab.repeat * WFixed::Long2FixedT(FindTabRepetition(*tp, inIndent));
					
					if (stop >= mEnd) {
						tp = mTabs.end();
					} else if (tp+1 != mTabs.end()) {
						if (stop >= (*(tp+1)).location)
							tp++;
					}
					break;
				}
			}

			if (tp == mTabs.begin()) {
				break;
			}
			tp--;
		}
	}
	
	return tp;
}

Int32	LRulerSet::FindTabRepetition(
	const TabRecord			&inTab,
	FixedT					inIndent) const
{
	Assert_(inTab.repeat != FixedT::sZero);
	
	if (inIndent >= inTab.location) {
		FixedT	rval = (inIndent - inTab.location) / inTab.repeat;
		return rval.ToLong() + 1;
	} else {
		return 0;
	}
}

void	LRulerSet::NextTabStop(
	FixedT 					inIndent,
	FixedT					&outStop,
	VTextTabOrientationE	&outType,
	Int16					&outAligner) const
{
	if ((inIndent < mBegin) || (inIndent < mBeginFirst)) {

		//	closest indent or begin
		if (inIndent < mBegin)
			outStop = mBegin;
		if (inIndent < mBeginFirst)
			outStop = min(outStop, mBeginFirst);
		outType = kVTextTabLeading;
		outAligner = 0;
	} else {
		vector<TabRecord>::const_iterator	tp = FindTab(inIndent);
		
		if (tp == mTabs.end()) {

			//	past all tabs
			outStop = mEnd;
			outType = kVTextTabTrailing;
			outAligner = 0;
		} else {
		
			//	look it up
			const TabRecord &	tab = *tp;
			outStop = tab.location;
			outType = tab.orientation;
			outAligner = tab.delimiter;
			if (tab.repeat != FixedT::sZero) {
				outStop += tab.repeat * WFixed::Long2FixedT(FindTabRepetition(tab, inIndent));
			}
		}
	}
}

Int32	LRulerSet::NewTabIndex(const TabRecord &inTabRecord) const
{
	vector<TabRecord>::const_iterator
		ip = mTabs.begin(),
		ie = mTabs.end();
	
	while (ip != ie) {
		if (inTabRecord.location < (*ip).location) {
			break;
		}
		ip++;
	}
	
	return ip - mTabs.begin();
}

void	LRulerSet::DeleteTab(TabRecord *inTabRecord)
{
	vector<TabRecord>::iterator
		ip = mTabs.begin(),
		ie = mTabs.end();
	
	while (ip != ie) {
		if (&(*ip) == inTabRecord) {
			mTabs.erase(ip, ip+1);
			break;
		}
		ip++;
	}
}

//	----------------------------------------------------------------------

void	LRulerSet::DrawLine(
	const VTextEngine	&inTextEngine,
	const Ptr			inText,
	Int32				inLineIndex) const
/*
	You can override this member for rulers that don't have to layout
	text before associated lines can be drawn.
	
	For most uses this is very rare.  But for unidirectional text with
	only leading edge justfication, it is significantly faster.
*/
{
	VLine	*line = inTextEngine.GetLine(inLineIndex);
	line->Draw(inText);
}
						
void	LRulerSet::LayoutLine(
	const VTextEngine	&inTextEngine,
	const Ptr			inText,
	const TextRangeT	&inParaRange,
	const TextRangeT	&inLineStartPt,
	VLine				&ioLine
) const
{
StSystemDirection	tempChange(IsLeftToRight());
	ThrowIf_(inLineStartPt.Length());
	Assert_(inParaRange.Contains(inLineStartPt));
	Assert_(	inTextEngine.GetTotalRange().Contains(inParaRange)
				||	(
					(inParaRange.Length() == 0) &&
					(inParaRange.Start() == inTextEngine.GetTotalRange().End())
				)
	);
	
	const TextRangeT		scanRange(inLineStartPt.Start(), inParaRange.End());
	TextRangeT				scriptRange = inLineStartPt,
							styleRange = inLineStartPt,
							nextTabRange = inLineStartPt,
							alignerRange(scanRange.End()),
							runRange = inLineStartPt;
	FixedT					indent = inLineStartPt.Start() == inParaRange.Start()
										?	mBeginFirst
										:	mBegin,
							widthRemaining = mEnd - indent,
							widthUnused;
	LStyleSet				*style = NULL;
	StyledLineBreakCode		breakCode;
	LineMemorySegmentT		segment;
	VCharScanner			tabScanner(inTextEngine, scanRange, runRange),
							alignScanner(inTextEngine, scanRange, runRange);
	
	Int32					beginPixel = IsLeftToRight()
								?	GetPixels().Start()
								:	GetPixels().End();
	
	if (inLineStartPt.Start() == inParaRange.Start()) {
		//	Fix me -- this way loses accuracy -- there should be a better way of determining the first
		//	pixel for the first line
		if (IsLeftToRight)
			beginPixel += mBeginFirstDelta.ToLong();
		else
			beginPixel -= mBeginFirstDelta.ToLong();
	}
								
	ioLine.Open(
		WFixed::Long2FixedT(beginPixel),
		IsLeftToRight(),
		(inTextEngine.GetStyleSet(inLineStartPt))->GetHeight(),
		(inTextEngine.GetStyleSet(inLineStartPt))->GetAscent(),
		inText,
		inLineStartPt.Start() == inParaRange.Start()
	);

	//	еее	for every run (style | tab | tab aligner) -- loop runs in memory order
	do {
	
		//	ее	update loop ranges
		
		//	nextTabRange
		if (runRange.End() >= nextTabRange.End()) {
			tabScanner.FindNext('\t');
			nextTabRange = tabScanner.PresentRange();
		}
		
		//	styleRange
		if (runRange.End() >= styleRange.End()) {
			styleRange.After();
			style = inTextEngine.GetStyleSet(styleRange);
			inTextEngine.GetStyleSetRange(&styleRange);
			styleRange.Crop(scanRange);
		}
		
		//	scriptRange
		if (runRange.End() >= scriptRange.End()) {
			scriptRange.After();
			inTextEngine.GetScriptRange(scriptRange, scriptRange);
			scriptRange.Crop(scanRange);
		}
		
		//	runRange
		if ((runRange.Start() == nextTabRange.Start()) && nextTabRange.Length()) {

			//	ее	do the tab
			runRange = nextTabRange;
			
			//	purge/sort segments for display
			FixedT	correction = ioLine.SyncSegments(widthRemaining);
			indent += correction;
			widthRemaining -= correction;
			
			//	get important values for this tab
			FixedT					tabStop;
			VTextTabOrientationE	tabType;
			Int16					tabAligner;
			NextTabStop(indent, tabStop, tabType, tabAligner);
			FixedT					widthToTabStop = tabStop - indent;
		
			Assert_(widthToTabStop >= 0);
			
			//	find alignerRange
			if (tabType == kVTextTabAligned) {
				Int32	oldTabEnd = nextTabRange.End();
				tabScanner.FindNext('\t');
				nextTabRange = tabScanner.PresentRange();
				
				alignScanner.Reset(TextRangeT(oldTabEnd, nextTabRange.Start()), TextRangeT(oldTabEnd));
				alignScanner.FindNext(tabAligner);
				alignerRange = alignScanner.PresentRange();
			}

			//	output segment for this tab
			segment.range = runRange;
			segment.style = style;
			segment.width = (tabType == kVTextTabLeading)
								?	widthToTabStop
								:	FixedT::sZero;
			ioLine.AddSegment(segment);
			ioLine.NoteTab(tabType, widthToTabStop);
			
			breakCode = smBreakOverflow;

		} else {

			//	ее	put a range thru find break
		
			if ((runRange.Start() == alignerRange.Start()) && alignerRange.Length()) {
				runRange = alignerRange;
				ioLine.NoteTabAligner(runRange);
				alignerRange = scanRange;
				alignerRange.Back();
			} else {
				Int32	runEnd = styleRange.End();
				
				runEnd = min(runEnd, nextTabRange.Start());
				if (alignerRange.Length())
					runEnd = min(runEnd, alignerRange.Start());

				runRange.SetEnd(runEnd);
			}
			style->LineBreakScan(
								scanRange, scriptRange, runRange, inText, widthRemaining,
								runRange, widthUnused, breakCode);

			//	line was previously finished but couldn't tell until just now
			if (runRange.Undefined())
				break;
			
			segment.range = runRange;
			segment.style = style;
			segment.width = widthRemaining - widthUnused;
			ioLine.AddSegment(segment);
		}
		indent += segment.width;
		widthRemaining -= segment.width;
		runRange.After();
	} while (
		(breakCode == smBreakOverflow) &&
		(runRange.Start() < scanRange.End()) &&
		(widthRemaining > 0)
	);
	
	//	еее	Close up line
	TextRangeT			range = ioLine.GetRange();

	if (range.Undefined()) {
		Assert_(inParaRange.End() == inTextEngine.GetTotalRange().End());
		range = TextRangeT(inTextEngine.GetTotalRange().End(), 0, rangeCode_After);
	} else if ((range.End() < inParaRange.End()) && (inTextEngine.ThisChar(range.End()) == 0x0d)) {
		range.SetEnd(range.End() + 1);
	}
	
	VTextJustificationE	justification = mJustification;
	if (justification == kVTextRulerFilled) {
		//	never fill the last line of a paragraph
		if (range.End() == inParaRange.End())
			justification = kVTextRulerLeading;
	}
	
	ioLine.Close(range, justification, widthRemaining, range.End() == inParaRange.End());
}

//	----------------------------------------------------------------------

void	LRulerSet::GetAEProperty(
	DescType		inProperty,
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
	OSErr	err = noErr;
	
	switch (inProperty) {

		case pName:
		{
			LAEStream	stream;
		
			stream.WriteDesc(typeChar, mName + 1, mName[0]);
			stream.Close(&outPropertyDesc);
			break;
		}
		
		case pVTextDirection:
		{
			DescType	direction = !mIsLeftToRight
							? kVTextRightToLeft
							: kVTextLeftToRight;
			err = AECreateDesc(typeEnumeration, &direction, sizeof(direction), &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;
		}
		
		case pVTextJustification:
			err = AECreateDesc(typeEnumeration, &mJustification, sizeof(mJustification), &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;
			
		case pBegin:
		{
			Fixed	fixed = mBeginDelta.ToFixed();
			err = AECreateDesc(typeFixed, &fixed, sizeof(fixed), &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;
		}
		
		case pEnd:
		{
			Fixed	fixed = mEndDelta.ToFixed();
			err = AECreateDesc(typeFixed, &fixed, sizeof(fixed), &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;
		}
		
		case pFirstIndent:
		{
			Fixed	fixed = mBeginFirstDelta.ToFixed();
			err = AECreateDesc(typeFixed, &fixed, sizeof(fixed), &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;
		}
		
/*?
		case pParent:
		{
			LAEStream	stream;
			if (mParent)
				stream.WriteSpecifier(mParent);
			stream.Close(&outPropertyDesc);
		}
*/
		
		default:
			inheritAEOM::GetAEProperty(inProperty, inRequestedType, outPropertyDesc);
			break;
	}
}

void	LRulerSet::SetAEProperty(
	DescType		inProperty,
	const AEDesc	&inValue,
	AEDesc			&outAEReply)
{
	LAESubDesc	value(inValue);
	
	switch (inProperty) {

		case pName:
		{
			Str255	str;
			
			value.ToPString(str);
			LString::CopyPStr(str, mName, sizeof(mName));
			Changed();
			break;
		}
			
		case pVTextDirection:
			mIsLeftToRight = (value.ToEnum() == kVTextRightToLeft)
				? false
				: true;
			Changed();
			break;

		case pVTextJustification:
			mJustification = value.ToEnum();
			Changed();
			break;

		case pBegin:
		{
			Fixed	fixed;
			value.ToPtr(typeFixed, &fixed, sizeof(fixed));
			mBeginDelta = WFixed::Fix2FixedT(fixed);
			Changed();
			break;
		}

		case pEnd:
		{
			Fixed	fixed;
			value.ToPtr(typeFixed, &fixed, sizeof(fixed));
			mEndDelta = WFixed::Fix2FixedT(fixed);
			Changed();
			break;
		}

		case pFirstIndent:
		{
			Fixed	fixed;
			value.ToPtr(typeFixed, &fixed, sizeof(fixed));
			mBeginFirstDelta = WFixed::Fix2FixedT(fixed);
			Changed();
			break;
		}

/*
		case pParent:
		{
			LModelObject	*mo = value.ToModelObjectOptional();
			LRulerSet		*newParent = dynamic_cast<LRulerSet *>(mo);
			
			if (mo != newParent)
				Throw_(paramErr);	//	mo isn't a rulerset
			
			SetParent(newParent);
			break;
		}
*/
		
		default:
			inheritAEOM::SetAEProperty(inProperty, inValue, outAEReply);
			break;
	}
}

const StringPtr	LRulerSet::GetModelNamePtr(void) const
{
	return (const StringPtr) mName;
}

void	LRulerSet::MakeSelfSpecifier(
	AEDesc		&inSuperSpecifier,
	AEDesc		&outSelfSpecifier) const
{
	inheritAEOM::MakeSelfSpecifier(inSuperSpecifier, outSelfSpecifier);
}

Int32	LRulerSet::CountSubModels(
	DescType	inModelID) const
{
	if (inModelID == cRulerTab) {
		return mTabs.size();
	} else {
		return inheritAEOM::CountSubModels(inModelID);
	}
}

void	LRulerSet::GetSubModelByPosition(
	DescType		inModelID,
	Int32			inPosition,
	AEDesc			&outToken) const
{
	if (inModelID == cRulerTab) {
		VTabAEOM	*lazy = NULL;
		
		inPosition--;
		if ((0 <= inPosition) && (inPosition < mTabs.size())) {
			LRulerSet	*my = (LRulerSet *)this;
			vector<TabRecord>::iterator	ip = my->mTabs.begin() + inPosition;
			lazy = new VTabAEOM(my, my, *ip);
		} else {
			Throw_(errAENoSuchObject);
		}
		PutInToken(lazy, outToken);
	} else {
		inheritAEOM::GetSubModelByPosition(inModelID, inPosition, outToken);
	}
}

Int32	LRulerSet::GetPositionOfSubModel(
	DescType			inModelID,
	const LModelObject	*inSubModel) const
{
	if (inModelID == cRulerTab) {
		const VTabAEOM	*tabElem = dynamic_cast<const VTabAEOM *>(inSubModel);
		ThrowIfNULL_(tabElem);
		
		vector<TabRecord>::const_iterator
				ip = mTabs.begin(),
				ie = mTabs.end();
		Int32	rval = 0;

		while (ip != ie) {
			if (&(*ip) == &tabElem->GetTabRecord()) {
				rval = ip - mTabs.begin();
				break;
			}
			ip++;
		}
		
		if (ip == ie)
			Throw_(paramErr);	//	tab didn't exist
		
		return rval + 1;
	} else {
		return inheritAEOM::GetPositionOfSubModel(inModelID, inSubModel);
	}
}

LModelObject*	LRulerSet::HandleCreateElementEvent(
	DescType			inElemClass,
	DescType			inInsertPosition,
	LModelObject*		inTargetObject,
	const AppleEvent&	inAppleEvent,
	AppleEvent&			outAEReply)
{
	if (inElemClass == cRulerTab) {
		VTabAEOM	*rval = NULL;
		TabRecord	newTab;
		LAESubDesc	properties(inAppleEvent),
					prop;
		
		properties = properties.KeyedItem(keyAEPropData);
		
		//	location (req'd -- rest are not)
		prop = properties.KeyedItem(pLocation);
		Fixed	fixed;
		prop.ToPtr(typeFixed, &fixed, sizeof(fixed));
		newTab.location = fixed;
		
		//	orientation
		newTab.orientation = kVTextTabLeading;
		prop = properties.KeyedItem(pVTextTabOrientation);
		if (prop.GetType() != typeNull) {
			newTab.orientation = prop.ToEnum();
		}
		
		//	repeating
		newTab.repeat = FixedT::sZero;
		prop = properties.KeyedItem(pRepeating);
		if (prop.GetType() != typeNull) {
			prop.ToPtr(typeFixed, &fixed, sizeof(fixed));
			newTab.repeat.SetFromFixed(fixed);
		}
		
		//	delimiter
		newTab.delimiter = '.';
		prop = properties.KeyedItem(pDelimiter);
		if (prop.GetType() != typeNull) {
			LAESubDesc	value(prop, typeChar);
			
			Int16	newDelimiter;
			Size	length = value.GetDataLength();
			length = min(length, 2L);
			if (length) {
				value.ToPtr(typeChar, &newDelimiter, length);
				if (length == 1)
					newDelimiter = (newDelimiter >> 8) & 0x0ff;
				newTab.delimiter = newDelimiter;
			}
		}

		Int32	index = NewTabIndex(newTab);
		
		if (index < 0)
			Throw_(paramErr);	//	badly formed tab
		
		if ((index < mTabs.size()) && (mTabs[index].location == newTab.location))
			mTabs.erase(mTabs.begin() + index, mTabs.begin() + index + 1);
		
		mTabs.insert(mTabs.begin() + index, newTab);
		
		return new VTabAEOM(this, this, *(mTabs.begin() + index));
	} else {
		return inheritAEOM::HandleCreateElementEvent(inElemClass, inInsertPosition, inTargetObject, inAppleEvent, outAEReply);
	}
}

