//	===========================================================================
//	LClipboardTube.cp				©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

//	For routine descriptions see LDataTube.cp.

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include "LClipboardTube.h"
#include <UMemoryMgr.h>
#include <LDynamicArray.h>
#include <LTubeableItem.h>
#include <UAEGizmos.h>
#include <URange32.h>

#ifndef __SCRAP__
#include <Scrap.h>
#endif

#ifndef __ERRORS__
#include <Errors.h>
#endif

#pragma	warn_unusedarg off

LClipboardTube::LClipboardTube()
/*
	Create a tube from existing data on the clipboard.
*/
{
	OSErr		err = noErr;
	ScrapStuffPtr	p = InfoScrap();

	ThrowIfNULL_(p);
	
	if (!p->scrapHandle)
		err = LoadScrap();
	ThrowIfOSErr_(err);
	
	p = InfoScrap();
	ThrowIfNULL_(p);
	mScrapHandle = p->scrapHandle;
	ThrowIfNULL_(mScrapHandle);
	
	//	Get or construct tube map.
	Int32	n = UScrap::CountItems(mScrapHandle, typeTubeMapItems);
	Assert_(n <= 1);
	if (n) {
		StHandleLocker		lock(mScrapHandle);
		ScrapRecT			*timi = UScrap::NthItem(mScrapHandle, typeTubeMapItems, 1);
		ThrowIfNULL_(timi);

#if	1
		mOwnedDesc.mDesc.dataHandle = NewHandle(timi->size);
		ThrowIfNULL_(mOwnedDesc.mDesc.dataHandle);
		BlockMove(&timi->data, *(mOwnedDesc.mDesc.dataHandle), timi->size);
		mOwnedDesc.mDesc.descriptorType = typeAERecord;
		mTubeMap = LAESubDesc(mOwnedDesc.mDesc);		
#else
		{
			LAEStream		stream;
			
			stream.OpenRecord();	//	base item flavors
			stream.WriteKey(typeTubeMapItems);
			{
				AEDesc			desc;
				StHandleBlock	handle(timi->size);
				BlockMove(&timi->data, *(handle.mHandle), timi->size);
				desc.dataHandle = handle.mHandle;
				desc.descriptorType = typeAERecord;
				stream.WriteDesc(desc);
			}
			stream.CloseRecord();
			
			stream.Close(&mOwnedDesc.mDesc);
		}
		mTubeMap = LAESubDesc(mOwnedDesc.mDesc);
#endif
	} else {
/*-
		ScrapRecT		*end = UScrap::Terminal(mScrapHandle),
						*p = (ScrapRecT *)*mScrapHandle;
		LAEStream		ae;
		LDynamicArray	itemFlavors(sizeof(FlavorType));
		FlavorType		flavor;

		ae.OpenList();													//	items
		
		while (p < end) {
			ae.OpenList();												//	item flavors
			itemFlavors.RemoveItemsAt(max_Int32, 1);					//	empty list
			while (p < end) {
				flavor = p->type;
				if (itemFlavors.FetchIndexOf(&flavor)) {
					break;
				} else {
					itemFlavors.InsertItemsAt(1, max_Int32, &flavor);	//	append
					ae.WriteTypeDesc(flavor);							//	need a record?
				}
				p = UScrap::Next(p);
			}
			ae.CloseList();												//	item flavors
		}
		
		ae.CloseList();													//	items
		
		ae.Close(&mTubeMap.mDesc);
*/
	}
}

LClipboardTube::LClipboardTube(
	const LTubeableItem	*inTubeableItem,
	Boolean				inReqdFlavorsOnly
)
{
	ZeroScrap();

	FillFrom(inTubeableItem, inReqdFlavorsOnly);
}

LClipboardTube::~LClipboardTube()
{
}

//	----------------------------------------------------------------------
//	Reading:

Boolean	LClipboardTube::FlavorExists(FlavorType inFlavor) const
{
	Boolean	rval = false;
	
	if (mTubeDepth == mSwapDepth) {
	
		if (inFlavor == typeWildCard) {
			Int32	count = UScrap::CountItems(mScrapHandle, inFlavor);
			
			count -= UScrap::CountItems(mScrapHandle, typeTubeMapItems);
			
			rval = count > 0;
		} else {
			rval = UScrap::CountItems(mScrapHandle, inFlavor) > 0;
		}
		
	} else {
		rval = inherited::FlavorExists(inFlavor);
	}
	
	return rval;
}

