//	===========================================================================
//	LDataDragEventHandler.AEOM.cp	й1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LDataDragEventHandler.h"
#include	<LSelection.h>
#include	<LSelectableItem.h>
#include	<LAEAction.h>
#include	<UAppleEventsMgr.h>
#include	<UExtractFromAEDesc.h>
#include	<UMemoryMgr.h>
#include	<LDragTube.h>
#include	<PP_Resources.h>
#include	<UAEGizmos.h>

#ifndef		__ERRORS__
#include	<Errors.h>
#endif

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#ifndef		__AEOBJECTS__
#include	<AEObjects.h>
#endif

#define	DragTask	LDataDragTask::sActiveTask

LAction *	LDataDragEventHandler::MakeCreateTask(void)
{
	LAEAction			*action = NULL;
	StAEDescriptor		doerAppleEvent,
						undoerAppleEvent,
						postUndoerAE;
	FlavorType			desiredFlavor;

	if (DragTask == NULL)
		Throw_(paramErr);
	if ((DragTask->mDragTube == NULL) || (DragTask->mReceiver == NULL))
		Throw_(paramErr);

	//	ее	Make the "semantic action"
	action = new LAEAction(STRx_RedoDrag, str_DragDrop);
	action->SetSelection(mSelection);

	//	ее	Make doer event
	{
		LAEStream		ae(kAECoreSuite, kAECreateElement);
		
			//	keyAEObjectClass
			ae.WriteKey(keyAEObjectClass);
			ae.WriteTypeDesc(DragTask->mReceiver->GetModelKind());
		
			//	keyAEData
			ae.WriteKey(keyAEData);
			desiredFlavor = DragTask->mReceiver->PickFlavorFrom(DragTask->mDragTube);
			if (desiredFlavor == typeWildCard) {
				DragTask->mDragTube->WriteDesc(&ae);
			} else {
				StAEDescriptor	data;

				DragTask->mDragTube->GetFlavorAsDesc(desiredFlavor, &data.mDesc);

				ae.WriteDesc(data.mDesc);
			}

			//	keyAEPropData
			//		is optional
	
			//	keyAEInsertHere
			ae.WriteKey(keyAEInsertHere);
			StAEDescriptor	destination;
			DragTask->mReceiver->MakeSpecifier(destination.mDesc);
			if (destination.mDesc.descriptorType == typeInsertionLoc) {
				ae.WriteDesc(destination.mDesc);
			} else {
				ae.OpenRecord(typeInsertionLoc);
				
				ae.WriteKey(keyAEObject);
				ae.WriteDesc(destination.mDesc);
				
				ae.WriteKey(keyAEPosition);
				ae.WriteEnumDesc(kAEReplace);
				
				ae.CloseRecord();
			}
	
		ae.Close(&doerAppleEvent.mDesc);
		
		action->SetRedoAE(doerAppleEvent.mDesc);
	}
	
	//	ее	Make undoer event
	UAppleEventsMgr::MakeAppleEvent(kAECoreSuite, kAEDelete, undoerAppleEvent.mDesc);
	action->UndoAESetKeyFed(keyDirectObject);
	action->SetUndoAE(undoerAppleEvent.mDesc);

	//	ее	Make post undoer event
	{
		LAESubDesc	doer(doerAppleEvent.mDesc);
		DescType	position = doer.KeyedItem(keyAEInsertHere).KeyedItem(keyAEPosition).ToEnum();
		
		if (position == kAEReplace) {			

			LAEStream	ae(kAECoreSuite, kAECreateElement);
				
				//	keyAEObjectClass
				ae.WriteKey(keyAEObjectClass);
				ae.WriteTypeDesc(DragTask->mReceiver->GetModelKind());
			
				//	keyAEData
				//		optional -- uses keyAEPropData
				
				//	keyAEPropData
				ae.WriteKey(keyAEPropData);
				StAEDescriptor	oldDestProps;
				DragTask->mReceiver->GetImportantAEProperties(oldDestProps.mDesc);
				ae.WriteDesc(oldDestProps.mDesc);
		
			ae.Close(&postUndoerAE.mDesc);
			
			action->SetPostUndoAE(postUndoerAE.mDesc);
	
			//	keyAEInsertHere
			action->PostUndoAESetKeyFed(keyAEInsertHere, keyAEInsertHere);
		}
	}

	return action;
}

