//	===========================================================================
//	VTypingAction.h
//	===========================================================================

#pragma once

#include	<LAETypingAction.h>
#include	<vector.h>
#include	<VTextCacheTypes.h>

class	VTypingAction
			: public LAETypingAction
{
public:
						VTypingAction(LTextEngine *inTextEngine);
	virtual				~VTypingAction();

	virtual void		FinishCreateSelf(void);
	
	virtual Boolean		NewTypingLocation(void);

	virtual void		RedoSelf(void);
	virtual void		UndoSelf(void);

	virtual void		PrimeFirstRemember(const TextRangeT &inRange);
	virtual	void		KeyStreamRememberNew(const TextRangeT &inRange);
	virtual void		KeyStreamRememberAppend(const TextRangeT &inRange);
	virtual void		KeyStreamRememberPrepend(const TextRangeT &inRange);

protected:
	virtual void		WriteTypingData(LAEStream &inStream) const;

	vector<StyleRunT>	mStyles;
	vector<RulerRunT>	mRulers;
	
	LStyleSet			*mTypingStyle;
};
