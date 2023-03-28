//	===========================================================================
//	VSimpleTextView.h
//	===========================================================================

#pragma once

#include	<VTextView.h>

class	VSimpleTextView
			:	public VTextView
{
public:
				//	Maintenance
						VSimpleTextView();
						VSimpleTextView(LStream *inStream);
	virtual				~VSimpleTextView();
	enum {class_ID = kVTCID_VSimpleTextView};
	virtual ClassIDT	GetClassID() const {return class_ID;}
	virtual void		FinishCreateSelf(void);

//-	virtual void		BuildTextObjects(LModelObject *inSuperModel);

	virtual Boolean		ObeyCommand(CommandT inCommand, void *ioParam);
protected:
	virtual void		MakeTextObjects(void);
//-	virtual void		LinkTextObjects(LModelObject *inSuperModel);
	
	//	just for delayed instantiation issues...
	Rect		pMarginsRect;
	ResIDT		pTraitsID,
				pStyleSetID,
				pRulerSetID,
				pInitialTextID;
	Int32		pImageWidth;
	Int32		pAttributes;
	Boolean		mTabSelectsAll;
};