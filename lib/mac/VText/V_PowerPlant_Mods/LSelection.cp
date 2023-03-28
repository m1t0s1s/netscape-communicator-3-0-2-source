//	===========================================================================
//	LSelection.cp					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LSelection.h"
#include	<PP_Resources.h>
#include	<LListIterator.h>
#include	<LSelectableItem.h>
#include	<UDrawingState.h>
#include	<UAppleEventsMgr.h>
#include	<UAEGizmos.h>
#include	<LDataTube.h>
#include	<LTubeableItem.h>
#include	<URegions.h>
#include	<LStream.h>
#include	<LSelectionModelAEOM.h>
#include	<AERegistry.h>
#include	<AEPowerPlantSuite.h>
#include	<AEDebugSuite.h>

#ifndef	__AEOBJECTS__
#include <AEObjects.h>
#endif

#ifndef __AEREGISTRY__
#include <AERegistry.h>
#endif

#pragma	warn_unusedarg off

//	===========================================================================
//	Maintenance

LSelection::LSelection()
	:	LSelectableItem(NULL, cProperty)
{
	SetModelKind(pSelection);
	
	mRecordAllSelections = false;
	
//-	mRecordAllSelections = true;	//	True for debugging ONLY
	
	mAnchor = NULL;
	mSelectionID = 0;
}

LSelection::~LSelection(void)
{
	ListClear();
}

#ifndef	PP_No_ObjectStreaming

void	LSelection::WriteObject(LStream &inStream) const
{	
	//	for this
	inStream << mRecordAllSelections;

	//	for LSelectableItem
	LSelectionModelAEOM	*model = dynamic_cast<LSelectionModelAEOM *>(GetSuperModel());
	inStream << model;
}

void	LSelection::ReadObject(LStream &inStream)
{
	//	for this
	inStream >> mRecordAllSelections;
	
	SetSuperModel(ReadStreamable<LSelectionModelAEOM *>()(inStream));
}

#endif

//	===========================================================================
//	New features

void	LSelection::SelectSimple(LSelectableItem *inItem)
{
	SelectionAboutToChange();

	ListClear();
	ListAddItem(inItem);
	
	SelectionChanged();

	if (mRecordAllSelections)
		RecordPresentSelection();
	
	ReplaceSharable(mAnchor, inItem, this);
}

void	LSelection::SelectContinuous(LSelectableItem *inItem)
/*
	May need to override depending on implementation strategy for continuous
	selections.
*/
{
	if (!mAnchor) {
		SelectDiscontinuous(inItem);
	} else {
		StSharer		lockItem(inItem);
		LSelectableItem	*lastItem = ListNthItem(ListCount()),
						*newItem = mAnchor->FindExtensionItem(inItem),
						*p;
		StSharer		lockNew(newItem);

		ThrowIfNULL_(newItem);
		
		SelectionAboutToChange();
		
		if (lastItem)
			ListRemoveItem(lastItem);
		p = ListDependentItem(newItem);
		if (p)
			ListRemoveItem(p);
		ListAddItem(newItem);
		
		SelectionChanged();
		
		if (mRecordAllSelections)
			RecordPresentSelection();
	}
}

void	LSelection::SelectDiscontinuous(LSelectableItem *inItem)
{
	if (mRecordAllSelections) {
		StAEDescriptor	desc,
						temp;
		
		if (inItem) {
			StAEDescriptor	appleEvent;
			LModelObject	*super = GetSuperModel();
			if (super != NULL) {
			
				//	In an AE context
				StAEDescriptor	newValue;
				LAEStream		s;
				
				s.OpenList();
				s.WriteSpecifier(this);
				s.WriteSpecifier(inItem);
				s.CloseList();				
				s.Close(&newValue.mDesc);
				
				DoAESelection(newValue);
			}
		}
	} else {

		SelectionAboutToChange();

		LSelectableItem	*p;
		p = ListDependentItem(inItem);
		if (p) {
			ListRemoveItem(p);
		} else {
			ListAddItem(inItem);
		}

		SelectionChanged();
	}
	
	ReplaceSharable(mAnchor, inItem, this);
}

//	===========================================================================
//	Query

Boolean	LSelection::IsSubstantive(void) const
/*
	Does the selection contain data that can be copied & cut?
*/
{
	Boolean	rval = false;
	
	LListIterator	iter(((LSelection *)this)->mItems, iterate_FromStart);
	LSelectableItem	*p = NULL;
	
	while (iter.Next(&p)) {
		Assert_(p);
		if (p->IsSubstantive()) {
			rval = true;
			break;
		}
	}
	
	return rval;
}

