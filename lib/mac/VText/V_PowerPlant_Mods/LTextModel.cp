//	===========================================================================
//	LTextModel.cp					©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LTextModel.h"

#include	<LTextElemAEOM.h>
#include	<LStyleSet.h>
#include	<AETextSuite.h>
#include	<LListIterator.h>
#include	<LAEAction.h>
#include	<LTextSelection.h>
#include	<PP_Messages.h>
#include	<LStream.h>
#include	<UAEGizmos.h>
#include	<UMemoryMgr.h>
#include	<UAppleEventsMgr.h>
#include	<AEPowerPlantSuite.h>

#ifndef		__ERRORS__
#include	<Errors.h>
#endif

#pragma	warn_unusedarg off

//	===========================================================================
//	Maintenance

LTextModel::LTextModel()
:	LSelectionModelAEOM(NULL, cText)
{
	mTextEngine = NULL;
	mTextLink = NULL;
	mTextSelection = NULL;
}

/*-
LTextModel::LTextModel(LTextEngine *inTextEngine)
:	LSelectionModelAEOM(NULL, cText)
{
	mTextEngine = NULL;
	mTextLink = NULL;
	mTextSelection = NULL;

	SetTextEngine(inTextEngine);
	Assert_(mTextEngine);

	SetDefaultSubModel(new LTextElemAEOM(LTextEngine::sTextStart, cChar, this, this, mTextEngine));
}
*/

LTextModel::~LTextModel()
{
	SetTextEngine(NULL);
	SetDefaultSubModel(NULL);
}

#ifndef	PP_No_ObjectStreaming

void	LTextModel::WriteObject(LStream &inStream) const
{
	//	no simple data
	
	LSelectionModelAEOM::WriteObject(inStream);
	
	inStream << mTextEngine;
}

void	LTextModel::ReadObject(LStream &inStream)
{
	//	no simple data

	LSelectionModelAEOM::ReadObject(inStream);
		
	SetTextEngine(ReadStreamable<LTextEngine *>()(inStream));

//-	SetDefaultSubModel(MakeNewElem(LTextEngine::sTextStart, cChar, this));
}

#endif

LTextEngine	*	LTextModel::GetTextEngine(void)
{
	return mTextEngine;
}

void	LTextModel::SetTextEngine(LTextEngine *inTextEngine)
{
	if (mTextEngine != inTextEngine) {

		if (mTextEngine)
			mTextEngine->AddListener(this);

		ReplaceSharable(mTextEngine, inTextEngine, this);

		if (mTextEngine)
			mTextEngine->AddListener(this);

		if (mTextEngine)
			SetDefaultSubModel(MakeNewElem(mTextEngine->GetTotalRange(), cChar, this));
		else
			SetDefaultSubModel(NULL);
	}
}

void	LTextModel::SetSelection(LSelection *inSelection)
{
	inheritAEOM::SetSelection(inSelection);
	
	mTextSelection = dynamic_cast<LTextSelection *>(mSelection);
}

void	LTextModel::SetDefaultSubModel(LModelObject *inSubModel)
{
	inheritAEOM::SetDefaultSubModel(inSubModel);
	
	if (GetDefaultSubModel()) {
		ReplaceSharable(mTextLink, (LTextElemAEOM *)GetDefaultSubModel(), this);
//		Assert_(member(mTextLink, LTextElemAEOM));
	} else {
		ReplaceSharable(mTextLink, (LTextElemAEOM *)NULL, this);
	}
}

const TextRangeT&	LTextModel::GetRange(void)
{
	if (mTextLink)
		return mTextLink->GetRange();
	else
		return LTextEngine::sTextUndefined;
}

//	===========================================================================
//	Data tube i/o:

void	LTextModel::AddFlavorTypesTo(
	const TextRangeT	&inRange,
	LDataTube			*inTube) const
{
	if (inRange.Length() >= 0) {
		inTube->AddFlavor(typeChar);
		
		if (!inTube->GetOnlyReqdFlavors())
			inTube->AddFlavor(typeIntlText);
	}
}

