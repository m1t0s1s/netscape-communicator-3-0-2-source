//	===========================================================================
//	LFileTube.cp				©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

//	For routine descriptions see LDataTube.cp.

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include "LFileTube.h"
#include <UMemoryMgr.h>
#include <LTubeableItem.h>
#include <LFileStream.h>
#include <URange32.h>

#ifndef __ERRORS__
#include <Errors.h>
#endif

#ifndef __RESOURCES__
#include <Resources.h>
#endif

#pragma	warn_unusedarg off

const Int16		refNum_Undefined	= -1;

#define	AssertValidResID_(num)	Assert_((min_Int16 <= ((Int32)(num))) && (((Int32)(num)) <= max_Int16))

LFileTube::LFileTube(
	LFile			*inFile,
	Int16			inPrivileges,
	ResIDT			inLevel1Ref)
{
	FSSpec	fsSpec;
	FInfo	fndrInfo;
	OSErr	err;

	mSwapDepth = 2;
	mLevel1Ref = inLevel1Ref;
	mNextRef = mLevel1Ref;
	mItemRef = mLevel1Ref;

	mFile = inFile;
	mPrivileges = inPrivileges;
	
	//	mFileType
	mFile->GetSpecifier(fsSpec);
	err = FSpGetFInfo(&fsSpec, &fndrInfo);
	ThrowIfOSErr_(err);
	mFileType = fndrInfo.fdType;
}

LFileTube::LFileTube(
	LFile				*inFile,
	const LTubeableItem	*inTubeableItem,
	Int16				inPrivileges,
	Boolean				inReqdFlavorsOnly,
	ResIDT				inLevel1Ref
)
{
	FSSpec	fsSpec;
	FInfo	fndrInfo;
	OSErr	err;

	mSwapDepth = 2;
	mLevel1Ref = inLevel1Ref;
	mNextRef = mLevel1Ref;

	mFile = inFile;
	mPrivileges = inPrivileges;
	
	//	mFileType
	mFile->GetSpecifier(fsSpec);
	err = FSpGetFInfo(&fsSpec, &fndrInfo);
	ThrowIfOSErr_(err);
	mFileType = fndrInfo.fdType;
	
	FillFrom(inTubeableItem, inReqdFlavorsOnly);
}

LFileTube::~LFileTube()
{
	if (mFile) {
		mFile->CloseDataFork();
		mFile->CloseResourceFork();
	}
}

//	----------------------------------------------------------------------
//	Read:

Boolean	LFileTube::FlavorExists(FlavorType inFlavor) const
{
	Boolean rval = false;
	
	if ((mTubeDepth+1 == mSwapDepth) || (mTubeDepth == mSwapDepth)) {
		do {
			if (inFlavor == typeWildCard) {
				rval = true;
				break;
			}
	
			if ((mFileType == inFlavor) && (mItemRef == mLevel1Ref)) {
				rval = true;
				break;
			}
			
			if (HasResourceFork()) {
				CheckResourceFork();
				
				//	An incredibly inefficient but safe way to do this...
				AssertValidResID_(mItemRef);				
				StResource	r(inFlavor, mItemRef, false, true);
				if (r.mResourceH)
					rval = true;
				break;
			}
		
		} while (false);
	} else {
		rval = inherited::FlavorExists(inFlavor);
	}
		
	return rval;
}

Size	LFileTube::GetFlavorSize(FlavorType inFlavor) const
{
	long	rval = 0;
	
	if ((mTubeDepth+1 == mSwapDepth) || (mTubeDepth == mSwapDepth)) {
		OSErr	err;
		
		if ((mFileType == inFlavor) && (mItemRef == mLevel1Ref)) {
			CheckDataFork();
			err = GetEOF(mFile->GetDataForkRefNum(), &rval);
			ThrowIfOSErr_(err);
		} else if (HasResourceFork()) {
			CheckResourceFork();
				
			AssertValidResID_(mItemRef);				
			StResource	r(inFlavor, mItemRef, false, true);
			
			if (r.mResourceH)
				rval = GetHandleSize(r);
		}
	} else {
		rval = inherited::GetFlavorSize(inFlavor);
	}

	return rval;
}

