//	===========================================================================
//	LEventMachine.cp				©1995 Metrowerks Inc. All rights reserved.
//	===========================================================================

#ifdef PowerPlant_PCH
#include PowerPlant_PCH
#endif

#include	"LEventMachine.h"
#include	<UEventUtils.h>
#include	<UCursor.h>
#include	<PP_Messages.h>

#ifndef __ERRORS_
#include <Errors.h>
#endif

//	===========================================================================
/*
	NOTICE	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	
	If you try to understand this code make certain you read the separate 
	documentation. The documentation includes a state diagram for the state
	machine implemented by this event handler.  Knowledge of that state
	diagram makes this code much more understandable.
*/
//	===========================================================================

Boolean	LEventMachine::DoEvent(const EventRecord &inEvent)
/*
	Return value indicates whether event was completely
	handled and caller need take no further action.
	
	Note:
		The following should be appropriately set or
		prepared before this member is called:
		
			QuickDraw port
			pane or view Prepare'd or FocusDraw'ed.
	
	IGNORE return value for the time being.
*/
{
/*
	The complicating factor for an event dispatching routine
	(DoLiteralEvent) is that Mac event fields are only valid
	for given event types.  This routine stuffs the invalid fields
	with the correct information -- this way the event handler has real 
	information in its state instance variables.
	
*/
	EventRecord	theEvent = inEvent;
	
	UEventUtils::FleshOutEvent(
		(mEvtState != evs_Cold) && (mEvtRecord.modifiers & btnState),
		&theEvent);
	
	return DoLiteralEvent(theEvent);
}

Boolean	LEventMachine::DoLiteralEvent(const EventRecord &inEvent)
/*
	DoLiteralEvent does the real work of DoEvent.  DoLiteralEvent
	is provided as an additional entry point in the unlikely event
	you want to send an exact event without the event handler
	filling in the blanks that are normally present.
	
	You will likely never need to call DoLiteralEvent directly.
*/
{
/*
	Another complicating factor for an event dispatching routine
	is that a Mac event can actually be multiple events -- a mouse
	move and a mouse down.
*/
	EventRecord	theEvent = inEvent;
	Boolean		rval = false;
	Point		localMouse;
	Boolean		doMove = false;
	Boolean		doDown = false;
	Boolean		doUp = false;
	
	switch (theEvent.what) {

		case keyDown:
		case keyUp:
		case autoKey:
		case mouseDown:
		case mouseUp:
		case nullEvent:
			break;
		
		default:
			return false;
	}
	
	localMouse = theEvent.where;
	GlobalToLocal(&localMouse);
	
	//	Check startup state info...
	if (mEvtState == evs_Cold) {
		theEvent.modifiers = 0;
		mLastEvtRecord = mEvtRecord = theEvent;
		mEvtMouse = mLastEvtMouse = localMouse;
		mEvtState = mLastEvtState = evs_Idle;
		mEvtClickCount = 0;
		CheckCursor();
		return true;
	}
	
	Try_ {
	
		if (sActiveDragger && (sActiveDragger != this))
			return false;

		
		//	Boundary checking...
		switch (GetEvtState()) {
			case evs_Maybe:
			case evs_Drag:

				//	PowerPlant has an annoying habit of changing the cursor
				//	when over a pane that hasn't overridden AdjustCursorSelf.
				//	This isn't what we want for auto scrolling but...
				//
				//	This technique will at least flash the cursor to what
				//	it ought to be.
				UCursor::Restore();
				break;
			
			default:
				if (!PointInHandlerRgn(localMouse)) {
					if (mInBounds)
						OutOfBounds(localMouse);
				} else {
					if (!mInBounds) {
						InBounds(localMouse);
						doMove = true;
					}
				}
				break;
		}
	
		//	Find out what to do before it's done and the state info
		//	gets messed up.
		if (!EqualPt(localMouse, mEvtMouse))
			doMove = true;
		if ((theEvent.modifiers & btnState) && !(mEvtRecord.modifiers & btnState))
			doDown = true;
		if (!(theEvent.modifiers & btnState) && (mEvtRecord.modifiers & btnState))
			doUp = true;
				
		//	Always update mouse first...
		if (doMove) {
			UpdateLastEvt(theEvent, localMouse);
			MouseMove();
			rval = true;
		}

		//	Now a prioritized list of other event types...
		if (doDown && mInBounds) {							//	note mInBounds check
		
			//	mouseDown
			UpdateLastEvt(theEvent, localMouse);
			MouseDown();
			rval = true;
		
		} else if (doUp) {
	
			//	mouseUp
			UpdateLastEvt(theEvent, localMouse);
			MouseUp();
			rval = true;
		
		} else if (theEvent.what == keyDown) {
	
			//	KeyDown
			UpdateLastEvt(theEvent, localMouse);
			KeyDown();
			rval = true;
		
		} else if (theEvent.what == autoKey) {
	
			//	AutoKey (KeyDown)
			UpdateLastEvt(theEvent, localMouse);
			KeyDown();
			rval = true;
		
		} else if (theEvent.what == keyUp) {
	
			//	KeyUp
			UpdateLastEvt(theEvent, localMouse);
			KeyUp();
			rval = true;
		
		} else if (!doMove) {

			//	Idle
			UpdateLastEvt(theEvent, localMouse);
			Idle();
			rval = true;
		
		}

	} Catch_(err) {
		//	Make sure exceptions leave handler in a safe state
		Reset();
		Throw_(err);
	} EndCatch_;

	return rval;
}

