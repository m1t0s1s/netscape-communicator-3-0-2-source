//	===========================================================================
//	LDataTube.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<UAEGizmos.h>

#ifndef __DRAG__
#include <Drag.h>
#endif

//	---------------------------------------------------------------------------
/*
	Data tubes constitute an abstract i/o interface for "tubeable items."
	As a result, tubeable item data i/o is simplified since a tubeable item
	need not be concerned with wether i/o is to a drag, clipboard, file, or
	whatever.
	
	Tube instances come in two varieties:
	
		"tube filling"			fill structures (file | clipboard | drag) with
								data from a tubeable item.
		
		"data item extraction"	get data from existing structures (file |
								clipboard | drag).
	
	These varieties are determined at tube construction time with tube filling
	constructors typically having a tubeable item parameter.
*/
		
/*
	Sometimes, a given flavor type could refer to different types of data
	depending on the source of the data.  This overlap should never be
	designed into a system.  But, if a legacy system or legacy file format is
	being worked with, the flavors must be distinguished.  Use
	GetFlavorForType to do that distinguishing.  Associated enumerations...
*/
             
class	LTubeableItem;

typedef enum {
	flavorForClipboard,
	flavorForDrag,
	flavorForFile,
	flavorForDesc
} FlavorForT;

//	Reserved flavor types -- don't use
const FlavorType	typeTubeMapItems = 'timi';

const ItemReference	defaultItemReference = -1;

//	===========================================================================
//	Abstract data tube:

class	StCurrentItemChanger;
class	LTubeFlavorStream;
class	LStream;

class	LDataTube
{
public:
friend	class	StTubeItemChanger;
friend	class	LSelection;

				//	Maintenance
protected:				//	Only tubes can construct abstract tubes
						LDataTube();
						LDataTube(
							const LDataTube		*inSuperTube,
							ItemReference		inItemRef,
							const LAESubDesc	&inSubTubeMap,
							Int32				inTubeDepth
						);
public:
	virtual				~LDataTube();

	virtual FlavorForT	GetFlavorForType(void) const;
	virtual Boolean		GetOnlyReqdFlavors(void) const;
	virtual void		WriteDesc(LAEStream *inStream) const;

				//	New features
					//	Read
	virtual	Boolean		FlavorExists(FlavorType inFlavor) const;
	virtual	Int32		GetFlavorSize(FlavorType inFlavor) const;
	virtual void *		GetFlavorData(
							FlavorType	inFlavor,
							void		*outFlavorData) const;
	virtual	LStream &	GetFlavorStreamOpen(FlavorType inFlavor) const;
	virtual	void		GetFlavorStreamClose(void) const;
						//	Multi-item read
	virtual Int32		CountItems(void) const;
	virtual LDataTube	NthItem(Int32 inIndex, ItemReference *outReference = NULL) const;
	virtual LDataTube	KeyedItem(ItemReference inItemRef) const;

					//	Write
	virtual void		AddFlavor(FlavorType inFlavor);
	virtual void		SetFlavorData(
							FlavorType	inFlavor,
							const void	*inFlavorData,
							Int32		inFlavorSize);
	virtual LStream &	SetFlavorStreamOpen(FlavorType inFlavor);
	virtual void		SetFlavorStreamClose(void);
						//	Multi-item write
	virtual	void		OpenItem(const LTubeableItem *inItem, ItemReference inItemRef = defaultItemReference);
	virtual void		CloseItem(void);
	
					//	Helper read write f's:
						//	Read
	virtual void		GetFlavorHandle(FlavorType inFlavor, Handle outFlavorData) const;
	virtual void		GetFlavorAsDesc(FlavorType inFlavor, AEDesc *outFlavorDesc) const;
						//	Write
	virtual void		SetFlavorHandle(FlavorType inFlavor, const Handle inFlavorData);
//?	virtual void		SetFlavorByDesc(FlavorType inFlavor, const AEDesc &inFlavorDesc);


protected:
	virtual void		SetItem(LTubeableItem *inItem);	//	Don't use this
	virtual	Int32		FlavorCount(void) const;
	virtual FlavorType	NthFlavor(Int32 inIndex) const;
	
						//	all for reading...
	LDataTube			*mSuperTube;
	ItemReference		mItemRef;
	LAESubDesc			mTubeMap;
	Int32				mTubeDepth;
};

class	StTubeItemChanger
{
public:
	StTubeItemChanger(const LDataTube *inRealTube, const LDataTube *inSubTube);
	~StTubeItemChanger();

protected:
	LDataTube		*mRealTube,
					*mSubTube;

	Int32			mOldDepth;
	LAESubDesc		mOldMap;
	ItemReference	mOldRef;
};
	
