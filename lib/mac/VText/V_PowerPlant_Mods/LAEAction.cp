// ===========================================================================
//	LAEAction.cp			©1994 Metrowerks Inc. All rights reserved.
// ===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	<PP_Prefix.h>
#include	"LAEAction.h"
#include	<LSelection.h>
#include	<UAppleEventsMgr.h>
#include	<AERegistry.h>
#include	<LModelObject.h>
#include	<LRecalcAccumulator.h>
#include	<StClasses.h>
#include	<UAEGizmos.h>

//	===========================================================================
//	Maintenance / initialization:

LAEAction::LAEAction(
	ResIDT		inStringResID,
	Int16		inStringIndex,
	Boolean		inAlreadyDone
)	:	LAction(inStringResID, inStringIndex, inAlreadyDone)
{
	mRecordOnlyFinalState = false;
	mUndoAEKeyFed = typeNull;
	mPostUndoAEKeyFed = typeNull;
	mFromReplyKey = typeNull;
	
	mSelection = NULL;
	mRedoSelectionModifier = typeNull;
	mUndoSelectionModifier = typeNull;
	mPreRecordSelection = true;
	
	mAccumulator = NULL;
	
	//	Tell target focusing...
	LModelObject	*target = LModelObject::GetTellTarget();
	if (target) {
		StTempTellTarget	tell(NULL);	//	Make complete ospec
		target->MakeSpecifier(mTellTarget.mDesc);
	}
}

LAEAction::~LAEAction()
{
	ReplaceSharable(mSelection, (LSelection *)NULL, this);
}

void	LAEAction::Finalize(void)
{
	if (mRecordOnlyFinalState && mIsDone) {

		if (mSelection && mPreRecordSelection && (mOriginalSelection.mDesc.descriptorType != typeNull))
			mSelection->DoAESelection(mOriginalSelection, false);

		SendAppleEvent(mRedoEvent.mDesc, kAEDontExecute);
	}
	
	//	Once an event has been finalized, subsequent redo's & undo's must be
	//	recorded.
	SetRecordOnlyFinalState(false);
	
	inherited::Finalize();
}

void	LAEAction::SetRecordOnlyFinalState(Boolean inRecordOnlyFinalState)
{
	mRecordOnlyFinalState = inRecordOnlyFinalState;
}

void	LAEAction::SetRedoAE(AEEventClass inEventClass, AEEventID inEventID)
/*
	Don't use this routine if the associated event has parameters.
	Instead, make an AppleEvent and use SetRedo/UndoAE(AppleEvent).
*/
{
	UAppleEventsMgr::MakeAppleEvent(inEventClass, inEventID, mRedoEvent.mDesc);
}

//	===========================================================================
//	AE loading:

void	LAEAction::SetRedoAE(const AppleEvent &inAppleEvent)
{
	OSErr	err;
	
	err = AEDuplicateDesc(&inAppleEvent, &mRedoEvent.mDesc);
	ThrowIfOSErr_(err);
}

void	LAEAction::SetUndoAE(
	AEEventClass		inEventClass,
	AEEventID			inEventID,
	Boolean				inFeedUndoWithRedoReply)
/*
	Don't use this routine if the associated event has parameters.
	Instead, make an AppleEvent and use SetRedo/UndoAE(AppleEvent).
	
	Avoid using explicitly using the optional inFeedUndoWithRedoReply.
	Instead, call UndoAESetKeyFed after all SetUndoAE's have been called.
*/
{
	UAppleEventsMgr::MakeAppleEvent(inEventClass, inEventID, mUndoEvent.mDesc);
	
	if (inFeedUndoWithRedoReply)
		UndoAESetKeyFed(keyDirectObject);
}

void	LAEAction::SetUndoAE(
	const AppleEvent	&inAppleEvent,
	Boolean				inFeedUndoWithRedoReply)
/*	
	Avoid using explicitly using the optional inFeedUndoWithRedoReply.
	Instead, call UndoAESetKeyFed after all SetUndoAE's have been called.
*/
{
	OSErr	err;

	err = AEDuplicateDesc(&inAppleEvent, &mUndoEvent.mDesc);
	ThrowIfOSErr_(err);
	
	if (inFeedUndoWithRedoReply)
		UndoAESetKeyFed(keyDirectObject);
}

void	LAEAction::SetPostUndoAE(const AppleEvent &inAppleEvent)
{
	OSErr	err;

	err = AEDuplicateDesc(&inAppleEvent, &mPostUndoEvent.mDesc);
	ThrowIfOSErr_(err);
}

void	LAEAction::UndoAESetKeyFed(DescType inKey, DescType inFromReplyKey)
/*
	Used to set the "key parameter receiver" for the undo event
	from the reply of the redo event.
	
	Avoid calling this method with typeNull because that is the natural
	constructed state of an LAEAction.
*/
{
	mUndoAEKeyFed = inKey;
	mFromReplyKey = inFromReplyKey;
}

