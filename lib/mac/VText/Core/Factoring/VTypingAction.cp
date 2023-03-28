// ===========================================================================
//	VTypingAction.cp
// ===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VTypingAction.h"
#include	<LTextSelection.h>
#include	<LStyleSet.h>
#include	<LRulerSet.h>

#include	<UMemoryMgr.h>

#pragma	warn_unusedarg off

VTypingAction::VTypingAction(LTextEngine *inTextEngine)
:	LAETypingAction(inTextEngine)
{
	mTypingStyle = NULL;
}

VTypingAction::~VTypingAction()
{
	{
		StyleSetRefF	styleRefF;
		
		vector<StyleRunT>::iterator	ip = mStyles.begin(),
									ie = mStyles.end();

		while (ip != ie) {
			ReplaceSharable(styleRefF(*ip), (LStyleSet *)NULL, this);
			ip++;
		}
	}
	{
		RulerSetRefF	rulerRefF;
		
		vector<RulerRunT>::iterator	ip = mRulers.begin(),
									ie = mRulers.end();

		while (ip != ie) {
			ReplaceSharable(rulerRefF(*ip), (LRulerSet *)NULL, this);
			ip++;
		}
	}
	
	ReplaceSharable(mTypingStyle, (LStyleSet *)NULL, this);
}

void	VTypingAction::FinishCreateSelf(void)
{
	inherited::FinishCreateSelf();
	
	ReplaceSharable(mTypingStyle, mTextSelection->GetCurrentStyle(), this);
}

Boolean	VTypingAction::NewTypingLocation(void)	
{
	if (inherited::NewTypingLocation())
		return true;
	
	if (mTypingStyle != mTextSelection->GetCurrentStyle())
		return true;
	else
		return false;
}

void	VTypingAction::WriteTypingData(LAEStream &inStream) const
{
	StHandleLocker	lock(mNewText.mData);
	ScriptCode		script = mTypingStyle->GetScript();
	LangCode		language = mTypingStyle->GetLanguage();
	Size			size = mNewText.GetSize();
	
	inStream.OpenDesc(typeIntlText);
	inStream.WriteData(&script, sizeof(script));
	inStream.WriteData(&language, sizeof(language));
	inStream.WriteData(*(mNewText.mData), size);
	inStream.CloseDesc();
}

void	VTypingAction::RedoSelf(void)
{
	if (mFinalized) {
		inherited::RedoSelf();
	} else {
		if (!mIsDone) {
		
			inherited::RedoSelf();
			
			mTextEngine->SetStyleSet(
				TextRangeT(
					mOldRange.Start(), 
					mOldRange.Start() + mNewText.GetSize()
				),
				mTypingStyle
			);

		}
	}
}

void	VTypingAction::UndoSelf(void)
{
	if (mFinalized) {
		inherited::UndoSelf();
	} else {
		if (mIsDone) {
		
			inherited::UndoSelf();
			
			//	restore styles
			{
				TextRangeT	scanRange = mOldRange;
				scanRange.Front();
				vector<StyleRunT>::iterator	ip = mStyles.begin(),
											ie = mStyles.end();
											
				while ((scanRange.End() < mOldRange.End()) && (ip != ie)) {
					scanRange.SetLength((*ip).length);
					ThrowIf_(!mOldRange.Contains(scanRange));
					mTextEngine->SetStyleSet(scanRange, (*ip).style);
					scanRange.After();
					ip++;
				}
			}
			
			//	restore rulers
			{
				TextRangeT	scanRange = mOldRange;
				scanRange.Front();
				vector<RulerRunT>::iterator	ip = mRulers.begin(),
											ie = mRulers.end();
											
				while ((scanRange.End() < mOldRange.End()) && (ip != ie)) {
					scanRange.SetLength((*ip).length);
					ThrowIf_(!mOldRange.Contains(scanRange));
					mTextEngine->SetRulerSet(scanRange, (*ip).ruler);
					scanRange.After();
					ip++;
				}
			}
			

		}
	}
}

void	VTypingAction::PrimeFirstRemember(const TextRangeT &inRange)
{
	inherited::PrimeFirstRemember(inRange);
}

