//	===========================================================================
//	LTextEngine.cp				©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LTextEngine.h"
#include	<LListIterator.h>
#include	<UMemoryMgr.h>
#include	<LHandlerView.h>
#include	<LEventHandler.h>
#include	<LStyleSet.h>
#include	<LRulerSet.h>
#include	<LStream.h>
#include	<URegions.h>
#include	<UFontsStyles.h>

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#ifndef		__ERRORS__
#include	<Errors.h>
#endif

#ifndef		__SCRAP__
#include	<Scrap.h>
#endif

#ifndef		__SCRIPT__
#include	<Script.h>
#endif

#pragma	warn_unusedarg off

//	===========================================================================

const	TextRangeT	LTextEngine::sTextAll(0, max_Int32);
const	TextRangeT	LTextEngine::sTextStart(0, 0, rangeCode_After);
const	TextRangeT	LTextEngine::sTextEnd(max_Int32);
const	TextRangeT	LTextEngine::sTextUndefined;

LTextEngine::LTextEngine(void)
{
	mHandlerView = NULL;
	
	mAttributes = textEngineAttr_Editable | textEngineAttr_Selectable;
	if (UFontsStyles::MultiByteScriptPresent())
		mAttributes |= textEngineAttr_MultiByte;

	mMargins.top = 4;
	mMargins.left = 4;
	mMargins.right = 4;
	mMargins.bottom = 4;
	
	mDefaultStyleSet = NULL;
	mDefaultRulerSet = NULL;
	
	mImageSize.width = 1000;
	mImageSize.height = 0;
	mImageWidth = mImageSize.width;
	
	mUpdateRange = sTextUndefined;
	mUpdatePreLength = 0;
	mTotalRange = sTextStart;
	
	mFlags = 0;
	mUpdateLevel = 0;
}

LTextEngine::~LTextEngine(void)
{
	ReplaceSharable(mDefaultStyleSet, (LStyleSet *)NULL, this);
	ReplaceSharable(mDefaultRulerSet, (LRulerSet *)NULL, this);
}

#ifndef	PP_No_ObjectStreaming

void	LTextEngine::WriteObject(LStream &inStream) const
{
	inStream << mAttributes;
	inStream << mMargins;
	inStream << mImageSize.width;
	inStream << mImageSize.height;
	
//	inherited::WriteObject(inStream);	//	none!
	
	inStream << mDefaultStyleSet;
}

void	LTextEngine::ReadObject(LStream &inStream)
{
	inStream >> mAttributes;
	inStream >> mMargins;
	inStream >> mImageSize.width;
	inStream >> mImageSize.height;
		
	mImageWidth = mImageSize.width;
	
//	inherited::ReadObject(inStream);	//	none!
	
	SetDefaultStyleSet(ReadStreamable<LStyleSet *>()(inStream));
	SetDefaultRulerSet(ReadStreamable<LRulerSet *>()(inStream));
	
	if (UFontsStyles::MultiByteScriptPresent())
		mAttributes |= textEngineAttr_MultiByte;
	else
		mAttributes &= ~textEngineAttr_MultiByte;
}

#endif

//	===========================================================================

void	LTextEngine::SetAttributes(Uint16 inAttributes)
{
	if (mAttributes != inAttributes) {
		mAttributes = inAttributes;
		
		mAttributes &= ~textEngineAttr_MultiByte;
		if (UFontsStyles::MultiByteScriptPresent())
			mAttributes |= textEngineAttr_MultiByte;
		
		Refresh(GetTotalRange());
	}
}

Uint16	LTextEngine::GetAttributes(void) const
{
	return mAttributes;
}

void	LTextEngine::SetTextMargins(const Rect &inMargins)
{
	if (!EqualRect(&mMargins, &inMargins)) {
		mMargins = inMargins;
		Refresh(recalc_AllWidth);
	}
}

void	LTextEngine::GetTextMargins(Rect *outMargins) const
{
	*outMargins = mMargins;
}

LHandlerView *	LTextEngine::GetHandlerView(void)
{
	return mHandlerView;
}

void	LTextEngine::SetHandlerView(LHandlerView *inHandlerView)
{
	if (inHandlerView == mHandlerView)
		return;
	
	mHandlerView = inHandlerView;
}

GrafPtr	LTextEngine::GetPort(void)
{
//-	Assert_(false);	//	must override
	return NULL;
}

void	LTextEngine::SetPort(GrafPtr inPort)
{
//-	Assert_(false);	//	must override
}

void	LTextEngine::GetImageSize(
	SDimension32	*outSize
) const
{
	Assert_(false);	//	Must override
}

