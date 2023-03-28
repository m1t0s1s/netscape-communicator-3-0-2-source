//	===========================================================================
//	LDragTube.cp					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

//	For routine descriptions see LDataTube.cp.

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LDragTube.h"
#include	<UMemoryMgr.h>
#include	<LDynamicArray.h>
#include	<LTubeableItem.h>

#ifndef __ERRORS__
#include	<Errors.h>
#endif

#pragma	warn_unusedarg off

Boolean	LDragTube::sSendingData = false;

LDragTube::LDragTube(
	DragReference	inDragRef)
{
	mDragRef = inDragRef;

	mSwapDepth = 2;

	mAddSendsData = false;
}

LDragTube::LDragTube(
	DragReference		inDragRef,
	const LTubeableItem	*inTubeableItem,
	Boolean				inReqdFlavorsOnly
)
{
	mDragRef = inDragRef;
	
	mSwapDepth = 2;
	
	mAddSendsData = false;

	FillFrom(inTubeableItem, inReqdFlavorsOnly);
}

LDragTube::~LDragTube()
{
}

//	----------------------------------------------------------------------
//	Read:

Boolean	LDragTube::FlavorExists(FlavorType inFlavor) const
{
	Boolean	rval = false;
	
	if (mTubeDepth == mSwapDepth) {
		OSErr		err;
		FlavorFlags	flags;
		
		err = GetFlavorFlags(mDragRef, mItemRef, inFlavor, &flags);
		
		if (err == noErr)
			rval = true;
		else if (err == badDragFlavorErr)
			;
		else
			ThrowIfOSErr_(err);

	} else {
		rval = inherited::FlavorExists(inFlavor);
	}
	
	return rval;
}

Size	LDragTube::GetFlavorSize(FlavorType inFlavor) const
{
	Size	rval = 0;
	
	if (mTubeDepth == mSwapDepth) {
		OSErr		err;
		Size		dataSize;
		
		err = GetFlavorDataSize(mDragRef, mItemRef, inFlavor, &dataSize);
		
#if	0
		//	for debugging...
		if (err != noErr) {
			OSErr			err2;
			short			i;
			unsigned short	numItems;
			ItemReference	item;
			LTubeableItem	*tubeItem;
			
			Assert_(false);
			err2 = CountDragItems(mDragRef, &numItems);
			
			for (i = 1; i <= numItems; i++)  {
				err2 = GetDragItemReferenceNumber(mDragRef, i, &item);
				tubeItem = (LTubeableItem *) item;
			}
		}
#endif

		ThrowIfOSErr_(err);
		
		rval = dataSize;
	} else {
		rval = inherited::GetFlavorSize(inFlavor);
	}
	
	return rval;
}

void *	LDragTube::GetFlavorData(
	FlavorType	inFlavor,
	void		*outFlavorData
) const
{
	void	*rval = NULL;
	
	if (mTubeDepth == mSwapDepth) {
		OSErr		err;
		Size		preDataSize, dataSize;
		
		Assert_(outFlavorData);
		
		preDataSize = dataSize = GetFlavorSize(inFlavor);
		
		err = ::GetFlavorData(mDragRef, mItemRef, inFlavor, outFlavorData, &dataSize, 0);
		ThrowIfOSErr_(err);
		
		Assert_(dataSize == preDataSize);
		
		rval = NULL;
	} else {
		rval = inherited::GetFlavorData(inFlavor, outFlavorData);
	}
	
	return rval;
}

Int32	LDragTube::CountItems(void) const
{
	Int32	rval = 0;

	if (mTubeDepth == mSwapDepth-1) {
		Uint16	count = 0;
		OSErr	err = CountDragItems(mDragRef, &count);
		ThrowIfOSErr_(err);
		rval = count;
	} else {
		rval = inherited::CountItems();
	}
	
	return rval;
}

LDataTube	LDragTube::KeyedItem(ItemReference inItemRef) const
{
	if (mTubeDepth == mSwapDepth-1) {		
		OSErr			err;
		LAESubDesc		sd;
		
		do {
			FlavorFlags		flags;
			Size			dataSize = 0;

			err = GetFlavorFlags(mDragRef, inItemRef, typeTubeMapItems, &flags);
			if (err == badDragFlavorErr)
				break;
			ThrowIfOSErr_(err);
			
			err = GetFlavorDataSize(mDragRef, inItemRef, typeTubeMapItems, &dataSize);
			ThrowIfOSErr_(err);			
			if (!dataSize)
				Throw_(paramErr);
				
			//	Get typeTubeMapItems data
			StHandleBlock	h(dataSize);
			StHandleLocker	lock(h);
			err = ::GetFlavorData(mDragRef, inItemRef, typeTubeMapItems, *(h.mHandle), &dataSize, 0);
			ThrowIfOSErr_(err);
			
			//	Swap mOwnedDesc
			AEDesc	*desc = &(((LDragTube *)this)->mOwnedDesc.mDesc);
			AEDisposeDesc(desc);
			desc->descriptorType = typeAERecord;
			desc->dataHandle = h.mHandle;
			h.mHandle = NULL;			
			sd = LAESubDesc(*desc);
		} while (false);
		
		return LDataTube(this, inItemRef, sd, mTubeDepth + 1);
		
	} else {

		return inherited::KeyedItem(inItemRef);

	}
}

