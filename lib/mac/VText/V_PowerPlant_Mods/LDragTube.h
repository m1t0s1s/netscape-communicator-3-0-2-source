//	===========================================================================
//	LDragTube.h						©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LFlatTube.h>


class	LDragTube
			:	public	LFlatTube
{
public:
				//	Maintenance
						LDragTube(DragReference inDragRef);
						LDragTube(
							DragReference		inDragRef,
							const LTubeableItem	*inTubeableItem,
							Boolean				inReqdFlavorsOnly = false);
	virtual				~LDragTube();
	virtual FlavorForT	GetFlavorForType(void) const			{return flavorForDrag;}

				//	Implementation
	virtual	Boolean		FlavorExists(FlavorType inFlavor) const;
	virtual	Int32		GetFlavorSize(FlavorType inFlavor) const;
	virtual void *		GetFlavorData(
							FlavorType	inFlavor,
							void		*outFlavorData) const;
	virtual Int32		CountItems(void) const;
	virtual LDataTube	NthItem(Int32 inIndex, ItemReference *outReference = NULL) const;
	virtual LDataTube	KeyedItem(ItemReference inItemRef) const;

	virtual void		AddFlavor(FlavorType inFlavor);
	virtual void		SetFlavorData(
							FlavorType	inFlavor,
							const void	*inFlavorData,
							Int32		inFlavorSize);

	virtual	void		OpenItem(const LTubeableItem *inItem, ItemReference inItemRef = defaultItemReference);
	virtual void		CloseItem(void);
	
				//	New features
	virtual void		SendFlavorData(ItemReference inItemRef, FlavorType inFlavor);

protected:
	virtual	Int32		FlavorCount(void) const;
	virtual FlavorType	NthFlavor(Int32 inIndex) const;

	static Boolean		sSendingData;
	DragReference		mDragRef;
};

