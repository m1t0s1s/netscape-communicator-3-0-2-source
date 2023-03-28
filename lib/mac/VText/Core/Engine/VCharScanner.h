//	===========================================================================
//	VCharScanner
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#pragma once

#include	<LTextEngine.h>
#include	<UMemoryMgr.h>

class VCharScanner
{
public:
						VCharScanner(const LTextEngine &inTextEngine);
						VCharScanner(
							const LTextEngine 	&inTextEngine,
							const TextRangeT	&inQueryRange,
							const TextRangeT	&inStartRange);

	void				Reset(const TextRangeT	&inStartRange);
	void				Reset(
							const TextRangeT	&inQueryRange,
							const TextRangeT	&inStartRange);

	Boolean				Finished(void) const		{return mFinished;}

	void				Next();
	void				Prev();
	void				FindNext(Int16 inChar, ScriptCode inScript = smAllScripts);
	void				FindPrev(Int16 inChar, ScriptCode inScript = smAllScripts);
	
	const TextRangeT &	PresentRange(void) const	{return mRange;}
	Int16				PresentChar(void) const;
	ScriptCode			PresentScript(void) const;

protected:
	const LTextEngine	*mTextEngine;
	StHandleLocker		mTextLocker;
	const Ptr			mText;
	TextRangeT			mQueryRange;
	TextRangeT			mStyleRange;
	LStyleSet			*mStyle;

	TextRangeT			mRange;
	Boolean				mFinished;
};