#if	0
//-
void	LTextEngine::SetImageSize(
	const SDimension32	&inSize
)
/*
	SetImageSize will NOT:

		change the view rect 
		
			or
		
		broadcast msg_ViewParmsChanged
	
	But may make an update event that will
	
		broadcast msg_ModelScopeChanged
*/
{
	Assert_(false);
}
#endif

void	LTextEngine::SetImageWidth(
	Int32	inWidth
)
{
	if (mImageWidth != inWidth) {
		mImageWidth = inWidth;
		mImageSize.width = mImageWidth;
		Refresh(recalc_AllWidth);
	}
}

void	LTextEngine::GetViewRect(
	SPoint32		*outOrigin,
	SDimension32	*outSize) const
{
	Assert_(false);	//	Must override
}

void	LTextEngine::SetViewRect(
	const SPoint32		&inLocation,
	const SDimension32	&inSize
)
/*
	SetViewRect will not:

		change the image size 
		
			or
		
		broadcast msg_ViewParmsChanged, or msg_ModelScopeChanged
*/
{
	Assert_(false);	//	Must override
}

void	LTextEngine::ScrollView(
	Int32	inLeftDelta,
	Int32	inTopDelta,
	Boolean	inRefresh)
{
	Assert_(false);	//	Must override
}

void	LTextEngine::ScrollToRange(const TextRangeT	&inRange)
{
	SPoint32	scrollVector;
	
	if (GetRangeScrollVector(inRange, &scrollVector)) {
		mHandlerView->ScrollPinnedImageBy(scrollVector.h, scrollVector.v, true);
	}
}

Boolean	LTextEngine::GetRangeScrollVector(
	const TextRangeT	&inRange,
	SPoint32			*outVector
) const
/*
	Returns a scrolling vector to bring the selection into view.  Vector is valid even if false
	is returned.
	
	Returns true iff the "critical" point of the current selection is outside the
	current view rectangle.
*/
{
	Boolean				rval = false;
	TextRangeT			range = inRange;
	
	//	Find "where ranges" (frame coords to put critical pt inside of).
	Range32T		hWhere,
					vWhere;
	SPoint32		origin;
	SDimension32	size,
					imageSize;
	GetViewRect(&origin, &size);
	GetImageSize(&imageSize);
	hWhere.SetStart((mScrollPrefs.hWhere.Start() * size.width) >> 8);
	hWhere.SetEnd((mScrollPrefs.hWhere.End() * size.width) >> 8);
	vWhere.SetStart((mScrollPrefs.vWhere.Start() * size.height) >> 8);
	vWhere.SetEnd((mScrollPrefs.vWhere.End() * size.height) >> 8);
	
	//	Find critical points (image coords that should be in view)
	SPoint32		criticalPt1,	//	Bottom of critical "caret"
					criticalPt2;	//	Top of critical "caret"
	if (mScrollPrefs.doEnd) {
		range.Back();
		Range2Image(range, true, &criticalPt1);
	} else {
		range.Front();
		Range2Image(range, false, &criticalPt1);
	}
	criticalPt2 = criticalPt1;
	criticalPt2.v = GetRangeTop(range);
		
	//	Find desired vertical scroll position
	SPoint32		scrollPosition = origin;	//	image coord of top left of frame
	if (criticalPt2.v - origin.v < vWhere.Start()) {
		scrollPosition.v = criticalPt2.v - vWhere.Start();
	} else if (criticalPt1.v - origin.v > vWhere.End()) {
		scrollPosition.v = criticalPt1.v - vWhere.End();
	}
	scrollPosition.v = URange32::Min(scrollPosition.v, imageSize.height - size.height);	//	Dont scroll past max boundary
	scrollPosition.v = URange32::Max(scrollPosition.v, 0);								//	Dont scroll past min boundary
	
	//	Find horizontal scroll position
	if (criticalPt1.h - origin.h < hWhere.Start()) {
		scrollPosition.h = criticalPt1.h - hWhere.Start();
	} else if (criticalPt1.h - origin.h > hWhere.End()) {
		scrollPosition.h = criticalPt1.h - hWhere.End() + 1;	//	+1 is for caret width of 1 pixel
	}
	if (criticalPt1.h < size.width)
		scrollPosition.h = 0;	//	Hug left edge
	scrollPosition.h = URange32::Min(scrollPosition.h, imageSize.width - size.width);	//	Dont scroll past max boundary
	scrollPosition.h = URange32::Max(scrollPosition.h, 0);								//	Dont scroll past min boundary

	//	Determine if scrolling should occur
	if (mScrollPrefs.doAlways) {

		//	if criticalPt isn't visible, scroll.
		Boolean	doH = (criticalPt1.h < origin.h) || (criticalPt1.h >= (origin.h + size.width)),
				doV = (criticalPt2.v < origin.v) || (criticalPt1.v >= (origin.v + size.height));
		
		rval = doH || doV;
		
		//	Only scroll each axis as necessary
		if (!doH)
			scrollPosition.h = origin.h;
		if (!doV)
			scrollPosition.v = origin.v;
			
	} else {

		//	if any part of range is visible, don't scroll!
		StRegion	rangeRgn;
		Rect		r;
		r.top = origin.v;
		r.left = origin.h;
		r.right = r.left + size.width;
		r.bottom = r.top + size.height;
		GetRangeRgn(inRange, &rangeRgn.mRgn);
		if (EmptyRgn(rangeRgn) || !RectInRgn(&r, rangeRgn)) {
			rval = true;
		} else {
			rval = false;
		}		
	}

	//	Return
	if (outVector) {
		//	These values are valid even if false is returned
		outVector->v = scrollPosition.v - origin.v;
		outVector->h = scrollPosition.h - origin.h;
	}
	return rval;
}

