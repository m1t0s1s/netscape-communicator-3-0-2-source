//	===========================================================================
//	VTextView.h
//	===========================================================================

#pragma once

#include	<LTextView.h>
#include 	<LTextEngine.h>
#include	<VText_ClassIDs.h>

class	VInlineTSMProxy;

class	VTextView
			:	public LTextView
{
public:
				//	Maintenance
						VTextView();
						VTextView(LStream *inStream);
	virtual				~VTextView();
	enum {class_ID = kVTCID_VTextView};
	virtual ClassIDT	GetClassID() const {return class_ID;}
	virtual void		FinishCreateSelf(void);

	virtual void		BuildTextObjects(LModelObject *inSuperModel);
//?	virtual LStyleSet *	GetCurrentStyle(void);
	virtual void		SetTSMProxy(VInlineTSMProxy *inProxy);
	virtual VInlineTSMProxy *
						GetTSMProxy(void);
	
	virtual	void		SetTextEngine(LTextEngine *inTextEngine);
	virtual void		ScrollImageBy(Int32 inLeftDelta, Int32 inTopDelta,
							Boolean inRefresh);
	virtual	void		DrawSelfSelection(void);
//-	virtual Boolean		ObeyCommand(CommandT inCommand, void *ioParam);
//-	virtual void		FindCommandStatus(CommandT inCommand,
//-							Boolean &outEnabled, Boolean &outUsesMark,
//-							Char16 &outMark, Str255 outName);
														
protected:

	virtual void		BeTarget(void);
	virtual void		DontBeTarget(void);

	virtual void		MakeTextObjects(void);
	virtual void		LinkTextObjects(LModelObject *inSuperModel);
	
	VInlineTSMProxy *	mTSMProxy;
};