Int32	LSelection::SelectionID(void) const
/*
	Client code can use SelectionID to return a "serial" number to tell if
	the selection has changed (the SelectionID return value will be different
	than the previous value).
	
	The msg_SelectionChanged broadcast indicates the selection did just change.
	The mechanism you use depends on what makes most sense.
*/
{
	return mSelectionID;
}

LSelectableItem *	LSelection::ListEquivalentItem(const LSelectableItem *inItem) const
/*
	Returns a pointer to the selection item that is equivalent to inItem.
	
	If no selection item is equivalent to inItem, NULL is returned.
	
	Equivalent means the model objects resolve to the same set of 
	actual objects.
	
	Notes:
			You may treat the return value as a boolean indicating whether
			the present selection includes the inItem.
			
			For non-lazy objects, the return value may be inItem.

			For lazy objects, the return value will be the pointer to the
				selection's copy of the equivalent inItem.  If inItem is
				one of the selection's lazy objects, the return value will
				be inItem.
				
			If a value is returned consider it an object owned by the
			selection.  Specifically, don't dispose or Finalize it.
			If you need it in a longer term scenario, follow the policies
			regarding objects of class LSharable.
*/
{
	LListIterator	iter(((LSelection *)this)->mItems, iterate_FromStart);
	LSelectableItem	*p = NULL;
	
	while (iter.Next(&p)) {
		Assert_(p);
		if (p->EquivalentTo(inItem))
			return p;
	}
	
	return NULL;
}

LSelectableItem *	LSelection::ListDependentItem(const LSelectableItem *inItem) const
/*
	Returns a pointer to a selection item that is not independent from inItem.
	
	If all selection items are independent from inItem, NULL is returned.
	
	Independent means none of the set of actual objects indicated by inItem
	are represented in the set indicated by the selection.
	
	Notes:
			You may treat the return value as a boolean indicating whether
			the present selection is dependent with inItem.
			
			For non-lazy objects, the return value may be inItem.

			For lazy objects, the return value will be the pointer to the
				selection's copy of the equivalent inItem.  If inItem is
				one of the selection's lazy objects, the return value will
				be inItem.
				
			If a value is returned consider it an object owned by the
			selection.  Specifically, don't dispose or Finalize it.
			If you need it in a longer term scenario, follow the policies
			regarding objects of class LSharable.
*/
{
	LListIterator	iter(((LSelection *)this)->mItems, iterate_FromStart);
	LSelectableItem	*p = NULL;
	
	while (iter.Next(&p)) {
		Assert_(p);
		if (!p->IndependentFrom(inItem))
			return p;
	}
	
	return NULL;
}

LSelectableItem *	LSelection::ListAdjacentItem(const LSelectableItem *inItem) const
/*
	Returns a pointer to the first selection item that is adjacent with inItem.
	
	If no selection items adjacent with inItem, NULL is returned.
	
	Adjacent means two objects share "boundaries."
	are represented in the set indicated by the selection.
	
	Notes:
			For non-lazy objects, the return value may be inItem.

			For lazy objects, the return value will be the pointer to the
				selection's copy of the equivalent inItem.  If inItem is
				one of the selection's lazy objects, the return value will
				be inItem.
				
			If a value is returned consider it an object owned by the
			selection.  Specifically, don't dispose or Finalize it.
			If you need it in a longer term scenario, follow the policies
			regarding objects of class LSharable.
*/
{
	LListIterator	iter(((LSelection *)this)->mItems, iterate_FromStart);
	LSelectableItem	*p = NULL;
	
	while (iter.Next(&p)) {
		Assert_(p);
		if (p->AdjacentWith(inItem))
			return p;
	}
	
	return NULL;
}

//	===========================================================================
//	Implementation -- manipulator

Boolean		LSelection::EquivalentTo(const LSelectableItem *inItem) const
{
	if (ListCount() == 1)
		return ListEquivalentItem(inItem) != NULL;
	else
		return false;
}

Boolean		LSelection::AdjacentWith(const LSelectableItem *inItem) const
{
	if (ListCount() == 1)
		return ListAdjacentItem(inItem) != NULL;
	else
		return false;
}

Boolean		LSelection::IndependentFrom(const LSelectableItem *inItem) const
{
	return ListDependentItem(inItem) == NULL;
}

