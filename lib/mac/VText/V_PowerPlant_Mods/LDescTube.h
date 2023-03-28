//	===========================================================================
//	LDescTube.h			©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LDataTube.h>
#include	<LStack.h>
#include	<UAppleEventsMgr.h>

//	----------------------------------------------------------------------
/*
	Class providing r/w tube access via AEDesc's.
	
	This class also serves as the base class for other concrete data tube
	classes.
*/
//	----------------------------------------------------------------------

//	WARNING:	This structure should be considered private and may change.
typedef struct {
public:
	const LTubeableItem	*item;
	ItemReference		ref;		//	of this item
	Int32				subCt;		//	current count of sub items of this item
	LAEStream			*stream;
} ItemRecT;

//	----------------------------------------------------------------------

class	LDescTube
			:	public	LDataTube
{
public:
				//	Maintenance
protected:
						LDescTube();
public:
						LDescTube(
							const AEDesc		&inTubeDesc);
						LDescTube(
							AEDesc				*ioTubeDesc,
							const LTubeableItem	*inTubeableItem,
							Boolean				inReqdFlavorsOnly = true);
	virtual				~LDescTube();

	virtual FlavorForT	GetFlavorForType(void) const	{return flavorForDesc;}
	virtual Boolean		GetOnlyReqdFlavors(void) const;
	virtual void		WriteDesc(LAEStream *inStream) const;

					//	Read
	virtual	Boolean		FlavorExists(FlavorType inFlavor) const;
	virtual	Int32		GetFlavorSize(FlavorType inFlavor) const;
	virtual void *		GetFlavorData(
							FlavorType	inFlavor,
							void		*outFlavorData) const;
	virtual	LStream &	GetFlavorStreamOpen(FlavorType inFlavor) const;
	virtual	void		GetFlavorStreamClose(void) const;
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
	virtual	void		OpenItem(const LTubeableItem *inItem, ItemReference inItemRef = defaultItemReference);
	virtual void		CloseItem(void);
					
protected:
					//	For all r/w tubes
	Boolean				mReqdFlavorsOnly;
	Boolean				mAddSendsData;		//	AddFlavor immediately adds data
	
					//	Implementation
	LStack<ItemRecT>	mItemStack;
	Int32				mNextRef;

	virtual	void		FillFrom(const LTubeableItem *inItem, Boolean inReqdFlavorsOnly);
	virtual void		SetItem(LTubeableItem *inItem);
	virtual	Int32		FlavorCount(void) const;
	virtual FlavorType	NthFlavor(Int32 inIndex) const;
	virtual	void		NewTop(void);
	virtual	void		OldTop(void);

					//	LDescTube attributes
	AEDesc				*mDesc;
	StAEDescriptor		mOwnedDesc;
	
	LStream				*mStream;
	FlavorType			mStreamFlavor;
};
