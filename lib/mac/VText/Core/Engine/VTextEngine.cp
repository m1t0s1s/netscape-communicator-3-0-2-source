//	===========================================================================
//	VTextEngine
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VTextEngine.h"
#include	<LHandlerView.h>
#include	<LStyleSet.h>
#include	<LRulerSet.h>
#include	<VPartCache.h>
#include	<VLine.h>
#include	<vector.h>
#include	<UMemoryMgr.h>
#include	<URegions.h>

#pragma	warn_unusedarg off

//	===========================================================================

/*?-
void	Insertion2OffsetLeading(
	const TextRangeT	&inInsertionPt,
	Int32				&outOffset,
	Boolean				&outLeadingEdge)
{
	switch (inInsertionPt.Insertion()) {
	
		case rangeCode_Before:
			outOffset = inInsertionPt.Start() -1;
			outLeadingEdge = false;
			break;
		
		case rangeCode_After:
			outOffset = inInsertionPt.Start();
			outLeadingEdge = true;
			break;
		
		default:
			Throw_(paramErr);
			break;
	}
}

void	OffsetLeading2Insertion(
	Int32				inOffset,
	Boolean				inLeadingEdge,
	TextRangeT			&outInsertionPt)
{
	if (inLeadingEdge) {
		outInsertionPt.SetStart(inOffset);
		outInsertionPt.SetInsertion(rangeCode_After);
	} else {
		outInsertionPt.SetStart(inOffset -1);
		outInsertionPt.SetInsertion(rangeCode_Before);
	}
}
*/
		
VTextEngine::VTextEngine(void)
{
mFlags = recalc_WindowUpdate;

	mStyles = NULL;
	mRulers = NULL;
	mParas = new VParaCache(*this);
	mLines = new VLineCache(*this);
	
	mBlockingFactor = 1024;
	mText = NewHandle(mBlockingFactor);
	ThrowIfNULL_(mText);
	
	for (Int32 i = 0; i < kLineCacheSize; i++) {
		mLineCache[i].lineIndex = -1;
		mLineCache[i].line = new VLine();
	}
}

VTextEngine::~VTextEngine(void)
{
	delete mStyles;
	delete mRulers;
	delete mParas;
	delete mLines;

	if (mText)
		DisposeHandle(mText);
		
	for (Int32 i = 0; i < kLineCacheSize; i++) {
		delete mLineCache[i].line;
		mLineCache[i].line = NULL;
	}
}

void	VTextEngine::GetImageSize(
	SDimension32	*outSize
) const
{
	outSize->width = mImageSize.width;
	outSize->height = mLines->GetTotalPixels().Length() + mMargins.top + mMargins.bottom;
}

void	VTextEngine::GetViewRect(
	SPoint32		*outOrigin,
	SDimension32	*outSize) const
{
	*outOrigin = mViewOrigin;
	*outSize = mViewSize;
}

void	VTextEngine::SetViewRect(
	const SPoint32		&inLocation,
	const SDimension32	&inSize
)
{
	mViewOrigin = inLocation;
	mViewSize = inSize;
}

void	VTextEngine::ScrollView(
	Int32	inLeftDelta,
	Int32	inTopDelta,
	Boolean	inRefresh)
{
	mViewOrigin.h += inLeftDelta;
	mViewOrigin.v += inTopDelta;
}