Boolean		LSelection::PointInRepresentation(Point inWhere) const
{
	LListIterator	iter(((LSelection *)this)->mItems, iterate_FromStart);
	LSelectableItem	*p = NULL;
	
	if (!IsSubstantive())
		return false;
		
	while (iter.Next(&p)) {
		if (p->PointInRepresentation(inWhere))
			return true;
	}
	
	return false;
}

void	LSelection::DrawSelf(void)
{
	DrawLatent();
}

void	LSelection::UnDrawSelf(void)
{
}

void	LSelection::MakeRegion(Point inOrigin, RgnHandle *ioRgn) const
/*
	Resulting region will have it's origin as inOrigin --
	this means the region can be independent of coordinate
	system origin.
*/
{
	Boolean		createdRgn = false;
	OSErr		err = noErr;
	
	ThrowIfNULL_(ioRgn);

	if (*ioRgn == NULL) {
		*ioRgn = NewRgn();
		createdRgn = true;
	}
	ThrowIfNULL_(*ioRgn);
	
	Try_ {
		
		//	Get latent region
		{
			StRegionBuilder	buildRgn(*ioRgn);
			
//			DrawSelected();	//	doesn't work for text case.
			((LSelection *)this)->DrawLatent();	//	this works for any case
		}
		
		//	Set "origin" of region
		Point	offset;
		offset.h = -inOrigin.h;
		offset.v = -inOrigin.v;
		URegions::Translate(*ioRgn, offset);
	} Catch_(inErr) {

		if (createdRgn && (*ioRgn == NULL)) {
			DisposeRgn(*ioRgn);
			*ioRgn = NULL;
		}
		
		Throw_(inErr);
	} EndCatch_;
}

void	LSelection::DrawContent(void)
/*
	Draw selection content.
	
	This routine should be rarely called
		-- only for something like "print selection."
*/
{
	LListIterator		iter(mItems, iterate_FromStart);
	LSelectableItem		*p;
	
	while(iter.Next(&p)) {
		p->DrawSelf();
	}
}

//	===========================================================================
//	Implementation

void	LSelection::DrawSelected(void)
{
	LListIterator		iter(mItems, iterate_FromStart);
	LSelectableItem		*p;
	
	while(iter.Next(&p)) {
		p->DrawSelfSelected();
	}
}

void	LSelection::DrawLatent(void)
{
	LListIterator		iter(mItems, iterate_FromStart);
	LSelectableItem		*p;
	
	while(iter.Next(&p)) {
		p->DrawSelfLatent();
	}
}

void	LSelection::UnDrawSelected(void)
{
	LListIterator		iter(mItems, iterate_FromStart);
	LSelectableItem		*p;

	while(iter.Next(&p)) {
		p->UnDrawSelfSelected();
	}
}

void	LSelection::UnDrawLatent(void)
{
	LListIterator		iter(mItems, iterate_FromStart);
	LSelectableItem		*p;
	
	while(iter.Next(&p)) {
		p->UnDrawSelfLatent();
	}
}

//	===========================================================================
//	Data tubing

void	LSelection::AddFlavorTypesTo(LDataTube *inTube) const
{
	ThrowIfNULL_(inTube);
	
	inTube->AddFlavor(typeObjectSpecifier);	//	for selection
	
	//	items...
	LSelectableItem	*item;
	LListIterator	iter(((LSelection *)this)->mItems, iterate_FromStart);
	
	while (iter.Next(&item)) {
		inTube->OpenItem(item);
	
		item->AddFlavorTypesTo(inTube);
		
		inTube->CloseItem();
	}
}

Boolean	LSelection::SendFlavorTo(FlavorType inFlavor, LDataTube *inTube) const
{
	Boolean	rval = false;
	
	if (inFlavor == typeObjectSpecifier) {
		StAEDescriptor	ospec;
		MakeSpecifier(ospec.mDesc);
		inTube->SetFlavorHandle(typeObjectSpecifier, ospec.mDesc.dataHandle);
		rval = true;
	}
	
	return rval;
}

FlavorType	LSelection::PickFlavorFrom(const LDataTube *inTube) const
{
	ThrowIfNULL_(inTube);

	LSelectableItem	*selItem;
	LListIterator	iter(((LSelection *)this)->mItems, iterate_FromStart);
	FlavorType		flavor,
					rval = typeNull;
	
	while (iter.Next(&selItem)) {
		flavor = selItem->PickFlavorFrom(inTube);

		if (flavor == typeNull) {
			rval = typeNull;
			break;
		}
		
		if (rval == typeNull) {
			rval = flavor;
		} else {
			rval = typeWildCard;
		}
	}

	return rval;
}

