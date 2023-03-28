//	===========================================================================
//	LTextElemAEOM.cp					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LTextElemAEOM.h"

#include	<LTextModel.h>
#include	<LTextEngine.h>
#include	<LDataTube.h>
#include	<UDrawingState.h>
#include	<UDrawingUtils.h>
#include	<LHandlerView.h>
#include	<LListIterator.h>
#include	<URegions.h>

#ifndef	__AEREGISTRY__
#include <AERegistry.h>
#endif

//	===========================================================================
//	Maintenance

LTextElemAEOM::LTextElemAEOM(
	const TextRangeT	&inRange,
	DescType			inKind,
	LModelObject		*inSuperModel,
	LTextModel			*inTextModel,
	LTextEngine			*inTextEngine)
	:	LSelectableItem(inSuperModel, inKind)
{
	mRange = inRange;

	mTextEngine = inTextEngine;
	mTextModel = inTextModel;

	Assert_(mTextEngine);
	Assert_(mTextModel);
	
	//	Text elem's are not AE "accessed" through the submodel list.
	//	But, the submodel list is used to keep a list of all instantiated
	//	text objects that need to be updated when the text changes.
	//	See AdjustRanges.
	SetUseSubModelList(true);
	
	SetLaziness(true);
}

LTextElemAEOM::~LTextElemAEOM()
{
}

LTextEngine *	LTextElemAEOM::GetTextEngine(void) const
{
	return mTextEngine;
}

LTextModel *	LTextElemAEOM::GetTextModel(void) const
{
	return mTextModel;
}

//	===========================================================================
//	LSelectableItem implementation

Boolean	LTextElemAEOM::IsSubstantive(void) const
{
	return mRange.Length() > 0;
}

Boolean	LTextElemAEOM::EquivalentTo(const LSelectableItem *inItem) const
{
	Boolean	rval = false;
	
	if (inItem) {
		LTextElemAEOM*	that = (LTextElemAEOM*) inItem;
//		Assert_(member(that, LTextElemAEOM));
		
		rval = mRange == that->GetRange();
	}
	
	return rval;
}

Boolean	LTextElemAEOM::AdjacentWith(const LSelectableItem *inItem) const
{
	Boolean	rval = false;
	
	if (inItem) {
		LTextElemAEOM*	that = (LTextElemAEOM*) inItem;
//		Assert_(member(that, LTextElemAEOM));

		rval = mRange.Adjacent(that->GetRange());
	}
	
	return rval;
}

Boolean	LTextElemAEOM::IndependentFrom(const LSelectableItem *inItem) const
{
	Boolean	rval = true;
	
	if (inItem) {
		LTextElemAEOM*	that = (LTextElemAEOM*) inItem;
//		Assert_(member(that, LTextElemAEOM));

		rval = !mRange.Intersects(that->GetRange());
	}

	return rval;
}

LSelectableItem	*	LTextElemAEOM::FindExtensionItem(const LSelectableItem *inItem) const
{
	if (inItem) {
		TextRangeT		newRange = GetRange();
		DescType		newType;
		LTextElemAEOM	*inTextItem = (LTextElemAEOM *)inItem;
//		Assert_(member(inTextItem, LTextElemAEOM));
	
		newRange.Union(inTextItem->GetRange());
		
		if (GetModelKind() == inTextItem->GetModelKind())
			newType = GetModelKind();
		else
			newType = cChar;
	
		return mTextModel->MakeNewElem(newRange, newType);
	} else {
		return (LTextElemAEOM *)this;
	}
}

Boolean	LTextElemAEOM::PointInRepresentation(Point inWhere) const
{
	SPoint32		where;
	LHandlerView	*theView;
	
	theView = mTextEngine->GetHandlerView();
	ThrowIfNULL_(theView);
	theView->LocalToImagePoint(inWhere, where);

	return mTextEngine->PointInRange(where, mRange);
}

//	---------------------------------------------------------------------------
//	Visual representation implementation.

void	LTextElemAEOM::DrawSelfSelected(void)
{
	StRegion		rgn;
	StColorPenState	savePen;
		
	mTextEngine->GetRangeRgn(mRange, &rgn.mRgn);
	
	StColorPenState::Normalize();
	UDrawingUtils::SetHiliteModeOn();
	InvertRgn(rgn.mRgn);
	ThrowIfOSErr_(QDError());
}

void	LTextElemAEOM::DrawSelfLatent(void)
{
	StRegion		rgn;
	StColorPenState	savePen;
		
	mTextEngine->GetRangeRgn(mRange, &rgn.mRgn);
	
	PenPat(&UQDGlobals::GetQDGlobals()->black);
	PenMode(srcXor);
	UDrawingUtils::SetHiliteModeOn();
	FrameRgn(rgn.mRgn);
	ThrowIfOSErr_(QDError());
}

void	LTextElemAEOM::UnDrawSelfSelected(void)
{
	DrawSelfSelected();
}