void	VTextEngine::DrawArea(
	const SPoint32		&inLocation,
	const SDimension32	&inSize
)
{
	Recalc();
	
	if (!mHandlerView || !inSize.height || !inSize.width)
		return;
	
	Rect		r;
	SPoint32	p;
	p.h = inLocation.h;
	p.v = inLocation.v;
	mHandlerView->ImageToLocalPoint(p, topLeft(r));
	p.h += inSize.width;
	p.v += inSize.height;
	mHandlerView->ImageToLocalPoint(p, botRight(r));
	EraseRect(&r);
	
	Range32T	lines,
				linePixels = Range32T(inLocation.v - mMargins.top, inLocation.v + inSize.height - mMargins.top);
	
	if (!mLines->Pixels2Index(linePixels, &lines))
		mLines->Index2Pixels(Range32T(lines.Start(), lines.Start() + 1), &linePixels);
	
	StHandleLocker	lock(mText);
	for (Int32 i = lines.Start(); i < lines.End(); i++) {

		Range32T	lineIndex(i, i+1);
		TextRangeT	lineRange;
		LRulerSet	*ruler;
		mLines->Index2Range(lineIndex, &lineRange);
		mLines->Index2Pixels(lineIndex, &linePixels);
		ruler = GetRulerSet(lineRange);

		SPoint32	lineAnchor;
		Point		pt;
		lineAnchor.v = linePixels.End() + mMargins.top;
		lineAnchor.h = 0 + mMargins.left;
		mHandlerView->ImageToLocalPoint(lineAnchor, pt);
		MoveTo(pt.h, pt.v);

		ruler->DrawLine(*this, *mText, i);
	}
}

//	===========================================================================
//	Text get/set

void	VTextEngine::TextGetThruPtr(const TextRangeT &inRange, Ptr outBuffer)
{
	TextRangeT	range = inRange;
	range.Crop(GetTotalRange());
	
	BlockMoveData(*mText + range.Start(), outBuffer, range.Length());
}

void	VTextEngine::TextReplaceByPtr(
	const TextRangeT	&inRange,
	const Ptr			inSrcText,
	Int32				inSrcLength)
{
	TextRangeT	range = inRange;
	
	range.Crop(GetTotalRange());
	
	Int32	sizeDelta = inSrcLength - range.Length(),
			sizeNeeded = GetTotalRange().Length() + sizeDelta;
	
//-.	StRecalculator	change(this);

	SetTextChanged();
	
	Int32	tailSize = GetTotalRange().End() - range.End(),
			tailHead = range.End();

	if (sizeDelta > 0)
		Reserve(sizeNeeded);

	TextFluxPre(range);
	
	if (tailSize > 0)
		BlockMoveData(*mText + tailHead, *mText + tailHead + sizeDelta, tailSize);
	
	if (inSrcLength > 0)
		BlockMoveData(inSrcText, *mText + range.Start(), inSrcLength);
	
	if (sizeDelta < 0)
		Reserve(sizeNeeded);
		
	TextFluxPost(inSrcLength - range.Length());
}

void	VTextEngine::Reserve(Int32 inNeeded)
/*
	Reserve text space for at inNeeded bytes.
	
	This routine will grow and shrink the text handle to mBlockingFactor
	chunk sizes.
	
	This routine will throw if memory errors occur.
*/
{
	Int32	sizeAllocated = GetHandleSize(mText);
	
	if (
		(inNeeded > sizeAllocated) ||
		(inNeeded < (sizeAllocated - mBlockingFactor))
	) {

		//	change handle size
		Int32	numerator = inNeeded / mBlockingFactor,
				remainder = inNeeded % mBlockingFactor;
				
		sizeAllocated = numerator * mBlockingFactor;
		if (remainder)
			sizeAllocated += mBlockingFactor;
		SetHandleSize(mText, sizeAllocated);
		ThrowIf_(MemError());
		
	}
}
//	===========================================================================
//	Support

const Handle	VTextEngine::GetTextHandle(void) const
{
	return mText;
}

void	VTextEngine::SetTextHandle(Handle inHandle)
{
//-.	StRecalculator	change(this);

	SetTextChanged();
	
	TextFluxPre(GetTotalRange());
	
	mText = inHandle;
		
	mTotalRange = Range32T(0, GetHandleSize(mText));
	
	TextFluxPost(GetTotalRange().Length() - mPreFluxRange.Length());
}

//	===========================================================================
//	Presentation query

Boolean	VTextEngine::RangeIsMultiLine(const TextRangeT &inRange) const
{
	Range32T	indexRange;
	TextRangeT	range = inRange;
	
	range.Crop(GetTotalRange());	
	mLines->Range2Index(range, &indexRange);
	
	return indexRange.Length() > 1;
}