void	LSelection::ReceiveDataFrom(LDataTube *inTube)
{
	SelectionAboutToChange();
	
	LSelectableItem	*selItem;
	LListIterator	iter(mItems, iterate_FromStart);
	
	Assert_(inTube);

	while (iter.Next(&selItem)) {
		selItem->ReceiveDataFrom(inTube);
	}

	SelectionChanged();
}

//	===========================================================================
//	AEOM support

void	LSelection::DoAESelection(const AEDesc &inValue, Boolean inExecute)
{
	LModelObject	*super = GetSuperModel();
	
	if (super == NULL)
		return;		//	not being used in an AE context.
	
	StAEDescriptor	appleEvent;
	{
		LAEStream		s(kAEMiscStandards, kAESelect);
		
		//	keyDirectObject
		if (!super->IsDefaultSubModel()) {
			s.WriteKey(keyDirectObject);
			s.WriteSpecifier(super);
		}
		
		//	keyAEData (what to select)
		s.WriteKey(keyAEData);
		s.WriteDesc(inValue);
		
		s.Close(&appleEvent.mDesc);
	}
	
	UAppleEventsMgr::SendAppleEvent(appleEvent.mDesc, inExecute);
}

void	LSelection::DoAESelectAsResult(
	const AEDesc	&inValue,
	DescType		inPositionModifier
)
{
#if	1
	//	Till we figure out a way of recording a reference to result in a script
	StAEDescriptor		modifiedSelection,
						rubbish;

	if (inPositionModifier != typeNull) {
		if (inValue.descriptorType != typeNull) {
			switch (inPositionModifier) {
			
				//	Unknown selection modifiers
				default:
					Assert_(false);
					
				//	Normal selection modifiers.
				case kAEBefore:
				case kAEAfter:
					//	fall through
	
				//	Possibly useful selection modifiers
				case kAEBeginning:
				case kAEEnd:
					UAEDesc::MakeInsertionLoc(inValue, inPositionModifier, &modifiedSelection.mDesc);
					break;

				case kAEReplace:
					//	Why make an insertion loc when we already have the needed
					//	ospec?
					break;
				
			}
			if (modifiedSelection.mDesc.descriptorType == typeNull)
				SetAEValue(inValue, rubbish.mDesc);
			else
				SetAEValue(modifiedSelection.mDesc, rubbish.mDesc);
		}
	}
#else
	//	This is what it should be for serialized selections...
		if (inValue.descriptorType != typeNull) {
		LAESubDesc		valSD(inValue);
		
		Assert_(inValue.descriptorType == typeObjectSpecifier);
		DescType		elemType = valSD.KeyedItem(keyAEDesiredClass).ToType();
		
		//	Adjust the actual selection
		{
			StAEDescriptor	bogus;
			StAEDescriptor	modifiedSelection;
			LAEStream		stream;
			
			switch (inPositionModifier) {
		
				//	Normal position modifiers.
				case pBeginning:
				case pEnding:
				case pBefore:
				case pAfter:
					stream.OpenRecord(typeObjectSpecifier);
						stream.WriteKey(keyAEDesiredClass);
						stream.WriteTypeDesc(elemType);
						stream.WriteKey(keyAEContainer);
						stream.WriteDesc(inValue);
						stream.WriteKey(keyAEKeyForm);
						stream.WriteEnumDesc(formPropertyID);
						stream.WriteKey(keyAEKeyData);
						stream.WriteTypeDesc(inPositionModifier);
					stream.CloseRecord();
					stream.Close(&modifiedSelection.mDesc);
					SetAEValue(modifiedSelection, bogus.mDesc);
					break;
		
				case pSame:
					SetAEValue(inValue, bogus.mDesc);
					break;
				
				//	Unknown position modifier
				default:
					Assert_(false);
					break;
			}
		}
		
		//	Record using "result"
		{
			StAEDescriptor	bogus,
							event;
			LAEStream		stream(kAEMiscStandards, kAESelect);
			LModelObject	*super = GetSuperModel();
			AEDesc			currentContainer = {typeNull, NULL};
			
			if (!super->IsDefaultSubModel()) {
				stream.WriteKey(keyDirectObject);
				stream.WriteSpecifier(super);
			}
			
			//	keyAEData (what to select)
			stream.WriteKey(keyAEData);
			stream.OpenRecord(typeObjectSpecifier);
	
			switch (inPositionModifier) {
	
				case pSame:
					stream.WriteKey(keyAEDesiredClass);
					stream.WriteTypeDesc(elemType);
					stream.WriteKey(keyAEContainer);
					stream.WriteDesc(currentContainer);
					stream.WriteKey(keyAEKeyForm);
					stream.WriteEnumDesc(formPropertyID);
					stream.WriteKey(keyAEKeyData);
					stream.WriteTypeDesc(pResult);
					break;
					
				case pBeginning:
				case pEnding:
				case pBefore:
				case pAfter:
					stream.WriteKey(keyAEDesiredClass);
					stream.WriteTypeDesc(elemType);
					
					stream.WriteKey(keyAEContainer);
					stream.OpenRecord(typeObjectSpecifier);
						stream.WriteKey(keyAEDesiredClass);
						stream.WriteTypeDesc(elemType);
						stream.WriteKey(keyAEContainer);
						stream.WriteDesc(currentContainer);
						stream.WriteKey(keyAEKeyForm);
						stream.WriteEnumDesc(formPropertyID);
						stream.WriteKey(keyAEKeyData);
						stream.WriteTypeDesc(pResult);
					stream.CloseRecord();
			
					stream.WriteKey(keyAEKeyForm);
					stream.WriteEnumDesc(formPropertyID);
					stream.WriteKey(keyAEKeyData);
					stream.WriteTypeDesc(inPositionModifier);
					break;
			}
			stream.CloseRecord();
			stream.Close(&event.mDesc);
	
			//	record select event
			UAppleEventsMgr::SendAppleEvent(event.mDesc, false);
		}
	}
#endif
}

