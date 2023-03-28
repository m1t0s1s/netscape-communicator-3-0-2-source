//	===========================================================================
//	LDataTubeItem.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#ifndef __DRAG__
#include <Drag.h>
#endif

class	LDataTube;

class	LTubeableItem
{
public:
						LTubeableItem();
	virtual				~LTubeableItem();

					//	Read
	virtual FlavorType	PickFlavorFrom(const LDataTube *inTube) const;
	virtual void		ReceiveDataFrom(LDataTube *inTube);
						//	override:
//	virtual FlavorType	PickFlavorFromSelf(const LDataTube *inTube) const;
//	virtual void		ReceiveDataFromSelf(LDataTube *inTube);

					//	Write
						//	override:
	virtual void		AddFlavorTypesTo(LDataTube *inTube) const;
	virtual Boolean		SendFlavorTo(FlavorType inFlavor, LDataTube *inTube) const;
};
