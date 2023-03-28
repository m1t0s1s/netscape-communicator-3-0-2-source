//	===========================================================================
//	LTextElemAEOM.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include	<LSelectableItem.h>

#include	<LTextEngine.h>	//	Range type
#include	<UAEGizmos.h>

class	LTextModel;

//	===========================================================================

class	LTextElemAEOM
			:	public LSelectableItem
{
private:
	typedef	LSelectableItem	inheritAEOM;
public:
				//	Maintenance
						LTextElemAEOM(
								const TextRangeT	&inRange,
								DescType			inKind,
								LModelObject		*inSuperModel,
								LTextModel			*inTextModel,
								LTextEngine			*inTextEngine
						);
	virtual				~LTextElemAEOM();
	
	virtual	LTextEngine *	GetTextEngine(void) const;
	virtual LTextModel *	GetTextModel(void) const;

	virtual	const TextRangeT &	GetSuperRange(void) const;
	virtual const TextRangeT &	GetRange(void) const;
	virtual	void		SetRange(const TextRangeT &inRange);
	virtual void		AdjustRanges(const TextRangeT &inRange, Int32 inLengthDelta);

				//	LSelectableItem implementation
	virtual	Boolean		IsSubstantive(void) const;
	virtual Boolean		EquivalentTo(const LSelectableItem *inItem) const;
	virtual Boolean		AdjacentWith(const LSelectableItem *inItem) const;
	virtual	Boolean		IndependentFrom(const LSelectableItem *inItem) const;
	virtual LSelectableItem *
						FindExtensionItem(const LSelectableItem *inItem) const;
	virtual Boolean		PointInRepresentation(Point inWhere) const;
	virtual void		DrawSelfSelected(void);
	virtual void		DrawSelfLatent(void);
	virtual void		DrawSelfReceiver(void);
	virtual void		UnDrawSelfSelected(void);
	virtual void		UnDrawSelfLatent(void);

				//	Data tubing
	virtual void		AddFlavorTypesTo(LDataTube *inTube) const;
	virtual Boolean		SendFlavorTo(FlavorType inFlavor, LDataTube *inTube) const;
	virtual FlavorType	PickFlavorFrom(const LDataTube *inTube) const;
	virtual void		ReceiveDataFrom(LDataTube *inTube);

public:

				//	AEOM support
	virtual LModelObject*	GetModelProperty(DescType inProperty) const;
	virtual Int32		CountSubModels(DescType inModelID) const;
	virtual LModelObject*	GetInsertionElement(DescType inInsertPosition) const;
	virtual void		GetSubModelByPosition(
									DescType		inModelID,
									Int32			inPosition,
									AEDesc			&outToken) const;
	virtual void		GetModelByRelativePosition(
									DescType		inModelID,
									OSType			inRelativePosition,
									AEDesc			&outToken) const;
	virtual void		GetSubModelByComplexKey(
									DescType		inModelID,
									DescType		inKeyForm,
									const AEDesc	&inKeyData,
									AEDesc			&outToken) const;
	virtual Int32		GetPositionOfSubModel(
									DescType		inModelID,
									const LModelObject	*inSubModel) const;
	virtual void		GetAEProperty(DescType		inProperty,
									const AEDesc	&inRequestedType,
									AEDesc			&outPropertyDesc) const;
	virtual void		SetAEProperty(DescType		inProperty,
									const AEDesc	&inValue,
									AEDesc&			outAEReply);

	virtual Boolean		CompareToModel(
									DescType		inComparisonOperator,
									LModelObject	*inCompareModel) const;
									
	virtual Boolean		CompareToDescriptor(
									DescType		inComparisonOperator,
									const AEDesc	&inCompareDesc) const;
									
	virtual Boolean 	CompareSelf(
									DescType	inComparisonOperator,
									const Ptr	inTextA,
									Int32		inLengthA,
									const Ptr	inTextB,
									Int32		inLengthB) const;
	virtual void		MakeSpecifier(AEDesc &outSpecifier) const;
	virtual void		WriteSpecifier(LAEStream *outStream) const;

				//	AEOM verbs...
	virtual LModelObject*	HandleCreateElementEvent(
								DescType			inElemClass,
								DescType			inInsertPosition,
								LModelObject*		inTargetObject,
								const AppleEvent	&inAppleEvent,
								AppleEvent			&outAEReply);						
	virtual void		HandleDelete(
								AppleEvent			&outAEReply,
								AEDesc				&outResult);
	virtual	void		HandleMove(
								const AppleEvent	&inAppleEvent,
								AppleEvent			&outAEReply,
								AEDesc				&outResult);

				//	AEOM implementation help
protected:
	virtual void		FindInsertionRange(
									DescType		inInsertPosition,
									TextRangeT		*outRange,
									Boolean			*outInclusive = NULL) const;
	
	virtual void		GetRangeAEProperty(
								const TextRangeT	&inRange,
								DescType			inProperty,
								const AEDesc		&inRequestedType,
								AEDesc				&outPropertyDesc) const;
	virtual void		SetRangeAEProperty(
								const TextRangeT	&inRange,
								DescType			inProperty,
								const AEDesc		&inValue,
								AEDesc&				outAEReply) const;
	virtual void		WriteSuperSpecifier(LAEStream *outStream) const;
	virtual void		WriteSpecifierSelf(LAEStream *outStream) const;
	virtual void		WriteInsertionSpecifierSelf(LAEStream *outStream) const;
	static Boolean		sTerminalSpecWritten;

				//	Implementation...
protected:
	virtual void		AdjustRange(const TextRangeT &inRange, Int32 inLengthDelta);
	virtual const Ptr	GetTextPtr(void) const;

				//	Data members
protected:
	TextRangeT				mRange;
	LTextEngine				*mTextEngine;
	LTextModel				*mTextModel;
};

#define	pLength	'leng'
#define	pOffset	'offs'