void	LSelection::RecordPresentSelection(void)
{
	StAEDescriptor	requestedType,
					value;

	{
		LAEStream		typeStream;
		
		typeStream.OpenList();
		typeStream.WriteTypeDesc(typeObjectSpecifier);
		typeStream.CloseList();
		
		typeStream.Close(&requestedType.mDesc);
	}
	
	GetAEValue(requestedType.mDesc, value.mDesc);
	DoAESelection(value.mDesc, false);
}

void	LSelection::MakeSelfSpecifier(
	AEDesc&		inSuperSpecifier,
	AEDesc&		outSelfSpecifier) const
{
	LAEStream	stream;
	
	stream.OpenRecord(typeObjectSpecifier);		
		stream.WriteKey(keyAEDesiredClass);
		stream.WriteTypeDesc(cProperty);

		stream.WriteKey(keyAEKeyForm);
		stream.WriteEnumDesc(formPropertyID);
	
		stream.WriteKey(keyAEKeyData);
		stream.WriteTypeDesc(GetModelKind());	

		stream.WriteKey(keyAEContainer);
		stream.WriteSpecifier(mSuperModel);
	stream.CloseRecord();
	
	stream.Close(&outSelfSpecifier);
}

void	LSelection::GetAEValue(
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
	if (((LModelObject *)this)->GetSuperModel() == NULL)
		return;		//	not being used in an AE context.
		
//	inRequestedType is ignored in preference to typeObjectSpecifier
	OSErr	err;
	LListIterator	iter( ((LSelection *)this)->mItems, iterate_FromStart);
	LSelectableItem	*p = NULL;
	StAEDescriptor	tempDesc,
					typeDesc;
	
	{
		LAEStream	typeStream;
		typeStream.OpenList();
		typeStream.WriteTypeDesc(typeObjectSpecifier);
		typeStream.CloseList();
		typeStream.Close(&typeDesc.mDesc);
	}
	
	if (mItems.GetCount() == 0) {
		//	Force an empty list to be made
		err = AECreateList(NULL, 0, false, &outPropertyDesc);
		ThrowIfOSErr_(err);
	}

	while (iter.Next(&p)) {
		p->GetAEProperty(pContents, typeDesc, tempDesc.mDesc);
		UAEDesc::AddDesc(&outPropertyDesc, 0, tempDesc.mDesc);
		err = AEDisposeDesc(&tempDesc.mDesc);
		ThrowIfOSErr_(err);
	}
}