void *	LFileTube::GetFlavorData(
	FlavorType	inFlavor,
	void		*outFlavorData
) const
//	For routine description see LDataTube.cp.
{
	void	*rval = NULL;
	
	if ((mTubeDepth+1 == mSwapDepth) || (mTubeDepth == mSwapDepth)) {
		long	count = GetFlavorSize(inFlavor);
		OSErr	err;
		
		if (outFlavorData == NULL)
			return NULL;
	
		if ((mFileType == inFlavor) && (mItemRef == mLevel1Ref)) {
			CheckDataFork();
			err = FSRead(mFile->GetDataForkRefNum(), &count, outFlavorData);
			ThrowIfOSErr_(err);
		} else {
			CheckResourceFork();				
			AssertValidResID_(mItemRef);			
			StResource	r(inFlavor, mItemRef, true, true);
			
			count = URange32::Min(count, GetHandleSize(r));
			
			if (count > 0)
				BlockMoveData(*r.mResourceH, outFlavorData, count);
		}
	} else {
		rval = inherited::GetFlavorData(inFlavor, outFlavorData);
	}
	
	return rval;
}

LStream &	LFileTube::GetFlavorStreamOpen(FlavorType inFlavor) const
{
	if (mStream)
		Throw_(paramErr);	//	presently you can have a total of only one flavor stream open at a time
	
	if (	((mTubeDepth+1 == mSwapDepth) || (mTubeDepth == mSwapDepth)) &&
			(mFileType == inFlavor) &&
			(mItemRef == mLevel1Ref)
	) {
		LFileTube	&my = (LFileTube&)*this;
		
		my.mStreamFlavor = inFlavor;
		
		FSSpec		fsSpec;
		ThrowIfNULL_(mFile);
		mFile->GetSpecifier(fsSpec);
		LFileStream	*stream = new LFileStream(fsSpec);
		my.mStream = stream;
		
		stream->OpenDataFork(mPrivileges);
		stream->SetLength(0);
		stream->SetMarker(0, streamFrom_Start);

		return *mStream;
	} else {
		return inherited::GetFlavorStreamOpen(inFlavor);
	}
}

void	LFileTube::GetFlavorStreamClose(void) const
{
	if (!mStream)
		Throw_(paramErr);	//	close without balanced open

	if (	((mTubeDepth+1 == mSwapDepth) || (mTubeDepth == mSwapDepth)) &&
			(mFileType == mStreamFlavor) &&
			(mItemRef == mLevel1Ref)
	) {
		LFileTube	&my = (LFileTube&)*this;

		LFileStream	*stream = dynamic_cast<LFileStream *>(my.mStream);
		ThrowIfNULL_(stream);
		stream->CloseDataFork();
		
		delete mStream;
		my.mStream = NULL;
	} else {
		inherited::GetFlavorStreamClose();
	}
}

Int32	LFileTube::CountItems(void) const
{
	Int32	rval = 0;

	if (mTubeDepth+1 == mSwapDepth) {

		Assert_(false);	//	Can't count here		
		rval = 1;		//	not correct!

		//	should be...
//		rval = UResourceMap::CountIDs(currentResourceMap, typeWildCard);
//		Assert_(UResourceMap::IDExists(currentResourceMap, typeWildCard, sLevel1ID);
//		rval -= 1;	//	for sLevel1ID

	} else if (mTubeDepth == mSwapDepth) {
		StAEDescriptor	items;
		
		GetFlavorAsDesc(typeTubeMapItems, &items.mDesc);
		
		LAESubDesc	itemsSD(items);
		
		return itemsSD.CountItems();

	} else {

		rval = inherited::CountItems();

	}
	
	return rval;
}

