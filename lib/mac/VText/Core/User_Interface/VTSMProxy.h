//	===========================================================================
//	VTSMProxy
//
//	Copyright 1996 ViviStar Consulting.
//	===========================================================================

#pragma once

#include	<UAEGizmos.h>

#include	<LTextEngine.h>
#include	<LAttachment.h>
#include	<TextServices.h>
#include 	<TSMTE.h>
#include	<vector.h>

class	VTextView;
class	VTSMProxy;
class	VInlineTSMProxy;
class	VTSMTEProxy;

//	----------------------------------------------------------------------

class	WTSMManager
{
public:
	static	void		Initialize(void);
	static	void		Quit(void);
	
	static	VTSMProxy *	GetCurrent(void)				{	return sCurrent;	}
	static	void		SetCurrent(VTSMProxy *inProxy)	{	sCurrent = inProxy;	}

	static	VInlineTSMProxy *
						MakeInlineTSMProxy(VTextView &inTextView);
	static	VTSMTEProxy *
						MakeTSMTEProxy(TEHandle inTEH);
	
protected:
	static pascal OSErr	AEHandlerTSM(
							const AppleEvent	*inAppleEvent,
							AppleEvent			*outReply,
							Int32				inRefCon);

	static Boolean				sTSMPresent;
	static AEEventHandlerUPP	sAEHandler;

	static VTSMProxy*			sCurrent;
};

//	----------------------------------------------------------------------

class	VTSMDoEvent
			:	public	LAttachment
{
public:
						VTSMDoEvent(
							MessageT		inMessage = msg_AnyMessage,
							Boolean			inExecuteHost = true);

protected:
	virtual void		ExecuteSelf(
							MessageT		inMessage,
							void			*ioParam);
};

//	----------------------------------------------------------------------

class VTSMProxy
{
public:
						VTSMProxy();
	virtual				~VTSMProxy();
	
	virtual void		Activate(void);
	virtual void		Deactivate(void);
	virtual	void		FlushInput(void);

protected:

	TSMDocumentID		mTSMDocID;
	Boolean				mDocIsActive;
};

//	----------------------------------------------------------------------

class	VInlineTSMProxy : public VTSMProxy
{
	friend class	WTSMManager;

public:
						VInlineTSMProxy(
							LTextEngine			&inTextEngine,
							VTextView			&inTextView);
						~VInlineTSMProxy();

	virtual	void		FlushInput(void);
	virtual void		DrawHilite(void);

protected:
	virtual void		DrawRangeHilite(
							const TextRangeT	&inRange,
							Int16				inHiliteStyle);

	virtual void		AEUpdate(
							const LAESubDesc	&inAppleEvent,
							LAEStream			&inStream);
	virtual void		AEPos2Offset(
							const LAESubDesc	&inAppleEvent,
							LAEStream			&inStream) const;
	virtual void		AEOffset2Pos(
							const LAESubDesc	&inAppleEvent,
							LAEStream			&inStream) const;

	typedef struct RangeHiliteRec {
		TextRangeT		range;
		Int16			hilite;
	} RangeHiliteRec;
	
	vector<RangeHiliteRec>	mHilites;
	LTextEngine				&mTextEngine;
	TextRangeT				mActiveRange;
	VTextView				&mTextView;
	
};

//	----------------------------------------------------------------------

class VTSMTEProxy : public VTSMProxy
{
public:
						VTSMTEProxy(TEHandle inTEHandle);
	virtual				~VTSMTEProxy();
	
protected:

	static pascal void 	PreUpdate(
							TEHandle 	inTEHandle,
							Int32		inRefCon);
	static TSMTEPreUpdateUPP 	sPreUpdateUPP;

	TSMTERecHandle		mTSMTEHandle;
	Int32				mVersion;
	TEHandle			mTEH;
};