class StLockListItems
{
public:
						StLockListItems(LList &inList, LModelObject *inSuperModel);
						~StLockListItems();
protected:
	LList			*mList;
	LModelObject	*mSuperModel;	//	to scope incoming items
};

StLockListItems::StLockListItems(LList &inList, LModelObject *inSuperModel)
{
	mList = &inList;
	mSuperModel = inSuperModel;
	
	LModelObject		*model;
	LSelectableItem		*p;
	
	LListIterator	iter(*mList, iterate_FromStart);
	
	while(iter.Next(&model)) {
		if (!model->IsSubModelOf(mSuperModel))
			continue;

		p = (LSelectableItem *) model;	//	unsafe cast!
//		Assert_(member(p, LSelectableItem));

		p->AddUser(this);
	}
}

StLockListItems::~StLockListItems()
{
	LModelObject		*model;
	LSelectableItem		*p;
	
	LListIterator	iter(*mList, iterate_FromStart);
	
	while(iter.Next(&model)) {
		if (!model->IsSubModelOf(mSuperModel))
			continue;

		p = (LSelectableItem *) model;	//	unsafe cast!
//		Assert_(member(p, LSelectableItem));

		p->RemoveUser(this);
	}
}

void	LSelection::SetAEValue(
	const AEDesc		&inValue,
	AEDesc&				outAEReply)
{
	LAESubDesc		value(inValue);
	LList			newItems;
	Boolean			appending = false;
	
	//	build new items list
	switch(value.GetType()) {

		case typeNull:
			break;

		case typeObjectSpecifier:
		{
			LModelObject	*model = value.ToModelObject();
			newItems.InsertItemsAt(1, arrayIndex_Last, &model);
			break;
		}
		
		case typeAEList:
		{
			Int32	n = value.CountItems(),
					i;
			
			for (i = 1; i <= n; i++) {
				LModelObject	*model = value.NthItem(i).ToModelObject();
				if (model == this) {
					if (i != 1) {
						//	Can only add to end of the selection
						ThrowOSErr_(errAEEventNotHandled);
					}
					appending = true;
					continue;
				} else {
					newItems.InsertItemsAt(1, arrayIndex_Last, &model);
				}
			}
			break;
		}
		
		case typeInsertionLoc:
		{
			DescType		position = value.KeyedItem(keyAEPosition).ToEnum();
			LModelObject	*target = value.KeyedItem(keyAEObject).ToModelObjectOptional(),
							*model;

			if (target) {
				target = target->GetInsertionTarget(position);
			} else {
				target = GetSuperModel()->GetInsertionTarget(position);
			}
			model = target->GetInsertionElement(position);
			ThrowIfNULL_(model);
			newItems.InsertItemsAt(1, arrayIndex_Last, &model);
			break;
		}
		
		default:
			ThrowOSErr_(errAEBadKeyForm);
			break;
	}

	//	Make the change
	StSelectionChanger	changeMe(*this);
	StLockListItems		lockThem(newItems, GetSuperModel());	//	The clear below might delete lazies in the newItems list before they get added.
	{
		LModelObject		*model;
		LSelectableItem		*p,
							*q;
		
		//	The clear below might purge lazies in our newItems list -- so lock them down.
		if (!appending)
			ListClear();
		
		LListIterator	iter(newItems, iterate_FromStart);
		
		while(iter.Next(&model)) {

			//	Fix me!!!
			//	it should be IsSubModelOf(mTextLink || mTableModel);
			if (!model->IsSubModelOf(GetSuperModel()))
				Throw_(paramErr);	//	Should read "this object cant be used here."

			p = (LSelectableItem *) model;	//	unsafe cast!
//			Assert_(member(p, LSelectableItem));

			q = ListDependentItem(p);
			if (q) {
				ListRemoveItem(q);
			} else {
				ListAddItem(p);
			}
		}
	}
}

void	LSelection::GetImportantAEProperties(AERecord &outKeyDescList) const
/*
	Get all important AE property data for referred objects.
	
	"Important" includes things necessary for cloning.
*/
{
	const LSelectableItem	*item;
	OSErr					err = noErr;
	StAEDescriptor			contents,
							reqType;
	Int32					i;
	
	switch (ListCount()) {
		case 0:
			break;
		
		case 1:
			item = ListNthItem(1);
			item->GetImportantAEProperties(outKeyDescList);
			break;
			
		default:
		//	group the properties by item
		{
			LAEStream	stream;
			
			stream.OpenList();
			for (i = 1; i <= ListCount(); i++) {
				StAEDescriptor	itemProps;
				
				item = ListNthItem(i);
				item->GetImportantAEProperties(itemProps.mDesc);
				stream.WriteDesc(itemProps);
			}
			stream.CloseList();
			stream.Close(&outKeyDescList);
			break;
		}
	}
}