void	LTextElemAEOM::UnDrawSelfLatent(void)
{
	DrawSelfLatent();
}

void	LTextElemAEOM::DrawSelfReceiver(void)
{
	StRegion		rgn;
	StColorPenState	savePen;
		
	mTextEngine->GetRangeRgn(mRange, &rgn.mRgn);
	
	StColorPenState::Normalize();
	PenPat(&UQDGlobals::GetQDGlobals()->black);
	PenMode(srcXor);
	InvertRgn(rgn.mRgn);
	ThrowIfOSErr_(QDError());

	LSelectableItem::DrawSelfReceiver();
}

//	===========================================================================
//	Data tubing

void	LTextElemAEOM::AddFlavorTypesTo(LDataTube *inTube) const
{
	Assert_(this == (LSelectableItem *)this);	//	If this fails... it is bad and something must be systemically fixed.

	mTextModel->AddFlavorTypesTo(mRange, inTube);

	if (!inTube->GetOnlyReqdFlavors())
		LSelectableItem::AddFlavorTypesTo(inTube);
}

Boolean	LTextElemAEOM::SendFlavorTo(FlavorType inFlavor, LDataTube *inTube) const
{
	if (mTextModel->SendFlavorTo(inFlavor, mRange, inTube)) {
		return true;
	} else {
		return LSelectableItem::SendFlavorTo(inFlavor, inTube);
	}
}

FlavorType	LTextElemAEOM::PickFlavorFrom(const LDataTube *inTube) const
{
	return mTextModel->PickFlavorFrom(mRange, inTube);
}

void	LTextElemAEOM::ReceiveDataFrom(LDataTube *inTube)
{
	TextRangeT	range = mRange,
				preTotRange = mTextEngine->GetTotalRange();
	
	Assert_(GetSuperRange().WeakContains(range));
	
	//	Do the transfer
	mTextModel->ReceiveDataFrom(range, inTube);
	
	//	Fix up this elem's range...
	TextRangeT	newTotRange = mTextEngine->GetTotalRange(),
				resultRange = range,
				superRange = GetSuperRange();
	Int32		resultLength = newTotRange.Length() - (preTotRange.Length() - range.Length());
	
	if (resultLength < 0) {
		Assert_(false);
		Throw_(paramErr);
	}
	
	if (resultLength) {
		resultRange.SetLength(resultLength);
		Assert_(superRange.Contains(resultRange));
	} else {
		resultRange.Front();
		resultRange.Crop(superRange);
	}

	SetRange(resultRange);
}

//	===========================================================================
//	Implementation

const TextRangeT &	LTextElemAEOM::GetSuperRange(void) const
{
	if ((this == mTextModel->GetTextLink()) || !mTextModel->GetTextLink()) {
		//	This model is the link and has no super range
		Assert_(mTextEngine->GetTotalRange() == mRange);

		return GetRange();
	} else {
		const LTextElemAEOM	*super = dynamic_cast<LTextElemAEOM *>(GetSuperModel());
		
		ThrowIfNULL_(super);

		return super->GetRange();
	}
}

const TextRangeT &	LTextElemAEOM::GetRange(void) const
{
	return mRange;
}

void	LTextElemAEOM::SetRange(const TextRangeT &inRange)
{
	if (mRange != inRange) {

		if (this != mTextModel->GetTextLink()) {
			Assert_(GetSuperRange().Contains(inRange));
		}
	
		mRange = inRange;
	}
}

const Ptr	LTextElemAEOM::GetTextPtr(void) const
/*
	Result is valid only until next memory move.
	You may not change data pointed to by the result.
	The data pointed to by the result is only valid for mRange.Length() characters.
*/
{
	Ptr	rval = *mTextEngine->GetTextHandle();
	rval += mRange.Start();
	return rval;
}

void	LTextElemAEOM::AdjustRanges(const TextRangeT &inRange, Int32 inLengthDelta)
{
	AdjustRange(inRange, inLengthDelta);

	if (mSubModels) {
		LListIterator	iterator(*mSubModels, iterate_FromStart);
		LModelObject	*p = NULL;
		LTextElemAEOM	*q;
				
		while (iterator.Next(&p)) {
			ThrowIfNULL_(p);
			switch(p->GetModelKind()) {
				case cWord:
				case cLine:
				case cParagraph:
				case cChar:
					q = (LTextElemAEOM *)p;
//					Assert_(member(q, LTextElemAEOM));
					q->AdjustRanges(inRange, inLengthDelta);
					break;
				default:
					break;
			}
		}
	}
}

void	LTextElemAEOM::AdjustRange(const TextRangeT &inRange, Int32 inLengthDelta)
{
	if (mTextModel->GetTextLink() == this) {
		mRange.Adjust(inRange, inLengthDelta, true);
		mTextModel->FixLink();
	} else {
		mRange.Adjust(inRange, inLengthDelta, false);
		mRange.Crop(GetSuperRange());
	}
}
