//	===========================================================================
//	LDescTube.cp				©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

//	For routine descriptions see LDataTube.cp.

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LDescTube.h"
#include	<UMemoryMgr.h>
#include	<LTubeableItem.h>
#include	<LTubeStreams.h>
#include	<URange32.h>


#pragma	warn_unusedarg off

LDescTube::LDescTube()
{
	mReqdFlavorsOnly = false;
	mAddSendsData = true;

	mNextRef = 0x10000;
	mDesc = &mOwnedDesc.mDesc;
	
	mStream = NULL;
}

LDescTube::LDescTube(const AEDesc &inTubeDesc)
{
	mDesc = (AEDesc *) &inTubeDesc;
	mTubeMap = LAESubDesc(*mDesc);
	
	mReqdFlavorsOnly = false;
	mAddSendsData = true;
	
	mNextRef = 0x10000;
	
	mStream = NULL;
}

LDescTube::LDescTube(
	AEDesc				*ioTubeDesc,
	const LTubeableItem	*inTubeableItem,
	Boolean				inReqdFlavorsOnly
)
/*
	Don't call this constructor from a more derived tube -- you want
	FillFrom to be called when the tube is fully constructed
*/
{
	mDesc = ioTubeDesc;
	mAddSendsData = true;

	mNextRef = 0x10000;
	
	mStream = NULL;
	
	FillFrom(inTubeableItem, inReqdFlavorsOnly);
}

LDescTube::~LDescTube()
{
	delete mStream;
	mStream = NULL;
	
	while (mItemStack.Depth()) {
		if (	(mItemStack.Depth() == 1) ||
				(mItemStack.Top().stream != mItemStack.PeekBack(1).stream)
		) {
			delete mItemStack.Top().stream;
			mItemStack.Top().stream = NULL;
		}
		mItemStack.Pop();
	}
}

Boolean	LDescTube::GetOnlyReqdFlavors(void) const
{
	return mReqdFlavorsOnly;
}

void	LDescTube::FillFrom(const LTubeableItem *inItem, Boolean inReqdFlavorsOnly)
{
	mReqdFlavorsOnly = inReqdFlavorsOnly;
	
	if (inItem) {
		OpenItem(inItem);
		inItem->AddFlavorTypesTo(this);
		CloseItem();
		
		if (mItemStack.Depth() != 0)
			Throw_(paramErr);	//	nesting error
	}
}

//	----------------------------------------------------------------------
//	Read:

Boolean	LDescTube::FlavorExists(FlavorType inFlavor) const
{
	Boolean rval = false;
	
	if (inFlavor == typeWildCard) {
		switch (mTubeMap.CountItems()) {

			case 0:
				break;
			
			case 1:
				if (!mTubeMap.KeyExists(typeTubeMapItems))
					rval = true;
				break;
			
			default:
				rval = true;
				break;
		}
	} else {
		rval = mTubeMap.KeyExists(inFlavor);
	}
	
	return rval;	
}

Size	LDescTube::GetFlavorSize(FlavorType inFlavor) const
{
	Size	rval = 0;
	
	LAESubDesc	flavor = mTubeMap.KeyedItem(inFlavor);
	
	if (flavor.GetType() != typeNull)
		rval = flavor.GetDataLength();

	return rval;
}

void *	LDescTube::GetFlavorData(
	FlavorType	inFlavor,
	void		*outFlavorData
) const
//	For routine description see LDataTube.cp.
{
	if (inFlavor == typeTubeMapItems)
		Throw_(paramErr);
		
	Int32		amount = GetFlavorSize(inFlavor);
	void		*source = NULL;
	LAESubDesc	flavor = mTubeMap.KeyedItem(inFlavor);
	
	if (flavor.GetType() == typeNull)
		Throw_(noTypeErr);

	source = flavor.GetDataPtr();
	if (source) {
		source = (Int8 *) source;
		if (outFlavorData && amount)
			BlockMoveData(source, outFlavorData, amount);
	}

	if (outFlavorData)
		return NULL;
	else
		return source;
}

