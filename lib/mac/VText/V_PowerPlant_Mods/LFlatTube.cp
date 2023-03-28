//	===========================================================================
//	LFlatTube.cp				©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

//	For routine descriptions see LDataTube.cp.

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LFlatTube.h"
#include	<UMemoryMgr.h>
#include	<LTubeableItem.h>
#include	<LHandleStream.h>
#include	<URange32.h>


#pragma	warn_unusedarg off

LFlatTube::LFlatTube()
{
	mSwapDepth = 1;	
}

LFlatTube::~LFlatTube()
{
}

//	----------------------------------------------------------------------
//	Read:

Boolean	LFlatTube::FlavorExists(FlavorType inFlavor) const
{
	Boolean	rval = false;
	
	if (mTubeDepth > mSwapDepth) {
		rval = inherited::FlavorExists(inFlavor);
	}
	
	return rval;
}

Size	LFlatTube::GetFlavorSize(FlavorType inFlavor) const
{
	Size	rval = 0;

	if (mTubeDepth > mSwapDepth) {
		rval = inherited::GetFlavorSize(inFlavor);
	}
	
	return rval;
}

void *	LFlatTube::GetFlavorData(
	FlavorType	inFlavor,
	void		*outFlavorData
) const
{
	void	*rval = NULL;
	
	if (mTubeDepth > mSwapDepth) {
		rval = inherited::GetFlavorData(inFlavor, outFlavorData);
	} else {
		Throw_(noTypeErr);
	}
	
	return rval;
}

LStream &	LFlatTube::GetFlavorStreamOpen(FlavorType inFlavor) const
{
	if (mStream)
		Throw_(paramErr);	//	presently you can have a total of only one flavor stream open at a time
	
	if (mTubeDepth > mSwapDepth) {
		return inherited::GetFlavorStreamOpen(inFlavor);
	} else {
		LFlatTube	&my = (LFlatTube&)*this;
		
		my.mStreamFlavor = inFlavor;
		LHandleStream	*hStream = new LHandleStream();
		my.mStream = hStream;
		GetFlavorHandle(mStreamFlavor, hStream->GetDataHandle());
		hStream->SetLength(GetHandleSize(hStream->GetDataHandle()));
		return *mStream;
	}
}

void	LFlatTube::GetFlavorStreamClose(void) const
{
	if (!mStream)
		Throw_(paramErr);	//	close without balanced open

	if (mTubeDepth > mSwapDepth) {
		inherited::GetFlavorStreamClose();
	} else {
		LFlatTube	&my = (LFlatTube&)*this;

		delete mStream;
		my.mStream = NULL;
	}
}

//	----------------------------------------------------------------------
//	Write:

void	LFlatTube::AddFlavor(FlavorType inFlavor)
{
	if (mItemStack.Depth() > mSwapDepth) {
		inherited::AddFlavor(inFlavor);
	} else {
		//	data lost
	}
}

void	LFlatTube::SetFlavorData(
	FlavorType	inFlavor,
	const void	*inFlavorData,
	Int32		inFlavorSize
)
{
	if (mItemStack.Depth() > mSwapDepth) {
		inherited::SetFlavorData(inFlavor, inFlavorData, inFlavorSize);
	} else {
		//	data lost
	}
}

LStream &	LFlatTube::SetFlavorStreamOpen(FlavorType inFlavor)
{
	if (mStream)
		Throw_(paramErr);	//	presently you can have a total of only one flavor stream open at a time
	
	if (mItemStack.Depth() > mSwapDepth) {
		inherited::SetFlavorStreamOpen(inFlavor);
	} else {
		mStreamFlavor = inFlavor;
		mStream = new LHandleStream();
	}
	
	return *mStream;
}

void	LFlatTube::SetFlavorStreamClose(void)
{
	if (!mStream)
		Throw_(paramErr);	//	close without balanced open
	
	if (mItemStack.Depth() > mSwapDepth) {
		inherited::SetFlavorStreamClose();
	} else {
		LHandleStream	*hStream = dynamic_cast<LHandleStream*>(mStream);
		ThrowIfNULL_(hStream);
		SetFlavorHandle(mStreamFlavor, hStream->GetDataHandle());
		delete mStream;
		mStream = NULL;
	}
}

void	LFlatTube::WriteDesc(LAEStream *inStream) const
{
	if (mTubeDepth <= mSwapDepth) {
		StTempHandle	data(0);
		FlavorType		flavor;
		Int32			n,
						i;
		
		inStream->OpenRecord();
		
			//	Flavors
			n = FlavorCount();
			for (i = 1; i <= n; i++) {
				flavor = NthFlavor(i);
				GetFlavorHandle(flavor, data.mHandle);
				inStream->WriteKey(flavor);
				inStream->OpenDesc(flavor);
				StHandleLocker	lock(data.mHandle);
				inStream->WriteData(*(data.mHandle), GetHandleSize(data.mHandle));
				inStream->CloseDesc();
			}
			
			//	Items
			if (CountItems()) {
				inStream->WriteKey(typeTubeMapItems);
				inStream->OpenRecord();
				
				LDataTube		subTube;
				ItemReference	ref;
				
				n = CountItems();
				for (i = 1; i <= n; i++) {
					subTube = NthItem(i, &ref);
					inStream->WriteKey(ref);
					subTube.WriteDesc(inStream);
				}
				
				inStream->CloseRecord();
			}

		inStream->CloseRecord();

	} else {
		inherited::WriteDesc(inStream);
	}
}

void	LFlatTube::NewTop(void)
{
	if (mStream)
		Throw_(paramErr);	//	Flavor stream nesting error
		
	if (mItemStack.Depth() == mSwapDepth) {
		mItemStack.Top().stream = new LAEStream;
	} else if (mItemStack.Depth() > mSwapDepth) {
		mItemStack.Top().stream = mItemStack.PeekBack(1).stream;
	}
}

#include	<StClasses.t>

template class	StChange<void *>;

void	LFlatTube::OldTop(void)
{
	if (mStream)
		Throw_(paramErr);	//	Flavor stream nesting error
		
	if ((mItemStack.Depth() == mSwapDepth) && (mItemStack.Top().subCt)) {
		StAEDescriptor	subItems;	
		{
//+			StValueChanger<void *>	change(mDesc, &subItems.mDesc);
			StChange<void *>		change(&mDesc, &subItems.mDesc);
			
			inherited::OldTop();
		}
		Assert_(subItems.mDesc.descriptorType != typeNull);
		SetFlavorHandle(typeTubeMapItems, subItems.mDesc.dataHandle);
	} else {
		if (	(mItemStack.Depth() == mSwapDepth) || 
				(mItemStack.Depth() >= 2) && (mItemStack.Top().stream != mItemStack.PeekBack(1).stream)
		) {
//?			Assert_(mDesc->descriptorType == typeNull);
			mItemStack.Top().stream->Close(mDesc);
			delete mItemStack.Top().stream;
			mItemStack.Top().stream = NULL;
		}
	}
}

//	----------------------------------------------------------------------

