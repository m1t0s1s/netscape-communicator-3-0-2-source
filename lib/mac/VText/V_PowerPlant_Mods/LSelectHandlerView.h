//	===========================================================================
//	LSelectHandlerView.h			©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include	<LHandlerView.h>
#include	<LCommander.h>
#include	<LListener.h>

#include	<URegions.h>

class	LSelectHandlerView
			:	public LHandlerView
			,	public LCommander
			,	public LListener
{
private:
	typedef	LCommander	inheritCommander;
public:
	enum { class_ID = kPPCID_SelectHandlerView };

				//	Maintenance
						LSelectHandlerView();
						LSelectHandlerView(LStream *inStream);
						LSelectHandlerView(LStream *inStream, Boolean inOldStyle);
	virtual				~LSelectHandlerView();
	virtual void		FinishCreateSelf(void);

public:
	virtual	LSelection *	GetSelection(void);
	virtual void		SetSelection(LSelection *inSelection);
	
				//	New features
	virtual void		ShouldBeTarget(void);
	virtual void		GetPresentHiliteRgn(
								Boolean		inAsActive,
								RgnHandle	ioRegion,
								Int16		inOriginH = 0,
								Int16		inOriginV = 0);
	virtual void		SetTargeter(LTargeter *inTargeter);

				//	Implementation
	virtual void		ListenToMessage(MessageT inMessage, void *ioParam);
	virtual void		DrawSelf(void);
	virtual	void		Click(SMouseDownEvent &inMouseDown);
	virtual void		ResizeFrameBy(Int16 inWidthDelta, Int16 inHeightDelta,
								Boolean inRefresh);
	virtual void		MoveBy(Int32 inHorizDelta, Int32 inVertDelta,
								Boolean inRefresh);
	virtual void		ScrollImageBy(Int32 inLeftDelta, Int32 inTopDelta,
								Boolean inRefresh);
						//	LCommander features
	virtual Boolean		ObeyCommand(CommandT inCommand, void *ioParam = nil);
	virtual void		FindCommandStatus(CommandT inCommand,
							Boolean &outEnabled, Boolean &outUsesMark,
							Char16 &outMark, Str255 outName);
	virtual Boolean		HandleKeyPress(const EventRecord &inKeyEvent);
	virtual void		BeTarget(void);
	virtual void		DontBeTarget(void);

protected:
	virtual void		ClickSelf(const SMouseDownEvent &inMouseDown);

						//	Implementation help
	virtual void		DrawSelfSelection(void);
	virtual void		MutateSelection(Boolean inAsActive);

				//	Data members
	LTargeter			*mTargeter;
	LSelection			*mSelection;
	StRegion			mOldSelectionRgn;
	Boolean				mOldSelectionInvalid;
	
					//	ugly data members...
	Boolean				mScrollingUpdate;	//	ugly coupling...
	Int32				pTargeter;	//	UGLY communication from stream constructor to
									//	FinishCreateSelf.
};