void	VTextEngine::GetRangeRgn(const TextRangeT &inRange, RgnHandle *ioRgn) const
{
	ThrowIfNULL_(ioRgn);
	ThrowIfNULL_(*ioRgn);

	SetEmptyRgn(*ioRgn);
	ThrowIfOSErr_(QDError());
	
	do {
		if (!mHandlerView)
			break;

		VTextEngine		&my = *((VTextEngine *)this);
		my.Recalc();

		TextRangeT		range = inRange;
		range.WeakCrop(GetTotalRange());
		
		Range32T		lines;
		TextRangeT		lineRange = range;
		if (!mLines->Range2Index(range, &lines))
			mLines->Index2Range(lines, &lineRange);
		VLine			*line = NULL;
			
		Range32T		pixelHeight;
		mLines->Index2Pixels(lines, &pixelHeight);
		
		SPoint32		viewOrigin;
		SDimension32	viewSize;
		GetViewRect(&viewOrigin, &viewSize);
		Domain32T		viewDomain(
							Range32T(viewOrigin.h, viewOrigin.h + viewSize.width),
							Range32T(viewOrigin.v, viewOrigin.v + viewSize.height)
						);
		
		//	exclude margin areas
		viewDomain.Crop(Domain32T(
							Range32T(0 + mMargins.left -1, mImageSize.width - mMargins.right),
							Range32T(0 + mMargins.top, mImageSize.height - mMargins.bottom)
		));
		
		pixelHeight.Crop(viewDomain.Height());
		
		if (!pixelHeight.Length())
			break;
		
		StHandleLocker	lock(mText);
		SPoint32		coordTranslator;
		{
			Point	local;
			coordTranslator.h = viewDomain.Width().Start();
			coordTranslator.v = viewDomain.Height().Start();
			ThrowIfNULL_(mHandlerView);
			mHandlerView->ImageToLocalPoint(coordTranslator, local);
			coordTranslator.h = local.h - coordTranslator.h;
			coordTranslator.v = local.v - coordTranslator.v;
		}

		//	¥	first or only line
		{
			Range32T	lineIndex(lines.Start(), lines.Start() +1),
						linePixels;
			
			mLines->Index2Pixels(lineIndex, &linePixels);
			linePixels.SetStart(linePixels.Start() + mMargins.top);
			linePixels.Crop(viewDomain.Height());
			
			if (linePixels.Length()) {
				line = GetLine(lines.Start());
				line->AppendRangeRgn(*mText, range, coordTranslator,
						Domain32T(
							viewDomain.Width(),
							linePixels
						),
						*ioRgn);
			}
		}
		
		//	¥	area between two lines
		if (lines.Length() > 2) {
			Range32T	lineIndex(lines.Start() + 1, lines.End() -1),
						linePixels;
			
			mLines->Index2Pixels(lineIndex, &linePixels);
			linePixels.SetStart(linePixels.Start() + mMargins.top);
			linePixels.Crop(viewDomain.Height());
			
			if (linePixels.Length()) {
				StRegion	rgn;
				Domain32T	domain(viewDomain.Width(), linePixels);
				Rect		r;
				
				DomainToLocalRect(domain, &r);
				RectRgn(rgn, &r);
				ThrowIfOSErr_(QDError());
				UnionRgn(*ioRgn, rgn, *ioRgn);
				ThrowIfOSErr_(QDError());
			}
		}
		
		//	¥	last line
		if (lines.Length() > 1) {
			Range32T	lineIndex(lines.End() -1, lines.End()),
						linePixels;
			
			mLines->Index2Pixels(lineIndex, &linePixels);
			linePixels.SetStart(linePixels.Start() + mMargins.top);
			linePixels.Crop(viewDomain.Height());
			
			if (linePixels.Length()) {
				line = GetLine(lineIndex.Start());
				line->AppendRangeRgn(*mText, range, coordTranslator,
						Domain32T(
							viewDomain.Width(),
							linePixels
						),
						*ioRgn);
			}
		}
	} while (false);
}

