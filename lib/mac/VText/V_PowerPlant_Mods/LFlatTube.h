//	===========================================================================
//	LFlatTube.h			©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LDescTube.h>

class	LFlatTube
			:	public	LDescTube
{
public:
				//	Maintenance
protected:				//	There are no concrete LFlatTubes
						LFlatTube();
public:
	virtual				~LFlatTube();

	virtual void		WriteDesc(LAEStream *inStream) const;

	virtual	Boolean		FlavorExists(FlavorType inFlavor) const;
	virtual	Int32		GetFlavorSize(FlavorType inFlavor) const;
	virtual void *		GetFlavorData(
							FlavorType	inFlavor,
							void		*outFlavorData) const;
	virtual	LStream &	GetFlavorStreamOpen(FlavorType inFlavor) const;
	virtual	void		GetFlavorStreamClose(void) const;

	virtual void		AddFlavor(FlavorType inFlavor);
	virtual void		SetFlavorData(
							FlavorType	inFlavor,
							const void	*inFlavorData,
							Int32		inFlavorSize);
	virtual LStream &	SetFlavorStreamOpen(FlavorType inFlavor);
	virtual void		SetFlavorStreamClose(void);

protected:
	virtual	void		NewTop(void);
	virtual void		OldTop(void);

	Int32				mSwapDepth;
};
