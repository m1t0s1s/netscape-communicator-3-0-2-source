//	===========================================================================
//	LTextElemAEOM.cp					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LTextElemAEOM.h"
#include	<AEPowerPlantSuite.h>
#include	<AETextSuite.h>
#include	<LTextModel.h>
#include	<UAppleEventsMgr.h>
#include	<UMemoryMgr.h>
#include	<LStyleSet.h>
#include	<UAEGizmos.h>
#include	<LDescTube.h>

#ifndef	__AEREGISTRY__
#include <AERegistry.h>
#endif

#ifndef __AEOBJECTS__
#include <AEObjects.h>
#endif

#ifndef	__STRING__
#include <string.h>
#endif

#pragma	warn_unusedarg off

//	===========================================================================
//	AEOM support

Int32	LTextElemAEOM::CountSubModels(DescType inModelID) const
{
	switch (inModelID) {

		case cParagraph:
		case cLine:
		case cWord:
		case cChar:
			return mTextEngine->CountParts(mRange, inModelID);
			break;

//		case cText:			?
//		case cTextFlow:		?
//			break;

/*-?
		case cStyleSet:
			return mTextEngine->CountStyleSets(mRange);
			break;
*/

		default:
			return inherited::CountSubModels(inModelID);
			break;
	}
}

LModelObject *	LTextElemAEOM::GetModelProperty(
	DescType	inProperty) const
{
	LModelObject	*theProperty = nil;
	
	switch (inProperty) {

		case cParagraph:
		case cLine:
		case cWord:
		case cChar:
		{
			TextRangeT		superRange;
/*			1)

			Either of the following would be plausible but, since you can't
			record parenthesis easily and subspecs become finer and finer
			grained, it makes more sense to use the first.

			Ie. return the "absolute" paragraph, line, word or character.		*/
#if	1
			superRange = mTextEngine->GetTotalRange();
#else
			superRange = GetSuperRange();
#endif
		
			Assert_(
					superRange.Contains(mRange)
				||	(	!superRange.Length() &&
						!mRange.Length() &&
						(superRange.Start() == mRange.Start())
					)
			);

			TextRangeT		newRange = mRange;
			DescType		newType = inProperty;

			newType = mTextEngine->FindPart(superRange, &newRange, inProperty);

//			Again, because of (1) above, the "correct" super of the property is
//			the entire text model (instead of the later).
#if	1
			theProperty = mTextModel->MakeNewElem(newRange, newType);
#else
			theProperty = mTextModel->MakeNewElem(newRange, newType, super);
#endif
			break;
		}

		case pBeginning:
		case pEnding:
		case pBefore:
		case kAEBefore:
		case pAfter:
		case kAEAfter:
		{
			theProperty = GetInsertionElement(inProperty);
			break;
		}
			
		default:
			theProperty = inheritAEOM::GetModelProperty(inProperty);
	}
	
	return theProperty;
}

LModelObject *	LTextElemAEOM::GetInsertionElement(DescType inInsertPosition) const
{
	TextRangeT		range;

	FindInsertionRange(inInsertPosition, &range);

	if (range == mRange) {
		return (LModelObject *)this;
	} else {
		if (range.Length()) {
			return mTextModel->MakeNewElem(range, GetModelKind());
		} else {
			return mTextModel->MakeNewElem(range, cChar);
		}
	}
}

void	LTextElemAEOM::GetSubModelByPosition(
	DescType		inModelID,
	Int32			inPosition,
	AEDesc			&outToken) const
{
	switch (inModelID) {

		case cParagraph:
		case cLine:
		case cWord:
		case cChar:
		{
			TextRangeT		range;
			LTextElemAEOM	*subModel = NULL;
			DescType		partFoundType;		//	Maybe unnecessary since submodel is based
												//	off this rather than mTextLink.  But, it
												//	is safer.
			partFoundType = mTextEngine->FindNthPart(mRange, inModelID, inPosition, &range);
			subModel = mTextModel->MakeNewElem(range, partFoundType, (LModelObject *)this);
			PutInToken(subModel, outToken);
			break;
		}
		
/*-?
		case cStyleSet:
		{
			LStyleSet	*subModel = mTextEngine->GetStyleSet(mRange, inPosition);

			PutInToken(subModel, outToken);
			break;
		}
*/
		
		default:
			inherited::GetSubModelByPosition(inModelID, inPosition, outToken);
			break;
	}
}

