//	===========================================================================
//	LTextSelection.cp				©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LTextSelection.h"
#include	<LListIterator.h>
#include	<LSelectableItem.h>
#include	<LTextElemAEOM.h>
#include	<LTextModel.h>
#include	<LTextEngine.h>
#include	<UAppleEventsMgr.h>
#include	<LStream.h>
#include	<UDrawingState.h>
#include	<URegions.h>
#include	<LStyleSet.h>
#include	<LCommander.h>

#ifndef		__ERRORS__
#include	<Errors.h>
#endif

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

//	Shouldn't be defined for any user code -- just for internal debugging of
//	LTextEngine and derivatives.
//	#define	DEBUG_POINTINRANGE

/*-
LTextSelection::LTextSelection(
	LTextEngine		*inTextEngine
)
{
	mTextModel = NULL;
	mTextEngine = NULL;
	mTyping = NULL;

	SetTextEngine(inTextEngine);

	mPendingStyle = NULL;
	mLastCurrentStyle = NULL;
}
*/

LTextSelection::LTextSelection()
{
	mTextModel = NULL;
	mTextEngine = NULL;
	mTyping = NULL;

	mPendingStyle = NULL;
	mLastCurrentStyle = NULL;
}

LTextSelection::~LTextSelection(void)
{
	SetTextModel(NULL);
	SetTextEngine(NULL);
	
	SetPendingStyle(NULL);
	ReplaceSharable(mLastCurrentStyle, (LStyleSet *)NULL, this);
}

#ifndef	PP_No_ObjectStreaming

void	LTextSelection::WriteObject(LStream &inStream) const
{	
	inherited::WriteObject(inStream);
	
	inStream << mTextEngine;
	inStream << mTextModel;
}

void	LTextSelection::ReadObject(LStream &inStream)
{
	inherited::ReadObject(inStream);
	
	SetTextEngine(ReadStreamable<LTextEngine *>()(inStream));
	SetTextModel(ReadStreamable<LTextModel *>()(inStream));
}

#endif

LTextModel	*	LTextSelection::GetTextModel(void)
{
	return mTextModel;
}

void	LTextSelection::SetTextModel(LTextModel *inTextModel)
{
	//	ReplaceSharable(mTextModel, inTextModel, this);
	//
	//	NOT!
	//		The above would mean both the model and selection share each
	//		other so neither would ever go away.
	//
	//		It is safe to assume that mTextModel will never be left
	//		dangling because the model and selection will both be claimed
	//		by at least one other object.

	mTextModel = inTextModel;
}

LTextEngine *	LTextSelection::GetTextEngine(void)
{
	return mTextEngine;
}

void	LTextSelection::SetTextEngine(LTextEngine *inTextEngine)
{
	//	ReplaceSharable(mTextEngine, inTextEngine, this);
	//
	//	This is unnecessary.  Whatever owns the selection can dispose of it
	//	before the engine
	
	mTextEngine = inTextEngine;
}

TextRangeT	LTextSelection::GetSelectionRange(void) const
{
	TextRangeT	range = LTextEngine::sTextUndefined;
	
	if (ListCount() == 1) {
		LSelectableItem *item = ((LTextSelection *)this)->ListNthItem(1);
//		if (member(item, LTextElemAEOM))
			range = ((LTextElemAEOM *)item)->GetRange();
	}
	
	return range;
}

void	LTextSelection::SetSelectionRange(const TextRangeT &inRange)
{
	if (mTextModel) {
		LTextElemAEOM	*elem = mTextModel->MakeNewElem(inRange, cChar);
		StSharer		lock(elem);	//	in case of failure be sure and blow way elem
		
		SelectSimple(elem);
	}
}

void	LTextSelection::SelectDiscontinuous(LSelectableItem *inItem)
{
//	It isn't a good idea to allow discontinuous selection in text until
//	you could properly undo multi-item moves.  That means an "ordered" move
//	and that hasn't been implemented -- yet.
//
//	So defeat text discontinuous selections with...
SelectSimple(inItem);
}

void	LTextSelection::SetTypingAction(LAETypingAction *inAction)
{
	mTyping = inAction;
}

LAETypingAction *	LTextSelection::GetTypingAction(void)
{
	return mTyping;
}

LStyleSet *	LTextSelection::GetCurrentStyle(void)
{
	LStyleSet	*rval = mPendingStyle;
	TextRangeT	range = LTextEngine::sTextAll;

	if (!rval && ListCount()) {
		LSelectableItem	*item = ListNthItem(1);
		LTextElemAEOM	*elem =	dynamic_cast<LTextElemAEOM *>(item);
		ThrowIfNULL_(elem);
		range = elem->GetRange();
	}
	
	if (!rval && mTextEngine)
		rval = mTextEngine->GetStyleSet(range);
	
	if (rval != mLastCurrentStyle) {
		LCommander::SetUpdateCommandStatus(true);
		ReplaceSharable(mLastCurrentStyle, rval, this);
	}

	return rval;
}