SScrollPrefs::SScrollPrefs()
{
	hWhere = Range32T(0x0040, 0x00c0);	//	quarters
	vWhere = Range32T(0x0000, 0x0100);

//	hWhere = Range32T(0x0010, 0x00f0);	//	for testing
//	hWhere = Range32T(0x0080, 0x0080);	//	for testing
//	hWhere = Range32T(0x0000, 0x0100);	//	for testing

//	vWhere = hWhere;					//	for testing

	doEnd = true;
	doAlways = false;
}

const SScrollPrefs &	LTextEngine::ScrollPreferences(void) const
{
	return mScrollPrefs;
}

SScrollPrefs &	LTextEngine::ScrollPreferences(void)
{
	return mScrollPrefs;
}

void	LTextEngine::DrawArea(
	const SPoint32		&inLocation,
	const SDimension32	&inSize
)
{
	Assert_(mHandlerView);
	
	Recalc();
}

Boolean	LTextEngine::GetTextChanged(void) 
{
	return mIsChanged;
}

void	LTextEngine::SetTextChanged(Boolean inChanged)
{
	if (inChanged && ((mAttributes & textEngineAttr_Editable) == 0))
		Throw_(wrPermErr);	//	should read can't edit this file -- search string: paramErr

	mIsChanged = inChanged;
}

const TextRangeT &	LTextEngine::GetTotalRange(void) const
/*
	Override to load mTotalRange with the implementation engine value.
*/
{
	return mTotalRange;
}

//	===========================================================================
//	Text get/set

void	LTextEngine::TextMove(const TextRangeT &inRangeA, const TextRangeT &inRangeB)
/*
	Range A is moved to range B.
*/
{
	TextRangeT	srcRange = inRangeA,
				dstRange = inRangeB;
	
	srcRange.Crop(GetTotalRange());
	dstRange.Crop(GetTotalRange());
	
	if (srcRange.Intersects(dstRange))
		ThrowOSErr_(paramErr);
	
	if (!SpaceForBytes(srcRange.Length() - dstRange.Length()))
		ThrowOSErr_(teScrapSizeErr);

	StHandleBlock	temp(srcRange.Length(), true, true);
	TextGetThruHandle(srcRange, temp);

	TextDelete(srcRange);
	
	if (srcRange.Start() < dstRange.Start())
		dstRange.Translate(-srcRange.Length());
	
	TextReplaceByHandle(dstRange, temp);
}

void	LTextEngine::TextDelete(const TextRangeT &inRange)
{
	TextReplaceByPtr(inRange, NULL,  0);
}

void	LTextEngine::TextGetThruPtr(const TextRangeT &inRange, Ptr outBuffer)
/*
	Notes:
	
		This method is MAYBE only suitable for plain text.  Consider using
		one of the tube methods above unless really needing plain text
		from a non tube source.
		
		TextGetThruPtr or TextGetThruHandle must be overridden or infinite
		recursion will occur.
*/
{
	TextRangeT	range = inRange;
	range.Crop(GetTotalRange());
	
	StHandleBlock	text(range.Length(), true, true);
	ThrowIfNULL_(outBuffer);
	
	TextGetThruHandle(range, text.mHandle);
	BlockMoveData(*(text.mHandle), outBuffer, range.Length());
}

void	LTextEngine::TextGetThruHandle(const TextRangeT &inRange, Handle outTextH)
/*
	Notes:
	
		This method is MAYBE only suitable for plain text.  Consider using
		one of the tube methods above unless really needing plain text
		from a non tube source.
		
		TextGetThruPtr or TextGetThruHandle must be overridden or infinite
		recursion will occur.
*/
{
	ThrowIfNULL_(outTextH);
	
	TextRangeT	range = inRange;
	
	range.Crop(GetTotalRange());
	
	if (GetHandleSize(outTextH) != inRange.Length()) {
		SetHandleSize(outTextH, inRange.Length());
		ThrowIfMemError_();
	}
	
	StHandleLocker	p(outTextH);
	
	TextGetThruPtr(range, *outTextH);
}

