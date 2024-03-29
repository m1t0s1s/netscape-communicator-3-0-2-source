//	===========================================================================
//	LDataDragTask.cp				�1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LDataDragTask.h"
#include	<LDataDragEventHandler.h>
#include	<LDragTube.h>
#include	<LSelectableItem.h>
#include	<LSelectHandlerView.h>
#include	<LWindow.h>
#include	<UDesktop.h>

#include	<LAEAction.h>
#include	<UAppleEventsMgr.h>
#include	<UMemoryMgr.h>


#ifndef		__ERRORS__
#include	<Errors.h>
#endif

#ifndef		__AEOBJECTS__
#include	<AEOBJECTS.h>
#endif

/*
	LDataDragTask does the mapping and coordination of data transfer for a drag
	between:
		some other drag originator	-->	an LDataDragEventHandler
		an LDataDragEventHandler	-->	another LDataDragEventHandler
		an LDataDragEventHandler	-->	some other drag receiver
		
	LDataDragTask contains all the instance/state information for the single
	drag operation.
	
	LDataDragTask replaces much of the complexity and functionality found in
	the pre 9409 LDataDragEventHandler -- that code in that version of the
	handler was becoming unwieldy in trying to find the location of the
	correct state information.
*/

LDataDragTask	*LDataDragTask::sActiveTask = NULL;

LDataDragTask::LDataDragTask(LDataDragEventHandler *inHandler, Boolean inHandlerIsSource)
{
	OSErr	err;
	
	Assert_(sActiveTask == NULL);
	
	if (inHandlerIsSource) {
		mSourceHandler = inHandler;
		mReceiveHandler = NULL;
		
		err = NewDrag(&mDragRef);
		ThrowIfOSErr_(err);
	} else {
		mSourceHandler = NULL;
		mReceiveHandler = inHandler;
		mDragRef = NULL;
	}
	mReceiver = NULL;
	
	mDataTransferred = false;
	mNotified = false;
	mDragTube = NULL;
	mDragAction = NULL;
	
	mDragSemantic = dataDrag_Unknown;
	mDragFunction = dataDragFunc_Unknown;
	
	sActiveTask = this;
}

LDataDragTask::~LDataDragTask()
{
	OSErr	err;
	
	sActiveTask = NULL;
	
	if (mSourceHandler != NULL) {
		if (mDragRef != NULL) {
			err = DisposeDrag(mDragRef);
			ThrowIfOSErr_(err);
			mDragRef = NULL;
		}
		if (mDragTube != NULL) {
			delete mDragTube;
			mDragTube = NULL;
		}
	}
	SetReceiver(NULL);
}

void	LDataDragTask::SetReceiver(LSelectableItem *inReceiver)
{
	ReplaceSharable(mReceiver, inReceiver, this);
}

void	LDataDragTask::CheckTube(void)
{
	Assert_(mDragRef);
	
	if (mDragTube == NULL) {
		mDragTube = new LDragTube(mDragRef);
	}
}

LDataDragEventHandler *	LDataDragTask::GetReceiveHandler(void) const
{
	return mReceiveHandler;
}

void	LDataDragTask::NoteReceiveHandler(LDataDragEventHandler *inHandler)
{
	if (!mNotified)
		mReceiveHandler = inHandler;
}