void	LSelection::GetContentAEKinds(AEDesc &outKindList)
{
	LSelectableItem	*item;
	OSErr			err = noErr;
	DescType		kind;
	Int32			i;
	
	for (i = 1; i <= ListCount(); i++) {
		item = ListNthItem(i);
		Assert_(item);
//		Assert_(member(item, LSelectableItem));
		kind = item->GetModelKind();
		UAEDesc::AddPtr(&outKindList, 0, typeType, &kind, sizeof(kind));
	}
}

void	LSelection::HandleDelete(
	AppleEvent			&outAEReply,
	AEDesc				&outResult)
/*
	It makes no sense to delete the selection object but it does make sense
	to delete the objects referenced by it.
	
	This routine will set the keyAEInsertHere parameter of the reply to
	the result of the object's delete or multiple object deletes.  In the 
	latter case, a descriptor list will be returned in keyAEInsertHere.
*/
{
	StAEDescriptor	cumulativeResult;
	LSelectableItem	*p;
	LListIterator	iter(mItems, iterate_FromStart);
	
	while(iter.Next(&p)) {
		StAEDescriptor	objectResult,
						objectReply,
						bogus;

		Assert_(p);
		p->HandleDelete(objectReply.mDesc, bogus.mDesc);

		//	accumulate replies
		objectResult.GetParamDesc(objectReply.mDesc, keyAEInsertHere, typeWildCard);
		if (objectResult.mDesc.descriptorType != typeNull)
			UAEDesc::AddDesc(&cumulativeResult.mDesc, 0, objectResult.mDesc);
	}

	SelectSimple(NULL);
	
	if (cumulativeResult.mDesc.descriptorType != typeNull)
		UAEDesc::AddKeyDesc(&outAEReply, keyAEInsertHere, cumulativeResult);
}

void	LSelection::GetAEProperty(
	DescType		inProperty,
	const AEDesc	&inRequestedType,
	AEDesc			&outPropertyDesc) const
{
	Assert_(outPropertyDesc.descriptorType == typeNull);
	
	switch (inProperty) {

		case pShowAllSelections:
			UAEDesc::MakeBooleanDesc(mRecordAllSelections, &outPropertyDesc);
			break;

		break;

	}
	
	if (outPropertyDesc.descriptorType == typeNull)
		LSelectableItem::GetAEProperty(inProperty, inRequestedType, outPropertyDesc);
}

void	LSelection::SetAEProperty(
	DescType		inProperty,
	const AEDesc	&inValue,
	AEDesc&			outAEReply)
{
	switch (inProperty) {

		case pShowAllSelections:
		{
			LAESubDesc	value(inValue);
			mRecordAllSelections = value.ToBoolean();
			break;
		}

		default:
			LSelectableItem::SetAEProperty(inProperty, inValue, outAEReply);
			break;
	}
}

//	===========================================================================
//	List maintenance

void	LSelection::SelectionAboutToChange(void)
{
	BroadcastMessage(msg_SelectionAboutToChange);
}

void	LSelection::SelectionChanged(void)
{
	mSelectionID++;
	if (mSelectionID <= 0)
		mSelectionID = 1;	//	this will rarely happen
		
	BroadcastMessage(msg_SelectionChanged);
}

void	LSelection::ListClear(void)
{
	LListIterator		iter(mItems, iterate_FromEnd);
	LSelectableItem		*p;
	
	while(iter.Previous(&p)) {
		ListRemoveItem(p);
	}
	
	ReplaceSharable(mAnchor, (LSelectableItem *)NULL, this);
}

Int32	LSelection::ListCount(void) const
{
	return (mItems.GetCount());
}

LSelectableItem *	LSelection::ListNthItem(Int32 inIndex)
{
	LSelectableItem	*rval = NULL;
	
	mItems.FetchItemAt(inIndex, &rval);
	return rval;
}

const LSelectableItem *	LSelection::ListNthItem(Int32 inIndex) const
{
	LSelectableItem	*rval = NULL;
	
	mItems.FetchItemAt(inIndex, &rval);
	return rval;
}

