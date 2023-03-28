//	===========================================================================
//	LSelectHandlerView.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma once

#include	<LView.h>
#include	<LPeriodical.h>
#include	<PP_ClassIDs_DM.h>

class	LEventHandler;
class	LSelectableItem;
class	LSelection;
class	LManipulator;
class	LTargeter;
class	LGWorld;

#ifdef	EXP_OFFSCREEN
	#include	<LOffscreenView.h>

	class	LHandlerView
				:	public LOffscreenView
				,	public LPeriodical
	{
	private:
		typedef	LOffscreenView	inheritView;
#else
	class	LHandlerView
				:	public LView
				,	public LPeriodical
	{
	private:
		typedef	LView	inheritView;
#endif

public:
	enum { class_ID = kPPCID_HandlerView };

				//	Maintenance
						LHandlerView();
						LHandlerView(const LView &inOriginal);
						LHandlerView(LStream *inStream);
						LHandlerView(LStream *inStream, Boolean inOldStyle);
	virtual				~LHandlerView();
	
				//	Called by mEventHandler (override)
	virtual void				NoteOverNewThing(LManipulator *inThing);
	virtual	LManipulator *		OverManipulator(Point inWhere);
	virtual	LSelectableItem *	OverItem(Point inWhere);
	virtual	LSelectableItem *	OverReceiver(Point inWhere);
	virtual	LSelection *		OverSelection(Point inWhere);

				//	New features
	virtual void		SetScrollPosition(const SPoint32 &inLocation);

				//	Query
	virtual LEventHandler *	GetEventHandler(void);
	virtual void			SetEventHandler(LEventHandler *inEventHandler);
	
				//	Implementation & linkage to mEventHandler
	virtual	Boolean		FocusDraw(void);
	virtual void		ResizeFrameBy(Int16 inWidthDelta, Int16 inHeightDelta,
									Boolean inRefresh);
	virtual void		ScrollImageBy(Int32 inLeftDelta, Int32 inTopDelta,
									Boolean inRefresh);
	virtual void		EventMouseUp(const EventRecord &inMacEvent);
	virtual	void		SpendTime(const EventRecord &inMacEvent);
	virtual void		FinishCreateSelf(void);
	virtual void		AdjustCursorSelf(Point inPortPt, const EventRecord &inMacEvent);

protected:
	virtual void		ClickSelf(const SMouseDownEvent &inMouseDown);
	virtual void		ActivateSelf(void);
	virtual void		DeactivateSelf(void);

						//	Implementation help
	virtual void		FixHandlerFrame(void);
	
				//	Data members
				//		"Owned" by deriving class
	LEventHandler		*mEventHandler;
};