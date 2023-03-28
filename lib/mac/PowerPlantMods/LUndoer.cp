// ===========================================================================
//	LUndoer.cp						©1995 Metrowerks Inc. All rights reserved.
// ===========================================================================
//
//	Attachment for implementing Undo
//
//	An LUndoer object can be attached to a LCommander. The Undoer will
//	store the last Action object posted to the Commander, enable and
//	set the text for the "undo" menu item, and respond to the "undo"
//	command by telling the Action object to Undo/Redo itself.

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include <LUndoer.h>
#include <LAction.h>
#include <PP_Messages.h>
#include <PP_Resources.h>
#include <Resources.h>


// ---------------------------------------------------------------------------
//		¥ LUndoer
// ---------------------------------------------------------------------------
//	Default Constructor

LUndoer::LUndoer()
{
	mAction = nil;
}


// ---------------------------------------------------------------------------
//		¥ ~LUndoer
// ---------------------------------------------------------------------------
//	Destructor

LUndoer::~LUndoer()
{
	PostAction(nil);					// Will delete current Action
}


// ---------------------------------------------------------------------------
//		¥ ExecuteSelf
// ---------------------------------------------------------------------------
//	Execute an Undoer Attachment

void
LUndoer::ExecuteSelf(
	MessageT	inMessage,
	void		*ioParam)
{
	switch (inMessage) {

		case msg_PostAction:		// New Action to perform
			PostAction((LAction*) ioParam);
			SetExecuteHost(false);
			break;
		
		case msg_CommandStatus: {	// Enable and set text for "undo"
									//   menu item
			SCommandStatus *status = (SCommandStatus*) ioParam;
		
			if (status->command == cmd_Undo) {
				FindUndoStatus(status);
				SetExecuteHost(false);
			} else {
				SetExecuteHost(true);
			}
			break;
		}
		
		case cmd_Undo:				// Undo/Redo the Action
			ToggleAction();
			SetExecuteHost(false);
			break;

		default:
			SetExecuteHost(true);
			break;
	}
}


// ---------------------------------------------------------------------------
//		¥ PostAction
// ---------------------------------------------------------------------------
//	A new Action has been posted to the host Commander

void
LUndoer::PostAction(
	LAction	*inAction)
{
	ExceptionCode	actionFailure = noErr;
	
	if ((inAction == nil)  ||
		((inAction != nil) && inAction->IsPostable())) {

										// Save old Action
		LAction	*oldAction = mAction;
		mAction = inAction;
		
										// Force update of menu items status
		LCommander::SetUpdateCommandStatus(true);
		
		Boolean	deleteOldAction = false;

		if (oldAction != nil) {			// Finalize the old action
	
			try {		
				oldAction->Finalize();
			}
			
			catch(ExceptionCode inErr) { }
			
			deleteOldAction = true;
		}
		
		if (mAction != nil) {			// Do the new action
			try {
				mAction->Redo();
			}
			
			catch(ExceptionCode inErr) {
			
				// Failed to "Do" the newly posted Action. Finalize
				// and delete the new Action, then check to see if the
				// old Action can still be undone/redone. If so, keep
				// the old Action as the last undoable Action.
				
				actionFailure = inErr;

				try {
					mAction->Finalize();
				}
				catch(ExceptionCode inErr) { }
				
				delete mAction;
				mAction = NULL;
				
				if ( (oldAction != nil) &&
					 (oldAction->CanUndo() || oldAction->CanRedo()) ) {
					 
					mAction = oldAction;
					deleteOldAction = false;
				}
			}
		}
		
		if (deleteOldAction) {
			delete oldAction;
		}
		
	} else {							// A non-postable Action
		try {
			inAction->Redo();
		}
		catch(ExceptionCode inErr) {
			actionFailure = inErr;
		}
		
		try {
			inAction->Finalize();
		}
		catch(ExceptionCode inErr) { }
			
		delete inAction;
	}
	
	if (actionFailure != noErr) {
		Throw_(actionFailure);
	}
}


// ---------------------------------------------------------------------------
//		¥ ToggleAction
// ---------------------------------------------------------------------------
//	Undo/Redo the Action associated with this Undoer

void
LUndoer::ToggleAction()
{
	if (mAction != nil) {			// Shouldn't be nil, but let's be safe
		if (mAction->CanUndo()) {
			mAction->Undo();
		} else if (mAction->CanRedo()) {
			mAction->Redo();
		}
	}
}


// ---------------------------------------------------------------------------
//		¥ FindUndoStatus
// ---------------------------------------------------------------------------
//	Enable/disable and set the text for the "undo" menu item

void
LUndoer::FindUndoStatus(
	SCommandStatus	*ioStatus)
{
	*ioStatus->enabled = false;
	
	if (mAction != nil) {
		Str255	dummyString;
		
		if (mAction->CanRedo()) {			// Set "Redo" text
			*ioStatus->enabled = true;
			mAction->GetDescription(ioStatus->name, dummyString);
			
		} else if (mAction->CanUndo()) {	// Set "Undo" text
			*ioStatus->enabled = true;
			mAction->GetDescription(dummyString, ioStatus->name);
		}
	}

	if (!(*ioStatus->enabled)) {			// Set text to "Can't Undo"
		::GetIndString(ioStatus->name, STRx_UndoEdit, str_CantRedoUndo);
	}
}