void	LAEAction::PostUndoAESetKeyFed(DescType inKey, DescType inFromReplyKey)
{
	mPostUndoAEKeyFed = inKey;
	mPostFromReplyKey = inFromReplyKey;
}

void	LAEAction::RedoAEAdd(AEKeyword theAEKeyword, const AEDesc &inDesc)
{
	OSErr	err;
	
	err = AEPutParamDesc(&mRedoEvent.mDesc, theAEKeyword, &inDesc);
	ThrowIfOSErr_(err);
}

void	LAEAction::UndoAEAdd(AEKeyword theAEKeyword, const AEDesc &inDesc)
{
	OSErr	err;
	
	err = AEPutParamDesc(&mUndoEvent.mDesc, theAEKeyword, &inDesc);
	ThrowIfOSErr_(err);
}

//	===========================================================================
//	Selection things:

void	LAEAction::SetSelection(
	LSelection	*inSelection,
	DescType	inRedoSelectionModifier,
	DescType	inUndoSelectionModifier)
{
	ReplaceSharable(mSelection, inSelection, this);
	
	mRedoSelectionModifier = inRedoSelectionModifier;
	mUndoSelectionModifier = inUndoSelectionModifier;
}

LSelection *	LAEAction::GetSelection(void)
{
	return mSelection;
}

void	LAEAction::SetPreRecordSelection(Boolean inPreRecordSelection)
{
	mPreRecordSelection = inPreRecordSelection;
}

Boolean	LAEAction::GetPreRecordSelection(void)
{
	return mPreRecordSelection;
}
	
//	===========================================================================
//	RecalcAccumulator:
//		req'd only for "cool" updating

LRecalcAccumulator *	LAEAction::GetRecalcAccumulator(void)
{
	return mAccumulator;
}

void	LAEAction::SetRecalcAccumulator(LRecalcAccumulator *inAccumulator)
{
	mAccumulator = inAccumulator;
}

//	===========================================================================
//	Implementation:

void	LAEAction::Redo(void)
{
	if (CanRedo()) {
		StRecalculator	update(mAccumulator);
		LAESubDesc		targetSD(mTellTarget);
		LModelObject	*oldTarget = LModelObject::GetTellTarget(),
						*target = NULL;
						
		if (targetSD.GetType() != typeNull) {
			StTempTellTarget	doFrom(NULL);
			
			target = targetSD.ToModelObjectOptional();
		}
		
		Try_ {
			LModelObject::DoAESwitchTellTarget(target);
			
			if (mSelection) {
				StAEDescriptor	temp;

				if (mOriginalSelection.mDesc.descriptorType == typeNull) {
				
					//	must be first time -- record selection in case there's repeated redo/undos
					mSelection->GetAEValue(temp.mDesc, mOriginalSelection.mDesc);
				} else {
				
					//	Use the previous selection.
					mSelection->SetAEValue(mOriginalSelection.mDesc, temp.mDesc);
				}
				if (mPreRecordSelection)
					mSelection->RecordPresentSelection();
			}

			RedoSelf();
			
			mIsDone = true;
			
			LModelObject::DoAESwitchTellTarget(oldTarget);
		} Catch_(inErr) {
			LModelObject::DoAESwitchTellTarget(oldTarget);
			Throw_(inErr);
		} EndCatch_;
	} else {
		mIsDone = true;
	}
}

void	LAEAction::Undo(void)
{
	if (CanUndo()) {
		StRecalculator	update(mAccumulator);
		StAEDescriptor	temp;
		LAESubDesc		targetSD(mTellTarget);
		LModelObject	*oldTarget = LModelObject::GetTellTarget(),
						*target = NULL;
						
		if (targetSD.GetType() != typeNull) {
			StTempTellTarget	doFrom(NULL);
			
			target = targetSD.ToModelObjectOptional();
		}
		
		Try_ {
			LModelObject::DoAESwitchTellTarget(target);
			
			UndoSelf();

			mIsDone = false;

			LModelObject::DoAESwitchTellTarget(oldTarget);
		} Catch_(inErr) {
			LModelObject::DoAESwitchTellTarget(oldTarget);
			Throw_(inErr);
		} EndCatch_;
	} else {
		mIsDone = false;
	}
}

void	LAEAction::RedoSelf(void)
{
	Assert_(CanRedo());
	
	//	Do the event...
	SendAppleEvent(mRedoEvent.mDesc, mRecordOnlyFinalState ? kAEDontRecord : 0);

	//	Record information from reply for undo
	if ((mUndoEvent.mDesc.descriptorType != typeNull) && (mUndoAEKeyFed != typeNull)) {
		StAEDescriptor	aeResult;
	
		//	keyDirectObject -- the result from the original operation
		Assert_(mFromReplyKey != typeNull);
		aeResult.GetParamDesc(mReply.mDesc, mFromReplyKey, typeWildCard);
		UndoAEAdd(mUndoAEKeyFed, aeResult.mDesc);
		
		mUndoAEKeyFed = typeNull;	//	Don't add it twice
	}
	
	//	Update selection
	if (mSelection)
		FixSelectionWReply(mRedoSelectionModifier);
}

