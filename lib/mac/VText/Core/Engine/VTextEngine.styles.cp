//	===========================================================================
//	VTextEngine.styles
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VTextEngine.h"
#include	<LStyleSet.h>
#include	<VPartCache.h>

#pragma	warn_unusedarg off

//	===========================================================================
//	Styles

LStyleSet *	VTextEngine::GetStyleSet(const TextRangeT &inRange, Int32 inIndex) const
{
	LStyleSet	*rval = GetDefaultStyleSet();
	TextRangeT	range = inRange;
	Range32T	index;
	
	range.Crop(GetTotalRange());

	if (mStyles && mStyles->GetRecordCount()) {
		mStyles->Range2Index(range, &index);
		rval = mStyles->GetStyle(index.Start() + inIndex -1);
	}
	
	return rval;
}

void	VTextEngine::SetStyleSet(const TextRangeT &inRange, const LStyleSet *inNewStyle)
{
	TextRangeT	range = inRange;
	range.Crop(GetTotalRange());

	if (range.Length()) {
//-.		StRecalculator	change(this);
		
		if (!mStyles)
			mStyles = new VStyleCache(*this);
		
		mStyles->SetStyle(range, (LStyleSet *)inNewStyle);
		Refresh(range);
	}
}

Int32	VTextEngine::CountStyleSets(const TextRangeT &inRange) const
{
	Int32	rval = 1;
	
	if (mStyles) {
		Range32T	index;
		TextRangeT	range = inRange;
		
		range.Crop(GetTotalRange());
		mStyles->Range2Index(range, &index);
		rval = index.Length();
	}
	
	return rval;
}

void	VTextEngine::GetStyleSetRange(TextRangeT *ioRange) const
{
	ThrowIfNULL_(ioRange);
	
	if (mStyles) {
		ioRange->Crop(GetTotalRange());
		Range32T	index;
		if (!mStyles->Range2Index(*ioRange, &index))
			mStyles->Index2Range(index, ioRange);
	} else {
		*ioRange = GetTotalRange();
	}
}

void	VTextEngine::StyleSetChanged(LStyleSet *inChangedStyle)
{
	TextRangeT	affectedRange,
				totalIndex,
				range;

	if (mStyles && GetTotalRange().Length()) {
		mStyles->Range2Index(GetTotalRange(), &totalIndex);
		
		for (Int32 i = 0; i < totalIndex.End(); i++) {
			if (inChangedStyle != mStyles->GetStyle(i))
				continue;
			TextRangeT	indexRange(i, i+1);
			mStyles->Index2Range(indexRange, &range);
			affectedRange.Union(range);
		}
	} else {
		if (inChangedStyle == GetDefaultStyleSet())
			affectedRange = GetTotalRange();
	}
	
	if (!affectedRange.Undefined()) {
//?-	StRecalculator	change(this);
		
		Refresh(affectedRange);
	}
}