void	LTextEngine::TextReplaceByPtr(
	const TextRangeT	&inRange,
	const Ptr			inSrcText,
	Int32				inSrcLength)
/*
	Note:
	
		This method is MAYBE only suitable for plain text.  Consider using
		one of the tube methods above unless really needing plain text
		from a non tube source.
*/
{
/*	Must be overridden w/ something like...

	TextRangeT			range = inRange;
	
	range.Crop(GetTotalRange());
	
	SetTextChanged();
	
	TextFluxPre(range);
	
	//	change your data...
	
	TextFluxPost(inSrcLength - range.Length());
*/
}

void	LTextEngine::TextReplaceByHandle(
	const TextRangeT	&inRange,
	const Handle		inNewTextH)
/*
	Note:
	
		This method is MAYBE only suitable for plain text.  Consider using
		one of the tube methods above unless really needing plain text
		from a non tube source.
*/
{
	StHandleLocker	p(inNewTextH);
	Int32			length = GetHandleSize(inNewTextH);
	
	TextReplaceByPtr(inRange, *inNewTextH, length);
}

//	===========================================================================
//	Support

const Handle	LTextEngine::GetTextHandle(void) const
/*
	You can NOT GetTextHandle, change the data in the handle, and then hope
	that a stylized text engine will correctly update the
	styles for the text.
	
	You should only use this routine for such things as writing
	raw ascii to disk.
	
	The handle returned may be larger than the text contained by the text
	engine -- get the actual text length with GetTotalRange().Length() not
	GetHandleSize!
*/
{
	return NULL;
}

void	LTextEngine::SetTextHandle(Handle inHandle)
/*
	This routines causes the text engine to use the
	text inHandle.  Thereafter, the TextEngine owns the handle.
	
	The initial style of the text in the Handle will be
	the default styleset for the text engine.
	
	The handle can not contain any padding (in contrast to GetTextHandle
	whose handle may contain padding).
*/
{
	Assert_(false);
}

//	This member is protected -- you can't use it from client code
const void * LTextEngine::GetTextPtr(Int32 inOffset) const
{
	char	**q,
			*p;

	q = GetTextHandle();
	ThrowIfNULL_(q);
	p = *q;
	ThrowIfNULL_(p);
	
	return p + inOffset;
}
 
//	===========================================================================

void	LTextEngine::DomainToLocalRect(const Domain32T &inDomain, Rect *outRect) const
{
	SPoint32	p32;
	
	ThrowIfNULL_(mHandlerView);
	
	p32.h = inDomain.Width().Start();
	p32.v = inDomain.Height().Start();
	mHandlerView->ImageToLocalPoint(p32, topLeft(*outRect));
	p32.h = inDomain.Width().End();
	p32.v = inDomain.Height().End();
	mHandlerView->ImageToLocalPoint(p32, botRight(*outRect));
}

