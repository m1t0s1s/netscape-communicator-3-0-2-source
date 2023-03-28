//	===========================================================================
//	VTextElemAEOM.cp
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"VTextElemAEOM.h"

#include	<VOTextModel.h>
#include	<LTextEngine.h>
#include	<LDescTube.h>
#include	<AEVTextSuite.h>
#include	<LStyleSet.h>
#include	<LRulerSet.h>

#pragma	warn_unusedarg off

//	===========================================================================
//	Maintenance

VTextElemAEOM::VTextElemAEOM(
	const TextRangeT	&inRange,
	DescType			inKind,
	LModelObject		*inSuperModel,
	LTextModel			*inTextModel,
	LTextEngine			*inTextEngine)
	:	LTextElemAEOM(inRange, inKind, inSuperModel, inTextModel, inTextEngine)
{
}

VTextElemAEOM::~VTextElemAEOM()
{
}

void	VTextElemAEOM::GetImportantAEProperties(AERecord &outRecord) const
{
	VOTextModel	*model = dynamic_cast<VOTextModel*>(GetTextModel());
	if (model) {
		LDescTube	tube(&outRecord, this, false);
	} else {
		inherited::GetImportantAEProperties(outRecord);
	}
}

LModelObject *	VTextElemAEOM::HandleCreateElementEvent(
	DescType			inElemClass,
	DescType			inInsertPosition,
	LModelObject		*inTargetObject,
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outAEReply)
{
	LTextElemAEOM	*rval = NULL;

	switch (inElemClass) {

		case cChar:
		case cWord:
		case cLine:
		case cParagraph:
			break;

		default:
			return inherited::HandleCreateElementEvent(inElemClass, inInsertPosition, inTargetObject, inAppleEvent, outAEReply);
			break;
	}

	StRecalculator	changeMe(mTextEngine);
	LAESubDesc		ae(inAppleEvent),
					props = ae.KeyedItem(keyAEPropData),
					data;

	if (props.GetType() != typeNull)
		data = props;

	if (data.GetType() == typeNull)
		data = ae.KeyedItem(keyAEData);		

	if (data.GetType() == typeNull)
		Throw_(errAEEventFailed);

	StAEDescriptor	desc;	//	data

	if (data.GetType() == typeAERecord) {
		data.ToDesc(&desc.mDesc);
	} else {
		LAEStream	stream;

		stream.OpenRecord();
			stream.WriteKey(data.GetType());
			stream.WriteSubDesc(data);
		stream.CloseRecord();
		stream.Close(&desc.mDesc);
	}
	
	rval = dynamic_cast<LTextElemAEOM *>(inTargetObject->GetInsertionElement(inInsertPosition));
	ThrowIfNULL_(rval);

	LDescTube	tube(desc.mDesc);
	
	rval->ReceiveDataFrom(&tube);

	return rval;
}

void	VTextElemAEOM::HandleAppleEvent(
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outAEReply,
	AEDesc				&outResult,
	long				inAENumber)
{
	switch (inAENumber) {

		case ae_Modify:
		{
			LAESubDesc	ae(inAppleEvent),
						value;
			DescType	property = ae.KeyedItem(keyAEModifyProperty).ToEnum(),
						operation = typeNull;
			
			value = ae.KeyedItem(keyAEModifyToValue);
			
			if (value.GetType() != typeNull) {
				operation = keyAEModifyToValue;
				
				if (ae.KeyExists(keyAEModifyByValue))
					Throw_(errAEEventNotHandled);	//	only presence of by OR to is allowed
			} else {
				value = ae.KeyedItem(keyAEModifyByValue);
				if (value.GetType() == typeNull)
					Throw_(errAEEventNotHandled);	//	presence of by OR to is required
				operation = keyAEModifyByValue;
			}
			HandleModify(property, operation, value, inAppleEvent, outAEReply, outResult);
			break;
		}
				
		default:
			inherited::HandleAppleEvent(inAppleEvent, outAEReply, outResult, inAENumber);
			break;
	}
}

void	VTextElemAEOM::HandleModify(
	DescType			inProperty,
	DescType			inOperation,
	const LAESubDesc	&inValue,
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outAEReply,
	AEDesc				&outResult)
{
	VOTextModel	*model = dynamic_cast<VOTextModel *>(mTextModel);
	
	if (model)
		model->Modify(mRange, inProperty, inOperation, inValue);
	
	MakeSpecifier(outResult);
}

void	VTextElemAEOM::GetAEProperty(
	DescType		inProperty,
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
	LModelObject	*set = NULL;

	switch (inProperty) {

		case pStyleSet:
			set = mTextEngine->GetStyleSet(mRange);
			set->MakeSpecifier(outPropertyDesc);
			break;

		case pRulerSet:
			set = mTextEngine->GetRulerSet(mRange);
			set->MakeSpecifier(outPropertyDesc);
			break;

		default:
			inherited::GetAEProperty(inProperty, inRequestedType, outPropertyDesc);
			break;
	}
}

void	VTextElemAEOM::SetAEProperty(
	DescType		inProperty,
	const AEDesc	&inValue,
	AEDesc&			outAEReply)
{
	switch (inProperty) {

		case pStyleSet:
		{
			StRecalculator	changeMe(mTextEngine);
			LAESubDesc		valueSD(inValue);
			LStyleSet		*set = dynamic_cast<LStyleSet *>(valueSD.ToModelObject());

			if (!set || !set->IsSubModelOf(mTextModel))
				Throw_(errAEWrongDataType);
			
			mTextEngine->SetStyleSet(mRange, set);
			break;
		}

		case pRulerSet:
		{
			StRecalculator	changeMe(mTextEngine);
			LAESubDesc		valueSD(inValue);
			LRulerSet		*set = dynamic_cast<LRulerSet *>(valueSD.ToModelObject());

			if (!set || !set->IsSubModelOf(mTextModel))
				Throw_(errAEWrongDataType);
			
			mTextEngine->SetRulerSet(mRange, set);
			break;
		}

		default:
			inherited::SetAEProperty(inProperty, inValue, outAEReply);
			break;
	}
}