void	LTextElemAEOM::GetModelByRelativePosition(
	DescType		inModelID,
	OSType			inRelativePosition,
	AEDesc			&outToken) const
{
	switch (inRelativePosition) {
		case kAEBeginning:	//	Doesn't record well
		case kAEEnd:		//	Doesn't record well
		case kAEBefore:
		case kAEAfter:
		case kAEPrevious:
		case kAENext:
		{
			TextRangeT		range = mRange;
			DescType		partType = cChar;
			LTextElemAEOM	*p;
			
			switch (inRelativePosition) {

				case kAEBeginning:
				case kAEBefore:
					range.Front();
					break;

				case kAEEnd:
				case kAEAfter:
					range.Back();
					break;

				case kAEPrevious:
				{
					do {
						range.Before();
						partType = mTextEngine->FindPart(mTextEngine->GetTotalRange(), &range, inModelID);
						if (partType == typeNull)
							break;
					} while (partType != inModelID);
					if (partType == typeNull)
						Throw_(errAENoSuchObject);
					break;
				}
				
				case kAENext:
				{
					do {
						range.After();
						partType = mTextEngine->FindPart(mTextEngine->GetTotalRange(), &range, inModelID);
						if (partType == typeNull)
							break;
					} while (partType != inModelID);
					if (partType == typeNull)
						Throw_(errAENoSuchObject);
					break;
				}
			}

			p = mTextModel->MakeNewElem(range, partType);
			
			PutInToken(p, outToken);
			break;
		}

		default:
			inheritAEOM::GetModelByRelativePosition(inModelID, inRelativePosition, outToken);
			break;
	}
}

void	LTextElemAEOM::GetSubModelByComplexKey(
	DescType		inModelID,
	DescType		inKeyForm,
	const AEDesc	&inKeyData,
	AEDesc			&outToken) const
{
	StAEDescriptor	ospec1,
					ospec2;
	StAEDescriptor	token1,
					token2;
	LTextElemAEOM	*submodel = NULL,
					*obj1 = NULL,
					*obj2 = NULL;
	TextRangeT		range,
					range1,
					range2;
	DescType		rangePartType,
					partType1,
					partType2;
					
							
	switch (inKeyForm) {
		case formRange:
		{
			//	Load boundary data
			//		See IM-IAC 6-20 for (AERecord *) coercion information).
			LTextElemAEOM	*obj1 = NULL,
							*obj2 = NULL;
			LAESubDesc		rangeRec(inKeyData),
							start = rangeRec.KeyedItem(keyAERangeStart),
							stop = rangeRec.KeyedItem(keyAERangeStop);

			obj1 = (LTextElemAEOM *)start.ToModelObject();
//			Assert_(member(obj1, LTextElemAEOM));

			obj2 = (LTextElemAEOM *)stop.ToModelObject();
//			Assert_(member(obj2, LTextElemAEOM));

			range1 = obj1->GetRange();
			range2 = obj2->GetRange();
			partType1 = obj1->GetModelKind();
			partType2 = obj2->GetModelKind();
			
			//	Make corresponding LTextElemAEOM submodel.
			range = TextRangeT(range1.Start(), range2.End());
			if (partType1 == partType2)
				rangePartType = partType1;
			else
				rangePartType = cChar;
			submodel = mTextModel->MakeNewElem(range, rangePartType, (LModelObject *)this);
			PutInToken(submodel, outToken);
			break;
		}

		case formTest:
		default:
			inheritAEOM::GetSubModelByComplexKey(inModelID, inKeyForm, inKeyData, outToken);
			break;
	}
}

Int32	LTextElemAEOM::GetPositionOfSubModel(
	DescType		inModelID,
	const LModelObject	*inSubModel) const
{
	switch (inModelID) {
		case cParagraph:
		case cLine:
		case cWord:
		case cChar:
		{
			const LTextElemAEOM*	p = (LTextElemAEOM *)inSubModel;
			TextRangeT				partRange;
			Int32					index1, index2;
			
//			Assert_(member(p, LTextElemAEOM));
			partRange = p->GetRange();
			
			mTextEngine->FindSubRangePartIndices(mRange, partRange, inModelID, &index1, &index2);
			return index1;
			break;
		}
			
		default:
			return inherited::GetPositionOfSubModel(inModelID, inSubModel);
			break;
	}
}