LDataTube	LFileTube::KeyedItem(ItemReference inItemRef) const
{
	if (mTubeDepth+1 == mSwapDepth) {
	
		LAESubDesc	sd;	//	null sd
	
		return LDataTube(this, inItemRef, sd, mTubeDepth + 1);
		
	} else if (mTubeDepth == mSwapDepth) {
		CheckResourceFork();
		
		//	Get	typeTubeMapItems data
		AssertValidResID_(inItemRef);				
		StResource	r(typeTubeMapItems, inItemRef, true, true);
		
		//	Swap mOwnedDesc
		OSErr	err;
		AEDesc	*desc = &(((LFileTube *)this)->mOwnedDesc.mDesc);
		AEDisposeDesc(desc);
		desc->descriptorType = typeAERecord;
		desc->dataHandle = r.mResourceH;
		err = HandToHand(&(desc->dataHandle));
		ThrowIfOSErr_(err);
			
		return LDataTube(this, inItemRef, LAESubDesc(*desc), mTubeDepth + 1);
		
	} else {

		return inherited::KeyedItem(inItemRef);

	}
}

LDataTube	LFileTube::NthItem(Int32 inIndex, ItemReference *outReference) const
{
	if (mTubeDepth+1 == mSwapDepth) {

		if (inIndex != 1)
			Throw_(paramErr);	//	Can't do that
		
		if (outReference)
			*outReference = mLevel1Ref;
		
		return KeyedItem(mLevel1Ref);

	} else if (mTubeDepth == mSwapDepth) {

		Throw_(paramErr);	//	Can't do this here
		return LDataTube();	//	Bogus

	} else {

		return inherited::NthItem(inIndex, outReference);

	}
}

//	----------------------------------------------------------------------
//	Write:

void	LFileTube::AddFlavor(FlavorType inFlavor)
{
	if (	(mItemStack.Depth()+1 == mSwapDepth) ||
			(mItemStack.Depth() == mSwapDepth)
	) {
		if (mItemStack.Top().item && mAddSendsData) {
			mItemStack.Top().item->SendFlavorTo(inFlavor, this);
		}
	} else {
		inherited::AddFlavor(inFlavor);
	}
}

void	LFileTube::SetFlavorData(
	FlavorType	inFlavor,
	const void	*inFlavorData,
	Int32		inFlavorSize
)
//	For routine description see LDataTube.cp.
{
	if (	(mItemStack.Depth()+1 == mSwapDepth) ||
			(mItemStack.Depth() == mSwapDepth)
	) {
		OSErr	err;
		long	count = inFlavorSize;
		
		ThrowIfNULL_(inFlavorData);
		
		if ((mFileType == inFlavor) && (mItemStack.Top().ref == mLevel1Ref)) {
			CheckDataFork();
			err = FSWrite(mFile->GetDataForkRefNum(), &count, inFlavorData);
			ThrowIfOSErr_(err);

			err = SetEOF(mFile->GetDataForkRefNum(), inFlavorSize);
			ThrowIfOSErr_(err);
		} else {
			if (!HasResourceFork())
				AddResourceFork();
			CheckResourceFork();
			
			AssertValidResID_(mItemStack.Top().ref);				
			StResource	r(inFlavor, mItemStack.Top().ref, false, true);
			
			if (r.mResourceH) {
				SetHandleSize(r, inFlavorSize);
				ThrowIfMemError_();
				BlockMoveData(inFlavorData, *r, inFlavorSize);
				ChangedResource(r);
				ThrowIfResError_();
			} else {
				r.mResourceH = NewHandle(inFlavorSize);
				ThrowIfNULL_(r);
				BlockMoveData(inFlavorData, *r, inFlavorSize);
				AddResource(r, inFlavor, mItemStack.Top().ref, "\p");
				ThrowIfResError_();
			}
			WriteResource(r);
			ThrowIfResError_();
		}
	} else {
		inherited::SetFlavorData(
						inFlavor,
						inFlavorData,
						inFlavorSize);
	}
}