void	LTextEngine::GetRangeRgn(const TextRangeT &inRange, RgnHandle *ioRgn) const
/*
	Returns the region of inRange.
	
	IoRgn is clipped to the current handler view (GetHandlerView()) and the current
	handler view defines the 16 bit coordinate system used for ioRgn.
	
	If there is no current handler view or inRange lies outside the current handler
	view, ioRgn is set to an empty rgn.
*/
{
	mHandlerView->FocusDraw();

	Boolean		exists = false;
	TextRangeT	range = inRange;
	
	ThrowIfNULL_(ioRgn);
	ThrowIfNULL_(*ioRgn);

	range.Crop(GetTotalRange());
	do {
		if (!mHandlerView)
			break;
		
		((LTextEngine *)this)->Recalc();
		
		SPoint32		origin;
		SDimension32	size;
		GetViewRect(&origin, &size);
		Domain32T	vRect(
						Range32T(origin.h, origin.h + size.width),
						Range32T(origin.v, origin.v + size.height)
					);	//	view rect in image coords
		Range32T	rHeight(GetRangeTop(range));	//	top and bottom of range
		rHeight.SetLength(GetRangeHeight(range));
		rHeight.Crop(vRect.Height());

		if (!rHeight.Length())
			break;

		Rect		r;		//	scratch rect		
		
		if (range.Length()) {
		
			SPoint32	p1,	//	anchor points of range start and end
						p2;

			Range2Image(range, true, &p1);
			Range2Image(range, false, &p2);
			p1.h--;									//	include one pixel to left of char
			
			if (p1.v == p2.v) {
				
				//	one line
				if (p1.h >= p2.h)
					Throw_(paramErr);
				
				Domain32T	l1(Range32T(p1.h, p2.h), rHeight);
				
				l1.Crop(vRect);
				DomainToLocalRect(l1, &r);
				
				RectRgn(*ioRgn, &r);
				ThrowIfOSErr_(QDError());
				
			} else {
				
				//	two lines
				TextRangeT	front = range,
							back = range;
				
				front.Front();
				back.Back();
				
				Int32		h1 = GetRangeHeight(front), //	heights of first and last line of range
							h2 = GetRangeHeight(back),
							extremeLeft = 0 + mMargins.left,
							extremeRight = mImageSize.width - mMargins.right;
	
				extremeLeft--;							//	include one pixel to left of char
				
				Domain32T	l1(Range32T(p1.h, extremeRight), Range32T(p1.v - h1, p1.v)),
							l2(Range32T(extremeLeft, p2.h), Range32T(p2.v - h2, p2.v));
				
				l1.Crop(vRect);
				DomainToLocalRect(l1, &r);
				
				RectRgn(*ioRgn, &r);
				ThrowIfOSErr_(QDError());
				
				l2.Crop(vRect);
				DomainToLocalRect(l2, &r);
				StRegion	rgn(r);
				UnionRgn(*ioRgn, rgn, *ioRgn);
				ThrowIfOSErr_(QDError());
				
				if ((p2.v - h2) > p1.v) {

					//	region between the two lines
					Domain32T	mb(
									Range32T(extremeLeft, extremeRight),
									Range32T(p1.v, p2.v - h2)
								);
					
					mb.Crop(vRect);
					DomainToLocalRect(mb, &r);
					RectRgn(rgn, &r);
					UnionRgn(*ioRgn, rgn, *ioRgn);
					ThrowIfOSErr_(QDError());
				}
			}
			
		} else {
		
			//	An insertion point
			SPoint32	p1;
			Rect		r;
			Range2Image(range, true, &p1);
			Domain32T	l1(Range32T(p1.h -1, p1.h), rHeight);
			l1.Crop(vRect);
			DomainToLocalRect(l1, &r);
			RectRgn(*ioRgn, &r);
			ThrowIfOSErr_(QDError());
		}
		
		exists = true;
		
	} while (false);

	if (!exists) {
		SetEmptyRgn(*ioRgn);
		ThrowIfOSErr_(QDError());
	}
}

//	===========================================================================
//	Presentation query

Boolean	LTextEngine::RangeIsMultiLine(const TextRangeT &inRange) const
{
	return true;
}

Int32	LTextEngine::GetRangeTop(const TextRangeT &inRange) const
{
	TextRangeT	range = inRange;
	SPoint32	where;
	Int32		rval;
	
	range.Crop(GetTotalRange());
	range.SetLength(0);
	Range2Image(range, true, &where);
	rval = where.v - GetRangeHeight(range);
	return rval;
}

Int32	LTextEngine::GetRangeHeight(const TextRangeT &inRange) const
{
	Assert_(false);
	return 0;
}

Int32	LTextEngine::GetRangeWidth(const TextRangeT &inRange) const
{
	SPoint32	anchor, end;
	Int32		width;
	TextRangeT	range = inRange;

	if (range.Length() == 0)
		return 0;

	range.Crop(GetTotalRange());
	Range2Image(range, true, &anchor);
	Range2Image(range, false, &end);
	
	width = end.h - anchor.h;
	
	if (end.v != anchor.v) {
		SDimension32	size;
		
		GetImageSize(&size);
		
		return size.width;
	}
	
	if (width <= 0) {
		SDimension32	size;
		
		GetImageSize(&size);
		
		if (width == 0)
			return size.width;

		width += size.width;
	}
	
	Assert_(width > 0);
	
	return width;
}

void	LTextEngine::Range2Image(
	const TextRangeT	&inRange,
	Boolean				inLeadingEdge,
	SPoint32			*outWhere) const
{
}

void	LTextEngine::Image2Range(
	SPoint32			inWhere,
	Boolean				*outLeadingEdge,
	TextRangeT			*outRange) const
{
}

Boolean	LTextEngine::PointInRange(SPoint32 inWhere, const TextRangeT &inRange) const
/*
	Returns true iff:
	
		inRange.Length() > 0:	inWhere lies on top of any glyph in inRange
		inRange.Length() = 0:	inWhere lies on same "side" of indicated
								insertion point.
*/
{
	TextRangeT	rangeFound,
				range = inRange;
	Boolean		leadingEdge,
				rval = false;
	
	range.WeakCrop(GetTotalRange());
	Image2Range(inWhere, &leadingEdge, &rangeFound);
	
	if (range.Length()) {
		rval = range.Contains(rangeFound);
	} else {
		if (rangeFound.Length()) {
			if (leadingEdge)
				rangeFound.Front();
			else
				rangeFound.Back();
		}
		rval = range == rangeFound;
	}

	//	Unusual construction for breakpoint handles
	if (rval)
		return true;
	else
		return false;
}