Int32	VTextEngine::GetRangeTop(const TextRangeT &inRange) const
{
	Assert_(!mFlags);

	TextRangeT	range = inRange;
	Range32T	lines,
				pixels;
	
	range.Crop(GetTotalRange());
	mLines->Range2Index(range, &lines);
	mLines->Index2Pixels(lines, &pixels);
	
	return pixels.Start() + mMargins.top;
}

Int32	VTextEngine::GetRangeHeight(const TextRangeT &inRange) const
{
	Assert_(!mFlags);

	TextRangeT	range = inRange;
	Range32T	lines,
				pixels;
	
	range.Crop(GetTotalRange());
	mLines->Range2Index(range, &lines);
	mLines->Index2Pixels(lines, &pixels);
	
	return pixels.Length();
}

Int32	VTextEngine::GetRangeWidth(const TextRangeT &inRange) const
{
	Assert_(!mFlags);

	FixedT	width;
	
	TextRangeT	range = inRange;
	Range32T	lines;
	
	range.Crop(GetTotalRange());
	mLines->Range2Index(range, &lines);
	
	if (lines.Length() > 1) {
		SDimension32	size;
		GetImageSize(&size);
		width = size.width;
	} else {
		Assert_(lines.Length() == 1);
		
		StHandleLocker	lock(mText);
		VLine			*line = GetLine(lines.Start());
		TextRangeT		testRange = range;
		
		testRange.Back();
		width = line->Insertion2Pixel(*mText, testRange);
		range.Front();
		width -= line->Insertion2Pixel(*mText, range);
	}
	
	return width.ToLong();
}

void	VTextEngine::Range2Image(
	const TextRangeT	&inRange,
	Boolean				inLeadingEdge,
	SPoint32			*outWhere) const
{
	Assert_(!mFlags);

	TextRangeT	range = inRange;
	range.WeakCrop(GetTotalRange());
	
	if (range.Length()) {
		if (inLeadingEdge)
			range.Front();
		else
			range.Back();
	}
	
	Range32T	lines,
				pixels;
	
	mLines->Range2Index(range, &lines);
	mLines->Index2Pixels(lines, &pixels);
	
	StHandleLocker	lock(mText);
	VLine			*line = GetLine(lines.Start());
	
	outWhere->v = pixels.End() + mMargins.top;
	outWhere->h = line->Insertion2Pixel(*mText, range).ToLong();	//- + mMargins.left;
}

void	VTextEngine::Image2Range(
	SPoint32			inWhere,
	Boolean				*outLeadingEdge,
	TextRangeT			*outRange) const
{
	Assert_(!mFlags);

	Range32T	pixels(inWhere.v - mMargins.top),
				lineIndex;
	
	pixels.SetInsertion(rangeCode_Before);
	
	if (pixels.IsBefore(mLines->GetTotalPixels())) {
		*outRange = TextRangeT(0, 0, rangeCode_After);
		*outLeadingEdge = true;
	} else if (pixels.IsAfter(mLines->GetTotalPixels())) {
		*outRange = GetTotalRange();
		outRange->After();
		*outLeadingEdge = false;
	} else {
		mLines->Pixels2Index(pixels, &lineIndex);
		
		StHandleLocker	lock(mText);
		VLine			*line = GetLine(lineIndex.Start());

		line->Pixel2Range(*mText, WFixed::Long2FixedT(inWhere.h/*- - mMargins.left*/), *outRange, *outLeadingEdge);
	}
}

//	===========================================================================
//	Implementation help:

void	VTextEngine::RecalcSelf(void)
{
	if (mFlags & recalc_AllWidth) {
		mUpdateRange = GetTotalRange();
		BroadcastMessage(msg_ModelWidthChanged);
		mFlags &= ~recalc_AllWidth;
	}
	
	if (mFlags & recalc_Range) {
		if (!mUpdateRange.Undefined()) {
			Int32		lengthDelta = GetTotalRange().Length() - mUpdatePreLength;
			TextRangeT	preRange(mUpdateRange.Start(), mUpdateRange.End() - lengthDelta);

			ThrowIfNULL_(mLines);
			
			mUpdateRangeReformatted = sTextUndefined;
			
			mLines->TextChanged(preRange, mUpdateRange, &mUpdateRangeReformatted);
			
			mUpdateRange = sTextUndefined;
			mUpdatePreLength = GetTotalRange().Length();
		}
		mFlags &= ~recalc_Range;
	}
	
	inherited::RecalcSelf();
}