void	LTextElemAEOM::MakeSpecifier(AEDesc &outSpecifier) const
{
	LAEStream	stream;

	WriteSpecifier(&stream);
	
	stream.Close(&outSpecifier);
}

Boolean	LTextElemAEOM::sTerminalSpecWritten = false;

void	LTextElemAEOM::WriteSpecifier(LAEStream *outStream) const
{
	if (!sTerminalSpecWritten) {
		StValueChanger<Boolean>	change(sTerminalSpecWritten, true);
		
		if (!mRange.Length()) {
			WriteInsertionSpecifierSelf(outStream);
		} else {
			WriteSpecifierSelf(outStream);
		}
	} else {
		if (this == mTextModel->GetTextLink()) {
			WriteSuperSpecifier(outStream);
		} else {
			WriteSpecifierSelf(outStream);
		}
	}
}

void	LTextElemAEOM::WriteSuperSpecifier(LAEStream *outStream) const
{
	if (this == mTextModel->GetTextLink()) {
		StAEDescriptor	spec;
		
		GetSuperModel()->MakeSpecifier(spec.mDesc);
		outStream->WriteDesc(spec.mDesc);
	} else {
		LTextElemAEOM	*super = (LTextElemAEOM *)GetSuperModel();
//		Assert_(member(super, LTextElemAEOM));
		super->WriteSpecifier(outStream);
	}
}

void	LTextElemAEOM::WriteSpecifierSelf(LAEStream *outStream) const
{
	outStream->OpenRecord(typeObjectSpecifier);

	//	class
	outStream->WriteKey(keyAEDesiredClass);
	outStream->WriteTypeDesc(GetModelKind());
	
	//	container
	outStream->WriteKey(keyAEContainer);
	WriteSuperSpecifier(outStream);

	//	key form and key data
	{
		TextRangeT		superRange = GetSuperRange();
	
		DescType		partKind = GetModelKind(),
						partKindTry2;
		Int32			index1,
						index2;
	
		partKind = mTextEngine->FindSubRangePartIndices(superRange, mRange, partKind, &index1, &index2);
		ThrowIf_(partKind == typeNull);
		if (partKind != GetModelKind()) {
			partKindTry2 = mTextEngine->FindSubRangePartIndices(superRange, mRange, partKind, &index1, &index2);
			Assert_(partKindTry2 == partKind);
		}
		
		if (index2 > index1) {
		
			AEDesc	currentContainer = {typeCurrentContainer, NULL};
			
			//	A range
			outStream->WriteKey(keyAEKeyForm);
			outStream->WriteEnumDesc(formRange);
			outStream->WriteKey(keyAEKeyData);
			outStream->OpenRecord(typeRangeDescriptor);
			
				outStream->WriteKey(keyAERangeStart);
				outStream->OpenRecord(typeObjectSpecifier);
					outStream->WriteKey(keyAEDesiredClass);
					outStream->WriteTypeDesc(partKind);
					outStream->WriteKey(keyAEContainer);
					outStream->WriteDesc(currentContainer);
					outStream->WriteKey(keyAEKeyForm);
					outStream->WriteEnumDesc(formAbsolutePosition);
					outStream->WriteKey(keyAEKeyData);
					outStream->WriteDesc(typeLongInteger, &index1, sizeof(index1));
				outStream->CloseRecord();
	
				outStream->WriteKey(keyAERangeStop);
				outStream->OpenRecord(typeObjectSpecifier);
					outStream->WriteKey(keyAEDesiredClass);
					outStream->WriteTypeDesc(partKind);
					outStream->WriteKey(keyAEContainer);
					outStream->WriteDesc(currentContainer);
					outStream->WriteKey(keyAEKeyForm);
					outStream->WriteEnumDesc(formAbsolutePosition);
					outStream->WriteKey(keyAEKeyData);
					outStream->WriteDesc(typeLongInteger, &index2, sizeof(index2));
				outStream->CloseRecord();
				
			outStream->CloseRecord();
		} else {
		
			//	An absolute position
			outStream->WriteKey(keyAEKeyForm);
			outStream->WriteEnumDesc(formAbsolutePosition);
			outStream->WriteKey(keyAEKeyData);
			outStream->WriteDesc(typeLongInteger, &index1, sizeof(index1));
		}
	}
	
	outStream->CloseRecord();
}