Boolean	LTextModel::SendFlavorTo(
	FlavorType			inFlavor,
	const TextRangeT	&inRange,
	LDataTube			*inTube) const
{
	TextRangeT	range = inRange;
	Boolean		rval = false;
	
	range.Crop(mTextEngine->GetTotalRange());
	
	switch (inFlavor) {
		case typeChar:
		{
			Handle	h = mTextEngine->GetTextHandle();
			StHandleLocker	lock(h);
			
			inTube->SetFlavorData(inFlavor, *h + range.Start(), range.Length());
			rval = true;
			break;
		}
		
		case typeIntlText:
		{
			LStream			&stream = inTube->SetFlavorStreamOpen(typeIntlText);
			LStyleSet		*style = mTextEngine->GetStyleSet(range);
			ScriptCode		script = style->GetScript();
			LangCode		lang = style->GetLanguage();
			Handle			h = mTextEngine->GetTextHandle();
			StHandleLocker	lock(h);

			stream << script;
			stream << lang;
			stream.WriteBlock(*h + range.Start(), range.Length());
			
			inTube->SetFlavorStreamClose();
			break;
		}
	}

	return rval;
}

FlavorType	LTextModel::PickFlavorFrom(
	const TextRangeT	&inRange,
	const LDataTube		*inTube) const
/*
	typeIntlText currently ignored
*/
{
	Assert_(inTube);
	
	FlavorType	rval = typeNull,
				flavor;
	Int32		n = inTube->CountItems();
	
	do {
		if (inTube->FlavorExists(typeChar)) {
			rval = typeChar;
			break;
		}
		
		for (Int32 i = 1; i <= n; i++) {
			LDataTube	subTube = inTube->NthItem(i);
			flavor = PickFlavorFrom(inRange, &subTube);
			
			if (flavor == typeNull) {
				rval = flavor;
				break;
			}
			
			if (rval == typeNull) {
				rval = flavor;
			} else if (rval != flavor) {
				rval = typeWildCard;
			}
		}
if (rval != typeNull) rval = typeWildCard;

	} while (false);

	return rval;
}

/*
FlavorType	LTextModel::PickFlavorFrom(
	const TextRangeT	&inRange,
	const LDataTube		*inTube)
{
	Assert_(inTube);
	
	FlavorType	rval = typeNull,
				flavor;
	Int32		n = inTube->CountItems();
	
	do {
		if (inTube->FlavorExists(typeWildCard)) {
			rval = PickFlavorSelf(inTube);
			if (rval == typeNull)
				break;
		}
		
		for (i = 1; i <= n; i++) {
			LDataTube subTube = inTube->NthItem(i);
			flavor = PickFlavorFrom(inRange, &subTube)
			
			if (flavor == typeNull) {
				rval = flavor;
				break;
			}
			
			if (rval == typeNull) {
				rval = flavor;
			} else if (rval != flavor) {
				rval = typeWildCard;
			}
		}
	} while (false);
}

FlavorType	LTextModel::PickFlavorFromSelf(...)
{
	if (inTube->FlavorExists(typeChar))
		return typeChar;
	else
		return typeNull;
}
*/		

void	LTextModel::ReceiveDataFrom(
	const TextRangeT	&inRange,
	LDataTube			*inTube)
