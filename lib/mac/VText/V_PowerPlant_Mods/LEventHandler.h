//	===========================================================================
//	LEventHandler.h				©1994-5 Metrowerks Inc. All rights reserved.
//	===========================================================================

#pragma	once

#include	<LEventMachine.h>

//	===========================================================================
//	Types of dragging:

typedef enum {
	dragType_Unknown,
	dragType_Manipulator,	//	Movement of a manipulator or selectable item
	dragType_Selecting,		//	drag selections
	dragType_Data			//	data dragging -- ie the Drag Manager
} DragTypeT;

//	===========================================================================
//	Class:

class	LEventHandler
			:	public LEventMachine
{
public:
	enum {class_ID = kPPCID_EventHandler};

				//	Maintenance
						LEventHandler();
	virtual				~LEventHandler();
	virtual ClassIDT	GetClassID() const {return class_ID;}
#ifndef	PP_No_ObjectStreaming
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
#endif

	virtual	void		Reset(void);
					
	virtual LSelection *	GetSelection(void);
	virtual void		SetSelection(LSelection *inSelection);

	virtual void		NoteOverNewThing(LManipulator *inThing);
	virtual void		NoteOverNewThingSelf(LManipulator *inThing);

	virtual Boolean				OverDifferentThing(
									const Point		inWhere,
									LManipulator	*inOldThing,
									LManipulator	**outNewThing);
	virtual LManipulator *		OverManipulator(Point inWhere);
	virtual LSelection *		OverSelection(Point inWhere);
	virtual LSelectableItem *	OverReceiver(Point inWhere);
	virtual LSelectableItem *	OverItem(Point inWhere);
	virtual LManipulator *		OverManipulatorSelf(Point inWhere);
	virtual LSelection *		OverSelectionSelf(Point inWhere);
	virtual LSelectableItem *	OverReceiverSelf(Point inWhere);
	virtual LSelectableItem *	OverItemSelf(Point inWhere);

protected:
				//	Implementation
	virtual void		SetEvtThing(LManipulator *inThing);
	virtual	DragTypeT	GetDragType(void);
					//	overrides
	virtual Boolean		CheckCursor(void);
	virtual Boolean		CheckThings(void);
	virtual void		PreScroll(Point inVector);
	virtual void		PostScroll(Point inVector);
					//	machine overrides
	virtual	Boolean		DoCondition(EventConditionT inCondition);
	virtual void		MouseDown(void);
	virtual void		MouseUp(void);

				//	Data members
					//	manipulator "things" the mouse is presently over
	LSelectableItem		*mEvtItem;			//	selectable item
	LSelection			*mEvtSelection;		//	selection 
	LManipulator		*mEvtManipulator;	//	manipulator
	LManipulator		*mEvtThing;			//	highest priority of the three
											//	(OverDifferentThing() determines priority)

	LManipulator		*mDragItem;
	LSelection			*mSelection;
	DragTypeT			mDragType;

	Boolean				mEvtClickChangedSelection;	//	Did last down change selection
	Boolean				mSelectionCanMove;

	static	LManipulator	*sLastManDown;
};

enum {
	evs_EventHandler_Count = evs_EventMachine_Count
};