void	LEventMachine::Track(void)
/*
	Track mouse action until the mouse button is up.
	This routine only returns when the mouse button is up.
	
	THIS METHOD IS NEEDED EXTREMELY RARELY...
	
	Avoid the traditional urge to code "track" type operations and
	just lest the event handler state machine take care of
	everything.  Letting the event handler state machine process
	events allows better "cooperative" multitasking.  You also wont
	have to worry if your doing the "right thing" immediately
	before and after your tracking code.
	
	But, for the few cases where a track operation is needed
	and for the paranoid... 
*/
{
	EventRecord		event;

	FocusDraw();

	while (true) {

		//	Stuff an event.
		GetMouse(&event.where);
		LocalToGlobal(&event.where);
		event.when = TickCount();
		event.modifiers = (Button()) ? btnState : 0;
		if (event.modifiers & btnState)
			event.what = nullEvent;	//	MouseMoved
		else
			event.what = mouseUp;
	
		//	The event isn't stuffed exactly correctly right now
		//	but DoEvent will fill in the rest...
		DoEvent(event);
		
		if ((event.modifiers & btnState) == 0)
			break;
		
		SystemTask();	//	Be cooperative		
	}
}

void	LEventMachine::Reset(void)
/*
	Subclasses may find it necessary to override and inherit this
	method.
	
	This method is called when an exception occurs during DoEvent
	and allows the handler to "reset" itself to a known or usable
	state.
	
	Code executed during Reset must not throw another exception
	UNLESS it is caught and handled by subclass::Reset.
*/
{
	mEvtState = evs_Cold;
	mMaybeDragDragged = false;
	mBDHasEntered = false;
	UCursor::Reset();
	
	if (sActiveDragger == this)
		sActiveDragger = NULL;
}

//	===========================================================================
//	State machine tables
#pragma mark --- State machine tables

EventStateT	LEventMachine::sNextState[] = {
//	evv_Idle		evv_Move		evv_Down		evv_Up			evv_KeyDown		evv_KeyUp
	evs_BIdle,		evs_BIdle,		evs_Maybe,		evs_Err,		evs_,			evs_,			//	evs_Idle
	evs_Idle,		evs_Idle,		evs_Maybe,		evs_Err,		evs_,			evs_,			//	evs_Count
	evs_Delayed,	evs_Drag,		evs_Err,		evs_Count,		evs_,			evs_,			//	evs_Maybe
	evs_,			evs_Drag,		evs_Err,		evs_Count,		evs_,			evs_,			//	evs_Delayed
	evs_,			evs_,			evs_Err,		evs_Idle,		evs_,			evs_,			//	evs_Drag
	evs_Idle,		evs_Idle,		evs_BDown,		evs_Err,		evs_,			evs_,			//	evs_BIdle
	evs_,			evs_BUp,		evs_Err,		evs_Idle,		evs_,			evs_,			//	evs_BDown
	evs_,			evs_BDown,		evs_Err,		evs_Idle,		evs_,			evs_			//	evs_BUp
};

