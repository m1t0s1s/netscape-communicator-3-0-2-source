//	===========================================================================
//	LEventMachine.h					©1994 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LSharable.h>
#include	<LBroadcaster.h>
#include	<LStreamable.h>
#include	<PP_ClassIDs_DM.h>

class	LHandlerView;
class	LManipulator;
class	LSelectableItem;
class	LSelection;
class	LCommander;
class	LAction;
class	LStream;
class	LEventMachine;

//	===========================================================================
//	EVent state machine table constants:

//	States
typedef enum {
				//	Exceptional states:
	evs_Cold	= -3,	//	as in unstarted
	evs_Err		= -2,	//	as in an impossible transition -- will throw

	evs_		= -1,	//	same -- as in no change
	
				//	mouse button is up
	evs_Idle	= 0,	
	evs_Count,			//	click counting
	
				//	mouse button is down:
	evs_Maybe,			//	drag:	might turn into a drag
	evs_Delayed,		//	drag:	delayed release but may still drag
	evs_Drag,			//	drag:	dragging
	evs_BIdle,			//	button tracking:	up and over a button
	evs_BDown,			//	button tracking:	down in button
	evs_BUp,			//	button tracking:	down but outside of button
	
	evs_EventMachine_Count

} EventStateT;

//	Verbs (user actions)
typedef enum {

	evv_None	= -1,
	evv_Idle	= 0,
	evv_Move,
	evv_Down,
	evv_Up,
	evv_KeyDown,
	evv_KeyUp,

	evv_EventMachine_Count	
	//	Note: you can't easily add new verbs -- you'd have to
	//	change the width of the existing tables.

} EventVerbT;

//	Why are conditions, actions, & notification combined into one enum?
//	So segments of code can be used in any of the three scenarios.
typedef enum {

	//	Conditions
	evc_		= 0,	//	none -- conditional always true
	evc_Move,			//	significant move -- greater than multiclick distance
	evc_Tardy,			//	greater the dbl click time since last button transition
	evc_In,				//	inside last mEvtThing
	evc_Out,			//	outside last mEvtThing
	evc_Button,			//	"over a buttonish manipulator"
	evc_NButton,		//	not "over a buttonish manipulator"

	//	Actions
	eva_,				//	none
	eva_Count_Zero,		//	zero click count
	eva_Down,
	eva_Up,
	eva_DStart,
	eva_DMove,
	eva_DStop,
	eva_CBD,			//	check boundary drag
	eva_CC,				//	check cursor
	eva_BSet,
	eva_BClear,

	//	Notifications
	evn_,				//	none
	evn_Down,
	evn_Up,
	evn_Delayed,
	evn_DStart,
	evn_DMove,
	evn_DStop,
	evn_BUp,			//	successful button down and then up

	//	Fallbacks
	evf_,				//	none

	evc_EventMachine_Count	//	start application defined conditions from here.

} EventConditionT;

typedef struct {
	LEventMachine	*handler;
	EventConditionT	note;
}	SEventHandlerNote;

//	===========================================================================
//	LEventMachine:

