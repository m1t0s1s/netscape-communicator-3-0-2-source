//	===========================================================================
//	LClipboardTube.h			©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LFlatTube.h>

class	LClipboardTube
			:	public	LFlatTube
{
public:
				//	Maintenance
						LClipboardTube();
						LClipboardTube(
							const LTubeableItem	*inTubeableItem,
							Boolean				inReqdFlavorsOnly = false);
	virtual				~LClipboardTube();
	virtual FlavorForT	GetFlavorForType(void) const	{return flavorForClipboard;}

				//	Implementation
	virtual	Boolean		FlavorExists(FlavorType inFlavor) const;
	virtual	Int32		GetFlavorSize(FlavorType inFlavor) const;
	virtual void *		GetFlavorData(
							FlavorType	inFlavor,
							void		*outFlavorData) const;

	virtual void		AddFlavor(FlavorType inFlavor);
	virtual void		SetFlavorData(
							FlavorType	inFlavor,
							const void	*inFlavorData,
							Int32		inFlavorSize);

protected:
	Handle				mScrapHandle;
};

//	===========================================================================
//	UScrap (helps w/ multiple items on the scrap):

typedef struct {
	FlavorType	type;
	Size		size;	//	May be odd
	char		data;	//	variable amount of data.
						//		Padded to even number of bytes.
} ScrapRecT;

class	UScrap
{
public:

	static	Int32		CountItems(Handle inScrap, FlavorType inFlavor);
	static	ScrapRecT *	NthItem(Handle inScrap, FlavorType inFlavor, Int32 inIndex);
	static	ScrapRecT *	Next(ScrapRecT *inItemPtr);
	static	ScrapRecT *	Terminal(Handle inScrap);
};