LStream &	LDescTube::GetFlavorStreamOpen(FlavorType inFlavor) const
{
	LDescTube	&my = (LDescTube&)*this;
	
	if (mStream)
		Throw_(paramErr);	//	presently you can have a total of only one flavor stream open at a time
	
	my.mStreamFlavor = inFlavor;
	my.mStream = new LAESubDescReadStream(mTubeMap.KeyedItem(mStreamFlavor));
	return *mStream;
}

void	LDescTube::GetFlavorStreamClose(void) const
{
	LDescTube	&my = (LDescTube&)*this;

	if (!mStream)
		Throw_(paramErr);	//	close without balanced open

	delete mStream;
	my.mStream = NULL;
}

Int32	LDescTube::CountItems(void) const
{
	Int32	rval = 0;
	
	if (mTubeMap.KeyExists(typeTubeMapItems))
		rval = mTubeMap.KeyedItem(typeTubeMapItems).CountItems();

	return	rval;
}

LDataTube	LDescTube::NthItem(Int32 inIndex, ItemReference *outReference) const
{
	AEKeyword	key = 0;
	LAESubDesc	item = mTubeMap.KeyedItem(typeTubeMapItems).NthItem(inIndex, &key);
	
	if (item.GetType() == typeNull)
		Throw_(paramErr);	//	non existent item
	
	if (outReference)
		*outReference = key;
		
	return LDataTube(this, key, item, mTubeDepth + 1);
}

LDataTube	LDescTube::KeyedItem(ItemReference inItemRef) const
{
	LAESubDesc	item = mTubeMap.KeyedItem(typeTubeMapItems).KeyedItem(inItemRef);
	
	if (item.GetType() == typeNull)
		Throw_(paramErr);	//	non existent item
		
	return LDataTube(this, inItemRef, item, mTubeDepth + 1);
}

//	----------------------------------------------------------------------
//	Generic functions:

void	LDescTube::WriteDesc(LAEStream *inStream) const
{
	if (mItemStack.Depth() != 0)
		Throw_(paramErr);	//	Tube isn't finished
	
	inStream->WriteSubDesc(mTubeMap);
}

Int32	LDescTube::FlavorCount(void) const
{
	Int32	rval = mTubeMap.CountItems();
	
	if (mTubeMap.KeyExists(typeTubeMapItems))
		rval--;
		
	return rval;
}

FlavorType	LDescTube::NthFlavor(Int32 inIndex) const
{
	DescType	rval;
	AEKeyword	keyFound;
	Int32		index = 0;
	
	Assert_((0 < inIndex) && (inIndex <= FlavorCount()));
	
	do {
		index++;
		mTubeMap.NthItem(index, &keyFound);
		if (keyFound == typeTubeMapItems)
			index++;
		inIndex--;
	} while (inIndex > 0);
	
	mTubeMap.NthItem(index, &rval);

	return rval;
}

//	----------------------------------------------------------------------
//	Write:

void	LDescTube::AddFlavor(FlavorType inFlavor)
{
	if (inFlavor == typeTubeMapItems)
		Throw_(paramErr);	//	typeTubeMapItems is reserved.
		
	if (inFlavor == typeNull)
		Throw_(paramErr);	//	typeNull is reserved.
		
	if (!mItemStack.Depth())
		Throw_(paramErr);	//	An item must be opened first!
	
	if (mItemStack.Top().subCt)
		Throw_(paramErr);	//	Can't add item flavors after starting to add sub items
	
	//	Write flavor key
	if (mItemStack.Top().stream)
		mItemStack.Top().stream->WriteKey(inFlavor);
	
	//	Write flavor data?
	if (mItemStack.Top().item && mAddSendsData) {
		mItemStack.Top().item->SendFlavorTo(inFlavor, this);
	} else {
		StAEDescriptor	nullDesc;
		if (mItemStack.Top().stream)
			mItemStack.Top().stream->WriteDesc(nullDesc);
	}
}