void	VTextEngine::TextFluxPostSelf(
	const TextRangeT	&inRange,
	Int32				inLengthDelta)
{
	TextRangeT	postRange = inRange;
	postRange.Adjust(postRange, inLengthDelta, true);
	
#if	0
//+
	if (mLines)
		mLines->TextFluxed(inRange, postRange);
#endif

	if (mStyles)
		mStyles->TextFluxed(inRange, postRange);

	if (mRulers)
		mRulers->TextFluxed(inRange, postRange);

	if (mParas)
		mParas->TextFluxed(inRange, postRange);
}

void	VTextEngine::NestedUpdateOut(void)
{
	Assert_(mUpdateLevel > 0);
	
	if (mUpdateLevel != 1)
		return;
	
	TextRangeT	changedRange = mUpdateRange,
				formattedRange;
	
	if (!mUpdateRange.Undefined())
		BroadcastMessage(msg_CumulativeRangeChanged, &changedRange);
	
	//	reformat the text
	Recalc();	
	
	//	broadcast partial image update
	if (!mUpdateRangeReformatted.Undefined() && !(mFlags & recalc_WindowUpdate)) {
		TextUpdateRecordT	updateRec;
		SDimension32		imageSize;
		
		GetImageSize(&imageSize);
		
		updateRec.gap = Range32T(GetRangeTop(mUpdateRangeReformatted));
		if (mUpdateRangeReformatted.End() == GetTotalRange().End())
			updateRec.gap.SetLength(imageSize.height - updateRec.gap.Start());
		else
			updateRec.gap.SetLength(GetRangeHeight(mUpdateRangeReformatted));
		Assert_(updateRec.gap.Length() > 0);
	
		updateRec.scrollAmount = imageSize.height - mUpdatePreHeight;
		updateRec.scrollStart = updateRec.gap.End();
		if (updateRec.scrollAmount > 0)
			updateRec.scrollStart -= updateRec.scrollAmount;
		
		BroadcastMessage(msg_UpdateImage, &updateRec);
	}
	
	BroadcastMessage(msg_ModelChanged);
}

Boolean	VTextEngine::SpaceForBytes(Int32 inAdditionalBytes) const
{
	return true;
}

void	VTextEngine::Refresh(RecalcFlagsT inFlags)
{
	InvalidateLineCache();
	inherited::Refresh(inFlags);
}

void	VTextEngine::Refresh(const TextRangeT &inRange)
{
	InvalidateLineCache();
	inherited::Refresh(inRange);
}

void	VTextEngine::InvalidateLineCache(void)
{
	for (Int32 i = 0; i < kLineCacheSize; i++)
		mLineCache[i].lineIndex = -1;
}

void	VTextEngine::HitLine(Int32 inLineIndex) const
/*
	If inLineIndex is in the cache,
		that cache entry becomes the first entry
	else
		if one exists,
			the newest invalid entry is moved to the first entry
		else
			the oldest entry is moved to the first entry
			(and 
		the oldest cache entry is moved the first entry
*/
{
	Int32		i;	//	entry to move to [0].
	Boolean		found = false;
	VTextEngine	&my = *((VTextEngine *)this);
	
	for (i = 0; i < kLineCacheSize; i++) {
		if (mLineCache[i].lineIndex == inLineIndex) {
			found = true;
			break;
		}
	}
	
	if (!found) {
		for (i = 0; i < kLineCacheSize; i++) {
			if (mLineCache[i].lineIndex < 0) {
				found = true;
				break;
			}
		}
	}
	
	if (!found)
		i = kLineCacheSize -1;
	
	if (i) {
		//	move entries
		LineCacheT	temp = mLineCache[i];
		
		for (Int32	j = i; j > 0; j--)
			my.mLineCache[j] = mLineCache[j-1];
		
		my.mLineCache[0] = temp;
	}
}