LStyleSet *	LTextSelection::GetPendingStyle(void)
{
	return mPendingStyle;
}

void	LTextSelection::SetPendingStyle(LStyleSet *inStyle)
{
	if (inStyle != mPendingStyle) {
		ReplaceSharable(mPendingStyle, inStyle, this);

		LCommander::SetUpdateCommandStatus(true);
	}
}

void	LTextSelection::SelectionAboutToChange(void)
{
	SetPendingStyle(NULL);
	
	inherited::SelectionAboutToChange();
}

void	LTextSelection::MakeRegion(Point inOrigin, RgnHandle *ioRgn) const
{
	ThrowIfNULL_(ioRgn);
	ThrowIfNULL_(*ioRgn);
		
	URegions::MakeEmpty(*ioRgn);

	StRegion		rgn;
	LListIterator	iter(((LTextSelection *)this)->mItems, iterate_FromStart);
	LSelectableItem	*p;
	LTextElemAEOM	*elem;
			
	while (iter.Next(&p)) {
		ThrowIfNULL_(p);
		elem = (LTextElemAEOM *)p;
//		Assert_(member(elem, LTextElemAEOM));
		mTextEngine->GetRangeRgn(elem->GetRange(), &rgn.mRgn);
		UnionRgn(rgn, *ioRgn, *ioRgn);
		ThrowIfOSErr_(QDError());
	}

	//	Set "origin" of region
	Point	offset;
	offset.h = -inOrigin.h;
	offset.v = -inOrigin.v;
	URegions::Translate(*ioRgn, offset);
}

Boolean		LTextSelection::PointInRepresentation(Point inWhere) const
/*
	It is irritating to have to implement this method this way rather than using
	the inherited LTextEngine::PointInRange.  Unfortunately, there is no way to
	tell exactly what the selection region contains when gluing to other
	engines.  This method will work.
*/
{
#ifdef	DEBUG_POINTINRANGE
	return LSelection::PointInRepresentation(inWhere);
#else
	Boolean rval;
	
	if (!IsSubstantive())
		return false;
	
	StRegion	rgn;
	Point		origin;
	MakeRegion(inWhere, &rgn.mRgn);
	origin.h = origin.v = 0;
	rval = PtInRgn(origin, rgn);
	return rval;
#endif
}

void	LTextSelection::GetPresentHiliteRgn(Boolean inAsActive, RgnHandle ioRegion) const
{
	Point			origin = {0, 0};	
		
	if (IsSubstantive()) {
		MakeRegion(origin, &ioRegion);
		if (!inAsActive)
				URegions::MakeRegionOutline(ioRegion, &ioRegion);
	} else {
		if (inAsActive) {
			//	blinking caret is black and doesn't fit hilited XOR model
			//	(and happily everthing still turns out okay)
			URegions::MakeEmpty(ioRegion);
		} else {
			MakeRegion(origin, &ioRegion);
		}
	}
}

void	LTextSelection::AddFlavorTypesTo(LDataTube *inTube) const
{
	inherited::AddFlavorTypesTo(inTube);
	
	if (IsSubstantive() && (ListCount() == 1)) {
		//	if single item, add flavors to root of tube
		ListNthItem(1)->AddFlavorTypesTo(inTube);
	}
}

Boolean	LTextSelection::SendFlavorTo(FlavorType inFlavor, LDataTube *inTube) const
{
	Boolean	rval = inherited::SendFlavorTo(inFlavor, inTube);
	
	if (!rval && (ListCount() == 1)) {
		rval = ListNthItem(1)->SendFlavorTo(inFlavor, inTube);
	}
	
	return rval;
}

#include	<LAEAction.h>

LAction *	LTextSelection::MakePasteAction(void)
/*
	For text, the selection point should be moved to the end
	of the new text.
*/
{
	LAEAction	*action;

	action = (LAEAction *)LSelection::MakePasteAction();
//	Assert_(member(action, LAEAction));
	action->SetSelection(this, kAEAfter);  //	note selection modifier.

	return action;
}

void	LTextSelection::UndoAddOldData(LAEAction *inAction)
{
	inherited::UndoAddOldData(inAction);
	
	//	To make updating smooth
	inAction->SetRecalcAccumulator(mTextEngine);
}
