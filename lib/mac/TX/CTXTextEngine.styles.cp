// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ
//	CTXTextEngine.styles.cp				  ©1995 Netscape. All rights reserved.
// ÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑÑ

#include	"CTXTextEngine.h"
#include	<UMemoryMgr.h>
#include	"CTXStyleSet.h"

#include	"ObjectsRanges.h"
#include	"Textension.h"
#include	"QDTextRun.h"
//#include	"Frames.h"

#ifndef		__ERRORS__
#include	<Errors.h>
#endif

//#pragma	warn_unusedarg off

//	===========================================================================
//	Styles:

void	CTXTextEngine::SetDefaultStyleSet(LStyleSet *inNewStyle)
{
	inherited::SetDefaultStyleSet(inNewStyle);
	
	CTXStyleSet	*style = (CTXStyleSet *) inNewStyle;
//	Assert_(member(style, CTXStyleSet));
	
//	SetDefaultRun_(style);

	//	Changing the default styleset can change the fixed line height so...
	SetAttributes(GetAttributes());
	Refresh(recalc_FormatAllText);
}

/*
	Returns the number of style set changes + 1 in inRange.  If the same
	style set occurs multiple times in inRange, it is counted a multiple
	of times.
*/
Int32	CTXTextEngine::CountStyleSets(const TextRangeT &inRange) const
{
	Int32		rval = 0;
	Int32		offset, length;
	TextRangeT	range = inRange;

	range.Crop(GetTotalRange());

	if (!range.Length())
		return 1;
		
	offset = range.Start();
	
	const CRunObject	*run = mText->GetNextRun(offset, &length);
	while (run) {
		rval++;
		offset += length;
		if (offset > range.End())
			break;
		run = mText->GetNextRun(offset, &length);
	}

	return rval;
}

LStyleSet *	CTXTextEngine::GetStyleSet(const TextRangeT &inRange, Int32 inIndex) const
{
	LStyleSet			*rval = NULL;
	Int32				offset,
						length;
	TextRangeT			range = inRange;
	const CRunObject	*run;

	if (inIndex <= 0)
		Throw_(paramErr);
	
	range.Crop(GetTotalRange());
	
	if (range.Insertion() == rangeCode_Before)
		offset = range.Start() -1;
	else
		offset = range.Start();
	
	run = mText->GetNextRun(offset, &length);
	while (run) {
		inIndex--;

		if (inIndex <= 0)
			break;

		offset += length;
		if (offset > range.End())
			break;

		run = mText->GetNextRun(offset, &length);
	}

	if ((inIndex > 0) || (run == NULL)) {
//		Assert_(false);

		ThrowIfNULL_(mDefaultStyleSet);
		run = (CTXStyleSet *)mDefaultStyleSet;
//		Assert_(member(mDefaultStyleSet, CTXStyleSet));

//		old...
//		run = mText->GetDefaultRun();
	}

	ThrowIfNULL_(run);
	rval = (CTXStyleSet *)run;
//	Assert_(member(run, CTXStyleSet));
	return rval;
}

void	CTXTextEngine::SetStyleSet(const TextRangeT &inRange, const LStyleSet *inNewStyle)
{
	OSErr			err;
	TextRangeT		range = inRange,
					totRange;
	CTXStyleSet		*style = NULL;
	
	range.Crop(GetTotalRange());
	totRange = GetTotalRange();

	if (!inNewStyle)
		inNewStyle = GetDefaultStyleSet();
	
	ThrowIfNULL_(inNewStyle);
	style = (CTXStyleSet *) inNewStyle;
//	Assert_(member(style, CTXStyleSet));
	
//	style->Reference();
	err = mText->GetRunsRanges()->ReplaceRange(range.Start(), range.Length(), range.Length(),
		style, false);
	ThrowIfOSErr_(err);
	
	Refresh(range);
}

void CTXTextEngine::StyleSetChanged(LStyleSet *inChangedStyle)
{
	Refresh(recalc_FormatAllText);
}

void CTXTextEngine::GetStyleSetRange(TextRangeT *ioRange) const
{
	Int32	offset = ioRange->Start();
	if ((ioRange->Insertion() == rangeCode_Before) && (offset > 0)) {
		offset--;	//	This may not be a whole character size but it shouldn't make a
					//	difference for the binary search.
	}
	
	CRunsRanges	*runs = mText->GetRunsRanges();
	Int32		runIndex = runs->OffsetToRangeIndex(TOffset(offset));

	ioRange->SetStart(runs->GetRangeStart(runIndex));
	ioRange->SetLength(runs->GetRangeLen(runIndex));
}


