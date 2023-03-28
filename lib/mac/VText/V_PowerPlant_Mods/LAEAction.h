//	===========================================================================
//	LAEAction.h				©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include	<LAction.h>
#include	<UAppleEventsMgr.h>
#include	<AERegistry.h>

class	LSelection;
class	LRecalcAccumulator;

class	LAEAction :	public LAction
{
public:
				//	Maintenance / initialization
						LAEAction(
							ResIDT	inStringResID = STRx_RedoEdit,
							Int16	inStringIndex = str_RedoUndo,
							Boolean	inAlreadyDone = false);
	virtual				~LAEAction();
	virtual void		SetRecordOnlyFinalState(Boolean inRecordOnlyFinalState);
	virtual void		Finalize(void);

				//	AE loading...
	virtual void		SetRedoAE(AEEventClass inEventClass, AEEventID inEventID);
	virtual void		SetRedoAE(const AppleEvent &inAppleEvent);
	virtual void		RedoAEAdd(AEKeyword theAEKeyword, const AEDesc &inDesc);

	virtual void		SetUndoAE(AEEventClass inEventClass, AEEventID inEventID, Boolean inFeedUndoWithRedoReply = false);
	virtual void		SetUndoAE(const AppleEvent &inAppleEvent, Boolean inFeedUndoWithRedoReply = false);
	virtual void		UndoAEAdd(AEKeyword theAEKeyword, const AEDesc &inDesc);
	virtual void		UndoAESetKeyFed(DescType inKey, DescType inFromReplyKey = keyAEResult);

	virtual void		SetPostUndoAE(const AppleEvent &inAppleEvent);
	virtual void		PostUndoAESetKeyFed(DescType inKey, DescType inFromReplyKey = keyAEResult);

				//	Selection things...
	virtual LSelection *
						GetSelection(void);
	virtual void		SetSelection(
							LSelection	*inSelection,
							DescType	inRedoSelectionModifier = kAEReplace,
							DescType	inUndoSelectionModifier = kAEReplace);
	virtual Boolean		GetPreRecordSelection(void);
	virtual void		SetPreRecordSelection(Boolean inPreRecordSelection);
	
				//	Updating
	virtual LRecalcAccumulator *
						GetRecalcAccumulator(void);
	virtual void		SetRecalcAccumulator(LRecalcAccumulator *inAccumulator);

				//	Implementation
	virtual void		Redo(void);
	virtual	void		Undo(void);
	virtual void		RedoSelf(void);
	virtual void		UndoSelf(void);
	virtual Boolean		CanRedo(void) const;
	virtual Boolean		CanUndo(void) const;

	virtual	void		GetReplyDesc(AEDesc *outDesc);

protected:
				//	Implementation
	virtual	void		SendAppleEvent(AppleEvent &inAppleEvent, AESendMode inSendModifiers = 0);
	virtual void		FixSelectionWReply(DescType inSelectionModifier);
	
				//	Data members
	StAEDescriptor		mRedoEvent;				//	AE that performs or redoes the action.
	StAEDescriptor		mUndoEvent;				//	AE that undoes mRedoEvent.
	StAEDescriptor		mPostUndoEvent;			//	AE that executes of mUndoEvent

	StAEDescriptor		mReply;					//	Reply from last AE.
	DescType			mUndoAEKeyFed;			//	The undo key parameter being fed by the redo AE.
												//		(typeNull if none).
	DescType			mFromReplyKey;			//	Key from reply fed into Undo AE.
	DescType			mPostUndoAEKeyFed;
	DescType			mPostFromReplyKey;
	Boolean				mRecordOnlyFinalState;

	LRecalcAccumulator	*mAccumulator;			//	for optional "cool" updating
	StAEDescriptor		mTellTarget;
	LSelection			*mSelection;
	Boolean				mPreRecordSelection;
	DescType			mRedoSelectionModifier;	//	Ie. to place selection kAEAfter reply.
	DescType			mUndoSelectionModifier;
	StAEDescriptor		mOriginalSelection;
};


class	LAEMoveAction :	public LAEAction
{
public:
				//	Maintenance / initialization / implementation
						LAEMoveAction(
							ResIDT	inStringResID = STRx_RedoEdit,
							Int16	inStringIndex = str_RedoUndo,
							Boolean	inAlreadyDone = false);
	virtual				~LAEMoveAction();
	virtual void		RedoSelf(void);
};