Boolean	LSelection::ListContains(const LSelectableItem *inItem) const
/*
	Make sure you shouldn't be using ListDependentWith
*/
{
	LListIterator		iter(((LSelection *)this)->mItems, iterate_FromStart);
	LSelectableItem		*p;
	
	while(iter.Next(&p)) {
		if (p == inItem)
			return true;
	}
	return NULL;
}
		
void	LSelection::ListAddItem(LSelectableItem *inItem)
/*
	Only call directly if you nest the call between calls to SelectionAboutToChange
	and SelectionChanged.
*/
{
	if (inItem != NULL && !ListContains(inItem)) {

		mItems.InsertItemsAt(1, arrayIndex_Last, &inItem);
		inItem->AddUser(this);
	}
}

void	LSelection::ListRemoveItem(LSelectableItem *inItem)
/*
	Only call directly if you nest the call between calls to SelectionAboutToChange
	and SelectionChanged.
*/
{
	Int32	index;
	
	Assert_(inItem);
	index = mItems.FetchIndexOf(&inItem);
	Assert_(index);
	
	mItems.RemoveItemsAt(1, index);
	inItem->RemoveUser(this);
}

//	===========================================================================
//	Clipboard support...
		
#include	<LAEAction.h>

LAction *	LSelection::MakeCutAction(void)
{
	StAEDescriptor		aeTarget,
						aeUndoData;
	LAEAction			*action;
	LModelObject		*super = GetSuperModel();
	
	Assert_(super);
	
	action = new LAEAction(STRx_RedoEdit, str_Cut);
	action->SetSelection(this);
	
	//	Redo
	action->SetRedoAE(kAEMiscStandards, kAECut);
	
	//	Undo
	UndoAddOldData(action);
	
	return action;
}

LAction *	LSelection::MakeCopyAction(void)
{
	StAEDescriptor		aeTarget,
						aeUndoData,
						temp;
	LAEAction			*action;
	LModelObject		*super = GetSuperModel();
	
	Assert_(super);
	
	action = new LAEAction(STRx_RedoEdit, str_Copy);
	action->SetSelection(this);
	
	//	Redo
	action->SetRedoAE(kAEMiscStandards, kAECopy);
	
	//	Undo -- nothing to undo
	return action;
}

LAction *	LSelection::MakePasteAction(void)
{
	StAEDescriptor		aeTarget;
	LAEAction			*action;
	LModelObject		*super = GetSuperModel();
	
	Assert_(super);
	
	action = new LAEAction(STRx_RedoEdit, str_Paste);
	action->SetSelection(this);

	//	Redo
	action->SetRedoAE(kAEMiscStandards, kAEPaste);

	//	Undo
	UndoAddOldData(action);

	return action;
}

LAction *	LSelection::MakeClearAction(void)
{
	StAEDescriptor		aeTarget,
						aeUndoData,
						temp;
	LAEAction			*action;
	
	this->GetAEValue(temp.mDesc, aeTarget.mDesc);
	
	action = new LAEAction(STRx_RedoEdit, str_Clear);
	action->SetSelection(this);

	//	Redo
	action->SetRedoAE(kAECoreSuite, kAEDelete);
	action->RedoAEAdd(keyDirectObject, aeTarget.mDesc);

	//	Undo
	UndoAddOldData(action);
	action->UndoAESetKeyFed(keyAEInsertHere, keyAEInsertHere);
	
	return action;
}

void	LSelection::UndoAddOldData(LAEAction *inAction)
{
	StAEDescriptor		aeTarget,
						aeUndoData,
						aeClass;
	OSErr				err;
	
	err = AECreateList(NULL, 0, true, &aeUndoData.mDesc);
	ThrowIfOSErr_(err);
	GetImportantAEProperties(aeUndoData.mDesc);
	GetContentAEKinds(aeClass.mDesc);
	
	inAction->SetUndoAE(kAECoreSuite, kAECreateElement);
	inAction->UndoAEAdd(keyAEObjectClass, aeClass.mDesc);
	inAction->UndoAEAdd(keyAEPropData, aeUndoData.mDesc);
	inAction->UndoAESetKeyFed(keyAEInsertHere);
}

StSelectionChanger::StSelectionChanger(LSelection &inSelection)
{
	mSelection = &inSelection;
	
	mSelection->SelectionAboutToChange();
}

StSelectionChanger::~StSelectionChanger()
{
	mSelection->SelectionChanged();
}