LAction *	LDataDragEventHandler::MakeCopyTask(void)
{
	LAEAction		*action = NULL;
	StAEDescriptor	doerAppleEvent,
					undoerAppleEvent,
					postUndoerAE,
					source,
					target;
	OSErr			err;

	if (DragTask == NULL)
		Throw_(paramErr);
	if (DragTask->mDragTube == NULL)
		Throw_(paramErr);
	if (DragTask->mReceiver == NULL)
		Throw_(paramErr);
	if (DragTask->mDragRef == NULL)
		Throw_(paramErr);

	//	source
	ThrowIfNULL_(DragTask->mSourceHandler);
	ThrowIfNULL_(DragTask->mSourceHandler->GetSelection());
	DragTask->mSourceHandler->GetSelection()->MakeSpecifier(source.mDesc);

	//	target
	err = GetDropLocation(DragTask->mDragRef, &target.mDesc);
	ThrowIfOSErr_(err);
	Assert_(target.mDesc.descriptorType != typeNull);

	//	ее	Make the "semantic action"
	action = new LAEAction(STRx_RedoDrag, str_DragCopy);
	action->SetSelection(mSelection);
	
	//	ее	Make doer event
	{
		LAEStream	ae(kAECoreSuite, kAEClone);
		
			//	keyDirectObject
			ae.WriteKey(keyDirectObject);
			ae.WriteDesc(source);
			
			//	keyAEInsertHere
			ae.WriteKey(keyAEInsertHere);
			if (target.mDesc.descriptorType == typeInsertionLoc) {
				ae.WriteDesc(target.mDesc);
			} else {
				ae.OpenRecord(typeInsertionLoc);
				
				ae.WriteKey(keyAEObject);
				ae.WriteDesc(target.mDesc);
				
				ae.WriteKey(keyAEPosition);
				ae.WriteEnumDesc(kAEReplace);
				
				ae.CloseRecord();
			}

		ae.Close(&doerAppleEvent.mDesc);

		action->SetRedoAE(doerAppleEvent.mDesc);
	}
	
	//	ее	Make undoer event
	UAppleEventsMgr::MakeAppleEvent(kAECoreSuite, kAEDelete, undoerAppleEvent.mDesc);
	action->UndoAESetKeyFed(keyDirectObject);
	action->SetUndoAE(undoerAppleEvent.mDesc);

	//	ее	Make post undoer event
	LAESubDesc	doer(doerAppleEvent.mDesc);
	DescType	position = doer.KeyedItem(keyAEInsertHere).KeyedItem(keyAEPosition).ToEnum();
	if (position == kAEReplace) {
		LAEStream	ae(kAECoreSuite, kAECreateElement);
			
			//	keyAEObjectClass
			ae.WriteKey(keyAEObjectClass);
			ae.WriteTypeDesc(DragTask->mReceiver->GetModelKind());

			//	keyAEData
			//		optional -- uses keyAEPropData
			
			//	keyAEPropData
			ae.WriteKey(keyAEPropData);
			StAEDescriptor	oldDestProps;
			DragTask->mReceiver->GetImportantAEProperties(oldDestProps.mDesc);
			ae.WriteDesc(oldDestProps);

		ae.Close(&postUndoerAE.mDesc);

		action->SetPostUndoAE(postUndoerAE.mDesc);

		//	keyAEInsertHere
		action->PostUndoAESetKeyFed(keyAEInsertHere, keyAEInsertHere);
	}
	
	return action;
}

LAction *	LDataDragEventHandler::MakeMoveTask(void)
{
	LAEAction		*action = NULL;
	StAEDescriptor	source,
					destination,
					destinationPreData,
					doerAppleEvent,
					undoerAppleEvent,
					postUndoerAE,
					temp,
					temp2;
	OSErr			err;

	if (DragTask == NULL)
		Throw_(paramErr);
	if (DragTask->mDragTube == NULL)
		Throw_(paramErr);
	if (DragTask->mReceiver == NULL)
		Throw_(paramErr);
	if (DragTask->mDragRef == NULL)
		Throw_(paramErr);

	//	ее	Make the "semantic action"
	action = new LAEMoveAction(STRx_RedoDrag, str_DragMove);
	action->SetSelection(mSelection);

	//	Get source descriptor
	ThrowIfNULL_(DragTask->mSourceHandler);
	ThrowIfNULL_(DragTask->mSourceHandler->GetSelection());
	DragTask->mSourceHandler->GetSelection()->MakeSpecifier(source.mDesc);

	//	Get destination descriptor -- the drop location
	err = GetDropLocation(DragTask->mDragRef, &destination.mDesc);
	ThrowIfOSErr_(err);
	Assert_(destination.mDesc.descriptorType != typeNull);
	
	//	ее	Make doer copy event
	{
		LAEStream	ae(kAECoreSuite, kAEMove);
		
			//	keyDirectObject
			ae.WriteKey(keyDirectObject);
			ae.WriteDesc(source);
		
			//	keyAEInsertHere
			ae.WriteKey(keyAEInsertHere);
			Assert_(destination.mDesc.descriptorType == typeInsertionLoc);
			ae.WriteDesc(destination);
		
		ae.Close(&doerAppleEvent.mDesc);
	
		action->SetRedoAE(doerAppleEvent.mDesc);
	}

	//	ее	Make undoer event
	UAppleEventsMgr::MakeAppleEvent(kAECoreSuite, kAEMove, undoerAppleEvent.mDesc);
	action->UndoAESetKeyFed(keyDirectObject);
	action->SetUndoAE(undoerAppleEvent.mDesc);

	//	ее	Make post undoer event
	LAESubDesc	doer(doerAppleEvent.mDesc);
	DescType	position = doer.KeyedItem(keyAEInsertHere).KeyedItem(keyAEPosition).ToEnum();
	if (position == kAEReplace) {
		LAEStream	ae(kAECoreSuite, kAECreateElement);

			//	keyAEObjectClass
			ae.WriteKey(keyAEObjectClass);
			ae.WriteTypeDesc(DragTask->mReceiver->GetModelKind());
	
			//	keyAEData
			//		optional -- uses keyAEPropData
			
			//	keyAEPropData
			ae.WriteKey(keyAEPropData);
			StAEDescriptor	oldDestProps;
			DragTask->mReceiver->GetImportantAEProperties(oldDestProps.mDesc);
			ae.WriteDesc(oldDestProps);
		
		ae.Close(&postUndoerAE.mDesc);

		action->SetPostUndoAE(postUndoerAE.mDesc);

		//	keyAEInsertHere
		action->PostUndoAESetKeyFed(keyAEInsertHere, keyAEInsertHere);
	}

	return action;
}

LAction *	LDataDragEventHandler::MakeOSpecTask(void)
{
	LAEAction	*rval = NULL;
	
	ThrowIfNULL_(rval);
	return rval;
}
	
LAction *	LDataDragEventHandler::MakeLinkTask(void)
{
	LAEAction	*rval = NULL;
	
	ThrowIfNULL_(rval);
	return rval;
}