/*
	Script and language code in typeIntlText currently ignored
*/
{
	TextRangeT	range = inRange;
	Size		size;
	
	range.Crop(mTextEngine->GetTotalRange());
	
	if (inTube->FlavorExists(typeChar)) {

		size = inTube->GetFlavorSize(typeChar);
		StPointerBlock	data(size);
		inTube->GetFlavorData(typeChar, data.mPtr);
		mTextEngine->TextReplaceByPtr(range, data.mPtr, size);
		range = range.Start() + size;

	} else if (inTube->FlavorExists(typeIntlText)) {

		LStream			&stream = inTube->GetFlavorStreamOpen(typeIntlText);
		ScriptCode		script;
		LangCode		lang;
		Int32			size = inTube->GetFlavorSize(typeIntlText) - (sizeof(script) + sizeof(lang));
		ThrowIf_(size < 0);
		StPointerBlock	data(size);
		
		stream >> script;
		stream >> lang;
		stream.ReadBlock(data.mPtr, size);

		mTextEngine->TextReplaceByPtr(range, data.mPtr, size);

		inTube->GetFlavorStreamClose();

	} else {

		//	Items...
		Int32	n = inTube->CountItems();
		
		for (Int32 i = 1; i <= n; i++) {
			LDataTube	subTube = inTube->NthItem(i);
			TextRangeT	preTotRange = mTextEngine->GetTotalRange(),
						newTotRange;

			ReceiveDataFrom(range, &subTube);

			newTotRange = mTextEngine->GetTotalRange();
			range = range.Start() + (newTotRange.Length() - (preTotRange.Length() - range.Length()));
		}
	}
}
	
LTextElemAEOM *	LTextModel::MakeNewElem(
	const TextRangeT	&inRange,
	DescType			inKind,
	const LModelObject	*inSuperModel) const
/*
	Call this to make text "elems."
	
	You will VERY rarely need to override this method.  And, you will only rarely
	need to override MakeNewElemSelf.
*/
{
	LTextElemAEOM	*rval = NULL;
	
	if (inSuperModel) {
	
		//	Do explicitly what was indicated
		const LTextElemAEOM	*super = dynamic_cast<const LTextElemAEOM *>(inSuperModel);
		TextRangeT		range = inRange;

		if (super)
			range.Crop(super->GetRange());
		rval = MakeNewElemSelf(range, inKind, inSuperModel);
		if (super)
			Assert_(super->GetRange().Contains(rval->GetRange()));
	
	} else {
		
		//	Base off the head model
		rval = MakeNewElem(inRange, inKind, mTextLink);

	}

	return rval;
}

LTextElemAEOM *	LTextModel::MakeNewElemSelf(
	const TextRangeT	&inRange,
	DescType			inKind,
	const LModelObject	*inSuperModel
) const
/*
	Override this to have the text model instantiate elems of a more specialized
	LTextElemAEOM subclass.
*/
{
	return new LTextElemAEOM(inRange, inKind, (LModelObject *)inSuperModel, (LTextModel *)this, mTextEngine);
}

LStyleSet *	LTextModel::StyleSetForScript(
	const TextRangeT	&inRange,
	ScriptCode			inScript)
{
	return NULL;
}

void	LTextModel::FixLink(void) const
{
	if (mTextEngine && mTextLink) {
		if (mTextLink->GetRange() != mTextEngine->GetTotalRange()) {
			mTextLink->SetRange(mTextEngine->GetTotalRange());
		}
	}
}

void	LTextModel::ListenToMessage(MessageT inMessage, void *ioParam)
{
	switch (inMessage) {

		case msg_TextFluxed:
		{
			TextFluxRecordT	*p = (TextFluxRecordT *)ioParam;
			
			ThrowIfNULL_(mTextLink);
			mTextLink->AdjustRanges(p->range, p->lengthDelta);	//	true is new 950505
			FixLink();	//	precautionary test
			break;
		}

		case msg_ModelChanged:
			FixLink();	//	precautionary test
			break;
	}
}

//	===========================================================================
//	Implementation

#ifdef	INCLUDE_TX
	#include	<LTXStyleSet.h>
#endif