EventConditionT LEventMachine::sConditions[] = {
//	evv_Idle		evv_Move		evv_Down		evv_Up			evv_KeyDown		evv_KeyUp
	evc_Button,		evc_Button,		evc_,			evc_,			evc_,			evc_,			//	evs_Idle
	evc_Tardy,		evc_Move,		evc_,			evc_,			evc_,			evc_,			//	evs_Count
	evc_Tardy,		evc_Move,		evc_,			evc_,			evc_,			evc_,			//	evs_Maybe
	evc_,			evc_Move,		evc_,			evc_,			evc_,			evc_,			//	evs_Delayed
	evc_,			evc_,			evc_,			evc_,			evc_,			evc_,			//	evs_Drag
	evc_NButton,	evc_NButton,	evc_,			evc_,			evc_,			evc_,			//	evs_BIdle
	evc_,			evc_Out,		evc_,			evc_,			evc_,			evc_,			//	evs_BDown
	evc_,			evc_In,			evc_,			evc_,			evc_,			evc_			//	evs_BUp
};

EventConditionT	LEventMachine::sActions[] = {
//	evv_Idle		evv_Move		evv_Down		evv_Up			evv_KeyDown		evv_KeyUp
	eva_CC,			eva_CC,			eva_Down,		eva_,			eva_,			eva_,			//	evs_Idle
	eva_Count_Zero,	eva_Count_Zero,	eva_Down,		eva_,			eva_,			eva_,			//	evs_Count
	eva_,			eva_DStart,		eva_,			eva_Up,			eva_,			eva_,			//	evs_Maybe
	eva_,			eva_DStart,		eva_,			eva_Up,			eva_,			eva_,			//	evs_Delayed
	eva_CBD,		eva_DMove,		eva_,			eva_DStop,		eva_,			eva_,			//	evs_Drag
	eva_CC,			eva_CC,			eva_BSet,		eva_,			eva_,			eva_,			//	evs_BIdle
	eva_,			eva_,			eva_,			eva_BClear,		eva_,			eva_,			//	evs_BDown
	eva_,			eva_,			eva_,			eva_BClear,		eva_,			eva_			//	evs_BUp
};

EventConditionT	LEventMachine::sNotes[] = {
//	evv_Idle		evv_Move		evv_Down		evv_Up			evv_KeyDown		evv_KeyUp
	evn_,			evn_,			evn_Down,		evn_,			evn_,			evn_,			//	evs_Idle
	evn_,			evn_,			evn_Down,		evn_,			evn_,			evn_,			//	evs_Count
	evn_Delayed,	evn_DStart,		evn_,			evn_Up,			evn_,			evn_,			//	evs_Maybe
	evn_,			evn_DStart,		evn_,			evn_Up,			evn_,			evn_,			//	evs_Delayed
	evn_,			evn_,			evn_,			evn_DStop,		evn_,			evn_,			//	evs_Drag
	evn_,			evn_,			evn_Down,		evn_,			evn_,			evn_,			//	evs_BIdle
	evn_,			evn_,			evn_,			evn_BUp,		evn_,			evn_,			//	evs_BDown
	evn_,			evn_,			evn_,			evn_,			evn_,			evn_			//	evs_BUp
};

EventConditionT	LEventMachine::sFallbacks[] = {
//	evv_Idle		evv_Move		evv_Down		evv_Up			evv_KeyDown		evv_KeyUp
	evf_,			evf_,			evf_,			evf_,			evf_,			evf_,			//	evs_Idle
	evf_,			evf_,			evf_,			evf_,			evf_,			evf_,			//	evs_Count
	evf_,			evf_,			evf_,			evf_,			evf_,			evf_,			//	evs_Maybe
	evf_,			evf_,			evf_,			evf_,			evf_,			evf_,			//	evs_Delayed
	evf_,			evf_,			evf_,			evf_,			evf_,			evf_,			//	evs_Drag
	evf_,			evf_,			evf_,			evf_,			evf_,			evf_,			//	evs_BIdle
	evf_,			evf_,			evf_,			evf_,			evf_,			evf_,			//	evs_BDown
	evf_,			evf_,			evf_,			evf_,			evf_,			evf_			//	evs_BUp
};