void	LTextElemAEOM::WriteInsertionSpecifierSelf(LAEStream *outStream) const
{
	Assert_(mRange.Length() == 0);

	outStream->OpenRecord(typeObjectSpecifier);
	{
		TextRangeT		superRange = GetSuperRange();
	
		//	An insertion
		DescType		relType = typeNull;
		TextRangeT		front = superRange,
						back = superRange;
		
		front.Front();
		back.Back();
		
		if ((mRange == front) || mRange.IsBefore(superRange))
			relType = pBeginning;
		else if ((mRange == back) || mRange.IsAfter(superRange))
			relType = pEnding;
		
		if (relType != typeNull) {
			outStream->WriteKey(keyAEDesiredClass);
			outStream->WriteTypeDesc(GetModelKind());
			outStream->WriteKey(keyAEContainer);
			WriteSuperSpecifier(outStream);
			outStream->WriteKey(keyAEKeyForm);
			outStream->WriteEnumDesc(formPropertyID);
			outStream->WriteKey(keyAEKeyData);
			outStream->WriteTypeDesc(relType);
	
		} else {
		
			DescType		partKind = cChar,
							partKindTry2;
			Int32			index1,
							index2;
			TextRangeT		range(mRange.Start());
				
			partKind = mTextEngine->FindSubRangePartIndices(superRange, range, partKind, &index1, &index2);
			ThrowIf_(partKind == typeNull);
			if (partKind != GetModelKind()) {
				partKindTry2 = mTextEngine->FindSubRangePartIndices(superRange, range, partKind, &index1, &index2);
				Assert_(partKindTry2 == partKind);
			}
			
			relType = pBeginning;
			
			outStream->WriteKey(keyAEDesiredClass);
			outStream->WriteTypeDesc(GetModelKind());
			outStream->WriteKey(keyAEContainer);
			outStream->OpenRecord(typeObjectSpecifier);
				outStream->WriteKey(keyAEDesiredClass);
				outStream->WriteTypeDesc(cChar);
				outStream->WriteKey(keyAEContainer);
				WriteSuperSpecifier(outStream);
				outStream->WriteKey(keyAEKeyForm);
				outStream->WriteEnumDesc(formAbsolutePosition);
				outStream->WriteKey(keyAEKeyData);
				outStream->WriteDesc(typeLongInteger, &index1, sizeof(index1));
			outStream->CloseRecord();
			outStream->WriteKey(keyAEKeyForm);
			outStream->WriteEnumDesc(formPropertyID);
			outStream->WriteKey(keyAEKeyData);
			outStream->WriteTypeDesc(relType);
	
		}
	}
	outStream->CloseRecord();

}

void	LTextElemAEOM::GetAEProperty(
	DescType		inProperty,
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
	LStyleSet	*style = NULL;

	switch (inProperty) {

		case pFont:
		case pPointSize:
		case pTextColor:
		case pTextStyles:
		case pScriptTag:
			style = mTextEngine->GetStyleSet(mRange);
			style->GetAEProperty(inProperty, inRequestedType, outPropertyDesc);
			break;

/*
		case pUniformStyles:
		{
			Int32	i;

			for (i = 1; i <= mTextEngine->CountStylesSets(mRange); i++) {
				//	loop through styles
				GetRangeAEProperty(mRange, inProperty, inRequestedType, outPropertyDesc);
			}
			break;
		}
*/

//	use a getdatasize event?

		case pLength:
		{
			LAEStream	stream;
			Int32		len = mRange.Length();

			stream.WriteDesc(typeLongInteger, &len, sizeof(len));
			stream.Close(&outPropertyDesc);
			break;
		}

		case pOffset:
		{
			LAEStream	stream;
			Int32		start = mRange.Start();

			stream.WriteDesc(typeLongInteger, &start, sizeof(start));
			stream.Close(&outPropertyDesc);
			break;
		}

		case pContents:
		{
			LAESubDesc	typeList(inRequestedType);
			DescType	type;
			
			//	set default type
			if (mTextEngine->GetAttributes() & textEngineAttr_MultiByte)
				type = typeIntlText;
			else
				type = typeChar;

			//	look for a recognized specific type in the type list
			for (Int16 i = 1; i <= typeList.CountItems(); i++) {
				DescType	testType = typeList.NthItem(i).ToType();
				if (	testType == typeObjectSpecifier
					||	testType == typeIntlText
					||	testType == typeChar
				) {
					type = testType;
					break;
				}
			}

			LAEStream	stream;
			switch (type) {

				case typeObjectSpecifier:
					stream.WriteSpecifier(this);
					break;

				case typeIntlText:
				{
					style = mTextEngine->GetStyleSet(mRange);
					ScriptCode		script = style->GetScript();
					LangCode		lang = style->GetLanguage();
					Handle			h = mTextEngine->GetTextHandle();
					StHandleLocker	lock(h);
					
					stream.OpenDesc(typeIntlText);
					stream.WriteData(&script, sizeof(script));
					stream.WriteData(&lang, sizeof(lang));
					stream.WriteData((void *) (*h + mRange.Start()), mRange.Length());
					stream.CloseDesc();
					break;
				}
				
				case typeChar:
				{
					Handle			h = mTextEngine->GetTextHandle();
					StHandleLocker	lock(h);
					stream.WriteDesc(typeChar, (void *) (*h + mRange.Start()), mRange.Length());
					break;
				}
				
				default:
					Throw_(errAEWrongDataType);	//	actually a programmer error
					break;
			}
			stream.Close(&outPropertyDesc);
			break;
		}
		
		default:
			inherited::GetAEProperty(inProperty, inRequestedType, outPropertyDesc);
			break;
	}
}

