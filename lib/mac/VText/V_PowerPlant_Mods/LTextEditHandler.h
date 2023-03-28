//	===========================================================================
//	LTextHandler.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LDataDragEventHandler.h>

//	for TextRangeT...
#include	<LTextEngine.h>

class	LTextModel;
class	LTextSelection;

class	LTextEditHandler
			:	public LDataDragEventHandler
{
public:
	enum {class_ID = kPPCID_TextHandler};

				//	Maintenance
						LTextEditHandler();
	virtual				~LTextEditHandler();
	virtual ClassIDT	GetClassID() const {return class_ID;}
#ifndef	PP_No_ObjectStreaming
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
#endif
	virtual	void		Reset(void);
	
	virtual	LTextEngine *	GetTextEngine(void);
	virtual	void		SetTextEngine(LTextEngine *inTextEngine);
	virtual LTextModel *	GetTextModel(void);
	virtual void		SetTextModel(LTextModel *inTextModel);
	virtual void		SetSelection(LSelection *inSelection);
					
	virtual	LAETypingAction *
						NewTypingAction(void);
						
	virtual	Boolean		DoLiteralEvent(const EventRecord &inEvent);
protected:
				//	Implementation
	virtual	Boolean		DoCondition(EventConditionT inCondition);
	virtual void		KeyDown(void);
	virtual void		MouseMove(void);
	
	virtual void		FollowKeyScript(void);
	virtual void		FollowStyleScript(void);
	virtual void		ApplyPendingStyle(const TextRangeT &inRange);
	virtual void		PurgePendingStyle(void);
	
	virtual Boolean		IsNonTypingKey(const EventRecord &inEvent) const;
	virtual Boolean		IsTypingKey(const EventRecord &inEvent) const;
	virtual Boolean		IsNormalChar(const EventRecord &inEvent) const;

	virtual	void		DoTypingKey(void);
	virtual void		DoNormalChars(void);
	virtual	void		DoTypingKeySelf(
							const TextRangeT	&inPreSelectionRange,
							TextRangeT			*outPostSelectionRange);

	virtual void		DoNonTypingKey(void);
	virtual void		DoNonTypingKeySelf(
							const TextRangeT	&inPreSelectionRange,
							TextRangeT			*outPostSelectionRange);

	virtual Boolean		DragIsAcceptable(DragReference inDragRef);
	virtual void		EnterDropArea(DragReference inDragRef,
								Boolean inDragHasLeftSender);
	virtual void		LeaveDropArea(DragReference inDragRef);
	static LHandlerView	*sSavedHandlerView;

				//	for better updating
	virtual	LAction *	MakeCreateTask(void);
	virtual	LAction *	MakeCopyTask(void);
	virtual	LAction *	MakeMoveTask(void);
	virtual	LAction *	MakeLinkTask(void);
	virtual	LAction *	MakeOSpecTask(void);

				//	New data members
	LTextEngine			*mTextEngine;
	LTextModel			*mTextModel;
	LTextSelection		*mTextSelection;	//	just a casted version of mSelection
	
	static	const Int32	kNormalCharBuffMaxSize;
	static	Int32		sNormalCharBuffSize;
	static	char		sNormalCharBuff[];
	
	Boolean				mHasBufferedKeyEvent;
	EventRecord			mBufferedKeyEvent;
	
	Int32				mUpDownHorzPixel;
	
	Boolean				mKeyEvent,
						mKeyHandled;
};
