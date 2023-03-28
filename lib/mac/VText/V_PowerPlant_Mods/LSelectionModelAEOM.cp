//	===========================================================================
//	LSelectionModelAEOM.cp			©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

//	===========================================================================
/*
	This class provides AE:
		
		clipboard behavior
		selection linkage
*/
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LSelectionModelAEOM.h"

#include	<LSelection.h>
#include	<LClipboardTube.h>
#include	<LAEAction.h>
#include	<UAppleEventsMgr.h>
#include	<UExtractFromAEDesc.h>
#include	<LStream.h>

#ifndef		__AEOBJECTS__
#include	<AEObjects.h>
#endif

#pragma	warn_unusedarg off

//	===========================================================================
//	Maintenance

LSelectionModelAEOM::LSelectionModelAEOM()
{
	mSelection = NULL;
	SetUseSubModelList(true);
}

LSelectionModelAEOM::LSelectionModelAEOM(
		LModelObject	*inSuperModel,
		DescType		inKind)
	:	LSharableModel(inSuperModel, inKind)
{
	mSelection = NULL;
	SetUseSubModelList(true);
}

LSelectionModelAEOM::~LSelectionModelAEOM()
{
	SetSelection(NULL);
}

#ifndef	PP_No_ObjectStreaming

void	LSelectionModelAEOM::WriteObject(LStream &inStream) const
{
	//	for LModelObject
	inStream << GetModelKind();
	Uint32	superModel = (Uint32)((void *)GetSuperModel());
	inStream << superModel;
	
	//	for this
	inStream <<	mSelection;
}

void	LSelectionModelAEOM::ReadObject(LStream &inStream)
{
	DescType	modelKind;
	inStream >> modelKind;
	SetModelKind(modelKind);

	SetSuperModel(ReadVoidPtr<LModelObject *>()(inStream));
	SetSelection(ReadStreamable<LSelection *>()(inStream));	
}

#endif

LSelection *	LSelectionModelAEOM::GetSelection(void)
{
	return mSelection;
}

void	LSelectionModelAEOM::SetSelection(LSelection *inSelection)
{
	if (mSelection)
		mSelection->ListClear();
		
	ReplaceSharable(mSelection, inSelection, this);
}

//	===========================================================================
//	AEOM

void	LSelectionModelAEOM::GetAEProperty(
	DescType		inProperty,
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
	switch (inProperty) {

		case pSelection:
			Assert_(mSelection);
			mSelection->GetAEValue(inRequestedType, outPropertyDesc);
			break;

		default:
			inheritAEOM::GetAEProperty(inProperty, inRequestedType, outPropertyDesc);
			break;
	}
}

void	LSelectionModelAEOM::SetAEProperty(
	DescType		inProperty,
	const AEDesc	&inValue,
	AEDesc&			outAEReply)
{
	switch (inProperty) {

		case pSelection:
			Assert_(mSelection);
			mSelection->SetAEValue(inValue, outAEReply);
			break;

		default:
			inheritAEOM::SetAEProperty(inProperty, inValue, outAEReply);
			break;
	}
}

LModelObject *	LSelectionModelAEOM::GetModelProperty(
	DescType	inProperty) const
{
	LModelObject	*theProperty = nil;
	
	switch (inProperty) {
		case pSelection:
			theProperty = mSelection;
			break;
			
		default:
			theProperty = inheritAEOM::GetModelProperty(inProperty);
	}
	
	return theProperty;
}

void	LSelectionModelAEOM::GetModelTokenSelf(
	DescType		inModelID,
	DescType		inKeyForm,
	const AEDesc	&inKeyData,
	AEDesc			&outToken) const
{
	Boolean	handled = false;

//	This WOULD work but the present aete/AppleScript arrangement doesn't
//	record "selection" as an ospec but a constant.
//	
//	Bummer.

	do {
		if (!mSelection)
			break;
			
		if (inKeyForm != formPropertyID)
			break;
		
		LAESubDesc	typeSD(inKeyData);
		if (typeSD.ToType() != pSelection)
			break;
		
		//	Add selection items to outToken
		Int32	i,
				count = mSelection->ListCount();
		
		if (count <= 0)
			Throw_(paramErr);	//	selection is empty
				
		for (i = 1; i <= count; i++) {
			LModelObject *p = mSelection->ListNthItem(i);
			PutInToken(p, outToken);
		}
		
		handled = true;
	} while (false);

	if (!handled)
		inheritAEOM::GetModelTokenSelf(inModelID, inKeyForm, inKeyData, outToken);
}

//	===========================================================================
//	AEOM Misc suite (clipboard) support

#include	<UAEGizmos.h>
#include	<LListIterator.h>
#include	<LSelectableItem.h>

void LSelectionModelAEOM::HandleAppleEvent(
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outAEReply,
	AEDesc				&outResult,
	long				inAENumber)
{
	switch (inAENumber) {

		case ae_Copy:
			HandleCopy(outAEReply);
			break;
			
		case ae_Cut:
			HandleCut(outAEReply);
			break;
			
		case ae_Paste:
			HandlePaste(outAEReply);
			break;
			
		case ae_Select:
			HandleSelect(inAppleEvent, outAEReply, outResult);
			break;
			
		default:
			inheritAEOM::HandleAppleEvent(inAppleEvent, outAEReply, outResult, inAENumber);
			break;
	}
}

void	LSelectionModelAEOM::HandleSelect(
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outAEReply,
	AEDesc				&outResult
)
{
	StAEDescriptor	value;
	
	value.GetParamDesc(inAppleEvent, keyAEData, typeWildCard);
	
	if (mSelection)
		mSelection->SetAEValue(value.mDesc, outAEReply);
}

void	LSelectionModelAEOM::HandleCut(AppleEvent &outAEReply)
{
	StAEDescriptor	result,
					deleteReply,
					insertLoc;
	OSErr			err;

	HandleCopy(outAEReply);
	
	if (outAEReply.descriptorType == typeNull) {
		err = AECreateList(NULL, 0, true, &deleteReply.mDesc);
		ThrowIfOSErr_(err);
	}
		
	mSelection->HandleDelete(deleteReply.mDesc, result.mDesc);

	insertLoc.GetOptionalParamDesc(deleteReply.mDesc, keyAEInsertHere, typeWildCard);
	if (insertLoc.mDesc.descriptorType != typeNull) {
		err = AEPutParamDesc(&outAEReply, keyAEResult, &insertLoc.mDesc);
		ThrowIfOSErr_(err);
	}
}

void	LSelectionModelAEOM::HandleCopy(AppleEvent &outAEReply)
{
	LClipboardTube	tube(mSelection);
}

//-
#include	<StClasses.h>

void	LSelectionModelAEOM::HandlePaste(AppleEvent &outAEReply)
{
	OSErr			err;
	LClipboardTube	tube;
//-
StProfile	profileMe;

	mSelection->ReceiveDataFrom(&tube);
	
	//	As an added bonus (so that undo will work easy for text like things), 
	//	lets add the resulting selection value as a reply...
	StAEDescriptor	result;

	mSelection->GetAEValue(UAppleEventsMgr::sAnyType, result.mDesc); 

	err = AEPutParamDesc(&outAEReply, keyAEResult, &result.mDesc);
	ThrowIfOSErr_(err);
}
