//	===========================================================================
//	VTextHandler.h
//	===========================================================================

#pragma	once

#include	<LTextEditHandler.h>
#include	<VText_ClassIDs.h>

class	VInlineTSMProxy;

class	VTextHandler
			:	public LTextEditHandler
{
public:

				//	Maintenance
						VTextHandler();
	virtual				~VTextHandler();
	enum {class_ID = kVTCID_VTextHandler};
	virtual ClassIDT	GetClassID() const {return class_ID;}
	virtual void		SetTSMProxy(VInlineTSMProxy *inProxy);
	virtual	void		Reset(void);
//-	virtual void		Activate(void);
//-	virtual void		Deactivate(void);	
	
	virtual void		FollowKeyScript(void);
	virtual void		FollowStyleScript(void);

//-	virtual Boolean		DoLiteralEvent(const EventRecord &inEvent);

	virtual	LAETypingAction *
						NewTypingAction(void);
						
protected:
	
	virtual void		ApplyPendingStyle(const TextRangeT &inRange);
	virtual void		PurgePendingStyle(void);
	virtual	Boolean		DoCondition(EventConditionT inCondition);

	virtual void		DoNonTypingKeySelf(
							const TextRangeT	&inPreSelectionRange,
							TextRangeT			*outPostSelectionRange);

	virtual	Boolean		TryAdvance(
							char				inKey,
							Boolean				inCharIsLeftToRight,
							const TextRangeT	&inInsertion,
							TextRangeT			&ioRange);

	VInlineTSMProxy	*mTSMProxy;
	
	Boolean		mMemoryOrderArrowKeys;
};