//	===========================================================================
//	Styles

LStyleSet *	LTextEngine::GetDefaultStyleSet(void) const
{
	return mDefaultStyleSet;
}

void	LTextEngine::SetDefaultStyleSet(LStyleSet *inNewStyle)
{
	ThrowIfNULL_(inNewStyle);
	ReplaceSharable(mDefaultStyleSet, inNewStyle, this);
}

LStyleSet *	LTextEngine::GetStyleSet(const TextRangeT &inRange, Int32 inIndex) const
{
	return mDefaultStyleSet;
}

void	LTextEngine::SetStyleSet(const TextRangeT &inRange, const LStyleSet *inNewStyle)
{
	Assert_(false);
}

Int32	LTextEngine::CountStyleSets(const TextRangeT &inRange) const
{
	return 1;
}

void	LTextEngine::GetStyleSetRange(TextRangeT *ioRange) const
{
	ThrowIfNULL_(ioRange);
	
	*ioRange = GetTotalRange();
}

void	LTextEngine::StyleSetChanged(LStyleSet *inChangedStyle)
{
}

//	===========================================================================
//	Rulers

LRulerSet *	LTextEngine::GetDefaultRulerSet(void) const
{
	return mDefaultRulerSet;
}

void	LTextEngine::SetDefaultRulerSet(LRulerSet *inNewRuler)
{
	ThrowIfNULL_(inNewRuler);
	ReplaceSharable(mDefaultRulerSet, inNewRuler, this);
}

LRulerSet *	LTextEngine::GetRulerSet(const TextRangeT &inRange, Int32 inIndex) const
{
	return mDefaultRulerSet;
}

void	LTextEngine::SetRulerSet(const TextRangeT &inRange, const LRulerSet *inNewRuler)
{
	Assert_(false);
}

Int32	LTextEngine::CountRulerSets(const TextRangeT &inRange) const
{
	return 1;
}

void	LTextEngine::GetRulerSetRange(TextRangeT *ioRange) const
{
	ThrowIfNULL_(ioRange);
	
	*ioRange = GetTotalRange();
}

void	LTextEngine::RulerSetChanged(LRulerSet *inChangedRuler)
{
}

//	===========================================================================
//	Implementation help:

void	LTextEngine::Focus(void)
{
	if (mHandlerView)
		mHandlerView->FocusDraw();
}

void	LTextEngine::CheckImageSize(void)
{
	SDimension32	imageSize;
	
	GetImageSize(&imageSize);
	if ((mImageSize.width != imageSize.width) || (mImageSize.height != imageSize.height)) {
		BroadcastMessage(msg_ModelScopeChanged);
		Focus();
		mImageSize = imageSize;
	}
}

void	LTextEngine::RecalcSelf(void)
{
	if (mFlags & recalc_WindowUpdate) {

		//	views have presumably already been invalidated		
		mFlags &= ~recalc_WindowUpdate;
	}
	
	CheckImageSize();
	
	LRecalcAccumulator::RecalcSelf();
}

void	LTextEngine::Refresh(RecalcFlagsT inFlags)
{
	RecalcFlagsT	oldFlags = mFlags;
	
	if (mUpdateLevel) {
		Assert_(!(inFlags & recalc_WindowUpdate));
		LRecalcAccumulator::Refresh(inFlags & (~recalc_WindowUpdate));
	} else {
		LRecalcAccumulator::Refresh(inFlags | recalc_WindowUpdate);
	
		if (!(oldFlags & recalc_WindowUpdate)) {
			BroadcastMessage(msg_InvalidateImage);
		}
	}
}

void	LTextEngine::Refresh(const TextRangeT &inRange)
{
	TextRangeT	range = inRange;
	range.Crop(GetTotalRange());

	if (mUpdateRange.Undefined())
		mUpdateRange = range;
	else
		mUpdateRange.Union(range);
	
	Refresh(recalc_Range);
}

void	LTextEngine::TextFluxPre(const TextRangeT &inRange)
{
//-.	Assert_(mUpdateLevel > 0);	//	can only be used inside a StRecalculator
	
	mPreFluxRange = inRange;	
	
	Refresh(inRange);
}

