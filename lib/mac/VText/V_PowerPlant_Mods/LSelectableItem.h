//	===========================================================================
//	LSelectableItem.h				©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LManipulator.h>
#include	<LSharableModel.h>
#include	<LTubeableItem.h>

class	LDataTube;

class	LSelectableItem
			:	public	LSharableModel
			,	public	LManipulator
			,	public	LTubeableItem
{
private:
	typedef	LSharableModel	inheritAEOM;
	
						LSelectableItem();	//	Parameters required
public:
				//	Maintenance
						LSelectableItem(LModelObject *inSuperModel, DescType inKind);
	virtual				~LSelectableItem();

						//	Disambiguation between LManipulator LSharable &
						//		LSharableModel LSharable.
	virtual void		AddUser(void *inUser);
	virtual void		Finalize();
	virtual void		SuperDeleted();

public:
	virtual ManipT		ItemType(void) const {return kSelectableItem;};

				//	Query (override)
	virtual Boolean		IsSubstantive(void) const;
	virtual Boolean		EquivalentTo(const LSelectableItem *inItem) const;
	virtual	Boolean		AdjacentWith(const LSelectableItem *inItem) const;
	virtual	Boolean		IndependentFrom(const LSelectableItem *inItem) const;
	virtual LSelectableItem *	FindExtensionItem(const LSelectableItem *inItem) const;

				//	Visual (override)
	virtual void		DrawSelfSelected(void);
	virtual void		UnDrawSelfSelected(void);
	virtual void		DrawSelfLatent(void);
	virtual void		UnDrawSelfLatent(void);
	virtual void		DrawSelfReceiver(void);
	virtual void		UnDrawSelfReceiver(void);
	virtual void		DrawSelfReceiverTick(void);
		
				//	Data tubing:
	virtual void		AddFlavorTypesTo(LDataTube *inTube) const;
	virtual Boolean		SendFlavorTo(FlavorType inFlavor, LDataTube *inTube) const;

protected:
	static	Int32		sReceiverTicker;
	static	Boolean		sReceiverOn;
};