LStream &	LFileTube::SetFlavorStreamOpen(FlavorType inFlavor)
{
	if (mStream)
		Throw_(paramErr);	//	presently you can have a total of only one flavor stream open at a time
	
	if (	((mItemStack.Depth()+1 == mSwapDepth) || (mItemStack.Depth() == mSwapDepth))	&&
			(mFileType == inFlavor) &&
			(mItemStack.Top().ref == mLevel1Ref)
	) {
		mStreamFlavor = inFlavor;

		FSSpec		fsSpec;
		ThrowIfNULL_(mFile);
		mFile->GetSpecifier(fsSpec);
		LFileStream	*stream = new LFileStream(fsSpec);
		mStream = stream;
		
		stream->OpenDataFork(mPrivileges);
		stream->SetLength(0);
		stream->SetMarker(0, streamFrom_Start);
	} else {
		inherited::SetFlavorStreamOpen(inFlavor);
	}
	
	return *mStream;
}

void	LFileTube::SetFlavorStreamClose(void)
{
	if (!mStream)
		Throw_(paramErr);	//	close without balanced open
	
	if (	((mItemStack.Depth()+1 == mSwapDepth) || (mItemStack.Depth() == mSwapDepth))	&&
			(mFileType == mStreamFlavor) &&
			(mItemStack.Top().ref == mLevel1Ref)
	) {
		
		LFileStream	*stream = dynamic_cast<LFileStream*>(mStream);
		ThrowIfNULL_(stream);
		stream->CloseDataFork();
		
		delete mStream;
		mStream = NULL;
	} else {
		inherited::SetFlavorStreamClose();
	}
}

//	----------------------------------------------------------------------
//	Implementation:

void	LFileTube::CheckDataFork(void) const
{
	ThrowIfNULL_(mFile);
	
	if (mFile->GetDataForkRefNum() == refNum_Undefined) {
		mFile->OpenDataFork(mPrivileges);
	}
}

void	LFileTube::CheckResourceFork(void) const
{
	ThrowIfNULL_(mFile);
	
	if (HasResourceFork())
		MakeResourceForkCurrent();
}

Boolean	LFileTube::HasResourceFork(void) const
{
	Boolean	rval = true;
	Try_ {
		#ifdef	Debug_Throw
			//	It IS okay for this to fail, so turn off Throw debugging.
			StValueChanger<EDebugAction>	okayToFail(gDebugThrow, debugAction_Nothing);
		#endif
	
		if (mFile->GetResourceForkRefNum() == refNum_Undefined)
			mFile->OpenResourceFork(mPrivileges);
	} Catch_(err) {
		if (err != eofErr)
			ThrowIfOSErr_(err);
		rval = false;
	} EndCatch_;
	
	return rval;
}

void	LFileTube::AddResourceFork(void)
{
	OSErr		err;
	FSSpec		fSpec;
	FInfo		fInfo;
	
	if (!HasResourceFork()) {
		mFile->GetSpecifier(fSpec);
		err = FSpGetFInfo(&fSpec, &fInfo);
		ThrowIfOSErr_(err);
		mFile->CreateNewFile(fInfo.fdCreator, fInfo.fdType);
	}
}

void	LFileTube::MakeResourceForkCurrent(void) const
{
	if (CurResFile() != mFile->GetResourceForkRefNum()) {
		UseResFile(mFile->GetResourceForkRefNum());
		ThrowIfResError_();
	}
}

//	----------------------------------------------------------------------
//	Helper class:

/*
	Wouldn't this be nice?  It would allow a sensible CountItems for
	level 2.  IF, it didn't have to illegally walk the resource map.
	
	If this is ever implemented, it should be placed in a shared
	library -- so it can easily be replaced if a subseqent OS releases
	breaks it.

	typedef	Handle	ResourceMapH;
	
	class UResourceMap {
	public:
		
		static	Int32			CountIDs(const ResourceMapH inMap, ResType inType);
		static	Boolean			IDExists(const ResourceMapH inMap, ResType inType, ResIDT inID);
		static	ResIDT			NthID(const ResourceMapH inMap, ResType inType, Int32 inIndex);
		static	ResourceMapH	CurrentMap(void);
	};
*/