LModelObject*	LTextModel::HandleCreateElementEvent(
	DescType			inElemClass,
	DescType			inInsertPosition,
	LModelObject		*inTargetObject,
	const AppleEvent	&inAppleEvent,

	AppleEvent			&outAEReply)
{
	LModelObject	*rval = NULL;

	switch(inElemClass) {
	
#ifdef	INCLUDE_TX
		case cStyleSet:
		{
			StAEDescriptor	properties;
			Int32			i,
							count;
			StAEDescriptor	bogusReply,
							propDesc;
			AEKeyword		keyword;
			LStyleSet		*newStyleSet;
			
			newStyleSet = new LTXStyleSet();
			newStyleSet->SetSuperModel(this);
			properties.GetOptionalParamDesc(inAppleEvent, keyAEPropData, typeAERecord);
			if (properties.mDesc.descriptorType != typeNull) {
				LAESubDesc	aProperty,
							theProps(properties);
				count = theProps.CountItems();
				
				for (i = 1; i <= count; i++) {
					aProperty = theProps.NthItem(i, &keyword);
					AEDisposeDesc(propDesc);
					aProperty.ToDesc(propDesc);
					newStyleSet->SetAEProperty(keyword, propDesc, bogusReply);
				}
			}
			
			rval = newStyleSet;
			break;
		}
#endif
		default:
			rval = inheritAEOM::HandleCreateElementEvent(inElemClass, inInsertPosition,
						mTextLink, inAppleEvent, outAEReply);
			break;
	}
	return rval;
}

LModelObject *	LTextModel::GetModelProperty(
	DescType	inProperty) const
{
	LModelObject	*theProperty = nil;
	
	switch (inProperty) {

		case pSelection:
		{
			if (mSelection) {
				if (mSelection->ListCount() != 1) {
					Assert_(false);
					theProperty = inheritAEOM::GetModelProperty(inProperty);
					break;
				}

				LTextElemAEOM	*p = (LTextElemAEOM *) mSelection->ListNthItem(1);
			//	Assert_(member(p, LTextElemAEOM());
				theProperty = p;
				break;
			} else {
				//	Fall through
			}
		}
		
		default:
			theProperty = inheritAEOM::GetModelProperty(inProperty);
	}
	
	return theProperty;
}

void	LTextModel::GetAEProperty(
	DescType		inProperty,
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
 	switch (inProperty) {
		case pPPTextAEOMVersion:
		{
			OSErr	err;
			Int32	intValue;

			intValue = PPTEXTAEOMVERSION;
			err = AECreateDesc(typeLongInteger, (Ptr) &intValue, sizeof(intValue), &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;
		}

		case pUpdateLevel:
		{
			Int32	intValue = 0;
			OSErr	err;
			
			if (mTextEngine) {
				intValue = mTextEngine->GetNestingLevel();
			}
			
			err = AECreateDesc(typeLongInteger, (Ptr) &intValue, sizeof(intValue), &outPropertyDesc);
			ThrowIfOSErr_(err);
			break;
		}
		
		default:
			inheritAEOM::GetAEProperty(inProperty, inRequestedType, outPropertyDesc);
			break;
	}
}

void	LTextModel::SetAEProperty(
	DescType		inProperty,
	const AEDesc	&inValue,
	AEDesc&			outAEReply)
{
	switch (inProperty) {

		case pUpdateLevel:
		{
			LAESubDesc	valSD(inValue);
			Int32		inValue = valSD.ToInt32(),
						value;
			OSErr		err = noErr;
			
			if (mTextEngine) {
				value = mTextEngine->GetNestingLevel();
				
				if (value +1 == inValue) {

					//	increment
					mTextEngine->SetNestingLevel(value +1);
					mTextEngine->NestedUpdateIn();

				} else if (value -1 == inValue) {

					//	decrement
					Try_ {
						mTextEngine->NestedUpdateOut();
					} Catch_(inErr) {
						err = inErr;
					} EndCatch_;		
					mTextEngine->SetNestingLevel(value -1);
					Assert_(mTextEngine->GetNestingLevel() >= 0);

					ThrowIfOSErr_(err);
					
				} else {
					Throw_(paramErr);	//	should read "you can only increment or decrement the nesting level"
				}
			}
			break;
		}
		
		default:
			inheritAEOM::SetAEProperty(inProperty, inValue, outAEReply);
			break;
	}
}

LModelObject *	LTextModel::GetInsertionTarget(DescType inInsertPosition) const
{
	return inheritAEOM::GetInsertionTarget(inInsertPosition);
}