void	LTextElemAEOM::SetAEProperty(
	DescType		inProperty,
	const AEDesc	&inValue,
	AEDesc&			outAEReply)
{
	switch (inProperty) {

		case pFont:
		case pPointSize:
		case pScriptTag:
		case pTextStyles:
		case pUniformStyles:
			Assert_(false);
//			SetRangeAEProperty(mRange, inProperty, inValue, outAEReply);
			break;

		case pContents:
			Throw_(errAEEventNotHandled);
			break;

		default:
			inherited::SetAEProperty(inProperty, inValue, outAEReply);
			break;
	}
}

//	===========================================================================
//	Implementation

void	LTextElemAEOM::GetRangeAEProperty(
	const TextRangeT		&inRange,
	DescType		inProperty,
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
}

void	LTextElemAEOM::SetRangeAEProperty(
	const TextRangeT		&inRange,
	DescType		inProperty,
	const AEDesc	&inValue,
	AEDesc&			outAEReply) const
{
}

//	===========================================================================
//	Verb handling...

void	LTextElemAEOM::HandleDelete(
	AppleEvent			&outAEReply,
	AEDesc				&outResult)
{
	StRecalculator	changeMe(mTextEngine);
	TextRangeT		holeRange(mRange.Start()),
					range = mRange;

	mTextEngine->TextDelete(range);

	//	Make the extra deletion "hole" location parameter for the reply.
	//		This extra ae reply parameter helps PowerPlant with undo.  And
	//		is keyed with keyAEInsertHere.
	{
		StAEDescriptor	holeDesc;
		LTextElemAEOM	*hole = mTextModel->MakeNewElem(holeRange, cChar);
		StSharer		lock(hole);
	
		hole->MakeSpecifier(holeDesc.mDesc);

		UAEDesc::AddKeyDesc(&outAEReply, keyAEInsertHere, holeDesc.mDesc);
	}
}

void	LTextElemAEOM::HandleMove(
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outAEReply,
	AEDesc				&outResult)
{
	StRecalculator	changeMe(mTextEngine);
	inherited::HandleMove(inAppleEvent, outAEReply, outResult);
}

LModelObject *	LTextElemAEOM::HandleCreateElementEvent(
	DescType			inElemClass,
	DescType			inInsertPosition,
	LModelObject		*inTargetObject,
	const AppleEvent	&inAppleEvent,
	AppleEvent			&outAEReply)
{
	LTextElemAEOM	*result = NULL;

	switch (inElemClass) {
		case cChar:
		case cWord:
		case cLine:
		case cParagraph:
			break;

		default:
			return inherited::HandleCreateElementEvent(inElemClass, inInsertPosition,
									inTargetObject, inAppleEvent, outAEReply);
			break;
	}

	StRecalculator	changeMe(mTextEngine);
	LAESubDesc		ae(inAppleEvent),
					props = ae.KeyedItem(keyAEPropData),
					data;

	//	AERegistry indicates the data from KeyAEPropData takes precedence...
	if (props.GetType() != typeNull)
		data = props.KeyedItem(pContents);

	if (data.GetType() == typeNull)
		data = ae.KeyedItem(keyAEData);		

	if (data.GetType() == typeNull)
		Throw_(errAEEventFailed);

	StAEDescriptor	desc;	//	data

	if (data.GetType() == typeAERecord) {
		data.ToDesc(&desc.mDesc);
	} else {
		//	Fabricate a "timi" descriptor of just the text...
		LAEStream	ds;
		ds.OpenRecord();
		ds.WriteKey(data.GetType());
		ds.WriteSubDesc(data);
		ds.CloseRecord();
		ds.Close(&desc.mDesc);
	}
	
	result = (LTextElemAEOM *) inTargetObject->GetInsertionElement(inInsertPosition);
//	Assert_(member(result, LTextElemAEOM));

	LDescTube	tube(desc.mDesc);
	
	result->ReceiveDataFrom(&tube);

	return result;
}