void	VTypingAction::KeyStreamRememberNew(const TextRangeT &inRange)
{
	inherited::KeyStreamRememberNew(inRange);
	
	//	Nothing new to do since all new text will be of one style?
}

void	VTypingAction::KeyStreamRememberAppend(const TextRangeT &inRange)
{
	inherited::KeyStreamRememberAppend(inRange);
	
	if (inRange.Length()) {

		//	remember styles
		{
			TextRangeT	scanRange = inRange;
			scanRange.Front();
			vector<StyleRunT>::iterator	ip = mStyles.size()
											? (mStyles.end() -1)
											: mStyles.end(),
										ie = mStyles.end();

			while (scanRange.End() < inRange.End()) {
				mTextEngine->GetStyleSetRange(&scanRange);
				LStyleSet	*set = mTextEngine->GetStyleSet(scanRange);
				scanRange.Crop(inRange);
				
				if ((ip != ie) && ((*ip).style == set)) {
					(*ip).length += scanRange.Length();
				} else {
					StyleRunT	record;
					record.style = NULL;
					ReplaceSharable(record.style, set, this);
					record.length = scanRange.Length();
					mStyles.push_back(record);
					ie = mStyles.end();
					ip = ie -1;
				}
				scanRange.After();
			}
		}
		
		//	remember rulers
		{
			TextRangeT	scanRange = inRange;
			scanRange.Front();
			vector<RulerRunT>::iterator	ip = mRulers.size()
											? (mRulers.end() -1)
											: mRulers.end(),
										ie = mRulers.end();

			while (scanRange.End() < inRange.End()) {
				mTextEngine->GetRulerSetRange(&scanRange);
				LRulerSet	*ruler = mTextEngine->GetRulerSet(scanRange);
				scanRange.Crop(inRange);
				
				if ((ip != ie) && ((*ip).ruler == ruler)) {
					(*ip).length += scanRange.Length();
				} else {
					RulerRunT	record;
					record.ruler = NULL;
					ReplaceSharable(record.ruler, ruler, this);
					record.length = scanRange.Length();
					mRulers.push_back(record);
					ie = mRulers.end();
					ip = ie -1;
				}
				scanRange.After();
			}
		}
		
	}
}

void	VTypingAction::KeyStreamRememberPrepend(const TextRangeT &inRange)
{
	inherited::KeyStreamRememberPrepend(inRange);

	if (inRange.Length()) {
		//	remember styles
		{
			TextRangeT	scanRange = inRange;
			scanRange.Back();
			vector<StyleRunT>::iterator	ip = mStyles.begin(),
										ie = mStyles.end();

			while (scanRange.Start() > inRange.Start()) {
				mTextEngine->GetStyleSetRange(&scanRange);
				LStyleSet	*set = mTextEngine->GetStyleSet(scanRange);
				scanRange.Crop(inRange);
				
				if ((ip != ie) && ((*ip).style == set)) {
					(*ip).length += scanRange.Length();
				} else {
					StyleRunT	record;
					record.style = NULL;
					ReplaceSharable(record.style, set, this);
					record.length = scanRange.Length();
					mStyles.insert(ip, record);
					ip = mStyles.begin();
					ie = mStyles.end();
				}
				scanRange.Before();
			}
		}
				
		//	remember rulers
		{
			TextRangeT	scanRange = inRange;
			scanRange.Back();
			vector<RulerRunT>::iterator	ip = mRulers.begin(),
										ie = mRulers.end();

			while (scanRange.Start() > inRange.Start()) {
				mTextEngine->GetRulerSetRange(&scanRange);
				LRulerSet	*ruler = mTextEngine->GetRulerSet(scanRange);
				scanRange.Crop(inRange);
				
				if ((ip != ie) && ((*ip).ruler == ruler)) {
					(*ip).length += scanRange.Length();
				} else {
					RulerRunT	record;
					record.ruler = NULL;
					ReplaceSharable(record.ruler, ruler, this);
					record.length = scanRange.Length();
					mRulers.insert(ip, record);
					ip = mRulers.begin();
					ie = mRulers.end();
				}
				scanRange.Before();
			}
		}
				
	}
}