void	LAEAction::UndoSelf(void)
{
	OSErr	err;
	
	Assert_(CanUndo());
	
	//	Do the event
	SendAppleEvent(mUndoEvent.mDesc, mRecordOnlyFinalState ? kAEDontRecord : 0);
	
	//	Update selection
	if (mSelection)
		FixSelectionWReply(mUndoSelectionModifier);

	//	Post undo AE...
	if (mPostUndoAEKeyFed != typeNull) {
		StAEDescriptor	aeResult;
	
		//	mPostFromReplyKey -- the result from the undo operation
		Assert_(mPostFromReplyKey != typeNull);
		aeResult.GetParamDesc(mReply.mDesc, mPostFromReplyKey, typeWildCard);

		err = AEPutParamDesc(&mPostUndoEvent.mDesc, mPostFromReplyKey, &aeResult.mDesc);
		ThrowIfOSErr_(err);
		
		mPostUndoAEKeyFed = typeNull;	//	Don't add it twice
	}
	if (mPostUndoEvent.mDesc.descriptorType != typeNull)
		SendAppleEvent(mPostUndoEvent.mDesc, mRecordOnlyFinalState ? kAEDontRecord : 0);
}

Boolean	LAEAction::CanRedo(void) const
{
	return	inherited::CanRedo() &&	(mRedoEvent.mDesc.descriptorType != typeNull);
}


Boolean	LAEAction::CanUndo(void) const
{
	return	inherited::CanUndo() && (mUndoEvent.mDesc.descriptorType != typeNull);
}

void	LAEAction::GetReplyDesc(AEDesc *outDesc)
{
	OSErr	err;
	
	err = AEDuplicateDesc(&mReply.mDesc, outDesc);
	ThrowIfOSErr_(err);
}

void	LAEAction::SendAppleEvent(AppleEvent &inAppleEvent, AESendMode inSendModifiers)
{
	StAEDescriptor	aeResult;
	StAEDescriptor	aeHole;
	OSErr			err;
	
	inSendModifiers |= kAEWaitReply;

	err = AEDisposeDesc(&mReply.mDesc);
	ThrowIfOSErr_(err);
	
	Int32	version;
	err = ::AEManagerInfo(keyAEVersion, &version);
	Boolean	dontExecuteWorks = (err == noErr)
				?	version >= 0x01018000
				:	false;
	
	if ((inSendModifiers & kAEDontExecute) && !dontExecuteWorks)
		return;
		
	err = AESend(&inAppleEvent, &mReply.mDesc, inSendModifiers, kAENormalPriority,
						kAEDefaultTimeout, nil, nil);
	ThrowIfOSErr_(err);
}

void	LAEAction::FixSelectionWReply(DescType inSelectionModifier)
{
	StAEDescriptor		aeResult;

	if (mSelection && inSelectionModifier != typeNull) {
		aeResult.GetOptionalParamDesc(mReply.mDesc, keyAEResult, typeWildCard);
		mSelection->DoAESelectAsResult(aeResult, inSelectionModifier);
	}
}

//	===========================================================================
//	LAEMoveAction:

LAEMoveAction::LAEMoveAction(
	ResIDT		inStringResID,
	Int16		inStringIndex,
	Boolean		inAlreadyDone
)	:	LAEAction(inStringResID, inStringIndex, inAlreadyDone)
{
}

LAEMoveAction::~LAEMoveAction()
{
}

void	LAEMoveAction::RedoSelf(void)
{
	SendAppleEvent(mRedoEvent.mDesc, mRecordOnlyFinalState ? kAEDontRecord : 0);

	if (mUndoAEKeyFed != typeNull) {
		StAEDescriptor	aeResult,
						aeHole;
	
		//	keyDirectObject -- the result from the original move
		Assert_(mFromReplyKey != typeNull);
		aeResult.GetParamDesc(mReply.mDesc, mFromReplyKey, typeWildCard);
		UndoAEAdd(keyDirectObject, aeResult.mDesc);
		
		//	keyAEInsertHere -- the resulting hole from the original move
		aeHole.GetParamDesc(mReply.mDesc, keyAEInsertHere, typeWildCard);
		UndoAEAdd(keyAEInsertHere, aeHole.mDesc);
		
		mUndoAEKeyFed = typeNull;	//	Don't add it twice
	}
	
	//	Update selection
	if (mSelection)
		FixSelectionWReply(mRedoSelectionModifier);
}