void	LTextElemAEOM::FindInsertionRange(
	DescType			inInsertPosition,
	TextRangeT			*outRange,
	Boolean				*outInclusive
) const
/*
	"This" is the "target" object.
*/
{
	TextRangeT		range = mRange;
	
	//	outRange
	switch (inInsertPosition) {

		case pBeginning:
			range.Front();
			break;
			
		case pBefore:
		case kAEBefore:
			range.Front();
			range.SetInsertion(rangeCode_Before);
			break;

		case pEnding:
			range.Back();
			break;
			
		case pAfter:
		case kAEAfter:
			range.Back();
			range.SetInsertion(rangeCode_After);
			break;

		case pSame:
			break;

		default:
			Throw_(errAEBadKeyForm);
			break;
	}	
	ThrowIfNULL_(outRange);
	*outRange = range;

	//	outInclusive
	if (outInclusive) {
		switch (inInsertPosition) {

			case pBeginning:
			case pEnding:
			case pSame:
				*outInclusive = true;
				break;
				
			default:
				*outInclusive = false;
				break;
		}
	}
}

Boolean	LTextElemAEOM::CompareToModel(
	DescType		inComparisonOperator,
	LModelObject*	inCompareModel
) const
{
	if (!IsSubModelOf(mTextModel))
//	if (!member(inCompareModel, LTextElemAEOM))
		ThrowOSErr_(errAEEventNotHandled);

	LTextElemAEOM	*bModel = (LTextElemAEOM *)inCompareModel;	//	unsafe type cast
	Int32			alen = mRange.Length(),
					blen = bModel->GetRange().Length();
	Ptr				a = GetTextPtr(),
					b = bModel->GetTextPtr();

	return CompareSelf(inComparisonOperator, a, alen, b, blen);
}	

Boolean	LTextElemAEOM::CompareToDescriptor(
	DescType		inComparisonOperator,
	const AEDesc&	inCompareDesc
) const
{
	LAESubDesc		dataSD(inCompareDesc, typeChar);

	Int32	alen = mRange.Length(),
			blen = dataSD.GetDataLength();
	Ptr		a = GetTextPtr(),
			b = (Ptr) dataSD.GetDataPtr();

	return CompareSelf(inComparisonOperator, a, alen, b, blen);
}

Boolean LTextElemAEOM::CompareSelf(
	DescType	inComparisonOperator,
	const Ptr	inTextA,
	Int32		inLengthA,
	const Ptr	inTextB,
	Int32		inLengthB

) const
{
	Int32	len = URange32::Max(inLengthA, inLengthB);
	Int16	result;
	Boolean	rval = false;

	switch (inComparisonOperator) {

		case kAEGreaterThan:
		case kAEGreaterThanEquals:
		case kAEEquals:
		case kAELessThan:
		case kAELessThanEquals:
		case kAEBeginsWith:
			result = memcmp(inTextA, inTextB, len);
			switch (inComparisonOperator) {
				case kAEGreaterThan:
					rval = result > 0;
					break;
				case kAEGreaterThanEquals:
					rval = result >= 0;
					break;
				case kAEBeginsWith:
					rval = result == 0;
					break;	
				case kAEEquals:
					rval = (result == 0) && (inLengthA == inLengthB);
					break;
				case kAELessThan:
					rval = result < 0;
					break;
				case kAELessThanEquals:
					rval = result <= 0;
					break;
			}
			break;

		case kAEEndsWith:
			if (inLengthA >= inLengthB) {
				result = memcmp(inTextA + (inLengthA - inLengthB), inTextB, inLengthB);
				rval = result == 0;
			}
			break;

		case kAEContains:
//		case kAEGrep:
		default:
			ThrowOSErr_(errAEEventNotHandled);
			break;
	}

	return rval;
}