class	LEventMachine
			:	public virtual LSharable
			,	public LBroadcaster
			,	public LStreamable
{
public:
	enum {class_ID = kPPCID_EventMachine};
	
							LEventMachine();
	virtual					~LEventMachine();
	virtual ClassIDT		GetClassID() const {return class_ID;}
#ifndef	PP_No_ObjectStreaming
	virtual void			WriteObject(LStream &inStream) const;
	virtual void			ReadObject(LStream &inStream);
#endif
	virtual	void			Reset(void);
					
	virtual LHandlerView *	GetHandlerView(void);
	virtual void			SetHandlerView(LHandlerView *inView);
	virtual LCommander *	GetCommander(void) const;	//	Req'd for action posting
	virtual void			SetCommander(LCommander *inCommander);
	virtual void			PostAction(LAction *inAction);

	virtual void			GetMouseBounds(Rect *outBounds);
	virtual void			SetMouseBounds(const Rect &inBounds);
//	virtual void			GetBoundaryRects(Rect *outInside, Rect *outOutside);
	virtual void			SetBoundaryRects(const Rect &inInside, const Rect &inOutside);
	virtual Boolean			GetAllowSingleGestureDrag(void);
	virtual void			SetAllowSingleGestureDrag(Boolean inValue);

					//	Required incoming hooks (must be called)
	virtual Boolean			DoEvent(const EventRecord &inEvent);
	virtual Boolean			DoLiteralEvent(const EventRecord &inEvent);
	virtual void			Activate(void);
	virtual void			Deactivate(void);	

					//	Query
							//	state info
	virtual EventStateT		GetEvtState(void);
	virtual Point &			GetEvtMouse(void);
	virtual EventRecord &	GetEvtRecord(void);
	virtual	Int16			GetClickCount(void);
	virtual Boolean			IsTardy(void) const;
	virtual Boolean			LastMaybeDragDragged(void);

							//	general
	virtual Boolean			IsActive(void);
	virtual Boolean			PointIsClose(Point inOldPt, Point inNewPt);
	virtual	Boolean			PointInHandlerRgn(Point inWhere);
	virtual Boolean			PtInBoundaryArea(Point inWhere);

protected:
					//	Implementation:  table driven state machine
	virtual	void				ProcessVerb(EventVerbT inVerb);
	virtual	EventConditionT		FindCondition(EventStateT inState, EventVerbT inVerb);
	virtual	EventStateT			FindNextState(EventStateT inState, EventVerbT inVerb);
	virtual	EventConditionT		FindAction(EventStateT inState, EventVerbT inVerb);
	virtual	EventConditionT		FindNote(EventStateT inState, EventVerbT inVerb);
	virtual EventConditionT		FindFallback(EventStateT inState, EventVerbT inVerb);
	virtual	Boolean				DoCondition(EventConditionT inCondition);
	virtual void				SetEvtState(EventStateT inState);
	virtual	void				DoAction(EventConditionT inAction);
	virtual	void				DoNote(EventConditionT inNote);
	virtual	void				DoFallback(EventConditionT inNote);
protected:
								//	Verbs
	virtual void				Idle(void);
	virtual void				MouseMove(void);
	virtual void				MouseDown(void);
	virtual void				MouseUp(void);
	virtual void				KeyDown(void);
	virtual void				KeyUp(void);
public:
								//	For changing the tables (rare)
	virtual	EventStateT *		GetStateTable(void);
	virtual void				SetStateTable(EventStateT *inTable = NULL);
	virtual	EventConditionT *	GetConditionTable(void);
	virtual void				SetConditionTable(EventConditionT *inTable = NULL);
	virtual	EventConditionT *	GetActionTable(void);
	virtual	void				SetActionTable(EventConditionT *inTable = NULL);
	virtual	EventConditionT *	GetNoteTable(void);
	virtual	void				SetNoteTable(EventConditionT *inTable = NULL);
	virtual	EventConditionT *	GetFallbackTable(void);
	virtual	void				SetFallbackTable(EventConditionT *inTable = NULL);

					//	To override...
	virtual Boolean			CheckThings(void);
	virtual Boolean			CheckCursor(void);

					//	Implementation
							//	usage context
	virtual void			FocusDraw(void);
	virtual void			Track(void);	//	You don't need this!
protected:
							//	boundaries
	virtual void			CheckBoundaryDrag(void);
	virtual void			OutOfBounds(Point inWhere);
	virtual void			InBounds(Point inWhere);
	virtual void			BoundaryDrag(Point inWhere);
	virtual void			AutoScrollVector(Point inVector);
	virtual void			PreScroll(Point inVector);
	virtual void			PostScroll(Point inVector);
							//	machine
	virtual void			UpdateLastEvt(EventRecord &inEvent, Point inLocalMouse);
	
protected:
				//	Data members
					//	General state information
	EventStateT			mEvtState;
	EventRecord			mEvtRecord;
	Point				mEvtMouse;			//	Mouse position (local coords)
	EventStateT			mLastEvtState;
	EventRecord			mLastEvtRecord;
	Point				mLastEvtMouse;		//	Mouse position (local coords)
	
					//	Button transition state information
	Int16				mEvtClickCount;			//	1st, 2nd, 3rd, etc. click count
	Point				mEvtLastButtonWhere;	//	Compare to this pt to see
												//	 if subsequent pts are close
												//	 to location of button transition.
	long				mEvtLastButtonTime;		//	Time of last mouse button transition.
	
					//	Misc state info
	Boolean				mMaybeDragDragged;
	Boolean				mInBounds;
				
					//	Boundary Drag (BD) state information
	Rect				mBDArea,			//	Interesting boundary drag area (auto
						mBDAreaOutside;		//		ajusted by SetMouseBounds). 
	Int32				mBDDelay;			//	Time between consecutive BD notifications
	Int32				mBDInitialDelay;	//	Time from entry in interesting area
											//		to first BD notification.

	Boolean				mBDHasEntered;		//	Has entered BD area.
	Boolean				mBDFirstTime;		//	First BD notification in a possible series of
											//		notifications.
	Int32				mLastBDTime;		//	Time of last BD notification
	
					//	Linkage data members	
	LCommander			*mCommander;
	LHandlerView		*mHandlerView;
	Rect				mMouseBounds;
	Boolean				mAllowSingleGestureDrag;
	Boolean				mActive;
	Int16				mMultiClickDistance;

					//	instance state tables
	EventStateT			*mNextState;
	EventConditionT		*mConditions;
	EventConditionT		*mActions;
	EventConditionT		*mNotes;
	EventConditionT		*mFallbacks;

				//	Static data members
static	LEventMachine	*sActiveDragger;
						//	default state tables
static	EventStateT		sNextState[];
static	EventConditionT	sConditions[];
static	EventConditionT	sActions[];
static	EventConditionT	sNotes[];
static	EventConditionT	sFallbacks[];
	
};