LDataTube	LDragTube::NthItem(Int32 inIndex, ItemReference *outReference) const
{
	if (mTubeDepth == mSwapDepth-1) {
		ItemReference	ref;
		
		OSErr err = GetDragItemReferenceNumber(mDragRef, inIndex, &ref);
		ThrowIfOSErr_(err);
	
		if (outReference)
			*outReference = ref;
		
		return KeyedItem(ref);

	} else {

		return inherited::NthItem(inIndex, outReference);

	}
}

//	----------------------------------------------------------------------

Int32	LDragTube::FlavorCount(void) const
{
	Int32	rval = 0;
	OSErr	err;
	
	if (mTubeDepth == mSwapDepth) {
		Uint16	flavors = 0;
		
		err = CountDragItemFlavors(mDragRef, mItemRef, &flavors);
		ThrowIfOSErr_(err);
		
		if (FlavorExists(typeTubeMapItems))
			flavors--;
		
		rval = flavors;			
	} else {
		rval = inherited::FlavorCount();
	}
	
	return rval;
}

FlavorType	LDragTube::NthFlavor(Int32 inIndex) const
{
	OSErr		err;
	FlavorType	rval = typeNull;
	
	if (mTubeDepth == mSwapDepth) {
		FlavorType	keyFound;
		Int32		index = 0;
		
		Assert_((0 < inIndex) && (inIndex <= FlavorCount()));
		
		do {
			index++;
			err = GetFlavorType(mDragRef, mItemRef, index, &keyFound);
			ThrowIfOSErr_(err);
			
			if (keyFound == typeTubeMapItems)
				index++;
			inIndex--;
		} while (inIndex > 0);
		
		rval = keyFound;
	} else {
		rval = inherited::NthFlavor(inIndex);
	}
	
	return rval;
}

//	----------------------------------------------------------------------
//	Write:

void	LDragTube::AddFlavor(FlavorType inFlavor)
{
	if (mItemStack.Depth() == mSwapDepth) {	
		OSErr	err;
		
		err = AddDragItemFlavor(mDragRef, mItemStack.Top().ref, inFlavor, NULL, 0, 0);
		ThrowIfOSErr_(err);
	} else {
		inherited::AddFlavor(inFlavor);
	}
}

void	LDragTube::SetFlavorData(
	FlavorType	inFlavor,
	const void	*inFlavorData,
	Int32		inFlavorSize
)
{
	if (!mItemStack.Depth() && sSendingData) {
		OSErr	err = SetDragItemFlavorData(mDragRef, mItemRef, inFlavor, inFlavorData, inFlavorSize, 0);
		ThrowIfOSErr_(err);	//	If -48 see note above.
	} else if ((mItemStack.Depth() == mSwapDepth)) {
		OSErr		err;
		
		//	for debugging
		#if	0
		{
			unsigned short	count,
							i;
			ItemReference	item;
			Boolean			found = false;
			
			err = CountDragItems(inDrag, &count);
			ThrowIfOSErr_(err);
			
			for (i = 1; i <= count; i++) {
				err = GetDragItemReferenceNumber(inDrag, i, &item);
				ThrowIfOSErr_(err);
				
				if (item == (ItemReference) this) {
					found = true;
					break;
				}
			}
			
			Assert_(found);	//	Discovery!  If this fails or something above
							//	throws a -48 error, it is most likely a sign
							//	the Drag Mgr has died -- time to reboot.
		}
		#endif
		
		err = SetDragItemFlavorData(mDragRef, mItemStack.Top().ref, inFlavor, inFlavorData, inFlavorSize, 0);
		ThrowIfOSErr_(err);	//	If -48 see note above.
	} else {
		inherited::SetFlavorData(inFlavor, inFlavorData, inFlavorSize);
	}
}

void	LDragTube::OpenItem(
			const LTubeableItem	*inItem,
			ItemReference		inItemRef
)
{
	if (mTubeDepth == mSwapDepth) {
		if (inItemRef != defaultItemReference)
			Throw_(paramErr);	//	Sorry, but you can't specify item ref's w/ base level drag items.
								//	To do so would require LDragTube to keep a cache of tubeable items
								//	and tubable item refs -- all to fulfill delayed "gets" of flavors.
	}
	
	if (mTubeDepth >= mSwapDepth)
		mAddSendsData = true;
		
	inherited::OpenItem(inItem, (ItemReference) inItem);
}

void	LDragTube::CloseItem(void)
{
	if (mTubeDepth <= mSwapDepth)
		mAddSendsData = false;
	
	inherited::CloseItem();
}

//	----------------------------------------------------------------------

void	LDragTube::SendFlavorData(ItemReference inItemRef, FlavorType inFlavor)
{
	LTubeableItem		*item = (LTubeableItem *) inItemRef;

	ThrowIfNULL_(item);
//	Assert_(member(item, LTubeableItem));

	Assert_(!mItemStack.Depth());
	
	StValueChanger<Int32>	change(mItemRef, inItemRef);
	StValueChanger<Boolean>	change2(sSendingData, true);
	
	item->SendFlavorTo(inFlavor, this);
}