void	LTextEngine::TextFluxPost(Int32 inLengthDelta)
{
//-.	Assert_(mUpdateLevel > 0);	//	can only be used inside a StRecalculator

	//	Update total range
	mTotalRange.SetLength(mTotalRange.Length() + inLengthDelta, rangeCode_After);
	
	//	Figure flux...
	Assert_(mPreFluxRange.Length() >= (-inLengthDelta));
	TextFluxRecordT	flux;
	flux.range = mPreFluxRange;
	flux.lengthDelta = inLengthDelta;
	
	//	Adjust mUpdateRange
	{
		TextRangeT	affectedRange = mPreFluxRange;
		affectedRange.Adjust(flux.range, inLengthDelta, true);
		
		if (mUpdateRange.Undefined()) {
			mUpdateRange = affectedRange;
		} else {
			mUpdateRange.Adjust(flux.range, inLengthDelta, true);
			mUpdateRange.Union(affectedRange);
		}
		mUpdateRange.Crop(GetTotalRange());

	}
	
	TextFluxPostSelf(mPreFluxRange, inLengthDelta);
	
	//	Broadcast flux
	try {
		BroadcastMessage(msg_TextFluxed, &flux);
	} catch (...) {
		Assert_(false);	//	you must not throw in a listen to msg_TextFluxed!
	};
}

void	LTextEngine::TextFluxPostSelf(
	const TextRangeT	&inRange,
	Int32				inLengthDelta)
{
}

void	LTextEngine::NestedUpdateIn(void)
{
	Assert_(mUpdateLevel > 0);
	
	if (mUpdateLevel == 1) {
		SDimension32	size;

		GetImageSize(&size);

		mUpdatePreHeight = size.height;	
		mUpdateRange = sTextUndefined;
		mUpdatePreLength = GetTotalRange().Length();
		
		BroadcastMessage(msg_ModelAboutToChange);
	}
}

void	LTextEngine::NestedUpdateOut(void)
{
	Assert_(mUpdateLevel > 0);
	
	if (mUpdateLevel != 1)
		return;
		
	TextRangeT	changedRange = mUpdateRange;	//	Shadow because the recalc & broadcast may change it again

	Recalc();
	
	if (!mUpdateRange.Undefined())
		BroadcastMessage(msg_CumulativeRangeChanged, &changedRange);
	
	if (!mUpdateRange.Undefined() && !(mFlags & recalc_WindowUpdate)) {
	
		Recalc();	//	Maybe the previous broadcast caused a change
	
		TextUpdateRecordT	update;
		SDimension32		sizeNow;
		TextRangeT			range = mUpdateRange;
		
		GetImageSize(&sizeNow);
		range.Crop(GetTotalRange());

		//	expand range to include all of changed paragraphs -- their lines
		//	may have been reflowed
		{
			TextRangeT	next = range;
			next.After();
			
			if (typeNull != FindPart(GetTotalRange(), &next, cParagraph))
				range.SetEnd(URange32::Max(range.End(), next.End()));
		}
		
		//	gap area (affected area)
		update.gap = Range32T(GetRangeTop(range));
		if (range.End() == GetTotalRange().End()) {
			update.gap.SetLength(sizeNow.height - update.gap.Start());
		} else {
			update.gap.SetLength(GetRangeHeight(range));	
		}
		Assert_(update.gap.Length() > 0);
		
		//	scolling parameters
		update.scrollAmount = sizeNow.height - mUpdatePreHeight;
		update.scrollStart = update.gap.End();
		if (update.scrollAmount > 0)
			update.scrollStart -= update.scrollAmount;

		BroadcastMessage(msg_UpdateImage, &update);		
	}
	
	mUpdatePreLength = GetTotalRange().Length();

	BroadcastMessage(msg_ModelChanged);		
}

Boolean	LTextEngine::SpaceForBytes(Int32 inAdditionalBytes) const
{
	return false;
}

