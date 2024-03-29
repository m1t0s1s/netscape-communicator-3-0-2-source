//	===========================================================================
//	LTargeter.h						�1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include <LView.h>

class	LCommander;

class	LTargeter : public LView {
public:
					LTargeter();
//					LTargeter(LStream *inStream);
	virtual			~LTargeter();
	
	virtual void	FinishCreateSelf(void);

	virtual void	AttachPane(LPane *inPane);
	virtual void	AttachTarget(LCommander *inTarget);
	
	virtual void	ShowFocus(void);
	virtual void	HideFocus(void);

protected:
	virtual void	DrawSelf();	
	virtual void	ClickSelf(const SMouseDownEvent &inMouseDown);
	
	Boolean			mIsTargeted;
	LCommander		*mTarget;
	LPane			*mGrappledPane;
};