Size	LClipboardTube::GetFlavorSize(FlavorType inFlavor) const
{
	Size	rval = 0;

	if (mTubeDepth == mSwapDepth) {
		ScrapRecT	*p = UScrap::NthItem(mScrapHandle, inFlavor, 1);
		
		if (p) 
			rval = p->size;
	} else {
		rval = inherited::GetFlavorSize(inFlavor);
	}
	
	return rval;
}

void *	LClipboardTube::GetFlavorData(
	FlavorType	inFlavor,
	void		*outFlavorData
) const
//	For routine description see LDataTube.cp.
{
	void	*rval = NULL;
	
	if (mTubeDepth == mSwapDepth) {
		ScrapRecT	*p = UScrap::NthItem(mScrapHandle, inFlavor, 1);

		if (p) {
			if (outFlavorData && (p->size > 0)) {
				BlockMove(&p->data, outFlavorData, p->size);
			} else {
				rval = &p->data;
			}
		}
	} else {
		rval = inherited::GetFlavorData(inFlavor, outFlavorData);
	}
	
	return rval;
}

//	----------------------------------------------------------------------
//	Writing:

void	LClipboardTube::AddFlavor(FlavorType inFlavor)
{
	if (mItemStack.Depth() == mSwapDepth) {
		if (mItemStack.Top().item && mAddSendsData)
			mItemStack.Top().item->SendFlavorTo(inFlavor, this);
	} else {
		inherited::AddFlavor(inFlavor);
	}
}

void	LClipboardTube::SetFlavorData(
	FlavorType	inFlavor,
	const void	*inFlavorData,
	Int32		inFlavorSize
)
{
	if (mItemStack.Depth() == mSwapDepth) {
		long	err;	//	Long for unusual scrap calling convention
		
		Assert_(inFlavorData);
		
		err = PutScrap(inFlavorSize, inFlavor, (Ptr)inFlavorData);
		ThrowIfOSErr_(err);
	} else {
		inherited::SetFlavorData(inFlavor, inFlavorData, inFlavorSize);
	}
}

//	===========================================================================
//	UScrap (utilities for dealing with scrap handles):

Int32	UScrap::CountItems(Handle inScrap, FlavorType inFlavor)
{
	ScrapRecT	*end = Terminal(inScrap),
				*p = (ScrapRecT *)(*inScrap);
	Int32		count = 0;
	
	while (p < end) {
		if (inFlavor == typeWildCard) {
			count++;
		} else if (p->type == inFlavor) {
			count++;
		}
		
		p = Next(p);
	}
	
	return count;
}

ScrapRecT *	UScrap::NthItem(Handle inScrap, FlavorType inFlavor, Int32 inIndex)
{
	Assert_(inIndex >= 1);

	ScrapRecT	*rval = NULL,
				*end = Terminal(inScrap),
				*p = (ScrapRecT *)(*inScrap);
	Int32		count = 0;
	
	while (p < end) {
		if (inFlavor == typeWildCard) {
			inIndex--;
		} else if (p->type == inFlavor) {
			inIndex--;
		}
		
		if (!inIndex) {
			rval = p;
			break;
		}

		p = Next(p);
	}
	
	return rval;
}

ScrapRecT *	UScrap::Next(ScrapRecT *inItemPtr)
{
	Ptr			p = (Ptr)inItemPtr;
	
	ThrowIfNULL_(p);
	
	p += inItemPtr->size;
	p += inItemPtr->size % 2;	//	Records are padded to even byte.
	
	p += sizeof(inItemPtr->type);
	p += sizeof(inItemPtr->size);
		
	return (ScrapRecT *)p;
}

ScrapRecT *	UScrap::Terminal(Handle inScrap)
{
	ThrowIfNULL_(inScrap);
	ThrowIfNULL_(*inScrap);
	Size	size = GetHandleSize(inScrap);
	ScrapRecT	*end = (ScrapRecT *)(*inScrap + size);
	return end;
}