void	LDescTube::SetFlavorData(
	FlavorType	inFlavor,
	const void	*inFlavorData,
	Int32		inFlavorSize
)
//	For routine description see LDataTube.cp.
{
	mItemStack.Top().stream->OpenDesc(inFlavor);

	mItemStack.Top().stream->WriteData(inFlavorData, inFlavorSize);
	
	mItemStack.Top().stream->CloseDesc();
}

LStream &	LDescTube::SetFlavorStreamOpen(FlavorType inFlavor)
{
	if (mStream)
		Throw_(paramErr);	//	presently you can have a total of only one flavor stream open at a time
	
	mStreamFlavor = inFlavor;
	
	mItemStack.Top().stream->OpenDesc(mStreamFlavor);
	mStream = new LAEStreamWriteStream(mItemStack.Top().stream);
	
	return *mStream;
}

void	LDescTube::SetFlavorStreamClose(void)
{
	if (!mStream)
		Throw_(paramErr);	//	close without balanced open
	
	mItemStack.Top().stream->CloseDesc();
	delete mStream;
	mStream = NULL;
}

void	LDescTube::OpenItem(
	const LTubeableItem	*inItem,
	ItemReference		inItemRef
)
{
	//	Make stack record
	ItemRecT	r;
	
	r.item = NULL;
	r.ref = (inItemRef == defaultItemReference) ? mNextRef++ : inItemRef;
	r.subCt = 0;
	r.stream = 0;
		
	if (mItemStack.Depth()) {
		if (mItemStack.Top().stream) {
			//	New group of items?
			if (!mItemStack.Top().subCt) {
				mItemStack.Top().stream->WriteKey(typeTubeMapItems);
				mItemStack.Top().stream->OpenRecord();	//	of subitems
			}
			mItemStack.Top().stream->WriteKey(r.ref);	//	of item	
		}		

		mItemStack.Top().subCt++;
	}

	//	Push it
	mItemStack.Push(r);
	SetItem((LTubeableItem *)inItem);
	NewTop();
	
	if (mItemStack.Top().stream)
		mItemStack.Top().stream->OpenRecord();	//	of item flavors
}

void	LDescTube::CloseItem(void)
{
	if (!mItemStack.Depth())
		Throw_(paramErr);
	
	if (mItemStack.Top().stream) {
		if (mItemStack.Top().subCt)
			mItemStack.Top().stream->CloseRecord();	//	of subitems
		
		mItemStack.Top().stream->CloseRecord();		//	of item flavors
	}
	
	OldTop();
	
	//	Don't leak an LAEStream!
	Assert_(!mItemStack.Top().stream ||
			(mItemStack.Depth() >= 2) && (mItemStack.Top().stream == mItemStack.PeekBack(1).stream)
	);
	
	mItemStack.Pop();
}

void	LDescTube::SetItem(LTubeableItem *inItem)
{
	Assert_(mItemStack.Depth());
	
	mItemStack.Top().item = inItem;
}

void	LDescTube::NewTop(void)
{
	if (mStream)
		Throw_(paramErr);	//	Flavor stream nesting error
		
	if (mItemStack.Depth() == 1)
		mItemStack.Top().stream = new LAEStream;
}

void	LDescTube::OldTop(void)
{
	if (mStream)
		Throw_(paramErr);	//	Flavor stream nesting error
		
	if (mItemStack.Depth() == 1) {
//?		Assert_(mDesc->descriptorType == typeNull);
		mItemStack.Top().stream->Close(mDesc);
		delete mItemStack.Top().stream;
		mItemStack.Top().stream = NULL;
	}
}

//	----------------------------------------------------------------------
//	stack template instantiation...

#include	<LStack.t>

template class	LStack<ItemRecT>;