void	LDataDragTask::NoteTransfer(void)
{
	OSErr			err;
	StAEDescriptor	dropLoc;
	Boolean			receivable = false;
	
	if (mNotified)
		return;
	
	mNotified = true;

	mDragFunction = dataDragFunc_Unknown;
	mDragSemantic = dataDrag_Unknown;
	
	//	Check receivability
	if (mReceiveHandler) {
		if (mReceiver && mDragTube) {
			receivable = (typeNull != mReceiver->PickFlavorFrom(mDragTube));
		}
		if (!receivable) {
			//	We really shouldn't be here...
			//	?
			return;
		}

		//	Drop location -- for sender's pleasure
		mReceiver->MakeSpecifier(dropLoc.mDesc);
		if (dropLoc.mDesc.descriptorType == typeInsertionLoc) {
			err = SetDropLocation(mDragRef, &dropLoc.mDesc);
		} else {
			StAEDescriptor	temp;
			UAEDesc::MakeInsertionLoc(dropLoc.mDesc, kAEReplace, &temp.mDesc);
			err = SetDropLocation(mDragRef, &temp.mDesc);
		}
		ThrowIfOSErr_(err);
	}
	
	//	Codify type of drag function...
	if (!mReceiveHandler && !mSourceHandler) {
		Assert_(false);	//	Impossible!
	}
	if (!mReceiveHandler && mSourceHandler) {
		err = GetDropLocation(mDragRef, &dropLoc.mDesc);
		if (err == noErr)
			mDragFunction = dataDragFunc_OSpecDrop;
		else
			mDragFunction = dataDragFunc_Drop;
	}
	if (mReceiveHandler && !mSourceHandler) {
		mDragFunction = dataDragFunc_Receive;
	}
	if (mReceiveHandler && mSourceHandler) {
		mDragFunction = dataDragFunc_Drag;
	}
	
	//	Delay or do the transfer...
	switch (mDragFunction) {

		case dataDragFunc_Receive:
			DoTransfer();
			break;

		case dataDragFunc_OSpecDrop:
			/*
				To bad we can't tell a smart, drop location aware
				receiver not to receive the data.  If that was the case,
				it would simplify things and we could package up a nice
				apple event for the receiver.
			*/
			//	Falls through for now...

		case dataDragFunc_Drop:
			DoTransfer();
			break;
			
		case dataDragFunc_Drag:
			/*
				DoFactoredDataDrag();
	
				Not!
	
				We are the sender & receiver and have more control.
				Don't do anything now -- the transfer will take place later
				outside of the TrackDrag function.
								
				As an added bonus of this technique, the response to the
				drag can be debugged with the source level debugger!
			*/
			/*
				What an ugly pain...
	
				If a flavor hasn't been "gotten" before a TrackDrag ends,
				it is forgotten.
				
				So, let's make sure the object specifier needed later
				has been loaded.
			*/
			mDragTube->GetFlavorSize(typeObjectSpecifier);
			break;

		default:
			Throw_(paramErr);	//	Drag must be codified by now
			break;
	}
}

void	LDataDragTask::DoTransfer(void)
{
	if (mDataTransferred)
		return;		//	Must have been done during NoteTransfer.

	//	Uglyness to the max.  This really should go somewhere else --
	//	maybe it is a complication with cross application drags & LDragAndDrap?
	if (mDragFunction == dataDragFunc_Receive) {
		if (mReceiveHandler && mDragRef) {
			if (mReceiver)
				mReceiver->UnDrawSelfReceiver();
			HideDragHilite(mDragRef);
		}
	}
	
	//	Make the drag action...
	mDragAction = MakeDragAction();
	
	//	Do the transfer...
	switch (mDragFunction) {

		case dataDragFunc_Receive:
			mReceiveHandler->PostAction(mDragAction);
			break;

		case dataDragFunc_OSpecDrop:
			//	Fall through...

		case dataDragFunc_Drop:
			//	Nothing to post!  Responsibility of unknown receiver.
			break;
			
		case dataDragFunc_Drag:
			mReceiveHandler->PostAction(mDragAction);
			break;

		case dataDragFunc_Unknown:
			Throw_(userCanceledErr);	//	User must have stopped
										//	before drag was determined.
			break;
			
		default:
			Throw_(paramErr);	//	Drag must be codified by now
			break;
	}

	//	Switch target if mouse up in same window
	//	
	//	From a user i/f standpoint, in the case of dragging
	//	to an active window of multiple select handler views,
	//	it might be more natural if the receiving select handler view
	//	automatically becomes the target...
	if (mReceiveHandler) {
		LSelectHandlerView	*view = (LSelectHandlerView *)mReceiveHandler->GetHandlerView();
		
		if (view) {
//			Assert_(member(view, LSelectHandlerView));
			view->ShouldBeTarget();
		}
	}
	
	mDataTransferred = true;
}

LAction	*	LDataDragTask::MakeDragAction(void)
{
	LAction	*action = NULL;
	
	switch(mDragFunction) {

		case dataDragFunc_Receive:
			//	can only be a create
			Assert_(mReceiveHandler);
			action = mReceiveHandler->MakeCreateTask();
			break;
		
		case dataDragFunc_Drop:
		case dataDragFunc_OSpecDrop:
			//	We'll never know.  Also, action = NULL should not be posted.
			break;
			
		case dataDragFunc_Drag:
			Assert_(mReceiveHandler);
			mDragSemantic = mReceiveHandler->FindDragSemantic();
			switch (mDragSemantic) {
				case dataDrag_Copy:
					action = mReceiveHandler->MakeCopyTask();
					break;
				case dataDrag_Move:
					action = mReceiveHandler->MakeMoveTask();
					break;
				case dataDrag_OSpec:
					action = mReceiveHandler->MakeOSpecTask();
					break;
				case dataDrag_Link:
					action = mReceiveHandler->MakeLinkTask();
					break;
			}
			break;
	}
	return action;
}