//	===========================================================================
//	Table driven state machine

void	LEventMachine::ProcessVerb(EventVerbT inVerb)
{
	EventStateT			oldState = GetEvtState(),
						newState = oldState;
	EventConditionT		condition = FindCondition(oldState, inVerb);
	EventConditionT		action;
	EventConditionT		note;
	EventConditionT		fallback;
	
	if (DoCondition(condition)) {

		newState = FindNextState(oldState, inVerb);
		SetEvtState(newState);

		action = FindAction(oldState, inVerb);
		DoAction(action);

		note = FindNote(oldState, inVerb);
		DoNote(note);

	}
	
	if (newState == oldState) {
	
		fallback = FindFallback(oldState, inVerb);
		DoFallback(fallback);
	
	}
		
}


Boolean	LEventMachine::DoCondition(EventConditionT inCondition)
{
	Boolean	rval = false;
	
	switch (inCondition) {

		//	Conditions
		case evc_:
			rval = true;
			break;

		case evc_Move:
			if (!PointIsClose(mEvtMouse, mEvtLastButtonWhere) || (!mInBounds))
				rval = true;
			break;
		
		case evc_Tardy:
			if (IsTardy())
				rval = true;
			break;
			
		
		//	Actions
		case eva_Count_Zero:
			mEvtClickCount = 0;
			CheckThings();
			CheckCursor();	//	note absent if
			break;
			
		case eva_Down:
			mEvtClickCount++;
			mMaybeDragDragged = false;
			CheckCursor();
			break;
		
		case eva_CC:
			if (CheckThings())
				CheckCursor();
			break;
		
		case eva_CBD:
			CheckBoundaryDrag();
			break;
			
		default:
			break;
	}
	
	return rval;
}

void	LEventMachine::DoAction(EventConditionT inAction)
{
	DoCondition(inAction);
}

void	LEventMachine::DoNote(EventConditionT inNote)
{
	switch (inNote) {
		case evn_:
			//	don't broadcast anything!
			break;
		
		default:
		{
			SEventHandlerNote	rec;
			
			rec.handler = this;
			rec.note = inNote;
			
			BroadcastMessage(msg_EventHandlerNote, &rec);
			break;
		}
	};
}

void	LEventMachine::DoFallback(EventConditionT inFallback)
{
	DoCondition(inFallback);
}

EventStateT	LEventMachine::GetEvtState(void)
{
	return mEvtState;
}

void	LEventMachine::SetEvtState(EventStateT inState)
{
	switch (inState) {

		case evs_Err:
			//	Impossible state transition.
			//
			//	Were you tracking a mouse down when the
			//	debugger switched in?  Then, when you
			//	continued in this app, was your mouse up?
			//	If so, that's why the state machine got
			//	confused.
			Throw_(paramErr);
			break;

		case evs_:
			//	no change -- leave as is
			break;
		
		default:
			if (inState < 0) {
				//	Once started, you can't set the state to an exceptional state.
				Throw_(paramErr);
			}
			mEvtState = inState;
			break;
	}
}

//	===========================================================================
//	Verbs

void	LEventMachine::Idle(void)
{
	ProcessVerb(evv_Idle);
}

void	LEventMachine::MouseMove(void)
{
	ProcessVerb(evv_Move);
}

void	LEventMachine::MouseDown(void)
{
	Assert_(sActiveDragger == NULL);
	sActiveDragger = this;
	
	ProcessVerb(evv_Down);
	
	mEvtLastButtonWhere = mEvtMouse;	
	mEvtLastButtonTime = mEvtRecord.when;
}

void	LEventMachine::MouseUp(void)
{
	sActiveDragger = NULL;
	
	ProcessVerb(evv_Up);
		
	mEvtLastButtonWhere = mEvtMouse;	
	mEvtLastButtonTime = mEvtRecord.when;
}

void	LEventMachine::KeyDown(void)
{
	ProcessVerb(evv_KeyDown);
}

void	LEventMachine::KeyUp(void)
{
	ProcessVerb(evv_KeyUp);
}

