//	===========================================================================
//	LDataTube.cp					©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	<UMemoryMgr.h>
#include	<LTubeableItem.h>
#include	"LDataTube.h"

#pragma	warn_unusedarg off

LDataTube::LDataTube()
{
	mSuperTube = NULL;
	mItemRef = 0;
	mTubeDepth = 1;
}

LDataTube::LDataTube(
	const LDataTube		*inSuperTube,
	ItemReference		inItemRef,
	const LAESubDesc	&inSubTubeMap,
	Int32				inTubeDepth
)
{
	mSuperTube = (LDataTube *) inSuperTube;
	mItemRef = inItemRef;
	mTubeMap = inSubTubeMap;
	mTubeDepth = inTubeDepth;
}

LDataTube::~LDataTube()
{
}

Boolean	LDataTube::GetOnlyReqdFlavors(void) const
{
	return mSuperTube->GetOnlyReqdFlavors();
}

FlavorForT	LDataTube::GetFlavorForType(void) const
{
	return mSuperTube->GetFlavorForType();
}

//	----------------------------------------------------------------------
//	Reading:

Boolean	LDataTube::FlavorExists(FlavorType inFlavor) const
{
	StTubeItemChanger	changer(mSuperTube, this);
	
	return mSuperTube->FlavorExists(inFlavor);
}

Int32	LDataTube::GetFlavorSize(FlavorType inFlavor) const
{
	StTubeItemChanger	changer(mSuperTube, this);
	
	return mSuperTube->GetFlavorSize(inFlavor);
}

void *	LDataTube::GetFlavorData(
	FlavorType	inFlavor,
	void		*outFlavorData
) const
/*
	InOffset and inAmount are optional parameters for "spooled data"...
	
	For "spooled data," inOffset indicates the flavor data offset position
	to transfer inAmount bytes to outFlavorData.
	
	If outFlavorData is NULL, the data tube will attempt to return a
	pointer to its own copy of the flavor data.  If the flavor is internally
	represented as a handle or is discontinuous, NULL will be returned.
	In that case, you must call GetFlavorData again with a non-NULL
	outFlavorData.
*/
{
	StTubeItemChanger	changer(mSuperTube, this);
	
	return mSuperTube->GetFlavorData(inFlavor, outFlavorData);
}

LStream &	LDataTube::GetFlavorStreamOpen(FlavorType inFlavor) const
{
	StTubeItemChanger	changer(mSuperTube, this);
	
	return mSuperTube->GetFlavorStreamOpen(inFlavor);
}

void	LDataTube::GetFlavorStreamClose(void) const
{
	StTubeItemChanger	changer(mSuperTube, this);
	
	mSuperTube->GetFlavorStreamClose();
}

Int32	LDataTube::CountItems(void) const
{
	StTubeItemChanger	changer(mSuperTube, this);
	
	return mSuperTube->CountItems();
}

LDataTube	LDataTube::NthItem(Int32 inIndex, ItemReference *outReference) const
{
	StTubeItemChanger	changer(mSuperTube, this);
	
	return mSuperTube->NthItem(inIndex, outReference);
}

LDataTube	LDataTube::KeyedItem(ItemReference inItemRef) const
{
	StTubeItemChanger	changer(mSuperTube, this);
	
	return mSuperTube->KeyedItem(inItemRef);
}

//	----------------------------------------------------------------------
//	protected read helpers

void	LDataTube::WriteDesc(LAEStream *inStream) const
{
	StTubeItemChanger	changer(mSuperTube, this);
	
	mSuperTube->WriteDesc(inStream);
}	

Int32	LDataTube::FlavorCount(void) const
{
	StTubeItemChanger	changer(mSuperTube, this);
	
	return mSuperTube->FlavorCount();
}

FlavorType	LDataTube::NthFlavor(Int32 inIndex) const
{
	StTubeItemChanger	changer(mSuperTube, this);
	
	return mSuperTube->NthFlavor(inIndex);
}

//	----------------------------------------------------------------------
//	Writing:

void	LDataTube::AddFlavor(FlavorType inFlavor)
{
	Assert_(false);
}

void	LDataTube::SetFlavorData(
	FlavorType	inFlavor,
	const void	*inFlavorData,
	Int32		inFlavorSize
)
/*
	Set functions should only be called by initialization routines
	or a tubeable item in response to a send flavor message.

	InOffset and inAmount are optional parameters for "spooled data"...
	
	For "spooled data," inOffset indicates the flavor data offset position
	to transfer inAmount bytes from inFlavorData.
*/
{
	Assert_(false);
}

LStream &	LDataTube::SetFlavorStreamOpen(FlavorType inFlavor)
{
	Throw_(paramErr);
	
	LStream	*nullStream = NULL;
	
	return *nullStream;
}

void	LDataTube::SetFlavorStreamClose(void)
{
	Assert_(false);
}

void	LDataTube::OpenItem(
			const LTubeableItem	*inItem,
			ItemReference		inItemRef
)
{
	Assert_(false);
}

void	LDataTube::CloseItem(void)
{
	Assert_(false);
}

void	LDataTube::SetItem(LTubeableItem *inItem)
{
	Assert_(false);
}

//	----------------------------------------------------------------------
//	Helper functions:

void	LDataTube::GetFlavorHandle(FlavorType inFlavor, Handle outFlavorData) const
{
	ThrowIfNULL_(outFlavorData);
	
	Size	size = GetFlavorSize(inFlavor);
	
	SetHandleSize(outFlavorData, size);
	ThrowIfMemError_();	
	StHandleLocker	lock(outFlavorData);	
	GetFlavorData(inFlavor, *outFlavorData);
}

void	LDataTube::SetFlavorHandle(FlavorType inFlavor, const Handle inFlavorData)
/*
	Set functions should only be called by initialization routines
	or a tubeable item in response to a send flavor message.
*/
{
	ThrowIfNULL_(inFlavorData);	
	StHandleLocker	lock(inFlavorData);	
	SetFlavorData(inFlavor, *inFlavorData, GetHandleSize(inFlavorData));
}

void	LDataTube::GetFlavorAsDesc(FlavorType inFlavor, AEDesc *outFlavorDesc) const
{
	ThrowIfNULL_(outFlavorDesc);
	
	Size	flavorSize = GetFlavorSize(inFlavor);
	
	if (flavorSize) {
		if (outFlavorDesc->dataHandle == NULL) {
			outFlavorDesc->dataHandle = NewHandle(flavorSize);
			ThrowIfNULL_(outFlavorDesc->dataHandle);
		} else if (flavorSize > GetHandleSize(outFlavorDesc->dataHandle)) {
			SetHandleSize(outFlavorDesc->dataHandle, flavorSize);
			ThrowIfMemError_();
		}
	
		outFlavorDesc->descriptorType = inFlavor;
		
		GetFlavorHandle(inFlavor, outFlavorDesc->dataHandle);
	} else {
		AEDisposeDesc(outFlavorDesc);
	}
}

//	----------------------------------------------------------------------
//	Helper class:

StTubeItemChanger::StTubeItemChanger(
	const LDataTube	*inRealTube,
	const LDataTube	*inSubTube
)
{
	mRealTube = (LDataTube *)inRealTube;
	mSubTube = (LDataTube *)inSubTube;
	
	ThrowIfNULL_(mRealTube);
	ThrowIfNULL_(mSubTube);
	
	mOldDepth = mRealTube->mTubeDepth;
	mOldMap = mRealTube->mTubeMap;
	mOldRef = mRealTube->mItemRef;
	
	mRealTube->mTubeDepth = mSubTube->mTubeDepth;
	mRealTube->mTubeMap = mSubTube->mTubeMap;
	mRealTube->mItemRef = mSubTube->mItemRef;
}

StTubeItemChanger::~StTubeItemChanger()
{	
	mRealTube->mTubeDepth = mOldDepth;
	mRealTube->mTubeMap = mOldMap;
	mRealTube->mItemRef = mOldRef;
}

