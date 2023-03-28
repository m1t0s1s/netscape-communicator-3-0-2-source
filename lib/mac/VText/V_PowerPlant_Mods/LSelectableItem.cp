//	===========================================================================
//	LSelectableItem.cp					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LSelectableItem.h"
#include	<UAppleEventsMgr.h>
#include	<UMemoryMgr.h>
#include	<LDataTube.h>

#ifndef		__AEREGISTRY__
#include	<AERegistry.h>
#endif

#ifndef		__AEOBJECTS__
#include	<AEObjects.h>
#endif

#pragma	warn_unusedarg off

//	===========================================================================
//	Maintenance

LSelectableItem::LSelectableItem()
{
}

LSelectableItem::LSelectableItem(LModelObject *inSuperModel, DescType inKind)
	:	inheritAEOM(inSuperModel, inKind)
{
}

LSelectableItem::~LSelectableItem()
{
}

void	LSelectableItem::AddUser(void *inUser)
{
	inheritAEOM::AddUser(inUser);
}

void	LSelectableItem::Finalize()
{
	inheritAEOM::Finalize();
}

void	LSelectableItem::SuperDeleted()
{
	inheritAEOM::SuperDeleted();
}

//	===========================================================================
//	Query (override)

Boolean	LSelectableItem::IsSubstantive(void) const
/*
	Returns whether this selectable item has any real data associated with
	it (GetImportantAEProperties) or if it is just a "placeholder" (insertion
	point).
	
	OVERRIDE.
*/
{
	return false;
}

Boolean	LSelectableItem::EquivalentTo(const LSelectableItem *inItem) const
/*
	Returns whether this is equivalent to inItem.
	
	Equivalent means the model objects refer to the same set of 
	actual objects.

	
	OVERRIDE for lazy objects.
*/
{
	return this == inItem;
}

Boolean	LSelectableItem::AdjacentWith(const LSelectableItem *inItem) const
/*
	Returns whether this is shares a border with inItem.
	
	Adjacent means the model objects "boundaries" meet.

	
	OVERRIDE (with something like):
	
		Boolean	rval = inherited::AdjacentWith(inItem);
		
		if (!rval) {
			rval = some application specific condition
		}
		
		return rval;
*/
{
	return this == inItem;	//	An item shares borders with itself.
}

Boolean	LSelectableItem::IndependentFrom(const LSelectableItem *inItem) const
/*
	Returns whether this is independent from inItem.
	
	Independent means none of the set of actual objects indicated by
	this are represented in the set indicated by inItem.
	
	OVERRIDE (with something like):
	
		Boolean	rval = inherited::IndependentFrom(inItem);
		
		if (rval) {
			rval = some application specific condition
		}
		
		return rval;
*/
{
	return this != inItem;
}

LSelectableItem *	LSelectableItem::FindExtensionItem(const LSelectableItem *inItem) const
/*
	Returns the item that represents all items from "this" to inItem in a
	selection extension scenario.
	
	If the extension doesn't exist or can't be found, NULL is returned.
*/
{
	return NULL;
}

//	===========================================================================
//	Visual (override)

void	LSelectableItem::DrawSelfSelected(void)
{
	Assert_(false);
}

void	LSelectableItem::DrawSelfLatent(void)
{
	Assert_(false);
}

void	LSelectableItem::UnDrawSelfSelected(void)
{
	DrawSelfSelected();
}

void	LSelectableItem::UnDrawSelfLatent(void)
{
	DrawSelfLatent();
}

Int32		LSelectableItem::sReceiverTicker;
Boolean		LSelectableItem::sReceiverOn;

void	LSelectableItem::DrawSelfReceiver(void)
/*
	Override but inherit at end of overriden method to have "receiver" blink.
*/
{
	sReceiverTicker = TickCount();
	sReceiverOn = true;
}

void	LSelectableItem::UnDrawSelfReceiver(void)
{
	if (sReceiverOn)
		DrawSelfReceiver();

	sReceiverTicker = TickCount();
	sReceiverOn = false;
}

void	LSelectableItem::DrawSelfReceiverTick(void)
{
	if ((TickCount() - sReceiverTicker) > GetCaretTime()) {
		if (sReceiverOn)
			UnDrawSelfReceiver();
		else
			DrawSelfReceiver();
	}
}

//	===========================================================================
//	Data dragging

void	LSelectableItem::AddFlavorTypesTo(LDataTube *inTube) const
/*
	Promises the object specifier for this item.
	
	When overriding, remember to "inherit" this method.
*/
{
	inTube->AddFlavor(typeObjectSpecifier);

/*	Subclass template:

	inTube->AddFlavor(<your data type 1>);
	inTube->AddFlavor(<your data type 2>);
	...
	
	inherited::AddFlavorTypesTo(inTube);
*/
}

Boolean	LSelectableItem::SendFlavorTo(FlavorType inFlavor, LDataTube *inTube) const
/*
	When overriding, remember to "inherit" this method.
	
	This method also serves as a template for subclass methods.
*/
{
	Int32	size = 0;
	
	switch(inFlavor) {

		case typeObjectSpecifier:
		{
			StAEDescriptor	desc;

			MakeSpecifier(desc.mDesc);
			inTube->SetFlavorHandle(inFlavor, desc.mDesc.dataHandle);
			return true;
			break;
		}
/*
		case <your data type>:
			inTube->SetFlavorData(inFlavor, ptr, size);
			break;
		
		default:
			return inherited::SendFlavorTo(inTube);
			break;
*/
	}
	return false;
}