VLine *	VTextEngine::GetLine(Int32 inLineIndex) const
{
	VTextEngine	&my = *((VTextEngine *)this);
	
	Assert_(inLineIndex >= 0);
	
	HitLine(inLineIndex);
	
	if (mLineCache[0].lineIndex != inLineIndex) {
		my.mLineCache[0].lineIndex = -1;
		
		TextRangeT	lineRange,
					paraRange;
		Range32T	lineIndex(inLineIndex, inLineIndex+1),
					paraIndex;
		
		mLines->Index2Range(lineIndex, &lineRange);
		paraRange = lineRange;
		if (!mParas->Range2Index(paraRange, &paraIndex))
			mParas->Index2Range(paraIndex, &paraRange);
		
		LRulerSet		*ruler = GetRulerSet(paraRange);
		TextRangeT		range = lineRange;
		StHandleLocker	lock(mText);
		
		range.Front();		
		ruler->LayoutLine(*this, *mText, paraRange, range, *((my.mLineCache[0]).line));
		
		Assert_(mLineCache[0].line->GetRange() == lineRange);
			
		my.mLineCache[0].lineIndex = inLineIndex;
	}
	
	//	fatal programmer errors
	ThrowIfNULL_(mLineCache[0].line);
	ThrowIf_(inLineIndex != mLineCache[0].lineIndex);
	ThrowIf_(!GetTotalRange().WeakContains(mLineCache[0].line->GetRange()));
	
	return mLineCache[0].line;
}

VLine *	VTextEngine::LayoutLine(
	Int32				inLineIndex,	//	(for caching purposes)
	const LRulerSet		&inRuler,		//	ruler for paragraph
	const TextRangeT	&inParaRange,	//	enclosing paragraph
	const TextRangeT	&inLineStartPt,	//	insertionPt of line start,
	TextRangeT			&outLineRange	//	range of resulting line
) const
{
	HitLine(inLineIndex);

	VTextEngine	&my = *((VTextEngine *)this);

	my.mLineCache[0].lineIndex = -1;
	inRuler.LayoutLine(*this, *mText, inParaRange, inLineStartPt, *(mLineCache[0].line));
	my.mLineCache[0].lineIndex = inLineIndex;

	outLineRange = mLineCache[0].line->GetRange();
	return mLineCache[0].line;
}

ScriptCode	VTextEngine::GetScriptRange(
	const TextRangeT	&inInsertionPt,
	TextRangeT			&outRange) const
{
	Assert_(!inInsertionPt.Length());
	
	ScriptCode	rval;
	
	if (mStyles && mStyles->GetRecordCount()) {
		Range32T		index;
		TextRangeT		range = inInsertionPt,
						scriptRange,
						styleRun;
		ScriptCode		script;
		const StyleRunT	*run;
		Int32			i;
					
		range.Crop(GetTotalRange());
		
		mStyles->Range2Index(range, &index);
		run = mStyles->GetRecord(index.Start());
		script = run->style->GetScript();
		mStyles->Index2Range(Range32T(index.Start(), index.Start() +1), &scriptRange);
		
		//	scan backwards
		for (i = index.Start() -1; i >= 0; i--) {
			run = mStyles->GetRecord(i);
			if (run->style->GetScript() != script)
				break;
			scriptRange.MoveStart(scriptRange.Start() - run->length);
		}
			
		//	scan forwards
		for (i = index.Start() +1; scriptRange.End() < GetTotalRange().End(); i++) {
			run = mStyles->GetRecord(i);
			if (run->style->GetScript() != script)
				break;
			scriptRange.SetEnd(scriptRange.End() + run->length);
		}
		
		outRange = scriptRange;
		rval = script;
	} else {
		outRange = GetTotalRange();
		rval = GetDefaultStyleSet()->GetScript();
	}
	
	return rval;
}