//	PREMULTIBYTE is for regression performance testing only
#ifndef	PREMULTIBYTE

	Int32	LTextEngine::ThisCharSize(Int32 inInsertionLocation) const
	/*
		Returns the size of the character at inInsertionLocation
		offset.  Usage is required for multi-byte text engines.
		
		Will return 0 if inInsertionLocation is past an end of the text.
		
		Will fail if inInsertionLocation is not at a character boundary.
	*/
	{
	/*
		Defaults to a single byte per character system.
	*/
		if (inInsertionLocation >= GetTotalRange().End()) {
			return 0;
		} else {
			TextRangeT	range(inInsertionLocation);
			range.SetInsertion(rangeCode_After);
			
			StHandleLocker	lockText(GetTextHandle());
			LStyleSet		*ss = GetStyleSet(range);
			ThrowIfNULL_(ss);
			TextRangeT		queryRange = range;
			GetStyleSetRange(&queryRange);
			return ss->FindCharSize(queryRange, *GetTextHandle(), range);
		}
	}
	
	Int32	LTextEngine::PrevCharSize(Int32 inInsertionLocation) const
	/*
		Returns the size of the character immediately before the inInsertionLocation
		offset.  Usage is required for multi-byte text engines.
	
		Will return 0 if inInsertionLocation is at the begginning of all text.
	*/
	{
		if (inInsertionLocation <= GetTotalRange().Start()) {
			return 0;
		} else {
			TextRangeT	range(inInsertionLocation);
			range.SetInsertion(rangeCode_Before);
			
			StHandleLocker	lockText(GetTextHandle());
			LStyleSet		*ss = GetStyleSet(range);
			ThrowIfNULL_(ss);
			TextRangeT		queryRange = range;
			GetStyleSetRange(&queryRange);
			return ss->FindCharSize(queryRange, *GetTextHandle(), range);
		}
#if	0	
			//	dang! -- we have to make an inefficient search!
			TextRangeT	styleRange(inInsertionLocation);
			styleRange.SetInsertion(rangeCode_Before);
			TextRangeT	paraRange = styleRange;
			DescType	partFound = FindPart(sTextAll, &paraRange, cParagraph);
//-?			ThrowIf_(partFound == typeNull);

			if ((paraRange.Start() == styleRange.Start()) || !paraRange.Length()) {

				if (paraRange.Start() == GetTotalRange().Start())
					return 0;
				
				//	backup a line
				if (paraRange.Start() > GetTotalRange().End()) {
					paraRange = TextRangeT(GetTotalRange().End());
				}
				paraRange.SetInsertion(rangeCode_Before);
				
				partFound = FindPart(sTextAll, &paraRange, cParagraph);
				ThrowIf_(partFound == typeNull);
			}

			GetStyleSetRange(&styleRange);
			LStyleSet *ss = GetStyleSet(styleRange);
			ThrowIfNULL_(ss);

			styleRange.Crop(paraRange);	//	restrict search -- para start starts on char boundary

			Int32		startPos = styleRange.Start();
			Int32		offset = inInsertionLocation - startPos;
			ScriptCode	styleScript = ss->GetScript();
			
			offset--;	//	back up to possible previous single byte
			if (offset > max_Int16) {

				//	There is a line longer than the "short offset" in CharacterByteType
				//	can handle.  This should really never happens except in a psycho test
				//	case.  Rather than throw, we'll assume the character length was 1.
				Assert_(false);
				rval = 1;
				
			} else {
				short type = CharacterByteType((Ptr)GetTextPtr(startPos), offset, styleScript);
				switch (type) {

					case smSingleByte:
						rval = 1;
						break;
					
					case smLastByte:
						rval = 2;
						break;
					
					default:
						Throw_(paramErr);	//	what?
						break;
				}
			}
		} else {
			rval = 1;
		}
		
		return rval;
#endif
	}
	
	Int16	LTextEngine::ThisChar(Int32 inInsertionLocation) const
	/*
		Returns the character immediately following the character at
		inInsertionLocation.
		
		Note the return value is the 16 bit multi-byte value!
		
		Will fail if attempting to return character past ends of text.
	*/
	{
		Int32			size = ThisCharSize(inInsertionLocation);
		Uint16			c = 0;
		unsigned char	*p,
						**q = (unsigned char **)((LTextEngine *)this)->GetTextHandle();
		
		ThrowIfNULL_(q);
		p = *q;
		ThrowIfNULL_(p);
		
		p += inInsertionLocation;
		
		switch (size) {
			
			case 2:
				c |= *p++;
				c = c << 8;
				//	fall through...
				
			case 1:
				c |= *p;
				break;
			
			case 0:		//	Can't get value for a non-existent character
			default:	//	Can't get value for a non-text "character"
				break;
		}
		
		return c;
	}

#else

	//	For regression performance testing only...

	Int32	LTextEngine::ThisCharSize(Int32 inInsertionLocation) const
	{
		TextRangeT	totalRange;
		
		GetTotalRange(&totalRange);
		
		if (inInsertionLocation >= (totalRange.start + totalRange.length))
			return 0;
		else
			return 1;
	}
	
	Int32	LTextEngine::PrevCharSize(Int32 inInsertionLocation) const
	{
		TextRangeT	totalRange;
		
		GetTotalRange(&totalRange);
		
		if (inInsertionLocation <= totalRange.start)
			return 0;
		else
			return 1;
	}
	
	Int16	LTextEngine::ThisChar(Int32 inInsertionLocation) const
	{
		char	*p, **q = ((LTextEngine *)this)->GetTextHandle();
		
		ThrowIfNULL_(q);
		p = *q;
		ThrowIfNULL_(p);
		
		return *(p + inInsertionLocation);
	}
	
#endif

