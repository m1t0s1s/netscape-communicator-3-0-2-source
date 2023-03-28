//	===========================================================================
//	VCharScanner
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VCharScanner.h"
#include	<LTextEngine.h>
#include	<LStyleSet.h>

VCharScanner::VCharScanner(
	const LTextEngine 	&inTextEngine)
: mTextEngine(&inTextEngine)
, mTextLocker(inTextEngine.GetTextHandle())
, mText(*(inTextEngine.GetTextHandle()))
{
	mQueryRange = mTextEngine->GetTotalRange();
	Reset(LTextEngine::sTextStart);
}

VCharScanner::VCharScanner(
	const LTextEngine 	&inTextEngine,
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inStartRange)
: mTextEngine(&inTextEngine)
, mTextLocker(inTextEngine.GetTextHandle())
, mText(*(inTextEngine.GetTextHandle()))
{
	mQueryRange = inQueryRange;
	mQueryRange.Crop(mTextEngine->GetTotalRange());
	Reset(inStartRange);
}

void	VCharScanner::Reset(
	const TextRangeT	&inStartRange)
{
	mStyle = NULL;
	mStyleRange = Range32T::sUndefined;
	
	mRange = inStartRange;

	mFinished = false;
}

void	VCharScanner::Reset(
	const TextRangeT	&inQueryRange,
	const TextRangeT	&inStartRange)
{
	mQueryRange = inQueryRange;
	mQueryRange.Crop(mTextEngine->GetTotalRange());
	Reset(inStartRange);
}

void	VCharScanner::Next(void)
{
	mRange.After();
	
	if (mRange.Start() == mQueryRange.End()) {
		mFinished = true;
		return;
	}
		
	if (mStyleRange.Undefined() || (mRange.Start() == mStyleRange.End())) {
		//	Advance style range;
		mStyleRange = mRange;
		mStyle = mTextEngine->GetStyleSet(mStyleRange);
		mTextEngine->GetStyleSetRange(&mStyleRange);
		mStyleRange.Crop(mQueryRange);
	}
	
	mRange.SetLength(mStyle->FindCharSize(mStyleRange, mText, mRange));
	
	ThrowIf_(!mRange.Length());
}

void	VCharScanner::Prev(void)
{
	mRange.Before();
	
	if (mRange.End() == mQueryRange.Start()) {
		mFinished = true;
		return;
	}
		
	if (mStyleRange.Undefined() || (mRange.Start() == mStyleRange.Start())) {
		//	Advance style range;
		mStyleRange = mRange;
		mStyle = mTextEngine->GetStyleSet(mStyleRange);
		mTextEngine->GetStyleSetRange(&mStyleRange);

		//	make PrevCharSize even faster by constraining mStyleRange to the enclosing
		//	paragraph -- which is assured of starting at a character boundary
		TextRangeT	paraRange = mRange;
		mTextEngine->FindPart(mQueryRange, &paraRange, cParagraph);
		mStyleRange.Crop(paraRange);
	}
	
	mRange.MoveStart(mRange.Start() - mStyle->FindCharSize(mStyleRange, mText, mRange));
	
	ThrowIf_(!mRange.Length());
}

void	VCharScanner::FindNext(
	Int16		inChar,
	ScriptCode	inScript)
{
	while (!Finished()) {
		Next();
		if ((inScript == smAllScripts) || inScript == mStyle->GetScript()) {
			if (PresentChar() == inChar)
				break;
		}
	}
}

void	VCharScanner::FindPrev(
	Int16		inChar,
	ScriptCode	inScript)
{
	while (!Finished()) {
		Prev();
		if ((inScript == smAllScripts) || inScript == mStyle->GetScript()) {
			if (PresentChar() == inChar)
				break;
		}
	}
}

Int16	VCharScanner::PresentChar(void) const
{
	Int16 rval = 0;
	
	if (mRange.Length() == 1)
		rval = *(mText + mRange.Start());
	else if (mRange.Length() == 2)
		rval = *((Int16 *)(mText + mRange.Start()));

	return rval;
}

