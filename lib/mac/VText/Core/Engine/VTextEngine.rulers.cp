//	===========================================================================
//	VTextEngine.rulers
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VTextEngine.h"
#include	<LRulerSet.h>
#include	<VPartCache.h>

#pragma	warn_unusedarg off

//	===========================================================================
//	Rulers

LRulerSet *	VTextEngine::GetRulerSet(const TextRangeT &inRange, Int32 inIndex) const
{
	LRulerSet	*rval = GetDefaultRulerSet();
	TextRangeT	range = inRange;
	Range32T	index;
	
	range.Crop(GetTotalRange());

	if (mRulers && mRulers->GetRecordCount()) {
		mRulers->Range2Index(range, &index);
		rval = mRulers->GetRuler(index.Start() + inIndex -1);
	}
	
	return rval;
}

void	VTextEngine::SetRulerSet(const TextRangeT &inRange, const LRulerSet *inNewRuler)
{
	TextRangeT	range = inRange;
	range.Crop(GetTotalRange());

	if (range.Length()) {
//-.		StRecalculator	change(this);
		
		if (!mRulers)
			mRulers = new VRulerCache(*this);
		
		mRulers->SetRuler(range, (LRulerSet *)inNewRuler);
		Refresh(range);
	}
}

Int32	VTextEngine::CountRulerSets(const TextRangeT &inRange) const
{
	Int32	rval = 1;
	
	if (mRulers) {
		Range32T	index;
		TextRangeT	range = inRange;
		
		range.Crop(GetTotalRange());
		mRulers->Range2Index(range, &index);
		rval = index.Length();
	}
	
	return rval;
}

void	VTextEngine::GetRulerSetRange(TextRangeT *ioRange) const
{
	ThrowIfNULL_(ioRange);
	
	if (mRulers) {
		ioRange->Crop(GetTotalRange());
		Range32T	index;
		if (!mRulers->Range2Index(*ioRange, &index))
			mRulers->Index2Range(index, ioRange);
	} else {
		*ioRange = GetTotalRange();
	}
}

void	VTextEngine::RulerSetChanged(LRulerSet *inChangedRuler)
{
	TextRangeT	affectedRange,
				totalIndex,
				range;

	if (mRulers) {
		mRulers->Range2Index(GetTotalRange(), &totalIndex);
		
		for (Int32 i = 0; i < totalIndex.End(); i++) {
			if (inChangedRuler != mRulers->GetRuler(i))
				continue;
			TextRangeT	indexRange(i, i+1);
			mRulers->Index2Range(indexRange, &range);
			affectedRange.Union(range);
		}
	} else {
		if (inChangedRuler == GetDefaultRulerSet())
			affectedRange = GetTotalRange();
	}
	
	if (!affectedRange.Undefined()) {
//?-	StRecalculator	change(this);
		
		Refresh(affectedRange);
	}
}

