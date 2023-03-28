//	===========================================================================
//	LFileTube.h						©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LFlatTube.h>


class	LFile;

class	LFileTube
			:	public	LFlatTube
{
public:
				//	Maintenance
						LFileTube(
							LFile				*inFile,
							Int16				inPrivileges = fsRdPerm,
							ResIDT				inLevel1Ref = 129);
						LFileTube(
							LFile				*inFile,
							const LTubeableItem	*inTubeableItem,
							Int16				inPrivileges = fsRdWrPerm,
							Boolean				inReqdFlavorsOnly = true,
							ResIDT				inLevel1Ref = 129);
	virtual				~LFileTube();
	virtual FlavorForT	GetFlavorForType(void) const	{return flavorForFile;}

				//	Read
	virtual	Boolean		FlavorExists(FlavorType inFlavor) const;
	virtual	Int32		GetFlavorSize(FlavorType inFlavor) const;
	virtual void *		GetFlavorData(
							FlavorType	inFlavor,
							void		*outFlavorData) const;
	virtual Int32		CountItems(void) const;
	virtual LDataTube	NthItem(Int32 inIndex, ItemReference *outReference = NULL) const;
	virtual LDataTube	KeyedItem(ItemReference inItemRef) const;
	virtual	LStream &	GetFlavorStreamOpen(FlavorType inFlavor) const;
	virtual	void		GetFlavorStreamClose(void) const;

				//	Write
	virtual void		AddFlavor(FlavorType inFlavor);
	virtual void		SetFlavorData(
							FlavorType	inFlavor,
							const void	*inFlavorData,
							Int32		inFlavorSize);
	virtual LStream &	SetFlavorStreamOpen(FlavorType inFlavor);
	virtual void		SetFlavorStreamClose(void);

				//	Implementation
protected:
	virtual	void		CheckDataFork(void) const;
	virtual void		CheckResourceFork(void) const;
	virtual	Boolean	 	HasResourceFork(void) const;
	virtual	void		AddResourceFork(void);
	virtual	void		MakeResourceForkCurrent(void) const;

	ResIDT				mLevel1Ref;
	LFile				*mFile;
	Int16				mPrivileges;
	FlavorType			mFileType;
};
